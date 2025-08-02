/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT microchip_mtch9010

#include <errno.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys_clock.h>

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_instance.h>
LOG_MODULE_REGISTER(mtch9010);

#include <sys/errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/drivers/sensor/mtch9010.h>
#include <zephyr/sys/__assert.h>

#include <stdio.h>
#include "mtch9010_priv.h"

#define MTCH9010_INTERNAL_BUFFER_SIZE 64u

/* Private Function Declarations */
static int mtch9010_init(const struct device *dev);
static int mtch9010_command_send(const struct device *dev, const char *str);
static int mtch9010_configure_gpio(const struct device *dev);
static int mtch9010_configure_int_gpio(const struct device *dev);
static int mtch9010_device_reset(const struct device *dev);
static int mtch9010_timeout_receive(const struct device *dev, char *buffer, uint8_t buffer_len,
				    uint16_t milliseconds);
static int mtch9010_lock_settings(const struct device *dev);

/* Callbacks */
static void mtch9010_heartbeat_callback(const struct device *dev, struct gpio_callback *cb,
					uint32_t pins);

/* API Functions */
static int mtch9010_sample_fetch(const struct device *dev, enum sensor_channel chan);
static int mtch9010_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val);

/* Private Function Implementations */
static int mtch9010_init(const struct device *dev)
{
	const struct mtch9010_config_t *config = dev->config;
	struct mtch9010_data_t *data = dev->data;
	const struct device *uart_dev = config->uart_dev;

	/* Verify UART Setup */
	struct uart_config uart_cfg = {0};

	if (uart_config_get(uart_dev, &uart_cfg) < 0) {
		LOG_INST_WRN(config->log, "Failed to read UART Config; Settings were not verified");
	} else {
		__ASSERT(uart_cfg.baudrate == MTCH9010_UART_BAUDRATE,
			 "Incorrect UART baudrate for MTCH9010");
		__ASSERT(uart_cfg.parity == MTCH9010_UART_PARITY,
			 "Incorrect UART parity for MTCH9010");
		__ASSERT(uart_cfg.stop_bits == MTCH9010_UART_STOP_BITS,
			 "Incorrect number of UART stop bits for MTCH9010");
		__ASSERT(uart_cfg.data_bits == MTCH9010_UART_DATA_BITS,
			 "Incorrect number of UART data bits for MTCH9010");
	}

	LOG_INST_DBG(config->log, "Starting device configuration");

	/* Configure heartbeat timing */
	k_sem_init(&data->heartbeat_sem, 0, 1);

	/* Configure device I/O, as needed */
	mtch9010_configure_gpio(dev);

	/* Configure INT I/O, as needed */
	mtch9010_configure_int_gpio(dev);

	/* Assert device reset, if set */
	mtch9010_device_reset(dev);

	/* Set the last heartbeat to post reset time */
	data->last_heartbeat = k_uptime_get();

	/* Wait for boot-up */
	k_msleep(MTCH9010_BOOT_TIME_MS);

	if (config->uart_init) {
		if (!device_is_ready(uart_dev)) {
			LOG_INST_ERR(config->log, "UART is not ready; Configuration skipped");
			return -EBUSY;
		}
		/* Temp Char Buffer */
		char temp_buffer[MTCH9010_INTERNAL_BUFFER_SIZE];

		if (config->mode == MTCH9010_CAPACITIVE) {
			if (mtch9010_command_send(dev, MTCH9010_CMD_STR_CAPACITIVE_MODE) < 0) {
				LOG_INST_ERR(config->log, "Failed to send capacitive mode command");
				return -EIO;
			}
		} else if (config->mode == MTCH9010_CONDUCTIVE) {
			if (mtch9010_command_send(dev, MTCH9010_CMD_STR_CONDUCTIVE_MODE) < 0) {
				LOG_INST_ERR(config->log, "Failed to send conductive mode command");
				return -EIO;
			}
		} else {
			LOG_INST_ERR(config->log, "Invalid operating mode specified");
			return -EINVAL;
		}

		/* Sleep Mode - enum matched to strings */
		snprintf(temp_buffer, sizeof(temp_buffer), "%d", config->sleep_time);
		if (mtch9010_command_send(dev, temp_buffer) < 0) {
			LOG_INST_ERR(config->log, "Failed to send sleep mode command");
			return -EIO;
		}

		/* Extended Mode Enable */
		if (config->extended_mode_enable) {
			/* Extended Mode */
			if (mtch9010_command_send(dev, MTCH9010_CMD_STR_EXTENDED_MODE_EN) < 0) {
				LOG_INST_ERR(config->log, "Failed to send extended mode command");
				return -EIO;
			}
			LOG_INST_DBG(config->log, "Extended mode is enabled");

			if (config->format == MTCH9010_OUTPUT_FORMAT_DELTA) {
				if (mtch9010_command_send(
					    dev, MTCH9010_CMD_STR_EXTENDED_FORMAT_DELTA) < 0) {
					LOG_INST_ERR(config->log,
						     "Failed to send output format command");
					return -EIO;
				}
			} else if (config->format == MTCH9010_OUTPUT_FORMAT_CURRENT) {
				if (mtch9010_command_send(
					    dev, MTCH9010_CMD_STR_EXTENDED_FORMAT_CURRENT) < 0) {
					LOG_INST_ERR(config->log,
						     "Failed to send output format command");
					return -EIO;
				}
			} else if (config->format == MTCH9010_OUTPUT_FORMAT_BOTH) {
				if (mtch9010_command_send(
					    dev, MTCH9010_CMD_STR_EXTENDED_FORMAT_BOTH) < 0) {
					LOG_INST_ERR(config->log,
						     "Failed to send output format command");
					return -EIO;
				}
			} else if (config->format == MTCH9010_OUTPUT_FORMAT_MPLAB_DATA_VISUALIZER) {
				if (mtch9010_command_send(
					    dev, MTCH9010_CMD_STR_EXTENDED_FORMAT_MPLAB_DV) < 0) {
					LOG_INST_ERR(config->log,
						     "Failed to send output format command");
					return -EIO;
				}
			} else {
				LOG_INST_ERR(config->log, "Invalid data format specified");
				return -EINVAL;
			}
		} else {
			/* Extended Config Disabled */
			if (mtch9010_command_send(dev, MTCH9010_CMD_STR_EXTENDED_MODE_DIS) < 0) {
				LOG_INST_ERR(config->log, "Failed to send extended mode command");
				return -EIO;
			}
			LOG_INST_DBG(config->log, "Extended mode is not enabled");
		}

		/* Now, the device will send it's current reference value */
		if (mtch9010_timeout_receive(dev, temp_buffer, sizeof(temp_buffer),
					     CONFIG_MTCH9010_SAMPLE_DELAY_TIMEOUT_MS) == 0) {
			LOG_INST_ERR(config->log, "Reference value was not received");
			return -EIO;
		}

		/* Reference Value */
		if (config->ref_mode == MTCH9010_REFERENCE_CURRENT_VALUE) {
			/* Convert the first value */
			struct mtch9010_result_t result = {0, 0, 0};

			if (mtch9010_decode_char_buffer(temp_buffer, MTCH9010_OUTPUT_FORMAT_CURRENT,
							&result) < 0) {
				LOG_INST_ERR(config->log, "Failed to decode reference value");
				return -EINVAL;
			}

#if (CONFIG_MTCH9010_REFERENCE_AVERAGING_COUNT == 1)
			/* No Averaging */
			if (mtch9010_command_send(dev, MTCH9010_CMD_STR_REF_MODE_CURRENT_VALUE) <
			    0) {
				LOG_INST_ERR(config->log, "Failed to send reference mode command");
				return -EIO;
			}
			data->reference = result.measurement;
#else
			uint32_t total = result.measurement;

			/* Average the value out */
			LOG_INST_DBG(config->log, "Averaging values");

			uint8_t samples = 1;

			while (samples < CONFIG_MTCH9010_REFERENCE_AVERAGING_COUNT) {
				if (mtch9010_command_send(
					    dev, MTCH9010_CMD_STR_REF_MODE_REPEAT_MEAS) < 0) {
					LOG_INST_ERR(config->log,
						     "Failed to send measurement repeat command");
					return -EIO;
				}

				if (mtch9010_timeout_receive(
					    dev, temp_buffer, sizeof(temp_buffer),
					    CONFIG_MTCH9010_SAMPLE_DELAY_TIMEOUT_MS) == 0) {
					LOG_INST_ERR(config->log, "Reference value timed "
								  "out during averaging");
					temp_buffer[0] = '\0';
					return -EIO;
				}

				if (mtch9010_decode_char_buffer(temp_buffer,
								MTCH9010_OUTPUT_FORMAT_CURRENT,
								&result) < 0) {
					LOG_INST_ERR(config->log,
						     "Failed to decode reference value");
					return -EINVAL;
				}

				total += result.measurement;

				samples++;
			}

			/* Check to see if we received averages correctly */
			uint16_t calc_ref_val =
				lround(total / CONFIG_MTCH9010_REFERENCE_AVERAGING_COUNT);

			if (calc_ref_val > MTCH9010_MAX_RESULT) {
				LOG_INST_ERR(config->log, "Computed reference out of range");
				return -EIO;
			}

			/* Update the reference value */
			data->reference = calc_ref_val;

			LOG_INST_DBG(config->log, "Average reference value = %u", calc_ref_val);

			/* Set the average value as a custom value */
			if (mtch9010_command_send(dev, MTCH9010_CMD_STR_REF_MODE_CUSTOM) < 0) {
				LOG_INST_ERR(config->log, "Failed to send custom reference value "
							  "command (for averaging)");
				return -EIO;
			}

			/* Send the computed reference */
			snprintf(temp_buffer, MTCH9010_INTERNAL_BUFFER_SIZE, "%u", data->reference);
			if (mtch9010_command_send(dev, temp_buffer) < 0) {
				LOG_INST_ERR(config->log,
					     "Failed to send averaged reference value");
				return -EIO;
			}
#endif
		} else if (config->ref_mode == MTCH9010_REFERENCE_CUSTOM_VALUE) {
			/* The user manually set a value */

			if (data->reference > MTCH9010_MAX_RESULT) {
				LOG_INST_ERR(config->log, "Reference out of range");
				return -EIO;
			}

			if (mtch9010_command_send(dev, MTCH9010_CMD_STR_REF_MODE_CUSTOM) < 0) {
				LOG_INST_ERR(config->log,
					     "Failed to send custom reference value command");
				return -EIO;
			}
			snprintf(temp_buffer, MTCH9010_INTERNAL_BUFFER_SIZE, "%u", data->reference);
			if (mtch9010_command_send(dev, temp_buffer) < 0) {
				LOG_INST_ERR(config->log, "Failed to send custom reference value");
				return -EIO;
			}
		} else {
			LOG_INST_ERR(config->log, "Invalid ref mode set");
			return -EINVAL;
		}

		if (data->threshold > MTCH9010_MAX_RESULT) {
			LOG_INST_ERR(config->log, "Threshold out of range");
			return -EINVAL;
		}

		/* Detection Threshold */
		snprintf(temp_buffer, MTCH9010_INTERNAL_BUFFER_SIZE, "%u", data->threshold);
		if (mtch9010_command_send(dev, temp_buffer) < 0) {
			LOG_INST_ERR(config->log, "Failed to send detection threshold value");
			return -EIO;
		}

		mtch9010_lock_settings(dev);
	} else {
		LOG_INST_DBG(config->log, "UART setup not enabled");
	}

#ifdef CONFIG_MTCH9010_OVERRIDE_DELAY_ENABLE
	/* Print warning */
	if (config->sleep_time != 0) {
		uint16_t timeout =
			(config->sleep_time) * 1000 + CONFIG_MTCH9010_SAMPLE_DELAY_TIMEOUT_MS;
		LOG_INST_WRN(config->log, "Device will wait up-to %u ms when fetching samples",
			     (timeout));
	}
#endif

	LOG_INST_DBG(config->log, "MTCH9010 configuration complete");

	return 0;
}

