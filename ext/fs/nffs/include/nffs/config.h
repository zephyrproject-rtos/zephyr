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

#ifndef H_CONFIG_
#define H_CONFIG_

#if __ZEPHYR__

typedef struct k_mem_slab nffs_os_mempool_t;

#define NFFS_CONFIG_USE_HEAP            0
#define NFFS_CONFIG_MAX_AREAS           CONFIG_NFFS_FILESYSTEM_MAX_AREAS
#define NFFS_CONFIG_MAX_BLOCK_SIZE      CONFIG_NFFS_FILESYSTEM_MAX_BLOCK_SIZE

#else

/* Default to Mynewt */

typedef struct os_mempool nffs_os_mempool_t;

#define NFFS_CONFIG_USE_HEAP            1
#define NFFS_CONFIG_MAX_AREAS           256
#define NFFS_CONFIG_MAX_BLOCK_SIZE      2048

#endif

#endif
