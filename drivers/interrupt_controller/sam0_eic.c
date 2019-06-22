/*
 * Copyright (c) 2019 Derek Hageman <hageman@inthat.cloud>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <soc.h>
#include "sam0_eic.h"
#include "sam0_eic_priv.h"

struct sam0_eic_line_assignment {
	u8_t pin : 5;
	u8_t port : 2;
	u8_t enabled : 1;
};

struct sam0_eic_port_data {
	sam0_eic_callback_t cb;
	void *data;
};

struct sam0_eic_data {
	struct sam0_eic_port_data ports[PORT_GROUPS];
	struct sam0_eic_line_assignment lines[EIC_EXTINT_NUM];
};

#define DEV_DATA(dev) \
	((struct sam0_eic_data *const)(dev)->driver_data)

DEVICE_DECLARE(sam0_eic);


static void wait_synchronization(void)
{
	while (EIC->STATUS.bit.SYNCBUSY) {
	}
}

static void sam0_eic_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct sam0_eic_data *const dev_data = DEV_DATA(dev);
	u16_t bits = EIC->INTFLAG.reg;
	u32_t line_index;

	/* Acknowledge all interrupts */
	EIC->INTFLAG.reg = bits;

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
	struct device *dev = DEVICE_GET(sam0_eic);
	struct sam0_eic_data *dev_data = dev->driver_data;
	struct sam0_eic_port_data *port_data;
	struct sam0_eic_line_assignment *line_assignment;
	u32_t mask;
	int line_index;
	int config_index;
	int config_shift;
	int key;
	u32_t config;

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
	EIC->CTRL.bit.ENABLE = 0;

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

	config = EIC->CONFIG[config_index].reg;
	config &= ~(0xF << config_shift);
	switch (trigger) {
	case SAM0_EIC_RISING:
		config |= EIC_CONFIG_SENSE0_RISE << config_shift;
		break;
	case SAM0_EIC_FALLING:
		config |= EIC_CONFIG_SENSE0_FALL << config_shift;
		break;
	case SAM0_EIC_BOTH:
		config |= EIC_CONFIG_SENSE0_BOTH << config_shift;
		break;
	case SAM0_EIC_HIGH:
		config |= EIC_CONFIG_SENSE0_HIGH << config_shift;
		break;
	case SAM0_EIC_LOW:
		config |= EIC_CONFIG_SENSE0_LOW << config_shift;
		break;
	}

	if (filter) {
		config |= EIC_CONFIG_FILTEN0 << config_shift;
	}

	/* Apply the config to the EIC itself */
	EIC->CONFIG[config_index].reg = config;

	EIC->CTRL.bit.ENABLE = 1;
	wait_synchronization();
	/*
	 * Errata: The EIC generates a spurious interrupt for the newly
	 * enabled pin after being enabled, so clear it before re-enabling
	 * the IRQ.
	 */
	EIC->INTFLAG.reg = mask;
	irq_unlock(key);
	return 0;

err_in_use:
	EIC->CTRL.bit.ENABLE = 1;
	wait_synchronization();
	irq_unlock(key);
	return -EBUSY;
}

static bool sam0_eic_check_ownership(int port, int pin, int line_index)
{
	struct device *dev = DEVICE_GET(sam0_eic);
	struct sam0_eic_data *dev_data = dev->driver_data;
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
	struct device *dev = DEVICE_GET(sam0_eic);
	struct sam0_eic_data *dev_data = dev->driver_data;
	u32_t mask;
	int line_index;
	int config_index;
	int config_shift;
	int key;

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
	EIC->CTRL.bit.ENABLE = 0;
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
	EIC->CONFIG[config_index].reg &= ~(0xF << config_shift);

	/* Clear any pending interrupt for it */
	EIC->INTENCLR.reg = mask;
	EIC->INTFLAG.reg = mask;

done:
	EIC->CTRL.bit.ENABLE = 1;
	wait_synchronization();
	irq_unlock(key);
	return 0;
}

int sam0_eic_enable_interrupt(int port, int pin)
{
	u32_t mask;
	int line_index;

	line_index = sam0_eic_map_to_line(port, pin);
	if (line_index < 0) {
		return line_index;
	}

	if (!sam0_eic_check_ownership(port, pin, line_index)) {
		return -EBUSY;
	}

	mask = BIT(line_index);
	EIC->INTFLAG.reg = mask;
	EIC->INTENSET.reg = mask;

	return 0;
}

