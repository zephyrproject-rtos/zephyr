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

#include <assert.h>
#include <string.h>
#include <nffs/nffs.h>
#include <nffs/os.h>

static int
nffs_write_fill_crc16_overwrite(struct nffs_disk_block *disk_block,
                                uint8_t src_area_idx, uint32_t src_area_offset,
                                uint16_t left_copy_len, uint16_t right_copy_len,
                                const void *new_data, uint16_t new_data_len)
{
    uint16_t block_off;
    uint16_t crc16;
    int rc;

    block_off = 0;

    crc16 = nffs_crc_disk_block_hdr(disk_block);
    block_off += sizeof *disk_block;

    /* Copy data from the start of the old block, in case the new data starts
     * at a non-zero offset.
     */
    if (left_copy_len > 0) {
        rc = nffs_crc_flash(crc16, src_area_idx, src_area_offset + block_off,
                            left_copy_len, &crc16);
        if (rc != 0) {
            return rc;
        }
        block_off += left_copy_len;
    }

    /* Write the new data into the data block.  This may extend the block's
     * length beyond its old value.
     */
    crc16 = nffs_os_crc16_ccitt(crc16, new_data, new_data_len, 0);
    block_off += new_data_len;

    /* Copy data from the end of the old block, in case the new data doesn't
     * extend to the end of the block.
     */
    if (right_copy_len > 0) {
        rc = nffs_crc_flash(crc16, src_area_idx, src_area_offset + block_off,
                            right_copy_len, &crc16);
        if (rc != 0) {
            return rc;
        }
        block_off += right_copy_len;
    }

    assert(block_off == sizeof *disk_block + disk_block->ndb_data_len);

    /* Finish CRC */
    crc16 = nffs_os_crc16_ccitt(crc16, NULL, 0, 1);

    disk_block->ndb_crc16 = crc16;

    return 0;
}

/**
 * Overwrites an existing data block.  The resulting block has the same ID as
 * the old one, but it supersedes it with a greater sequence number.
 *
 * @param entry                 The data block to overwrite.
 * @param left_copy_len         The number of bytes of existing data to retain
 *                                  before the new data begins.
 * @param new_data              The new data to write to the block.
 * @param new_data_len          The number of new bytes to write to the block.
 *                                  If this value plus left_copy_len is less
 *                                  than the existing block's data length,
 *                                  previous data at the end of the block is
 *                                  retained.
 *
 * @return                      0 on success; nonzero on failure.
 */
static int
nffs_write_over_block(struct nffs_hash_entry *entry, uint16_t left_copy_len,
                      const void *new_data, uint16_t new_data_len)
{
    struct nffs_disk_block disk_block;
    struct nffs_block block;
    uint32_t src_area_offset;
    uint32_t dst_area_offset;
    uint16_t right_copy_len;
    uint16_t block_off;
    uint8_t src_area_idx;
    uint8_t dst_area_idx;
    int rc;

    rc = nffs_block_from_hash_entry(&block, entry);
    if (rc != 0) {
        return rc;
    }

    assert(left_copy_len <= block.nb_data_len);

    /* Determine how much old data at the end of the block needs to be
     * retained.  If the new data doesn't extend to the end of the block, the
     * the rest of the block retains its old contents.
     */
    if (left_copy_len + new_data_len > block.nb_data_len) {
        right_copy_len = 0;
    } else {
        right_copy_len = block.nb_data_len - left_copy_len - new_data_len;
    }

    block.nb_seq++;
    block.nb_data_len = left_copy_len + new_data_len + right_copy_len;
    nffs_block_to_disk(&block, &disk_block);

    nffs_flash_loc_expand(entry->nhe_flash_loc,
                          &src_area_idx, &src_area_offset);

    rc = nffs_write_fill_crc16_overwrite(&disk_block,
                                         src_area_idx, src_area_offset,
                                         left_copy_len, right_copy_len,
                                         new_data, new_data_len);
    if (rc != 0) {
        return rc;
    }

    rc = nffs_misc_reserve_space(sizeof disk_block + disk_block.ndb_data_len,
                                 &dst_area_idx, &dst_area_offset);
    if (rc != 0) {
        return rc;
    }

    block_off = 0;

    /* Write the block header. */
    rc = nffs_flash_write(dst_area_idx, dst_area_offset + block_off,
                          &disk_block, sizeof disk_block);
    if (rc != 0) {
        return rc;
    }
    block_off += sizeof disk_block;

    /* Copy data from the start of the old block, in case the new data starts
     * at a non-zero offset.
     */
    if (left_copy_len > 0) {
        rc = nffs_flash_copy(src_area_idx, src_area_offset + block_off,
                             dst_area_idx, dst_area_offset + block_off,
                             left_copy_len);
        if (rc != 0) {
            return rc;
        }
        block_off += left_copy_len;
    }

    /* Write the new data into the data block.  This may extend the block's
     * length beyond its old value.
     */
    rc = nffs_flash_write(dst_area_idx, dst_area_offset + block_off,
                          new_data, new_data_len);
    if (rc != 0) {
        return rc;
    }
    block_off += new_data_len;

    /* Copy data from the end of the old block, in case the new data doesn't
     * extend to the end of the block.
     */
    if (right_copy_len > 0) {
        rc = nffs_flash_copy(src_area_idx, src_area_offset + block_off,
                             dst_area_idx, dst_area_offset + block_off,
                             right_copy_len);
        if (rc != 0) {
            return rc;
        }
        block_off += right_copy_len;
    }

    assert(block_off == sizeof disk_block + block.nb_data_len);

    entry->nhe_flash_loc = nffs_flash_loc(dst_area_idx, dst_area_offset);

    ASSERT_IF_TEST(nffs_crc_disk_block_validate(&disk_block, dst_area_idx,
                                                dst_area_offset) == 0);

    return 0;
}

