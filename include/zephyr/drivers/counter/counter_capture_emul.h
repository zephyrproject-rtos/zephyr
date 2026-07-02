/*
 * Copyright (c) 2026 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Backend API for emulated counter capture
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_COUNTER_COUNTER_CAPTURE_EMUL_H_
#define ZEPHYR_INCLUDE_DRIVERS_COUNTER_COUNTER_CAPTURE_EMUL_H_

#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Emulated counter backend API
 * @defgroup counter_emul Emulated counter
 * @ingroup io_emulators
 * @ingroup counter_interface
 * @{
 *
 * Behaviour of the emulated counter capture is application-defined. Tests
 * drive a capture event by setting the emulated input level on a capture
 * channel with @ref counter_capture_emul_input_set. When the level transition
 * matches the edge the channel was configured for (via
 * @ref counter_capture_configure) and the channel is enabled, the registered
 * capture callback is invoked synchronously, much like @ref gpio_emul_input_set
 * fires GPIO interrupt callbacks.
 *
 * The edges an emulated counter supports are declared in devicetree using the
 * `rising-edge` and `falling-edge` boolean properties on the counter node.
 */

/**
 * @brief Set the emulated capture input level on a channel
 *
 * Drives @p level on the emulated capture input of @p chan_id. If the resulting
 * edge matches the channel's configured capture flags and the channel is
 * enabled, the capture callback is invoked synchronously with the current
 * counter value.
 *
 * @param dev Emulated counter device
 * @param chan_id Capture channel ID
 * @param level New input level (0 or non-zero)
 *
 * @return 0 on success
 * @return -EINVAL if @p chan_id is out of range
 */
int counter_capture_emul_input_set(const struct device *dev, uint8_t chan_id, int level);

/**
 * @brief Set the emulated capture input level on a channel from a DT spec
 *
 * Equivalent to:
 *
 *     counter_capture_emul_input_set(spec->dev, spec->chan_id, level);
 *
 * @param spec Counter capture DT spec
 * @param level New input level (0 or non-zero)
 *
 * @return a value from counter_capture_emul_input_set()
 */
static inline int counter_capture_emul_input_set_dt(const struct counter_capture_dt_spec *spec,
						    int level)
{
	return counter_capture_emul_input_set(spec->dev, spec->chan_id, level);
}

/**
 * @brief Get the emulated capture input level on a channel
 *
 * @param dev Emulated counter device
 * @param chan_id Capture channel ID
 *
 * @return the current input level (0 or 1) on success
 * @return -EINVAL if @p chan_id is out of range
 */
int counter_capture_emul_input_get(const struct device *dev, uint8_t chan_id);

/**
 * @brief Get the emulated capture input level on a channel from a DT spec
 *
 * Equivalent to:
 *
 *     counter_capture_emul_input_get(spec->dev, spec->chan_id);
 *
 * @param spec Counter capture DT spec
 *
 * @return a value from counter_capture_emul_input_get()
 */
static inline int counter_capture_emul_input_get_dt(const struct counter_capture_dt_spec *spec)
{
	return counter_capture_emul_input_get(spec->dev, spec->chan_id);
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_COUNTER_COUNTER_CAPTURE_EMUL_H_ */
