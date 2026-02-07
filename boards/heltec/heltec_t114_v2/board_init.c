/*
 * Copyright (c) 2023 The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "board_init.h"

#define VEXT_CONTROL_NODE DT_ALIAS(vext_control)
#define LED4_NODE         DT_ALIAS(led4)
#define BUTTON0_NODE      DT_ALIAS(button0)
#define STRIP_NODE        DT_NODELABEL(led_strip)
#define ADC_CONTROL_NODE  DT_ALIAS(adc_control)
#define TFT_EN_NODE       DT_ALIAS(tft_en)
#define TFT_LED_EN        DT_ALIAS(tft_led_en)

const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED4_NODE, gpios);
const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON0_NODE, gpios);
const struct gpio_dt_spec vext_ctl = GPIO_DT_SPEC_GET(VEXT_CONTROL_NODE, gpios);
const struct gpio_dt_spec adc_ctl = GPIO_DT_SPEC_GET(ADC_CONTROL_NODE, gpios);
const struct gpio_dt_spec tft_en = GPIO_DT_SPEC_GET(TFT_EN_NODE, gpios);
const struct gpio_dt_spec tft_led_en = GPIO_DT_SPEC_GET(TFT_LED_EN, gpios);
const struct gpio_dt_spec gnss_rst = GPIO_DT_SPEC_GET(DT_ALIAS(gnss_rst), gpios);
const struct gpio_dt_spec gnss_wakeup = GPIO_DT_SPEC_GET(DT_ALIAS(gnss_wakeup), gpios);

const struct device *led_strip = DEVICE_DT_GET(STRIP_NODE);
const struct device *adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc));
const struct device *tft_display_dev = DEVICE_DT_GET(DT_NODELABEL(tft_display));
const struct device *lora_dev = DEVICE_DT_GET(DT_NODELABEL(lora));
const struct device *gnss_dev = DEVICE_DT_GET(DT_NODELABEL(gnss));

static int board_heltec_t114_v2_init(void)
{
	/* VEXT Control */
	/* ------------------------------------------------------------ */
	if (!device_is_ready(vext_ctl.port)) {
		return -ENODEV;
	}

	if (gpio_pin_configure_dt(&vext_ctl, GPIO_OUTPUT)) {
		return -EINVAL;
	}

	if (gpio_pin_set_dt(&vext_ctl, 1)) {
		return -EINVAL;
	}

	/* LED Strip */
	/* ------------------------------------------------------------ */
	if (!device_is_ready(led_strip)) {
		return -ENODEV;
	}

	if (!device_is_ready(led.port)) {
		return -ENODEV;
	}

	if (gpio_pin_configure_dt(&led, GPIO_OUTPUT)) {
		return -EINVAL;
	}

	/* Button */
	/* ------------------------------------------------------------ */
	if (!device_is_ready(button.port)) {
		return -ENODEV;
	}

	if (gpio_pin_configure_dt(&button, GPIO_INPUT)) {
		return -EINVAL;
	}

	/* ADC Control */
	/* ------------------------------------------------------------ */
	if (!device_is_ready(adc_ctl.port)) {
		return -ENODEV;
	}

	if (gpio_pin_configure_dt(&adc_ctl, GPIO_OUTPUT)) {
		return -EINVAL;
	}

	if (gpio_pin_set_dt(&adc_ctl, 1)) {
		return -EINVAL;
	}
	/* Wait for ADC to stabilize */
	k_msleep(10);

	if (!device_is_ready(adc_dev)) {
		return -ENODEV;
	}

	/* LoRa */
	/* ------------------------------------------------------------ */
	if (!device_is_ready(lora_dev)) {
		return -ENODEV;
	}

	/* GNSS */
	/* ------------------------------------------------------------ */
#ifdef CONFIG_GNSS
	if (!device_is_ready(gnss_dev)) {
		return -ENODEV;
	}

	if (!device_is_ready(gnss_rst.port)) {
		return -ENODEV;
	}

	if (gpio_pin_configure_dt(&gnss_rst, GPIO_OUTPUT)) {
		return -EINVAL;
	}

	if (!device_is_ready(gnss_wakeup.port)) {
		return -ENODEV;
	}

	if (gpio_pin_configure_dt(&gnss_wakeup, GPIO_OUTPUT)) {
		return -EINVAL;
	}
#endif

	return 0;
}

SYS_INIT(board_heltec_t114_v2_init, PRE_KERNEL_2, CONFIG_GPIO_INIT_PRIORITY);

#ifdef CONFIG_ST7789V
static int board_tft_init(void)
{
	if (!device_is_ready(tft_en.port)) {
		return -ENODEV;
	}

	if (gpio_pin_configure_dt(&tft_en, GPIO_OUTPUT)) {
		return -EINVAL;
	}

	if (gpio_pin_set_dt(&tft_en, 1)) {
		return -EINVAL;
	}
	k_msleep(5);

	if (!device_is_ready(tft_led_en.port)) {
		return -ENODEV;
	}

	if (gpio_pin_configure_dt(&tft_led_en, GPIO_OUTPUT)) {
		return -EINVAL;
	}

	if (gpio_pin_set_dt(&tft_led_en, 1)) {
		return -EINVAL;
	}
	k_msleep(5);

	if (!device_is_ready(tft_display_dev)) {
		return -ENODEV;
	}
	return 0;
}

SYS_INIT(board_tft_init, POST_KERNEL, 90);
#endif
