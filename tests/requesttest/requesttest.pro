QT += testlib
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase link_prl gcov
CONFIG -= app_bundle

TEMPLATE = app

SOURCES +=  tst_requesttest.cpp

unix|win32: LIBS += -L$$OUT_PWD/../../lib/ -lqfcgi

INCLUDEPATH += $$PWD/../../lib
DEPENDPATH  += $$PWD/../../lib

win32:!win32-g++:    PRE_TARGETDEPS += $$OUT_PWD/../../lib/qfcgi.lib
else:unix|win32-g++: PRE_TARGETDEPS += $$OUT_PWD/../../lib/libqfcgi.a
