#ifndef FASTCGI_CONNECTION_PRIVATE_H
#define FASTCGI_CONNECTION_PRIVATE_H

#include <QtCore/QHash>
#include <QtCore/QSharedPointer>
#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QLocalSocket>
#include "socketinfo_p.h"

QT_FORWARD_DECLARE_CLASS(QIODevice)

namespace FastCGI {
namespace LowLevel {

class Connection;
class Request;

class Q_DECL_HIDDEN ConnectionPrivate {
	Q_DECLARE_PUBLIC(Connection)
	Connection* const q_ptr;
public:
	ConnectionPrivate(QIODevice* sock, Connection* q);
	~ConnectionPrivate();

private:
	QSharedPointer<QIODevice> m_sock;
	SocketInfo m_info;

	void _q_readyRead();
	void _q_disconnected();
	void _q_error();
	void _q_requestFinished(quint16 id);

	void processManagementRecord();
	void processBeginRequestRecord();
	bool processOtherRecord();

	void disconnectSocketSignals();
	void killSocket();
};

} // namespace LowLevel
} // namespace FastCGI

#endif // FASTCGI_CONNECTION_PRIVATE_H