static int mtch9010_command_send(const struct device *dev, const char *str)
{
	const struct mtch9010_config_t *config = (const struct mtch9010_config_t *)dev->config;
	const struct device *uart_dev = config->uart_dev;

	LOG_INST_INF(config->log, "\"%s\"", str);

	uint8_t len = strlen(str);

	for (uint8_t i = 0; i < len; i++) {
		uart_poll_out(uart_dev, str[i]);
	}

	uart_poll_out(uart_dev, MTCH9010_SUBMIT_CHAR);

	char temp_buffer[4] = {'\0', '\0', '\0', '\0'};

	if (mtch9010_timeout_receive(dev, &temp_buffer[0], sizeof(temp_buffer),
				     MTCH9010_UART_COMMAND_TIMEOUT_MS) == 0) {
		LOG_INST_ERR(config->log, "CMD Timeout waiting for response");
		return -EIO;
	}

	if (temp_buffer[0] == MTCH9010_ACK_CHAR) {
		LOG_INST_DBG(config->log, "ACK received");
		return 0;
	} else if (temp_buffer[0] == MTCH9010_NACK_CHAR) {
		LOG_INST_ERR(config->log, "NACK received from command");
	} else {
		LOG_INST_ERR(config->log, "Unknown character received during setup");
	}
	return -EIO;
}

