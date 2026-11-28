/*
 * Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for the status LED subsystem
 */

#ifndef ZEPHYR_INCLUDE_LED_STATUS_LED_H_
#define ZEPHYR_INCLUDE_LED_STATUS_LED_H_

#include <zephyr/led/led_player.h>
#include <zephyr/sys/iterable_sections.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Status LED
 * @defgroup status_led_interface Status LED
 * @ingroup led_interface
 *
 * Reports statuses on a single LED, with one animation per status. Several
 * statuses can be active at once: the LED plays the animation of the active
 * status with the highest priority. When it is cleared, the animation of the
 * next active status, if any, is played.
 *
 * A status whose animation is finite (see @ref led_anim.loop) clears itself
 * once its animation has completed. A status whose animation loops forever
 * stays active until it is cleared with status_led_clear().
 *
 * When no status is active, the animation is stopped and the LED is set to
 * the default state given to STATUS_LED_DEFINE(). The default state is also
 * applied at boot.
 *
 * All functions of this API may block and must be called from thread
 * context, never from an ISR.
 * @{
 */

/**
 * @brief A status and the animation reporting it.
 *
 * Usually defined with STATUS_LED_ANIM() or STATUS_LED_BLINK_ANIM(), in a
 * @c const array.
 */
struct status_led_anim {
	/** Status identifier, unique within a status LED. */
	int id;
	/**
	 * Priority of the status. The highest value wins; between active
	 * statuses of equal priority, the one defined first in the array of
	 * statuses wins.
	 */
	uint8_t prio;
	/** Animation played on the LED while this status is reported. */
	struct led_anim anim;
};

/**
 * @brief A status LED.
 *
 * Must be defined with STATUS_LED_DEFINE(). Its fields are managed by the
 * subsystem and must not be modified directly.
 */
struct status_led {
	/** @cond INTERNAL_HIDDEN */
	/* LED player channel driving the LED. */
	struct led_player_channel channel;
	/* Statuses this LED can report. */
	const struct status_led_anim *anims;
	/* Bit i set: status anims[i] is active */
	uint32_t actives;
	/* Index in anims of the status being reported, -1 if none. */
	int current;
	/* Number of entries in anims. */
	const int count;
	/** @endcond */
};

/**
 * @brief Initializer for a @ref status_led_anim made of alternating on/off
 * frames.
 *
 * The first frame turns the LED on, the second off, and so on.
 *
 * @param _id Status identifier, see @ref status_led_anim.id.
 * @param _prio Priority of the status, see @ref status_led_anim.prio.
 * @param _loop Number of times the sequence is played, see @ref led_anim.loop.
 * @param ... Delay in milliseconds of each frame, see LED_ANIM_FRAMES().
 */
#define STATUS_LED_ANIM(_id, _prio, _loop, ...)                                                    \
	{                                                                                          \
		.id = _id,                                                                         \
		.prio = _prio,                                                                     \
		.anim =                                                                            \
			{                                                                          \
				.frames = LED_ANIM_FRAME_ARRAY(__VA_ARGS__),                       \
				.count = ARRAY_SIZE(LED_ANIM_FRAME_ARRAY(__VA_ARGS__)),            \
				.loop = _loop,                                                     \
			},                                                                         \
	}

/**
 * @brief Initializer for a @ref status_led_anim blinking forever.
 *
 * @param _id Status identifier, see @ref status_led_anim.id.
 * @param _prio Priority of the status, see @ref status_led_anim.prio.
 * @param _on_ms Time in milliseconds the LED is on.
 * @param _off_ms Time in milliseconds the LED is off.
 */
#define STATUS_LED_BLINK_ANIM(_id, _prio, _on_ms, _off_ms)                                         \
	STATUS_LED_ANIM(_id, _prio, LED_ANIM_INFINITE, _on_ms, _off_ms)

/**
 * @brief Define a status LED.
 *
 * For example:
 * @code{.c}
 * static const struct status_led_anim anims[] = {
 *	STATUS_LED_BLINK_ANIM(STATUS_NO_NETWORK, 1, 500, 500),
 *	STATUS_LED_ANIM(STATUS_ERROR, 2, 3, 100, 100),
 * };
 * STATUS_LED_DEFINE(status_led, DEVICE_DT_GET(DT_NODELABEL(led)), 0, false, anims);
 * @endcode
 *
 * The LED is set to @p _default_state at boot, at the APPLICATION init level,
 * and whenever no status is active.
 *
 * @param _name Name of the @ref status_led variable to define, with static
 * storage.
 * @param _dev LED controller device driving @p _led. Must be a constant
 * expression, such as DEVICE_DT_GET().
 * @param _led LED index on @p _dev.
 * @param _default_state State of the LED while no status is active, @c true
 * for on.
 * @param _anims Array of @ref status_led_anim, up to 32 entries.
 */
#define STATUS_LED_DEFINE(_name, _dev, _led, _default_state, _anims)                               \
	BUILD_ASSERT(ARRAY_SIZE(_anims) <= 32, "a status LED supports up to 32 statuses");         \
	static STRUCT_SECTION_ITERABLE(status_led, _name) = {                                      \
		.channel = LED_PLAYER_CHANNEL_INITIALIZER(_dev, _led, _default_state),             \
		.anims = _anims,                                                                   \
		.count = ARRAY_SIZE(_anims),                                                       \
		.current = -1,                                                                     \
	}

/**
 * @brief Set a status as active.
 *
 * Its animation is played if no active status has a higher priority.
 * Setting a status that is already active has no effect: its animation is
 * not restarted.
 *
 * @param status_led Status LED.
 * @param id Identifier of the status.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p status_led is @c NULL.
 * @retval -ENOENT @p id is not a status of @p status_led.
 * @retval <0 Error returned by the LED player.
 */
int status_led_set(struct status_led *status_led, int id);

/**
 * @brief Clear a status.
 *
 * If its animation is playing, it is replaced by the animation of the next
 * active status. If no status is active anymore, the animation is stopped and
 * the LED is set to its default state. Clearing a status that is not active
 * has no effect.
 *
 * @param status_led Status LED.
 * @param id Identifier of the status.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p status_led is @c NULL.
 * @retval -ENOENT @p id is not a status of @p status_led.
 * @retval <0 Error returned by the LED player.
 */
int status_led_clear(struct status_led *status_led, int id);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_LED_STATUS_LED_H_ */
