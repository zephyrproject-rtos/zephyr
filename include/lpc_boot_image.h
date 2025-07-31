/*
 * Copyright (c) 2024 VCI Development - LPC54S018J4MET180E
 * Private Porting , by David Hor - Xtooltech 2025, david.hor@xtooltech.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LPC_BOOT_IMAGE_H
#define LPC_BOOT_IMAGE_H

#include <stdint.h>

/* LPC Boot ROM expects these markers in the image */
#define LPC_BOOT_HEADER_MARKER   0xFEEDA5A5  /* Header marker in vector table */
#define LPC_BOOT_IMAGE_MARKER    0xEDDC94BD  /* Image marker at offset 0x24 */

/* Image types supported by LPC54S018 */
enum lpc_image_type {
	LPC_IMAGE_TYPE_PLAIN     = 0x00000000,  /* Plain image, no authentication */
	LPC_IMAGE_TYPE_CRC       = 0x00000001,  /* CRC32 checksum only */
	LPC_IMAGE_TYPE_CMAC      = 0x00000002,  /* CMAC authentication */
	LPC_IMAGE_TYPE_ECDSA     = 0x00000003,  /* ECDSA P-256 signature */
	LPC_IMAGE_TYPE_ENCRYPTED = 0x00000010,  /* Image is encrypted (OR with auth type) */
};

/* Boot image header structure for LPC54S018 (based on actual vector table layout) */
struct lpc_boot_header {
	uint32_t header_marker;    /* 0x00: Header marker (0xFEEDA5A5) */
	uint32_t image_type;       /* 0x04: Image type and flags */
	uint32_t load_address;     /* 0x08: Load address */
	uint32_t load_length;      /* 0x0C: Load length (excluding CRC) */
	uint32_t crc32;           /* 0x10: CRC32 value */
	uint32_t version;         /* 0x14: Version (for DUAL_ENH images) */
	uint32_t emc_settings;    /* 0x18: EMC static memory config */
	uint32_t image_baudrate;  /* 0x1C: Image baudrate */
	uint32_t reserved;        /* 0x20: Reserved */
	uint32_t image_marker;    /* 0x24: Image marker (0xEDDC94BD) */
	uint32_t reserved2[2];    /* 0x28: Reserved */
	/* SPI flash descriptor follows for SPIFI boot */
} __packed;

/* CMAC authentication tag */
struct lpc_cmac_auth {
	uint8_t cmac[16];         /* AES-128-CMAC authentication tag */
} __packed;

/* ECDSA signature */
struct lpc_ecdsa_signature {
	uint8_t r[32];            /* ECDSA signature R component */
	uint8_t s[32];            /* ECDSA signature S component */
} __packed;

/* Certificate header for key storage */
struct lpc_certificate {
	uint32_t cert_type;       /* Certificate type */
	uint32_t cert_length;     /* Total certificate length */
	uint32_t key_type;        /* Key type (RSA/ECDSA) */
	uint32_t key_length;      /* Key length in bits */
	uint8_t  key_hash[32];    /* SHA-256 hash of public key */
	/* Followed by actual key data */
} __packed;

/* SPI flash descriptor for SPIFI boot */
struct lpc_spi_descriptor {
	uint32_t dev_str_addr;     /* Device string address */
	uint32_t mfg_id_ext;       /* Manufacturer ID + ext count */
	uint32_t ext_id[2];        /* Extended ID */
	uint32_t caps;             /* Capabilities */
	uint32_t blocks_resv;      /* Blocks + reserved */
	uint32_t block_size;       /* Block size */
	uint32_t reserved;         /* Reserved */
	uint32_t page_size_resv;   /* Page size + reserved */
	uint32_t max_read_size;    /* Max read size */
	uint32_t clk_rates;        /* Clock rates */
	uint32_t func_ids1;        /* Function IDs */
	uint32_t func_ids2;        /* More function IDs */
} __packed;

/* Complete boot image structure */
struct lpc_boot_image {
	/* Note: Header is part of vector table, not separate */
	uint8_t vectors[0x158];    /* Interrupt vectors up to header */
	struct lpc_boot_header header;
	struct lpc_spi_descriptor spi_desc;  /* For SPIFI boot */
	/* Application code follows */
} __packed;

/* Helper macros */
#define LPC_BOOT_HEADER_SIZE     sizeof(struct lpc_boot_header)
#define LPC_BOOT_IS_ENCRYPTED(type) ((type) & LPC_IMAGE_TYPE_ENCRYPTED)
#define LPC_BOOT_AUTH_TYPE(type)    ((type) & 0x0F)

/* CRC32 polynomial used by LPC boot ROM */
#define LPC_CRC32_POLY          0xEDB88320

/* Calculate CRC32 for boot image */
uint32_t lpc_boot_crc32(const uint8_t *data, size_t len);

/* Validate boot header */
int lpc_boot_validate_header(const struct lpc_boot_header *header);

/* Check if image version meets requirements */
bool lpc_boot_check_version(const struct lpc_boot_header *header, uint8_t min_version);

#endif /* LPC_BOOT_IMAGE_H */