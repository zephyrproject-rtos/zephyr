/*
 * Copyright 2020 Broadcom
 * Copyright 2025 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTC_GICV3_PRIV_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTC_GICV3_PRIV_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/sys/atomic.h>

/* Cache and Share ability for ITS & Redistributor LPI state tables */
#define GIC_BASER_CACHE_NGNRNE     0x0UL /* Device-nGnRnE */
#define GIC_BASER_CACHE_INNERLIKE  0x0UL /* Same as Inner Cacheability. */
#define GIC_BASER_CACHE_NCACHEABLE 0x1UL /* Non-cacheable */
#define GIC_BASER_CACHE_RAWT       0x2UL /* Cacheable R-allocate, W-through */
#define GIC_BASER_CACHE_RAWB       0x3UL /* Cacheable R-allocate, W-back */
#define GIC_BASER_CACHE_WAWT       0x4UL /* Cacheable W-allocate, W-through */
#define GIC_BASER_CACHE_WAWB       0x5UL /* Cacheable W-allocate, W-back */
#define GIC_BASER_CACHE_RAWAWT     0x6UL /* Cacheable R-allocate, W-allocate, W-through */
#define GIC_BASER_CACHE_RAWAWB     0x7UL /* Cacheable R-allocate, W-allocate, W-back */
#define GIC_BASER_SHARE_NO         0x0UL /* Non-shareable */
#define GIC_BASER_SHARE_INNER      0x1UL /* Inner Shareable */
#define GIC_BASER_SHARE_OUTER      0x2UL /* Outer Shareable */

/* SGI base is at 64K offset from Redistributor */
#define GICR_SGI_BASE_OFF 0x10000

/* GICR registers offset from RD_base(n) */
#define GICR_CTLR      0x0000
#define GICR_IIDR      0x0004
#define GICR_TYPER     0x0008
#define GICR_STATUSR   0x0010
#define GICR_WAKER     0x0014
#define GICR_PWRR      0x0024
#define GICR_PROPBASER 0x0070
#define GICR_PENDBASER 0x0078

/* Register bit definitions */

/* GICD_CTLR Interrupt group definitions */
#define GICD_CTLR_ENABLE_G0   0
#define GICD_CTLR_ENABLE_G1NS 1
#define GICD_CTLR_ENABLE_G1S  2
#define GICD_CTRL_ARE_S       4
#define GICD_CTRL_ARE_NS      5
#define GICD_CTRL_NS          6
#define GICD_CGRL_E1NWF       7

/* GICD_CTLR Register write progress bit */
#define GICD_CTLR_RWP 31

/* GICR_CTLR */
#define GICR_CTLR_ENABLE_LPIS BIT(0)
#define GICR_CTLR_RWP         3

/* GICR_IIDR */
#define GICR_IIDR_PRODUCT_ID_SHIFT     24
#define GICR_IIDR_PRODUCT_ID_MASK      0xFFUL
#define GICR_IIDR_PRODUCT_ID_GET(_val) MASK_GET(_val, GICR_IIDR_PRODUCT_ID)

/* GICR_TYPER */
#define GICR_TYPER_AFFINITY_VALUE_SHIFT       32
#define GICR_TYPER_AFFINITY_VALUE_MASK        0xFFFFFFFFUL
#define GICR_TYPER_AFFINITY_VALUE_GET(_val)   MASK_GET(_val, GICR_TYPER_AFFINITY_VALUE)
#define GICR_TYPER_LAST_SHIFT                 4
#define GICR_TYPER_LAST_MASK                  0x1UL
#define GICR_TYPER_LAST_GET(_val)             MASK_GET(_val, GICR_TYPER_LAST)
#define GICR_TYPER_PROCESSOR_NUMBER_SHIFT     8
#define GICR_TYPER_PROCESSOR_NUMBER_MASK      0xFFFFUL
#define GICR_TYPER_PROCESSOR_NUMBER_GET(_val) MASK_GET(_val, GICR_TYPER_PROCESSOR_NUMBER)

