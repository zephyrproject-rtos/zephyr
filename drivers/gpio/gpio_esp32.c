/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_gpio

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/gpio/espressif-esp32-gpio.h>
#ifdef CONFIG_SOC_ESP32C3
#include <zephyr/drivers/interrupt_controller/intc_esp32c3.h>
#else
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#endif
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/logging/log.h>

#include <esp_attr.h>
#include <hal/gpio_ll.h>
#include <hal/rtc_io_hal.h>
#include <soc.h>
#include <soc/gpio_reg.h>
#include <soc/io_mux_reg.h>
#include <soc/soc.h>

LOG_MODULE_REGISTER(gpio_esp32, CONFIG_LOG_DEFAULT_LEVEL);

#ifdef CONFIG_SOC_ESP32C3
/* gpio structs in esp32c3 series are different from xtensa ones */
#define out out.data
#define in in.data
#define out_w1ts out_w1ts.val
#define out_w1tc out_w1tc.val
/* arch_curr_cpu() is not available for riscv based chips */
#define CPU_ID()  0
#define ISR_HANDLER isr_handler_t
#else
#define CPU_ID() arch_curr_cpu()->id
#define ISR_HANDLER intr_handler_t
#endif

#ifndef SOC_GPIO_SUPPORT_RTC_INDEPENDENT
#define SOC_GPIO_SUPPORT_RTC_INDEPENDENT 0
#endif

struct gpio_esp32_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config drv_cfg;
	gpio_dev_t *const gpio_base;
	gpio_dev_t *const gpio_dev;
	const int gpio_port;
};

struct gpio_esp32_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	sys_slist_t cb;
};

static inline bool rtc_gpio_is_valid_gpio(uint32_t gpio_num)
{
#if SOC_RTCIO_INPUT_OUTPUT_SUPPORTED
	return (gpio_num < SOC_GPIO_PIN_COUNT && rtc_io_num_map[gpio_num] >= 0);
#else
	return false;
#endif
}

static inline bool gpio_pin_is_valid(uint32_t pin)
{
	return ((BIT(pin) & SOC_GPIO_VALID_GPIO_MASK) != 0);
}

static inline bool gpio_pin_is_output_capable(uint32_t pin)
{
	return ((BIT(pin) & SOC_GPIO_VALID_OUTPUT_GPIO_MASK) != 0);
}

