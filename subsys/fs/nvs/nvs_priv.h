/*  NVS: non volatile storage in flash
 *
 * Copyright (c) 2018 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __NVS_PRIV_H_
#define __NVS_PRIV_H_

#ifdef __cplusplus
extern "C" {
#endif

#define SYS_LOG_DOMAIN "fs/nvs"
#define SYS_LOG_LEVEL CONFIG_NVS_LOG_LEVEL
#include <logging/sys_log.h>

#define NVS_ID_GT(a, b) (((a) > (b)) ? ((((a)-(b)) > 0x7FFF) ? (0):(1)) : \
	((((b)-(a)) > 0x7FFF) ? (1):(0)))

/*
 * Special id values
 */
#define NVS_ID_EMPTY      0xFFFF
#define NVS_ID_SECTOR_END 0xFFFE
/*
 * Status return values
 */
#define NVS_STATUS_NOSPACE 1

#define NVS_MOVE_BLOCK_SIZE 8

struct nvs_entry {
	off_t data_addr; /* address in flash to write data */
	u16_t len;       /* entry length in bytes */
	u16_t id;        /* entry id, is 0xFFFFF when empty, set to 0xFFFE
			  * for the last entry in a sector
			  */
};

struct _nvs_sector_hdr {
	u32_t fd_magic;
	u16_t fd_id;
	u16_t _pad;
};

struct _nvs_data_hdr {
	u16_t id;
	u16_t len;
};

struct _nvs_data_slt {
	u16_t crc16;
	u16_t _pad;
};


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

/**
 * @}
 */
#ifdef __cplusplus
}
#endif

#endif /* __NVS_PRIV_H_ */
