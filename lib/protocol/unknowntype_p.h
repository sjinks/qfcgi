#ifndef FASTCGI_PROTOCOL_UNKNOWNTYPE_P_H
#define FASTCGI_PROTOCOL_UNKNOWNTYPE_P_H

#include <QtCore/QtGlobal>
#include "fastcgi.h"
#include "protocol/header_p.h"

namespace FastCGI {
namespace Protocol {

/**
 * @see http://www.mit.edu/~yandros/doc/specs/fcgi-spec.html#S4.2
 */
struct Q_DECL_HIDDEN unknown_type_t {
	quint8 type;
	quint8 reserved[7];
};

Q_STATIC_ASSERT_X(sizeof(unknown_type_t) == 8, "sizeof(unknown_type_t) must be 8");

class Q_DECL_HIDDEN UnknownTypeRecord {
public:
	UnknownTypeRecord(quint8 type) : s()
	{
		this->s.h.setType(FCGI_UNKNOWN_TYPE);
		this->s.h.setRequestId(0);
		this->s.h.setPaddingLength(0);
		this->s.h.setContentLength(sizeof(this->s.b));

		this->s.b.type = type;
		memset(this->s.b.reserved, 0, sizeof(this->s.b.reserved));
	}

	const void* raw() const Q_DECL_NOEXCEPT
	{
		return &this->s;
	}

private:
	struct {
		Header h;
		unknown_type_t b;
	} s;
};

Q_STATIC_ASSERT_X(sizeof(UnknownTypeRecord) == 16, "sizeof(UnknownTypeRecord) must be 16");

} // namespace Protocol
} // namespace FastCGI


#endif // FASTCGI_PROTOCOL_UNKNOWNTYPE_P_H
