/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcm_gpio

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_npcm, CONFIG_GPIO_LOG_LEVEL);

/*
 * Byte offsets of the port registers. Bit n represents pin n in every
 * register. Series with a different layout provide another initializer
 * selected by compatible in NPCM_GPIO_REGS().
 */
struct npcm_gpio_regs {
	uint8_t pdout;
	uint8_t pdin;
	uint8_t pdir;
	uint8_t ppull;  /* pull resistor enable */
	uint8_t ppud;   /* 0 pull-up, 1 pull-down */
	uint8_t potype; /* 0 push-pull, 1 open-drain */
};

#define NPCM_GPIO_REGS(n)                                                                          \
	{                                                                                          \
		.pdout = 0x00,                                                                     \
		.pdin = 0x01,                                                                      \
		.pdir = 0x02,                                                                      \
		.ppull = 0x03,                                                                     \
		.ppud = 0x04,                                                                      \
		.potype = 0x06,                                                                    \
	}

/* DEVALTCX bit enabling pull resistors on open-drain outputs. */
#define NPCM_SCFG_DEVALTCX            0x24
#define NPCM_DEVALTCX_GPIO_OUT_PULLEN BIT(7)

#define NPCM_GPIO_HAS_PINCTRL DT_ANY_INST_HAS_PROP_STATUS_OKAY(pinctrl_0)

struct gpio_npcm_config {
	struct gpio_driver_config common;
	mm_reg_t base; /* port registers */
	mm_reg_t scfg_base;
	struct npcm_gpio_regs regs;
#if NPCM_GPIO_HAS_PINCTRL
	const struct pinctrl_dev_config *pcfg;
#endif
};

struct gpio_npcm_data {
	struct gpio_driver_data common;
	struct k_spinlock lock;
};

static void npcm_reg_update(mm_reg_t addr, uint8_t mask, bool set)
{
	uint8_t val = sys_read8(addr);

	sys_write8(set ? (val | mask) : (val & ~mask), addr);
}

static int gpio_npcm_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_npcm_config *cfg = dev->config;
	const struct npcm_gpio_regs *r = &cfg->regs;
	struct gpio_npcm_data *data = dev->data;
	const uint8_t mask = BIT(pin);
	k_spinlock_key_t key;

	/* NPCM does not support open-source outputs. */
	if ((flags & GPIO_SINGLE_ENDED) != 0 && (flags & GPIO_LINE_OPEN_DRAIN) == 0) {
		return -ENOTSUP;
	}
	if ((flags & GPIO_PULL_UP) != 0 && (flags & GPIO_PULL_DOWN) != 0) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	/* Select input before changing the output state to prevent a stale level. */
	if ((flags & GPIO_OUTPUT) == 0) {
		npcm_reg_update(cfg->base + r->pdir, mask, false);
	}

	npcm_reg_update(cfg->base + r->potype, mask, (flags & GPIO_OPEN_DRAIN) == GPIO_OPEN_DRAIN);

	/* Output pull resistors are effective only in open-drain mode. */
	if ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0) {
		npcm_reg_update(cfg->base + r->ppud, mask, (flags & GPIO_PULL_DOWN) != 0);
		npcm_reg_update(cfg->base + r->ppull, mask, true);
	} else {
		npcm_reg_update(cfg->base + r->ppull, mask, false);
	}

	if ((flags & GPIO_OUTPUT) != 0) {
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			npcm_reg_update(cfg->base + r->pdout, mask, true);
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			npcm_reg_update(cfg->base + r->pdout, mask, false);
		} else {
			/* No initial level requested; keep the current PDOUT value. */
		}
		npcm_reg_update(cfg->base + r->pdir, mask, true);
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

