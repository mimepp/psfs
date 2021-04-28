## Process this file with automake to produce Makefile.in
include $(abs_top_builddir)/src/rules.mk

SUBDIRS = . tests

DIST_SUBDIRS = $(SUBDIRS)

lib_LTLIBRARIES = libpsfs.la

libpsfs_la_SOURCES = \
	psfs_api.c	\
	psfs_util.c	\
	psfs_conf.c	\
	psfs_curl.c	\
	psfs_node.c
	
libpsfs_la_LIBADD = $(PSFS_LIB_LDFLAGS)

AM_CPPFLAGS = -DFUSE_USE_VERSION=26 -D_FILE_OFFSET_BITS=64

bin_PROGRAMS = psfs

psfs_SOURCES = \
	psfs.h		\
	psfs.c

psfs_LDADD = $(PSFS_BIN_LDFLAGS)

EXTRA_DIST = \
	rules.mk	\
	psfs.h		\
	psfs_api.h	\
	psfs_util.h	\
	psfs_conf.h	\
	psfs_curl.h	\
	psfs_node.h

psfsdir=$(includedir)/psfs
psfs_HEADERS = \
	psfs.h		\
	psfs_api.h	\
	psfs_util.h	\
	psfs_conf.h	\
	psfs_curl.h	\
	psfs_node.h

CLEANFILES =

if WITH_GOBJECT_INTROSPECTION
psfs-1.0.gir: $(lib_LTLIBRARIES) $(G_IR_SCANNER) Makefile.am
	$(G_IR_SCANNER) \
	--warn-error \
	-v \
	--namespace psfs \
	--nsversion 1.0 \
	--add-include-path=$(srcdir) \
	--add-include-path=$(builddir) \
	--library=$(lib_LTLIBRARIES) \
	--output $@ \
	--verbose \
	--pkg=glib-2.0 \
	$(psfs_HEADERS) \
	$(libpsfs_la_SOURCES)

girdir = $(datadir)/gir-1.0
gir_DATA = psfs-1.0.gir

typelibsdir = $(libdir)/girepository-1.0
typelibs_DATA = psfs-1.0.typelib

$(typelibs_DATA): $(gir_DATA)
	g-ir-compiler \
	--includedir=$(srcdir) \
	--includedir=$(builddir) \
	-o $@ $<

CLEANFILES += psfs-1.0.gir $(typelibs_DATA)

endif # WITH_GOBJECT_INTROSPECTION
