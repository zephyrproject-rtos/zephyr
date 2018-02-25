/*  NVS: non volatile storage in flash
 *
 * Copyright (c) 2018 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __NVS_H_
#define __NVS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

#define NVS_MOVE_BLOCK_SIZE 8

/*
 * Error codes.
 */
#define NVS_OK		       0
#define NVS_ERR_ARGS	  -1
#define NVS_ERR_FLASH	  -2
#define NVS_ERR_NOVAR   -3
#define NVS_ERR_NOSPACE	-4
#define NVS_ERR_FULL	  -5
#define NVS_ERR_MAGIC   -7
#define NVS_ERR_CRC     -8
#define NVS_ERR_CFG     -9
#define NVS_ERR_LEN     -10
/*
 * Special id values
 */
#define NVS_ID_EMPTY      0xFFFF
#define NVS_ID_SECTOR_END 0xFFFE

struct nvs_entry {
	off_t data_addr; /* address in flash to write data */
	u16_t len;       /* entry length in bytes */
	u16_t id;        /* entry id, is 0xFFFFF when empty, set to 0xFFFE
			  * for the last entry in a sector
			  */
};

/**
 * nvs_fs instance. Should be initialized with nvs_init() before use.
 **/
struct nvs_fs {
	u32_t magic; /* filesystem magic, repeated at start of each sector */
	off_t write_location; /* next write location for entry header */
	off_t offset; /* filesystem offset in flash */
	u16_t sector_id; /* sector id, a counter for each created sector */
	u16_t sector_size; /* filesystem is divided into sectors,
			    * sector size should be multiple of pagesize
			    * and a power of 2
			    */
	u16_t max_len; /* maximum size of stored item, set to sector_size/4 */
	u8_t sector_count; /* how many sectors in the filesystem */

	u8_t entry_sector; /* oldest sector in use */
	u8_t write_block_size; /* write block size for alignment */

	struct k_mutex fcb_lock;
	struct device *flash_device;
};

int nvs_init(struct nvs_fs *fs, const char *dev_name, u32_t magic);
int nvs_clear(struct nvs_fs *fs);
int nvs_write(struct nvs_fs *fs, struct nvs_entry *entry, const void *data);
int nvs_read(struct nvs_fs *fs, struct nvs_entry *entry, void *data);
int nvs_read_hist(struct nvs_fs *fs, struct nvs_entry *entry, void *data,
	u8_t mode);

int nvs_append(struct nvs_fs *fs, struct nvs_entry *entry);
int nvs_append_close(struct nvs_fs *fs, const struct nvs_entry *entry);
void nvs_set_start_entry(struct nvs_fs *fs, struct nvs_entry *entry);
int nvs_walk_entry(struct nvs_fs *fs, struct nvs_entry *entry);
int nvs_get_first_entry(struct nvs_fs *fs, struct nvs_entry *entry);
int nvs_get_last_entry(struct nvs_fs *fs, struct nvs_entry *entry);
int nvs_check_crc(struct nvs_fs *fs, struct nvs_entry *entry);
int nvs_rotate(struct nvs_fs *fs);
int nvs_flash_read(struct nvs_fs *fs, off_t offset, void *data, size_t len);
int nvs_flash_write(struct nvs_fs *fs, off_t offset, const void *data,
	size_t len);

#ifdef __cplusplus
}
#endif

#endif /* __NVS_H_ */
