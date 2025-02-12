/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <string.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/debug/stack.h>

#include <zephyr/display/mb_display.h>

#include <zephyr/bluetooth/bluetooth.h>

#include "pong.h"

/* The micro:bit has a 5x5 LED display, using (x, y) notation the top-left
 * corner has coordinates (0, 0) and the bottom-right has (4, 4). To make
 * the game dynamics more natural, the uses a virtual 50x50 coordinate
 * system where top-left is (0, 0) and bottom-right is (49, 49).
 */

#define SCROLL_SPEED      400          /* Text scrolling speed */

#define PIXEL_SIZE        10           /* Virtual coordinates per real pixel */

#define GAME_REFRESH      K_MSEC(100)  /* Animation refresh rate of the game */

#define PADDLE_ROW        4            /* Real Y coordinate of the paddle */
#define PADDLE_MIN        0            /* Minimum paddle real X coordinate */
#define PADDLE_MAX        3            /* Maximum paddle real X coordinate */

#define BALL_VEL_Y_START  -4           /* Default ball vertical speed */

#define BALL_POS_X_MIN    0            /* Maximum ball X coordinate */
#define BALL_POS_X_MAX    49           /* Maximum ball X coordinate */
#define BALL_POS_Y_MIN    0            /* Maximum ball Y coordinate */
#define BALL_POS_Y_MAX    39           /* Maximum ball Y coordinate */

#define START_THRESHOLD   100          /* Max time between A & B press */
#define RESTART_THRESHOLD (2 * MSEC_PER_SEC) /* Time before restart is
					      *	allowed
					      */

#define REAL_TO_VIRT(r)  ((r) * 10)
#define VIRT_TO_REAL(v)  ((v) / 10)

/* Ball starting position (just to the left of the paddle mid-point) */
#define BALL_START       (struct x_y){ 4, BALL_POS_Y_MAX }

struct x_y {
	int x;
	int y;
};

enum pong_state {
	INIT,
	MULTI,
	SINGLE,
	CONNECTED,
};

static enum pong_state pg_state = INIT;

struct pong_choice {
	int val;
	const char *str;
};

struct pong_selection {
	const struct pong_choice *choice;
	size_t choice_count;
	void (*complete)(int val);
};

static int select_idx;
static const struct pong_selection *select;

static const struct pong_choice mode_choice[] = {
	{ SINGLE,   "Single" },
	{ MULTI,    "Multi" },
};

static bool remote_lost;
static bool started;
static int64_t ended;

static struct k_work_delayable refresh;

/* Semaphore to indicate that there was an update to the display */
static K_SEM_DEFINE(disp_update, 0, 1);

/* X coordinate of the left corner of the paddle */
static volatile int paddle_x = PADDLE_MIN;

/* Ball position */
static struct x_y ball_pos = BALL_START;

/* Ball velocity */
static struct x_y ball_vel = { 0, 0 };

static int64_t a_timestamp;
static int64_t b_timestamp;

#define SOUND_PERIOD_PADDLE  PWM_USEC(200)
#define SOUND_PERIOD_WALL    PWM_USEC(1000)

static const struct pwm_dt_spec pwm = PWM_DT_SPEC_GET(DT_PATH(zephyr_user));

static enum sound_state {
	SOUND_IDLE,    /* No sound */
	SOUND_PADDLE,  /* Ball has hit the paddle */
	SOUND_WALL,    /* Ball has hit a wall */
} sound_state;

static const struct gpio_dt_spec sw0_gpio = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static const struct gpio_dt_spec sw1_gpio = GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios);

/* ensure SW0 & SW1 are on same gpio controller */
BUILD_ASSERT(DT_SAME_NODE(DT_GPIO_CTLR(DT_ALIAS(sw0), gpios), DT_GPIO_CTLR(DT_ALIAS(sw1), gpios)));

static inline void beep(uint32_t period)
{
	pwm_set_dt(&pwm, period, period / 2);
}

