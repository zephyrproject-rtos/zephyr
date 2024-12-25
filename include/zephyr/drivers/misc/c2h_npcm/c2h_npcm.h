/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MISC_C2H_NPCM_H_
#define ZEPHYR_INCLUDE_DRIVERS_MISC_C2H_NPCM_H_

#include <soc.h>

void c2h_write_io_cfg_reg(const struct device *dev, uint8_t reg_index, uint8_t reg_data);

uint8_t c2h_read_io_cfg_reg(const struct device *dev, uint8_t reg_index);

void rtc_write_offset(const struct device *dev, SIB_RTC_OFFSET_Enum offset, uint8_t value);

uint8_t rtc_read_offset(const struct device *dev, SIB_RTC_OFFSET_Enum offset);

#endif
