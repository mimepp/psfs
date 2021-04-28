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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <curl/curl.h>
#include <errno.h>
#include <fuse.h>
#include <fuse_opt.h>
#include <oauth.h>

#define __STRICT_ANSI__
#include <json-c/json.h>

#include "psfs.h"
#include "psfs_node.h"
#include "psfs_conf.h"
#include "psfs_oauth.h"
#include "psfs_api.h"
#include "psfs_curl.h"
#include "psfs_util.h"

static int psfs_first_run = 1;

static char *psfs_get_tmpfile(struct fuse_file_info *fi)
{
	return (char *)(uintptr_t) fi->fh;
}

static void psfs_sha1_replace_slash(char *buf, int len)
{
	int i = 0;
	if (NULL == buf)
		return;
	for (i = 0; i < len; i++) {
		if (buf[i] == '/')
			buf[i] = 'a';
	}
}

static char *psfs_gen_tmp_fullpath(const char *fullpath)
{
	char *t_key = psfs_oauth_token_get();
	char *tmp_fullpath = NULL;
	char *sha1 = NULL;

	if (NULL == fullpath)
		return NULL;

	tmp_fullpath = calloc(PSFS_MAX_PATH, 1);
	if (NULL == tmp_fullpath)
		return NULL;

	sha1 = oauth_sign_hmac_sha1(fullpath, t_key);

	psfs_sha1_replace_slash(sha1, strlen(sha1));

	snprintf(tmp_fullpath, PSFS_MAX_PATH, "%s/%s", psfs_conf_get_writable_tmp_path(), (char *)sha1);
	PSFS_SAFE_FREE(sha1);
	return tmp_fullpath;
}

static int psfs_is_swap_file(const char *path)
{
	char *p = NULL;
	if (NULL == path)
		return 0;
	p = strrchr(path, '.');
	if (p && (0 == strcasecmp(p, ".swp"))) {
		return 1;
	}
	return 0;
}

static int psfs_getattr(const char *path, struct stat *stbuf)
{
	psfs_node *node = NULL;

	if (!path || !stbuf)
		return -1;

	if (1 == psfs_first_run) {
		psfs_node_get_root_path();
		psfs_node_parse_dir((psfs_node *) psfs_node_root_get(), "/");
		psfs_first_run = 0;
	}

	memset(stbuf, 0, sizeof(*stbuf));

	PSFS_FILE_LOG("[%s:%d] enter\n", __FUNCTION__, __LINE__);
	PSFS_FILE_LOG("path: %s\n", path);

	if (0 == strcmp(path, "/.Trash") || 0 == strcmp(path, "/.Trash-1000")) {
		PSFS_FILE_LOG("we will not handle %s\n", path);
		return -1;
	}
	node = (psfs_node *) psfs_node_get_by_path((psfs_node *) psfs_node_root_get(), path);
	if (NULL == node) {
		PSFS_FILE_LOG("%s:%d, could not find: %s\n", __FUNCTION__, __LINE__, path);
		return -ENOENT;
	}
	psfs_node_dump(node);
	memcpy(stbuf, &(node->st), sizeof(node->st));
	return 0;
}

static int psfs_access(const char *path, int mask)
{
	PSFS_FILE_LOG("[%s:%d] enter\n", __FUNCTION__, __LINE__);
	PSFS_FILE_LOG("[%s:%d] path: %s, mask: %d.\n", __FUNCTION__, __LINE__, path, mask);
	return 0;
}

static int psfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
	psfs_node *node = NULL;

	(void)offset;
	(void)fi;

	PSFS_FILE_LOG("[%s:%d] enter\n", __FUNCTION__, __LINE__);
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	node = (psfs_node *) psfs_node_get_by_path((psfs_node *) psfs_node_root_get(), path);
	if (node) {
		GHashTableIter iter;
		char *key = NULL;
		psfs_node *value = NULL;

		if (1 != strlen(path)) {
			//psfs_node_parse_dir(node, node->fullpath);
		}

		g_hash_table_iter_init(&iter, node->sub_nodes);
		while (g_hash_table_iter_next(&iter, (gpointer *) & key, (gpointer *) & value)) {
			PSFS_FILE_LOG("%s:%d, path: %s, filler: %s\n", __FUNCTION__, __LINE__, path, value->name);
			filler(buf, value->name, NULL, 0);
		}
	} else {
		return -1;
	}
	return 0;
}

