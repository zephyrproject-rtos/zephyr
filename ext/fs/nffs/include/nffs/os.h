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

#ifndef H_OS_
#define H_OS_

#include <stdint.h>
#include <kernel.h>
#include <nffs/config.h>

#ifdef __cplusplus
extern "C" {
#endif

extern nffs_os_mempool_t nffs_file_pool;
extern nffs_os_mempool_t nffs_dir_pool;
extern nffs_os_mempool_t nffs_inode_entry_pool;
extern nffs_os_mempool_t nffs_block_entry_pool;
extern nffs_os_mempool_t nffs_cache_inode_pool;
extern nffs_os_mempool_t nffs_cache_block_pool;

/* Initialize mempools */
int nffs_os_mempool_init(void);

/* Get block from mempool */
void *nffs_os_mempool_get(nffs_os_mempool_t *pool);

/* Free block from mempool */
int nffs_os_mempool_free(nffs_os_mempool_t *pool, void *block);

/* Read from flash */
int nffs_os_flash_read(uint8_t id, uint32_t address, void *dst,
                       uint32_t num_bytes);

/* Write to flash */
int nffs_os_flash_write(uint8_t id, uint32_t address, const void *src,
                        uint32_t num_bytes);

/* Erase flash */
int nffs_os_flash_erase(uint8_t id, uint32_t address, uint32_t num_bytes);

/* Get flash sector information */
int nffs_os_flash_info(uint8_t id, uint32_t sector, uint32_t *address,
                       uint32_t *size);

/* CRC16-CCIT implementation */
uint16_t nffs_os_crc16_ccitt(uint16_t initial_crc, const void *buf, int len,
                             int final);

#ifdef __cplusplus
}
#endif

#endif
