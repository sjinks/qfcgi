#ifndef FASTCGI_PROTOCOL_HEADER_P_H
#define FASTCGI_PROTOCOL_HEADER_P_H

#include <QtCore/QtGlobal>
#include "fastcgi.h"

namespace FastCGI {
namespace Protocol {

/**
 * @see http://www.mit.edu/~yandros/doc/specs/fcgi-spec.html#S3.3
 */
struct Q_DECL_HIDDEN header_t {
	quint8 version;
	quint8 type;
	quint16 requestId;
	quint16 contentLength;
	quint8 paddingLength;
	quint8 reserved;

	quint8 getType() const Q_DECL_NOEXCEPT
	{
		return this->type;
	}

	quint8 getVersion() const Q_DECL_NOEXCEPT
	{
		return this->version;
	}

	bool isManagementRecord() const Q_DECL_NOEXCEPT
	{
		return 0 == this->requestId;
	}

	quint16 getRequestId() const Q_DECL_NOEXCEPT
	{
		return qFromBigEndian(this->requestId);
	}

	quint16 getContentLength() const Q_DECL_NOEXCEPT
	{
		return qFromBigEndian(this->contentLength);
	}

	quint32 getFullLength() const Q_DECL_NOEXCEPT
	{
		quint32 res = this->getContentLength();
		res        += this->paddingLength;
		return res;
	}
};

Q_STATIC_ASSERT_X(sizeof(header_t) == 8, "sizeof(header_t) must be 8");

class Q_DECL_HIDDEN Header : private header_t {
public:
	Header()
	{
		this->version  = FASTCGI_VERSION;
		this->reserved = 0;
	}

	Header(quint8 type)
	{
		this->version       = FASTCGI_VERSION;
		this->type          = type;
		this->paddingLength = 0;
		this->reserved      = 0;
	}

	Header(std::nullptr_t)
	{
	}

	using header_t::getVersion;
	using header_t::getType;
	using header_t::getContentLength;
	using header_t::getRequestId;
	using header_t::isManagementRecord;
	using header_t::getFullLength;

	void setType(quint8 v) Q_DECL_NOEXCEPT
	{
		this->type = v;
	}

	void setRequestId(quint16 v) Q_DECL_NOEXCEPT
	{
		this->requestId = qToBigEndian(v);
	}

	void setContentLength(quint16 v) Q_DECL_NOEXCEPT
	{
		this->contentLength = qToBigEndian(v);
	}

	void setPaddingLength(quint8 v) Q_DECL_NOEXCEPT
	{
		this->paddingLength = v;
	}

	const void* raw() const Q_DECL_NOEXCEPT
	{
		return this;
	}
};

Q_STATIC_ASSERT_X(sizeof(Header) == 8, "sizeof(Header) must be 8");

} // namespace Protocol
} // namespace FastCGI

#endif // FASTCGI_PROTOCOL_HEADER_P_H
