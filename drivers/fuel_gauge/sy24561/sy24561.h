/*
 * Copyright (c) 2025 Romain Paupe <rpaupe@free.fr>, Franck Duriez <franck.lucien.duriez@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr driver for SY24561 Battery Monitor
 *
 * Datasheet:
 * https://www.silergy.com/download/downloadFile?id=4987&type=product&ftype=note
 */
#pragma once

enum sy24561_reg {
	SY24561_REG_VBAT = 0x02,
	SY24561_REG_SOC = 0x04,
	SY24561_REG_MODE = 0x06,
	SY24561_REG_VERSION = 0x08,
	SY24561_REG_CONFIG = 0x0C,
	SY24561_REG_RESET = 0x18,
	SY24561_REG_STATUS = 0x1A,
	SY24561_REG_POR = 0xFE,
};
