/*
 * Copyright (c) 2022-2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/retained_mem.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/poweroff.h>
#include <esp_sleep.h>

#include <esp_attr.h>

#define RETAINED_MEM_CONST 0x01234567

#define WAKE_GPIOS DT_ALIAS(wakeup_gpios)

#if !DT_NODE_HAS_STATUS_OKAY(WAKE_GPIOS)
#error "Unsupported: wakeup-gpios node/alias not defined"
#endif

#define BUILD_GPIO_SPEC(node_id, prop, idx) GPIO_DT_SPEC_GET_BY_IDX(node_id, prop, idx),

static const struct gpio_dt_spec wake_gpios[] = {
	DT_FOREACH_PROP_ELEM(WAKE_GPIOS, gpios, BUILD_GPIO_SPEC)};

void dev_enable_wakeup(const struct device *dev)
{
	if (!device_is_ready(dev)) {
		printk("%s: device not ready\n", dev->name);
	}

	if (!pm_device_wakeup_enable(dev, true)) {
		printk("%s: could not enable wakeup source\n", dev->name);
	}
}

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
	case ESP_SLEEP_WAKEUP_EXT1:
	case ESP_SLEEP_WAKEUP_GPIO:
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
	for (int i = 0; i < ARRAY_SIZE(wake_gpios); i++) {
		const struct device *dev_gpio = wake_gpios[i].port;

		dev_enable_wakeup(dev_gpio);
	}

	esp_sleep_wakeup_cause_t sleep_wakeup_cause = esp_sleep_get_wakeup_cause();

	print_counter(sleep_wakeup_cause);

	switch (sleep_wakeup_cause) {
	case ESP_SLEEP_WAKEUP_EXT1:
	case ESP_SLEEP_WAKEUP_GPIO: {
		uint64_t wakeup_pin_mask = 0;
#ifdef SOC_PM_SUPPORT_EXT1_WAKEUP
		wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
#elif defined(SOC_GPIO_SUPPORT_DEEPSLEEP_WAKEUP)
		wakeup_pin_mask = esp_sleep_get_gpio_wakeup_status();
#endif
		if (wakeup_pin_mask != 0) {
			int pin = __builtin_ffsll(wakeup_pin_mask) - 1;

			printk("Wake up from GPIO %d\n", pin);
		} else {
			printk("Wake up from GPIO\n");
		}
		break;
	}
	case ESP_SLEEP_WAKEUP_TIMER:
		printk("Wake up from timer.\n");
		break;
	case ESP_SLEEP_WAKEUP_UNDEFINED:
	default:
		printk("Not a deep sleep reset\n");
	}

	k_busy_wait(1000000);

	for (int i = 0; i < ARRAY_SIZE(wake_gpios); i++) {
		const struct gpio_dt_spec *g = &wake_gpios[i];

		int ret = gpio_pin_configure_dt(g, g->dt_flags | GPIO_INPUT);

		if (ret != 0) {
			printk("Error %d: failed to configure %s pin %d\n", ret, g->port->name,
			       g->pin);
			return 0;
		}
	}

	const int wakeup_time_sec = CONFIG_EXAMPLE_WAKEUP_TIME_SEC;

	printk("Enabling timer wakeup, %ds\n", wakeup_time_sec);

	/* Since sys_poweroff() is being called directly, timer wakeup
	 * must be programmed by application.
	 */
	esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000);

	printk("Powering off\n");
	sys_poweroff();

	return 0;
}
