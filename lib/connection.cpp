#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QLocalSocket>
#include "connection.h"
#include "connection_p.h"

FastCGI::LowLevel::Connection::Connection(QIODevice* sock)
	: QObject(sock), d_ptr(new FastCGI::LowLevel::ConnectionPrivate(sock, this))
{
}

FastCGI::LowLevel::Connection::~Connection()
{
}
