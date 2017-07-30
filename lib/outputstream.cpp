#include "outputstream.h"
#include "outputstream_p.h"
#include "fastcgi.h"

void FastCGI::LowLevel::OutputStream::flush()
{
	Q_D(FastCGI::LowLevel::OutputStream);
	d->doWriteData();
}

void FastCGI::LowLevel::OutputStream::setBuffered(bool v)
{
	Q_D(FastCGI::LowLevel::OutputStream);
	if (d->buffered != v) {
		d->buffered = v;
		if (!v) {
			this->flush();
		}
	}
}

bool FastCGI::LowLevel::OutputStream::isBuffered() const
{
	Q_D(const FastCGI::LowLevel::OutputStream);
	return d->buffered;
}

void FastCGI::LowLevel::OutputStream::write(const QByteArray& buf)
{
	Q_D(FastCGI::LowLevel::OutputStream);
	d->buf.append(buf);
	if (!d->buffered || static_cast<unsigned int>(d->buf.size()) >= FCGI_MY_MAX_LENGTH) {
		d->doWriteData();
	}
}

quint8 FastCGI::LowLevel::OutputStream::type() const
{
	Q_D(const FastCGI::LowLevel::OutputStream);
	return d->type;
}

quint16 FastCGI::LowLevel::OutputStream::requestId() const
{
	Q_D(const FastCGI::LowLevel::OutputStream);
	return d->id;
}

FastCGI::LowLevel::OutputStream::OutputStream(quint8 type, quint16 id, const QSharedPointer<QIODevice>& sock)
	: d_ptr(new FastCGI::LowLevel::OutputStreamPrivate(type, id, sock))
{
}

FastCGI::LowLevel::OutputStream::~OutputStream()
{
}
