/*
 * Copyright (c) 2018 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gpio.h>
#include <soc.h>

#if CONFIG_EIC_SAM0_BASE_ADDRESS

#include "gpio_sam0.h"
#include "gpio_utils.h"

/* Flags for packing the target port into a u8_t */
#define EIC_TARGET_PORT_MASK 0x0f
#define EIC_TARGET_GPIO_HIGH 0x40
#define EIC_TARGET_GPIO_USED 0x80

#define REGS ((Eic *)CONFIG_EIC_SAM0_BASE_ADDRESS)

DEVICE_DECLARE(eic_sam0_0);

struct eic_sam0_data {
	struct device *ports[PORT_GROUPS];
	sys_slist_t cbs[PORT_GROUPS];
	/* Records which port and half is mapped for each channel */
	u8_t targets[EIC_EXTINT_NUM];
};

/* Wait for the external interrupt controller to synchronise */
static void eic_sam0_sync(void)
{
	while (REGS->STATUS.bit.SYNCBUSY) {
	}
}

static void eic_sam0_isr(struct device *dev)
{
	struct eic_sam0_data *data = dev->driver_data;
	u32_t flags = REGS->INTFLAG.reg;
	int pin = 0;

	/* Acknowledge all interrupts */
	REGS->INTFLAG.reg = flags;

	/* The Cortex-M0+ does't have clz.  flags is normally
	 * sparse so do a small optimisation for when high
	 * bits only are set.
	 */
	if ((flags & 0xFF) == 0) {
		flags >>= 8;
		pin += 8;
	}

	/* Scan through the triggered interrupts, find the configured
	 * port and port half, and invoke the callback.  Note that
	 * scans the flags by mask, then checks the port by index,
	 * then dispatches by mask.
	 */
	while (flags != 0) {
		if ((flags & 1) != 0) {
			u8_t target = data->targets[pin];
			int id = target & EIC_TARGET_PORT_MASK;
			struct device *port = data->ports[id];
			u32_t pins = 1 << pin;

			if ((target & EIC_TARGET_GPIO_HIGH) != 0) {
				pins <<= 16;
			}

			_gpio_fire_callbacks(&data->cbs[id], port, pins);
		}

		flags >>= 1;
		pin++;
	}
}

u8_t eic_sam0_get_target(struct device *port, u32_t pin)
{
	/* The 16 EIC channels are shared between ports and also
	 * within a port.  Pin 0 and 16 map to channel 0, pin 1 and 17
	 * to channel 1, etc.  Only one pin on one port can map to a
	 * EIC channel at a time.
	 */

	const struct gpio_sam0_config *cfg = port->config->config_info;
	u8_t target = cfg->id | EIC_TARGET_GPIO_USED;

	if (pin >= EIC_EXTINT_NUM) {
		target |= EIC_TARGET_GPIO_HIGH;
	}

	return target;
}

/* Configure the channel using the GPIO style flags */
int eic_sam0_config(u8_t target, u8_t extint, int flags)
{
	struct device *dev = DEVICE_GET(eic_sam0_0);
	struct eic_sam0_data *data = dev->driver_data;
	int idx = extint / 8;
	int nibble = extint % 8;
	u32_t mask = 1 << extint;
	bool edge = (flags & GPIO_INT_EDGE) != 0;
	bool high = (flags & GPIO_INT_ACTIVE_HIGH) != 0;
	u32_t config;
	u32_t cmask;

	if ((flags & GPIO_INT) == 0) {
		/* Disable the interrupt if it was mapped */
		if (data->targets[extint] == target) {
			REGS->INTENCLR.reg = mask;
			data->targets[extint] = 0;
		}

		return 0;
	}

	if (data->targets[extint] != 0 && data->targets[extint] != target) {
		/* A different pin is already using the target */
		return -EBUSY;
	}

	/* Configure the interrupt */
	data->targets[extint] = target;

	if ((flags & GPIO_INT_DOUBLE_EDGE) != 0) {
		config = EIC_CONFIG_SENSE0_BOTH;
	} else if (edge) {
		config = high ? EIC_CONFIG_SENSE0_RISE
			: EIC_CONFIG_SENSE0_FALL;
	} else {
		config = high ? EIC_CONFIG_SENSE0_HIGH
			: EIC_CONFIG_SENSE0_LOW;
	}

	if ((flags & GPIO_INT_DEBOUNCE) != 0) {
		config |= EIC_CONFIG_FILTEN0;
	}

	/* The config is 4 bits long and is packed in at 8
	 * pins per 32 bit word.
	 */
	config <<= (EIC_CONFIG_SENSE1_Pos * nibble);
	cmask = ~(0x0F << (EIC_CONFIG_SENSE1_Pos * nibble));

	/* Disable the EIC so it can be updated */
	REGS->CTRL.bit.ENABLE = 0;
	eic_sam0_sync();

	/* Commit the configuration */
	REGS->CONFIG[idx].reg = (REGS->CONFIG[idx].reg & cmask) | config;

	/* And re-enable the unit */
	REGS->CTRL.bit.ENABLE = 1;
	eic_sam0_sync();

	return 0;
}

