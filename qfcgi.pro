TEMPLATE = subdirs

SUBDIRS = \
	lib \
	tests/utilstest \
	tests/outputstreamtest \
	tests/requesttest

lib.file = lib/qfcgi.pro
tests/utilstest.depends = lib
tests/outputstreamtest.depends = lib
tests/requesttest.depends = lib
