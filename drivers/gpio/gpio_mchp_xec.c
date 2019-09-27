/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <device.h>
#include <drivers/gpio.h>
#include <soc.h>

#include "gpio_utils.h"


static const u32_t valid_ctrl_masks[NUM_MCHP_GPIO_PORTS] = {
	(MCHP_GPIO_PORT_A_BITMAP),
	(MCHP_GPIO_PORT_B_BITMAP),
	(MCHP_GPIO_PORT_C_BITMAP),
	(MCHP_GPIO_PORT_D_BITMAP),
	(MCHP_GPIO_PORT_E_BITMAP),
	(MCHP_GPIO_PORT_F_BITMAP)
};

struct gpio_xec_data {
	/* port ISR callback routine address */
	sys_slist_t callbacks;
	/* pin callback routine enable flags, by pin number */
	u32_t pin_callback_enables;
};

struct gpio_xec_config {
	__IO u32_t *pcr1_base;
	u8_t girq_id;
	u32_t port_num;
	u32_t flags;
};

static int gpio_xec_configure(struct device *dev,
			      int access_op, u32_t pin, int flags)
{
	const struct gpio_xec_config *config = dev->config->config_info;
	__IO u32_t *current_pcr1;
	u32_t pcr1 = 0;
	u32_t mask = 0;
	u32_t gpio_interrupt = 0;

	/* Validate pin number range in terms of current port */
	if ((valid_ctrl_masks[config->port_num] & BIT(pin)) == 0) {
		return -EINVAL;
	}

	/* Check for an invalid pin configuration */
	if ((flags & GPIO_INT) && (flags & GPIO_DIR_OUT)) {
		return -EINVAL;
	}

	/* Check if GPIO port supports interrupts */
	if ((flags & GPIO_INT) && ((config->flags & GPIO_INT) == 0)) {
		return -EINVAL;
	}

	/* The flags contain options that require touching registers in the
	 * PCRs for a given GPIO. There are no GPIO modules in Microchip SOCs!
	 *
	 * Start with the GPIO module and set up the pin direction register.
	 * 0 - pin is input, 1 - pin is output
	 */
	mask |= MCHP_GPIO_CTRL_DIR_MASK;
	mask |= MCHP_GPIO_CTRL_INPAD_DIS_MASK;
	if (access_op == GPIO_ACCESS_BY_PIN) {
		if ((flags & GPIO_DIR_MASK) == GPIO_DIR_IN) {
			pcr1 &= ~BIT(MCHP_GPIO_CTRL_DIR_POS);
		} else { /* GPIO_DIR_OUT */
			pcr1 |= BIT(MCHP_GPIO_CTRL_DIR_POS);
		}
	} else { /* GPIO_ACCESS_BY_PORT is not supported */
		return -EINVAL;
	}

	/* Figure out the pullup/pulldown configuration and keep it in the
	 * pcr1 variable
	 */
	mask |= MCHP_GPIO_CTRL_PUD_MASK;

	if ((flags & GPIO_PUD_MASK) == GPIO_PUD_PULL_UP) {
		/* Enable the pull and select the pullup resistor. */
		pcr1 |= MCHP_GPIO_CTRL_PUD_PU;
	} else if ((flags & GPIO_PUD_MASK) == GPIO_PUD_PULL_DOWN) {
		/* Enable the pull and select the pulldown resistor */
		pcr1 |= MCHP_GPIO_CTRL_PUD_PD;
	}

	/* Assemble mask for level/edge triggered interrrupts */
	mask |= MCHP_GPIO_CTRL_IDET_MASK;

	if (flags & GPIO_INT) {
		if (flags & GPIO_INT_EDGE) {
			/* Enable edge interrupts */
			if (flags & GPIO_INT_ACTIVE_HIGH) {
				gpio_interrupt = MCHP_GPIO_CTRL_IDET_REDGE;
			} else if (flags & GPIO_INT_DOUBLE_EDGE) {
				gpio_interrupt = MCHP_GPIO_CTRL_IDET_BEDGE;
			} else {
				gpio_interrupt = MCHP_GPIO_CTRL_IDET_FEDGE;
			}
		} else { /* GPIO_INT_LEVEL */
			if (flags & GPIO_INT_ACTIVE_HIGH) {
				gpio_interrupt = MCHP_GPIO_CTRL_IDET_LVL_HI;
			} else {
				gpio_interrupt = MCHP_GPIO_CTRL_IDET_LVL_LO;
			}
		}
		pcr1 |= gpio_interrupt;
		/* We enable the interrupts in the EC aggregator so that the
		 * result  can be forwarded to the ARM NVIC
		 */
		MCHP_GIRQ_ENSET(config->girq_id) = BIT(pin);
	} else {
		/* Explicitly disable interrupts, otherwise the configuration
		 * results in level triggered/low interrupts
		 */
		pcr1 |= MCHP_GPIO_CTRL_IDET_DISABLE;
	}
	/* Use Gpio output register for writing in order to have simetry with
	 * respect of Gpio input
	 */
	mask |= MCHP_GPIO_CTRL_AOD_MASK;
	pcr1 |= MCHP_GPIO_CTRL_AOD_MASK;
	/* Now write contents of pcr1 variable to the PCR1 register that
	 * corresponds to the GPIO being configured
	 */
	current_pcr1 = config->pcr1_base + pin;
	*current_pcr1 = (*current_pcr1 & ~mask) | pcr1;

	return 0;
}


