#ifndef FASTCGI_PROTOCOL_ENDREQUEST_P_H
#define FASTCGI_PROTOCOL_ENDREQUEST_P_H

#include <QtCore/QtGlobal>
#include <new>
#include "fastcgi.h"
#include "protocol/header_p.h"

namespace FastCGI {
namespace Protocol {

/**
 * @see http://www.mit.edu/~yandros/doc/specs/fcgi-spec.html#S5.5
 */
struct Q_DECL_HIDDEN end_request_t {
	quint32 appStatus;
	quint8 protocolStatus;
	quint8 reserved[3];
};

Q_STATIC_ASSERT_X(sizeof(end_request_t) == 8, "sizeof(end_request_t) must be 8");

class Q_DECL_HIDDEN EndRequest {
public:
	EndRequest(quint16 id, quint32 appStatus, quint8 protocolStatus)
	{
		this->s.h.setRequestId(id);
		this->s.h.setType(FCGI_END_REQUEST);
		this->s.h.setContentLength(sizeof(this->s.b));
		this->s.h.setPaddingLength(0);
		this->s.b.appStatus      = qToBigEndian(appStatus);
		this->s.b.protocolStatus = protocolStatus;
		std::memset(this->s.b.reserved, 0, sizeof(this->s.b.reserved));
	}

	EndRequest(std::nullptr_t)
	{
	}

	const void* raw() const Q_DECL_NOEXCEPT
	{
		return &this->s;
	}

private:
	struct {
		Header h;
		end_request_t b;
	} s;
};

Q_STATIC_ASSERT_X(sizeof(EndRequest) == 16, "sizeof(BeginRequest) must be 8");

} // namespace Protocol
} // namespace FastCGI

#endif // FASTCGI_PROTOCOL_ENDREQUEST_P_H
