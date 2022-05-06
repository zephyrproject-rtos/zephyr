/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_gpio_v2

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/pinctrl/mchp-xec-pinctrl.h>
#include <soc.h>
#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>

#include "gpio_utils.h"

#define XEC_GPIO_EDGE_DLY_COUNT		4

static const uint32_t valid_ctrl_masks[NUM_MCHP_GPIO_PORTS] = {
	(MCHP_GPIO_PORT_A_BITMAP),
	(MCHP_GPIO_PORT_B_BITMAP),
	(MCHP_GPIO_PORT_C_BITMAP),
	(MCHP_GPIO_PORT_D_BITMAP),
	(MCHP_GPIO_PORT_E_BITMAP),
	(MCHP_GPIO_PORT_F_BITMAP),
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
	uintptr_t pcr1_base;
	uintptr_t parin_addr;
	uintptr_t parout_addr;
	uint8_t girq_id;
	uint8_t port_num;
	uint32_t flags;
};

/* Each GPIO pin 32-bit control register located consecutively in memory */
static inline uintptr_t pin_ctrl_addr(const struct device *dev, gpio_pin_t pin)
{
	const struct gpio_xec_config *config = dev->config;

	return config->pcr1_base + ((uintptr_t)pin * 4u);
}

/* GPIO Parallel input is a single 32-bit register per bank of 32 pins */
static inline uintptr_t pin_parin_addr(const struct device *dev)
{
	const struct gpio_xec_config *config = dev->config;

	return config->parin_addr;
}

/* GPIO Parallel output is a single 32-bit register per bank of 32 pins */
static inline uintptr_t pin_parout_addr(const struct device *dev)
{
	const struct gpio_xec_config *config = dev->config;

	return config->parout_addr;
}

/*
 * Use Zephyr system API to implement
 * reg32(addr) = (reg32(addr) & ~mask) | (val & mask)
 */
static inline void xec_mask_write32(uintptr_t addr, uint32_t mask, uint32_t val)
{
	uint32_t r = (sys_read32(addr) & ~mask) | (val & mask);

	sys_write32(r, addr);
}

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
	uintptr_t pcr1_addr = pin_ctrl_addr(dev, pin);
	uintptr_t pout_addr = pin_parout_addr(dev);
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
	pcr1 |= MCHP_GPIO_CTRL_DIR_INPUT;

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

	/* Use GPIO output register to control pin output, instead of
	 * using the control register (=> alternate output disable).
	 */
	mask |= MCHP_GPIO_CTRL_AOD_MASK;
	pcr1 |= MCHP_GPIO_CTRL_AOD_DIS;

	/* Make sure disconnected on first control register write */
	if (flags == GPIO_DISCONNECTED) {
		pcr1 |= MCHP_GPIO_CTRL_PWRG_OFF;
	}

	/* Now write contents of pcr1 variable to the PCR1 register that
	 * corresponds to the GPIO being configured.
	 * AOD is 1 and direction is input. HW will allow use to set the
	 * GPIO parallel output bit for this pin and with the pin direction
	 * as input no glitch will occur.
	 */
	xec_mask_write32(pcr1_addr, mask, pcr1);

	if ((flags & GPIO_OUTPUT) != 0U) {
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
			sys_set_bit(pout_addr, pin);
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
			sys_clear_bit(pout_addr, pin);
		}

		mask = MCHP_GPIO_CTRL_DIR_MASK;
		pcr1 = MCHP_GPIO_CTRL_DIR_OUTPUT;
		xec_mask_write32(pcr1_addr, mask, pcr1);
	}

	return 0;
}

