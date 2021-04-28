include $(abs_top_builddir)/src/rules.mk

noinst_PROGRAMS = \
	psfs_test_download_by_range	\
	psfs_test_upload_file
	
AM_CPPFLAGS = -DFUSE_USE_VERSION=26

psfs_test_download_by_range_SOURCES = psfs_test_download_by_range.c
psfs_test_download_by_range_LDADD = $(PSFS_BIN_LDFLAGS)

psfs_test_upload_file_SOURCES = psfs_test_upload_file.c
psfs_test_upload_file_LDADD = $(PSFS_BIN_LDFLAGS)
