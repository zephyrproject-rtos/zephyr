/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_IMG_MGMT_CONFIG_
#define H_IMG_MGMT_CONFIG_

/* Number of updatable images */
#define IMG_MGMT_UPDATABLE_IMAGE_NUMBER 1
/* Image status list will only contain image attributes that are true/non-zero */
#define IMG_MGMT_FRUGAL_LIST    0

#define IMG_MGMT_UL_CHUNK_SIZE  CONFIG_IMG_MGMT_UL_CHUNK_SIZE
#define IMG_MGMT_VERBOSE_ERR    CONFIG_IMG_MGMT_VERBOSE_ERR
#define IMG_MGMT_LAZY_ERASE     CONFIG_IMG_ERASE_PROGRESSIVELY
#define IMG_MGMT_DUMMY_HDR      CONFIG_IMG_MGMT_DUMMY_HDR
#define IMG_MGMT_BOOT_CURR_SLOT 0
#ifdef CONFIG_IMG_MGMT_UPDATABLE_IMAGE_NUMBER
/* Up to two images are supported */
#undef IMG_MGMT_UPDATABLE_IMAGE_NUMBER
#define IMG_MGMT_UPDATABLE_IMAGE_NUMBER CONFIG_IMG_MGMT_UPDATABLE_IMAGE_NUMBER
#endif
#ifdef CONFIG_IMG_MGMT_FRUGAL_LIST
#undef IMG_MGMT_FRUGAL_LIST
#define IMG_MGMT_FRUGAL_LIST    CONFIG_IMG_MGMT_FRUGAL_LIST
#endif

#endif