/* GICR_WAKER */
#define GICR_WAKER_PS 1
#define GICR_WAKER_CA 2

/* GICR_PWRR */
#define GICR_PWRR_RDPD  0
#define GICR_PWRR_RDAG  1
#define GICR_PWRR_RDGPO 3

/* GICR_PROPBASER */
#define GITR_PROPBASER_ID_BITS_MASK       0x1fUL
#define GITR_PROPBASER_INNER_CACHE_SHIFT  7
#define GITR_PROPBASER_INNER_CACHE_MASK   0x7UL
#define GITR_PROPBASER_SHAREABILITY_SHIFT 10
#define GITR_PROPBASER_SHAREABILITY_MASK  0x3UL
#define GITR_PROPBASER_ADDR_SHIFT         12
#define GITR_PROPBASER_ADDR_MASK          0xFFFFFFFFFFUL
#define GITR_PROPBASER_OUTER_CACHE_SHIFT  56
#define GITR_PROPBASER_OUTER_CACHE_MASK   0x7UL

/* GICR_PENDBASER */
#define GITR_PENDBASER_INNER_CACHE_SHIFT  7
#define GITR_PENDBASER_INNER_CACHE_MASK   0x7UL
#define GITR_PENDBASER_SHAREABILITY_SHIFT 10
#define GITR_PENDBASER_SHAREABILITY_MASK  0x3UL
#define GITR_PENDBASER_ADDR_SHIFT         16
#define GITR_PENDBASER_ADDR_MASK          0xFFFFFFFFFUL
#define GITR_PENDBASER_OUTER_CACHE_SHIFT  56
#define GITR_PENDBASER_OUTER_CACHE_MASK   0x7UL
#define GITR_PENDBASER_PTZ                BIT64(62)

/* GICD_IROUTER */
#define GIC_DIST_IROUTER 0x6000
#define IROUTER(base, n) (base + GIC_DIST_IROUTER + (n) * 8)

/* GICD_IROUTERnE for GICv3.1 Extended SPI Range */
#define GIC_DIST_IROUTERnE 0x8000
#define IROUTERnE(base, n) (base + GIC_DIST_IROUTERnE + (n) * 8)

/*
 * ITS registers, offsets from ITS_base
 */
#define GITS_CTLR     0x0000
#define GITS_IIDR     0x0004
#define GITS_TYPER    0x0008
#define GITS_STATUSR  0x0040
#define GITS_UMSIR    0x0048
#define GITS_CBASER   0x0080
#define GITS_CWRITER  0x0088
#define GITS_CREADR   0x0090
#define GITS_BASER(n) (0x0100 + ((n) * 8))

#define GITS_TRANSLATER 0x10040

/* ITS CTLR register */
#define GITS_CTLR_ENABLED_SHIFT    0
#define GITS_CTLR_ENABLED_MASK     0x1UL
#define GITS_CTLR_ITS_NUMBER_SHIFT 4
#define GITS_CTLR_ITS_NUMBER_MASK  0xfUL
#define GITS_CTLR_QUIESCENT_SHIFT  31
#define GITS_CTLR_QUIESCENT_MASK   0x1UL

#define GITS_CTLR_ENABLED_GET(_val)   MASK_GET(_val, GITS_CTLR_ENABLED)
#define GITS_CTLR_QUIESCENT_GET(_val) MASK_GET(_val, GITS_CTLR_QUIESCENT)

