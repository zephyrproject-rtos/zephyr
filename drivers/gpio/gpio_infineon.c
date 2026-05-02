/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief GPIO driver functions for Infineon MCU family.
 */
#define DT_DRV_COMPAT infineon_gpio

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/irq.h>

#include <infineon_kconfig.h>
#include <cy_gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_infineon, CONFIG_GPIO_LOG_LEVEL);

/* Device config structure */
struct gpio_ifx_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	GPIO_PRT_Type *regs;
	uint8_t ngpios;
};

/* Data structure */
struct gpio_ifx_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;

	/* device's owner of this data */
	const struct device *dev;

	/* callbacks list */
	sys_slist_t callbacks;
};

static inline uint32_t gpio_ifx_valid_mask(uint8_t ngpios)
{
	if (IS_ENABLED(CONFIG_SOC_FAMILY_INFINEON_PSOC4)) {
		return BIT_MASK(ngpios);
	} else {
		return 0xFFFFFFFFU;
	}
}

/**
 * @brief Select the PDL drive mode for an input pin.
 *
 * @param[in]  flags      GPIO configuration flags.
 * @param[out] drive_mode PDL drive mode constant.
 */
static void gpio_ifx_select_input_drive_mode(gpio_flags_t flags, uint32_t *drive_mode)
{
	if ((flags & GPIO_PULL_UP) && (flags & GPIO_PULL_DOWN)) {
		*drive_mode = CY_GPIO_DM_PULLUP_DOWN;
	} else if (flags & GPIO_PULL_UP) {
		*drive_mode = CY_GPIO_DM_PULLUP;
	} else if (flags & GPIO_PULL_DOWN) {
		*drive_mode = CY_GPIO_DM_PULLDOWN;
	} else {
		*drive_mode = CY_GPIO_DM_HIGHZ;
	}
}

/**
 * @brief Select the PDL drive mode for an output pin.
 *
 * Maps push-pull, open-drain, and open-source configurations to the
 * corresponding PDL drive mode.  Pull-up is only valid with open-drain;
 * pull-down is only valid with open-source.
 *
 * @param[in]  flags      GPIO configuration flags.
 * @param[out] drive_mode PDL drive mode constant.
 *
 * @retval 0        Success.
 * @retval -ENOTSUP Unsupported pull direction for the requested mode.
 */
static int gpio_ifx_select_output_drive_mode(gpio_flags_t flags, uint32_t *drive_mode)
{
	if (!(flags & GPIO_SINGLE_ENDED)) {
		if (flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) {
			LOG_WRN("Pull-up/pull-down flags ignored"
				" in push-pull output mode");
		}
		*drive_mode = CY_GPIO_DM_STRONG;
		return 0;
	}

	if (flags & GPIO_LINE_OPEN_DRAIN) {
		if (flags & GPIO_PULL_DOWN) {
			return -ENOTSUP;
		}
		*drive_mode = (flags & GPIO_PULL_UP) ? CY_GPIO_DM_PULLUP : CY_GPIO_DM_OD_DRIVESLOW;
	} else {
		/* Open-source */
		if (flags & GPIO_PULL_UP) {
			return -ENOTSUP;
		}
		*drive_mode =
			(flags & GPIO_PULL_DOWN) ? CY_GPIO_DM_PULLDOWN : CY_GPIO_DM_OD_DRIVESHIGH;
	}

	return 0;
}

static int gpio_ifx_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	uint32_t drive_mode = CY_GPIO_DM_HIGHZ;
	bool pin_val = false;
	const struct gpio_ifx_config *const cfg = dev->config;
	GPIO_PRT_Type *const base = cfg->regs;

	if (pin >= cfg->ngpios) {
		return -EINVAL;
	}

	switch (flags & (GPIO_INPUT | GPIO_OUTPUT | GPIO_DISCONNECTED)) {
	case GPIO_INPUT:
		gpio_ifx_select_input_drive_mode(flags, &drive_mode);
		/*
		 * The data register must match the pull direction for resistive pull-up/pull-down
		 * modes to work correctly: DR=1 for pull-up, DR=0 for pull-down.
		 * For high-Z, DR is don't-care.
		 */
		pin_val = (flags & GPIO_PULL_UP) ? true : false;
		break;

	case (GPIO_INPUT | GPIO_OUTPUT):
		__fallthrough;
	case GPIO_OUTPUT:
		if (gpio_ifx_select_output_drive_mode(flags, &drive_mode) != 0) {
			return -ENOTSUP;
		}

		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			pin_val = true;
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			pin_val = false;
		} else {
			/*
			 * If high or low init is not specified, the API expects the current output
			 * pin state to be retained.
			 */
			pin_val = Cy_GPIO_ReadOut(base, pin);
		}
		break;

	case GPIO_DISCONNECTED:
#ifndef CONFIG_SOC_FAMILY_INFINEON_PSOC4
		Cy_GPIO_SetInterruptMask(base, pin, 0);
