/*
 * Copyright (c) 2025 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_LED_LED_BLINK_H_
#define ZEPHYR_DRIVERS_LED_LED_BLINK_H_

#define LED_BLINK_SOFTWARE_DATA_INIT(node_id) {}

#define LED_BLINK_SOFTWARE_DATA(inst, name) \
	COND_CODE_1(CONFIG_LED_BLINK_SOFTWARE, ( \
	.name = (struct led_blink_software_data[]) {\
		DT_INST_FOREACH_CHILD_SEP(inst, LED_BLINK_SOFTWARE_DATA_INIT, (,)) \
	},), ())

#ifdef CONFIG_LED_BLINK_SOFTWARE
struct led_blink_software_data {
	const struct device *dev;
	uint32_t led;
	struct k_work_delayable work;
	uint32_t delay_on;
	uint32_t delay_off;
};

int led_blink_software_start(const struct device *dev, uint32_t led, uint32_t delay_on,
			     uint32_t delay_off);
#else
struct led_blink_software_data;
static inline int led_blink_software_start(const struct device *dev, uint32_t led,
					   uint32_t delay_on, uint32_t delay_off)
{
	return -ENOSYS;
}
#endif

#endif /* ZEPHYR_DRIVERS_LED_LED_BLINK_H_ */
