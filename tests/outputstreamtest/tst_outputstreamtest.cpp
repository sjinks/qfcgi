#include <QtCore/QBuffer>
#include <QtCore/QSharedPointer>
#include <QtTest/QTest>
#include "fastcgi.h"
#include "outputstream.h"
#include "protocol/header_p.h"

class Q_DECL_HIDDEN OutputStreamTest : public QObject {
	Q_OBJECT
public:
	OutputStreamTest()
		: m_buf(new QBuffer())
	{
	}

private:
	QSharedPointer<QBuffer> m_buf;
	FastCGI::LowLevel::OutputStream* m_stream;

private Q_SLOTS:
	void init()
	{
		this->m_buf->setBuffer(nullptr);
		this->m_buf->open(QIODevice::ReadWrite);
		this->m_stream = new FastCGI::LowLevel::OutputStream(FCGI_STDOUT, 1, this->m_buf);
	}

	void cleanup()
	{
		delete this->m_stream;
		this->m_buf->close();
	}

	void testGettersSetters()
	{
		bool actual, expected;
		actual   = this->m_stream->isBuffered();
		expected = !actual;

		this->m_stream->setBuffered(expected);
		actual = this->m_stream->isBuffered();
		QCOMPARE(actual, expected);

		actual   = !actual;
		expected = !expected;

		this->m_stream->setBuffered(expected);
		actual = this->m_stream->isBuffered();
		QCOMPARE(actual, expected);

		QCOMPARE(this->m_stream->requestId(), quint16(1));
		QCOMPARE(this->m_stream->type(),      quint8(FCGI_STDOUT));
	}

	void testUnbufferedWrite()
	{
		QByteArray buffer("HELLO!");

		this->m_stream->setBuffered(false);
		this->m_stream->write(buffer);

		QByteArray& written = this->m_buf->buffer();
		FastCGI::Protocol::Header& hdr = *new(written.data()) FastCGI::Protocol::Header(nullptr);

		QCOMPARE(hdr.getVersion(),       quint8(FASTCGI_VERSION));
		QCOMPARE(hdr.getRequestId(),     this->m_stream->requestId());
		QCOMPARE(hdr.getType(),          this->m_stream->type());
		QCOMPARE(hdr.getContentLength(), static_cast<quint16>(buffer.size()));

		QByteArray actual_buf = QByteArray::fromRawData(written.data() + sizeof(FastCGI::Protocol::Header), hdr.getContentLength());
		QCOMPARE(actual_buf, buffer);

		QCOMPARE(static_cast<quint32>(written.size()), static_cast<quint32>(hdr.getFullLength() + sizeof(FastCGI::Protocol::Header)));
	}

	void testBufferedWrite1()
	{
		QByteArray buffer("HELLO!");

		this->m_stream->setBuffered(true);
		this->m_stream->write(buffer);

		QByteArray& written = this->m_buf->buffer();
		QCOMPARE(written.size(), 0);

		this->m_stream->setBuffered(false);
		QVERIFY(written.size() >= static_cast<int>(sizeof(FastCGI::Protocol::Header)) + buffer.size());

		FastCGI::Protocol::Header& hdr = *new(written.data()) FastCGI::Protocol::Header(nullptr);

		QCOMPARE(hdr.getVersion(),       quint8(FASTCGI_VERSION));
		QCOMPARE(hdr.getRequestId(),     this->m_stream->requestId());
		QCOMPARE(hdr.getType(),          this->m_stream->type());
		QCOMPARE(hdr.getContentLength(), static_cast<quint16>(buffer.size()));

		QByteArray actual_buf = QByteArray::fromRawData(written.data() + sizeof(FastCGI::Protocol::Header), hdr.getContentLength());
		QCOMPARE(actual_buf, buffer);

		QCOMPARE(static_cast<quint32>(written.size()), static_cast<quint32>(hdr.getFullLength() + sizeof(FastCGI::Protocol::Header)));
	}

