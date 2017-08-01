#ifndef REQUESTPRIVATE_H
#define REQUESTPRIVATE_H

#include <QtCore/QtGlobal>
#include <QtCore/QSharedPointer>

QT_FORWARD_DECLARE_CLASS(QIODevice)

namespace FastCGI {

namespace Protocol {
	class BeginRequest;
}

namespace LowLevel {

class Request;
class ConnectionPrivate;
class OutputStream;

class Q_DECL_HIDDEN RequestPrivate {
	Q_DECLARE_PUBLIC(Request)
	Request* const q_ptr;
public:
	RequestPrivate(quint16 id, const FastCGI::Protocol::BeginRequest& rec, Request* q, const QSharedPointer<QIODevice>& sock);
	~RequestPrivate();

	void finish(quint8 reason, quint32 code);

	// For tests only
	void setRole(quint16 role);

	// For tests only
	void setFlags(quint8 flags);

	bool _q_processRecord(quint8 type, const QByteArray& payload);

private:
	QSharedPointer<QIODevice> socket;
	quint16 id;
	quint16 role;
	quint8 flags;
	QByteArray params;
	bool params_eof;
	bool stdin_started;
	bool stdin_eof;
	bool data_started;
	bool data_eof;
	QList<QPair<QByteArray, QByteArray> > nvp;

	OutputStream* stdout;
	OutputStream* stderr;

	bool appendParams(const QByteArray& buf);
	bool appendStdin(const QByteArray& buf);
	bool appendData(const QByteArray& buf);
};

} // namespace LowLevel
} // namespace FastCGI

#endif // REQUESTPRIVATE_H
