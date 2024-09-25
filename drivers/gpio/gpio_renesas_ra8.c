/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra8_gpio

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/gpio/renesas-ra8-gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/irq.h>
#include <soc.h>

struct gpio_ra8_config {
	struct gpio_driver_config common;
	uint8_t port_num;
	R_PORT0_Type *port;
	gpio_pin_t vbatt_pins[];
};

struct gpio_ra8_data {
	struct gpio_driver_data common;
};

static int gpio_ra8_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_ra8_config *config = dev->config;

	struct ra_pinctrl_soc_pin pincfg = {0};

	if (((flags & GPIO_INPUT) != 0U) && ((flags & GPIO_OUTPUT) != 0U)) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_PULL_DOWN) != 0U) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_INT_ENABLE) != 0) {
		return -ENOTSUP;
	}

	if (config->vbatt_pins[0] != 0xFF) {
		uint32_t clear = 0;

		for (int i = 0; config->vbatt_pins[i] != '\0'; i++) {
			if (config->vbatt_pins[i] == pin) {
				WRITE_BIT(clear, i, 1);
			}
		}

		R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_OM_LPC_BATT);

		R_SYSTEM->VBTICTLR &= (uint8_t)~clear;

		R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_OM_LPC_BATT);
	}

	pincfg.port_num = config->port_num;
	pincfg.pin_num = pin;

	/* Change mode to general IO mode and disable IRQ and Analog input */
	WRITE_BIT(pincfg.cfg, R_PFS_PORT_PIN_PmnPFS_PMR_Pos, 0);
	WRITE_BIT(pincfg.cfg, R_PFS_PORT_PIN_PmnPFS_ASEL_Pos, 0);
	WRITE_BIT(pincfg.cfg, R_PFS_PORT_PIN_PmnPFS_ISEL_Pos, 0);

	if ((flags & GPIO_OUTPUT) != 0U) {
		/* Set output pin initial value */
		if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
			WRITE_BIT(pincfg.cfg, R_PFS_PORT_PIN_PmnPFS_PODR_Pos, 0);
		} else if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
			WRITE_BIT(pincfg.cfg, R_PFS_PORT_PIN_PmnPFS_PODR_Pos, 1);
		}

		WRITE_BIT(pincfg.cfg, R_PFS_PORT_PIN_PmnPFS_PDR_Pos, 1);
	} else {
		WRITE_BIT(pincfg.cfg, R_PFS_PORT_PIN_PmnPFS_PDR_Pos, 0);
	}

	if ((flags & GPIO_LINE_OPEN_DRAIN) != 0) {
		WRITE_BIT(pincfg.cfg, R_PFS_PORT_PIN_PmnPFS_NCODR_Pos, 1);
	}

	if ((flags & GPIO_PULL_UP) != 0) {
		WRITE_BIT(pincfg.cfg, R_PFS_PORT_PIN_PmnPFS_PCR_Pos, 1);
	}

	pincfg.cfg = pincfg.cfg |
		     (((flags & RENESAS_GPIO_DS_MSK) >> 8) << R_PFS_PORT_PIN_PmnPFS_DSCR_Pos);

	return pinctrl_configure_pins(&pincfg, 1, PINCTRL_REG_NONE);
}

static int gpio_ra8_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_ra8_config *config = dev->config;
	R_PORT0_Type *port = config->port;

	*value = port->PIDR;

	return 0;
}

static int gpio_ra8_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					gpio_port_value_t value)
{
	const struct gpio_ra8_config *config = dev->config;
	R_PORT0_Type *port = config->port;

	port->PODR = ((port->PODR & ~mask) | (value & mask));

	return 0;
}

static int gpio_ra8_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_ra8_config *config = dev->config;
	R_PORT0_Type *port = config->port;

	port->PODR = (port->PODR | pins);

	return 0;
}

static int gpio_ra8_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_ra8_config *config = dev->config;
	R_PORT0_Type *port = config->port;

	port->PODR = (port->PODR & ~pins);

	return 0;
}

static int gpio_ra8_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_ra8_config *config = dev->config;
	R_PORT0_Type *port = config->port;

	port->PODR = (port->PODR ^ pins);

	return 0;
}

static const struct gpio_driver_api gpio_ra8_drv_api_funcs = {
	.pin_configure = gpio_ra8_pin_configure,
	.port_get_raw = gpio_ra8_port_get_raw,
	.port_set_masked_raw = gpio_ra8_port_set_masked_raw,
	.port_set_bits_raw = gpio_ra8_port_set_bits_raw,
	.port_clear_bits_raw = gpio_ra8_port_clear_bits_raw,
	.port_toggle_bits = gpio_ra8_port_toggle_bits,
	.pin_interrupt_configure = NULL,
	.manage_callback = NULL,
};

#define GPIO_DEVICE_INIT(node, port_number, suffix, addr)                                      \
	static const struct gpio_ra8_config gpio_ra8_config_##suffix = {                           \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(16U),              \
			},                                                                         \
		.port_num = port_number,                                                           \
		.port = (R_PORT0_Type *)addr,                                                      \
		.vbatt_pins = DT_PROP_OR(DT_NODELABEL(ioport##suffix), vbatts_pins, {0xFF}),       \
	};                                                                                         \
	static struct gpio_ra8_data gpio_ra8_data_##suffix;                                        \
	DEVICE_DT_DEFINE(node, NULL, NULL, &gpio_ra8_data_##suffix,     \
			 &gpio_ra8_config_##suffix, PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,       \
			 &gpio_ra8_drv_api_funcs)

#define GPIO_DEVICE_INIT_RA8(suffix)                                                               \
	GPIO_DEVICE_INIT(DT_NODELABEL(ioport##suffix),                                             \
			 DT_PROP(DT_NODELABEL(ioport##suffix), port), suffix,                      \
			 DT_REG_ADDR(DT_NODELABEL(ioport##suffix)))

#if DT_NODE_HAS_STATUS(DT_NODELABEL(ioport0), okay)
GPIO_DEVICE_INIT_RA8(0);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(ioport0), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(ioport1), okay)
GPIO_DEVICE_INIT_RA8(1);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(ioport1), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(ioport2), okay)
GPIO_DEVICE_INIT_RA8(2);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(ioport2), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(ioport3), okay)
GPIO_DEVICE_INIT_RA8(3);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(ioport3), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(ioport4), okay)
GPIO_DEVICE_INIT_RA8(4);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(ioport4), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(ioport5), okay)
GPIO_DEVICE_INIT_RA8(5);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(ioport5), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(ioport6), okay)
GPIO_DEVICE_INIT_RA8(6);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(ioport6), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(ioport7), okay)
GPIO_DEVICE_INIT_RA8(7);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(ioport7), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(ioport8), okay)
GPIO_DEVICE_INIT_RA8(8);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(ioport8), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(ioport9), okay)
GPIO_DEVICE_INIT_RA8(9);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(ioport9), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(ioporta), okay)
GPIO_DEVICE_INIT_RA8(a);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(ioporta), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(ioportb), okay)
GPIO_DEVICE_INIT_RA8(b);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(ioportb), okay) */
