/*
 * Copyright (c) 2025 Synaptics, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT syna_aon_event_gpio

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/util.h>

#define GPO_EVENT_SHIFT       0
#define GPO_PULSE_WIDTH_SHIFT 4
#define GPO_POLARITY_SHIFT    8

#define SYNA_AON_INIT_AT_BOOT_PIN_COUNT DT_INST_CHILD_NUM(0)

struct syna_aon_gpio_config {
	uint8_t event;
	uint8_t pulse_width;
	uint8_t polarity;
};

struct syna_aon_config {
	mem_addr_t reg;
	struct syna_aon_gpio_config pins[SYNA_AON_INIT_AT_BOOT_PIN_COUNT];
	const struct pinctrl_dev_config *pcfg;
};

static int syna_aon_gpio_configure(const struct device *dev, uint8_t gpo,
				   const struct syna_aon_gpio_config *pin_config)
{
	const struct syna_aon_config *config = dev->config;
	uint32_t mask, shift, value, pincfg;

	if (gpo > 2 || pin_config->pulse_width > 15) {
		return -EINVAL;
	}
	shift = gpo * 9;
	mask = 0xFFF << shift;

	pincfg = 0;
	pincfg |= ((uint32_t)pin_config->event << GPO_EVENT_SHIFT);
	pincfg |= ((uint32_t)pin_config->pulse_width << GPO_PULSE_WIDTH_SHIFT);
	/* Register polarity values are inverted compared to Zephyr's GPIO_ACTIVE_* defines */
	pincfg |= ((uint32_t)(1 - pin_config->polarity) << GPO_POLARITY_SHIFT);

	value = sys_read32(config->reg);
	value &= ~mask;
	value |= pincfg << shift;
	sys_write32(value, config->reg);

	return 0;
}

static int syna_aon_gpio_initialize(const struct device *dev)
{
	const struct syna_aon_config *config = dev->config;
	const struct syna_aon_gpio_config *pin_config;
	int err;

	err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	ARRAY_FOR_EACH(config->pins, idx) {
		pin_config = &config->pins[idx];
		err = syna_aon_gpio_configure(dev, idx, pin_config);
		if (err < 0) {
			return err;
		}
	}

#ifdef CONFIG_BOARD_SR100_RDK
	/* On SR100_RDK, delay by roughly 30ms to allow 1V8 to stabilize. */
	k_sleep(K_MSEC(30));
#endif

	return 0;
}

#define SYNA_EVENT_CONFIG(idx)                                                                     \
	(struct syna_aon_gpio_config){                                                             \
		.event = DT_PROP(DT_INST_CHILD(0, CONCAT(gpo, idx)), event),                       \
		.pulse_width = DT_PROP(DT_INST_CHILD(0, CONCAT(gpo, idx)), pulse_width),           \
		.polarity = DT_PROP(DT_INST_CHILD(0, CONCAT(gpo, idx)), polarity),                 \
	},

PINCTRL_DT_INST_DEFINE(0);
static const struct syna_aon_config aon_gpio_config = {
	.reg = DT_INST_REG_ADDR(0),
	.pins = {SYNA_EVENT_CONFIG(0) SYNA_EVENT_CONFIG(1) SYNA_EVENT_CONFIG(2)},
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0)};

DEVICE_DT_INST_DEFINE(0, syna_aon_gpio_initialize, NULL, NULL, &aon_gpio_config, POST_KERNEL,
		      CONFIG_GPIO_INIT_PRIORITY, NULL);
