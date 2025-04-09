/*
 * Copyright (c) 2025 GP Orcullo
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FLASH_FLASH_SAM0_H_
#define ZEPHYR_DRIVERS_FLASH_FLASH_SAM0_H_

/* MCLK register */

#define APBBMASK_NVMCTRL_BIT 2

#if defined(CONFIG_SOC_SERIES_SAMD20) ||                                                           \
	defined(CONFIG_SOC_SERIES_SAMD21) && defined(CONFIG_SOC_SERIES_SAMR21)
#define APBBMASK_OFFSET 0x1C
#else
#define APBBMASK_OFFSET 0x18
#endif

/* NVMCTRL registers */

#if defined(CONFIG_SOC_SERIES_SAMD51) || defined(CONFIG_SOC_SERIES_SAME51) ||                      \
	defined(CONFIG_SOC_SERIES_SAME53) || defined(CONFIG_SOC_SERIES_SAME54)

#define FLASH_PAGE_SIZE 512
#define ROW_SIZE        8192

#define NVME_BIT 6

#define CTRLA_WMODE_MASK (GENMASK(5, 4))

#define CTRL_CMD_ER  0x01
#define CTRL_CMD_LR  0x11
#define CTRL_CMD_PBC 0x15
#define CTRL_CMD_UR  0x12
#define CTRL_CMD_WP  0x03

#define ADDR_OFFSET   0x14
#define CTRLA_OFFSET  0x00
#define CTRLB_OFFSET  0x04
#define READY_OFFSET  0x12
#define STATUS_OFFSET 0x10

#define CTRL_OFFSET CTRLB_OFFSET

#else /* ! (SERIES_SAMD51 || SERIES_SAME51 || SERIES_SAME53) || SERIES_SAME54) */

#define FLASH_PAGE_SIZE 64
#define ROW_SIZE        256

#define NVME_BIT 4

#define CTRLB_MANW_BIT 7

#define CTRL_CMD_ER  0x02
#define CTRL_CMD_LR  0x40
#define CTRL_CMD_PBC 0x44
#define CTRL_CMD_UR  0x41
#define CTRL_CMD_WP  0x04

#define ADDR_OFFSET   0x1C
#define CTRLA_OFFSET  0x00
#define CTRLB_OFFSET  0x04
#define READY_OFFSET  0x14
#define STATUS_OFFSET 0x18

#define CTRL_OFFSET CTRLA_OFFSET

#endif

#define READY_BIT 0
#define PROGE_BIT 2
#define LOCKE_BIT 3

#define CTRL_CMDEX_KEY FIELD_PREP(GENMASK(15, 8), 0xA5)

#endif /* ZEPHYR_DRIVERS_FLASH_FLASH_SAM0_H_ */
