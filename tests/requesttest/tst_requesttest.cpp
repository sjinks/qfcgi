#include <QtCore/QBuffer>
#include <QtCore/QSharedPointer>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>
#include "outputstream.h"
#include "request.h"
#include "request_p.h"
#include "protocol/beginrequest_p.h"

class Q_DECL_HIDDEN RequestTest : public QObject {
	Q_OBJECT
public:
	using QByteArrayPair = QPair<QByteArray, QByteArray>;

	RequestTest()
		: m_buf(new QBuffer())
	{
	}

private:
	QSharedPointer<QBuffer> m_buf;
	QPointer<FastCGI::LowLevel::Request> m_req;

private Q_SLOTS:
	void init()
	{
		FastCGI::Protocol::BeginRequest r(FCGI_RESPONDER, 0);

		if (this->m_buf->isOpen()) {
			this->m_buf->close();
		}

		this->m_buf->setBuffer(nullptr);
		this->m_buf->open(QIODevice::ReadWrite);
		this->m_req = new FastCGI::LowLevel::Request(2, r, this->m_buf);
	}

	void cleanup()
	{
		if (!this->m_req.isNull()) {
			QSignalSpy spy(this->m_req, SIGNAL(requestFinished(quint16)));
			delete this->m_req;
			QCOMPARE(spy.count(), 0);
			QCOMPARE(this->m_req.isNull(), true);
		}
		else {
			qWarning("!!!");
		}

		this->m_buf->close();
	}

	void testGetters()
	{
		QCOMPARE(this->m_req->id(),             quint16(2));
		QCOMPARE(this->m_req->role(),           FastCGI::LowLevel::Responder);
		QCOMPARE(this->m_req->keepConnection(), false);

		QVERIFY(this->m_req->stdOut() == nullptr);
		QVERIFY(this->m_req->stdErr() == nullptr);
	}

	void testAppendParamsOneChunk_data()
	{
		QTest::addColumn<QByteArray>("input");
		QTest::addColumn<QList<QPair<QByteArray, QByteArray> > >("res_expected");

		QTest::newRow("std")
			<< QByteArray("\013\002SERVER_PORT80\013\016SERVER_ADDR199.170.183.42")
			<< (
					QList<QPair<QByteArray, QByteArray> >()
						<< qMakePair(QByteArray("SERVER_PORT"), QByteArray("80"))
						<< qMakePair(QByteArray("SERVER_ADDR"), QByteArray("199.170.183.42"))
			   )
		;

		QTest::newRow("largesize")
			<< QByteArray("\x80\x00\x00\x0B\x02SERVER_PORT80\x0B\x80\x00\x00\x0ESERVER_ADDR199.170.183.42", sizeof("\x80\x00\x00\x0B\x02SERVER_PORT80\x0B\x80\x00\x00\x0ESERVER_ADDR199.170.183.42")-1)
			<< (
					QList<QPair<QByteArray, QByteArray> >()
						<< qMakePair(QByteArray("SERVER_PORT"), QByteArray("80"))
						<< qMakePair(QByteArray("SERVER_ADDR"), QByteArray("199.170.183.42"))
			   )
		;
	}

	void testAppendParamsOneChunk()
	{
		bool f;
		FastCGI::LowLevel::RequestPrivate* d = this->m_req->d_func();

		QFETCH(QByteArray, input);
		QFETCH(QList<QByteArrayPair>, res_expected);

		QSignalSpy spy(this->m_req, SIGNAL(parametersParsed(const QList<QPair<QByteArray, QByteArray> >&)));
		QSignalSpy spy_rf(this->m_req, SIGNAL(requestFinished(quint16)));

		QVERIFY(this->m_req->stdOut() == nullptr);
		QVERIFY(this->m_req->stdErr() == nullptr);
		f = d->_q_processRecord(FCGI_PARAMS, input);
		QCOMPARE(f, true);

		QVERIFY(this->m_req->stdOut() == nullptr);
		QVERIFY(this->m_req->stdErr() == nullptr);
		f = d->_q_processRecord(FCGI_PARAMS, QByteArray());
		QCOMPARE(f, true);

		QCOMPARE(spy.count(), 1);
		QList<QVariant> args = spy.takeFirst();

		QCOMPARE(args[0].canConvert<QList<QByteArrayPair> >(), true);
		QList<QByteArrayPair> res_actual = args[0].value<QList<QPair<QByteArray, QByteArray> > >();

		QCOMPARE(res_actual, res_expected);
		QVERIFY(this->m_req->stdOut() != nullptr);
		QVERIFY(this->m_req->stdErr() != nullptr);
		QCOMPARE(this->m_req->stdOut()->isBuffered(), true);
		QCOMPARE(this->m_req->stdErr()->isBuffered(), false);
		QCOMPARE(this->m_req->stdOut()->type(), quint8(FCGI_STDOUT));
		QCOMPARE(this->m_req->stdErr()->type(), quint8(FCGI_STDERR));
		QCOMPARE(this->m_req->stdOut()->requestId(), this->m_req->id());
		QCOMPARE(this->m_req->stdErr()->requestId(), this->m_req->id());

		QCOMPARE(spy_rf.count(), 0);
	}

