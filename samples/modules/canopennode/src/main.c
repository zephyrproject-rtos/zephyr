/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/settings/settings.h>

#include <canopennode.h>
#ifdef CONFIG_CANOPENNODE_STORAGE
#include <storage/CO_storage.h>
#endif
#include "OD.h"

#define LOG_LEVEL CONFIG_CANOPEN_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

#define CAN_INTERFACE DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus))
#define CAN_BITRATE \
	(DT_PROP_OR(DT_CHOSEN(zephyr_canbus), bitrate, \
		    DT_PROP_OR(DT_CHOSEN(zephyr_canbus), bus_speed, \
					 CONFIG_CAN_DEFAULT_BITRATE)) / 1000U)

/* default values for CO_CANopenInit as enums for debugger symbols */
enum {
	NMT_CONTROL =
		CO_NMT_STARTUP_TO_OPERATIONAL |
		CO_NMT_ERR_ON_ERR_REG |
		CO_ERR_REG_GENERIC_ERR |
		CO_ERR_REG_COMMUNICATION,
	FIRST_HB_TIME = 500, /* ms */
	SDO_SRV_TIMEOUT_TIME = 1000, /* ms */
	SDO_CLI_TIMEOUT_TIME = 500, /* ms */
	SDO_CLI_BLOCK = false, /* block transfer */
};


static struct gpio_dt_spec led_green_gpio = GPIO_DT_SPEC_GET_OR(
		DT_ALIAS(green_led), gpios, {0});
static struct gpio_dt_spec led_red_gpio = GPIO_DT_SPEC_GET_OR(
		DT_ALIAS(red_led), gpios, {0});

static struct gpio_dt_spec button_gpio = GPIO_DT_SPEC_GET_OR(
		DT_ALIAS(sw0), gpios, {0});
static struct gpio_callback button_callback;

struct led_indicator {
	const struct device *dev;
	gpio_pin_t pin;
};

static uint32_t counter;
static OD_extension_t OD_2102_buttonPressCounter_extension;

/**
 * @brief Callback for setting LED indicator state.
 *
 * @param value true if the LED indicator shall be turned on, false otherwise.
 * @param arg argument that was passed when LEDs were initialized.
 */
static void led_callback(bool value, void *arg)
{
	struct gpio_dt_spec *led_gpio = arg;

	if (!led_gpio || !led_gpio->port) {
		return;
	}

	gpio_pin_set_dt(led_gpio, value);
}

/**
 * @brief Configure LED indicators pins and callbacks.
 *
 * This routine configures the GPIOs for the red and green LEDs (if
 * available).
 *
 * @param nmt CANopenNode NMT object.
 */
static void config_leds(struct canopen_context *ctx)
{
	int err;

	if (!led_green_gpio.port) {
		LOG_INF("Green LED not available");
	} else if (!gpio_is_ready_dt(&led_green_gpio)) {
		LOG_ERR("Green LED device not ready");
		led_green_gpio.port = NULL;
	} else {
		err = gpio_pin_configure_dt(&led_green_gpio,
					    GPIO_OUTPUT_INACTIVE);
		if (err) {
			LOG_ERR("failed to configure Green LED gpio: %d", err);
			led_green_gpio.port = NULL;
		}
	}

	if (!led_red_gpio.port) {
		LOG_INF("Red LED not available");
	} else if (!gpio_is_ready_dt(&led_red_gpio)) {
		LOG_ERR("Red LED device not ready");
		led_red_gpio.port = NULL;
	} else {
		err = gpio_pin_configure_dt(&led_red_gpio,
					    GPIO_OUTPUT_INACTIVE);
		if (err) {
			LOG_ERR("failed to configure Red LED gpio: %d", err);
			led_red_gpio.port = NULL;
		}
	}

	canopen_leds_init(ctx,
			  led_callback, &led_green_gpio,
			  led_callback, &led_red_gpio);
}

/**
 * @brief Button press counter object dictionary handler function.
 *
 * This function is called upon SDO access to the button press counter
 * object (index 0x2102) in the object dictionary.
 *
 * @param odf_arg object dictionary function argument.
 *
 * @return SDO abort code.
 */
static ODR_t OD_write_2102(OD_stream_t *stream, const void *buf,
			   OD_size_t count, OD_size_t *written)
{
	if (stream == NULL || stream->subIndex != 0 ||
	    buf == NULL || count != sizeof(uint32_t) || written == NULL) {
		return ODR_DEV_INCOMPAT;
	}

	uint32_t value = CO_getUint32(buf);
	if (value == 0) {
		LOG_INF("Resetting button press counter");
		counter = 0;
	}

	/* Preserve old value */
	return OD_writeOriginal(stream, buf, count, written);
}

