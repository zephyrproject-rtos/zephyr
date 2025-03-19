/*
 * Copyright (C) 2024-2025, Xiaohua Semiconductor Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <hc32_ll.h>

#define HC32_GPIO_PORT_VALID(nodelabel)				\
		(DT_NODE_EXISTS(DT_NODELABEL(nodelabel)))?	\
		(DT_NODE_HAS_STATUS(DT_NODELABEL(nodelabel), okay)):	\
		(0)

/**
 * @brief Array containing bool value to each GPIO port.
 *
 * Entries will be 0 if the GPIO port is not enabled.
 */
static volatile uint8_t gpio_ports_valid[] = {
	HC32_GPIO_PORT_VALID(gpioa),
	HC32_GPIO_PORT_VALID(gpiob),
	HC32_GPIO_PORT_VALID(gpioc),
	HC32_GPIO_PORT_VALID(gpiod),
	HC32_GPIO_PORT_VALID(gpioe),
	HC32_GPIO_PORT_VALID(gpiof),
	HC32_GPIO_PORT_VALID(gpiog),
	HC32_GPIO_PORT_VALID(gpioh),
	HC32_GPIO_PORT_VALID(gpioi),
};

static inline uint8_t HC32_get_port(uint8_t port_num)
{
#if defined(HC32F460)
	if ('H' == (port_num + 'A'))
		return GPIO_PORT_H;
	else
		return port_num;
#elif defined(HC32F4A0)
	return port_num;
#endif
}

static inline bool HC32_pin_is_valid(uint16_t pin)
{
	return ((pin & GPIO_PIN_ALL) == 0);
}

static int hc32_pin_configure(const uint32_t pin_mux)
{
	uint8_t port_num = HC32_PORT(pin_mux);
	uint16_t pin_num = BIT(HC32_PIN(pin_mux));
	uint8_t mode = HC32_MODE(pin_mux);
	uint16_t func_num = HC32_FUNC_NUM(pin_mux);
	stc_gpio_init_t stc_gpio_init;

	GPIO_StructInit(&stc_gpio_init);

	if ((gpio_ports_valid[port_num] != 1) || (HC32_pin_is_valid(pin_num))) {
		return -EINVAL;
	}
	port_num = HC32_get_port(port_num);

	switch (mode) {
	case HC32_ANALOG:
		stc_gpio_init.u16PinAttr = PIN_ATTR_ANALOG;
		goto end;
		break;

	case HC32_GPIO:
		func_num = 0;
		stc_gpio_init.u16PinAttr = PIN_ATTR_DIGITAL;
		if (HC32_INPUT_ENABLE == HC32_PIN_EN_DIR(pin_mux)) {
			/* input */
			stc_gpio_init.u16PinDir = PIN_DIR_IN;
		} else {
			/* output */
			stc_gpio_init.u16PinDir = PIN_DIR_OUT;
			if (HC32_OUTPUT_HIGH == HC32_OUT_LEVEL(pin_mux)) {
				stc_gpio_init.u16PinState = PIN_STAT_SET;
			}
		}
		break;

	case HC32_FUNC:
		GPIO_SetFunc(port_num, pin_num, func_num);
		break;

	case HC32_SUBFUNC:
		GPIO_SetSubFunc(func_num);
		GPIO_SubFuncCmd(port_num, pin_num, ENABLE);
		break;

	default:
		break;
	}

	if (HC32_PULL_UP == HC32_PIN_BIAS(pin_mux)) {
		stc_gpio_init.u16PullUp = PIN_PU_ON;
	}
	if (HC32_PUSH_PULL == HC32_PIN_DRV(pin_mux)) {
		stc_gpio_init.u16PinOutputType = PIN_OUT_TYPE_CMOS;
	} else {
		stc_gpio_init.u16PinOutputType = PIN_OUT_TYPE_NMOS;
	}

	switch (HC32_PIN_DRIVER_STRENGTH(pin_mux)) {
	case HC32_DRIVER_STRENGTH_LOW:
		stc_gpio_init.u16PinDrv = PIN_LOW_DRV;
		break;

	case HC32_DRIVER_STRENGTH_MEDIUM:
		stc_gpio_init.u16PinDrv = PIN_MID_DRV;
		break;

	case HC32_DRIVER_STRENGTH_HIGH:
		stc_gpio_init.u16PinDrv = PIN_HIGH_DRV;
		break;

	default:
		break;
	}
#if defined(HC32F4A0)
	stc_gpio_init.u16PinInputType = (HC32_CINSEL(pin_mux) == HC32_CINSEL_SCHMITT) ?
					PIN_IN_TYPE_SMT : PIN_IN_TYPE_CMOS;
#endif

	stc_gpio_init.u16Invert = (HC32_INVERT_ENABLE == HC32_INVERT(pin_mux))? PIN_INVT_ON: PIN_INVT_OFF;

end:
	return GPIO_Init(port_num, pin_num, &stc_gpio_init);
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	int ret = 0;

	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		ret = hc32_pin_configure(pins[i]);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}
