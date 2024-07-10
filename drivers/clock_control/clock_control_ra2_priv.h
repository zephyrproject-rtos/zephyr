/**
 * clock_control_ra2_priv.h
 *
 * RA2 Clock Generation Circuit drivers common definitions
 *
 * Copyright (c) 2022-2024 MUNIC Car Data
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#ifndef __CLOCK_CONTROL_RA_PRIV_H__
#define __CLOCK_CONTROL_RA_PRIV_H__

#include <soc.h>

#define CGC_BASE	SYSC_BASE

#define CGC_SCKDIVCR	(CGC_BASE + 0x020)

#define CGC_SCKDIVCR_PCKD_Off	0
#define CGC_SCKDIVCR_PCKD_Msk	GENMASK(2, 0)
#define CGC_SCKDIVCR_Msk	GENMASK(2, 0)
#define CGC_SCKDIVCR_PCKD(x)	((x) & CGC_SCKDIVCR_PCKD_Msk)

#define CGC_SCKDIVCR_PCKB_Off	8
#define CGC_SCKDIVCR_PCKB_Msk	GENMASK(10, 8)
#define CGC_SCKDIVCR_PCKB(x)	\
	(((x)<<CGC_SCKDIVCR_PCKB_Off) & CGC_SCKDIVCR_PCKD_Msk)

#define CGC_SCKDIVCR_ICK_Off	24
#define CGC_SCKDIVCR_ICK_Msk	GENMASK(26, 24)
#define CGC_SCKDIVCR_ICK(x)	\
	(((x)<<CGC_SCKDIVCR_ICK_Off) & CGC_SCKDIVCR_ICK_Msk)

#define CGC_SCKSCR	(CGC_BASE + 0x026)
#define CGC_SCKSCR_CKSEL_Msk	GENMASK(2, 0)
#define CGC_SCKSCR_CKSEL(x)	((x) & CGC_SCKSCR_CKSEL_Msk)
#define CGC_SCKSCR_CKSEL_MAX	4

#define CGC_MOSCCR	(CGC_BASE + 0x032)
#define CGC_MOSCCR_MOSTP	BIT(0)

#define CGC_SOSCCR	(CGC_BASE + 0x480)
#define CGC_SOSCCR_SOSTP	BIT(0)

#define CGC_LOCOCR	(CGC_BASE + 0x490)
#define CGC_LOCOCR_LCSTP	BIT(0)

#define CGC_HOCOCR	(CGC_BASE + 0x036)
#define CGC_HOCOCR_HCSTP	BIT(0)

#define CGC_MOCOCR	(CGC_BASE + 0x038)
#define CGC_MOCOCR_MCSTP	BIT(0)

#define CGC_OSCSF	(CGC_BASE + 0x03c)
#define CGC_OSCSF_HOCOSF	BIT(0)
#define CGC_OSCSF_MOSCSF	BIT(3)

#define CGC_OSTDCR	(CGC_BASE + 0x040)
#define CGC_OSTDCR_OSTDIE	BIT(0)
#define CGC_OSTDCR_OSTDE	BIT(7)

#define CGC_OSTDSR	(CGC_BASE + 0x041)
#define CGC_OSTDSR_OSTDF	BIT(0)

#define CGC_MOSCWTCR	(CGC_BASE + 0x0a2)
#define CGC_MOSCWTCR_MSTS_Msk	GENMASK(3, 0)
#define CGC_MOSCWTCR_MSTS(x)	((x) & CGC_MOSCWTCR_MSTS_Msk)

#define CGC_MOMCR	(CGC_BASE + 0x413)
#define CGC_MOMCR_MODRV1	BIT(3)
#define CGC_MOMCR_MOSEL		BIT(6)

#define CGC_SOMCR	(CGC_BASE + 0x481)
#define CGC_SOMCR_SODRV_Msk	GENMASK(1, 0)
#define CGC_SOMCR_SODRV(x)	((x) & CGC_SOMCR_SODRV_Msk)

#define CGC_SOMRG	(CGC_BASE + 0x482)
#define CGC_SOMRG_SOSCMRG_Msk	GENMASK(1, 0)
#define CGC_SOMRG_SOSCMRG	((x) & CGC_SOMRG_SOSCMRG_Msk)

#define CGC_CKOCR	(CGC_BASE + 0x03e)

#define CGC_LOCOUTCR	(CGC_BASE + 0x492)
#define CGC_MOCOUTCR	(CGC_BASE + 0x061)
#define CGC_HOCOUTCR	(CGC_BASE + 0x062)

/* All topmost and internal clocks (like HOCO, MAIN, ICLK etc) have to include
 * this struct to there config as the FIRST member
 */
struct ra_common_osc_config {
	uint8_t id;
};

struct ra_root_osc_data {
	struct k_spinlock lock;
};

#endif /*  __CLOCK_CONTROL_RA2_PRIV_H__ */
