#include <QtCore/QIODevice>
#include "protocol/beginrequest_p.h"
#include "client.h"

FastCGI::LowLevel::Client::Client(QIODevice* socket)
	: m_socket(socket)
{
}

FastCGI::LowLevel::Client::~Client()
{
}

qint64 FastCGI::LowLevel::Client::beginRequest(FastCGI::LowLevel::Role role, bool keepAlive)
{
	FastCGI::Protocol::BeginRequestRecord r(quint16(role), keepAlive ? FCGI_KEEP_CONN : 0);
	return this->m_socket->write(static_cast<const char*>(r.raw()), sizeof(r));
}

qint64 FastCGI::LowLevel::Client::abortRequest()
{
	return 0;
}
