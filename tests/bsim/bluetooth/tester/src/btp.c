/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/drivers/uart_pipe.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys_clock.h>

#include "babblekit/testcase.h"

#include "btp/btp.h"

LOG_MODULE_REGISTER(bsim_btp, CONFIG_BTTESTER_LOG_LEVEL);

static uint8_t *btp_buffer;
static size_t btp_buffer_len;
static uart_pipe_recv_cb btp_cb;

K_MSGQ_DEFINE(btp_rsp_queue, BTP_MTU, 2, 4);
K_MSGQ_DEFINE(btp_evt_queue, BTP_MTU, 100, 4);

static void wait_for_response(const struct btp_hdr *cmd_hdr)
{
	const struct btp_hdr *rsp_hdr;
	uint8_t rsp_buffer[BTP_MTU + sizeof(*rsp_hdr)];
	int err;

	err = k_msgq_get(&btp_rsp_queue, rsp_buffer, K_SECONDS(1));
	TEST_ASSERT(err == 0, "err %d", err);

	rsp_hdr = (struct btp_hdr *)rsp_buffer;
	TEST_ASSERT(rsp_hdr->len <= BTP_MTU, "len %u > %d", rsp_hdr->len, BTP_MTU);

	LOG_DBG("rsp service %u and opcode %u len %u", cmd_hdr->service, cmd_hdr->opcode,
		rsp_hdr->len);

	TEST_ASSERT(rsp_hdr->service == cmd_hdr->service && rsp_hdr->opcode == cmd_hdr->opcode);
}

/* BTP communication is inspired from `uart_pipe_rx` to achieve a similar API */
void btp_send_to_tester(const uint8_t *data, size_t len)
{
	const struct btp_hdr *cmd_hdr;
	size_t offset = len;

	TEST_ASSERT(data != NULL);
	TEST_ASSERT(btp_buffer != NULL);
	TEST_ASSERT(len <= btp_buffer_len, "len %zu > %zu", len, btp_buffer_len);
	TEST_ASSERT(len > sizeof(*cmd_hdr), "len %zu <= %zu", len, sizeof(*cmd_hdr));

	memcpy(btp_buffer, data, len);

	cmd_hdr = (const struct btp_hdr *)data;
	LOG_DBG("cmd service %u and opcode %u", cmd_hdr->service, cmd_hdr->opcode);
	btp_buffer = btp_cb(btp_buffer, &offset);
	TEST_ASSERT(offset == 0U);

	wait_for_response(cmd_hdr);
}

void btp_register_tester(uint8_t *buffer, size_t len, uart_pipe_recv_cb cb)
{
	TEST_ASSERT(cb != NULL);
	TEST_ASSERT(buffer != NULL);
	TEST_ASSERT(len > sizeof(struct btp_hdr), "len %zu", len);
	TEST_ASSERT(len <= BTP_MTU, "len %zu > %d", len, BTP_MTU);

	btp_buffer = buffer;
	btp_buffer_len = len;
	btp_cb = cb;

	LOG_INF("btp registered with %p %zu %p", buffer, len, cb);
}

void btp_send_to_bsim(const uint8_t *data, size_t len)
{
	static const struct btp_hdr *hdr;
	static uint8_t buffer[BTP_MTU] = {};
	bool full_evt;

	TEST_ASSERT(len >= 0);
	TEST_ASSERT(data != NULL);
	TEST_ASSERT(len <= BTP_MTU - sizeof(*hdr), "len %zu < %d", len, BTP_MTU);
	TEST_ASSERT((hdr == NULL && len == sizeof(*hdr)) || (hdr != NULL && len == hdr->len),
		    "len %zu hdr %p (%u)", len, hdr, hdr == NULL ? 0 : hdr->len);

	/* tester_send_with_index always sends the header first, and then any additional information
	 * afterwards. We thus need to reassemble the full event.
	 */
	if (hdr == NULL) {
		hdr = (const struct btp_hdr *)data;
		LOG_DBG("hdr service %u and opcode %u", hdr->service, hdr->opcode);

		memcpy(buffer, data, len);
		full_evt = hdr->len == 0; /* we are not awaiting additional data if hdr->len == 0*/
	} else {
		full_evt = true;
		memcpy(buffer + sizeof(*hdr), data, len);
	}

	if (full_evt) {
		TEST_ASSERT(hdr->opcode != BTP_STATUS, "hdr service %u", hdr->service);

		if (hdr->opcode < BTP_EVENT_OPCODE) {
			int err;

			err = k_msgq_put(&btp_rsp_queue, buffer, K_NO_WAIT);
			TEST_ASSERT(err == 0, "err %d", err);
		} else {
			if (k_msgq_put(&btp_evt_queue, buffer, K_NO_WAIT) != 0) {
				int err;

				/* Discard the oldest message and put the new message */
				err = k_msgq_get(&btp_evt_queue, NULL, K_NO_WAIT);
				TEST_ASSERT(err == 0, "err %d", err);
				err = k_msgq_put(&btp_evt_queue, buffer, K_NO_WAIT);
				TEST_ASSERT(err == 0, "err %d", err);
			}
		}

		hdr = NULL;
	}
}

void btp_wait_for_evt(uint8_t service, uint8_t opcode, uint8_t evt_buffer[BTP_MTU])
{
	LOG_DBG("Waiting for evt with service %u and opcode %u", service, opcode);

	while (true) {
		const struct btp_hdr *hdr;
		int err;

		err = k_msgq_get(&btp_evt_queue, evt_buffer, K_FOREVER);
		TEST_ASSERT(err == 0, "err %d", err);

		hdr = (struct btp_hdr *)evt_buffer;
		LOG_DBG("evt service %u and opcode %u", hdr->service, hdr->opcode);
		if (hdr->service == service && hdr->opcode == opcode) {
			return;
		}
	}
}
