/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Show the OpenThread device role on the board RGB LED:
 * disabled = white, detached = red, child = yellow,
 * router = green, leader = blue.
 */

#include <stdlib.h>

#include <zephyr/drivers/pwm.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

#include <openthread.h>
#include <openthread/instance.h>
#include <openthread/thread.h>

#define RGB_THREAD_STACK_SIZE 768
#define RGB_THREAD_PRIORITY   K_LOWEST_APPLICATION_THREAD_PRIO

struct rgb_color {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

static const struct pwm_dt_spec red_led = PWM_DT_SPEC_GET(DT_ALIAS(red_pwm_led));
static const struct pwm_dt_spec green_led = PWM_DT_SPEC_GET(DT_ALIAS(green_pwm_led));
static const struct pwm_dt_spec blue_led = PWM_DT_SPEC_GET(DT_ALIAS(blue_pwm_led));

static atomic_t blink_enabled = ATOMIC_INIT(1);
static atomic_t follow_role = ATOMIC_INIT(1);
static atomic_t blink_ms = ATOMIC_INIT(CONFIG_OT_SHELL_SAMPLE_RGB_STATUS_LED_BLINK_MS);
static atomic_t color; /* packed 0x00RRGGBB */

static void color_store(struct rgb_color c)
{
	atomic_set(&color, ((atomic_val_t)c.r << 16) | ((atomic_val_t)c.g << 8) | c.b);
}

static struct rgb_color color_load(void)
{
	atomic_val_t v = atomic_get(&color);

	return (struct rgb_color){
		.r = (v >> 16) & 0xff,
		.g = (v >> 8) & 0xff,
		.b = v & 0xff,
	};
}

static struct rgb_color role_to_color(otDeviceRole role)
{
	switch (role) {
	case OT_DEVICE_ROLE_DISABLED:
		return (struct rgb_color){255, 255, 255};
	case OT_DEVICE_ROLE_DETACHED:
		return (struct rgb_color){255, 0, 0};
	case OT_DEVICE_ROLE_CHILD:
		return (struct rgb_color){255, 255, 0};
	case OT_DEVICE_ROLE_ROUTER:
		return (struct rgb_color){0, 255, 0};
	case OT_DEVICE_ROLE_LEADER:
		return (struct rgb_color){0, 0, 255};
	default:
		return (struct rgb_color){0, 0, 0};
	}
}

static void led_set(const struct pwm_dt_spec *led, uint8_t level)
{
	uint32_t pulse = (uint32_t)(((uint64_t)led->period * level) / UINT8_MAX);

	(void)pwm_set_pulse_dt(led, pulse);
}

static void rgb_set(struct rgb_color c)
{
	led_set(&red_led, c.r);
	led_set(&green_led, c.g);
	led_set(&blue_led, c.b);
}

/* Must be called from outside the OpenThread context. */
static void color_from_current_role(void)
{
	otDeviceRole role;

	openthread_mutex_lock();
	role = otThreadGetDeviceRole(openthread_get_default_instance());
	openthread_mutex_unlock();

	color_store(role_to_color(role));
}

/* Runs in the OpenThread context with the OpenThread mutex held. */
static void ot_state_changed(otChangedFlags flags, void *user_data)
{
	ARG_UNUSED(user_data);

	if ((flags & OT_CHANGED_THREAD_ROLE) && atomic_get(&follow_role)) {
		color_store(role_to_color(
			otThreadGetDeviceRole(openthread_get_default_instance())));
	}
}

static struct openthread_state_changed_callback ot_state_cb = {
	.otCallback = ot_state_changed,
};

static void rgb_thread(void *p1, void *p2, void *p3)
{
	bool on = false;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	color_from_current_role();

	while (true) {
		on = atomic_get(&blink_enabled) ? !on : false;
		rgb_set(on ? color_load() : (struct rgb_color){0, 0, 0});
		k_msleep((int32_t)atomic_get(&blink_ms));
	}
}

K_THREAD_STACK_DEFINE(rgb_stack, RGB_THREAD_STACK_SIZE);
static struct k_thread rgb_thread_data;

static int rgb_status_led_init(void)
{
	if (!pwm_is_ready_dt(&red_led) || !pwm_is_ready_dt(&green_led) ||
	    !pwm_is_ready_dt(&blue_led)) {
		return -ENODEV;
	}

	color_store(role_to_color(OT_DEVICE_ROLE_DISABLED));
	(void)openthread_state_changed_callback_register(&ot_state_cb);

	k_thread_create(&rgb_thread_data, rgb_stack, K_THREAD_STACK_SIZEOF(rgb_stack),
			rgb_thread, NULL, NULL, NULL, RGB_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&rgb_thread_data, "rgb_status_led");

	return 0;
}

SYS_INIT(rgb_status_led_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#if defined(CONFIG_SHELL)
static int parse_u8(const struct shell *sh, const char *arg, uint8_t *out)
{
	char *end;
	unsigned long v = strtoul(arg, &end, 10);

	if (*end != '\0' || v > UINT8_MAX) {
		shell_error(sh, "invalid value: %s (0-255)", arg);
		return -EINVAL;
	}

	*out = (uint8_t)v;
	return 0;
}

static int cmd_rgb_on(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	atomic_set(&blink_enabled, 1);
	shell_print(sh, "RGB LED on");
	return 0;
}

static int cmd_rgb_off(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	atomic_set(&blink_enabled, 0);
	shell_print(sh, "RGB LED off");
	return 0;
}

static int cmd_rgb_rate(const struct shell *sh, size_t argc, char **argv)
{
	char *end;
	unsigned long ms = strtoul(argv[1], &end, 10);

	ARG_UNUSED(argc);

	if (*end != '\0' || ms < 10 || ms > 10000) {
		shell_error(sh, "invalid period: %s (10-10000 ms)", argv[1]);
		return -EINVAL;
	}

	atomic_set(&blink_ms, (atomic_val_t)ms);
	shell_print(sh, "Blink period %lu ms", ms);
	return 0;
}

static int cmd_rgb_color(const struct shell *sh, size_t argc, char **argv)
{
	struct rgb_color c;

	ARG_UNUSED(argc);

	if (parse_u8(sh, argv[1], &c.r) || parse_u8(sh, argv[2], &c.g) ||
	    parse_u8(sh, argv[3], &c.b)) {
		return -EINVAL;
	}

	atomic_set(&follow_role, 0);
	color_store(c);
	shell_print(sh, "Colour %u %u %u (role tracking off)", c.r, c.g, c.b);
	return 0;
}

static int cmd_rgb_role(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	atomic_set(&follow_role, 1);
	color_from_current_role();
	shell_print(sh, "Colour follows the Thread device role");
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(rgb_cmds,
	SHELL_CMD_ARG(on, NULL, "Enable blinking", cmd_rgb_on, 1, 0),
	SHELL_CMD_ARG(off, NULL, "Disable blinking", cmd_rgb_off, 1, 0),
	SHELL_CMD_ARG(rate, NULL, "Set blink period: rate <ms>", cmd_rgb_rate, 2, 0),
	SHELL_CMD_ARG(color, NULL, "Set a fixed colour: color <r> <g> <b>",
		      cmd_rgb_color, 4, 0),
	SHELL_CMD_ARG(role, NULL, "Follow the Thread device role", cmd_rgb_role, 1, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(rgb, &rgb_cmds, "RGB status LED control", NULL);
#endif /* CONFIG_SHELL */