static int gpio_xec_write(struct device *dev,
			  int access_op, u32_t pin, u32_t value)
{
	const struct gpio_xec_config *config = dev->config->config_info;
	u32_t port_n = config->port_num;

	/* GPIO output registers are used for writing */
	__IO u32_t *gpio_base = (__IO u32_t *)(GPIO_PAROUT_BASE
							+ (port_n << 2));

	if (access_op == GPIO_ACCESS_BY_PIN) {
		if (value) {
			/* Set the data output for the corresponding pin.
			 * Writing zeros to the other bits leaves the data
			 * output unchanged for the other pins
			 */
			*gpio_base |= BIT(pin);
		} else {
			/* Clear the data output for the corresponding pin.
			 * Writing zeros to the other bits leaves the data
			 * output unchanged for the other pins
			 */
			*gpio_base &= ~BIT(pin);
		}
	} else { /* GPIO_ACCESS_BY_PORT not supported */
		return -EINVAL;
	}

	return 0;

}

static int gpio_xec_read(struct device *dev,
			 int access_op, u32_t pin, u32_t *value)
{
	const struct gpio_xec_config *config = dev->config->config_info;
	u32_t port_n = config->port_num;

	/* GPIO input registers are used for reading */
	__IO u32_t *gpio_base = (__IO u32_t *)(GPIO_PARIN_BASE + (port_n << 2));

	*value = *gpio_base;
	if (access_op == GPIO_ACCESS_BY_PIN) {
		*value = (*value & BIT(pin)) >> pin;
	} else { /* GPIO_ACCESS_BY_PORT not supported */
		return -EINVAL;
	}

	return 0;
}


static int gpio_xec_manage_callback(struct device *dev,
				    struct gpio_callback *callback, bool set)
{
	struct gpio_xec_data *data = dev->driver_data;

	gpio_manage_callback(&data->callbacks, callback, set);

	return 0;
}

static int gpio_xec_enable_callback(struct device *dev,
				    int access_op, u32_t pin)
{
	struct gpio_xec_data *data = dev->driver_data;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		data->pin_callback_enables |= BIT(pin);
	} else { /* GPIO_ACCESS_BY_PORT not supported */
		return -EINVAL;
	}

	return 0;
}

