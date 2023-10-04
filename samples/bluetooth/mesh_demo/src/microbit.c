/* microbit.c - BBC micro:bit specific hooks */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <soc.h>
#include <zephyr/sys/printk.h>
#include <ctype.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>

#include <zephyr/display/mb_display.h>

#include <zephyr/bluetooth/mesh.h>

#include "board.h"

#define SCROLL_SPEED   300

#define BUZZER_PWM_CHANNEL 0
#define BEEP_DURATION  K_MSEC(60)

#define SEQ_PER_BIT  976
#define SEQ_PAGE     (NRF_FICR->CODEPAGESIZE * (NRF_FICR->CODESIZE - 1))
#define SEQ_MAX      (NRF_FICR->CODEPAGESIZE * 8 * SEQ_PER_BIT)

static const struct gpio_dt_spec button_a =
	GPIO_DT_SPEC_GET(DT_NODELABEL(buttona), gpios);
static const struct gpio_dt_spec button_b =
	GPIO_DT_SPEC_GET(DT_NODELABEL(buttonb), gpios);
static const struct device *const nvm =
	DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));
static const struct device *const pwm =
	DEVICE_DT_GET_ANY(nordic_nrf_sw_pwm);

static struct k_work button_work;

static void button_send_pressed(struct k_work *work)
{
	printk("button_send_pressed()\n");
	board_button_1_pressed();
}

static void button_pressed(const struct device *dev, struct gpio_callback *cb,
			   uint32_t pins)
{
	struct mb_display *disp = mb_display_get();

	if (pins & BIT(button_a.pin)) {
		k_work_submit(&button_work);
	} else {
		uint16_t target = board_set_target();

		if (target > 0x0009) {
			mb_display_print(disp, MB_DISPLAY_MODE_SINGLE,
					 2 * MSEC_PER_SEC, "A");
		} else {
			mb_display_print(disp, MB_DISPLAY_MODE_SINGLE,
					 2 * MSEC_PER_SEC, "%X", (target & 0xf));
		}
	}
}

static const struct {
	char  note;
	uint32_t period;
	uint32_t sharp;
} period_map[] = {
	{ 'C',  3822,  3608 },
	{ 'D',  3405,  3214 },
	{ 'E',  3034,  3034 },
	{ 'F',  2863,  2703 },
	{ 'G',  2551,  2407 },
	{ 'A',  2273,  2145 },
	{ 'B',  2025,  2025 },
};

static uint32_t get_period(char note, bool sharp)
{
	int i;

	if (note == ' ') {
		return 0;
	}

	for (i = 0; i < ARRAY_SIZE(period_map); i++) {
		if (period_map[i].note != note) {
			continue;
		}

		if (sharp) {
			return period_map[i].sharp;
		} else {
			return period_map[i].period;
		}
	}

	return 1500;
}

void board_play_tune(const char *str)
{
	while (*str) {
		uint32_t period, duration = 0U;

		while (*str && isdigit((unsigned char)*str) == 0) {
			str++;
		}

		while (isdigit((unsigned char)*str) != 0) {
			duration *= 10U;
			duration += *str - '0';
			str++;
		}

		if (!*str) {
			break;
		}

		if (str[1] == '#') {
			period = get_period(*str, true);
			str += 2;
		} else {
			period = get_period(*str, false);
			str++;
		}

		if (period) {
			pwm_set(pwm, BUZZER_PWM_CHANNEL, PWM_USEC(period),
				PWM_USEC(period) / 2U, 0);
		}

		k_sleep(K_MSEC(duration));

		/* Disable the PWM */
		pwm_set(pwm, BUZZER_PWM_CHANNEL, 0, 0, 0);
	}
}

