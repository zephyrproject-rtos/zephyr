/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_IMAGE_
#define H_IMAGE_

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IMAGE_MAGIC			0x96f3b83d
#define IMAGE_TLV_INFO_MAGIC		0x6907
#define IMAGE_TLV_PROT_INFO_MAGIC	0x6908

#define IMAGE_HEADER_SIZE		32

/** Image header flags. */
#define IMAGE_F_NON_BOOTABLE		0x00000010 /* Split image app. */
#define IMAGE_F_ROM_FIXED_ADDR		0x00000100

/** Image trailer TLV types. */
#define IMAGE_TLV_SHA256		0x10   /* SHA256 of image hdr and body */

/** Image TLV-specific definitions. */
#define IMAGE_HASH_LEN			32

struct image_version {
	uint8_t iv_major;
	uint8_t iv_minor;
	uint16_t iv_revision;
	uint32_t iv_build_num;
} __packed;

/** Image header.  All fields are in little endian byte order. */
struct image_header {
	uint32_t ih_magic;
	uint32_t ih_load_addr;
	uint16_t ih_hdr_size; /* Size of image header (bytes). */
	uint16_t _pad2;
	uint32_t ih_img_size; /* Does not include header. */
	uint32_t ih_flags;	/* IMAGE_F_[...]. */
	struct image_version ih_ver;
	uint32_t _pad3;
} __packed;

/** Image TLV header.  All fields in little endian. */
struct image_tlv_info {
	uint16_t it_magic;
	uint16_t it_tlv_tot;  /* size of TLV area (including tlv_info header) */
} __packed;

/** Image trailer TLV format. All fields in little endian. */
struct image_tlv {
	uint8_t  it_type;   /* IMAGE_TLV_[...]. */
	uint8_t  _pad;
	uint16_t it_len;	/* Data length (not including TLV header). */
} __packed;

_Static_assert(sizeof(struct image_header) == IMAGE_HEADER_SIZE,
			   "struct image_header not required size");

#ifdef __cplusplus
}
#endif

#endif