static int gpio_xec_disable_callback(struct device *dev,
				     int access_op, u32_t pin)
{
	struct gpio_xec_data *data = dev->driver_data;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		data->pin_callback_enables &= ~BIT(pin);
	} else { /* GPIO_ACCESS_BY_PORT not supported */
		return -EINVAL;
	}
	return 0;
}

static void gpio_gpio_xec_port_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct gpio_xec_config *config = dev->config->config_info;
	struct gpio_xec_data *data = dev->driver_data;
	u32_t girq_result;
	u32_t enabled_int;

	/* Figure out which interrupts have been triggered from the EC
	 * aggregator result register
	 */
	girq_result = MCHP_GIRQ_RESULT(config->girq_id);
	enabled_int = girq_result  & data->pin_callback_enables;

	/* Clear source register in aggregator before firing callbacks */
	REG32(MCHP_GIRQ_SRC_ADDR(config->girq_id)) = girq_result;

	gpio_fire_callbacks(&data->callbacks, dev, enabled_int);
}

static const struct gpio_driver_api gpio_xec_driver_api = {
	.config = gpio_xec_configure,
	.write = gpio_xec_write,
	.read = gpio_xec_read,
	.manage_callback = gpio_xec_manage_callback,
	.enable_callback = gpio_xec_enable_callback,
	.disable_callback = gpio_xec_disable_callback,
};

#ifdef CONFIG_GPIO_XEC_GPIO000_036
static int gpio_xec_port000_036_init(struct device *dev);

static const struct gpio_xec_config gpio_xec_port000_036_config = {
	.pcr1_base = (u32_t *)  DT_GPIO_XEC_GPIO000_036_BASE_ADDR,
	.port_num = MCHP_GPIO_000_036,
#ifdef DT_GPIO_XEC_GPIO000_036_IRQ
	.girq_id = MCHP_GIRQ11_ID,
	.flags = GPIO_INT,
#else
	.flags = 0,
#endif
};

static struct gpio_xec_data gpio_xec_port000_036_data;

DEVICE_AND_API_INIT(gpio_xec_port000_036, DT_GPIO_XEC_GPIO000_036_LABEL,
		gpio_xec_port000_036_init,
		&gpio_xec_port000_036_data, &gpio_xec_port000_036_config,
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&gpio_xec_driver_api);

static int gpio_xec_port000_036_init(struct device *dev)
{
#ifdef DT_GPIO_XEC_GPIO000_036_IRQ
	const struct gpio_xec_config *config = dev->config->config_info;

	/* Turn on the block enable in the EC aggregator */
	MCHP_GIRQ_BLK_SETEN(config->girq_id);

	IRQ_CONNECT(DT_GPIO_XEC_GPIO000_036_IRQ,
		DT_GPIO_XEC_GPIO000_036_IRQ_PRIORITY,
		gpio_gpio_xec_port_isr, DEVICE_GET(gpio_xec_port000_036), 0);

	irq_enable(DT_GPIO_XEC_GPIO000_036_IRQ);
#endif
	return 0;
}
#endif /* CONFIG_GPIO_XEC_GPIO000_036 */

#ifdef CONFIG_GPIO_XEC_GPIO040_076
static int gpio_xec_port040_076_init(struct device *dev);

static const struct gpio_xec_config gpio_xec_port040_076_config = {
	.pcr1_base = (u32_t *)  DT_GPIO_XEC_GPIO040_076_BASE_ADDR,
	.port_num = MCHP_GPIO_040_076,
#ifdef DT_GPIO_XEC_GPIO040_076_IRQ
	.girq_id = MCHP_GIRQ10_ID,
	.flags = GPIO_INT,
#else
	.flags = 0,
#endif
};

static struct gpio_xec_data gpio_xec_port040_076_data;

DEVICE_AND_API_INIT(gpio_xec_port040_076, DT_GPIO_XEC_GPIO040_076_LABEL,
		gpio_xec_port040_076_init,
		&gpio_xec_port040_076_data, &gpio_xec_port040_076_config,
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&gpio_xec_driver_api);

