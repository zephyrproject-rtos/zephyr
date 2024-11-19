/*
 * Copyright (c) 2022-2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "analog.h"
#include <zephyr/drivers/pinctrl.h>
#if CONFIG_SOC_RISCV_TELINK_B91
#include <zephyr/dt-bindings/pinctrl/b91-pinctrl.h>
#elif CONFIG_SOC_RISCV_TELINK_B92
#include <zephyr/dt-bindings/pinctrl/b92-pinctrl.h>
#elif CONFIG_SOC_RISCV_TELINK_B95
#include <zephyr/dt-bindings/pinctrl/b95-pinctrl.h>
#endif
#include <zephyr/pm/device.h>

#define DT_DRV_COMPAT telink_b9x_pinctrl

#if CONFIG_SOC_RISCV_TELINK_B91 || CONFIG_SOC_RISCV_TELINK_B92
/**
 *      GPIO Function Enable Register
 *         ADDR              PINS
 *      gpio_en:          PORT_A[0-7]
 *      gpio_en + 1*8:    PORT_B[0-7]
 *      gpio_en + 2*8:    PORT_C[0-7]
 *      gpio_en + 3*8:    PORT_D[0-7]
 *      gpio_en + 4*8:    PORT_E[0-7]
 *      gpio_en + 5*8:    PORT_F[0-7]
 */
#define reg_gpio_en(pin) (*(volatile uint8_t *)((uint32_t)DT_INST_REG_ADDR_BY_NAME(0, gpio_en) + \
						((pin >> 8) * 8)))
#elif CONFIG_SOC_RISCV_TELINK_B95
/**
 *      GPIO Function Enable Register
 *      ADDR                 PINS
 *      gpio_en + 0*0x10:    PORT_A[0-7]
 *      gpio_en + 1*0x10:    PORT_B[0-7]
 *      gpio_en + 2*0x10:    PORT_C[0-7]
 *      gpio_en + 3*0x10:    PORT_D[0-7]
 *      gpio_en + 4*0x10:    PORT_E[0-7]
 *      gpio_en + 5*0x10:    PORT_F[0-7]
 */
#define reg_gpio_en(pin) (*(volatile uint8_t *)((uint32_t)DT_INST_REG_ADDR_BY_NAME(0, gpio_en) + \
						((pin >> 8) * 0x10)))
#endif

#if CONFIG_SOC_RISCV_TELINK_B91
/**
 *      Function Multiplexer Register
 *         ADDR              PINS
 *      pin_mux:          PORT_A[0-3]
 *      pin_mux + 1:      PORT_A[4-7]
 *      pin_mux + 2:      PORT_B[0-3]
 *      pin_mux + 3:      PORT_B[4-7]
 *      pin_mux + 4:      PORT_C[0-3]
 *      pin_mux + 5:      PORT_C[4-7]
 *      pin_mux + 6:      PORT_D[0-3]
 *      pin_mux + 7:      PORT_D[4-7]
 *      pin_mux + 0x20:   PORT_E[0-3]
 *      pin_mux + 0x21:   PORT_E[4-7]
 *      pin_mux + 0x26:   PORT_F[0-3]
 *      pin_mux + 0x27:   PORT_F[4-7]
 */
#define reg_pin_mux(pin) (*(volatile uint8_t *)((uint32_t)DT_INST_REG_ADDR_BY_NAME(0, pin_mux) + \
						(((pin >> 8) < 4)  ? ((pin >> 8) * 2) : 0) +	 \
						(((pin >> 8) == 4) ? 0x20          : 0) +	 \
						(((pin >> 8) == 5) ? 0x26          : 0) +	 \
						((pin & 0x0f0)     ? 1             : 0)))

#elif CONFIG_SOC_RISCV_TELINK_B92
/**
 *      Function Multiplexer Register
 *         ADDR              PINS
 *      pin_mux:          PORT_A[0]
 *      pin_mux + 1:      PORT_A[1]
 *      ...........       ...........
 *      pin_mux + 0x2E:   PORT_F[6]
 *      pin_mux + 0x2F:   PORT_F[7]
 */
#define reg_pin_mux(pin) (*(volatile uint8_t *)((uint32_t)DT_INST_REG_ADDR_BY_NAME(0, pin_mux) + \
						((pin >> 8) * 8) +            \
						((pin & B9x_PIN_1) ? 1 : 0) + \
						((pin & B9x_PIN_2) ? 2 : 0) + \
						((pin & B9x_PIN_3) ? 3 : 0) + \
						((pin & B9x_PIN_4) ? 4 : 0) + \
						((pin & B9x_PIN_5) ? 5 : 0) + \
						((pin & B9x_PIN_6) ? 6 : 0) + \
						((pin & B9x_PIN_7) ? 7 : 0)))
