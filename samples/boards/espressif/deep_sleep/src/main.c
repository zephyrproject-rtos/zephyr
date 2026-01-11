/*
 * Copyright (c) 2022 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/drivers/retained_mem.h>
#include <esp_sleep.h>
#if SOC_RTCIO_INPUT_OUTPUT_SUPPORTED
#include <driver/rtc_io.h>
#endif

#include <esp_attr.h>

#define RETAINED_MEM_CONST 0x01234567

#ifdef CONFIG_EXAMPLE_EXT1_WAKEUP
#ifdef CONFIG_SOC_SERIES_ESP32H2
#define EXT_WAKEUP_PIN_1	(10)
#define EXT_WAKEUP_PIN_2	(11)
#else
#define EXT_WAKEUP_PIN_1	(2)
#define EXT_WAKEUP_PIN_2	(4)
#endif
#endif

#ifdef CONFIG_EXAMPLE_GPIO_WAKEUP
#if !DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(wakeup_button))
#error "Unsupported: wakeup-button alias is not defined"
#else
static const struct gpio_dt_spec wakeup_button = GPIO_DT_SPEC_GET(DT_ALIAS(wakeup_button), gpios);
#endif
#endif

static void print_counter(__maybe_unused esp_sleep_wakeup_cause_t sleep_wakeup_cause)
{
#ifdef CONFIG_RETAINED_MEM
	const struct device *retained_mem_device = DEVICE_DT_GET(DT_ALIAS(retainedmemdevice));

	off_t offset;
	uint32_t aux;
	int err;

	if (!device_is_ready(retained_mem_device)) {
		printk("retained_mem device is not ready!\n");
		return;
	}

	switch (sleep_wakeup_cause) {
#ifdef CONFIG_EXAMPLE_EXT1_WAKEUP
	case ESP_SLEEP_WAKEUP_EXT1:
#endif /* CONFIG_EXAMPLE_EXT1_WAKEUP */
#ifdef CONFIG_EXAMPLE_GPIO_WAKEUP
	case ESP_SLEEP_WAKEUP_GPIO:
#endif /* CONFIG_EXAMPLE_GPIO_WAKEUP */
	case ESP_SLEEP_WAKEUP_TIMER:
		offset = 0;
		err = retained_mem_read(retained_mem_device, offset, (uint8_t *)&aux, sizeof(aux));
		if (err < 0) {
			printk("retained_mem_read() failed: %d\n", err);
			return;
		}

		if (aux != RETAINED_MEM_CONST) {
			printk("retained_mem_read() retrieved wrong value - expected: "
				STRINGIFY(RETAINED_MEM_CONST) "; actual: 0x%" PRIx32 "\n", aux);
			return;
		}

		offset += sizeof(aux);
		err = retained_mem_read(retained_mem_device, offset, (uint8_t *)&aux, sizeof(aux));
		if (err < 0) {
			printk("retained_mem_read() failed: %d\n", err);
			return;
		}

		printk("ESP32 deep sleep example, current counter is %" PRIu32 "\n", aux++);

		err = retained_mem_write(retained_mem_device, offset, (uint8_t *)&aux, sizeof(aux));
		if (err < 0) {
			printk("retained_mem_write() failed: %d\n", err);
			return;
		}
		break;
	case ESP_SLEEP_WAKEUP_UNDEFINED:
	default:
		aux = RETAINED_MEM_CONST;
		offset = 0;
		err = retained_mem_write(retained_mem_device, offset, (uint8_t *)&aux, sizeof(aux));
		if (err < 0) {
			printk("retained_mem_write() failed: %d\n", err);
			return;
		}

		aux = 1;
		offset += sizeof(aux);
		err = retained_mem_write(retained_mem_device, offset, (uint8_t *)&aux, sizeof(aux));
		if (err < 0) {
			printk("retained_mem_write() failed: %d\n", err);
			return;
		}
	}
#endif /* CONFIG_RETAINED_MEM */
}

