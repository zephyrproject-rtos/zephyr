/*
 * Copyright (c) 2022 Valerio Setti <valerio.setti@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>

#define QUAD_ENC_EMUL_ENABLED \
	DT_NODE_EXISTS(DT_ALIAS(qenca)) && DT_NODE_EXISTS(DT_ALIAS(qencb))

#if QUAD_ENC_EMUL_ENABLED

#include <zephyr/drivers/gpio.h>

#define QUAD_ENC_EMUL_PERIOD 100

static const struct gpio_dt_spec phase_a =
			GPIO_DT_SPEC_GET(DT_ALIAS(qenca), gpios);
static const struct gpio_dt_spec phase_b =
			GPIO_DT_SPEC_GET(DT_ALIAS(qencb), gpios);
static bool toggle_a;

void qenc_emulate_work_handler(struct k_work *work)
{
	toggle_a = !toggle_a;
	if (toggle_a) {
		gpio_pin_toggle_dt(&phase_a);
	} else {
		gpio_pin_toggle_dt(&phase_b);
	}
}

static K_WORK_DEFINE(qenc_emulate_work, qenc_emulate_work_handler);

static void qenc_emulate_timer_handler(struct k_timer *dummy)
{
	k_work_submit(&qenc_emulate_work);
}

static K_TIMER_DEFINE(qenc_emulate_timer, qenc_emulate_timer_handler, NULL);

static void qenc_emulate_init(void)
{
	printk("Quadrature encoder emulator enabled with %u ms period\n",
		QUAD_ENC_EMUL_PERIOD);

	if (!gpio_is_ready_dt(&phase_a)) {
		printk("%s: device not ready.", phase_a.port->name);
		return;
	}
	gpio_pin_configure_dt(&phase_a, GPIO_OUTPUT);

	if (!gpio_is_ready_dt(&phase_b)) {
		printk("%s: device not ready.", phase_b.port->name);
		return;
	}
	gpio_pin_configure_dt(&phase_b, GPIO_OUTPUT);

	k_timer_start(&qenc_emulate_timer, K_MSEC(QUAD_ENC_EMUL_PERIOD / 2),
			K_MSEC(QUAD_ENC_EMUL_PERIOD / 2));
}

#else

static void qenc_emulate_init(void) { };

#endif /* QUAD_ENC_EMUL_ENABLED */

int main(void)
{
	struct sensor_value val;
	int rc;
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(qdec0));

	if (!device_is_ready(dev)) {
		printk("Qdec device is not ready\n");
		return 0;
	}

	printk("Quadrature decoder sensor test\n");

	qenc_emulate_init();

#ifndef CONFIG_COVERAGE
	while (true) {
#else
	for (int i = 0; i < 3; i++) {
#endif
		rc = sensor_sample_fetch(dev);
		if (rc != 0) {
			printk("Failed to fetch sample (%d)\n", rc);
			return 0;
		}

		rc = sensor_channel_get(dev, SENSOR_CHAN_ROTATION, &val);
		if (rc != 0) {
			printk("Failed to get data (%d)\n", rc);
			return 0;
		}

		printk("Position = %d degrees\n", val.val1);

		k_msleep(1000);
	}
	return 0;
}
