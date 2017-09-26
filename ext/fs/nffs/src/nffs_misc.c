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
#include <stdlib.h>
#include <string.h>
#include <kernel.h>
#include <nffs/nffs.h>
#include <nffs/os.h>

/**
 * Determines if the file system contains a valid root directory.  For the root
 * directory to be valid, it must be present and have the following traits:
 *     o ID equal to NFFS_ID_ROOT_DIR.
 *     o No parent inode.
 *
 * @return                      0 if there is a valid root directory;
 *                              FS_ECORRUPT if there is not a valid root
 *                                  directory;
 *                              nonzero on other error.
 */
int
nffs_misc_validate_root_dir(void)
{
    struct nffs_inode inode;
    int rc;

    if (nffs_root_dir == NULL) {
        return FS_ECORRUPT;
    }

    if (nffs_root_dir->nie_hash_entry.nhe_id != NFFS_ID_ROOT_DIR) {
        return FS_ECORRUPT;
    }

    rc = nffs_inode_from_entry(&inode, nffs_root_dir);
    /*
     * nffs_root_dir is automatically flagged a "dummy" inode but it's special
     */
    if (rc != 0 && rc != FS_ENOENT) {
        return rc;
    }

    if (inode.ni_parent != NULL) {
        return FS_ECORRUPT;
    }

    return 0;
}

/**
 * Determines if the system contains a valid scratch area.  For a scratch area
 * to be valid, it must be at least as large as the other areas in the file
 * system.
 *
 * @return                      0 if there is a valid scratch area;
 *                              FS_ECORRUPT otherwise.
 */
int
nffs_misc_validate_scratch(void)
{
    uint32_t scratch_len;
    int i;

    if (nffs_scratch_area_idx == NFFS_AREA_ID_NONE) {
        /* No scratch area. */
        return FS_ECORRUPT;
    }

    scratch_len = nffs_areas[nffs_scratch_area_idx].na_length;
    for (i = 0; i < nffs_num_areas; i++) {
        if (nffs_areas[i].na_length > scratch_len) {
            return FS_ECORRUPT;
        }
    }

    return 0;
}

/**
 * Performs a garbage cycle to free up memory, if necessary.  This function
 * should be called repeatedly until either:
 *   o The subsequent allocation is successful, or
 *   o Garbage collection is not successfully performed (indicated by a return
 *     code other than FS_EAGAIN).
 *
 * This function determines if garbage collection is necessary by inspecting
 * the value of the supplied "resource" parameter.  If resource is null, that
 * implies that allocation failed.
 *
 * This function will not initiate garbage collection if all areas have already
 * been collected in an attempt to free memory for the allocation in question.
 *
 * @param resource              The result of the allocation attempt; null
 *                                  implies that garbage collection is
 *                                  necessary.
 * @param out_rc                The status of this operation gets written here.
 *                                   0: garbage collection was successful or
 *                                       unnecessary.
 *                                   FS_EFULL: Garbage collection was not
 *                                       performed because all areas have
 *                                       already been collected.
 *                                   Other nonzero: garbage collection failed.
 *
 * @return                      FS_EAGAIN if garbage collection was
 *                                  successfully performed and the allocation
 *                                  should be retried;
 *                              Other value if the allocation should not be
 *                                  retried; the value of the out_rc parameter
 *                                  indicates whether allocation was successful
 *                                  or there was an error.
 */
int
nffs_misc_gc_if_oom(void *resource, int *out_rc)
{
    /**
     * Keeps track of the number of repeated garbage collection cycles.
     * Necessary for ensuring GC stops after all areas have been collected.
     */
    static uint8_t total_gc_cycles;

    if (resource != NULL) {
        /* Allocation succeeded.  Reset cycle count in preparation for the next
         * allocation failure.
         */
        total_gc_cycles = 0;
        *out_rc = 0;
        return 0;
    }

    /* If every area has already been garbage collected, there is nothing else
     * that can be done ("- 1" to account for the scratch area).
     */
    if (total_gc_cycles >= nffs_num_areas - 1) {
        *out_rc = FS_ENOMEM;
        return 0;
    }

    /* Attempt a garbage collection on the next area. */
    *out_rc = nffs_gc(NULL);
    total_gc_cycles++;
    STATS_INC(nffs_stats, nffs_gccnt);
    if (*out_rc != 0) {
        return 0;
    }

    /* Indicate that garbage collection was successfully performed. */
    return 1;
}