#ifdef CONFIG_GPIO_GET_CONFIG
static int gpio_npcm_pin_get_config(const struct device *dev, gpio_pin_t pin,
				    gpio_flags_t *out_flags)
{
	const struct gpio_npcm_config *cfg = dev->config;
	const struct npcm_gpio_regs *r = &cfg->regs;
	const uint8_t mask = BIT(pin);
	gpio_flags_t flags;

	if ((sys_read8(cfg->base + r->pdir) & mask) != 0) {
		flags = GPIO_OUTPUT;
		flags |= (sys_read8(cfg->base + r->pdout) & mask) != 0 ? GPIO_OUTPUT_INIT_HIGH
								       : GPIO_OUTPUT_INIT_LOW;
		if ((sys_read8(cfg->base + r->potype) & mask) != 0) {
			flags |= GPIO_OPEN_DRAIN;
		}
	} else {
		flags = GPIO_INPUT;
	}

	if ((sys_read8(cfg->base + r->ppull) & mask) != 0) {
		flags |= (sys_read8(cfg->base + r->ppud) & mask) != 0 ? GPIO_PULL_DOWN
								      : GPIO_PULL_UP;
	}

	*out_flags = flags;

	return 0;
}
#endif /* CONFIG_GPIO_GET_CONFIG */

static int gpio_npcm_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct gpio_npcm_config *cfg = dev->config;

	*value = sys_read8(cfg->base + cfg->regs.pdin);

	return 0;
}

static int gpio_npcm_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					 gpio_port_value_t value)
{
	const struct gpio_npcm_config *cfg = dev->config;
	const mm_reg_t pdout = cfg->base + cfg->regs.pdout;
	struct gpio_npcm_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	uint8_t out = sys_read8(pdout);

	sys_write8((out & ~mask) | (value & mask), pdout);
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int gpio_npcm_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_npcm_port_set_masked_raw(dev, pins, pins);
}

static int gpio_npcm_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_npcm_port_set_masked_raw(dev, pins, 0);
}

static int gpio_npcm_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_npcm_config *cfg = dev->config;
	const mm_reg_t pdout = cfg->base + cfg->regs.pdout;
	struct gpio_npcm_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	sys_write8(sys_read8(pdout) ^ pins, pdout);
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int gpio_npcm_init(const struct device *dev)
{
	const struct gpio_npcm_config *cfg = dev->config;

#if NPCM_GPIO_HAS_PINCTRL
	if (cfg->pcfg != NULL) {
		int ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);

		if (ret < 0) {
			return ret;
		}
	}
#endif

	/*
	 * Enable output pulls globally. The PxPULL register still controls each
	 * pin independently.
	 */
	npcm_reg_update(cfg->scfg_base + NPCM_SCFG_DEVALTCX, NPCM_DEVALTCX_GPIO_OUT_PULLEN, true);

	return 0;
}

static DEVICE_API(gpio, gpio_npcm_api) = {
	.pin_configure = gpio_npcm_pin_configure,
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = gpio_npcm_pin_get_config,
#endif
	.port_get_raw = gpio_npcm_port_get_raw,
	.port_set_masked_raw = gpio_npcm_port_set_masked_raw,
	.port_set_bits_raw = gpio_npcm_port_set_bits_raw,
	.port_clear_bits_raw = gpio_npcm_port_clear_bits_raw,
	.port_toggle_bits = gpio_npcm_port_toggle_bits,
};

#define GPIO_NPCM_PINCTRL_DEFINE(n)                                                                \
	IF_ENABLED(DT_INST_PINCTRL_HAS_NAME(n, default), (PINCTRL_DT_INST_DEFINE(n);))
#define GPIO_NPCM_PINCTRL_INIT(n)                                                                  \
	IF_ENABLED(DT_INST_PINCTRL_HAS_NAME(n, default),                                           \
		   (.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),))

#define GPIO_NPCM_INIT(n)                                                                          \
	GPIO_NPCM_PINCTRL_DEFINE(n)                                                                \
                                                                                                   \
	static const struct gpio_npcm_config gpio_npcm_cfg_##n = {                                 \
		.common = {.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n)},                   \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.scfg_base = DT_REG_ADDR(DT_INST_PHANDLE(n, nuvoton_scfg)),                        \
		.regs = NPCM_GPIO_REGS(n),                                                         \
		GPIO_NPCM_PINCTRL_INIT(n)};                                                        \
                                                                                                   \
	static struct gpio_npcm_data gpio_npcm_data_##n;                                           \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, gpio_npcm_init, NULL, &gpio_npcm_data_##n, &gpio_npcm_cfg_##n,    \
			      PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY, &gpio_npcm_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_NPCM_INIT)