static void sound_set(enum sound_state state)
{
	switch (state) {
	case SOUND_IDLE:
		beep(0);
		break;
	case SOUND_PADDLE:
		beep(SOUND_PERIOD_PADDLE);
		break;
	case SOUND_WALL:
		beep(SOUND_PERIOD_WALL);
		break;
	}

	sound_state = state;
}

static void pong_select(const struct pong_selection *sel)
{
	struct mb_display *disp = mb_display_get();

	if (select) {
		printk("Other selection still busy\n");
		return;
	}

	select = sel;
	select_idx = 0;

	mb_display_print(disp, MB_DISPLAY_MODE_DEFAULT | MB_DISPLAY_FLAG_LOOP,
			 SCROLL_SPEED, "%s", select->choice[select_idx].str);
}

static void pong_select_change(void)
{
	struct mb_display *disp = mb_display_get();

	select_idx = (select_idx + 1) % select->choice_count;
	mb_display_print(disp, MB_DISPLAY_MODE_DEFAULT | MB_DISPLAY_FLAG_LOOP,
			 SCROLL_SPEED, "%s", select->choice[select_idx].str);
}

static void pong_select_complete(void)
{
	struct mb_display *disp = mb_display_get();
	void (*complete)(int val) = select->complete;
	int val = select->choice[select_idx].val;

	mb_display_stop(disp);

	select = NULL;
	complete(val);
}

static void game_init(bool initiator)
{
	started = false;
	ended = 0;

	ball_pos = BALL_START;
	if (!initiator) {
		ball_pos.y = -1;
	}

	paddle_x = PADDLE_MIN;

	a_timestamp = 0;
	b_timestamp = 0;
}

static void mode_selected(int val)
{
	struct mb_display *disp = mb_display_get();

	pg_state = val;

	switch (pg_state) {
	case SINGLE:
		game_init(true);
		k_sem_give(&disp_update);
		break;
	case MULTI:
		ble_connect();
		mb_display_print(disp,
				 MB_DISPLAY_MODE_DEFAULT | MB_DISPLAY_FLAG_LOOP,
				 SCROLL_SPEED, "Connecting...");
		break;
	default:
		printk("Unknown state %d\n", pg_state);
		return;
	}
}

static const struct pong_selection mode_selection = {
	.choice = mode_choice,
	.choice_count = ARRAY_SIZE(mode_choice),
	.complete = mode_selected,
};

static bool ball_visible(void)
{
	return (ball_pos.y >= BALL_POS_Y_MIN);
}

static void check_start(void)
{
	uint32_t delta;
	uint8_t rnd;

	if (!a_timestamp || !b_timestamp) {
		return;
	}

	if (a_timestamp > b_timestamp) {
		delta = a_timestamp - b_timestamp;
	} else {
		delta = b_timestamp - a_timestamp;
	}

	printk("delta %u ms\n", delta);

	if (delta > START_THRESHOLD) {
		return;
	}

	ball_vel.y = BALL_VEL_Y_START;

	bt_rand(&rnd, sizeof(rnd));
	rnd %= 8;

	if (a_timestamp > b_timestamp) {
		ball_vel.x = 2 + rnd;
	} else {
		ball_vel.x = -2 - rnd;
	}

	started = true;
	remote_lost = false;
	k_work_reschedule(&refresh, K_NO_WAIT);
}

