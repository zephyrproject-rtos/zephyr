/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header file for buzzer driver API.
 * @ingroup buzzer_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_BUZZER_H_
#define ZEPHYR_INCLUDE_DRIVERS_BUZZER_H_

/**
 * @brief Interfaces for buzzers
 * @defgroup buzzer_interface Buzzer
 * @since 4.5
 * @version 0.1.0
 * @ingroup io_interfaces
 *
 * Audio output abstraction for simple buzzers and similar single-tone
 * sound sources.
 *
 * Buzzer API calls do not wait for the requested tone duration: a
 * duration describes how long the hardware should keep emitting a tone,
 * not how long the calling thread should sleep.
 *
 * @{
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Special frequency value representing silence. */
#define BUZZER_FREQ_REST 0U

/** @brief Maximum volume level, range is 0 to BUZZER_VOLUME_MAX. */
#define BUZZER_VOLUME_MAX 100U

/**
 * @brief Special duration meaning "play until explicitly stopped".
 *
 * Use this as the @p duration_ms argument to buzzer_tone() or
 * buzzer_beep() when the tone should not be silenced automatically.
 * The caller must invoke buzzer_stop() or start another tone to end it.
 */
#define BUZZER_DURATION_FOREVER UINT32_MAX

/**
 * @def_driverbackendgroup{Buzzer, buzzer_interface}
 * @{
 */

/**
 * @brief Play a single tone for a given duration.
 *
 * See buzzer_tone() for argument descriptions.
 */
typedef int (*buzzer_tone_t)(const struct device *dev, uint32_t freq_hz,
			     uint32_t duration_ms);

/**
 * @brief Set the buzzer volume.
 *
 * See buzzer_set_volume() for argument descriptions.
 */
typedef int (*buzzer_set_volume_t)(const struct device *dev, uint8_t percent);

/**
 * @brief Play the buzzer's default tone.
 *
 * See buzzer_beep() for argument descriptions.
 */
typedef int (*buzzer_beep_t)(const struct device *dev, uint32_t duration_ms);

/**
 * @brief Stop any tone currently being played.
 *
 * See buzzer_stop() for argument descriptions.
 */
typedef int (*buzzer_stop_t)(const struct device *dev);

/**
 * @driver_ops{Buzzer}
 */
__subsystem struct buzzer_driver_api {
	/** @driver_ops_mandatory @copybrief buzzer_tone */
	buzzer_tone_t tone;
	/** @driver_ops_mandatory @copybrief buzzer_set_volume */
	buzzer_set_volume_t set_volume;
	/** @driver_ops_mandatory @copybrief buzzer_beep */
	buzzer_beep_t beep;
	/** @driver_ops_mandatory @copybrief buzzer_stop */
	buzzer_stop_t stop;
};

/** @} */

/**
 * @brief Play a single tone at @p freq_hz for @p duration_ms.
 *
 * The call returns after the hardware has been configured; it does not
 * wait for @p duration_ms to elapse. The buzzer keeps emitting the tone
 * for @p duration_ms, after which the driver silences it automatically.
 * Calling buzzer_tone() again while a previous tone is still playing
 * preempts it.
 *
 * Passing @ref BUZZER_FREQ_REST silences the buzzer immediately.
 *
 * Backends that cannot honor a requested frequency exactly play their
 * nearest supported tone or, if the request is for silence, stop.
 *
 * @param dev          Buzzer device.
 * @param freq_hz      Tone frequency in Hz.
 * @param duration_ms  How long the buzzer should keep emitting the
 *                     tone. Use @ref BUZZER_DURATION_FOREVER to play
 *                     until an explicit buzzer_stop() or another
 *                     tone-emitting call.
 *
 * @retval 0       Tone scheduled successfully.
 * @retval -errno  Negative errno code on failure.
 */
static inline int buzzer_tone(const struct device *dev, uint32_t freq_hz,
			      uint32_t duration_ms)
{
	return DEVICE_API_GET(buzzer, dev)->tone(dev, freq_hz, duration_ms);
}

/**
 * @brief Set the buzzer volume.
 *
 * Volume is expressed as a percentage between 0 and
 * @ref BUZZER_VOLUME_MAX, where 0 is silent and BUZZER_VOLUME_MAX is
 * the loudest level the hardware can produce. The volume control
 * affects subsequent tones until changed. Setting the volume to 0 also
 * silences the buzzer immediately.
 *
 * Backends without programmatic volume control map any non-zero value to
 * their single audible level and zero to silent.
 *
 * @param dev      Buzzer device.
 * @param percent  Desired volume, 0 to @ref BUZZER_VOLUME_MAX inclusive.
 *
 * @retval 0        Volume updated successfully.
 * @retval -EINVAL  @p percent is out of range.
 * @retval -errno   Negative errno code on failure.
 */
static inline int buzzer_set_volume(const struct device *dev, uint8_t percent)
{
	return DEVICE_API_GET(buzzer, dev)->set_volume(dev, percent);
}

/**
 * @brief Play the buzzer's default tone for @p duration_ms.
 *
 * The call returns after the hardware has been configured; it does not
 * wait for @p duration_ms to elapse. For PWM-driven passive piezos, the
 * default tone is derived from the period cell of the device's @c pwms
 * devicetree property. For GPIO-driven active buzzers, the hardware
 * oscillator determines the actual pitch.
 *
 * @param dev          Buzzer device.
 * @param duration_ms  How long the buzzer should keep emitting the
 *                     tone. Use @ref BUZZER_DURATION_FOREVER to play
 *                     until an explicit buzzer_stop() or another
 *                     tone-emitting call.
 *
 * @retval 0       Tone scheduled successfully.
 * @retval -errno  Negative errno code on failure.
 */
static inline int buzzer_beep(const struct device *dev, uint32_t duration_ms)
{
	return DEVICE_API_GET(buzzer, dev)->beep(dev, duration_ms);
}

/**
 * @brief Stop any tone currently being played.
 *
 * Idempotent: calling stop() on an already-silent buzzer is a no-op.
 *
 * @param dev  Buzzer device.
 *
 * @retval 0       Buzzer stopped.
 * @retval -errno  Negative errno code on failure.
 */
static inline int buzzer_stop(const struct device *dev)
{
	return DEVICE_API_GET(buzzer, dev)->stop(dev);
}

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_BUZZER_H_ */
