/*
 * Copyright (c) 2024 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef COAP_UTILS_H
#define COAP_UTILS_H

#include <zephyr/net/openthread.h>
#include <openthread/coap.h>

#define COAP_MAX_BUF_SIZE   128
#define COAP_DEVICE_ID_SIZE 25

typedef int (*coap_req_handler_put)(void *ctx, uint8_t *buf, int size);
typedef int (*coap_req_handler_get)(void *ctx, otMessage *msg, const otMessageInfo *msg_info);

int coap_init(void);
int coap_req_handler(void *ctx, otMessage *msg, const otMessageInfo *msg_info,
		     coap_req_handler_put put_fn, coap_req_handler_get get_fn);
int coap_resp_send(otMessage *req, const otMessageInfo *req_info, uint8_t *buf, int len);
int coap_put_req_send(const char *addr, const char *uri, uint8_t *buf, int len,
		      otCoapResponseHandler handler, void *ctx);
int coap_get_req_send(const char *addr, const char *uri, uint8_t *buf, int len,
		      otCoapResponseHandler handler, void *ctx);
const char *coap_device_id(void);
int coap_get_data(otMessage *msg, void *buf, int *len);

#endif /* COAP_UTILS_H */
