#ifndef OUTPUTSTREAMPRIVATE_H
#define OUTPUTSTREAMPRIVATE_H

#include <QtCore/QByteArray>
#include <QtCore/QSharedPointer>

QT_FORWARD_DECLARE_CLASS(QIODevice)

namespace FastCGI {
namespace LowLevel {

class Q_DECL_HIDDEN OutputStreamPrivate {
public:
	OutputStreamPrivate(quint8 type, quint16 id, const QSharedPointer<QIODevice>& sock);
	~OutputStreamPrivate();

	void doWriteData();

	QSharedPointer<QIODevice> socket;
	quint8 type;
	quint16 id;
	bool buffered;
	bool open;
	QByteArray buf;
};

} // namespace LowLevel
} // namespace FastCGI

#endif // OUTPUTSTREAMPRIVATE_H
