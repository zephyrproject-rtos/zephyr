/*
 * Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Internal API of the LED player subsystem
 *
 * Not part of the public API: these definitions are only exposed outside of
 * led_player.c so that the scheduling logic can be unit tested against a
 * private player instance. Applications must use
 * @ref zephyr/led/led_player.h instead.
 */

#ifndef ZEPHYR_SUBSYS_LED_LED_PLAYER_PRIV_H_
#define ZEPHYR_SUBSYS_LED_LED_PLAYER_PRIV_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>
#include <zephyr/led/led_player.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Deadline of a channel whose animation has completed. */
#define LED_PLAYER_NO_DEADLINE INT64_MAX

/**
 * @brief Scheduler state shared by all the channels of a player.
 *
 * The subsystem owns a single instance, initialized at boot, which backs
 * led_player_play_anim(), led_player_play_anim_with_cb() and
 * led_player_stop_anim(). Tests may instantiate their own and drive it with
 * the functions below.
 */
struct led_anim_player {
	/**
	 * Work item playing the overdue frames, armed for the closest deadline
	 * of the channels in @ref led_anim_player.list.
	 */
	struct k_work_delayable dwork;
	/** List of @ref led_player_channel currently tracked by the player. */
	sys_slist_t list;
	/** Protects all fields of this struct and of the tracked channels. */
	struct k_mutex mutex;
	/**
	 * Work item removing completed animations from
	 * @ref led_anim_player.list and invoking their completion callback.
	 */
	struct k_work work_complete;
};

/**
 * @brief Initialize a LED animation player.
 *
 * @param player Player to initialize.
 *
 * @retval 0 always.
 */
int led_player_do_init(struct led_anim_player *player);

/**
 * @brief Find the playing channel with the closest deadline.
 *
 * Animations that have completed have a @ref LED_PLAYER_NO_DEADLINE deadline
 * and are never returned.
 *
 * @param player Player to search.
 *
 * @return The channel with the closest deadline, or @c NULL if @p player
 * is @c NULL or no tracked channel has a pending deadline.
 */
struct led_player_channel *led_player_get_next_anim(struct led_anim_player *player);

/**
 * @brief (Re)arm the player's work item for the next due animation frame.
 *
 * Schedules @ref led_anim_player.dwork for the closest deadline of the
 * channels of @p player, if any. Frames are not played: a deadline that has
 * already expired makes the work run right away.
 *
 * Must be called with @ref led_anim_player.mutex held.
 *
 * @param player Player to schedule.
 *
 * @retval 0 on success, including when no animation is running.
 * @retval -EINVAL if @p player is @c NULL.
 * @retval <0 error returned by k_work_reschedule().
 */
int led_player_schedule_next(struct led_anim_player *player);

/**
 * @brief Play the current frame of an animation and advance it.
 *
 * Turns the LED on or off according to the current frame, moves to the next
 * frame (wrapping around and incrementing the loop counter at the end of the
 * sequence) and sets the deadline of the following frame, counted from the
 * deadline of the current one.
 *
 * Once the last frame of the last loop has been held for its delay, the
 * animation is marked as not running and the completion work of the default
 * player is submitted, which removes it from the player and invokes its
 * completion callback.
 *
 * @param channel Channel to advance. Its animation must not be @c NULL.
 *
 * @retval 0 A frame was played, or the animation has completed.
 * @retval -EINVAL @p channel is @c NULL.
 * @retval <0 Error returned by led_on() or led_off(); the frame is consumed
 * and the deadline of the next one is set.
 */
int led_player_play_frame(struct led_player_channel *channel);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_LED_LED_PLAYER_PRIV_H_ */