/**
 * Appends a new block to an inode block chain.
 *
 * @param inode_entry           The inode to append a block to.
 * @param data                  The contents of the new block.
 * @param len                   The number of bytes of data to write.
 *
 * @return                      0 on success; nonzero on failure.
 */
static int
nffs_write_append(struct nffs_cache_inode *cache_inode, const void *data,
                  uint16_t len)
{
    struct nffs_inode_entry *inode_entry;
    struct nffs_hash_entry *entry;
    struct nffs_disk_block disk_block;
    uint32_t area_offset;
    uint8_t area_idx;
    int rc;

    rc = nffs_block_entry_reserve(&entry);
    if (entry == NULL) {
        return FS_ENOMEM;
    }

    inode_entry = cache_inode->nci_inode.ni_inode_entry;

    memset(&disk_block, 0, sizeof disk_block);
    disk_block.ndb_id = nffs_hash_next_block_id++;
    disk_block.ndb_seq = 0;
    disk_block.ndb_inode_id = inode_entry->nie_hash_entry.nhe_id;
    if (inode_entry->nie_last_block_entry == NULL) {
        disk_block.ndb_prev_id = NFFS_ID_NONE;
    } else {
        disk_block.ndb_prev_id = inode_entry->nie_last_block_entry->nhe_id;
    }
    disk_block.ndb_data_len = len;
    nffs_crc_disk_block_fill(&disk_block, data);

    rc = nffs_block_write_disk(&disk_block, data, &area_idx, &area_offset);
    if (rc != 0) {
        return rc;
    }

    entry->nhe_id = disk_block.ndb_id;
    entry->nhe_flash_loc = nffs_flash_loc(area_idx, area_offset);
    nffs_hash_insert(entry);

    inode_entry->nie_last_block_entry = entry;

    /*
     * When a new block is appended to a file, the inode must be written
     * out to flash so the last block id is is stored persistently.
     * Need to be written atomically with writing the block out so filesystem
     * remains consistent. nffs_lock is held in nffs_write().
     * The inode will not be updated if it's been unlinked on disk and this
     * is signaled by setting the hash entry's flash location to NONE
     */
    if (inode_entry->nie_hash_entry.nhe_flash_loc != NFFS_FLASH_LOC_NONE) {
        rc = nffs_inode_update(inode_entry);
    }

    /* Update cached inode with the new file size. */
    cache_inode->nci_file_size += len;

    /* Add appended block to the cache. */
    nffs_cache_seek(cache_inode, cache_inode->nci_file_size - 1, NULL);

    return 0;
}

/**
 * Performs a single write operation.  The data written must be no greater
 * than the maximum block data length.  If old data gets overwritten, then
 * the existing data blocks are superseded as necessary.
 *
 * @param write_info            Describes the write operation being perfomred.
 * @param inode_entry           The file inode to write to.
 * @param data                  The new data to write.
 * @param data_len              The number of bytes of new data to write.
 *
 * @return                      0 on success; nonzero on failure.
 */
