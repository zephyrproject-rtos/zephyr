/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/misc/adg1736/adg1736.h>

#define ADG1736_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(adi_adg1736)

static const char *const switch_names[] = {"SW1", "SW2"};

static const char *state_str(enum adg1736_state state)
{
	return state == ADG1736_CONNECT_A ? "A" : "B";
}

int main(void)
{
	const struct device *dev = DEVICE_DT_GET(ADG1736_NODE);
	enum adg1736_state state;
	int ret;

	printk("ADG1736 Dual SPDT Analog Switch Sample\n");

	if (!device_is_ready(dev)) {
		printk("Error: ADG1736 device not ready\n");
		return -ENODEV;
	}

	printk("Device ready\n\n");

	/* Toggle each switch through both paths */
	printk("--- Individual switch control ---\n");
	for (int i = ADG1736_SW1; i <= ADG1736_SW2; i++) {
		/* Connect to path A */
		ret = adg1736_set_switch_state(dev, i, ADG1736_CONNECT_A);
		if (ret) {
			printk("Error setting %s to A: %d\n",
			       switch_names[i], ret);
			return ret;
		}

		ret = adg1736_get_switch_state(dev, i, &state);
		if (ret) {
			printk("Error reading %s: %d\n",
			       switch_names[i], ret);
			return ret;
		}
		printk("%s: path %s\n", switch_names[i], state_str(state));

		/* Connect to path B */
		ret = adg1736_set_switch_state(dev, i, ADG1736_CONNECT_B);
		if (ret) {
			printk("Error setting %s to B: %d\n",
			       switch_names[i], ret);
			return ret;
		}

		ret = adg1736_get_switch_state(dev, i, &state);
		if (ret) {
			printk("Error reading %s: %d\n",
			       switch_names[i], ret);
			return ret;
		}
		printk("%s: path %s\n", switch_names[i], state_str(state));
	}

	/* Test enable/disable if supported */
	printk("\n--- Enable/Disable control ---\n");
	ret = adg1736_enable(dev);
	if (ret == -ENOTSUP) {
		printk("EN pin not available on this variant\n");
	} else if (ret) {
		printk("Error enabling device: %d\n", ret);
		return ret;
	}

	if (ret == 0) {
		printk("Device enabled\n");

		ret = adg1736_disable(dev);
		if (ret) {
			printk("Error disabling device: %d\n", ret);
			return ret;
		}
		printk("Device disabled\n");

		/* Re-enable for further operation */
		ret = adg1736_enable(dev);
		if (ret) {
			printk("Error re-enabling device: %d\n", ret);
			return ret;
		}
		printk("Device re-enabled\n");
	}

	/* Reset all switches */
	printk("\n--- Reset ---\n");
	ret = adg1736_reset_switches(dev);
	if (ret) {
		printk("Error resetting switches: %d\n", ret);
		return ret;
	}

	for (int i = ADG1736_SW1; i <= ADG1736_SW2; i++) {
		ret = adg1736_get_switch_state(dev, i, &state);
		if (ret) {
			printk("Error reading %s: %d\n",
			       switch_names[i], ret);
			return ret;
		}
		printk("%s: path %s\n", switch_names[i], state_str(state));
	}

	printk("\nADG1736 sample completed\n");
	return 0;
}
