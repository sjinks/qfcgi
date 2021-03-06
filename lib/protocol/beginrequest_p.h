#ifndef FASTCGI_PROTOCOL_BEGINREQUEST_P_H
#define FASTCGI_PROTOCOL_BEGINREQUEST_P_H

#include <QtCore/QtGlobal>
#include <new>
#include "fastcgi.h"
#include "protocol/header_p.h"

namespace FastCGI {
namespace Protocol {

/**
 * @see http://www.mit.edu/~yandros/doc/specs/fcgi-spec.html#S5.1
 */
struct Q_DECL_HIDDEN begin_request_t {
	quint16 role;
	quint8 flags;
	quint8 reserved[5];
};

Q_STATIC_ASSERT_X(sizeof(begin_request_t) == 8, "sizeof(begin_request_t) must be 8");

class Q_DECL_HIDDEN BeginRequest : private begin_request_t {
public:
	BeginRequest(quint16 role, quint8 flags)
	{
		this->role  = qToBigEndian(role);
		this->flags = flags;
	}

	BeginRequest(std::nullptr_t)
	{
	}

	quint16 getRole() const Q_DECL_NOEXCEPT
	{
		return qFromBigEndian(this->role);
	}

	quint8 getFlags() const Q_DECL_NOEXCEPT
	{
		return this->flags;
	}

	bool keepConn() const Q_DECL_NOEXCEPT
	{
		return this->flags & FCGI_KEEP_CONN;
	}

	void setRole(quint16 v) Q_DECL_NOEXCEPT
	{
		this->role = v;
	}

	void setFlags(quint8 v) Q_DECL_NOEXCEPT
	{
		this->flags = v;
	}

	const void* raw() const Q_DECL_NOEXCEPT
	{
		return this;
	}
};

Q_STATIC_ASSERT_X(sizeof(BeginRequest) == 8, "sizeof(BeginRequest) must be 8");

} // namespace Protocol
} // namespace FastCGI

#endif // FASTCGI_PROTOCOL_BEGINREQUEST_P_H