/* ITS TYPER register */
#define GITS_TYPER_PHY_SHIFT            0
#define GITS_TYPER_PHY_MASK             0x1UL
#define GITS_TYPER_VIRT_SHIFT           1
#define GITS_TYPER_VIRT_MASK            0x1UL
#define GITS_TYPER_ITT_ENTRY_SIZE_SHIFT 4
#define GITS_TYPER_ITT_ENTRY_SIZE_MASK  0xfUL
#define GITS_TYPER_IDBITS_SHIFT         8
#define GITS_TYPER_IDBITS_MASK          0x1fUL
#define GITS_TYPER_DEVBITS_SHIFT        13
#define GITS_TYPER_DEVBITS_MASK         0x1fUL
#define GITS_TYPER_SEIS_SHIFT           18
#define GITS_TYPER_SEIS_MASK            0x1UL
#define GITS_TYPER_PTA_SHIFT            19
#define GITS_TYPER_PTA_MASK             0x1UL
#define GITS_TYPER_HCC_SHIFT            24
#define GITS_TYPER_HCC_MASK             0xffUL
#define GITS_TYPER_CIDBITS_SHIFT        32
#define GITS_TYPER_CIDBITS_MASK         0xfUL
#define GITS_TYPER_CIL_SHIFT            36
#define GITS_TYPER_CIL_MASK             0x1UL

#define GITS_TYPER_ITT_ENTRY_SIZE_GET(_val) MASK_GET(_val, GITS_TYPER_ITT_ENTRY_SIZE)
#define GITS_TYPER_PTA_GET(_val)            MASK_GET(_val, GITS_TYPER_PTA)
#define GITS_TYPER_HCC_GET(_val)            MASK_GET(_val, GITS_TYPER_HCC)
#define GITS_TYPER_DEVBITS_GET(_val)        MASK_GET(_val, GITS_TYPER_DEVBITS)

/* ITS COMMON BASER / CBASER register */

/* ITS CBASER register */
#define GITS_CBASER_SIZE_SHIFT         0
#define GITS_CBASER_SIZE_MASK          0xffUL
#define GITS_CBASER_SHAREABILITY_SHIFT 10
#define GITS_CBASER_SHAREABILITY_MASK  0x3UL
#define GITS_CBASER_ADDR_SHIFT         12
#define GITS_CBASER_ADDR_MASK          0xfffffffffUL
#define GITS_CBASER_OUTER_CACHE_SHIFT  53
#define GITS_CBASER_OUTER_CACHE_MASK   0x7UL
#define GITS_CBASER_INNER_CACHE_SHIFT  59
#define GITS_CBASER_INNER_CACHE_MASK   0x7UL
#define GITS_CBASER_VALID_SHIFT        63
#define GITS_CBASER_VALID_MASK         0x1UL

/* ITS BASER<n> register */
#define GITS_BASER_SIZE_SHIFT         0
#define GITS_BASER_SIZE_MASK          0xffUL
#define GITS_BASER_PAGE_SIZE_SHIFT    8
#define GITS_BASER_PAGE_SIZE_MASK     0x3UL
#define GITS_BASER_PAGE_SIZE_4K       0
#define GITS_BASER_PAGE_SIZE_16K      1
#define GITS_BASER_PAGE_SIZE_64K      2
#define GITS_BASER_SHAREABILITY_SHIFT 10
#define GITS_BASER_SHAREABILITY_MASK  0x3UL
#define GITS_BASER_ADDR_SHIFT         12
#define GITS_BASER_ADDR_MASK          0xfffffffff
#define GITS_BASER_ENTRY_SIZE_SHIFT   48
#define GITS_BASER_ENTRY_SIZE_MASK    0x1fUL
#define GITS_BASER_OUTER_CACHE_SHIFT  53
#define GITS_BASER_OUTER_CACHE_MASK   0x7UL
#define GITS_BASER_TYPE_SHIFT         56
#define GITS_BASER_TYPE_MASK          0x7UL
#define GITS_BASER_INNER_CACHE_SHIFT  59
#define GITS_BASER_INNER_CACHE_MASK   0x7UL
#define GITS_BASER_INDIRECT_SHIFT     62
#define GITS_BASER_INDIRECT_MASK      0x1UL
#define GITS_BASER_VALID_SHIFT        63
#define GITS_BASER_VALID_MASK         0x1UL

