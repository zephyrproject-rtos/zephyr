/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/linker/section_tags.h>

#define CACHE_FLG            (0x00000000)
#define CS0BCR_V_WRAPCFG_V   (0x00000000)
#define CS0WCR_V_COMCFG_V    (0x00000000)
#define DUMMY0_BMCFG_V       (0x00000000)
#define BSC_FLG_xSPI_FLG     (0x00000000)
#define LDR_ADDR_NML         (0x6000004C)
#define LDR_SIZE_NML         (0x00006000)
#define DEST_ADDR_NML        (0x00102000)
#define DUMMY1               (0x00000000)
#define DUMMY2               (0x00000000)
#define DUMMY3_CSSCTL_V      (0x0000003F)
#define DUMMY4_LIOCFGCS0_V   (0x00070000)
#define DUMMY5               (0x00000000)
#define DUMMY6               (0x00000000)
#define DUMMY7               (0x00000000)
#define DUMMY8               (0x00000000)
#define DUMMY9               (0x00000000)
#define DUMMY10_ACCESS_SPEED (0x00000006)
#define CHECK_SUM            (0xE0A8)
#define LOADER_PARAM_MAX     (19)

#define __loader_param Z_GENERIC_SECTION(.loader_param)

const uint32_t loader_param[LOADER_PARAM_MAX] __loader_param = {
	CACHE_FLG,
	CS0BCR_V_WRAPCFG_V,
	CS0WCR_V_COMCFG_V,
	DUMMY0_BMCFG_V,
	BSC_FLG_xSPI_FLG,
	LDR_ADDR_NML,
	LDR_SIZE_NML,
	DEST_ADDR_NML,
	DUMMY1,
	DUMMY2,
	DUMMY3_CSSCTL_V,
	DUMMY4_LIOCFGCS0_V,
	DUMMY5,
	DUMMY6,
	DUMMY7,
	DUMMY8,
	DUMMY9,
	DUMMY10_ACCESS_SPEED,
	CHECK_SUM,
};
