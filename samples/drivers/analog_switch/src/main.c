/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/analog_switch.h>

#define ANALOG_SWITCH_NODE DT_ALIAS(analog_switch0)

#if !DT_NODE_HAS_STATUS_OKAY(ANALOG_SWITCH_NODE)
#error "Unsupported board: analog-switch0 devicetree alias is not defined"
#endif

static const struct device *const sw_dev = DEVICE_DT_GET(ANALOG_SWITCH_NODE);

static const char *state_str(uint8_t state)
{
	return state ? "ON" : "OFF";
}

int main(void)
{
	uint8_t num_ch;
	uint8_t state;
	uint32_t all_states;
	int ret;

	printk("Analog Switch Sample\n");

	if (!device_is_ready(sw_dev)) {
		printk("Error: device %s not ready\n", sw_dev->name);
		return 0;
	}

	printk("Device ready: %s\n", sw_dev->name);

	num_ch = analog_switch_get_num_channels(sw_dev);
	printk("Number of channels: %u\n\n", num_ch);

	printk("--- Individual channel control ---\n");
	for (uint8_t ch = 0; ch < num_ch; ch++) {
		ret = analog_switch_set(sw_dev, ch, 1);
		if (ret) {
			printk("Error setting channel %u: %d\n", ch, ret);
			return 0;
		}

		ret = analog_switch_get(sw_dev, ch, &state);
		if (ret) {
			printk("Error reading channel %u: %d\n", ch, ret);
			return 0;
		}
		printk("Channel %u: %s\n", ch, state_str(state));

		ret = analog_switch_set(sw_dev, ch, 0);
		if (ret) {
			printk("Error clearing channel %u: %d\n", ch, ret);
			return 0;
		}
	}

	printk("\n--- Bitmask control ---\n");
	ret = analog_switch_set_all(sw_dev, BIT(0) | BIT(2));
	if (ret) {
		printk("Error setting channels: %d\n", ret);
		return 0;
	}

	ret = analog_switch_get_all(sw_dev, &all_states);
	if (ret) {
		printk("Error reading channels: %d\n", ret);
		return 0;
	}
	printk("Channel states after setting ch0+ch2: 0x%02x\n", all_states);

	for (uint8_t ch = 0; ch < num_ch; ch++) {
		printk("  Channel %u: %s\n", ch,
		       (all_states & BIT(ch)) ? "ON" : "OFF");
	}

	printk("\n--- Reset ---\n");
	ret = analog_switch_reset(sw_dev);
	if (ret) {
		printk("Error resetting channels: %d\n", ret);
		return 0;
	}

	ret = analog_switch_get_all(sw_dev, &all_states);
	if (ret) {
		printk("Error reading channels: %d\n", ret);
		return 0;
	}
	printk("Channel states after reset: 0x%02x\n", all_states);

	printk("\nAnalog switch sample completed\n");

	return 0;
}
