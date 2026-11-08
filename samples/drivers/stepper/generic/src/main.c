/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/stepper/stepper.h>
#include <zephyr/drivers/stepper/stepper_ctrl.h>
#include <zephyr/init.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(stepper_generic, CONFIG_STEPPER_LOG_LEVEL);

/*
 * The stepper-driver alias is optional: some stepper controllers (e.g. h-bridge based ones)
 * drive the motor directly and do not have a separate hardware driver to enable/disable.
 */
#if DT_NODE_EXISTS(DT_ALIAS(stepper_driver))
#define HAS_STEPPER_DRIVER 1
#define STEPPER_DRIVER_MICRO_STEP_RES DT_PROP_OR(DT_ALIAS(stepper_driver), micro_step_res, 1)
static const struct device *stepper_driver = DEVICE_DT_GET(DT_ALIAS(stepper_driver));
#else
#define HAS_STEPPER_DRIVER 0
#define STEPPER_DRIVER_MICRO_STEP_RES 1
#endif
static const struct device *stepper_ctrl = DEVICE_DT_GET(DT_ALIAS(stepper_ctrl));

/*
 * Some boards (e.g. the TMC2241-EVKIT via the Sternbruecke Nucleo V0.1) need a
 * handful of static GPIOs driven once at boot for signals not covered by any
 * devicetree binding (SLEEP, UART_EN, IREF_R2/R3) - declared under zephyr,user.
 */
#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)
#if DT_NODE_HAS_PROP(ZEPHYR_USER_NODE, sleep_gpios)
#define HAS_STATIC_PINS 1
static const struct gpio_dt_spec sleep_gpio = GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, sleep_gpios);
static const struct gpio_dt_spec uart_en_gpio =
	GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, uart_en_gpios);
static const struct gpio_dt_spec iref_r2_gpio =
	GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, iref_r2_gpios);
static const struct gpio_dt_spec iref_r3_gpio =
	GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, iref_r3_gpios);

static void init_static_pins(void)
{
	gpio_pin_configure_dt(&sleep_gpio, GPIO_OUTPUT_ACTIVE);
	gpio_pin_configure_dt(&uart_en_gpio, GPIO_OUTPUT_INACTIVE);
	/* Raised to the next current step (IREF_R2 high) for a real StallGuard
	 * test - default (both low, 48kOhm) gives ~0.245A, well under the
	 * QSH2818-32-07-006's rated 0.67A/phase. Watch motor temperature.
	 */
	gpio_pin_configure_dt(&iref_r2_gpio, GPIO_OUTPUT_ACTIVE);
	gpio_pin_configure_dt(&iref_r3_gpio, GPIO_OUTPUT_INACTIVE);
}

/* Must run before CONFIG_STEPPER_INIT_PRIORITY (default 90) so SLEEP is driven
 * high before the stepper driver's own init tries to talk SPI to the chip -
 * calling this from main() ran too late, after all POST_KERNEL device inits.
 */
static int init_static_pins_sys_init(void)
{
	init_static_pins();
	return 0;
}
SYS_INIT(init_static_pins_sys_init, POST_KERNEL, 50);
#else
#define HAS_STATIC_PINS 0
#endif

enum stepper_mode {
	STEPPER_MODE_ENABLE,
	STEPPER_MODE_PING_PONG_RELATIVE,
	STEPPER_MODE_PING_PONG_ABSOLUTE,
	STEPPER_MODE_ROTATE_CW,
	STEPPER_MODE_ROTATE_CCW,
	STEPPER_MODE_STOP,
	STEPPER_MODE_DISABLE,
};

static atomic_t stepper_mode = ATOMIC_INIT(STEPPER_MODE_DISABLE);

static int32_t ping_pong_target_position = CONFIG_STEPS_PER_REV * CONFIG_PING_PONG_N_REV *
					   STEPPER_DRIVER_MICRO_STEP_RES;

static K_SEM_DEFINE(stepper_generic_sem, 0, 1);

static void stepper_callback(const struct device *dev, const enum stepper_ctrl_event event,
			     void *user_data)
{
	switch (event) {
	case STEPPER_CTRL_EVENT_STEPS_COMPLETED:
		k_sem_give(&stepper_generic_sem);
		break;
	default:
		break;
	}
}

#if HAS_STEPPER_DRIVER
static void stepper_driver_callback(const struct device *dev, const enum stepper_event event,
				    void *user_data)
{
	ARG_UNUSED(user_data);
	switch (event) {
	case STEPPER_EVENT_STALL_DETECTED:
		LOG_WRN("event: stall detected");
		break;
	case STEPPER_EVENT_FAULT_DETECTED:
		LOG_ERR("event: fault detected");
		break;
	default:
		break;
	}
}
#endif /* HAS_STEPPER_DRIVER */