	void testBufferedWrite2()
	{
		QByteArray buffer("HELLO!");

		this->m_stream->setBuffered(true);
		this->m_stream->write(buffer);

		QByteArray& written = this->m_buf->buffer();
		QCOMPARE(written.size(), 0);

		this->m_stream->flush();
		QVERIFY(written.size() >= static_cast<int>(sizeof(FastCGI::Protocol::Header)) + buffer.size());

		FastCGI::Protocol::Header& hdr = *new(written.data()) FastCGI::Protocol::Header(nullptr);

		QCOMPARE(hdr.getVersion(),       quint8(FASTCGI_VERSION));
		QCOMPARE(hdr.getRequestId(),     this->m_stream->requestId());
		QCOMPARE(hdr.getType(),          this->m_stream->type());
		QCOMPARE(hdr.getContentLength(), static_cast<quint16>(buffer.size()));

		QByteArray actual_buf = QByteArray::fromRawData(written.data() + sizeof(FastCGI::Protocol::Header), hdr.getContentLength());
		QCOMPARE(actual_buf, buffer);

		QCOMPARE(static_cast<quint32>(written.size()), static_cast<quint32>(hdr.getFullLength() + sizeof(FastCGI::Protocol::Header)));
	}

	void testLargeBuffer()
	{
		QByteArray realbuf;
		QByteArray buffer(2*FCGI_MY_MAX_LENGTH, 'x');
		this->m_stream->write(buffer);

		QByteArray& written = this->m_buf->buffer();
		QVERIFY(written.size() >= static_cast<int>(2*sizeof(FastCGI::Protocol::Header)) + buffer.size());

		FastCGI::Protocol::Header& hdr1 = *new(written.data()) FastCGI::Protocol::Header(nullptr);
		QCOMPARE(hdr1.getVersion(),       quint8(FASTCGI_VERSION));
		QCOMPARE(hdr1.getRequestId(),     this->m_stream->requestId());
		QCOMPARE(hdr1.getType(),          this->m_stream->type());
		QCOMPARE(hdr1.getContentLength(), static_cast<quint16>(FCGI_MY_MAX_LENGTH));
		realbuf.append(written.data() + sizeof(FastCGI::Protocol::Header), hdr1.getContentLength());

		quint32 offset = hdr1.getFullLength() + sizeof(FastCGI::Protocol::Header);
		FastCGI::Protocol::Header& hdr2 = *new(written.data() + offset) FastCGI::Protocol::Header(nullptr);
		QCOMPARE(hdr2.getVersion(),       quint8(FASTCGI_VERSION));
		QCOMPARE(hdr2.getRequestId(),     this->m_stream->requestId());
		QCOMPARE(hdr2.getType(),          this->m_stream->type());
		QCOMPARE(hdr2.getContentLength(), static_cast<quint16>(FCGI_MY_MAX_LENGTH));
		realbuf.append(written.data() + offset + sizeof(FastCGI::Protocol::Header), hdr2.getContentLength());

		QCOMPARE(realbuf, buffer);

		QCOMPARE(static_cast<quint32>(written.size()), static_cast<quint32>(hdr1.getFullLength() + hdr2.getFullLength() + 2*sizeof(FastCGI::Protocol::Header)));
	}

	void testNoWrite()
	{
		QSharedPointer<QBuffer> dev(new QBuffer());
		dev->open(QIODevice::ReadWrite);

		{
			FastCGI::LowLevel::OutputStream* str = new FastCGI::LowLevel::OutputStream(FCGI_STDOUT, 1, dev);
			delete str;
		}

		QByteArray& written = dev->buffer();
		QCOMPARE(written.size(), 0);
	}