	void testAppendParamsSeveralChunks_data()
	{
		this->testAppendParamsOneChunk_data();
	}

	void testAppendParamsSeveralChunks()
	{
		FastCGI::LowLevel::RequestPrivate* d = this->m_req->d_func();

		QFETCH(QByteArray, input);
		QFETCH(QList<QByteArrayPair>, res_expected);

		QSignalSpy spy(this->m_req, SIGNAL(parametersParsed(const QList<QPair<QByteArray, QByteArray> >&)));
		QSignalSpy spy_rf(this->m_req, SIGNAL(requestFinished(quint16)));

		int chunk_size = input.size() / 3;
		int offset     = 0;
		bool f;

		while (offset < input.size()) {
			QVERIFY(this->m_req->stdOut() == nullptr);
			QVERIFY(this->m_req->stdErr() == nullptr);
			f = d->_q_processRecord(FCGI_PARAMS, input.mid(offset, chunk_size));
			QCOMPARE(f, true);
			offset += chunk_size;
		}

		QVERIFY(this->m_req->stdOut() == nullptr);
		QVERIFY(this->m_req->stdErr() == nullptr);
		f = d->_q_processRecord(FCGI_PARAMS, QByteArray());
		QCOMPARE(f, true);

		QCOMPARE(spy.count(), 1);
		QList<QVariant> args = spy.takeFirst();

		QCOMPARE(args[0].canConvert<QList<QByteArrayPair> >(), true);
		QList<QByteArrayPair> res_actual = args[0].value<QList<QPair<QByteArray, QByteArray> > >();

		QCOMPARE(res_actual, res_expected);
		QVERIFY(this->m_req->stdOut() != nullptr);
		QVERIFY(this->m_req->stdErr() != nullptr);
		QCOMPARE(this->m_req->stdOut()->isBuffered(), true);
		QCOMPARE(this->m_req->stdErr()->isBuffered(), false);
		QCOMPARE(this->m_req->stdOut()->type(), quint8(FCGI_STDOUT));
		QCOMPARE(this->m_req->stdErr()->type(), quint8(FCGI_STDERR));
		QCOMPARE(this->m_req->stdOut()->requestId(), this->m_req->id());
		QCOMPARE(this->m_req->stdErr()->requestId(), this->m_req->id());

		QCOMPARE(spy_rf.count(), 0);
	}

	void testAppendBadParams_data()
	{
		QTest::addColumn<QByteArray>("input");
		QTest::addColumn<QList<QPair<QByteArray, QByteArray> > >("res_expected");
		QTest::newRow("bad1")
			<< QByteArray("\x88\x00\x00\x0B\x02SERVER_PORT80\x0B\x80\x00\x00\x0ESERVER_ADDR199.170.183.42", sizeof("\x88\x00\x00\x0B\x02SERVER_PORT80\x0B\x80\x00\x00\x0ESERVER_ADDR199.170.183.42")-1)
			<< (
					QList<QPair<QByteArray, QByteArray> >()
			   )
		;

		QTest::newRow("bad2")
			<< QByteArray("\x80\x00\x00\x0B\x02SERVER_PORT80\x0B\x80\x00\x00\x0FSERVER_ADDR199.170.183.42", sizeof("\x80\x00\x00\x0B\x02SERVER_PORT80\x0B\x80\x00\x00\x0FSERVER_ADDR199.170.183.42")-1)
			<< (
					QList<QPair<QByteArray, QByteArray> >()
						<< qMakePair(QByteArray("SERVER_PORT"), QByteArray("80"))
			   )
		;
	}

