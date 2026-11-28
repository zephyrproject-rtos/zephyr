/*
 * Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/led/led_fake.h>

#include <zephyr/fff.h>
DEFINE_FFF_GLOBALS;

#include <zephyr/led/led_player.h>

#include "led_player_priv.h"

#define FAKE_LED1_DEV DEVICE_DT_GET(DT_NODELABEL(led_1))
#define FAKE_LED2_DEV DEVICE_DT_GET(DT_NODELABEL(led_2))
#define FAKE_LED3_DEV DEVICE_DT_GET(DT_NODELABEL(led_3))

static DEFINE_LED_ANIM(test_anim1, 1, 1000, 2000);
static DEFINE_LED_ANIM(test_anim2, 1, 500, 500);
static DEFINE_LED_INFINITE_ANIM(test_anim3, 500, 500);
static DEFINE_LED_ANIM(test_anim4, 1, 750, 750);

static struct led_player_channel channel1;
static struct led_player_channel channel2;
static struct led_player_channel init_channel =
	LED_PLAYER_CHANNEL_INITIALIZER(FAKE_LED1_DEV, 0, false);

ZTEST(led_player, test_led_player_get_next_anim_invalid_vals)
{
	struct led_player_channel *data;

	/* In case of error, dealy should be 0 */
	data = led_player_get_next_anim(NULL);
	zassert_equal_ptr(data, NULL);
}

ZTEST(led_player, test_led_player_get_next_anim)
{
	struct led_player_channel *data;
	struct led_player_channel channel3;
	struct led_anim_player player;

	led_player_do_init(&player);

	led_player_channel_init(&channel1, FAKE_LED1_DEV, 0, false);
	channel1.anim = &test_anim1;
	channel1.current_frame = 0;
	channel1.current_loop = 0;
	channel1.deadline = k_uptime_get() + 34;

	led_player_channel_init(&channel2, FAKE_LED2_DEV, 0, false);
	channel2.anim = &test_anim2;
	channel2.current_frame = 0;
	channel2.current_loop = 0;
	channel2.deadline = k_uptime_get() + 2;

	led_player_channel_init(&channel3, FAKE_LED3_DEV, 0, false);
	channel3.anim = &test_anim3;
	channel3.current_frame = 0;
	channel3.current_loop = 0;
	channel3.deadline = k_uptime_get() + 21;

	sys_slist_prepend(&player.list, &channel1.node);
	sys_slist_prepend(&player.list, &channel2.node);
	sys_slist_prepend(&player.list, &channel3.node);

	data = led_player_get_next_anim(&player);
	zassert_equal_ptr(data, &channel2);

	/* Remove animation and get the new one */
	sys_slist_find_and_remove(&player.list, &channel2.node);
	data = led_player_get_next_anim(&player);
	zassert_equal_ptr(data, &channel3);

	/* If there is not animation loaded, should return 0*/
	sys_slist_init(&player.list);
	data = led_player_get_next_anim(&player);
	zassert_equal_ptr(data, NULL);
}

ZTEST(led_player, test_led_player_channel_init_invalid_vals)
{
	int ret;

	ret = led_player_channel_init(NULL, FAKE_LED1_DEV, 0, false);
	zassert_equal(ret, -EINVAL);

	ret = led_player_channel_init(&channel1, NULL, 0, false);
	zassert_equal(ret, -EINVAL);
}

ZTEST(led_player, test_led_player_channel_init)
{
	int ret;

	ret = led_player_channel_init(&channel1, FAKE_LED1_DEV, 0, false);
	zassert_equal(ret, 0);
	zassert_equal(channel1.dev, FAKE_LED1_DEV);
	zassert_equal(channel1.led, 0);
	zassert_false(channel1.default_state);
	zassert_equal(fake_led_off_fake.call_count, 1);
	zassert_equal(fake_led_on_fake.call_count, 0);

	ret = led_player_channel_init(&channel2, FAKE_LED2_DEV, 0, true);
	zassert_equal(ret, 0);
	zassert_true(channel2.default_state);
	zassert_equal(fake_led_on_fake.call_count, 1);
	zassert_equal_ptr(fake_led_on_fake.arg0_val, FAKE_LED2_DEV);
}

ZTEST(led_player, test_led_player_channel_initializer)
{
	int ret;

	zassert_equal(init_channel.dev, FAKE_LED1_DEV);
	zassert_equal(init_channel.led, 0);
	zassert_is_null(init_channel.anim);
	zassert_false(led_player_is_playing(&init_channel));

	ret = led_player_play_anim(&init_channel, &test_anim1);
	zassert_equal(ret, 0);
	zassert_true(led_player_is_playing(&init_channel));
	zassert_equal_ptr(fake_led_on_fake.arg0_val, FAKE_LED1_DEV);

	led_player_stop_anim(&init_channel);
}

