/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_gpio

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/sys_io.h>
#include <soc.h>

#include <zephyr/drivers/gpio/gpio_utils.h>

#define XEC_GPIO_EDGE_DLY_COUNT		4

#define GPIO_IN_BASE(config) \
	((__IO uint32_t *)(GPIO_PARIN_BASE + (config->port_num << 2)))

#define GPIO_OUT_BASE(config) \
	((__IO uint32_t *)(GPIO_PAROUT_BASE + (config->port_num << 2)))

static const uint32_t valid_ctrl_masks[NUM_MCHP_GPIO_PORTS] = {
	(MCHP_GPIO_PORT_A_BITMAP),
	(MCHP_GPIO_PORT_B_BITMAP),
	(MCHP_GPIO_PORT_C_BITMAP),
	(MCHP_GPIO_PORT_D_BITMAP),
	(MCHP_GPIO_PORT_E_BITMAP),
	(MCHP_GPIO_PORT_F_BITMAP)
};

struct gpio_xec_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* port ISR callback routine address */
	sys_slist_t callbacks;
};

struct gpio_xec_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	__IO uint32_t *pcr1_base;
	uint8_t girq_id;
	uint32_t port_num;
	uint32_t flags;
};

/*
 * notes: The GPIO parallel output bits are read-only until the
 * Alternate-Output-Disable (AOD) bit is set in the pin's control
 * register. To preload a parallel output value to prevent certain
 * classes of glitching for output pins we must:
 * Set GPIO control AOD=1 with the pin direction set to input.
 * Program the new pin value in the respective GPIO parallel output
 * register.
 * Program other GPIO control bits except direction.
 * Last step set the GPIO control register direction bit to output.
 */
static int gpio_xec_configure(const struct device *dev,
			      gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_xec_config *config = dev->config;
	__IO uint32_t *current_pcr1;
	uint32_t pcr1 = 0U;
	uint32_t mask = 0U;

	/* Validate pin number range in terms of current port */
	if ((valid_ctrl_masks[config->port_num] & BIT(pin)) == 0U) {
		return -EINVAL;
	}

	/* Don't support "open source" mode */
	if (((flags & GPIO_SINGLE_ENDED) != 0U) &&
	    ((flags & GPIO_LINE_OPEN_DRAIN) == 0U)) {
		return -ENOTSUP;
	}

	/* The flags contain options that require touching registers in the
	 * PCRs for a given GPIO. There are no GPIO modules in Microchip SOCs!
	 * Keep direction as input until last.
	 * Clear input pad disable allowing input pad to operate.
	 * Clear Power gate to allow pads to operate.
	 */
	mask |= MCHP_GPIO_CTRL_DIR_MASK;
	mask |= MCHP_GPIO_CTRL_INPAD_DIS_MASK;
	mask |= MCHP_GPIO_CTRL_PWRG_MASK;
	mask |= MCHP_GPIO_CTRL_AOD_MASK;

	current_pcr1 = config->pcr1_base + pin;

	if (flags == GPIO_DISCONNECTED) {
		pcr1 |= MCHP_GPIO_CTRL_PWRG_OFF;
		*current_pcr1 = (*current_pcr1 & ~mask) | pcr1;
		return 0;
	}

	pcr1 = MCHP_GPIO_CTRL_PWRG_VTR_IO;

	/* Always enable input pad */
	if (*current_pcr1 & BIT(MCHP_GPIO_CTRL_INPAD_DIS_POS)) {
		*current_pcr1 &= ~BIT(MCHP_GPIO_CTRL_INPAD_DIS_POS);
	}

	/* Figure out the pullup/pulldown configuration and keep it in the
	 * pcr1 variable
	 */
	mask |= MCHP_GPIO_CTRL_PUD_MASK;

	if ((flags & GPIO_PULL_UP) != 0U) {
		/* Enable the pull and select the pullup resistor. */
		pcr1 |= MCHP_GPIO_CTRL_PUD_PU;
	} else if ((flags & GPIO_PULL_DOWN) != 0U) {
		/* Enable the pull and select the pulldown resistor */
		pcr1 |= MCHP_GPIO_CTRL_PUD_PD;
	}

	/* Push-pull or open drain */
	mask |= MCHP_GPIO_CTRL_BUFT_MASK;

	if ((flags & GPIO_OPEN_DRAIN) != 0U) {
		/* Open drain */
		pcr1 |= MCHP_GPIO_CTRL_BUFT_OPENDRAIN;
	} else {
		/* Push-pull */
		pcr1 |= MCHP_GPIO_CTRL_BUFT_PUSHPULL;
	}

	if ((flags & GPIO_OUTPUT) != 0U) {
		mask |= MCHP_GPIO_CTRL_OUTV_HI;
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
			pcr1 |= BIT(MCHP_GPIO_CTRL_OUTVAL_POS);
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
			pcr1 &= ~BIT(MCHP_GPIO_CTRL_OUTVAL_POS);
		} else { /* Copy current input state to output state */
			if ((*current_pcr1 & MCHP_GPIO_CTRL_PWRG_MASK) ==
			     MCHP_GPIO_CTRL_PWRG_OFF) {
				*current_pcr1 = (*current_pcr1 &
						 ~MCHP_GPIO_CTRL_PWRG_MASK) |
						 MCHP_GPIO_CTRL_PWRG_VTR_IO;
			}
			if (*current_pcr1 & BIT(MCHP_GPIO_CTRL_INPAD_VAL_POS)) {
				pcr1 |= BIT(MCHP_GPIO_CTRL_OUTVAL_POS);
			} else {
				pcr1 &= ~BIT(MCHP_GPIO_CTRL_OUTVAL_POS);
			}
		}

		pcr1 |= MCHP_GPIO_CTRL_DIR_OUTPUT;
	}

	*current_pcr1 = (*current_pcr1 & ~mask) | pcr1;
	/* Control output bit becomes ready only and parallel output r/w */
	*current_pcr1 = *current_pcr1 | BIT(MCHP_GPIO_CTRL_AOD_POS);

	return 0;
}

