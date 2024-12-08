/*
 * Copyright 2023-2024, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_rgpio

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <fsl_common.h>
#include <fsl_rgpio.h>

#include <zephyr/drivers/gpio/gpio_utils.h>

/* Required by DEVICE_MMIO_NAMED_* macros */
#define DEV_CFG(_dev) \
	((const struct mcux_rgpio_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct mcux_rgpio_data *)(_dev)->data)

struct mcux_rgpio_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;

	DEVICE_MMIO_NAMED_ROM(reg_base);

	const struct pinctrl_soc_pinmux *pin_muxes;
	uint8_t mux_count;
};

struct mcux_rgpio_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data general;

	DEVICE_MMIO_NAMED_RAM(reg_base);

	/* port ISR callback routine address */
	sys_slist_t callbacks;
};

static int mcux_rgpio_configure(const struct device *dev,
				gpio_pin_t pin, gpio_flags_t flags)
{
	RGPIO_Type *base = (RGPIO_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	const struct mcux_rgpio_config *config = dev->config;

	struct pinctrl_soc_pin pin_cfg;
	int cfg_idx = pin, i;

	/* Make sure pin is supported */
	if ((config->common.port_pin_mask & BIT(pin)) == 0) {
		return -ENOTSUP;
	}

	/* Some SOCs have non-contiguous gpio pin layouts, account for this */
	for (i = 0; i < pin; i++) {
		if ((config->common.port_pin_mask & BIT(i)) == 0) {
			cfg_idx--;
		}
	}

	/* Init pin configuration struct, and use pinctrl api to apply settings */
	if (cfg_idx >= config->mux_count) {
		/* Pin is not connected to a mux */
		return -ENOTSUP;
	}

	/* Set appropriate bits in pin configuration register */
	volatile uint32_t *gpio_cfg_reg = (volatile uint32_t *)
			((size_t)config->pin_muxes[cfg_idx].config_register);
	uint32_t reg = *gpio_cfg_reg;

#if defined(CONFIG_SOC_SERIES_IMXRT118X)
	/* PUE/PDRV types have the same ODE bit */
	if ((flags & GPIO_SINGLE_ENDED)) {
		/* Set ODE bit */
		reg |= IOMUXC_SW_PAD_CTL_PAD_ODE_MASK;
	} else {
		reg &= ~IOMUXC_SW_PAD_CTL_PAD_ODE_MASK;
	}

	if (config->pin_muxes[pin].pue_mux) {
		if (flags & GPIO_PULL_UP) {
			reg |= (IOMUXC_SW_PAD_CTL_PAD_PUS_MASK | IOMUXC_SW_PAD_CTL_PAD_PUE_MASK);
		} else if (flags & GPIO_PULL_DOWN) {
			reg |= IOMUXC_SW_PAD_CTL_PAD_PUE_MASK;
			reg &= ~IOMUXC_SW_PAD_CTL_PAD_PUS_MASK;
		} else {
			/* Set pin to highz */
			reg &= ~IOMUXC_SW_PAD_CTL_PAD_PUE_MASK;
		}
	} else {
		/* PDRV type register layout */
		if (flags & GPIO_PULL_UP) {
			reg &= ~IOMUXC_SW_PAD_CTL_PAD_PULL_MASK;
			reg |= IOMUXC_SW_PAD_CTL_PAD_PULL(0x1U);
		} else if (flags & GPIO_PULL_DOWN) {
			reg &= ~IOMUXC_SW_PAD_CTL_PAD_PULL_MASK;
			reg |= IOMUXC_SW_PAD_CTL_PAD_PULL(0x2U);
		} else {
			/* Set pin to no pull */
			reg |= IOMUXC_SW_PAD_CTL_PAD_PULL_MASK;
		}
	}
#else
	/* TODO: Default flags, work for i.MX 9352 */
	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		/* Set ODE bit */
		reg |= (0x1 << MCUX_IMX_DRIVE_OPEN_DRAIN_SHIFT);
	} else {
		reg &= ~(0x1 << MCUX_IMX_DRIVE_OPEN_DRAIN_SHIFT);
	}
	if (((flags & GPIO_PULL_UP) != 0) || ((flags & GPIO_PULL_DOWN) != 0)) {
		/* i.MX93 has no pull enable bit */
		if (((flags & GPIO_PULL_UP) != 0)) {
			reg |= (0x1 << MCUX_IMX_BIAS_PULL_UP_SHIFT);
			reg &= ~(0x1 << MCUX_IMX_BIAS_PULL_DOWN_SHIFT);
		} else {
			reg |= (0x1 << MCUX_IMX_BIAS_PULL_DOWN_SHIFT);
			reg &= ~(0x1 << MCUX_IMX_BIAS_PULL_UP_SHIFT);
		}
	} else {
		/* Set pin to highz */
		reg &= ~((0x1 << MCUX_IMX_BIAS_PULL_DOWN_SHIFT) |
				(0x1 << MCUX_IMX_BIAS_PULL_UP_SHIFT));
	}
#endif

	memcpy(&pin_cfg.pinmux, &config->pin_muxes[cfg_idx], sizeof(pin_cfg));
	/* cfg register will be set by pinctrl_configure_pins */
	pin_cfg.pin_ctrl_flags = reg;
	pinctrl_configure_pins(&pin_cfg, 1, PINCTRL_REG_NONE);

	if (((flags & GPIO_INPUT) != 0) && ((flags & GPIO_OUTPUT) != 0)) {
		return -ENOTSUP;
	}

	if (flags & GPIO_OUTPUT_INIT_HIGH) {
		RGPIO_WritePinOutput(base, pin, 1);
	}

	if (flags & GPIO_OUTPUT_INIT_LOW) {
		RGPIO_WritePinOutput(base, pin, 0);
	}

	WRITE_BIT(base->PDDR, pin, flags & GPIO_OUTPUT);

	return 0;
}