ZTEST(led_player, test_led_player_play_frame_invalid_vals)
{
	int ret;

	ret = led_player_play_frame(NULL);
	zassert_equal(ret, -EINVAL);
}

ZTEST(led_player, test_led_player_play_frame)
{
	int ret;

	ret = led_player_channel_init(&channel1, FAKE_LED1_DEV, 0, false);
	zassert_equal(ret, 0);

	channel1.current_frame = 0;
	channel1.current_loop = 0;
	channel1.anim = &test_anim1;

	ret = led_player_play_frame(&channel1);
	zassert_equal(ret, 0);

	zassert_equal_ptr(fake_led_on_fake.arg0_val, FAKE_LED1_DEV);
	zassert_equal_ptr(fake_led_on_fake.arg1_val, 0);

	ret = led_player_play_frame(&channel1);
	zassert_equal(ret, 0);
	zassert_equal_ptr(fake_led_off_fake.arg0_val, FAKE_LED1_DEV);
	zassert_equal_ptr(fake_led_off_fake.arg1_val, 0);

	ret = led_player_play_frame(&channel1);
	zassert_equal(ret, 0);
	/* led_player_channel_init() turned the LED off once */
	zassert_equal_ptr(fake_led_on_fake.call_count, 1);
	zassert_equal_ptr(fake_led_off_fake.call_count, 2);
}

ZTEST(led_player, test_led_player_schedule_next_invalid_vals)
{
	int ret;

	ret = led_player_schedule_next(NULL);
	zassert_equal(ret, -EINVAL);
}

ZTEST(led_player, test_led_player_schedule_next)
{
	struct led_anim_player player;
	int ret;

	led_player_do_init(&player);
	led_player_channel_init(&channel1, FAKE_LED1_DEV, 0, false);

	channel1.current_frame = 0;
	channel1.current_loop = 0;
	channel1.anim = &test_anim1;

	ret = led_player_schedule_next(&player);
	zassert_equal(ret, 0);

	/* Prepare manually the anim to play and run it */
	channel1.deadline = k_uptime_get();
	sys_slist_prepend(&player.list, &channel1.node);
	ret = led_player_schedule_next(&player);
	zassert_equal(ret, 0);

	/* Give some time to run animation */
	k_sleep(K_MSEC(5000));

	zassert_equal_ptr(fake_led_on_fake.arg0_val, FAKE_LED1_DEV);
	zassert_equal_ptr(fake_led_on_fake.arg1_val, 0);
	zassert_equal_ptr(fake_led_off_fake.arg0_val, FAKE_LED1_DEV);
	zassert_equal_ptr(fake_led_off_fake.arg1_val, 0);
	/*
	 * led_player_channel_init() turned the LED off once. The completion is
	 * handled by the default player, not this one, so the default state is
	 * not applied again.
	 */
	zassert_equal_ptr(fake_led_on_fake.call_count, 1);
	zassert_equal_ptr(fake_led_off_fake.call_count, 2);
}

ZTEST(led_player, test_led_player_schedule_next_multiple_led)
{
	struct led_anim_player player;
	int ret;

	led_player_do_init(&player);

	led_player_channel_init(&channel1, FAKE_LED1_DEV, 0, false);
	channel1.anim = &test_anim1;
	channel1.current_frame = 0;
	channel1.current_loop = 0;

	led_player_channel_init(&channel2, FAKE_LED2_DEV, 0, false);
	channel2.anim = &test_anim1;
	channel2.current_frame = 0;
	channel2.current_loop = 0;

	/* Prepare manually the anim to play and run it */
	channel1.deadline = k_uptime_get();
	channel2.deadline = k_uptime_get() + 500;
	sys_slist_prepend(&player.list, &channel1.node);
	sys_slist_prepend(&player.list, &channel2.node);
	ret = led_player_schedule_next(&player);
	zassert_equal(ret, 0);

	/* Give some time to run animation */
	k_sleep(K_MSEC(5000));

	/* The first two led_off() calls come from led_player_channel_init() */
	zassert_equal_ptr(fake_led_on_fake.arg0_history[0], FAKE_LED1_DEV);
	zassert_equal_ptr(fake_led_on_fake.arg0_history[1], FAKE_LED2_DEV);
	zassert_equal_ptr(fake_led_off_fake.arg0_history[2], FAKE_LED1_DEV);
	zassert_equal_ptr(fake_led_off_fake.arg0_history[3], FAKE_LED2_DEV);
	zassert_equal_ptr(fake_led_on_fake.call_count, 2);
	zassert_equal_ptr(fake_led_off_fake.call_count, 4);
}

