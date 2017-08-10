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

#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <nffs/nffs.h>
#include <nffs/os.h>

struct nffs_hash_entry *
nffs_block_entry_alloc(void)
{
    struct nffs_hash_entry *entry;

    entry = nffs_os_mempool_get(&nffs_block_entry_pool);
    if (entry != NULL) {
        memset(entry, 0, sizeof *entry);
    }

    return entry;
}

void
nffs_block_entry_free(struct nffs_hash_entry *block_entry)
{
    assert(nffs_hash_id_is_block(block_entry->nhe_id));
    nffs_os_mempool_free(&nffs_block_entry_pool, block_entry);
}

/**
 * Allocates a block entry.  If allocation fails due to memory exhaustion,
 * garbage collection is performed and the allocation is retried.  This
 * process is repeated until allocation is successful or all areas have been
 * garbage collected.
 *
 * @param out_block_entry           On success, the address of the allocated
 *                                      block gets written here.
 *
 * @return                          0 on successful allocation;
 *                                  FS_ENOMEM on memory exhaustion;
 *                                  other nonzero on garbage collection error.
 */
int
nffs_block_entry_reserve(struct nffs_hash_entry **out_block_entry)
{
    int rc;

    do {
        *out_block_entry = nffs_block_entry_alloc();
    } while (nffs_misc_gc_if_oom(*out_block_entry, &rc));

    return rc;
}

/**
 * Reads a data block header from flash.
 *
 * @param area_idx              The index of the area to read from.
 * @param area_offset           The offset within the area to read from.
 * @param out_disk_block        On success, the block header is writteh here.
 *
 * @return                      0 on success;
 *                              FS_EOFFSET on an attempt to read an invalid
 *                                  address range;
 *                              FS_EHW on flash error;
 *                              FS_EUNEXP if the specified disk location does
 *                                  not contain a block.
 */
int
nffs_block_read_disk(uint8_t area_idx, uint32_t area_offset,
                     struct nffs_disk_block *out_disk_block)
{
    int rc;

    STATS_INC(nffs_stats, nffs_readcnt_block);
    rc = nffs_flash_read(area_idx, area_offset, out_disk_block,
                         sizeof *out_disk_block);
    if (rc != 0) {
        return rc;
    }
    if (!nffs_hash_id_is_block(out_disk_block->ndb_id)) {
        return FS_EUNEXP;
    }

    return 0;
}

/**
 * Writes the specified data block to a suitable location in flash.
 *
 * @param disk_block            Points to the disk block to write.
 * @param data                  The contents of the data block.
 * @param out_area_idx          On success, contains the index of the area
 *                                  written to.
 * @param out_area_offset       On success, contains the offset within the area
 *                                  written to.
 *
 * @return                      0 on success; nonzero on failure.
 */
int
nffs_block_write_disk(const struct nffs_disk_block *disk_block,
                      const void *data,
                      uint8_t *out_area_idx, uint32_t *out_area_offset)
{
    uint32_t area_offset;
    uint8_t area_idx;
    int rc;

    rc = nffs_misc_reserve_space(sizeof *disk_block + disk_block->ndb_data_len,
                                &area_idx, &area_offset);
    if (rc != 0) {
        return rc;
    }

    rc = nffs_flash_write(area_idx, area_offset, disk_block,
                         sizeof *disk_block);
    if (rc != 0) {
        return rc;
    }

    if (disk_block->ndb_data_len > 0) {
        rc = nffs_flash_write(area_idx, area_offset + sizeof *disk_block,
                             data, disk_block->ndb_data_len);
        if (rc != 0) {
            return rc;
        }
    }

    *out_area_idx = area_idx;
    *out_area_offset = area_offset;

    ASSERT_IF_TEST(nffs_crc_disk_block_validate(disk_block, area_idx,
                                               area_offset) == 0);

    return 0;
}

static void
nffs_block_from_disk_no_ptrs(struct nffs_block *out_block,
                             const struct nffs_disk_block *disk_block)
{
    out_block->nb_seq = disk_block->ndb_seq;
    out_block->nb_inode_entry = NULL;
    out_block->nb_prev = NULL;
    out_block->nb_data_len = disk_block->ndb_data_len;
}

/**
 * Constructs a block representation from a disk record.  If the disk block
 * references other objects (inode or previous data block), the resulting block
 * object is populated with pointers to the referenced objects.  If the any
 * referenced objects are not present in the NFFS RAM representation, this
 * indicates file system corruption.  In this case, the resulting block is
 * populated with all valid references, and an FS_ECORRUPT code is returned.
 *
 * @param out_block             The resulting block is written here (regardless
 *                                  of this function's return code).
 * @param disk_block            The source disk record to convert.
 *
 * @return                      0 if the block was successfully constructed;
 *                              FS_ECORRUPT if one or more pointers could not
 *                                  be filled in due to file system corruption.
 */