int gpio_sam0_manage_callback(struct device *dev,
			      struct gpio_callback *callback, bool set)
{
	struct device *eic = DEVICE_GET(eic_sam0_0);
	struct eic_sam0_data *data = eic->driver_data;
	const struct gpio_sam0_config *config = dev->config->config_info;

	_gpio_manage_callback(&data->cbs[config->id], callback, set);

	return 0;
}

int gpio_sam0_enable_callback(struct device *dev, int access_op, u32_t pin)
{
	struct device *eic = DEVICE_GET(eic_sam0_0);
	struct eic_sam0_data *data = eic->driver_data;
	u8_t target = eic_sam0_get_target(dev, pin);
	int extint = pin % EIC_EXTINT_NUM;
	u32_t mask = 1 << extint;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	if (data->targets[extint] != target) {
		/* Something else is using this channel */
		return -EINVAL;
	}

	/* Clear any pending interrupts and unmask */
	REGS->INTFLAG.reg = mask;
	REGS->INTENSET.reg = mask;

	return 0;
}

int gpio_sam0_disable_callback(struct device *dev, int access_op, u32_t pin)
{
	struct device *eic = DEVICE_GET(eic_sam0_0);
	struct eic_sam0_data *data = eic->driver_data;
	u8_t target = eic_sam0_get_target(dev, pin);
	int extint = pin % EIC_EXTINT_NUM;
	u32_t mask = 1 << extint;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	if (data->targets[extint] != target) {
		/* Something else is using this channel */
		return -EINVAL;
	}

	/* Mask the interrupt */
	REGS->INTENCLR.reg = mask;

	return 0;
}

u32_t gpio_sam0_get_pending_int(struct device *dev) { return 0; }

static int eic_sam0_init(struct device *dev)
{
	struct eic_sam0_data *data = dev->driver_data;

#if CONFIG_GPIO_SAM0_PORTA_BASE_ADDRESS
	data->ports[0] = device_get_binding(CONFIG_GPIO_SAM0_PORTA_LABEL);
#endif
#if CONFIG_GPIO_SAM0_PORTB_BASE_ADDRESS
	data->ports[1] = device_get_binding(CONFIG_GPIO_SAM0_PORTB_LABEL);
#endif

	/* Enable the EIC clock in PM */
	PM->APBAMASK.bit.EIC_ = 1;

	/* Enable the GCLK */
	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID_EIC | GCLK_CLKCTRL_GEN_GCLK0 |
			    GCLK_CLKCTRL_CLKEN;

	/* Reset the EIC */
	REGS->CTRL.reg = EIC_CTRL_SWRST;
	eic_sam0_sync();

	/* Route and enable the interrupt.  This is safe as nothing
	 * happens until an individual channel is unmasked.
	 */
	IRQ_CONNECT(CONFIG_EIC_SAM0_IRQ, CONFIG_EIC_SAM0_IRQ_PRIORITY,
		    eic_sam0_isr, DEVICE_GET(eic_sam0_0), 0);
	irq_enable(CONFIG_EIC_SAM0_IRQ);

	return 0;
}

static struct eic_sam0_data eic_sam0_data_0;

DEVICE_AND_API_INIT(eic_sam0_0, CONFIG_EIC_SAM0_LABEL, eic_sam0_init,
		    &eic_sam0_data_0, NULL, POST_KERNEL,
		    /* Initialize before the GPIO device */
		    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &eic_sam0_data_0);
#endif
