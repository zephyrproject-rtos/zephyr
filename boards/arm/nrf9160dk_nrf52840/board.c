/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <hal/nrf_gpio.h>

LOG_MODULE_REGISTER(board_control, CONFIG_BOARD_NRF9160DK_LOG_LEVEL);

#define GET_CTLR(name, prop, idx) \
	DT_GPIO_CTLR_BY_IDX(DT_NODELABEL(name), prop, idx)
#define GET_PIN(name, prop, idx) \
	DT_GPIO_PIN_BY_IDX(DT_NODELABEL(name), prop, idx)
#define GET_PORT(name, prop, idx) \
	DT_PROP_BY_PHANDLE_IDX(DT_NODELABEL(name), prop, idx, port)
#define GET_FLAGS(name, prop, idx) \
	DT_GPIO_FLAGS_BY_IDX(DT_NODELABEL(name), prop, idx)
#define GET_DEV(name, prop, idx) DEVICE_DT_GET(GET_CTLR(name, prop, idx))

/* If the GPIO pin selected to be the reset line is actually the pin that
 * exposes the nRESET function (P0.18 in nRF52840), there is no need to
 * provide any additional GPIO configuration for it.
 */
#define RESET_INPUT_IS_PINRESET (IS_ENABLED(CONFIG_GPIO_AS_PINRESET) && \
				 GET_PORT(reset_input, gpios, 0) == 0 && \
				 GET_PIN(reset_input, gpios, 0) == 18)
#define USE_RESET_GPIO \
	(DT_NODE_HAS_STATUS(DT_NODELABEL(reset_input), okay) && \
	 !RESET_INPUT_IS_PINRESET)

struct switch_cfg {
	const struct device *gpio;
	gpio_pin_t pin;
	gpio_dt_flags_t flags;
	bool on;
#if defined(CONFIG_LOG)
	uint8_t port;
	bool info;
	const char *name;
#endif
};

#define ROUTING_ENABLED(_name) DT_NODE_HAS_STATUS(DT_NODELABEL(_name), okay)
#define SWITCH_CFG(_name, _idx)					\
{								\
	.gpio  = GET_DEV(_name, control_gpios, _idx),		\
	.pin   = GET_PIN(_name, control_gpios, _idx),		\
	.flags = GET_FLAGS(_name, control_gpios, _idx),		\
	.on    = ROUTING_ENABLED(_name),			\
	COND_CODE_1(CONFIG_LOG,					\
	(							\
		.port = GET_PORT(_name, control_gpios, _idx),	\
		.info = (_idx == 0),				\
		.name = #_name,					\
	), ())							\
}
#define HAS_TWO_PINS(_name) \
	DT_PHA_HAS_CELL_AT_IDX(DT_NODELABEL(_name), control_gpios, 1, pin)

#define ROUTING_SWITCH(_name)					\
	COND_CODE_1(DT_NODE_EXISTS(DT_NODELABEL(_name)),	\
	(							\
		COND_CODE_1(HAS_TWO_PINS(_name),		\
		(						\
			SWITCH_CFG(_name, 1),			\
		), ())						\
		SWITCH_CFG(_name, 0),				\
	), ())

static const struct switch_cfg routing_switches[] = {
	ROUTING_SWITCH(vcom0_pins_routing)
	ROUTING_SWITCH(vcom2_pins_routing)
	ROUTING_SWITCH(led1_pin_routing)
	ROUTING_SWITCH(led2_pin_routing)
	ROUTING_SWITCH(led3_pin_routing)
	ROUTING_SWITCH(led4_pin_routing)
	ROUTING_SWITCH(switch1_pin_routing)
	ROUTING_SWITCH(switch2_pin_routing)
	ROUTING_SWITCH(button1_pin_routing)
	ROUTING_SWITCH(button2_pin_routing)
	ROUTING_SWITCH(nrf_interface_pins_0_2_routing)
	ROUTING_SWITCH(nrf_interface_pins_3_5_routing)
	ROUTING_SWITCH(nrf_interface_pins_6_8_routing)
	ROUTING_SWITCH(nrf_interface_pin_9_routing)
	ROUTING_SWITCH(io_expander_pins_routing)
	ROUTING_SWITCH(external_flash_pins_routing)
};

#if USE_RESET_GPIO
static void chip_reset(const struct device *gpio,
		       struct gpio_callback *cb, uint32_t pins)
{
	const uint32_t stamp = k_cycle_get_32();

	printk("GPIO reset line asserted, device reset.\n");
	printk("Bye @ cycle32 %u\n", stamp);