static int gpio_xec_pin_interrupt_configure(const struct device *dev,
					    gpio_pin_t pin,
					    enum gpio_int_mode mode,
					    enum gpio_int_trig trig)
{
	const struct gpio_xec_config *config = dev->config;
	__IO uint32_t *current_pcr1;
	uint32_t pcr1 = 0U;
	uint32_t mask = 0U;
	uint32_t gpio_interrupt = 0U;

	/* Validate pin number range in terms of current port */
	if ((valid_ctrl_masks[config->port_num] & BIT(pin)) == 0U) {
		return -EINVAL;
	}

	/* Check if GPIO port supports interrupts */
	if ((mode != GPIO_INT_MODE_DISABLED) &&
	    ((config->flags & GPIO_INT_ENABLE) == 0U)) {
		return -ENOTSUP;
	}

	/* Disable interrupt in the EC aggregator */
	MCHP_GIRQ_ENCLR(config->girq_id) = BIT(pin);

	/* Assemble mask for level/edge triggered interrupts */
	mask |= MCHP_GPIO_CTRL_IDET_MASK;

	if (mode == GPIO_INT_MODE_DISABLED) {
		/* Explicitly disable interrupts, otherwise the configuration
		 * results in level triggered/low interrupts
		 */
		pcr1 |= MCHP_GPIO_CTRL_IDET_DISABLE;
	} else {
		if (mode == GPIO_INT_MODE_LEVEL) {
			/* Enable level interrupts */
			if (trig == GPIO_INT_TRIG_HIGH) {
				gpio_interrupt = MCHP_GPIO_CTRL_IDET_LVL_HI;
			} else {
				gpio_interrupt = MCHP_GPIO_CTRL_IDET_LVL_LO;
			}
		} else {
			/* Enable edge interrupts */
			switch (trig) {
			case GPIO_INT_TRIG_LOW:
				gpio_interrupt = MCHP_GPIO_CTRL_IDET_FEDGE;
				break;
			case GPIO_INT_TRIG_HIGH:
				gpio_interrupt = MCHP_GPIO_CTRL_IDET_REDGE;
				break;
			case GPIO_INT_TRIG_BOTH:
				gpio_interrupt = MCHP_GPIO_CTRL_IDET_BEDGE;
				break;
			default:
				return -EINVAL;
			}
		}

		pcr1 |= gpio_interrupt;
	}

