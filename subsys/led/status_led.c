/*
 * Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/led/status_led.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/iterable_sections.h>

LOG_MODULE_DECLARE(led_player, CONFIG_LED_PLAYER_LOG_LEVEL);

static K_MUTEX_DEFINE(status_led_mutex);

static void status_led_anim_completed(struct led_player_channel *channel, void *user_data);

static int status_led_set_active(struct status_led *led, int id, bool set)
{
	for (int i = 0; i < led->count; i++) {
		if (led->anims[i].id == id) {
			WRITE_BIT(led->actives, i, set);
			return 0;
		}
	}

	return -ENOENT;
}

static int status_led_update_current(struct status_led *led, bool *updated)
{
	int current = led->current;
	int current_prio = -1;

	led->current = -1;
	for (int i = 0; i < led->count; i++) {
		if (IS_BIT_SET(led->actives, i)) {
			if (current_prio >= led->anims[i].prio) {
				continue;
			}
			led->current = i;
			current_prio = led->anims[i].prio;
		}
	}

	*updated = current != led->current;
	if (current_prio == -1) {
		return -ENOENT;
	}

	return 0;
}

static int status_led_play(struct status_led *led)
{
	int current = led->current;

	return led_player_play_anim_with_cb(&led->channel, &led->anims[current].anim,
					    status_led_anim_completed, (void *)(uintptr_t)current);
}

static int status_led_update(struct status_led *led)
{
	bool updated;
	int ret;

	ret = status_led_update_current(led, &updated);
	if (ret == -ENOENT) {
		/* No active status */
		return led_player_stop_anim(&led->channel);
	}

	if (updated) {
		return status_led_play(led);
	}

	return 0;
}

static void status_led_anim_completed(struct led_player_channel *channel, void *user_data)
{
	struct status_led *led = CONTAINER_OF(channel, struct status_led, channel);
	int i = (int)(uintptr_t)user_data;

	k_mutex_lock(&status_led_mutex, K_FOREVER);

	if (led->current == i && led_player_is_playing(channel)) {
		k_mutex_unlock(&status_led_mutex);
		return;
	}

	WRITE_BIT(led->actives, i, 0);
	status_led_update(led);

	k_mutex_unlock(&status_led_mutex);
}

int status_led_set(struct status_led *led, int id)
{
	int ret;

	if (!led) {
		return -EINVAL;
	}

	k_mutex_lock(&status_led_mutex, K_FOREVER);
	ret = status_led_set_active(led, id, true);
	if (!ret) {
		ret = status_led_update(led);
	}
	k_mutex_unlock(&status_led_mutex);

	return ret;
}

int status_led_clear(struct status_led *led, int id)
{
	int ret;

	if (!led) {
		return -EINVAL;
	}

	k_mutex_lock(&status_led_mutex, K_FOREVER);
	ret = status_led_set_active(led, id, false);
	if (!ret) {
		ret = status_led_update(led);
	}
	k_mutex_unlock(&status_led_mutex);

	return ret;
}

static int status_led_init(void)
{
	int ret = 0;

	STRUCT_SECTION_FOREACH(status_led, led) {
		if (!device_is_ready(led->channel.dev)) {
			LOG_ERR("%s is not ready", led->channel.dev->name);
			ret = -ENODEV;
			continue;
		}

		k_mutex_lock(&status_led_mutex, K_FOREVER);
		/* Leave the LED alone if a status was set by an earlier init function */
		if (led->current == -1) {
			(void)led_player_stop_anim(&led->channel);
		}
		k_mutex_unlock(&status_led_mutex);
	}

	return ret;
}

/* APPLICATION runs after all the POST_KERNEL devices, so the LED drivers are ready */
SYS_INIT(status_led_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