/* Returns the number of bytes received */
static int mtch9010_timeout_receive(const struct device *dev, char *buffer, uint8_t buffer_len,
				    uint16_t milliseconds)
{
	const struct mtch9010_config_t *config = (const struct mtch9010_config_t *)dev->config;
	const struct device *uart_dev = config->uart_dev;

	uint8_t count = 0;
	k_timepoint_t end = sys_timepoint_calc(K_MSEC(milliseconds));

	while ((count < (buffer_len - 1)) && (!sys_timepoint_expired(end))) {
		int code = uart_poll_in(uart_dev, &buffer[count]);

		if (code == 0) {
			count++;
			if ((count >= 2) && (buffer[count - 1] == '\r') &&
			    (buffer[count - 2] == '\n')) {
				/* Found end of packet - exit early */
				buffer[count] = '\0';
				return count;
			}
		}
	}

	buffer[count] = '\0';
	return count;
}

static int mtch9010_configure_gpio(const struct device *dev)
{
	const struct mtch9010_config_t *config = (const struct mtch9010_config_t *)dev->config;
	struct mtch9010_data_t *data = (struct mtch9010_data_t *)dev->data;

	/* Note: nRESET is handled in device reset function */
	int rtn = 0;

	/* UART EN (active LOW) */
	if (gpio_is_ready_dt(&config->enable_uart_gpio)) {
		if (config->uart_init) {
			gpio_pin_configure_dt(&config->enable_uart_gpio,
					      GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW);
		} else {
			gpio_pin_configure_dt(&config->enable_uart_gpio,
					      GPIO_OUTPUT | GPIO_OUTPUT_INIT_HIGH);
		}
	} else {
		LOG_INST_DBG(config->log, "UART EN line is not ready");
	}

	/* CFG EN */
	if (gpio_is_ready_dt(&config->enable_cfg_gpio)) {
		if (config->extended_mode_enable) {
			gpio_pin_configure_dt(&config->enable_cfg_gpio,
					      GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW);
		} else {
			gpio_pin_configure_dt(&config->enable_cfg_gpio,
					      GPIO_OUTPUT | GPIO_OUTPUT_INIT_HIGH);
		}
	} else {
		LOG_INST_DBG(config->log, "ECFG line is not ready");
	}

	/* OUT */
	if (gpio_is_ready_dt(&config->out_gpio)) {
		/* Setup as Input */
		gpio_pin_configure_dt(&config->out_gpio, GPIO_INPUT);
		data->last_out_state = gpio_pin_get_dt(&config->out_gpio);
	} else {
		LOG_INST_DBG(config->log, "OUT line is not ready");
		data->last_out_state = -EIO;
	}

	/* SYSTEM LOCK */
	if (gpio_is_ready_dt(&config->lock_gpio)) {
		/* Keep HIGH until ready to configure */
		gpio_pin_configure_dt(&config->lock_gpio, GPIO_OUTPUT | GPIO_OUTPUT_INIT_HIGH);
	} else {
		LOG_INST_DBG(config->log, "System Lock line is not ready");
	}

	/* WAKE */
	if (gpio_is_ready_dt(&config->wake_gpio)) {
		/* Keep HIGH until ready to assert WU / WAKE */
		gpio_pin_configure_dt(&config->wake_gpio, GPIO_OUTPUT | GPIO_OUTPUT_INIT_HIGH);
	} else {
		LOG_INST_DBG(config->log, "Wake line is not ready.");
	}

	/* MODE */
	if (gpio_is_ready_dt(&config->mode_gpio)) {
		/* Keep HIGH until ready to assert  */
		if (config->mode == MTCH9010_CAPACITIVE) {
			gpio_pin_configure_dt(&config->mode_gpio,
					      GPIO_OUTPUT | GPIO_OUTPUT_INIT_HIGH);
		} else if (config->mode == MTCH9010_CONDUCTIVE) {
			gpio_pin_configure_dt(&config->mode_gpio,
					      GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW);
		} else {
			LOG_INST_ERR(config->log, "Invalid operating mode specified");
			return -EINVAL;
		}
	} else {
		LOG_INST_DBG(config->log, "Wake line is not ready.");
	}

	return rtn;
}