static void button_pressed(struct input_event *event, void *user_data)
{
	ARG_UNUSED(user_data);

	if (event->value == 0 && event->type == INPUT_EV_KEY) {
		return;
	}
	enum stepper_mode mode = atomic_get(&stepper_mode);

	if (mode == STEPPER_MODE_DISABLE) {
		atomic_set(&stepper_mode, STEPPER_MODE_ENABLE);
	} else {
		atomic_inc(&stepper_mode);
	}
	k_sem_give(&stepper_generic_sem);
}

INPUT_CALLBACK_DEFINE(NULL, button_pressed, NULL);

int main(void)
{
	LOG_INF("Starting generic stepper sample");
	if (!device_is_ready(stepper_ctrl)) {
		LOG_ERR("Device %s is not ready", stepper_ctrl->name);
		return -ENODEV;
	}
#if HAS_STEPPER_DRIVER
	if (!device_is_ready(stepper_driver)) {
		LOG_ERR("Device %s is not ready", stepper_driver->name);
		return -ENODEV;
	}
	stepper_set_event_cb(stepper_driver, stepper_driver_callback, NULL);

	/* STS Software Items 2/3 (SRS-DRV-003/004): microstep resolution
	 * get/set round-trip, and rejection of a non-power-of-two value.
	 */
	enum stepper_micro_step_resolution ms_res;
	int ms_err;

	ms_err = stepper_set_micro_step_res(stepper_driver, STEPPER_MICRO_STEP_256);
	stepper_get_micro_step_res(stepper_driver, &ms_res);
	LOG_INF("Microstep resolution round-trip: set 256 (err %d), get %d", ms_err, ms_res);

	ms_err = stepper_set_micro_step_res(stepper_driver, (enum stepper_micro_step_resolution)3);
	LOG_INF("Invalid microstep resolution (3) rejected with err %d", ms_err);

	/* Restore the devicetree resolution after the round-trip test above */
	stepper_set_micro_step_res(
		stepper_driver, (enum stepper_micro_step_resolution)STEPPER_DRIVER_MICRO_STEP_RES);
#endif
	LOG_DBG("stepper is %p, name is %s", stepper_ctrl, stepper_ctrl->name);

	stepper_ctrl_set_event_cb(stepper_ctrl, stepper_callback, NULL);
	stepper_ctrl_set_reference_position(stepper_ctrl, 0);
	stepper_ctrl_set_microstep_interval(stepper_ctrl, CONFIG_STEP_INTERVAL_NS);

	for (;;) {
		k_sem_take(&stepper_generic_sem, K_FOREVER);
		switch (atomic_get(&stepper_mode)) {
		case STEPPER_MODE_ENABLE:
#if HAS_STEPPER_DRIVER
			stepper_enable(stepper_driver);
#endif
			LOG_INF("mode: enable");
			break;
		case STEPPER_MODE_PING_PONG_RELATIVE:
			ping_pong_target_position *= -1;
			stepper_ctrl_move_by(stepper_ctrl, ping_pong_target_position);
			LOG_INF("mode: ping pong relative");
			break;
		case STEPPER_MODE_PING_PONG_ABSOLUTE:
			ping_pong_target_position *= -1;
			stepper_ctrl_move_to(stepper_ctrl, ping_pong_target_position);
			LOG_INF("mode: ping pong absolute");
			break;
		case STEPPER_MODE_ROTATE_CW:
			stepper_ctrl_run(stepper_ctrl, STEPPER_CTRL_DIRECTION_POSITIVE);
			LOG_INF("mode: rotate cw");
			break;
		case STEPPER_MODE_ROTATE_CCW:
			stepper_ctrl_run(stepper_ctrl, STEPPER_CTRL_DIRECTION_NEGATIVE);
			LOG_INF("mode: rotate ccw");
			break;
		case STEPPER_MODE_STOP:
			stepper_ctrl_stop(stepper_ctrl);
			LOG_INF("mode: stop");
			break;
		case STEPPER_MODE_DISABLE:
#if HAS_STEPPER_DRIVER
			stepper_disable(stepper_driver);
#endif
			LOG_INF("mode: disable");
			break;
		default:
			break;
		}
	}
	return 0;
}

static void monitor_thread(void)
{
	for (;;) {
		int32_t actual_position;

		stepper_ctrl_get_actual_position(stepper_ctrl, &actual_position);
		LOG_DBG("Actual position: %d", actual_position);
		k_sleep(K_MSEC(CONFIG_MONITOR_THREAD_TIMEOUT_MS));
	}
}

K_THREAD_DEFINE(monitor_tid, CONFIG_MONITOR_THREAD_STACK_SIZE, monitor_thread, NULL, NULL, NULL, 5,
		0, 0);
