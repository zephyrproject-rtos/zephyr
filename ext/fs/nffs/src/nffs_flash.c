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
#include <nffs/nffs.h>
#include <nffs/os.h>

/** A buffer used for flash reads; shared across all of nffs. */
uint8_t nffs_flash_buf[NFFS_FLASH_BUF_SZ];

/**
 * Reads a chunk of data from flash.
 *
 * @param area_idx              The index of the area to read from.
 * @param area_offset           The offset within the area to read from.
 * @param data                  On success, the flash contents are written
 *                                  here.
 * @param len                   The number of bytes to read.
 *
 * @return                      0 on success;
 *                              FS_EOFFSET on an attempt to read an invalid
 *                                  address range;
 *                              FS_EHW on flash error.
 */
int
nffs_flash_read(uint8_t area_idx, uint32_t area_offset, void *data,
                uint32_t len)
{
    const struct nffs_area *area;
    int rc;

    assert(area_idx < nffs_num_areas);

    area = nffs_areas + area_idx;

    if (area_offset + len > area->na_length) {
        return FS_EOFFSET;
    }

    STATS_INC(nffs_stats, nffs_iocnt_read);
    rc = nffs_os_flash_read(area->na_flash_id, area->na_offset + area_offset, data,
                        len);
    if (rc != 0) {
        return FS_EHW;
    }

    return 0;
}

/**
 * Writes a chunk of data to flash.
 *
 * @param area_idx              The index of the area to write to.
 * @param area_offset           The offset within the area to write to.
 * @param data                  The data to write to flash.
 * @param len                   The number of bytes to write.
 *
 * @return                      0 on success;
 *                              FS_EOFFSET on an attempt to write to an
 *                                  invalid address range, or on an attempt to
 *                                  perform a non-strictly-sequential write;
 *                              FS_EFLASH_ERROR on flash error.
 */
int
nffs_flash_write(uint8_t area_idx, uint32_t area_offset, const void *data,
                 uint32_t len)
{
    struct nffs_area *area;
    int rc;

    assert(area_idx < nffs_num_areas);
    area = nffs_areas + area_idx;

    if (area_offset + len > area->na_length) {
        return FS_EOFFSET;
    }

    if (area_offset < area->na_cur) {
        return FS_EOFFSET;
    }

    STATS_INC(nffs_stats, nffs_iocnt_write);
    rc = nffs_os_flash_write(area->na_flash_id, area->na_offset + area_offset,
                         data, len);
    if (rc != 0) {
        return FS_EHW;
    }

    area->na_cur = area_offset + len;

    return 0;
}

/**
 * Copies a chunk of data from one region of flash to another.
 *
 * @param area_idx_from         The index of the area to copy from.
 * @param area_offset_from      The offset within the area to copy from.
 * @param area_idx_to           The index of the area to copy to.
 * @param area_offset_to        The offset within the area to copy to.
 * @param len                   The number of bytes to copy.
 *
 * @return                      0 on success; nonzero on failure.
 */
int
nffs_flash_copy(uint8_t area_idx_from, uint32_t area_offset_from,
                uint8_t area_idx_to, uint32_t area_offset_to,
                uint32_t len)
{
    uint32_t chunk_len;
    int rc;

    /* Copy data in chunks small enough to fit in the flash buffer. */
    while (len > 0) {
        if (len > sizeof nffs_flash_buf) {
            chunk_len = sizeof nffs_flash_buf;
        } else {
            chunk_len = len;
        }

        STATS_INC(nffs_stats, nffs_readcnt_copy);
        rc = nffs_flash_read(area_idx_from, area_offset_from, nffs_flash_buf,
                             chunk_len);
        if (rc != 0) {
            return rc;
        }

        rc = nffs_flash_write(area_idx_to, area_offset_to, nffs_flash_buf,
                              chunk_len);
        if (rc != 0) {
            return rc;
        }

        area_offset_from += chunk_len;
        area_offset_to += chunk_len;
        len -= chunk_len;
    }

    return 0;
}

/**
 * Compresses a flash-area-index,flash-area-offset pair into a 32-bit flash
 * location.
 */
uint32_t
nffs_flash_loc(uint8_t area_idx, uint32_t area_offset)
{
    assert(area_offset <= 0x00ffffff);
    return area_idx << 24 | area_offset;
}

/**
 * Expands a compressed 32-bit flash location into a
 * flash-area-index,flash-area-offset pair.
 */
void
nffs_flash_loc_expand(uint32_t flash_loc, uint8_t *out_area_idx,
                     uint32_t *out_area_offset)
{
    *out_area_idx = flash_loc >> 24;
    *out_area_offset = flash_loc & 0x00ffffff;
}