static int mtch9010_configure_int_gpio(const struct device *dev)
{
	const struct mtch9010_config_t *config = (const struct mtch9010_config_t *)dev->config;
	struct mtch9010_data_t *data = (struct mtch9010_data_t *)dev->data;

	int rtn = 0;

	/* In some cases, data may not be used */
	(void)data;

	/* HEARTBEAT */
	if (gpio_is_ready_dt(&config->heartbeat_gpio)) {
		gpio_pin_configure_dt(&config->heartbeat_gpio, GPIO_INPUT);
#ifdef CONFIG_MTCH9010_HEARTBEAT_MONITORING_ENABLE
		gpio_init_callback(&data->heartbeat_cb, mtch9010_heartbeat_callback,
				   BIT(config->heartbeat_gpio.pin));
		rtn = gpio_add_callback_dt(&config->heartbeat_gpio, &data->heartbeat_cb);
		if (rtn == 0) {
			rtn = gpio_pin_interrupt_configure_dt(&config->heartbeat_gpio,
							      GPIO_INT_EDGE_RISING);
			if (rtn < 0) {
				LOG_INST_ERR(config->log, "Unable to configure interrupt; code %d",
					     rtn);
			} else {
				LOG_INST_DBG(config->log, "Configured Heartbeat Interrupt");
			}
		} else {
			LOG_INST_ERR(config->log, "Unable to add callback; code %d", rtn);
		}
#endif
	} else {
		LOG_INST_DBG(config->log, "Heartbeat line is not ready.");
	}

	return rtn;
}

