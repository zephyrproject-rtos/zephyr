/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Driver for the NXP i.MX GPIO controller (nxp,imx-gpio-v2).
 *
 * Pin mux is handled by an external IOMUXC controller. The pad configuration
 * register is a combined read-modify-write register that holds both the mux
 * selector and pad attributes; the driver reads the current value, sets the
 * attribute bits, clears the mux field, and delegates the mux write to
 * pinctrl_configure_pins() which ORs in the GPIO function.
 */

#define DT_DRV_COMPAT nxp_imx_gpio_v2

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <fsl_common.h>
#include <fsl_gpio.h>

#include <zephyr/drivers/gpio/gpio_utils.h>

/* Required by DEVICE_MMIO_NAMED_* macros */
#define DEV_CFG(_dev)  ((const struct mcux_imx_gpio_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct mcux_imx_gpio_data *)(_dev)->data)

/*
 * Default PAD config value written before pinctrl_configure_pins() when the
 * driver cannot read the current pad register value (e.g. SCMI platforms).
 * Pull down, slight fast slew rate, x4 driver strength.
 */
#define GPIO_PIN_DEFAULT_PAD_VAL 0x0000051eU

struct mcux_imx_gpio_config {
	/* gpio_driver_config must be first */
	struct gpio_driver_config common;
	DEVICE_MMIO_NAMED_ROM(reg_base);
	const struct pinctrl_soc_pinmux *pin_muxes;
	uint8_t mux_count;
	uint8_t irq_sel;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};

struct mcux_imx_gpio_data {
	/* gpio_driver_data must be first */
	struct gpio_driver_data common;
	sys_slist_t callbacks;
	struct k_spinlock lock;
};

