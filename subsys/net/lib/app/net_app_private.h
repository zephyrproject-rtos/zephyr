/** @file
 * @brief Private net_api API routines
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Print extra info about received TLS data */
#define RX_EXTRA_DEBUG 0

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
#include <mbedtls/memory_buffer_alloc.h>
#endif

#if defined(MBEDTLS_ERROR_C)
#define _net_app_print_error(fmt, ret)					\
	do {								\
		char error[80];						\
									\
		mbedtls_strerror(ret, error, sizeof(error));		\
									\
		NET_ERR(fmt " (%s)", -ret, error);			\
	} while (0)
#else
#define _net_app_print_error(fmt, ret) NET_ERR(fmt, -ret)
#endif

/* Direction of the packet (sending / receiving) */
enum _net_app_dir {
	NET_APP_PKT_UNKNOWN = 0,
	NET_APP_PKT_TX = 1,
	NET_APP_PKT_RX = 2,
};

#define BUF_ALLOC_TIMEOUT 100

#if defined(CONFIG_NET_DEBUG_APP)
void _net_app_print_info(struct net_app_ctx *ctx);
#else
#define _net_app_print_info(...)
#endif /* CONFIG_NET_DEBUG_APP */

#if defined(CONFIG_NET_APP_SERVER) || defined(CONFIG_NET_APP_CLIENT)
char *_net_app_sprint_ipaddr(char *buf, int buflen,
			     const struct sockaddr *addr);
void _net_app_received(struct net_context *net_ctx,
		       struct net_pkt *pkt,
		       int status,
		       void *user_data);
int _net_app_set_local_addr(struct net_app_ctx *ctx, struct sockaddr *addr,
			    const char *myaddr, u16_t port);
int _net_app_set_net_ctx(struct net_app_ctx *ctx,
			 struct net_context *net_ctx,
			 struct sockaddr *addr,
			 socklen_t socklen,
			 enum net_ip_protocol proto);
int _net_app_config_local_ctx(struct net_app_ctx *ctx,
			      enum net_sock_type sock_type,
			      enum net_ip_protocol proto,
			      struct sockaddr *addr);

#if NET_LOG_ENABLED > 0
struct net_context *_net_app_select_net_ctx_debug(struct net_app_ctx *ctx,
						  const struct sockaddr *dst,
						  const char *caller,
						  int line);
#define _net_app_select_net_ctx(ctx, dst)				\
	_net_app_select_net_ctx_debug(ctx, dst, __func__, __LINE__)
#else
struct net_context *_net_app_select_net_ctx(struct net_app_ctx *ctx,
					    const struct sockaddr *dst);
#endif

int _net_app_ssl_mux(void *context, unsigned char *buf, size_t size);
int _net_app_tls_sendto(struct net_pkt *pkt,
			const struct sockaddr *dst_addr,
			socklen_t addrlen,
			net_context_send_cb_t cb,
			s32_t timeout,
			void *token,
			void *user_data);
void _net_app_tls_received(struct net_context *context,
			   struct net_pkt *pkt,
			   int status,
			   void *user_data);
int _net_app_ssl_mainloop(struct net_app_ctx *ctx);
int _net_app_tls_trigger_close(struct net_app_ctx *ctx);

#if defined(CONFIG_NET_APP_SERVER)
void _net_app_accept_cb(struct net_context *net_ctx,
			struct sockaddr *addr,
			socklen_t addrlen,
			int status, void *data);
#endif /* CONFIG_NET_APP_SERVER */

#if defined(CONFIG_NET_APP_CLIENT)
#endif /* CONFIG_NET_APP_CLIENT */

#if defined(CONFIG_NET_APP_TLS) || defined(CONFIG_NET_APP_DTLS)
bool _net_app_server_tls_enable(struct net_app_ctx *ctx);
bool _net_app_server_tls_disable(struct net_app_ctx *ctx);
void _net_app_tls_handler_stop(struct net_app_ctx *ctx);
int _net_app_tls_init(struct net_app_ctx *ctx, int client_or_server);
int _net_app_entropy_source(void *data, unsigned char *output, size_t len,
			    size_t *olen);
int _net_app_ssl_tx(void *context, const unsigned char *buf, size_t size);
#endif /* CONFIG_NET_APP_TLS || CONFIG_NET_APP_DTLS */

#if defined(CONFIG_NET_APP_DTLS)
#include "../../ip/connection.h"
enum net_verdict _net_app_dtls_established(struct net_conn *conn,
					   struct net_pkt *pkt,
					   void *user_data);
#endif /* CONFIG_NET_APP_DTLS */

#if defined(CONFIG_NET_DEBUG_APP)
void _net_app_register(struct net_app_ctx *ctx);
void _net_app_unregister(struct net_app_ctx *ctx);
#else
#define _net_app_register(...)
#define _net_app_unregister(...)
#endif /* CONFIG_NET_DEBUG_APP */

#endif /* CONFIG_NET_APP_SERVER || CONFIG_NET_APP_CLIENT */
