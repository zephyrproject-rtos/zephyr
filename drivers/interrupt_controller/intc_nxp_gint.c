/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/interrupt_controller/intc_nxp_gint.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/irq.h>

/*
 * GINT (Group GPIO Input Interrupt) Controller Driver
 *
 * The GINT peripheral provides grouped GPIO interrupt functionality, allowing
 * multiple GPIO pins to be combined into a single interrupt source. Key features:
 * - Supports monitoring multiple GPIO ports and pins
 * - Configurable polarity (rising/falling edge or high/low level) per pin
 * - Combinational logic modes: AND or OR of selected pins
 * - Edge or level triggered interrupt generation
 *
 * NOTE: The pin don't need to be configured as GPIO input, GINT can monitor
 * any pin configured as digital functions.
 */

#define DT_DRV_COMPAT nxp_gint

LOG_MODULE_REGISTER(nxp_gint, CONFIG_INTC_LOG_LEVEL);

/* GINT register offsets */
#define GINT_CTRL_OFFSET		0x000
#define GINT_PORT_POLN_OFFSET(n)	(0x020 + 4 * (n))
#define GINT_PORT_ENAN_OFFSET(n)	(0x040 + 4 * (n))

/* GINT CTRL register bits */
#define GINT_CTRL_INT_BIT		BIT(0)
#define GINT_CTRL_COMB_BIT		BIT(1)
#define GINT_CTRL_TRIG_BIT		BIT(2)

/* Max number of PORT the GINT support. */
#ifdef GINT_PORT_POL_COUNT
#define GINT_PORT_COUNT GINT_PORT_POL_COUNT
#else
#define GINT_PORT_COUNT 1
#endif

struct nxp_gint_config {
	/** GINT base address */
	uintptr_t base;
	/** IRQ number */
	uint32_t irq;
	/** IRQ priority */
	uint32_t irq_priority;
	/** Reset controller specification */
	struct reset_dt_spec reset;
	/** IRQ initialization function */
	void (*irq_init_func)(void);
};

struct nxp_gint_data {
	/** Interrupt callback */
	nxp_gint_callback_t callback;
	/** User data for callback */
	void *user_data;
	/** Spinlock for thread-safe access */
	struct k_spinlock lock;
};

static inline uint32_t gint_read_reg(const struct nxp_gint_config *config, uint32_t offset)
{
	return sys_read32(config->base + offset);
}

static inline void gint_write_reg(const struct nxp_gint_config *config,
				  uint32_t offset,
				  uint32_t value)
{
	sys_write32(value, config->base + offset);
}

static void nxp_gint_isr(const struct device *dev)
{
	struct nxp_gint_data *data = dev->data;
	const struct nxp_gint_config *config = dev->config;
	uint32_t ctrl;

	ctrl = gint_read_reg(config, GINT_CTRL_OFFSET);

	/* Clear interrupt flag */
	if (0U != (ctrl & GINT_CTRL_INT_BIT)) {
		gint_write_reg(config, GINT_CTRL_OFFSET, ctrl);

		/* Call user callback if registered */
		if (data->callback != NULL) {
			data->callback(dev, data->user_data);
		}
	}

}

int nxp_gint_configure_group(const struct device *dev, const struct nxp_gint_group_config *config)
{
	uint32_t ctrl;
	const struct nxp_gint_config *hw_config;

	if (!dev || !config) {
		return -EINVAL;
	}

	hw_config = dev->config;

	/* Configure control register */
	ctrl = 0;
	if (config->combination) {
		ctrl |= GINT_CTRL_COMB_BIT;
	}
	if (config->trigger) {
		ctrl |= GINT_CTRL_TRIG_BIT;
	}
	gint_write_reg(hw_config, GINT_CTRL_OFFSET, ctrl);

	return 0;
}

int nxp_gint_enable_pin(const struct device *dev, uint8_t port, uint8_t pin,
			nxp_gint_polarity_type polarity)
{
	uint32_t reg;

	const struct nxp_gint_config *config = dev->config;
	struct nxp_gint_data *data = dev->data;

	/* Validate port number */
	if (port >= GINT_PORT_COUNT) {
		LOG_ERR("Invalid port number: %d (max: %d)", port, GINT_PORT_COUNT - 1);
		return -EINVAL;
	}

	/* Validate pin number */
	if (pin > 31) {
		LOG_ERR("Invalid pin number: %d (max: 31)", pin);
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		/* Set polarity */
		reg = gint_read_reg(config, GINT_PORT_POLN_OFFSET(port));
		if (polarity == NXP_GINT_POL_HIGH) {
			reg |= BIT(pin);
		} else {
			reg &= ~BIT(pin);
		}
		gint_write_reg(config, GINT_PORT_POLN_OFFSET(port), reg);

		/* Enable pin */
		reg = gint_read_reg(config, GINT_PORT_ENAN_OFFSET(port));
		reg |= BIT(pin);
		gint_write_reg(config, GINT_PORT_ENAN_OFFSET(port), reg);
	}

	LOG_DBG("Enabled port %d pin %d with polarity %d", port, pin, polarity);

	return 0;
}

