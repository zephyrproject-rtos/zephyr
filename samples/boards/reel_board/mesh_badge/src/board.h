/*
 * Copyright (c) 2018 Phytec Messtechnik GmbH
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

enum led_periph_device {
	DEV_IDX_LED0 = 0,
	DEV_IDX_LED1,
	DEV_IDX_LED2,
};

enum periph_device {
	DEV_IDX_BUTTON = 0,
	DEV_IDX_HDC1010,
	DEV_IDX_MMA8652,
	DEV_IDX_APDS9960,
	DEV_IDX_EPD,
	DEV_IDX_NUMOF,
};

void board_refresh_display(void);
void board_show_text(const char *text, bool center, s32_t duration);
void board_blink_leds(void);
void board_add_hello(u16_t addr, const char *name);
void board_add_heartbeat(u16_t addr, u8_t hops);
int get_hdc1010_val(struct sensor_value *val);
int get_mma8652_val(struct sensor_value *val);
int get_apds9960_val(struct sensor_value *val);
int set_led_state(u8_t id, bool state);
int periphs_init(void);
int board_init(void);
