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

void qenc_emulate_work_handler(struct k_work *work)
{
	gpio_pin_toggle_dt(&phase_a);
	k_msleep(1);
	gpio_pin_toggle_dt(&phase_b);
}

K_WORK_DEFINE(qenc_emulate_work, qenc_emulate_work_handler);

void qenc_emulate_timer_handler(struct k_timer *dummy)
{
	k_work_submit(&qenc_emulate_work);
}

K_TIMER_DEFINE(qenc_emulate_timer, qenc_emulate_timer_handler, NULL);

void qenc_emulate_init(void)
{
	printk("Quadrature encoder emulator enabled with %u ms period\n",
		QUAD_ENC_EMUL_PERIOD);

	if (!device_is_ready(phase_a.port)) {
		printk("%s: device not ready.", phase_a.port->name);
		return;
	}
	gpio_pin_configure_dt(&phase_a, GPIO_OUTPUT);

	if (!device_is_ready(phase_b.port)) {
		printk("%s: device not ready.", phase_b.port->name);
		return;
	}
	gpio_pin_configure_dt(&phase_b, GPIO_OUTPUT);

	k_timer_start(&qenc_emulate_timer, K_MSEC(QUAD_ENC_EMUL_PERIOD),
			K_MSEC(QUAD_ENC_EMUL_PERIOD));
}

#else

void qenc_emulate_init(void) { };

#endif /* QUAD_ENC_EMUL_ENABLED */

void main(void)
{
	struct sensor_value val;
	int rc;
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(qdec0));

	if (!device_is_ready(dev)) {
		printk("Qdec device is not ready\n");
		return;
	}

	printk("Quadrature decoder sensor test\n");

	qenc_emulate_init();

	while (true) {
		rc = sensor_sample_fetch(dev);
		if (rc != 0) {
			printk("Failed to fetch sample (%d)\n", rc);
			return;
		}

		rc = sensor_channel_get(dev, SENSOR_CHAN_ROTATION, &val);
		if (rc != 0) {
			printk("Failed to get data (%d)\n", rc);
			return;
		}

		printk("Position = %d degrees\n", val.val1);

		k_msleep(1000);
	}
}
