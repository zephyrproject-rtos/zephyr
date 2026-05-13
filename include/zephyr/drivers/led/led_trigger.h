/*
 * Copyright (c) 2026 Siemens
 * Copyright (c) 2026 Stefan Gloor
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file led_trigger.h
 *
 * @brief LED trigger core
 *
 * Common layer for LED triggers that manages the common
 * memory pool and defines the API trigger implementations
 * have to implement.
 *
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_LED_LED_TRIGGER_H_
#define ZEPHYR_INCLUDE_DRIVERS_LED_LED_TRIGGER_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/types.h>

#ifdef CONFIG_LED_TRIGGER_TIMER
#include <zephyr/drivers/led/led_trigger_timer.h>
#endif

struct led_trigger_channel;


/**
 * @brief LED trigger type definition.
 *
 * Each trigger type (e.g., timer, network activity) provides an instance of
 * this structure with its own activate/deactivate callbacks.
 * Inspired by struct led_trigger in Linux.
 */
struct led_trigger {
	/** Human-readable trigger name (e.g., "timer", "diskactivity"). */
	const char *name;

	/**
	 * @brief Called when the trigger is attached to a channel.
	 *
	 * The trigger should initialize its per-channel state in place at
	 * @c &ch->data.member and prepare to drive the LED.
	 *
	 * Called with @c ch->lock held and @c ch->transition_lock serializing
	 * the install. The implementation must not sleep.
	 *
	 * @param ch Channel being activated.
	 * @return 0 on success, negative errno on failure.
	 */
	int (*activate)(struct led_trigger_channel *ch);

	/**
	 * @brief Called when the trigger is detached from a channel.
	 *
	 * The trigger must stop all asynchronous work that references
	 * @c &ch->data and clean up before returning.
	 *
	 * Called with @c ch->transition_lock held but @c ch->lock released,
	 * so the implementation may sleep (e.g., to call
	 * @ref k_work_cancel_delayable_sync). The transition lock prevents
	 * any concurrent activator from installing a new trigger on
	 * @p ch and therefore from touching @c ch->data until this
	 * callback returns.
	 *
	 * @param ch Channel being deactivated.
	 */
	void (*deactivate)(struct led_trigger_channel *ch);

	/**
	 * @brief Update brightness while the trigger is active.
	 *
	 * Let the trigger implementation know what the "ON" brightness is.
	 * This should not override the momentary state of the LED.
	 *
	 * Called with @c ch->lock held. The implementation must not sleep.
	 *
	 * @param ch    Active channel.
	 * @param value New brightness (0...LED_BRIGHTNESS_MAX).
	 * @return 0 on success, negative errno on failure.
	 */
	int (*update_brightness)(struct led_trigger_channel *ch,
				 uint8_t value);
};

/**
 * @brief Storage union large enough to hold any enabled trigger's
 *        per-channel state.
 *
 * Embedded directly into @c led_trigger_channel: each channel can have
 * at most one trigger attached at a time, so a single union slot per
 * channel is sufficient regardless of how many trigger types are compiled
 * in. Adding a new trigger only grows the slot size if its data struct is
 * larger than the current maximum; the slot count stays fixed.
 *
 * All members share the same starting address, so a trigger implementation
 * may access its own state as @c &ch->data.member.
 */
union led_trigger_data {
#ifdef CONFIG_LED_TRIGGER_TIMER
	struct led_trigger_timer_data timer; /**< timer trigger data */
#endif
};

/**
 * @brief Per-channel trigger state managed by the core.
 *
 * Two locks cover this structure:
 *
 * - @c transition_lock serializes trigger activate/deactivate so the two
 *   never overlap, and so a concurrent activator cannot interleave with
 *   a pending deactivate. May-sleep contexts only.
 * - @c lock protects short critical sections that read or mutate
 *   @c trigger, @c active, and the contents of @c data while a trigger
 *   is running (e.g. brightness updates, work-handler state snapshots).
 *
 * Lock ordering when both are needed: @c transition_lock first, then
 * @c lock. Readers in atomic contexts (e.g. the work handler) take only
 * @c lock.
 */
struct led_trigger_channel {
	const struct device *dev; /**< LED device */
	uint32_t led_idx; /**< LED channel index of device */
	const struct led_trigger *trigger; /**< Trigger implementation */
	union led_trigger_data data; /**< Trigger implementation data */
	struct k_mutex transition_lock; /**< Locked during activate/deactivate */
	struct k_spinlock lock; /**< Locked during read/write of any member */
	bool active; /**< True if channel is in use */
};

/**
 * @brief Find an existing trigger channel for a (device, led) pair.
 *
 * @param dev LED device.
 * @param led LED index on the device.
 * @return Pointer to the channel, or NULL if none is allocated.
 */
struct led_trigger_channel *led_trigger_find_channel(const struct device *dev,
						     uint32_t led);

/**
 * @brief Get or allocate a trigger channel for a (device, led) pair.
 *
 * @param dev LED device.
 * @param led LED index on the device.
 * @return Pointer to the channel, or NULL if the pool is exhausted.
 */
struct led_trigger_channel *led_trigger_get_channel(const struct device *dev,
						    uint32_t led);

/**
 * @brief Cancel any active trigger on a (device, led) channel.
 *
 * Calls the trigger's deactivate callback and clears the binding.
 * May sleep.
 *
 * @param dev LED device.
 * @param led LED index on the device.
 */
void led_trigger_cancel(const struct device *dev, uint32_t led);

/**
 * @brief Notify the active trigger of a brightness change.
 *
 * Delegates to the trigger's update_brightness callback if available.
 *
 * @param dev   LED device.
 * @param led   LED index on the device.
 * @param value New brightness (0...LED_BRIGHTNESS_MAX).
 * @return 0 on success, negative errno on failure.
 */
int led_trigger_update_brightness(const struct device *dev, uint32_t led,
				   uint8_t value);

/**
 * @brief Set LED brightness via the driver (helper for triggers).
 *
 * Calls the driver's set_brightness callback, falling back to on/off
 * if the driver does not support brightness control.
 *
 * @param dev   LED device.
 * @param led   LED index on the device.
 * @param value Brightness (0...LED_BRIGHTNESS_MAX).
 * @return 0 on success, negative errno on failure.
 */
int led_trigger_set_brightness(const struct device *dev, uint32_t led,
			       uint8_t value);

#endif /* ZEPHYR_INCLUDE_DRIVERS_LED_LED_TRIGGER_H_ */
