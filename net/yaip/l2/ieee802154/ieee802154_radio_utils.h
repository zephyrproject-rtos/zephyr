/*
 * Copyright (c) 2016 Intel Corporation.
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

#ifndef __IEEE802154_RADIO_UTILS_H__
#define __IEEE802154_RADIO_UTILS_H__

static inline bool prepare_for_ack(struct ieee802154_context *ctx,
				   struct net_buf *buf)
{
	if (ieee802154_ack_required(buf)) {
		ctx->ack_received = false;
		nano_sem_init(&ctx->ack_lock);

		return true;
	}

	return false;
}

static inline int wait_for_ack(struct ieee802154_context *ctx,
			       bool ack_required)
{
	if (!ack_required) {
		return 0;
	}

	if (nano_sem_take(&ctx->ack_lock, MSEC(10)) == 0) {
		/*
		 * We reinit the semaphore in case aloha_radio_handle_ack
		 * got called multiple times.
		 */
		nano_sem_init(&ctx->ack_lock);
	}

	return ctx->ack_received ? 0 : -EIO;
}

static inline int handle_ack(struct ieee802154_context *ctx,
			     struct net_buf *buf)
{
	if (buf->len == IEEE802154_ACK_PKT_LENGTH) {
		ctx->ack_received = true;
		nano_sem_give(&ctx->ack_lock);

		return NET_OK;
	}

	return NET_CONTINUE;
}

#endif /* __IEEE802154_RADIO_UTILS_H__ */
