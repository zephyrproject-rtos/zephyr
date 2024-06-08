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

#define LOG_LEVEL CONFIG_CANOPEN_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

#define CAN_INTERFACE DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus))
#define CAN_BITRATE (DT_PROP_OR(DT_CHOSEN(zephyr_canbus), bitrate, \
					  DT_PROP_OR(DT_CHOSEN(zephyr_canbus), bus_speed, \
						     CONFIG_CAN_DEFAULT_BITRATE)) / 1000)

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
static void config_leds(CO_NMT_t *nmt)
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

	canopen_leds_init(nmt,
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
static CO_SDO_abortCode_t odf_2102(CO_ODF_arg_t *odf_arg)
{
	uint32_t value;

	value = CO_getUint32(odf_arg->data);

	if (odf_arg->reading) {
		return CO_SDO_AB_NONE;
	}

	if (odf_arg->subIndex != 0U) {
		return CO_SDO_AB_NONE;
	}

	if (value != 0) {
		/* Preserve old value */
		memcpy(odf_arg->data, odf_arg->ODdataStorage, sizeof(uint32_t));
		return CO_SDO_AB_DATA_TRANSF;
	}

	LOG_INF("Resetting button press counter");
	counter = 0;

	return CO_SDO_AB_NONE;
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
	struct canopen_context can;
	uint16_t timeout;
	uint32_t elapsed;
	int64_t timestamp;
#ifdef CONFIG_CANOPENNODE_STORAGE
	int ret;
#endif /* CONFIG_CANOPENNODE_STORAGE */

	can.dev = CAN_INTERFACE;
	if (!device_is_ready(can.dev)) {
		LOG_ERR("CAN interface not ready");
		return 0;
	}

#ifdef CONFIG_CANOPENNODE_STORAGE
	ret = settings_subsys_init();
	if (ret) {
		LOG_ERR("failed to initialize settings subsystem (err = %d)",
			ret);
		return 0;
	}

	ret = settings_load();
	if (ret) {
		LOG_ERR("failed to load settings (err = %d)", ret);
		return 0;
	}
#endif /* CONFIG_CANOPENNODE_STORAGE */

	OD_powerOnCounter++;

	config_button();

	while (reset != CO_RESET_APP) {
		elapsed =  0U; /* milliseconds */

		err = CO_init(&can, CONFIG_CANOPEN_NODE_ID, CAN_BITRATE);
		if (err != CO_ERROR_NO) {
			LOG_ERR("CO_init failed (err = %d)", err);
			return 0;
		}

		LOG_INF("CANopen stack initialized");

#ifdef CONFIG_CANOPENNODE_STORAGE
		canopen_storage_attach(CO->SDO[0], CO->em);
#endif /* CONFIG_CANOPENNODE_STORAGE */

		config_leds(CO->NMT);
		CO_OD_configure(CO->SDO[0], OD_2102_buttonPressCounter,
				odf_2102, NULL, 0U, 0U);

		if (IS_ENABLED(CONFIG_CANOPENNODE_PROGRAM_DOWNLOAD)) {
			canopen_program_download_attach(CO->NMT, CO->SDO[0],
							CO->em);
		}

		CO_CANsetNormalMode(CO->CANmodule[0]);

		while (true) {
			timeout = 1U; /* default timeout in milliseconds */
			timestamp = k_uptime_get();
			reset = CO_process(CO, (uint16_t)elapsed, &timeout);

			if (reset != CO_RESET_NOT) {
				break;
			}

			if (timeout > 0) {
				CO_LOCK_OD();
				OD_buttonPressCounter = counter;
				CO_UNLOCK_OD();

#ifdef CONFIG_CANOPENNODE_STORAGE
				ret = canopen_storage_save(
					CANOPEN_STORAGE_EEPROM);
				if (ret) {
					LOG_ERR("failed to save EEPROM");
				}
#endif /* CONFIG_CANOPENNODE_STORAGE */
				/*
				 * Try to sleep for as long as the
				 * stack requested and calculate the
				 * exact time elapsed.
				 */
				k_sleep(K_MSEC(timeout));
				elapsed = (uint32_t)k_uptime_delta(&timestamp);
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

	CO_delete(&can);
	sys_reboot(SYS_REBOOT_COLD);
}
