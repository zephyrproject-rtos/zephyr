/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_EEPROM_FAKE_EEPROM_H_
#define ZEPHYR_INCLUDE_DRIVERS_EEPROM_FAKE_EEPROM_H_

#include <zephyr/drivers/eeprom.h>
#include <zephyr/fff.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

DECLARE_FAKE_VALUE_FUNC(int, fake_eeprom_read, const struct device *, k_off_t, void *, size_t);

DECLARE_FAKE_VALUE_FUNC(int, fake_eeprom_write, const struct device *, k_off_t, const void *,
			size_t);

DECLARE_FAKE_VALUE_FUNC(size_t, fake_eeprom_size, const struct device *);

size_t fake_eeprom_size_delegate(const struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_EEPROM_FAKE_EEPROM_H_ */
