/*
 * Copyright (c) 2024 Junho Lee <junho@tsnlab.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raspberrypi_rp1_gpio

#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>

#define GPIO_STATUS(base, n) (base + 0x8 * n)
#define GPIO_CTRL(base, n)   (GPIO_STATUS(base, n) + 0x4)

#define GPIO_STATUS_OUT_TO_PAD    0x200
#define GPIO_STATUS_OUT_FROM_PERI 0x100

#define GPIO_CTRL_OUTOVER_MASK 0x3000
#define GPIO_CTRL_OUTOVER_PERI 0x0

#define GPIO_CTRL_OEOVER_MASK 0xc000
#define GPIO_CTRL_OEOVER_PERI 0x0

#define GPIO_CTRL_FUNCSEL_MASK 0x001f
#define GPIO_CTRL_FUNCSEL_RIO  0x5

#define RIO_OUT(base) (base)
#define RIO_OE(base)  (base + 0x4)
#define RIO_IN(base)  (base + 0x8)

#define RIO_SET 0x2000
#define RIO_CLR 0x3000

#define RIO_OUT_SET(base) (RIO_OUT(base) + RIO_SET)
#define RIO_OUT_CLR(base) (RIO_OUT(base) + RIO_CLR)

#define RIO_OE_SET(base) (RIO_OE(base) + RIO_SET)
#define RIO_OE_CLR(base) (RIO_OE(base) + RIO_CLR)

#define PADS_CTRL(base, n) (base + 0x4 * (n + 1))

#define PADS_OUTPUT_DISABLE 0x80
#define PADS_INPUT_ENABLE   0x40

#define PADS_PULL_UP_ENABLE   0x8
#define PADS_PULL_DOWN_ENABLE 0x4

#define DEV_CFG(dev)  ((const struct gpio_rp1_config *)(dev)->config)
#define DEV_DATA(dev) ((struct gpio_rp1_data *)(dev)->data)

struct gpio_rp1_config {
	struct gpio_driver_config common;

	DEVICE_MMIO_NAMED_ROM(reg_base);
	mem_addr_t gpio_offset;
	mem_addr_t rio_offset;
	mem_addr_t pads_offset;

	uint8_t ngpios;
};

struct gpio_rp1_data {
	struct gpio_driver_data common;

	DEVICE_MMIO_NAMED_RAM(reg_base);
	mem_addr_t gpio_base;
	mem_addr_t rio_base;
	mem_addr_t pads_base;
};

static int gpio_rp1_pin_configure(const struct device *port, gpio_pin_t pin, gpio_flags_t flags)
{
	struct gpio_rp1_data *data = port->data;

	if (flags & GPIO_SINGLE_ENDED) {
		return -ENOTSUP;
	}

	/* Let RIO handle the input/output of GPIO */
	sys_clear_bits(GPIO_CTRL(data->gpio_base, pin), GPIO_CTRL_OEOVER_MASK);
	sys_set_bits(GPIO_CTRL(data->gpio_base, pin), GPIO_CTRL_OEOVER_PERI);

	sys_clear_bits(GPIO_CTRL(data->gpio_base, pin), GPIO_CTRL_OUTOVER_MASK);
	sys_set_bits(GPIO_CTRL(data->gpio_base, pin), GPIO_CTRL_OUTOVER_PERI);

	sys_clear_bits(GPIO_CTRL(data->gpio_base, pin), GPIO_CTRL_FUNCSEL_MASK);
	sys_set_bits(GPIO_CTRL(data->gpio_base, pin), GPIO_CTRL_FUNCSEL_RIO);

	/* Set the direction */
	if (flags & GPIO_OUTPUT) {
		sys_set_bit(RIO_OE_SET(data->rio_base), pin);
		sys_clear_bits(PADS_CTRL(data->pads_base, pin),
			       PADS_OUTPUT_DISABLE | PADS_INPUT_ENABLE);

		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			sys_set_bit(RIO_OUT_SET(data->rio_base), pin);
			sys_clear_bit(RIO_OUT_CLR(data->rio_base), pin);
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			sys_set_bit(RIO_OUT_CLR(data->rio_base), pin);
			sys_clear_bit(RIO_OUT_SET(data->rio_base), pin);
		}
	} else if (flags & GPIO_INPUT) {
		sys_set_bit(RIO_OE_CLR(data->rio_base), pin);
		sys_set_bits(PADS_CTRL(data->pads_base, pin),
			     PADS_OUTPUT_DISABLE | PADS_INPUT_ENABLE);
	}

	/* Set pull up/down */
	sys_clear_bits(PADS_CTRL(data->pads_base, pin),
		       PADS_PULL_UP_ENABLE | PADS_PULL_DOWN_ENABLE);

	if (flags & GPIO_PULL_UP) {
		sys_set_bits(PADS_CTRL(data->pads_base, pin), PADS_PULL_UP_ENABLE);
	} else if (flags & GPIO_PULL_DOWN) {
		sys_set_bits(PADS_CTRL(data->pads_base, pin), PADS_PULL_DOWN_ENABLE);
	}

	return 0;
}