void board_heartbeat(uint8_t hops, uint16_t feat)
{
	struct mb_display *disp = mb_display_get();
	const struct mb_image hops_img[] = {
		MB_IMAGE({ 1, 1, 1, 1, 1 },
			 { 1, 1, 1, 1, 1 },
			 { 1, 1, 1, 1, 1 },
			 { 1, 1, 1, 1, 1 },
			 { 1, 1, 1, 1, 1 }),
		MB_IMAGE({ 1, 1, 1, 1, 1 },
			 { 1, 1, 1, 1, 1 },
			 { 1, 1, 0, 1, 1 },
			 { 1, 1, 1, 1, 1 },
			 { 1, 1, 1, 1, 1 }),
		MB_IMAGE({ 1, 1, 1, 1, 1 },
			 { 1, 0, 0, 0, 1 },
			 { 1, 0, 0, 0, 1 },
			 { 1, 0, 0, 0, 1 },
			 { 1, 1, 1, 1, 1 }),
		MB_IMAGE({ 1, 1, 1, 1, 1 },
			 { 1, 0, 0, 0, 1 },
			 { 1, 0, 0, 0, 1 },
			 { 1, 0, 0, 0, 1 },
			 { 1, 1, 1, 1, 1 }),
		MB_IMAGE({ 1, 0, 1, 0, 1 },
			 { 0, 0, 0, 0, 0 },
			 { 1, 0, 0, 0, 1 },
			 { 0, 0, 0, 0, 0 },
			 { 1, 0, 1, 0, 1 })
	};

	printk("%u hops\n", hops);

	if (hops) {
		hops = MIN(hops, ARRAY_SIZE(hops_img));
		mb_display_image(disp, MB_DISPLAY_MODE_SINGLE, 2 * MSEC_PER_SEC,
				 &hops_img[hops - 1], 1);
	}
}

void board_other_dev_pressed(uint16_t addr)
{
	struct mb_display *disp = mb_display_get();

	printk("board_other_dev_pressed(0x%04x)\n", addr);

	mb_display_print(disp, MB_DISPLAY_MODE_SINGLE, 2 * MSEC_PER_SEC, "%X",
			 (addr & 0xf));
}

void board_attention(bool attention)
{
	struct mb_display *disp = mb_display_get();
	static const struct mb_image attn_img[] = {
		MB_IMAGE({ 0, 0, 0, 0, 0 },
			 { 0, 0, 0, 0, 0 },
			 { 0, 0, 1, 0, 0 },
			 { 0, 0, 0, 0, 0 },
			 { 0, 0, 0, 0, 0 }),
		MB_IMAGE({ 0, 0, 0, 0, 0 },
			 { 0, 1, 1, 1, 0 },
			 { 0, 1, 1, 1, 0 },
			 { 0, 1, 1, 1, 0 },
			 { 0, 0, 0, 0, 0 }),
		MB_IMAGE({ 1, 1, 1, 1, 1 },
			 { 1, 1, 1, 1, 1 },
			 { 1, 1, 0, 1, 1 },
			 { 1, 1, 1, 1, 1 },
			 { 1, 1, 1, 1, 1 }),
		MB_IMAGE({ 1, 1, 1, 1, 1 },
			 { 1, 0, 0, 0, 1 },
			 { 1, 0, 0, 0, 1 },
			 { 1, 0, 0, 0, 1 },
			 { 1, 1, 1, 1, 1 }),
	};

	if (attention) {
		mb_display_image(disp,
				 MB_DISPLAY_MODE_DEFAULT | MB_DISPLAY_FLAG_LOOP,
				 150, attn_img, ARRAY_SIZE(attn_img));
	} else {
		mb_display_stop(disp);
	}
}

static int configure_button(const struct gpio_dt_spec *button)
{
	int err;

	err = gpio_pin_configure_dt(button, GPIO_INPUT);
	if (err) {
		return err;
	}
	return gpio_pin_interrupt_configure_dt(button, GPIO_INT_EDGE_TO_ACTIVE);
}

static int configure_buttons(void)
{
	static struct gpio_callback button_cb;
	int err;

	k_work_init(&button_work, button_send_pressed);

	err = configure_button(&button_a);
	if (err) {
		return err;
	}

	err = configure_button(&button_b);
	if (err) {
		return err;
	}

	if (button_a.port != button_b.port) {
		/* These should be the same device on this board. */
		return -EINVAL;
	}

	gpio_init_callback(&button_cb, button_pressed,
			   BIT(button_a.pin) | BIT(button_b.pin));
	return gpio_add_callback(button_a.port, &button_cb);
}

int board_init(uint16_t *addr)
{
	struct mb_display *disp = mb_display_get();

	if (!(device_is_ready(nvm) && device_is_ready(pwm) &&
	      device_is_ready(button_a.port) &&
	      device_is_ready(button_b.port))) {
		printk("One or more devices are not ready\n");
		return -ENODEV;
	}

	*addr = NRF_UICR->CUSTOMER[0];
	if (!*addr || *addr == 0xffff) {
#if defined(NODE_ADDR)
		*addr = NODE_ADDR;
#else
		*addr = 0x0b0c;
#endif
	}

	mb_display_print(disp, MB_DISPLAY_MODE_DEFAULT, SCROLL_SPEED,
			 "0x%04x", *addr);

	return configure_buttons();
}
