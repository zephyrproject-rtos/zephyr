/*
 * Copyright (c) 2024 NXP
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/modem/ubx.h>
#include <zephyr/sys/check.h>

#include "modem_workqueue.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modem_ubx, CONFIG_MODEM_MODULES_LOG_LEVEL);

static void modem_ubx_pipe_callback(struct modem_pipe *pipe,
				    enum modem_pipe_event event,
				    void *user_data)
{
	struct modem_ubx *ubx = (struct modem_ubx *)user_data;

	if (event == MODEM_PIPE_EVENT_RECEIVE_READY) {
		modem_work_submit(&ubx->process_work);
	}
}

int modem_ubx_run_script(struct modem_ubx *ubx, struct modem_ubx_script *script)
{
	int ret;
	bool wait_for_rsp = script->match.filter.class != 0;

	ret = k_sem_take(&ubx->script_running_sem, script->timeout);
	if (ret != 0) {
		return -EBUSY;
	}

	ubx->script = script;
	k_sem_reset(&ubx->script_stopped_sem);

	int tries = ubx->script->retry_count + 1;
	int32_t ms_per_attempt = (uint64_t)k_ticks_to_ms_floor64(script->timeout.ticks) / tries;

	do {
		ret = modem_pipe_transmit(ubx->pipe,
					  (const uint8_t *)ubx->script->request.buf,
					  ubx->script->request.len);

		if (wait_for_rsp) {
			ret = k_sem_take(&ubx->script_stopped_sem, K_MSEC(ms_per_attempt));
		}
		tries--;
	} while ((tries > 0) && (ret < 0));

	k_sem_give(&ubx->script_running_sem);

	return (ret > 0) ? 0 : ret;
}

enum ubx_process_result {
	UBX_PROCESS_RESULT_NO_DATA_FOUND,
	UBX_PROCESS_RESULT_FRAME_INCOMPLETE,
	UBX_PROCESS_RESULT_FRAME_FOUND
};

static inline enum ubx_process_result process_incoming_data(const uint8_t *data,
							    size_t len,
							    const struct ubx_frame **frame_start,
							    size_t *frame_len,
							    size_t *iterator)
{
	for (int i  = (*iterator) ; i < len ; i++) {
		if (data[i] == UBX_PREAMBLE_SYNC_CHAR_1) {

			const struct ubx_frame *frame = (const struct ubx_frame *)&data[i];
			size_t remaining_bytes = len - i;

			/* Wait until we've got the full header to keep processing data */
			if (UBX_FRAME_HEADER_SZ > remaining_bytes) {
				*frame_start = frame;
				*frame_len = remaining_bytes;
				return UBX_PROCESS_RESULT_FRAME_INCOMPLETE;
			}

			/* Filter false-positive: Sync-byte 1 contained in payload */
			if (frame->preamble_sync_char_2 != UBX_PREAMBLE_SYNC_CHAR_2) {
				continue;
			}

			/* Invalid length filtering */
			if (UBX_FRAME_SZ(frame->payload_size) > UBX_FRAME_SZ_MAX) {
				continue;
			}

			/* Check if we should wait until packet is completely received */
			if (UBX_FRAME_SZ(frame->payload_size) > remaining_bytes) {
				*frame_start = frame;
				*frame_len = remaining_bytes;
				return UBX_PROCESS_RESULT_FRAME_INCOMPLETE;
			}

			/* We should have all the packet, so we validate checksum. */
			uint16_t valid_checksum = ubx_calc_checksum(frame,
								UBX_FRAME_SZ(frame->payload_size));
			uint16_t ck_a = frame->payload_and_checksum[frame->payload_size];
			uint16_t ck_b = frame->payload_and_checksum[frame->payload_size + 1];
			uint16_t actual_checksum = ck_a | (ck_b << 8);

			if (valid_checksum != actual_checksum) {
				continue;
			}

			*frame_start = frame;
			*frame_len = UBX_FRAME_SZ(frame->payload_size);

			*iterator = i + 1;
			return UBX_PROCESS_RESULT_FRAME_FOUND;
		}
	}

	return UBX_PROCESS_RESULT_NO_DATA_FOUND;
}