	void testAppendBadParams()
	{
		bool f;
		FastCGI::LowLevel::RequestPrivate* d = this->m_req->d_func();

		QFETCH(QByteArray, input);
		QFETCH(QList<QByteArrayPair>, res_expected);

		QSignalSpy spy(this->m_req, SIGNAL(parametersParsed(const QList<QPair<QByteArray, QByteArray> >&)));
		QSignalSpy spy_rf(this->m_req, SIGNAL(requestFinished(quint16)));

		QVERIFY(this->m_req->stdOut() == nullptr);
		QVERIFY(this->m_req->stdErr() == nullptr);
		f = d->_q_processRecord(FCGI_PARAMS, input);
		QCOMPARE(f, true);

		QVERIFY(this->m_req->stdOut() == nullptr);
		QVERIFY(this->m_req->stdErr() == nullptr);
		f = d->_q_processRecord(FCGI_PARAMS, QByteArray());
		QCOMPARE(f, false);
		QCOMPARE(spy.count(), 0);

		QCOMPARE(spy_rf.count(), 1);
	}

	void testAppendParamsAfterEOF()
	{
		bool f;
		FastCGI::LowLevel::RequestPrivate* d = this->m_req->d_func();

		QSignalSpy spy(this->m_req, SIGNAL(parametersParsed(const QList<QPair<QByteArray, QByteArray> >&)));
		QSignalSpy spy_rf(this->m_req, SIGNAL(requestFinished(quint16)));

		QVERIFY(this->m_req->stdOut() == nullptr);
		QVERIFY(this->m_req->stdErr() == nullptr);
		f = d->_q_processRecord(FCGI_PARAMS, QByteArray());
		QCOMPARE(f, true);

		QCOMPARE(spy.count(), 1);
		QList<QVariant> args = spy.takeFirst();

		QCOMPARE(args[0].canConvert<QList<QByteArrayPair> >(), true);
		QList<QByteArrayPair> res_actual = args[0].value<QList<QPair<QByteArray, QByteArray> > >();

		QList<QByteArrayPair> res_expected;
		QCOMPARE(res_actual, res_expected);
		QVERIFY(this->m_req->stdOut() != nullptr);
		QVERIFY(this->m_req->stdErr() != nullptr);
		QCOMPARE(this->m_req->stdOut()->isBuffered(), true);
		QCOMPARE(this->m_req->stdErr()->isBuffered(), false);
		QCOMPARE(this->m_req->stdOut()->type(), quint8(FCGI_STDOUT));
		QCOMPARE(this->m_req->stdErr()->type(), quint8(FCGI_STDERR));
		QCOMPARE(this->m_req->stdOut()->requestId(), this->m_req->id());
		QCOMPARE(this->m_req->stdErr()->requestId(), this->m_req->id());

		QCOMPARE(spy_rf.count(), 0);

		f = d->_q_processRecord(FCGI_PARAMS, QByteArray());
		QCOMPARE(f, false);

		QCOMPARE(spy_rf.count(), 1);
	}

	void test_appendStdin()
	{
		bool f;
		FastCGI::LowLevel::RequestPrivate* d = this->m_req->d_func();

		QSignalSpy spy_pp(this->m_req, SIGNAL(parametersParsed(const QList<QPair<QByteArray, QByteArray> >&)));
		QSignalSpy spy_rf(this->m_req, SIGNAL(requestFinished(quint16)));

		QVERIFY(this->m_req->stdOut() == nullptr);
		QVERIFY(this->m_req->stdErr() == nullptr);
		f = d->_q_processRecord(FCGI_PARAMS, QByteArray());
		QCOMPARE(f, true);

		QCOMPARE(spy_pp.count(), 1);

		QSignalSpy spy_sdr(this->m_req, SIGNAL(stdinDataReady(const QByteArray&)));
		QSignalSpy spy_sr(this->m_req, SIGNAL(stdinRead()));

		QByteArray buf1("Hello");
		QByteArray buf2(", ");
		QByteArray buf3("World!\n");

		f = d->_q_processRecord(FCGI_STDIN, buf1);
		QCOMPARE(f, true);
		f = d->_q_processRecord(FCGI_STDIN, buf2);
		QCOMPARE(f, true);
		f = d->_q_processRecord(FCGI_STDIN, buf3);
		QCOMPARE(f, true);

		QCOMPARE(spy_sdr.count(), 3);
		QCOMPARE(spy_sr.count(), 0);

		QList<QVariant> params;
		params = spy_sdr.takeFirst();
		QCOMPARE(params[0].type(), QVariant::ByteArray);
		QCOMPARE(params[0].toByteArray(), buf1);

		params = spy_sdr.takeFirst();
		QCOMPARE(params[0].type(), QVariant::ByteArray);
		QCOMPARE(params[0].toByteArray(), buf2);

		params = spy_sdr.takeFirst();
		QCOMPARE(params[0].type(), QVariant::ByteArray);
		QCOMPARE(params[0].toByteArray(), buf3);

		f = d->_q_processRecord(FCGI_STDIN, QByteArray());
		QCOMPARE(f, true);
		QCOMPARE(spy_sdr.count(), 0);
		QCOMPARE(spy_sr.count(), 1);

		QCOMPARE(spy_rf.count(), 0);
	}

