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
#include <sys/stat.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <curl/curl.h>

#define __STRICT_ANSI__
#include <json-c/json.h>

#include "psfs.h"
#include "psfs_node.h"
#include "psfs_api.h"
#include "psfs_util.h"

psfs_node *g_psfs_node_root = NULL;

void psfs_node_free_sub_nodes(GHashTable * sub_nodes)
{
	GHashTableIter iter;
	char *key = NULL;
	psfs_node *value = NULL;

	if (NULL == sub_nodes)
		return;

	g_hash_table_iter_init(&iter, sub_nodes);
	while (g_hash_table_iter_next(&iter, (gpointer *) & key, (gpointer *) & value)) {
		if (value) {
			psfs_node_free_sub_nodes(value->sub_nodes);
			g_hash_table_destroy(value->sub_nodes);
			psfs_node_free(value);
			PSFS_SAFE_FREE(value);
		}
	}
}

void psfs_node_free(gpointer p)
{
	psfs_node *node = (psfs_node *) p;

	if (NULL == p)
		return;

	PSFS_SAFE_FREE(node->fullpath);
	PSFS_SAFE_FREE(node->id);
	PSFS_SAFE_FREE(node->name);
	PSFS_SAFE_FREE(node->revision);
	pthread_mutex_destroy(&(node->mutex));
}

const psfs_node *psfs_node_root_get()
{
	return g_psfs_node_root;
}

const psfs_node *psfs_node_root_create(char *id, char *name, off_t size)
{
	psfs_node *root = NULL;

	if (NULL != g_psfs_node_root)
		return g_psfs_node_root;

	root = calloc(sizeof(psfs_node), 1);
	if (NULL == root)
		return NULL;

	root->fullpath = strdup("/");
	root->id = id;
	root->name = name;
	root->revision = strdup("1");
	root->type = PSFS_NODE_TYPE_FOLDER;
	root->is_deleted = 0;
	root->st.st_mode = S_IFDIR | 0755;
	root->st.st_nlink = 2;
	root->st.st_size = size;
	pthread_mutex_init(&(root->mutex), NULL);
	root->sub_nodes = g_hash_table_new(g_str_hash, g_str_equal);
	g_psfs_node_root = root;

	return g_psfs_node_root;
}

const psfs_node *psfs_node_get_by_path(psfs_node * node, const char *path)
{
	psfs_node *ret = NULL;
	GHashTableIter iter;

	if (NULL == path || NULL == node)
		return NULL;

	if (1 == strlen(path) && path[0] == '/')
		return psfs_node_root_get();

	ret = g_hash_table_lookup(node->sub_nodes, path);
	if (NULL == ret) {
		char *key = NULL;
		psfs_node *value = NULL;

		PSFS_LOG("[%s:%d] could not find %s in node->sub_nodes.\n", __FUNCTION__, __LINE__, path);
		g_hash_table_iter_init(&iter, node->sub_nodes);
		while (g_hash_table_iter_next(&iter, (gpointer *) & key, (gpointer *) & value)) {
			PSFS_LOG("[%s:%d] iter, key: %s, path: %s\n", __FUNCTION__, __LINE__, key, path);
			ret = (psfs_node *) psfs_node_get_by_path(value, path);
			if (ret)
				return ret;
		}
	} else {
		return ret;
	}
	return ret;
}

void psfs_node_dump(psfs_node * node)
{
	if (NULL == node) {
		PSFS_FILE_LOG("[%s:%d] node is NULL.\n", __FUNCTION__, __LINE__);
		return;
	}

	PSFS_FILE_LOG("node data:\n");
	PSFS_FILE_LOG("\tfullpath: %s\n", node->fullpath);
	PSFS_FILE_LOG("\tid: %s\n", node->id);
	PSFS_FILE_LOG("\tname: %s\n", node->name);
	PSFS_FILE_LOG("\trevision: %s\n", node->revision);
	PSFS_FILE_LOG("\ttype: %d (0:file, 1: folder)\n", node->type);
	PSFS_FILE_LOG("\tis_deleted: %d\n", node->is_deleted);
	PSFS_FILE_LOG("\tst.st_size: " CURL_FORMAT_OFF_T "\n", node->st.st_size);
	PSFS_FILE_LOG("\tst.st_ctime: %lu\n", node->st.st_ctime);
	PSFS_FILE_LOG("\tst.st_mtime: %lu\n", node->st.st_mtime);
}

