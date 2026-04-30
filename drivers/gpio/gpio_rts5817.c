/*
 * Copyright (c) 2026 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/dt-bindings/gpio/realtek-rts5817-gpio.h>
#include "../pinctrl/pinctrl_rts5817.h"

#define DT_DRV_COMPAT realtek_rts5817_gpio

#define PAD_CFG_SIZE 0x40

struct rts_fp_gpio_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	sys_slist_t callbacks;
	struct k_spinlock lock;
};

struct rts_fp_gpio_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	mem_addr_t base;
	mem_addr_t sp_base0;
	mem_addr_t sp_base1;
	uint32_t ngpios;
};

static inline void rts_writel_mask(uint32_t reg, uint32_t val, uint32_t mask)
{
	uint32_t temp = sys_read32(reg) & ~(mask);

	sys_write32(temp | ((val) & (mask)), reg);
};

static int rts_fp_get_raw(const struct device *port, gpio_port_value_t *value)
{
	const struct rts_fp_gpio_config *config = port->config;
	struct rts_fp_gpio_data *data = port->data;
	mem_addr_t base = config->base;
	mem_addr_t sp_base0 = config->sp_base0;
	mem_addr_t sp_base1 = config->sp_base1;
	k_spinlock_key_t key;
	bool input;
	mem_addr_t addr;

	*value = 0;

	key = k_spin_lock(&data->lock);

	for (int i = 0; i < config->ngpios; i++) {
		if (i < GPIO_AL0) {
			input = sys_read32(base + PAD_GPIO_I + i * PAD_CFG_SIZE);
			*value |= input << i;
		}
		if (i >= GPIO_AL0 && i <= GPIO_AL2) {
			*value |= ((sys_read32(sp_base0 + AL_GPIO_VALUE) &
				    BIT(AL_GPIO_I_OFFSET + i - GPIO_AL0)) >>
				   (AL_GPIO_I_OFFSET + i - GPIO_AL0))
				  << i;
		}
		if (i >= GPIO_SNR_RST && i <= GPIO_SNR_GPIO) {
			if (i == GPIO_SNR_RST) {
				addr = sp_base0 + AL_SENSOR_RST_CTL;
			} else if (i == GPIO_SNR_CS) {
				addr = sp_base0 + AL_SENSOR_SCS_N_CTL;
			} else { /* GPIO_SNR_GPIO */
				addr = sp_base0 + GPIO_SVIO_CFG;
			}

			*value |= ((sys_read32(addr) & PAD_SENSOR_RSTN_I_MASK) >>
				   PAD_SENSOR_RSTN_I_OFFSET)
				  << i;
		}
		if (i >= GPI_WAKE1 && i <= GPI_WAKE2) {
			k_busy_wait(30);
			*value |= ((sys_read32(sp_base1 + SENSOR_INT_FLAG) &
				    BIT(WAKE_UP_I_OFFSET + i - GPI_WAKE1)) >>
				   (WAKE_UP_I_OFFSET + i - GPI_WAKE1))
				  << i;
		}
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int rts_fp_set_masked_raw(const struct device *port, gpio_port_pins_t pins,
				 gpio_port_value_t value)
{
	const struct rts_fp_gpio_config *config = port->config;
	struct rts_fp_gpio_data *data = port->data;
	mem_addr_t base = config->base;
	mem_addr_t sp_base0 = config->sp_base0;
	k_spinlock_key_t key;
	mem_addr_t addr;

	key = k_spin_lock(&data->lock);

	for (int i = 0; i < config->ngpios; i++) {
		if ((pins & (1 << i)) == 0) {
			continue;
		}
		if (i < GPIO_AL0) {
			sys_write32((value >> i) & BIT(0), base + PAD_GPIO_O + i * PAD_CFG_SIZE);
		} else if (i >= GPIO_AL0 && i <= GPIO_AL2) {
			rts_writel_mask(sp_base0 + AL_GPIO_VALUE,
					((value >> i) & BIT(0))
						<< (AL_GPIO_O_OFFSET + i - GPIO_AL0),
					BIT(AL_GPIO_O_OFFSET + i - GPIO_AL0));
		} else if (i >= GPIO_SNR_RST && i <= GPIO_SNR_GPIO) {
			if (i == GPIO_SNR_RST) {
				addr = sp_base0 + AL_SENSOR_RST_CTL;
			} else if (i == GPIO_SNR_CS) {
				addr = sp_base0 + AL_SENSOR_SCS_N_CTL;
			} else { /* GPIO_SNR_GPIO */
				addr = sp_base0 + GPIO_SVIO_CFG;
			}
			rts_writel_mask(addr, ((value >> i) & BIT(0)) << SENSOR_RSTN_O_OFFSET,
					SENSOR_RSTN_O_MASK);
		}
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static void rts_fp_set_bits(const struct device *port, gpio_port_pins_t pins)
{
	const struct rts_fp_gpio_config *config = port->config;
	mem_addr_t base = config->base;
	mem_addr_t sp_base0 = config->sp_base0;

	for (int i = 0; i < config->ngpios; i++) {
		if ((pins & (1 << i)) == 0) {
			continue;
		}
		if (i < GPIO_AL0) {
			sys_set_bit(base + PAD_GPIO_O + i * PAD_CFG_SIZE, GPIO_O_OFFSET);
		} else if (i >= GPIO_AL0 && i <= GPIO_AL2) {
			sys_set_bit(sp_base0 + AL_GPIO_VALUE, AL_GPIO_O_OFFSET + i - GPIO_AL0);
		} else if (i == GPIO_SNR_RST) {
			sys_set_bit(sp_base0 + AL_SENSOR_RST_CTL, SENSOR_RSTN_O_OFFSET);
		} else if (i == GPIO_SNR_CS) {
			sys_set_bit(sp_base0 + AL_SENSOR_SCS_N_CTL, SENSOR_SCS_N_O_OFFSET);
		} else if (i == GPIO_SNR_GPIO) {
			sys_set_bit(sp_base0 + GPIO_SVIO_CFG, GPIO_SVIO_O_OFFSET);
		}
	}
}

static int rts_fp_set_bits_raw(const struct device *port, gpio_port_pins_t pins)
{
	struct rts_fp_gpio_data *data = port->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	rts_fp_set_bits(port, pins);

	k_spin_unlock(&data->lock, key);

	return 0;
}
static void rts_fp_clear_bits(const struct device *port, gpio_port_pins_t pins)
{
	const struct rts_fp_gpio_config *config = port->config;
	mem_addr_t base = config->base;
	mem_addr_t sp_base0 = config->sp_base0;

	for (int i = 0; i < config->ngpios; i++) {
		if ((pins & (1 << i)) == 0) {
			continue;
		}
		if (i < GPIO_AL0) {
			sys_clear_bit(base + PAD_GPIO_O + i * PAD_CFG_SIZE, GPIO_O_OFFSET);
		} else if (i >= GPIO_AL0 && i <= GPIO_AL2) {
			sys_clear_bit(sp_base0 + AL_GPIO_VALUE, AL_GPIO_O_OFFSET + i - GPIO_AL0);
		} else if (i == GPIO_SNR_RST) {
			sys_clear_bit(sp_base0 + AL_SENSOR_RST_CTL, SENSOR_RSTN_O_OFFSET);
		} else if (i == GPIO_SNR_CS) {
			sys_clear_bit(sp_base0 + AL_SENSOR_SCS_N_CTL, SENSOR_SCS_N_O_OFFSET);
		} else if (i == GPIO_SNR_GPIO) {
			sys_clear_bit(sp_base0 + GPIO_SVIO_CFG, GPIO_SVIO_O_OFFSET);
		}
	}
}

static int rts_fp_clear_bits_raw(const struct device *port, gpio_port_pins_t pins)
{
	struct rts_fp_gpio_data *data = port->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	rts_fp_clear_bits(port, pins);

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int rts_fp_toggle_bits(const struct device *port, gpio_port_pins_t pins)
{
	const struct rts_fp_gpio_config *config = port->config;
	struct rts_fp_gpio_data *data = port->data;
	mem_addr_t base = config->base;
	mem_addr_t sp_base0 = config->sp_base0;
	k_spinlock_key_t key;
	uint32_t value;
	mem_addr_t addr;

	key = k_spin_lock(&data->lock);

	for (int i = 0; i < config->ngpios; i++) {
		if ((pins & (1 << i)) == 0) {
			continue;
		}
		if (i < GPIO_AL0) {
			value = sys_read32(base + PAD_GPIO_O + i * PAD_CFG_SIZE);
			value ^= GPIO_O_MASK;
			sys_write32(value, base + PAD_GPIO_O + i * PAD_CFG_SIZE);
		} else if (i >= GPIO_AL0 && i <= GPIO_AL2) {
			value = sys_read32(sp_base0 + AL_GPIO_VALUE);
			value ^= BIT(AL_GPIO_O_OFFSET + i - GPIO_AL0);
			sys_write32(value, sp_base0 + AL_GPIO_VALUE);
		} else if (i >= GPIO_SNR_RST && i <= GPIO_SNR_GPIO) {
			if (i == GPIO_SNR_RST) {
				addr = sp_base0 + AL_SENSOR_RST_CTL;
			} else if (i == GPIO_SNR_CS) {
				addr = sp_base0 + AL_SENSOR_SCS_N_CTL;
			} else { /* GPIO_SNR_GPIO */
				addr = sp_base0 + GPIO_SVIO_CFG;
			}
			value = sys_read32(addr);
			value ^= BIT(SENSOR_RSTN_O_OFFSET);
			sys_write32(value, addr);
		}
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int rts_fp_pin_configure(const struct device *port, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct rts_fp_gpio_config *config = port->config;
	struct rts_fp_gpio_data *data = port->data;
	mem_addr_t base = config->base;
	mem_addr_t sp_base0 = config->sp_base0;
	k_spinlock_key_t key;

	if (pin >= config->ngpios) {
		return -EINVAL;
	}

	if (((flags & GPIO_INPUT) != 0) && ((flags & GPIO_OUTPUT) != 0)) {
		return -ENOTSUP;
	}

	if ((flags & (GPIO_INPUT | GPIO_OUTPUT)) == 0) {
		return -ENOTSUP;
	}

	if (((flags & GPIO_POWER_1V8) != 0) && ((flags & GPIO_POWER_3V3) != 0)) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&data->lock);

	if (flags & GPIO_OUTPUT) {
		if (pin < GPIO_AL0) {
			sys_set_bit(base + PAD_GPIO_OE + pin * PAD_CFG_SIZE, GPIO_OE_OFFSET);
		} else if (pin >= GPIO_AL0 && pin <= GPIO_AL2) {
			sys_set_bit(sp_base0 + AL_GPIO_CFG, AL_GPIO_OE_OFFSET + pin - GPIO_AL0);
		} else if (pin == GPIO_SNR_RST) {
			sys_set_bit(sp_base0 + AL_SENSOR_RST_CTL, SENSOR_RSTN_OE0_OFFSET);
		} else if (pin == GPIO_SNR_CS) {
			sys_set_bit(sp_base0 + AL_SENSOR_SCS_N_CTL, SENSOR_SCS_N_OE0_OFFSET);
		} else if (pin == GPIO_SNR_GPIO) {
			sys_set_bit(sp_base0 + GPIO_SVIO_CFG, GPIO_SVIO_OE0_OFFSET);
		}

		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
			rts_fp_set_bits(port, BIT(pin));
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
			rts_fp_clear_bits(port, BIT(pin));
		}
	} else if (flags & GPIO_INPUT) {
		if (pin < GPIO_AL0) {
			sys_clear_bit(base + PAD_GPIO_OE + pin * PAD_CFG_SIZE, GPIO_OE_OFFSET);
		} else if (pin >= GPIO_AL0 && pin <= GPIO_AL2) {
			sys_clear_bit(sp_base0 + AL_GPIO_CFG, AL_GPIO_OE_OFFSET + pin - GPIO_AL0);
		} else if (pin == GPIO_SNR_RST) {
			sys_clear_bit(sp_base0 + AL_SENSOR_RST_CTL, SENSOR_RSTN_OE0_OFFSET);
		} else if (pin == GPIO_SNR_CS) {
			sys_clear_bit(sp_base0 + AL_SENSOR_SCS_N_CTL, SENSOR_SCS_N_OE0_OFFSET);
		} else if (pin == GPIO_SNR_GPIO) {
			sys_clear_bit(sp_base0 + GPIO_SVIO_CFG, GPIO_SVIO_OE0_OFFSET);
		}

		if ((flags & GPIO_PULL_DOWN) != 0) {
			if (pin < GPIO_AL0) {
				rts_writel_mask(base + PAD_CFG + pin * PAD_CFG_SIZE, PD_MASK,
						PU_MASK | PD_MASK);
			} else if (pin >= GPIO_AL0 && pin <= GPIO_AL2) {
				rts_writel_mask(
					sp_base0 + AL_GPIO_CFG,
					BIT(AL_GPIO_PD_CTRL_OFFSET + pin - GPIO_AL0),
					BIT(AL_GPIO_PD_CTRL_OFFSET + pin - GPIO_AL0) |
						BIT(AL_GPIO_PU_CTRL_OFFSET + pin - GPIO_AL0));
			} else if (pin == GPIO_SNR_RST) {
				rts_writel_mask(sp_base0 + AL_SENSOR_RST_CTL, SENSOR_RSTN_PDE,
						SENSOR_RSTN_PUE_MASK | SENSOR_RSTN_PDE_MASK);
			} else if (pin == GPIO_SNR_CS) {
				rts_writel_mask(sp_base0 + AL_SENSOR_SCS_N_CTL,
						SENSOR_SCS_N_PDE_MASK,
						SENSOR_SCS_N_PUE_MASK | SENSOR_SCS_N_PDE_MASK);
			} else if (pin == GPIO_SNR_GPIO) {
				rts_writel_mask(sp_base0 + GPIO_SVIO_CFG,
						BIT(GPIO_SVIO_PULLCTL_OFFSET),
						GPIO_SVIO_PULLCTL_MASK);
			}
		} else if ((flags & GPIO_PULL_UP) != 0) {
			if (pin < GPIO_AL0) {
				rts_writel_mask(base + PAD_CFG + pin * PAD_CFG_SIZE, PU_MASK,
						PU_MASK | PD_MASK);
			} else if (pin >= GPIO_AL0 && pin <= GPIO_AL2) {
				rts_writel_mask(
					sp_base0 + AL_GPIO_CFG,
					BIT(AL_GPIO_PU_CTRL_OFFSET + pin - GPIO_AL0),
					BIT(AL_GPIO_PD_CTRL_OFFSET + pin - GPIO_AL0) |
						BIT(AL_GPIO_PU_CTRL_OFFSET + pin - GPIO_AL0));
			} else if (pin == GPIO_SNR_RST) {
				rts_writel_mask(sp_base0 + AL_SENSOR_RST_CTL, SENSOR_RSTN_PUE_MASK,
						SENSOR_RSTN_PUE_MASK | SENSOR_RSTN_PDE_MASK);
			} else if (pin == GPIO_SNR_CS) {
				rts_writel_mask(sp_base0 + AL_SENSOR_SCS_N_CTL,
						SENSOR_SCS_N_PUE_MASK,
						SENSOR_SCS_N_PUE_MASK | SENSOR_SCS_N_PDE_MASK);
			} else if (pin == GPIO_SNR_GPIO) {
				rts_writel_mask(sp_base0 + GPIO_SVIO_CFG,
						BIT(GPIO_SVIO_PULLCTL_OFFSET + 1),
						GPIO_SVIO_PULLCTL_MASK);
			}
		} else { /* Floating */
			if (pin < GPIO_AL0) {
				rts_writel_mask(base + PAD_CFG + pin * PAD_CFG_SIZE, 0,
						PU_MASK | PD_MASK);
			} else if (pin >= GPIO_AL0 && pin <= GPIO_AL2) {
				rts_writel_mask(
					sp_base0 + AL_GPIO_CFG, 0,
					BIT(AL_GPIO_PD_CTRL_OFFSET + pin - GPIO_AL0) |
						BIT(AL_GPIO_PU_CTRL_OFFSET + pin - GPIO_AL0));
			} else if (pin == GPIO_SNR_RST) {
				rts_writel_mask(sp_base0 + AL_SENSOR_RST_CTL, 0,
						SENSOR_RSTN_PUE_MASK | SENSOR_RSTN_PDE_MASK);
			} else if (pin == GPIO_SNR_CS) {
				rts_writel_mask(sp_base0 + AL_SENSOR_SCS_N_CTL, 0,
						SENSOR_SCS_N_PUE_MASK | SENSOR_SCS_N_PDE_MASK);
			} else if (pin == GPIO_SNR_GPIO) {
				rts_writel_mask(sp_base0 + GPIO_SVIO_CFG, 0,
						GPIO_SVIO_PULLCTL_MASK);
			}
		}
	}

	if (flags & GPIO_POWER_3V3) {
		if (pin < GPIO_AL0) {
			rts_writel_mask(base + PAD_CFG + pin * PAD_CFG_SIZE, H3L1_MASK,
					H3L1_MASK | IEV18_MASK);
		} else if (pin == GPIO_SNR_RST) {
			rts_writel_mask(sp_base0 + AL_SENSOR_RST_CTL, SENSOR_RSTN_H3L1_MASK,
					SENSOR_RSTN_IEV18_MASK | SENSOR_RSTN_H3L1_MASK);
		} else if (pin == GPIO_SNR_CS) {
			rts_writel_mask(sp_base0 + AL_SENSOR_SCS_N_CTL, SENSOR_SCS_N_H3L1_MASK,
					SENSOR_SCS_N_IEV18_MASK | SENSOR_SCS_N_H3L1_MASK);
		} else if (pin == GPIO_SNR_GPIO) {
			rts_writel_mask(sp_base0 + GPIO_SVIO_CFG, GPIO_SVIO_H3L1_MASK,
					GPIO_SVIO_IEV18_MASK | GPIO_SVIO_H3L1_MASK);
		}
	} else if (flags & GPIO_POWER_1V8) {
		if (pin < GPIO_AL0) {
			rts_writel_mask(base + PAD_CFG + pin * PAD_CFG_SIZE, 0,
					H3L1_MASK | IEV18_MASK);
		} else if (pin == GPIO_SNR_RST) {
			rts_writel_mask(sp_base0 + AL_SENSOR_RST_CTL, 0,
					SENSOR_RSTN_IEV18_MASK | SENSOR_RSTN_H3L1_MASK);
		} else if (pin == GPIO_SNR_CS) {
			rts_writel_mask(sp_base0 + AL_SENSOR_SCS_N_CTL, 0,
					SENSOR_SCS_N_IEV18_MASK | SENSOR_SCS_N_H3L1_MASK);
		} else if (pin == GPIO_SNR_GPIO) {
			rts_writel_mask(sp_base0 + GPIO_SVIO_CFG, 0,
					GPIO_SVIO_IEV18_MASK | GPIO_SVIO_H3L1_MASK);
		}
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int rts_interrupt_configure(const struct device *port, gpio_pin_t pin,
				   enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct rts_fp_gpio_config *config = port->config;
	struct rts_fp_gpio_data *data = port->data;
	mem_addr_t base = config->base;
	mem_addr_t sp_base0 = config->sp_base0;
	mem_addr_t sp_base1 = config->sp_base1;
	k_spinlock_key_t key;
	int ret = 0;

	if (pin >= config->ngpios) {
		return -EINVAL;
	}

	if (mode == GPIO_INT_MODE_LEVEL) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&data->lock);

	if (mode == GPIO_INT_MODE_DISABLED) {
		if (pin == GPIO_CACHE_CS2) {
			ret = 0;
			goto exit;
		} else if (pin < GPIO_AL0) {
			sys_write32(0x0, base + PAD_GPIO_INT_EN + pin * PAD_CFG_SIZE);
		} else if (pin >= GPIO_AL0 && pin <= GPIO_AL2) {
			sys_clear_bits(sp_base0 + AL_GPIO_IRQ_EN,
				       BIT(AL_GPIO_INT_EN_FALL_OFFSET + pin - GPIO_AL0) |
					       BIT(AL_GPIO_INT_EN_RISE_OFFSET + pin - GPIO_AL0));
		} else if (pin == GPIO_SNR_RST) {
			sys_clear_bits(sp_base0 + AL_SENSOR_RST_CTL,
				       SENSOR_RSTN_GPIO_INT_EN_RISE_MASK |
					       SENSOR_RSTN_GPIO_INT_EN_FALL_MASK);
		} else if (pin == GPIO_SNR_CS) {
			sys_clear_bits(sp_base0 + AL_SENSOR_SCS_N_CTL,
				       SENSOR_SCS_N_GPIO_INT_EN_RISE_MASK |
					       SENSOR_SCS_N_GPIO_INT_EN_RISE_MASK);
		} else if (pin == GPIO_SNR_GPIO) {
			sys_clear_bits(sp_base0 + GPIO_SVIO_CFG,
				       GPIO_SVIO_INT_EN_RISE_MASK | GPIO_SVIO_INT_EN_FALL_MASK);
		} else if (pin >= GPI_WAKE1 && pin <= GPI_WAKE2) {
			sys_clear_bits(sp_base1 + SENSOR_INT_CFG,
				       BIT(CFG_SENSOR_INT_NEG_EN_OFFSET + pin - GPI_WAKE1) |
					       BIT(CFG_SENSOR_INT_POS_EN_OFFSET + pin - GPI_WAKE1));
		}
	} else if (mode == GPIO_INT_MODE_EDGE) {
		if (pin == GPIO_CACHE_CS2) {
			ret = -ENOTSUP;
			goto exit;
		} else if (pin < GPIO_AL0) {
			/* Clear interrupt flags */
			sys_write32(GPIO_FALLING_INT_MASK | GPIO_RISING_INT_MASK,
				    base + PAD_GPIO_INT + pin * PAD_CFG_SIZE);

			if (trig == GPIO_INT_TRIG_LOW) {
				sys_write32(GPIO_FALLING_INT_MASK,
					    base + PAD_GPIO_INT_EN + pin * PAD_CFG_SIZE);
			} else if (trig == GPIO_INT_TRIG_HIGH) {
				sys_write32(GPIO_RISING_INT_MASK,
					    base + PAD_GPIO_INT_EN + pin * PAD_CFG_SIZE);
			} else { /* GPIO_INT_TRIG_BOTH */
				sys_write32(GPIO_FALLING_INT_MASK | GPIO_RISING_INT_MASK,
					    base + PAD_GPIO_INT_EN + pin * PAD_CFG_SIZE);
			}
		} else if (pin >= GPIO_AL0 && pin <= GPIO_AL2) {
			sys_write32(BIT(AL_GPIO_INT_EN_FALL_OFFSET + pin - GPIO_AL0) |
					    BIT(AL_GPIO_INT_EN_RISE_OFFSET + pin - GPIO_AL0),
				    sp_base0 + AL_GPIO_IRQ);

			if (trig == GPIO_INT_TRIG_LOW) {
				sys_set_bit(sp_base0 + AL_GPIO_IRQ_EN,
					    AL_GPIO_INT_EN_FALL_OFFSET + pin - GPIO_AL0);
			} else if (trig == GPIO_INT_TRIG_HIGH) {
				sys_set_bit(sp_base0 + AL_GPIO_IRQ_EN,
					    AL_GPIO_INT_EN_RISE_OFFSET + pin - GPIO_AL0);
			} else { /* GPIO_INT_TRIG_BOTH */
				sys_set_bits(
					sp_base0 + AL_GPIO_IRQ_EN,
					BIT(AL_GPIO_INT_EN_FALL_OFFSET + pin - GPIO_AL0) |
						BIT(AL_GPIO_INT_EN_RISE_OFFSET + pin - GPIO_AL0));
			}
		} else if (pin == GPIO_SNR_RST) {
			sys_write32(SENSOR_RSTN_GPIO_INT_FALL_MASK | SENSOR_RSTN_GPIO_INT_RISE_MASK,
				    sp_base1 + SENSOR_INT_FLAG);

			if (trig == GPIO_INT_TRIG_LOW) {
				sys_set_bit(sp_base0 + AL_SENSOR_RST_CTL,
					    SENSOR_RSTN_GPIO_INT_EN_FALL_OFFSET);
			} else if (trig == GPIO_INT_TRIG_HIGH) {
				sys_set_bit(sp_base0 + AL_SENSOR_RST_CTL,
					    SENSOR_RSTN_GPIO_INT_EN_RISE_OFFSET);
			} else { /* GPIO_INT_TRIG_BOTH */
				sys_set_bits(sp_base0 + AL_SENSOR_RST_CTL,
					     SENSOR_RSTN_GPIO_INT_EN_FALL_MASK |
						     SENSOR_RSTN_GPIO_INT_EN_RISE_MASK);
			}
		} else if (pin == GPIO_SNR_CS) {
			sys_write32(SENSOR_SCS_N_GPIO_INT_FALL_MASK |
					    SENSOR_SCS_N_GPIO_INT_RISE_MASK,
				    sp_base1 + SENSOR_INT_FLAG);

			if (trig == GPIO_INT_TRIG_LOW) {
				sys_set_bit(sp_base0 + AL_SENSOR_SCS_N_CTL,
					    SENSOR_SCS_N_GPIO_INT_EN_FALL_OFFSET);
			} else if (trig == GPIO_INT_TRIG_HIGH) {
				sys_set_bit(sp_base0 + AL_SENSOR_SCS_N_CTL,
					    SENSOR_SCS_N_GPIO_INT_EN_RISE_OFFSET);
			} else { /* GPIO_INT_TRIG_BOTH */
				sys_set_bits(sp_base0 + AL_SENSOR_SCS_N_CTL,
					     SENSOR_SCS_N_GPIO_INT_EN_FALL_MASK |
						     SENSOR_SCS_N_GPIO_INT_EN_RISE_MASK);
			}
		} else if (pin == GPIO_SNR_GPIO) {
			sys_write32(GPIO_SVIO_INT_FALL_MASK | GPIO_SVIO_INT_RISE_MASK,
				    sp_base1 + SENSOR_INT_FLAG);

			if (trig == GPIO_INT_TRIG_LOW) {
				sys_set_bit(sp_base0 + GPIO_SVIO_CFG, GPIO_SVIO_INT_EN_FALL_OFFSET);
			} else if (trig == GPIO_INT_TRIG_HIGH) {
				sys_set_bit(sp_base0 + GPIO_SVIO_CFG, GPIO_SVIO_INT_EN_RISE_OFFSET);
			} else { /* GPIO_INT_TRIG_BOTH */
				sys_set_bits(sp_base0 + GPIO_SVIO_CFG,
					     GPIO_SVIO_INT_EN_FALL_MASK |
						     GPIO_SVIO_INT_EN_RISE_MASK);
			}
		} else if (pin >= GPI_WAKE1 && pin <= GPI_WAKE2) {
			sys_write32(BIT(I_SENSOR_INTF_FALL_OFFSET + pin - GPI_WAKE1) |
					    BIT(I_SENSOR_INTF_RISE_OFFSET + pin - GPI_WAKE1),
				    sp_base1 + SENSOR_INT_FLAG);

			if (trig == GPIO_INT_TRIG_LOW) {
				sys_set_bit(sp_base1 + SENSOR_INT_CFG,
					    CFG_SENSOR_INT_NEG_EN_OFFSET + pin - GPI_WAKE1);
			} else if (trig == GPIO_INT_TRIG_HIGH) {
				sys_set_bit(sp_base1 + SENSOR_INT_CFG,
					    CFG_SENSOR_INT_POS_EN_OFFSET + pin - GPI_WAKE1);
			} else { /* GPIO_INT_TRIG_BOTH */
				sys_set_bits(sp_base1 + SENSOR_INT_CFG,
					     BIT(CFG_SENSOR_INT_NEG_EN_OFFSET + pin - GPI_WAKE1) |
						     BIT(CFG_SENSOR_INT_POS_EN_OFFSET + pin -
							 GPI_WAKE1));
			}
		} else {
			ret = -ENOTSUP;
		}
	}

exit:
	k_spin_unlock(&data->lock, key);

	return ret;
}

static inline int rts_fp_manage_callback(const struct device *port, struct gpio_callback *callback,
					 bool set)
{
	struct rts_fp_gpio_data *data = port->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static int rts_fp_gpio_isr(const struct device *port)
{
	struct rts_fp_gpio_data *data = port->data;
	const struct rts_fp_gpio_config *config = port->config;
	mem_addr_t base = config->base;
	uint32_t pins = 0;
	uint32_t int_rising_flag;
	uint32_t int_rising_en;
	uint32_t int_falling_flag;
	uint32_t int_falling_en;

	for (int i = 0; i < config->ngpios; i++) {
		if (i < GPIO_AL0) {
			int_rising_flag = sys_read32(base + PAD_GPIO_INT + i * PAD_CFG_SIZE) &
					  BIT(GPIO_RISING_INT_OFFSET);
			int_rising_en = sys_read32(base + PAD_GPIO_INT_EN + i * PAD_CFG_SIZE) &
					BIT(GPIO_RISING_INT_EN_OFFSET);

			int_falling_flag = sys_read32(base + PAD_GPIO_INT + i * PAD_CFG_SIZE) &
					   BIT(GPIO_FALLING_INT_OFFSET);
			int_falling_en = sys_read32(base + PAD_GPIO_INT_EN + i * PAD_CFG_SIZE) &
					 BIT(GPIO_FALLING_INT_EN_OFFSET);

			if (int_rising_flag && int_rising_en) {
				sys_write32(GPIO_RISING_INT_MASK,
					    base + PAD_GPIO_INT + i * PAD_CFG_SIZE);
				pins |= BIT(i);
			} else if (int_falling_flag && int_falling_en) {
				sys_write32(GPIO_FALLING_INT_MASK,
					    base + PAD_GPIO_INT + i * PAD_CFG_SIZE);
				pins |= BIT(i);
			}
		}
	}

	gpio_fire_callbacks(&data->callbacks, port, pins);

	return 0;
}
static int rts_fp_sp_gpio_isr(const struct device *port)
{
	struct rts_fp_gpio_data *data = port->data;
	const struct rts_fp_gpio_config *config = port->config;
	mem_addr_t sp_base0 = config->sp_base0;
	mem_addr_t sp_base1 = config->sp_base1;
	uint32_t pins = 0;
	uint32_t int_rising_flag;
	uint32_t int_rising_en;
	uint32_t int_falling_flag;
	uint32_t int_falling_en;

	for (int i = 0; i < config->ngpios; i++) {
		if (i >= GPIO_AL0 && i <= GPIO_AL2) {
			int_rising_flag = sys_read32(sp_base0 + AL_GPIO_IRQ) &
					  BIT(AL_GPIO_INT_RISE_OFFSET + i - GPIO_AL0);
			int_rising_en = sys_read32(sp_base0 + AL_GPIO_IRQ_EN) &
					BIT(AL_GPIO_INT_EN_RISE_OFFSET + i - GPIO_AL0);

			int_falling_flag = sys_read32(sp_base0 + AL_GPIO_IRQ) &
					   BIT(AL_GPIO_INT_FALL_OFFSET + i - GPIO_AL0);
			int_falling_en = sys_read32(sp_base0 + AL_GPIO_IRQ_EN) &
					 BIT(AL_GPIO_INT_EN_FALL_OFFSET + i - GPIO_AL0);

			if (int_rising_flag && int_rising_en) {
				sys_write32(BIT(AL_GPIO_INT_RISE_OFFSET + i - GPIO_AL0),
					    sp_base0 + AL_GPIO_IRQ);
				pins |= BIT(i);
			} else if (int_falling_flag && int_falling_en) {
				sys_write32(BIT(AL_GPIO_INT_FALL_OFFSET + i - GPIO_AL0),
					    sp_base0 + AL_GPIO_IRQ);
				pins |= BIT(i);
			}
		} else if (i == GPIO_SNR_RST) {
			int_rising_flag = sys_read32(sp_base1 + SENSOR_INT_FLAG) &
					  SENSOR_RSTN_GPIO_INT_RISE_MASK;
			int_rising_en = sys_read32(sp_base0 + AL_SENSOR_RST_CTL) &
					SENSOR_RSTN_GPIO_INT_EN_RISE_MASK;

			int_falling_flag = sys_read32(sp_base1 + SENSOR_INT_FLAG) &
					   SENSOR_RSTN_GPIO_INT_FALL_MASK;
			int_falling_en = sys_read32(sp_base0 + AL_SENSOR_RST_CTL) &
					 SENSOR_RSTN_GPIO_INT_EN_FALL_MASK;

			if (int_rising_flag && int_rising_en) {
				sys_write32(SENSOR_RSTN_GPIO_INT_RISE_MASK,
					    sp_base1 + SENSOR_INT_FLAG);
				pins |= BIT(i);
			} else if (int_falling_flag && int_falling_en) {
				sys_write32(SENSOR_RSTN_GPIO_INT_FALL_MASK,
					    sp_base1 + SENSOR_INT_FLAG);
				pins |= BIT(i);
			}
		} else if (i == GPIO_SNR_CS) {
			int_rising_flag = sys_read32(sp_base1 + SENSOR_INT_FLAG) &
					  SENSOR_SCS_N_GPIO_INT_RISE_MASK;
			int_rising_en = sys_read32(sp_base0 + AL_SENSOR_SCS_N_CTL) &
					SENSOR_SCS_N_GPIO_INT_EN_RISE_MASK;

			int_falling_flag = sys_read32(sp_base1 + SENSOR_INT_FLAG) &
					   SENSOR_SCS_N_GPIO_INT_FALL_MASK;
			int_falling_en = sys_read32(sp_base0 + AL_SENSOR_SCS_N_CTL) &
					 SENSOR_SCS_N_GPIO_INT_EN_FALL_MASK;

			if (int_rising_flag && int_rising_en) {
				sys_write32(SENSOR_SCS_N_GPIO_INT_RISE_MASK,
					    sp_base1 + SENSOR_INT_FLAG);
				pins |= BIT(i);
			} else if (int_falling_flag && int_falling_en) {
				sys_write32(SENSOR_SCS_N_GPIO_INT_FALL_MASK,
					    sp_base1 + SENSOR_INT_FLAG);
				pins |= BIT(i);
			}
		} else if (i == GPIO_SNR_GPIO) {
			int_rising_flag =
				sys_read32(sp_base1 + SENSOR_INT_FLAG) & GPIO_SVIO_INT_RISE_MASK;
			int_rising_en =
				sys_read32(sp_base0 + GPIO_SVIO_CFG) & GPIO_SVIO_INT_EN_RISE_MASK;

			int_falling_flag =
				sys_read32(sp_base1 + SENSOR_INT_FLAG) & GPIO_SVIO_INT_FALL_MASK;
			int_falling_en =
				sys_read32(sp_base0 + GPIO_SVIO_CFG) & GPIO_SVIO_INT_EN_FALL_MASK;

			if (int_rising_flag && int_rising_en) {
				sys_write32(GPIO_SVIO_INT_RISE_MASK, sp_base1 + SENSOR_INT_FLAG);
				pins |= BIT(i);
			} else if (int_falling_flag && int_falling_en) {
				sys_write32(GPIO_SVIO_INT_FALL_MASK, sp_base1 + SENSOR_INT_FLAG);
				pins |= BIT(i);
			}
		} else if (i >= GPI_WAKE1 && i <= GPI_WAKE2) {
			/* Wake1 and wake2 have 20us deglitch */
			k_busy_wait(30);
			int_rising_flag = sys_read32(sp_base1 + SENSOR_INT_FLAG) &
					  BIT(I_SENSOR_INTF_RISE_OFFSET + i - GPI_WAKE1);
			int_rising_en = sys_read32(sp_base1 + SENSOR_INT_CFG) &
					BIT(CFG_SENSOR_INT_POS_EN_OFFSET + i - GPI_WAKE1);

			int_falling_flag = sys_read32(sp_base1 + SENSOR_INT_FLAG) &
					   BIT(I_SENSOR_INTF_FALL_OFFSET + i - GPI_WAKE1);
			int_falling_en = sys_read32(sp_base1 + SENSOR_INT_CFG) &
					 BIT(CFG_SENSOR_INT_NEG_EN_OFFSET + i - GPI_WAKE1);

			if (int_rising_flag && int_rising_en) {
				sys_write32(BIT(I_SENSOR_INTF_RISE_OFFSET + i - GPI_WAKE1),
					    sp_base1 + SENSOR_INT_FLAG);
				pins |= BIT(i);
			} else if (int_falling_flag && int_falling_en) {
				sys_write32(BIT(I_SENSOR_INTF_FALL_OFFSET + i - GPI_WAKE1),
					    sp_base1 + SENSOR_INT_FLAG);
				pins |= BIT(i);
			}
		}
	}

	gpio_fire_callbacks(&data->callbacks, port, pins);

	return 0;
}

static DEVICE_API(gpio, rts_fp_gpio_api) = {
	.port_get_raw = rts_fp_get_raw,
	.pin_configure = rts_fp_pin_configure,
	.port_set_masked_raw = rts_fp_set_masked_raw,
	.port_set_bits_raw = rts_fp_set_bits_raw,
	.port_clear_bits_raw = rts_fp_clear_bits_raw,
	.port_toggle_bits = rts_fp_toggle_bits,
	.pin_interrupt_configure = rts_interrupt_configure,
	.manage_callback = rts_fp_manage_callback,

};

static int rts_fp_pins_init(const struct device *dev)
{
	const struct rts_fp_gpio_config *config = dev->config;
	mem_addr_t base = config->base;
	mem_addr_t sp_base0 = config->sp_base0;

	/* Make sure pins default function are gpio. Then the pins not used as pinmux can be used as
	 * gpios. rts5817 has cache_cs2 and sensor_cs two pins which default function is not gpio
	 */
	sys_write32(0x1, base + PAD_GPIO_INC);
	rts_writel_mask(base + PAD_CFG, BIT(9), PAD_FUNCTION_SEL_MASK);
	rts_writel_mask(sp_base0 + AL_SENSOR_SCS_N_CTL, SENSOR_SCS_N_SEL_MASK,
			SENSOR_SCS_N_SEL_MASK);

	return 0;
}

static int rts_fp_gpio_init(const struct device *dev)
{
	rts_fp_pins_init(dev);

	IRQ_CONNECT(DT_INST_IRQN_BY_IDX(0, 0), DT_INST_IRQ_BY_IDX(0, 0, priority), rts_fp_gpio_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN_BY_IDX(0, 0));

	IRQ_CONNECT(DT_INST_IRQN_BY_IDX(0, 1), DT_INST_IRQ_BY_IDX(0, 1, priority),
		    rts_fp_sp_gpio_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN_BY_IDX(0, 1));

	return 0;
}

static struct rts_fp_gpio_data rts_fp_gpio_data_0;

static const struct rts_fp_gpio_config rts_fp_gpio_config_0 = {
	.common = {.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(0)},
	.base = DT_INST_REG_ADDR_BY_IDX(0, 0),
	.sp_base0 = DT_INST_REG_ADDR_BY_IDX(0, 1),
	.sp_base1 = DT_INST_REG_ADDR_BY_IDX(0, 2),
	.ngpios = DT_INST_PROP(0, ngpios),
};

DEVICE_DT_DEFINE(DT_DRV_INST(0), rts_fp_gpio_init, NULL, &rts_fp_gpio_data_0, &rts_fp_gpio_config_0,
		 POST_KERNEL, CONFIG_GPIO_INIT_PRIORITY, &rts_fp_gpio_api);
