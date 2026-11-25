/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/ztest.h>

#include "gpio_rockchip.h"

#define FAKE_GPIO_SIZE 0x100U

static uint32_t fake_regs[FAKE_GPIO_SIZE / sizeof(uint32_t)];
static struct gpio_rockchip_config fake_config;
static struct gpio_rockchip_data fake_data;
static struct device_state fake_state;
static struct device fake_dev = {
	.name = "gpio_rockchip_fake",
	.config = &fake_config,
	.data = &fake_data,
	.api = &gpio_rockchip_api,
	.state = &fake_state,
};

static uint32_t cb1_pins;
static uint32_t cb2_pins;
static uint32_t cb1_count;
static uint32_t cb2_count;

static void fake_irq_config(const struct device *port)
{
	ARG_UNUSED(port);
}

static uint32_t reg_read(uint32_t offset)
{
	return fake_regs[offset / sizeof(uint32_t)];
}

static void reg_write(uint32_t offset, uint32_t value)
{
	fake_regs[offset / sizeof(uint32_t)] = value;
}

static void gpio_cb1(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins)
{
	ARG_UNUSED(cb);

	zassert_equal_ptr(port, &fake_dev);
	cb1_pins = pins;
	cb1_count++;
}

static void gpio_cb2(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins)
{
	ARG_UNUSED(cb);

	zassert_equal_ptr(port, &fake_dev);
	cb2_pins = pins;
	cb2_count++;
}

static void gpio_rockchip_before(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(fake_regs, 0, sizeof(fake_regs));
	memset(&fake_config, 0, sizeof(fake_config));
	memset(&fake_data, 0, sizeof(fake_data));
	memset(&fake_state, 0, sizeof(fake_state));
	fake_state.initialized = true;
	fake_config.common.port_pin_mask = 0x7fffffffU; /* Leave pin 31 reserved for mask tests. */
	fake_config.irq_config_func = fake_irq_config;
#ifdef DEVICE_MMIO_IS_IN_RAM
	fake_data.reg_base = (mm_reg_t)fake_regs;
#else
	fake_config.reg_base.addr = (mm_reg_t)fake_regs;
#endif
	cb1_pins = 0U;
	cb2_pins = 0U;
	cb1_count = 0U;
	cb2_count = 0U;
}

ZTEST(gpio_rockchip, test_pin_configure_output_and_input)
{
	zassert_ok(gpio_pin_configure(&fake_dev, 8U, GPIO_OUTPUT_HIGH));
	zassert_equal(reg_read(RK_GPIO_SWPORT_DR_L), 0x01000100U);
	zassert_equal(reg_read(RK_GPIO_SWPORT_DDR_L), 0x01000100U);

	zassert_ok(gpio_pin_configure(&fake_dev, 20U, GPIO_OUTPUT_LOW));
	zassert_equal(reg_read(RK_GPIO_SWPORT_DR_H), 0x00100000U);
	zassert_equal(reg_read(RK_GPIO_SWPORT_DDR_H), 0x00100010U);

	zassert_ok(gpio_pin_configure(&fake_dev, 8U, GPIO_INPUT));
	zassert_equal(reg_read(RK_GPIO_SWPORT_DDR_L), 0x01000000U);

	zassert_ok(gpio_pin_configure(&fake_dev, 20U, GPIO_DISCONNECTED));
	zassert_equal(reg_read(RK_GPIO_SWPORT_DDR_H), 0x00100000U);

	/* Unsupported electrical or simultaneous direction flags. */
	zassert_equal(gpio_pin_configure(&fake_dev, 8U, GPIO_INPUT | GPIO_PULL_UP), -ENOTSUP);
	zassert_equal(gpio_pin_configure(&fake_dev, 8U, GPIO_INPUT | GPIO_PULL_DOWN), -ENOTSUP);
	zassert_equal(gpio_pin_configure(&fake_dev, 8U, GPIO_OUTPUT | GPIO_OPEN_DRAIN), -ENOTSUP);
	zassert_equal(gpio_pin_configure(&fake_dev, 8U, GPIO_INPUT | GPIO_OUTPUT), -ENOTSUP);

	/* Reserved or out-of-range pins (bypassing gpio.h __ASSERT wrapper). */
	zassert_equal(gpio_rockchip_api.pin_configure(&fake_dev, 31U, GPIO_INPUT), -EINVAL);
	zassert_equal(gpio_rockchip_api.pin_configure(&fake_dev, 32U, GPIO_INPUT), -EINVAL);
	zassert_equal(gpio_rockchip_api.pin_interrupt_configure(&fake_dev, 31U, GPIO_INT_MODE_EDGE,
								GPIO_INT_TRIG_HIGH),
		      -EINVAL);
}