/**
 * Reserves the specified number of bytes within the specified area.
 *
 * @param area_idx              The index of the area to reserve from.
 * @param space                 The number of bytes of free space required.
 * @param out_area_offset       On success, the offset within the area gets
 *                                  written here.
 *
 * @return                      0 on success;
 *                              FS_EFULL if the area has insufficient free
 *                                  space.
 */
static int
nffs_misc_reserve_space_area(uint8_t area_idx, uint16_t space,
                             uint32_t *out_area_offset)
{
    const struct nffs_area *area;
    uint32_t available;

    area = nffs_areas + area_idx;
    available = area->na_length - area->na_cur;
    if (available >= space) {
        *out_area_offset = area->na_cur;
        return 0;
    }

    return FS_EFULL;
}

/**
 * Finds an area that can accommodate an object of the specified size.  If no
 * such area exists, this function performs a garbage collection cycle.
 *
 * @param space                 The number of bytes of free space required.
 * @param out_area_idx          On success, the index of the suitable area gets
 *                                  written here.
 * @param out_area_offset       On success, the offset within the suitable area
 *                                  gets written here.
 *
 * @return                      0 on success; nonzero on failure.
 */
int
nffs_misc_reserve_space(uint16_t space,
                        uint8_t *out_area_idx, uint32_t *out_area_offset)
{
    uint8_t area_idx;
    int rc;
    int i;

    /* Find the first area with sufficient free space. */
    for (i = 0; i < nffs_num_areas; i++) {
        if (i != nffs_scratch_area_idx) {
            rc = nffs_misc_reserve_space_area(i, space, out_area_offset);
            if (rc == 0) {
                *out_area_idx = i;
                return 0;
            }
        }
    }

    /* No area can accommodate the request.  Garbage collect until an area
     * has enough space.
     */
    rc = nffs_gc_until(space, &area_idx);
    if (rc != 0) {
        return rc;
    }

    /* Now try to reserve space.  If insufficient space was reclaimed with
     * garbage collection, the above call would have failed, so this should
     * succeed.
     */
    rc = nffs_misc_reserve_space_area(area_idx, space, out_area_offset);
    assert(rc == 0);

    *out_area_idx = area_idx;

    return rc;
}

int
nffs_misc_set_num_areas(uint8_t num_areas)
{
#if NFFS_CONFIG_USE_HEAP
    if (num_areas == 0) {
        free(nffs_areas);
        nffs_areas = NULL;
    } else {
        nffs_areas = realloc(nffs_areas, num_areas * sizeof *nffs_areas);
        if (nffs_areas == NULL) {
            return FS_ENOMEM;
        }
    }
#endif

    nffs_num_areas = num_areas;

    return 0;
}

/**
 * Calculates the data length of the largest block that could fit in an area of
 * the specified size.
 */
static uint32_t
nffs_misc_area_capacity_one(uint32_t area_length)
{
    return area_length -
           sizeof (struct nffs_disk_area) -
           sizeof (struct nffs_disk_block);
}

/**
 * Calculates the data length of the largest block that could fit as a pair in
 * an area of the specified size.
 */
static uint32_t
nffs_misc_area_capacity_two(uint32_t area_length)
{
    return (area_length - sizeof (struct nffs_disk_area)) / 2 -
           sizeof (struct nffs_disk_block);
}

/**
 * Calculates and sets the maximum block data length that the system supports.
 * The result of the calculation is the greatest number which satisfies all of
 * the following restrictions:
 *     o No more than half the size of the smallest area.
 *     o No more than 2048.
 *     o No smaller than the data length of any existing data block.
 *
 * @param min_size              The minimum allowed data length.  This is the
 *                                  data length of the largest block currently
 *                                  in the file system.
 *
 * @return                      0 on success; nonzero on failure.
 */