static inline bool matches_filter(const struct ubx_frame *frame,
				  const struct ubx_frame_match *filter)
{
	if ((frame->class == filter->class) &&
	    (frame->id == filter->id) &&
	    ((filter->payload.len == 0) ||
	     ((frame->payload_size == filter->payload.len) &&
	      (0 == memcmp(frame->payload_and_checksum,
			   filter->payload.buf,
			   filter->payload.len))))) {
		return true;
	} else {
		return false;
	}
}

static void modem_ubx_process_handler(struct k_work *item)
{
	struct modem_ubx *ubx = CONTAINER_OF(item, struct modem_ubx, process_work);
	int ret;

	ret = modem_pipe_receive(ubx->pipe,
				 &ubx->receive_buf[ubx->receive_buf_offset],
				 (ubx->receive_buf_size - ubx->receive_buf_offset));

	const uint8_t *received_data = ubx->receive_buf;
	size_t length = ret > 0 ? (ret + ubx->receive_buf_offset) : 0;
	const struct ubx_frame *frame = NULL;
	size_t frame_len = 0;
	size_t iterator = 0;
	enum ubx_process_result process_result;

	do {
		process_result = process_incoming_data(received_data, length,
						       &frame, &frame_len,
						       &iterator);
		switch (process_result) {
		case UBX_PROCESS_RESULT_FRAME_FOUND:
			/** Serve script first */
			if (matches_filter(frame, &ubx->script->match.filter)) {
				memcpy(ubx->script->response.buf, frame, frame_len);
				ubx->script->response.received_len = frame_len;

				k_sem_give(&ubx->script_stopped_sem);
			}
			/** Check for unsolicited matches */
			for (size_t i = 0 ; i < ubx->unsol_matches.size ; i++) {
				if (ubx->unsol_matches.array[i].handler &&
				    matches_filter(frame, &ubx->unsol_matches.array[i].filter)) {
					ubx->unsol_matches.array[i].handler(ubx, frame, frame_len,
									    ubx->user_data);
				}
			}
			break;
		case UBX_PROCESS_RESULT_FRAME_INCOMPLETE:
			/** If we had an incomplete packet, discard prior data
			 * and offset next pipe-receive to process remaining
			 * info.
			 */
			memcpy(ubx->receive_buf, frame, frame_len);
			ubx->receive_buf_offset = frame_len;
			break;
		case UBX_PROCESS_RESULT_NO_DATA_FOUND:
			ubx->receive_buf_offset = 0;
			break;
		default:
			CODE_UNREACHABLE;
		}
	} while (process_result == UBX_PROCESS_RESULT_FRAME_FOUND);
}

int modem_ubx_attach(struct modem_ubx *ubx, struct modem_pipe *pipe)
{
	if (atomic_test_and_set_bit(&ubx->attached, 0) == true) {
		return 0;
	}

	ubx->pipe = pipe;
	modem_pipe_attach(ubx->pipe, modem_ubx_pipe_callback, ubx);
	k_sem_give(&ubx->script_running_sem);

	return 0;
}

void modem_ubx_release(struct modem_ubx *ubx)
{
	struct k_work_sync sync;

	if (atomic_test_and_clear_bit(&ubx->attached, 0) == false) {
		return;
	}

	modem_pipe_release(ubx->pipe);
	k_work_cancel_sync(&ubx->process_work, &sync);
	k_sem_reset(&ubx->script_stopped_sem);
	k_sem_reset(&ubx->script_running_sem);
	ubx->pipe = NULL;
}

int modem_ubx_init(struct modem_ubx *ubx, const struct modem_ubx_config *config)
{
	__ASSERT_NO_MSG(ubx != NULL);
	__ASSERT_NO_MSG(config != NULL);
	__ASSERT_NO_MSG(config->receive_buf != NULL);
	__ASSERT_NO_MSG(config->receive_buf_size > 0);

	memset(ubx, 0x00, sizeof(*ubx));
	ubx->user_data = config->user_data;

	ubx->receive_buf = config->receive_buf;
	ubx->receive_buf_size = config->receive_buf_size;

	ubx->pipe = NULL;

	ubx->unsol_matches.array = config->unsol_matches.array;
	ubx->unsol_matches.size = config->unsol_matches.size;

	k_work_init(&ubx->process_work, modem_ubx_process_handler);
	k_sem_init(&ubx->script_stopped_sem, 0, 1);
	k_sem_init(&ubx->script_running_sem, 1, 1);

	return 0;
}
