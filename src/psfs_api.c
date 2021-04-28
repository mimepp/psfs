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
#include <sys/types.h>
#include <sys/stat.h>
#include <oauth.h>

#include "psfs.h"
#include "psfs_oauth.h"
#include "psfs_api.h"
#include "psfs_conf.h"
#include "psfs_util.h"
#include "psfs_curl.h"

static char *psfs_api_common_request(char *url)
{
	char *t_key = psfs_oauth_token_get();
	char *t_secret = psfs_oauth_token_secret_get();
	char *req_url = NULL;
	char *reply = NULL;
	char *ret = NULL;

	PSFS_FILE_LOG("request url: %s ...\n", url);

	req_url =
	    oauth_sign_url2(url, NULL, OA_HMAC, NULL, (const char *)psfs_conf_get_consumer_key(), (const char *)psfs_conf_get_consumer_secret(), t_key,
			    t_secret);
	if (!req_url) {
		goto error_out;
	}

	PSFS_FILE_LOG("real url: %s.\n", req_url);

	reply = psfs_curl_fetch(req_url);

	if (!reply) {
		goto error_out;
	}

	PSFS_FILE_LOG("response: %s\n", reply);
	ret = reply;

error_out:
	PSFS_SAFE_FREE(req_url);

	return ret;
}

static char *psfs_api_common_request_with_path(char *url, char *root, char *path)
{
	char *t_key = psfs_oauth_token_get();
	char *t_secret = psfs_oauth_token_secret_get();
	char *req_url = NULL;
	char *reply = NULL;
	char *ret = NULL;
	char *path_escape = NULL;
	int malloc_more_room = 100;
	char *url_with_path = NULL;
	int len = 0;

	if (NULL == url || NULL == root || NULL == path)
		return NULL;

	PSFS_FILE_LOG("request url: %s ...\n", url);

	path_escape = oauth_url_escape(path);
	if (NULL == path_escape)
		return NULL;

	len = strlen(url) + strlen(root) + strlen(path_escape) + malloc_more_room;
	url_with_path = calloc(len, 1);
	if (NULL == url_with_path) {
		PSFS_SAFE_FREE(path_escape);
		return NULL;
	}

	snprintf(url_with_path, len, "%s?root=%s&path=%s", url, root, path_escape);
	PSFS_SAFE_FREE(path_escape);

	req_url =
	    oauth_sign_url2(url_with_path, NULL, OA_HMAC, NULL, (const char *)psfs_conf_get_consumer_key(), (const char *)psfs_conf_get_consumer_secret(),
			    t_key, t_secret);
	if (!req_url) {
		goto error_out;
	}

	PSFS_FILE_LOG("real url: %s.\n", req_url);

	reply = psfs_curl_fetch(req_url);

	if (!reply) {
		goto error_out;
	}

	PSFS_FILE_LOG("response: %s\n", reply);
	ret = reply;

error_out:
	PSFS_SAFE_FREE(req_url);
	PSFS_SAFE_FREE(url_with_path);

	return ret;
}

static char *psfs_api_common_request_from_to(char *url, char *root, char *from_path, char *to_path)
{
	char *t_key = psfs_oauth_token_get();
	char *t_secret = psfs_oauth_token_secret_get();
	char *req_url = NULL;
	char *reply = NULL;
	char *ret = NULL;
	char *from_path_escape = NULL;
	char *to_path_escape = NULL;
	int malloc_more_room = 100;
	char *url_from_to = NULL;
	int len = 0;

	if (NULL == url || NULL == root || NULL == from_path || NULL == to_path)
		return NULL;

	PSFS_FILE_LOG("request url: %s ...\n", url);

	from_path_escape = oauth_url_escape(from_path);
	if (NULL == from_path_escape)
		goto error_out;

	to_path_escape = oauth_url_escape(to_path);
	if (NULL == to_path_escape)
		goto error_out;

	len = strlen(url) + strlen(root) + strlen(from_path_escape) + strlen(to_path_escape) + malloc_more_room;
	url_from_to = calloc(len, 1);
	if (NULL == url_from_to)
		goto error_out;

	snprintf(url_from_to, len, "%s?root=%s&from_path=%s&to_path=%s", url, root, from_path_escape, to_path_escape);

	req_url =
	    oauth_sign_url2(url_from_to, NULL, OA_HMAC, NULL, (const char *)psfs_conf_get_consumer_key(), (const char *)psfs_conf_get_consumer_secret(),
			    t_key, t_secret);
	if (!req_url) {
		goto error_out;
	}

	PSFS_FILE_LOG("real url: %s.\n", req_url);

	reply = psfs_curl_fetch(req_url);

	if (!reply) {
		goto error_out;
	}

	PSFS_FILE_LOG("response: %s\n", reply);
	ret = reply;

error_out:
	PSFS_SAFE_FREE(req_url);
	PSFS_SAFE_FREE(url_from_to);
	PSFS_SAFE_FREE(to_path_escape);
	PSFS_SAFE_FREE(from_path_escape);

	return ret;
}

char *psfs_api_account_info()
{
	return psfs_api_common_request(PSFS_API_ACCOUNT_INFO);
}