static int gpio_xec_port040_076_init(struct device *dev)
{
#ifdef DT_GPIO_XEC_GPIO040_076_IRQ
	const struct gpio_xec_config *config = dev->config->config_info;

	/* Turn on the block enable in the EC aggregator */
	MCHP_GIRQ_BLK_SETEN(config->girq_id);

	IRQ_CONNECT(DT_GPIO_XEC_GPIO040_076_IRQ,
		DT_GPIO_XEC_GPIO040_076_IRQ_PRIORITY,
		gpio_gpio_xec_port_isr, DEVICE_GET(gpio_xec_port040_076), 0);

	irq_enable(DT_GPIO_XEC_GPIO040_076_IRQ);
#endif
	return 0;
}
#endif /* CONFIG_GPIO_XEC_GPIO040_076 */

#ifdef CONFIG_GPIO_XEC_GPIO100_136
static int gpio_xec_port100_136_init(struct device *dev);

static const struct gpio_xec_config gpio_xec_port100_136_config = {
	.pcr1_base = (u32_t *)  DT_GPIO_XEC_GPIO100_136_BASE_ADDR,
	.port_num = MCHP_GPIO_100_136,
#ifdef DT_GPIO_XEC_GPIO100_136_IRQ
	.girq_id = MCHP_GIRQ09_ID,
	.flags = GPIO_INT,
#else
	.flags = 0,
#endif
};

static struct gpio_xec_data gpio_xec_port100_136_data;

DEVICE_AND_API_INIT(gpio_xec_port100_136, DT_GPIO_XEC_GPIO100_136_LABEL,
		gpio_xec_port100_136_init,
		&gpio_xec_port100_136_data, &gpio_xec_port100_136_config,
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&gpio_xec_driver_api);

static int gpio_xec_port100_136_init(struct device *dev)
{
#ifdef DT_GPIO_XEC_GPIO100_136_IRQ
	const struct gpio_xec_config *config = dev->config->config_info;

	/* Turn on the block enable in the EC aggregator */
	MCHP_GIRQ_BLK_SETEN(config->girq_id);

	IRQ_CONNECT(DT_GPIO_XEC_GPIO100_136_IRQ,
		DT_GPIO_XEC_GPIO100_136_IRQ_PRIORITY,
		gpio_gpio_xec_port_isr, DEVICE_GET(gpio_xec_port100_136), 0);

	irq_enable(DT_GPIO_XEC_GPIO100_136_IRQ);
#endif
	return 0;
}
#endif /* CONFIG_GPIO_XEC_GPIO100_136 */

#ifdef CONFIG_GPIO_XEC_GPIO140_176
static int gpio_xec_port140_176_init(struct device *dev);

static const struct gpio_xec_config gpio_xec_port140_176_config = {
	.pcr1_base = (u32_t *)  DT_GPIO_XEC_GPIO140_176_BASE_ADDR,
	.port_num = MCHP_GPIO_140_176,
#ifdef DT_GPIO_XEC_GPIO140_176_IRQ
	.girq_id = MCHP_GIRQ08_ID,
	.flags = GPIO_INT,
#else
	.flags = 0,
#endif
};

static struct gpio_xec_data gpio_xec_port140_176_data;

DEVICE_AND_API_INIT(gpio_xec_port140_176, DT_GPIO_XEC_GPIO140_176_LABEL,
		gpio_xec_port140_176_init,
		&gpio_xec_port140_176_data, &gpio_xec_port140_176_config,
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&gpio_xec_driver_api);

