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

#include <nffs/nffs.h>
#include <nffs/os.h>

int
nffs_crc_flash(uint16_t initial_crc, uint8_t area_idx, uint32_t area_offset,
               uint32_t len, uint16_t *out_crc)
{
    uint32_t chunk_len;
    uint16_t crc;
    int rc;

    crc = initial_crc;

    /* Copy data in chunks small enough to fit in the flash buffer. */
    while (len > 0) {
        if (len > sizeof nffs_flash_buf) {
            chunk_len = sizeof nffs_flash_buf;
        } else {
            chunk_len = len;
        }

        STATS_INC(nffs_stats, nffs_readcnt_crc);
        rc = nffs_flash_read(area_idx, area_offset, nffs_flash_buf, chunk_len);
        if (rc != 0) {
            return rc;
        }

        crc = nffs_os_crc16_ccitt(crc, nffs_flash_buf, chunk_len, 0);

        area_offset += chunk_len;
        len -= chunk_len;
    }

    *out_crc = crc;
    return 0;
}

uint16_t
nffs_crc_disk_block_hdr(const struct nffs_disk_block *disk_block)
{
    uint16_t crc;

    crc = nffs_os_crc16_ccitt(0, disk_block, NFFS_DISK_BLOCK_OFFSET_CRC, 0);

    return crc;
}

static int
nffs_crc_disk_block(const struct nffs_disk_block *disk_block,
                    uint8_t area_idx, uint32_t area_offset,
                    uint16_t *out_crc)
{
    uint16_t crc;
    int rc;

    crc = nffs_crc_disk_block_hdr(disk_block);

    rc = nffs_crc_flash(crc, area_idx, area_offset + sizeof *disk_block,
                        disk_block->ndb_data_len, &crc);
    if (rc != 0) {
        return rc;
    }

    /* Finish CRC */
    crc = nffs_os_crc16_ccitt(crc, NULL, 0, 1);

    *out_crc = crc;
    return 0;
}

int
nffs_crc_disk_block_validate(const struct nffs_disk_block *disk_block,
                             uint8_t area_idx, uint32_t area_offset)
{
    uint16_t crc;
    int rc;

    rc = nffs_crc_disk_block(disk_block, area_idx, area_offset, &crc);
    if (rc != 0) {
        return rc;
    }

    if (crc != disk_block->ndb_crc16) {
        return FS_ECORRUPT;
    }

    return 0;
}

void
nffs_crc_disk_block_fill(struct nffs_disk_block *disk_block, const void *data)
{
    uint16_t crc16;

    crc16 = nffs_crc_disk_block_hdr(disk_block);
    crc16 = nffs_os_crc16_ccitt(crc16, data, disk_block->ndb_data_len, 1);

    disk_block->ndb_crc16 = crc16;
}

static uint16_t
nffs_crc_disk_inode_hdr(const struct nffs_disk_inode *disk_inode)
{
    uint16_t crc;

    crc = nffs_os_crc16_ccitt(0, disk_inode, NFFS_DISK_INODE_OFFSET_CRC, 1);

    return crc;
}

static int
nffs_crc_disk_inode(const struct nffs_disk_inode *disk_inode,
                    uint8_t area_idx, uint32_t area_offset,
                    uint16_t *out_crc)
{
    uint16_t crc;
    int rc;

    crc = nffs_crc_disk_inode_hdr(disk_inode);

    rc = nffs_crc_flash(crc, area_idx, area_offset + sizeof *disk_inode,
                        disk_inode->ndi_filename_len, &crc);
    if (rc != 0) {
        return rc;
    }

    /* Finish CRC */
    crc = nffs_os_crc16_ccitt(crc, NULL, 0, 1);

    *out_crc = crc;
    return 0;
}

int
nffs_crc_disk_inode_validate(const struct nffs_disk_inode *disk_inode,
                             uint8_t area_idx, uint32_t area_offset)
{
    uint16_t crc;
    int rc;

    rc = nffs_crc_disk_inode(disk_inode, area_idx, area_offset, &crc);
    if (rc != 0) {
        return rc;
    }

    if (crc != disk_inode->ndi_crc16) {
        return FS_ECORRUPT;
    }

    return 0;
}

void
nffs_crc_disk_inode_fill(struct nffs_disk_inode *disk_inode,
                         const char *filename)
{
    uint16_t crc16;

    crc16 = nffs_crc_disk_inode_hdr(disk_inode);
    crc16 = nffs_os_crc16_ccitt(crc16, filename,
                                  disk_inode->ndi_filename_len, 1);

    disk_inode->ndi_crc16 = crc16;
}
