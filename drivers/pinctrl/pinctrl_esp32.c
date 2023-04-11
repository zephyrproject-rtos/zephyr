/*
 * Copyright (c) 2022 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pinctrl/pinctrl_esp32_common.h>

#include <soc.h>
#include <hal/gpio_ll.h>
#include <hal/rtc_io_hal.h>

#ifdef CONFIG_SOC_ESP32C3
/* gpio structs in esp32c3 series are different from xtensa ones */
#define out out.data
#define in in.data
#define out_w1ts out_w1ts.val
#define out_w1tc out_w1tc.val
#endif

#ifndef SOC_GPIO_SUPPORT_RTC_INDEPENDENT
#define SOC_GPIO_SUPPORT_RTC_INDEPENDENT 0
#endif

#define ESP32_INVALID_PORT_ADDR          0UL

#define ESP32_GPIO_PORT_ADDR(nodelabel)				\
	COND_CODE_1(DT_NODE_EXISTS(DT_NODELABEL(nodelabel)),	\
		(DT_REG_ADDR(DT_NODELABEL(nodelabel)),),	\
		(ESP32_INVALID_PORT_ADDR))

/**
 * @brief Array containing each GPIO port address.
 *
 * Entries will be an invalid address if the port is not enabled.
 */
static const uint32_t esp32_gpio_ports_addrs[] = {
	ESP32_GPIO_PORT_ADDR(gpio0)
	ESP32_GPIO_PORT_ADDR(gpio1)
};

/** Number of GPIO ports. */
static const size_t esp32_gpio_ports_cnt = ARRAY_SIZE(esp32_gpio_ports_addrs);

static inline bool rtc_gpio_is_valid_gpio(uint32_t gpio_num)
{
#if SOC_RTCIO_INPUT_OUTPUT_SUPPORTED
	return (gpio_num < SOC_GPIO_PIN_COUNT && rtc_io_num_map[gpio_num] >= 0);
#else
	return false;
#endif
}

static inline bool esp32_pin_is_valid(uint32_t pin)
{
	return ((BIT(pin) & SOC_GPIO_VALID_GPIO_MASK) != 0);
}

static inline bool esp32_pin_is_output_capable(uint32_t pin)
{
	return ((BIT(pin) & SOC_GPIO_VALID_OUTPUT_GPIO_MASK) != 0);
}

static int esp32_pin_apply_config(uint32_t pin, uint32_t flags)
{
	gpio_dev_t *const gpio_base = (gpio_dev_t *)DT_REG_ADDR(DT_NODELABEL(gpio0));
	uint32_t io_pin = (uint32_t) pin + ((ESP32_PORT_IDX(pin) == 1 && pin < 32) ? 32 : 0);
	int ret = 0;

	if (!esp32_pin_is_valid(io_pin)) {
		return -EINVAL;
	}

#if SOC_RTCIO_INPUT_OUTPUT_SUPPORTED
	if (rtc_gpio_is_valid_gpio(io_pin)) {
		rtcio_hal_function_select(rtc_io_num_map[io_pin], RTCIO_FUNC_DIGITAL);
	}
#endif

	if (io_pin >= GPIO_NUM_MAX) {
		ret = -EINVAL;
		goto end;
	}

	/* Set pin function as GPIO */
	gpio_ll_iomux_func_sel(GPIO_PIN_MUX_REG[io_pin], PIN_FUNC_GPIO);

	if (flags & ESP32_PULL_UP_FLAG) {
		if (!rtc_gpio_is_valid_gpio(io_pin) || SOC_GPIO_SUPPORT_RTC_INDEPENDENT) {
			gpio_ll_pulldown_dis(&GPIO, io_pin);
			gpio_ll_pullup_en(&GPIO, io_pin);
		} else {
#if SOC_RTCIO_INPUT_OUTPUT_SUPPORTED
			int rtcio_num = rtc_io_num_map[io_pin];

			rtcio_hal_pulldown_disable(rtc_io_num_map[io_pin]);

			if (rtc_io_desc[rtcio_num].pullup) {
				rtcio_hal_pullup_enable(rtc_io_num_map[io_pin]);
			} else {
				ret = -ENOTSUP;
				goto end;
			}
#endif
		}
	} else if (flags & ESP32_PULL_DOWN_FLAG) {
		if (!rtc_gpio_is_valid_gpio(io_pin) || SOC_GPIO_SUPPORT_RTC_INDEPENDENT) {
			gpio_ll_pullup_dis(&GPIO, io_pin);
			gpio_ll_pulldown_en(&GPIO, io_pin);
		} else {
#if SOC_RTCIO_INPUT_OUTPUT_SUPPORTED
			int rtcio_num = rtc_io_num_map[io_pin];

			rtcio_hal_pulldown_enable(rtc_io_num_map[io_pin]);

			if (rtc_io_desc[rtcio_num].pullup) {
				rtcio_hal_pullup_disable(rtc_io_num_map[io_pin]);
			} else {
				ret = -ENOTSUP;
				goto end;
			}
#endif
		}
	}

	if (flags & ESP32_DIR_OUT_FLAG) {
		if (!esp32_pin_is_output_capable(pin)) {
			ret = -EINVAL;
			goto end;
		}

		if (flags & ESP32_OPEN_DRAIN_FLAG) {
			gpio_ll_od_enable(gpio_base, io_pin);
		} else {
			gpio_ll_od_disable(gpio_base, io_pin);
		}

		/* Set output pin initial value */
		if (flags & ESP32_PIN_OUT_HIGH_FLAG) {
			gpio_ll_set_level(gpio_base, io_pin, 1);
		} else if (flags & ESP32_PIN_OUT_LOW_FLAG) {
			gpio_ll_set_level(gpio_base, io_pin, 0);
		}

		gpio_ll_output_enable(&GPIO, io_pin);
		esp_rom_gpio_matrix_out(io_pin, SIG_GPIO_OUT_IDX, false, false);
	} else {
		gpio_ll_output_disable(&GPIO, io_pin);
	}

	if (flags & ESP32_DIR_INP_FLAG) {
		gpio_ll_input_enable(&GPIO, io_pin);
	} else {
		gpio_ll_input_disable(&GPIO, io_pin);
	}

end:
	return ret;
}

