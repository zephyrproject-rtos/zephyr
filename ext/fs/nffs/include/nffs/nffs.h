/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef H_NFFS_
#define H_NFFS_

#include <stddef.h>
#include <inttypes.h>
#include <nffs/config.h>
#include <nffs/queue.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * File access flags.
 */
#define FS_ACCESS_READ          0x01
#define FS_ACCESS_WRITE         0x02
#define FS_ACCESS_APPEND        0x04
#define FS_ACCESS_TRUNCATE      0x08

/*
 * File access return codes.
 */
#define FS_EOK          0  /* Success */
#define FS_ECORRUPT     1  /* File system corrupt */
#define FS_EHW          2  /* Error accessing storage medium */
#define FS_EOFFSET      3  /* Invalid offset */
#define FS_EINVAL       4  /* Invalid argument */
#define FS_ENOMEM       5  /* Insufficient memory */
#define FS_ENOENT       6  /* No such file or directory */
#define FS_EEMPTY       7  /* Specified region is empty (internal only) */
#define FS_EFULL        8  /* Disk full */
#define FS_EUNEXP       9  /* Disk contains unexpected metadata */
#define FS_EOS          10  /* OS error */
#define FS_EEXIST       11  /* File or directory already exists */
#define FS_EACCESS      12  /* Operation prohibited by file open mode */
#define FS_EUNINIT      13  /* File system not initialized */

#define NFFS_FILENAME_MAX_LEN   256  /* Does not require null terminator. */

#define NFFS_HASH_SIZE               256

#define NFFS_ID_DIR_MIN              0
#define NFFS_ID_DIR_MAX              0x10000000
#define NFFS_ID_FILE_MIN             0x10000000
#define NFFS_ID_FILE_MAX             0x80000000
#define NFFS_ID_BLOCK_MIN            0x80000000
#define NFFS_ID_BLOCK_MAX            0xffffffff

#define NFFS_ID_ROOT_DIR             0
#define NFFS_ID_NONE                 0xffffffff
#define NFFS_HASH_ENTRY_NONE         0xffffffff

#define NFFS_AREA_MAGIC0             0xb98a31e2
#define NFFS_AREA_MAGIC1             0x7fb0428c
#define NFFS_AREA_MAGIC2             0xace08253
#define NFFS_AREA_MAGIC3             0xb185fc8e
#define NFFS_BLOCK_MAGIC             0x53ba23b9
#define NFFS_INODE_MAGIC             0x925f8bc0

#define NFFS_AREA_ID_NONE            0xff
#define NFFS_AREA_VER_0                 0
#define NFFS_AREA_VER_1              1
#define NFFS_AREA_VER                NFFS_AREA_VER_1
#define NFFS_AREA_OFFSET_ID          23

#define NFFS_SHORT_FILENAME_LEN      3

#define NFFS_BLOCK_MAX_DATA_SZ_MAX   NFFS_CONFIG_MAX_BLOCK_SIZE

#define NFFS_DETECT_FAIL_IGNORE     1
#define NFFS_DETECT_FAIL_FORMAT     2

struct nffs_flash_desc {
    uint8_t id;
    uint32_t sector_count;
    uint32_t area_offset;
    uint32_t area_size;
};

struct nffs_area_desc {
    uint32_t nad_offset;    /* Flash offset of start of area. */
    uint32_t nad_length;    /* Size of area, in bytes. */
    uint8_t nad_flash_id;   /* Logical flash id */
};

/** On-disk representation of an area header. */
struct nffs_disk_area {
    uint32_t nda_magic[4];  /* NFFS_AREA_MAGIC{0,1,2,3} */
    uint32_t nda_length;    /* Total size of area, in bytes. */
    uint8_t nda_ver;        /* Current nffs version: 0 */
    uint8_t nda_gc_seq;     /* Garbage collection count. */
    uint8_t reserved8;
    uint8_t nda_id;         /* 0xff if scratch area. */
};