int
nffs_misc_set_max_block_data_len(uint16_t min_data_len)
{
    uint32_t smallest_area;
    uint32_t half_smallest;
    int i;

    smallest_area = -1;
    for (i = 0; i < nffs_num_areas; i++) {
        if (nffs_areas[i].na_length < smallest_area) {
            smallest_area = nffs_areas[i].na_length;
        }
    }

    /* Don't allow a data block size bigger than the smallest area. */
    if (nffs_misc_area_capacity_one(smallest_area) < min_data_len) {
        return FS_ECORRUPT;
    }

    half_smallest = nffs_misc_area_capacity_two(smallest_area);
    if (half_smallest < NFFS_BLOCK_MAX_DATA_SZ_MAX) {
        nffs_block_max_data_sz = half_smallest;
    } else {
        nffs_block_max_data_sz = NFFS_BLOCK_MAX_DATA_SZ_MAX;
    }

    if (nffs_block_max_data_sz < min_data_len) {
        nffs_block_max_data_sz = min_data_len;
    }

    return 0;
}

int
nffs_misc_create_lost_found_dir(void)
{
    int rc;

    rc = nffs_path_new_dir("/lost+found", &nffs_lost_found_dir);
    switch (rc) {
    case 0:
        return 0;

    case FS_EEXIST:
        rc = nffs_path_find_inode_entry("/lost+found", &nffs_lost_found_dir);
        return rc;

    default:
        return rc;
    }
}


/**
 * Fully resets the nffs RAM representation.
 *
 * @return                      0 on success; nonzero on failure.
 */
int
nffs_misc_reset(void)
{
    int rc;

    nffs_cache_clear();

    nffs_os_mempool_init();

    rc = nffs_hash_init();
    if (rc != 0) {
        return rc;
    }

#if NFFS_CONFIG_USE_HEAP
    free(nffs_areas);
    nffs_areas = NULL;
#endif

    nffs_num_areas = 0;

    nffs_root_dir = NULL;
    nffs_lost_found_dir = NULL;
    nffs_scratch_area_idx = NFFS_AREA_ID_NONE;

    nffs_hash_next_file_id = NFFS_ID_FILE_MIN;
    nffs_hash_next_dir_id = NFFS_ID_DIR_MIN;
    nffs_hash_next_block_id = NFFS_ID_BLOCK_MIN;

    return 0;
}

/**
 * Indicates whether a valid filesystem has been initialized, either via
 * detection or formatting.
 *
 * @return                  1 if a file system is present; 0 otherwise.
 */
int
nffs_misc_ready(void)
{
    return nffs_root_dir != NULL;
}


/*
 * Turn flash region into a set of areas for NFFS use.
 *
 * Limit the number of regions we return to be less than *cnt.
 * If sector count within region exceeds that, collect multiple sectors
 * to a region.
 */
int
nffs_misc_desc_from_flash_area(const struct nffs_flash_desc *flash, int *cnt, struct nffs_area_desc *nad)
{
    int i, j;
    int max_cnt, move_on;
    int first_idx, last_idx;
    uint32_t start, size;
    uint32_t min_size;

    first_idx = last_idx = -1;
    max_cnt = *cnt;
    *cnt = 0;

    for (i = 0; i < flash->sector_count; i++) {
        nffs_os_flash_info(flash->id, i, &start, &size);
        if (start >= flash->area_offset && start < flash->area_offset + flash->area_size) {
            if (first_idx == -1) {
                first_idx = i;
            }
            last_idx = i;
            *cnt = *cnt + 1;
        }
    }
    if (*cnt > max_cnt) {
        min_size = flash->area_size / max_cnt;
    } else {
        min_size = 0;
    }
    *cnt = 0;

    move_on = 1;
    for (i = first_idx, j = 0; i < last_idx + 1; i++) {
        nffs_os_flash_info(flash->id, i, &start, &size);
        if (move_on) {
            nad[j].nad_flash_id = flash->id;
            nad[j].nad_offset = start;
            nad[j].nad_length = size;
            *cnt = *cnt + 1;
            move_on = 0;
        } else {
            nad[j].nad_length += size;
        }
        if (nad[j].nad_length >= min_size) {
            j++;
            move_on = 1;
        }
    }
    nad[*cnt].nad_length = 0;

    return 0;
}
