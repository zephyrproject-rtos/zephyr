/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/modem/pipe.h>

#ifndef ZEPHYR_MODEM_UBX_
#define ZEPHYR_MODEM_UBX_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Modem Ubx
 * @defgroup modem_ubx Modem Ubx
 * @ingroup modem
 * @{
 */

#define UBX_FRM_HEADER_SZ			6
#define UBX_FRM_FOOTER_SZ			2
#define UBX_FRM_SZ_WITHOUT_PAYLOAD		(UBX_FRM_HEADER_SZ + UBX_FRM_FOOTER_SZ)
#define UBX_FRM_SZ(payload_size)		(payload_size + UBX_FRM_SZ_WITHOUT_PAYLOAD)

#define UBX_PREAMBLE_SYNC_CHAR_1		0xB5
#define UBX_PREAMBLE_SYNC_CHAR_2		0x62

#define UBX_FRM_PREAMBLE_SYNC_CHAR_1_IDX	0
#define UBX_FRM_PREAMBLE_SYNC_CHAR_2_IDX	1
#define UBX_FRM_MSG_CLASS_IDX			2
#define UBX_FRM_MSG_ID_IDX			3
#define UBX_FRM_PAYLOAD_SZ_L_IDX		4
#define UBX_FRM_PAYLOAD_SZ_H_IDX		5
#define UBX_FRM_PAYLOAD_IDX			6
#define UBX_FRM_CHECKSUM_START_IDX		2
#define UBX_FRM_CHECKSUM_STOP_IDX(frame_len)	(frame_len - 2)

#define UBX_PAYLOAD_SZ_MAX			256
#define UBX_FRM_SZ_MAX				UBX_FRM_SZ(UBX_PAYLOAD_SZ_MAX)

struct ubx_frame {
	uint8_t preamble_sync_char_1;
	uint8_t preamble_sync_char_2;
	uint8_t message_class;
	uint8_t message_id;
	uint8_t payload_size_low;
	uint8_t payload_size_high;
	uint8_t payload_and_checksum[];
};

struct modem_ubx_script {
	struct ubx_frame *request;
	struct ubx_frame *response;
	struct ubx_frame *match;

	uint16_t retry_count;
	k_timeout_t timeout;
};

struct modem_ubx {
	void *user_data;

	atomic_t state;

	uint8_t *receive_buf;
	uint16_t receive_buf_size;

	uint8_t *work_buf;
	uint16_t work_buf_size;
	uint16_t work_buf_len;
	bool ubx_preamble_sync_chars_received;

	const struct modem_ubx_script *script;

	struct modem_pipe *pipe;

	struct k_work send_work;
	struct k_work process_work;
	struct k_sem script_stopped_sem;
	struct k_sem script_running_sem;
};

struct modem_ubx_config {
	void *user_data;

	uint8_t *receive_buf;
	uint16_t receive_buf_size;
	uint8_t *work_buf;
	uint16_t work_buf_size;
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
 * @note The length of ubx frame in the script.request should not exceed UBX_FRM_SZ_MAX
 * @note Modem Ubx instance must be attached to a pipe instance
 * @returns 0 if device acknowledged via UBX-ACK and no "get" response was received
 * @returns positive integer denoting the length of "get" response that was received
 * @returns negative errno code if failure
 */
int modem_ubx_run_script(struct modem_ubx *ubx, const struct modem_ubx_script *script);

/**
 * @brief Initialize ubx frame
 * @param ubx_frame Ubx frame buffer
 * @param ubx_frame_size Ubx frame buffer size
 * @param msg_cls Message class
 * @param msg_id Message id
 * @param payload Payload buffer
 * @param payload_size Payload buffer size
 * @returns positive integer denoting the length of the ubx frame created
 * @returns negative errno code if failure
 */
int modem_ubx_create_frame(uint8_t *ubx_frame, uint16_t ubx_frame_size, uint8_t msg_cls,
			   uint8_t msg_id, const void *payload, uint16_t payload_size);
/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_MODEM_UBX_ */
