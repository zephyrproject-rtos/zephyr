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
