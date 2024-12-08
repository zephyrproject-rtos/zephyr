/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_gpio

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/irq.h>
#include <soc.h>
#include <fsl_common.h>
#include <fsl_gpio.h>

#include <zephyr/drivers/pinctrl.h>

#include <zephyr/drivers/gpio/gpio_utils.h>


struct mcux_igpio_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	GPIO_Type *base;
	const struct pinctrl_soc_pinmux *pin_muxes;
	uint8_t mux_count;
};

struct mcux_igpio_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data general;
	/* port ISR callback routine address */
	sys_slist_t callbacks;
};

static int mcux_igpio_configure(const struct device *dev,
				gpio_pin_t pin, gpio_flags_t flags)
{
	const struct mcux_igpio_config *config = dev->config;
	GPIO_Type *base = config->base;

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
	volatile uint32_t *gpio_cfg_reg =
		(volatile uint32_t *)config->pin_muxes[cfg_idx].config_register;
	uint32_t reg = *gpio_cfg_reg;

#ifdef CONFIG_SOC_SERIES_IMXRT10XX
	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		/* Set ODE bit */
		reg |= IOMUXC_SW_PAD_CTL_PAD_ODE_MASK;
	} else {
		reg &= ~IOMUXC_SW_PAD_CTL_PAD_ODE_MASK;
	}
	if (((flags & GPIO_PULL_UP) != 0) || ((flags & GPIO_PULL_DOWN) != 0)) {
		reg |= IOMUXC_SW_PAD_CTL_PAD_PUE_MASK;
		if (((flags & GPIO_PULL_UP) != 0)) {
			/* Use 100K pullup */
			reg |= IOMUXC_SW_PAD_CTL_PAD_PUS(2);
		} else {
			/* 100K pulldown */
			reg &= ~IOMUXC_SW_PAD_CTL_PAD_PUS_MASK;
		}
	} else {
		/* Set pin to keeper */
		reg &= ~IOMUXC_SW_PAD_CTL_PAD_PUE_MASK;
	}
#elif defined(CONFIG_SOC_SERIES_IMXRT11XX)
	if (config->pin_muxes[pin].pue_mux) {
		/* PUE type register layout (GPIO_AD pins) */
		if ((flags & GPIO_SINGLE_ENDED) != 0) {
			/* Set ODE bit */
			reg |= IOMUXC_SW_PAD_CTL_PAD_ODE_MASK;
		} else {
			reg &= ~IOMUXC_SW_PAD_CTL_PAD_ODE_MASK;
		}

		if (((flags & GPIO_PULL_UP) != 0) || ((flags & GPIO_PULL_DOWN) != 0)) {
			reg |= IOMUXC_SW_PAD_CTL_PAD_PUE_MASK;
			if (((flags & GPIO_PULL_UP) != 0)) {
				reg |= IOMUXC_SW_PAD_CTL_PAD_PUS_MASK;
			} else {
				reg &= ~IOMUXC_SW_PAD_CTL_PAD_PUS_MASK;
			}
		} else {
			/* Set pin to highz */
			reg &= ~IOMUXC_SW_PAD_CTL_PAD_PUE_MASK;
		}
	} else {
		/* PDRV/SNVS/LPSR type register layout */
		if (((flags & GPIO_PULL_UP) != 0) || ((flags & GPIO_PULL_DOWN) != 0)) {
			reg &= ~IOMUXC_SW_PAD_CTL_PAD_PULL_MASK;
			if (((flags & GPIO_PULL_UP) != 0)) {
				reg |= IOMUXC_SW_PAD_CTL_PAD_PULL(0x1U);
			} else {
				reg |= IOMUXC_SW_PAD_CTL_PAD_PULL(0x2U);
			}
		} else {
			/* Set pin to no pull */
			reg |= IOMUXC_SW_PAD_CTL_PAD_PULL_MASK;
		}
		/* PDRV/SNVS/LPSR reg have different ODE bits */
		if (config->pin_muxes[cfg_idx].pdrv_mux) {
			if ((flags & GPIO_SINGLE_ENDED) != 0) {
				/* Set ODE bit */
				reg |= IOMUXC_SW_PAD_CTL_PAD_ODE_MASK;
			} else {
				reg &= ~IOMUXC_SW_PAD_CTL_PAD_ODE_MASK;
			}
		} else if (config->pin_muxes[cfg_idx].lpsr_mux) {
			if ((flags & GPIO_SINGLE_ENDED) != 0) {
				/* Set ODE bit */
				reg |= (IOMUXC_SW_PAD_CTL_PAD_ODE_MASK << 1);
			} else {
				reg &= ~(IOMUXC_SW_PAD_CTL_PAD_ODE_MASK << 1);
			}
		} else if (config->pin_muxes[cfg_idx].snvs_mux) {
			if ((flags & GPIO_SINGLE_ENDED) != 0) {
				/* Set ODE bit */
				reg |= (IOMUXC_SW_PAD_CTL_PAD_ODE_MASK << 2);
			} else {
				reg &= ~(IOMUXC_SW_PAD_CTL_PAD_ODE_MASK << 2);
			}
		}


	}
