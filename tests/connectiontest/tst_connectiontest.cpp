#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <QtCore/QCoreApplication>
#include <QtNetwork/QLocalSocket>
#include <QtTest/QTest>
#include <QtTest/QSignalSpy>
#include "connection.h"
#include "request.h"

class ConnectionTest : public QObject {
	Q_OBJECT
public:
	ConnectionTest()
		: m_sr(nullptr), m_sw(nullptr)
	{
	}

private:
	QLocalSocket* m_sr;
	QLocalSocket* m_sw;
	FastCGI::LowLevel::Connection* m_conn;

private Q_SLOTS:
	void init()
	{
		int sv[2];
		bool f;
		int res = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
		QVERIFY(res != -1);

		this->m_sr = new QLocalSocket(this);
		this->m_sw = new QLocalSocket(this);

		f = this->m_sr->setSocketDescriptor(sv[0]);
		QCOMPARE(f, true);

		f = this->m_sw->setSocketDescriptor(sv[1]);
		QCOMPARE(f, true);

		this->m_conn = new FastCGI::LowLevel::Connection(this->m_sr);
	}

	void cleanup()
	{
		delete this->m_conn;
		delete this->m_sr;
		delete this->m_sw;
		this->m_sr   = nullptr;
		this->m_sw   = nullptr;
		this->m_conn = nullptr;
	}

	void test1()
	{
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
		qint64 nw = this->m_sw->write(buf);
		QCOMPARE(nw, buf.size());
		this->m_sw->flush();
		QCoreApplication::processEvents();

		QCOMPARE(spy_conn.count(), 1);

		QList<QVariant> args = spy_conn.takeFirst();
		QCOMPARE(args[0].canConvert<FastCGI::LowLevel::Request*>(), true);
		FastCGI::LowLevel::Request* request = args[0].value<FastCGI::LowLevel::Request*>();
		QVERIFY(request != nullptr);

		QSignalSpy spy_pp(request, SIGNAL(parametersParsed(const QList<QPair<QByteArray, QByteArray> >&)));
		QCoreApplication::processEvents();;
		QCOMPARE(spy_pp.count(), 1);
	}
};

QTEST_GUILESS_MAIN(ConnectionTest)

#include "tst_connectiontest.moc"
