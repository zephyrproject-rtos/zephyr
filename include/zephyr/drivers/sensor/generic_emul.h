/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for controlling generic emulated sensor test device
 * @ingroup generic_emul
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_GENERIC_EMUL_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_GENERIC_EMUL_H_

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/dsp/types.h>

/**
 * @defgroup generic_emul Generic Sensor Emulator
 * @ingroup sensor_interface_ext_zephyr
 * @brief Generic sensor emulator for testing
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Reset all channels back to unconfigured
 *
 * @param target Generic sensor emulator to reset
 * @param reset_rc Reset the return codes
 */
void generic_emul_reset(const struct emul *target, bool reset_rc);

/**
 * @brief Configure return value for generic sensor device
 *
 * @param target Generic sensor emulator to configure
 * @param resume_rc Return code for @a PM_DEVICE_ACTION_RESUME
 * @param suspend_rc Return code for @a PM_DEVICE_ACTION_SUSPEND
 * @param fetch_rc Return code for @a sensor_sample_fetch and one-shot reads
 */
void generic_emul_func_rc(const struct emul *target, int resume_rc, int suspend_rc, int fetch_rc);

/**
 * @brief Produce one streamed sample
 *
 * Called from the sampling timer, in interrupt context, for each channel of
 * each streamed frame. @p value holds the value last set on the channel, and
 * is read with the shift last set on it.
 *
 * @param target Generic sensor emulator
 * @param ch Channel of the sample
 * @param seq Index of the sample since the stream started
 * @param value Sample value to fill
 * @param user_data User data given to generic_emul_set_sample_source()
 * @retval 0 Success
 * @retval <0 Error to end the read with; a re-armed stream resumes where it
 *         left off
 */
typedef int (*generic_emul_sample_fn)(const struct emul *target, struct sensor_chan_spec ch,
				      uint32_t seq, q31_t *value, void *user_data);

/**
 * @brief Set the source of streamed samples
 *
 * A stream carries every channel that has been set, except
 * SENSOR_CHAN_PROX and SENSOR_CHAN_GAUGE_CYCLE_COUNT, which RTIO reads leave
 * out. Without a source, each sample is the value last set on its channel.
 * An error from @p fn ends the stream. Only for instances with a
 * @a sampling-frequency.
 *
 * @param target Generic sensor emulator to configure
 * @param fn Sample source, or NULL for the set values
 * @param user_data Passed to @p fn
 */
void generic_emul_set_sample_source(const struct emul *target, generic_emul_sample_fn fn,
				    void *user_data);

/**
 * @brief Get the number of samples dropped since the stream started
 *
 * Only for instances with a @a sampling-frequency.
 *
 * @param target Generic sensor emulator to query
 * @return Number of samples produced but not delivered
 */
uint32_t generic_emul_stream_overruns(const struct emul *target);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_GENERIC_EMUL_H_ */
