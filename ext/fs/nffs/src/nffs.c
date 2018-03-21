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

#include <stdint.h>
#include <nffs/nffs.h>

#if NFFS_CONFIG_USE_HEAP
struct nffs_area *nffs_areas;
#else
struct nffs_area nffs_areas[NFFS_CONFIG_MAX_AREAS];
#endif
uint8_t nffs_num_areas;
uint8_t nffs_scratch_area_idx;
uint16_t nffs_block_max_data_sz;
struct nffs_area_desc *nffs_current_area_descs;

struct nffs_inode_entry *nffs_root_dir;
struct nffs_inode_entry *nffs_lost_found_dir;

#ifdef STATS_SECT_START
STATS_SECT_DECL(nffs_stats) nffs_stats;
STATS_NAME_START(nffs_stats)
    STATS_NAME(nffs_stats, nffs_hashcnt_ins)
    STATS_NAME(nffs_stats, nffs_hashcnt_rm)
    STATS_NAME(nffs_stats, nffs_object_count)
    STATS_NAME(nffs_stats, nffs_iocnt_read)
    STATS_NAME(nffs_stats, nffs_iocnt_write)
    STATS_NAME(nffs_stats, nffs_gccnt)
    STATS_NAME(nffs_stats, nffs_readcnt_data)
    STATS_NAME(nffs_stats, nffs_readcnt_block)
    STATS_NAME(nffs_stats, nffs_readcnt_crc)
    STATS_NAME(nffs_stats, nffs_readcnt_copy)
    STATS_NAME(nffs_stats, nffs_readcnt_format)
    STATS_NAME(nffs_stats, nffs_readcnt_gccollate)
    STATS_NAME(nffs_stats, nffs_readcnt_inode)
    STATS_NAME(nffs_stats, nffs_readcnt_inodeent)
    STATS_NAME(nffs_stats, nffs_readcnt_rename)
    STATS_NAME(nffs_stats, nffs_readcnt_update)
    STATS_NAME(nffs_stats, nffs_readcnt_filename)
    STATS_NAME(nffs_stats, nffs_readcnt_object)
    STATS_NAME(nffs_stats, nffs_readcnt_detect)
STATS_NAME_END(nffs_stats);
#endif
