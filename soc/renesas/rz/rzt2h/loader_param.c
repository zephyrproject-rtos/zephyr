/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/linker/section_tags.h>

#define CACHE_FLG            (0x00000000)
#define WRAPCFG_V_DUMMY0     (0x00000000)
#define COMCFG_V_DUMMY1      (0x00000000)
#define BMCFG_V_DUMMY2       (0x00000000)
#define RESTORE_FLG          (0x00000000)
#define LDR_ADDR_NML         (0x50000050)
#define LDR_SIZE_NML         (0x00006000)
#define DEST_ADDR_NML        (0x00102000)
#define DUMMY3               (0x00000000)
#define DUMMY4               (0x00000000)
#define CS0_SIZE_DUMMY5      (0x000FFFFF)
#define LIOCFGCS0_V_DUMMY6   (0x00070000)
#define PLL0_SSC_CTR_V       (0x00000000)
#define BOOTCPU_FLG          (0x00000000)
#define DUMMY7               (0x00000000)
#define DUMMY8               (0x00000000)
#define DUMMY9               (0x00000000)
#define ACCESS_SPEED_DUMMY10 (0x00000600)
#define CHECK_SUM            (0x1D675)
#define LOADER_PARAM_MAX     (19)

#define __loader_param Z_GENERIC_SECTION(.loader_param)

const uint32_t loader_param[LOADER_PARAM_MAX] __loader_param = {
	CACHE_FLG,
	WRAPCFG_V_DUMMY0,
	COMCFG_V_DUMMY1,
	BMCFG_V_DUMMY2,
	RESTORE_FLG,
	LDR_ADDR_NML,
	LDR_SIZE_NML,
	DEST_ADDR_NML,
	DUMMY3,
	DUMMY4,
	CS0_SIZE_DUMMY5,
	LIOCFGCS0_V_DUMMY6,
	PLL0_SSC_CTR_V,
	BOOTCPU_FLG,
	DUMMY7,
	DUMMY8,
	DUMMY9,
	ACCESS_SPEED_DUMMY10,
	CHECK_SUM
};
