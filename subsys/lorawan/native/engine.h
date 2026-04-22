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

enum lwan_req_type {
	LWAN_REQ_JOIN,
	LWAN_REQ_SEND,
};

struct lwan_req {
	/* Request type (join or send) */
	enum lwan_req_type type;
	/* Pointer to lwan_join_req or lwan_send_req */
	void *data;
};

void engine_init(struct lwan_ctx *ctx);
int engine_post_req(const struct lwan_req *req);
int engine_wait_join_result(void);
int engine_wait_send_result(void);

/* Called by mac_do_join / mac_do_send to unblock the waiting caller */
void engine_signal_join_result(int result);
void engine_signal_send_result(int result);

#ifdef __cplusplus
}
#endif

#endif /* SUBSYS_LORAWAN_NATIVE_ENGINE_H_ */
