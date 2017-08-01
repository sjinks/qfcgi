#ifndef REQUEST_H
#define REQUEST_H

#include "qfcgi_global.h"
#include <QtCore/QObject>
#include <QtCore/QScopedPointer>
#include <QtCore/QSharedPointer>
#include "fastcgi.h"

QT_FORWARD_DECLARE_CLASS(QIODevice)

class RequestTest;

namespace FastCGI {

namespace Protocol {
	class BeginRequest;
}

namespace LowLevel {

class RequestPrivate;
class ConnectionPrivate;
class OutputStream;

class QFCGISHARED_EXPORT Request : public QObject {
	Q_OBJECT
public:
	quint16 id() const;
	FastCGI::LowLevel::Role role() const;
	bool keepConnection() const;

	OutputStream* stdOut();
	OutputStream* stdErr();

	void finish(FastCGI::LowLevel::FinishReason reason = FastCGI::LowLevel::Complete, quint32 exit_code = 0);

Q_SIGNALS:
	void abortRequest();
	void parametersParsed(const QList<QPair<QByteArray, QByteArray> >& params);
	void stdinDataReady(const QByteArray& data);
	void stdinRead();
	void dataDataReady(const QByteArray& data);
	void dataRead();
	void requestFinished(quint16 id);

private:
	friend class ConnectionPrivate;
	friend class ::RequestTest;

	Request(quint16 id, const FastCGI::Protocol::BeginRequest& rec, const QSharedPointer<QIODevice>& socket);
	virtual ~Request() Q_DECL_OVERRIDE;

	Q_DISABLE_COPY(Request)
	Q_DECLARE_PRIVATE(Request)
	QScopedPointer<RequestPrivate> d_ptr;

	Q_PRIVATE_SLOT(d_func(), void _q_processRecord(quint8, const QByteArray&))
};

} // namespace LowLevel
} // namespace FastCGI

#endif // REQUEST_H
