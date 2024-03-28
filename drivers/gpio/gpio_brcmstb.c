/*
 * Copyright (c) 2024 Junho Lee <junho@tsnlab.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT brcm_brcmstb_gpio

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/irq.h>

#define GIO_ODEN  0x00
#define GIO_DATA  0x04
#define GIO_IODIR 0x08
#define GIO_EC    0x0c
#define GIO_EI    0x10
#define GIO_MASK  0x14
#define GIO_LEVEL 0x18
#define GIO_STAT  0x1c

#define SET_BIT(reg, pin) reg |= (uint32_t)(1 << pin);
#define SET_BIT_RW(reg, addr, pin)                                                                 \
	do {                                                                                       \
		reg = sys_read32(addr);                                                            \
		SET_BIT(reg, pin);                                                                 \
		sys_write32(reg, addr);                                                            \
	} while (0)
#define CLR_BIT(reg, pin) reg &= ~((uint32_t)(1 << pin));
#define CLR_BIT_RW(reg, addr, pin)                                                                 \
	do {                                                                                       \
		reg = sys_read32(addr);                                                            \
		CLR_BIT(reg, pin);                                                                 \
		sys_write32(reg, addr);                                                            \
	} while (0)

#define DEV_CFG(dev)  ((const struct gpio_brcmstb_config *)(dev)->config)
#define DEV_DATA(dev) ((struct gpio_brcmstb_data *)(dev)->data)

struct gpio_brcmstb_config {
	struct gpio_driver_config common;

	DEVICE_MMIO_NAMED_ROM(reg_base);
	mem_addr_t offset;
};

struct gpio_brcmstb_data {
	struct gpio_driver_data common;

	DEVICE_MMIO_NAMED_RAM(reg_base);
	mem_addr_t base;

	sys_slist_t cb;
};

static int gpio_brcmstb_pin_configure(const struct device *port, gpio_pin_t pin, gpio_flags_t flags)
{
	struct gpio_brcmstb_data *data = DEV_DATA(port);
	uint32_t reg_data = 0;
	uint32_t reg_iodir = 0;

	if (flags & (GPIO_SINGLE_ENDED | GPIO_PULL_UP | GPIO_PULL_DOWN)) {
		return -ENOTSUP;
	}

	reg_data = sys_read32(data->base + GIO_DATA);
	reg_iodir = sys_read32(data->base + GIO_IODIR);

	if (flags & GPIO_INPUT) {
		SET_BIT(reg_iodir, pin);
	} else if (flags & GPIO_OUTPUT) {
		CLR_BIT(reg_iodir, pin);

		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			SET_BIT(reg_data, pin);
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			CLR_BIT(reg_data, pin);
		}
	}
	sys_write32(reg_data, data->base + GIO_DATA);
	sys_write32(reg_iodir, data->base + GIO_IODIR);

	return 0;
}

static int gpio_brcmstb_port_get_raw(const struct device *port, gpio_port_value_t *value)
{
	struct gpio_brcmstb_data *data = DEV_DATA(port);

	*value = sys_read32(data->base + GIO_DATA);

	return 0;
}

static int gpio_brcmstb_port_set_masked_raw(const struct device *port, gpio_port_pins_t mask,
					    gpio_port_value_t value)
{
	struct gpio_brcmstb_data *data = DEV_DATA(port);
	uint32_t reg_data = 0;

	reg_data = sys_read32(data->base + GIO_DATA);
	reg_data &= ~mask;
	reg_data |= (value & mask);
	sys_write32(reg_data, data->base + GIO_DATA);

	return 0;
}

static int gpio_brcmstb_port_set_bits_raw(const struct device *port, gpio_port_pins_t pins)
{
	struct gpio_brcmstb_data *data = DEV_DATA(port);
	uint32_t reg_data = 0;

	reg_data = sys_read32(data->base + GIO_DATA);
	reg_data |= pins;
	sys_write32(reg_data, data->base + GIO_DATA);

	return 0;
}

static int gpio_brcmstb_port_clear_bits_raw(const struct device *port, gpio_port_pins_t pins)
{
	struct gpio_brcmstb_data *data = DEV_DATA(port);
	uint32_t reg_data = 0;

	reg_data = sys_read32(data->base + GIO_DATA);
	reg_data &= ~pins;
	sys_write32(reg_data, data->base + GIO_DATA);

	return 0;
}

static int gpio_brcmstb_port_toggle_bits(const struct device *port, gpio_port_pins_t pins)
{
	struct gpio_brcmstb_data *data = DEV_DATA(port);
	uint32_t reg_data = 0;

	reg_data = sys_read32(data->base + GIO_DATA);
	reg_data ^= pins;
	sys_write32(reg_data, data->base + GIO_DATA);

	return 0;
}

static const struct gpio_driver_api gpio_brcmstb_api = {
	.pin_configure = gpio_brcmstb_pin_configure,
	.port_get_raw = gpio_brcmstb_port_get_raw,
	.port_set_masked_raw = gpio_brcmstb_port_set_masked_raw,
	.port_set_bits_raw = gpio_brcmstb_port_set_bits_raw,
	.port_clear_bits_raw = gpio_brcmstb_port_clear_bits_raw,
	.port_toggle_bits = gpio_brcmstb_port_toggle_bits,
};

int gpio_brcmstb_init(const struct device *port)
{
	const struct gpio_brcmstb_config *config = DEV_CFG(port);
	struct gpio_brcmstb_data *data = DEV_DATA(port);

	DEVICE_MMIO_NAMED_MAP(port, reg_base, K_MEM_CACHE_NONE);
	data->base = DEVICE_MMIO_NAMED_GET(port, reg_base) + config->offset;

	return 0;
}

#define GPIO_BRCMSTB_INIT(n)                                                                       \
	static struct gpio_brcmstb_data gpio_brcmstb_data_##n;                                     \
                                                                                                   \
	static const struct gpio_brcmstb_config gpio_brcmstb_cfg_##n = {                           \
		.common = {.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(0)},                   \
		DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_INST_PARENT(n)),                           \
		.offset = DT_INST_REG_ADDR(n),                                                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, gpio_brcmstb_init, NULL, &gpio_brcmstb_data_##n,                  \
			      &gpio_brcmstb_cfg_##n, PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,      \
			      &gpio_brcmstb_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_BRCMSTB_INIT)
