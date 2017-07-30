#include <QtCore/QtEndian>
#include <QtCore/QList>
#include "utils.h"

bool FastCGI::LowLevel::parseNV(const QByteArray& buf, QList<QPair<QByteArray, QByteArray> >& res)
{
	const uchar* p = reinterpret_cast<const uchar*>(buf.constData());
	const uchar* e = p + buf.size();

	while (p < e) {
		quint32 nsize = 0, vsize = 0;

		if (*p & 0x80) {
			if (e - p > 4) {
				nsize = qFromBigEndian<quint32>(p) & 0x7FFFFFFFul;
			}

			p += 4;
		}
		else {
			nsize = *p;
			++p;
		}

		if (p >= e) {
			return false;
		}

		if (*p & 0x80) {
			if (e - p > 4) {
				vsize = qFromBigEndian<quint32>(p) & 0x7FFFFFFFul;
			}

			p += 4;
		}
		else {
			vsize = *p;
			++p;
		}

		if (p + nsize + vsize > e) {
			return false;
		}

		QByteArray name  = QByteArray::fromRawData(reinterpret_cast<const char*>(p), int(nsize)); p += nsize;
		QByteArray value = QByteArray::fromRawData(reinterpret_cast<const char*>(p), int(vsize)); p += vsize;
		res.append(qMakePair(name, value));
	}

	return true;
}

QByteArray FastCGI::LowLevel::flattenNV(const QList<QPair<QByteArray, QByteArray> >& pairs)
{
	QByteArray res;
	for (int i=0; i<pairs.size(); ++i) {
		const QPair<QByteArray, QByteArray>& pair = pairs.at(i);

		if (pair.first.size() < 128) {
			char c = static_cast<char>(pair.first.size());
			res.append(c);
		}
		else {
			quint32 c = static_cast<quint32>(pair.first.size()) | 0x80000000u;
			c = qToBigEndian(c);
			char* s   = reinterpret_cast<char*>(&c);
			res.append(s, sizeof(c));
		}

		if (pair.second.size() < 128) {
			char c = static_cast<char>(pair.second.size());
			res.append(c);
		}
		else {
			quint32 c = static_cast<quint32>(pair.second.size()) | 0x80000000u;
			c = qToBigEndian(c);
			char* s   = reinterpret_cast<char*>(&c);
			res.append(s, sizeof(c));
		}

		res.append(pair.first).append(pair.second);
	}

	return res;
}
