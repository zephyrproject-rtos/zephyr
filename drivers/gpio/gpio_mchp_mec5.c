/*
 * Copyright (c) 2024 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mec5_gpio

#include <errno.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/gpio/gpio.h>
#include <zephyr/dt-bindings/pinctrl/mchp-xec-pinctrl.h>
#include <soc.h>
#include <zephyr/irq.h>
#include <mec_gpio_api.h>

#include <zephyr/drivers/gpio/gpio_utils.h>

/* 32 pins per bank. Each pin has a 4-byte control register */
#define MEC5_GPIO_PIN_CTRL_RSHFT    7
#define MEC5_GPIO_PIN_CTRL_ADDR_MSK 0xfu

struct gpio_mec5_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* port ISR callback routine address */
	sys_slist_t callbacks;
};

struct gpio_mec5_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	uintptr_t pcr1_base;
	uintptr_t parin_addr;
	uintptr_t parout_addr;
	uint32_t flags;
};

static inline uint32_t mec5_addr_to_port(uint32_t base_addr)
{
	return ((base_addr >> MEC5_GPIO_PIN_CTRL_RSHFT) & MEC5_GPIO_PIN_CTRL_ADDR_MSK);
}

/* NOTE: gpio_flags_t b[0:15] are defined in the dt-binding gpio header.
 * b[31:16] are defined in the driver gpio header.
 */
static int gpio_mec5_validate_flags(gpio_flags_t flags)
{
	if (flags & GPIO_LINE_OPEN_SOURCE) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_OUTPUT_INIT_LOW) && (flags & GPIO_OUTPUT_INIT_HIGH)) {
		return -EINVAL;
	}

	return 0;
}

static const struct mec_gpio_props cfg_props_init[] = {
	{MEC_GPIO_PWRGT_PROP_ID, MEC_GPIO_PROP_PWRGT_VTR},
	{MEC_GPIO_OSEL_PROP_ID, MEC_GPIO_PROP_OSEL_CTRL},
	{MEC_GPIO_INPAD_DIS_PROP_ID, MEC_GPIO_PROP_INPAD_EN},
};

/* Each GPIO pin has two 32-bit control registers. Control 1 configures pin
 * features except for drive strength and slew rate in Control 2.
 * A pin's input and output state can be read/written from either the Control 1
 * register or from corresponding bits in the GPIO parallel input/output registers.
 * The parallel input and output registers group 32 pins into each register.
 * The GPIO hardware restricts the pin output state to Control 1 or the parallel bit.
 * Both output bits reflect each other on read and writes but only one is writable
 * selected by the output control select bit in Control 1. In the configuration API
 * we use Control 1 to configure all pin features and output state. Before exiting,
 * we set the output select for parallel mode enabling writes to the parallel output bit.
 */
