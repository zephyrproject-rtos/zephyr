/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Fake EEPROM driver API functions.
 * @ingroup eeprom_fake
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_EEPROM_EEPROM_FAKE_H_
#define ZEPHYR_INCLUDE_DRIVERS_EEPROM_EEPROM_FAKE_H_

#include <zephyr/drivers/eeprom.h>
#include <zephyr/fff.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Fake EEPROM driver
 * @defgroup eeprom_fake Fake EEPROM
 * @ingroup io_emulators
 * @ingroup eeprom_interface
 *
 * @driver_fake{eeprom_interface,CONFIG_EEPROM_FAKE,zephyr\,fake-eeprom}
 *
 * `fake_eeprom_read()` and `fake_eeprom_size()` are given a default
 * `custom_fake` honouring the size from devicetree and zero-filling reads,
 * re-installed on every reset. Install your own `custom_fake` in the test case
 * to change what they report.
 *
 * @code{.c}
 * const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(fake_eeprom));
 * const uint8_t data[] = {0x01, 0x02};
 *
 * zassert_ok(eeprom_write(dev, 8, data, sizeof(data)));
 *
 * zassert_equal(1, fake_eeprom_write_fake.call_count);
 * zassert_equal(dev, fake_eeprom_write_fake.arg0_val);
 * zassert_equal(8, fake_eeprom_write_fake.arg1_val);
 * zassert_equal(sizeof(data), fake_eeprom_write_fake.arg3_val);
 * @endcode
 *
 * @{
 */

/** @fake_of{eeprom_driver_api::read} */
DECLARE_FAKE_VALUE_FUNC(int, fake_eeprom_read, const struct device *, off_t, void *, size_t);

/** @fake_of{eeprom_driver_api::write} */
DECLARE_FAKE_VALUE_FUNC(int, fake_eeprom_write, const struct device *, off_t, const void *, size_t);

/** @fake_of{eeprom_driver_api::size} */
DECLARE_FAKE_VALUE_FUNC(size_t, fake_eeprom_size, const struct device *);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_EEPROM_EEPROM_FAKE_H_ */
