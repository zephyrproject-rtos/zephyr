/*
 * Copyright (c) 2022 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/smf.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/shell/shell.h>

#include "controls.h"
#include "meas.h"
#include "view.h"
#include "model.h"

LOG_MODULE_REGISTER(main);

#define DEBUG 1

/* The devicetree node identifier for the led aliases. */
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)

/* various flags for the current state of the snooper */
#define FLAGS_SNOOP0		0
#define FLAGS_SNOOP1		1
#define FLAGS_SNOOP2		2
#define FLAGS_SNOOP3		3
#define FLAGS_NO_CONNECTION	4
#define FLAGS_CC1_CONNECTION	5
#define FLAGS_CC2_CONNECTION	6

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);

static const struct smf_state view_states[];

enum led_t {LED_OFF, LED_RED, LED_GREEN, LED_BLUE};
enum view_state_t {SNOOP0, SNOOP1, SNOOP2, SNOOP3};

struct view_obj_t {
	struct smf_ctx ctx;
	k_timeout_t timeout;
	bool toggle;
	atomic_t flags;
} view_obj;

snooper_mask_t get_view_snoop() {
	if (atomic_test_bit(&view_obj.flags, FLAGS_SNOOP0)) return 0;
	if (atomic_test_bit(&view_obj.flags, FLAGS_SNOOP1)) return CC1_CHANNEL_BIT;
	if (atomic_test_bit(&view_obj.flags, FLAGS_SNOOP2)) return CC2_CHANNEL_BIT;
	if (atomic_test_bit(&view_obj.flags, FLAGS_SNOOP3)) return (CC1_CHANNEL_BIT | CC2_CHANNEL_BIT);
	return 0;
}

int view_set_connection(snooper_mask_t vc)
{
	switch (vc) {
	case 0:
		atomic_clear_bit(&view_obj.flags, FLAGS_CC1_CONNECTION);
		atomic_clear_bit(&view_obj.flags, FLAGS_CC2_CONNECTION);
		atomic_set_bit(&view_obj.flags, FLAGS_NO_CONNECTION);
		break;
	case CC1_CHANNEL_BIT:
		atomic_clear_bit(&view_obj.flags, FLAGS_NO_CONNECTION);
		atomic_clear_bit(&view_obj.flags, FLAGS_CC2_CONNECTION);
		atomic_set_bit(&view_obj.flags, FLAGS_CC1_CONNECTION);
		break;
	case CC2_CHANNEL_BIT:
		atomic_clear_bit(&view_obj.flags, FLAGS_NO_CONNECTION);
		atomic_clear_bit(&view_obj.flags, FLAGS_CC1_CONNECTION);
		atomic_set_bit(&view_obj.flags, FLAGS_CC2_CONNECTION);
		break;
	default:
	 	return -EIO;
	}
	return 0;
}

int view_set_snoop(snooper_mask_t vs)
{
	switch (vs) {
	case 0:
		atomic_clear_bit(&view_obj.flags, FLAGS_SNOOP1);
		atomic_clear_bit(&view_obj.flags, FLAGS_SNOOP2);
		atomic_clear_bit(&view_obj.flags, FLAGS_SNOOP3);
		atomic_set_bit(&view_obj.flags, FLAGS_SNOOP0);
		break;
	case CC1_CHANNEL_BIT:
		atomic_clear_bit(&view_obj.flags, FLAGS_SNOOP0);
		atomic_clear_bit(&view_obj.flags, FLAGS_SNOOP2);
		atomic_clear_bit(&view_obj.flags, FLAGS_SNOOP3);
		atomic_set_bit(&view_obj.flags, FLAGS_SNOOP1);
		break;
	case CC2_CHANNEL_BIT:
		atomic_clear_bit(&view_obj.flags, FLAGS_SNOOP0);
		atomic_clear_bit(&view_obj.flags, FLAGS_SNOOP1);
		atomic_clear_bit(&view_obj.flags, FLAGS_SNOOP3);
		atomic_set_bit(&view_obj.flags, FLAGS_SNOOP2);
		break;
	case (CC1_CHANNEL_BIT | CC2_CHANNEL_BIT):
		atomic_clear_bit(&view_obj.flags, FLAGS_SNOOP0);
		atomic_clear_bit(&view_obj.flags, FLAGS_SNOOP1);
		atomic_clear_bit(&view_obj.flags, FLAGS_SNOOP2);
		atomic_set_bit(&view_obj.flags, FLAGS_SNOOP3);
		break;
	default:
	 	return -EIO;
	}

	view_set_connection(0);
	return 0;
}

