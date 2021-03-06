/*
   (c) Copyright 2012  Tao Yu

   All rights reserved.

   Written by Tao Yu <yut616@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <curl/curl.h>
#include "psfs.h"
#include "psfs_oauth.h"
#include "psfs_conf.h"
#include "psfs_api.h"
#include "psfs_util.h"

static void psfs_test_usage(char *argv0)
{
	if (NULL == argv0)
		return;
	fprintf(stderr, "%s version: %s\n", argv0, PSFS_VERSION);
	fprintf(stderr, "usage:  %s -c <path of psfs.conf> dest_fullpath src_fullpath\n", argv0);
	fprintf(stderr, "\t -c <path of psfs.conf> \tset config file for psfs, it is json format.\n");
	fprintf(stderr, "\t -h \t --help \t\tthis usage.\n");
	fprintf(stderr, "\t dest_fullpath \t\t\tthe full path you want to upload file to.\n");
	fprintf(stderr, "\t src_fullpath \t\t\tthe full path of the file you want to upload.\n");
	fprintf(stderr, "\t e.g.: %s -c /etc/psfs.conf /demo/abc /etc/psfs.conf.\n", argv0);
	fprintf(stderr, "\t you should set mount point in psfs.conf\n");
}

int main(int argc, char *argv[])
{
	psfs_ret ret = PSFS_RET_OK;
	struct stat st;
	char *response = NULL;

	if (argc < 5) {
		psfs_test_usage(argv[0]);
		return -1;
	}
	if (0 == strcmp("-c", argv[1])) {
		if (argv[2] == '\0') {
			psfs_test_usage(argv[0]);
			return -1;
		}
		if (0 != stat(argv[2], &st)) {
			printf("config file of psfs (%s) is not existed.\n", argv[2]);
			psfs_test_usage(argv[0]);
			return -1;
		}
		if (PSFS_RET_OK != psfs_conf_load(argv[2])) {
			psfs_test_usage(argv[0]);
			return -1;
		}
	} else {
		psfs_test_usage(argv[0]);
		return -1;
	}

	psfs_oauth_init();

	ret = psfs_oauth_load();
	if (PSFS_RET_OK != ret) {
		printf("This is the first time you use psfs.\n");
		ret = psfs_oauth_request_token();
		if (PSFS_RET_OK != ret) {
			return -1;
		}
		ret = psfs_oauth_authorize();
		if (PSFS_RET_OK != ret) {
			return -1;
		}
		ret = psfs_oauth_access_token();
		if (PSFS_RET_OK != ret) {
			PSFS_LOG("Fail to get oauth token for psfs.\n");
			return -1;
		}
	}

	curl_global_init(CURL_GLOBAL_ALL);

	response = (char *)psfs_api_upload_file(argv[3], argv[4]);
	printf("%s\n", response);
	PSFS_SAFE_FREE(response);
	return ret;
}
