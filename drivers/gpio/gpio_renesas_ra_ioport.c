/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_gpio_ioport

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/gpio/renesas-ra-gpio-ioport.h>
#include <zephyr/drivers/misc/renesas_ra_external_interrupt/renesas_ra_external_interrupt.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <soc.h>

struct gpio_ra_irq_info {
	const struct device *port_irq;
	const uint8_t *const pins;
	size_t num;
};

struct gpio_ra_config {
	struct gpio_driver_config common;
	uint8_t port_num;
	R_PORT0_Type *port;
	const struct gpio_ra_irq_info *irq_info;
	const size_t irq_info_size;
	gpio_pin_t vbatt_pins[];
};

struct gpio_ra_data {
	struct gpio_driver_data common;
	sys_slist_t callbacks;
};

#if CONFIG_RENESAS_RA_EXTERNAL_INTERRUPT
static const struct gpio_ra_irq_info *query_irq_info(const struct device *dev, uint32_t pin)
{
	const struct gpio_ra_config *config = dev->config;

	for (int i = 0; i < config->irq_info_size; i++) {
		const struct gpio_ra_irq_info *info = &config->irq_info[i];

		for (int j = 0; j < info->num; j++) {
			if (info->pins[j] == pin) {
				return info;
			}
		}
	}

	return NULL;
}

static void gpio_ra_callback_adapter(const struct device *dev, gpio_pin_t pin)
{
	struct gpio_ra_data *data = dev->data;

	gpio_fire_callbacks(&data->callbacks, dev, BIT(pin));
}
#endif

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

	if (!IS_ENABLED(CONFIG_RENESAS_RA_EXTERNAL_INTERRUPT) && ((flags & GPIO_INT_ENABLE) != 0)) {
		return -ENOTSUP;
	}

#if CONFIG_GPIO_RA_HAS_VBTICTLR
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
#endif

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

#if CONFIG_RENESAS_RA_EXTERNAL_INTERRUPT
	if ((flags & GPIO_INT_ENABLE) != 0) {
		const struct gpio_ra_irq_info *irq_info = query_irq_info(dev, pin);
		int err = 0;

		if (irq_info == NULL) {
			return -EINVAL;
		}

		if (!device_is_ready(irq_info->port_irq)) {
			return -EWOULDBLOCK;
		}

		struct gpio_ra_callback callback = {
			.port = (struct device *)dev,
			.port_num = config->port_num,
			.pin = pin,
			.mode = flags & (GPIO_INT_EDGE | GPIO_INT_DISABLE | GPIO_INT_ENABLE),
			.trigger = flags & (GPIO_INT_LOW_0 | GPIO_INT_HIGH_1),
			.isr = gpio_ra_callback_adapter,
		};

		err = gpio_ra_interrupt_set(irq_info->port_irq, &callback);
		if (err < 0) {
			return err;
		}

		WRITE_BIT(pfs_cfg, R_PFS_PORT_PIN_PmnPFS_ISEL_Pos, 1);
	}

	if ((flags & GPIO_INT_DISABLE) != 0) {
		const struct gpio_ra_irq_info *irq_info = query_irq_info(dev, pin);

		if (irq_info == NULL) {
			return -EINVAL;
		}

		if (!device_is_ready(irq_info->port_irq)) {
			return -EWOULDBLOCK;
		}

		gpio_ra_interrupt_unset(irq_info->port_irq, config->port_num, pin);
		WRITE_BIT(pfs_cfg, R_PFS_PORT_PIN_PmnPFS_ISEL_Pos, 0);
	}
#endif

	pincfg.cfg =
		pfs_cfg | (((flags & RENESAS_GPIO_DS_MSK) >> 8) << R_PFS_PORT_PIN_PmnPFS_DSCR_Pos);

	return pinctrl_configure_pins(&pincfg, 1, PINCTRL_REG_NONE);
}