static int mcux_imx_gpio_configure(const struct device *dev,
				   gpio_pin_t pin, gpio_flags_t flags)
{
	GPIO_Type *base = (GPIO_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	const struct mcux_imx_gpio_config *config = dev->config;
	struct mcux_imx_gpio_data *data = dev->data;

	struct pinctrl_soc_pin pin_cfg;
	int cfg_idx = pin, i;
	k_spinlock_key_t key;

	if (flags == GPIO_DISCONNECTED) {
		return -ENOTSUP;
	}

	/* Verify pin is in the port mask */
	if ((config->common.port_pin_mask & BIT(pin)) == 0) {
		return -ENOTSUP;
	}

	/* Account for non-contiguous pin layouts when indexing pin_muxes[] */
	for (i = 0; i < pin; i++) {
		if ((config->common.port_pin_mask & BIT(i)) == 0) {
			cfg_idx--;
		}
	}

	if (cfg_idx >= config->mux_count) {
		return -ENOTSUP;
	}

	/*
	 * Read the current combined pad register value. Its layout is SoC-specific;
	 * set the attribute bits for the requested flags, then let
	 * pinctrl_configure_pins() write the mux field.
	 */
	volatile uint32_t *gpio_cfg_reg =
		(volatile uint32_t *)((size_t)config->pin_muxes[cfg_idx].config_register);
	uint32_t reg = *gpio_cfg_reg;

#if defined(CONFIG_SOC_SERIES_IMXRT266X)
	/*
	 * Combined PIO register: MUX_MODE [3:0], PULLENA [5:4], IBENA [7], ODENA [10].
	 * Clear MUX_MODE so pinctrl ORs in the GPIO function. Set IBENA so PDIR
	 * always reflects the pad state regardless of direction.
	 */
	reg &= ~IOMUXC_PIO_MUX_MODE_MASK;
	reg |= BIT(MCUX_RT266X_IBENA_SHIFT);

	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		reg |= BIT(MCUX_RT266X_ODENA_SHIFT);
	} else {
		reg &= ~BIT(MCUX_RT266X_ODENA_SHIFT);
	}

	reg &= ~(0x3U << MCUX_RT266X_PULLENA_SHIFT);
	if ((flags & GPIO_PULL_UP) != 0) {
		reg |= (MCUX_RT266X_PULL_UP << MCUX_RT266X_PULLENA_SHIFT);
	} else if ((flags & GPIO_PULL_DOWN) != 0) {
		reg |= (MCUX_RT266X_PULL_DOWN << MCUX_RT266X_PULLENA_SHIFT);
	}
#endif

	/*
	 * On CM85 (non-CMSE) grant this pin to the non-secure world and verify
	 * we actually have access. PCNS is a per-pin bitmap; clearing bit N
	 * allows NS access to pin N.
	 */
#if !defined(__ARM_FEATURE_CMSE) && !defined(CONFIG_CPU_CORTEX_A)
	base->PCNS &= ~BIT(pin);
	if (base->PCNS & BIT(pin)) {
		return -ENOTSUP;
	}
#endif

	memcpy(&pin_cfg.pinmux, &config->pin_muxes[cfg_idx], sizeof(pin_cfg.pinmux));
	pin_cfg.pin_ctrl_flags = reg;
	pinctrl_configure_pins(&pin_cfg, 1, PINCTRL_REG_NONE);

	if (((flags & GPIO_INPUT) != 0) && ((flags & GPIO_OUTPUT) != 0)) {
		return -ENOTSUP;
	}

	if (flags & GPIO_OUTPUT_INIT_HIGH) {
		GPIO_PinWrite(base, pin, 1);
	}

	if (flags & GPIO_OUTPUT_INIT_LOW) {
		GPIO_PinWrite(base, pin, 0);
	}

	key = k_spin_lock(&data->lock);
	WRITE_BIT(base->PDDR, pin, flags & GPIO_OUTPUT);
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int mcux_imx_gpio_port_get_raw(const struct device *dev, uint32_t *value)
{
	GPIO_Type *base = (GPIO_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);

	*value = base->PDIR;

	return 0;
}

static int mcux_imx_gpio_port_set_masked_raw(const struct device *dev,
					     uint32_t mask, uint32_t value)
{
	GPIO_Type *base = (GPIO_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);

	GPIO_PortSet(base, value & mask);
	GPIO_PortClear(base, (~value) & mask);

	return 0;
}

static int mcux_imx_gpio_port_set_bits_raw(const struct device *dev, uint32_t mask)
{
	GPIO_Type *base = (GPIO_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);

	GPIO_PortSet(base, mask);

	return 0;
}

static int mcux_imx_gpio_port_clear_bits_raw(const struct device *dev, uint32_t mask)
{
	GPIO_Type *base = (GPIO_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);

	GPIO_PortClear(base, mask);

	return 0;
}

static int mcux_imx_gpio_port_toggle_bits(const struct device *dev, uint32_t mask)
{
	GPIO_Type *base = (GPIO_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);

	GPIO_PortToggle(base, mask);

	return 0;
}

static int mcux_imx_gpio_pin_interrupt_configure(const struct device *dev,
						 gpio_pin_t pin,
						 enum gpio_int_mode mode,
						 enum gpio_int_trig trig)
{
	GPIO_Type *base = (GPIO_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	const struct mcux_imx_gpio_config *config = dev->config;
	struct mcux_imx_gpio_data *data = dev->data;
	gpio_interrupt_config_t irqc;
	k_spinlock_key_t key;

	if (mode == GPIO_INT_MODE_DISABLED) {
		irqc = kGPIO_InterruptStatusFlagDisabled;
	} else if (mode == GPIO_INT_MODE_LEVEL) {
		if (trig == GPIO_INT_TRIG_LOW) {
			irqc = kGPIO_InterruptLogicZero;
		} else if (trig == GPIO_INT_TRIG_HIGH) {
			irqc = kGPIO_InterruptLogicOne;
		} else {
			return -ENOTSUP;
		}
	} else { /* GPIO_INT_MODE_EDGE */
		if (trig == GPIO_INT_TRIG_LOW) {
			irqc = kGPIO_InterruptFallingEdge;
		} else if (trig == GPIO_INT_TRIG_HIGH) {
			irqc = kGPIO_InterruptRisingEdge;
		} else {
			irqc = kGPIO_InterruptEitherEdge;
		}
	}

	key = k_spin_lock(&data->lock);
	GPIO_SetPinInterruptConfig(base, pin, irqc);
	/*
	 * Route the interrupt to the channel selected by irq_output_select
	 * (0 = CH0, 1 = CH1). GICLR controls pins 0-15, GICHR pins 16-31.
	 */
	if (pin < 16U) {
		if (config->irq_sel == 0) {
			base->GICLR &= ~BIT(pin);
		} else {
			base->GICLR |= BIT(pin);
		}
	} else {
		uint32_t bit = pin - 16U;

		if (config->irq_sel == 0) {
			base->GICHR &= ~BIT(bit);
		} else {
			base->GICHR |= BIT(bit);
		}
	}
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int mcux_imx_gpio_manage_callback(const struct device *dev,
					 struct gpio_callback *callback, bool set)
{
	struct mcux_imx_gpio_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static void mcux_imx_gpio_port_isr(const struct device *dev)
{
	GPIO_Type *base = (GPIO_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	struct mcux_imx_gpio_data *data = dev->data;
	uint32_t int_status;

	int_status = base->ISFR[0];
	base->ISFR[0] = int_status;

	gpio_fire_callbacks(&data->callbacks, dev, int_status);
}

static DEVICE_API(gpio, mcux_imx_gpio_driver_api) = {
	.pin_configure           = mcux_imx_gpio_configure,
	.port_get_raw            = mcux_imx_gpio_port_get_raw,
	.port_set_masked_raw     = mcux_imx_gpio_port_set_masked_raw,
	.port_set_bits_raw       = mcux_imx_gpio_port_set_bits_raw,
	.port_clear_bits_raw     = mcux_imx_gpio_port_clear_bits_raw,
	.port_toggle_bits        = mcux_imx_gpio_port_toggle_bits,
	.pin_interrupt_configure = mcux_imx_gpio_pin_interrupt_configure,
	.manage_callback         = mcux_imx_gpio_manage_callback,
};

/* These macros declare an array of pinctrl_soc_pinmux types, one per pad. */
#define PINMUX_INIT(node, prop, idx) MCUX_IMX_PINMUX(DT_PROP_BY_IDX(node, prop, idx)),

#define MCUX_IMX_GPIO_PIN_DECLARE(n)						\
	const struct pinctrl_soc_pinmux mcux_imx_gpio_pinmux_##n[] = {		\
		DT_FOREACH_PROP_ELEM(DT_DRV_INST(n), pinmux, PINMUX_INIT)	\
	};

#define MCUX_IMX_GPIO_PIN_INIT(n)						\
	.pin_muxes = mcux_imx_gpio_pinmux_##n,					\
	.mux_count = DT_PROP_LEN(DT_DRV_INST(n), pinmux),			\
	.irq_sel   = DT_INST_PROP(n, irq_output_select)

#define MCUX_IMX_GPIO_IRQ_INIT(n, i)						\
	do {									\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, i, irq),			\
			    DT_INST_IRQ_BY_IDX(n, i, priority),		\
			    mcux_imx_gpio_port_isr,				\
			    DEVICE_DT_INST_GET(n), 0);				\
		irq_enable(DT_INST_IRQ_BY_IDX(n, i, irq));			\
	} while (false)

/* clang-format off */
static int mcux_imx_gpio_enable_clock(const struct mcux_imx_gpio_config *config)
{
	if (config->clock_dev == NULL) {
		return 0;
	}
	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}
	return clock_control_on(config->clock_dev, config->clock_subsys);
}

#define MCUX_IMX_GPIO_CLOCK_INIT(n)						\
	.clock_dev = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, clocks),		\
			(DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n))), (NULL)),	\
	.clock_subsys = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, clocks),		\
			((clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name)),	\
			(NULL)),

