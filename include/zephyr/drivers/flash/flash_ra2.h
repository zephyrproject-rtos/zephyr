/*
 * flash_ra2.h
 *
 * RA2 on-chip FLash CoNtroller driver declarations
 *
 * Copyright (c) 2023-2024 MUNIC SA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef __INCLUDE_DRIVERS_FLASH_FLASH_RA_H__
#define __INCLUDE_DRIVERS_FLASH_FLASH_RA_H__

#define FLCN_BASE	DT_REG_ADDR(DT_NODELABEL(flcn))

/*  Prefetch Buffer Enable Register */
#define FLCN_FLDWAITR	(FLCN_BASE + 0x3fc4)
#define FLCN_FLDWAITR_FLDWAIT1	BIT(0)

/*  Prefetch Buffer Enable Register */
#define FLCN_PFBER	(FLCN_BASE + 0x3fc8)
#define FLCN_PFBER_PFBE	BIT(0)

#endif /* __INCLUDE_DRIVERS_FLASH_FLASH_RA_H__ */
