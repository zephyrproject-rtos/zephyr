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

/**
 * Turns a scratch area into a non-scratch area.  If the specified area is not
 * actually a scratch area, this function falls back to a slower full format
 * operation.
 */
int
nffs_format_from_scratch_area(uint8_t area_idx, uint8_t area_id)
{
    struct nffs_disk_area disk_area;
    int rc;

    assert(area_idx < nffs_num_areas);
    STATS_INC(nffs_stats, nffs_readcnt_format);
    rc = nffs_flash_read(area_idx, 0, &disk_area, sizeof disk_area);
    if (rc != 0) {
        return rc;
    }

    nffs_areas[area_idx].na_id = area_id;
    if (!nffs_area_is_scratch(&disk_area)) {
        rc = nffs_format_area(area_idx, 0);
        if (rc != 0) {
            return rc;
        }
    } else {
        disk_area.nda_id = area_id;
        rc = nffs_flash_write(area_idx, NFFS_AREA_OFFSET_ID,
                              &disk_area.nda_id, sizeof disk_area.nda_id);
        if (rc != 0) {
            return rc;
        }
    }

    return 0;
}

/**
 * Formats a single scratch area.
 */
int
nffs_format_area(uint8_t area_idx, int is_scratch)
{
    struct nffs_disk_area disk_area;
    struct nffs_area *area;
    uint32_t write_len;
    int rc;

    area = nffs_areas + area_idx;

    rc = nffs_os_flash_erase(area->na_flash_id, area->na_offset, area->na_length);
    if (rc != 0) {
        return FS_EHW;
    }
    area->na_cur = 0;

    nffs_area_to_disk(area, &disk_area);

    if (is_scratch) {
        nffs_areas[area_idx].na_id = NFFS_AREA_ID_NONE;
        write_len = sizeof disk_area - sizeof disk_area.nda_id;
    } else {
        write_len = sizeof disk_area;
    }

    rc = nffs_flash_write(area_idx, 0, &disk_area.nda_magic, write_len);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

/**
 * Erases all the specified areas and initializes them with a clean nffs
 * file system.
 *
 * @param area_descs        The set of areas to format.
 *
 * @return                  0 on success;
 *                          nonzero on failure.
 */
int
nffs_format_full(const struct nffs_area_desc *area_descs)
{
    int rc;
    int i;

    /* Start from a clean state. */
    nffs_misc_reset();

    /* Select largest area to be the initial scratch area. */
    nffs_scratch_area_idx = 0;
    for (i = 1; area_descs[i].nad_length != 0; i++) {
        if (i >= NFFS_CONFIG_MAX_AREAS) {
            rc = FS_EINVAL;
            goto err;
        }

        if (area_descs[i].nad_length >
            area_descs[nffs_scratch_area_idx].nad_length) {

            nffs_scratch_area_idx = i;
        }
    }

    rc = nffs_misc_set_num_areas(i);
    if (rc != 0) {
        goto err;
    }

    for (i = 0; i < nffs_num_areas; i++) {
        nffs_areas[i].na_offset = area_descs[i].nad_offset;
        nffs_areas[i].na_length = area_descs[i].nad_length;
        nffs_areas[i].na_flash_id = area_descs[i].nad_flash_id;
        nffs_areas[i].na_cur = 0;
        nffs_areas[i].na_gc_seq = 0;

        if (i == nffs_scratch_area_idx) {
            nffs_areas[i].na_id = NFFS_AREA_ID_NONE;
        } else {
            nffs_areas[i].na_id = i;
        }

        rc = nffs_format_area(i, i == nffs_scratch_area_idx);
        if (rc != 0) {
            goto err;
        }
    }

    rc = nffs_misc_validate_scratch();
    if (rc != 0) {
        goto err;
    }

    /* Create root directory. */
    rc = nffs_file_new(NULL, "", 0, 1, &nffs_root_dir);
    if (rc != 0) {
        goto err;
    }

    /* Create "lost+found" directory. */
    rc = nffs_misc_create_lost_found_dir();
    if (rc != 0) {
        goto err;
    }

    rc = nffs_misc_validate_root_dir();
    if (rc != 0) {
        goto err;
    }

    rc = nffs_misc_set_max_block_data_len(0);
    if (rc != 0) {
        goto err;
    }

    nffs_current_area_descs = (struct nffs_area_desc*) area_descs;

    return 0;

err:
    nffs_misc_reset();
    return rc;
}