/** On-disk representation of an inode (file or directory). */
struct nffs_disk_inode {
    uint32_t ndi_id;            /* Unique object ID. */
    uint32_t ndi_parent_id;     /* Object ID of parent directory inode. */
    uint32_t ndi_lastblock_id;     /* Object ID of parent directory inode. */
    uint16_t ndi_seq;           /* Sequence number; greater supersedes
                                   lesser. */
    uint16_t reserved16;
    uint8_t ndi_flags;            /* flags */
    uint8_t ndi_filename_len;   /* Length of filename, in bytes. */
    uint16_t ndi_crc16;         /* Covers rest of header and filename. */
    /* Followed by filename. */
};

#define NFFS_DISK_INODE_OFFSET_CRC  18

/** On-disk representation of a data block. */
struct nffs_disk_block {
    uint32_t ndb_id;        /* Unique object ID. */
    uint32_t ndb_inode_id;  /* Object ID of owning inode. */
    uint32_t ndb_prev_id;   /* Object ID of previous block in file;
                               NFFS_ID_NONE if this is the first block. */
    uint16_t ndb_seq;       /* Sequence number; greater supersedes lesser. */
    uint16_t reserved16;
    uint16_t ndb_data_len;  /* Length of data contents, in bytes. */
    uint16_t ndb_crc16;     /* Covers rest of header and data. */
    /* Followed by 'ndb_data_len' bytes of data. */
};

#define NFFS_DISK_BLOCK_OFFSET_CRC  18

/**
 * What gets stored in the hash table.  Each entry represents a data block or
 * an inode.
 */
struct nffs_hash_entry {
    SLIST_ENTRY(nffs_hash_entry) nhe_next;
    uint32_t nhe_id;        /* 0 - 0x7fffffff if inode; else if block. */
    uint32_t nhe_flash_loc; /* Upper-byte = area idx; rest = area offset. */
};


SLIST_HEAD(nffs_hash_list, nffs_hash_entry);
SLIST_HEAD(nffs_inode_list, nffs_inode_entry);

/** Each inode hash entry is actually one of these. */
struct nffs_inode_entry {
    struct nffs_hash_entry nie_hash_entry;
    SLIST_ENTRY(nffs_inode_entry) nie_sibling_next;
    union {
        struct nffs_inode_list nie_child_list;           /* If directory */
        struct nffs_hash_entry *nie_last_block_entry;    /* If file */
        uint32_t nie_lastblock_id;
    };
    uint8_t nie_refcnt;
    uint8_t nie_flags;
    uint8_t nie_blkcnt;
    uint8_t reserved8;
};

#define    NFFS_INODE_FLAG_FREE        0x00
#define    NFFS_INODE_FLAG_DUMMY       0x01    /* inode is a dummy */
#define    NFFS_INODE_FLAG_DUMMYPARENT 0x02    /* parent not in cache */
#define    NFFS_INODE_FLAG_DUMMYLSTBLK 0x04    /* lastblock not in cache */
#define    NFFS_INODE_FLAG_DUMMYINOBLK 0x08    /* dummy inode for blk */
#define    NFFS_INODE_FLAG_OBSOLETE    0x10    /* always replace if same ID */
#define    NFFS_INODE_FLAG_INTREE      0x20    /* in directory structure */
#define    NFFS_INODE_FLAG_INHASH      0x40    /* in hash table */
#define    NFFS_INODE_FLAG_DELETED     0x80    /* inode deleted */

#define nie_id            nie_hash_entry.nhe_id
#define nie_flash_loc    nie_hash_entry.nhe_flash_loc

/** Full inode representation; not stored permanently RAM. */
struct nffs_inode {
    struct nffs_inode_entry *ni_inode_entry; /* Points to real inode entry. */
    uint32_t ni_seq;                         /* Sequence number; greater
                                                supersedes lesser. */
    struct nffs_inode_entry *ni_parent;      /* Points to parent directory. */
    uint8_t ni_filename_len;                 /* # chars in filename. */
    uint8_t ni_filename[NFFS_SHORT_FILENAME_LEN]; /* First 3 bytes. */
};