ZTEST(gpio_rockchip, test_port_read_write_and_toggle)
{
	gpio_port_value_t value = 0U;

	reg_write(RK_GPIO_EXT_PORT, 0x5a00a501U);
	zassert_ok(gpio_port_get_raw(&fake_dev, &value));
	zassert_equal(value, 0x5a00a501U);

	/* Pin 31 is masked out in port_pin_mask and must not be written. */
	zassert_ok(gpio_port_set_masked_raw(&fake_dev, 0x8003000fU, 0x80010005U));
	zassert_equal(reg_read(RK_GPIO_SWPORT_DR_L), 0x000f0005U);
	zassert_equal(reg_read(RK_GPIO_SWPORT_DR_H), 0x00030001U);

	zassert_ok(gpio_port_set_bits_raw(&fake_dev, 0x80040020U));
	zassert_equal(reg_read(RK_GPIO_SWPORT_DR_L), 0x00200020U);
	zassert_equal(reg_read(RK_GPIO_SWPORT_DR_H), 0x00040004U);

	zassert_ok(gpio_port_clear_bits_raw(&fake_dev, 0x80040020U));
	zassert_equal(reg_read(RK_GPIO_SWPORT_DR_L), 0x00200000U);
	zassert_equal(reg_read(RK_GPIO_SWPORT_DR_H), 0x00040000U);

	/* Toggle reads the current 16-bit halves and writes inverted bits. */
	reg_write(RK_GPIO_SWPORT_DR_L, 0x0005U);
	reg_write(RK_GPIO_SWPORT_DR_H, 0x0002U);
	zassert_ok(gpio_port_toggle_bits(&fake_dev, 0x0003000fU));
	zassert_equal(reg_read(RK_GPIO_SWPORT_DR_L), 0x000f000aU);
	zassert_equal(reg_read(RK_GPIO_SWPORT_DR_H), 0x00030001U);
}

ZTEST(gpio_rockchip, test_port_get_direction)
{
	gpio_port_pins_t inputs = 0U;
	gpio_port_pins_t outputs = 0U;

	reg_write(RK_GPIO_SWPORT_DDR_L, 0x0005U);
	reg_write(RK_GPIO_SWPORT_DDR_H, 0x0002U);

	zassert_ok(gpio_port_get_direction(&fake_dev, 0x8003000fU, &inputs, &outputs));
	zassert_equal(outputs, 0x00020005U);
	zassert_equal(inputs, 0x0001000aU);
}

