/*
 * Copyright (c) 2024 NXP
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/modem/pipe.h>
#include <zephyr/modem/ubx/protocol.h>

#ifndef ZEPHYR_MODEM_UBX_
#define ZEPHYR_MODEM_UBX_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Modem Ubx
 * @defgroup modem_ubx Modem Ubx
 * @since 3.7
 * @version 1.0.0
 * @ingroup modem
 * @{
 */

struct modem_ubx;

typedef void (*modem_ubx_match_callback)(struct modem_ubx *ubx,
					 const struct ubx_frame *frame,
					 size_t len,
					 void *user_data);

struct modem_ubx_match {
	struct ubx_frame_match filter;
	modem_ubx_match_callback handler;
};

#define MODEM_UBX_MATCH_ARRAY_DEFINE(_name, ...)						   \
	struct modem_ubx_match _name[] = {__VA_ARGS__};

#define MODEM_UBX_MATCH_DEFINE(_class_id, _msg_id, _handler)					   \
{												   \
	.filter = {										   \
		.class = _class_id,								   \
		.id = _msg_id,									   \
	},											   \
	.handler = _handler,									   \
}

struct modem_ubx_script {
	struct {
		const void *buf;
		uint16_t len;
	} request;
	struct {
		uint8_t *buf;
		uint16_t buf_len;
		uint16_t received_len;
	} response;
	struct modem_ubx_match match;
	uint16_t retry_count;
	k_timeout_t timeout;
};

struct modem_ubx {
	void *user_data;
	atomic_t attached;
	uint8_t *receive_buf;
	uint16_t receive_buf_size;
	uint16_t receive_buf_offset;
	struct modem_ubx_script *script;
	struct modem_pipe *pipe;
	struct k_work process_work;
	struct k_sem script_stopped_sem;
	struct k_sem script_running_sem;
	struct {
		const struct modem_ubx_match *array;
		size_t size;
	} unsol_matches;
};

struct modem_ubx_config {
	void *user_data;
	uint8_t *receive_buf;
	uint16_t receive_buf_size;
	struct {
		const struct modem_ubx_match *array;
		size_t size;
	} unsol_matches;
};

/**
 * @brief Attach pipe to Modem Ubx
 *
 * @param ubx Modem Ubx instance
 * @param pipe Pipe instance to attach Modem Ubx instance to
 * @returns 0 if successful
 * @returns negative errno code if failure
 * @note Modem Ubx instance is enabled if successful
 */
int modem_ubx_attach(struct modem_ubx *ubx, struct modem_pipe *pipe);

/**
 * @brief Release pipe from Modem Ubx instance
 *
 * @param ubx Modem Ubx instance
 */
void modem_ubx_release(struct modem_ubx *ubx);

/**
 * @brief Initialize Modem Ubx instance
 * @param ubx Modem Ubx instance
 * @param config Configuration which shall be applied to the Modem Ubx instance
 * @note Modem Ubx instance must be attached to a pipe instance
 */
int modem_ubx_init(struct modem_ubx *ubx, const struct modem_ubx_config *config);

/**
 * @brief Writes the ubx frame in script.request and reads back its response (if available)
 * @details For each ubx frame sent, the device responds in 0, 1 or both of the following ways:
 *	1. The device sends back a UBX-ACK frame to denote 'acknowledge' and 'not-acknowledge'.
 *		Note: the message id of UBX-ACK frame determines whether the device acknowledged.
 *		Ex: when we send a UBX-CFG frame, the device responds with a UBX-ACK frame.
 *	2. The device sends back the same frame that we sent to it, with it's payload populated.
 *		It's used to get the current configuration corresponding to the frame that we sent.
 *		Ex: frame types such as "get" or "poll" ubx frames respond this way.
 *		This response (if received) is written to script.response.
 *
 *	This function writes the ubx frame in script.request then reads back it's response.
 *	If script.match is not NULL, then every ubx frame received from the device is compared with
 *	script.match to check if a match occurred. This could be used to match UBX-ACK frame sent
 *	from the device by populating script.match with UBX-ACK that the script expects to receive.
 *
 *	The script terminates when either of the following happens:
 *		1. script.match is successfully received and matched.
 *		2. timeout (denoted by script.timeout) occurs.
 * @param ubx Modem Ubx instance
 * @param script Script to be executed
 * @note The length of ubx frame in the script.request should not exceed UBX_FRAME_SZ_MAX
 * @note Modem Ubx instance must be attached to a pipe instance
 * @returns 0 if successful
 * @returns negative errno code if failure
 */
int modem_ubx_run_script(struct modem_ubx *ubx, struct modem_ubx_script *script);

int modem_ubx_run_script_for_each(struct modem_ubx *ubx, struct modem_ubx_script *script,
				  struct ubx_frame *array, size_t array_size);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_MODEM_UBX_ */
