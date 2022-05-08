/* Copyright (c) 2022 Trent Piepho <trent.piepho@igorinstitute.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Constants for MCP356x ADCs.
 */
#pragma once

#include <math/ilog2.h>

#define REG_ADCDATA	0x0
#define REG_CONFIG0	0x1
#define REG_CONFIG0_NO_SHUTDOWN		(3 << 6)
#define REG_CONFIG0_SHUTDOWN		(0 << 6) /* plus all other CONFIG0 bits 0 */
#define REG_CONFIG0_CLK_SEL_INT_OUT	(3 << 4)
#define REG_CONFIG0_CLK_SEL_INT		(2 << 4)
#define REG_CONFIG0_CLK_SEL_EXT		(0 << 4)
#define REG_CONFIG0_ADC_MODE_CONV	(3)
#define REG_CONFIG0_ADC_MODE_STBY	(2)
#define REG_CONFIG0_ADC_MODE_SHDN	(0)
#define REG_CONFIG1	0x2
#define REG_CONFIG1_PRE_SHIFT		6
#define REG_CONFIG1_PRE_8		(3 << REG_CONFIG1_PRE_SHIFT)
#define REG_CONFIG1_PRE_4		(2 << REG_CONFIG1_PRE_SHIFT)
#define REG_CONFIG1_PRE_2		(1 << REG_CONFIG1_PRE_SHIFT)
#define REG_CONFIG1_PRE_1		(0 << REG_CONFIG1_PRE_SHIFT)
#define REG_CONFIG1_OSR_SHIFT		2
#define REG_CONFIG1_OSR_MASK		BIT_MASK(4)
#define REG_CONFIG1_OSR_256		(3 << REG_CONFIG1_OSR_SHIFT) /* default */
#define REG_CONFIG2	0x3
#define REG_CONFIG2_BOOST_SHIFT		6
#define REG_CONFIG2_BOOST_2x		(3 << REG_CONFIG2_BOOST_SHIFT)
#define REG_CONFIG2_BOOST_1x		(2 << REG_CONFIG2_BOOST_SHIFT)
#define REG_CONFIG2_BOOST_2x3		(1 << REG_CONFIG2_BOOST_SHIFT)
#define REG_CONFIG2_BOOST_1x2		(0 << REG_CONFIG2_BOOST_SHIFT)
#define REG_CONFIG2_GAIN_1_3		(0 << 3)
#define REG_CONFIG2_GAIN_1		(1 << 3)
#define REG_CONFIG2_GAIN_2		(2 << 3)
#define REG_CONFIG2_GAIN_4		(3 << 3)
#define REG_CONFIG2_GAIN_8		(4 << 3)
#define REG_CONFIG2_GAIN_16		(5 << 3)
#define REG_CONFIG2_GAIN_32		(6 << 3)
#define REG_CONFIG2_GAIN_64		(7 << 3)
#define REG_CONFIG2_AZ_MUX		BIT(2)
#define REG_CONFIG2_RES			(0x3)
#define REG_CONFIG3	0x4
#define REG_CONFIG3_CONV_MODE_CONT	(3 << 6)
#define REG_CONFIG3_CONV_MODE_OS_STBY	(2 << 6)
#define REG_CONFIG3_CONV_MODE_OS_SHDN	(0 << 6)
#define REG_CONFIG3_DATA_FORMAT_32_ID	(3 << 4)
#define REG_CONFIG3_DATA_FORMAT_32_RJ	(2 << 4)
#define REG_CONFIG3_DATA_FORMAT_32_LJ	(1 << 4)
#define REG_CONFIG3_DATA_FORMAT_24	(0 << 4)
#define REG_CONFIG3_CRC_FORMAT		BIT(3)
#define REG_CONFIG3_EN_CRCCOM		BIT(2)
#define REG_CONFIG3_EN_OFFCAL		BIT(1)
#define REG_CONFIG3_EN_GAINCAL		BIT(0)
#define REG_IRQ		0x5
#define REG_IRQ_DR_STATUS		BIT(6)
#define REG_IRQ_CRCCFG_STATUS		BIT(5)
#define REG_IRQ_POR_STATUS		BIT(4)
#define REG_IRQ_MODE_MDAT		BIT(3)
#define REG_IRQ_MODE_PP			BIT(2)
#define REG_IRQ_MODE_HIGHZ		(0)
#define REG_IRQ_EN_FAST_CMD		BIT(1)
#define REG_IRQ_EN_STP			BIT(0)
#define REG_MUX		0x6
#define REG_SCAN	0x7
#define REG_SCAN_DLY(x)			((ilog2(x) - 2) << 21)
#define REG_SCAN_INT_CH_MASK		(0xf000)
#define REG_SCAN_EXT_CH_MASK		(0x0fff)
#define REG_TIMER	0x8
#define REG_OFFSETCAL	0x9
#define REG_GAINCAL	0xa
#define REG_LOCK	0xd
#define REG_LOCK_MAGIC			0xa5
#define REG_DEVID	0xe
#define REG_CRCCFG	0xf

/* Command bits: Operation to perform */
#define OP_FAST_CMD	0
#define OP_READ		1
#define OP_WRITE_INC	2
#define OP_READ_INC	3

/* Command bits: Fast command type, for OP_FAST_CMD */
#define CMD_START	(0xa << 2)
#define CMD_STANDBY	(0xb << 2)
#define CMD_SHUTDOWN	(0xc << 2)
#define CMD_OFF		(0xd << 2)
#define CMD_RESET	(0xe << 2)

/* Command bits: Status bits read back while command is sent */
#define STATUS_nDR	BIT(2)
#define STATUS_nCRCCFG	BIT(1)
#define STATUS_nPOR	BIT(0)

static const uint8_t reg_lens[16] = {
	[REG_ADCDATA]	= 4,
	[REG_CONFIG0]	= 1,
	[REG_CONFIG1]	= 1,
	[REG_CONFIG2]	= 1,
	[REG_CONFIG3]	= 1,
	[REG_IRQ]	= 1,
	[REG_MUX]	= 1,
	[REG_SCAN]	= 3,
	[REG_TIMER]	= 3,
	[REG_OFFSETCAL] = 3,
	[REG_GAINCAL]	= 3,
	[0xb]		= 3,
	[0xc]		= 1,
	[REG_LOCK]	= 1,
	[REG_DEVID]	= 2,
	[REG_CRCCFG]	= 2
};

/* divided by 32 so it fits in one byte */
static const uint8_t osr3_div32[16] = {
	1, 2, 4, 8,		16, 16, 16, 16,
	16, 16, 16, 16,		16, 16, 16, 16,
};
#define OSR3(osr)		(osr3_div32[osr] * 32u)
static const uint8_t osr1[16] = {
	1, 1, 1, 1,		1, 2, 4, 8,
	16, 32, 40, 48,		80, 96, 160, 192
};

/* Conversion time and data rate, see §5.5 */
#define OSR_TO_TODR(osr)	(OSR3(osr) * osr1[osr])
#define OSR_TO_TCONV(osr)	(OSR3(osr) * 3u + (osr1[osr] - 1u) * OSR3(osr))
