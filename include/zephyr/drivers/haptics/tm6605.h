/*
 * Copyright (c) 2026 Anuj Deshpande
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file providing the API for the TM6605 haptic driver
 * @ingroup tm6605_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_HAPTICS_TM6605_H_
#define ZEPHYR_INCLUDE_DRIVERS_HAPTICS_TM6605_H_

#include <zephyr/drivers/haptics.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup tm6605_interface TM6605
 * @ingroup haptics_interface_ext
 * @brief TM6605 Linear Resonant Actuator (LRA) Haptic Driver
 *
 * The TM6605 from Titan Micro Electronics is an I2C-controlled haptic driver
 * for LRAs. It exposes 44 pre-programmed effects selectable by ID and is
 * controlled by writing two registers (effect select and playback control).
 * The I2C interface is write-only.
 * @{
 */

/**
 * @brief TM6605 pre-programmed effect IDs
 *
 * Valid effect IDs that can be written to the effect-select register.
 * Approximate durations (in milliseconds) are taken from the datasheet and
 * are reference values; actual playback time depends on the LRA.
 */
enum tm6605_effect {
	/** Sharp click, ~65 ms */
	TM6605_EFFECT_SHARP_CLICK = 1,
	/** Instant click, ~45 ms */
	TM6605_EFFECT_INSTANT_CLICK = 4,
	/** Light tap, ~130 ms */
	TM6605_EFFECT_LIGHT_TAP = 7,
	/** Double click, ~200 ms */
	TM6605_EFFECT_DOUBLE_CLICK = 10,
	/** Light pulse, ~215 ms */
	TM6605_EFFECT_LIGHT_PULSE = 13,
	/** Strong alert, ~190 ms */
	TM6605_EFFECT_STRONG_ALERT = 14,
	/** Medium-duration alert, ~730 ms */
	TM6605_EFFECT_MEDIUM_DURATION_ALERT = 15,
	/** Sharp click 2, ~90 ms */
	TM6605_EFFECT_SHARP_CLICK_2 = 17,
	/** Medium-strength click, ~65 ms */
	TM6605_EFFECT_MEDIUM_CLICK = 21,
	/** Flash strike, ~20 ms */
	TM6605_EFFECT_FLASH_STRIKE = 24,
	/** Short-gap double high-click, ~120 ms */
	TM6605_EFFECT_DOUBLE_HIGH_CLICK_SHORT = 27,
	/** Short-gap double medium-click, ~120 ms */
	TM6605_EFFECT_DOUBLE_MEDIUM_CLICK_SHORT = 31,
	/** Short-gap double flash-strike, ~100 ms */
	TM6605_EFFECT_DOUBLE_FLASH_STRIKE_SHORT = 34,
	/** Long-gap double instant-click, ~150 ms */
	TM6605_EFFECT_DOUBLE_INSTANT_CLICK_LONG = 37,
	/** Long-gap double medium-instant-click, ~150 ms */
	TM6605_EFFECT_DOUBLE_MEDIUM_INSTANT_CLICK_LONG = 41,
	/** Long-gap double flash-strike, ~150 ms */
	TM6605_EFFECT_DOUBLE_FLASH_STRIKE_LONG = 44,
	/** Alert, ~240 ms */
	TM6605_EFFECT_ALERT = 47,
	/** Toggle click, ~620 ms */
	TM6605_EFFECT_TOGGLE_CLICK = 58,
	/** Long slow fade-out transition 1, ~390 ms */
	TM6605_EFFECT_LONG_SLOW_FADE_1 = 70,
	/** Long slow fade-out transition 2, ~620 ms */
	TM6605_EFFECT_LONG_SLOW_FADE_2 = 71,
	/** Medium slow fade-out transition 1, ~400 ms */
	TM6605_EFFECT_MEDIUM_SLOW_FADE_1 = 72,
	/** Medium slow fade-out transition 2, ~650 ms */
	TM6605_EFFECT_MEDIUM_SLOW_FADE_2 = 73,
	/** Short slow fade-out transition 1, ~410 ms */
	TM6605_EFFECT_SHORT_SLOW_FADE_1 = 74,
	/** Short slow fade-out transition 2, ~490 ms */
	TM6605_EFFECT_SHORT_SLOW_FADE_2 = 75,
	/** Long fast fade-out transition 1, ~340 ms */
	TM6605_EFFECT_LONG_FAST_FADE_1 = 76,
	/** Long fast fade-out transition 2, ~390 ms */
	TM6605_EFFECT_LONG_FAST_FADE_2 = 77,
	/** Medium fast fade-out transition 1, ~310 ms */
	TM6605_EFFECT_MEDIUM_FAST_FADE_1 = 78,
	/** Medium fast fade-out transition 2, ~360 ms */
	TM6605_EFFECT_MEDIUM_FAST_FADE_2 = 79,
	/** Short fast fade-out transition 1, ~340 ms */
	TM6605_EFFECT_SHORT_FAST_FADE_1 = 80,
	/** Short fast fade-out transition 2, ~350 ms */
	TM6605_EFFECT_SHORT_FAST_FADE_2 = 81,
	/** Long slow boost transition 1, ~320 ms */
	TM6605_EFFECT_LONG_SLOW_BOOST_1 = 82,
	/** Long slow boost transition 2, ~650 ms */
	TM6605_EFFECT_LONG_SLOW_BOOST_2 = 83,
	/** Medium slow boost transition 1, ~310 ms */
	TM6605_EFFECT_MEDIUM_SLOW_BOOST_1 = 84,
	/** Medium slow boost transition 2, ~640 ms */
	TM6605_EFFECT_MEDIUM_SLOW_BOOST_2 = 85,
	/** Short slow boost transition 1, ~320 ms */
	TM6605_EFFECT_SHORT_SLOW_BOOST_1 = 86,
	/** Short slow boost transition 2, ~460 ms */
	TM6605_EFFECT_SHORT_SLOW_BOOST_2 = 87,
	/** Long fast boost transition 1, ~290 ms */
	TM6605_EFFECT_LONG_FAST_BOOST_1 = 88,
	/** Long fast boost transition 2, ~615 ms */
	TM6605_EFFECT_LONG_FAST_BOOST_2 = 89,
	/** Medium fast boost transition 1, ~320 ms */
	TM6605_EFFECT_MEDIUM_FAST_BOOST_1 = 90,
	/** Medium fast boost transition 2, ~590 ms */
	TM6605_EFFECT_MEDIUM_FAST_BOOST_2 = 91,
	/** Short fast boost transition 1, ~330 ms */
	TM6605_EFFECT_SHORT_FAST_BOOST_1 = 92,
	/** Short fast boost transition 2, ~470 ms */
	TM6605_EFFECT_SHORT_FAST_BOOST_2 = 93,
	/** Long alert, ~10 s */
	TM6605_EFFECT_LONG_ALERT = 118,
	/** Soft noise, ~480 ms */
	TM6605_EFFECT_SOFT_NOISE = 119,
	/**
	 * @brief Sleep command.
	 *
	 * Selecting and starting this "effect" puts the TM6605 into sleep mode.
	 * A falling edge on SCL (i.e. the next I2C transaction) wakes the device,
	 * though the first command after wake may be discarded by the chip.
	 */
	TM6605_EFFECT_SLEEP_COMMAND = 123,
};

/**
 * @brief Select the haptic effect to be played by the next start_output call.
 *
 * The TM6605 effect-select register is loaded but playback is not started.
 * Call haptics_start_output() afterwards to trigger the effect.
 *
 * @param dev TM6605 device instance.
 * @param effect Effect ID (see @ref tm6605_effect).
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p effect is not a valid effect ID.
 * @retval -errno from the underlying I2C controller on failure.
 */
int tm6605_select_effect(const struct device *dev, enum tm6605_effect effect);

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZEPHYR_INCLUDE_DRIVERS_HAPTICS_TM6605_H_ */