static int psfs_read(const char *path, char *rbuf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	char *url = NULL;
	psfs_node *node = NULL;
	int ret = 0;
	off_t end_pos = 0;

	if (NULL == path || NULL == rbuf)
		return -1;
	PSFS_FILE_LOG("[%s:%d] enter\n", __FUNCTION__, __LINE__);
	PSFS_FILE_LOG("[%s:%d] path: %s, rbuf:%s, size: %lu, offset: %lu, file info: %p\n", __FUNCTION__, __LINE__, path, rbuf, size, offset, fi);

	node = (psfs_node *) psfs_node_get_by_path((psfs_node *) psfs_node_root_get(), path);
	if (NULL == node)
		return -1;

	url = psfs_api_download_link_create(path);

	if ((offset + size) > node->st.st_size)
		end_pos = node->st.st_size - 1;
	else
		end_pos = offset + size - 1;
	if (end_pos < 0)
		end_pos = 0;
	ret = psfs_curl_range_get(url, rbuf, offset, end_pos);
	PSFS_SAFE_FREE(url);
	return ret;
}

static int psfs_write(const char *path, const char *wbuf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	int fd = 0;
	int ret = 0;
	char *tmpfile = NULL;

	PSFS_FILE_LOG("[%s:%d] enter\n", __FUNCTION__, __LINE__);
	PSFS_FILE_LOG("[%s:%d] path: %s, wbuf:%s, size: %lu, offset: %lu, file info: %p\n", __FUNCTION__, __LINE__, path, wbuf, size, offset, fi);

	if (1 == psfs_is_swap_file(path))
		return -ENOTSUP;

	tmpfile = psfs_get_tmpfile(fi);
	fd = open(tmpfile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (fd == -1)
		return -errno;

	ret = pwrite(fd, wbuf, size, offset);
	if (ret == -1)
		ret = -errno;
	close(fd);

	return ret;
}

static int psfs_statfs(const char *path, struct statvfs *buf)
{
	off_t quota_total = psfs_util_account_quota_total_get();
	off_t quota_used = psfs_util_account_quota_used_get();

	PSFS_FILE_LOG("[%s:%d] enter\n", __FUNCTION__, __LINE__);
	PSFS_FILE_LOG("[%s:%d] path: %s, statvfs: %p.\n", __FUNCTION__, __LINE__, path, buf);

	psfs_util_account_info_dump();

	buf->f_bsize = 1;
	buf->f_frsize = 1;
	buf->f_blocks = quota_total;
	buf->f_bfree = quota_total - quota_used;
	buf->f_bavail = quota_total - quota_used;

	return 0;
}

static int psfs_mkdir(const char *path, mode_t mode)
{
	psfs_node *node = NULL;
	psfs_node *parent_node = NULL;
	char parent_path[PSFS_MAX_PATH] = { 0 };
	char *p = NULL;
	char *tmp = NULL;

	if (NULL == path)
		return -1;

	PSFS_FILE_LOG("[%s:%d] enter\n", __FUNCTION__, __LINE__);
	PSFS_FILE_LOG("[%s:%d] path: %s, mode: %d.\n", __FUNCTION__, __LINE__, path, mode);

	node = (psfs_node *) psfs_node_get_by_path((psfs_node *) psfs_node_root_get(), path);
	if (node) {
		PSFS_FILE_LOG("[%s:%d] psfs_mkdir, %s is existed.\n", __FUNCTION__, __LINE__, path);
		return -EEXIST;
	}
	if (PSFS_RET_FAIL == psfs_api_create_folder(path))
		return -1;

	p = strrchr(path, '/');
	if (p == path)
		parent_node = (psfs_node *) psfs_node_root_get();
	else {
		tmp = (char *)path;
		while (1) {
			memset(parent_path, 0, sizeof(parent_path));
			strncpy(parent_path, tmp, p - tmp);
			parent_node = (psfs_node *) psfs_node_get_by_path((psfs_node *) psfs_node_root_get(), parent_path);;
			if (parent_node)
				break;
			p = strrchr(parent_path, '/');
			tmp = parent_path;
			if (p == parent_path) {
				parent_node = (psfs_node *) psfs_node_root_get();
				break;
			}
		};
	}
	PSFS_FILE_LOG("[%s:%d] parent_path: %s, fullpath: %s\n", __FUNCTION__, __LINE__, parent_path, parent_node->fullpath);
	psfs_node_rebuild(parent_node);

	return 0;
}

static int psfs_delete(const char *path)
{
	psfs_node *node = NULL;
	psfs_node *parent_node = NULL;
	char *parent_path = NULL;

	if (NULL == path)
		return -1;

	PSFS_FILE_LOG("[%s:%d] enter\n", __FUNCTION__, __LINE__);
	PSFS_FILE_LOG("[%s:%d] path: %s.\n", __FUNCTION__, __LINE__, path);

	node = (psfs_node *) psfs_node_get_by_path((psfs_node *) psfs_node_root_get(), path);
	if (NULL == node) {
		PSFS_FILE_LOG("[%s:%d] %s is not existed.\n", __FUNCTION__, __LINE__, path);
		return -EEXIST;
	}
	if (PSFS_RET_FAIL == psfs_api_delete(path))
		return -1;

	parent_path = psfs_util_get_parent_path((char *)path);
	parent_node = (psfs_node *) psfs_node_get_by_path((psfs_node *) psfs_node_root_get(), parent_path);
	PSFS_FILE_LOG("[%s:%d] parent_path: %s, fullpath: %s\n", __FUNCTION__, __LINE__, parent_path, parent_node->fullpath);
	PSFS_SAFE_FREE(parent_path);

	psfs_node_rebuild(parent_node);

	return 0;
}

static int psfs_rmdir(const char *path)
{
	PSFS_FILE_LOG("[%s:%d] enter\n", __FUNCTION__, __LINE__);
	PSFS_FILE_LOG("[%s:%d] path: %s.\n", __FUNCTION__, __LINE__, path);

	return psfs_delete(path);
}

static int psfs_release(const char *path, struct fuse_file_info *fi)
{
	char *response = NULL;
	char *tmpfile = NULL;
	char *parent_path = NULL;
	psfs_node *parent_node = NULL;

	PSFS_FILE_LOG("[%s:%d] enter\n", __FUNCTION__, __LINE__);
	PSFS_FILE_LOG("[%s:%d] path: %s, file info: %p.\n", __FUNCTION__, __LINE__, path, fi);

	if (1 == psfs_is_swap_file(path))
		return -ENOTSUP;

	if (fi->flags & (O_RDWR | O_WRONLY)) {
		tmpfile = psfs_get_tmpfile(fi);
		response = psfs_api_upload_file((char *)path, tmpfile);
		unlink(tmpfile);
		PSFS_SAFE_FREE(tmpfile);
		PSFS_FILE_LOG("response: %s\n", response);
		PSFS_SAFE_FREE(response);

		parent_path = psfs_util_get_parent_path((char *)path);
		parent_node = (psfs_node *) psfs_node_get_by_path((psfs_node *) psfs_node_root_get(), parent_path);
		PSFS_FILE_LOG("[%s:%d] parent_path: %s, fullpath: %s\n", __FUNCTION__, __LINE__, parent_path, parent_node->fullpath);
		psfs_node_rebuild(parent_node);
		PSFS_SAFE_FREE(parent_path);
	}
	return 0;
}

static int psfs_rename(const char *from, const char *to)
{
	psfs_node *node = NULL;
	psfs_node *parent_node = NULL;
	char *parent_path = NULL;

	if (NULL == from || NULL == to)
		return -1;

	PSFS_FILE_LOG("[%s:%d] enter\n", __FUNCTION__, __LINE__);
	PSFS_FILE_LOG("[%s:%d] from: %s, to: %s.\n", __FUNCTION__, __LINE__, from, to);

	node = (psfs_node *) psfs_node_get_by_path((psfs_node *) psfs_node_root_get(), from);
	if (NULL == node) {
		PSFS_FILE_LOG("[%s:%d] %s is not existed.\n", __FUNCTION__, __LINE__, from);
		return -EEXIST;
	}
	if (PSFS_RET_FAIL == psfs_api_move(from, to))
		return -1;

	parent_path = psfs_util_get_parent_path((char *)from);
	parent_node = (psfs_node *) psfs_node_get_by_path((psfs_node *) psfs_node_root_get(), parent_path);
	PSFS_FILE_LOG("[%s:%d] parent_path: %s, fullpath: %s\n", __FUNCTION__, __LINE__, parent_path, parent_node->fullpath);
	PSFS_SAFE_FREE(parent_path);

	psfs_node_rebuild(parent_node);

	return 0;
}

static int psfs_truncate(const char *path, off_t offset)
{
	PSFS_FILE_LOG("[%s:%d] enter\n", __FUNCTION__, __LINE__);
	PSFS_FILE_LOG("[%s:%d] path: %s, offset: %d.\n", __FUNCTION__, __LINE__, path, offset);
	return 0;
}

static int psfs_utimens(const char *path, const struct timespec ts[2])
{
	int res;
	struct timeval tv[2];

	PSFS_FILE_LOG("[%s:%d] enter\n", __FUNCTION__, __LINE__);

	tv[0].tv_sec = ts[0].tv_sec;
	tv[0].tv_usec = ts[0].tv_nsec / 1000;
	tv[1].tv_sec = ts[1].tv_sec;
	tv[1].tv_usec = ts[1].tv_nsec / 1000;

	res = utimes(path, tv);
	if (res == -1)
		return -errno;

	return 0;
}

static int psfs_open(const char *path, struct fuse_file_info *fi)
{
	int ret = 0;
	psfs_node *node = NULL;

	if (NULL == path)
		return -1;

	PSFS_FILE_LOG("[%s:%d] enter\n", __FUNCTION__, __LINE__);
	PSFS_FILE_LOG("[%s:%d] path: %s, file info: %p.\n", __FUNCTION__, __LINE__, path, fi);

	if (1 == psfs_is_swap_file(path))
		return -ENOTSUP;

	node = (psfs_node *) psfs_node_get_by_path((psfs_node *) psfs_node_root_get(), path);
	if (NULL == node)
		return -1;
	if ((fi->flags & O_ACCMODE) == O_RDONLY) {
		return 0;
	} else if (fi->flags & (O_RDWR | O_WRONLY)) {
		char *tmpfile = psfs_gen_tmp_fullpath(path);
		fi->fh = (unsigned long)tmpfile;
	} else {
		ret = -ENOTSUP;
	}
	return ret;
}

static int psfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	int fd = 0;
	char *tmpfile = NULL;
	int ret = 0;

	PSFS_FILE_LOG("[%s:%d] enter\n", __FUNCTION__, __LINE__);
	PSFS_FILE_LOG("[%s:%d] path: %s, mode: %d, file info: %p.\n", __FUNCTION__, __LINE__, path, mode, fi);

	if (1 == psfs_is_swap_file(path))
		return -ENOTSUP;

	tmpfile = psfs_gen_tmp_fullpath(path);

	fi->fh = (unsigned long)tmpfile;

	fd = open(tmpfile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (fd == -1)
		return -errno;
	PSFS_FILE_LOG("[%s:%d] ret: %d\n", __FUNCTION__, __LINE__, ret);
	close(fd);

	return 0;
}

static int psfs_unlink(const char *path)
{
	PSFS_FILE_LOG("[%s:%d] enter\n", __FUNCTION__, __LINE__);
	PSFS_FILE_LOG("[%s:%d] path: %s.\n", __FUNCTION__, __LINE__, path);

	return psfs_delete(path);
}

static struct fuse_operations psfs_oper = {
	.getattr = psfs_getattr,
	.access = psfs_access,
	.readdir = psfs_readdir,
	.mkdir = psfs_mkdir,
	.unlink = psfs_unlink,
	.rmdir = psfs_rmdir,
	.release = psfs_release,
	.rename = psfs_rename,
	.truncate = psfs_truncate,
	.utimens = psfs_utimens,
	.open = psfs_open,
	.create = psfs_create,
	.read = psfs_read,
	.write = psfs_write,
	.statfs = psfs_statfs,
};

static void psfs_usage(char *argv0)
{
	if (NULL == argv0)
		return;
	fprintf(stderr, "%s version: %s\n", argv0, PSFS_VERSION);
	fprintf(stderr, "usage:  %s -c <path of psfs.conf> \n", argv0);
	fprintf(stderr, "\t -c <path of psfs.conf> \tset config file for psfs, it is json format.\n");
	fprintf(stderr, "\t -h \t --help \t\tthis usage.\n");
	fprintf(stderr, "\t e.g.: %s -c /etc/psfs.conf\n", argv0);
	fprintf(stderr, "\t you should set mount point in psfs.conf\n");
}

int main(int argc, char *argv[])
{
	psfs_ret ret = PSFS_RET_OK;
	int fuse_ret = 0;
	struct stat st;
#ifdef PSFS_FUSE_DEBUG
	int fuse_argc = 3;
	char *fuse_argv[3] = { 0 };
#else
	int fuse_argc = 2;
	char *fuse_argv[2] = { 0 };
#endif

	if (argc < 3) {
		psfs_usage(argv[0]);
		return -1;
	}
	if (0 == strcmp("-c", argv[1])) {
		if (argv[2][0] == '\0') {
			psfs_usage(argv[0]);
			return -1;
		}
		if (0 != stat(argv[2], &st)) {
			printf("config file of psfs (%s) is not existed.\n", argv[2]);
			psfs_usage(argv[0]);
			return -1;
		}
		if (PSFS_RET_OK == psfs_conf_load(argv[2])) {
			psfs_conf_dump();
			fuse_argv[0] = calloc(PSFS_MAX_PATH, 1);
			if (NULL == fuse_argv[0]) {
				printf("%s:%d, calloc fail.\n", __FUNCTION__, __LINE__);
				return -1;
			}
			fuse_argv[1] = calloc(PSFS_MAX_PATH, 1);
			if (NULL == fuse_argv[1]) {
				printf("%s:%d, calloc fail.\n", __FUNCTION__, __LINE__);
				return -1;
			}
			snprintf(fuse_argv[0], PSFS_MAX_PATH, "%s", argv[0]);
			snprintf(fuse_argv[1], PSFS_MAX_PATH, "%s", psfs_conf_get_mount_point());
#ifdef PSFS_FUSE_DEBUG
			fuse_argv[2] = calloc(PSFS_MAX_BUF, 1);
			if (NULL == fuse_argv[2]) {
				printf("%s:%d, calloc fail.\n", __FUNCTION__, __LINE__);
				return -1;
			}
			snprintf(fuse_argv[2], PSFS_MAX_BUF, "%s", "-d");
#endif
		}
	} else {
		psfs_usage(argv[0]);
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
		printf("Success to get oauth token, please run %s again to mount as FUSE filesystem.\n", PSFS_APP_NAME);
		return 0;
	}
	curl_global_init(CURL_GLOBAL_DEFAULT);

	fuse_ret = fuse_main(fuse_argc, fuse_argv, &psfs_oper, NULL);

	return fuse_ret;
}