static int mtch9010_device_reset(const struct device *dev)
{
	const struct mtch9010_config_t *config = (const struct mtch9010_config_t *)dev->config;

#ifndef CONFIG_MTCH9010_RESET_ON_STARTUP
	LOG_INST_DBG(config->log, "MTCH9010 reset on startup is disabled.");

	return -ENOSYS;
#else
	const struct gpio_dt_spec *reset_gpio_ptr = &config->reset_gpio;

	if (!gpio_is_ready_dt(reset_gpio_ptr)) {
		LOG_INST_WRN(config->log, "Reset line is not ready. Reset was not performed.");
		return -EBUSY;
	}

	gpio_pin_configure_dt(reset_gpio_ptr, GPIO_OUTPUT_LOW);

	LOG_INST_DBG(config->log, "Resetting MTCH9010");
	k_msleep(MTCH9010_RESET_TIME_MS);

	gpio_pin_set_dt(reset_gpio_ptr, 1);

	return 0;
#endif
}

int mtch9010_decode_char_buffer(const char *buffer, enum mtch9010_output_format format,
				struct mtch9010_result_t *result)
{
	if (result == NULL) {
		return -EINVAL;
	}

	/* Check to see if the first digit is valid */
	if (isdigit(buffer[0]) == 0) {
		return -EINVAL;
	}

	char *end_str_ptr = NULL;
	long temp = 0;

	/* Ensure that these variables are always used*/
	(void)temp;
	(void)end_str_ptr;

	switch (format) {
	case MTCH9010_OUTPUT_FORMAT_CURRENT:
		/* Data: <val>\n\r */
		temp = strtol(buffer, &end_str_ptr, 10);

		if ((temp > MTCH9010_MAX_RESULT) || (temp < MTCH9010_MIN_RESULT)) {
			return -EINVAL;
		}

		result->prev_measurement = result->measurement;
		result->measurement = (uint16_t)temp;
		break;
	case MTCH9010_OUTPUT_FORMAT_DELTA:
		/* Data: <delta>\n\r */
		temp = strtol(buffer, &end_str_ptr, 10);

		/* Note: Delta can go negative */
		if ((temp > MTCH9010_MAX_RESULT) || (temp < -MTCH9010_MAX_RESULT)) {
			return -EINVAL;
		}

		result->delta = (int16_t)temp;
		break;
	case MTCH9010_OUTPUT_FORMAT_BOTH:
		/* Data: <val> <delta>\n\r */
		temp = strtol(buffer, &end_str_ptr, 10);

		if ((temp > MTCH9010_MAX_RESULT) || (temp < MTCH9010_MIN_RESULT)) {
			return -EINVAL;
		}

		result->prev_measurement = result->measurement;
		result->measurement = (uint16_t)temp;

		if (*end_str_ptr == ' ') {
			/* Increment string pointer to the next number */
			end_str_ptr++;

			temp = strtol(end_str_ptr, &end_str_ptr, 10);

			if ((temp > MTCH9010_MAX_RESULT) || (temp < -MTCH9010_MAX_RESULT)) {
				return -EINVAL;
			}

			result->delta = (int16_t)temp;
		} else {
			return -EINVAL;
		}

		break;
	case MTCH9010_OUTPUT_FORMAT_MPLAB_DATA_VISUALIZER:
		/* Data: <start><val in hex><delta in hex><~start> */
		return -ENOTSUP;
	default:
		return -EIO;
	}

	/* Verify the \n\r is at the end */
	if ((*end_str_ptr == '\n') && (*(end_str_ptr + 1) == '\r')) {
		return 0;
	}