ZTEST(led_player, test_led_player_schedule_next_multiple_led_and_anim)
{
	struct led_anim_player player;
	int ret;

	led_player_do_init(&player);

	led_player_channel_init(&channel1, FAKE_LED1_DEV, 0, false);
	channel1.anim = &test_anim1;
	channel1.current_frame = 0;
	channel1.current_loop = 0;

	led_player_channel_init(&channel2, FAKE_LED2_DEV, 0, false);
	channel2.anim = &test_anim2;
	channel2.current_frame = 0;
	channel2.current_loop = 0;

	/* Prepare manually the anim to play and run it */
	channel1.deadline = k_uptime_get();
	channel2.deadline = k_uptime_get() + 100;
	sys_slist_prepend(&player.list, &channel1.node);
	sys_slist_prepend(&player.list, &channel2.node);
	ret = led_player_schedule_next(&player);
	zassert_equal(ret, 0);

	/* Give some time to run animation */
	k_sleep(K_MSEC(5000));

	/* The first two led_off() calls come from led_player_channel_init() */
	zassert_equal_ptr(fake_led_on_fake.arg0_history[0], FAKE_LED1_DEV);
	zassert_equal_ptr(fake_led_on_fake.arg0_history[1], FAKE_LED2_DEV);
	zassert_equal_ptr(fake_led_off_fake.arg0_history[2], FAKE_LED2_DEV);
	zassert_equal_ptr(fake_led_off_fake.arg0_history[3], FAKE_LED1_DEV);
	zassert_equal_ptr(fake_led_on_fake.call_count, 2);
	zassert_equal_ptr(fake_led_off_fake.call_count, 4);
}

ZTEST(led_player, test_led_player_schedule_next_loop)
{
	struct k_work_sync sync;
	struct led_anim_player player;
	int ret;

	led_player_do_init(&player);
	led_player_channel_init(&channel1, FAKE_LED1_DEV, 0, false);
	channel1.anim = &test_anim3;
	channel1.current_frame = 0;
	channel1.current_loop = 0;

	/* Prepare manually the anim to play and run it */
	channel1.deadline = k_uptime_get();
	sys_slist_prepend(&player.list, &channel1.node);
	ret = led_player_schedule_next(&player);
	zassert_equal(ret, 0);

	/* Give some time to run animation */
	k_sleep(K_MSEC(5000));

	/* The anim is set to infinite, so we need to stop it to let the test finish */
	k_work_cancel_delayable_sync(&player.dwork, &sync);

	for (int i = 0; i < 4; i++) {
		zassert_equal_ptr(fake_led_on_fake.arg0_history[0], FAKE_LED1_DEV);
		zassert_equal_ptr(fake_led_off_fake.arg0_history[0], FAKE_LED1_DEV);
	}
}

ZTEST(led_player, test_led_player_play_anim_invalid_vals)
{
	int ret;

	ret = led_player_channel_init(&channel1, FAKE_LED1_DEV, 0, false);
	zassert_equal(ret, 0);

	ret = led_player_play_anim(NULL, &test_anim1);
	zassert_equal(ret, -EINVAL);

	ret = led_player_play_anim(&channel1, NULL);
	zassert_equal(ret, -EINVAL);
}

ZTEST(led_player, test_led_player_play_anim)
{
	int ret;

	led_player_channel_init(&channel1, FAKE_LED1_DEV, 0, false);
	channel1.anim = &test_anim1;
	channel1.current_frame = 0;
	channel1.current_loop = 0;

	ret = led_player_play_anim(&channel1, &test_anim1);
	zassert_equal(ret, 0);

	/* Give some time to run animation */
	k_sleep(K_MSEC(5000));

	zassert_equal_ptr(fake_led_on_fake.arg0_history[0], FAKE_LED1_DEV);
	zassert_equal_ptr(fake_led_off_fake.arg0_history[0], FAKE_LED1_DEV);
	/* led_off() by led_player_channel_init(), the second frame and the completion */
	zassert_equal_ptr(fake_led_on_fake.call_count, 1);
	zassert_equal_ptr(fake_led_off_fake.call_count, 3);
}

static struct led_player_channel *test_cb_channel;
static bool test_cb_playing;