static int gpio_mec5_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_mec5_config *config = dev->config;
	uint32_t pin_num = 0u;
	uint32_t port_num = 0u;
	uint32_t temp = 0u;
	int ret = 0;
	size_t idx = 0;
	struct mec_gpio_props props[8];

	port_num = mec5_addr_to_port(config->pcr1_base);
	ret = mec_hal_gpio_pin_num(port_num, pin, &pin_num);
	if (ret != MEC_RET_OK) {
		return -EINVAL;
	}

	ret = mec_hal_gpio_port_pin_valid(port_num, pin);
	if (ret != MEC_RET_OK) {
		return -EINVAL;
	}

	ret = gpio_mec5_validate_flags(flags);
	if (ret) {
		return ret;
	}

	if (flags == GPIO_DISCONNECTED) {
		ret = mec_hal_gpio_set_property(pin_num, MEC_GPIO_PWRGT_PROP_ID,
						MEC_GPIO_PROP_PWRGT_OFF);
		if (ret != MEC_RET_OK) {
			ret = -EIO;
		}
		return ret;
	}

	ret = mec_hal_gpio_set_props(pin_num, cfg_props_init, ARRAY_SIZE(cfg_props_init));
	if (ret != MEC_RET_OK) {
		return -EIO;
	}

	if (flags & GPIO_OUTPUT) {
		props[idx].prop = MEC_GPIO_DIR_PROP_ID;
		props[idx].val = MEC_GPIO_PROP_DIR_OUT;
		idx++;

		props[idx].prop = MEC_GPIO_OBUFT_PROP_ID;
		props[idx].val = MEC_GPIO_PROP_PUSH_PULL;
		if (flags & GPIO_LINE_OPEN_DRAIN) {
			props[idx].val = MEC_GPIO_PROP_OPEN_DRAIN;
		}
		idx++;

		props[idx].prop = MEC_GPIO_CTRL_OUT_VAL_ID;
		props[idx].val = 0u;
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			props[idx].val = 1u;
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			props[idx].val = 0u;
		} else {
			mec_hal_gpio_pad_in(pin_num, &props[idx].val);
		}
		idx++;
	}

	if (flags & GPIO_INPUT) {
		props[idx].prop = MEC_GPIO_DIR_PROP_ID;
		props[idx].val = MEC_GPIO_PROP_DIR_IN;
		idx++;
	}

	temp = flags & (GPIO_PULL_UP | GPIO_PULL_DOWN);
	if (temp) {
		props[idx].prop = MEC_GPIO_PUD_PROP_ID;
		if (temp == (GPIO_PULL_UP | GPIO_PULL_DOWN)) {
			props[idx].val = MEC_GPIO_PROP_REPEATER;
		} else if (temp & GPIO_PULL_UP) {
			props[idx].val = MEC_GPIO_PROP_PULL_UP;
		} else {
			props[idx].val = MEC_GPIO_PROP_PULL_DN;
		}
		idx++;
	}

	ret = mec_hal_gpio_set_props(pin_num, props, idx);
	if (ret != MEC_RET_OK) {
		return -EIO;
	}

	/* make output state in control read-only in control and read-write in parallel reg */
	ret = mec_hal_gpio_set_property(pin_num, MEC_GPIO_OSEL_PROP_ID, MEC_GPIO_PROP_OSEL_PAROUT);
	if (ret != MEC_RET_OK) {
		return -EIO;
	}

	return 0;
}

static uint8_t gen_gpio_ctrl_icfg(enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	uint8_t idet;

	if (mode == GPIO_INT_MODE_DISABLED) {
		idet = MEC_GPIO_PROP_IDET_DIS;
	} else {
		if (mode == GPIO_INT_MODE_LEVEL) {
			if (trig == GPIO_INT_TRIG_HIGH) {
				idet = MEC_GPIO_PROP_IDET_HI_LVL;
			} else {
				idet = MEC_GPIO_PROP_IDET_LO_LVL;
			}
		} else {
			switch (trig) {
			case GPIO_INT_TRIG_LOW:
				idet = MEC_GPIO_PROP_IDET_EDGE_DN;
				break;
			case GPIO_INT_TRIG_HIGH:
				idet = MEC_GPIO_PROP_IDET_EDGE_UP;
				break;
			case GPIO_INT_TRIG_BOTH:
				idet = MEC_GPIO_PROP_IDET_EDGE_BOTH;
				break;
			default:
				idet = MEC_GPIO_PROP_IDET_DIS;
				break;
			}
		}
	}

	return idet;
}

/* Enable interrupt to propagate via its GIRQ to the NVIC */
static void gpio_mec5_intr_en(uint8_t port, gpio_pin_t pin, enum gpio_int_mode mode)
{
	uint8_t en = 0u;

	if (mode != GPIO_INT_MODE_DISABLED) {
		en = 1u;
	}

	mec_hal_gpio_port_pin_ia_enable(port, pin, en);
}

static const struct mec_gpio_props icfg_props_init[] = {
	{MEC_GPIO_PWRGT_PROP_ID, MEC_GPIO_PROP_PWRGT_VTR},
	{MEC_GPIO_INPAD_DIS_PROP_ID, MEC_GPIO_PROP_INPAD_EN},
};

