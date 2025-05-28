/*
 * Copyright (c) 2019 Derek Hageman <hageman@inthat.cloud>
 * Copyright (c) 2025 GP Orcullo
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam0_eic

#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <soc.h>
#include <zephyr/drivers/interrupt_controller/sam0_eic.h>
#include "intc_sam0_eic_priv.h"

struct sam0_eic_line_assignment {
	uint8_t pin : 5;
	uint8_t port : 2;
	uint8_t enabled : 1;
};

struct sam0_eic_port_data {
	sam0_eic_callback_t cb;
	void *data;
};

struct sam0_eic_data {
	struct sam0_eic_port_data ports[NUM_PORT_GROUPS];
	struct sam0_eic_line_assignment lines[EIC_EXTINT_NUM];
};

struct sam0_eic_config {
	uintptr_t base;
};

static void wait_synchronization(void)
{
	const struct device *const dev = DEVICE_DT_INST_GET(0);
	const struct sam0_eic_config *config = dev->config;

#if !defined(CONFIG_SOC_SERIES_SAMD20) && !defined(CONFIG_SOC_SERIES_SAMD21) &&                    \
	!defined(CONFIG_SOC_SERIES_SAMR21)
	while (sys_read32(config->base + SYNCBUSY_OFFSET)) {
	}
#else
	while (IS_BIT_SET(sys_read8(config->base + STATUS_OFFSET), SYNCBUSY_BIT)) {
	}
#endif
}

static inline void set_eic_enable(bool on)
{
	const struct device *const dev = DEVICE_DT_INST_GET(0);
	const struct sam0_eic_config *config = dev->config;
	uint8_t ctrl = sys_read8(config->base);

	WRITE_BIT(ctrl, EIC_ENABLE_BIT, on);
	sys_write8(ctrl, config->base);
}

static void sam0_eic_isr(const struct device *dev)
{
	struct sam0_eic_data *const dev_data = dev->data;
	const struct sam0_eic_config *config = dev->config;
	uint32_t bits = sys_read32(config->base + INTFLAG_OFFSET);
	uint32_t line_index;

	/* Acknowledge all interrupts */
	sys_write32(bits, config->base + INTFLAG_OFFSET);

	/* No clz on M0, so just do a quick test */
#if __CORTEX_M >= 3
	line_index = __CLZ(__RBIT(bits));
	bits >>= line_index;
#else
	if (bits & 0xFF) {
		line_index = 0;
	} else {
		line_index = 8;
		bits >>= 8;
	}
#endif

	/*
	 * Map the EIC lines to the port pin masks based on which port is
	 * selected in the line data.
	 */
	for (; bits; bits >>= 1, line_index++) {
		if (!(bits & 1)) {
			continue;
		}

		/*
		 * These could be aggregated together into one call, but
		 * usually on a single one will be set, so just call them
		 * one by one.
		 */
		struct sam0_eic_line_assignment *line_assignment =
			&dev_data->lines[line_index];
		struct sam0_eic_port_data *port_data =
			&dev_data->ports[line_assignment->port];

		port_data->cb(BIT(line_assignment->pin), port_data->data);
	}
}

