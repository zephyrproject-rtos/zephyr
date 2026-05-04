/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/misc/adg1712/adg1712.h>

#define ADG1712_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(adi_adg1712)

static const char *const switch_names[] = {"SW1", "SW2", "SW3", "SW4"};

static const char *state_str(enum adg1712_state state)
{
	return state == ADG1712_ENABLE ? "ON" : "OFF";
}

int main(void)
{
	const struct device *dev = DEVICE_DT_GET(ADG1712_NODE);
	enum adg1712_state state;
	uint8_t mask;
	int ret;

	printk("ADG1712 Quad SPST Analog Switch Sample\n");

	if (!device_is_ready(dev)) {
		printk("Error: ADG1712 device not ready\n");
		return -ENODEV;
	}

	printk("Device ready\n\n");

	/* Toggle each switch individually */
	printk("--- Individual switch control ---\n");
	for (int i = ADG1712_SW1; i <= ADG1712_SW4; i++) {
		ret = adg1712_set_switch_state(dev, i, ADG1712_ENABLE);
		if (ret) {
			printk("Error enabling %s: %d\n", switch_names[i], ret);
			return ret;
		}

		ret = adg1712_get_switch_state(dev, i, &state);
		if (ret) {
			printk("Error reading %s: %d\n", switch_names[i], ret);
			return ret;
		}
		printk("%s: %s\n", switch_names[i], state_str(state));

		ret = adg1712_set_switch_state(dev, i, ADG1712_DISABLE);
		if (ret) {
			printk("Error disabling %s: %d\n", switch_names[i], ret);
			return ret;
		}
	}

	/* Set multiple switches via bitmask: enable SW1 and SW3 */
	printk("\n--- Bitmask control ---\n");
	ret = adg1712_set_switches(dev, BIT(ADG1712_SW1) | BIT(ADG1712_SW3));
	if (ret) {
		printk("Error setting switches: %d\n", ret);
		return ret;
	}

	ret = adg1712_get_switches(dev, &mask);
	if (ret) {
		printk("Error reading switches: %d\n", ret);
		return ret;
	}
	printk("Switch mask after setting SW1+SW3: 0x%02x\n", mask);

	for (int i = ADG1712_SW1; i <= ADG1712_SW4; i++) {
		printk("  %s: %s\n", switch_names[i],
		       (mask & BIT(i)) ? "ON" : "OFF");
	}

	/* Reset all switches */
	printk("\n--- Reset ---\n");
	ret = adg1712_reset_switches(dev);
	if (ret) {
		printk("Error resetting switches: %d\n", ret);
		return ret;
	}

	ret = adg1712_get_switches(dev, &mask);
	if (ret) {
		printk("Error reading switches: %d\n", ret);
		return ret;
	}
	printk("Switch mask after reset: 0x%02x\n", mask);

	printk("\nADG1712 sample completed\n");
	return 0;
}
