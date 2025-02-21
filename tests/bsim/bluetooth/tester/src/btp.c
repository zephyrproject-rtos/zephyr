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
#include <zephyr/net_buf.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys_clock.h>

#include "babblekit/testcase.h"

#include "btp/btp.h"

LOG_MODULE_REGISTER(bsim_btp, CONFIG_BTTESTER_LOG_LEVEL);

static uint8_t *btp_buffer;
static size_t btp_buffer_len;
static uart_pipe_recv_cb btp_cb;

K_FIFO_DEFINE(btp_rsp_fifo);
NET_BUF_POOL_FIXED_DEFINE(btp_rsp_pool, 1, BTP_MTU, 0, NULL);
K_FIFO_DEFINE(btp_evt_fifo);
NET_BUF_POOL_FIXED_DEFINE(btp_evt_pool, 100, BTP_MTU, 0, NULL);

static void wait_for_response(const struct btp_hdr *cmd_hdr)
{
	const struct btp_hdr *rsp_hdr;
	struct net_buf *buf;

	buf = k_fifo_get(&btp_rsp_fifo, K_SECONDS(1));
	TEST_ASSERT(buf != NULL);

	rsp_hdr = (struct btp_hdr *)buf->data;
	TEST_ASSERT(rsp_hdr->len <= BTP_MTU, "len %u > %d", rsp_hdr->len, BTP_MTU);

	LOG_DBG("rsp service %u and opcode %u len %u", cmd_hdr->service, cmd_hdr->opcode,
		rsp_hdr->len);

	TEST_ASSERT(rsp_hdr->service == cmd_hdr->service && rsp_hdr->opcode == cmd_hdr->opcode);

	net_buf_unref(buf);
}

/* BTP communication is inspired from `uart_pipe_rx` to achieve a similar API */
void bsim_btp_send_to_tester(const uint8_t *data, size_t len)
{
	const struct btp_hdr *cmd_hdr;
	size_t offset = len;

	TEST_ASSERT(data != NULL);
	TEST_ASSERT(btp_buffer != NULL);
	TEST_ASSERT(len <= btp_buffer_len, "len %zu > %zu", len, btp_buffer_len);
	TEST_ASSERT(len >= sizeof(*cmd_hdr), "len %zu <= %zu", len, sizeof(*cmd_hdr));

	memcpy(btp_buffer, data, len);

	cmd_hdr = (const struct btp_hdr *)data;
	LOG_DBG("cmd service %u and opcode %u", cmd_hdr->service, cmd_hdr->opcode);
	btp_buffer = btp_cb(btp_buffer, &offset);
	TEST_ASSERT(offset == 0U);

	wait_for_response(cmd_hdr);
}

void bsim_btp_register_tester(uint8_t *buffer, size_t len, uart_pipe_recv_cb cb)
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

void bsim_btp_send_to_bsim(const uint8_t *data, size_t len)
{
	static struct net_buf *buf;
	bool full_evt;

	TEST_ASSERT(len >= 0);
	TEST_ASSERT(data != NULL);
	TEST_ASSERT(len <= BTP_MTU - sizeof(struct btp_hdr), "len %zu < %d", len, BTP_MTU);
	TEST_ASSERT(buf != NULL || (buf == NULL && len == sizeof(struct btp_hdr)), "len %zu buf %p",
		    len, buf);

	/* tester_send_with_index always sends the header first, and then any additional information
	 * afterwards. We thus need to reassemble the full event.
	 */
	if (buf == NULL) {
		const struct btp_hdr *hdr = (const struct btp_hdr *)data;

		LOG_DBG("hdr service %u and opcode %u", hdr->service, hdr->opcode);
		TEST_ASSERT(hdr->opcode != BTP_STATUS, "hdr service %u", hdr->service);

		if (hdr->opcode < BTP_EVENT_OPCODE) {
			buf = net_buf_alloc(&btp_rsp_pool, K_NO_WAIT);
			TEST_ASSERT(buf != NULL);
		} else {
			buf = net_buf_alloc(&btp_evt_pool, K_NO_WAIT);
			if (buf == NULL) {
				/* Discard the oldest event */
				buf = k_fifo_get(&btp_evt_fifo, K_NO_WAIT);
				TEST_ASSERT(buf != NULL);
				net_buf_unref(buf);

				buf = net_buf_alloc(&btp_evt_pool, K_NO_WAIT);
				TEST_ASSERT(buf != NULL);
			}
		}

		full_evt = hdr->len == 0; /* we are not awaiting additional data if hdr->len == 0*/
	} else {
		/* If buf != NULL then we have received the hdr and this is the additional data for
		 * the event/response. We always assume that we receive the rest here
		 */
		const struct btp_hdr *hdr = (const struct btp_hdr *)buf->data;

		TEST_ASSERT(buf != NULL && len == hdr->len, "len %zu hdr len %u", len, hdr->len);
		full_evt = true;
	}

	net_buf_add_mem(buf, data, len);

	if (full_evt) {
		const struct btp_hdr *hdr = (const struct btp_hdr *)buf->data;

		if (hdr->opcode < BTP_EVENT_OPCODE) {
			k_fifo_put(&btp_rsp_fifo, buf);
		} else {
			k_fifo_put(&btp_evt_fifo, buf);
		}

		buf = NULL;
	}
}

void bsim_btp_wait_for_evt(uint8_t service, uint8_t opcode, struct net_buf **out_buf)
{
	LOG_DBG("Waiting for evt with service %u and opcode %u", service, opcode);

	while (true) {
		const struct btp_hdr *hdr;
		struct net_buf *buf;

		buf = k_fifo_get(&btp_evt_fifo, K_FOREVER);
		TEST_ASSERT(buf != NULL);

		LOG_HEXDUMP_DBG(buf->data, buf->len, "evt");
		hdr = net_buf_pull_mem(buf, sizeof(struct btp_hdr));

		/* TODO: Verify length of event based on the service and opcode */
		if (hdr->service == service && hdr->opcode == opcode) {
			if (out_buf != NULL) {
				/* If the caller provides an out_buf, they are responsible for
				 * unref'ing it
				 */
				*out_buf = net_buf_ref(buf);
			}

			net_buf_unref(buf);
			return;
		}

		net_buf_unref(buf);
	}
}