static void set_led(enum led_t led)
{
	switch (led) {
	case LED_OFF:
		gpio_pin_set_dt(&led0, 0);
		gpio_pin_set_dt(&led1, 0);
		gpio_pin_set_dt(&led2, 0);
		break;
	case LED_RED:
		gpio_pin_set_dt(&led0, 1);
		gpio_pin_set_dt(&led1, 0);
		gpio_pin_set_dt(&led2, 0);
		break;
	case LED_GREEN:
		gpio_pin_set_dt(&led0, 0);
		gpio_pin_set_dt(&led1, 1);
		gpio_pin_set_dt(&led2, 0);
		break;
	case LED_BLUE:
		gpio_pin_set_dt(&led0, 0);
		gpio_pin_set_dt(&led1, 0);
		gpio_pin_set_dt(&led2, 1);
		break;
	}
}

static void snoop0_entry(void *o)
{
	struct view_obj_t *vo = (struct view_obj_t *)o;

	set_led(LED_OFF);
	vo->timeout = K_MSEC(1000);
}

static void snoop0_run(void *o)
{
	struct view_obj_t *vo = (struct view_obj_t *)o;

	if (atomic_test_bit(&vo->flags, FLAGS_SNOOP1)) {
		smf_set_state(SMF_CTX(vo), &view_states[SNOOP1]);
	} else if (atomic_test_bit(&vo->flags, FLAGS_SNOOP2)) {
		smf_set_state(SMF_CTX(vo), &view_states[SNOOP2]);
	} else if (atomic_test_bit(&vo->flags, FLAGS_SNOOP3)) {
		smf_set_state(SMF_CTX(vo), &view_states[SNOOP3]);
	}
}

static void snoop0_exit(void *o)
{
}

static void snoop1_entry(void *o)
{
	struct view_obj_t *vo = (struct view_obj_t *)o;

	vo->timeout = K_MSEC(1000);
}

static void snoop1_run(void *o)
{
	struct view_obj_t *vo = (struct view_obj_t *)o;

	if (atomic_test_bit(&vo->flags, FLAGS_SNOOP0)) {
		smf_set_state(SMF_CTX(vo), &view_states[SNOOP0]);
	} else if (atomic_test_bit(&vo->flags, FLAGS_SNOOP2)) {
		smf_set_state(SMF_CTX(vo), &view_states[SNOOP2]);
	} else if (atomic_test_bit(&vo->flags, FLAGS_SNOOP3)) {
		smf_set_state(SMF_CTX(vo), &view_states[SNOOP3]);
	} else if (atomic_test_bit(&vo->flags, FLAGS_NO_CONNECTION)) {
		if (vo->toggle) {
			set_led(LED_BLUE);
			vo->toggle = false;
		} else {
			set_led(LED_OFF);
			vo->toggle = true;
		}
	} else if (atomic_test_bit(&vo->flags, FLAGS_CC1_CONNECTION)) {
		set_led(LED_GREEN);
	} else if (atomic_test_bit(&vo->flags, FLAGS_CC2_CONNECTION)) {
		set_led(LED_RED);
	}
}

static void snoop1_exit(void *o)
{
	set_led(LED_OFF);
}

static void snoop2_entry(void *o)
{
	struct view_obj_t *vo = (struct view_obj_t *)o;

	vo->timeout = K_MSEC(500);
}

