/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __NET_TLS_INTERNAL_H
#define __NET_TLS_INTERNAL_H

#include <net/net_context.h>
#include <net/net_tls.h>

#if defined(CONFIG_NET_TLS) || defined(CONFIG_NET_DTLS)

#if defined(CONFIG_MBEDTLS)

#if !defined(CONFIG_MBEDTLS_CFG_FILE)
#include "mbedtls/config.h"
#else
#include CONFIG_MBEDTLS_CFG_FILE
#endif /* CONFIG_MBEDTLS_CFG_FILE */

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <stdlib.h>
#define mbedtls_time_t       time_t
#define MBEDTLS_EXIT_SUCCESS EXIT_SUCCESS
#define MBEDTLS_EXIT_FAILURE EXIT_FAILURE
#endif /* MBEDTLS_PLATFORM_C */

#include <mbedtls/ssl_cookie.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/x509.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/ssl.h>
#include <mbedtls/error.h>
#include <mbedtls/debug.h>

#endif /* CONFIG_MBEDTLS */

/** A list of secure tags that context should use */
struct sec_tag_list {
	/** An array of secure tags referencing TLS credentials */
	sec_tag_t sec_tags[CONFIG_NET_MAX_CREDENTIALS_NUMBER];

	/** Number of configured secure tags */
	int sec_tag_count;
};

/** TLS context information. */
struct net_tls {
	/** Network context back pointer. */
	struct net_context *context;

	/** TLS specific option values. */
	struct {
		/** Select which credentials to use with TLS. */
		struct sec_tag_list sec_tag_list;
	} options;

#if defined(CONFIG_MBEDTLS)
	mbedtls_ssl_context ssl;
	mbedtls_ssl_config config;
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	mbedtls_x509_crt ca_chain;
#endif /* MBEDTLS_X509_CRT_PARSE_C */
#endif /* CONFIG_MBEDTLS */

	/** Intermediated buffer to store decrypted content. */
	char rx_ssl_buf[64];

	/** Currently processed packet. */
	struct net_pkt *rx_pkt;

	/** Offset in the currently processed packet. */
	int rx_offset;

	/** TLS packet fifo. */
	struct k_fifo rx_fifo;

	/** Receive callback for TLS */
	net_context_recv_cb_t tls_cb;
};

/**
 * @brief Initialize TLS module.
 */
void net_tls_init(void);

/**
 * @brief Allocate TLS context.
 *
 * @param context A pointer to net_context structure that allocates TLS context.
 *
 * @return A pointer to allocated TLS context. NULL if no context was available.
 */
struct net_tls *net_tls_alloc(struct net_context *context);

/**
 * @brief Release TLS context.
 *
 * @param tls TLS context to deallocate.
 *
 * @return 0 if ok, <0 if error.
 */
int net_tls_release(struct net_tls *tls);

int net_tls_enable(struct net_context *context, bool enabled);
int net_tls_connect(struct net_context *context, bool listening);
int net_tls_send(struct net_pkt *pkt);
int net_tls_recv(struct net_context *context, net_context_recv_cb_t cb,
		 void *user_data);
int net_tls_sec_tag_list_get(struct net_context *context, sec_tag_t *sec_tags,
			     int *sec_tag_cnt);
int net_tls_sec_tag_list_set(struct net_context *context,
			     const sec_tag_t *sec_tags, int sec_tag_cnt);

#else
static inline void net_tls_init(void)
{
}

static inline struct net_tls *net_tls_alloc(struct net_context *context)
{
	ARG_UNUSED(context);

	return NULL;
}

static inline int net_tls_release(struct net_tls *tls)
{
	ARG_UNUSED(tls);

	return 0;
}

static inline int net_tls_enable(struct net_context *context, bool enabled)
{
	ARG_UNUSED(context);
	ARG_UNUSED(enabled);

	return 0;
}

static inline int net_tls_connect(struct net_context *context, bool listening)
{
	ARG_UNUSED(context);
	ARG_UNUSED(listening);

	return 0;
}

static inline int net_tls_send(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return 0;
}

static inline int net_tls_recv(struct net_context *context,
			       net_context_recv_cb_t cb, void *user_data)
{
	ARG_UNUSED(context);
	ARG_UNUSED(cb);
	ARG_UNUSED(user_data);

	return 0;
}

static inline int net_tls_sec_tag_list_get(struct net_context *context,
					   sec_tag_t *sec_tags,
					   int *sec_tag_cnt)
{
	ARG_UNUSED(context);
	ARG_UNUSED(sec_tags);
	ARG_UNUSED(sec_tag_cnt);

	return 0;
}

static inline int net_tls_sec_tag_list_set(struct net_context *context,
					   const sec_tag_t *sec_tags,
					   int sec_tag_cnt)
{
	ARG_UNUSED(context);
	ARG_UNUSED(sec_tags);
	ARG_UNUSED(sec_tag_cnt);

	return 0;
}

#endif /* CONFIG_NET_TLS || CONFIG_NET_DTLS */

#endif /* __NET_TLS_INTERNAL_H */