int psfs_node_get_root_path()
{
	char *response = NULL;
	json_object *jobj = NULL;
	off_t quota_used = 0;
	off_t quota_total = 0;
	off_t max_file_size = 0;
	int len = 512;
	char *user_name = NULL;
	char *user_id = NULL;
	char *p = NULL;

	response = (char *)psfs_api_account_info();
	PSFS_FILE_LOG("access [/] %s:\n", response);

	p = strstr(response, PSFS_ID_QUOTA_TOTAL);
	if (p) {
		sscanf(p, PSFS_ID_QUOTA_TOTAL "\": " CURL_FORMAT_OFF_T ",", &quota_total);
	}

	jobj = json_tokener_parse(response);
	if (NULL == jobj || is_error(jobj)) {
		PSFS_FILE_LOG("%s:%d, json_tokener_parse return error.\n", __FUNCTION__, __LINE__);
		PSFS_SAFE_FREE(response);
		return -1;
	}
	json_object_object_foreach(jobj, key, val) {
		if (!strcmp(key, PSFS_ID_QUOTA_USED)) {
			quota_used = (off_t) json_object_get_double(val);;
		} else if (!strcmp(key, PSFS_ID_MAX_FILE_SIZE)) {
			if (json_type_int == json_object_get_type(val)) {
				max_file_size = (off_t) json_object_get_int(val);
			}
		} else if (!strcmp(key, PSFS_ID_USER_ID)) {
			if (json_type_int == json_object_get_type(val)) {
				user_id = calloc(len, 1);
				snprintf(user_id, len, "%d", json_object_get_int(val));
			}
		} else if (!strcmp(key, PSFS_ID_USER_NAME)) {
			if (json_type_string == json_object_get_type(val)) {
				user_name = calloc(len, 1);
				snprintf(user_name, len, "%s", json_object_get_string(val));
			}
		}
	}
	json_object_put(jobj);
	PSFS_SAFE_FREE(response);

	psfs_util_account_info_store(user_name, user_id, quota_total, quota_used, max_file_size);

	if (NULL == psfs_node_root_create(user_id, user_name, quota_used))
		return -1;

	return 0;
}

static time_t psfs_node_str2time(char *str)
{
	struct timeval tv;
	time_t time;
	struct tm atm;
	int year = 0;
	int month = 0;
	int day = 0;
	int hour = 0;
	int min = 0;
	int sec = 0;

	gettimeofday(&tv, NULL);
	time = tv.tv_sec;

	if (NULL == str)
		return time;

	/* 2011-09-19 18:13:13 */
	sscanf(str, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &min, &sec);

	atm.tm_year = year - 1900;
	atm.tm_mon = month - 1;
	atm.tm_mday = day;
	atm.tm_hour = hour;
	atm.tm_min = min;
	atm.tm_sec = sec;

	time = mktime(&atm);

	return time;
}