	return -EINVAL;
}

static int mtch9010_lock_settings(const struct device *dev)
{
#ifdef CONFIG_MTCH9010_LOCK_ON_STARTUP
	const struct mtch9010_config_t *config = dev->config;

	if (config->lock_gpio.port == NULL) {
		LOG_INST_ERR(config->log, "Lock line not ready");
		return -EIO;
	}

	LOG_INST_INF(config->log, "Locking MTCH9010");

	/* Set lock line */
	gpio_pin_set_dt(&config->lock_gpio, 0);
#endif

	return 0;
}

static int mtch9010_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct mtch9010_config_t *config = dev->config;
	struct mtch9010_data_t *data = dev->data;

	switch ((int)chan) {
	case SENSOR_CHAN_MTCH9010_OUT_STATE: {
		/* I/O output state - poll GPIO */

		data->last_out_state = gpio_pin_get_dt(&config->out_gpio);
		if (data->last_out_state < 0) {
			LOG_ERR("GPIO Error %d", data->last_out_state);
			return -EIO;
		}

		return 0;
	}
	case SENSOR_CHAN_MTCH9010_REFERENCE_VALUE: {
		/* Constant value - nothing to do */
		return 0;
	}
	case SENSOR_CHAN_MTCH9010_THRESHOLD_VALUE: {
		/* Constant value - nothing to do */
		return 0;
	}
	case SENSOR_CHAN_ALL:
	case SENSOR_CHAN_MTCH9010_SW_OUT_STATE:
	case SENSOR_CHAN_MTCH9010_MEAS_RESULT:
	case SENSOR_CHAN_MTCH9010_MEAS_DELTA: {
		/* Get the Out State */

		if (config->out_gpio.port != NULL) {
			/* Hardware OUT State - SW OUT calculated elsewhere */
			data->last_out_state = gpio_pin_get_dt(&config->out_gpio);
		}

		/* Check to see if >150 ms have passed */
		if (!sys_timepoint_expired(data->last_wake)) {
			LOG_INST_ERR(config->log, "Insufficient time between wake provided");
			return -EBUSY;
		}

		/* Blocking Wait for Sensor Data */
		uint16_t timeout = CONFIG_MTCH9010_SAMPLE_DELAY_TIMEOUT_MS;

		if (config->sleep_time != 0) {

#ifdef CONFIG_MTCH9010_OVERRIDE_DELAY_ENABLE
			timeout = (config->sleep_time) * 1000 +
				  CONFIG_MTCH9010_SAMPLE_DELAY_TIMEOUT_MS;
#else
			LOG_INST_ERR(config->log, "Wake mode is disabled if sleep period is "
						  "defined. Use SENSOR_CHAN_MTCH9010_OUT_STATE or "
						  "set CONFIG_MTCH9010_OVERRIDE_DELAY_ENABLE.");
			return -EBUSY;
#endif
		} else {
			const struct gpio_dt_spec *wake_gpio = &config->wake_gpio;

			if (!gpio_is_ready_dt(wake_gpio)) {
				LOG_INST_ERR(config->log, "Wake GPIO is not ready");
				return -EIO;
			}

			/* Wake is falling edge detected */
			gpio_pin_set_dt(wake_gpio, 0);
			k_msleep(MTCH9010_WAKE_PULSE_WIDTH_MS);
			gpio_pin_set_dt(wake_gpio, 1);

			/* Update last call */
			data->last_wake = sys_timepoint_calc(K_MSEC(MTCH9010_WAKE_TIME_BETWEEN_MS));
		}

		if (config->uart_dev == NULL) {
			LOG_INST_ERR(config->log, "UART device is not ready");
			return -ENODEV;
		}

		LOG_INST_DBG(config->log, "Fetching sample");
		char temp_buffer[MTCH9010_INTERNAL_BUFFER_SIZE];

		int char_count = mtch9010_timeout_receive(dev, &temp_buffer[0], sizeof(temp_buffer),
							  timeout);

		if (char_count == 0) {
			LOG_INST_ERR(config->log, "Unable to receive data during fetch.");
			return -EIO;
		}

		if (mtch9010_decode_char_buffer(&temp_buffer[0], config->format,
						&data->last_result) < 0) {
			LOG_INST_ERR(config->log, "Unable to decode result for channel %u", chan);
			return -EINVAL;
		}
	} break;
	case SENSOR_CHAN_MTCH9010_HEARTBEAT_ERROR_STATE: {
		/* Returns true if the heartbeat is an error state */
#ifdef CONFIG_MTCH9010_HEARTBEAT_MONITORING_ENABLE
		if (k_sem_take(&data->heartbeat_sem, K_MSEC(MTCH9010_UART_COMMAND_TIMEOUT_MS)) <
		    0) {
			return -EBUSY;
		}

		/* Compute the last access time */
		int64_t time_delta = k_uptime_delta(&data->last_heartbeat);

		k_sem_give(&data->heartbeat_sem);

		if (time_delta < MTCH9010_ERROR_PERIOD_MS) {
			data->heartbeat_error_state = true;
		} else {
			data->heartbeat_error_state = false;
		}

		return 0;
#else
		return -ENOTSUP;
#endif
	}
	default: {
		return -ENOTSUP;
	}
	}

	return 0;
}

