/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr.h>
#include <stdio.h>
#include <errno.h>

#include "netz.h"

#include "nats_client.h"

#define STACK_SIZE		1024
uint8_t stack[STACK_SIZE];

/* Change this value to modify the size of the tx and rx buffers	*/
#define BUF_SIZE		256
uint8_t tx_raw_buf[BUF_SIZE];
uint8_t rx_raw_buf[BUF_SIZE];

#define SLEEP_TIME		30

#define RC_STR(rc)		(rc == 0 ? "OK" : "ERROR")

int handle_msg(struct nats_clapp_ctx_t *ctx);
/* Unpacks and prints the NATS MSG message */
int unpack_msg(struct app_buf_t *buf);

void fiber(void)
{
	/* tx_buf and rx_buf are application-level buffers		*/
	struct app_buf_t tx_buf = APP_BUF_INIT(tx_raw_buf,
					       sizeof(tx_raw_buf), 0);
	struct app_buf_t rx_buf = APP_BUF_INIT(rx_raw_buf,
					       sizeof(rx_raw_buf), 0);

	/* netz context is initialized with default values. See netz.h	*/
	struct netz_ctx_t netz_ctx = NETZ_CTX_INIT;

	struct nats_cl_ctx_t nats_client = NATS_CL_INIT;

	struct nats_clapp_ctx_t nats = NATS_CLAPP_INIT(&nats_client, &netz_ctx,
						       &tx_buf, &rx_buf);

	int rc;

	/* First we configure network related stuff			*/
	netz_host_ipv4(&netz_ctx, 192, 168, 1, 110);
	netz_netmask_ipv4(&netz_ctx, 255, 255, 255, 0);
	/* NATS server address and port					*/
	netz_remote_ipv4(&netz_ctx, 192, 168, 1, 10, 4222);

	rc = nats_connect(&nats, "zephyr", 1);
	if (rc != 0) {
		printf("[%s:%d] Unable to connect to NATS server: %d\n",
		       __func__, __LINE__, rc);
		return;
	}
	rc = nats_sub(&nats, "sensors", NULL, "z1");
	if (rc != 0) {
		printf("[%s:%d] Unable to subscribe: %d\n",
		       __func__, __LINE__, rc);
		return;
	}

	do {
		printf("--------------------------------\n");
		handle_msg(&nats);
		fiber_sleep(SLEEP_TIME);
	} while (1);

}

void main(void)
{
	net_init();

	task_fiber_start(stack, STACK_SIZE, (nano_fiber_entry_t)fiber,
			 0, 0, 7, 0);
}


int handle_msg(struct nats_clapp_ctx_t *ctx)
{
	int rc;

	rc = netz_rx(ctx->netz_ctx, ctx->rx_buf);
	if (rc != 0) {
		return -EIO;
	}

	/* ping pong */
	rc = nats_unpack_ping(ctx->rx_buf);
	if (rc == 0) {
		rc = nats_pack_pong(ctx->tx_buf);
		if (rc != 0) {
			return -EINVAL;
		}

		rc = netz_tx(ctx->netz_ctx, ctx->tx_buf);
		if (rc != 0) {
			return -EIO;
		}

		printf("Ping-pong message processed\n");
		return 0;
	}
	/* msg */
	rc = unpack_msg(ctx->rx_buf);

	return rc;
}

int unpack_msg(struct app_buf_t *buf)
{
	char *str;
	int subject_start;
	int subject_len;
	int sid_start;
	int sid_len;
	int reply_start;
	int reply_len;
	int payload_start;
	int payload_len;
	int rc;

	str = (char *)buf->buf;

	rc = nats_unpack_msg(buf, &subject_start, &subject_len,
			     &sid_start, &sid_len, &reply_start, &reply_len,
			     &payload_start, &payload_len);
	if (rc != 0) {
		return rc;
	}

	printf("Subject: [%d] %.*s\n", subject_len, subject_len,
	       str + subject_start);
	printf("Sid: [%d] %.*s\n", sid_len, sid_len, str + sid_start);

	if (reply_start > 0) {
		printf("Reply-to: [%d] %.*s\n", reply_len,
		       reply_len, str + reply_start);
	} else {
		printf("Reply-to: not found\n");
	}
	printf("Payload: [%d] %.*s\n", payload_len, payload_len,
	       str + payload_start);

	return 0;
}