	void testAppendStdinAfterEOF()
	{
		bool f;
		FastCGI::LowLevel::RequestPrivate* d = this->m_req->d_func();

		QSignalSpy spy_pp(this->m_req, SIGNAL(parametersParsed(const QList<QPair<QByteArray, QByteArray> >&)));
		QSignalSpy spy_rf(this->m_req, SIGNAL(requestFinished(quint16)));

		QVERIFY(this->m_req->stdOut() == nullptr);
		QVERIFY(this->m_req->stdErr() == nullptr);
		f = d->_q_processRecord(FCGI_PARAMS, QByteArray());
		QCOMPARE(f, true);

		QCOMPARE(spy_pp.count(), 1);

		QSignalSpy spy_sdr(this->m_req, SIGNAL(stdinDataReady(const QByteArray&)));
		QSignalSpy spy_sr(this->m_req, SIGNAL(stdinRead()));

		QByteArray buf("Hello!");

		f = d->_q_processRecord(FCGI_STDIN, buf);
		QCOMPARE(f, true);

		QCOMPARE(spy_sdr.count(), 1);
		QCOMPARE(spy_sr.count(), 0);

		f = d->_q_processRecord(FCGI_STDIN, QByteArray());
		QCOMPARE(f, true);
		QCOMPARE(spy_sdr.count(), 1);
		QCOMPARE(spy_sr.count(), 1);

		QCOMPARE(spy_rf.count(), 0);

		f = d->_q_processRecord(FCGI_STDIN, QByteArray());
		QCOMPARE(f, false);
		QCOMPARE(spy_sdr.count(), 1);
		QCOMPARE(spy_sr.count(), 1);

		QCOMPARE(spy_rf.count(), 1);
	}

	void testAppendStdinBeforeParams()
	{
		bool f;
		FastCGI::LowLevel::RequestPrivate* d = this->m_req->d_func();

		QSignalSpy spy_sdr(this->m_req, SIGNAL(stdinDataReady(const QByteArray&)));
		QSignalSpy spy_sr(this->m_req, SIGNAL(stdinRead()));
		QSignalSpy spy_rf(this->m_req, SIGNAL(requestFinished(quint16)));

		QByteArray buf("Hello!");

		f = d->_q_processRecord(FCGI_STDIN, buf);
		QCOMPARE(f, false);

		QCOMPARE(spy_sdr.count(), 0);
		QCOMPARE(spy_sr.count(), 0);
		QCOMPARE(spy_rf.count(), 1);
	}

	void testAppendStdinWithAuthorizer()
	{
		bool f;
		FastCGI::LowLevel::RequestPrivate* d = this->m_req->d_func();
		d->setRole(FCGI_AUTHORIZER);
		QCOMPARE(this->m_req->role(), FastCGI::LowLevel::Authorizer);

		QSignalSpy spy_pp(this->m_req, SIGNAL(parametersParsed(const QList<QPair<QByteArray, QByteArray> >&)));
		QSignalSpy spy_rf(this->m_req, SIGNAL(requestFinished(quint16)));

		QVERIFY(this->m_req->stdOut() == nullptr);
		QVERIFY(this->m_req->stdErr() == nullptr);
		f = d->_q_processRecord(FCGI_PARAMS, QByteArray());
		QCOMPARE(f, true);

		QCOMPARE(spy_pp.count(), 1);

		QSignalSpy spy_sdr(this->m_req, SIGNAL(stdinDataReady(const QByteArray&)));
		QSignalSpy spy_sr(this->m_req, SIGNAL(stdinRead()));

		QByteArray buf("Hello!");

		f = d->_q_processRecord(FCGI_STDIN, buf);
		QCOMPARE(f, false);

		QCOMPARE(spy_sdr.count(), 0);
		QCOMPARE(spy_sr.count(), 0);
		QCOMPARE(spy_rf.count(), 1);
	}

