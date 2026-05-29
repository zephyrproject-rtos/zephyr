/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT brcm_bcm2835_gpio

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>

#define GPIO_REG_GROUP(n, cnt)       (n / cnt)
#define GPIO_REG_SHIFT(n, cnt, bits) ((n % cnt) * bits)

#define GPFSEL(base, n)      (base + 0x00 + 0x04 * n)
#define GPSET(base, n)       (base + 0x1C + 0x04 * n)
#define GPCLR(base, n)       (base + 0x28 + 0x04 * n)
#define GPLEV(base, n)       (base + 0x34 + 0x04 * n)
#define GPEDS(base, n)       (base + 0x40 + 0x04 * n)
#define GPREN(base, n)       (base + 0x4C + 0x04 * n)
#define GPFEN(base, n)       (base + 0x58 + 0x04 * n)
#define GPHEN(base, n)       (base + 0x64 + 0x04 * n)
#define GPLEN(base, n)       (base + 0x70 + 0x04 * n)
#define GPAREN(base, n)      (base + 0x7C + 0x04 * n)
#define GPAFEN(base, n)      (base + 0x88 + 0x04 * n)
#define BCM2835_GPPUD(base)  (base + 0x94)
#define BCM2835_GPPUDCLK(base, n) (base + 0x98 + 0x04 * n)

#define FSEL_GROUPS (10)
#define FSEL_BITS   (3)
#define FSEL_OUTPUT (0x1)

#define IO_GROUPS (32)
#define IO_BITS   (1)

#define DEV_CFG(dev)  ((const struct gpio_bcm2835_config *const)(dev)->config)
#define DEV_DATA(dev) ((struct gpio_bcm2835_data *const)(dev)->data)

#define RPI_PIN_NUM(dev, n) (DEV_CFG(dev)->offset + n)

#define FROM_U64(val, idx) ((uint32_t)((val >> (idx * 32)) & UINT32_MAX))

struct gpio_bcm2835_config {
	struct gpio_driver_config common;

	DEVICE_MMIO_NAMED_ROM(reg_base);

	void (*irq_config_func)(void);

	uint8_t offset;
	uint8_t ngpios;
};

struct gpio_bcm2835_data {
	struct gpio_driver_data common;

	DEVICE_MMIO_NAMED_RAM(reg_base);
	mem_addr_t base;

	sys_slist_t cb;
};

static void gpio_bcm2835_set_pull(const struct device *port, gpio_pin_t pin, gpio_flags_t flags)
{
	struct gpio_bcm2835_data *data = DEV_DATA(port);
	uint32_t group;
	uint32_t shift;
	uint32_t pud = 0U;
	uint32_t pin_num = RPI_PIN_NUM(port, pin);

	if (flags & GPIO_PULL_UP) {
		pud = 0x2U;
	} else if (flags & GPIO_PULL_DOWN) {
		pud = 0x1U;
	} else {
		pud = 0U;
	}

	group = GPIO_REG_GROUP(pin_num, IO_GROUPS);
	shift = GPIO_REG_SHIFT(pin_num, IO_GROUPS, IO_BITS);

	sys_write32(pud, BCM2835_GPPUD(data->base));
	k_busy_wait(1);
	sys_write32(BIT(shift), BCM2835_GPPUDCLK(data->base, group));
	k_busy_wait(1);
	sys_write32(0U, BCM2835_GPPUD(data->base));
	sys_write32(0U, BCM2835_GPPUDCLK(data->base, group));
}

static int gpio_bcm2835_pin_configure(const struct device *port, gpio_pin_t pin, gpio_flags_t flags)
{
	struct gpio_bcm2835_data *data = DEV_DATA(port);
	uint32_t group;
	uint32_t shift;
	uint32_t regval;

	if (flags & GPIO_OPEN_DRAIN) {
		return -ENOTSUP;
	}

	group = GPIO_REG_GROUP(RPI_PIN_NUM(port, pin), FSEL_GROUPS);
	shift = GPIO_REG_SHIFT(RPI_PIN_NUM(port, pin), FSEL_GROUPS, FSEL_BITS);

	regval = sys_read32(GPFSEL(data->base, group));
	regval &= ~(BIT_MASK(FSEL_BITS) << shift);
	if (flags & GPIO_OUTPUT) {
		regval |= (FSEL_OUTPUT << shift);
	}
	sys_write32(regval, GPFSEL(data->base, group));

	if (flags & GPIO_OUTPUT) {
		group = GPIO_REG_GROUP(RPI_PIN_NUM(port, pin), IO_GROUPS);
		shift = GPIO_REG_SHIFT(RPI_PIN_NUM(port, pin), IO_GROUPS, IO_BITS);

		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			sys_write32(BIT(shift), GPSET(data->base, group));
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			sys_write32(BIT(shift), GPCLR(data->base, group));
		} else {
			/* Leave the current output value unchanged. */
		}
	}

	gpio_bcm2835_set_pull(port, pin, flags);

	return 0;
}