	/* Now write contents of pcr1 variable to the PCR1 register that
	 * corresponds to the GPIO being configured
	 */
	current_pcr1 = config->pcr1_base + pin;
	*current_pcr1 = (*current_pcr1 & ~mask) | pcr1;
	/* delay for HW to synchronize after it ungates its clock */
	for (int i = 0; i < XEC_GPIO_EDGE_DLY_COUNT; i++) {
		(void)*current_pcr1;
	}

	if (mode != GPIO_INT_MODE_DISABLED) {
		/* We enable the interrupts in the EC aggregator so that the
		 * result can be forwarded to the ARM NVIC
		 */
		MCHP_GIRQ_SRC_CLR(config->girq_id, pin);
		MCHP_GIRQ_ENSET(config->girq_id) = BIT(pin);
	}

	return 0;
}

static int gpio_xec_port_set_masked_raw(const struct device *dev,
					uint32_t mask,
					uint32_t value)
{
	const struct gpio_xec_config *config = dev->config;

	/* GPIO output registers are used for writing */
	__IO uint32_t *gpio_base = GPIO_OUT_BASE(config);

	*gpio_base = (*gpio_base & ~mask) | (mask & value);

	return 0;
}

static int gpio_xec_port_set_bits_raw(const struct device *dev, uint32_t mask)
{
	const struct gpio_xec_config *config = dev->config;

	/* GPIO output registers are used for writing */
	__IO uint32_t *gpio_base = GPIO_OUT_BASE(config);

	*gpio_base |= mask;

	return 0;
}

static int gpio_xec_port_clear_bits_raw(const struct device *dev,
					uint32_t mask)
{
	const struct gpio_xec_config *config = dev->config;

	/* GPIO output registers are used for writing */
	__IO uint32_t *gpio_base = GPIO_OUT_BASE(config);

	*gpio_base &= ~mask;

	return 0;
}

static int gpio_xec_port_toggle_bits(const struct device *dev, uint32_t mask)
{
	const struct gpio_xec_config *config = dev->config;

	/* GPIO output registers are used for writing */
	__IO uint32_t *gpio_base = GPIO_OUT_BASE(config);

	*gpio_base ^= mask;

	return 0;
}

static int gpio_xec_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_xec_config *config = dev->config;

	/* GPIO input registers are used for reading */
	__IO uint32_t *gpio_base = GPIO_IN_BASE(config);

	*value = *gpio_base;

	return 0;
}

static int gpio_xec_manage_callback(const struct device *dev,
				    struct gpio_callback *callback, bool set)
{
	struct gpio_xec_data *data = dev->data;

	gpio_manage_callback(&data->callbacks, callback, set);

	return 0;
}

static void gpio_gpio_xec_port_isr(const struct device *dev)
{
	const struct gpio_xec_config *config = dev->config;
	struct gpio_xec_data *data = dev->data;
	uint32_t girq_result;

	/* Figure out which interrupts have been triggered from the EC
	 * aggregator result register
	 */
	girq_result = MCHP_GIRQ_RESULT(config->girq_id);

	/* Clear source register in aggregator before firing callbacks */
	REG32(MCHP_GIRQ_SRC_ADDR(config->girq_id)) = girq_result;

	gpio_fire_callbacks(&data->callbacks, dev, girq_result);
}