static int mcux_rgpio_port_get_raw(const struct device *dev, uint32_t *value)
{
	RGPIO_Type *base = (RGPIO_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);

	*value = base->PDIR;

	return 0;
}

static int mcux_rgpio_port_set_masked_raw(const struct device *dev,
					  uint32_t mask,
					  uint32_t value)
{
	RGPIO_Type *base = (RGPIO_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);

	base->PDOR = (base->PDOR & ~mask) | (mask & value);

	return 0;
}

static int mcux_rgpio_port_set_bits_raw(const struct device *dev,
					uint32_t mask)
{
	RGPIO_Type *base = (RGPIO_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);

	RGPIO_PortSet(base, mask);

	return 0;
}

static int mcux_rgpio_port_clear_bits_raw(const struct device *dev,
					  uint32_t mask)
{
	RGPIO_Type *base = (RGPIO_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);

	RGPIO_PortClear(base, mask);

	return 0;
}

static int mcux_rgpio_port_toggle_bits(const struct device *dev,
					   uint32_t mask)
{
	RGPIO_Type *base = (RGPIO_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);

	RGPIO_PortToggle(base, mask);

	return 0;
}

static int mcux_rgpio_pin_interrupt_configure(const struct device *dev,
						  gpio_pin_t pin,
						  enum gpio_int_mode mode,
						  enum gpio_int_trig trig)
{
	RGPIO_Type *base = (RGPIO_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	const struct mcux_rgpio_config *config = dev->config;
	unsigned int key;
	uint8_t irqs, irqc;

	/* Make sure pin is supported */
	if ((config->common.port_pin_mask & BIT(pin)) == 0) {
		return -ENOTSUP;
	}

	irqs = 0; /* only irq0 is used for irq */

	if (mode == GPIO_INT_MODE_DISABLED) {
		irqc = kRGPIO_InterruptOrDMADisabled;
	} else if ((mode == GPIO_INT_MODE_EDGE) &&
		   (trig == GPIO_INT_TRIG_LOW)) {
		irqc = kRGPIO_InterruptFallingEdge;
	} else if ((mode == GPIO_INT_MODE_EDGE) &&
		   (trig == GPIO_INT_TRIG_HIGH)) {
		irqc = kRGPIO_InterruptRisingEdge;
	} else if ((mode == GPIO_INT_MODE_EDGE) &&
		   (trig == GPIO_INT_TRIG_BOTH)) {
		irqc = kRGPIO_InterruptEitherEdge;
	} else if ((mode == GPIO_INT_MODE_LEVEL) &&
		   (trig == GPIO_INT_TRIG_LOW)) {
		irqc = kRGPIO_InterruptLogicZero;
	} else if ((mode == GPIO_INT_MODE_LEVEL) &&
		   (trig == GPIO_INT_TRIG_HIGH)) {
		irqc = kRGPIO_InterruptLogicOne;
	} else {
		return -EINVAL; /* should never end up here */
	}

	key = irq_lock();
	RGPIO_SetPinInterruptConfig(base, pin, irqs, irqc);
	irq_unlock(key);

	return 0;
}

static int mcux_rgpio_manage_callback(const struct device *dev,
					  struct gpio_callback *callback,
					  bool set)
{
	struct mcux_rgpio_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static void mcux_rgpio_port_isr(const struct device *dev)
{
	RGPIO_Type *base = (RGPIO_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct mcux_rgpio_data *data = dev->data;
	uint32_t int_flags;

	int_flags = base->ISFR[0]; /* Notice: only irq0 is used for now */
	base->ISFR[0] = int_flags;

	gpio_fire_callbacks(&data->callbacks, dev, int_flags);
}

static DEVICE_API(gpio, mcux_rgpio_driver_api) = {
	.pin_configure = mcux_rgpio_configure,
	.port_get_raw = mcux_rgpio_port_get_raw,
	.port_set_masked_raw = mcux_rgpio_port_set_masked_raw,
	.port_set_bits_raw = mcux_rgpio_port_set_bits_raw,
	.port_clear_bits_raw = mcux_rgpio_port_clear_bits_raw,
	.port_toggle_bits = mcux_rgpio_port_toggle_bits,
	.pin_interrupt_configure = mcux_rgpio_pin_interrupt_configure,
	.manage_callback = mcux_rgpio_manage_callback,
};

/* These macros will declare an array of pinctrl_soc_pinmux types */
#define PINMUX_INIT(node, prop, idx) MCUX_IMX_PINMUX(DT_PROP_BY_IDX(node, prop, idx)),
#define MCUX_RGPIO_PIN_DECLARE(n)						\
	const struct pinctrl_soc_pinmux mcux_rgpio_pinmux_##n[] = {		\
		DT_FOREACH_PROP_ELEM(DT_DRV_INST(n), pinmux, PINMUX_INIT)	\
	};
#define MCUX_RGPIO_PIN_INIT(n)							\
	.pin_muxes = mcux_rgpio_pinmux_##n,					\
	.mux_count = DT_PROP_LEN(DT_DRV_INST(n), pinmux),

#define MCUX_RGPIO_IRQ_INIT(n, i)					\
	do {								\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, i, irq),		\
				DT_INST_IRQ_BY_IDX(n, i, priority),		\
				mcux_rgpio_port_isr,			\
				DEVICE_DT_INST_GET(n), 0);			\
									\
		irq_enable(DT_INST_IRQ_BY_IDX(n, i, irq));		\
	} while (false)