static void test_led_player_complete_cb(struct led_player_channel *channel, void *user_data)
{
	int *count = (int *)(user_data);

	test_cb_channel = channel;
	test_cb_playing = led_player_is_playing(channel);
	*count += 1;
}

ZTEST(led_player, test_led_player_play_anim_with_cb)
{
	int ret;
	int count = 1;

	led_player_channel_init(&channel1, FAKE_LED1_DEV, 0, false);

	test_cb_channel = NULL;
	ret = led_player_play_anim_with_cb(&channel1, &test_anim1, test_led_player_complete_cb,
					   &count);
	zassert_equal(ret, 0);
	zassert_equal(count, 1);

	/* Give some time to run animation */
	k_sleep(K_MSEC(5000));

	zassert_equal(count, 2);
	zassert_equal_ptr(test_cb_channel, &channel1);
}

ZTEST(led_player, test_led_player_play_anim_with_cb_other_completes_first)
{
	int count = 1;

	led_player_channel_init(&channel1, FAKE_LED1_DEV, 0, false);
	led_player_channel_init(&channel2, FAKE_LED2_DEV, 0, false);

	led_player_play_anim_with_cb(&channel1, &test_anim1, test_led_player_complete_cb, &count);
	led_player_play_anim(&channel2, &test_anim4);

	/* channel2 completes at 1.5 s, while channel1 holds its last frame until 3 s */
	k_sleep(K_MSEC(2000));
	zassert_equal(count, 1);
	zassert_true(led_player_is_playing(&channel1));

	k_sleep(K_MSEC(1500));
	zassert_equal(count, 2);
	zassert_false(led_player_is_playing(&channel1));
}

ZTEST(led_player, test_led_player_stop_anim_invalid_vals)
{
	int ret;

	ret = led_player_stop_anim(NULL);
	zassert_equal(ret, -EINVAL);
}

ZTEST(led_player, test_led_player_stop_anim)
{
	int count_clear, count_set;
	int ret;

	led_player_channel_init(&channel1, FAKE_LED1_DEV, 0, false);
	led_player_play_anim(&channel1, &test_anim3);

	/* Give some time to run animation */
	k_sleep(K_MSEC(5000));

	ret = led_player_stop_anim(&channel1);
	zassert_equal(ret, 0);

	/* Make sure anim has been stopped */
	count_clear = fake_led_off_fake.call_count;
	count_set = fake_led_on_fake.call_count;
	k_sleep(K_MSEC(5000));
	zassert_equal(fake_led_off_fake.call_count, count_clear);
	zassert_equal(fake_led_on_fake.call_count, count_set);
}

ZTEST(led_player, test_led_player_stop_anim_one_of_two)
{
	int count_clear, count_set;
	int ret;

	led_player_channel_init(&channel1, FAKE_LED1_DEV, 0, false);
	led_player_channel_init(&channel2, FAKE_LED2_DEV, 0, false);

	led_player_play_anim(&channel1, &test_anim3);
	led_player_play_anim(&channel2, &test_anim3);

	/* Give some time to run animation */
	k_sleep(K_MSEC(5000));

	ret = led_player_stop_anim(&channel1);
	zassert_equal(ret, 0);

	/* Make sure that we still have an anim running */
	count_clear = fake_led_off_fake.call_count;
	count_set = fake_led_on_fake.call_count;
	k_sleep(K_MSEC(5000));
	zassert_not_equal(fake_led_off_fake.call_count, count_clear);
	zassert_not_equal(fake_led_on_fake.call_count, count_set);

	led_player_stop_anim(&channel2);
}

ZTEST(led_player, test_led_player_stop_anim_already_stopped)
{
	int ret;

	led_player_channel_init(&channel1, FAKE_LED1_DEV, 0, false);
	led_player_play_anim(&channel1, &test_anim1);

	/* Give some time to run animation. Anim should have stop */
	k_sleep(K_MSEC(5000));

	ret = led_player_stop_anim(&channel1);
	zassert_equal(ret, 0);
}

ZTEST(led_player, test_led_player_play_anim_multiple_led_with_one_finite_anim)
{
	struct led_anim_player player;

	led_player_do_init(&player);
	led_player_channel_init(&channel1, FAKE_LED1_DEV, 0, false);
	led_player_channel_init(&channel2, FAKE_LED2_DEV, 0, false);

	led_player_play_anim(&channel1, &test_anim3);
	led_player_play_anim(&channel2, &test_anim4);

	/*
	 * Check halfway between two frames, so that the tick k_sleep() adds
	 * doesn't change which frames have been played.
	 */
	k_sleep(K_MSEC(1250));

	/* Two led_off() calls come from led_player_channel_init() */
	zassert_equal_ptr(fake_led_on_fake.call_count, 3);
	zassert_equal_ptr(fake_led_off_fake.call_count, 4);

	/*
	 * The finite anim has completed, which set its LED to the default state,
	 * the infinite one is still running.
	 */
	k_sleep(K_MSEC(1000));

	zassert_equal_ptr(fake_led_on_fake.call_count, 4);
	zassert_equal_ptr(fake_led_off_fake.call_count, 6);

	led_player_stop_anim(&channel1);
}