#elif defined(CONFIG_SOC_MIMX8MQ6_M4)
	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		/* Set ODE bit */
		reg |= (0x1 << MCUX_IMX_DRIVE_OPEN_DRAIN_SHIFT);
	} else {
		reg &= ~(0x1 << MCUX_IMX_DRIVE_OPEN_DRAIN_SHIFT);
	}
	if ((flags & GPIO_PULL_UP) != 0) {
		reg |= (0x1 << MCUX_IMX_BIAS_PULL_UP_SHIFT);
	}
	if ((flag & GPIO_PULL_DOWN) != 0) {
		return -ENOTSUP;
	}
#else
	/* Default flags, should work for most SOCs */
	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		/* Set ODE bit */
		reg |= (0x1 << MCUX_IMX_DRIVE_OPEN_DRAIN_SHIFT);
	} else {
		reg &= ~(0x1 << MCUX_IMX_DRIVE_OPEN_DRAIN_SHIFT);
	}
	if (((flags & GPIO_PULL_UP) != 0) || ((flags & GPIO_PULL_DOWN) != 0)) {
		reg |= (0x1 << MCUX_IMX_BIAS_PULL_ENABLE_SHIFT);
		if (((flags & GPIO_PULL_UP) != 0)) {
			reg |= (0x1 << MCUX_IMX_BIAS_PULL_UP_SHIFT);
		} else {
			reg &= ~(0x1 << MCUX_IMX_BIAS_PULL_UP_SHIFT);
		}
	} else {
		/* Set pin to highz */
		reg &= ~(0x1 << MCUX_IMX_BIAS_PULL_ENABLE_SHIFT);
	}
#endif /* CONFIG_SOC_SERIES_IMXRT10XX */

	memcpy(&pin_cfg.pinmux, &config->pin_muxes[cfg_idx], sizeof(pin_cfg.pinmux));
	/* cfg register will be set by pinctrl_configure_pins */
	pin_cfg.pin_ctrl_flags = reg;
	pinctrl_configure_pins(&pin_cfg, 1, PINCTRL_REG_NONE);

	if (((flags & GPIO_INPUT) != 0) && ((flags & GPIO_OUTPUT) != 0)) {
		return -ENOTSUP;
	}

	if (flags & GPIO_OUTPUT_INIT_HIGH) {
		GPIO_WritePinOutput(base, pin, 1);
	}

	if (flags & GPIO_OUTPUT_INIT_LOW) {
		GPIO_WritePinOutput(base, pin, 0);
	}

	WRITE_BIT(base->GDIR, pin, flags & GPIO_OUTPUT);

	return 0;
}

static int mcux_igpio_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct mcux_igpio_config *config = dev->config;
	GPIO_Type *base = config->base;

	*value = base->DR;

	return 0;
}

static int mcux_igpio_port_set_masked_raw(const struct device *dev,
					  uint32_t mask,
					  uint32_t value)
{
	const struct mcux_igpio_config *config = dev->config;
	GPIO_Type *base = config->base;

	base->DR = (base->DR & ~mask) | (mask & value);

	return 0;
}

static int mcux_igpio_port_set_bits_raw(const struct device *dev,
					uint32_t mask)
{
	const struct mcux_igpio_config *config = dev->config;
	GPIO_Type *base = config->base;

	GPIO_PortSet(base, mask);

	return 0;
}

static int mcux_igpio_port_clear_bits_raw(const struct device *dev,
					  uint32_t mask)
{
	const struct mcux_igpio_config *config = dev->config;
	GPIO_Type *base = config->base;

	GPIO_PortClear(base, mask);

	return 0;
}

static int mcux_igpio_port_toggle_bits(const struct device *dev,
				       uint32_t mask)
{
	const struct mcux_igpio_config *config = dev->config;
	GPIO_Type *base = config->base;

	GPIO_PortToggle(base, mask);

	return 0;
}

static int mcux_igpio_pin_interrupt_configure(const struct device *dev,
					      gpio_pin_t pin,
					      enum gpio_int_mode mode,
					      enum gpio_int_trig trig)
{
	const struct mcux_igpio_config *config = dev->config;
	GPIO_Type *base = config->base;
	unsigned int key;
	uint8_t icr;
	int shift;

	/* Make sure pin is supported */
	if ((config->common.port_pin_mask & BIT(pin)) == 0) {
		return -ENOTSUP;
	}

	if (mode == GPIO_INT_MODE_DISABLED) {
		key = irq_lock();

		WRITE_BIT(base->IMR, pin, 0);

		irq_unlock(key);

		return 0;
	}

