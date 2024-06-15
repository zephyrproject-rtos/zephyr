/* btp_bap_audio_stream.h - Bluetooth BAP Tester */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/audio/cap.h>

struct btp_bap_audio_stream {
	struct bt_cap_stream cap_stream;
	atomic_t seq_num;
	uint16_t last_req_seq_num;
	uint16_t last_sent_seq_num;
	struct k_work_delayable audio_clock_work;
	struct k_work_delayable audio_send_work;
};

int btp_bap_audio_stream_init_send_worker(void);
void btp_bap_audio_stream_started(struct btp_bap_audio_stream *stream);
void btp_bap_audio_stream_stopped(struct btp_bap_audio_stream *a_stream);
uint8_t btp_bap_audio_stream_send(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len);