#elif CONFIG_SOC_RISCV_TELINK_B95
/**
 *      Function Multiplexer Register
 *         ADDR              PINS
 *      pin_mux:          PORT_A[0]
 *      pin_mux + 1:      PORT_A[1]
 *      ...........       ...........
 *      pin_mux + 0x2E:   PORT_F[6]
 *      pin_mux + 0x2F:   PORT_F[7]
 */

/* Return the bit index of the lowest 1 in y. ex: 0b00110111000 --> 3 */
#define PINCTR_BIT_LOW_BIT(y) \
		(((y) & BIT(0))  ? 0  : (((y) & BIT(1))  ?  1 : (((y) & BIT(2))  ?  2 : \
		(((y) & BIT(3))  ? 3  : (((y) & BIT(4))  ?  4 : (((y) & BIT(5))  ?  5 : \
		(((y) & BIT(6))  ? 6  : (((y) & BIT(7))  ?  7 : (((y) & BIT(8))  ?  8 : \
		(((y) & BIT(9))  ? 9  : (((y) & BIT(10)) ? 10 : (((y) & BIT(11)) ? 11 : \
		(((y) & BIT(12)) ? 12 : (((y) & BIT(13)) ? 13 : (((y) & BIT(14)) ? 14 : \
		(((y) & BIT(15)) ? 15 : (((y) & BIT(16)) ? 16 : (((y) & BIT(17)) ? 17 : \
		(((y) & BIT(18)) ? 18 : (((y) & BIT(19)) ? 19 : (((y) & BIT(20)) ? 20 : \
		(((y) & BIT(21)) ? 21 : (((y) & BIT(22)) ? 22 : (((y) & BIT(23)) ? 23 : \
		(((y) & BIT(24)) ? 24 : (((y) & BIT(25)) ? 25 : (((y) & BIT(26)) ? 26 : \
		(((y) & BIT(27)) ? 27 : (((y) & BIT(28)) ? 28 : (((y) & BIT(29)) ? 29 : \
		(((y) & BIT(30)) ? 30 : (((y) & BIT(31)) ? 31 : 32 \
		))))))))))))))))))))))))))))))))

#define reg_pin_mux(pin) \
		(*(volatile uint8_t *)((uint32_t)DT_INST_REG_ADDR_BY_NAME(0, pin_mux) + \
					((pin >> 8) * 8) + \
					PINCTR_BIT_LOW_BIT(pin) \
		))
#endif

/**
 *      Pull Up resistors enable
 *          ADDR               PINS
 *      pull_up_en:         PORT_A[0-3]
 *      pull_up_en + 1:     PORT_A[4-7]
 *      pull_up_en + 2:     PORT_B[0-3]
 *      pull_up_en + 3:     PORT_B[4-7]
 *      pull_up_en + 4:     PORT_C[0-3]
 *      pull_up_en + 5:     PORT_C[4-7]
 *      pull_up_en + 6:     PORT_D[0-3]
 *      pull_up_en + 7:     PORT_D[4-7]
 *      pull_up_en + 8:     PORT_E[0-3]
 *      pull_up_en + 9:     PORT_E[4-7]
 *      pull_up_en + 10:    PORT_F[0-3]
 *      pull_up_en + 11:    PORT_F[4-7]
 */
#define reg_pull_up_en(pin) ((uint8_t)(DT_INST_REG_ADDR_BY_NAME(0, pull_up_en) + \
				       ((pin >> 8) * 2) +			 \
				       ((pin & 0xf0) ? 1 : 0)))


#if CONFIG_PM_DEVICE && CONFIG_SOC_SERIES_RISCV_TELINK_B9X_RETENTION

static int pinctrl_b9x_init(const struct device *dev)
{
	ARG_UNUSED(dev);
#if CONFIG_SOC_RISCV_TELINK_B91 || CONFIG_SOC_RISCV_TELINK_B92
	/* set pad_mul_sel register value from dts */
	reg_gpio_pad_mul_sel |= DT_INST_PROP(0, pad_mul_sel);
#endif
	return 0;
}

