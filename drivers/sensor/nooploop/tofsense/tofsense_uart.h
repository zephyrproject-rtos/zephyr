/*
 * Copyright (c) 2025 Alex Fabre
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_NOOPLOOP_TOFSENSE_UART_H_
#define ZEPHYR_DRIVERS_SENSOR_NOOPLOOP_TOFSENSE_UART_H_

/* UART generic byte values */
#define UART_FRAME_HEADER_BYTE           (0x57)
#define UART_ACTIVE_OUTPUT_PROTOCOL_BYTE (0x00)
#define UART_QUERY_OUTPUT_PROTOCOL_BYTE  (0x10)

/* UART Configuration byte values */
#define UART_CONFIG_HEADER_BYTE  (0x54)
#define UART_CONFIG_RANGE_SHORT  (0x00)
#define UART_CONFIG_RANGE_MEDIUM (0x01)
#define UART_CONFIG_RANGE_LONG   (0x02)

/* UART Configuration frame info */
#define UART_CONFIG_FRAME_LENGTH   (32)
#define UART_CONFIG_CHECKSUM_INDEX (0x1F)

/* UART data frame info */
#define UART_DATA_FRAME_LENGTH   (16)
#define UART_DATA_CHECKSUM_INDEX (0x0F)

/* UART read request frame info */
#define UART_QUERY_FRAME_LENGTH   (8)
#define UART_QUERY_CHECKSUM_INDEX (0x07)

union tofsense_operating_param {
	uint8_t raw_byte;
	struct {
		uint8_t can_mode: 1;
		uint8_t query_mode: 1;
		uint8_t range: 2;
		uint8_t io_mode: 1;
		uint8_t reserved: 3;
	};
};

/* Device configuration frame */
struct tofsense_configuration_frame {
	uint8_t header;      /* Fixed 0x54 => UART_CONFIG_HEADER_BYTE */
	uint8_t length;      /* Fixed 0x20 => UART_CONFIG_FRAME_LENGTH */
	uint8_t op_code;     /* 0: Apply new settings 1: Read settings */
	uint8_t reserved[1]; /* 0xFF */
	tofsense_id_t id;    /* Sensor ID */
	uint32_t system_time;
	union tofsense_operating_param operating;
	uint8_t reserved2[2]; /* 0xFFFF */
	struct {
		unsigned int baudrate: 24; /* For CAN or UART */
	} __packed;
	uint8_t fov_X; /* Field of view X */
	uint8_t fov_Y; /* Field of view Y */
	uint8_t fov_X_offset;
	uint8_t fov_Y_offset;
	uint16_t band_start;  /* IO mode trigger 0 */
	uint16_t band_width;  /* IO mode trigger 1 */
	uint8_t reserved3[8]; /* 0xFFFFFFFF */
	uint8_t sum_check;
} __packed;

BUILD_ASSERT(sizeof(struct tofsense_configuration_frame) == UART_CONFIG_FRAME_LENGTH,
	     "structure tofsense_configuration_frame has invalid size !");

/* Data part of the raw UART frame (All fields in little endian) */
struct uart_data {
	uint8_t reserved_0;
	uint8_t id;
	uint32_t system_time;
	struct tofsense_distance distance;
	uint16_t signal_strength;
	uint8_t reserved_1;
} __packed;

/* Raw UART frame received from the module (All fields in little endian) */
struct tofsense_uart_data_frame {
	uint8_t header;
	uint8_t function_mark;
	struct uart_data data;
	uint8_t sum_check;
} __packed;

BUILD_ASSERT(sizeof(struct tofsense_uart_data_frame) == UART_DATA_FRAME_LENGTH,
	     "struct tofsense_uart_data_frame has invalid size !");

/* Raw UART frame sent to query the module in QUERY mode (All fields in little endian) */
struct tofsense_uart_query_data_frame {
	uint8_t header;
	uint8_t function_mark;
	uint16_t reserved_0;
	uint8_t id;
	uint16_t reserved_1;
	uint8_t sum_check;
} __packed;

BUILD_ASSERT(sizeof(struct tofsense_uart_query_data_frame) == UART_QUERY_FRAME_LENGTH,
	     "structure tofsense_uart_query_data_frame has invalid size !");

#endif /* ZEPHYR_DRIVERS_SENSOR_NOOPLOOP_TOFSENSE_UART_H_ */