static int gpio_mec5_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					     enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_mec5_config *config = dev->config;
	uint32_t pin_num = (uint32_t)MEC_PIN_MAX;
	uint32_t port_num = 0;
	int ret = 0;
	uint8_t idet = 0, idet_curr = 0;

	/* Validate pin number range in terms of current port */
	port_num = mec5_addr_to_port(config->pcr1_base);
	ret = mec_hal_gpio_port_pin_valid(port_num, pin);
	if (ret != MEC_RET_OK) {
		return -EINVAL;
	}

	/* Check if GPIO port supports interrupts */
	if ((mode != GPIO_INT_MODE_DISABLED) && !(config->flags & GPIO_INT_ENABLE)) {
		return -ENOTSUP;
	}

	/* Disable interrupt in the EC aggregator */
	gpio_mec5_intr_en(port_num, pin, 0);

	mec_hal_gpio_pin_num(port_num, pin, &pin_num);
	ret = mec_hal_gpio_set_props(pin_num, icfg_props_init, ARRAY_SIZE(icfg_props_init));
	if (ret) {
		return -EIO;
	}

	idet_curr = MEC_GPIO_PROP_IDET_DIS;
	ret = mec_hal_gpio_get_property(pin_num, MEC_GPIO_IDET_PROP_ID, &idet_curr);
	if (ret != MEC_RET_OK) {
		return -EIO;
	}

	idet = gen_gpio_ctrl_icfg(mode, trig);
	if (idet_curr == idet) {
		gpio_mec5_intr_en(port_num, pin, mode);
		return 0;
	}

	ret = mec_hal_gpio_set_property(pin_num, MEC_GPIO_IDET_PROP_ID, idet);
	if (ret != MEC_RET_OK) {
		return -EIO;
	}

	ret = mec_hal_gpio_pin_ia_status_clr(pin_num);
	if (ret != MEC_RET_OK) {
		return -EIO;
	}

	gpio_mec5_intr_en(port_num, pin, mode);

	return 0;
}

static int gpio_mec5_port_set_masked_raw(const struct device *dev, uint32_t mask, uint32_t value)
{
	const struct gpio_mec5_config *config = dev->config;
	uint32_t port_num = mec5_addr_to_port(config->pcr1_base);
	int ret = mec_hal_gpio_parout_port_mask(port_num, value, (const uint32_t)mask);

	if (ret != MEC_RET_OK) {
		return -EIO;
	}

	return 0;
}

static int gpio_mec5_port_set_bits_raw(const struct device *dev, uint32_t mask)
{
	const struct gpio_mec5_config *config = dev->config;
	uint32_t port_num = mec5_addr_to_port(config->pcr1_base);
	int ret = mec_hal_gpio_parout_port_set_bits(port_num, (const uint32_t)mask);

	if (ret != MEC_RET_OK) {
		return -EIO;
	}

	return 0;
}

static int gpio_mec5_port_clear_bits_raw(const struct device *dev, uint32_t mask)
{
	const struct gpio_mec5_config *config = dev->config;
	uint32_t port_num = mec5_addr_to_port(config->pcr1_base);
	int ret = mec_hal_gpio_parout_port_mask(port_num, 0u, (const uint32_t)mask);

	if (ret != MEC_RET_OK) {
		return -EIO;
	}

	return 0;
}

static int gpio_mec5_port_toggle_bits(const struct device *dev, uint32_t mask)
{
	const struct gpio_mec5_config *config = dev->config;
	uint32_t port_num = mec5_addr_to_port(config->pcr1_base);

	if (mec_hal_gpio_parout_port_xor(port_num, mask) != MEC_RET_OK) {
		return -EIO;
	}

	return 0;
}

static int gpio_mec5_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_mec5_config *config = dev->config;
	uint32_t port_num = mec5_addr_to_port(config->pcr1_base);

	if (mec_hal_gpio_parin_port(port_num, value) != MEC_RET_OK) {
		return -EIO;
	}

	return 0;
}