/** Full data block representation; not stored permanently RAM. */
struct nffs_block {
    struct nffs_hash_entry *nb_hash_entry;   /* Points to real block entry. */
    uint32_t nb_seq;                         /* Sequence number; greater
                                                supersedes lesser. */
    struct nffs_inode_entry *nb_inode_entry; /* Owning inode. */
    struct nffs_hash_entry *nb_prev;         /* Previous block in file. */
    uint16_t nb_data_len;                    /* # of data bytes in block. */
    uint16_t reserved16;
};

struct nffs_file {
    struct nffs_inode_entry *nf_inode_entry;
    uint32_t nf_offset;
    uint8_t nf_access_flags;
};

struct nffs_area {
    uint32_t na_offset;
    uint32_t na_length;
    uint32_t na_cur;
    uint16_t na_id;
    uint8_t na_gc_seq;
    uint8_t na_flash_id;
    uint32_t na_obsolete;   /* deleted bytecount */
};

struct nffs_disk_object {
    int ndo_type;
    uint8_t ndo_area_idx;
    uint32_t ndo_offset;
    union {
        struct nffs_disk_inode ndo_disk_inode;
        struct nffs_disk_block ndo_disk_block;
    } ndo_un_obj;
};

#define ndo_disk_inode    ndo_un_obj.ndo_disk_inode
#define ndo_disk_block    ndo_un_obj.ndo_disk_block

struct nffs_seek_info {
    struct nffs_block nsi_last_block;
    uint32_t nsi_block_file_off;
    uint32_t nsi_file_len;
};

#define NFFS_OBJECT_TYPE_INODE   1
#define NFFS_OBJECT_TYPE_BLOCK   2

#define NFFS_PATH_TOKEN_NONE     0
#define NFFS_PATH_TOKEN_BRANCH   1
#define NFFS_PATH_TOKEN_LEAF     2

struct nffs_path_parser {
    int npp_token_type;
    const char *npp_path;
    const char *npp_token;
    int npp_token_len;
    int npp_off;
};

/** Represents a single cached data block. */
struct nffs_cache_block {
    TAILQ_ENTRY(nffs_cache_block) ncb_link; /* Next / prev cached block. */
    struct nffs_block ncb_block;            /* Full data block. */
    uint32_t ncb_file_offset;               /* File offset of this block. */
};

TAILQ_HEAD(nffs_cache_block_list, nffs_cache_block);

/** Represents a single cached file inode. */
struct nffs_cache_inode {
    TAILQ_ENTRY(nffs_cache_inode) nci_link;        /* Sorted; LRU at tail. */
    struct nffs_inode nci_inode;                   /* Full inode. */
    struct nffs_cache_block_list nci_block_list;   /* List of cached blocks. */
    uint32_t nci_file_size;                        /* Total file size. */
};

struct nffs_dirent {
    struct fs_ops *fops;
    struct nffs_inode_entry *nde_inode_entry;
};

struct nffs_dir {
    struct nffs_inode_entry *nd_parent_inode_entry;
    struct nffs_dirent nd_dirent;
};

extern void *nffs_file_mem;
extern void *nffs_block_entry_mem;
extern void *nffs_inode_mem;
extern void *nffs_cache_inode_mem;
extern void *nffs_cache_block_mem;
extern void *nffs_dir_mem;
extern uint32_t nffs_hash_next_file_id;
extern uint32_t nffs_hash_next_dir_id;
extern uint32_t nffs_hash_next_block_id;
extern struct nffs_area nffs_areas[CONFIG_NFFS_FILESYSTEM_MAX_AREAS];
extern uint8_t nffs_num_areas;
extern uint8_t nffs_scratch_area_idx;
extern uint16_t nffs_block_max_data_sz;
extern unsigned int nffs_gc_count;
extern struct nffs_area_desc *nffs_current_area_descs;

#define NFFS_FLASH_BUF_SZ        256
extern uint8_t nffs_flash_buf[NFFS_FLASH_BUF_SZ];

extern struct nffs_hash_list *nffs_hash;
extern struct nffs_inode_entry *nffs_root_dir;
extern struct nffs_inode_entry *nffs_lost_found_dir;

extern struct log nffs_log;

