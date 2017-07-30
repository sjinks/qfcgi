#ifndef OUTPUT_STREAM_H
#define OUTPUT_STREAM_H

#include <QtCore/QScopedPointer>
#include <QtCore/QSharedPointer>
#include "qfcgi_global.h"

QT_FORWARD_DECLARE_CLASS(QIODevice)

class OutputStreamTest;

namespace FastCGI {
namespace LowLevel {

class OutputStreamPrivate;

class QFCGISHARED_EXPORT OutputStream {
public:
	void flush();
	void setBuffered(bool v);
	bool isBuffered() const;
	void write(const QByteArray& buf);

	quint8 type() const;
	quint16 requestId() const;
private:
	friend class RequestPrivate;
	friend class ::OutputStreamTest;

	OutputStream(quint8 type, quint16 id, const QSharedPointer<QIODevice>& sock);
	~OutputStream();

	Q_DISABLE_COPY(OutputStream)
	Q_DECLARE_PRIVATE(OutputStream)
	QScopedPointer<OutputStreamPrivate> d_ptr;
};

} // namespace LowLevel
} // namespace FastCGI

#endif // OUTPUT_STREAM_H
