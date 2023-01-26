/*
 * Copyright (c) 2021 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/gd32.h>
#include <zephyr/drivers/pinctrl.h>

#include <gd32_gpio.h>

BUILD_ASSERT((GD32_PUPD_NONE == GPIO_PUPD_NONE) &&
	     (GD32_PUPD_PULLUP == GPIO_PUPD_PULLUP) &&
	     (GD32_PUPD_PULLDOWN == GPIO_PUPD_PULLDOWN),
	     "pinctrl pull-up/down definitions != HAL definitions");

BUILD_ASSERT((GD32_OTYPE_PP == GPIO_OTYPE_PP) &&
	     (GD32_OTYPE_OD == GPIO_OTYPE_OD),
	     "pinctrl output type definitions != HAL definitions");

BUILD_ASSERT((GD32_OSPEED_2MHZ == GPIO_OSPEED_2MHZ) &&
#if defined(CONFIG_SOC_SERIES_GD32F3X0) || \
	defined(CONFIG_SOC_SERIES_GD32A50X) || \
	defined(CONFIG_SOC_SERIES_GD32L23X)
	     (GD32_OSPEED_10MHZ == GPIO_OSPEED_10MHZ) &&
	     (GD32_OSPEED_50MHZ == GPIO_OSPEED_50MHZ) &&
#else
	     (GD32_OSPEED_25MHZ == GPIO_OSPEED_25MHZ) &&
	     (GD32_OSPEED_50MHZ == GPIO_OSPEED_50MHZ) &&
	     (GD32_OSPEED_MAX == GPIO_OSPEED_MAX) &&
#endif
	     1U,
	     "pinctrl output speed definitions != HAL definitions");

/** Utility macro that expands to the GPIO port address if it exists */
#define GD32_PORT_ADDR_OR_NONE(nodelabel)				       \
	COND_CODE_1(DT_NODE_EXISTS(DT_NODELABEL(nodelabel)),		       \
		   (DT_REG_ADDR(DT_NODELABEL(nodelabel)),), ())

/** Utility macro that expands to the GPIO clock id if it exists */
#define GD32_PORT_CLOCK_ID_OR_NONE(nodelabel)				       \
	COND_CODE_1(DT_NODE_EXISTS(DT_NODELABEL(nodelabel)),		       \
		   (DT_CLOCKS_CELL(DT_NODELABEL(nodelabel), id),), ())

/** GD32 port addresses */
static const uint32_t gd32_port_addrs[] = {
	GD32_PORT_ADDR_OR_NONE(gpioa)
	GD32_PORT_ADDR_OR_NONE(gpiob)
	GD32_PORT_ADDR_OR_NONE(gpioc)
	GD32_PORT_ADDR_OR_NONE(gpiod)
	GD32_PORT_ADDR_OR_NONE(gpioe)
	GD32_PORT_ADDR_OR_NONE(gpiof)
	GD32_PORT_ADDR_OR_NONE(gpiog)
	GD32_PORT_ADDR_OR_NONE(gpioh)
	GD32_PORT_ADDR_OR_NONE(gpioi)
};

/** GD32 port clock identifiers */
static const uint16_t gd32_port_clkids[] = {
	GD32_PORT_CLOCK_ID_OR_NONE(gpioa)
	GD32_PORT_CLOCK_ID_OR_NONE(gpiob)
	GD32_PORT_CLOCK_ID_OR_NONE(gpioc)
	GD32_PORT_CLOCK_ID_OR_NONE(gpiod)
	GD32_PORT_CLOCK_ID_OR_NONE(gpioe)
	GD32_PORT_CLOCK_ID_OR_NONE(gpiof)
	GD32_PORT_CLOCK_ID_OR_NONE(gpiog)
	GD32_PORT_CLOCK_ID_OR_NONE(gpioh)
	GD32_PORT_CLOCK_ID_OR_NONE(gpioi)
};

/**
 * @brief Configure a pin.
 *
 * @param pin The pin to configure.
 */
static void pinctrl_configure_pin(pinctrl_soc_pin_t pin)
{
	uint8_t port_idx;
	uint32_t port, pin_num, af, mode;
	uint16_t clkid;

	port_idx = GD32_PORT_GET(pin);
	__ASSERT_NO_MSG(port_idx < ARRAY_SIZE(gd32_port_addrs));

	clkid = gd32_port_clkids[port_idx];
	port = gd32_port_addrs[port_idx];
	pin_num = BIT(GD32_PIN_GET(pin));
	af = GD32_AF_GET(pin);

	(void)clock_control_on(GD32_CLOCK_CONTROLLER,
			       (clock_control_subsys_t *)&clkid);

	if (af != GD32_ANALOG) {
		mode = GPIO_MODE_AF;
		gpio_af_set(port, af, pin_num);
	} else {
		mode = GPIO_MODE_ANALOG;
	}

	gpio_mode_set(port, mode, GD32_PUPD_GET(pin), pin_num);
	gpio_output_options_set(port, GD32_OTYPE_GET(pin),
				GD32_OSPEED_GET(pin), pin_num);
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		pinctrl_configure_pin(pins[i]);
	}

	return 0;
}