static int gpio_rp1_port_get_raw(const struct device *port, gpio_port_value_t *value)
{
	struct gpio_rp1_data *data = port->data;

	*value = sys_read32(RIO_IN(data->rio_base));

	return 0;
}

static int gpio_rp1_port_set_masked_raw(const struct device *port, gpio_port_pins_t mask,
					gpio_port_value_t value)
{
	struct gpio_rp1_data *data = port->data;

	sys_clear_bits(RIO_OUT_SET(data->rio_base), mask);
	sys_set_bits(RIO_OUT_CLR(data->rio_base), mask);

	sys_clear_bits(RIO_OUT_CLR(data->rio_base), (value & mask));
	sys_set_bits(RIO_OUT_SET(data->rio_base), (value & mask));

	return 0;
}

static int gpio_rp1_port_set_bits_raw(const struct device *port, gpio_port_pins_t pins)
{
	struct gpio_rp1_data *data = port->data;

	sys_clear_bits(RIO_OUT_CLR(data->rio_base), pins);
	sys_set_bits(RIO_OUT_SET(data->rio_base), pins);

	return 0;
}

static int gpio_rp1_port_clear_bits_raw(const struct device *port, gpio_port_pins_t pins)
{
	struct gpio_rp1_data *data = port->data;

	sys_clear_bits(RIO_OUT_SET(data->rio_base), pins);
	sys_set_bits(RIO_OUT_CLR(data->rio_base), pins);

	return 0;
}

static int gpio_rp1_port_toggle_bits(const struct device *port, gpio_port_pins_t pins)
{
	struct gpio_rp1_data *data = port->data;
	uint32_t val;

	val = sys_read32(RIO_OUT(data->rio_base));

	/* Low to high */
	sys_set_bits(RIO_OUT_SET(data->rio_base), val ^ pins);
	sys_clear_bits(RIO_OUT_CLR(data->rio_base), val ^ pins);

	/* High to low */
	sys_set_bits(RIO_OUT_CLR(data->rio_base), val & pins);
	sys_clear_bits(RIO_OUT_SET(data->rio_base), val & pins);

	return 0;
}

static DEVICE_API(gpio, gpio_rp1_api) = {
	.pin_configure = gpio_rp1_pin_configure,
	.port_get_raw = gpio_rp1_port_get_raw,
	.port_set_masked_raw = gpio_rp1_port_set_masked_raw,
	.port_set_bits_raw = gpio_rp1_port_set_bits_raw,
	.port_clear_bits_raw = gpio_rp1_port_clear_bits_raw,
	.port_toggle_bits = gpio_rp1_port_toggle_bits,
};

static int gpio_rp1_init(const struct device *port)
{
	const struct gpio_rp1_config *config = port->config;
	struct gpio_rp1_data *data = port->data;

	DEVICE_MMIO_NAMED_MAP(port, reg_base, K_MEM_CACHE_NONE);
	data->gpio_base = DEVICE_MMIO_NAMED_GET(port, reg_base) + config->gpio_offset;
	data->rio_base = DEVICE_MMIO_NAMED_GET(port, reg_base) + config->rio_offset;
	data->pads_base = DEVICE_MMIO_NAMED_GET(port, reg_base) + config->pads_offset;

	return 0;
}

#define GPIO_RP1_INIT(n)                                                                           \
	static struct gpio_rp1_data gpio_rp1_data_##n;                                             \
                                                                                                   \
	static const struct gpio_rp1_config gpio_rp1_cfg_##n = {                                   \
		.common = {.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(0)},                   \
		DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_INST_PARENT(n)),                           \
		.gpio_offset = DT_INST_REG_ADDR_BY_IDX(n, 0),                                      \
		.rio_offset = DT_INST_REG_ADDR_BY_IDX(n, 1),                                       \
		.pads_offset = DT_INST_REG_ADDR_BY_IDX(n, 2),                                      \
		.ngpios = DT_INST_PROP(n, ngpios),                                                 \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, gpio_rp1_init, NULL, &gpio_rp1_data_##n, &gpio_rp1_cfg_##n,       \
			      POST_KERNEL, CONFIG_GPIO_INIT_PRIORITY, &gpio_rp1_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_RP1_INIT)
