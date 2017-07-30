#include <QtCore/QtEndian>
#include <QtCore/QIODevice>
#include "outputstream_p.h"
#include "fastcgi.h"
#include "protocol/header_p.h"

FastCGI::LowLevel::OutputStreamPrivate::OutputStreamPrivate(quint8 type, quint16 id, const QSharedPointer<QIODevice>& sock)
	: socket(sock), type(type), id(id), buffered(true), open(false), buf()
{
}

FastCGI::LowLevel::OutputStreamPrivate::~OutputStreamPrivate()
{
	if (!this->buf.isEmpty()) {
		this->doWriteData();
	}

	if (this->open) {
		FastCGI::Protocol::Header h(this->type);
		h.setRequestId(this->id);
		h.setContentLength(0);
		this->socket->write(static_cast<const char*>(h.raw()), sizeof(h));
	}
}

void FastCGI::LowLevel::OutputStreamPrivate::doWriteData()
{
	if (this->buf.size()) {
		this->open = true;
	}

	while (static_cast<unsigned int>(this->buf.size()) > FCGI_MY_MAX_LENGTH) {
		QByteArray tmp;
		FastCGI::Protocol::Header h(this->type);
		h.setRequestId(this->id);
		h.setContentLength(FCGI_MY_MAX_LENGTH);

		tmp.append(static_cast<const char*>(h.raw()), sizeof(h));
		tmp.append(this->buf.left(FCGI_MY_MAX_LENGTH));
		this->buf.remove(0, FCGI_MY_MAX_LENGTH);
		this->socket->write(tmp);
	}

	if (this->buf.size()) {
		FastCGI::Protocol::Header h(this->type);
		h.setRequestId(this->id);
		h.setContentLength(static_cast<quint16>(this->buf.size()));
		this->buf.prepend(static_cast<const char*>(h.raw()), sizeof(h));
		this->socket->write(this->buf);
		this->buf.clear();
	}
}