static void game_ended(bool won)
{
	struct mb_display *disp = mb_display_get();

	if (sound_state != SOUND_IDLE) {
		sound_set(SOUND_IDLE);
	}

	remote_lost = won;
	ended = k_uptime_get();
	started = false;

	if (won) {
		struct mb_image img = MB_IMAGE({ 0, 1, 0, 1, 0 },
					       { 0, 1, 0, 1, 0 },
					       { 0, 0, 0, 0, 0 },
					       { 1, 0, 0, 0, 1 },
					       { 0, 1, 1, 1, 0 });
		mb_display_image(disp, MB_DISPLAY_MODE_SINGLE,
				 RESTART_THRESHOLD, &img, 1);
		printk("You won!\n");
	} else {
		struct mb_image img = MB_IMAGE({ 0, 1, 0, 1, 0 },
					       { 0, 1, 0, 1, 0 },
					       { 0, 0, 0, 0, 0 },
					       { 0, 1, 1, 1, 0 },
					       { 1, 0, 0, 0, 1 });
		mb_display_image(disp, MB_DISPLAY_MODE_SINGLE,
				 RESTART_THRESHOLD, &img, 1);
		printk("You lost!\n");
	}

	k_work_reschedule(&refresh, K_MSEC(RESTART_THRESHOLD));
}

#if CONFIG_THREAD_MONITOR
static void game_stack_dump(const struct k_thread *thread, void *user_data)
{
	ARG_UNUSED(user_data);

	log_stack_usage(thread);
}
#endif

static void game_refresh(struct k_work *work)
{
	if (sound_state != SOUND_IDLE) {
		sound_set(SOUND_IDLE);
#if CONFIG_THREAD_MONITOR
		k_thread_foreach(game_stack_dump, NULL);
#endif
	}

	if (pg_state == INIT) {
		pong_select(&mode_selection);
		return;
	}

	if (ended) {
		game_init(pg_state == SINGLE || remote_lost);
		k_sem_give(&disp_update);
		return;
	}

	ball_pos.x += ball_vel.x;
	ball_pos.y += ball_vel.y;

	/* Ball went over to the other side */
	if (ball_vel.y < 0 && ball_pos.y < BALL_POS_Y_MIN) {
		if (pg_state == SINGLE) {
			ball_pos.y = -ball_pos.y;
			ball_vel.y = -ball_vel.y;
			sound_set(SOUND_WALL);
		} else {
			ble_send_ball(BALL_POS_X_MAX - ball_pos.x, ball_pos.y,
				      -ball_vel.x, -ball_vel.y);
			k_sem_give(&disp_update);
			return;
		}
	}

	/* Check for side-wall collision */
	if (ball_pos.x < BALL_POS_X_MIN) {
		ball_pos.x = -ball_pos.x;
		ball_vel.x = -ball_vel.x;
		sound_set(SOUND_WALL);
	} else if (ball_pos.x > BALL_POS_X_MAX) {
		ball_pos.x = (2 * BALL_POS_X_MAX) - ball_pos.x;
		ball_vel.x = -ball_vel.x;
		sound_set(SOUND_WALL);
	}

	/* Ball approaching paddle */
	if (ball_vel.y > 0 && ball_pos.y > BALL_POS_Y_MAX) {
		if (ball_pos.x < REAL_TO_VIRT(paddle_x) ||
		    ball_pos.x >= REAL_TO_VIRT(paddle_x + 2)) {
			game_ended(false);
			if (pg_state == CONNECTED) {
				ble_send_lost();
			}
			return;
		}

		ball_pos.y = (2 * BALL_POS_Y_MAX) - ball_pos.y;

		/* Make the game play gradually harder */
		if (ball_vel.y < PIXEL_SIZE) {
			ball_vel.y++;
		}

		ball_vel.y = -ball_vel.y;

		sound_set(SOUND_PADDLE);
	}

	k_work_reschedule(&refresh, GAME_REFRESH);
	k_sem_give(&disp_update);
}

void pong_ball_received(int8_t x_pos, int8_t y_pos, int8_t x_vel, int8_t y_vel)
{
	printk("ball_received(%d, %d, %d, %d)\n", x_pos, y_pos, x_vel, y_vel);

	ball_pos.x = x_pos;
	ball_pos.y = y_pos;
	ball_vel.x = x_vel;
	ball_vel.y = y_vel;

	k_work_reschedule(&refresh, K_NO_WAIT);
}

