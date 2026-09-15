/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file cs47l63_fault.h
 * @brief CS47L63 fault decode, sticky fault state and error callback (internal)
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_CS47L63_FAULT_H_
#define ZEPHYR_DRIVERS_AUDIO_CS47L63_FAULT_H_

#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Loss of the system clock or of FLL1 lock.
 *
 * Driver-specific, above the class API's own @ref audio_codec_error_type bits.
 * The class enumerates over-current, over-temperature, under- and over-voltage
 * and DC, none of which names a clock failure - and a clock that stopped is the
 * single most common reason this part goes silent, so it is reported rather
 * than folded into a flag that means something else.
 *
 * This part has no thermal-shutdown status bit at all: nothing in the vendor
 * register map reports temperature, so @ref AUDIO_CODEC_ERROR_OVERTEMPERATURE
 * is never raised by this driver.
 */
#define CS47L63_ERROR_CLOCK BIT(5)

/**
 * @brief The headphone amplifier never reported itself enabled.
 *
 * Driver-specific for the same reason as @ref CS47L63_ERROR_CLOCK: the class
 * API has no bit for a stage that simply did not come up. Raised by the
 * deferred check that completes a start, so it means the enable was written,
 * the clock had time to arrive, and OUT1L_EN_STS still reads back low - a dead
 * output rather than a start that was merely early.
 */
#define CS47L63_ERROR_OUTPUT BIT(6)

/**
 * @brief Class API @c register_error_callback.
 *
 * @param dev Codec device
 * @param cb  Callback invoked with a bitmask of @ref audio_codec_error_type
 *            values plus the driver-specific @ref CS47L63_ERROR_CLOCK and
 *            @ref CS47L63_ERROR_OUTPUT, or NULL to unregister
 * @return 0
 */
int cs47l63_fault_register_callback(const struct device *dev, audio_codec_error_callback_t cb);

/**
 * @brief Class API @c clear_errors.
 *
 * Clears the chip's sticky interrupt flags as well as the driver's shadow. A
 * clear that only forgets locally leaves the flags set on the part, so the
 * next poll re-reports the same fault forever.
 *
 * @param dev Codec device
 * @return 0 on success, negative errno on a bus failure
 */
int cs47l63_fault_clear(const struct device *dev);

/**
 * @brief Read the fault flags, latch anything new, and report it once.
 *
 * The callback fires only for bits this call newly observed, so a fault that
 * is still asserted on the next poll is not reported a second time. With no
 * callback registered the bits are still latched and still cleared correctly,
 * so nothing accumulates on the part.
 *
 * The part does drive an interrupt line, but this driver polls instead: it is
 * called from the class operations that already touch the output stage -
 * @c start_output and an output volume or mute property set. A caller that
 * only configures and streams will not learn about a fault until one of those
 * happens.
 *
 * @param dev Codec device
 * @return 0 on success, negative errno on a bus failure
 */
int cs47l63_fault_check(const struct device *dev);

/**
 * @brief Latch a fault observed elsewhere in the driver and report it once.
 *
 * The reporting half of @ref cs47l63_fault_check, for a condition that is not
 * one of the part's interrupt flags - the FLL still being unlocked, or the
 * amplifier still not enabled, once the clock has had time to run. Same once-per-fault contract:
 * the callback fires only for bits that were not latched already.
 *
 * @param dev Codec device
 * @param errors Bitmask of @ref audio_codec_error_type values plus
 *               @ref CS47L63_ERROR_CLOCK and @ref CS47L63_ERROR_OUTPUT
 */
void cs47l63_fault_raise(const struct device *dev, uint32_t errors);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_AUDIO_CS47L63_FAULT_H_ */
