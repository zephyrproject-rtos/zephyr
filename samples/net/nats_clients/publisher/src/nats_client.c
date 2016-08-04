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

#include "nats_client.h"
#include "nats_pack.h"

#include <errno.h>

int nats_connect(struct nats_clapp_ctx_t *ctx, char *client_name, int verbose)
{
	int rc;

	ctx->nats->name = client_name;
	ctx->nats->verbose = verbose ? 1 : 0;

	rc = netz_tcp(ctx->netz_ctx);
	if (rc != 0) {
		return -EIO;
	}

	rc = netz_rx(ctx->netz_ctx, ctx->rx_buf);
	if (rc != 0) {
		return -EIO;
	}

	rc = nats_unpack_info(ctx->rx_buf);
	if (rc != 0) {
		return -EINVAL;
	}

	rc = nats_pack_quickcon(ctx->tx_buf, client_name, verbose);
	if (rc != 0) {
		return -EINVAL;
	}

	rc = netz_tx(ctx->netz_ctx, ctx->tx_buf);
	if (rc != 0) {
		return -EIO;
	}

	return nats_read_ok(ctx);
}

int nats_pub(struct nats_clapp_ctx_t *ctx, char *subject, char *reply_to,
	     char *payload)
{
	int rc;

	rc = nats_pack_pub(ctx->tx_buf, subject, reply_to, payload);
	if (rc != 0) {
		return -EINVAL;
	}

	rc = netz_tx(ctx->netz_ctx, ctx->tx_buf);
	if (rc != 0) {
		return -EIO;
	}

	return nats_read_ok(ctx);
}

int nats_sub(struct nats_clapp_ctx_t *ctx, char *subject, char *queue_grp,
	     char *sid)
{
	int rc;

	rc = nats_pack_sub(ctx->tx_buf, subject, queue_grp, sid);
	if (rc != 0) {
		return -EINVAL;
	}

	rc = netz_tx(ctx->netz_ctx, ctx->tx_buf);
	if (rc != 0) {
		return -EIO;
	}

	return nats_read_ok(ctx);
}

int nats_unsub(struct nats_clapp_ctx_t *ctx, char *sid, int max_msgs)
{
	int rc;

	rc = nats_pack_unsub(ctx->tx_buf, sid, max_msgs);
	if (rc != 0) {
		return -EINVAL;
	}

	rc = netz_tx(ctx->netz_ctx, ctx->tx_buf);
	if (rc != 0) {
		return -EIO;
	}

	return nats_read_ok(ctx);
}

int nats_read_ok(struct nats_clapp_ctx_t *ctx)
{
	int rc;

	if (ctx->nats->verbose) {
		rc = netz_rx(ctx->netz_ctx, ctx->rx_buf);
		if (rc != 0) {
			return -EIO;
		}

		rc = nats_find_msg(ctx->rx_buf, "+OK");
		if (rc != 0) {
			return -EINVAL;
		}
	}
	return 0;
}

int nats_ping_pong(struct nats_clapp_ctx_t *ctx)
{
	int rc;

	rc = nats_pack_ping(ctx->tx_buf);
	if (rc != 0) {
		return -EINVAL;
	}

	rc = netz_tx(ctx->netz_ctx, ctx->tx_buf);
	if (rc != 0) {
		return -EIO;
	}

	rc = netz_rx(ctx->netz_ctx, ctx->rx_buf);
	if (rc != 0) {
		return -EIO;
	}

	rc = nats_unpack_pong(ctx->rx_buf);
	if (rc != 0) {
		return -EINVAL;
	}

	return 0;
}
