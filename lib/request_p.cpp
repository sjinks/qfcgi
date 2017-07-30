#include <QtCore/QtEndian>
#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QLocalSocket>
#include <cstring>
#include "protocol/beginrequest_p.h"
#include "protocol/endrequest_p.h"
#include "request_p.h"
#include "request.h"
#include "connection_p.h"
#include "outputstream.h"
#include "utils.h"
#include "fastcgi.h"

FastCGI::LowLevel::RequestPrivate::RequestPrivate(quint16 reqid, const FastCGI::Protocol::BeginRequest& rec, FastCGI::LowLevel::Request* q, const QSharedPointer<QIODevice>& sock)
	: q_ptr(q), socket(sock), id(reqid), role(), flags(),
	  params(), params_eof(false),
	  stdin_started(false), stdin_eof(false),
	  data_started(false), data_eof(false),
	  nvp(),
	  stdout(Q_NULLPTR), stderr(Q_NULLPTR)
{
	this->role  = rec.getRole();
	this->flags = rec.getFlags();
}

FastCGI::LowLevel::RequestPrivate::~RequestPrivate()
{
}

bool FastCGI::LowLevel::RequestPrivate::appendParams(const QByteArray& buf)
{
	if (!this->params_eof) {
		if (buf.size()) {
			this->params.append(buf);
			return true;
		}

		this->params_eof = true;
		if (!FastCGI::LowLevel::parseNV(this->params, this->nvp)) {
			return false;
		}

		this->stdout = new FastCGI::LowLevel::OutputStream(FCGI_STDOUT, this->id, this->socket);
		this->stderr = new FastCGI::LowLevel::OutputStream(FCGI_STDERR, this->id, this->socket);
		this->stderr->setBuffered(false);

		Q_Q(FastCGI::LowLevel::Request);
		Q_EMIT q->parametersParsed(this->nvp);
		return true;
	}

	return false;
}

bool FastCGI::LowLevel::RequestPrivate::appendStdin(const QByteArray& buf)
{
	if (!this->stdin_eof) {
		if (!this->stdin_started) {
			// http://www.mit.edu/~yandros/doc/specs/fcgi-spec.html#S6.3
			// AUTHORIZER does not receive FCGI_STDIN stream
			// http://www.mit.edu/~yandros/doc/specs/fcgi-spec.html#S6
			// FCGI_PARAMS go before FCGI_STDIN
			if (this->role == quint16(FCGI_AUTHORIZER) || !this->params_eof) {
				return false;
			}

			this->stdin_started = true;
		}

		Q_Q(FastCGI::LowLevel::Request);
		if (buf.size()) {
			Q_EMIT q->stdinDataReady(buf);
		}
		else {
			this->stdin_eof = true;
			Q_EMIT q->stdinRead();
		}

		return true;
	}

	return false;
}

bool FastCGI::LowLevel::RequestPrivate::appendData(const QByteArray& buf)
{
	if (!this->data_eof) {
		// http://www.mit.edu/~yandros/doc/specs/fcgi-spec.html#S6
		// FCGI_PARAMS go before FCGI_STDIN / FCGI_DATA
		if (!this->params_eof) {
			return false;
		}

		Q_Q(FastCGI::LowLevel::Request);
		if (buf.size()) {
			if (!this->data_started) {
				if (this->role != FastCGI::LowLevel::Filter) {
					// http://www.mit.edu/~yandros/doc/specs/fcgi-spec.html#S6.4
					// Only Filter role accepts FCGI_DATA stream
					return false;
				}

				this->data_started = true;
			}

			Q_EMIT q->dataDataReady(buf);
		}
		else {
			this->data_eof = true;
			Q_EMIT q->dataRead();
		}

		return true;
	}

	return false;
}

void FastCGI::LowLevel::RequestPrivate::finish(quint8 reason, quint32 code)
{
	delete this->stdout;
	this->stdout = Q_NULLPTR;
	delete this->stderr;
	this->stderr = Q_NULLPTR;

	if (this->socket) {
		FastCGI::Protocol::EndRequest r(this->id, code, reason);
		this->socket->write(static_cast<const char*>(r.raw()), sizeof(r));
		QIODevice* s       = this->socket.data();
		QAbstractSocket* x = qobject_cast<QAbstractSocket*>(s);
		QLocalSocket* y    = qobject_cast<QLocalSocket*>(s);
		if (x) {
			x->flush();
		}
		else if (y) {
			y->flush();
		}

		// (5.1) flags & FCGI_KEEP_CONN: If zero, the application closes the connection after responding to this request.
		// If not zero, the application does not close the connection after responding to this request;
		// the Web server retains responsibility for the connection.
		if (!(this->flags & FCGI_KEEP_CONN)) {
			this->socket->close();
		}
	}

	this->socket.reset(Q_NULLPTR);
}

void FastCGI::LowLevel::RequestPrivate::setRole(quint16 role)
{
	this->role = role;
}

void FastCGI::LowLevel::RequestPrivate::setFlags(quint8 flags)
{
	this->flags = flags;
}