/* @area */
int nffs_area_magic_is_set(const struct nffs_disk_area *disk_area);
int nffs_area_is_scratch(const struct nffs_disk_area *disk_area);
int nffs_area_is_current_version(const struct nffs_disk_area *disk_area);
void nffs_area_to_disk(const struct nffs_area *area,
                       struct nffs_disk_area *out_disk_area);
uint32_t nffs_area_free_space(const struct nffs_area *area);
int nffs_area_find_corrupt_scratch(uint16_t *out_good_idx,
                                   uint16_t *out_bad_idx);

/* @block */
struct nffs_hash_entry *nffs_block_entry_alloc(void);
void nffs_block_entry_free(struct nffs_hash_entry *entry);
int nffs_block_entry_reserve(struct nffs_hash_entry **out_block_entry);
int nffs_block_read_disk(uint8_t area_idx, uint32_t area_offset,
                         struct nffs_disk_block *out_disk_block);
int nffs_block_write_disk(const struct nffs_disk_block *disk_block,
                          const void *data,
                          uint8_t *out_area_idx, uint32_t *out_area_offset);
int nffs_block_delete_from_ram(struct nffs_hash_entry *entry);
void nffs_block_delete_list_from_ram(struct nffs_block *first,
                                     struct nffs_block *last);
void nffs_block_delete_list_from_disk(const struct nffs_block *first,
                                      const struct nffs_block *last);
void nffs_block_to_disk(const struct nffs_block *block,
                        struct nffs_disk_block *out_disk_block);
int nffs_block_find_predecessor(struct nffs_hash_entry *start,
                                uint32_t sought_id);
int nffs_block_from_hash_entry_no_ptrs(struct nffs_block *out_block,
                                       struct nffs_hash_entry *entry);
int nffs_block_from_hash_entry(struct nffs_block *out_block,
                               struct nffs_hash_entry *entry);
int nffs_block_read_data(const struct nffs_block *block, uint16_t offset,
                         uint16_t length, void *dst);
int nffs_block_is_dummy(struct nffs_hash_entry *entry);

/* @cache */
void nffs_cache_inode_delete(const struct nffs_inode_entry *inode_entry);
int nffs_cache_inode_ensure(struct nffs_cache_inode **out_entry,
                            struct nffs_inode_entry *inode_entry);
int nffs_cache_inode_refresh(void);
void nffs_cache_inode_range(const struct nffs_cache_inode *cache_inode,
                            uint32_t *out_start, uint32_t *out_end);
int nffs_cache_seek(struct nffs_cache_inode *cache_inode, uint32_t to,
                    struct nffs_cache_block **out_cache_block);
void nffs_cache_clear(void);

/* @crc */
int nffs_crc_flash(uint16_t initial_crc, uint8_t area_idx,
                   uint32_t area_offset, uint32_t len, uint16_t *out_crc);
uint16_t nffs_crc_disk_block_hdr(const struct nffs_disk_block *disk_block);
int nffs_crc_disk_block_validate(const struct nffs_disk_block *disk_block,
                                uint8_t area_idx, uint32_t area_offset);
void nffs_crc_disk_block_fill(struct nffs_disk_block *disk_block,
                              const void *data);
int nffs_crc_disk_inode_validate(const struct nffs_disk_inode *disk_inode,
                                 uint8_t area_idx, uint32_t area_offset);
void nffs_crc_disk_inode_fill(struct nffs_disk_inode *disk_inode,
                              const char *filename);

/* @config */
void nffs_config_init(void);

/* @dir */
int nffs_dir_open(const char *path, struct nffs_dir **out_dir);
int nffs_dir_read(struct nffs_dir *dir, struct nffs_dirent **out_dirent);
int nffs_dir_close(struct nffs_dir *dir);

/* @file */
int nffs_file_open(struct nffs_file **out_file, const char *filename,
                   uint8_t access_flags);
int nffs_file_seek(struct nffs_file *file, uint32_t offset);
int nffs_file_read(struct nffs_file *file, uint32_t len, void *out_data,
                   uint32_t *out_len);