static void button_pressed(const struct device *dev, struct gpio_callback *cb,
			   uint32_t pins)
{
	/* Filter out spurious presses */
	if (pins & BIT(sw0_gpio.pin)) {
		printk("A pressed\n");
		if (k_uptime_delta(&a_timestamp) < 100) {
			printk("Too quick A presses\n");
			return;
		}
	} else {
		printk("B pressed\n");
		if (k_uptime_delta(&b_timestamp) < 100) {
			printk("Too quick B presses\n");
			return;
		}
	}

	if (ended && (k_uptime_get() - ended) > RESTART_THRESHOLD) {
		int busy = k_work_cancel_delayable(&refresh);

		if (busy != 0) {
			printk("WARNING: Data-race (work and event)\n");
		}

		game_init(pg_state == SINGLE || remote_lost);
		k_sem_give(&disp_update);
		return;
	}

	if (pg_state == MULTI) {
		ble_cancel_connect();
		pg_state = INIT;
		pong_select(&mode_selection);
		return;
	}

	if (pins & BIT(sw0_gpio.pin)) {
		if (select) {
			pong_select_change();
			return;
		}

		if (!started) {
			check_start();
		}

		if (paddle_x > PADDLE_MIN) {
			paddle_x--;
			if (!started) {
				ball_pos.x -= PIXEL_SIZE;
			}

			k_sem_give(&disp_update);
		}
	} else {
		if (select) {
			pong_select_complete();
			return;
		}

		if (!started) {
			check_start();
		}

		if (paddle_x < PADDLE_MAX) {
			paddle_x++;
			if (!started) {
				ball_pos.x += PIXEL_SIZE;
			}

			k_sem_give(&disp_update);
		}
	}
}

void pong_conn_ready(bool initiator)
{
	pg_state = CONNECTED;
	game_init(initiator);
	k_sem_give(&disp_update);
}

void pong_remote_disconnected(void)
{
	pg_state = INIT;
	k_work_reschedule(&refresh, K_SECONDS(1));
}

void pong_remote_lost(void)
{
	printk("Remote lost!\n");
	game_ended(true);
}

static void configure_buttons(void)
{
	static struct gpio_callback button_cb_data;

	/* since sw0_gpio.port == sw1_gpio.port, we only need to check ready once */
	if (!gpio_is_ready_dt(&sw0_gpio)) {
		printk("%s: device not ready.\n", sw0_gpio.port->name);
		return;
	}

	gpio_pin_configure_dt(&sw0_gpio, GPIO_INPUT);
	gpio_pin_configure_dt(&sw1_gpio, GPIO_INPUT);

	gpio_pin_interrupt_configure_dt(&sw0_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	gpio_pin_interrupt_configure_dt(&sw1_gpio, GPIO_INT_EDGE_TO_ACTIVE);

	gpio_init_callback(&button_cb_data, button_pressed,
			   BIT(sw0_gpio.pin) | BIT(sw1_gpio.pin));

	gpio_add_callback(sw0_gpio.port, &button_cb_data);
}

int main(void)
{
	struct mb_display *disp = mb_display_get();

	configure_buttons();

	k_work_init_delayable(&refresh, game_refresh);

	if (!pwm_is_ready_dt(&pwm)) {
		printk("%s: device not ready.\n", pwm.dev->name);
		return 0;
	}

	ble_init();

	pong_select(&mode_selection);

	printk("Started\n");

	while (1) {
		struct mb_image img = { };

		k_sem_take(&disp_update, K_FOREVER);

		if (ended) {
			continue;
		}

		img.row[PADDLE_ROW] = (BIT(paddle_x) | BIT(paddle_x + 1));

		if (ball_visible()) {
			img.row[VIRT_TO_REAL(ball_pos.y)] =
				BIT(VIRT_TO_REAL(ball_pos.x));
		}

		mb_display_image(disp, MB_DISPLAY_MODE_SINGLE,
				 SYS_FOREVER_MS, &img, 1);
	}
	return 0;
}
