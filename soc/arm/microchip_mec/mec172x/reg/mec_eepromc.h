/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC_EEPROMC_H
#define _MEC_EEPROMC_H

#include <stdint.h>
#include <stddef.h>

#include "mec_defs.h"

#define MCHP_EEPROM_0_BASE_ADDR 0x40002C00ul

#define MCHP_EEPROM_GIRQ	18u
#define MCHP_EEPROM_NVIC_GIRQ	10u
#define MCHP_EEPROM_POS 13u
#define MCHP_EEPROM_NVIC	155u
#define MCHP_EEPROM_GIRQ_VAL	BIT(13)

/* EC-only registers */

/* EEPROM controller mode register at 0x00 */
#define MCHP_EEPROM_MODE_OFS		0x00u
#define MCHP_EEPROM_MODE_MASK		0x03u
#define MCHP_EEPROM_MODE_ACTV		BIT(0)
#define MCHP_EEPROM_MODE_SRST		BIT(1)

/* EEPROM controller execute register at 0x04 */
#define MCHP_EEPROM_EXE_OFS		0x04u
#define MCHP_EEPROM_EXE_ADDR_POS	0
#define MCHP_EEPROM_EXE_ADDR_MSK	0xFFFFu
#define MCHP_EEPROM_EXE_CMD_POS		16
#define MCHP_EEPROM_EXE_CMD_MSK		0x70000u
#define MCHP_EEPROM_EXE_SZ_POS		24
#define MCHP_EEPROM_EXE_SZ_MSK		0x1F000000u

#define MCHP_EEPROM_EXE_VAL(addr, cmd, sz) \
	((SHLU32(addr, MCHP_EEPROM_EXE_ADDR_POS) & MCHP_EEPROM_EXE_ADDR_MSK) \
	 | (SHLU32(cmd, MCHP_EEPROM_EXE_CMD_POS) & MCHP_EEPROM_EXE_CMD_MSK) \
	 | (SHLU32(sz, MCHP_EEPROM_EXE_SZ_POS) & MCHP_EEPROM_EXE_SZ_MSK))

/* EEPROM controller status register at 0x08 */
#define MCHP_EEPROM_STS_OFS		0x08u
#define MCHP_EEPROM_STS_MASK		0x103u
#define MCHP_EEPROM_STS_DONE		BIT(0)
#define MCHP_EEPROM_STS_ERR		BIT(1)
#define MCHP_EEPROM_STS_ACTIVE_RO	BIT(8)

/* EEPROM controller interrupt enable register at 0x0C */
#define MCHP_EEPROM_IEN_OFS		0x0Cu
#define MCHP_EEPROM_IEN_MASK		0x03u
#define MCHP_EEPROM_IEN_DONE		BIT(0)
#define MCHP_EEPROM_IEN_ERR		BIT(1)

/* EEPROM controller password register at 0x10 (WO) */
#define MCHP_EEPROM_PSWD_OFS		0x10u
#define MCHP_EEPROM_PSWD_MASK		0x7FFFFFFFul

/* EEPROM controller unlock register at 0x14 (WO) */
#define MCHP_EEPROM_UNLOCK_OFS		0x14u
#define MCHP_EEPROM_ULOCK_MASK		0x7FFFFFFFul

/* EEPROM controller lock register at 0x18 */
#define MCHP_EEPROM_LOCK_OFS		0x18u
#define MCHP_EEPROM_LOCK_MASK		0x03u
#define MCHP_EEPROM_LOCK_JTAG		BIT(0)
#define MCHP_EEPROM_LOCK_ACCESS		BIT(1)

/* EEPROM controller buffer/data registers at 0x20 - 0x3F */
#define MCHP_EEPROM_DATA_OFS		0x20u

#endif	/* #ifndef _MEC_EEPROMC_H */
