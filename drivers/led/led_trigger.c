/*
 * Copyright (c) 2026 Siemens
 * Copyright (c) 2026 Stefan Gloor
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/led/led_trigger.h>

LOG_MODULE_REGISTER(led_trigger, CONFIG_LED_LOG_LEVEL);

static struct led_trigger_channel pool[CONFIG_LED_TRIGGER_MAX_CHANNELS];
static struct k_spinlock pool_lock;
static uint32_t pool_count;

int led_trigger_set_brightness(const struct device *dev, uint32_t led,
			       uint8_t value)
{
	const struct led_driver_api *api = DEVICE_API_GET(led, dev);

	if (api->set_brightness != NULL) {
		return api->set_brightness(dev, led, value);
	}
	if (value > 0 && api->on != NULL) {
		return api->on(dev, led);
	}
	if (value == 0U && api->off != NULL) {
		return api->off(dev, led);
	}
	return -ENOSYS;
}

struct led_trigger_channel *led_trigger_find_channel(const struct device *dev,
						     uint32_t led)
{
	k_spinlock_key_t key = k_spin_lock(&pool_lock);

	for (uint32_t i = 0; i < pool_count; i++) {
		if (pool[i].dev == dev && pool[i].led_idx == led) {
			k_spin_unlock(&pool_lock, key);
			return &pool[i];
		}
	}

	k_spin_unlock(&pool_lock, key);
	return NULL;
}

struct led_trigger_channel *led_trigger_get_channel(const struct device *dev,
						    uint32_t led)
{
	k_spinlock_key_t key;
	struct led_trigger_channel *ch;

	key = k_spin_lock(&pool_lock);

	for (uint32_t i = 0; i < pool_count; i++) {
		if (pool[i].dev == dev && pool[i].led_idx == led) {
			k_spin_unlock(&pool_lock, key);
			return &pool[i];
		}
	}

	if (pool_count >= CONFIG_LED_TRIGGER_MAX_CHANNELS) {
		k_spin_unlock(&pool_lock, key);
		return NULL;
	}

	ch = &pool[pool_count++];
	ch->dev = dev;
	ch->led_idx = led;
	k_mutex_init(&ch->transition_lock);

	k_spin_unlock(&pool_lock, key);
	return ch;
}

void led_trigger_cancel(const struct device *dev, uint32_t led)
{
	struct led_trigger_channel *ch;
	const struct led_trigger *trigger;
	k_spinlock_key_t key;

	ch = led_trigger_find_channel(dev, led);
	if (ch == NULL) {
		return;
	}

	/*
	 * Hold the transition lock across the entire detach so a concurrent
	 * activator cannot install a new trigger on this channel and thus
	 * cannot touch ch->data until our deactivate has fully finished.
	 * That removes any aliasing between an outgoing trigger's pending
	 * work and an incoming trigger's freshly-initialised state.
	 */
	k_mutex_lock(&ch->transition_lock, K_FOREVER);

	key = k_spin_lock(&ch->lock);
	if (!ch->active) {
		k_spin_unlock(&ch->lock, key);
		k_mutex_unlock(&ch->transition_lock);
		return;
	}
	trigger = ch->trigger;
	ch->active = false;
	k_spin_unlock(&ch->lock, key);

	/*
	 * deactivate() may sleep; the transition lock keeps activators out
	 * and the work-handler short path sees active=false and bails.
	 */
	if (trigger != NULL && trigger->deactivate != NULL) {
		trigger->deactivate(ch);
	}

	key = k_spin_lock(&ch->lock);
	ch->trigger = NULL;
	k_spin_unlock(&ch->lock, key);

	k_mutex_unlock(&ch->transition_lock);
}

int led_trigger_update_brightness(const struct device *dev, uint32_t led,
				   uint8_t value)
{
	struct led_trigger_channel *ch;
	k_spinlock_key_t key;
	int ret;

	ch = led_trigger_find_channel(dev, led);
	if (ch == NULL) {
		return -ENODEV;
	}

	/*
	 * The update_brightness callback is invoked with ch->lock held so
	 * that a concurrent cancel cannot detach the trigger between our
	 * check and the call. Trigger implementations must not sleep here.
	 */
	key = k_spin_lock(&ch->lock);
	if (!ch->active || ch->trigger == NULL ||
	    ch->trigger->update_brightness == NULL) {
		k_spin_unlock(&ch->lock, key);
		return -EINVAL;
	}
	ret = ch->trigger->update_brightness(ch, value);
	k_spin_unlock(&ch->lock, key);

	return ret;
}
