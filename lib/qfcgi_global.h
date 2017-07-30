#ifndef QFCGI_GLOBAL_H
#define QFCGI_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(QFCGI_LIBRARY)
#  define QFCGISHARED_EXPORT Q_DECL_EXPORT
#else
#  define QFCGISHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // QFCGI_GLOBAL_H