#endif
		drive_mode = CY_GPIO_DM_ANALOG;
		pin_val = false;
		break;

	default:
		return -ENOTSUP;
	}

#ifdef CY_PDL_TZ_ENABLED
	Cy_GPIO_Pin_SecFastInit(base, pin, drive_mode, pin_val, HSIOM_SEL_GPIO);
#else
	Cy_GPIO_Pin_FastInit(base, pin, drive_mode, pin_val, HSIOM_SEL_GPIO);
#endif

	return 0;
}

static int gpio_ifx_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_ifx_config *const cfg = dev->config;
	GPIO_PRT_Type *const base = cfg->regs;

	*value = GPIO_PRT_IN(base) & gpio_ifx_valid_mask(cfg->ngpios);

	return 0;
}

static int gpio_ifx_port_set_masked_raw(const struct device *dev, uint32_t mask, uint32_t value)
{
	const struct gpio_ifx_config *const cfg = dev->config;
	GPIO_PRT_Type *const base = cfg->regs;

	mask &= gpio_ifx_valid_mask(cfg->ngpios);

#ifdef CONFIG_SOC_FAMILY_INFINEON_PSOC4
	uint32_t tmp = base->DR;

	base->DR = (tmp & ~mask) | (mask & value);
#else
	GPIO_PRT_OUT(base) = (GPIO_PRT_OUT(base) & ~mask) | (mask & value);
#endif
	return 0;
}

static int gpio_ifx_port_set_bits_raw(const struct device *dev, uint32_t mask)
{
	const struct gpio_ifx_config *const cfg = dev->config;
	GPIO_PRT_Type *const base = cfg->regs;

	GPIO_PRT_OUT_SET(base) = mask & gpio_ifx_valid_mask(cfg->ngpios);

	return 0;
}

static int gpio_ifx_port_clear_bits_raw(const struct device *dev, uint32_t mask)
{
	const struct gpio_ifx_config *const cfg = dev->config;
	GPIO_PRT_Type *const base = cfg->regs;

	GPIO_PRT_OUT_CLR(base) = mask & gpio_ifx_valid_mask(cfg->ngpios);

	return 0;
}

static int gpio_ifx_port_toggle_bits(const struct device *dev, uint32_t mask)
{
	const struct gpio_ifx_config *const cfg = dev->config;
	GPIO_PRT_Type *const base = cfg->regs;

	GPIO_PRT_OUT_INV(base) = mask & gpio_ifx_valid_mask(cfg->ngpios);

	return 0;
}

static uint32_t gpio_ifx_get_pending_int(const struct device *dev)
{
	const struct gpio_ifx_config *const cfg = dev->config;
	GPIO_PRT_Type *const base = cfg->regs;

#ifdef CONFIG_SOC_FAMILY_INFINEON_PSOC4
	return base->INTR & gpio_ifx_valid_mask(cfg->ngpios);
#else
	return GPIO_PRT_INTR_MASKED(base);
#endif
}

static uint32_t __maybe_unused gpio_get_pending_pins(const struct gpio_ifx_config *const cfg,
							GPIO_PRT_Type * const base)
{
	uint32_t pending = GPIO_PRT_INTR(base) & gpio_ifx_valid_mask(cfg->ngpios);

	return pending;
}

static void __maybe_unused gpio_ifx_isr(const struct device *dev)
{
	const struct gpio_ifx_config *const cfg = dev->config;
	GPIO_PRT_Type *const base = cfg->regs;
	struct gpio_ifx_data *const data = dev->data;
	uint32_t pending = gpio_get_pending_pins(dev->config, cfg->regs);

	if (pending == 0U) {
		return;
	}

	for (uint8_t i = 0; i < cfg->ngpios; i++) {
		if (pending & BIT(i)) {
			Cy_GPIO_ClearInterrupt(base, i);
		}
	}

	gpio_fire_callbacks(&data->callbacks, dev, pending);
}

static int gpio_ifx_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					     enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	uint32_t trig_pdl = CY_GPIO_INTR_DISABLE;
	const struct gpio_ifx_config *const cfg = dev->config;
	GPIO_PRT_Type *const base = cfg->regs;

	if (pin >= cfg->ngpios) {
		return -EINVAL;
	}

	if (mode == GPIO_INT_MODE_DISABLED) {
		Cy_GPIO_SetInterruptEdge(base, pin, CY_GPIO_INTR_DISABLE);
		Cy_GPIO_ClearInterrupt(base, pin);
		return 0;
	}

	if (mode != GPIO_INT_MODE_EDGE) {
		return -ENOTSUP;
	}

	switch (trig) {
	case GPIO_INT_TRIG_LOW:
		trig_pdl = CY_GPIO_INTR_FALLING;
		break;
	case GPIO_INT_TRIG_HIGH:
		trig_pdl = CY_GPIO_INTR_RISING;
		break;
	case GPIO_INT_TRIG_BOTH:
		trig_pdl = CY_GPIO_INTR_BOTH;
		break;
	default:
		return -ENOTSUP;
	}

	Cy_GPIO_SetInterruptEdge(base, pin, trig_pdl);
	Cy_GPIO_ClearInterrupt(base, pin);