static int gpio_bcm2835_port_get_raw(const struct device *port, gpio_port_value_t *value)
{
	const struct gpio_bcm2835_config *cfg = DEV_CFG(port);
	struct gpio_bcm2835_data *data = DEV_DATA(port);
	uint64_t regval;

	regval = ((uint64_t)sys_read32(GPLEV(data->base, 0))) |
		 ((uint64_t)sys_read32(GPLEV(data->base, 1)) << 32);

	*value = (regval >> cfg->offset) & BIT_MASK(cfg->ngpios);

	return 0;
}

static int gpio_bcm2835_port_set_masked_raw(const struct device *port, gpio_port_pins_t mask,
					    gpio_port_value_t value)
{
	const struct gpio_bcm2835_config *cfg = DEV_CFG(port);
	struct gpio_bcm2835_data *data = DEV_DATA(port);
	uint64_t regval, regmask;
	uint64_t set, clr;

	value &= BIT_MASK(cfg->ngpios);
	mask &= BIT_MASK(cfg->ngpios);

	regval = (uint64_t)value << cfg->offset;
	regmask = (uint64_t)mask << cfg->offset;

	set = regval & regmask;
	clr = regval ^ regmask;

	sys_write32(FROM_U64(set, 0), GPSET(data->base, 0));
	sys_write32(FROM_U64(clr, 0), GPCLR(data->base, 0));
	sys_write32(FROM_U64(set, 1), GPSET(data->base, 1));
	sys_write32(FROM_U64(clr, 1), GPCLR(data->base, 1));

	return 0;
}

static int gpio_bcm2835_port_set_bits_raw(const struct device *port, gpio_port_pins_t pins)
{
	const struct gpio_bcm2835_config *cfg = DEV_CFG(port);
	struct gpio_bcm2835_data *data = DEV_DATA(port);
	uint64_t regval;

	regval = ((uint64_t)pins & BIT_MASK(cfg->ngpios)) << cfg->offset;

	sys_write32(FROM_U64(regval, 0), GPSET(data->base, 0));
	sys_write32(FROM_U64(regval, 1), GPSET(data->base, 1));

	return 0;
}

static int gpio_bcm2835_port_clear_bits_raw(const struct device *port, gpio_port_pins_t pins)
{
	const struct gpio_bcm2835_config *cfg = DEV_CFG(port);
	struct gpio_bcm2835_data *data = DEV_DATA(port);
	uint64_t regval;

	regval = ((uint64_t)pins & BIT_MASK(cfg->ngpios)) << cfg->offset;

	sys_write32(FROM_U64(regval, 0), GPCLR(data->base, 0));
	sys_write32(FROM_U64(regval, 1), GPCLR(data->base, 1));

	return 0;
}

static int gpio_bcm2835_port_toggle_bits(const struct device *port, gpio_port_pins_t pins)
{
	const struct gpio_bcm2835_config *cfg = DEV_CFG(port);
	struct gpio_bcm2835_data *data = DEV_DATA(port);
	uint64_t regval, regmask;
	uint64_t set, clr;

	regval = ((uint64_t)sys_read32(GPLEV(data->base, 0))) |
		 ((uint64_t)sys_read32(GPLEV(data->base, 1)) << 32);

	regmask = ((uint64_t)pins & BIT_MASK(cfg->ngpios)) << cfg->offset;

	set = regval ^ regmask;
	clr = regval & regmask;

	sys_write32(FROM_U64(set, 0), GPSET(data->base, 0));
	sys_write32(FROM_U64(clr, 0), GPCLR(data->base, 0));
	sys_write32(FROM_U64(set, 1), GPSET(data->base, 1));
	sys_write32(FROM_U64(clr, 1), GPCLR(data->base, 1));

	return 0;
}

