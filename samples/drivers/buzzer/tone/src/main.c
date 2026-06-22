/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/buzzer.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define BUZZER_NODE DT_ALIAS(buzzer0)
BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(BUZZER_NODE),
	     "No default buzzer specified in DT (add `aliases { buzzer0 = ... };`)");

#define TONE_C5_HZ 523U
#define TONE_E5_HZ 659U
#define TONE_G5_HZ 784U

int main(void)
{
	const struct device *buzzer = DEVICE_DT_GET(BUZZER_NODE);

	if (!device_is_ready(buzzer)) {
		printk("Buzzer device %s not ready\n", buzzer->name);
		return 0;
	}

	buzzer_set_volume(buzzer, 50);

	while (1) {
		printk("Attention beep\n");
		buzzer_beep(buzzer, 150);
		k_sleep(K_MSEC(400));

		printk("Playing tones\n");
		buzzer_tone(buzzer, TONE_C5_HZ, 200);
		k_sleep(K_MSEC(280));
		buzzer_tone(buzzer, TONE_E5_HZ, 200);
		k_sleep(K_MSEC(280));
		buzzer_tone(buzzer, TONE_G5_HZ, 400);
		k_sleep(K_MSEC(480));

		k_sleep(K_SECONDS(3));
	}
	return 0;
}