	NVIC_SystemReset();
}

static void reset_pin_wait_inactive(const struct device *gpio, uint32_t pin)
{
	int val;

	/* Wait until the pin becomes inactive. */
	do {
		val = gpio_pin_get(gpio, pin);
	} while (val > 0);
}

static int reset_pin_configure(void)
{
	int rc;
	static struct gpio_callback gpio_ctx;

	const struct device *gpio = GET_DEV(reset_input, gpios, 0);
	gpio_pin_t pin = GET_PIN(reset_input, gpios, 0);
	gpio_dt_flags_t flags = GET_FLAGS(reset_input, gpios, 0);

	if (!device_is_ready(gpio)) {
		LOG_ERR("%s is not ready", gpio->name);
		return -ENODEV;
	}

	rc = gpio_pin_configure(gpio, pin, flags | GPIO_INPUT);
	if (rc) {
		LOG_ERR("Error %d while configuring pin P%d.%02d",
			rc, GET_PORT(reset_input, gpios, 0), pin);
		return rc;
	}

	gpio_init_callback(&gpio_ctx, chip_reset, BIT(pin));

	rc = gpio_add_callback(gpio, &gpio_ctx);
	if (rc) {
		return rc;
	}

	rc = gpio_pin_interrupt_configure(gpio, pin, GPIO_INT_EDGE_TO_ACTIVE);
	if (rc) {
		return rc;
	}

	LOG_INF("GPIO reset line enabled on pin P%d.%02d, holding...",
		GET_PORT(reset_input, gpios, 0), pin);

	/* Wait until the pin becomes inactive before continuing.
	 * This lets the other side ensure that they are ready.
	 */
	reset_pin_wait_inactive(gpio, pin);

	return 0;
}
#endif /* USE_RESET_GPIO */

static int init(void)
{
	int rc;

	for (int i = 0; i < ARRAY_SIZE(routing_switches); ++i) {
		const struct switch_cfg *cfg = &routing_switches[i];
		gpio_flags_t flags = cfg->flags;

		if (!device_is_ready(cfg->gpio)) {
			LOG_ERR("%s is not ready", cfg->gpio->name);
			return -ENODEV;
		}

		flags |= (cfg->on ? GPIO_OUTPUT_ACTIVE
				  : GPIO_OUTPUT_INACTIVE);
		rc = gpio_pin_configure(cfg->gpio, cfg->pin, flags);
#if defined(CONFIG_LOG)
		LOG_DBG("Configuring P%d.%02d with flags: 0x%08x",
			cfg->port, cfg->pin, flags);
		if (rc) {
			LOG_ERR("Error %d while configuring pin P%d.%02d (%s)",
				rc, cfg->port, cfg->pin, cfg->name);
		} else if (cfg->info) {
			LOG_INF("%s is %s",
				cfg->name, cfg->on ? "ENABLED" : "disabled");
		}
#endif
		if (rc) {
			return rc;
		}
	}

	/* Make sure to configure the switches before initializing
	 * the GPIO reset pin, so that we are connected to
	 * the nRF9160 before enabling our interrupt.
	 */

#if USE_RESET_GPIO
	rc = reset_pin_configure();
	if (rc) {
		LOG_ERR("Unable to configure reset pin, err %d", rc);
		return -EIO;
	}
#endif

	LOG_INF("Board configured.");

	return 0;
}

SYS_INIT(init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

#define EXT_MEM_CTRL DT_NODELABEL(external_flash_pins_routing)
#if DT_NODE_EXISTS(EXT_MEM_CTRL)

static int early_init(void)
{
	/* As soon as possible after the system starts up, enable the analog
	 * switch that routes signals to the external flash. Otherwise, the
	 * HOLD line in the flash chip may not be properly pulled up internally
	 * and consequently the chip will not respond to any command.
	 * Later on, during the normal initialization performed above, this
	 * analog switch will get configured according to what is selected
	 * in devicetree.
	 */
	uint32_t psel = NRF_DT_GPIOS_TO_PSEL(EXT_MEM_CTRL, control_gpios);
	gpio_dt_flags_t flags = DT_GPIO_FLAGS(EXT_MEM_CTRL, control_gpios);

	if (flags & GPIO_ACTIVE_LOW) {
		nrf_gpio_pin_clear(psel);
	} else {
		nrf_gpio_pin_set(psel);
	}
	nrf_gpio_cfg_output(psel);

	return 0;
}

SYS_INIT(early_init, PRE_KERNEL_1, 0);
#endif
