/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_NXP_IMXRT_IMXRT266X_XSPI_NOR_BOOT_H_
#define ZEPHYR_SOC_NXP_IMXRT_IMXRT266X_XSPI_NOR_BOOT_H_

#include <stdint.h>
#include <zephyr/toolchain.h>

/*
 * RT266x AHAB boot container.
 *
 * This SoC's boot ROM is container-based: it reads a container header, an image
 * array entry describing where and how to place the image, and a signature
 * block. The layout and field values below match the RT266x reference boot
 * header; on an evaluation part the out-of-fab life cycle skips authentication,
 * so the signature block is present but empty.
 */

/* Container header. */
#define CONTAINER_VER        0x02U
#define CONTAINER_SIZE       ((uint16_t)sizeof(boot_container_t))
#define CONTAINER_HEADER_TAG 0x87U
#define CONTAINER_FLAGS      0x00000000U /* not authenticated */
#define CONTAINER_SW_VER     0U
#define CONTAINER_FUSE_VER   1U
#define CONTAINER_NUM_IMG    1U
#define CONTAINER_CERT_VER   1U
#define CONTAINER_SIGNATURE_BLOCK_OFFSET                                                           \
	((uint16_t)(sizeof(boot_container_header_t) +                                              \
		    CONTAINER_NUM_IMG * sizeof(boot_image_entry_t)))

struct __packed boot_container_header {
	uint8_t version;
	uint16_t length;
	uint8_t tag;

	uint32_t flags;

	uint16_t sw_ver;
	uint8_t fuse_ver;
	uint8_t num_images;

	uint16_t signature_block_offset;
	uint8_t cert_ver;
	uint8_t reserved1;
};

typedef struct boot_container_header boot_container_header_t;

/* Image array entry: Type=Executable(0x3) | Core ID=0x1 (CM85) | Hash=SHA2_512(0x2). */
#define IMG_FLAGS 0x00000213U

struct __packed boot_image_entry {
	uint32_t offset;
	uint32_t size;
	uint32_t load_addr;
	uint32_t load_addr_high;
	uint32_t entry;
	uint32_t entry_high;
	uint32_t flags;
	uint32_t metadata;
	uint8_t hash[64];
	uint8_t iv[32];
};

typedef struct boot_image_entry boot_image_entry_t;

/* Signature block: left empty on an unauthenticated evaluation image. */
#define SIGNATURE_BLOCK_VER  0x01U
#define SIGNATURE_BLOCK_SIZE ((uint16_t)sizeof(boot_signature_block_t))
#define SIGNATURE_BLOCK_TAG  0x90U

struct __packed boot_signature_block {
	uint8_t version;
	uint16_t length;
	uint8_t tag;

	uint16_t cert_offset;
	uint16_t srk_table_array_offset;

	uint16_t signature_offset;
	uint16_t blob_offset;

	uint32_t key_identifier;
};

typedef struct boot_signature_block boot_signature_block_t;

struct __packed boot_container {
	boot_container_header_t header;
	boot_image_entry_t image_array[CONTAINER_NUM_IMG];
	boot_signature_block_t signature_block;
};

typedef struct boot_container boot_container_t;

#endif /* ZEPHYR_SOC_NXP_IMXRT_IMXRT266X_XSPI_NOR_BOOT_H_ */
