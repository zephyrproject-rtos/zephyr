/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_gpio_ioport

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/gpio/renesas-ra-gpio-ioport.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/irq.h>
#include <soc.h>

struct gpio_ra_config {
	struct gpio_driver_config common;
	uint8_t port_num;
	R_PORT0_Type *port;
	gpio_pin_t vbatt_pins[];
};

struct gpio_ra_data {
	struct gpio_driver_data common;
};

static int gpio_ra_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_ra_config *config = dev->config;

	struct ra_pinctrl_soc_pin pincfg;
	uint32_t pfs_cfg;

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

	/* Initial pin settings when PFS is zeroed:
	 * - Low output, input mode
	 * - Pull-up disabled, CMOS output
	 * - Low drive strength
	 * - Not used for IRQ or analog
	 * - Configured as general I/O
	 */
	pfs_cfg = 0;

	if ((flags & GPIO_OUTPUT) != 0U) {
		/* Set output pin initial value */
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
			WRITE_BIT(pfs_cfg, R_PFS_PORT_PIN_PmnPFS_PODR_Pos, 1);
		}

		WRITE_BIT(pfs_cfg, R_PFS_PORT_PIN_PmnPFS_PDR_Pos, 1);
	}

	if ((flags & GPIO_LINE_OPEN_DRAIN) != 0) {
		WRITE_BIT(pfs_cfg, R_PFS_PORT_PIN_PmnPFS_NCODR_Pos, 1);
	}

	if ((flags & GPIO_PULL_UP) != 0) {
		WRITE_BIT(pfs_cfg, R_PFS_PORT_PIN_PmnPFS_PCR_Pos, 1);
	}

	pincfg.cfg = pfs_cfg |
		     (((flags & RENESAS_GPIO_DS_MSK) >> 8) << R_PFS_PORT_PIN_PmnPFS_DSCR_Pos);

	return pinctrl_configure_pins(&pincfg, 1, PINCTRL_REG_NONE);
}

static int gpio_ra_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_ra_config *config = dev->config;
	R_PORT0_Type *port = config->port;

	*value = port->PIDR;

	return 0;
}

static int gpio_ra_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
				       gpio_port_value_t value)
{
	const struct gpio_ra_config *config = dev->config;
	R_PORT0_Type *port = config->port;

	port->PODR = ((port->PODR & ~mask) | (value & mask));

	return 0;
}

static int gpio_ra_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_ra_config *config = dev->config;
	R_PORT0_Type *port = config->port;

	port->PODR = (port->PODR | pins);

	return 0;
}

static int gpio_ra_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_ra_config *config = dev->config;
	R_PORT0_Type *port = config->port;

	port->PODR = (port->PODR & ~pins);

	return 0;
}

static int gpio_ra_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_ra_config *config = dev->config;
	R_PORT0_Type *port = config->port;

	port->PODR = (port->PODR ^ pins);

	return 0;
}

static DEVICE_API(gpio, gpio_ra_drv_api_funcs) = {
	.pin_configure = gpio_ra_pin_configure,
	.port_get_raw = gpio_ra_port_get_raw,
	.port_set_masked_raw = gpio_ra_port_set_masked_raw,
	.port_set_bits_raw = gpio_ra_port_set_bits_raw,
	.port_clear_bits_raw = gpio_ra_port_clear_bits_raw,
	.port_toggle_bits = gpio_ra_port_toggle_bits,
	.pin_interrupt_configure = NULL,
	.manage_callback = NULL,
};

#define GPIO_DEVICE_INIT(node, port_number, suffix, addr)                                          \
	static const struct gpio_ra_config gpio_ra_config_##suffix = {                             \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(16U),              \
			},                                                                         \
		.port_num = port_number,                                                           \
		.port = (R_PORT0_Type *)addr,                                                      \
		.vbatt_pins = DT_PROP_OR(DT_NODELABEL(ioport##suffix), vbatts_pins, {0xFF}),       \
	};                                                                                         \
	static struct gpio_ra_data gpio_ra_data_##suffix;                                          \
	DEVICE_DT_DEFINE(node, NULL, NULL, &gpio_ra_data_##suffix, &gpio_ra_config_##suffix,       \
			 PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY, &gpio_ra_drv_api_funcs)

#define GPIO_DEVICE_INIT_RA(suffix)                                                                \
	GPIO_DEVICE_INIT(DT_NODELABEL(ioport##suffix),                                             \
			 DT_PROP(DT_NODELABEL(ioport##suffix), port), suffix,                      \
			 DT_REG_ADDR(DT_NODELABEL(ioport##suffix)))

#define GPIO_DEVICE_INIT_RA_IF_OKAY(suffix)                                                        \
	COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ioport##suffix)),                         \
		    (GPIO_DEVICE_INIT_RA(suffix)),                                                 \
		    ())

GPIO_DEVICE_INIT_RA_IF_OKAY(0);
GPIO_DEVICE_INIT_RA_IF_OKAY(1);
GPIO_DEVICE_INIT_RA_IF_OKAY(2);
GPIO_DEVICE_INIT_RA_IF_OKAY(3);
GPIO_DEVICE_INIT_RA_IF_OKAY(4);
GPIO_DEVICE_INIT_RA_IF_OKAY(5);
GPIO_DEVICE_INIT_RA_IF_OKAY(6);
GPIO_DEVICE_INIT_RA_IF_OKAY(7);
GPIO_DEVICE_INIT_RA_IF_OKAY(8);
GPIO_DEVICE_INIT_RA_IF_OKAY(9);
GPIO_DEVICE_INIT_RA_IF_OKAY(a);
GPIO_DEVICE_INIT_RA_IF_OKAY(b);
