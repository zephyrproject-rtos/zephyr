/*
 * Copyright (c) 2026 Nicolas Belin <nbelin@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>

/* On platforms with gpio_emul the sample generates synthetic pulses so no
 * physical sensor is required.  On real hardware this block is compiled
 * out; connect the flow meter output to the GPIO defined in the board
 * overlay.
 */
#ifdef CONFIG_GPIO_EMUL
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#define FLOW_METER_NODE DT_NODELABEL(flow_meter_0)
static const struct device *gpio_sim = DEVICE_DT_GET(DT_GPIO_CTLR(FLOW_METER_NODE, pulse_gpios));
static const uint32_t pulse_pin = DT_GPIO_PIN(FLOW_METER_NODE, pulse_gpios);

/* Simulate count pulses with the given half-period in microseconds. */
static void simulate_pulses(uint32_t count, uint32_t half_period_us)
{
	for (uint32_t i = 0; i < count; i++) {
		gpio_emul_input_set(gpio_sim, pulse_pin, 1);
		k_busy_wait(half_period_us);
		gpio_emul_input_set(gpio_sim, pulse_pin, 0);
		k_busy_wait(half_period_us);
	}
}
#endif /* CONFIG_GPIO_EMUL */

static const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(flow_meter_0));
static volatile bool threshold_reached;

static void volume_threshold_cb(const struct device *sensor_dev, const struct sensor_trigger *trig)
{
	ARG_UNUSED(sensor_dev);
	ARG_UNUSED(trig);
	threshold_reached = true;
}

int main(void)
{
	struct sensor_value vol, rate, threshold_val;
	static const struct sensor_trigger trig = {
		.type = SENSOR_TRIG_THRESHOLD,
		.chan = SENSOR_CHAN_VOLUME,
	};
	int ret;

	if (!device_is_ready(dev)) {
		printk("flow meter not ready\n");
		return -ENODEV;
	}

	printk("Flow meter sample\n");

	/* Arm a 1 L volume threshold.  The sequence matters: set the threshold
	 * value first, then register the handler.  SENSOR_ATTR_UPPER_THRESH
	 * snapshots the current pulse count; the trigger fires once that many
	 * additional pulses have accumulated.  The trigger is one-shot — it
	 * disarms automatically after firing.
	 *
	 * sensor_value encodes liters: val1 = whole liters, val2 = microliters
	 * (millionths of a liter), so {1, 500000} means 1.5 L.
	 */
	threshold_val.val1 = 1;
	threshold_val.val2 = 0;
	ret = sensor_attr_set(dev, SENSOR_CHAN_VOLUME, SENSOR_ATTR_UPPER_THRESH, &threshold_val);
	if (ret < 0) {
		printk("failed to set volume threshold: %d\n", ret);
		return ret;
	}
	ret = sensor_trigger_set(dev, &trig, volume_threshold_cb);
	if (ret < 0) {
		printk("failed to set trigger: %d\n", ret);
		return ret;
	}
	printk("1 L volume threshold armed\n\n");

	/* Poll volume and rate every 500 ms.  In a real application the
	 * threshold callback replaces polling for the dispense-complete event.
	 * sensor_sample_fetch() also resets the flow-rate measurement window,
	 * so call it once per measurement interval for accurate rate readings.
	 */
	while (!threshold_reached) {
#ifdef CONFIG_GPIO_EMUL
		/* Simulate 45 pulses at 450 p/L = 0.1 L with 555 µs half-period
		 * (0.1 L per ~550 ms loop iteration ≈ 11 L/min).
		 */
		simulate_pulses(45, 555);
#endif
		ret = sensor_sample_fetch(dev);
		if (ret < 0) {
			printk("sample fetch failed: %d\n", ret);
			return ret;
		}
		sensor_channel_get(dev, SENSOR_CHAN_VOLUME, &vol);
		sensor_channel_get(dev, SENSOR_CHAN_FLOW_RATE, &rate);

		printk("volume=%d.%06d L  rate=%d.%06d L/min\n", vol.val1, vol.val2, rate.val1,
		       rate.val2);

		k_sleep(K_MSEC(500));
	}

	ret = sensor_sample_fetch(dev);
	if (ret < 0) {
		printk("sample fetch failed: %d\n", ret);
		return ret;
	}
	sensor_channel_get(dev, SENSOR_CHAN_VOLUME, &vol);
	printk("\nVolume threshold reached! Total volume: %d.%06d L\n", vol.val1, vol.val2);

	return 0;
}
