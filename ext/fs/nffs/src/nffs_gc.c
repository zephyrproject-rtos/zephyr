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
#include <kernel.h>
#include <nffs/nffs.h>

/**
 * Keeps track of the number of garbage collections performed.  The exact
 * number is not important, but it is useful to compare against an older copy
 * to determine if garbage collection occurred.
 */
unsigned int nffs_gc_count;

static int
nffs_gc_copy_object(struct nffs_hash_entry *entry, uint16_t object_size,
                    uint8_t to_area_idx)
{
    uint32_t from_area_offset;
    uint32_t to_area_offset;
    uint8_t from_area_idx;
    int rc;

    nffs_flash_loc_expand(entry->nhe_flash_loc,
                          &from_area_idx, &from_area_offset);
    to_area_offset = nffs_areas[to_area_idx].na_cur;

    rc = nffs_flash_copy(from_area_idx, from_area_offset, to_area_idx,
                         to_area_offset, object_size);
    if (rc != 0) {
        return rc;
    }

    entry->nhe_flash_loc = nffs_flash_loc(to_area_idx, to_area_offset);

    return 0;
}

static int
nffs_gc_copy_inode(struct nffs_inode_entry *inode_entry, uint8_t to_area_idx)
{
    struct nffs_inode inode;
    uint16_t copy_len;
    int rc;

    rc = nffs_inode_from_entry(&inode, inode_entry);
    if (rc != 0) {
        return rc;
    }
    copy_len = sizeof (struct nffs_disk_inode) + inode.ni_filename_len;

    rc = nffs_gc_copy_object(&inode_entry->nie_hash_entry, copy_len,
                             to_area_idx);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

/**
 * Selects the most appropriate area for garbage collection.
 *
 * @return                  The ID of the area to garbage collect.
 */
static uint16_t
nffs_gc_select_area(void)
{
    const struct nffs_area *area;
    uint8_t best_area_idx;
    int8_t diff;
    int i;

    best_area_idx = 0;
    for (i = 1; i < nffs_num_areas; i++) {
        if (i == nffs_scratch_area_idx) {
            continue;
        }

        area = nffs_areas + i;
        if (area->na_length > nffs_areas[best_area_idx].na_length) {
            best_area_idx = i;
        } else if (best_area_idx == nffs_scratch_area_idx) {
            best_area_idx = i;
        } else {
            diff = nffs_areas[i].na_gc_seq -
                   nffs_areas[best_area_idx].na_gc_seq;
            if (diff < 0) {
                best_area_idx = i;
            }
        }
    }

    assert(best_area_idx != nffs_scratch_area_idx);

    return best_area_idx;
}

static int
nffs_gc_block_chain_copy(struct nffs_hash_entry *last_entry, uint32_t data_len,
                         uint8_t to_area_idx)
{
    struct nffs_hash_entry *entry;
    struct nffs_block block;
    uint32_t data_bytes_copied;
    uint16_t copy_len;
    int rc;

    data_bytes_copied = 0;
    entry = last_entry;

    while (data_bytes_copied < data_len) {
        assert(entry != NULL);

        rc = nffs_block_from_hash_entry(&block, entry);
        if (rc != 0) {
            return rc;
        }

        copy_len = sizeof (struct nffs_disk_block) + block.nb_data_len;
        rc = nffs_gc_copy_object(entry, copy_len, to_area_idx);
        if (rc != 0) {
            return rc;
        }
        data_bytes_copied += block.nb_data_len;

        entry = block.nb_prev;
    }

    return 0;
}

/**
 * Moves a chain of blocks from one area to another.  This function attempts to
 * collate the blocks into a single new block in the destination area.
 *
 * @param last_entry            The last block entry in the chain.
 * @param data_len              The total length of data to collate.
 * @param to_area_idx           The index of the area to copy to.
 * @param inout_next            This parameter is only necessary if you are
 *                                  calling this function during an iteration
 *                                  of the entire hash table; pass null
 *                                  otherwise.
 *                              On input, this points to the next hash entry
 *                                  you plan on processing.
 *                              On output, this points to the next hash entry
 *                                  that should be processed.
 *
 * @return                      0 on success;
 *                              FS_ENOMEM if there is insufficient heap;
 *                              other nonzero on failure.
 */
static int
nffs_gc_block_chain_collate(struct nffs_hash_entry *last_entry,
                            uint32_t data_len, uint8_t to_area_idx,
                            struct nffs_hash_entry **inout_next)
{
    struct nffs_disk_block disk_block;
    struct nffs_hash_entry *entry;
    struct nffs_area *to_area;
    struct nffs_block last_block;
    struct nffs_block block;
    uint32_t to_area_offset;
    uint32_t from_area_offset;
    uint32_t data_offset;
#if NFFS_CONFIG_USE_HEAP
    uint8_t *data;
#else
    static uint8_t data[NFFS_BLOCK_MAX_DATA_SZ_MAX];
#endif
    uint8_t from_area_idx;
    int rc;

    memset(&last_block, 0, sizeof last_block);

#if NFFS_CONFIG_USE_HEAP
    data = malloc(data_len);
    if (data == NULL) {
        rc = FS_ENOMEM;
        goto done;
    }
#endif

    to_area = nffs_areas + to_area_idx;

    entry = last_entry;
    data_offset = data_len;
    while (data_offset > 0) {
        rc = nffs_block_from_hash_entry(&block, entry);
        if (rc != 0) {
            goto done;
        }
        data_offset -= block.nb_data_len;

        nffs_flash_loc_expand(block.nb_hash_entry->nhe_flash_loc,
                              &from_area_idx, &from_area_offset);
        from_area_offset += sizeof disk_block;
        STATS_INC(nffs_stats, nffs_readcnt_gccollate);
        rc = nffs_flash_read(from_area_idx, from_area_offset,
                             data + data_offset, block.nb_data_len);
        if (rc != 0) {
            goto done;
        }

        if (entry != last_entry) {
            if (inout_next != NULL && *inout_next == entry) {
                *inout_next = SLIST_NEXT(entry, nhe_next);
            }
            nffs_block_delete_from_ram(entry);
        } else {
            last_block = block;
        }
        entry = block.nb_prev;
    }

    /* we had better have found the last block */
    assert(last_block.nb_hash_entry);

    /* The resulting block should inherit its ID from its last constituent
     * block (this is the ID referenced by the parent inode and subsequent data
     * block).  The previous ID gets inherited from the first constituent
     * block.
     */
    memset(&disk_block, 0, sizeof disk_block);
    disk_block.ndb_id = last_block.nb_hash_entry->nhe_id;
    disk_block.ndb_seq = last_block.nb_seq + 1;
    disk_block.ndb_inode_id = last_block.nb_inode_entry->nie_hash_entry.nhe_id;
    if (entry == NULL) {
        disk_block.ndb_prev_id = NFFS_ID_NONE;
    } else {
        disk_block.ndb_prev_id = entry->nhe_id;
    }
    disk_block.ndb_data_len = data_len;
    nffs_crc_disk_block_fill(&disk_block, data);

    to_area_offset = to_area->na_cur;
    rc = nffs_flash_write(to_area_idx, to_area_offset,
                          &disk_block, sizeof disk_block);
    if (rc != 0) {
        goto done;
    }

    rc = nffs_flash_write(to_area_idx, to_area_offset + sizeof disk_block,
                          data, data_len);
    if (rc != 0) {
        goto done;
    }

    last_entry->nhe_flash_loc = nffs_flash_loc(to_area_idx, to_area_offset);

    rc = 0;

    ASSERT_IF_TEST(nffs_crc_disk_block_validate(&disk_block, to_area_idx,
                                                to_area_offset) == 0);

done:
#if NFFS_CONFIG_USE_HEAP
    free(data);
#endif
    return rc;
}

/**
 * Moves a chain of blocks from one area to another.  This function attempts to
 * collate the blocks into a single new block in the destination area.  If
 * there is insufficient heap memory do to this, the function falls back to
 * copying each block separately.
 *
 * @param last_entry            The last block entry in the chain.
 * @param multiple_blocks       0=single block; 1=more than one block.
 * @param data_len              The total length of data to collate.
 * @param to_area_idx           The index of the area to copy to.
 * @param inout_next            This parameter is only necessary if you are
 *                                  calling this function during an iteration
 *                                  of the entire hash table; pass null
 *                                  otherwise.
 *                              On input, this points to the next hash entry
 *                                  you plan on processing.
 *                              On output, this points to the next hash entry
 *                                  that should be processed.
 *
 * @return                      0 on success; nonzero on failure.
 */
static int
nffs_gc_block_chain(struct nffs_hash_entry *last_entry, int multiple_blocks,
                    uint32_t data_len, uint8_t to_area_idx,
                    struct nffs_hash_entry **inout_next)
{
    int rc;

    if (!multiple_blocks) {
        /* If there is only one block, collation has the same effect as a
         * simple copy.  Just perform the more efficient copy.
         */
        rc = nffs_gc_block_chain_copy(last_entry, data_len, to_area_idx);
    } else {
        rc = nffs_gc_block_chain_collate(last_entry, data_len, to_area_idx,
                                         inout_next);
        if (rc == FS_ENOMEM) {
            /* Insufficient heap for collation; just copy each block one by
             * one.
             */
            rc = nffs_gc_block_chain_copy(last_entry, data_len, to_area_idx);
        }
    }

    return rc;
}

static int
nffs_gc_inode_blocks(struct nffs_inode_entry *inode_entry,
                     uint8_t from_area_idx, uint8_t to_area_idx,
                     struct nffs_hash_entry **inout_next)
{
    struct nffs_hash_entry *last_entry;
    struct nffs_hash_entry *entry;
    struct nffs_block block;
    uint32_t prospective_data_len;
    uint32_t area_offset;
    uint32_t data_len;
    uint8_t area_idx;
    int multiple_blocks;
    int rc;

    assert(nffs_hash_id_is_file(inode_entry->nie_hash_entry.nhe_id));

    data_len = 0;
    last_entry = NULL;
    multiple_blocks = 0;
    entry = inode_entry->nie_last_block_entry;
    while (entry != NULL) {
        rc = nffs_block_from_hash_entry(&block, entry);
        if (rc != 0) {
            return rc;
        }

        nffs_flash_loc_expand(entry->nhe_flash_loc, &area_idx, &area_offset);
        if (area_idx == from_area_idx) {
            if (last_entry == NULL) {
                last_entry = entry;
            }

            prospective_data_len = data_len + block.nb_data_len;
            if (prospective_data_len <= nffs_block_max_data_sz) {
                data_len = prospective_data_len;
                if (last_entry != entry) {
                    multiple_blocks = 1;
                }
            } else {
                rc = nffs_gc_block_chain(last_entry, multiple_blocks, data_len,
                                         to_area_idx, inout_next);
                if (rc != 0) {
                    return rc;
                }
                last_entry = entry;
                data_len = block.nb_data_len;
                multiple_blocks = 0;
            }
        } else {
            if (last_entry != NULL) {
                rc = nffs_gc_block_chain(last_entry, multiple_blocks, data_len,
                                         to_area_idx, inout_next);
                if (rc != 0) {
                    return rc;
                }

                last_entry = NULL;
                data_len = 0;
                multiple_blocks = 0;
            }
        }

        entry = block.nb_prev;
    }

    if (last_entry != NULL) {
        rc = nffs_gc_block_chain(last_entry, multiple_blocks, data_len,
                                 to_area_idx, inout_next);
        if (rc != 0) {
            return rc;
        }
    }

    return 0;
}

/**
 * Triggers a garbage collection cycle.  This is implemented as follows:
 *
 *  (1) The non-scratch area with the lowest garbage collection sequence
 *      number is selected as the "source area."  If there are other areas
 *      with the same sequence number, the first one encountered is selected.
 *
 *  (2) The source area's ID is written to the scratch area's header,
 *      transforming it into a non-scratch ID.  The former scratch area is now
 *      known as the "destination area."
 *
 *  (3) The RAM representation is exhaustively searched for objects which are
 *      resident in the source area.  The copy is accomplished as follows:
 *
 *      For each inode:
 *          (a) If the inode is resident in the source area, copy the inode
 *              record to the destination area.
 *
 *          (b) Walk the inode's list of data blocks, starting with the last
 *              block in the file.  Each block that is resident in the source
 *              area is copied to the destination area.  If there is a run of
 *              two or more blocks that are resident in the source area, they
 *              are consolidated and copied to the destination area as a single
 *              new block.
 *
 *  (4) The source area is reformatted as a scratch sector (i.e., its header
 *      indicates an ID of 0xffff).  The area's garbage collection sequence
 *      number is incremented prior to rewriting the header.  This area is now
 *      the new scratch sector.
 *
 * NOTE:
 *     Garbage collection invalidates all cached data blocks.  Whenever this
 *     function is called, all existing nffs_cache_block pointers are rendered
 *     invalid.  If you maintain any such pointers, you need to reset them
 *     after calling this function.  Cached inodes are not invalidated by
 *     garbage collection.
 *
 *     If a parent function potentially calls this function, the caller of the
 *     parent function needs to explicitly check if garbage collection
 *     occurred.  This is done by inspecting the nffs_gc_count variable before
 *     and after calling the function.
 *
 * @param out_area_idx      On success, the ID of the cleaned up area gets
 *                              written here.  Pass null if you do not need
 *                              this information.
 *
 * @return                  0 on success; nonzero on error.
 */
int
nffs_gc(uint8_t *out_area_idx)
{
    struct nffs_hash_entry *entry;
    struct nffs_hash_entry *next;
    struct nffs_area *from_area;
    struct nffs_area *to_area;
    struct nffs_inode_entry *inode_entry;
    uint32_t area_offset;
    uint8_t from_area_idx;
    uint8_t area_idx;
    int rc;
    int i;

    from_area_idx = nffs_gc_select_area();
    from_area = nffs_areas + from_area_idx;
    to_area = nffs_areas + nffs_scratch_area_idx;

    rc = nffs_format_from_scratch_area(nffs_scratch_area_idx,
                                       from_area->na_id);
    if (rc != 0) {
        return rc;
    }

    for (i = 0; i < NFFS_HASH_SIZE; i++) {
        entry = SLIST_FIRST(nffs_hash + i);
        while (entry != NULL) {
            next = SLIST_NEXT(entry, nhe_next);

            if (nffs_hash_id_is_inode(entry->nhe_id)) {
                /* The inode gets copied if it is in the source area. */
                nffs_flash_loc_expand(entry->nhe_flash_loc,
                                      &area_idx, &area_offset);
                inode_entry = (struct nffs_inode_entry *)entry;
                if (area_idx == from_area_idx) {
                    rc = nffs_gc_copy_inode(inode_entry,
                                            nffs_scratch_area_idx);
                    if (rc != 0) {
                        return rc;
                    }
                }

                /* If the inode is a file, all constituent data blocks that are
                 * resident in the source area get copied.
                 */
                if (nffs_hash_id_is_file(entry->nhe_id)) {
                    rc = nffs_gc_inode_blocks(inode_entry, from_area_idx,
                                              nffs_scratch_area_idx, &next);
                    if (rc != 0) {
                        return rc;
                    }
                }
            }

            entry = next;
        }
    }

    /* The amount of written data should never increase as a result of a gc
     * cycle.
     */
    assert(to_area->na_cur <= from_area->na_cur);

    /* Turn the source area into the new scratch area. */
    from_area->na_gc_seq++;
    rc = nffs_format_area(from_area_idx, 1);
    if (rc != 0) {
        return rc;
    }

    if (out_area_idx != NULL) {
        *out_area_idx = nffs_scratch_area_idx;
    }

    nffs_scratch_area_idx = from_area_idx;

    /* Garbage collection renders the cache invalid:
     *     o All cached blocks are now invalid; drop them.
     *     o Flash locations of inodes may have changed; the cached inodes need
     *       updated to reflect this.
     */
    rc = nffs_cache_inode_refresh();
    if (rc != 0) {
        return rc;
    }

    /* Increment the garbage collection counter so that client code knows to
     * reset its pointers to cached objects.
     */
    nffs_gc_count++;
    STATS_INC(nffs_stats, nffs_gccnt);

    return 0;
}

/**
 * Repeatedly performs garbage collection cycles until there is enough free
 * space to accommodate an object of the specified size.  If there still isn't
 * enough free space after every area has been garbage collected, this function
 * fails.
 *
 * @param space                 The number of bytes of free space required.
 * @param out_area_idx          On success, the index of the area which can
 *                                  accommodate the necessary data.
 *
 * @return                      0 on success;
 *                              FS_EFULL if the necessary space could not be
 *                                  freed.
 *                              nonzero on other failure.
 */
int
nffs_gc_until(uint32_t space, uint8_t *out_area_idx)
{
    int rc;
    int i;

    for (i = 0; i < nffs_num_areas; i++) {
        rc = nffs_gc(out_area_idx);
        if (rc != 0) {
            return rc;
        }

        if (nffs_area_free_space(nffs_areas + *out_area_idx) >= space) {
            return 0;
        }
    }

    return FS_EFULL;
}
