/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_IMG_MGMT_INFO_
#define H_IMG_MGMT_INFO_

#include <inttypes.h>
#include "image.h"
#include "mgmt/mgmt.h"
#include <zcbor_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 255.255.65535.4294967295\0 */
#define IMG_MGMT_VER_MAX_STR_LEN	(sizeof("255.255.65535.4294967295"))

/*
 * @brief Read info of an image give the slot number
 *
 * @param image_slot	 Image slot to read info from
 * @param image_version  Image version to be filled up
 * @param hash		   Ptr to the read image hash
 * @param flags		  Ptr to flags filled up from the image
 */
int img_mgmt_read_info(int image_slot, struct image_version *ver, uint8_t *hash, uint32_t *flags);

/**
 * @brief Get the current running image version
 *
 * @param image_version Given image version
 *
 * @return 0 on success, non-zero on failure
 */
int img_mgmt_my_version(struct image_version *ver);

/**
 * @brief Get image version in string from image_version
 *
 * @param image_version Structure filled with image version
 *					  information
 * @param dst		   Destination string created from the given
 *					  in image version
 *
 * @return Non-negative on success, negative value on error.
 */
int img_mgmt_ver_str(const struct image_version *ver, char *dst);

/**
 * Compares two image version numbers in a semver-compatible way.
 *
 * @param a	The first version to compare.
 * @param b	The second version to compare.
 *
 * @return	-1 if a < b
 * @return	0 if a = b
 * @return	1 if a > b
 */
int img_mgmt_vercmp(const struct image_version *a, const struct image_version *b);

#ifdef __cplusplus
}
#endif

#endif /* H_IMG_MGMT_INFO_ */