static int mtch9010_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct mtch9010_data_t *data = dev->data;
	const struct mtch9010_config_t *config = dev->config;

	/* Val2 is not used */
	val->val2 = 0;

	switch ((int)chan) {
	case SENSOR_CHAN_MTCH9010_OUT_STATE: {
		/* I/O output state */
		val->val1 = data->last_out_state;
		break;
	}
	case SENSOR_CHAN_MTCH9010_SW_OUT_STATE: {
		/* Calculates if the OUT line would be asserted based on previous result */

		if (config->format == MTCH9010_OUTPUT_FORMAT_DELTA) {
			LOG_INST_ERR(config->log, "Cannot support SW decode in Delta mode");
			return -ENOTSUP;
		}

		if ((data->last_result.measurement - data->reference) >= data->threshold) {
			val->val1 = 1;
		} else {
			val->val1 = 0;
		}

		break;
	}
	case SENSOR_CHAN_MTCH9010_REFERENCE_VALUE: {
		/* Returns the reference value set for the sensor */
		val->val1 = data->reference;
		break;
	}
	case SENSOR_CHAN_MTCH9010_THRESHOLD_VALUE: {
		/* Returns the threshold value set for the sensor */
		val->val1 = data->threshold;
		break;
	}
	case SENSOR_CHAN_MTCH9010_MEAS_RESULT: {
		/* Returns the last measured result */
		val->val1 = data->last_result.measurement;
		break;
	}
	case SENSOR_CHAN_MTCH9010_MEAS_DELTA: {
		/* Returns the last delta */
		if ((config->format == MTCH9010_OUTPUT_FORMAT_DELTA) ||
		    (config->format == MTCH9010_OUTPUT_FORMAT_BOTH)) {
			val->val1 = data->last_result.delta;
		} else if (config->format == MTCH9010_OUTPUT_FORMAT_CURRENT) {
			/* Calculate delta from last measurement */
			val->val1 =
				data->last_result.measurement - data->last_result.prev_measurement;
		} else {
			return -ENOTSUP;
		}

		break;
	}
	case SENSOR_CHAN_MTCH9010_HEARTBEAT_ERROR_STATE: {
		/* Returns true if the heartbeat is an error state */
		val->val1 = data->heartbeat_error_state;
		break;
	}
	default: {
		return -ENOTSUP;
	}
	}

	return 0;
}

static void mtch9010_heartbeat_callback(const struct device *dev, struct gpio_callback *cb,
					gpio_port_pins_t pins)
{
	(void)pins;
	(void)dev;

	struct mtch9010_data_t *data = CONTAINER_OF(cb, struct mtch9010_data_t, heartbeat_cb);

	if (k_sem_take(&data->heartbeat_sem, K_NO_WAIT) == 0) {
		data->last_heartbeat = k_uptime_get();
		k_sem_give(&data->heartbeat_sem);
	}
}

/* Sensor APIs */
static DEVICE_API(sensor, mtch9010_api_funcs) = {
	.sample_fetch = mtch9010_sample_fetch,
	.channel_get = mtch9010_channel_get,
};

/* Device Init Macros */
#define MTCH9010_OPERATING_MODE_INIT(inst)                                                         \
	COND_CODE_1(DT_INST_ENUM_HAS_VALUE(inst, mtch9010_operating_mode,\
		capacitive), (MTCH9010_CAPACITIVE), (MTCH9010_CONDUCTIVE))

#define MTCH9010_SLEEP_TIME_INIT(inst)                                                             \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, mtch9010_sleep_period),\
		(DT_INST_ENUM_IDX(inst, mtch9010_sleep_period)), (0))