#ifndef CONFIG_SOC_FAMILY_INFINEON_PSOC4
	/* Interrupt mask function wants a 0 for disabled, 1 for enabled as the value */
	uint32_t int_en = (mode == GPIO_INT_MODE_DISABLED ? 0 : 1);

	Cy_GPIO_SetInterruptMask(base, pin, int_en);
#endif

	return 0;
}

static int gpio_ifx_manage_callback(const struct device *port, struct gpio_callback *callback,
				     bool set)
{
	struct gpio_ifx_data *const data = port->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static DEVICE_API(gpio, gpio_ifx_api) = {
	.pin_configure = gpio_ifx_configure,
	.port_get_raw = gpio_ifx_port_get_raw,
	.port_set_masked_raw = gpio_ifx_port_set_masked_raw,
	.port_set_bits_raw = gpio_ifx_port_set_bits_raw,
	.port_clear_bits_raw = gpio_ifx_port_clear_bits_raw,
	.port_toggle_bits = gpio_ifx_port_toggle_bits,
	.pin_interrupt_configure = gpio_ifx_pin_interrupt_configure,
	.manage_callback = gpio_ifx_manage_callback,
	.get_pending_int = gpio_ifx_get_pending_int,
};

#define GPIO_PORT_STRUCTS_DEFINE(n)                                                                \
	static const struct gpio_ifx_config gpio_ifx_config_##n = {                                \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(n),                                      \
		.ngpios = DT_INST_PROP_OR(n, ngpios, 8),                                           \
		.regs = (GPIO_PRT_Type *)DT_INST_REG_ADDR(n),                                      \
	};                                                                                         \
	static struct gpio_ifx_data gpio_ifx_data_##n;

#define GPIO_PORT_DEFINE(n)                                                                        \
	static int gpio_ifx##n##_init(const struct device *dev)                                    \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), gpio_ifx_isr,               \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
                                                                                                   \
		return 0;                                                                          \
	}                                                                                          \
	GPIO_PORT_STRUCTS_DEFINE(n)                                                                \
	DEVICE_DT_INST_DEFINE(n, gpio_ifx##n##_init, NULL, &gpio_ifx_data_##n,                     \
			      &gpio_ifx_config_##n, POST_KERNEL,                                   \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &gpio_ifx_api);


#define GPIO_SHARED_PORT_DEFINE(n)                                                                 \
	GPIO_PORT_STRUCTS_DEFINE(n)                                                                \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, &gpio_ifx_data_##n,                                   \
			      &gpio_ifx_config_##n, POST_KERNEL,                                   \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &gpio_ifx_api);

/*
 * A few variants of this define are required due to interrupt connectivity variations.
 * 1. A gpio port with a dedicated NVIC interrupt (GPIO_PORT_DEFINE)
 * 2. A gpio port without a dedicated NVIC interrupt (GPIO_SHARED_PORT_DEFINE)
 * 3. TODO A gpio port with an added interrupt controller/mux before the NVIC (xmc7xxx, psoc6 m0+,)
 *
 * For the TODO option we'd likely want to look at the interrupt-parent DT property
 * where the parent might be a psoc6 m0+ mux or an xmc7xxx system interrupt controller. Along with
 * perhaps the multi-level IRQ number encoding option in Zephyr.
 */
#define GPIO_IFX_DEFINE(n)                                                                         \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, interrupts),                                          \
		(GPIO_PORT_DEFINE(n)),                                                             \
		(GPIO_SHARED_PORT_DEFINE(n)))

DT_INST_FOREACH_STATUS_OKAY(GPIO_IFX_DEFINE)


#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT infineon_shared_gpio

struct gpio_shared_config {
	size_t port_count;
	const struct device *ports[];
};

static __maybe_unused void gpio_shared_isr(const struct device *dev)
{
	const struct gpio_shared_config *cfg = dev->config;

	for (size_t i = 0; i < cfg->port_count; i++) {
		gpio_ifx_isr(cfg->ports[i]);
	}
}

#define GPIO_SHARED_PORT_DEVICES(n) DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(n, DEVICE_DT_GET, (,))

#define GPIO_SHARED_INIT(n)                                                                        \
	const static struct gpio_shared_config gpio_shared##n##_cfg = {                            \
		.port_count = DT_INST_CHILD_NUM_STATUS_OKAY(n),                           \
		.ports = {GPIO_SHARED_PORT_DEVICES(n)},                                            \
	};                                                                                         \
                                                                                                   \
	static int gpio_shared##n##_init(const struct device *dev)                                 \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), gpio_shared_isr,            \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
		return 0;                                                                          \
	}                                                                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, gpio_shared##n##_init, NULL, NULL, &gpio_shared##n##_cfg,         \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

DT_INST_FOREACH_STATUS_OKAY(GPIO_SHARED_INIT)