ZTEST(led_player, test_led_player_is_playing_invalid_vals)
{
	zassert_false(led_player_is_playing(NULL));
}

ZTEST(led_player, test_led_player_is_playing)
{

	led_player_channel_init(&channel1, FAKE_LED1_DEV, 0, false);
	zassert_false(led_player_is_playing(&channel1));

	zassert_false(led_player_is_playing(&channel1));

	led_player_play_anim(&channel1, &test_anim1);
	zassert_true(led_player_is_playing(&channel1));

	/* test_anim1 lasts 3 s */
	k_sleep(K_MSEC(1500));
	zassert_true(led_player_is_playing(&channel1));

	k_sleep(K_MSEC(2000));
	zassert_false(led_player_is_playing(&channel1));
}

ZTEST(led_player, test_led_player_is_playing_stopped)
{

	led_player_channel_init(&channel1, FAKE_LED1_DEV, 0, false);
	led_player_play_anim(&channel1, &test_anim3);
	k_sleep(K_MSEC(1500));
	zassert_true(led_player_is_playing(&channel1));

	led_player_stop_anim(&channel1);
	zassert_false(led_player_is_playing(&channel1));
}

ZTEST(led_player, test_led_player_is_playing_one_of_two)
{

	led_player_channel_init(&channel1, FAKE_LED1_DEV, 0, false);
	led_player_channel_init(&channel2, FAKE_LED2_DEV, 0, false);

	led_player_play_anim(&channel1, &test_anim3);
	led_player_play_anim(&channel2, &test_anim3);

	led_player_stop_anim(&channel1);
	zassert_false(led_player_is_playing(&channel1));
	zassert_true(led_player_is_playing(&channel2));

	led_player_stop_anim(&channel2);
}

ZTEST(led_player, test_led_player_is_playing_in_complete_cb)
{
	int count = 1;

	test_cb_playing = true;
	led_player_channel_init(&channel1, FAKE_LED1_DEV, 0, false);
	led_player_play_anim_with_cb(&channel1, &test_anim1, test_led_player_complete_cb, &count);

	k_sleep(K_MSEC(5000));

	zassert_equal(count, 2);
	zassert_false(test_cb_playing);
}

ZTEST(led_player, test_led_player_stop_anim_sets_default_state)
{
	int ret;

	led_player_channel_init(&channel1, FAKE_LED1_DEV, 0, true);
	led_player_play_anim(&channel1, &test_anim3);

	/* Stop during an off frame of test_anim3 */
	k_sleep(K_MSEC(750));
	zassert_equal(fake_led_on_fake.call_count, 2);
	zassert_equal(fake_led_off_fake.call_count, 1);

	ret = led_player_stop_anim(&channel1);
	zassert_equal(ret, 0);
	zassert_equal(fake_led_on_fake.call_count, 3);
	zassert_equal(fake_led_off_fake.call_count, 1);
}

ZTEST(led_player, test_led_player_complete_sets_default_state)
{
	led_player_channel_init(&channel1, FAKE_LED1_DEV, 0, true);
	led_player_play_anim(&channel1, &test_anim1);

	/* test_anim1 ends with an off frame, and completes after 3 s */
	k_sleep(K_MSEC(2500));
	zassert_equal(fake_led_on_fake.call_count, 2);
	zassert_equal(fake_led_off_fake.call_count, 1);

	k_sleep(K_MSEC(1000));
	zassert_equal(fake_led_on_fake.call_count, 3);
	zassert_equal(fake_led_off_fake.call_count, 1);
	zassert_false(led_player_is_playing(&channel1));
}

static void led_player_after(void *fixture)
{
	ARG_UNUSED(fixture);

	/*
	 * A failed assertion returns before the test stops its channels, which
	 * would keep driving the fake LEDs during the following tests.
	 */
	led_player_stop_anim(&channel1);
	led_player_stop_anim(&channel2);
	led_player_stop_anim(&init_channel);
}

ZTEST_SUITE(led_player, NULL, NULL, NULL, led_player_after, NULL);
