/*
 * Copyright (c) 2020 Christian Taedcke
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/gpio.h>
#include <sys/printk.h>

struct supply_cfg {
	const struct device *gpio;
	gpio_pin_t pin;
	gpio_dt_flags_t flags;
};

static int enable_supply(const struct supply_cfg *cfg)
{
	int rv = -ENODEV;

	if (device_is_ready(cfg->gpio)) {
		gpio_pin_configure(cfg->gpio, cfg->pin,
				   GPIO_OUTPUT | cfg->flags);
		gpio_pin_set(cfg->gpio, cfg->pin, 1);
		rv = 0;
	}

	return rv;
}

static int efr32mg_sltb004a_init(const struct device *dev)
{
	struct supply_cfg cfg;
	int rc = 0;

	ARG_UNUSED(dev);
	(void)cfg;

#define CCS811 DT_NODELABEL(ccs811)

#if DT_NODE_HAS_STATUS(CCS811, okay)
	cfg = (struct supply_cfg){
		.gpio = DEVICE_DT_GET(DT_GPIO_CTLR(CCS811, supply_gpios)),
		.pin = DT_GPIO_PIN(CCS811, supply_gpios),
		.flags = DT_GPIO_FLAGS(CCS811, supply_gpios),
	};

	/* Enable the CCS811 power */
	rc = enable_supply(&cfg);
	if (rc < 0) {
		printk("CCS811 supply not enabled: %d\n", rc);
	}
#endif

	return rc;
}

/* needs to be done after GPIO driver init */
SYS_INIT(efr32mg_sltb004a_init, PRE_KERNEL_1, CONFIG_BOARD_INIT_PRIORITY);