int sam0_eic_acquire(int port, int pin, enum sam0_eic_trigger trigger,
		     bool filter, sam0_eic_callback_t cb, void *data)
{
	const struct device *const dev = DEVICE_DT_INST_GET(0);
	const struct sam0_eic_config *config = dev->config;
	struct sam0_eic_data *dev_data = dev->data;
	struct sam0_eic_port_data *port_data;
	struct sam0_eic_line_assignment *line_assignment;
	uint32_t mask;
	int line_index;
	int config_index;
	int config_shift;
	unsigned int key;
	uint32_t cfg;

	line_index = sam0_eic_map_to_line(port, pin);
	if (line_index < 0) {
		return line_index;
	}

	mask = BIT(line_index);
	config_index = line_index / 8;
	config_shift = (line_index % 8) * 4;

	/* Lock everything so it's safe to reconfigure */
	key = irq_lock();
	/* Disable the EIC for reconfiguration */
	set_eic_enable(0);

	line_assignment = &dev_data->lines[line_index];

	/* Check that the required line is available */
	if (line_assignment->enabled) {
		if (line_assignment->port != port ||
		    line_assignment->pin != pin) {
			goto err_in_use;
		}
	}

	/* Set the EIC configuration data */
	port_data = &dev_data->ports[port];
	port_data->cb = cb;
	port_data->data = data;
	line_assignment->pin = pin;
	line_assignment->port = port;
	line_assignment->enabled = 1;

	cfg = sys_read32(config->base + CFG_OFFSET + (config_index * 4));
	cfg &= ~(0xF << config_shift);
	switch (trigger) {
	case SAM0_EIC_RISING:
		cfg |= CFG_SENSE0_RISE << config_shift;
		break;
	case SAM0_EIC_FALLING:
		cfg |= CFG_SENSE0_FALL << config_shift;
		break;
	case SAM0_EIC_BOTH:
		cfg |= CFG_SENSE0_BOTH << config_shift;
		break;
	case SAM0_EIC_HIGH:
		cfg |= CFG_SENSE0_HIGH << config_shift;
		break;
	case SAM0_EIC_LOW:
		cfg |= CFG_SENSE0_LOW << config_shift;
		break;
	}

	if (filter) {
		cfg |= CFG_FILTEN0 << config_shift;
	}

	/* Apply the config to the EIC itself */
	sys_write32(cfg, config->base + CFG_OFFSET + (config_index * 4));

	set_eic_enable(1);
	wait_synchronization();
	/*
	 * Errata: The EIC generates a spurious interrupt for the newly
	 * enabled pin after being enabled, so clear it before re-enabling
	 * the IRQ.
	 */
	sys_write32(mask, config->base + INTFLAG_OFFSET);
	irq_unlock(key);
	return 0;

err_in_use:
	set_eic_enable(1);
	wait_synchronization();
	irq_unlock(key);
	return -EBUSY;
}

static bool sam0_eic_check_ownership(int port, int pin, int line_index)
{
	const struct device *const dev = DEVICE_DT_INST_GET(0);
	struct sam0_eic_data *dev_data = dev->data;
	struct sam0_eic_line_assignment *line_assignment =
		&dev_data->lines[line_index];

	if (!line_assignment->enabled) {
		return false;
	}

	if (line_assignment->port != port ||
	    line_assignment->pin != pin) {
		return false;
	}

	return true;
}

int sam0_eic_release(int port, int pin)
{
	const struct device *const dev = DEVICE_DT_INST_GET(0);
	const struct sam0_eic_config *config = dev->config;
	struct sam0_eic_data *dev_data = dev->data;
	uint32_t mask;
	uint32_t cfg;
	int line_index;
	int config_index;
	int config_shift;
	unsigned int key;

	line_index = sam0_eic_map_to_line(port, pin);
	if (line_index < 0) {
		return line_index;
	}

	mask = BIT(line_index);
	config_index = line_index / 8;
	config_shift = (line_index % 8) * 4;

	/* Lock everything so it's safe to reconfigure */
	key = irq_lock();
	/* Disable the EIC */
	set_eic_enable(0);
	wait_synchronization();

	/*
	 * Check to make sure the requesting actually owns the line and do
	 * nothing if it does not.
	 */
	if (!sam0_eic_check_ownership(port, pin, line_index)) {
		goto done;
	}

	dev_data->lines[line_index].enabled = 0;

	/* Clear the EIC config, including the trigger condition */
	cfg = sys_read32(config->base + CFG_OFFSET + (config_index * 4));
	cfg &= ~(0xF << config_shift);
	sys_write32(cfg, config->base + CFG_OFFSET + (config_index * 4));

	/* Clear any pending interrupt for it */
	sys_write32(mask, config->base + INTENCLR_OFFSET);
	sys_write32(mask, config->base + INTFLAG_OFFSET);

done:
	set_eic_enable(1);
	wait_synchronization();
	irq_unlock(key);
	return 0;
}

int sam0_eic_enable_interrupt(int port, int pin)
{
	const struct device *const dev = DEVICE_DT_INST_GET(0);
	const struct sam0_eic_config *config = dev->config;
	uint32_t mask;
	int line_index;

	line_index = sam0_eic_map_to_line(port, pin);
	if (line_index < 0) {
		return line_index;
	}

	if (!sam0_eic_check_ownership(port, pin, line_index)) {
		return -EBUSY;
	}

	mask = BIT(line_index);
	sys_write32(mask, config->base + INTFLAG_OFFSET);
	sys_write32(mask, config->base + INTENSET_OFFSET);

	return 0;
}

