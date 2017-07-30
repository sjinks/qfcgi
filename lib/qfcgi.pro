QT       += network
QT       -= gui

TARGET    = qfcgi
TEMPLATE  = lib
DEFINES  += QFCGI_LIBRARY

CONFIG   += c++11 gcov static create_prl

SOURCES += \
	connection.cpp \
	connection_p.cpp \
	request.cpp \
	request_p.cpp \
	utils.cpp \
	outputstream.cpp \
	outputstream_p.cpp

HEADERS +=\
	qfcgi_global.h \
	fastcgi.h \
	connection.h \
	connection_p.h \
	request.h \
	request_p.h \
	utils.h \
	outputstream.h \
	outputstream_p.h \
	socketinfo_p.h \
	protocol/header_p.h \
	protocol/unknowntype_p.h \
	protocol/beginrequest_p.h \
	protocol/endrequest_p.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