int nffs_file_close(struct nffs_file *file);
int nffs_file_new(struct nffs_inode_entry *parent, const char *filename,
                  uint8_t filename_len, int is_dir,
                  struct nffs_inode_entry **out_inode_entry);

/* @format */
int nffs_format_area(uint8_t area_idx, int is_scratch);
int nffs_format_from_scratch_area(uint8_t area_idx, uint8_t area_id);
int nffs_format_full(const struct nffs_area_desc *area_descs);

/* @gc */
int nffs_gc(uint8_t *out_area_idx);
int nffs_gc_until(uint32_t space, uint8_t *out_area_idx);

/* @flash */
struct nffs_area *nffs_flash_find_area(uint16_t logical_id);
int nffs_flash_read(uint8_t area_idx, uint32_t offset,
                    void *data, uint32_t len);
int nffs_flash_write(uint8_t area_idx, uint32_t offset,
                     const void *data, uint32_t len);
int nffs_flash_copy(uint8_t area_id_from, uint32_t offset_from,
                    uint8_t area_id_to, uint32_t offset_to,
                    uint32_t len);
uint32_t nffs_flash_loc(uint8_t area_idx, uint32_t offset);
void nffs_flash_loc_expand(uint32_t flash_loc, uint8_t *out_area_idx,
                           uint32_t *out_area_offset);

/* @hash */
int nffs_hash_id_is_dir(uint32_t id);
int nffs_hash_id_is_file(uint32_t id);
int nffs_hash_id_is_inode(uint32_t id);
int nffs_hash_id_is_block(uint32_t id);
struct nffs_hash_entry *nffs_hash_find(uint32_t id);
struct nffs_inode_entry *nffs_hash_find_inode(uint32_t id);
struct nffs_hash_entry *nffs_hash_find_block(uint32_t id);
void nffs_hash_insert(struct nffs_hash_entry *entry);
void nffs_hash_remove(struct nffs_hash_entry *entry);
int nffs_hash_init(void);
int nffs_hash_entry_is_dummy(struct nffs_hash_entry *he);
int nffs_hash_id_is_dummy(uint32_t id);

/* @inode */
struct nffs_inode_entry *nffs_inode_entry_alloc(void);
void nffs_inode_entry_free(struct nffs_inode_entry *inode_entry);
int nffs_inode_entry_reserve(struct nffs_inode_entry **out_inode_entry);
int nffs_inode_calc_data_length(struct nffs_inode_entry *inode_entry,
                                uint32_t *out_len);
int nffs_inode_data_len(struct nffs_inode_entry *inode_entry,
                        uint32_t *out_len);
uint32_t nffs_inode_parent_id(const struct nffs_inode *inode);
int nffs_inode_delete_from_disk(struct nffs_inode *inode);
int nffs_inode_entry_from_disk(struct nffs_inode_entry *out_inode,
                               const struct nffs_disk_inode *disk_inode,
                               uint8_t area_idx, uint32_t offset);
int nffs_inode_rename(struct nffs_inode_entry *inode_entry,
                      struct nffs_inode_entry *new_parent,
                      const char *new_filename);
int nffs_inode_update(struct nffs_inode_entry *inode_entry);
void nffs_inode_insert_block(struct nffs_inode *inode,
                             struct nffs_block *block);
int nffs_inode_read_disk(uint8_t area_idx, uint32_t offset,
                         struct nffs_disk_inode *out_disk_inode);
int nffs_inode_write_disk(const struct nffs_disk_inode *disk_inode,
                          const char *filename, uint8_t area_idx,
                          uint32_t offset);
int nffs_inode_inc_refcnt(struct nffs_inode_entry *inode_entry);
int nffs_inode_dec_refcnt(struct nffs_inode_entry *inode_entry);
int nffs_inode_add_child(struct nffs_inode_entry *parent,
                         struct nffs_inode_entry *child);
void nffs_inode_remove_child(struct nffs_inode *child);
int nffs_inode_is_root(const struct nffs_disk_inode *disk_inode);
int nffs_inode_read_filename(struct nffs_inode_entry *inode_entry,
                             size_t max_len, char *out_name,
                             uint8_t *out_full_len);
