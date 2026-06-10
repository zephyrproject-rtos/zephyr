/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SUBSYS_LORAWAN_NATIVE_ENGINE_H_
#define SUBSYS_LORAWAN_NATIVE_ENGINE_H_

#include <zephyr/kernel.h>
#include <zephyr/lorawan/lorawan.h>

struct lwan_ctx;

#ifdef __cplusplus
extern "C" {
#endif

struct lwan_join_req {
	/* 8-byte device EUI (big-endian) */
	const uint8_t *dev_eui;
	/* 8-byte join EUI (big-endian) */
	const uint8_t *join_eui;
	/* 16-byte network key */
	const uint8_t *nwk_key;
	/* Device nonce for replay protection */
	uint16_t dev_nonce;
};

struct lwan_send_req {
	/* Application payload */
	const uint8_t *data;
	/* Payload length in bytes */
	uint8_t len;
	/* LoRaWAN FPort (1-223) */
	uint8_t port;
	/* Confirmed or unconfirmed */
	enum lorawan_message_type type;
};

struct lwan_set_datarate_req {
	enum lorawan_datarate dr;
};

struct lwan_enable_adr_req {
	bool enable;
};

struct lwan_set_conf_msg_tries_req {
	uint8_t tries;
};

struct lwan_set_channels_mask_req {
	const uint16_t *channels_mask;
	size_t channels_mask_size;
};

enum lwan_req_type {
	LWAN_REQ_JOIN,
	LWAN_REQ_SEND,
	LWAN_REQ_SET_DATARATE,
	LWAN_REQ_ENABLE_ADR,
	LWAN_REQ_SET_CONF_MSG_TRIES,
	LWAN_REQ_SET_CHANNELS_MASK,
};

struct lwan_req {
	/* Request type */
	enum lwan_req_type type;
	/* Pointer to request payload for @p type */
	void *data;
	/* Caller-owned completion for synchronous requests */
	struct k_sem *done;
	/* Caller-owned result storage */
	int *result;
};

#define LWAN_REQ(_type, _data) \
	((struct lwan_req){ \
		.type = (_type), \
		.data = (_data), \
	})

void engine_init(struct lwan_ctx *ctx);
int engine_post_req_wait(struct lwan_req *req);

/* Called by MAC handlers to unblock the waiting caller. */
void engine_signal_result(const struct lwan_req *req, int result);

#ifdef __cplusplus
}
#endif

#endif /* SUBSYS_LORAWAN_NATIVE_ENGINE_H_ */
