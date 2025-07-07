/*
 * Copyright (c) 2025 Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_EEPROM_AT25XV021A_EEPROM_H_
#define ZEPHYR_INCLUDE_DRIVERS_EEPROM_AT25XV021A_EEPROM_H_

#include <zephyr/drivers/eeprom.h>

#ifdef __cplusplus
extern "C" {
#endif

int eeprom_at25xv021a_chip_erase(const struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_EEPROM_AT25XV021A_EEPROM_H_ */