char *psfs_api_metadata(const char *path)
{
	char fullpath[1024] = { 0 };
	char *path_escape = NULL;

	path_escape = oauth_url_escape(path);
	if (NULL == path_escape)
		return NULL;

	snprintf(fullpath, sizeof(fullpath), "%s%s%s", PSFS_API_METADATA "/", psfs_conf_get_root(), path_escape);
	PSFS_SAFE_FREE(path_escape);
	PSFS_FILE_LOG("fullpath: %s\n", fullpath);
	return psfs_api_common_request(fullpath);
}

char *psfs_api_download_link_create(const char *path)
{
	char *t_key = psfs_oauth_token_get();
	char *t_secret = psfs_oauth_token_secret_get();
	char *url = PSFS_API_DOWNLOAD_FILE;
	char *req_url = NULL;
	char *ret = NULL;
	char *url_with_path = NULL;
	int len = 0;
	int malloc_more_room = 100;
	char *root = NULL;

	if (NULL == path)
		return ret;

	PSFS_FILE_LOG("request url: %s ...\n", url);
	root = psfs_conf_get_root();
	len = strlen(url) + strlen(root) + strlen(path) + malloc_more_room;
	url_with_path = calloc(len, 1);
	if (NULL == url_with_path)
		return ret;

	snprintf(url_with_path, len, "%s?root=%s&path=%s", url, root, path);

	req_url =
	    oauth_sign_url2(url_with_path, NULL, OA_HMAC, NULL, (const char *)psfs_conf_get_consumer_key(), (const char *)psfs_conf_get_consumer_secret(),
			    t_key, t_secret);
	if (!req_url) {
		return ret;
	}

	PSFS_FILE_LOG("download link: %s.\n", req_url);
	ret = req_url;

	return ret;
}

psfs_ret psfs_api_create_folder(const char *path)
{
	char *response = NULL;
	response = psfs_api_common_request_with_path(PSFS_API_CREATE_FOLDER, psfs_conf_get_root(), (char *)path);
	PSFS_SAFE_FREE(response);
	return PSFS_RET_OK;
}

char *psfs_api_upload_locate()
{
	char url[PSFS_MAX_BUF] = { 0 };

	snprintf(url, sizeof(url), "%s", PSFS_API_UPLOAD_LOCATE);

	return psfs_api_common_request(url);
}

char *psfs_api_upload_file(char *dest_fullpath, char *src_fullpath)
{
	char *t_key = psfs_oauth_token_get();
	char *t_secret = psfs_oauth_token_secret_get();
	char *postarg = NULL;
	char *req_url = NULL;
	char *reply = NULL;
	char *ret = NULL;
	char *dest_fullpath_escape = NULL;
	int malloc_more_room = 100;
	char *url_with_path = NULL;
	int len = 0;
	char *url = psfs_util_upload_locate_get();
	char *root = NULL;
	char post_url[PSFS_MAX_BUF] = { 0 };

	if (NULL == dest_fullpath || NULL == src_fullpath)
		return ret;

	PSFS_FILE_LOG("request url: %s ...\n", url);
	root = psfs_conf_get_root();

	dest_fullpath_escape = oauth_url_escape(dest_fullpath);
	if (NULL == dest_fullpath_escape)
		goto error_out;

	len = strlen(url) + strlen(PSFS_API_UPLOAD_FILE) + strlen(root) + strlen(dest_fullpath_escape) + malloc_more_room;
	url_with_path = calloc(len, 1);
	if (NULL == url_with_path)
		goto error_out;

	snprintf(url_with_path, len, "%s%s?root=%s&path=%s&overwrite=true", url, PSFS_API_UPLOAD_FILE, root, dest_fullpath_escape);

	PSFS_FILE_LOG("url_with_path: %s, ...\n", url_with_path);

	req_url =
	    oauth_sign_url2(url_with_path, &postarg, OA_HMAC, NULL, (const char *)psfs_conf_get_consumer_key(), (const char *)psfs_conf_get_consumer_secret(),
			    t_key, t_secret);
	PSFS_SAFE_FREE(url_with_path);
	if (!req_url)
		goto error_out;

	snprintf(post_url, sizeof(post_url), "%s?%s", req_url, postarg);
	PSFS_FILE_LOG("post_url: %s.\n", post_url);

	reply = calloc(PSFS_MAX_BUF, 1);
	if (NULL == reply)
		goto error_out;

	psfs_curl_upload(post_url, src_fullpath, reply);

	PSFS_FILE_LOG("upload reply: %s\n", reply);
	ret = reply;

error_out:
	PSFS_SAFE_FREE(dest_fullpath_escape);
	PSFS_SAFE_FREE(req_url);

	return ret;
}

psfs_ret psfs_api_delete(const char *path)
{
	char *response = NULL;
	response = psfs_api_common_request_with_path(PSFS_API_DELETE, psfs_conf_get_root(), (char *)path);
	PSFS_SAFE_FREE(response);
	return PSFS_RET_OK;
}

psfs_ret psfs_api_move(const char *from_path, const char *to_path)
{
	char *response = NULL;
	response = psfs_api_common_request_from_to(PSFS_API_MOVE, psfs_conf_get_root(), (char *)from_path, (char *)to_path);
	PSFS_SAFE_FREE(response);
	return PSFS_RET_OK;
}