static int gpio_esp32_config(const struct device *dev,
			     gpio_pin_t pin,
			     gpio_flags_t flags)
{
	const struct gpio_esp32_config *const cfg = dev->config;
	struct gpio_esp32_data *data = dev->data;
	uint32_t io_pin = (uint32_t) pin + ((cfg->gpio_port == 1 && pin < 32) ? 32 : 0);
	uint32_t key;
	int ret = 0;

	if (!gpio_pin_is_valid(io_pin)) {
		LOG_ERR("Selected IO pin is not valid.");
		return -EINVAL;
	}

	key = irq_lock();

#if SOC_RTCIO_INPUT_OUTPUT_SUPPORTED
	if (rtc_gpio_is_valid_gpio(io_pin)) {
		rtcio_hal_function_select(rtc_io_num_map[io_pin], RTCIO_FUNC_DIGITAL);
	}
#endif

	if (io_pin >= GPIO_NUM_MAX) {
		LOG_ERR("Invalid pin.");
		ret = -EINVAL;
		goto end;
	}

	/* Set pin function as GPIO */
	gpio_ll_iomux_func_sel(GPIO_PIN_MUX_REG[io_pin], PIN_FUNC_GPIO);

	if (flags & GPIO_PULL_UP) {
		if (!rtc_gpio_is_valid_gpio(io_pin) || SOC_GPIO_SUPPORT_RTC_INDEPENDENT) {
			gpio_ll_pullup_en(&GPIO, io_pin);
		} else {
#if SOC_RTCIO_INPUT_OUTPUT_SUPPORTED
			int rtcio_num = rtc_io_num_map[io_pin];

			if (rtc_io_desc[rtcio_num].pullup) {
				rtcio_hal_pullup_enable(rtcio_num);
			} else {
				ret = -ENOTSUP;
				goto end;
			}
#endif
		}
	} else {
		if (!rtc_gpio_is_valid_gpio(io_pin) || SOC_GPIO_SUPPORT_RTC_INDEPENDENT) {
			gpio_ll_pullup_dis(&GPIO, io_pin);
		} else {
#if SOC_RTCIO_INPUT_OUTPUT_SUPPORTED
			int rtcio_num = rtc_io_num_map[io_pin];

			if (rtc_io_desc[rtcio_num].pullup) {
				rtcio_hal_pullup_disable(rtcio_num);
			}
#else
			ret = -ENOTSUP;
			goto end;
#endif
		}
	}

	if (flags & GPIO_SINGLE_ENDED) {
		if (flags & GPIO_LINE_OPEN_DRAIN) {
			gpio_ll_od_enable(cfg->gpio_base, io_pin);
		} else {
			LOG_ERR("GPIO configuration not supported");
			ret = -ENOTSUP;
			goto end;
		}
	} else {
		gpio_ll_od_disable(cfg->gpio_base, io_pin);
	}

	if (flags & GPIO_PULL_DOWN) {
		if (!rtc_gpio_is_valid_gpio(io_pin) || SOC_GPIO_SUPPORT_RTC_INDEPENDENT) {
			gpio_ll_pulldown_en(&GPIO, io_pin);
		} else {
#if SOC_RTCIO_INPUT_OUTPUT_SUPPORTED
			int rtcio_num = rtc_io_num_map[io_pin];

			if (rtc_io_desc[rtcio_num].pulldown) {
				rtcio_hal_pulldown_enable(rtcio_num);
			} else {
				ret = -ENOTSUP;
				goto end;
			}
#endif
		}
	} else {
		if (!rtc_gpio_is_valid_gpio(io_pin) || SOC_GPIO_SUPPORT_RTC_INDEPENDENT) {
			gpio_ll_pulldown_dis(&GPIO, io_pin);
		} else {
#if SOC_RTCIO_INPUT_OUTPUT_SUPPORTED
			int rtcio_num = rtc_io_num_map[io_pin];

			if (rtc_io_desc[rtcio_num].pulldown) {
				rtcio_hal_pulldown_disable(rtcio_num);
			}
#else
			ret = -ENOTSUP;
			goto end;
#endif
		}
	}

	if (flags & GPIO_OUTPUT) {

		if (!gpio_pin_is_output_capable(pin)) {
			LOG_ERR("GPIO can only be used as input");
			ret = -EINVAL;
			goto end;
		}

		/*
		 * By default, drive strength is set to its maximum value when the pin is set
		 * to either low or high states. Alternative drive strength is weak-only,
		 * while any other intermediary combination is considered invalid.
		 */
		switch (flags & ESP32_GPIO_DS_MASK) {
		case ESP32_GPIO_DS_DFLT:
			if (!rtc_gpio_is_valid_gpio(io_pin) || SOC_GPIO_SUPPORT_RTC_INDEPENDENT) {
				gpio_ll_set_drive_capability(cfg->gpio_base,
						io_pin,
						GPIO_DRIVE_CAP_3);
			} else {
#if SOC_RTCIO_INPUT_OUTPUT_SUPPORTED
				rtcio_hal_set_drive_capability(rtc_io_num_map[io_pin],
						GPIO_DRIVE_CAP_3);
#endif
			}
			break;
		case ESP32_GPIO_DS_ALT:
			if (!rtc_gpio_is_valid_gpio(io_pin) || SOC_GPIO_SUPPORT_RTC_INDEPENDENT) {
				gpio_ll_set_drive_capability(cfg->gpio_base,
						io_pin,
						GPIO_DRIVE_CAP_0);
			} else {
#if SOC_RTCIO_INPUT_OUTPUT_SUPPORTED
				rtcio_hal_set_drive_capability(rtc_io_num_map[io_pin],
						GPIO_DRIVE_CAP_0);
#endif
			}
			break;
		default:
			ret = -EINVAL;
			goto end;
		}

		gpio_ll_output_enable(&GPIO, io_pin);
		esp_rom_gpio_matrix_out(io_pin, SIG_GPIO_OUT_IDX, false, false);

		/* Set output pin initial value */
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			gpio_ll_set_level(cfg->gpio_base, io_pin, 1);
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			gpio_ll_set_level(cfg->gpio_base, io_pin, 0);
		}
	} else {
		gpio_ll_output_disable(&GPIO, io_pin);
	}

	if (flags & GPIO_INPUT) {
		gpio_ll_input_enable(&GPIO, io_pin);
	} else {
		gpio_ll_input_disable(&GPIO, io_pin);
	}

