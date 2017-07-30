#ifndef UTILS_H
#define UTILS_H

#include <QtCore/QByteArray>
#include <QtCore/QPair>

namespace FastCGI {
namespace LowLevel {

Q_DECL_HIDDEN bool parseNV(const QByteArray& buf, QList<QPair<QByteArray, QByteArray> >& res);
Q_DECL_HIDDEN QByteArray flattenNV(const QList<QPair<QByteArray, QByteArray> >& pairs);

}
}


#endif // UTILS_H