#define MTCH9010_OUTPUT_MODE_INIT(inst)                                                            \
	COND_CODE_0(DT_INST_PROP_OR(inst, mtch9010_extended_output_enable, false),\
		(MTCH9010_OUTPUT_FORMAT_CURRENT),\
		(COND_CODE_1(DT_INST_ENUM_HAS_VALUE(inst,\
		mtch9010_extended_output_format, delta),\
		((MTCH9010_OUTPUT_FORMAT_DELTA)),\
		((COND_CODE_1(DT_INST_ENUM_HAS_VALUE(inst,\
		mtch9010_extended_output_format, current) \
		, (MTCH9010_OUTPUT_FORMAT_CURRENT), \
		(COND_CODE_1(DT_INST_ENUM_HAS_VALUE(inst,\
		mtch9010_extended_output_format, both_delta_and_current),\
		(MTCH9010_OUTPUT_FORMAT_BOTH),\
		(MTCH9010_OUTPUT_FORMAT_MPLAB_DATA_VISUALIZER)\
		))))))))

#define MTCH9010_REF_MODE_INIT(inst)                                                               \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst,\
	mtch9010_reference_value),\
	(MTCH9010_REFERENCE_CUSTOM_VALUE), (MTCH9010_REFERENCE_CURRENT_VALUE))

#define MTCH9010_REF_VAL_INIT(inst)                                                                \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst,\
	mtch9010_reference_value),\
	(DT_INST_PROP(inst, mtch9010_reference_value)),\
	(0))

#define MTCH9010_USER_LABEL(inst) inst

#define MTCH9010_DEFINE(inst)                                                                      \
	LOG_INSTANCE_REGISTER(mtch9010, MTCH9010_USER_LABEL(inst), CONFIG_MTCH9010_LOG_LEVEL);     \
	static struct mtch9010_data_t mtch9010_data_##inst = {                                     \
		.reference = MTCH9010_REF_VAL_INIT(inst),                                          \
		.threshold = DT_INST_PROP(inst, mtch9010_detect_value),                            \
		.heartbeat_error_state = false,                                                    \
		.last_wake = {0},                                                                  \
		.last_result = {0, 0, 0},                                                          \
	};                                                                                         \
	static const struct mtch9010_config_t mtch9010_config_##inst = {                           \
		.uart_init = DT_PROP(DT_DRV_INST(inst), mtch9010_uart_config_enable),              \
		.uart_dev = DEVICE_DT_GET(DT_PARENT(DT_DRV_INST(inst))),                           \
		.reset_gpio = GPIO_DT_SPEC_GET_OR(DT_DRV_INST(inst), mtch9010_reset_gpios, {0}),   \
		.mode_gpio = GPIO_DT_SPEC_GET_OR(DT_DRV_INST(inst), mtch9010_mode_gpios, {0}),     \
		.wake_gpio = GPIO_DT_SPEC_GET_OR(DT_DRV_INST(inst), mtch9010_wake_gpios, {0}),     \
		.out_gpio = GPIO_DT_SPEC_GET_OR(DT_DRV_INST(inst), mtch9010_output_gpios, {0}),    \
		.heartbeat_gpio =                                                                  \
			GPIO_DT_SPEC_GET_OR(DT_DRV_INST(inst), mtch9010_heartbeat_gpios, {0}),     \
		.lock_gpio =                                                                       \
			GPIO_DT_SPEC_GET_OR(DT_DRV_INST(inst), mtch9010_system_lock_gpios, {0}),   \
		.enable_uart_gpio =                                                                \
			GPIO_DT_SPEC_GET_OR(DT_DRV_INST(inst), mtch9010_uart_en_gpios, {0}),       \
		.enable_cfg_gpio =                                                                 \
			GPIO_DT_SPEC_GET_OR(DT_DRV_INST(inst), mtch9010_cfg_en_gpios, {0}),        \
		.mode = MTCH9010_OPERATING_MODE_INIT(inst),                                        \
		.sleep_time = MTCH9010_SLEEP_TIME_INIT(inst),                                      \
		.extended_mode_enable =                                                            \
			DT_INST_PROP_OR(inst, mtch9010_extended_output_enable, false),             \
		.format = MTCH9010_OUTPUT_MODE_INIT(inst),                                         \
		.ref_mode = MTCH9010_REF_MODE_INIT(inst),                                          \
		LOG_INSTANCE_PTR_INIT(log, mtch9010, MTCH9010_USER_LABEL(inst))};                  \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, mtch9010_init, NULL, &mtch9010_data_##inst,             \
				     &mtch9010_config_##inst, POST_KERNEL,                         \
				     CONFIG_SENSOR_INIT_PRIORITY, &mtch9010_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(MTCH9010_DEFINE)