static int gpio_bcm2835_pin_interrupt_configure(const struct device *port, gpio_pin_t pin,
						enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	struct gpio_bcm2835_data *data = DEV_DATA(port);
	uint32_t group;
	uint32_t shift;
	uint32_t regval;

	group = GPIO_REG_GROUP(RPI_PIN_NUM(port, pin), IO_GROUPS);
	shift = GPIO_REG_SHIFT(RPI_PIN_NUM(port, pin), IO_GROUPS, IO_BITS);

	regval = sys_read32(GPREN(data->base, group));
	regval &= ~BIT(shift);
	sys_write32(regval, GPREN(data->base, group));

	regval = sys_read32(GPFEN(data->base, group));
	regval &= ~BIT(shift);
	sys_write32(regval, GPFEN(data->base, group));

	regval = sys_read32(GPHEN(data->base, group));
	regval &= ~BIT(shift);
	sys_write32(regval, GPHEN(data->base, group));

	regval = sys_read32(GPLEN(data->base, group));
	regval &= ~BIT(shift);
	sys_write32(regval, GPLEN(data->base, group));

	regval = sys_read32(GPAREN(data->base, group));
	regval &= ~BIT(shift);
	sys_write32(regval, GPAREN(data->base, group));

	regval = sys_read32(GPAFEN(data->base, group));
	regval &= ~BIT(shift);
	sys_write32(regval, GPAFEN(data->base, group));

	if (mode == GPIO_INT_MODE_LEVEL) {
		if (trig & GPIO_INT_LOW_0) {
			regval = sys_read32(GPLEN(data->base, group));
			regval |= BIT(shift);
			sys_write32(regval, GPLEN(data->base, group));
		}
		if (trig & GPIO_INT_HIGH_1) {
			regval = sys_read32(GPHEN(data->base, group));
			regval |= BIT(shift);
			sys_write32(regval, GPHEN(data->base, group));
		}
	} else if (mode == GPIO_INT_MODE_EDGE) {
		if (trig & GPIO_INT_LOW_0) {
			regval = sys_read32(GPAFEN(data->base, group));
			regval |= BIT(shift);
			sys_write32(regval, GPAFEN(data->base, group));
		}
		if (trig & GPIO_INT_HIGH_1) {
			regval = sys_read32(GPAREN(data->base, group));
			regval |= BIT(shift);
			sys_write32(regval, GPAREN(data->base, group));
		}
	} else {
		/* GPIO_INT_MODE_DISABLED: interrupt enables were cleared above. */
	}

	return 0;
}

static int gpio_bcm2835_manage_callback(const struct device *port, struct gpio_callback *cb,
					bool set)
{
	struct gpio_bcm2835_data *data = DEV_DATA(port);

	return gpio_manage_callback(&data->cb, cb, set);
}

static void gpio_bcm2835_isr(const struct device *port)
{
	const struct gpio_bcm2835_config *cfg = DEV_CFG(port);
	struct gpio_bcm2835_data *data = DEV_DATA(port);
	uint64_t regval;
	uint32_t pins;

	regval = ((uint64_t)sys_read32(GPEDS(data->base, 0))) |
		 ((uint64_t)sys_read32(GPEDS(data->base, 1)) << 32);

	regval &= BIT_MASK(cfg->ngpios) << cfg->offset;

	pins = (uint32_t)(regval >> cfg->offset);
	gpio_fire_callbacks(&data->cb, port, pins);

	sys_write32(FROM_U64(regval, 0), GPEDS(data->base, 0));
	sys_write32(FROM_U64(regval, 1), GPEDS(data->base, 1));
}

static int gpio_bcm2835_init(const struct device *port)
{
	const struct gpio_bcm2835_config *cfg = DEV_CFG(port);
	struct gpio_bcm2835_data *data = DEV_DATA(port);

	DEVICE_MMIO_NAMED_MAP(port, reg_base, K_MEM_CACHE_NONE);
	data->base = DEVICE_MMIO_NAMED_GET(port, reg_base);

	cfg->irq_config_func();

	return 0;
}

static DEVICE_API(gpio, gpio_bcm2835_api) = {
	.pin_configure = gpio_bcm2835_pin_configure,
	.port_get_raw = gpio_bcm2835_port_get_raw,
	.port_set_masked_raw = gpio_bcm2835_port_set_masked_raw,
	.port_set_bits_raw = gpio_bcm2835_port_set_bits_raw,
	.port_clear_bits_raw = gpio_bcm2835_port_clear_bits_raw,
	.port_toggle_bits = gpio_bcm2835_port_toggle_bits,
	.pin_interrupt_configure = gpio_bcm2835_pin_interrupt_configure,
	.manage_callback = gpio_bcm2835_manage_callback,
};

#define GPIO_BCM2835_IRQ_PRIO(n)                                                                  \
	COND_CODE_1(DT_INST_IRQ_HAS_CELL(n, priority), (DT_INST_IRQ(n, priority)), (0))

#define GPIO_BCM2835_INST(n)                                                                       \
	static struct gpio_bcm2835_data gpio_bcm2835_data_##n;                                   \
	                                                                                           \
	static void gpio_bcm2835_irq_config_func_##n(void)                                       \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), GPIO_BCM2835_IRQ_PRIO(n),                            \
			    gpio_bcm2835_isr,                                                   \
			    DEVICE_DT_INST_GET(n), 0);                                         \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	static const struct gpio_bcm2835_config gpio_bcm2835_cfg_##n = {                         \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(n),                                    \
		DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_INST_PARENT(n)),                         \
		.irq_config_func = gpio_bcm2835_irq_config_func_##n,                             \
		.offset = DT_INST_REG_ADDR(n),                                                   \
		.ngpios = DT_INST_PROP(n, ngpios),                                               \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, gpio_bcm2835_init, NULL, &gpio_bcm2835_data_##n,                \
			      &gpio_bcm2835_cfg_##n, PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,    \
			      &gpio_bcm2835_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_BCM2835_INST)
