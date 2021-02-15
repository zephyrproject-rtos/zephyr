/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_pinmux

/* Include esp-idf headers first to avoid redefining BIT() macro */
#include <soc/gpio_reg.h>
#include <soc/io_mux_reg.h>
#include <soc/soc.h>

#include <errno.h>
#include <sys/util.h>
#include <drivers/pinmux.h>

/* DR_REG_IO_MUX_BASE is a 32-bit constant.  Define a pin mux table
 * using only offsets, in order to reduce ROM footprint.
 * This table has been compiled from information present in "ESP32
 * Technical Reference Manual", "IO_MUX Pad List".  The items in
 * this array covers only the first function of each I/O pin.
 * Items with offset `0` are not present in the documentation, and
 * trying to configure them will result in -EINVAL being returned.
 *
 * Note: DR_REG_IO_MUX_BASE here is used to extract GPIO_X register offset.
 *       Don't replace it by device tree value, because PERIPHS_IO_MUX_
 *       is "internally" depends on it.
 */
#define PIN(id)   ((PERIPHS_IO_MUX_ ## id ## _U) - (DR_REG_IO_MUX_BASE))
static const uint8_t pin_mux_off[] = {
	PIN(GPIO0),    PIN(U0TXD),    PIN(GPIO2),    PIN(U0RXD),
	PIN(GPIO4),    PIN(GPIO5),    PIN(SD_CLK),   PIN(SD_DATA0),
	PIN(SD_DATA1), PIN(SD_DATA2), PIN(SD_DATA3), PIN(SD_CMD),
	PIN(MTDI),     PIN(MTCK),     PIN(MTMS),     PIN(MTDO),
	PIN(GPIO16),   PIN(GPIO17),   PIN(GPIO18),   PIN(GPIO19),
	0,             PIN(GPIO21),   PIN(GPIO22),   PIN(GPIO23),
	0,             PIN(GPIO25),   PIN(GPIO26),   PIN(GPIO27),
	0,             0,             0,             0,
	PIN(GPIO32),   PIN(GPIO33),   PIN(GPIO34),   PIN(GPIO35),
	PIN(GPIO36),   PIN(GPIO37),   PIN(GPIO38),   PIN(GPIO39)
};
#undef PIN

static uint32_t *reg_for_pin(uint32_t pin)
{
	uint8_t off;

	if (pin >= ARRAY_SIZE(pin_mux_off)) {
		return NULL;
	}

	off = pin_mux_off[pin];
	if (!off) {
		return NULL;
	}

	return (uint32_t *)(DT_INST_REG_ADDR(0) + off);
}

static int set_reg(uint32_t pin, uint32_t clr_mask, uint32_t set_mask)
{
	volatile uint32_t *reg = reg_for_pin(pin);
	uint32_t v;

	if (!reg) {
		return -EINVAL;
	}

	v = *reg;
	v &= ~clr_mask;
	v |= set_mask;
	*reg = v;

	return 0;
}

static int pinmux_set(const struct device *dev, uint32_t pin, uint32_t func)
{
	ARG_UNUSED(dev);

	/* FIXME: Drive strength (FUN_DRV) is also set here to its maximum
	 * value due to a deficiency in the pinmux API.  This setting is
	 * part of the GPIO API.
	 */

	if (func > 6) {
		return -EINVAL;
	}

	return set_reg(pin, MCU_SEL_M, func<<MCU_SEL_S | 2<<FUN_DRV_S);
}

static int pinmux_get(const struct device *dev, uint32_t pin, uint32_t *func)
{
	ARG_UNUSED(dev);
	volatile uint32_t *reg = reg_for_pin(pin);

	if (!reg) {
		return -EINVAL;
	}

	*func = (*reg & MCU_SEL_M) >> MCU_SEL_S;

	return 0;
}

static int pinmux_pullup(const struct device *dev, uint32_t pin, uint8_t func)
{
	ARG_UNUSED(dev);

	switch (func) {
	case PINMUX_PULLUP_DISABLE:
		return set_reg(pin, FUN_PU, FUN_PD);
	case PINMUX_PULLUP_ENABLE:
		return set_reg(pin, FUN_PD, FUN_PU);
	}

	return -EINVAL;
}

#define CFG(id)   ((GPIO_ ## id ## _REG) & 0xff)
static int pinmux_input(const struct device *dev, uint32_t pin, uint8_t func)
{
	ARG_UNUSED(dev);

	static const uint8_t offs[2][3] = {
		{ CFG(ENABLE1_W1TC), CFG(ENABLE1_W1TS), 32 },
		{ CFG(ENABLE_W1TC), CFG(ENABLE_W1TS), 0 },
	};
	const uint8_t *line = offs[pin < 32];
	volatile uint32_t *reg;
	int r;

	/* Since PINMUX_INPUT_ENABLED == 1 and PINMUX_OUTPUT_ENABLED == 0,
	 * we can not set a gpio port as input and output at the same time,
	 * So we always set the gpio as input. Thus, the gpio can be used on
	 * I2C drivers for example.
	 */
	r = set_reg(pin, 0, FUN_IE);
	if (func == PINMUX_INPUT_ENABLED) {
		reg = (uint32_t *)(DR_REG_GPIO_BASE + line[0]);
	} else if (func == PINMUX_OUTPUT_ENABLED) {
		if (pin >= 34U && pin <= 39U) {
			/* These pins are input only */
			return -EINVAL;
		}
		reg = (uint32_t *)(DR_REG_GPIO_BASE + line[1]);
	} else {
		return -EINVAL;
	}

	if (r < 0) {
		return r;
	}

	*reg = BIT(pin - line[2]);

	return 0;
}
#undef CFG

static struct pinmux_driver_api api_funcs = {
	.set = pinmux_set,
	.get = pinmux_get,
	.pullup = pinmux_pullup,
	.input = pinmux_input
};

static int pinmux_initialize(const struct device *device)
{
	ARG_UNUSED(device);

#if !CONFIG_BOOTLOADER_ESP_IDF
	uint32_t pin;

	for (pin = 0U; pin < ARRAY_SIZE(pin_mux_off); pin++) {
		pinmux_set(NULL, pin, 0);
	}

#endif

	return 0;
}

/* Initialize using PRE_KERNEL_1 priority so that GPIO can use the pin
 * mux driver.
 */
DEVICE_DT_INST_DEFINE(0, &pinmux_initialize,
		    device_pm_control_nop, NULL, NULL,
		    PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &api_funcs);
