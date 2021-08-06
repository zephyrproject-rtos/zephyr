/*
 * Copyright 2020 Broadcom
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTC_GICV3_PRIV_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTC_GICV3_PRIV_H_

#include <zephyr/types.h>
#include <device.h>

/* Cache and Share ability for ITS & Redistributor LPI state tables */
#define GIC_BASER_CACHE_NGNRNE		0x0UL /* Device-nGnRnE */
#define GIC_BASER_CACHE_INNERLIKE	0x0UL /* Same as Inner Cacheability. */
#define GIC_BASER_CACHE_NCACHEABLE	0x1UL /* Normal Outer Non-cacheable */
#define GIC_BASER_CACHE_RAWT		0x2UL /* Normal Outer Cacheable Read-allocate, Write-through */
#define GIC_BASER_CACHE_RAWB		0x3UL /* Normal Outer Cacheable Read-allocate, Write-back */
#define GIC_BASER_CACHE_WAWT		0x4UL /* Normal Outer Cacheable Write-allocate, Write-through */
#define GIC_BASER_CACHE_WAWB		0x5UL /* Normal Outer Cacheable Write-allocate, Write-back */
#define GIC_BASER_CACHE_RAWAWT		0x6UL /* Normal Outer Cacheable Read-allocate, Write-allocate, Write-through */
#define GIC_BASER_CACHE_RAWAWB		0x7UL /* Normal Outer Cacheable Read-allocate, Write-allocate, Write-back */
#define GIC_BASER_SHARE_NO		0x0UL /* Non-shareable */
#define GIC_BASER_SHARE_INNER		0x1UL /* Inner Shareable */
#define GIC_BASER_SHARE_OUTER		0x2UL /* Outer Shareable */

/*
 * GIC Register Interface Base Addresses
 */

#define GIC_RDIST_BASE	DT_REG_ADDR_BY_IDX(DT_INST(0, arm_gic), 1)

/* SGI base is at 64K offset from Redistributor */
#define GICR_SGI_BASE_OFF		0x10000

/* GICR registers offset from RD_base(n) */
#define GICR_CTLR			0x0000
#define GICR_IIDR			0x0004
#define GICR_TYPER			0x0008
#define GICR_STATUSR			0x0010
#define GICR_WAKER			0x0014
#define GICR_PROPBASER			0x0070
#define GICR_PENDBASER			0x0078

/* Register bit definations */

/* GICD_CTLR Interrupt group definitions */
#define GICD_CTLR_ENABLE_G0		0
#define GICD_CTLR_ENABLE_G1NS		1
#define GICD_CTLR_ENABLE_G1S		2
#define GICD_CTRL_ARE_S			4
#define GICD_CTRL_ARE_NS		5
#define GICD_CTRL_NS			6
#define GICD_CGRL_E1NWF			7

/* GICD_CTLR Register write progress bit */
#define GICD_CTLR_RWP			31

/* GICR_CTLR */
#define GICR_CTLR_ENABLE_LPIS		BIT(0)
#define GICR_CTLR_RWP			3

/* GICR_WAKER */
#define GICR_WAKER_PS			1
#define GICR_WAKER_CA			2

/* GICR_PROPBASER */
#define GITR_PROPBASER_ID_BITS_MASK		0x1fUL
#define GITR_PROPBASER_INNER_CACHE_SHIFT	7
#define GITR_PROPBASER_INNER_CACHE_MASK		0x7UL
#define GITR_PROPBASER_SHAREABILITY_SHIFT	10
#define GITR_PROPBASER_SHAREABILITY_MASK	0x3UL
#define GITR_PROPBASER_ADDR_SHIFT		12
#define GITR_PROPBASER_ADDR_MASK		0xFFFFFFFFFFUL
#define GITR_PROPBASER_OUTER_CACHE_SHIFT	56
#define GITR_PROPBASER_OUTER_CACHE_MASK		0x7UL

/* GICR_PENDBASER */
#define GITR_PENDBASER_INNER_CACHE_SHIFT	7
#define GITR_PENDBASER_INNER_CACHE_MASK		0x7UL
#define GITR_PENDBASER_SHAREABILITY_SHIFT	10
#define GITR_PENDBASER_SHAREABILITY_MASK	0x3UL
#define GITR_PENDBASER_ADDR_SHIFT		16
#define GITR_PENDBASER_ADDR_MASK		0xFFFFFFFFFUL
#define GITR_PENDBASER_OUTER_CACHE_SHIFT	56
#define GITR_PENDBASER_OUTER_CACHE_MASK		0x7UL
#define GITR_PENDBASER_PTZ			BIT64(62)

/* GITCD_IROUTER */
#define GIC_DIST_IROUTER		0x6000
#define IROUTER(base, n)		(base + GIC_DIST_IROUTER + (n) * 8)

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTC_GICV3_PRIV_H_ */