end:
	irq_unlock(key);

	return ret;
}

static int gpio_esp32_port_get_raw(const struct device *port, uint32_t *value)
{
	const struct gpio_esp32_config *const cfg = port->config;

	if (cfg->gpio_port == 0) {
		*value = cfg->gpio_dev->in;
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio1), okay)
	} else {
		*value = cfg->gpio_dev->in1.data;
#endif
	}

	return 0;
}

static int gpio_esp32_port_set_masked_raw(const struct device *port,
					  uint32_t mask, uint32_t value)
{
	const struct gpio_esp32_config *const cfg = port->config;

	uint32_t key = irq_lock();

	if (cfg->gpio_port == 0) {
		cfg->gpio_dev->out = (cfg->gpio_dev->out & ~mask) | (mask & value);
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio1), okay)
	} else {
		cfg->gpio_dev->out1.data = (cfg->gpio_dev->out1.data & ~mask) | (mask & value);
#endif
	}

	irq_unlock(key);

	return 0;
}

static int gpio_esp32_port_set_bits_raw(const struct device *port,
					uint32_t pins)
{
	const struct gpio_esp32_config *const cfg = port->config;

	if (cfg->gpio_port == 0) {
		cfg->gpio_dev->out_w1ts = pins;
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio1), okay)
	} else {
		cfg->gpio_dev->out1_w1ts.data = pins;
#endif
	}

	return 0;
}

static int gpio_esp32_port_clear_bits_raw(const struct device *port,
					  uint32_t pins)
{
	const struct gpio_esp32_config *const cfg = port->config;

	if (cfg->gpio_port == 0) {
		cfg->gpio_dev->out_w1tc = pins;
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio1), okay)
	} else {
		cfg->gpio_dev->out1_w1tc.data = pins;
#endif
	}

	return 0;
}

static int gpio_esp32_port_toggle_bits(const struct device *port,
				       uint32_t pins)
{
	const struct gpio_esp32_config *const cfg = port->config;
	uint32_t key = irq_lock();

	if (cfg->gpio_port == 0) {
		cfg->gpio_dev->out ^= pins;
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio1), okay)
	} else {
		cfg->gpio_dev->out1.data ^= pins;
#endif
	}

	irq_unlock(key);

	return 0;
}

static int convert_int_type(enum gpio_int_mode mode,
			    enum gpio_int_trig trig)
{
	if (mode == GPIO_INT_MODE_DISABLED) {
		return GPIO_INTR_DISABLE;
	}

	if (mode == GPIO_INT_MODE_LEVEL) {
		switch (trig) {
		case GPIO_INT_TRIG_LOW:
			return GPIO_INTR_LOW_LEVEL;
		case GPIO_INT_TRIG_HIGH:
			return GPIO_INTR_HIGH_LEVEL;
		default:
			return -EINVAL;
		}
	} else { /* edge interrupts */
		switch (trig) {
		case GPIO_INT_TRIG_HIGH:
			return GPIO_INTR_POSEDGE;
		case GPIO_INT_TRIG_LOW:
			return GPIO_INTR_NEGEDGE;
		case GPIO_INT_TRIG_BOTH:
			return GPIO_INTR_ANYEDGE;
		default:
			return -EINVAL;
		}
	}

	/* Any other type of interrupt triggering is invalid. */
	return -EINVAL;
}

static int gpio_esp32_pin_interrupt_configure(const struct device *port,
					      gpio_pin_t pin,
					      enum gpio_int_mode mode,
					      enum gpio_int_trig trig)
{
	const struct gpio_esp32_config *const cfg = port->config;
	uint32_t io_pin = (uint32_t) pin + ((cfg->gpio_port == 1 && pin < 32) ? 32 : 0);
	int intr_trig_mode = convert_int_type(mode, trig);
	uint32_t key;

	if (intr_trig_mode < 0) {
		return intr_trig_mode;
	}

	key = irq_lock();
	if (cfg->gpio_port == 0) {
		gpio_ll_clear_intr_status(cfg->gpio_base, BIT(pin));
	} else {
		gpio_ll_clear_intr_status_high(cfg->gpio_base, BIT(pin));
	}

