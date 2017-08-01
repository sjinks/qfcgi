#include <QtCore/QCoreApplication>
#include <QtNetwork/QLocalServer>
#include <QtNetwork/QLocalSocket>
#include <QtTest/QTest>
#include <QtTest/QSignalSpy>
#include "connection.h"
#include "request.h"
#include "outputstream.h"
#include "protocol/beginrequest_p.h"

class ConnectionTest : public QObject {
	Q_OBJECT
public:
	using QByteArrayPair = QPair<QByteArray, QByteArray>;

	ConnectionTest()
		: m_srv(new QLocalServer()), m_client(nullptr), m_server(nullptr)
	{
	}

private:
	QScopedPointer<QLocalServer> m_srv;
	QPointer<QLocalSocket> m_client;
	QPointer<QLocalSocket> m_server;
	QPointer<FastCGI::LowLevel::Connection> m_conn;

private Q_SLOTS:
	void init()
	{
		const QLatin1String server_name("fcgi");

		bool f;
		QLocalServer::removeServer(server_name);
		f = this->m_srv->listen(server_name);
		QCOMPARE(f, true);

		this->m_client = new QLocalSocket(this);
		this->m_client->connectToServer(server_name);

		QCoreApplication::processEvents();

		QCOMPARE(this->m_srv->hasPendingConnections(), true);
		this->m_server = this->m_srv->nextPendingConnection();

		QVERIFY(this->m_server != nullptr);
		this->m_conn = new FastCGI::LowLevel::Connection(this->m_server);
	}

	void cleanup()
	{
		this->m_srv->close();
		delete this->m_conn;

		QVERIFY(this->m_server.isNull() == true);
		delete this->m_client;
	}

