#ifndef FASTCGI_CLIENT_H
#define FASTCGI_CLIENT_H

#include <QtCore/QObject>
#include "fastcgi.h"

QT_FORWARD_DECLARE_CLASS(QIODevice)

namespace FastCGI {
namespace LowLevel {

class Client : public QObject {
	Q_OBJECT
public:
	Client(QIODevice* socket);
	virtual ~Client() Q_DECL_OVERRIDE;

	qint64 beginRequest(FastCGI::LowLevel::Role role, bool keepAlive = false);
	qint64 abortRequest();

private:
	QIODevice* m_socket;
};

}
}

#endif // FASTCGI_CLIENT_H