static int gen_gpio_ctrl_icfg(enum gpio_int_mode mode, enum gpio_int_trig trig,
			      uint32_t *pin_ctr1)
{
	if (!pin_ctr1) {
		return -EINVAL;
	}

	if (mode == GPIO_INT_MODE_DISABLED) {
		*pin_ctr1 = MCHP_GPIO_CTRL_IDET_DISABLE;
	} else {
		if (mode == GPIO_INT_MODE_LEVEL) {
			if (trig == GPIO_INT_TRIG_HIGH) {
				*pin_ctr1 = MCHP_GPIO_CTRL_IDET_LVL_HI;
			} else {
				*pin_ctr1 = MCHP_GPIO_CTRL_IDET_LVL_LO;
			}
		} else {
			switch (trig) {
			case GPIO_INT_TRIG_LOW:
				*pin_ctr1 = MCHP_GPIO_CTRL_IDET_FEDGE;
				break;
			case GPIO_INT_TRIG_HIGH:
				*pin_ctr1 = MCHP_GPIO_CTRL_IDET_REDGE;
				break;
			case GPIO_INT_TRIG_BOTH:
				*pin_ctr1 = MCHP_GPIO_CTRL_IDET_BEDGE;
				break;
			default:
				return -EINVAL;
			}
		}
	}

	return 0;
}

static void gpio_xec_intr_en(gpio_pin_t pin, enum gpio_int_mode mode,
			     uint8_t girq_id)
{
	if (mode != GPIO_INT_MODE_DISABLED) {
		/* Enable interrupt to propagate via its GIRQ to the NVIC */
		mchp_soc_ecia_girq_src_en(girq_id, pin);
	}
}

static int gpio_xec_pin_interrupt_configure(const struct device *dev,
					    gpio_pin_t pin,
					    enum gpio_int_mode mode,
					    enum gpio_int_trig trig)
{
	const struct gpio_xec_config *config = dev->config;
	uintptr_t pcr1_addr = pin_ctrl_addr(dev, pin);
	uint32_t pcr1 = 0u;
	uint32_t pcr1_req = 0u;

	/* Validate pin number range in terms of current port */
	if ((valid_ctrl_masks[config->port_num] & BIT(pin)) == 0U) {
		return -EINVAL;
	}

	/* Check if GPIO port supports interrupts */
	if ((mode != GPIO_INT_MODE_DISABLED) &&
	    ((config->flags & GPIO_INT_ENABLE) == 0)) {
		return -ENOTSUP;
	}

	pcr1_req = MCHP_GPIO_CTRL_IDET_DISABLE;
	if (gen_gpio_ctrl_icfg(mode, trig, &pcr1_req)) {
		return -EINVAL;
	}

	/* Disable interrupt in the EC aggregator */
	mchp_soc_ecia_girq_src_dis(config->girq_id, pin);

	/* pin configuration matches requested detection mode? */
	pcr1 = sys_read32(pcr1_addr);

	if ((pcr1 & MCHP_GPIO_CTRL_IDET_MASK) == pcr1_req) {
		gpio_xec_intr_en(pin, mode, config->girq_id);
		return 0;
	}

	pcr1 &= ~MCHP_GPIO_CTRL_IDET_MASK;

	if (mode == GPIO_INT_MODE_LEVEL) {
		if (trig == GPIO_INT_TRIG_HIGH) {
			pcr1 |= MCHP_GPIO_CTRL_IDET_LVL_HI;
		} else {
			pcr1 |= MCHP_GPIO_CTRL_IDET_LVL_LO;
		}
	} else if (mode == GPIO_INT_MODE_EDGE) {
		if (trig == GPIO_INT_TRIG_LOW) {
			pcr1 |= MCHP_GPIO_CTRL_IDET_FEDGE;
		} else if (trig == GPIO_INT_TRIG_HIGH) {
			pcr1 |= MCHP_GPIO_CTRL_IDET_REDGE;
		} else if (trig == GPIO_INT_TRIG_BOTH) {
			pcr1 |= MCHP_GPIO_CTRL_IDET_BEDGE;
		}
	} else {
		pcr1 |= MCHP_GPIO_CTRL_IDET_DISABLE;
	}

