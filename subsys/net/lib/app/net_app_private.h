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

#if defined(MBEDTLS_DEBUG_C)
#include <mbedtls/debug.h>
/* - Debug levels (from ext/lib/crypto/mbedtls/include/mbedtls/debug.h)
 *    - 0 No debug
 *    - 1 Error
 *    - 2 State change
 *    - 3 Informational
 *    - 4 Verbose
 */
#if defined(CONFIG_NET_DEBUG_APP_TLS_LEVEL)
#define DEBUG_THRESHOLD CONFIG_NET_DEBUG_APP_TLS_LEVEL
#else
#define DEBUG_THRESHOLD 0
#endif /* CONFIG_NET_DEBUG_APP_TLS_LEVEL */
#endif

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
int _net_app_set_local_addr(struct sockaddr *addr, const char *myaddr,
			    u16_t port);
int _net_app_set_net_ctx(struct net_app_ctx *ctx,
			 struct net_context *net_ctx,
			 struct sockaddr *addr,
			 socklen_t socklen,
			 enum net_ip_protocol proto);
int _net_app_config_local_ctx(struct net_app_ctx *ctx,
			      enum net_sock_type sock_type,
			      enum net_ip_protocol proto,
			      struct sockaddr *addr);
struct net_context *_net_app_select_net_ctx(struct net_app_ctx *ctx);
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

#if defined(CONFIG_NET_APP_SERVER)
void _net_app_accept_cb(struct net_context *net_ctx,
			struct sockaddr *addr,
			socklen_t addrlen,
			int status, void *data);
#endif /* CONFIG_NET_APP_SERVER */

#if defined(CONFIG_NET_APP_CLIENT)
#endif /* CONFIG_NET_APP_CLIENT */

#if defined(CONFIG_NET_APP_TLS)
void _net_app_tls_handler_stop(struct net_app_ctx *ctx);
int _net_app_tls_init(struct net_app_ctx *ctx, int client_or_server);
int _net_app_entropy_source(void *data, unsigned char *output, size_t len,
			    size_t *olen);
int _net_app_ssl_tx(void *context, const unsigned char *buf, size_t size);
#endif /* CONFIG_NET_APP_TLS */

#endif /* CONFIG_NET_APP_SERVER || CONFIG_NET_APP_CLIENT */