static int pinctrl_b9x_pm_action(const struct device *dev, enum pm_device_action action)
{
	ARG_UNUSED(dev);

	extern volatile bool b9x_deep_sleep_retention;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		if (b9x_deep_sleep_retention) {
#if CONFIG_SOC_RISCV_TELINK_B91 || CONFIG_SOC_RISCV_TELINK_B92
			/* set pad_mul_sel register value from dts */
			reg_gpio_pad_mul_sel |= DT_INST_PROP(0, pad_mul_sel);
#endif
		}
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

PM_DEVICE_DEFINE(pinctrl_b9x_pm, pinctrl_b9x_pm_action);
DEVICE_DEFINE(pinctrl_b9x, "pinctrl_b9x", pinctrl_b9x_init, PM_DEVICE_GET(pinctrl_b9x_pm),
	NULL, NULL, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

#else

/* Pinctrl driver initialization */
static int pinctrl_b9x_init(void)
{
#if CONFIG_SOC_RISCV_TELINK_B91 || CONFIG_SOC_RISCV_TELINK_B92
	/* set pad_mul_sel register value from dts */
	reg_gpio_pad_mul_sel |= DT_INST_PROP(0, pad_mul_sel);
#endif
	return 0;
}

SYS_INIT(pinctrl_b9x_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /* CONFIG_PM_DEVICE && CONFIG_SOC_SERIES_RISCV_TELINK_B9X_RETENTION */

/* Act as GPIO function disable */
static inline void pinctrl_b9x_gpio_function_disable(uint32_t pin)
{
	uint8_t bit = pin & 0xff;

	reg_gpio_en(pin) &= ~bit;
}

/* Get pull up (and function for B91) value bits start position (offset) */
static inline int pinctrl_b9x_get_offset(uint32_t pin, uint8_t *offset)
{
	switch (B9x_PINMUX_GET_PIN_ID(pin)) {
	case B9x_PIN_0:
		*offset = B9x_PIN_0_PULL_UP_EN_POS;
		break;
	case B9x_PIN_1:
		*offset = B9x_PIN_1_PULL_UP_EN_POS;
		break;
	case B9x_PIN_2:
		*offset = B9x_PIN_2_PULL_UP_EN_POS;
		break;
	case B9x_PIN_3:
		*offset = B9x_PIN_3_PULL_UP_EN_POS;
		break;
	case B9x_PIN_4:
		*offset = B9x_PIN_4_PULL_UP_EN_POS;
		break;
	case B9x_PIN_5:
		*offset = B9x_PIN_5_PULL_UP_EN_POS;
		break;
	case B9x_PIN_6:
		*offset = B9x_PIN_6_PULL_UP_EN_POS;
		break;
	case B9x_PIN_7:
		*offset = B9x_PIN_7_PULL_UP_EN_POS;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

/* Set pin's function */
static int pinctrl_configure_pin(const pinctrl_soc_pin_t *pinctrl)
{
	int status;
	uint8_t mask;
	uint8_t offset = 0;
	uint8_t pull = B9x_PINMUX_GET_PULL(*pinctrl);
	uint8_t func = B9x_PINMUX_GET_FUNC(*pinctrl);
	uint32_t pin = B9x_PINMUX_GET_PIN(*pinctrl);
	uint8_t pull_up_en_addr = reg_pull_up_en(pin);

	/* calculate offset and mask for the func and pull values */
	status = pinctrl_b9x_get_offset(pin, &offset);
	if (status != 0) {
		return status;
	}

	mask = (uint8_t) ~(BIT(offset) | BIT(offset + 1));

	/* set func value */
#if CONFIG_SOC_RISCV_TELINK_B91
	func = func << offset;
	reg_pin_mux(pin) = (reg_pin_mux(pin) & mask) | func;
#elif CONFIG_SOC_RISCV_TELINK_B92
	reg_pin_mux(pin) = (reg_pin_mux(pin) & (~B92_PIN_FUNC_POS)) | (func & B92_PIN_FUNC_POS);
#elif CONFIG_SOC_RISCV_TELINK_B95
	reg_pin_mux(pin) = (reg_pin_mux(pin) & (~B95_PIN_FUNC_POS)) | (func & B95_PIN_FUNC_POS);
#endif

	/* disable GPIO function (can be enabled back by GPIO init using GPIO driver) */
	pinctrl_b9x_gpio_function_disable(pin);

	/* set pull value */
	pull = pull << offset;
	analog_write_reg8(pull_up_en_addr, (analog_read_reg8(pull_up_en_addr) & mask) | pull);

	return status;
}

/* API implementation: configure_pins */
int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	int status = 0;

	for (uint8_t i = 0; i < pin_cnt; i++) {
		status = pinctrl_configure_pin(pins++);
		if (status < 0) {
			break;
		}
	}

	return status;
}
