#include "request.h"
#include "request_p.h"
#include "fastcgi.h"
#include "outputstream.h"
#include "protocol/beginrequest_p.h"

quint16 FastCGI::LowLevel::Request::id() const
{
	Q_D(const FastCGI::LowLevel::Request);
	return d->id;
}

FastCGI::LowLevel::Role FastCGI::LowLevel::Request::role() const
{
	Q_D(const FastCGI::LowLevel::Request);
	return static_cast<FastCGI::LowLevel::Role>(d->role);
}

bool FastCGI::LowLevel::Request::keepConnection() const
{
	Q_D(const FastCGI::LowLevel::Request);
	return d->flags & FCGI_KEEP_CONN;
}

FastCGI::LowLevel::OutputStream* FastCGI::LowLevel::Request::stdOut()
{
	Q_D(FastCGI::LowLevel::Request);
	return d->stdout;
}

FastCGI::LowLevel::OutputStream* FastCGI::LowLevel::Request::stdErr()
{
	Q_D(FastCGI::LowLevel::Request);
	return d->stderr;
}

void FastCGI::LowLevel::Request::finish(FinishReason reason, quint32 exit_code)
{
	Q_D(FastCGI::LowLevel::Request);
	d->finish(static_cast<quint8>(reason), exit_code);
	Q_EMIT this->requestFinished(d->id);
	this->deleteLater();
}

FastCGI::LowLevel::Request::Request(quint16 id, const FastCGI::Protocol::BeginRequest& rec, const QSharedPointer<QIODevice>& socket)
	: QObject(), d_ptr(new FastCGI::LowLevel::RequestPrivate(id, rec, this, socket))
{
}

FastCGI::LowLevel::Request::~Request()
{
}
