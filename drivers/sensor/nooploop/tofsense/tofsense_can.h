/*
 * Copyright (c) 2025 Alex Fabre
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_NOOPLOOP_TOFSENSE_CAN_H_
#define ZEPHYR_DRIVERS_SENSOR_NOOPLOOP_TOFSENSE_CAN_H_

/* Incoming TOFSense data frame ID: 0x200 + module ID */
#define CAN_TOFSENSE_RECEIVE_ID_BASE (0x200)

/* Outcomming TOFSense query ID. All TOFSense configured in CAN Query are listening to this ID */
#define CAN_TOFSENSE_QUERY_ID (0x402)

/* CAN data length in bytes */
#define CAN_DATA_FRAME_LENGTH (8)

/* CAN query data length in bytes */
#define CAN_QUERY_FRAME_LENGTH (8)

/* Data part of the CAN frame received from the module (All fields in little endian) */
struct tofsense_can_data_frame {
	struct tofsense_distance distance;
	uint16_t signal_strength;
	uint16_t reserved;
} __packed;

BUILD_ASSERT(sizeof(struct tofsense_can_data_frame) == CAN_DATA_FRAME_LENGTH,
	     "struct 'tofsense_can_data_frame' has invalid size !");

/* Raw CAN frame sent to query the module in QUERY mode (All fields in little endian) */
struct tofsense_can_query_data_frame {
	uint16_t reserved_0;
	uint8_t reserved_1;
	uint8_t id;
	uint32_t reserved_2;
} __packed;

BUILD_ASSERT(sizeof(struct tofsense_can_query_data_frame) == CAN_QUERY_FRAME_LENGTH,
	     "structure 'tofsense_can_query_data_frame' has invalid size !");

#endif /* ZEPHYR_DRIVERS_SENSOR_NOOPLOOP_TOFSENSE_CAN_H_ */
