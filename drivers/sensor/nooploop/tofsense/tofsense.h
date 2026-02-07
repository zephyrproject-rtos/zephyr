/*
 * Copyright (c) 2025 Alex Fabre
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_NOOPLOOP_TOFSENSE_H_
#define ZEPHYR_DRIVERS_SENSOR_NOOPLOOP_TOFSENSE_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <stdint.h>
#include <string.h>

/* Struct and types common to all instance of the sensor (UART and CAN) */

/* A TOFSense sensor ID is 8bit wide (0 - 255) for both UART and CAN */
typedef uint8_t tofsense_id_t;

/* Distance part of the a data frame (All fields in little endian) */
struct tofsense_distance {
	/**
	 * Distance is expressed in millimeters
	 */
	uint32_t value_mm: 24;
	/**
	 * When the ranging exceeds the measurement range, the data will jump. It is recommended to
	 * judge the data availability directly through the distance status, and generally only a
	 * distance status of 0 indicates that the data is available.
	 */
	uint32_t status: 8;
} __packed;

#ifdef CONFIG_TOFSENSE_BUS_UART
#include <zephyr/drivers/uart.h>
#include "tofsense_uart.h"
#endif

#ifdef CONFIG_TOFSENSE_BUS_CAN
/* This 'can_bus' property is used to distinguish from the uart bus
 * and the can bus, as long as the 'can-controller' does not implement the
 *'bus: can' property.
 */
#include <zephyr/drivers/can.h>
#include "tofsense_can.h"
#endif

/* Operating Mode */
/* (Factory default mode) In this mode, the module actively
 * outputs measurement information at a frequency of 30Hz
 */
#define TOFSENSE_MODE_ACTIVE 0
/* In this mode, the controller sends a query instruction
 * containing the module ID to the expected query module,
 * and the module can output one frame of measurement information
 */
#define TOFSENSE_MODE_QUERY  1

#define BUFFER_CLEAR(buf)         memset(buf, 0, sizeof(buf))
#define BUFFER_COPY(dest, source) memcpy(dest, source, sizeof(dest))

/* It is recommended to judge the data availability directly
 * through the distance status, and generally only a distance
 * status of 0 indicates that the data is available.
 */
enum distance_status {
	/* Ranging measurement is valid. */
	DISTANCE_STATUS_OKAY = 0,
	/* Standard deviation greater than 15mm. */
	DISTANCE_STATUS_DEVIATION_ERR = 1,
	/* Signal strength lower than 1Mcps. */
	DISTANCE_STATUS_SIGNAL_STRENGTH = 2,
	/* Distance measurement below threshold. */
	DISTANCE_STATUS_BELOW_THRESHOLD = 3,
	/* Phase exceeding limit. */
	DISTANCE_STATUS_PHASE_LIMITS = 4,
	/* Phase mismatch. */
	DISTANCE_STATUS_PHASE_MISMATCH = 7,
	/* Signal lower than crosstalk threshold. */
	DISTANCE_STATUS_BELOW_CROSSTALK_THRESHOLD = 9,
	/* Distances of multiple targets. */
	DISTANCE_STATUS_MULTIPLE_TARGETS = 11,
	/* Weak signal strength. */
	DISTANCE_STATUS_WEAK_SIGNAL = 12,
	/* Invalid measurement distance. */
	DISTANCE_STATUS_INVALID_DISTANCE = 14,
	/* No target detected. */
	DISTANCE_STATUS_NO_TARGET = 255
};

/* A shorter operating range increases precision and standard deviation */
enum operating_range {
	/* 3cm to 1m measurement. Accuracy +/- 1.5cm. Standard deviation < 0.3cm */
	SHORT_RANGE = 0,
	/* 3cm to 6.5m measurement. Accuracy +/- 3cm. Standard deviation < 3cm */
	MEDIUM_RANGE = 1,
	/* 3cm to 8m measurement. Accuracy +/- 3cm. Standard deviation < 5cm */
	LONG_RANGE = 2,
};

struct tofsense_bus_cfg {
#ifdef CONFIG_TOFSENSE_BUS_UART
	const struct device *uart_dev;
	uart_irq_callback_user_data_t uart_irq_cb;
#endif
#ifdef CONFIG_TOFSENSE_BUS_CAN
	const struct device *can_dev;
	can_rx_callback_t can_irq_cb;
#endif
};

struct tofsense_data {
	/* Sensor data */
	unsigned int id;
	uint32_t system_time;
	uint32_t distance_mm;
	int distance_status;
	unsigned int signal_strength;

	struct k_mutex mutex;

#ifdef CONFIG_TOFSENSE_BUS_UART
	/* Number of bytes of the current frame received into uart_rx_buffer. */
	unsigned int nb_frame_bytes_received;
	/* UART Rx buffer: Each byte received is added into this buffer. */
	uint8_t uart_rx_buffer[UART_DATA_FRAME_LENGTH];
	/* UART data frame: Once the whole uart_rx_buffer is received the content is copied into
	 * this data frame representation.
	 */
	union {
		uint8_t uart_data_frame_bytes[UART_DATA_FRAME_LENGTH];
		struct tofsense_uart_data_frame uart_data_frame;
	};
#endif
#ifdef CONFIG_TOFSENSE_BUS_CAN
	struct tofsense_can_data_frame latest_can_data_received;
#endif
};

struct tofsense_cfg {
	/* Bus config */
	int (*bus_init)(const struct device *dev);
	const struct tofsense_bus_cfg bus_cfg;
	/* Request and read callbacks */
	int (*query_data)(const struct device *dev);
	int (*read_data)(const struct device *dev);
	/* Device ID */
	const unsigned int id;
	/* Operating mode: Active (0) or Query (1) */
	const unsigned int operating_mode;
	/* When the device is configured in ACTIVE mode, 'communication_timeout' reflects the period
	 * of the output frequency defined in device tree with 'active_mode_frequency=<...>'.
	 * (default freq 30Hz gives timeout of 34ms).
	 * In QUERY mode, this timeout is always set to 34ms (arbitrary)
	 */
	const unsigned int communication_timeout;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_NOOPLOOP_TOFSENSE_H_ */
