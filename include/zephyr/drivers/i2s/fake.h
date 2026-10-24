/*
 * SPDX-FileCopyrightText: Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Fake I2S driver API functions.
 * @ingroup i2s_fake
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_I2S_FAKE_H_
#define ZEPHYR_INCLUDE_DRIVERS_I2S_FAKE_H_

#include <zephyr/drivers/i2s.h>
#include <zephyr/fff.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Fake I2S driver
 * @defgroup i2s_fake Fake I2S
 * @ingroup io_emulators
 * @ingroup i2s_interface
 *
 * @driver_fake{i2s_interface,CONFIG_I2S_FAKE,zephyr\,fake-i2s}
 *
 * @code{.c}
 * const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(fake_i2s));
 * struct i2s_config config = {0};
 *
 * i2s_fake_configure_fake.return_val = -EINVAL;
 *
 * zassert_equal(-EINVAL, i2s_configure(dev, &config));
 * zassert_equal(1, i2s_fake_configure_fake.call_count);
 * zassert_equal(dev, i2s_fake_configure_fake.arg0_val);
 * @endcode
 *
 * @{
 */

/** @fake_of{i2s_driver_api::configure} */
DECLARE_FAKE_VALUE_FUNC(int,
			i2s_fake_configure,
			const struct device *,
			enum i2s_dir,
			const struct i2s_config *);

/** @fake_of{i2s_driver_api::config_get} */
DECLARE_FAKE_VALUE_FUNC(const struct i2s_config *,
			i2s_fake_config_get,
			const struct device *,
			enum i2s_dir);

/** @fake_of{i2s_driver_api::read} */
DECLARE_FAKE_VALUE_FUNC(int,
			i2s_fake_read,
			const struct device *,
			void **,
			size_t *);

/** @fake_of{i2s_driver_api::write} */
DECLARE_FAKE_VALUE_FUNC(int,
			i2s_fake_write,
			const struct device *,
			void *,
			size_t);

/** @fake_of{i2s_driver_api::trigger} */
DECLARE_FAKE_VALUE_FUNC(int,
			i2s_fake_trigger,
			const struct device *,
			enum i2s_dir,
			enum i2s_trigger_cmd);

#if defined(CONFIG_I2S_RTIO) || defined(__DOXYGEN__)
/**
 * @fake_of{i2s_driver_api::iodev_submit}
 * @kconfig_dep{CONFIG_I2S_RTIO}
 */
DECLARE_FAKE_VOID_FUNC(i2s_fake_iodev_submit,
		       const struct device *,
		       struct rtio_iodev_sqe *);
#endif /* CONFIG_I2S_RTIO */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_I2S_FAKE_H_ */