int nffs_inode_filename_cmp_ram(const struct nffs_inode *inode,
                                const char *name, int name_len,
                                int *result);
int nffs_inode_filename_cmp_flash(const struct nffs_inode *inode1,
                                  const struct nffs_inode *inode2,
                                  int *result);
int nffs_inode_read(struct nffs_inode_entry *inode_entry, uint32_t offset,
                    uint32_t len, void *data, uint32_t *out_len);
int nffs_inode_seek(struct nffs_inode_entry *inode_entry, uint32_t offset,
                    uint32_t length, struct nffs_seek_info *out_seek_info);
int nffs_inode_from_entry(struct nffs_inode *out_inode,
                          struct nffs_inode_entry *entry);
int nffs_inode_unlink_from_ram(struct nffs_inode *inode,
                               struct nffs_hash_entry **out_next);
int nffs_inode_unlink_from_ram_corrupt_ok(struct nffs_inode *inode,
                                          struct nffs_hash_entry **out_next);
int nffs_inode_unlink(struct nffs_inode *inode);
int nffs_inode_is_dummy(struct nffs_inode_entry *inode_entry);
int nffs_inode_is_deleted(struct nffs_inode_entry *inode_entry);
int nffs_inode_setflags(struct nffs_inode_entry *entry, uint8_t flag);
int nffs_inode_unsetflags(struct nffs_inode_entry *entry, uint8_t flag);
int nffs_inode_getflags(struct nffs_inode_entry *entry, uint8_t flag);

/* @misc */
int nffs_misc_gc_if_oom(void *resource, int *out_rc);
int nffs_misc_reserve_space(uint16_t space,
                            uint8_t *out_area_idx, uint32_t *out_area_offset);
int nffs_misc_set_num_areas(uint8_t num_areas);
int nffs_misc_validate_root_dir(void);
int nffs_misc_validate_scratch(void);
int nffs_misc_create_lost_found_dir(void);
int nffs_misc_set_max_block_data_len(uint16_t min_data_len);
int nffs_misc_reset(void);
int nffs_misc_ready(void);
int nffs_misc_desc_from_flash_area(const struct nffs_flash_desc *flash,
                                   int *cnt, struct nffs_area_desc *nad);

/* @path */
int nffs_path_parse_next(struct nffs_path_parser *parser);
void nffs_path_parser_new(struct nffs_path_parser *parser, const char *path);
int nffs_path_find(struct nffs_path_parser *parser,
                   struct nffs_inode_entry **out_inode_entry,
                   struct nffs_inode_entry **out_parent);
int nffs_path_find_inode_entry(const char *filename,
                               struct nffs_inode_entry **out_inode_entry);
int nffs_path_unlink(const char *filename);
int nffs_path_rename(const char *from, const char *to);
int nffs_path_new_dir(const char *path,
                      struct nffs_inode_entry **out_inode_entry);

/* @restore */
int nffs_restore_full(const struct nffs_area_desc *area_descs);

/* @write */
int nffs_write_to_file(struct nffs_file *file, const void *data, int len);


#define NFFS_HASH_FOREACH(entry, i, next)                               \
    for ((i) = 0; (i) < NFFS_HASH_SIZE; (i)++)                          \
        for ((entry) = SLIST_FIRST(nffs_hash + (i));                    \
             (entry) && (((next)) = SLIST_NEXT((entry), nhe_next), 1);  \
             (entry) = ((next)))

#define NFFS_FLASH_LOC_NONE  nffs_flash_loc(NFFS_AREA_ID_NONE, 0)

#if __ZEPHYR__

#define NFFS_LOG(lvl, ...)
#define MYNEWT_VAL(val) 0
#define ASSERT_IF_TEST(cond)
#define STATS_INC(sectvarname, var)

#else

/* Default to Mynewt */

#define NFFS_LOG(lvl, ...) \
    LOG_ ## lvl(&nffs_log, LOG_MODULE_NFFS, __VA_ARGS__)
#endif

#ifdef __cplusplus
}
#endif

#endif
