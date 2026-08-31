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
 * The fake EEPROM driver implements every EEPROM API callback as a Fake
 * Function Framework (FFF) fake. It is enabled by @kconfig{CONFIG_EEPROM_FAKE}
 * and instantiated from @dtcompatible{zephyr,fake-eeprom} devicetree nodes.
 *
 * Each fake is named after the API function it backs (`fake_eeprom_read()` for
 * `eeprom_read()`, and so on) and is paired with an FFF control structure
 * carrying an additional `_fake` suffix (`fake_eeprom_read_fake`). Test suites
 * include this header to set return values, install custom fakes, or inspect
 * call counts and captured arguments. See @rstref{mocking-fff}.
 *
 * When @kconfig{CONFIG_ZTEST} is enabled, a ztest rule resets all fakes before
 * each test case. The reset also re-installs a default `custom_fake` for
 * `fake_eeprom_read()` and `fake_eeprom_size()`, which honour the size from
 * devicetree and zero-fill reads. FFF gives `custom_fake` precedence over
 * `return_val`, so clear it before setting a return value for those two.
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

/** @cond INTERNAL_HIDDEN */

DECLARE_FAKE_VALUE_FUNC(int, fake_eeprom_read, const struct device *, off_t, void *, size_t);

DECLARE_FAKE_VALUE_FUNC(int, fake_eeprom_write, const struct device *, off_t, const void *, size_t);

DECLARE_FAKE_VALUE_FUNC(size_t, fake_eeprom_size, const struct device *);

/** @endcond */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_EEPROM_EEPROM_FAKE_H_ */
