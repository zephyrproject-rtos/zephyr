/*
 * Copyright (c) 2018 Christian Taedcke
 * Copyright (c) 2022 T-Mobile USA, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include "board.h"
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>

#define BATTERY_TABLE_LEN 15
float battery_discharging_tbl[BATTERY_TABLE_LEN][2] = {
	{4.160, 100.0 },
	{4.100, 95.0  },
	{4.058, 90.0  },
	{3.962, 80.0  },
	{3.915, 70.0  },
	{3.842, 60.0  },
	{3.798, 50.0  },
	{3.764, 40.0  },
	{3.742, 30.0  },
	{3.702, 20.0  },
	{3.660, 10.0  },
	{3.594, 5.0   },
	{3.397, 2.0   },
	{3.257, 1.0   },
	{2.772, 0.0   }};

float battery_charging_tbl[BATTERY_TABLE_LEN][2] = {
	{4.160, 100.0 },
	{4.100, 95.0  },
	{4.058, 90.0  },
	{3.962, 80.0  },
	{3.915, 70.0  },
	{3.842, 60.0  },
	{3.798, 50.0  },
	{3.764, 40.0  },
	{3.742, 30.0  },
	{3.702, 20.0  },
	{3.660, 10.0  },
	{3.594, 5.0   },
	{3.397, 2.0   },
	{3.257, 1.0   },
	{2.772, 0.0   }};

static void powerup_led_on(struct k_timer *timer_id);

K_TIMER_DEFINE(powerup_led_timer, powerup_led_on, NULL);

static int tmo_dev_edge(void)
{
	const struct device *rs_dev; /* RS9116 Gpio Device */
	const struct device *bz_dev; /* PAM8904E Gpio Device */
	const struct device *gpiof;

	rs_dev = device_get_binding(RS9116_GPIO_NAME);

	if (!rs_dev) {
		printk("RS9116 gpio port was not found!\n");
		return -ENODEV;
	}

	gpio_pin_configure(rs_dev, RS9116_RST_GPIO_PIN, GPIO_OUTPUT_LOW);
	gpio_pin_configure(rs_dev, RS9116_POC_GPIO_PIN, GPIO_OUTPUT_LOW);
	k_msleep(10);

	gpio_pin_configure(rs_dev, RS9116_POC_GPIO_PIN, GPIO_OUTPUT_HIGH);

	bz_dev = device_get_binding(BUZZER_ENx_GPIO_NAME);

	if (!bz_dev) {
		printk("PAM8904E gpio port was not found!\n");
		return -ENODEV;
	}

	gpio_pin_configure(bz_dev, BUZZER_EN1_GPIO_PIN, GPIO_OUTPUT_HIGH);
	gpio_pin_configure(bz_dev, BUZZER_EN2_GPIO_PIN, GPIO_OUTPUT_HIGH);

	gpio_pin_configure(bz_dev, WHITE_LED_GPIO_PIN, GPIO_OUTPUT_LOW);
	gpio_pin_configure(bz_dev, RED_LED_GPIO_PIN, GPIO_OUTPUT_LOW);
	gpio_pin_configure(bz_dev, GREEN_LED_GPIO_PIN, GPIO_OUTPUT_LOW);
	gpio_pin_configure(bz_dev, BLUE_LED_GPIO_PIN, GPIO_OUTPUT_LOW);

	gpiof = device_get_binding(GNSS_GPIO_NAME);
	if (gpiof == NULL) {
		printk("GNSS gpio port was not found!\n");
		return -ENODEV;
	}
	gpio_pin_configure(gpiof, GNSS_PWR_ON, GPIO_OUTPUT_HIGH);
	gpio_pin_configure(gpiof, GNSS_BOOT_REQ, GPIO_OUTPUT_LOW);
	gpio_pin_configure(gpiof, GNSS_RESET, GPIO_OUTPUT_LOW);

	k_sleep(K_MSEC(100));

	gpio_pin_configure(gpiof, GNSS_RESET, GPIO_OUTPUT_HIGH);

	gpio_pin_configure(gpiof, BQ_CHIP_ENABLE, GPIO_OUTPUT_LOW);

	k_timer_start(&powerup_led_timer, K_SECONDS(1), K_FOREVER);
	return 0;
}

#include <zephyr/drivers/pwm.h>

static void powerup_led_on(struct k_timer *timer_id)
{
#ifdef CONFIG_PWM
	const struct device *led_pwm = LED_PWM_DEV;

	if (!led_pwm) {
		return;
	}

	pwm_set(led_pwm, LED_PWM_RED, 100000, 10000, 0);
	pwm_set(led_pwm, LED_PWM_BLUE, 100000, 5000, 0);
#endif
}

/* needs to be done after GPIO driver init */
SYS_INIT(tmo_dev_edge, POST_KERNEL,
		CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
