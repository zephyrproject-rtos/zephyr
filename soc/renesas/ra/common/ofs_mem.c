/**
 * ofs_mem.c
 *
 * RA2 Option-Setting Memory configuration
 *
 * Copyright (c) 2022-2024 MUNIC Car Data
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <ofs_regs_ra.h>

/* HOCO frequency
 * 0 -> 24 MHz
 * 2 -> 32 MHz
 * 4 -> 48 MHz
 * 5 -> 64 MHz
 */
#define HOCO_FREQ_MAP(idx)						       \
	COND_CODE_1(IS_EQ(idx, 0), (0),					       \
		(COND_CODE_1(IS_EQ(idx, 1), (2),			       \
			(COND_CODE_1(IS_EQ(idx, 2), (4),		       \
				(COND_CODE_1(IS_EQ(idx, 3), (5), (0xff))))))))

#define HOCO_FREQ_INDEX	COND_CODE_1(EARLY_BOOT_HOCO_EN, \
	    (HOCO_FREQ_MAP(DT_ENUM_IDX(DT_NODELABEL(hoco), clock_frequency))), \
	    (0xff))

BUILD_ASSERT(!(IS_ENABLED(EARLY_BOOT_HOCO_EN) && HOCO_FREQ_INDEX == 0xff),
		"Bad HOCO oscillator configuration!");

#define RA2_REG_OFS1_LVDAS_Pos		2
#define RA2_REG_OFS1_LVDAS_Msk	BIT(RA2_REG_OFS1_LVDAS_Pos)
#define RA2_REG_OFS1_LVDAS(x)	\
		(((~(x)) << RA2_REG_OFS1_LVDAS_Pos) & RA2_REG_OFS1_LVDAS_Msk)

#define RA2_REG_OFS1_VDSEL0_Pos		3
#define RA2_REG_OFS1_VDSEL0_Msk	GENMASK(5, 3)
#define RA2_REG_OFS1_VDSEL0(x)	\
		(((x) << RA2_REG_OFS1_VDSEL0_Pos) & RA2_REG_OFS1_VDSEL0_Msk)

#define RA2_REG_OFS1_HOCOEN_Pos		8
#define RA2_REG_OFS1_HOCOEN_Msk	BIT(RA2_REG_OFS1_HOCOEN_Pos)
#define RA2_REG_OFS1_HOCOEN(x)   \
		(((~(x)) << RA2_REG_OFS1_HOCOEN_Pos) & RA2_REG_OFS1_HOCOEN_Msk)

#define RA2_REG_OFS1_HOCOFRQ1_Pos	12
#define RA2_REG_OFS1_HOCOFRQ1_Msk  GENMASK(14, 12)
#define RA2_REG_OFS1_HOCOFRQ1(x)   \
		(((x) << RA2_REG_OFS1_HOCOFRQ1_Pos) & RA2_REG_OFS1_HOCOFRQ1_Msk)

/* Clock architecture
 * 0 -> Type B (Fix the 3 clocks to the same speed)
 * 1 -> Type A (Maximum clock flexibility)
 */

#define RA2_REG_OFS1_ICSATS_Pos		31
#define RA2_REG_OFS1_ICSATS_Msk	BIT(RA2_REG_OFS1_ICSATS_Pos)
#define RA2_REG_OFS1_ICSATS(x)   \
		(((x) << RA2_REG_OFS1_ICSATS_Pos) & RA2_REG_OFS1_ICSATS_Msk)

#define RA2_REG_OFS1(hocoen, hocofrq, lvdas, vdsel, icsats)	\
	(((uint32_t)UINT32_MAX & ~(RA2_REG_OFS1_LVDAS_Msk    | \
			RA2_REG_OFS1_VDSEL0_Msk   | \
			RA2_REG_OFS1_HOCOEN_Msk   | \
			RA2_REG_OFS1_HOCOFRQ1_Msk | \
			RA2_REG_OFS1_ICSATS_Msk)) | \
	 COND_CODE_0(lvdas,		      \
		(RA2_REG_OFS1_LVDAS_Msk | RA2_REG_OFS1_VDSEL0_Msk),	  \
		(RA2_REG_OFS1_VDSEL0(vdsel)))				| \
	 COND_CODE_0(hocoen,						  \
		(RA2_REG_OFS1_HOCOEN_Msk | RA2_REG_OFS1_HOCOFRQ1_Msk),	  \
		(RA2_REG_OFS1_HOCOFRQ1(hocofrq)))			| \
	RA2_REG_OFS1_ICSATS(icsats))

#define SYSC_NODE	DT_NODELABEL(sysc)
#define LVD0_NODE	DT_NODELABEL(lvd0)

static const uint32_t ofs1 Z_GENERIC_DOT_SECTION(ofs_mem.ofs1) __used =
	RA2_REG_OFS1(
		EARLY_BOOT_HOCO_EN,
		HOCO_FREQ_INDEX,
		DT_NODE_HAS_STATUS(LVD0_NODE, okay),
		DT_ENUM_IDX_OR(LVD0_NODE, vdet, 0xff),
		DT_PROP_OR(SYSC_NODE, arch_type_a, 1)
	);

/* TODO Allow changing the security MPU regions in Kconfig/DT */
/* NOTE All security bits are deactivated by default */
#define RA2_SECMPUPCS0 0xFFFFFFFF
#define RA2_SECMPUPCE0 0xFFFFFFFF

#define RA2_SECMPUPCS1 0xFFFFFFFF
#define RA2_SECMPUPCE1 0xFFFFFFFF

#define RA2_SECMPUS0 0xFFFFFFFF
#define RA2_SECMPUE0 0xFFFFFFFF

#define RA2_SECMPUS1 0xFFFFFFFF
#define RA2_SECMPUE1 0xFFFFFFFF

#define RA2_SECMPUS2 0xFFFFFFFF
#define RA2_SECMPUE2 0xFFFFFFFF

#define RA2_SECMPUS3 0xFFFFFFFF
#define RA2_SECMPUE3 0xFFFFFFFF

#define RA2_SECMPUAC 0xFFFFFFFF

struct secmpu {
	uint32_t s;
	uint32_t e;
};

/* TODO Implement the actual masks instead of 0xFF */
static const struct {
	struct secmpu secmpupc[2];
	struct secmpu secmpu[4];
	uint32_t secmpuac;
} __packed ofs_sec_mpu Z_GENERIC_DOT_SECTION(ofs_mem.sec_mpu) __used = {
	.secmpupc = {
		{ RA2_SECMPUPCS0, RA2_SECMPUPCE0 },
		{ RA2_SECMPUPCS1, RA2_SECMPUPCE1 }
	},
	.secmpu = {
		{ RA2_SECMPUS0, RA2_SECMPUE0 },
		{ RA2_SECMPUS1, RA2_SECMPUE1 },
		{ RA2_SECMPUS2, RA2_SECMPUE2 },
		{ RA2_SECMPUS3, RA2_SECMPUE3 },
	},
	.secmpuac = RA2_SECMPUAC,
};