static const struct gpio_driver_api gpio_xec_driver_api = {
	.pin_configure = gpio_xec_configure,
	.port_get_raw = gpio_xec_port_get_raw,
	.port_set_masked_raw = gpio_xec_port_set_masked_raw,
	.port_set_bits_raw = gpio_xec_port_set_bits_raw,
	.port_clear_bits_raw = gpio_xec_port_clear_bits_raw,
	.port_toggle_bits = gpio_xec_port_toggle_bits,
	.pin_interrupt_configure = gpio_xec_pin_interrupt_configure,
	.manage_callback = gpio_xec_manage_callback,
};

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpio_000_036))
static int gpio_xec_port000_036_init(const struct device *dev);

static const struct gpio_xec_config gpio_xec_port000_036_config = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_NODE(
			DT_NODELABEL(gpio_000_036)),
	},
	.pcr1_base = (uint32_t *) DT_REG_ADDR(DT_NODELABEL(gpio_000_036)),
	.port_num = MCHP_GPIO_000_036,
#if DT_IRQ_HAS_CELL(DT_NODELABEL(gpio_000_036), irq)
	.girq_id = MCHP_GIRQ11_ID,
	.flags = GPIO_INT_ENABLE,
#else
	.flags = 0,
#endif
};

static struct gpio_xec_data gpio_xec_port000_036_data;

DEVICE_DT_DEFINE(DT_NODELABEL(gpio_000_036),
		    gpio_xec_port000_036_init,
		    NULL,
		    &gpio_xec_port000_036_data, &gpio_xec_port000_036_config,
		    PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,
		    &gpio_xec_driver_api);

static int gpio_xec_port000_036_init(const struct device *dev)
{
#if DT_IRQ_HAS_CELL(DT_NODELABEL(gpio_000_036), irq)
	const struct gpio_xec_config *config = dev->config;

	/* Turn on the block enable in the EC aggregator */
	MCHP_GIRQ_BLK_SETEN(config->girq_id);

	IRQ_CONNECT(DT_IRQ(DT_NODELABEL(gpio_000_036), irq),
		    DT_IRQ(DT_NODELABEL(gpio_000_036), priority),
		    gpio_gpio_xec_port_isr,
		    DEVICE_DT_GET(DT_NODELABEL(gpio_000_036)), 0U);

	irq_enable(DT_IRQ(DT_NODELABEL(gpio_000_036), irq));
#endif
	return 0;
}
#endif /* DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpio_000_036)) */

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpio_040_076))
static int gpio_xec_port040_076_init(const struct device *dev);

static const struct gpio_xec_config gpio_xec_port040_076_config = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_NODE(
			DT_NODELABEL(gpio_040_076)),
	},
	.pcr1_base = (uint32_t *) DT_REG_ADDR(DT_NODELABEL(gpio_040_076)),
	.port_num = MCHP_GPIO_040_076,
#if DT_IRQ_HAS_CELL(DT_NODELABEL(gpio_040_076), irq)
	.girq_id = MCHP_GIRQ10_ID,
	.flags = GPIO_INT_ENABLE,
#else
	.flags = 0,
#endif
};

static struct gpio_xec_data gpio_xec_port040_076_data;

DEVICE_DT_DEFINE(DT_NODELABEL(gpio_040_076),
		    gpio_xec_port040_076_init,
		    NULL,
		    &gpio_xec_port040_076_data, &gpio_xec_port040_076_config,
		    PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,
		    &gpio_xec_driver_api);