	void testRealRequest()
	{
		static QList<QByteArrayPair> params_expected = {
			{ "QUERY_STRING",      ""                                  },
			{ "REQUEST_METHOD",    "GET"                               },
			{ "CONTENT_TYPE",      ""                                  },
			{ "CONTENT_LENGTH",    ""                                  },
			{ "SCRIPT_NAME",       "/"                                 },
			{ "REQUEST_URI",       "/"                                 },
			{ "DOCUMENT_URI",      "/"                                 },
			{ "DOCUMENT_ROOT",     "/home/xxxxxxxx/test.dev"           },
			{ "SERVER_PROTOCOL",   "HTTP/1.0"                          },
			{ "REQUEST_SCHEME",    "http"                              },
			{ "GATEWAY_INTERFACE", "CGI/1.1"                           },
			{ "SERVER_SOFTWARE",   "nginx/1.10.3"                      },
			{ "REMOTE_ADDR",       "127.0.0.1"                         },
			{ "REMOTE_PORT",       "45664"                             },
			{ "SERVER_ADDR",       "127.0.0.1"                         },
			{ "SERVER_PORT",       "8080"                              },
			{ "SERVER_NAME",       "test.dev"                          },
			{ "REDIRECT_STATUS",   "200"                               },
			{ "SCRIPT_FILENAME",   "/home/xxxxxxxx/test.dev/index.php" },
			{ "HTTP_HOST",         "127.0.0.1:8080"                    },
			{ "HTTP_USER_AGENT",   "ApacheBench/2.3"                   },
			{ "HTTP_ACCEPT",       "*/*"                               }
		};

		QByteArray buf = QByteArray::fromHex(
			"010100010008000000010100000000000104000101e701000c00515545"
			"52595f535452494e470e03524551554553545f4d4554484f444745540c"
			"00434f4e54454e545f545950450e00434f4e54454e545f4c454e475448"
			"0b015343524950545f4e414d452f0b01524551554553545f5552492f0c"
			"01444f43554d454e545f5552492f0d17444f43554d454e545f524f4f54"
			"2f686f6d652f78787878787878782f746573742e6465760f0853455256"
			"45525f50524f544f434f4c485454502f312e300e04524551554553545f"
			"534348454d45687474701107474154455741595f494e54455246414345"
			"4347492f312e310f0c5345525645525f534f4654574152456e67696e78"
			"2f312e31302e330b0952454d4f54455f414444523132372e302e302e31"
			"0b0552454d4f54455f504f525434353636340b095345525645525f4144"
			"44523132372e302e302e310b045345525645525f504f5254383038300b"
			"085345525645525f4e414d45746573742e6465760f0352454449524543"
			"545f5354415455533230300f215343524950545f46494c454e414d452f"
			"686f6d652f78787878787878782f746573742e6465762f696e6465782e"
			"706870090e485454505f484f53543132372e302e302e313a383038300f"
			"0f485454505f555345525f4147454e5441706163686542656e63682f32"
			"2e330b03485454505f4143434550542a2f2a0001040001000000000105"
			"000100000000"
		);

		QSignalSpy spy_conn(this->m_conn, SIGNAL(newRequest(FastCGI::LowLevel::Request*)));
		qint64 nw = this->m_client->write(buf);
		QCOMPARE(nw, buf.size());
		this->m_client->flush();
		QCoreApplication::processEvents();

		QCOMPARE(spy_conn.count(), 1);

		QList<QVariant> args = spy_conn.takeFirst();
		QCOMPARE(args[0].canConvert<FastCGI::LowLevel::Request*>(), true);
		FastCGI::LowLevel::Request* request = args[0].value<FastCGI::LowLevel::Request*>();
		QVERIFY(request != nullptr);

		QSignalSpy spy_pp(request, SIGNAL(parametersParsed(const QList<QPair<QByteArray, QByteArray> >&)));
		QSignalSpy spy_sdr(request, SIGNAL(stdinDataReady(const QByteArray&)));
		QSignalSpy spy_sr(request, SIGNAL(stdinRead()));
		QCoreApplication::processEvents();
		QCOMPARE(spy_pp.count(), 1);
		QCOMPARE(spy_sdr.count(), 0);
		QCOMPARE(spy_sr.count(), 1);

		args = spy_pp.takeFirst();
		QCOMPARE(args[0].canConvert<QList<QByteArrayPair> >(), true);
		QList<QByteArrayPair> params_actual = args[0].value<QList<QPair<QByteArray, QByteArray> > >();
		QCOMPARE(params_actual, params_expected);

		QVERIFY(request->stdOut() != nullptr);
		request->stdOut()->write("Status: 200 OK\r\n\r\nHello!\r\n");
		request->stdOut()->flush();

		QTRY_COMPARE(this->m_client->waitForReadyRead(100) || this->m_client->bytesAvailable(), true);
		QByteArray response_actual   = this->m_client->readAll();
		QByteArray response_expected = QByteArray::fromHex("01060001001a00005374617475733a20323030204f4b0d0a0d0a48656c6c6f210d0a");
		QCOMPARE(response_actual, response_expected);

		QSignalSpy spy_od(request, SIGNAL(destroyed()));
		request->finish(FastCGI::LowLevel::Complete, 0);
		QTRY_COMPARE(this->m_client->waitForReadyRead(100) || this->m_client->bytesAvailable(), true);

		response_actual   = this->m_client->readAll();
		response_expected = QByteArray::fromHex("010600010000000001030001000800000000000000000000");
		QCOMPARE(response_actual, response_expected);

		QTest::qWait(0);
		QCOMPARE(spy_od.count(), 1);
	}

