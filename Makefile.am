## Process this file with automake to produce Makefile.in
SUBDIRS = src #docs

pkgconfigdir = $(libdir)/pkgconfig

pkgconfig_DATA = psfs.pc

ACLOCAL_AMFLAGS = -I m4

confdir = $(datadir)/psfs/
conf_DATA = psfs.conf

EXTRA_DIST = autogen.sh psfs.pc.in gtk-doc.make

psfsdir = $(includedir)/psfs
psfs_HEADERS = config.h

indent:
	indent -npro -kr -i8 -ts8 -sob -l160 -ss -ncs -cp1 -il0 src/*.c src/*.h src/tests/*.c