static int gpio_mec5_manage_callback(const struct device *dev, struct gpio_callback *callback,
				     bool set)
{
	struct gpio_mec5_data *data = dev->data;

	gpio_manage_callback(&data->callbacks, callback, set);

	return 0;
}

#ifdef CONFIG_GPIO_GET_DIRECTION
static int gpio_mec5_get_direction(const struct device *port, gpio_port_pins_t map,
				   gpio_port_pins_t *inputs, gpio_port_pins_t *outputs)
{
	if (!port) {
		return -EINVAL;
	}

	const struct gpio_mec5_config *config = port->config;
	uint32_t valid_msk = 0u;
	uint32_t pin_num = 0u, port_num = 0u;
	int ret = 0;
	uint8_t pwr_gate = 0u, dir = 0u, in_pad_dis = 0u;

	port_num = mec5_addr_to_port(config->pcr1_base);
	ret = mec_hal_gpio_port_valid_mask(port_num, &valid_msk);
	if (ret != MEC_RET_OK) {
		return -EIO;
	}

	*inputs = 0u;
	*outputs = 0u;
	for (uint8_t pin_pos = 0; pin_pos < 32; pin++) {
		if (!map) {
			break;
		}
		if ((map & BIT(pin_pos)) && (valid_msk & BIT(pin_pos))) {
			mec_hal_gpio_pin_num(port_num, pin, &pin_num);
			mec_hal_gpio_get_property(pin_num, MEC_GPIO_PWRGT_PROP_ID, &pwr_gate);
			mec_hal_gpio_get_property(pin_num, MEC_GPIO_DIR_PROP_ID, &dir);
			mec_hal_gpio_get_property(pin_num, MEC_GPIO_INPAD_DIS_PROP_ID, &in_pad_dis);

			if (pwr_gate != MEC_GPIO_PROP_PWRGT_OFF) {
				if (outputs && (dir == MEC_GPIO_PROP_DIR_OUT)) {
					*outputs |= BIT(pin_pos);
				} else if (inputs && (in_pad_dis == MEC_GPIO_PROP_INPAD_EN)) {
					*inputs |= BIT(pin_pos);
				}
			}

			map &= ~BIT(pin_pos);
		}
	}

	return 0;
}
#endif

#ifdef CONFIG_GPIO_GET_CONFIG
int gpio_mec5_get_config(const struct device *port, gpio_pin_t pin, gpio_flags_t *flags)
{
	if (!port || !flags) {
		return -EINVAL;
	}

	const struct gpio_mec5_config *config = port->config;
	uint32_t port_num = mec5_addr_to_port(config->pcr1_base);
	int ret = mec_hal_gpio_port_pin_valid(port_num, pin);

	if (ret != MEC_RET_OK) {
		return -EINVAL;
	}

	uint32_t pin_ctrl = mec_hal_gpio_port_get_ctrl_nc(port_num, pin);
	uint32_t pin_flags = 0u;
	uint8_t prop = MEC_GPIO_PROP_DIR_IN;

	mec_hal_gpio_get_ctrl_property(pin_ctrl, MEC_GPIO_DIR_PROP_ID, &prop);

	if (prop == MEC_GPIO_PROP_DIR_OUT) {
		pin_flags |= GPIO_OUTPUT;
		mec_hal_gpio_get_ctrl_property(pin_ctrl, MEC_GPIO_CTRL_OUT_VAL_ID, &prop);
		if (prop != 0) {
			pin_flags |= GPIO_OUTPUT_INIT_HIGH;
		} else {
			pin_flags |= GPIO_OUTPUT_INIT_LOW;
		}

		prop = MEC_GPIO_PROP_PUSH_PULL;
		mec_hal_gpio_get_ctrl_property(pin_ctrl, MEC_GPIO_OBUFT_PROP_ID, &prop);
		if (prop == MEC_GPIO_PROP_OPEN_DRAIN) {
			pin_flags |= GPIO_OPEN_DRAIN;
		}
	} else {
		prop = MEC_GPIO_PROP_INPAD_DIS;
		mec_hal_gpio_get_ctrl_property(pin_ctrl, MEC_GPIO_INPAD_DIS_PROP_ID, &prop);
		if (prop == MEC_GPIO_PROP_INPAD_EN) {
			pin_flags |= GPIO_INPUT;
		}
	}

	if (pin_flags) {
		*flags = pin_flags;
	} else {
		*flags = GPIO_DISCONNECTED;
	}

	return 0;
}
#endif