	void testUnknownManagementRecord()
	{
		QByteArray brr = QByteArray::fromHex("01010001000800000001010000000000");
		QByteArray umr = QByteArray::fromHex("01ff000000000000");

		QSignalSpy spy_conn(this->m_conn, SIGNAL(newRequest(FastCGI::LowLevel::Request*)));
		qint64 nw = this->m_client->write(brr);
		QCOMPARE(nw, brr.size());
		this->m_client->flush();
		QCoreApplication::processEvents();

		QCOMPARE(spy_conn.count(), 1);

		QList<QVariant> args = spy_conn.takeFirst();
		QCOMPARE(args[0].canConvert<FastCGI::LowLevel::Request*>(), true);
		FastCGI::LowLevel::Request* request = args[0].value<FastCGI::LowLevel::Request*>();
		QVERIFY(request != nullptr);

		nw = this->m_client->write(umr);
		QCOMPARE(nw, umr.size());
		this->m_client->flush();

		QTRY_COMPARE(this->m_client->waitForReadyRead(100) || this->m_client->bytesAvailable(), true);
		QByteArray response_actual   = this->m_client->readAll();
		QByteArray response_expected = QByteArray::fromHex("010b000000080000ff00000000000000");
		QCOMPARE(response_actual, response_expected);

		QSignalSpy spy_cd(this->m_conn, SIGNAL(disconnected()));
		this->m_client->close();
		QTest::qWait(100);
		QCOMPARE(spy_cd.count(), 1);
	}

	void testGetValues()
	{
		QByteArray brr = QByteArray::fromHex("01010001000800000001010000000000");
		QByteArray gvr = QByteArray::fromHex("0109000000000000");

		QSignalSpy spy_conn(this->m_conn, SIGNAL(newRequest(FastCGI::LowLevel::Request*)));
		qint64 nw = this->m_client->write(brr);
		QCOMPARE(nw, brr.size());
		this->m_client->flush();
		QCoreApplication::processEvents();

		QCOMPARE(spy_conn.count(), 1);

		QList<QVariant> args = spy_conn.takeFirst();
		QCOMPARE(args[0].canConvert<FastCGI::LowLevel::Request*>(), true);
		FastCGI::LowLevel::Request* request = args[0].value<FastCGI::LowLevel::Request*>();
		QVERIFY(request != nullptr);

		nw = this->m_client->write(gvr);
		QCOMPARE(nw, gvr.size());
		this->m_client->flush();

		QTRY_COMPARE(this->m_client->waitForReadyRead(100) || this->m_client->bytesAvailable(), true);
		QByteArray response_actual   = this->m_client->readAll();
		QByteArray response_expected = QByteArray::fromHex("010a000000000000");
		QCOMPARE(response_actual, response_expected);

		QSignalSpy spy_cd(this->m_conn, SIGNAL(disconnected()));
		request->finish(FastCGI::LowLevel::Complete, 0);
		QTRY_COMPARE(this->m_client->waitForReadyRead(100) || this->m_client->bytesAvailable(), true);

		response_actual   = this->m_client->readAll();
		response_expected = QByteArray::fromHex("01030001000800000000000000000000");
		QCOMPARE(response_actual, response_expected);

		QCOMPARE(spy_cd.count(), 0);
	}

	void testInvalidRole()
	{
		QByteArray brr = QByteArray::fromHex("01010001000800000004010000000000");

		QSignalSpy spy_conn(this->m_conn, SIGNAL(newRequest(FastCGI::LowLevel::Request*)));
		QSignalSpy spy_pe(this->m_conn, SIGNAL(protocolError()));
		qint64 nw = this->m_client->write(brr);
		QCOMPARE(nw, brr.size());
		this->m_client->flush();
		QCoreApplication::processEvents();

		QCOMPARE(spy_conn.count(), 0);
		QCOMPARE(spy_pe.count(), 1);
	}

	void testDisconnnect()
	{
		QSignalSpy spy_cd(this->m_conn, SIGNAL(disconnected()));
		this->m_client->close();
		QTest::qWait(100);
		QCOMPARE(spy_cd.count(), 1);
	}

	void testBadProtocol()
	{
		QByteArray header = QByteArray::fromHex("FF010001000800000");
		QSignalSpy spy_pe(this->m_conn, SIGNAL(protocolError()));
		QSignalSpy spy_cd(this->m_conn, SIGNAL(disconnected()));

		qint64 nw = this->m_client->write(header);
		QCOMPARE(nw, header.size());

		QTest::qWait(100);

		QCOMPARE(spy_pe.count(), 1);
		QCOMPARE(spy_cd.count(), 0);
	}
};

QTEST_GUILESS_MAIN(ConnectionTest)

#include "tst_connectiontest.moc"