	void testFullCycleWithEmptyWrite()
	{
		QSharedPointer<QBuffer> dev(new QBuffer());
		dev->open(QIODevice::ReadWrite);
		QByteArray& written = dev->buffer();

		{
			FastCGI::LowLevel::OutputStream* str = new FastCGI::LowLevel::OutputStream(FCGI_STDOUT, 1, dev);
			str->flush();
			delete str;
		}

		QCOMPARE(written.size(), 0);

		{
			FastCGI::LowLevel::OutputStream* str = new FastCGI::LowLevel::OutputStream(FCGI_STDOUT, 1, dev);
			str->write(QByteArray());
			delete str;
		}

		QCOMPARE(written.size(), 0);
	}

	void testFullCycleBuffered()
	{
		QByteArray data("test");
		QSharedPointer<QBuffer> dev(new QBuffer());
		dev->open(QIODevice::ReadWrite);
		QByteArray& written = dev->buffer();

		{
			FastCGI::LowLevel::OutputStream* str = new FastCGI::LowLevel::OutputStream(FCGI_STDOUT, 1, dev);
			str->setBuffered(true);
			str->write(data);
			delete str;
		}

		FastCGI::Protocol::Header& hdr1 = *new(written.data()) FastCGI::Protocol::Header(nullptr);
		QCOMPARE(hdr1.getVersion(),       quint8(FASTCGI_VERSION));
		QCOMPARE(hdr1.getRequestId(),     quint16(1));
		QCOMPARE(hdr1.getType(),          quint8(FCGI_STDOUT));
		QCOMPARE(hdr1.getContentLength(), static_cast<quint16>(data.length()));

		quint32 offset = hdr1.getFullLength() + sizeof(FastCGI::Protocol::Header);
		FastCGI::Protocol::Header& hdr2 = *new(written.data() + offset) FastCGI::Protocol::Header(nullptr);
		QCOMPARE(hdr2.getVersion(),       quint8(FASTCGI_VERSION));
		QCOMPARE(hdr2.getRequestId(),     quint16(1));
		QCOMPARE(hdr2.getType(),          quint8(FCGI_STDOUT));
		QCOMPARE(hdr2.getContentLength(), quint16(0));

		QCOMPARE(static_cast<quint32>(written.size()), static_cast<quint32>(hdr1.getFullLength() + hdr2.getFullLength() + 2*sizeof(FastCGI::Protocol::Header)));
	}

	void testFullCycleUnbuffered()
	{
		QByteArray data("test");
		QSharedPointer<QBuffer> dev(new QBuffer());
		dev->open(QIODevice::ReadWrite);
		QByteArray& written = dev->buffer();

		{
			FastCGI::LowLevel::OutputStream* str = new FastCGI::LowLevel::OutputStream(FCGI_STDOUT, 1, dev);
			str->setBuffered(false);
			str->write(data);
			delete str;
		}

		FastCGI::Protocol::Header& hdr1 = *new(written.data()) FastCGI::Protocol::Header(nullptr);
		QCOMPARE(hdr1.getVersion(),       quint8(FASTCGI_VERSION));
		QCOMPARE(hdr1.getRequestId(),     quint16(1));
		QCOMPARE(hdr1.getType(),          quint8(FCGI_STDOUT));
		QCOMPARE(hdr1.getContentLength(), static_cast<quint16>(data.length()));

		quint32 offset = hdr1.getFullLength() + sizeof(FastCGI::Protocol::Header);
		FastCGI::Protocol::Header& hdr2 = *new(written.data() + offset) FastCGI::Protocol::Header(nullptr);
		QCOMPARE(hdr2.getVersion(),       quint8(FASTCGI_VERSION));
		QCOMPARE(hdr2.getRequestId(),     quint16(1));
		QCOMPARE(hdr2.getType(),          quint8(FCGI_STDOUT));
		QCOMPARE(hdr2.getContentLength(), quint16(0));

		QCOMPARE(static_cast<quint32>(written.size()), static_cast<quint32>(hdr1.getFullLength() + hdr2.getFullLength() + 2*sizeof(FastCGI::Protocol::Header)));
	}
};


QTEST_GUILESS_MAIN(OutputStreamTest)

#include "tst_outputstreamtest.moc"
