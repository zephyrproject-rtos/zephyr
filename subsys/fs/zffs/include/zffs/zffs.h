/*
 * Copyright (c) 2018 Findlay Feng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_ZFFS_
#define H_ZFFS_

#include "config.h"
#include <atomic.h>
#include <crc16.h>
#include <errno.h>
#include <flash_map.h>
#include <misc/byteorder.h>
#include <misc/sflist.h>
#include <misc/util.h>
#include <zephyr.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ZFFS_NAME "zffs"
#define ZFFS_VER 0
#define ZFFS_NULL 0
#define ZFFS_ROOT_ID 0

#define ZFFS_TYPE_DIR 0
#define ZFFS_TYPE_FILE 1

#define ZFFS_FILE_SEEK_SET 0
#define ZFFS_FILE_SEEK_CUR 1
#define ZFFS_FILE_SEEK_END 2

#define zffs_disk(__type, __name) u8_t __name[sizeof(__type)]

struct zffs_area {
	u32_t offset;
	u32_t length;
	u16_t erase_seq;
	u8_t gc_seq;
	union {
		u8_t full_id;
		struct {
			u8_t id : 6;
			u8_t is_swap : 1;
			u8_t is_gc : 1;
		};
		struct {
			u8_t : 6;
			u8_t type : 2;
		};
	};
};


struct zffs_dir_data {
	char *name;
	u32_t head;
};

struct zffs_file_data {
	char *name;
	u32_t head;
	u32_t tail;
	u32_t size;
	u32_t next_id;
};

struct zffs_node_data {
	u8_t type;
	u32_t id;
	union {
		char *name;
		struct zffs_dir_data dir;
		struct zffs_file_data file;
	};
};

struct zffs_data {
	const struct flash_area *flash;
	void *tree_root;
	struct k_mutex lock;
	sys_slist_t opened;
	u8_t area_num;
	struct zffs_area base_area[ZFFS_CONFIG_AREA_MAX];
	struct zffs_area **area;
	struct zffs_area **data_area;
	struct zffs_area **swap_area;
	u32_t data_write_addr;
	u32_t swap_write_addr;
	u32_t next_id;
};

int zffs_mount(struct zffs_data *zffs);
int zffs_unmount(struct zffs_data *zffs);
int zffs_mkdir(struct zffs_data *zffs, const char *path);
int zffs_opendir(struct zffs_data *zffs, void *dir, const char *path);
int zffs_readdir(struct zffs_data *zffs, void *dir,
		  struct zffs_node_data *node_data);
int zffs_closedir(struct zffs_data *zffs, void *dir);
int zffs_open(struct zffs_data *zffs, void *file, const char *path);
int zffs_sync(struct zffs_data *zffs, void *file);
int zffs_close(struct zffs_data *zffs, void *file);
int zffs_write(struct zffs_data *zffs, void *file, const void *data,
		size_t len);
int zffs_read(struct zffs_data *zffs, void *file, void *data, size_t len);
off_t zffs_tell(struct zffs_data *zffs, void *file);
int zffs_truncate(struct zffs_data *zffs, void *file, off_t length);
int zffs_lseek(struct zffs_data *zffs, void *file, off_t off, int whence);
int zffs_rename(struct zffs_data *zffs, const char *from, const char *to);
int zffs_unlink(struct zffs_data *zffs, const char *path);
int zffs_stat(struct zffs_data *zffs, const char *path,
	       struct zffs_node_data *node_data);

#ifdef __cplusplus
}
#endif

#endif