static int esp32_pin_configure(const uint32_t pin_mux, const uint32_t pin_cfg)
{
	uint32_t port_addr;
	uint32_t pin_num = ESP32_PIN_NUM(pin_mux);
	uint32_t sig_in = ESP32_PIN_SIGI(pin_mux);
	uint32_t sig_out = ESP32_PIN_SIGO(pin_mux);
	uint32_t flags = 0;

	if (ESP32_PORT_IDX(pin_num) >= esp32_gpio_ports_cnt) {
		return -EINVAL;
	}

	port_addr = esp32_gpio_ports_addrs[ESP32_PORT_IDX(pin_num)];

	if (port_addr == ESP32_INVALID_PORT_ADDR) {
		return -EINVAL;
	}

	switch (ESP32_PIN_BIAS(pin_cfg)) {
	case ESP32_PULL_UP:
		flags |= ESP32_PULL_UP_FLAG;
		break;
	case ESP32_PULL_DOWN:
		flags |= ESP32_PULL_DOWN_FLAG;
		break;
	default:
		break;
	}

	switch (ESP32_PIN_DRV(pin_cfg)) {
	case ESP32_PUSH_PULL:
		flags |= ESP32_PUSH_PULL_FLAG;
		break;
	case ESP32_OPEN_DRAIN:
		flags |= ESP32_OPEN_DRAIN_FLAG;
		break;
	default:
		break;
	}

	if (sig_in == ESP_SIG_INVAL && sig_out == ESP_SIG_INVAL) {
		return -ENOTSUP;
	}

	if (sig_in != ESP_SIG_INVAL) {
		flags |= ESP32_DIR_INP_FLAG;
	}

	if (sig_out != ESP_SIG_INVAL) {
		flags |= ESP32_DIR_OUT_FLAG;
	}

	switch (ESP32_PIN_MODE_OUT(pin_cfg)) {
	case ESP32_PIN_OUT_HIGH:
		flags |= ESP32_PIN_OUT_HIGH_FLAG;
		break;
	case ESP32_PIN_OUT_LOW:
		flags |= ESP32_PIN_OUT_LOW_FLAG;
		break;
	default:
		break;
	}

	if (flags & ESP32_PIN_OUT_HIGH_FLAG) {
		if (ESP32_PORT_IDX(pin_num) == 0) {
			gpio_dev_t *const gpio_dev =
				(gpio_dev_t *)DT_REG_ADDR(DT_NODELABEL(gpio0));
			gpio_dev->out_w1ts = pin_num;
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio1), okay)
		} else {
			gpio_dev_t *const gpio_dev =
				(gpio_dev_t *)DT_REG_ADDR(DT_NODELABEL(gpio1));
			gpio_dev->out1_w1ts.data = pin_num;
#endif
		}
	}

	if (flags & ESP32_PIN_OUT_LOW_FLAG) {
		if (ESP32_PORT_IDX(pin_num) == 0) {
			gpio_dev_t *const gpio_dev =
				(gpio_dev_t *)DT_REG_ADDR(DT_NODELABEL(gpio0));
			gpio_dev->out_w1tc = pin_num;
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio1), okay)
		} else {
			gpio_dev_t *const gpio_dev =
				(gpio_dev_t *)DT_REG_ADDR(DT_NODELABEL(gpio1));
			gpio_dev->out1_w1tc.data = pin_num;
#endif
		}
	}

	esp32_pin_apply_config(pin_num, flags);

	if (flags & ESP32_DIR_OUT_FLAG) {
		esp_rom_gpio_matrix_out(pin_num, sig_out, 0, 0);
	}

	if (flags & ESP32_DIR_INP_FLAG) {
		esp_rom_gpio_matrix_in(pin_num, sig_in, 0);
	}

	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	uint32_t pin_mux, pin_cfg;
	int ret = 0;

	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		pin_mux = pins[i].pinmux;
		pin_cfg = pins[i].pincfg;

		ret = esp32_pin_configure(pin_mux, pin_cfg);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}
