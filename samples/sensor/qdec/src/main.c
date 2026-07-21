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

#ifdef CONFIG_EQDC_MCUX_TRIGGER

#include <zephyr/sys/atomic.h>

/* Incremented from the trigger handler (system workqueue context). */
static atomic_t trigger_count;

static void revolution_trigger_handler(const struct device *dev,
				       const struct sensor_trigger *trig)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(trig);

	atomic_inc(&trigger_count);
}

/*
 * Register SENSOR_TRIG_OVERFLOW on the revolution channel. With
 * revolution-count-mode = modulus, the EQDC revolution counter rolls
 * over/under without an INDEX signal, firing this trigger as the emulated
 * encoder drives the position counter past a full revolution.
 */
static void qdec_trigger_init(const struct device *dev)
{
	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_OVERFLOW,
		.chan = SENSOR_CHAN_ENCODER_REVOLUTIONS,
	};
	int rc = sensor_trigger_set(dev, &trig, revolution_trigger_handler);

	if (rc != 0) {
		printk("Failed to set SENSOR_TRIG_OVERFLOW (%d)\n", rc);
		return;
	}
	printk("Registered SENSOR_TRIG_OVERFLOW on SENSOR_CHAN_ENCODER_REVOLUTIONS\n");
}

#else

static void qdec_trigger_init(const struct device *dev)
{
	ARG_UNUSED(dev);
};

#endif /* CONFIG_EQDC_MCUX_TRIGGER */

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

	qdec_trigger_init(dev);

#ifndef CONFIG_COVERAGE
	while (true) {
#else
	for (int i = 0; i < 3; i++) {
#endif
		/* sleep first to gather position from first period */
		k_msleep(1000);

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

		/* Get speed (optional)*/
		rc = sensor_channel_get(dev, SENSOR_CHAN_RPM, &val);
		if (rc == 0) {
			printk("Speed = %d RPM\n", val.val1);
		}

		/* Get revolutions (optional)*/
		rc = sensor_channel_get(dev, SENSOR_CHAN_ENCODER_REVOLUTIONS, &val);
		if (rc == 0) {
			printk("Revolutions = %d\n", val.val1);
		}

#ifdef CONFIG_EQDC_MCUX_TRIGGER
		/*
		 * "triggers" is how many times the SENSOR_TRIG_OVERFLOW handler
		 * ran. If the trigger path works it tracks the revolution
		 * roll-over events; if it stays 0 while revolutions change, the
		 * IRQ never reaches the handler.
		 */
		printk("Triggers = %ld\n", (long)atomic_get(&trigger_count));
#endif
	}
	return 0;
}