	if ((mode == GPIO_INT_MODE_EDGE) && (trig == GPIO_INT_TRIG_LOW)) {
		icr = 3;
	} else if ((mode == GPIO_INT_MODE_EDGE) &&
		   (trig == GPIO_INT_TRIG_HIGH)) {
		icr = 2;
	} else if ((mode == GPIO_INT_MODE_LEVEL) &&
		   (trig == GPIO_INT_TRIG_HIGH)) {
		icr = 1;
	} else {
		icr = 0;
	}

	if (pin < 16) {
		shift = 2 * pin;
		base->ICR1 = (base->ICR1 & ~(3 << shift)) | (icr << shift);
	} else if (pin < 32) {
		shift = 2 * (pin - 16);
		base->ICR2 = (base->ICR2 & ~(3 << shift)) | (icr << shift);
	} else {
		return -EINVAL;
	}

	key = irq_lock();

	WRITE_BIT(base->EDGE_SEL, pin, trig == GPIO_INT_TRIG_BOTH);
	WRITE_BIT(base->ISR, pin, 1);
	WRITE_BIT(base->IMR, pin, 1);

	irq_unlock(key);

	return 0;
}

static int mcux_igpio_manage_callback(const struct device *dev,
				      struct gpio_callback *callback,
				      bool set)
{
	struct mcux_igpio_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static void mcux_igpio_port_isr(const struct device *dev)
{
	const struct mcux_igpio_config *config = dev->config;
	struct mcux_igpio_data *data = dev->data;
	GPIO_Type *base = config->base;
	uint32_t int_flags;

	int_flags = base->ISR;
	base->ISR = int_flags;

	gpio_fire_callbacks(&data->callbacks, dev, int_flags);
}

static DEVICE_API(gpio, mcux_igpio_driver_api) = {
	.pin_configure = mcux_igpio_configure,
	.port_get_raw = mcux_igpio_port_get_raw,
	.port_set_masked_raw = mcux_igpio_port_set_masked_raw,
	.port_set_bits_raw = mcux_igpio_port_set_bits_raw,
	.port_clear_bits_raw = mcux_igpio_port_clear_bits_raw,
	.port_toggle_bits = mcux_igpio_port_toggle_bits,
	.pin_interrupt_configure = mcux_igpio_pin_interrupt_configure,
	.manage_callback = mcux_igpio_manage_callback,
};


/* These macros will declare an array of pinctrl_soc_pinmux types */
#define PINMUX_INIT(node, prop, idx) MCUX_IMX_PINMUX(DT_PROP_BY_IDX(node, prop, idx)),
#define MCUX_IGPIO_PIN_DECLARE(n)						\
	const struct pinctrl_soc_pinmux mcux_igpio_pinmux_##n[] = {		\
		DT_FOREACH_PROP_ELEM(DT_DRV_INST(n), pinmux, PINMUX_INIT)	\
	};
#define MCUX_IGPIO_PIN_INIT(n)							\
	.pin_muxes = mcux_igpio_pinmux_##n,					\
	.mux_count = DT_PROP_LEN(DT_DRV_INST(n), pinmux)

#define MCUX_IGPIO_IRQ_INIT(n, i)					\
	do {								\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, i, irq),		\
			    DT_INST_IRQ_BY_IDX(n, i, priority),		\
			    mcux_igpio_port_isr,			\
			    DEVICE_DT_INST_GET(n), 0);			\
									\
		irq_enable(DT_INST_IRQ_BY_IDX(n, i, irq));		\
	} while (false)

#define MCUX_IGPIO_INIT(n)						\
	MCUX_IGPIO_PIN_DECLARE(n)					\
	static int mcux_igpio_##n##_init(const struct device *dev);	\
									\
	static const struct mcux_igpio_config mcux_igpio_##n##_config = {\
		.common = {						\
			.port_pin_mask = GPIO_DT_INST_PORT_PIN_MASK_NGPIOS_EXC(\
				n, DT_INST_PROP(n, ngpios)),\
		},							\
		.base = (GPIO_Type *)DT_INST_REG_ADDR(n),		\
		MCUX_IGPIO_PIN_INIT(n)					\
	};								\
									\
	static struct mcux_igpio_data mcux_igpio_##n##_data;		\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			    mcux_igpio_##n##_init,			\
			    NULL,					\
			    &mcux_igpio_##n##_data,			\
			    &mcux_igpio_##n##_config,			\
			    POST_KERNEL,				\
			    CONFIG_GPIO_INIT_PRIORITY,			\
			    &mcux_igpio_driver_api);			\
									\
	static int mcux_igpio_##n##_init(const struct device *dev)	\
	{								\
		IF_ENABLED(DT_INST_IRQ_HAS_IDX(n, 0),			\
		   (MCUX_IGPIO_IRQ_INIT(n, 0);))		\
									\
		IF_ENABLED(DT_INST_IRQ_HAS_IDX(n, 1),			\
			   (MCUX_IGPIO_IRQ_INIT(n, 1);))		\
									\
		return 0;						\
	}

DT_INST_FOREACH_STATUS_OKAY(MCUX_IGPIO_INIT)
