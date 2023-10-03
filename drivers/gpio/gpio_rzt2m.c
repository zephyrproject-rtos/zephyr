/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rzt2m_gpio

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/errno_private.h>
#include <zephyr/dt-bindings/gpio/renesas-rzt2m-gpio.h>
#include <soc.h>

#define PMm_OFFSET    0x200
#define PINm_OFFSET   0x800
#define DRCTLm_OFFSET 0xa00

#define DRIVE_SHIFT           0
#define SCHMITT_TRIGGER_SHIFT 4
#define SLEW_RATE_SHIFT       5

#define PULL_SHIFT 2
#define PULL_NONE  (0 << PULL_SHIFT)
#define PULL_UP    (1 << PULL_SHIFT)
#define PULL_DOWN  (2 << PULL_SHIFT)

struct rzt2m_gpio_config {
	struct gpio_driver_config common;
	uint8_t *port_nsr;
	uint8_t *ptadr;
	uint8_t port;
};

struct rzt2m_gpio_data {
	struct gpio_driver_data common;
};

static void rzt2m_gpio_unlock(void)
{
	rzt2m_unlock_prcrn(PRCRN_PRC1);
	rzt2m_unlock_prcrs(PRCRS_GPIO);
}

static void rzt2m_gpio_lock(void)
{
	rzt2m_lock_prcrn(PRCRN_PRC1);
	rzt2m_lock_prcrs(PRCRS_GPIO);
}

/* Port m output data store */
static volatile uint8_t *rzt2m_gpio_get_p_reg(const struct device *dev)
{
	const struct rzt2m_gpio_config *config = dev->config;

	return (volatile uint8_t *)(config->port_nsr + config->port);
}

/* Port m input data store */
static volatile uint8_t *rzt2m_gpio_get_pin_reg(const struct device *dev)
{
	const struct rzt2m_gpio_config *config = dev->config;

	return (volatile uint8_t *)(config->port_nsr + PINm_OFFSET + config->port);
}

/* Port m mode register */
static volatile uint16_t *rzt2m_gpio_get_pm_reg(const struct device *dev)
{
	const struct rzt2m_gpio_config *config = dev->config;

	return (volatile uint16_t *)(config->port_nsr + PMm_OFFSET + 0x2 * config->port);
}

/* IO Buffer m function switching register */
static volatile uint64_t *rzt2m_gpio_get_drctl_reg(const struct device *dev)
{
	const struct rzt2m_gpio_config *config = dev->config;

	return (volatile uint64_t *)(config->port_nsr + DRCTLm_OFFSET + 0x8 * config->port);
}

/* Port m region select register */
static volatile uint8_t *rzt2m_gpio_get_rselp_reg(const struct device *dev)
{
	const struct rzt2m_gpio_config *config = dev->config;

	return (volatile uint8_t *)(config->ptadr + config->port);
}

static int rzt2m_gpio_init(const struct device *dev)
{
	rzt2m_gpio_unlock();

	volatile uint8_t *rselp_reg = rzt2m_gpio_get_rselp_reg(dev);
	*rselp_reg = 0xFF;

	rzt2m_gpio_lock();

	return 0;
}

static int rzt2m_gpio_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	rzt2m_gpio_unlock();

	volatile uint8_t *pin_reg = rzt2m_gpio_get_pin_reg(dev);
	*value = *pin_reg;

	rzt2m_gpio_lock();

	return 0;
}

static int rzt2m_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
				     gpio_port_value_t value)
{
	rzt2m_gpio_unlock();

	volatile uint8_t *p_reg = rzt2m_gpio_get_p_reg(dev);
	*p_reg = (*p_reg & ~mask) | (value & mask);

	rzt2m_gpio_lock();

	return 0;
}

static int rzt2m_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	rzt2m_gpio_unlock();

	volatile uint8_t *p_reg = rzt2m_gpio_get_p_reg(dev);
	*p_reg |= pins;

	rzt2m_gpio_lock();

	return 0;
}

static int rzt2m_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	rzt2m_gpio_unlock();

	volatile uint8_t *p_reg = rzt2m_gpio_get_p_reg(dev);
	*p_reg &= ~pins;

	rzt2m_gpio_lock();

	return 0;
}

