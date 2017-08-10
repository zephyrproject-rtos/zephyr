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

static void
nffs_area_set_magic(struct nffs_disk_area *disk_area)
{
    disk_area->nda_magic[0] = NFFS_AREA_MAGIC0;
    disk_area->nda_magic[1] = NFFS_AREA_MAGIC1;
    disk_area->nda_magic[2] = NFFS_AREA_MAGIC2;
    disk_area->nda_magic[3] = NFFS_AREA_MAGIC3;
}

int
nffs_area_magic_is_set(const struct nffs_disk_area *disk_area)
{
    return disk_area->nda_magic[0] == NFFS_AREA_MAGIC0 &&
           disk_area->nda_magic[1] == NFFS_AREA_MAGIC1 &&
           disk_area->nda_magic[2] == NFFS_AREA_MAGIC2 &&
           disk_area->nda_magic[3] == NFFS_AREA_MAGIC3;
}

int
nffs_area_is_scratch(const struct nffs_disk_area *disk_area)
{
    return nffs_area_magic_is_set(disk_area) &&
           disk_area->nda_id == NFFS_AREA_ID_NONE;
}

int
nffs_area_is_current_version(const struct nffs_disk_area *disk_area)
{
    return disk_area->nda_ver == NFFS_AREA_VER;
}

void
nffs_area_to_disk(const struct nffs_area *area,
                  struct nffs_disk_area *out_disk_area)
{
    memset(out_disk_area, 0, sizeof *out_disk_area);
    nffs_area_set_magic(out_disk_area);
    out_disk_area->nda_length = area->na_length;
    out_disk_area->nda_ver = NFFS_AREA_VER;
    out_disk_area->nda_gc_seq = area->na_gc_seq;
    out_disk_area->nda_id = area->na_id;
}

uint32_t
nffs_area_free_space(const struct nffs_area *area)
{
    return area->na_length - area->na_cur;
}

/**
 * Finds a corrupt scratch area.  An area is indentified as a corrupt scratch
 * area if it and another area share the same ID.  Among two areas with the
 * same ID, the one with fewer bytes written is the corrupt scratch area.
 *
 * @param out_good_idx          On success, the index of the good area (longer
 *                                  of the two areas) gets written here.
 * @param out_bad_idx           On success, the index of the corrupt scratch
 *                                  area gets written here.
 *
 * @return                      0 if a corrupt scratch area was identified;
 *                              FS_ENOENT if one was not found.
 */
int
nffs_area_find_corrupt_scratch(uint16_t *out_good_idx, uint16_t *out_bad_idx)
{
    const struct nffs_area *iarea;
    const struct nffs_area *jarea;
    int i;
    int j;

    for (i = 0; i < nffs_num_areas; i++) {
        iarea = nffs_areas + i;
        for (j = i + 1; j < nffs_num_areas; j++) {
            jarea = nffs_areas + j;

            if (jarea->na_id == iarea->na_id) {
                /* Found a duplicate.  The shorter of the two areas should be
                 * used as scratch.
                 */
                if (iarea->na_cur < jarea->na_cur) {
                    *out_good_idx = j;
                    *out_bad_idx = i;
                } else {
                    *out_good_idx = i;
                    *out_bad_idx = j;
                }

                return 0;
            }
        }
    }

    return FS_ENOENT;
}
