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
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define __STRICT_ANSI__
#include <json-c/json.h>

#include "psfs_conf.h"

psfs_conf g_psfs_conf;

char *psfs_conf_get_consumer_key()
{
	return g_psfs_conf.consumer_key;
}

char *psfs_conf_get_consumer_secret()
{
	return g_psfs_conf.consumer_secret;
}

char *psfs_conf_get_root()
{
	return g_psfs_conf.root;
}

char *psfs_conf_get_mount_point()
{
	return g_psfs_conf.mount_point;
}

char *psfs_conf_get_oauth_json_file()
{
	return g_psfs_conf.oauth_json_file;
}

char *psfs_conf_get_writable_tmp_path()
{
	return g_psfs_conf.writable_tmp_path;
}

static psfs_ret psfs_conf_parse_json(const char *buf)
{
	json_object *jobj = NULL;

	if (!buf)
		return PSFS_RET_FAIL;

	jobj = json_tokener_parse(buf);

	if (!jobj || is_error(jobj))
		return PSFS_RET_FAIL;

	memset(&g_psfs_conf, 0, sizeof(g_psfs_conf));

	json_object_object_foreach(jobj, key, val) {
		if (!strcmp(key, PSFS_CONF_ID_CONSUMER_KEY)) {
			if (json_type_string == json_object_get_type(val))
				snprintf(g_psfs_conf.consumer_key, sizeof(g_psfs_conf.consumer_key), "%s", json_object_get_string(val));
		} else if (!strcmp(key, PSFS_CONF_ID_CONSUMER_SECRET)) {
			if (json_type_string == json_object_get_type(val))
				snprintf(g_psfs_conf.consumer_secret, sizeof(g_psfs_conf.consumer_secret), "%s", json_object_get_string(val));
		} else if (!strcmp(key, PSFS_CONF_ID_ROOT)) {
			if (json_type_string == json_object_get_type(val)) {
				snprintf(g_psfs_conf.root, sizeof(g_psfs_conf.root), "%s", json_object_get_string(val));
				if (0 != strcmp(PSFS_API_ROOT_KUAIPAN, g_psfs_conf.root) && 0 != strcmp(PSFS_API_ROOT_APP_FOLDER, g_psfs_conf.root)) {
					snprintf(g_psfs_conf.root, sizeof(g_psfs_conf.root), "%s", PSFS_API_ROOT_KUAIPAN);
				}
			}
		} else if (!strcmp(key, PSFS_CONF_ID_MOUNT_POINT)) {
			if (json_type_string == json_object_get_type(val))
				snprintf(g_psfs_conf.mount_point, sizeof(g_psfs_conf.mount_point), "%s", json_object_get_string(val));
		} else if (!strcmp(key, PSFS_CONF_ID_OAUTH_JSON_FILE)) {
			if (json_type_string == json_object_get_type(val))
				snprintf(g_psfs_conf.oauth_json_file, sizeof(g_psfs_conf.oauth_json_file), "%s", json_object_get_string(val));
		} else if (!strcmp(key, PSFS_CONF_ID_WRITABLE_TMP_PATH)) {
			if (json_type_string == json_object_get_type(val))
				snprintf(g_psfs_conf.writable_tmp_path, sizeof(g_psfs_conf.writable_tmp_path), "%s", json_object_get_string(val));
		}
	}
	json_object_put(jobj);

	if (g_psfs_conf.mount_point[0] == '\0') {
		return PSFS_RET_FAIL;
	}

	if (g_psfs_conf.consumer_key[0] == '\0')
		snprintf(g_psfs_conf.consumer_key, sizeof(g_psfs_conf.consumer_key), "%s", PSFS_CONSUMER_KEY);
	if (g_psfs_conf.consumer_secret[0] == '\0')
		snprintf(g_psfs_conf.consumer_secret, sizeof(g_psfs_conf.consumer_secret), "%s", PSFS_CONSUMER_SECRET);
	if (g_psfs_conf.root[0] == '\0')
		snprintf(g_psfs_conf.root, sizeof(g_psfs_conf.root), "%s", PSFS_API_ROOT_KUAIPAN);
	if (g_psfs_conf.oauth_json_file[0] == '\0')
		snprintf(g_psfs_conf.oauth_json_file, sizeof(g_psfs_conf.oauth_json_file), "%s", PSFS_DEFAULT_OAUTH_JSON_FILE);
	if (g_psfs_conf.writable_tmp_path[0] == '\0')
		snprintf(g_psfs_conf.writable_tmp_path, sizeof(g_psfs_conf.writable_tmp_path), "%s", PSFS_DEFAULT_WRITABLE_TMP_PATH);

	return PSFS_RET_OK;
}

psfs_ret psfs_conf_load(char *file)
{
	struct stat st;
	int fd = 0;
	char *buf = NULL;
	int ret = 0;

	if (NULL == file)
		return PSFS_RET_FAIL;

	if (0 != stat(file, &st)) {
		return PSFS_RET_FAIL;
	}
	fd = open(file, O_RDONLY);
	if (-1 == fd) {
		PSFS_LOG("fail to open file (%s)\n", file);
		return PSFS_RET_FAIL;
	}

	buf = calloc(st.st_size, 1);
	ret = read(fd, buf, st.st_size);
	if (-1 == ret) {
		PSFS_LOG("fail to read file (%s)\n", file);
	}
	close(fd);
	if (PSFS_RET_FAIL == psfs_conf_parse_json(buf)) {
		PSFS_LOG("fail to load correct conf params from (%s).\n", file);
		PSFS_SAFE_FREE(buf);
		return PSFS_RET_FAIL;
	}
	PSFS_SAFE_FREE(buf);
	return PSFS_RET_OK;
}

void psfs_conf_dump()
{
	PSFS_LOG("psfs conf:\n");
	PSFS_LOG("\tconsumer_key: %s\n", g_psfs_conf.consumer_key);
	PSFS_LOG("\tconsumer_secret: %s\n", g_psfs_conf.consumer_secret);
	PSFS_LOG("\troot: %s\n", g_psfs_conf.root);
	PSFS_LOG("\tmount_point: %s\n", g_psfs_conf.mount_point);
	PSFS_LOG("\toauth_json_file: %s\n", g_psfs_conf.oauth_json_file);
	PSFS_LOG("\twritable_tmp_path: %s\n", g_psfs_conf.writable_tmp_path);
}
