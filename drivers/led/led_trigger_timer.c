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
#include <zephyr/drivers/led/led_trigger_timer.h>

LOG_MODULE_DECLARE(led_trigger, CONFIG_LED_LOG_LEVEL);

/* Recover the owning channel from the embedded timer-data pointer. */
static inline struct led_trigger_channel *
timer_data_to_channel(struct led_trigger_timer_data *td)
{
	union led_trigger_data *data =
		CONTAINER_OF(td, union led_trigger_data, timer);

	return CONTAINER_OF(data, struct led_trigger_channel, data);
}

static void blink_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct led_trigger_timer_data *td =
		CONTAINER_OF(dwork, struct led_trigger_timer_data, work);
	struct led_trigger_channel *ch = timer_data_to_channel(td);
	k_spinlock_key_t key;
	uint32_t next_delay;
	uint8_t brightness;
	bool on_phase;

	key = k_spin_lock(&ch->lock);
	if (!ch->active) {
		k_spin_unlock(&ch->lock, key);
		return;
	}

	td->on_phase = !td->on_phase;
	on_phase = td->on_phase;
	brightness = td->brightness;
	next_delay = on_phase ? td->delay_on : td->delay_off;
	k_spin_unlock(&ch->lock, key);

	led_trigger_set_brightness(ch->dev, ch->led_idx,
				   on_phase ? brightness : 0);

	key = k_spin_lock(&ch->lock);
	if (ch->active) {
		k_work_schedule(&td->work, K_MSEC(next_delay));
	}
	k_spin_unlock(&ch->lock, key);
}

static int timer_activate(struct led_trigger_channel *ch)
{
	struct led_trigger_timer_data *td = &ch->data.timer;

	/* We start "ON" with full brightness. */
	td->on_phase = true;
	td->brightness = LED_BRIGHTNESS_MAX;
	k_work_init_delayable(&td->work, blink_work_handler);

	return 0;
}

static void timer_deactivate(struct led_trigger_channel *ch)
{
	struct led_trigger_timer_data *td = &ch->data.timer;
	struct k_work_sync sync;

	/*
	 * Safe to wait: the core holds the transition lock across this
	 * callback, so no concurrent activator can install a new trigger
	 * and trample ch->data while we drain the work item.
	 */
	k_work_cancel_delayable_sync(&td->work, &sync);
}

/* Called with ch->lock held; must not sleep. */
static int timer_update_brightness(struct led_trigger_channel *ch,
				    uint8_t value)
{
	ch->data.timer.brightness = value;
	return 0;
}

static const struct led_trigger led_trigger_timer = {
	.name = "timer",
	.activate = timer_activate,
	.deactivate = timer_deactivate,
	.update_brightness = timer_update_brightness,
};

int led_trigger_timer_start(const struct device *dev, uint32_t led,
			    uint32_t delay_on, uint32_t delay_off)
{
	struct led_trigger_channel *ch;
	struct led_trigger_timer_data *td;
	k_spinlock_key_t key;
	uint8_t brightness;
	int ret;

	/*
	 * A zero in either delay cancels any active blink.
	 * A zero @p delay_on describes an "always off" duty cycle
	 * and turns the LED off; this case takes precedence when both
	 * delays are zero. A zero @p delay_off with non-zero @p delay_on
	 * describes an "always on" duty cycle and leaves the LED lit (at
	 * the trigger's current ON-phase brightness, or LED_BRIGHTNESS_MAX
	 * if no trigger was running).
	 */
	if (delay_on == 0U || delay_off == 0U) {
		uint8_t stay_on_brightness = LED_BRIGHTNESS_MAX;

		ch = led_trigger_find_channel(dev, led);
		if (ch != NULL) {
			key = k_spin_lock(&ch->lock);
			if (ch->active && ch->trigger == &led_trigger_timer) {
				stay_on_brightness = ch->data.timer.brightness;
			}
			k_spin_unlock(&ch->lock, key);
		}

		led_trigger_cancel(dev, led);

		if (delay_on == 0U) {
			return led_trigger_set_brightness(dev, led, 0);
		}
		return led_trigger_set_brightness(dev, led, stay_on_brightness);
	}

	ch = led_trigger_get_channel(dev, led);
	if (ch == NULL) {
		LOG_WRN("LED trigger pool exhausted "
			"(CONFIG_LED_TRIGGER_MAX_CHANNELS=%u)",
			CONFIG_LED_TRIGGER_MAX_CHANNELS);
		return -ENOMEM;
	}

	/*
	 * Hold the transition lock across the whole install. This serialises
	 * us against any concurrent activator or canceller and, combined with
	 * the recursive cancel below, lets us treat the in-place fast path
	 * and the fresh-install path as one atomic operation from the
	 * caller's point of view.
	 */
	k_mutex_lock(&ch->transition_lock, K_FOREVER);

	/* Fast path: timer trigger already attached -> update in place. */
	key = k_spin_lock(&ch->lock);
	if (ch->active && ch->trigger == &led_trigger_timer) {
		td = &ch->data.timer;
		td->delay_on = delay_on;
		td->delay_off = delay_off;

		k_timeout_t next = K_MSEC(td->on_phase ?
					  td->delay_on : td->delay_off);
		k_spin_unlock(&ch->lock, key);
		k_work_reschedule(&td->work, next);
		k_mutex_unlock(&ch->transition_lock);
		return 0;
	}
	k_spin_unlock(&ch->lock, key);

	/*
	 * A different (or no) trigger is attached. led_trigger_cancel takes
	 * the transition lock recursively, drains the current trigger, then
	 * leaves the channel with trigger=NULL and active=false.
	 */
	led_trigger_cancel(dev, led);

	key = k_spin_lock(&ch->lock);
	ch->trigger = &led_trigger_timer;
	ret = led_trigger_timer.activate(ch);
	if (ret != 0) {
		ch->trigger = NULL;
		k_spin_unlock(&ch->lock, key);
		k_mutex_unlock(&ch->transition_lock);
		return ret;
	}

	td = &ch->data.timer;
	td->delay_on = delay_on;
	td->delay_off = delay_off;
	ch->active = true;
	brightness = td->brightness;
	k_spin_unlock(&ch->lock, key);

	led_trigger_set_brightness(dev, led, brightness);
	k_work_schedule(&td->work, K_MSEC(delay_on));

	k_mutex_unlock(&ch->transition_lock);
	return 0;
}