psfs_ret psfs_node_parse_dir(psfs_node * parent_node, const char *path)
{
	char *response = NULL;
	json_object *jobj;
	json_object *tmp_jobj;
	psfs_node *node = NULL;
	int len = 0;
	char *parent_path = NULL;
	psfs_ret ret = PSFS_RET_FAIL;

	if (NULL == parent_node)
		return PSFS_RET_FAIL;

	response = (char *)psfs_api_metadata(path);
	PSFS_LOG("response %s:\n", response);

	jobj = json_tokener_parse(response);
	if (NULL == jobj || is_error(jobj)) {
		PSFS_FILE_LOG("%s:%d, json_tokener_parse return error.\n", __FUNCTION__, __LINE__);
		PSFS_SAFE_FREE(response);
		return PSFS_RET_FAIL;
	}

	json_object_object_foreach(jobj, key, val) {
		if (!strcmp(key, PSFS_ID_MSG)) {
			if (json_type_string == json_object_get_type(val)) {
				if (0 == strcmp(PSFS_MSG_STR_REUSED_NONCE, json_object_get_string(val))) {
					PSFS_FILE_LOG("%s:%d, receive reused nonce.\n", __FUNCTION__, __LINE__);
					goto error_out;
				}
			}
		} else if (!strcmp(key, PSFS_ID_PATH)) {
		} else if (!strcmp(key, PSFS_ID_ROOT)) {
			if (json_type_string == json_object_get_type(val)) {
			}
		} else if (!strcmp(key, PSFS_ID_HASH)) {
		} else if (!strcmp(key, PSFS_ID_FILES)) {
			if (json_type_array == json_object_get_type(val)) {
				int j = 0, array_len = 0;

				array_len = json_object_array_length(val);
				for (j = 0; j < array_len; j++) {
					tmp_jobj = json_object_array_get_idx(val, j);
					if (tmp_jobj && !is_error(tmp_jobj)) {
						node = calloc(sizeof(psfs_node), 1);
						if (NULL == node) {
							PSFS_FILE_LOG("%s:%d, fail to calloc node.\n", __FUNCTION__, __LINE__);
							goto error_out;
						}
						errno = pthread_mutex_init(&(node->mutex), NULL);
						if (errno) {
							PSFS_FILE_LOG("%s:%d, pthread_mutex_init fail.\n", __FUNCTION__, __LINE__);
							goto error_out;
						}
						node->sub_nodes = g_hash_table_new(g_str_hash, g_str_equal);
						len = strlen(path) + 1;
						parent_path = calloc(len, 1);
						snprintf(parent_path, len, "%s", path);

						PSFS_FILE_LOG("\n", __FUNCTION__, __LINE__);
						json_object_object_foreach(tmp_jobj, key2, val2) {
							if (!strcmp(key2, PSFS_ID_IS_DELETED)) {
								if (json_type_boolean == json_object_get_type(val2)) {
									node->is_deleted = json_object_get_boolean(val2) == TRUE ? 1 : 0;
									PSFS_FILE_LOG("%s:%d, is_deleted: %d.\n", __FUNCTION__, __LINE__, node->is_deleted);
								}
							} else if (!strcmp(key2, PSFS_ID_NAME)) {
								if (json_type_string == json_object_get_type(val2)) {
									node->name = strdup(json_object_get_string(val2));
									PSFS_FILE_LOG("%s:%d, name: %s.\n", __FUNCTION__, __LINE__, node->name);
								}
							} else if (!strcmp(key2, PSFS_ID_REV)) {
								if (json_type_string == json_object_get_type(val2)) {
									node->revision = strdup(json_object_get_string(val2));
									PSFS_FILE_LOG("%s:%d, revision: %s.\n", __FUNCTION__, __LINE__,
										      json_object_get_string(val2));
								}
							} else if (!strcmp(key2, PSFS_ID_CREATE_TIME)) {
								if (json_type_string == json_object_get_type(val2)) {
									char str[128] = { 0 };
									snprintf(str, sizeof(str), "%s", json_object_get_string(val2));
									node->st.st_ctime = psfs_node_str2time(str);
									PSFS_FILE_LOG("%s:%d, ctime: %d.\n", __FUNCTION__, __LINE__, node->st.st_ctime);
								}
							} else if (!strcmp(key2, PSFS_ID_MODIFY_TIME)) {
								if (json_type_string == json_object_get_type(val2)) {
									char str[128] = { 0 };
									snprintf(str, sizeof(str), "%s", json_object_get_string(val2));
									node->st.st_mtime = psfs_node_str2time(str);
									PSFS_FILE_LOG("%s:%d, mtime: %d.\n", __FUNCTION__, __LINE__, node->st.st_mtime);
								}
							} else if (!strcmp(key2, PSFS_ID_SIZE)) {
								if (json_type_int == json_object_get_type(val2)) {
									node->st.st_size = json_object_get_int(val2);
									PSFS_FILE_LOG("%s:%d, size: %d.\n", __FUNCTION__, __LINE__, node->st.st_size);
								}
							} else if (!strcmp(key2, PSFS_ID_TYPE)) {
								if (json_type_string == json_object_get_type(val2)) {
									if (0 == strcasecmp(PSFS_NODE_TYPE_STR_FILE, json_object_get_string(val2))) {
										node->type = PSFS_NODE_TYPE_FILE;
									} else {
										node->type = PSFS_NODE_TYPE_FOLDER;
									}
									PSFS_FILE_LOG("%s:%d, type: %d.\n", __FUNCTION__, __LINE__, node->type);
								}
							} else if (!strcmp(key2, PSFS_ID_FILE_ID)) {
								if (json_type_string == json_object_get_type(val2)) {
									node->id = strdup(json_object_get_string(val2));
									PSFS_FILE_LOG("%s:%d, id: %s.\n", __FUNCTION__, __LINE__, node->id);
								}
							}
						}
						if (node->name) {
							if (1 == strlen(parent_path) && parent_path[0] == '/') {
								len = strlen(parent_path) + strlen(node->name) + 1;
								node->fullpath = calloc(len, 1);
								snprintf(node->fullpath, len, "%s%s", parent_path, node->name);
							} else {
								len = strlen(parent_path) + strlen("/") + strlen(node->name) + 1;
								node->fullpath = calloc(len, 1);
								snprintf(node->fullpath, len, "%s/%s", parent_path, node->name);
							}
							PSFS_FILE_LOG("%s:%d, fullpath: %s.\n", __FUNCTION__, __LINE__, node->fullpath);
							if (PSFS_NODE_TYPE_FOLDER == node->type) {
								psfs_node_parse_dir(node, node->fullpath);
								node->st.st_mode = S_IFDIR | 0755;
								node->st.st_nlink = 2;
								if (0 == node->st.st_size)
									node->st.st_size = getpagesize();
							} else {
								node->st.st_mode = S_IFREG | 0666;
								node->st.st_nlink = 1;
							}

							PSFS_NODE_LOCK(parent_node);
							g_hash_table_insert(parent_node->sub_nodes, node->fullpath, node);
							PSFS_NODE_UNLOCK(parent_node);
						}
					}
				}
			}
		}
	}
	ret = PSFS_RET_OK;
error_out:
	json_object_put(jobj);

	PSFS_SAFE_FREE(response);
	return ret;
}

psfs_ret psfs_node_rebuild(psfs_node * node)
{
	if (NULL == node || NULL == node->sub_nodes)
		return PSFS_RET_FAIL;

	psfs_node_free_sub_nodes(node->sub_nodes);
	g_hash_table_remove_all(node->sub_nodes);

	return psfs_node_parse_dir(node, node->fullpath);
}
