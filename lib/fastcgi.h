#ifndef FASTCGI_H_
#define FASTCGI_H_

#include <QtCore/QtGlobal>
#include <QtCore/QtEndian>
#include <cstring>

#define FCGI_MAX_LENGTH    0xFFFFu
#define FCGI_MY_MAX_LENGTH 0xFFF8u
#define FASTCGI_VERSION          1

#define FCGI_BEGIN_REQUEST       1
#define FCGI_ABORT_REQUEST       2
#define FCGI_END_REQUEST         3
#define FCGI_PARAMS              4
#define FCGI_STDIN               5
#define FCGI_STDOUT              6
#define FCGI_STDERR              7
#define FCGI_DATA                8
#define FCGI_GET_VALUES          9
#define FCGI_GET_VALUES_RESULT  10
#define FCGI_UNKNOWN_TYPE       11

#define FCGI_RESPONDER           1
#define FCGI_AUTHORIZER          2
#define FCGI_FILTER              3

#define FCGI_ROLE_MIN  FCGI_RESPONDER
#define FCGI_ROLE_MAX  FCGI_FILTER

#define FCGI_KEEP_CONN           1

#define FCGI_REQUEST_COMPLETE    0
#define FCGI_CANT_MPX_CONN       1
#define FCGI_OVERLOADED          2
#define FCGI_UNKNOWN_ROLE        3

namespace FastCGI {
	namespace LowLevel {

		enum Role {
			Responder  = FCGI_RESPONDER,
			Authorizer = FCGI_AUTHORIZER,
			Filter     = FCGI_FILTER
		};

		enum FinishReason {
			Complete        = FCGI_REQUEST_COMPLETE,
			CannotMultiplex = FCGI_CANT_MPX_CONN,
			Overloaded      = FCGI_OVERLOADED,
			UnknownRole     = FCGI_UNKNOWN_ROLE
		};
	}
}

#endif /* FASTCGI_H_ */