	sys_write32(pcr1, pcr1_addr);
	/* delay for HW to synchronize after it ungates its clock */
	for (int i = 0; i < XEC_GPIO_EDGE_DLY_COUNT; i++) {
		sys_read32(pcr1_addr);
	}

	mchp_soc_ecia_girq_src_clr(config->girq_id, pin);

	gpio_xec_intr_en(pin, mode, config->girq_id);

	return 0;
}

static int gpio_xec_port_set_masked_raw(const struct device *dev,
					uint32_t mask,
					uint32_t value)
{
	uintptr_t pout_addr = pin_parout_addr(dev);

	xec_mask_write32(pout_addr, mask, value);

	return 0;
}

static int gpio_xec_port_set_bits_raw(const struct device *dev, uint32_t mask)
{
	uintptr_t pout_addr = pin_parout_addr(dev);

	sys_write32(sys_read32(pout_addr) | mask, pout_addr);

	return 0;
}

static int gpio_xec_port_clear_bits_raw(const struct device *dev,
					uint32_t mask)
{
	uintptr_t pout_addr = pin_parout_addr(dev);

	sys_write32(sys_read32(pout_addr) & ~mask, pout_addr);

	return 0;
}

static int gpio_xec_port_toggle_bits(const struct device *dev, uint32_t mask)
{
	uintptr_t pout_addr = pin_parout_addr(dev);

	sys_write32(sys_read32(pout_addr) ^ mask, pout_addr);

	return 0;
}

static int gpio_xec_port_get_raw(const struct device *dev, uint32_t *value)
{
	uintptr_t pin_addr = pin_parin_addr(dev);

	*value = sys_read32(pin_addr);

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
	girq_result = mchp_soc_ecia_girq_result(config->girq_id);

	/* Clear source register in aggregator before firing callbacks */
	mchp_soc_ecia_girq_src_clr_bitmap(config->girq_id, girq_result);

	gpio_fire_callbacks(&data->callbacks, dev, girq_result);
}

/* GPIO driver official API table */
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

#define XEC_GPIO_PORT_FLAGS(n)						\
	((DT_INST_IRQ_HAS_CELL(n, irq)) ? GPIO_INT_ENABLE : 0)

#define XEC_GPIO_PORT(n)						\
	static int gpio_xec_port_init_##n(const struct device *dev)	\
	{								\
		if (!(DT_INST_IRQ_HAS_CELL(n, irq))) {			\
			return 0;					\
		}							\
									\
		const struct gpio_xec_config *config = dev->config;	\
									\
		mchp_soc_ecia_girq_aggr_en(config->girq_id, 1);		\
									\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    DT_INST_IRQ(n, priority),			\
			    gpio_gpio_xec_port_isr,			\
			    DEVICE_DT_INST_GET(n), 0U);			\
									\
		irq_enable(DT_INST_IRQN(n));				\
									\
		return 0;						\
	}								\
									\
	static struct gpio_xec_data gpio_xec_port_data_##n;		\
									\
	static const struct gpio_xec_config xec_gpio_config_##n = {	\
		.common = {						\
			.port_pin_mask =				\
				GPIO_PORT_PIN_MASK_FROM_DT_INST(n),	\
		},							\
		.pcr1_base = (uintptr_t)DT_INST_REG_ADDR_BY_IDX(n, 0),	\
		.parin_addr = (uintptr_t)DT_INST_REG_ADDR_BY_IDX(n, 1),	\
		.parout_addr = (uintptr_t)DT_INST_REG_ADDR_BY_IDX(n, 2),\
		.port_num = DT_INST_PROP(n, port_id),			\
		.girq_id = DT_INST_PROP_OR(n, girq_id, 0),		\
		.flags = XEC_GPIO_PORT_FLAGS(n),			\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n, gpio_xec_port_init_##n, NULL,		\
		&gpio_xec_port_data_##n, &xec_gpio_config_##n,		\
		PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,			\
		&gpio_xec_driver_api);

DT_INST_FOREACH_STATUS_OKAY(XEC_GPIO_PORT)