static int gpio_xec_port140_176_init(struct device *dev)
{
#ifdef DT_GPIO_XEC_GPIO140_176_IRQ
	const struct gpio_xec_config *config = dev->config->config_info;

	/* Turn on the block enable in the EC aggregator */
	MCHP_GIRQ_BLK_SETEN(config->girq_id);

	IRQ_CONNECT(DT_GPIO_XEC_GPIO140_176_IRQ,
		DT_GPIO_XEC_GPIO140_176_IRQ_PRIORITY,
		gpio_gpio_xec_port_isr, DEVICE_GET(gpio_xec_port140_176), 0);

	irq_enable(DT_GPIO_XEC_GPIO140_176_IRQ);
#endif
	return 0;
}
#endif /* CONFIG_GPIO_XEC_GPIO140_176 */

#ifdef CONFIG_GPIO_XEC_GPIO200_236
static int gpio_xec_port200_236_init(struct device *dev);

static const struct gpio_xec_config gpio_xec_port200_236_config = {
	.pcr1_base = (u32_t *) DT_GPIO_XEC_GPIO200_236_BASE_ADDR,
	.port_num = MCHP_GPIO_200_236,
#ifdef DT_GPIO_XEC_GPIO200_236_IRQ
	.girq_id = MCHP_GIRQ12_ID,
	.flags = GPIO_INT,
#else
	.flags = 0,
#endif
};

static struct gpio_xec_data gpio_xec_port200_236_data;

DEVICE_AND_API_INIT(gpio_xec_port200_236, DT_GPIO_XEC_GPIO200_236_LABEL,
		gpio_xec_port200_236_init,
		&gpio_xec_port200_236_data, &gpio_xec_port200_236_config,
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&gpio_xec_driver_api);

static int gpio_xec_port200_236_init(struct device *dev)
{
#ifdef DT_GPIO_XEC_GPIO200_236_IRQ
	const struct gpio_xec_config *config = dev->config->config_info;

	/* Turn on the block enable in the EC aggregator */
	MCHP_GIRQ_BLK_SETEN(config->girq_id);

	IRQ_CONNECT(DT_GPIO_XEC_GPIO200_236_IRQ,
		DT_GPIO_XEC_GPIO200_236_IRQ_PRIORITY,
		gpio_gpio_xec_port_isr, DEVICE_GET(gpio_xec_port200_236), 0);

	irq_enable(DT_GPIO_XEC_GPIO200_236_IRQ);
#endif
	return 0;
}
#endif /* CONFIG_GPIO_XEC_GPIO200_236 */

#ifdef CONFIG_GPIO_XEC_GPIO240_276
static int gpio_xec_port240_276_init(struct device *dev);

static const struct gpio_xec_config gpio_xec_port240_276_config = {
	.pcr1_base = (u32_t *)  DT_GPIO_XEC_GPIO240_276_BASE_ADDR,
	.port_num = MCHP_GPIO_240_276,
#ifdef DT_GPIO_XEC_GPIO240_276_IRQ
	.girq_id = MCHP_GIRQ26_ID,
	.flags = GPIO_INT,
#else
	.flags = 0,
#endif
};

static struct gpio_xec_data gpio_xec_port240_276_data;

DEVICE_AND_API_INIT(gpio_xec_port240_276, DT_GPIO_XEC_GPIO240_276_LABEL,
		gpio_xec_port240_276_init,
		&gpio_xec_port240_276_data, &gpio_xec_port240_276_config,
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&gpio_xec_driver_api);

static int gpio_xec_port240_276_init(struct device *dev)
{
#ifdef DT_GPIO_XEC_GPIO240_276_IRQ
	const struct gpio_xec_config *config = dev->config->config_info;

	/* Turn on the block enable in the EC aggregator */
	MCHP_GIRQ_BLK_SETEN(config->girq_id);

	IRQ_CONNECT(DT_GPIO_XEC_GPIO240_276_IRQ,
		DT_GPIO_XEC_GPIO240_276_IRQ_PRIORITY,
		gpio_gpio_xec_port_isr, DEVICE_GET(gpio_xec_port240_276), 0);

	irq_enable(DT_GPIO_XEC_GPIO240_276_IRQ);
#endif
	return 0;
}
#endif /* CONFIG_GPIO_XEC_GPIO240_276 */
