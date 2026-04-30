/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/misc/adgm3121/adgm3121.h>

#define ADGM3121_NODE DT_NODELABEL(adgm3121)

static const char *const switch_names[] = {"SW1", "SW2", "SW3", "SW4"};

static const char *state_str(enum adgm3121_state state)
{
	return state == ADGM3121_ENABLE ? "ON" : "OFF";
}

int main(void)
{
	const struct device *dev = DEVICE_DT_GET(ADGM3121_NODE);
	enum adgm3121_state state;
	uint8_t mask;
	int ret;

	printk("ADGM3121 MEMS Switch Sample\n");

	if (!device_is_ready(dev)) {
		printk("Error: ADGM3121 device not ready\n");
		return -ENODEV;
	}

	printk("Device ready\n\n");

	/* Toggle each switch individually */
	printk("--- Individual switch control ---\n");
	for (int i = ADGM3121_SW1; i <= ADGM3121_SW4; i++) {
		ret = adgm3121_set_switch_state(dev, i, ADGM3121_ENABLE);
		if (ret) {
			printk("Error enabling %s: %d\n", switch_names[i], ret);
			return ret;
		}

		ret = adgm3121_get_switch_state(dev, i, &state);
		if (ret) {
			printk("Error reading %s: %d\n", switch_names[i], ret);
			return ret;
		}
		printk("%s: %s\n", switch_names[i], state_str(state));

		ret = adgm3121_set_switch_state(dev, i, ADGM3121_DISABLE);
		if (ret) {
			printk("Error disabling %s: %d\n", switch_names[i], ret);
			return ret;
		}
	}

	/* Set multiple switches via bitmask: enable SW1 and SW3 */
	printk("\n--- Bitmask control ---\n");
	ret = adgm3121_set_switches(dev, BIT(ADGM3121_SW1) | BIT(ADGM3121_SW3));
	if (ret) {
		printk("Error setting switches: %d\n", ret);
		return ret;
	}

	ret = adgm3121_get_switches(dev, &mask);
	if (ret) {
		printk("Error reading switches: %d\n", ret);
		return ret;
	}
	printk("Switch mask after setting SW1+SW3: 0x%02x\n", mask);

	for (int i = ADGM3121_SW1; i <= ADGM3121_SW4; i++) {
		printk("  %s: %s\n", switch_names[i],
		       (mask & BIT(i)) ? "ON" : "OFF");
	}

	/* Reset all switches */
	printk("\n--- Reset ---\n");
	ret = adgm3121_reset_switches(dev);
	if (ret) {
		printk("Error resetting switches: %d\n", ret);
		return ret;
	}

	ret = adgm3121_get_switches(dev, &mask);
	if (ret) {
		printk("Error reading switches: %d\n", ret);
		return ret;
	}
	printk("Switch mask after reset: 0x%02x\n", mask);

	printk("\nADGM3121 sample completed\n");
	return 0;
}