	gpio_ll_set_intr_type(cfg->gpio_base, io_pin, intr_trig_mode);
	gpio_ll_intr_enable_on_core(cfg->gpio_base, CPU_ID(), io_pin);
	irq_unlock(key);

	return 0;
}

static int gpio_esp32_manage_callback(const struct device *dev,
				      struct gpio_callback *callback,
				      bool set)
{
	struct gpio_esp32_data *data = dev->data;

	return gpio_manage_callback(&data->cb, callback, set);
}

static uint32_t gpio_esp32_get_pending_int(const struct device *dev)
{
	const struct gpio_esp32_config *const cfg = dev->config;
	uint32_t irq_status;
	uint32_t const core_id = CPU_ID();

	if (cfg->gpio_port == 0) {
		gpio_ll_get_intr_status(cfg->gpio_base, core_id, &irq_status);
	} else {
		gpio_ll_get_intr_status_high(cfg->gpio_base, core_id, &irq_status);
	}

	return irq_status;
}

static void IRAM_ATTR gpio_esp32_fire_callbacks(const struct device *dev)
{
	const struct gpio_esp32_config *const cfg = dev->config;
	struct gpio_esp32_data *data = dev->data;
	uint32_t irq_status;
	uint32_t const core_id = CPU_ID();

	if (cfg->gpio_port == 0) {
		gpio_ll_get_intr_status(cfg->gpio_base, core_id, &irq_status);
		gpio_ll_clear_intr_status(cfg->gpio_base, irq_status);
	} else {
		gpio_ll_get_intr_status_high(cfg->gpio_base, core_id, &irq_status);
		gpio_ll_clear_intr_status_high(cfg->gpio_base, irq_status);
	}

	if (irq_status != 0) {
		gpio_fire_callbacks(&data->cb, dev, irq_status);
	}
}

static void gpio_esp32_isr(void *param);

static int gpio_esp32_init(const struct device *dev)
{
	struct gpio_esp32_data *data = dev->data;
	static bool isr_connected;

	if (!isr_connected) {
		esp_intr_alloc(DT_IRQN(DT_NODELABEL(gpio0)),
			0,
			(ISR_HANDLER)gpio_esp32_isr,
			(void *)dev,
			NULL);

		isr_connected = true;
	}

	return 0;
}

static const struct gpio_driver_api gpio_esp32_driver_api = {
	.pin_configure = gpio_esp32_config,
	.port_get_raw = gpio_esp32_port_get_raw,
	.port_set_masked_raw = gpio_esp32_port_set_masked_raw,
	.port_set_bits_raw = gpio_esp32_port_set_bits_raw,
	.port_clear_bits_raw = gpio_esp32_port_clear_bits_raw,
	.port_toggle_bits = gpio_esp32_port_toggle_bits,
	.pin_interrupt_configure = gpio_esp32_pin_interrupt_configure,
	.manage_callback = gpio_esp32_manage_callback,
	.get_pending_int = gpio_esp32_get_pending_int
};

#define ESP_SOC_GPIO_INIT(_id)							\
	static struct gpio_esp32_data gpio_data_##_id;	\
	static struct gpio_esp32_config gpio_config_##_id = {			\
		.drv_cfg = {							\
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(_id),	\
		},								\
		.gpio_base = (gpio_dev_t *)DT_REG_ADDR(DT_NODELABEL(gpio0)),	\
		.gpio_dev = (gpio_dev_t *)DT_REG_ADDR(DT_NODELABEL(gpio##_id)),	\
		.gpio_port = _id	\
	};									\
	DEVICE_DT_DEFINE(DT_NODELABEL(gpio##_id),				\
			&gpio_esp32_init,					\
			NULL,							\
			&gpio_data_##_id,					\
			&gpio_config_##_id,					\
			PRE_KERNEL_1,						\
			CONFIG_GPIO_INIT_PRIORITY,				\
			&gpio_esp32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ESP_SOC_GPIO_INIT);

static void IRAM_ATTR gpio_esp32_isr(void *param)
{
	ARG_UNUSED(param);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio0), okay)
	gpio_esp32_fire_callbacks(DEVICE_DT_INST_GET(0));
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio1), okay)
	gpio_esp32_fire_callbacks(DEVICE_DT_INST_GET(1));
#endif
}
