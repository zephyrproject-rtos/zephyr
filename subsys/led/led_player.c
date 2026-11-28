/*
 * Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/led/led_player.h>
#include <zephyr/drivers/led.h>

#include "led_player_priv.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(led_player, CONFIG_LED_PLAYER_LOG_LEVEL);

static struct led_anim_player g_player;

static void led_player_stop_anim_no_lock(struct led_anim_player *player,
					 struct led_player_channel *channel);

static bool led_player_anim_has_finished(struct led_player_channel *channel)
{
	if (channel->anim->loop == LED_ANIM_INFINITE) {
		return false;
	}

	if (channel->current_frame < channel->anim->count) {
		return false;
	}

	if (channel->current_loop < (channel->anim->loop - 1)) {
		return false;
	}

	return true;
}

static bool led_player_anim_has_completed(struct led_player_channel *channel)
{
	/*
	 * led_player_anim_has_finished() is already true while the last frame is
	 * held: the animation only completes once led_player_play_frame() has
	 * been called after its delay, which sets the deadline to
	 * LED_PLAYER_NO_DEADLINE.
	 */
	return channel->deadline == LED_PLAYER_NO_DEADLINE;
}

struct led_player_channel *led_player_get_next_anim(struct led_anim_player *player)
{
	int64_t deadline = LED_PLAYER_NO_DEADLINE;
	struct led_player_channel *channel, *next_channel = NULL;

	if (!player || sys_slist_is_empty(&player->list)) {
		return NULL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&player->list, channel, node) {
		if (channel->deadline < deadline) {
			deadline = channel->deadline;
			next_channel = channel;
		}
	}

	return next_channel;
}

int led_player_play_frame(struct led_player_channel *channel)
{
	const struct led_anim_frame *frame;
	int64_t now;
	int ret;

	if (!channel) {
		return -EINVAL;
	}

	if (led_player_anim_has_finished(channel)) {
		channel->deadline = LED_PLAYER_NO_DEADLINE;
		k_work_submit(&g_player.work_complete);
		return 0;
	}

	if (channel->current_frame >= channel->anim->count) {
		channel->current_loop += 1;
		channel->current_frame = 0;
	}

	frame = &channel->anim->frames[channel->current_frame++];
	if (frame->state) {
		ret = led_on(channel->dev, channel->led);
	} else {
		ret = led_off(channel->dev, channel->led);
	}

	/*
	 * Counting from the previous deadline keeps the workqueue latency from
	 * accumulating. When a whole frame late, restart from now rather than
	 * playing the missed frames in a burst.
	 */
	now = k_uptime_get();
	if (channel->deadline + frame->delay_ms <= now) {
		channel->deadline = now + frame->delay_ms;
	} else {
		channel->deadline += frame->delay_ms;
	}

	return ret;
}

static void led_player_play_overdue_frames(struct led_anim_player *player)
{
	struct led_player_channel *channel, *channel_safe;
	int64_t now = k_uptime_get();
	int ret;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&player->list, channel, channel_safe, node) {
		if (channel->deadline <= now) {
			ret = led_player_play_frame(channel);
			if (ret < 0) {
				LOG_ERR("Failed to play a frame on %s:%d, stopping channel",
					channel->dev->name, channel->led);
				sys_slist_find_and_remove(&player->list, &channel->node);
			}
		}
	}
}

static k_timeout_t led_player_timeout(int64_t deadline)
{
#ifdef CONFIG_TIMEOUT_64BIT
	/* A relative timeout is rounded up by one tick on every frame */
	return K_TIMEOUT_ABS_MS(deadline);
#else
	return K_MSEC(MAX(deadline - k_uptime_get(), 0));
#endif
}

int led_player_schedule_next(struct led_anim_player *player)
{
	struct led_player_channel *channel;
	int ret = 0;

	if (!player) {
		return -EINVAL;
	}

	channel = led_player_get_next_anim(player);
	if (!channel) {
		return 0;
	}

	ret = k_work_reschedule(&player->dwork, led_player_timeout(channel->deadline));
	if (ret < 0) {
		LOG_ERR("Failed to schedule the next frame");
		return ret;
	}

	return 0;
}

static int led_player_check_anim(const struct led_anim *anim)
{
	if (anim == NULL) {
		return -EINVAL;
	}

	if (!anim->count || !anim->loop || !anim->frames) {
		return -EINVAL;
	}

	for (uint16_t i = 0; i < anim->count; i++) {
		if (anim->frames[i].delay_ms == 0) {
			return -EINVAL;
		}
	}

	return 0;
}