ZTEST(gpio_rockchip, test_interrupt_configuration)
{
	/* Rising edge on pin 3. */
	zassert_ok(gpio_pin_interrupt_configure(&fake_dev, 3U, GPIO_INT_EDGE_RISING));
	zassert_equal(reg_read(RK_GPIO_INT_BOTHEDGE_L), 0x00080000U);
	zassert_equal(reg_read(RK_GPIO_INT_TYPE_L), 0x00080008U);
	zassert_equal(reg_read(RK_GPIO_INT_POLARITY_L), 0x00080008U);
	zassert_equal(reg_read(RK_GPIO_PORT_EOI_L), 0x00080008U);
	zassert_equal(reg_read(RK_GPIO_INT_MASK_L), 0x00080000U);
	zassert_equal(reg_read(RK_GPIO_INT_EN_L), 0x00080008U);

	/* Both edges on pin 18 uses the hardware both-edge register. */
	zassert_ok(gpio_pin_interrupt_configure(&fake_dev, 18U, GPIO_INT_EDGE_BOTH));
	zassert_equal(reg_read(RK_GPIO_INT_BOTHEDGE_H), 0x00040004U);
	zassert_equal(reg_read(RK_GPIO_INT_TYPE_H), 0x00040004U);
	zassert_equal(reg_read(RK_GPIO_INT_MASK_H), 0x00040000U);
	zassert_equal(reg_read(RK_GPIO_INT_EN_H), 0x00040004U);

	/* Level low on pin 18 clears both-edge and sets type=0, polarity=0. */
	zassert_ok(gpio_pin_interrupt_configure(&fake_dev, 18U, GPIO_INT_LEVEL_LOW));
	zassert_equal(reg_read(RK_GPIO_INT_BOTHEDGE_H), 0x00040000U);
	zassert_equal(reg_read(RK_GPIO_INT_TYPE_H), 0x00040000U);
	zassert_equal(reg_read(RK_GPIO_INT_POLARITY_H), 0x00040000U);

	/* Disable-only / enable-only preserve EOI and trigger registers. */
	reg_write(RK_GPIO_PORT_EOI_H, 0U);
	zassert_ok(gpio_pin_interrupt_configure(&fake_dev, 18U,
						GPIO_INT_DISABLE | GPIO_INT_ENABLE_DISABLE_ONLY));
	zassert_equal(reg_read(RK_GPIO_INT_MASK_H), 0x00040004U);
	zassert_equal(reg_read(RK_GPIO_INT_EN_H), 0x00040000U);
	zassert_equal(reg_read(RK_GPIO_PORT_EOI_H), 0U);

	zassert_ok(gpio_pin_interrupt_configure(&fake_dev, 18U,
						GPIO_INT_ENABLE | GPIO_INT_ENABLE_DISABLE_ONLY));
	zassert_equal(reg_read(RK_GPIO_INT_MASK_H), 0x00040000U);
	zassert_equal(reg_read(RK_GPIO_INT_EN_H), 0x00040004U);
	zassert_equal(reg_read(RK_GPIO_PORT_EOI_H), 0U);

	/* Full disable masks, disables, clears both-edge, and acks EOI. */
	zassert_ok(gpio_pin_interrupt_configure(&fake_dev, 18U, GPIO_INT_DISABLE));
	zassert_equal(reg_read(RK_GPIO_INT_MASK_H), 0x00040004U);
	zassert_equal(reg_read(RK_GPIO_INT_EN_H), 0x00040000U);
	zassert_equal(reg_read(RK_GPIO_PORT_EOI_H), 0x00040004U);

	/* Wakeup interrupts and un-wired IRQ controllers are rejected. */
	zassert_equal(
		gpio_pin_interrupt_configure(&fake_dev, 3U, GPIO_INT_EDGE_RISING | GPIO_INT_WAKEUP),
		-ENOTSUP);
	fake_config.irq_config_func = NULL;
	zassert_equal(gpio_pin_interrupt_configure(&fake_dev, 3U, GPIO_INT_EDGE_RISING), -ENOTSUP);
	zassert_ok(gpio_pin_interrupt_configure(&fake_dev, 3U, GPIO_INT_DISABLE));
}

ZTEST(gpio_rockchip, test_isr_and_callbacks)
{
	struct gpio_callback cb1;
	struct gpio_callback cb2;

	gpio_init_callback(&cb1, gpio_cb1, BIT(3) | BIT(18));
	gpio_init_callback(&cb2, gpio_cb2, BIT(5));

	zassert_ok(gpio_add_callback(&fake_dev, &cb1));
	zassert_ok(gpio_add_callback(&fake_dev, &cb2));

	/* Pending interrupts on pins 3, 18, and reserved pin 31. */
	reg_write(RK_GPIO_INT_STATUS, BIT(3) | BIT(18) | BIT(31));
	gpio_rockchip_isr(&fake_dev);

	zassert_equal(reg_read(RK_GPIO_PORT_EOI_L), 0x00080008U);
	zassert_equal(reg_read(RK_GPIO_PORT_EOI_H), 0x80048004U);
	zassert_equal(cb1_count, 1U);
	zassert_equal(cb1_pins, BIT(3) | BIT(18));
	zassert_equal(cb2_count, 0U);

	zassert_ok(gpio_remove_callback(&fake_dev, &cb1));
	reg_write(RK_GPIO_INT_STATUS, BIT(3) | BIT(5));
	gpio_rockchip_isr(&fake_dev);

	zassert_equal(cb1_count, 1U);
	zassert_equal(cb2_count, 1U);
	zassert_equal(cb2_pins, BIT(5));
}

ZTEST_SUITE(gpio_rockchip, NULL, NULL, gpio_rockchip_before, NULL, NULL);
