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

#ifndef H_IMG_PRIV_
#define H_IMG_PRIV_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IMG_MGMT_SWAP_TYPE_NONE     0
#define IMG_MGMT_SWAP_TYPE_TEST     1
#define IMG_MGMT_SWAP_TYPE_PERM     2
#define IMG_MGMT_SWAP_TYPE_REVERT   3

/*
 * Response to list:
 * {
 *      "images":[ <version1>, <version2>]
 * }
 *
 *
 * Request to boot to version:
 * {
 *      "test":<version>
 * }
 *
 *
 * Response to boot read:
 * {
 *	"test":<version>,
 *	"main":<version>,
 *      "active":<version>
 * }
 *
 *
 * Request to image upload:
 * {
 *      "off":<offset>,
 *      "len":<img_size>		inspected when off = 0
 *      "data":<base64encoded binary>
 * }
 *
 *
 * Response to upload:
 * {
 *      "off":<offset>
 * }
 *
 *
 * Request to image upload:
 * {
 *      "off":<offset>
 *	    "name":<filename>		inspected when off = 0
 *      "len":<file_size>		inspected when off = 0
 *      "data":<base64encoded binary>
 * }
 */

struct mgmt_ctxt;

int img_mgmt_core_erase(struct mgmt_ctxt *);
int img_mgmt_core_list(struct mgmt_ctxt *);
int img_mgmt_core_load(struct mgmt_ctxt *);
int img_mgmt_find_by_hash(uint8_t *find, struct image_version *ver);
int img_mgmt_find_by_ver(struct image_version *find, uint8_t *hash);
int img_mgmt_read_info(int image_slot, struct image_version *ver,
                       uint8_t *hash, uint32_t *flags);
int img_mgmt_slot_in_use(int slot);
int img_mgmt_state_read(struct mgmt_ctxt *ctxt);
int img_mgmt_state_write(struct mgmt_ctxt *njb);
int img_mgmt_ver_str(const struct image_version *ver, char *dst);

#ifdef __cplusplus
}
#endif

#endif /* __IMG_PRIV_H */