__maybe_unused static int gpio_ra_pin_get_config(const struct device *dev, gpio_pin_t pin,
						 gpio_flags_t *flags)
{
	const struct gpio_ra_config *config = dev->config;
	uint32_t pincfg;

	if (pin >= RA_PINCTRL_PIN_NUM) {
		return -EINVAL;
	}

	memset(flags, 0, sizeof(gpio_flags_t));

	pincfg = R_PFS->PORT[config->port_num].PIN[pin].PmnPFS;

	if (pincfg & BIT(R_PFS_PORT_PIN_PmnPFS_PDR_Pos)) {
		*flags |= GPIO_OUTPUT;
	} else {
		*flags |= GPIO_INPUT;
	}

	if (pincfg & BIT(R_PFS_PORT_PIN_PmnPFS_NCODR_Pos)) {
		*flags |= GPIO_LINE_OPEN_DRAIN;
	}

	if (pincfg & BIT(R_PFS_PORT_PIN_PmnPFS_PCR_Pos)) {
		*flags |= GPIO_PULL_UP;
	}

	return 0;
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

#if CONFIG_RENESAS_RA_EXTERNAL_INTERRUPT
static int gpio_ra_pin_interrupt_configure(const struct device *port, gpio_pin_t pin,
					   enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	gpio_flags_t flags;
	int err;

	err = gpio_ra_pin_get_config(port, pin, &flags);
	if (err) {
		return err;
	}

	return gpio_ra_pin_configure(port, pin, (flags | mode | trig));
}

static int gpio_ra_manage_callback(const struct device *dev, struct gpio_callback *callback,
				   bool set)
{
	struct gpio_ra_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}
#endif

static DEVICE_API(gpio, gpio_ra_drv_api_funcs) = {
	.pin_configure = gpio_ra_pin_configure,
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = gpio_ra_pin_get_config,
#endif
	.port_get_raw = gpio_ra_port_get_raw,
	.port_set_masked_raw = gpio_ra_port_set_masked_raw,
	.port_set_bits_raw = gpio_ra_port_set_bits_raw,
	.port_clear_bits_raw = gpio_ra_port_clear_bits_raw,
	.port_toggle_bits = gpio_ra_port_toggle_bits,
#if CONFIG_RENESAS_RA_EXTERNAL_INTERRUPT
	.pin_interrupt_configure = gpio_ra_pin_interrupt_configure,
	.manage_callback = gpio_ra_manage_callback,
#endif
};

#define GPIO_RA_PINS_NAME(n, p, i) CONCAT(DT_STRING_TOKEN_BY_IDX(n, p, i), _pins)

#define GPIO_RA_DECL_PINS(n, p, i)                                                                 \
	const uint8_t CONCAT(n, ___pins##i[]) = {                                                  \
		DT_FOREACH_PROP_ELEM_SEP(n, GPIO_RA_PINS_NAME(n, p, i), DT_PROP_BY_IDX, (,))};

#define GPIO_RA_IRQ_INFO(n, p, i)                                                                  \
	{                                                                                          \
		.port_irq = DEVICE_DT_GET_OR_NULL(DT_PHANDLE_BY_IDX(n, port_irqs, i)),             \
		.pins = CONCAT(n, ___pins##i),                                                     \
		.num = ARRAY_SIZE(CONCAT(n, ___pins##i)),                                          \
	},

#define DECL_PINS_PARAMETER(node)                                                                  \
	COND_CODE_1(DT_NODE_HAS_PROP(node, port_irq_names),                                        \
	(DT_FOREACH_PROP_ELEM(node, port_irq_names, GPIO_RA_DECL_PINS)), ())
#define IRQ_INFO_PARAMETER(node)                                                                   \
	COND_CODE_1(DT_NODE_HAS_PROP(node, port_irq_names),                                        \
	(DT_FOREACH_PROP_ELEM(node, port_irq_names, GPIO_RA_IRQ_INFO)), ())

#define GPIO_DEVICE_INIT(node, port_number, suffix, addr)                                          \
	DECL_PINS_PARAMETER(node);                                                                 \
	struct gpio_ra_irq_info gpio_ra_irq_info_##suffix[] = {IRQ_INFO_PARAMETER(node)};          \
	static const struct gpio_ra_config gpio_ra_config_##suffix = {                             \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(16U),              \
			},                                                                         \
		.port_num = port_number,                                                           \
		.port = (R_PORT0_Type *)addr,                                                      \
		.vbatt_pins = DT_PROP_OR(DT_NODELABEL(ioport##suffix), vbatts_pins, {0xFF}),       \
		.irq_info = gpio_ra_irq_info_##suffix,                                             \
		.irq_info_size = DT_PROP_LEN_OR(DT_NODELABEL(ioport##suffix), port_irq_names, 0),  \
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