#define MCUX_RGPIO_INIT(n)						\
	MCUX_RGPIO_PIN_DECLARE(n)					\
	static int mcux_rgpio_##n##_init(const struct device *dev);	\
									\
	static const struct mcux_rgpio_config mcux_rgpio_##n##_config = { \
		.common = {						\
			.port_pin_mask = GPIO_DT_INST_PORT_PIN_MASK_NGPIOS_EXC( \
					n, DT_INST_PROP(n, ngpios)) \
		},							\
		DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_DRV_INST(n)), \
		MCUX_RGPIO_PIN_INIT(n)					\
	};								\
									\
	static struct mcux_rgpio_data mcux_rgpio_##n##_data;		\
									\
	DEVICE_DT_INST_DEFINE(n,					\
				mcux_rgpio_##n##_init,			\
				NULL,					\
				&mcux_rgpio_##n##_data,			\
				&mcux_rgpio_##n##_config,			\
				POST_KERNEL,				\
				CONFIG_GPIO_INIT_PRIORITY,			\
				&mcux_rgpio_driver_api);			\
									\
	static int mcux_rgpio_##n##_init(const struct device *dev)	\
	{								\
		DEVICE_MMIO_NAMED_MAP(dev, reg_base, \
			K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP); \
		IF_ENABLED(DT_INST_IRQ_HAS_IDX(n, 0),			\
		   (MCUX_RGPIO_IRQ_INIT(n, 0);))		\
									\
		IF_ENABLED(DT_INST_IRQ_HAS_IDX(n, 1),			\
			   (MCUX_RGPIO_IRQ_INIT(n, 1);))		\
									\
		return 0;						\
	}

DT_INST_FOREACH_STATUS_OKAY(MCUX_RGPIO_INIT)