static int gpio_xec_port040_076_init(const struct device *dev)
{
#if DT_IRQ_HAS_CELL(DT_NODELABEL(gpio_040_076), irq)
	const struct gpio_xec_config *config = dev->config;

	/* Turn on the block enable in the EC aggregator */
	MCHP_GIRQ_BLK_SETEN(config->girq_id);

	IRQ_CONNECT(DT_IRQ(DT_NODELABEL(gpio_040_076), irq),
		    DT_IRQ(DT_NODELABEL(gpio_040_076), priority),
		    gpio_gpio_xec_port_isr,
		    DEVICE_DT_GET(DT_NODELABEL(gpio_040_076)), 0U);

	irq_enable(DT_IRQ(DT_NODELABEL(gpio_040_076), irq));
#endif
	return 0;
}
#endif /* DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpio_040_076)) */

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpio_100_136))
static int gpio_xec_port100_136_init(const struct device *dev);

static const struct gpio_xec_config gpio_xec_port100_136_config = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_NODE(
			DT_NODELABEL(gpio_100_136)),
	},
	.pcr1_base = (uint32_t *) DT_REG_ADDR(DT_NODELABEL(gpio_100_136)),
	.port_num = MCHP_GPIO_100_136,
#if DT_IRQ_HAS_CELL(DT_NODELABEL(gpio_100_136), irq)
	.girq_id = MCHP_GIRQ09_ID,
	.flags = GPIO_INT_ENABLE,
#else
	.flags = 0,
#endif
};

static struct gpio_xec_data gpio_xec_port100_136_data;

DEVICE_DT_DEFINE(DT_NODELABEL(gpio_100_136),
		    gpio_xec_port100_136_init,
		    NULL,
		    &gpio_xec_port100_136_data, &gpio_xec_port100_136_config,
		    PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,
		    &gpio_xec_driver_api);

static int gpio_xec_port100_136_init(const struct device *dev)
{
#if DT_IRQ_HAS_CELL(DT_NODELABEL(gpio_100_136), irq)
	const struct gpio_xec_config *config = dev->config;

	/* Turn on the block enable in the EC aggregator */
	MCHP_GIRQ_BLK_SETEN(config->girq_id);

	IRQ_CONNECT(DT_IRQ(DT_NODELABEL(gpio_100_136), irq),
		    DT_IRQ(DT_NODELABEL(gpio_100_136), priority),
		    gpio_gpio_xec_port_isr,
		    DEVICE_DT_GET(DT_NODELABEL(gpio_100_136)), 0U);

	irq_enable(DT_IRQ(DT_NODELABEL(gpio_100_136), irq));
#endif
	return 0;
}
#endif /* DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpio_100_136)) */

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpio_140_176))
static int gpio_xec_port140_176_init(const struct device *dev);

static const struct gpio_xec_config gpio_xec_port140_176_config = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_NODE(
			DT_NODELABEL(gpio_140_176)),
	},
	.pcr1_base = (uint32_t *) DT_REG_ADDR(DT_NODELABEL(gpio_140_176)),
	.port_num = MCHP_GPIO_140_176,
#if DT_IRQ_HAS_CELL(DT_NODELABEL(gpio_140_176), irq)
	.girq_id = MCHP_GIRQ08_ID,
	.flags = GPIO_INT_ENABLE,
#else
	.flags = 0,
#endif
};

static struct gpio_xec_data gpio_xec_port140_176_data;

DEVICE_DT_DEFINE(DT_NODELABEL(gpio_140_176),
		    gpio_xec_port140_176_init,
		    NULL,
		    &gpio_xec_port140_176_data, &gpio_xec_port140_176_config,
		    PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,
		    &gpio_xec_driver_api);

static int gpio_xec_port140_176_init(const struct device *dev)
{
#if DT_IRQ_HAS_CELL(DT_NODELABEL(gpio_140_176), irq)
	const struct gpio_xec_config *config = dev->config;

	/* Turn on the block enable in the EC aggregator */
	MCHP_GIRQ_BLK_SETEN(config->girq_id);

	IRQ_CONNECT(DT_IRQ(DT_NODELABEL(gpio_140_176), irq),
		    DT_IRQ(DT_NODELABEL(gpio_140_176), priority),
		    gpio_gpio_xec_port_isr,
		    DEVICE_DT_GET(DT_NODELABEL(gpio_140_176)), 0U);

	irq_enable(DT_IRQ(DT_NODELABEL(gpio_140_176), irq));
#endif
	return 0;
}
#endif /* DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpio_140_176)) */

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpio_200_236))
static int gpio_xec_port200_236_init(const struct device *dev);

