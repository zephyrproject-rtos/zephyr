/*
 * Copyright (c) 2021 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/gd32.h>
#include <zephyr/drivers/pinctrl.h>

#include <gd32_gpio.h>

/** AFIO DT node */
#define AFIO_NODE DT_NODELABEL(afio)

/** GPIO mode: input floating (CTL bits) */
#define GPIO_MODE_INP_FLOAT 0x4U
/** GPIO mode: input with pull-up/down (CTL bits) */
#define GPIO_MODE_INP_PUPD 0x8U
/** GPIO mode: output push-pull (CTL bits) */
#define GPIO_MODE_ALT_PP 0x8U
/** GPIO mode: output open-drain (CTL bits) */
#define GPIO_MODE_ALT_OD 0xCU

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
};

/**
 * @brief Initialize AFIO
 *
 * This function enables AFIO clock and configures the I/O compensation if
 * available and enabled in Devicetree.
 *
 * @retval 0 Always
 */
static int afio_init(const struct device *dev)
{
	uint16_t clkid = DT_CLOCKS_CELL(AFIO_NODE, id);

	ARG_UNUSED(dev);

	(void)clock_control_on(GD32_CLOCK_CONTROLLER,
			       (clock_control_subsys_t *)&clkid);

#ifdef AFIO_CPSCTL
	if (DT_PROP(AFIO_NODE, enable_cps)) {
		gpio_compensation_config(GPIO_COMPENSATION_ENABLE);
		while (gpio_compensation_flag_get() == RESET)
			;
	}
#endif /* AFIO_CPSCTL */

	return 0;
}

SYS_INIT(afio_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

/**
 * @brief Helper function to configure the SPD register if available.
 *
 * @param port GPIO port address.
 * @param pin_bit GPIO pin, set as bit position.
 * @param speed GPIO speed.
 *
 * @return Value of the mode register (speed) that should be set later.
 */
static inline uint8_t configure_spd(uint32_t port, uint32_t pin_bit,
				    uint8_t speed)
{
	switch (speed) {
	case GD32_OSPEED_MAX:
#ifdef GPIOx_SPD
		GPIOx_SPD(port) |= pin_bit;
#endif /* GPIOx_SPD */
		return speed;
	default:
#ifdef GPIOx_SPD
		GPIOx_SPD(port) &= ~pin_bit;
#endif /* GPIOx_SPD */
		return speed + 1U;
	}
}

/**
 * @brief Configure a pin.
 *
 * @param pin The pin to configure.
 */
static void configure_pin(pinctrl_soc_pin_t pin)
{
	uint8_t port_idx, mode, pin_num;
	uint32_t port, pin_bit, reg_val;
	volatile uint32_t *reg;
	uint16_t clkid;

	port_idx = GD32_PORT_GET(pin);
	__ASSERT_NO_MSG(port_idx < ARRAY_SIZE(gd32_port_addrs));

	clkid = gd32_port_clkids[port_idx];
	port = gd32_port_addrs[port_idx];
	pin_num = GD32_PIN_GET(pin);
	pin_bit = BIT(pin_num);
	mode = GD32_MODE_GET(pin);

	if (pin_num < 8U) {
		reg = &GPIO_CTL0(port);
	} else {
		reg = &GPIO_CTL1(port);
		pin_num -= 8U;
	}

	(void)clock_control_on(GD32_CLOCK_CONTROLLER,
			       (clock_control_subsys_t *)&clkid);

	reg_val = *reg;
	reg_val &= ~GPIO_MODE_MASK(pin_num);

	if (mode == GD32_MODE_ALTERNATE) {
		uint8_t mode;

		mode = configure_spd(port, pin_bit, GD32_OSPEED_GET(pin));

		if (GD32_OTYPE_GET(pin) == GD32_OTYPE_PP) {
			mode |= GPIO_MODE_ALT_PP;
		} else {
			mode |= GPIO_MODE_ALT_OD;
		}

		reg_val |= GPIO_MODE_SET(pin_num, mode);
	} else if (mode == GD32_MODE_GPIO_IN) {
		uint8_t pupd = GD32_PUPD_GET(pin);

		if (pupd == GD32_PUPD_NONE) {
			reg_val |= GPIO_MODE_SET(pin_num, GPIO_MODE_INP_FLOAT);
		} else {
			reg_val |= GPIO_MODE_SET(pin_num, GPIO_MODE_INP_PUPD);

			if (pupd == GD32_PUPD_PULLDOWN) {
				GPIO_BC(port) = pin_bit;
			} else if (pupd == GD32_PUPD_PULLUP) {
				GPIO_BOP(port) = pin_bit;
			}
		}
	}

	*reg = reg_val;
}

/**
 * @brief Configure remap.
 *
 * @param remap Remap bit field as encoded by #GD32_REMAP.
 */
static void configure_remap(uint16_t remap)
{
	uint8_t pos;
	uint32_t reg_val;
	volatile uint32_t *reg;

	/* not remappable */
	if (remap == GD32_NORMP) {
		return;
	}

	if (GD32_REMAP_REG_GET(remap) == 0U) {
		reg = &AFIO_PCF0;
	} else {
		reg = &AFIO_PCF1;
	}

	pos = GD32_REMAP_POS_GET(remap);

	reg_val = *reg;
	reg_val &= ~(GD32_REMAP_MSK_GET(remap) << pos);
	reg_val |= GD32_REMAP_VAL_GET(remap) << pos;
	*reg = reg_val;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	ARG_UNUSED(reg);

	if (pin_cnt == 0U) {
		return -EINVAL;
	}

	/* same remap is encoded in all pins, so just pick the first */
	configure_remap(GD32_REMAP_GET(pins[0]));

	/* configure all pins */
	for (uint8_t i = 0U; i < pin_cnt; i++) {
		configure_pin(pins[i]);
	}

	return 0;
}