/**
 * @brief Button press interrupt callback.
 *
 * @param port GPIO device struct.
 * @param cb GPIO callback struct.
 * @param pins GPIO pin mask that triggered the interrupt.
 */
static void button_isr_callback(const struct device *port,
				struct gpio_callback *cb,
				uint32_t pins)
{
	counter++;
}

/**
 * @brief Configure button GPIO pin and callback.
 *
 * This routine configures the GPIO for the button (if available).
 */
static void config_button(void)
{
	int err;

	if (button_gpio.port == NULL) {
		LOG_INF("Button not available");
		return;
	}

	if (!gpio_is_ready_dt(&button_gpio)) {
		LOG_ERR("Button device not ready");
		return;
	}

	err = gpio_pin_configure_dt(&button_gpio, GPIO_INPUT);
	if (err) {
		LOG_ERR("failed to configure button gpio: %d", err);
		return;
	}

	gpio_init_callback(&button_callback, button_isr_callback,
			   BIT(button_gpio.pin));

	err = gpio_add_callback(button_gpio.port, &button_callback);
	if (err) {
		LOG_ERR("failed to add button callback: %d", err);
		return;
	}

	err = gpio_pin_interrupt_configure_dt(&button_gpio,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (err) {
		LOG_ERR("failed to enable button callback: %d", err);
		return;
	}
}

/**
 * @brief Main application entry point.
 *
 * The main application thread is responsible for initializing the
 * CANopen stack and doing the non real-time processing.
 */
int main(void)
{
	CO_NMT_reset_cmd_t reset = CO_RESET_NOT;
	CO_ReturnError_t err;
	uint32_t errinfo;
	uint32_t timeout;
	uint32_t elapsed;
	int64_t timestamp;
#ifdef CONFIG_CANOPENNODE_STORAGE
	int ret;
	OD_entry_t *OD_1010;
	OD_entry_t *OD_1011;
	CO_storage_entry_t storage_entries[] = {
		{
			.addr = &OD_ROM,
			.len = sizeof(OD_ROM),
			.subIndexOD = 2,
			.attr = CO_storage_cmd | CO_storage_restore,
			.type = CANOPEN_STORAGE_ROM
		},
		{
			.addr = &OD_EEPROM,
			.len = sizeof(OD_EEPROM),
			.subIndexOD = 3,
			.attr = CO_storage_cmd | CO_storage_restore,
			.type = CANOPEN_STORAGE_EEPROM
		}
	};
	OD_size_t storage_entries_count = ARRAY_SIZE(storage_entries);
#endif /* CONFIG_CANOPENNODE_STORAGE */

	struct canopen_context canctx = {
		.dev = CAN_INTERFACE,
		.pending_nodeid = CONFIG_CANOPEN_NODE_ID,
		.pending_bitrate = CAN_BITRATE
	};
	if (!device_is_ready(canctx.dev)) {
		LOG_ERR("CAN interface not ready");
		return 0;
	}

	/*
	 * This canopen module defaults to using globals, so there is no
	 * allocation and the CO_new will just init the object.
	 */
	canctx.co = CO_new(NULL, NULL);
	if (canctx.co == NULL) {
		LOG_ERR("CANopen init failed");
		return 0;
	}

	OD_set_u32(OD_ENTRY_H2102_buttonPressCounter, 0, 1, true);
	OD_2102_buttonPressCounter_extension.object = &canctx;
	OD_2102_buttonPressCounter_extension.read = OD_readOriginal;
	OD_2102_buttonPressCounter_extension.write = OD_write_2102;
	OD_extension_init(OD_ENTRY_H2102_buttonPressCounter,
			  &OD_2102_buttonPressCounter_extension);

#ifdef CONFIG_CANOPENNODE_STORAGE
	ret = settings_subsys_init();
	if (ret) {
		LOG_ERR("settings_subsys_init failed (err = %d)", ret);
		return 0;
	}

	ret = settings_load();
	if (ret) {
		LOG_ERR("settings_load failed (err = %d)", ret);
		return 0;
	}
#endif /* CONFIG_CANOPENNODE_STORAGE */

	config_button();

	while (reset != CO_RESET_APP) {
		elapsed = 0U; /* milliseconds */

		CO_CANmodule_disable(canctx.co->CANmodule);

		err = CO_CANinit(canctx.co, &canctx, canctx.pending_bitrate);
		if (err != CO_ERROR_NO) {
			LOG_ERR("CO_CANinit failed (err = %d)", err);
			return 0;
		}

		CO_LSS_address_t lss_address = {.identity = {
			.vendorID = OD_ROM.x1018_identity.vendor_ID,
			.productCode = OD_ROM.x1018_identity.productCode,
			.revisionNumber = OD_ROM.x1018_identity.revisionNumber,
			.serialNumber = OD_ROM.x1018_identity.serialNumber
		}};
		err = CO_LSSinit(canctx.co, &lss_address,
				 &canctx.pending_nodeid,
				 &canctx.pending_bitrate);
		if (err != CO_ERROR_NO) {
			LOG_ERR("CO_LSSinit failed (err = %d)", err);
			return 0;
		}
		canctx.active_nodeid = canctx.pending_nodeid;
		canctx.active_bitrate = canctx.pending_bitrate;
		canctx.pending_nodeid = CONFIG_CANOPEN_NODE_ID;
		canctx.pending_bitrate = CAN_BITRATE;

		err = CO_CANopenInit(canctx.co, /* CANopen object */
				     NULL,      /* alternate NMT */
				     NULL,      /* alternate em */
				     OD,        /* Object dictionary */
				     NULL,      /* Optional status bits */
				     NMT_CONTROL,    /* CO_NMT_control_t */
				     FIRST_HB_TIME,  /* First heartbeat */
				     SDO_SRV_TIMEOUT_TIME, /* Server timeout */
				     SDO_CLI_TIMEOUT_TIME, /* Client timeout */
				     SDO_CLI_BLOCK,  /* Client block transfer */
				     canctx.active_nodeid,
				     &errinfo);
		if (err == CO_ERROR_OD_PARAMETERS) {
			LOG_ERR("CO_CANopenInit OD entry 0x%X\n", errinfo);
			return 0;
		} else if (err != CO_ERROR_NO &&
			   err != CO_ERROR_NODE_ID_UNCONFIGURED_LSS) {
			LOG_ERR("CO_CANopenInit failed (err = %d)", err);
			return 0;
		}

		err = CO_CANopenInitPDO(canctx.co, canctx.co->em, OD,
					canctx.active_nodeid, &errinfo);
		if (err == CO_ERROR_OD_PARAMETERS) {
			LOG_ERR("CO_CANopenInitPDO OD entry 0x%X\n", errinfo);
			return 0;
		} else if (err != CO_ERROR_NO) {
			LOG_ERR("CO_CANopenInitPDO failed (err = %d)", err);
			return 0;
		}

		LOG_INF("CANopen stack initialized");

#ifdef CONFIG_CANOPENNODE_STORAGE
		OD_1010 = OD_find(OD, OD_H1010_STORE_PARAMETERS);
		if (OD_1010 == NULL) {
			LOG_ERR("OD find entry 0x1010 failed");
			return 0;
		}
		OD_1011 = OD_find(OD, OD_H1011_RESTORE_DEFAULT);
		if (OD_1011 == NULL) {
			LOG_ERR("OD find entry 0x1011 failed");
			return 0;
		}

		canopen_storage_attach(&canctx, OD_1010, OD_1011,
				       storage_entries, storage_entries_count);
#endif /* CONFIG_CANOPENNODE_STORAGE */

		config_leds(&canctx);

		if (IS_ENABLED(CONFIG_CANOPENNODE_PROGRAM_DOWNLOAD)) {
			canopen_program_download_attach(&canctx);
		}

		/* Start CAN stack */
		CO_CANsetNormalMode(canctx.co->CANmodule);
		LOG_INF("CANopen stack running");

		for (;;) {
			timeout = 1U; /* default timeout in milliseconds */
			timestamp = k_uptime_get();

			reset = CO_process(canctx.co, false, elapsed, &timeout);
			if (reset != CO_RESET_NOT) {
				break;
			}

			canopen_leds_update(&canctx);

			if (timeout > 0) {
				OD_set_u32(OD_ENTRY_H2102_buttonPressCounter,
					   0, counter, true);

				/*
				 * Try to sleep for as long as the
				 * stack requested and calculate the
				 * exact time elapsed.
				 */
				k_sleep(K_MSEC(timeout));
				elapsed = k_uptime_delta(&timestamp);
			} else {
				/*
				 * Do not sleep, more processing to be
				 * done by the stack.
				 */
				elapsed = 0U;
			}
		}

		if (reset == CO_RESET_COMM) {
			LOG_INF("Resetting communication");
		}
	}

	LOG_INF("Resetting device");

	CO_delete(canctx.co);
	sys_reboot(SYS_REBOOT_COLD);
}