static int
nffs_block_from_disk(struct nffs_block *out_block,
                     const struct nffs_disk_block *disk_block)
{
    int rc;

    rc = 0;

    nffs_block_from_disk_no_ptrs(out_block, disk_block);

    out_block->nb_inode_entry = nffs_hash_find_inode(disk_block->ndb_inode_id);
    if (out_block->nb_inode_entry == NULL) {
        rc = FS_ECORRUPT;
    }

    if (disk_block->ndb_prev_id != NFFS_ID_NONE) {
        out_block->nb_prev = nffs_hash_find_block(disk_block->ndb_prev_id);
        if (out_block->nb_prev == NULL) {
            rc = FS_ECORRUPT;
        }
    }

    return rc;
}

/**
 * Constructs a disk-representation of the specified data block.
 *
 * @param block                 The source block to convert.
 * @param out_disk_block        The disk block to write to.
 */
void
nffs_block_to_disk(const struct nffs_block *block,
                   struct nffs_disk_block *out_disk_block)
{
    assert(block->nb_inode_entry != NULL);

    memset(out_disk_block, 0, sizeof *out_disk_block);
    out_disk_block->ndb_id = block->nb_hash_entry->nhe_id;
    out_disk_block->ndb_seq = block->nb_seq;
    out_disk_block->ndb_inode_id =
        block->nb_inode_entry->nie_hash_entry.nhe_id;
    if (block->nb_prev == NULL) {
        out_disk_block->ndb_prev_id = NFFS_ID_NONE;
    } else {
        out_disk_block->ndb_prev_id = block->nb_prev->nhe_id;
    }
    out_disk_block->ndb_data_len = block->nb_data_len;
}

/**
 * Deletes the specified block entry from the nffs RAM representation.
 *
 * @param block_entry           The block entry to delete.
 *
 * @return                      0 on success; nonzero on failure.
 */
int
nffs_block_delete_from_ram(struct nffs_hash_entry *block_entry)
{
    struct nffs_inode_entry *inode_entry;
    struct nffs_block block;
    int rc;

    if (nffs_hash_entry_is_dummy(block_entry)) {
        /*
         * it's very limited to what we can do here as the block doesn't have
         * any way to get to the inode via hash entry. Just delete the
         * block and return FS_ECORRUPT
         */
        nffs_hash_remove(block_entry);
        nffs_block_entry_free(block_entry);
        return FS_ECORRUPT;
    }

    rc = nffs_block_from_hash_entry(&block, block_entry);
    if (rc == 0 || rc == FS_ECORRUPT) {
        /* If file system corruption was detected, the resulting block is still
         * valid and can be removed from RAM.
         * Note that FS_CORRUPT can occur because the owning inode was not
         * found in the hash table - this can occur during the sweep where
         * the inodes were deleted ahead of the blocks.
         */
        inode_entry = block.nb_inode_entry;
        if (inode_entry != NULL &&
            inode_entry->nie_last_block_entry == block_entry) {

            inode_entry->nie_last_block_entry = block.nb_prev;
        }

        nffs_hash_remove(block_entry);
        nffs_block_entry_free(block_entry);
    }

    return rc;
}

/**
 * Determines if a particular block can be found in RAM by following a chain of
 * previous block pointers, starting with the specified hash entry.
 *
 * @param start                 The block entry at which to start the search.
 * @param sought_id             The ID of the block to search for.
 *
 * @return                      0 if the sought after ID was found;
 *                              FS_ENOENT if the ID was not found;
 *                              Other FS code on error.
 */
int
nffs_block_find_predecessor(struct nffs_hash_entry *start, uint32_t sought_id)
{
    struct nffs_hash_entry *entry;
    struct nffs_disk_block disk_block;
    uint32_t area_offset;
    uint8_t area_idx;
    int rc;

    entry = start;
    while (entry != NULL && entry->nhe_id != sought_id) {
        nffs_flash_loc_expand(entry->nhe_flash_loc, &area_idx, &area_offset);
        rc = nffs_block_read_disk(area_idx, area_offset, &disk_block);
        if (rc != 0) {
            return rc;
        }

        if (disk_block.ndb_prev_id == NFFS_ID_NONE) {
            entry = NULL;
        } else {
            entry = nffs_hash_find(disk_block.ndb_prev_id);
        }
    }

    if (entry == NULL) {
        rc = FS_ENOENT;
    } else {
        rc = 0;
    }

    return rc;
}