static const struct gpio_xec_config gpio_xec_port200_236_config = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_NODE(
			DT_NODELABEL(gpio_200_236)),
	},
	.pcr1_base = (uint32_t *) DT_REG_ADDR(DT_NODELABEL(gpio_200_236)),
	.port_num = MCHP_GPIO_200_236,
#if DT_IRQ_HAS_CELL(DT_NODELABEL(gpio_200_236), irq)
	.girq_id = MCHP_GIRQ12_ID,
	.flags = GPIO_INT_ENABLE,
#else
	.flags = 0,
#endif
};

static struct gpio_xec_data gpio_xec_port200_236_data;

DEVICE_DT_DEFINE(DT_NODELABEL(gpio_200_236),
		    gpio_xec_port200_236_init,
		    NULL,
		    &gpio_xec_port200_236_data, &gpio_xec_port200_236_config,
		    PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,
		    &gpio_xec_driver_api);

static int gpio_xec_port200_236_init(const struct device *dev)
{
#if DT_IRQ_HAS_CELL(DT_NODELABEL(gpio_200_236), irq)
	const struct gpio_xec_config *config = dev->config;

	/* Turn on the block enable in the EC aggregator */
	MCHP_GIRQ_BLK_SETEN(config->girq_id);

	IRQ_CONNECT(DT_IRQ(DT_NODELABEL(gpio_200_236), irq),
		    DT_IRQ(DT_NODELABEL(gpio_200_236), priority),
		    gpio_gpio_xec_port_isr,
		    DEVICE_DT_GET(DT_NODELABEL(gpio_200_236)), 0U);

	irq_enable(DT_IRQ(DT_NODELABEL(gpio_200_236), irq));
#endif
	return 0;
}
#endif /* DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpio_200_236)) */

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpio_240_276))
static int gpio_xec_port240_276_init(const struct device *dev);

static const struct gpio_xec_config gpio_xec_port240_276_config = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_NODE(
			DT_NODELABEL(gpio_240_276)),
	},
	.pcr1_base = (uint32_t *) DT_REG_ADDR(DT_NODELABEL(gpio_240_276)),
	.port_num = MCHP_GPIO_240_276,
#if DT_IRQ_HAS_CELL(DT_NODELABEL(gpio_240_276), irq)
	.girq_id = MCHP_GIRQ26_ID,
	.flags = GPIO_INT_ENABLE,
#else
	.flags = 0,
#endif
};

static struct gpio_xec_data gpio_xec_port240_276_data;

DEVICE_DT_DEFINE(DT_NODELABEL(gpio_240_276),
		    gpio_xec_port240_276_init,
		    NULL,
		    &gpio_xec_port240_276_data, &gpio_xec_port240_276_config,
		    PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,
		    &gpio_xec_driver_api);

static int gpio_xec_port240_276_init(const struct device *dev)
{
#if DT_IRQ_HAS_CELL(DT_NODELABEL(gpio_240_276), irq)
	const struct gpio_xec_config *config = dev->config;

	/* Turn on the block enable in the EC aggregator */
	MCHP_GIRQ_BLK_SETEN(config->girq_id);

	IRQ_CONNECT(DT_IRQ(DT_NODELABEL(gpio_240_276), irq),
		    DT_IRQ(DT_NODELABEL(gpio_240_276), priority),
		    gpio_gpio_xec_port_isr,
		    DEVICE_DT_GET(DT_NODELABEL(gpio_240_276)), 0U);

	irq_enable(DT_IRQ(DT_NODELABEL(gpio_240_276), irq));
#endif
	return 0;
}
#endif /* DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpio_240_276)) */
