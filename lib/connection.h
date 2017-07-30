#ifndef FASTCGI_CONNECTION_H
#define FASTCGI_CONNECTION_H

#include "qfcgi_global.h"
#include <QtCore/QObject>
#include <QtCore/QScopedPointer>

QT_FORWARD_DECLARE_CLASS(QIODevice)

namespace FastCGI {
namespace LowLevel {

class ConnectionPrivate;
class Request;

class QFCGISHARED_EXPORT Connection : public QObject {
	Q_OBJECT
public:
	Connection(QIODevice* sock);
	virtual ~Connection() Q_DECL_OVERRIDE;

Q_SIGNALS:
	void newRequest(FastCGI::LowLevel::Request* req);
	void disconnected();

private:
	Q_DECLARE_PRIVATE(Connection)
	QScopedPointer<ConnectionPrivate> d_ptr;

	Q_PRIVATE_SLOT(d_func(), void _q_readyRead())
	Q_PRIVATE_SLOT(d_func(), void _q_disconnected())
	Q_PRIVATE_SLOT(d_func(), void _q_error())
	Q_PRIVATE_SLOT(d_func(), void _q_requestFinished(quint16))
};

} // namespace LowLevel
} // namespace FastCGI

#endif // FASTCGI_CONNECTION_H
