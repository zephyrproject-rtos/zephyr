/*
 * Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_MP_PLUGINS_IPC_PROTOCOL_H_
#define ZEPHYR_SUBSYS_MP_PLUGINS_IPC_PROTOCOL_H_

#include <stdint.h>

#define MAX_SERIALIZED_PAYLOAD 64

/**
 * @brief IPC Message Types
 */
enum ipc_msg_type {
	IPC_MSG_CMD_STATE_CHANGE = 0,
	IPC_MSG_CMD_SET_PROP,
	IPC_MSG_DATA_BUFFER,
	IPC_MSG_DATA_RELEASE,
	IPC_MSG_EVENT,         /* Downstream or Upstream */
	IPC_MSG_QUERY_REQ,     /* Request from P1 -> P2 */
	IPC_MSG_QUERY_RESP,    /* Response from P2 -> P1 */
	IPC_MSG_BUS_MESSAGE,   /* Async notifications from P2 -> P1 */
	IPC_MSG_CMD_CREATE_PIPELINE,
	IPC_MSG_CMD_DESTROY_PIPELINE
};

/**
 * @brief IPC Message Structure
 */
struct mp_ipc_msg {
	uint32_t type;         /* enum ipc_msg_type */
	union {
		/* State Change */
		struct {
			uint32_t state;
		} state_cmd;

		/* Property Set */
		struct {
			uint32_t element_id;
			uint32_t prop_id;
			uint32_t prop_val;
		} prop_cmd;
		
		/* Data Buffer passing */
		struct {
			uint32_t phys_addr;
			uint32_t size;
			uint32_t timestamp;
			uint32_t buffer_id;
		} data;
		
		/* Data Buffer release */
		struct {
			uint32_t buffer_id;
		} release;

		/* Event */
		struct {
			uint8_t event_type;
			uint8_t payload[MAX_SERIALIZED_PAYLOAD];
		} event;

		/* Query (Request and Response) */
		struct {
			uint8_t query_type;
			bool success; /* Used in RESP */
			uint8_t payload[MAX_SERIALIZED_PAYLOAD];
		} query;

		/* Bus Message */
		struct {
			uint32_t msg_type;
			uint8_t payload[MAX_SERIALIZED_PAYLOAD];
		} bus_msg;
	};
};

#endif /* ZEPHYR_SUBSYS_MP_PLUGINS_IPC_PROTOCOL_H_ */
