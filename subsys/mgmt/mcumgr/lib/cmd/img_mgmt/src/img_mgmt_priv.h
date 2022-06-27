/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_IMG_MGMT_PRIV_
#define H_IMG_MGMT_PRIV_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Response to list:
 * {
 *	"images":[ <version1>, <version2>]
 * }
 *
 *
 * Request to boot to version:
 * {
 *	"test":<version>
 * }
 *
 *
 * Response to boot read:
 * {
 *	"test":<version>,
 *	"main":<version>,
 *	"active":<version>
 * }
 *
 *
 * Request to image upload:
 * {
 *	"off":<offset>,
 *	"len":<img_size>		inspected when off = 0
 *	"data":<base64encoded binary>
 * }
 *
 *
 * Response to upload:
 * {
 *	"off":<offset>
 * }
 *
 *
 * Request to image upload:
 * {
 *	"off":<offset>
 *	"name":<filename>	inspected when off = 0
 *	"len":<file_size>	inspected when off = 0
 *	"data":<base64encoded binary>
 * }
 */

struct mgmt_ctxt;

int img_mgmt_find_by_hash(uint8_t *find, struct image_version *ver);
int img_mgmt_find_by_ver(struct image_version *find, uint8_t *hash);
int img_mgmt_state_read(struct mgmt_ctxt *ctxt);
int img_mgmt_state_write(struct mgmt_ctxt *njb);

#ifdef __cplusplus
}
#endif

#endif /* __IMG_MGMT_PRIV_H */
