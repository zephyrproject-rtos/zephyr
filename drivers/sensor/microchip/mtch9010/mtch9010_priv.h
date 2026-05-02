/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_MICROCHIP_MTCH9010_PRIV_H_
#define ZEPHYR_DRIVERS_SENSOR_MICROCHIP_MTCH9010_PRIV_H_

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log_instance.h>

#include <zephyr/drivers/sensor/mtch9010.h>

/* Character to submit requests */
#define MTCH9010_SUBMIT_CHAR '\r'

/* Characters for determining if commands were accepted / rejected */
#define MTCH9010_ACK_CHAR  0x06u
#define MTCH9010_NACK_CHAR 0x15u

/* Command Strings w/o submit character */
#define MTCH9010_CMD_STR_CAPACITIVE_MODE "0"
#define MTCH9010_CMD_STR_CONDUCTIVE_MODE "1"

/* WOR - Wake on Request */
#define MTCH9010_CMD_STR_SLEEP_TIME_WOR  "0"
#define MTCH9010_CMD_STR_SLEEP_TIME_1S   "1"
#define MTCH9010_CMD_STR_SLEEP_TIME_2S   "2"
#define MTCH9010_CMD_STR_SLEEP_TIME_4S   "3"
#define MTCH9010_CMD_STR_SLEEP_TIME_8S   "4"
#define MTCH9010_CMD_STR_SLEEP_TIME_16S  "5"
#define MTCH9010_CMD_STR_SLEEP_TIME_32S  "6"
#define MTCH9010_CMD_STR_SLEEP_TIME_64S  "7"
#define MTCH9010_CMD_STR_SLEEP_TIME_128S "8"
#define MTCH9010_CMD_STR_SLEEP_TIME_256S "9"

#define MTCH9010_CMD_STR_EXTENDED_MODE_DIS "0"
#define MTCH9010_CMD_STR_EXTENDED_MODE_EN  "1"

#define MTCH9010_CMD_STR_EXTENDED_FORMAT_DELTA    "0"
#define MTCH9010_CMD_STR_EXTENDED_FORMAT_CURRENT  "1"
#define MTCH9010_CMD_STR_EXTENDED_FORMAT_BOTH     "2"
#define MTCH9010_CMD_STR_EXTENDED_FORMAT_MPLAB_DV "3"

#define MTCH9010_CMD_STR_REF_MODE_CURRENT_VALUE "0"
#define MTCH9010_CMD_STR_REF_MODE_REPEAT_MEAS   "1"
#define MTCH9010_CMD_STR_REF_MODE_CUSTOM        "2"

/* Device Constants */
#define MTCH9010_RESET_TIME_MS           10u
#define MTCH9010_BOOT_TIME_MS            10u
#define MTCH9010_UART_COMMAND_TIMEOUT_MS 20u
#define MTCH9010_WAKE_PULSE_WIDTH_MS     1u
#define MTCH9010_ERROR_PERIOD_MS         220u

/* UART Constants */
#define MTCH9010_UART_BAUDRATE  38400u
#define MTCH9010_UART_DATA_BITS UART_CFG_DATA_BITS_8
#define MTCH9010_UART_PARITY    UART_CFG_PARITY_NONE
#define MTCH9010_UART_STOP_BITS UART_CFG_STOP_BITS_1

/* Time between wake requests */
#define MTCH9010_WAKE_TIME_BETWEEN_MS 150u

struct mtch9010_result {
	/* Received Reference Value */
	uint16_t measurement;
	/* Last Measurement Value */
	uint16_t prev_measurement;
	/* Received Delta Value */
	int16_t delta;
};

enum mtch9010_reference_value_init {
	/* MTCH9010 sets the current value as the reference value */
	MTCH9010_REFERENCE_CURRENT_VALUE = 0,
	/* MTCH9010 re-runs the measurement */
	MTCH9010_REFERENCE_RERUN_VALUE,
	/* MTCH9010 sets the reference to the value the user defines */
	MTCH9010_REFERENCE_CUSTOM_VALUE
};

struct mtch9010_data {
	/* Threshold of the Sensor */
	uint16_t threshold;
	/* Reference (Dry) value of the Sensor */
	uint16_t reference;
	/* Last time we asked for data */
	k_timepoint_t last_wake;
	/* Last time a heartbeat was detected */
	int64_t last_heartbeat;
	/* Semaphore for accessing heartbeat */
	struct k_sem heartbeat_sem;
	/* Heartbeat GPIO Callback */
	struct gpio_callback heartbeat_cb;
	/* Last state of the OUT pin */
	int last_out_state;
	/* If true, the heartbeat is sending the error pattern */
	bool heartbeat_error_state;
	/* Last result received from the sensor */
	struct mtch9010_result last_result;
};

struct mtch9010_config {
	/* Set to true if the init function should configure the device */
	bool uart_init;
	/* Pointer to UART Bus */
	const struct device *uart_dev;
	/* OP_MODE Signal for I/O mode */
	const struct gpio_dt_spec mode_gpio;
	/* nRESET Signal for MTCH9010 */
	const struct gpio_dt_spec reset_gpio;
	/* Wake-Up (WU) Signal for MTCH9010 */
	const struct gpio_dt_spec wake_gpio;
	/* OUT Signal for MTCH9010 */
	const struct gpio_dt_spec out_gpio;
	/* SYS_LK Signal to Program Startup Settings */
	const struct gpio_dt_spec lock_gpio;
	/* nUART_EN Signal to Enable UART Communication */
	const struct gpio_dt_spec enable_uart_gpio;
	/* nCFG_EN Signal for I/O Mode*/
	const struct gpio_dt_spec enable_cfg_gpio;
	/* Heartbeat (HB) Output of MTCH9010 */
	const struct gpio_dt_spec heartbeat_gpio;
	/* Operating mode (Capacitive / Conductive) */
	uint8_t mode;
	/* Sleep Time of device in seconds. Set to 0 for Wake on Request */
	int sleep_time;
	/* Set to true if extended format output is configured */
	bool extended_mode_enable;
	/* Format of the UART Output Data */
	uint8_t format;
	/* Initialization mode of the reference */
	enum mtch9010_reference_value_init ref_mode;
	/* Logging Instance */
	LOG_INSTANCE_PTR_DECLARE(log);
};

/* Helper function to decode a NULL-terminated string packet */
int mtch9010_decode_char_buffer(const char *buffer, uint8_t format,
				struct mtch9010_result *result);

#endif /* ZEPHYR_DRIVERS_SENSOR_MICROCHIP_MTCH9010_PRIV_H_ */