static void snoop2_run(void *o)
{
	struct view_obj_t *vo = (struct view_obj_t *)o;

	if (atomic_test_bit(&vo->flags, FLAGS_SNOOP0)) {
		smf_set_state(SMF_CTX(vo), &view_states[SNOOP0]);
	} else if (atomic_test_bit(&vo->flags, FLAGS_SNOOP1)) {
		smf_set_state(SMF_CTX(vo), &view_states[SNOOP1]);
	} else if (atomic_test_bit(&vo->flags, FLAGS_SNOOP3)) {
		smf_set_state(SMF_CTX(vo), &view_states[SNOOP3]);
	} else if (atomic_test_bit(&vo->flags, FLAGS_NO_CONNECTION)) {
		if (vo->toggle) {
			set_led(LED_BLUE);
			vo->toggle = false;
		} else {
			set_led(LED_OFF);
			vo->toggle = true;
		}
	} else if (atomic_test_bit(&vo->flags, FLAGS_CC1_CONNECTION)) {
		set_led(LED_RED);
	} else if (atomic_test_bit(&vo->flags, FLAGS_CC2_CONNECTION)) {
		set_led(LED_GREEN);
	}
}

static void snoop2_exit(void *o)
{
	set_led(LED_OFF);
}

static void snoop3_entry(void *o)
{
	struct view_obj_t *vo = (struct view_obj_t *)o;

	vo->timeout = K_MSEC(250);
}

static void snoop3_run(void *o)
{
	struct view_obj_t *vo = (struct view_obj_t *)o;

	if (atomic_test_bit(&vo->flags, FLAGS_SNOOP0)) {
		smf_set_state(SMF_CTX(vo), &view_states[SNOOP0]);
	} else if (atomic_test_bit(&vo->flags, FLAGS_SNOOP1)) {
		smf_set_state(SMF_CTX(vo), &view_states[SNOOP1]);
	} else if (atomic_test_bit(&vo->flags, FLAGS_SNOOP2)) {
		smf_set_state(SMF_CTX(vo), &view_states[SNOOP2]);
	} else if (atomic_test_bit(&vo->flags, FLAGS_NO_CONNECTION)) {
		if (vo->toggle) {
			set_led(LED_BLUE);
			vo->toggle = false;
		} else {
			set_led(LED_OFF);
			vo->toggle = true;
		}
	} else if (atomic_test_bit(&vo->flags, FLAGS_CC1_CONNECTION)) {
		set_led(LED_GREEN);
	} else if (atomic_test_bit(&vo->flags, FLAGS_CC2_CONNECTION)) {
		set_led(LED_GREEN);
	}
}

static void snoop3_exit(void *o)
{
	set_led(LED_OFF);
}

static const struct smf_state view_states[] = {
	[SNOOP0] = SMF_CREATE_STATE(snoop0_entry, snoop0_run, snoop0_exit),
	[SNOOP1] = SMF_CREATE_STATE(snoop1_entry, snoop1_run, snoop1_exit),
	[SNOOP2] = SMF_CREATE_STATE(snoop2_entry, snoop2_run, snoop2_exit),
	[SNOOP3] = SMF_CREATE_STATE(snoop3_entry, snoop3_run, snoop3_exit),
};

int view_init(void)
{
	int ret;

	if (!device_is_ready(led0.port)) {
		return -EIO;
	}

	if (!device_is_ready(led1.port)) {
		return -EIO;

	}

	if (!device_is_ready(led2.port)) {
		return -EIO;
	}

	ret = gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&led2, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	view_obj.flags = ATOMIC_INIT(0);

        return 0;
}

void main(void)
{
	const struct device *dev_pd_analyzer = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	if (usb_enable(NULL)) {
		return;
	}

	meas_init();
	controls_init();
	model_init(dev_pd_analyzer);
	view_init();

	smf_set_initial(SMF_CTX(&view_obj), &view_states[SNOOP0]);

        while(1) {
                smf_run_state(SMF_CTX(&view_obj));
                k_sleep(view_obj.timeout);
        }
}