/**
 * Constructs a full data block representation from the specified minimal
 * block entry.  However, the resultant block's pointers are set to null,
 * rather than populated via hash table lookups.  This behavior is useful when
 * the RAM representation has not been fully constructed yet.
 *
 * @param out_block             On success, this gets populated with the data
 *                                  block information.
 * @param block_entry           The source block entry to convert.
 *
 * @return                      0 on success; nonzero on failure.
 */
int
nffs_block_from_hash_entry_no_ptrs(struct nffs_block *out_block,
                                   struct nffs_hash_entry *block_entry)
{
    struct nffs_disk_block disk_block;
    uint32_t area_offset;
    uint8_t area_idx;
    int rc;

    assert(nffs_hash_id_is_block(block_entry->nhe_id));

    memset(out_block, 0, sizeof *out_block);

    if (nffs_hash_entry_is_dummy(block_entry)) {
        /*
         * We can't read this from disk so we'll be missing filling in anything
         * not already in inode_entry (e.g., prev_id).
         */
        out_block->nb_hash_entry = block_entry;
        return FS_ENOENT; /* let caller know it's a partial inode_entry */
    }

    nffs_flash_loc_expand(block_entry->nhe_flash_loc, &area_idx, &area_offset);
    rc = nffs_block_read_disk(area_idx, area_offset, &disk_block);
    if (rc != 0) {
        return rc;
    }

    out_block->nb_hash_entry = block_entry;
    nffs_block_from_disk_no_ptrs(out_block, &disk_block);

    return 0;
}

/**
 * Constructs a block representation from a minimal block hash entry.  If the
 * hash entry references other objects (inode or previous data block), the
 * resulting block object is populated with pointers to the referenced objects.
 * If the any referenced objects are not present in the NFFS RAM
 * representation, this indicates file system corruption.  In this case, the
 * resulting block is populated with all valid references, and an FS_ECORRUPT
 * code is returned.
 *
 * @param out_block             On success, this gets populated with the data
 *                                  block information.
 * @param block_entry           The source block entry to convert.
 *
 * @return                      0 on success;
 *                              FS_ECORRUPT if one or more pointers could not
 *                                  be filled in due to file system corruption;
 *                              FS_EOFFSET on an attempt to read an invalid
 *                                  address range;
 *                              FS_EHW on flash error;
 *                              FS_EUNEXP if the specified disk location does
 *                                  not contain a block.
 */
int
nffs_block_from_hash_entry(struct nffs_block *out_block,
                           struct nffs_hash_entry *block_entry)
{
    struct nffs_disk_block disk_block;
    uint32_t area_offset;
    uint8_t area_idx;
    int rc;

    assert(nffs_hash_id_is_block(block_entry->nhe_id));

    if (nffs_block_is_dummy(block_entry)) {
        out_block->nb_hash_entry = block_entry;
        out_block->nb_inode_entry = NULL;
        out_block->nb_prev = NULL;
        /*
         * Dummy block added when inode was read in before real block
         * (see nffs_restore_inode()). Return success (because there's 
         * too many places that ned to check for this,
         * but it's the responsibility fo the upstream code to check
         * whether this is still a dummy entry.  XXX
         */
        return 0;
        /*return FS_ENOENT;*/
    }
    nffs_flash_loc_expand(block_entry->nhe_flash_loc, &area_idx, &area_offset);
    rc = nffs_block_read_disk(area_idx, area_offset, &disk_block);
    if (rc != 0) {
        return rc;
    }

    out_block->nb_hash_entry = block_entry;
    rc = nffs_block_from_disk(out_block, &disk_block);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

int
nffs_block_read_data(const struct nffs_block *block, uint16_t offset,
                     uint16_t length, void *dst)
{
    uint32_t area_offset;
    uint8_t area_idx;
    int rc;

    nffs_flash_loc_expand(block->nb_hash_entry->nhe_flash_loc,
                         &area_idx, &area_offset);
    area_offset += sizeof (struct nffs_disk_block);
    area_offset += offset;

    STATS_INC(nffs_stats, nffs_readcnt_data);
    rc = nffs_flash_read(area_idx, area_offset, dst, length);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

int
nffs_block_is_dummy(struct nffs_hash_entry *entry)
{
    return (entry->nhe_flash_loc == NFFS_FLASH_LOC_NONE);
}