static int rzt2m_gpio_toggle(const struct device *dev, gpio_port_pins_t pins)
{
	rzt2m_gpio_unlock();

	volatile uint8_t *p_reg = rzt2m_gpio_get_p_reg(dev);
	*p_reg ^= pins;

	rzt2m_gpio_lock();

	return 0;
}

static int rzt2m_gpio_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	volatile uint16_t *pm_reg = rzt2m_gpio_get_pm_reg(dev);
	volatile uint64_t *drctl_reg = rzt2m_gpio_get_drctl_reg(dev);

	rzt2m_gpio_unlock();

	WRITE_BIT(*pm_reg, pin * 2, flags & GPIO_INPUT);
	WRITE_BIT(*pm_reg, pin * 2 + 1, flags & GPIO_OUTPUT);

	if (flags & GPIO_OUTPUT) {
		if (flags & GPIO_OUTPUT_INIT_LOW) {
			rzt2m_port_clear_bits_raw(dev, 1 << pin);
		} else if (flags & GPIO_OUTPUT_INIT_HIGH) {
			rzt2m_port_set_bits_raw(dev, 1 << pin);
		}
	}

	if (flags & GPIO_PULL_UP && flags & GPIO_PULL_DOWN) {
		rzt2m_gpio_lock();
		return -EINVAL;
	}

	uint8_t drctl_pin_config = 0;

	if (flags & GPIO_PULL_UP) {
		drctl_pin_config |= PULL_UP;
	} else if (flags & GPIO_PULL_DOWN) {
		drctl_pin_config |= PULL_DOWN;
	} else {
		drctl_pin_config |= PULL_NONE;
	}

	drctl_pin_config |=
		(flags & RZT2M_GPIO_DRIVE_MASK) >> (RZT2M_GPIO_DRIVE_OFFSET - DRIVE_SHIFT);
	drctl_pin_config |= (flags & RZT2M_GPIO_SCHMITT_TRIGGER_MASK) >>
			    (RZT2M_GPIO_SCHMITT_TRIGGER_OFFSET - SCHMITT_TRIGGER_SHIFT);
	drctl_pin_config |= (flags & RZT2M_GPIO_SLEW_RATE_MASK) >>
			    (RZT2M_GPIO_SLEW_RATE_OFFSET - SLEW_RATE_SHIFT);

	uint64_t drctl_pin_value = *drctl_reg & ~(0xFFULL << (pin * 8));
	*drctl_reg = drctl_pin_value | ((uint64_t)drctl_pin_config << (pin * 8));

	rzt2m_gpio_lock();

	return 0;
}

static const struct gpio_driver_api rzt2m_gpio_driver_api = {
	.pin_configure = rzt2m_gpio_configure,
	.port_get_raw = rzt2m_gpio_get_raw,
	.port_set_masked_raw = rzt2m_port_set_masked_raw,
	.port_set_bits_raw = rzt2m_port_set_bits_raw,
	.port_clear_bits_raw = rzt2m_port_clear_bits_raw,
	.port_toggle_bits = rzt2m_gpio_toggle};

#define RZT2M_GPIO_DEFINE(inst)                                                                    \
	static struct rzt2m_gpio_data rzt2m_gpio_data##inst;                                       \
	static struct rzt2m_gpio_config rzt2m_gpio_config##inst = {                                \
		.port_nsr = (uint8_t *)DT_REG_ADDR_BY_NAME(DT_INST_PARENT(inst), port_nsr),        \
		.ptadr = (uint8_t *)DT_REG_ADDR_BY_NAME(DT_INST_PARENT(inst), ptadr),              \
		.port = DT_INST_REG_ADDR(inst),                                                    \
		.common = {.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(inst)}};               \
	DEVICE_DT_INST_DEFINE(inst, rzt2m_gpio_init, NULL, &rzt2m_gpio_data##inst,                 \
			      &rzt2m_gpio_config##inst, PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,   \
			      &rzt2m_gpio_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RZT2M_GPIO_DEFINE)
