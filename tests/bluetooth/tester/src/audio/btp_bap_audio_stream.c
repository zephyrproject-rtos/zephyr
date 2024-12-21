/* btp_bap_audio_stream.c - Bluetooth BAP Tester */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <stddef.h>
#include <errno.h>

#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#define LOG_MODULE_NAME bttester_bap_audio_stream
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);
#include "btp/btp.h"
#include "btp_bap_audio_stream.h"

NET_BUF_POOL_FIXED_DEFINE(tx_pool, MAX(CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT,
			  CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT),
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

RING_BUF_DECLARE(audio_ring_buf, CONFIG_BT_ISO_TX_MTU);

#define ISO_DATA_THREAD_STACK_SIZE 512
#define ISO_DATA_THREAD_PRIORITY -7
K_THREAD_STACK_DEFINE(iso_data_thread_stack_area, ISO_DATA_THREAD_STACK_SIZE);
static struct k_work_q iso_data_work_q;
static bool send_worker_inited;

static inline struct bt_bap_stream *audio_stream_to_bap_stream(struct btp_bap_audio_stream *stream)
{
	return &stream->cap_stream.bap_stream;
}

static void audio_clock_timeout(struct k_work *work)
{
	struct btp_bap_audio_stream *stream;
	struct k_work_delayable *dwork;

	dwork = k_work_delayable_from_work(work);
	stream = CONTAINER_OF(dwork, struct btp_bap_audio_stream, audio_clock_work);
	atomic_inc(&stream->seq_num);

	k_work_schedule(dwork, K_USEC(audio_stream_to_bap_stream(stream)->qos->interval));
}

static void audio_send_timeout(struct k_work *work)
{
	struct bt_iso_tx_info info;
	struct btp_bap_audio_stream *stream;
	struct bt_bap_stream *bap_stream;
	struct k_work_delayable *dwork;
	struct net_buf *buf;
	uint32_t size;
	uint8_t *data;
	int err;

	dwork = k_work_delayable_from_work(work);
	stream = CONTAINER_OF(dwork, struct btp_bap_audio_stream, audio_send_work);
	bap_stream = audio_stream_to_bap_stream(stream);

	if (stream->last_req_seq_num % 201 == 200) {
		err = bt_bap_stream_get_tx_sync(bap_stream, &info);
		if (err != 0) {
			LOG_DBG("Failed to get last seq num: err %d", err);
		} else if (stream->last_req_seq_num > info.seq_num) {
			LOG_DBG("Previous TX request rejected by the controller: requested seq %u,"
				" last accepted seq %u", stream->last_req_seq_num, info.seq_num);
			stream->last_sent_seq_num = info.seq_num;
		} else {
			LOG_DBG("Host and Controller sequence number is in sync.");
			stream->last_sent_seq_num = info.seq_num;
		}
		/* TODO: Synchronize the Host clock with the Controller clock */
	}

	/* Get buffer within a ring buffer memory */
	size = ring_buf_get_claim(&audio_ring_buf, &data, bap_stream->qos->sdu);
	if (size > 0) {
		buf = net_buf_alloc(&tx_pool, K_NO_WAIT);
		if (!buf) {
			LOG_ERR("Cannot allocate net_buf. Dropping data.");
		} else {
			net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
			net_buf_add_mem(buf, data, size);

			/* Because the seq_num field of the audio_stream struct is atomic_val_t
			 * (4 bytes), let's allow an overflow and just cast it to uint16_t.
			 */
			stream->last_req_seq_num = (uint16_t)atomic_get(&stream->seq_num);

			LOG_DBG("Sending data to stream %p len %d seq %d", bap_stream, size,
				stream->last_req_seq_num);

			err = bt_bap_stream_send(bap_stream, buf, 0);
			if (err != 0) {
				LOG_ERR("Failed to send audio data to stream %p, err %d",
					bap_stream, err);
				net_buf_unref(buf);
			}
		}

		/* Free ring buffer memory */
		err = ring_buf_get_finish(&audio_ring_buf, size);
		if (err != 0) {
			LOG_ERR("Error freeing ring buffer memory: %d", err);
		}
	}

	k_work_schedule_for_queue(&iso_data_work_q, dwork,
				  K_USEC(bap_stream->qos->interval));
}

void btp_bap_audio_stream_started(struct btp_bap_audio_stream *a_stream)
{
	struct bt_bap_ep_info info;
	struct bt_bap_stream *bap_stream = audio_stream_to_bap_stream(a_stream);

	/* Callback called on transition to Streaming state */

	LOG_DBG("Started stream %p", bap_stream);

	(void)bt_bap_ep_get_info(bap_stream->ep, &info);
	if (info.can_send == true) {
		/* Schedule first TX ISO data at seq_num 1 instead of 0 to ensure
		 * we are in sync with the controller at start of streaming.
		 */
		a_stream->seq_num = 1;

		/* Run audio clock work in system work queue */
		k_work_init_delayable(&a_stream->audio_clock_work, audio_clock_timeout);
		k_work_schedule(&a_stream->audio_clock_work, K_NO_WAIT);

		/* Run audio send work in user defined work queue */
		k_work_init_delayable(&a_stream->audio_send_work, audio_send_timeout);
		k_work_schedule_for_queue(&iso_data_work_q, &a_stream->audio_send_work,
					  K_USEC(bap_stream->qos->interval));
	}
}

void btp_bap_audio_stream_stopped(struct btp_bap_audio_stream *a_stream)
{
	/* Stop send timer */
	k_work_cancel_delayable(&a_stream->audio_clock_work);
	k_work_cancel_delayable(&a_stream->audio_send_work);
}

uint8_t btp_bap_audio_stream_send_data(const uint8_t *data, uint8_t data_len)
{
	return ring_buf_put(&audio_ring_buf, data, data_len);
}

uint8_t btp_bap_audio_stream_send(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	struct btp_bap_send_rp *rp = rsp;
	const struct btp_bap_send_cmd *cp = cmd;
	uint32_t ret;

	ret = btp_bap_audio_stream_send_data(cp->data, cp->data_len);

	rp->data_len = ret;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

int btp_bap_audio_stream_init_send_worker(void)
{
	if (send_worker_inited) {
		return 0;
	}

	k_work_queue_init(&iso_data_work_q);
	k_work_queue_start(&iso_data_work_q, iso_data_thread_stack_area,
			   K_THREAD_STACK_SIZEOF(iso_data_thread_stack_area),
			   ISO_DATA_THREAD_PRIORITY, NULL);

	send_worker_inited = true;

	return 0;
}