int main(void)
{
	esp_sleep_wakeup_cause_t sleep_wakeup_cause = esp_sleep_get_wakeup_cause();

	print_counter(sleep_wakeup_cause);

	switch (sleep_wakeup_cause) {
#ifdef CONFIG_EXAMPLE_EXT1_WAKEUP
	case ESP_SLEEP_WAKEUP_EXT1:
	{
		uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();

		if (wakeup_pin_mask != 0) {
			int pin = __builtin_ffsll(wakeup_pin_mask) - 1;

			printk("Wake up from GPIO %d\n", pin);
		} else {
			printk("Wake up from GPIO\n");
		}
		break;
	}
#endif /* CONFIG_EXAMPLE_EXT1_WAKEUP */
#ifdef CONFIG_EXAMPLE_GPIO_WAKEUP
	case ESP_SLEEP_WAKEUP_GPIO:
	{
		uint64_t wakeup_pin_mask = esp_sleep_get_gpio_wakeup_status();

		if (wakeup_pin_mask != 0) {
			int pin = __builtin_ffsll(wakeup_pin_mask) - 1;

			printk("Wake up from GPIO %d\n", pin);
		} else {
			printk("Wake up from GPIO\n");
		}
		break;
	}
#endif /* CONFIG_EXAMPLE_GPIO_WAKEUP */
	case ESP_SLEEP_WAKEUP_TIMER:
		printk("Wake up from timer.\n");
		break;
	case ESP_SLEEP_WAKEUP_UNDEFINED:
	default:
		printk("Not a deep sleep reset\n");
	}

	k_busy_wait(1000000);

	const int wakeup_time_sec = CONFIG_EXAMPLE_WAKEUP_TIME_SEC;

	printk("Enabling timer wakeup, %ds\n", wakeup_time_sec);
	esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000);

#ifdef CONFIG_EXAMPLE_EXT1_WAKEUP
	const int ext_wakeup_pin_1 = EXT_WAKEUP_PIN_1;
	const uint64_t ext_wakeup_pin_1_mask = BIT64(ext_wakeup_pin_1);
	const int ext_wakeup_pin_2 = EXT_WAKEUP_PIN_2;
	const uint64_t ext_wakeup_pin_2_mask = BIT64(ext_wakeup_pin_2);

	printk("Enabling EXT1 wakeup on pins GPIO%d, GPIO%d\n",
			ext_wakeup_pin_1, ext_wakeup_pin_2);
	esp_sleep_enable_ext1_wakeup(ext_wakeup_pin_1_mask | ext_wakeup_pin_2_mask,
			ESP_EXT1_WAKEUP_ANY_HIGH);

#if SOC_RTCIO_INPUT_OUTPUT_SUPPORTED
	/* enable pull-down on ext1 pins to avoid random wake-ups */
	rtc_gpio_pullup_dis(EXT_WAKEUP_PIN_1);
	rtc_gpio_pulldown_en(EXT_WAKEUP_PIN_1);
	rtc_gpio_pullup_dis(EXT_WAKEUP_PIN_2);
	rtc_gpio_pulldown_en(EXT_WAKEUP_PIN_2);
#endif
#endif /* CONFIG_EXAMPLE_EXT1_WAKEUP */
#ifdef CONFIG_EXAMPLE_GPIO_WAKEUP
	if (!gpio_is_ready_dt(&wakeup_button)) {
		printk("Error: wakeup button device %s is not ready\n", wakeup_button.port->name);
		return 0;
	}

	int ret = gpio_pin_configure_dt(&wakeup_button, GPIO_INPUT);

	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n",
				ret, wakeup_button.port->name, wakeup_button.pin);
		return 0;
	}

	esp_deep_sleep_enable_gpio_wakeup(BIT(wakeup_button.pin), ESP_GPIO_WAKEUP_GPIO_HIGH);
	printk("Enabling GPIO wakeup on pins GPIO%d\n", wakeup_button.pin);
#endif /* CONFIG_EXAMPLE_GPIO_WAKEUP */

	printk("Powering off\n");
	sys_poweroff();

	return 0;
}
