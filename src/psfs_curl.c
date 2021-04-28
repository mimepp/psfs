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
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <curl/easy.h>

#include "psfs.h"
#include "psfs_curl.h"
#include "psfs_conf.h"

typedef struct psfs_curl_data_t psfs_curl_data;

struct psfs_curl_data_t {
	char *buf;
	off_t count;
};

static size_t write_func(void *data, size_t size, size_t nmemb, void *stream)
{
	psfs_curl_data *param = (psfs_curl_data *) stream;
	memcpy((param->buf + param->count), data, size * nmemb);
	param->count += size * nmemb;
	return size * nmemb;
}

char *psfs_curl_fetch(const char *url)
{
	char *buf = calloc(PSFS_MAX_BUF, 1);
	CURL *curl = NULL;
	CURLcode ret = -1;
	psfs_curl_data data;

	curl = curl_easy_init();
	if (NULL == curl)
		return NULL;

	data.buf = buf;
	data.count = 0;

	curl_easy_reset(curl);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&data);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_func);
	ret = curl_easy_perform(curl);
	printf("curl_easy_perform return value: %d (%s)\n", ret, curl_easy_strerror(ret));
	curl_easy_cleanup(curl);
	return buf;
}

int psfs_curl_range_get(const char *url, char *buf, off_t start_pos, off_t end_pos)
{
	CURL *curl_handle = NULL;
	CURLcode ret = CURLE_GOT_NOTHING;
	char errbuf[CURL_ERROR_SIZE];
	char range[128] = { 0 };
	char cookie_path[PSFS_MAX_PATH] = { 0 };
	psfs_curl_data data;

	if (NULL == url || NULL == buf)
		return -1;
	if (start_pos < 0 || end_pos < 0)
		return -1;

	memset(buf, 0, end_pos - start_pos);

	data.buf = buf;
	data.count = 0;

	curl_handle = curl_easy_init();

	snprintf(range, sizeof(range), CURL_FORMAT_OFF_T "-" CURL_FORMAT_OFF_T, start_pos, end_pos);
	PSFS_FILE_LOG("range = %s.\n", range);

	curl_easy_reset(curl_handle);

	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, PSFS_APP_NAME "/" PSFS_VERSION);

	curl_easy_setopt(curl_handle, CURLOPT_URL, url);

	curl_easy_setopt(curl_handle, CURLOPT_RANGE, range);

	curl_easy_setopt(curl_handle, CURLOPT_HTTPGET, 1L);

	curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);

	snprintf(cookie_path, sizeof(cookie_path), "%s/%s", (char *)psfs_conf_get_writable_tmp_path(), PSFS_COOKIE_FILE_NAME);
	curl_easy_setopt(curl_handle, CURLOPT_COOKIEFILE, cookie_path);

	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&data);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_func);

	curl_easy_setopt(curl_handle, CURLOPT_LOW_SPEED_LIMIT, PSFS_CURL_LOW_SPEED_LIMIT);	// Low limit : ? bytes per seconds
	curl_easy_setopt(curl_handle, CURLOPT_LOW_SPEED_TIME, PSFS_CURL_LOW_SPEED_TIMEOUT);	// Below low limit for ? seconds

	curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, errbuf);

	curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);

	curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);

	ret = curl_easy_perform(curl_handle);

	curl_easy_cleanup(curl_handle);

	if (ret != CURLE_OK) {
		PSFS_FILE_LOG("%s\n", curl_easy_strerror(ret));
		PSFS_FILE_LOG("[%s, %d] curl told us %d: %s.\n", __FUNCTION__, __LINE__, ret, errbuf);
		return -1;
	}
	return data.count;
}

int psfs_curl_upload(const char *url, char *file, char *reply)
{
	CURL *curl_handle = NULL;
	CURLcode ret = CURLE_GOT_NOTHING;
	struct curl_httppost *formpost = NULL;
	struct curl_httppost *lastptr = NULL;
	struct curl_slist *headerlist = NULL;
	static const char expect_buf[] = "Expect:";
	char errbuf[CURL_ERROR_SIZE];
	char cookie_path[PSFS_MAX_PATH] = { 0 };
	psfs_curl_data data;

	if (NULL == url || NULL == reply || NULL == file)
		return -1;

	data.buf = reply;
	data.count = 0;

	curl_handle = curl_easy_init();

	if (NULL == curl_handle)
		return -1;

	curl_easy_reset(curl_handle);

	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, PSFS_APP_NAME "/" PSFS_VERSION);

	curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);

	snprintf(cookie_path, sizeof(cookie_path), "%s/%s", (char *)psfs_conf_get_writable_tmp_path(), PSFS_COOKIE_FILE_NAME);
	curl_easy_setopt(curl_handle, CURLOPT_COOKIEFILE, cookie_path);

	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&data);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_func);

	curl_easy_setopt(curl_handle, CURLOPT_LOW_SPEED_LIMIT, PSFS_CURL_LOW_SPEED_LIMIT);	// Low limit : ? bytes per seconds
	curl_easy_setopt(curl_handle, CURLOPT_LOW_SPEED_TIME, PSFS_CURL_LOW_SPEED_TIMEOUT);	// Below low limit for ? seconds

	curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, errbuf);

	curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);

	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "file", CURLFORM_FILE, file, CURLFORM_END);
	headerlist = curl_slist_append(headerlist, expect_buf);
	curl_easy_setopt(curl_handle, CURLOPT_URL, url);
	curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headerlist);
	curl_easy_setopt(curl_handle, CURLOPT_HTTPPOST, formpost);

	curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);

	ret = curl_easy_perform(curl_handle);

	curl_easy_cleanup(curl_handle);
	curl_formfree(formpost);
	curl_slist_free_all(headerlist);

	if (ret != CURLE_OK) {
		PSFS_FILE_LOG("%s\n", curl_easy_strerror(ret));
		PSFS_FILE_LOG("[%s, %d] curl told us %d: %s.\n", __FUNCTION__, __LINE__, ret, errbuf);
		return -1;
	}
	return data.count;
}