#define GITS_BASER_TYPE_NONE       0
#define GITS_BASER_TYPE_DEVICE     1
#define GITS_BASER_TYPE_COLLECTION 4

#define GITS_BASER_TYPE_GET(_val)       MASK_GET(_val, GITS_BASER_TYPE)
#define GITS_BASER_PAGE_SIZE_GET(_val)  MASK_GET(_val, GITS_BASER_PAGE_SIZE)
#define GITS_BASER_ENTRY_SIZE_GET(_val) MASK_GET(_val, GITS_BASER_ENTRY_SIZE)
#define GITS_BASER_INDIRECT_GET(_val)   MASK_GET(_val, GITS_BASER_INDIRECT)

#define GITS_BASER_NR_REGS 8

/* ITS Commands */

#define GITS_CMD_ID_MOVI    0x01
#define GITS_CMD_ID_INT     0x03
#define GITS_CMD_ID_CLEAR   0x04
#define GITS_CMD_ID_SYNC    0x05
#define GITS_CMD_ID_MAPD    0x08
#define GITS_CMD_ID_MAPC    0x09
#define GITS_CMD_ID_MAPTI   0x0a
#define GITS_CMD_ID_MAPI    0x0b
#define GITS_CMD_ID_INV     0x0c
#define GITS_CMD_ID_INVALL  0x0d
#define GITS_CMD_ID_MOVALL  0x0e
#define GITS_CMD_ID_DISCARD 0x0f

#define GITS_CMD_ID_OFFSET 0
#define GITS_CMD_ID_SHIFT  0
#define GITS_CMD_ID_MASK   0xffUL

#define GITS_CMD_DEVICEID_OFFSET 0
#define GITS_CMD_DEVICEID_SHIFT  32
#define GITS_CMD_DEVICEID_MASK   0xffffffffUL

#define GITS_CMD_SIZE_OFFSET 1
#define GITS_CMD_SIZE_SHIFT  0
#define GITS_CMD_SIZE_MASK   0x1fUL

#define GITS_CMD_EVENTID_OFFSET 1
#define GITS_CMD_EVENTID_SHIFT  0
#define GITS_CMD_EVENTID_MASK   0xffffffffUL

#define GITS_CMD_PINTID_OFFSET 1
#define GITS_CMD_PINTID_SHIFT  32
#define GITS_CMD_PINTID_MASK   0xffffffffUL

#define GITS_CMD_ICID_OFFSET 2
#define GITS_CMD_ICID_SHIFT  0
#define GITS_CMD_ICID_MASK   0xffffUL

#define GITS_CMD_ITTADDR_OFFSET   2
#define GITS_CMD_ITTADDR_SHIFT    8
#define GITS_CMD_ITTADDR_MASK     0xffffffffffUL
#define GITS_CMD_ITTADDR_ALIGN    GITS_CMD_ITTADDR_SHIFT
#define GITS_CMD_ITTADDR_ALIGN_SZ (BIT(0) << GITS_CMD_ITTADDR_ALIGN)

#define GITS_CMD_RDBASE_OFFSET 2
#define GITS_CMD_RDBASE_SHIFT  16
#define GITS_CMD_RDBASE_MASK   0xffffffffUL
#define GITS_CMD_RDBASE_ALIGN  GITS_CMD_RDBASE_SHIFT

#define GITS_CMD_VALID_OFFSET 2
#define GITS_CMD_VALID_SHIFT  63
#define GITS_CMD_VALID_MASK   0x1UL

#define MASK(__basename)            (__basename##_MASK << __basename##_SHIFT)
#define MASK_SET(__val, __basename) (((__val) & __basename##_MASK) << __basename##_SHIFT)
#define MASK_GET(__reg, __basename) (((__reg) >> __basename##_SHIFT) & __basename##_MASK)

#ifdef CONFIG_GIC_V3_ITS
void its_rdist_map(void);
void its_rdist_invall(void);

extern atomic_t nlpi_intid;
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTC_GICV3_PRIV_H_ */
