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

#ifndef	__PSFS_H__
#define	__PSFS_H__

#define PSFS_APP_NAME			"psfs"
#define PSFS_VERSION			"0.1"

#define PSFS_CONSUMER_KEY		"xc8D2NfL9c53vkrP"
#define PSFS_CONSUMER_SECRET		"p7Lo7Q6XBMipbHBw"

#define PSFS_API_REQUEST_TOKEN		"https://openapi.kuaipan.cn/open/requestToken"
#define PSFS_API_USER_AUTH		"https://www.kuaipan.cn/api.php?ac=open&op=authorise"
#define PSFS_API_ACCESS_TOKEN 		"https://openapi.kuaipan.cn/open/accessToken"

#define PSFS_OAUTH_VERSION		"1.0"
#define PSFS_SIGNATURE_METHOD		"HMAC-SHA1"

#define PSFS_MAX_PATH			(512)
#define PSFS_MAX_BUF			(4096)
#define PSFS_CURL_LOW_SPEED_LIMIT	(1)
#define PSFS_CURL_LOW_SPEED_TIMEOUT	(60)

#define PSFS_HTTP_GET			"GET"
#define PSFS_HTTP_POST			"POST"

#define PSFS_API_BASE_URL		"http://openapi.kuaipan.cn/"
#define PSFS_API_VERSION		"1"
#define PSFS_API_ROOT_KUAIPAN		"kuaipan"
#define PSFS_API_ROOT_APP_FOLDER	"app_folder"
#define PSFS_API_ACCOUNT_INFO		PSFS_API_BASE_URL PSFS_API_VERSION "/account_info"
#define PSFS_API_METADATA		PSFS_API_BASE_URL PSFS_API_VERSION "/metadata"
#define PSFS_API_CREATE_FOLDER		PSFS_API_BASE_URL PSFS_API_VERSION "/fileops/create_folder"
#define PSFS_API_MOVE			PSFS_API_BASE_URL PSFS_API_VERSION "/fileops/move"
#define PSFS_API_COPY			PSFS_API_BASE_URL PSFS_API_VERSION "/fileops/copy"
#define PSFS_API_DELETE			PSFS_API_BASE_URL PSFS_API_VERSION "/fileops/delete"

#define PSFS_API_THUMBNAIL		"http://conv.kuaipan.cn/1/fileops/thumbnail"
#define PSFS_API_UPLOAD_LOCATE		"http://api-content.dfs.kuaipan.cn/1/fileops/upload_locate"
#define PSFS_API_UPLOAD_FILE		"1/fileops/upload_file"
#define PSFS_API_DOWNLOAD_FILE		"http://api-content.dfs.kuaipan.cn/1/fileops/download_file"

#define PSFS_ID_MAX_FILE_SIZE		"max_file_size"
#define PSFS_ID_QUOTA_TOTAL		"quota_total"
#define PSFS_ID_QUOTA_USED		"quota_used"
#define PSFS_ID_USER_NAME		"user_name"
#define PSFS_ID_USER_ID			"user_id"
#define PSFS_ID_FILES			"files"
#define PSFS_ID_IS_DELETED		"is_deleted"
#define PSFS_ID_NAME			"name"
#define PSFS_ID_REV			"rev"
#define PSFS_ID_CREATE_TIME		"create_time"
#define PSFS_ID_MODIFY_TIME		"modify_time"
#define PSFS_ID_SIZE			"size"
#define PSFS_ID_TYPE			"type"
#define PSFS_ID_FILE_ID			"file_id"
#define PSFS_ID_PATH			"path"
#define PSFS_ID_ROOT			"root"
#define PSFS_ID_HASH			"hash"
#define PSFS_ID_MSG			"msg"
#define PSFS_ID_URL			"url"

#define PSFS_MSG_STR_REUSED_NONCE	"reused nonce"

#define PSFS_NODE_TYPE_STR_FILE		"file"
#define PSFS_NODE_TYPE_STR_FOLDER	"folder"

#define PSFS_CONF_ID_CONSUMER_KEY	"consumer_key"
#define PSFS_CONF_ID_CONSUMER_SECRET	"consumer_secret"
#define PSFS_CONF_ID_ROOT		"root"
#define PSFS_CONF_ID_MOUNT_POINT	"mount_point"
#define PSFS_CONF_ID_OAUTH_JSON_FILE	"oauth_json_file"
#define PSFS_CONF_ID_WRITABLE_TMP_PATH	"writable_tmp_path"

#define PSFS_DEFAULT_WRITABLE_TMP_PATH	"/tmp"
#define PSFS_DEFAULT_OAUTH_JSON_FILE	"psfs_oauth.json"

int psfs_file_log(const char *fmt, ...);

#define PSFS_DEBUG

#ifdef PSFS_DEBUG
#define PSFS_LOG(format,args...)	fprintf(stdout, format, ## args)
#define PSFS_FILE_LOG			psfs_file_log
#else
#define PSFS_LOG(format,args...)
#define PSFS_FILE_LOG
#endif

#define PSFS_SAFE_FREE(a) { if (a) {free(a); a = NULL;} }

#define PSFS_LOG_FILE_NAME		"psfs.log"

#define PSFS_COOKIE_FILE_NAME		"cookie.txt"

typedef enum {
	PSFS_RET_FAIL = -1,
	PSFS_RET_OK = 0,
} psfs_ret;

#endif