int sam0_eic_disable_interrupt(int port, int pin)
{
	const struct device *const dev = DEVICE_DT_INST_GET(0);
	const struct sam0_eic_config *config = dev->config;
	uint32_t mask;
	int line_index;

	line_index = sam0_eic_map_to_line(port, pin);
	if (line_index < 0) {
		return line_index;
	}

	if (!sam0_eic_check_ownership(port, pin, line_index)) {
		return -EBUSY;
	}

	mask = BIT(line_index);
	sys_write32(mask, config->base + INTENCLR_OFFSET);
	sys_write32(mask, config->base + INTFLAG_OFFSET);

	return 0;
}

uint32_t sam0_eic_interrupt_pending(int port)
{
	const struct device *const dev = DEVICE_DT_INST_GET(0);
	const struct sam0_eic_config *config = dev->config;
	struct sam0_eic_data *dev_data = dev->data;
	struct sam0_eic_line_assignment *line_assignment;
	uint32_t set = sys_read32(config->base + INTFLAG_OFFSET);
	uint32_t mask = 0;

	for (int line_index = 0; line_index < EIC_EXTINT_NUM; line_index++) {
		line_assignment = &dev_data->lines[line_index];

		if (!line_assignment->enabled) {
			continue;
		}

		if (line_assignment->port != port) {
			continue;
		}

		if (!(set & BIT(line_index))) {
			continue;
		}

		mask |= BIT(line_assignment->pin);
	}

	return mask;
}


#define SAM0_EIC_IRQ_CONNECT(n)						\
	do {								\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(0, n, irq),		\
			    DT_INST_IRQ_BY_IDX(0, n, priority),		\
			    sam0_eic_isr, DEVICE_DT_INST_GET(0), 0);	\
		irq_enable(DT_INST_IRQ_BY_IDX(0, n, irq));		\
	} while (false)

static int sam0_eic_init(const struct device *dev)
{
	const uintptr_t mclk = DT_REG_ADDR(DT_INST(0, atmel_sam0_mclk));
	const uintptr_t gclk = DT_REG_ADDR(DT_INST(0, atmel_sam0_gclk));

	/* Enable the EIC clock in APBAMASK */
	sys_set_bit(mclk + APBAMASK_OFFSET, APBAMASK_EIC_BIT);

	/* Enable the GCLK */
#ifdef PCHCTRL_GEN_GCLK0
	sys_write32(PCHCTRL_GEN_GCLK0 | PCHCTRL_CHEN, gclk + PCHCTRL_OFFSET + (4 * GCLK_ID));
#else
	sys_write16(CLKCTRL_ID_EIC | CLKCTRL_GEN_GCLK0 | CLKCTRL_CLKEN, gclk + CLKCTRL_OFFSET);
#endif

#if DT_INST_IRQ_HAS_CELL(0, irq)
	SAM0_EIC_IRQ_CONNECT(0);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 1)
	SAM0_EIC_IRQ_CONNECT(1);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 2)
	SAM0_EIC_IRQ_CONNECT(2);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 3)
	SAM0_EIC_IRQ_CONNECT(3);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 4)
	SAM0_EIC_IRQ_CONNECT(4);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 5)
	SAM0_EIC_IRQ_CONNECT(5);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 6)
	SAM0_EIC_IRQ_CONNECT(6);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 7)
	SAM0_EIC_IRQ_CONNECT(7);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 8)
	SAM0_EIC_IRQ_CONNECT(8);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 9)
	SAM0_EIC_IRQ_CONNECT(9);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 10)
	SAM0_EIC_IRQ_CONNECT(10);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 11)
	SAM0_EIC_IRQ_CONNECT(11);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 12)
	SAM0_EIC_IRQ_CONNECT(12);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 13)
	SAM0_EIC_IRQ_CONNECT(13);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 14)
	SAM0_EIC_IRQ_CONNECT(14);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 15)
	SAM0_EIC_IRQ_CONNECT(15);
#endif

	set_eic_enable(1);
	wait_synchronization();

	return 0;
}

static struct sam0_eic_data eic_data;
static const struct sam0_eic_config eic_config = {
	.base = DT_INST_REG_ADDR(0),
};

DEVICE_DT_INST_DEFINE(0, sam0_eic_init, NULL, &eic_data, &eic_config, PRE_KERNEL_1,
		      CONFIG_INTC_INIT_PRIORITY, NULL);