#define MCUX_IMX_GPIO_INIT(n)							\
	MCUX_IMX_GPIO_PIN_DECLARE(n)						\
	BUILD_ASSERT((DT_INST_PROP(n, irq_output_select) != 1) ||		\
		     DT_INST_IRQ_HAS_IDX(n, 1),				\
		     "irq_output_select = 1 requires a second IRQ entry");	\
									\
	static int mcux_imx_gpio_##n##_init(const struct device *dev);		\
									\
	static const struct mcux_imx_gpio_config mcux_imx_gpio_##n##_config = { \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(n),			\
		DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_DRV_INST(n)),		\
		MCUX_IMX_GPIO_PIN_INIT(n),					\
		MCUX_IMX_GPIO_CLOCK_INIT(n)					\
	};									\
									\
	static struct mcux_imx_gpio_data mcux_imx_gpio_##n##_data;		\
									\
	DEVICE_DT_INST_DEFINE(n,						\
			      mcux_imx_gpio_##n##_init,				\
			      NULL,						\
			      &mcux_imx_gpio_##n##_data,			\
			      &mcux_imx_gpio_##n##_config,			\
			      POST_KERNEL,					\
			      CONFIG_GPIO_INIT_PRIORITY,			\
			      &mcux_imx_gpio_driver_api);			\
									\
	static int mcux_imx_gpio_##n##_init(const struct device *dev)		\
	{									\
		const struct mcux_imx_gpio_config *config = dev->config;	\
		int ret;							\
									\
		DEVICE_MMIO_NAMED_MAP(dev, reg_base,				\
			K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);			\
		ret = mcux_imx_gpio_enable_clock(config);			\
		if (ret < 0) {							\
			return ret;						\
		}								\
		IF_ENABLED(DT_INST_IRQ_HAS_IDX(n,				\
			   DT_INST_PROP(n, irq_output_select)),			\
		   (MCUX_IMX_GPIO_IRQ_INIT(n,					\
			DT_INST_PROP(n, irq_output_select));))			\
		return 0;							\
	}
/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(MCUX_IMX_GPIO_INIT)
