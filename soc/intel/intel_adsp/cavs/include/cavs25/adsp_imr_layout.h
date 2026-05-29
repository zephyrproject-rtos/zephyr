/* Copyright (c) 2023 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SOC_INTEL_ADSP_TGL_IMR_LAYOUT_H_
#define ZEPHYR_SOC_INTEL_ADSP_TGL_IMR_LAYOUT_H_

/* These structs and macros are from the ROM code header
 * on cAVS platforms, please keep them immutable.
 *
 * The ROM structs and code is lifted from:
 * https://github.com/thesofproject/sof
 * 6c0db22c65 - platform: cavs: configure resume from IMR
 */

/*
 * A magic that tells ROM to jump to imr_restore_vector instead of normal boot
 */
#define ADSP_IMR_MAGIC_VALUE		0x02468ACE

struct imr_header {
	uint32_t adsp_imr_magic;
	uint32_t structure_version;
	uint32_t structure_size;
	uint32_t imr_state;
	uint32_t imr_size;
	void (*imr_restore_vector)(void);
};

struct imr_state {
	struct imr_header header;
	uint8_t reserved[0x1000 - sizeof(struct imr_header)];
};

struct imr_layout {
	uint8_t css_reserved[0x1000];
	struct imr_state imr_state;
};

#endif /* ZEPHYR_SOC_INTEL_ADSP_TGL_IMR_LAYOUT_H_ */
