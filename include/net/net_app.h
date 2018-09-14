/** @file
 * @brief Common routines needed in various network applications.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_NET_APP_H_
#define ZEPHYR_INCLUDE_NET_NET_APP_H_

#if defined(CONFIG_NET_APP_TLS) || defined(CONFIG_NET_APP_DTLS)
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
#endif /* CONFIG_NET_APP_TLS || CONFIG_NET_APP_DTLS */

#include <net/net_ip.h>
#include <net/net_pkt.h>
#include <net/net_context.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Network application library
 * @defgroup net_app Network Application Library
 * @ingroup networking
 * @{
 */

enum net_app_type {
	NET_APP_UNSPEC = 0,
	NET_APP_SERVER,
	NET_APP_CLIENT,
} __packed;

struct net_app_ctx;

/**
 * @typedef net_app_recv_cb_t
 * @brief Network data receive callback.
 *
 * @details The recv callback is called after a network data is
 * received.
 *
 * @param ctx The context to use.
 * @param pkt Network buffer that is received. If the pkt is not NULL,
 * then the callback will own the buffer and it needs to to unref the pkt
 * as soon as it has finished working with it.  On EOF, pkt will be NULL.
 * @param status Value is set to 0 if some data or the connection is
 * at EOF, <0 if there was an error receiving data, in this case the
 * pkt parameter is set to NULL.
 * @param user_data The user data given in init call.
 */
typedef void (*net_app_recv_cb_t)(struct net_app_ctx *ctx,
				  struct net_pkt *pkt,
				  int status,
				  void *user_data);

/**
 * @typedef net_app_connect_cb_t
 * @brief Connection callback.
 *
 * @details The connect callback is called after a connection is being
 * established.
 *
 * @param ctx The context to use.
 * @param status Status of the connection establishment. This is 0
 * if the connection was established successfully, <0 if there was an
 * error.
 * @param user_data The user data given in init call.
 */
typedef void (*net_app_connect_cb_t)(struct net_app_ctx *ctx,
				     int status,
				     void *user_data);

/**
 * @typedef net_app_send_cb_t
 * @brief Network data send callback.
 *
 * @details The send callback is called after a network data is
 * sent.
 *
 * @param ctx The context to use.
 * @param status Value is set to 0 if all data was sent ok, <0 if
 * there was an error sending data. >0 amount of data that was
 * sent when not all data was sent ok.
 * @param user_data_send The user data given in net_app_send() call.
 * @param user_data The user data given in init call.
 */
typedef void (*net_app_send_cb_t)(struct net_app_ctx *ctx,
				  int status,
				  void *user_data_send,
				  void *user_data);

/**
 * @typedef net_app_close_cb_t
 * @brief Close callback.
 *
 * @details The close callback is called after a connection is being
 * shutdown.
 *
 * @param ctx The context to use.
 * @param status Error code for the closing.
 * @param user_data The user data given in init call.
 */
typedef void (*net_app_close_cb_t)(struct net_app_ctx *ctx,
				   int status,
				   void *user_data);

/** Network application callbacks */
struct net_app_cb {
	/** Function that is called when a connection is established.
	 */
	net_app_connect_cb_t connect;

	/** Function that is called when data is received from network.
	 */
	net_app_recv_cb_t recv;

	/** Function that is called when net_pkt is sent.
	 */
	net_app_send_cb_t send;

	/** Function that is called when connection is shutdown.
	 */
	net_app_close_cb_t close;
};

/* This is the same prototype as what net_context_sendto() has
 * so that we can override the sending of the data for TLS traffic.
 */
typedef int (*net_app_send_data_t)(struct net_pkt *pkt,
				   const struct sockaddr *dst_addr,
				   socklen_t addrlen,
				   net_context_send_cb_t cb,
				   s32_t timeout,
				   void *token,
				   void *user_data);

#if defined(CONFIG_NET_APP_TLS) || defined(CONFIG_NET_APP_DTLS)
/* Internal information for managing TLS data */
struct tls_context {
	struct net_pkt *rx_pkt;
	struct net_buf *hdr; /* IP + UDP/TCP header */
	struct net_buf *frag;
	struct k_sem tx_sem;
	struct k_fifo tx_rx_fifo;
	int remaining;
#if defined(CONFIG_NET_APP_DTLS) && defined(CONFIG_NET_APP_SERVER)
	char client_id;
#endif
};

/* This struct is used to pass data to TLS thread when reading or sending
 * data.
 */
struct net_app_fifo_block {
	struct k_mem_block block;
	struct net_pkt *pkt;
	void *token; /* Used when sending data */
	net_context_send_cb_t cb;
	u8_t dir;
};

#define NET_APP_TLS_POOL_DEFINE(name, count)				\
	K_MEM_POOL_DEFINE(name, sizeof(struct net_app_fifo_block),	\
			  sizeof(struct net_app_fifo_block), count, sizeof(int))

#if defined(CONFIG_NET_APP_SERVER)
/**
 * @typedef net_app_cert_cb_t
 * @brief Callback used when the API user needs to setup the certs.
 *
 * @param ctx Net app context.
 * @param cert MBEDTLS certificate
 * @param pkey MBEDTLS private key
 *
 * @return 0 if ok, <0 if there is an error
 */
typedef int (*net_app_cert_cb_t)(struct net_app_ctx *ctx,
				 mbedtls_x509_crt *cert,
				 mbedtls_pk_context *pkey);
#endif /* CONFIG_NET_APP_SERVER */

#if defined(CONFIG_NET_APP_CLIENT)
/**
 * @typedef net_app_ca_cert_cb_t
 * @brief Callback used when the API user needs to setup certs.
 *
 * @param ctx Net app client context.
 * @param ca_cert MBEDTLS certificate. This is of type mbedtls_x509_crt
 * if MBEDTLS_X509_CRT_PARSE_C is defined.
 *
 * @return 0 if ok, <0 if there is an error
 */
typedef int (*net_app_ca_cert_cb_t)(struct net_app_ctx *ctx,
				    void *ca_cert);
#endif /* CONFIG_NET_APP_CLIENT */

/**
 * @typedef net_app_entropy_src_cb_t
 * @brief Callback used when the API user needs to setup the entropy source.
 * @details This is the same as mbedtls_entropy_f_source_ptr callback.
 *
 * @param data Callback-specific data pointer
 * @param output Data to fill
 * @param len Maximum size to provide
 * @param olen The actual amount of bytes put into the buffer (Can be 0)
 */
typedef int (*net_app_entropy_src_cb_t)(void *data, unsigned char *output,
					size_t len, size_t *olen);
#endif /* CONFIG_NET_APP_TLS || CONFIG_NET_APP_DTLS */

#if defined(CONFIG_NET_APP_DTLS)
struct dtls_timing_context {
	u32_t snapshot;
	u32_t int_ms;
	u32_t fin_ms;
};
#endif /* CONFIG_NET_APP_DTLS */

/* Information for the context and local/remote addresses used. */
struct net_app_endpoint {
	/** Network context. */
	struct net_context *ctx;

	/** Local address */
	struct sockaddr local;

	/** Remote address */
	struct sockaddr remote;
};

/** Network application context. */
struct net_app_ctx {
#if defined(CONFIG_NET_IPV6)
	struct net_app_endpoint ipv6;
#endif
#if defined(CONFIG_NET_IPV4)
	struct net_app_endpoint ipv4;
#endif

	/** What is the default endpoint for this context. */
	struct net_app_endpoint *default_ctx;

	/** Internal function that is called when user data is sent to
	 * network. By default this is set to net_context_sendto() but
	 * is overridden for TLS as it requires special handling.
	 */
	net_app_send_data_t send_data;

	/** Connection callbacks */
	struct net_app_cb cb;

	/** Internal function that is called when data is received from
	 * network. This will do what ever needed and then pass data to
	 * application.
	 */
	net_context_recv_cb_t recv_cb;

#if defined(CONFIG_NET_APP_DTLS)
	struct {
		/** Currently active network context. This will contain the
		 * new context that is created after connection is established
		 * when UDP and DTLS is used.
		 */
		struct net_context *ctx;

		/** DTLS final timer. Connection is terminated if this expires.
		 */
		struct k_delayed_work fin_timer;
	} dtls;
#endif

#if defined(CONFIG_NET_APP_SERVER)
	struct {
#if defined(CONFIG_NET_TCP)
		/** Currently active network contexts. This will contain the
		 * new contexts that are created after connection is accepted
		 * when TCP is enabled.
		 */
		struct net_context *net_ctxs[CONFIG_NET_APP_SERVER_NUM_CONN];
#endif
	} server;
#endif /* CONFIG_NET_APP_SERVER */

#if defined(CONFIG_NET_APP_CLIENT)
	struct {
		/** Connect waiter */
		struct k_sem connect_wait;

#if defined(CONFIG_DNS_RESOLVER)
		/** DNS resolver waiter */
		struct k_sem dns_wait;

		/** DNS query id. This is needed if the query needs to be
		 * cancelled.
		 */
		u16_t dns_id;
#endif
	} client;
#endif /* CONFIG_NET_APP_CLIENT */

#if defined(CONFIG_NET_APP_TLS) || defined(CONFIG_NET_APP_DTLS)
	struct {
		/** TLS stack for mbedtls library. */
		k_thread_stack_t *stack;

		/** TLS stack size. */
		int stack_size;

		/** TLS thread id */
		k_tid_t tid;

		/** TLS thread */
		struct k_thread thread;

		/** Memory pool for RX data */
		struct k_mem_pool *pool;

		/** Where the encrypted request is stored, this is to be
		 * provided by the user.
		 */
		u8_t *request_buf;

		/** Hostname to be used in the certificate verification */
		const char *cert_host;

		/** Request buffer maximum length */
		size_t request_buf_len;

		/** mbedtls related configuration. */
		struct {
#if defined(CONFIG_NET_APP_SERVER)
			net_app_cert_cb_t cert_cb;
			mbedtls_x509_crt srvcert;
			mbedtls_pk_context pkey;
#endif
#if defined(CONFIG_NET_APP_CLIENT)
			net_app_ca_cert_cb_t ca_cert_cb;
			mbedtls_x509_crt ca_cert;
#endif
			struct tls_context ssl_ctx;
			net_app_entropy_src_cb_t entropy_src_cb;
			mbedtls_entropy_context entropy;
			mbedtls_ctr_drbg_context ctr_drbg;
			mbedtls_ssl_context ssl;
			mbedtls_ssl_config conf;
#if defined(CONFIG_NET_APP_DTLS)
			mbedtls_ssl_cookie_ctx cookie_ctx;
			struct dtls_timing_context timing_ctx;
#endif
			u8_t *personalization_data;
			size_t personalization_data_len;
		} mbedtls;

		/** Have we called connect cb yet? */
		u8_t connect_cb_called : 1;

		/** User wants to close the connection */
		u8_t close_requested : 1;

		/** Is there TX pending? If there is then the close operation
		 * will be postponed after we have sent the data.
		 */
		u8_t tx_pending : 1;

		/** Is the TLS/DTLS handshake fully done */
		u8_t handshake_done : 1;

		/** Is the connection closing */
		u8_t connection_closing : 1;
	} tls;
#endif /* CONFIG_NET_APP_TLS || CONFIG_NET_APP_DTLS */

#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
	/** Network packet (net_pkt) memory pool for network contexts attached
	 * to this network app context.
	 */
	net_pkt_get_slab_func_t tx_slab;

	/** Network data net_buf pool for network contexts attached to this
	 * network app context.
	 */
	net_pkt_get_pool_func_t data_pool;
#endif

	/** User data pointer */
	void *user_data;

#if defined(CONFIG_NET_DEBUG_APP)
	/** Used when debugging with net-shell */
	sys_snode_t node;
#endif

	/** Type of the connection (stream or datagram) */
	enum net_sock_type sock_type;

	/** IP protocol type (UDP or TCP) */
	enum net_ip_protocol proto;

	/** Application type (client or server) of this instance */
	enum net_app_type app_type;

	/** Is this context setup or not */
	u8_t is_init : 1;

	/** Is this instance supporting TLS or not.
	 */
	u8_t is_tls : 1;

	/** Running status of the server. If true, then the server is enabled.
	 * If false then it is disabled and will not serve clients.
	 * The server is disabled by default after initialization and needs to
	 * be manually enabled in order to serve any requests.
	 */
	u8_t is_enabled : 1;

	/** Unused bits */
	u8_t _padding : 5;
};

#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
/**
 * @brief Configure the net_pkt pool for this context.
 *
 * @details Use of this function is optional and if the pools are not set,
 * then the default TX and DATA pools are used.
 *
 * @param tx_slab Function which is used when allocating TX network packet.
 * This can be NULL in which case default TX memory pool is used.
 * @param data_pool Function which is used when allocating data network buffer.
 * This can be NULL in which case default DATA net_buf pool is used.
 */
int net_app_set_net_pkt_pool(struct net_app_ctx *ctx,
			     net_pkt_get_slab_func_t tx_slab,
			     net_pkt_get_pool_func_t data_pool);
#else
#define net_app_set_net_pkt_pool(...)
#endif /* CONFIG_NET_CONTEXT_NET_PKT_POOL */

#if defined(CONFIG_NET_APP_SERVER)
/**
 * @brief Create a network server application.
 *
 * @details Note that caller must create the context and initialize it to 0
 * before calling this function. The context must be valid for the whole
 * duration of the application life cycle. This usually means that it
 * cannot be allocated from stack.
 *
 * @param ctx Network application context.
 * @param sock_type Connection type (stream or datagram).
 * @param proto IP protocol (UDP or TCP)
 * @param server_addr Local address of the server. If set to NULL, then the
 * API will figure out a proper address where to bind the context.
 * @param port UDP or TCP port number where the service is located. This is
 * only used if server_addr parameter is NULL.
 * @param timeout Timeout for this function call. This timeout tells how
 * long to wait while accepting the data from network.
 * @param user_data User specific data.
 *
 * @return 0 if ok, <0 if error.
 */
int net_app_init_server(struct net_app_ctx *ctx,
			enum net_sock_type sock_type,
			enum net_ip_protocol proto,
			struct sockaddr *server_addr,
			u16_t port,
			void *user_data);

/**
 * @brief Create a TCP server application.
 *
 * @details Note that caller must create the context and initialize it to 0
 * before calling this function. The context must be valid for the whole
 * duration of the application life cycle. This usually means that it
 * cannot be allocated from stack.
 *
 * @param ctx Network application context.
 * @param server_addr Local address of the server. If set to NULL, then the
 * API will figure out a proper address where to bind the context.
 * @param port UDP or TCP port number where the service is located. This is
 * only used if server_addr parameter is NULL.
 * @param timeout Timeout for this function call. This timeout tells how
 * long to wait while accepting the data from network.
 * @param user_data User specific data.
 *
 * @return 0 if ok, <0 if error.
 */
static inline int net_app_init_tcp_server(struct net_app_ctx *ctx,
					  struct sockaddr *server_addr,
					  u16_t port,
					  void *user_data)
{
	return net_app_init_server(ctx,
				   SOCK_STREAM,
				   IPPROTO_TCP,
				   server_addr,
				   port,
				   user_data);
}

/**
 * @brief Create a UDP server application.
 *
 * @details Note that caller must create the context and initialize it to 0
 * before calling this function. The context must be valid for the whole
 * duration of the application life cycle. This usually means that it
 * cannot be allocated from stack.
 *
 * @param ctx Network application context.
 * @param server_addr Local address of the server. If set to NULL, then the
 * API will figure out a proper address where to bind the context.
 * @param port UDP or TCP port number where the service is located. This is
 * only used if server_addr parameter is NULL.
 * @param timeout Timeout for this function call. This timeout tells how
 * long to wait while accepting the data from network.
 * @param user_data User specific data.
 *
 * @return 0 if ok, <0 if error.
 */
static inline int net_app_init_udp_server(struct net_app_ctx *ctx,
					  struct sockaddr *server_addr,
					  u16_t port,
					  void *user_data)
{
	return net_app_init_server(ctx,
				   SOCK_DGRAM,
				   IPPROTO_UDP,
				   server_addr,
				   port,
				   user_data);
}

/**
 * @brief Wait for an incoming connection.
 *
 * @details Note that caller must have called net_app_init_server() before
 * calling this function. This functionality is separated from init function
 * so that user can setup the callbacks after calling init. Only after calling
 * this function the server starts to listen connection attempts. This function
 * will not block but will initialize the local end point address so that
 * receive callback will be called for incoming connection.
 *
 * @param ctx Network application context.
 *
 * @return 0 if ok, <0 if error.
 */
int net_app_listen(struct net_app_ctx *ctx);

/**
 * @brief Enable server to serve connections.
 *
 * @details By default the server status is disabled.
 *
 * @param ctx Network application context.
 *
 * @return 0 if ok, <0 if error.
 */
bool net_app_server_enable(struct net_app_ctx *ctx);

/**
 * @brief Disable server so that it will not serve any clients.
 *
 * @param ctx Network application context.
 *
 * @return 0 if ok, <0 if error.
 */
bool net_app_server_disable(struct net_app_ctx *ctx);

#endif /* CONFIG_NET_APP_SERVER */

#if defined(CONFIG_NET_APP_CLIENT)
/**
 * @brief Create a network client application.
 *
 * @details Note that caller must create the context and initialize it to 0
 * before calling this function. The context must be valid for the whole
 * duration of the application life cycle. This usually means that it
 * cannot be allocated from stack.
 *
 * @param ctx Network application context.
 * @param sock_type Connection type (stream or datagram).
 * @param proto IP protocol (UDP or TCP)
 * @param client_addr Local address of the client. If set to NULL, then the
 * API will figure out a proper address where to bind the context.
 * @param peer_addr Peer (target) address. If this is NULL, then the
 * peer_add_str string and peer_port are used when connecting to peer.
 * @param peer_addr_str Peer (target) address as a string. If this is NULL,
 * then the peer_addr sockaddr is used to set the peer address. If DNS is
 * configured in the system, then the hostname is automatically resolved if
 * given here. Note that the port number is optional in the string. If the
 * port number is not given in the string, then peer_port variable is used
 * instead.
 * Following syntax is supported for the address:
 *      192.0.2.1
 *      192.0.2.1:5353
 *      2001:db8::1
 *      [2001:db8::1]:5353
 *      peer.example.com
 *      peer.example.com:1234
 * @param peer_port Port number where to connect to. Ignored if port number is
 * found in the peer_addr_str.
 * @param timeout Timeout for this function call. This is used if the API
 * needs to do some time consuming operation, like resolving DNS address.
 * @param user_data User specific data.
 *
 * @return 0 if ok, <0 if error.
 */
int net_app_init_client(struct net_app_ctx *ctx,
			enum net_sock_type sock_type,
			enum net_ip_protocol proto,
			struct sockaddr *client_addr,
			struct sockaddr *peer_addr,
			const char *peer_addr_str,
			u16_t peer_port,
			s32_t timeout,
			void *user_data);

/**
 * @brief Create a TCP client application.
 *
 * @details Note that caller must create the context and initialize it to 0
 * before calling this function. The context must be valid for the whole
 * duration of the application life cycle. This usually means that it
 * cannot be allocated from stack.
 *
 * @param ctx Network application context.
 * @param client_addr Local address of the client. If set to NULL, then the
 * API will figure out a proper address where to bind the context.
 * @param peer_addr Peer (target) address. If this is NULL, then the
 * peer_add_str string and peer_port are used when connecting to peer.
 * @param peer_addr_str Peer (target) address as a string. If this is NULL,
 * then the peer_addr sockaddr is used to set the peer address. If DNS is
 * configured in the system, then the hostname is automatically resolved if
 * given here. Note that the port number is optional in the string. If the
 * port number is not given in the string, then peer_port variable is used
 * instead.
 * Following syntax is supported for the address:
 *      192.0.2.1
 *      192.0.2.1:5353
 *      2001:db8::1
 *      [2001:db8::1]:5353
 *      peer.example.com
 *      peer.example.com:1234
 * @param peer_port Port number where to connect to. Ignored if port number is
 * found in the peer_addr_str.
 * @param timeout Timeout for this function call. This is used if the API
 * needs to do some time consuming operation, like resolving DNS address.
 * @param user_data User specific data.
 *
 * @return 0 if ok, <0 if error.
 */
static inline int net_app_init_tcp_client(struct net_app_ctx *ctx,
					  struct sockaddr *client_addr,
					  struct sockaddr *peer_addr,
					  const char *peer_addr_str,
					  u16_t peer_port,
					  s32_t timeout,
					  void *user_data)
{
	return net_app_init_client(ctx,
				   SOCK_STREAM,
				   IPPROTO_TCP,
				   client_addr,
				   peer_addr,
				   peer_addr_str,
				   peer_port,
				   timeout,
				   user_data);
}

/**
 * @brief Create a UDP client application.
 *
 * @details Note that caller must create the context and initialize it to 0
 * before calling this function. The context must be valid for the whole
 * duration of the application life cycle. This usually means that it
 * cannot be allocated from stack.
 *
 * @param ctx Network application context.
 * @param client_addr Local address of the client. If set to NULL, then the
 * API will figure out a proper address where to bind the context.
 * @param peer_addr Peer (target) address. If this is NULL, then the
 * peer_add_str string and peer_port are used when connecting to peer.
 * @param peer_addr_str Peer (target) address as a string. If this is NULL,
 * then the peer_addr sockaddr is used to set the peer address. If DNS is
 * configured in the system, then the hostname is automatically resolved if
 * given here. Note that the port number is optional in the string. If the
 * port number is not given in the string, then peer_port variable is used
 * instead.
 * Following syntax is supported for the address:
 *      192.0.2.1
 *      192.0.2.1:5353
 *      2001:db8::1
 *      [2001:db8::1]:5353
 *      peer.example.com
 *      peer.example.com:1234
 * @param peer_port Port number where to connect to. Ignored if port number is
 * found in the peer_addr_str.
 * @param timeout Timeout for this function call. This is used if the API
 * needs to do some time consuming operation, like resolving DNS address.
 * @param user_data User specific data.
 *
 * @return 0 if ok, <0 if error.
 */
static inline int net_app_init_udp_client(struct net_app_ctx *ctx,
					  struct sockaddr *client_addr,
					  struct sockaddr *peer_addr,
					  const char *peer_addr_str,
					  u16_t peer_port,
					  s32_t timeout,
					  void *user_data)
{
	return net_app_init_client(ctx,
				   SOCK_DGRAM,
				   IPPROTO_UDP,
				   client_addr,
				   peer_addr,
				   peer_addr_str,
				   peer_port,
				   timeout,
				   user_data);
}

/**
 * @brief Establish a network connection to peer.
 *
 * @param ctx Network application context.
 * @param timeout How long to wait the network connection before giving up.
 *
 * @return 0 if ok, <0 if error.
 */
int net_app_connect(struct net_app_ctx *ctx, s32_t timeout);
#endif /* CONFIG_NET_APP_CLIENT */

/**
 * @brief Set various network application callbacks.
 *
 * @param ctx Network app context.
 * @param connect_cb Connect callback.
 * @param recv_cb Data receive callback.
 * @param send_cb Data sent callback.
 * @param close_cb Close callback.
 *
 * @return 0 if ok, <0 if error.
 */
int net_app_set_cb(struct net_app_ctx *ctx,
		   net_app_connect_cb_t connect_cb,
		   net_app_recv_cb_t recv_cb,
		   net_app_send_cb_t send_cb,
		   net_app_close_cb_t close_cb);

/**
 * @brief Send data that is found in net_pkt to peer.
 *
 * @details If the function return < 0, then it is caller responsibility
 * to unref the pkt. If the packet was sent successfully, then the lower
 * IP stack will release the network pkt.
 *
 * @param ctx Network application context.
 * @param pkt Network packet to send.
 * @param dst Destination address where to send packet. This is
 * ignored for TCP data.
 * @param dst_len Destination address structure size
 * @param timeout How long to wait the send before giving up.
 * @param user_data_send User data specific to this send call.
 *
 * @return 0 if ok, <0 if error.
 */
int net_app_send_pkt(struct net_app_ctx *ctx,
		     struct net_pkt *pkt,
		     const struct sockaddr *dst,
		     socklen_t dst_len,
		     s32_t timeout,
		     void *user_data_send);

/**
 * @brief Send data that is found in user specified buffer to peer.
 *
 * @param ctx Network application context.
 * @param buf Buffer to send.
 * @param buf_len Amount of data to send.
 * @param dst Destination address where to send packet. This is
 * ignored for TCP data.
 * @param dst_len Destination address structure size
 * @param timeout How long to wait the send before giving up.
 * @param user_data_send User data specific to this send call.
 *
 * @return 0 if ok, <0 if error.
 */
int net_app_send_buf(struct net_app_ctx *ctx,
		     u8_t *buf,
		     size_t buf_len,
		     const struct sockaddr *dst,
		     socklen_t dst_len,
		     s32_t timeout,
		     void *user_data_send);

/**
 * @brief Create network packet.
 *
 * @param ctx Network application context.
 * @param family What kind of network packet to get (AF_INET or AF_INET6)
 * @param timeout How long to wait the send before giving up.
 *
 * @return valid net_pkt if ok, NULL if error.
 */
struct net_pkt *net_app_get_net_pkt(struct net_app_ctx *ctx,
				    sa_family_t family,
				    s32_t timeout);

/**
 * @brief Create network packet based on dst address.
 *
 * @param ctx Network application context.
 * @param dst Destination address to select net_context
 * @param timeout How long to wait the send before giving up.
 *
 * @return valid net_pkt if ok, NULL if error.
 */
struct net_pkt *net_app_get_net_pkt_with_dst(struct net_app_ctx *ctx,
					     const struct sockaddr *dst,
					     s32_t timeout);

/**
 * @brief Create network buffer that will hold network data.
 *
 * @details The returned net_buf is automatically appended to the
 * end of network packet fragment chain.
 *
 * @param ctx Network application context.
 * @param pkt Network packet to where the data buf is appended.
 * @param timeout How long to wait the send before giving up.
 *
 * @return valid net_pkt if ok, NULL if error.
 */
struct net_buf *net_app_get_net_buf(struct net_app_ctx *ctx,
				    struct net_pkt *pkt,
				    s32_t timeout);

/**
 * @brief Close a network connection to peer.
 *
 * @param ctx Network application context.
 *
 * @return 0 if ok, <0 if error.
 */
int net_app_close(struct net_app_ctx *ctx);

/**
 * @brief Close a specific network connection.
 *
 * @param ctx Network application context.
 * @param net_ctx Network context.
 *
 * @return 0 if ok, <0 if error.
 */
int net_app_close2(struct net_app_ctx *ctx,
		   struct net_context *net_ctx);

/**
 * @brief Release this network application context.
 *
 * @details No network data will be received via this context after this
 * call.
 *
 * @param ctx Network application context.
 *
 * @return 0 if ok, <0 if error.
 */
int net_app_release(struct net_app_ctx *ctx);

#if defined(CONFIG_NET_APP_TLS) || defined(CONFIG_NET_APP_DTLS)
#if defined(CONFIG_NET_APP_CLIENT)
/**
 * @brief Initialize TLS support for this net_app client context.
 *
 * @param ctx net_app context.
 * @param request_buf Caller-supplied buffer where the TLS request will be
 * stored
 * @param request_buf_len Length of the caller-supplied buffer.
 * @param personalization_data Personalization data (Device specific
 * identifiers) for random number generator. (Can be NULL).
 * @param personalization_data_len Length of the personalization data.
 * @param cert_cb User-supplied callback that setups the certificates.
 * @param cert_host Hostname that is used to verify the server certificate.
 * This value is used when net_api API calls mbedtls_ssl_set_hostname()
 * which sets the hostname to check against the received server certificate.
 * See https://tls.mbed.org/kb/how-to/use-sni for more details.
 * This can be left NULL in which case mbedtls will silently skip certificate
 * verification entirely. This option is only used if MBEDTLS_X509_CRT_PARSE_C
 * is enabled in mbedtls config file.
 * @param entropy_src_cb User-supplied callback that setup the entropy. This
 * can be set to NULL, in which case default entropy source is used.
 * @param pool Memory pool for RX data reads.
 * @param stack TLS thread stack.
 * @param stack_len TLS thread stack size.
 *
 * @return Return 0 if ok, <0 if error.
 */
int net_app_client_tls(struct net_app_ctx *ctx,
		       u8_t *request_buf,
		       size_t request_buf_len,
		       u8_t *personalization_data,
		       size_t personalization_data_len,
		       net_app_ca_cert_cb_t cert_cb,
		       const char *cert_host,
		       net_app_entropy_src_cb_t entropy_src_cb,
		       struct k_mem_pool *pool,
		       k_thread_stack_t *stack,
		       size_t stack_size);
#endif /* CONFIG_NET_APP_CLIENT */

#if defined(CONFIG_NET_APP_SERVER)
/**
 * @brief Initialize TLS support for this net_app server context.
 *
 * @param ctx net_app context.
 * @param request_buf Caller-supplied buffer where the TLS request will be
 * stored
 * @param request_buf_len Length of the caller-supplied buffer.
 * @param server_banner Print information about started service. This is only
 * printed if net_app debugging is activated. The parameter can be set to NULL
 * if no extra prints are needed.
 * @param personalization_data Personalization data (Device specific
 * identifiers) for random number generator. (Can be NULL).
 * @param personalization_data_len Length of the personalization data.
 * @param cert_cb User-supplied callback that setups the certificates.
 * @param entropy_src_cb User-supplied callback that setup the entropy. This
 * can be set to NULL, in which case default entropy source is used.
 * @param pool Memory pool for RX data reads.
 * @param stack TLS thread stack.
 * @param stack_len TLS thread stack size.
 *
 * @return Return 0 if ok, <0 if error.
 */
int net_app_server_tls(struct net_app_ctx *ctx,
		       u8_t *request_buf,
		       size_t request_buf_len,
		       const char *server_banner,
		       u8_t *personalization_data,
		       size_t personalization_data_len,
		       net_app_cert_cb_t cert_cb,
		       net_app_entropy_src_cb_t entropy_src_cb,
		       struct k_mem_pool *pool,
		       k_thread_stack_t *stack,
		       size_t stack_len);

#endif /* CONFIG_NET_APP_SERVER */

#endif /* CONFIG_NET_APP_TLS || CONFIG_NET_APP_DTLS */

/**
 * @}
 */

#if defined(CONFIG_NET_DEBUG_APP)
typedef void (*net_app_ctx_cb_t)(struct net_app_ctx *ctx, void *user_data);
void net_app_server_foreach(net_app_ctx_cb_t cb, void *user_data);
void net_app_client_foreach(net_app_ctx_cb_t cb, void *user_data);
#endif /* CONFIG_NET_DEBUG_APP */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_NET_APP_H_ */