int sam0_eic_disable_interrupt(int port, int pin)
{
	u32_t mask;
	int line_index;

	line_index = sam0_eic_map_to_line(port, pin);
	if (line_index < 0) {
		return line_index;
	}

	if (!sam0_eic_check_ownership(port, pin, line_index)) {
		return -EBUSY;
	}

	mask = BIT(line_index);
	EIC->INTENCLR.reg = mask;
	EIC->INTFLAG.reg = mask;

	return 0;
}

u32_t sam0_eic_interrupt_pending(int port)
{
	struct device *dev = DEVICE_GET(sam0_eic);
	struct sam0_eic_data *dev_data = dev->driver_data;
	struct sam0_eic_line_assignment *line_assignment;
	u32_t set = EIC->INTFLAG.reg;
	u32_t mask = 0;

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
		IRQ_CONNECT(DT_INST_0_ATMEL_SAM0_EIC_IRQ_ ## n,		\
			    DT_INST_0_ATMEL_SAM0_EIC_IRQ_ ## n ## _PRIORITY,	\
			    sam0_eic_isr, DEVICE_GET(sam0_eic), 0);	\
		irq_enable(DT_INST_0_ATMEL_SAM0_EIC_IRQ_ ## n);		\
	} while (0)

static int sam0_eic_init(struct device *dev)
{
	ARG_UNUSED(dev);

	/* Enable the EIC clock in PM */
	PM->APBAMASK.bit.EIC_ = 1;

	/* Enable the GCLK */
	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID_EIC | GCLK_CLKCTRL_GEN_GCLK0 |
			    GCLK_CLKCTRL_CLKEN;

#ifdef DT_INST_0_ATMEL_SAM0_EIC_IRQ_0
	SAM0_EIC_IRQ_CONNECT(0);
#endif
#ifdef DT_INST_0_ATMEL_SAM0_EIC_IRQ_1
	SAM0_EIC_IRQ_CONNECT(1);
#endif
#ifdef DT_INST_0_ATMEL_SAM0_EIC_IRQ_2
	SAM0_EIC_IRQ_CONNECT(2);
#endif
#ifdef DT_INST_0_ATMEL_SAM0_EIC_IRQ_3
	SAM0_EIC_IRQ_CONNECT(3);
#endif
#ifdef DT_INST_0_ATMEL_SAM0_EIC_IRQ_4
	SAM0_EIC_IRQ_CONNECT(4);
#endif
#ifdef DT_INST_0_ATMEL_SAM0_EIC_IRQ_5
	SAM0_EIC_IRQ_CONNECT(5);
#endif
#ifdef DT_INST_0_ATMEL_SAM0_EIC_IRQ_6
	SAM0_EIC_IRQ_CONNECT(6);
#endif
#ifdef DT_INST_0_ATMEL_SAM0_EIC_IRQ_7
	SAM0_EIC_IRQ_CONNECT(7);
#endif
#ifdef DT_INST_0_ATMEL_SAM0_EIC_IRQ_8
	SAM0_EIC_IRQ_CONNECT(8);
#endif
#ifdef DT_INST_0_ATMEL_SAM0_EIC_IRQ_9
	SAM0_EIC_IRQ_CONNECT(9);
#endif
#ifdef DT_INST_0_ATMEL_SAM0_EIC_IRQ_10
	SAM0_EIC_IRQ_CONNECT(10);
#endif
#ifdef DT_INST_0_ATMEL_SAM0_EIC_IRQ_11
	SAM0_EIC_IRQ_CONNECT(11);
#endif
#ifdef DT_INST_0_ATMEL_SAM0_EIC_IRQ_12
	SAM0_EIC_IRQ_CONNECT(12);
#endif
#ifdef DT_INST_0_ATMEL_SAM0_EIC_IRQ_13
	SAM0_EIC_IRQ_CONNECT(13);
#endif
#ifdef DT_INST_0_ATMEL_SAM0_EIC_IRQ_14
	SAM0_EIC_IRQ_CONNECT(14);
#endif
#ifdef DT_INST_0_ATMEL_SAM0_EIC_IRQ_15
	SAM0_EIC_IRQ_CONNECT(15);
#endif

	EIC->CTRL.bit.ENABLE = 1;
	wait_synchronization();

	return 0;
}

static struct sam0_eic_data eic_data;
DEVICE_INIT(sam0_eic, DT_INST_0_ATMEL_SAM0_EIC_LABEL, sam0_eic_init,
	    &eic_data, NULL,
	    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