	void test_appendData()
	{
		bool f;
		FastCGI::LowLevel::RequestPrivate* d = this->m_req->d_func();

		// FCGI_DATA is defined only for Filter
		d->setRole(FCGI_FILTER);
		QCOMPARE(this->m_req->role(), FastCGI::LowLevel::Filter);

		QSignalSpy spy_pp(this->m_req, SIGNAL(parametersParsed(const QList<QPair<QByteArray, QByteArray> >&)));
		QSignalSpy spy_rf(this->m_req, SIGNAL(requestFinished(quint16)));

		QVERIFY(this->m_req->stdOut() == nullptr);
		QVERIFY(this->m_req->stdErr() == nullptr);
		f = d->_q_processRecord(FCGI_PARAMS, QByteArray());
		QCOMPARE(f, true);

		QCOMPARE(spy_pp.count(), 1);

		QSignalSpy spy_ddr(this->m_req, SIGNAL(dataDataReady(const QByteArray&)));
		QSignalSpy spy_dr(this->m_req, SIGNAL(dataRead()));

		QByteArray buf1("Hello");
		QByteArray buf2(", ");
		QByteArray buf3("World!\n");

		f = d->_q_processRecord(FCGI_DATA, buf1);
		QCOMPARE(f, true);
		f = d->_q_processRecord(FCGI_DATA, buf2);
		QCOMPARE(f, true);
		f = d->_q_processRecord(FCGI_DATA, buf3);
		QCOMPARE(f, true);

		QCOMPARE(spy_ddr.count(), 3);
		QCOMPARE(spy_dr.count(), 0);

		QList<QVariant> params;
		params = spy_ddr.takeFirst();
		QCOMPARE(params[0].type(), QVariant::ByteArray);
		QCOMPARE(params[0].toByteArray(), buf1);

		params = spy_ddr.takeFirst();
		QCOMPARE(params[0].type(), QVariant::ByteArray);
		QCOMPARE(params[0].toByteArray(), buf2);

		params = spy_ddr.takeFirst();
		QCOMPARE(params[0].type(), QVariant::ByteArray);
		QCOMPARE(params[0].toByteArray(), buf3);

		f = d->_q_processRecord(FCGI_DATA, QByteArray());
		QCOMPARE(f, true);
		QCOMPARE(spy_ddr.count(), 0);
		QCOMPARE(spy_dr.count(), 1);

		QCOMPARE(spy_rf.count(), 0);
	}

	void test_appendDataAfterEOF()
	{
		bool f;
		FastCGI::LowLevel::RequestPrivate* d = this->m_req->d_func();

		// FCGI_DATA is defined only for Filter
		d->setRole(FCGI_FILTER);
		QCOMPARE(this->m_req->role(), FastCGI::LowLevel::Filter);

		QSignalSpy spy_pp(this->m_req, SIGNAL(parametersParsed(const QList<QPair<QByteArray, QByteArray> >&)));
		QSignalSpy spy_rf(this->m_req, SIGNAL(requestFinished(quint16)));

		QVERIFY(this->m_req->stdOut() == nullptr);
		QVERIFY(this->m_req->stdErr() == nullptr);
		f = d->_q_processRecord(FCGI_PARAMS, QByteArray());
		QCOMPARE(f, true);

		QCOMPARE(spy_pp.count(), 1);

		QSignalSpy spy_ddr(this->m_req, SIGNAL(dataDataReady(const QByteArray&)));
		QSignalSpy spy_dr(this->m_req, SIGNAL(dataRead()));

		QByteArray buf("Hello!");

		f = d->_q_processRecord(FCGI_DATA, buf);
		QCOMPARE(f, true);

		QCOMPARE(spy_ddr.count(), 1);
		QCOMPARE(spy_dr.count(), 0);

		f = d->_q_processRecord(FCGI_DATA, QByteArray());
		QCOMPARE(f, true);
		QCOMPARE(spy_ddr.count(), 1);
		QCOMPARE(spy_dr.count(), 1);
		QCOMPARE(spy_rf.count(), 0);

		f = d->_q_processRecord(FCGI_DATA, QByteArray());
		QCOMPARE(f, false);
		QCOMPARE(spy_ddr.count(), 1);
		QCOMPARE(spy_dr.count(), 1);
		QCOMPARE(spy_rf.count(), 1);
	}

