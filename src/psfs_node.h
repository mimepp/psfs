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

#ifndef	__PSFS_NODE_H__
#define	__PSFS_NODE_H__

#include <glib.h>

typedef enum {
	PSFS_NODE_TYPE_FILE,
	PSFS_NODE_TYPE_FOLDER
} psfs_node_type;

typedef struct {
	char *fullpath;
	char *id;
	char *name;
	char *revision;
	psfs_node_type type;
	int is_deleted;
	struct stat st;
	pthread_mutex_t mutex;
	GHashTable *sub_nodes;
} psfs_node;

#define PSFS_NODE_LOCK(node)	pthread_mutex_lock(&(node->mutex));
#define PSFS_NODE_UNLOCK(node)	pthread_mutex_unlock(&(node->mutex));

const psfs_node *psfs_node_root_get();
const psfs_node *psfs_node_root_create(char *id, char *name, off_t size);
void psfs_node_free(gpointer p);
const psfs_node *psfs_node_get_by_path(psfs_node * node, const char *path);
void psfs_node_dump(psfs_node * node);
int psfs_node_get_root_path();
psfs_ret psfs_node_parse_dir(psfs_node * parent_node, const char *path);
psfs_ret psfs_node_rebuild(psfs_node * node);
#endif