int nxp_gint_disable_pin(const struct device *dev, uint8_t port, uint8_t pin)
{
	uint32_t reg;
	const struct nxp_gint_config *config = dev->config;
	struct nxp_gint_data *data = dev->data;

	/* Validate port number */
	if (port >= GINT_PORT_COUNT) {
		LOG_ERR("Invalid port number: %d (max: %d)", port, GINT_PORT_COUNT - 1);
		return -EINVAL;
	}

	/* Validate pin number */
	if (pin > 31) {
		LOG_ERR("Invalid pin number: %d (max: 31)", pin);
		return -EINVAL;
	}

	/* Disable pin */
	K_SPINLOCK(&data->lock) {
		reg = gint_read_reg(config, GINT_PORT_ENAN_OFFSET(port));
		reg &= ~BIT(pin);
		gint_write_reg(config, GINT_PORT_ENAN_OFFSET(port), reg);
	}

	LOG_DBG("Disabled port %d pin %d", port, pin);

	return 0;
}

bool nxp_gint_is_pending(const struct device *dev)
{
	uint32_t ctrl;

	if (!dev) {
		return false;
	}

	const struct nxp_gint_config *config = dev->config;

	/* Read control register and check interrupt bit */
	ctrl = gint_read_reg(config, GINT_CTRL_OFFSET);
	return (ctrl & GINT_CTRL_INT_BIT) != 0;
}

int nxp_gint_clear_pending(const struct device *dev)
{
	uint32_t ctrl;

	if (!dev) {
		return -EINVAL;
	}

	const struct nxp_gint_config *config = dev->config;

	/* Clear interrupt flag, the flag can be cleared by writing 1 to it */
	ctrl = gint_read_reg(config, GINT_CTRL_OFFSET);
	if (0U != (ctrl & GINT_CTRL_INT_BIT)) {
		gint_write_reg(config, GINT_CTRL_OFFSET, ctrl);
	}

	LOG_DBG("Cleared pending interrupt");

	return 0;
}

int nxp_gint_register_callback(const struct device *dev,
			       nxp_gint_callback_t callback,
			       void *user_data)
{
	if (!dev) {
		return -EINVAL;
	}

	struct nxp_gint_data *data = dev->data;

	data->callback = callback;
	data->user_data = user_data;

	LOG_DBG("Registered callback");

	return 0;
}

static void nxp_gint_reset_reg(const struct device *dev)
{
	const struct nxp_gint_config *config = dev->config;

	for (int port = 0; port < GINT_PORT_COUNT; port++) {
		gint_write_reg(config, GINT_PORT_ENAN_OFFSET(port), 0U);
	}

	gint_write_reg(config, GINT_CTRL_OFFSET, GINT_CTRL_INT_BIT);
}

static int nxp_gint_init(const struct device *dev)
{
	const struct nxp_gint_config *config = dev->config;
	struct nxp_gint_data *data = dev->data;

	/* Initialize data structure */
	data->callback = NULL;
	data->user_data = NULL;

	/* Release the GINT controller from reset */
	(void)reset_line_deassert_dt(&config->reset);

	/* Clear the registers */
	nxp_gint_reset_reg(dev);

	/* Init the interrupt */
	config->irq_init_func();

	return 0;
}

#define NXP_GINT_IRQ_CONFIG(n)							\
	static void nxp_gint_irq_init_##n(void)					\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n),					\
			    DT_INST_IRQ(n, priority),				\
			    nxp_gint_isr,					\
			    DEVICE_DT_INST_GET(n),				\
			    0);							\
		irq_enable(DT_INST_IRQN(n));					\
	}

#define NXP_GINT_INIT(n)							\
	NXP_GINT_IRQ_CONFIG(n)							\
	static const struct nxp_gint_config nxp_gint_config_##n = {		\
		.base		= DT_INST_REG_ADDR(n),				\
		.irq		= DT_INST_IRQN(n),				\
		.irq_priority	= DT_INST_IRQ(n, priority),			\
		.reset		= RESET_DT_SPEC_INST_GET(n),			\
		.irq_init_func	= nxp_gint_irq_init_##n,			\
	};									\
										\
	static struct nxp_gint_data nxp_gint_data_##n;				\
										\
	DEVICE_DT_INST_DEFINE(n, nxp_gint_init, NULL,				\
			&nxp_gint_data_##n, &nxp_gint_config_##n,		\
			POST_KERNEL, CONFIG_INTC_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(NXP_GINT_INIT)