static int
nffs_write_chunk(struct nffs_inode_entry *inode_entry, uint32_t file_offset,
                 const void *data, uint16_t data_len)
{
    struct nffs_cache_inode *cache_inode;
    struct nffs_cache_block *cache_block;
    unsigned int gc_count;
    uint32_t append_len;
    uint32_t data_offset;
    uint32_t block_end;
    uint32_t dst_off;
    uint16_t chunk_off;
    uint16_t chunk_sz;
    int rc;

    assert(data_len <= nffs_block_max_data_sz);

    rc = nffs_cache_inode_ensure(&cache_inode, inode_entry);
    if (rc != 0) {
        return rc;
    }

    /** Handle the simple append case first. */
    if (file_offset == cache_inode->nci_file_size) {
        rc = nffs_write_append(cache_inode, data, data_len);
        return rc;
    }

    /** This is not an append; i.e., old data is getting overwritten. */

    dst_off = file_offset + data_len;
    data_offset = data_len;
    cache_block = NULL;

    if (dst_off > cache_inode->nci_file_size) {
        append_len = dst_off - cache_inode->nci_file_size;
    } else {
        append_len = 0;
    }

    do {
        if (cache_block == NULL) {
            rc = nffs_cache_seek(cache_inode, dst_off - 1, &cache_block);
            if (rc != 0) {
                return rc;
            }
        }

        if (cache_block->ncb_file_offset < file_offset) {
            chunk_off = file_offset - cache_block->ncb_file_offset;
        } else {
            chunk_off = 0;
        }

        chunk_sz = cache_block->ncb_block.nb_data_len - chunk_off;
        block_end = cache_block->ncb_file_offset +
                    cache_block->ncb_block.nb_data_len;
        if (block_end != dst_off) {
            chunk_sz += (int)(dst_off - block_end);
        }

        /* Remember the current garbage collection count.  If the count
         * increases during the write, then the block cache has been
         * invalidated and we need to reset our pointers.
         */
        gc_count = nffs_gc_count;

        data_offset = cache_block->ncb_file_offset + chunk_off - file_offset;
        rc = nffs_write_over_block(cache_block->ncb_block.nb_hash_entry,
                                   chunk_off, data + data_offset, chunk_sz);
        if (rc != 0) {
            return rc;
        }

        dst_off -= chunk_sz;

        if (gc_count != nffs_gc_count) {
            /* Garbage collection occurred; the current cached block pointer is
             * invalid, so reset it.  The cached inode is still valid.
             */
            cache_block = NULL;
        } else {
            cache_block = TAILQ_PREV(cache_block, nffs_cache_block_list,
                                     ncb_link);
        }
    } while (data_offset > 0);

    cache_inode->nci_file_size += append_len;
    return 0;
}

/**
 * Writes a chunk of contiguous data to a file.
 *
 * @param file                  The file to write to.
 * @param data                  The data to write.
 * @param len                   The length of data to write.
 *
 * @return                      0 on success; nonzero on failure.
 */
int
nffs_write_to_file(struct nffs_file *file, const void *data, int len)
{
    struct nffs_cache_inode *cache_inode;
    const uint8_t *data_ptr;
    uint16_t chunk_size;
    int rc;

    if (!(file->nf_access_flags & FS_ACCESS_WRITE)) {
        return FS_EACCESS;
    }

    if (len == 0) {
        return 0;
    }

    rc = nffs_cache_inode_ensure(&cache_inode, file->nf_inode_entry);
    if (rc != 0) {
        return rc;
    }

    /* The append flag forces all writes to the end of the file, regardless of
     * seek position.
     */
    if (file->nf_access_flags & FS_ACCESS_APPEND) {
        file->nf_offset = cache_inode->nci_file_size;
    }

    /* Write data as a sequence of blocks. */
    data_ptr = data;
    while (len > 0) {
        if (len > nffs_block_max_data_sz) {
            chunk_size = nffs_block_max_data_sz;
        } else {
            chunk_size = len;
        }

        rc = nffs_write_chunk(file->nf_inode_entry, file->nf_offset, data_ptr,
                              chunk_size);
        if (rc != 0) {
            return rc;
        }

        len -= chunk_size;
        data_ptr += chunk_size;
        file->nf_offset += chunk_size;
    }

    return 0;
}
