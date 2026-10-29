/*
 * Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for the LED player subsystem
 */

#ifndef ZEPHYR_INCLUDE_LED_LED_PLAYER_H_
#define ZEPHYR_INCLUDE_LED_LED_PLAYER_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util_macro.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LED player
 * @defgroup led_player_interface LED player
 * @ingroup led_interface
 *
 * Plays on/off animations on LEDs driven by the LED driver API. All the
 * animations share a single delayed work item, running on the system
 * workqueue, armed for the closest frame deadline.
 *
 * All functions of this API may block and must be called from thread
 * context, never from an ISR.
 * @{
 */

/**
 * @brief A single step of a LED animation.
 */
struct led_anim_frame {
	/** Time in milliseconds the LED holds @ref led_anim_frame.state. Must not be 0. */
	uint16_t delay_ms;
	/** LED state for this frame: @c true for on, @c false for off. */
	bool state;
};

/**
 * @brief A LED animation, described as a sequence of frames.
 *
 * Usually defined with DEFINE_LED_ANIM() or DEFINE_LED_INFINITE_ANIM(). The
 * player only reads it, so it can be @c const, and the same animation can be
 * played on several LEDs at once.
 */
struct led_anim {
	/** Ordered array of @ref led_anim.count frames to play. Must not be @c NULL. */
	const struct led_anim_frame *frames;
	/** Number of entries in @ref led_anim.frames. Must not be 0. */
	uint16_t count;
	/**
	 * Number of times the sequence of frames is played.
	 *
	 * @ref LED_ANIM_INFINITE (UINT16_MAX) loops forever. Any other value is the exact
	 * number of times the sequence is played before the animation completes.
	 * Must not be 0.
	 */
	uint16_t loop;
};

struct led_player_channel;

/**
 * @brief Callback invoked when an animation completes.
 *
 * Only invoked when the animation has played its last frame of its last loop
 * (see @ref led_anim.loop), once the LED has been set back to the default
 * state of the channel; never invoked for an animation stopped with
 * led_player_stop_anim(), stopped because the LED driver failed, or replaced
 * by another call to led_player_play_anim() or led_player_play_anim_with_cb(),
 * nor for one that loops forever.
 *
 * Runs on the system workqueue, without any lock of the player held: it may
 * call led_player_play_anim(), led_player_play_anim_with_cb(),
 * led_player_stop_anim() or led_player_is_playing().
 *
 * @param channel The channel whose animation has completed.
 * @param user_data The @p user_data passed to led_player_play_anim_with_cb().
 */
typedef void (*led_anim_complete_cb)(struct led_player_channel *channel, void *user_data);

/**
 * @brief A LED driven by the player, and the animation played on it.
 *
 * One channel is needed per animated LED. It must be initialized with
 * led_player_channel_init() or LED_PLAYER_CHANNEL_INITIALIZER() before being
 * passed to any other function of this API, and must remain valid while its
 * animation is playing. Its fields are
 * managed by the player and must not be modified directly.
 */
struct led_player_channel {
	/** LED controller device driving @ref led_player_channel.led. */
	const struct device *dev;
	/** LED index on @ref led_player_channel.dev. */
	uint32_t led;
	/** State of the LED while no animation is playing: @c true for on. */
	bool default_state;

	/** Animation played on the LED. */
	const struct led_anim *anim;
	/** @cond INTERNAL_HIDDEN */
	/* Index of the next frame of anim to play. */
	uint16_t current_frame;
	/* Current iteration of animation. */
	uint16_t current_loop;
	/*
	 * Uptime in milliseconds at which the next frame must be played,
	 * INT64_MAX once the animation has completed. Only valid while the
	 * channel is in the player's list.
	 */
	int64_t deadline;

	/* Completion callback and its argument, see led_player_play_anim_with_cb(). */
	led_anim_complete_cb cb;
	void *user_data;

	/* Link in the player's list of playing channels. */
	sys_snode_t node;
	/** @endcond */
};

/** Value of @ref led_anim.loop for an animation looping forever. */
#define LED_ANIM_INFINITE UINT16_MAX

/**
 * @brief Initializer for a @ref led_anim_frame.
 *
 * @param _delay Time in milliseconds the LED holds @p _state. Must not be 0.
 * @param _state @c true for on, @c false for off.
 */
#define LED_ANIM_FRAME(_delay, _state)                                                             \
	{                                                                                          \
		.delay_ms = _delay,                                                                \
		.state = _state,                                                                   \
	}

/** @cond INTERNAL_HIDDEN */
/* Even frames turn the LED on, odd frames off. */

#define LED_ANIM_FRAME_IDX(_idx, _delay) LED_ANIM_FRAME(_delay, ((_idx) % 2) == 0)
/** @endcond */

/**
 * @brief Initializers for a sequence of alternating on/off frames.
 *
 * The first frame turns the LED on, the second off, and so on.
 *
 * @param ... Delay in milliseconds of each frame (up to 64), none being 0.
 */
#define LED_ANIM_FRAMES(...) FOR_EACH_IDX(LED_ANIM_FRAME_IDX, (,), __VA_ARGS__)

/**
 * @brief Compound literal array of alternating on/off frames.
 *
 * @param ... Delay in milliseconds of each frame, see LED_ANIM_FRAMES().
 */
#define LED_ANIM_FRAME_ARRAY(...)                                                                  \
	((const struct led_anim_frame[]){                                                          \
		LED_ANIM_FRAMES(__VA_ARGS__),                                                      \
	})

/**
 * @brief Define a constant LED animation made of alternating on/off frames.
 *
 * Must be used at file scope: the frames are a compound literal, which only
 * has static storage duration outside of a function.
 *
 * For example, a single 100 ms flash followed by 900 ms off, played 3 times:
 * @code{.c}
 * static DEFINE_LED_ANIM(flash_anim, 3, 100, 900);
 * @endcode
 *
 * @param _name Name of the @c const @ref led_anim variable to define.
 * @param _loop Number of times the sequence is played, see @ref led_anim.loop.
 * @param ... Delay in milliseconds of each frame, see LED_ANIM_FRAMES().
 */
#define DEFINE_LED_ANIM(_name, _loop, ...)                                                         \
	const struct led_anim _name = {                                                            \
		.frames = LED_ANIM_FRAME_ARRAY(__VA_ARGS__),                                       \
		.count = ARRAY_SIZE(LED_ANIM_FRAME_ARRAY(__VA_ARGS__)),                            \
		.loop = _loop,                                                                     \
	}

/**
 * @brief Define a constant LED animation made of alternating on/off frames,
 * looping forever.
 *
 * @param _name Name of the @c const @ref led_anim variable to define.
 * @param ... Delay in milliseconds of each frame, see LED_ANIM_FRAMES().
 */
#define DEFINE_LED_INFINITE_ANIM(_name, ...) DEFINE_LED_ANIM(_name, LED_ANIM_INFINITE, __VA_ARGS__)

/**
 * @brief Static initializer for a @ref led_player_channel.
 *
 * Alternative to led_player_channel_init() for a channel defined at build
 * time, for example:
 * @code{.c}
 * static struct led_player_channel channel =
 *	LED_PLAYER_CHANNEL_INITIALIZER(DEVICE_DT_GET(DT_NODELABEL(led)), 0, false);
 * @endcode
 *
 * The LED is not set to @p _default_state until the first call to
 * led_player_stop_anim() or the first completed animation: call
 * led_player_stop_anim() once the LED driver is ready to apply it.
 *
 * @param _dev LED controller device driving @p _led. Must be a constant
 * expression, such as DEVICE_DT_GET().
 * @param _led LED index on @p _dev.
 * @param _default_state State of the LED while no animation is playing,
 * @c true for on.
 */
#define LED_PLAYER_CHANNEL_INITIALIZER(_dev, _led, _default_state)                                 \
	{                                                                                          \
		.dev = (_dev),                                                                     \
		.led = (_led),                                                                     \
		.default_state = (_default_state),                                                 \
	}

/**
 * @brief Initialize a LED player channel.
 *
 * Must be called once before any other function of this API is called on
 * @p channel, unless it was defined with LED_PLAYER_CHANNEL_INITIALIZER(), and
 * not while @p channel is playing. The animation is given when the channel is
 * played, see led_player_play_anim().
 *
 * The LED is set to @p default_state before this function returns.
 *
 * @param channel Channel to initialize.
 * @param dev LED controller device driving @p led.
 * @param led LED index on @p dev.
 * @param default_state State of the LED while no animation is playing,
 * @c true for on.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p channel or @p dev is @c NULL.
 * @retval <0 Error returned by the LED driver; @p channel is initialized.
 */
int led_player_channel_init(struct led_player_channel *channel, const struct device *dev,
			    uint32_t led, bool default_state);

/**
 * @brief Play an animation on a channel.
 *
 * Equivalent to led_player_play_anim_with_cb() with no completion callback.
 *
 * @param channel Channel to play, previously initialized with
 * led_player_channel_init().
 * @param anim Animation to play.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p channel or @p anim is @c NULL, or @p anim is invalid;
 * @p channel is left unchanged.
 */
int led_player_play_anim(struct led_player_channel *channel, const struct led_anim *anim);

/**
 * @brief Play an animation on a channel and get notified when it completes.
 *
 * Plays @p anim on @p channel from its first frame. The first frame is
 * applied to the LED before this function returns.
 *
 * If @p channel is already playing, its animation is stopped without invoking
 * its completion callback, and replaced by @p anim, @p cb and @p user_data.
 *
 * If the LED driver fails to play a frame, including the first one, the error
 * is logged and the animation is stopped, without invoking @p cb.
 *
 * @param channel Channel to play, previously initialized with
 * led_player_channel_init().
 * @param anim Animation to play.
 * @param cb Callback invoked when the animation completes, or @c NULL. See
 * @ref led_anim_complete_cb.
 * @param user_data Opaque pointer passed back to @p cb.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p channel or @p anim is @c NULL, or @p anim is invalid;
 * @p channel is left unchanged.
 */
int led_player_play_anim_with_cb(struct led_player_channel *channel, const struct led_anim *anim,
				 led_anim_complete_cb cb, void *user_data);

/**
 * @brief Stop the animation of a channel.
 *
 * The LED is set to the default state of @p channel, and the completion
 * callback is not invoked. On a channel that is not playing, only the default
 * state is applied.
 *
 * If a frame of @p channel is being played on the system workqueue, waits
 * for it to finish: once this returns, the player doesn't touch the LED of
 * @p channel anymore.
 *
 * @param channel Channel to stop.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p channel is @c NULL.
 * @retval <0 Error returned by the LED driver; the animation is stopped.
 */
int led_player_stop_anim(struct led_player_channel *channel);

/**
 * @brief Check whether a channel is playing.
 *
 * A channel whose animation has completed is not playing, even if its
 * completion callback has not been invoked yet.
 *
 * @param channel Channel to check.
 *
 * @retval true @p channel is playing.
 * @retval false @p channel is stopped, has completed, was stopped because the
 * LED driver failed, or is @c NULL.
 */
bool led_player_is_playing(struct led_player_channel *channel);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_LED_LED_PLAYER_H_ */
