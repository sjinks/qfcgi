#ifndef SOCKETINFO_P_H
#define SOCKETINFO_P_H

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include "protocol/header_p.h"

namespace FastCGI {
namespace LowLevel {

class Request;

struct Q_DECL_HIDDEN SocketInfo {
	enum state_t {
		BeforeHeader,
		ReadingHeader,
		BeforeData,
		ReadingData
	};

	state_t state;
	union {
		char b[8];
		FastCGI::Protocol::header_t h;
	} hdr;
	qint64 read;
	QByteArray buf;
	// Request IDs are per connection
	// http://www.fastcgi.com/archives/fastcgi-developers/2009-August/000323.html
	QHash<quint16, Request*> reqs;

	SocketInfo()
		: state(BeforeHeader), hdr(), read(0), buf(), reqs()
	{
	}
};

}
}

#endif // SOCKETINFO_P_H
