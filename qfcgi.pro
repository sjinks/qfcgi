TEMPLATE = subdirs

SUBDIRS = \
	lib \
	tests

lib.file = lib/qfcgi.pro
tests.depends = lib
