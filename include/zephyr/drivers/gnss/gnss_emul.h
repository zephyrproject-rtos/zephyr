/*
 * Copyright (c) 2025 Embeint Pty Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_GNSS_GNSS_EMUL_H_
#define ZEPHYR_DRIVERS_GNSS_GNSS_EMUL_H_

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gnss.h>

/**
 * @brief Clear all internal GNSS data of the emulator
 *
 * @details Resets the cached navigation data, fix info, and (when
 * CONFIG_GNSS_ACCURACY is enabled) the cached per-axis accuracy. After this
 * call, the next emulator tick publishes zeroed gnss_data and does NOT
 * publish a gnss_accuracy event until @ref gnss_emul_set_accuracy is called
 * again.
 *
 * @param dev GNSS emulator device
 */
void gnss_emul_clear_data(const struct device *dev);

/**
 * @brief Set the internal GNSS data of the emulator
 *
 * @param dev GNSS emulator device
 * @param nav Updated navigation state
 * @param info Updated GNSS fix information
 * @param boot_realtime_ms Unix timestamp associated with system boot
 */
void gnss_emul_set_data(const struct device *dev, const struct navigation_data *nav,
			const struct gnss_info *info, int64_t boot_realtime_ms);

#if defined(CONFIG_GNSS_ACCURACY) || defined(__DOXYGEN__)
/**
 * @brief Set the per-axis accuracy values published on every emulator tick
 *
 * @details After this call the emulator publishes the supplied accuracy via
 * gnss_publish_accuracy() on every fix epoch (alongside the data publish).
 * Accuracy publishing is sticky — call @ref gnss_emul_clear_data to disable.
 *
 * Only available when CONFIG_GNSS_EMUL_MANUAL_UPDATE=y.
 *
 * @param dev GNSS emulator device
 * @param accuracy Accuracy state to publish; copied into the device, the
 *                 caller's storage may be reused after the call returns
 */
void gnss_emul_set_accuracy(const struct device *dev, const struct gnss_accuracy *accuracy);
#endif

#if defined(CONFIG_GNSS_RAW_NMEA) || defined(__DOXYGEN__)
/**
 * @brief Inject a raw NMEA sentence into the emulator output stream
 *
 * @details Publishes @p sentence via gnss_publish_raw_nmea() synchronously,
 * before the call returns. Independent of the fix-epoch tick — no state is
 * cached. Useful for tests that need to assert byte-faithful forwarding of
 * a specific sentence.
 *
 * Only available when CONFIG_GNSS_RAW_NMEA=y.
 *
 * @param dev GNSS emulator device
 * @param sentence NMEA sentence bytes — delimiter must NOT be included.
 *                 The receiver sees @p sentence verbatim.
 * @param len Length of @p sentence in bytes
 */
void gnss_emul_inject_raw_sentence(const struct device *dev,
				   const char *sentence, size_t len);
#endif

/**
 * @brief Retrieve the last configured fix rate, regardless of PM state
 *
 * @param dev GNSS emulator device
 * @param fix_interval_ms Output fix interval
 *
 * @retval 0 On success
 */
int gnss_emul_get_fix_rate(const struct device *dev, uint32_t *fix_interval_ms);

/**
 * @brief Retrieve the last configured navigation mode, regardless of PM state
 *
 * @param dev GNSS emulator device
 * @param mode Output navigation mode
 *
 * @retval 0 On success
 */
int gnss_emul_get_navigation_mode(const struct device *dev, enum gnss_navigation_mode *mode);

/**
 * @brief Retrieve the last configured systems, regardless of PM state
 *
 * @param dev GNSS emulator device
 * @param systems Output GNSS systems
 *
 * @retval 0 On success
 */
int gnss_emul_get_enabled_systems(const struct device *dev, gnss_systems_t *systems);

#endif /* ZEPHYR_DRIVERS_GNSS_GNSS_EMUL_H_ */
