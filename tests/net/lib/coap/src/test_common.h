/*
 * Copyright (c) 2026 Martin Schröder <info@swedishembedded.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_COMMON_H_
#define TEST_COMMON_H_

#include <errno.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/net/coap.h>
#include <zephyr/net/coap_client.h>
#include <zephyr/net/coap/coap_link_format.h>

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#include "net_private.h"

#if defined(CONFIG_COAP_OSCORE)
#include "coap_oscore.h"
#include <zephyr/net/coap/coap_service.h>
#include <oscore.h>
#include <oscore/security_context.h>
#include <common/oscore_edhoc_error.h>
#endif

#if defined(CONFIG_COAP_EDHOC)
#include "coap_edhoc.h"
#include "coap_edhoc_session.h"
#include "coap_oscore_ctx_cache.h"

/* Forward declarations for wrapper functions */
extern int coap_oscore_context_init_wrapper(void *ctx,
					     const uint8_t *master_secret,
					     size_t master_secret_len,
					     const uint8_t *master_salt,
					     size_t master_salt_len,
					     const uint8_t *sender_id,
					     size_t sender_id_len,
					     const uint8_t *recipient_id,
					     size_t recipient_id_len,
					     int aead_alg,
					     int hkdf_alg);
#endif

/* Common macros */
#define COAP_BUF_SIZE 128

#define NUM_PENDINGS 3
#define NUM_OBSERVERS 3
#define NUM_REPLIES 3

#define MY_PORT 12345
#define peer_addr { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, \
			0, 0, 0, 0, 0, 0, 0, 0x2 } } }

#define COAP_ROLLOVER_AGE (1 << 23)
#define COAP_MAX_AGE      0xffffff
#define COAP_FIRST_AGE    2

/* Shared data declarations */
extern struct coap_pending pendings[NUM_PENDINGS];
extern struct coap_observer observers[NUM_OBSERVERS];
extern struct coap_reply replies[NUM_REPLIES];

extern struct net_sockaddr_in6 dummy_addr;
extern uint8_t data_buf[2][COAP_BUF_SIZE];

extern struct coap_resource server_resources[];
extern const char * const server_resource_1_path[];
extern const char *const server_resource_2_path[];

/* This is exposed for this test in subsys/net/lib/coap/coap_link_format.c */
bool _coap_match_path_uri(const char * const *path,
			  const char *uri, uint16_t len);

/* Exposed from subsys/net/lib/coap */
extern bool coap_age_is_newer(int v1, int v2);

/* Shared helper functions */
bool ipaddr_cmp(const struct net_sockaddr *a, const struct net_sockaddr *b);

#endif /* TEST_COMMON_H_ */
