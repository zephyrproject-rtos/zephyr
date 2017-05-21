/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>
#include <board.h>
#include <gpio.h>
#include <device.h>
#include <string.h>

#include <display/mb_display.h>

#include <bluetooth/bluetooth.h>

/* Define this to do a single-device game */
#define SOLO 1

/* The micro:bit has a 5x5 LED display, using (x, y) notation the top-left
 * corner has coordinates (0, 0) and the bottom-right has (4, 4). To make
 * the game dynamics more natural, the uses a virtual 50x50 coordinate
 * system where top-left is (0, 0) and bottom-right is (49, 49).
 */

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

#define START_THRESHOLD   K_MSEC(100)  /* Max time between A & B press */
#define RESTART_THRESHOLD K_SECONDS(3) /* Time before restart is allowed */

#define REAL_TO_VIRT(r)  ((r) * 10)
#define VIRT_TO_REAL(v)  ((v) / 10)

/* Ball starting position (just to the left of the paddle mid-point) */
#define BALL_START       (struct x_y){ 4, BALL_POS_Y_MAX }

struct x_y {
	int x;
	int y;
};

static bool started;
static s64_t ended;

static struct k_delayed_work refresh;

/* Semaphore to indicate that there was an update to the display */
static struct k_sem disp_update = K_SEM_INITIALIZER(disp_update, 0, 1);

/* X coordinate of the left corner of the paddle */
static volatile int paddle_x = PADDLE_MIN;

/* Ball position */
static struct x_y ball_pos = BALL_START;

/* Ball velocity */
static struct x_y ball_vel = { 0, 0 };

static s64_t a_timestamp;
static s64_t b_timestamp;

static bool ball_visible(void)
{
	return (ball_pos.y >= BALL_POS_Y_MIN);
}

static void check_start(void)
{
	u32_t delta;
	u8_t rnd;

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
	k_delayed_work_submit(&refresh, K_NO_WAIT);
}

static void game_ended(bool won)
{
	struct mb_display *disp = mb_display_get();
	const char *str;

	ended = k_uptime_get();
	started = false;

	if (won) {
		str = "You won!";
	} else {
		str = "You lost!";
	}

	printk("%s\n", str);

	mb_display_print(disp, MB_DISPLAY_MODE_DEFAULT | MB_DISPLAY_FLAG_LOOP,
			 K_MSEC(500), "%s", str);
}

static void game_refresh(struct k_work *work)
{
	ball_pos.x += ball_vel.x;
	ball_pos.y += ball_vel.y;

	/* Ball went over to the other side */
	if (ball_pos.y < BALL_POS_Y_MIN) {
#if defined(SOLO)
		ball_pos.y = -ball_pos.y;
		ball_vel.y = -ball_vel.y;
#else
		k_sem_give(&disp_update);
		return;
#endif
	}

	/* Check for side-wall collision */
	if (ball_pos.x < BALL_POS_X_MIN) {
		ball_pos.x = -ball_pos.x;
		ball_vel.x = -ball_vel.x;
	} else if (ball_pos.x > BALL_POS_X_MAX) {
		ball_pos.x = (2 * BALL_POS_X_MAX) - ball_pos.x;
		ball_vel.x = -ball_vel.x;
	}

	/* Ball approaching paddle */
	if (ball_vel.y > 0 && ball_pos.y > BALL_POS_Y_MAX) {
		if (ball_pos.x < REAL_TO_VIRT(paddle_x) ||
		    ball_pos.x >= REAL_TO_VIRT(paddle_x + 2)) {
			game_ended(false);
			return;
		}

		ball_pos.y = (2 * BALL_POS_Y_MAX) - ball_pos.y;
		ball_vel.y = -ball_vel.y;
	}

	k_delayed_work_submit(&refresh, GAME_REFRESH);
	k_sem_give(&disp_update);
}

static void game_init(void)
{
	ended = 0;

	ball_pos = BALL_START;
	paddle_x = PADDLE_MIN;

	a_timestamp = 0;
	b_timestamp = 0;

	k_sem_give(&disp_update);
}

static void button_pressed(struct device *dev, struct gpio_callback *cb,
			   u32_t pins)
{
	if (ended && (k_uptime_get() - ended) > RESTART_THRESHOLD) {
		game_init();
		return;
	}

	if (pins & BIT(SW0_GPIO_PIN)) {
		printk("A pressed\n");

		if (!started) {
			a_timestamp = k_uptime_get();
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
		printk("B pressed\n");

		if (!started) {
			b_timestamp = k_uptime_get();
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

static void configure_buttons(void)
{
	static struct gpio_callback button_cb;
	struct device *gpio;

	gpio = device_get_binding(SW0_GPIO_NAME);

	gpio_pin_configure(gpio, SW0_GPIO_PIN,
			   (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			    GPIO_INT_ACTIVE_LOW));
	gpio_pin_configure(gpio, SW1_GPIO_PIN,
			   (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			    GPIO_INT_ACTIVE_LOW));
	gpio_init_callback(&button_cb, button_pressed,
			   BIT(SW0_GPIO_PIN) | BIT(SW1_GPIO_PIN));
	gpio_add_callback(gpio, &button_cb);

	gpio_pin_enable_callback(gpio, SW0_GPIO_PIN);
	gpio_pin_enable_callback(gpio, SW1_GPIO_PIN);
}

void main(void)
{
	struct mb_display *disp = mb_display_get();

	configure_buttons();

	k_delayed_work_init(&refresh, game_refresh);

	game_init();

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
				 K_FOREVER, &img, 1);
	}
}
