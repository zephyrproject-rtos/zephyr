/* microbit.c - BBC micro:bit specific hooks */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gpio.h>
#include <board.h>
#include <soc.h>
#include <misc/printk.h>
#include <ctype.h>
#include <flash.h>
#include <gpio.h>
#include <pwm.h>

#include <display/mb_display.h>

#include <bluetooth/mesh.h>

#include "board.h"

#define SCROLL_SPEED   K_MSEC(300)

#define BUZZER_PIN     EXT_P0_GPIO_PIN
#define BEEP_DURATION  K_MSEC(60)

#define SEQ_PER_BIT  976
#define SEQ_PAGE     (NRF_FICR->CODEPAGESIZE * (NRF_FICR->CODESIZE - 1))
#define SEQ_MAX      (NRF_FICR->CODEPAGESIZE * 8 * SEQ_PER_BIT)

static struct device *gpio;
static struct device *nvm;
static struct device *pwm;

static struct k_work button_work;

void board_seq_update(u32_t seq)
{
	u32_t loc, seq_map;
	int err;

	if (seq % SEQ_PER_BIT) {
		return;
	}

	loc = (SEQ_PAGE + ((seq / SEQ_PER_BIT) / 32));

	err = flash_read(nvm, loc, &seq_map, sizeof(seq_map));
	if (err) {
		printk("flash_read err %d\n", err);
		return;
	}

	seq_map >>= 1;

	flash_write_protection_set(nvm, false);
	err = flash_write(nvm, loc, &seq_map, sizeof(seq_map));
	flash_write_protection_set(nvm, true);
	if (err) {
		printk("flash_write err %d\n", err);
	}
}

static u32_t get_seq(void)
{
	u32_t seq_map, seq = 0;
	int err, i;

	for (i = 0; i < NRF_FICR->CODEPAGESIZE / sizeof(seq_map); i++) {
		err = flash_read(nvm, SEQ_PAGE + (i * sizeof(seq_map)),
				 &seq_map, sizeof(seq_map));
		if (err) {
			printk("flash_read err %d\n", err);
			return seq;
		}

		printk("seq_map 0x%08x\n", seq_map);

		if (seq_map) {
			seq = ((i * 32) +
			       (32 - popcount(seq_map))) * SEQ_PER_BIT;
			if (!seq) {
				return 0;
			}

			break;
		}
	}

	seq += SEQ_PER_BIT;
	if (seq >= SEQ_MAX) {
		seq = 0;
	}

	if (seq) {
		seq_map >>= 1;
		flash_write_protection_set(nvm, false);
		err = flash_write(nvm, SEQ_PAGE + (i * sizeof(seq_map)),
				  &seq_map, sizeof(seq_map));
		flash_write_protection_set(nvm, true);
		if (err) {
			printk("flash_write err %d\n", err);
		}
	} else {
		printk("Performing flash erase of page 0x%08x\n", SEQ_PAGE);
		err = flash_erase(nvm, SEQ_PAGE, NRF_FICR->CODEPAGESIZE);
		if (err) {
			printk("flash_erase err %d\n", err);
		}
	}

	return seq;
}

static void button_send_pressed(struct k_work *work)
{
	printk("button_send_pressed()\n");
	board_button_1_pressed();
}

static void button_pressed(struct device *dev, struct gpio_callback *cb,
			   uint32_t pins)
{
	struct mb_display *disp = mb_display_get();

	if (pins & BIT(SW0_GPIO_PIN)) {
		k_work_submit(&button_work);
	} else {
		u16_t target = board_set_target();

		if (target > 0x0009) {
			mb_display_print(disp, MB_DISPLAY_MODE_SINGLE,
					 K_SECONDS(2), "A");
		} else {
			mb_display_print(disp, MB_DISPLAY_MODE_SINGLE,
					 K_SECONDS(2), "%X", (target & 0xf));
		}
	}
}

static const struct {
	char  note;
	u32_t period;
	u32_t sharp;
} period_map[] = {
	{ 'C',  3822,  3608 },
	{ 'D',  3405,  3214 },
	{ 'E',  3034,  3034 },
	{ 'F',  2863,  2703 },
	{ 'G',  2551,  2407 },
	{ 'A',  2273,  2145 },
	{ 'B',  2025,  2025 },
};

static u32_t get_period(char note, bool sharp)
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
		u32_t period, duration = 0;

		while (*str && !isdigit(*str)) {
			str++;
		}

		while (isdigit(*str)) {
			duration *= 10;
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
			pwm_pin_set_usec(pwm, BUZZER_PIN, period, period / 2);
		}

		k_sleep(duration);

		/* Disable the PWM */
		pwm_pin_set_usec(pwm, BUZZER_PIN, 0, 0);
	}
}

void board_heartbeat(u8_t hops, u16_t feat)
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
		hops = min(hops, ARRAY_SIZE(hops_img));
		mb_display_image(disp, MB_DISPLAY_MODE_SINGLE, K_SECONDS(2),
				 &hops_img[hops - 1], 1);
	}
}

void board_other_dev_pressed(u16_t addr)
{
	struct mb_display *disp = mb_display_get();

	printk("board_other_dev_pressed(0x%04x)\n", addr);

	mb_display_print(disp, MB_DISPLAY_MODE_SINGLE, K_SECONDS(2),
			 "%X", (addr & 0xf));
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
				 K_MSEC(150), attn_img, ARRAY_SIZE(attn_img));
	} else {
		mb_display_stop(disp);
	}
}

static void configure_button(void)
{
	static struct gpio_callback button_cb;

	k_work_init(&button_work, button_send_pressed);

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

void board_init(u16_t *addr, u32_t *seq)
{
	struct mb_display *disp = mb_display_get();

	printk("SEQ_PAGE 0x%08x\n", SEQ_PAGE);

	nvm = device_get_binding(CONFIG_SOC_FLASH_NRF5_DEV_NAME);
	pwm = device_get_binding(CONFIG_PWM_NRF5_SW_0_DEV_NAME);

	*addr = NRF_UICR->CUSTOMER[0];
	if (!*addr || *addr == 0xffff) {
#if defined(NODE_ADDR)
		*addr = NODE_ADDR;
#else
		*addr = 0x0b0c;
#endif
	}

	*seq = get_seq();

	mb_display_print(disp, MB_DISPLAY_MODE_DEFAULT, SCROLL_SPEED,
			 "0x%04x", *addr);

	configure_button();
}
