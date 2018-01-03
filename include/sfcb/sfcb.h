/*  Simple Flash Circular Buffer for storage */

/*
 * Copyright (c) 2017 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SFCB_MOVE_BLOCK_SIZE 8

/*
 * Error codes.
 */
#define SFCB_OK		       0
#define SFCB_ERR_ARGS	  -1
#define SFCB_ERR_FLASH	  -2
#define SFCB_ERR_NOVAR   -3
#define SFCB_ERR_NOSPACE	-4
#define SFCB_ERR_FULL	  -5
#define SFCB_ERR_MAGIC   -7
#define SFCB_ERR_CRC     -8
#define SFCB_ERR_CFG     -9

/*
 * Special id values
 */
#define SFCB_ID_EMPTY      0xFFFF
#define SFCB_ID_SECTOR_END 0xFFFE

struct sfcb_entry {
    off_t data_addr; /* address in flash to write data */
    u16_t len;       /* entry length as written in flash (includes head and )*/
    u16_t id;        /* entry id, is 0xFFFFF when empty, set to 0xFFFE for the last entry in a sector */
};

/**
 * sfcb instance. Should be initialized with sfcb_fs_init() before use.
 **/
struct sfcb_fs {
    /* Constant values, set once at sfcb_fs_init(). */

    u32_t magic;               /* partition magic, repeated at start of each sector */
    off_t offset;              /* partition offset in bytes */
    off_t write_location;      /* next write location for entry header */
    u16_t sector_id;           /* sector id, a counter for each sector that is created */
    u16_t sector_size;         /* partition is divided into sectors,
                                  sector size should be multiple of pagesize */
    u8_t sector_count;         /* how many sectors in a partition */
    u8_t entry_sector;         /* oldest sector in use */
    u8_t write_block_size;     /* write block size for alignment */
    bool gc;                   /* enable garbage collection: if gc is true data from the oldest sector
                                  will be copied to the newest sector. This requires one sector to
                                  always be empty, and thus at least a sector_count of 2 */

    /* mutex definition */
    struct k_mutex fcb_lock;
    /* Active entry */
    struct device *flash_device;
};

int sfcb_fs_init(struct sfcb_fs *fs, u32_t magic);
int sfcb_fs_append(struct sfcb_fs *fs, struct sfcb_entry *entry);
int sfcb_fs_append_close(struct sfcb_fs *fs, const struct sfcb_entry *entry);
void sfcb_fs_set_start_entry(struct sfcb_fs *fs, struct sfcb_entry *entry);
int sfcb_fs_walk_entry(struct sfcb_fs *fs, struct sfcb_entry *entry);
int sfcb_fs_get_first_entry(struct sfcb_fs *fs, struct sfcb_entry *entry);
int sfcb_fs_get_last_entry(struct sfcb_fs *fs, struct sfcb_entry *entry);
int sfcb_fs_check_crc(struct sfcb_fs *fs, struct sfcb_entry *entry);
int sfcb_fs_rotate(struct sfcb_fs *fs);
int sfcb_fs_clear(struct sfcb_fs *fs);
int sfcb_fs_flash_read(struct sfcb_fs *fs, off_t offset, void *data, size_t len);
int sfcb_fs_flash_write(struct sfcb_fs *fs, off_t offset, const void *data, size_t len);