static void gpio_mec5_port_isr(const struct device *dev)
{
	const struct gpio_mec5_config *config = dev->config;
	struct gpio_mec5_data *data = dev->data;
	uint32_t girq_result = 0xffffffffu;
	uint32_t port_num = 0u;

	/* Figure out which interrupts have been triggered from the EC
	 * aggregator result register
	 */
	port_num = mec5_addr_to_port(config->pcr1_base);
	mec_hal_gpio_port_ia_result(port_num, &girq_result);

	/* Clear source register in aggregator before firing callbacks */
	mec_hal_gpio_port_ia_status_clr_mask(port_num, girq_result);

	gpio_fire_callbacks(&data->callbacks, dev, girq_result);
}

/* GPIO driver official API table */
static const struct gpio_driver_api gpio_mec5_driver_api = {
	.pin_configure = gpio_mec5_configure,
	.port_get_raw = gpio_mec5_port_get_raw,
	.port_set_masked_raw = gpio_mec5_port_set_masked_raw,
	.port_set_bits_raw = gpio_mec5_port_set_bits_raw,
	.port_clear_bits_raw = gpio_mec5_port_clear_bits_raw,
	.port_toggle_bits = gpio_mec5_port_toggle_bits,
	.pin_interrupt_configure = gpio_mec5_pin_interrupt_configure,
	.manage_callback = gpio_mec5_manage_callback,
#ifdef CONFIG_GPIO_GET_DIRECTION
	.port_get_direction = gpio_mec5_get_direction,
#endif
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = gpio_mec5_get_config,
#endif
};

#define MEC5_GPIO_PORT_FLAGS(n) ((DT_INST_IRQ_HAS_CELL(n, irq)) ? GPIO_INT_ENABLE : 0)

/* TODO remove port_num. Derive it from pin & pin_num? */
#define MEC5_GPIO_PORT(n)                                                                          \
	static int gpio_mec5_port_init_##n(const struct device *dev)                               \
	{                                                                                          \
		if (!(DT_INST_IRQ_HAS_CELL(n, irq))) {                                             \
			return 0;                                                                  \
		}                                                                                  \
                                                                                                   \
		const struct gpio_mec5_config *config = dev->config;                               \
		uint32_t port_num = mec5_addr_to_port(config->pcr1_base);                          \
                                                                                                   \
		mec_hal_gpio_port_ia_ctrl(port_num, 1u);                                           \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), gpio_mec5_port_isr,         \
			    DEVICE_DT_INST_GET(n), 0u);                                            \
		irq_enable(DT_INST_IRQN(n));                                                       \
		return 0;                                                                          \
	}                                                                                          \
	static struct gpio_mec5_data gpio_mec5_port_data_##n;                                      \
	static const struct gpio_mec5_config gpio_mec5_config_##n = {                              \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),               \
			},                                                                         \
		.pcr1_base = (uintptr_t)DT_INST_REG_ADDR_BY_IDX(n, 0),                             \
		.parin_addr = (uintptr_t)DT_INST_REG_ADDR_BY_IDX(n, 1),                            \
		.parout_addr = (uintptr_t)DT_INST_REG_ADDR_BY_IDX(n, 2),                           \
		.flags = MEC5_GPIO_PORT_FLAGS(n),                                                  \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, gpio_mec5_port_init_##n, NULL, &gpio_mec5_port_data_##n,          \
			      &gpio_mec5_config_##n, PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,      \
			      &gpio_mec5_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MEC5_GPIO_PORT)