static int led_player_set_default_state(struct led_player_channel *channel)
{
	int ret;

	if (channel->default_state) {
		ret = led_on(channel->dev, channel->led);
	} else {
		ret = led_off(channel->dev, channel->led);
	}

	if (ret < 0) {
		LOG_ERR("Failed to set the default state of %s:%d", channel->dev->name,
			channel->led);
	}

	return ret;
}

int led_player_channel_init(struct led_player_channel *channel, const struct device *dev,
			    uint32_t led, bool default_state)
{
	if (channel == NULL || dev == NULL) {
		return -EINVAL;
	}

	channel->dev = dev;
	channel->led = led;
	channel->default_state = default_state;

	return led_player_set_default_state(channel);
}

static void led_player_work_cb(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct led_anim_player *player = CONTAINER_OF(dwork, struct led_anim_player, dwork);

	k_mutex_lock(&player->mutex, K_FOREVER);
	led_player_play_overdue_frames(player);
	led_player_schedule_next(player);
	k_mutex_unlock(&player->mutex);
}

static void led_player_complete_work_cb(struct k_work *work)
{
	struct led_anim_player *player = CONTAINER_OF(work, struct led_anim_player, work_complete);
	struct led_player_channel *channel;
	led_anim_complete_cb cb;
	void *user_data;
	bool found;

	/*
	 * The callback may play or stop channels, so it is called without the
	 * mutex held, and the list must be scanned again from its head after it.
	 */
	do {
		found = false;
		cb = NULL;
		user_data = NULL;

		k_mutex_lock(&player->mutex, K_FOREVER);
		SYS_SLIST_FOR_EACH_CONTAINER(&player->list, channel, node) {
			if (led_player_anim_has_completed(channel)) {
				sys_slist_find_and_remove(&player->list, &channel->node);
				(void)led_player_set_default_state(channel);
				cb = channel->cb;
				user_data = channel->user_data;
				found = true;
				break;
			}
		}
		k_mutex_unlock(&player->mutex);

		if (cb) {
			cb(channel, user_data);
		}
	} while (found);
}

int led_player_play_anim_with_cb(struct led_player_channel *channel, const struct led_anim *anim,
				 led_anim_complete_cb cb, void *user_data)
{
	int ret;

	if (!channel) {
		return -EINVAL;
	}

	ret = led_player_check_anim(anim);
	if (ret != 0) {
		LOG_ERR("The animation is not valid");
		return ret;
	}

	k_mutex_lock(&g_player.mutex, K_FOREVER);
	led_player_stop_anim_no_lock(&g_player, channel);
	channel->current_frame = 0;
	channel->anim = anim;
	channel->current_loop = 0;
	channel->cb = cb;
	channel->user_data = user_data;

	/* Make sure anim start right now */
	channel->deadline = k_uptime_get();
	sys_slist_append(&g_player.list, &channel->node);
	led_player_play_overdue_frames(&g_player);
	led_player_schedule_next(&g_player);
	k_mutex_unlock(&g_player.mutex);

	return ret;
}

int led_player_play_anim(struct led_player_channel *channel, const struct led_anim *anim)
{
	return led_player_play_anim_with_cb(channel, anim, NULL, NULL);
}

static void led_player_stop_anim_no_lock(struct led_anim_player *player,
					 struct led_player_channel *channel)
{
	/*
	 * The work only plays channels in the list, with the mutex held: once
	 * removed, this channel is not played anymore.
	 */
	sys_slist_find_and_remove(&player->list, &channel->node);
	led_player_schedule_next(player);
}

int led_player_stop_anim(struct led_player_channel *channel)
{
	int ret;

	if (!channel) {
		return -EINVAL;
	}

	/* Under the mutex, so that a concurrent play can't be overwritten */
	k_mutex_lock(&g_player.mutex, K_FOREVER);
	led_player_stop_anim_no_lock(&g_player, channel);
	ret = led_player_set_default_state(channel);
	k_mutex_unlock(&g_player.mutex);

	return ret;
}

bool led_player_is_playing(struct led_player_channel *channel)
{
	sys_snode_t *prev;
	bool playing;

	if (!channel) {
		return false;
	}

	k_mutex_lock(&g_player.mutex, K_FOREVER);
	playing = sys_slist_find(&g_player.list, &channel->node, &prev) &&
		  !led_player_anim_has_completed(channel);
	k_mutex_unlock(&g_player.mutex);

	return playing;
}

int led_player_do_init(struct led_anim_player *player)
{
	k_work_init_delayable(&player->dwork, led_player_work_cb);
	k_work_init(&player->work_complete, led_player_complete_work_cb);
	sys_slist_init(&player->list);
	k_mutex_init(&player->mutex);

	return 0;
}

static int led_player_init(void)
{
	return led_player_do_init(&g_player);
}

SYS_INIT(led_player_init, POST_KERNEL, CONFIG_LED_PLAYER_INIT_PRIORITY);
