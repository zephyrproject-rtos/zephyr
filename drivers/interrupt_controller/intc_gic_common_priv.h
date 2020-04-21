/*
 * Copyright 2020 Broadcom
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTC_GIC_COMMON_PRIV_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTC_GIC_COMMON_PRIV_H_

/* Offsets from GICD base or GICR(n) SGI_base */
#define GIC_DIST_IGROUPR		0x0080
#define GIC_DIST_ISENABLER		0x0100
#define GIC_DIST_ICENABLER		0x0180
#define GIC_DIST_ISPENDR		0x0200
#define GIC_DIST_ICPENDR		0x0280
#define GIC_DIST_ISACTIVER		0x0300
#define GIC_DIST_ICACTIVER		0x0380
#define GIC_DIST_IPRIORITYR		0x0400
#define GIC_DIST_ITARGETSR		0x0800
#define GIC_DIST_ICFGR			0x0c00
#define GIC_DIST_IGROUPMODR		0x0d00
#define GIC_DIST_SGIR			0x0f00

/* GICD GICR common access macros */
#define IGROUPR(base, n)		(base + GIC_DIST_IGROUPR + (n) * 4)
#define ISENABLER(base, n)		(base + GIC_DIST_ISENABLER + (n) * 4)
#define ICENABLER(base, n)		(base + GIC_DIST_ICENABLER + (n) * 4)
#define ISPENDR(base, n)		(base + GIC_DIST_ISPENDR + (n) * 4)
#define ICPENDR(base, n)		(base + GIC_DIST_ICPENDR + (n) * 4)
#define IPRIORITYR(base, n)		(base + GIC_DIST_IPRIORITYR + n)
#define ITARGETSR(base, n)		(base + GIC_DIST_ITARGETSR + (n) * 4)
#define ICFGR(base, n)			(base + GIC_DIST_ICFGR + (n) * 4)
#define IGROUPMODR(base, n)		(base + GIC_DIST_IGROUPMODR + (n) * 4)

/*
 * selects redistributor SGI_base for current core for PPI and SGI
 * selects distributor base for SPI
 * The macro translates to distributor base for GICv2 and GICv1
 */

#if CONFIG_GIC_VER <= 2
#define GET_DIST_BASE(intid)	GIC_DIST_BASE
#else
#define GET_DIST_BASE(intid)	((intid < GIC_SPI_INT_BASE) ? \
				(GIC_GET_RDIST(GET_CPUID) + GICR_SGI_BASE_OFF) \
				: GIC_DIST_BASE)
#endif
#endif /* ZEPHYR_INCLUDE_DRIVERS_INTC_GIC_COMMON_PRIV_H */