	void testAppendDataBeforeParams()
	{
		bool f;
		FastCGI::LowLevel::RequestPrivate* d = this->m_req->d_func();

		QSignalSpy spy_ddr(this->m_req, SIGNAL(dataDataReady(const QByteArray&)));
		QSignalSpy spy_dr(this->m_req, SIGNAL(dataRead()));
		QSignalSpy spy_rf(this->m_req, SIGNAL(requestFinished(quint16)));

		QByteArray buf("Hello!");

		f = d->_q_processRecord(FCGI_DATA, buf);
		QCOMPARE(f, false);

		QCOMPARE(spy_ddr.count(), 0);
		QCOMPARE(spy_dr.count(), 0);
		QCOMPARE(spy_rf.count(), 1);
	}

	void testAppendDataWithResponder()
	{
		bool f;
		FastCGI::LowLevel::RequestPrivate* d = this->m_req->d_func();

		QSignalSpy spy_pp(this->m_req, SIGNAL(parametersParsed(const QList<QPair<QByteArray, QByteArray> >&)));
		QSignalSpy spy_rf(this->m_req, SIGNAL(requestFinished(quint16)));

		QVERIFY(this->m_req->stdOut() == nullptr);
		QVERIFY(this->m_req->stdErr() == nullptr);
		f = d->_q_processRecord(FCGI_PARAMS, QByteArray());
		QCOMPARE(f, true);

		QCOMPARE(spy_pp.count(), 1);

		QSignalSpy spy_ddr(this->m_req, SIGNAL(dataDataReady(const QByteArray&)));
		QSignalSpy spy_dr(this->m_req, SIGNAL(dataRead()));

		QByteArray buf("Hello!");

		f = d->_q_processRecord(FCGI_DATA, buf);
		QCOMPARE(f, false);

		QCOMPARE(spy_ddr.count(), 0);
		QCOMPARE(spy_dr.count(), 0);
		QCOMPARE(spy_rf.count(), 1);
	}

	void testFinish()
	{
		QSignalSpy spy(this->m_req, SIGNAL(requestFinished(quint16)));
		this->m_req->finish(FastCGI::LowLevel::Complete, 0xDEADC0DEu);
		QCOMPARE(spy.count(), 1);

		QList<QVariant> args = spy.takeFirst();
		QCOMPARE(args[0].value<quint16>(), quint16(2));

		QCOMPARE(this->m_buf->isOpen(), false);
		QCOMPARE(this->m_buf->buffer().toHex(), QByteArray("0103000200080000deadc0de00000000"));
	}

	void testFinishWithKeepAlive()
	{
		this->m_req->d_func()->setFlags(FCGI_KEEP_CONN);
		QCOMPARE(this->m_req->keepConnection(), true);

		QSignalSpy spy(this->m_req, SIGNAL(requestFinished(quint16)));
		this->m_req->finish(FastCGI::LowLevel::Complete, 0xDEADC0DEu);
		QCOMPARE(spy.count(), 1);

		QList<QVariant> args = spy.takeFirst();
		QCOMPARE(args[0].value<quint16>(), quint16(2));

		QCOMPARE(this->m_buf->isOpen(), true);
		QCOMPARE(this->m_buf->buffer().toHex(), QByteArray("0103000200080000deadc0de00000000"));
	}

	void testDoubleFinish()
	{
		this->m_req->d_func()->finish(FastCGI::LowLevel::Complete, 0xDEADC0DEu);
		QCOMPARE(this->m_buf->isOpen(), false);

		this->m_req->d_func()->finish(FastCGI::LowLevel::Complete, 0xDEADC0DEu);
		QCOMPARE(this->m_buf->isOpen(), false);
		QCOMPARE(this->m_buf->buffer().toHex(), QByteArray("0103000200080000deadc0de00000000"));
	}

	void testAbortRequest()
	{
		QSignalSpy spy(this->m_req, SIGNAL(abortRequest()));
		this->m_req->d_func()->_q_processRecord(FCGI_ABORT_REQUEST, QByteArray());
		QCOMPARE(spy.count(), 1);
	}

	void testUnknownRecord()
	{
		QSignalSpy spy_rf(this->m_req, SIGNAL(requestFinished(quint16)));
		this->m_req->d_func()->_q_processRecord(0xFF, QByteArray());
		QCOMPARE(spy_rf.count(), 1);
	}
};

QTEST_GUILESS_MAIN(RequestTest)

#include "tst_requesttest.moc"
