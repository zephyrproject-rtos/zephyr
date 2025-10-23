/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#if defined __has_include
#if __has_include("app_version.h")
#include "app_version.h"
#define SOC_IMAGE_VERSION APPVERSION
#endif
#endif

#if !defined(SOC_IMAGE_VERSION)
#include <zephyr/version.h>
#define SOC_IMAGE_VERSION KERNELVERSION
#endif

#if !defined(SOC_IMAGE_TYPE)
#define SOC_IMAGE_TYPE                                                                             \
	((IS_ENABLED(CONFIG_MCUBOOT) ? BIT(6) : 0) | (IS_ENABLED(CONFIG_BT) ? BIT(3) : 0))
#endif

#if !defined(SOC_IMAGE_CAPABILITIES)
#define SOC_IMAGE_CAPABILITIES 0
#endif

#if !defined(SOC_IMAGE_PRODUCT_ID)
#define SOC_IMAGE_PRODUCT_ID {0}
#endif

struct image_info {
	uint32_t type;
	uint32_t version;
	uint32_t capabilities;
	uint8_t id[16];
};

struct image_properties {
	uint8_t magic[16];
	uint32_t header_version;
	uint32_t signature_type;
	uint32_t signature_location;
	struct image_info info;
	void *cert;
	void *token_location;
};

const struct image_properties image_props __attribute__((used, section(".image_properties"))) = {
	.magic = {0x13, 0xb7, 0x79, 0xfa, 0xc9, 0x25, 0xdd, 0xb7, 0xad, 0xf3, 0xcf, 0xe0, 0xf1,
		  0xb6, 0x14, 0xb8},
	.header_version = 0x0101,
	.signature_type = 0,
	.signature_location = 0,
	.info = {.type = SOC_IMAGE_TYPE,
		 .version = SOC_IMAGE_VERSION,
		 .capabilities = SOC_IMAGE_CAPABILITIES,
		 .id = SOC_IMAGE_PRODUCT_ID},
	.cert = NULL,
	.token_location = NULL,
};
