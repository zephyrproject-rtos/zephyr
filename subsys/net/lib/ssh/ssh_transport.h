/*
 * Copyright (c) 2024 Grant Ramsay <grant.ramsay@hotmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_NET_LIB_SSH_SSH_TRANSPORT_H_
#define ZEPHYR_SUBSYS_NET_LIB_SSH_SSH_TRANSPORT_H_

#include <zephyr/net/ssh/client.h>
#include <zephyr/net/ssh/server.h>

#include "ssh_core.h"
#include "ssh_pkt.h"

#include "ssh_host_key.h"

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/slist.h>

enum ssh_recv_state {
	SSH_RECV_STATE_IDENTITY_INIT,
	SSH_RECV_STATE_IDENTITY,
	SSH_RECV_STATE_PKT_INIT,
	SSH_RECV_STATE_PKT_HEADER,
	SSH_RECV_STATE_PKT_PAYLOAD
};

struct ssh_transport_user_request {
	enum ssh_transport_user_request_type {
		SSH_TRANSPORT_USER_REQUEST_AUTHENTICATE,		/* Client only */
		SSH_TRANSPORT_USER_REQUEST_OPEN_CHANNEL,		/* Client only */
		SSH_TRANSPORT_USER_REQUEST_OPEN_CHANNEL_RESULT,		/* Server only */
		SSH_TRANSPORT_USER_REQUEST_CHANNEL_REQUEST,
		SSH_TRANSPORT_USER_REQUEST_CHANNEL_REQUEST_RESULT,
	} type;
	union {
		struct ssh_transport_user_request_authenticate {
			enum ssh_auth_type type;
			char user_name[32];
			union {
				char password[32];
			};
		} authenticate;
		struct ssh_transport_user_request_open_channel {
			ssh_channel_event_callback_t callback;
			void *user_data;
		} open_channel;
		struct ssh_transport_user_request_open_channel_result {
			struct ssh_channel *channel;
			bool success;
			ssh_channel_event_callback_t callback;
			void *user_data;
		} open_channel_result;
		struct ssh_transport_user_request_channel_request {
			struct ssh_channel *channel;
			enum ssh_channel_request_type type;
			bool want_reply;
		} channel_request;
		struct ssh_transport_user_request_channel_request_result {
			struct ssh_channel *channel;
			bool success;
		} channel_request_result;
	};
};

struct ssh_channel {
	struct ssh_transport *transport;
	ssh_channel_event_callback_t callback;
	void *user_data;
	uint32_t local_channel;
	uint32_t remote_channel;
	uint32_t tx_window_rem;
	uint32_t rx_window_rem;
	uint32_t tx_mtu;
	uint32_t rx_mtu;
	struct ring_buf tx_ring_buf;
	struct ring_buf tx_stderr_ring_buf;
	struct ring_buf rx_ring_buf;
	struct ring_buf rx_stderr_ring_buf;
	bool open : 1;
	bool in_use : 1;

	uint8_t tx_ring_buf_data[CONFIG_SSH_CHANNEL_BUF_SIZE];
	uint8_t tx_stderr_ring_buf_data[CONFIG_SSH_CHANNEL_BUF_SIZE];
	uint8_t rx_ring_buf_data[CONFIG_SSH_CHANNEL_BUF_SIZE];
	uint8_t rx_stderr_ring_buf_data[CONFIG_SSH_CHANNEL_BUF_SIZE];
};

struct ssh_transport {
	union {
		struct ssh_server *sshd;
		struct ssh_client *sshc;
		void *parent;
	};
	struct net_sockaddr_storage addr;
	int sock;
	int host_key_index;
	ssh_transport_event_callback_t callback;
	void *callback_user_data;
	enum ssh_recv_state recv_state;
	uint32_t auths_allowed_mask;
	uint32_t sig_algs_mask;
	struct k_msgq user_request_msgq;
	struct ssh_transport_user_request user_request_msgq_buf[8];
	struct sys_heap kex_heap;
	struct ssh_payload rx_pkt;
	uint32_t tx_seq_num;
	uint32_t rx_seq_num;
	uint32_t tx_bytes_since_kex;
	uint32_t rx_bytes_since_kex;
	k_timepoint_t kex_expiry;
	struct ssh_string *client_identity;
	struct ssh_string *server_identity;
	struct ssh_string *client_kexinit;
	struct ssh_string *server_kexinit;
	struct ssh_string *shared_secret;
	struct ssh_string *exchange_hash;
	struct ssh_string *session_id;
	psa_key_id_t ecdh_ephemeral_key;

	struct ssh_cipher_aes128_ctr {
		psa_key_id_t key_id;
		psa_cipher_operation_t op;
	} tx_cipher, rx_cipher;

	psa_key_id_t tx_mac_key_id;
	psa_key_id_t rx_mac_key_id;

	/* TODO: Different for each cipher */
	uint8_t iv_client[32];
	uint8_t iv_server[32];
	uint8_t encrypt_key_client[16];
	uint8_t encrypt_key_server[16];
	uint8_t integ_key_client[32];
	uint8_t integ_key_server[32];

	struct ssh_algs {
		enum ssh_kex_alg kex;
		enum ssh_host_key_alg server_host_key;
		enum ssh_encryption_alg encryption_c2s;
		enum ssh_encryption_alg encryption_s2c;
		enum ssh_mac_alg mac_c2s;
		enum ssh_mac_alg mac_s2c;
		enum ssh_compression_alg compression_c2s;
		enum ssh_compression_alg compression_s2c;
		/* Languages are not supported */
	} algs;

	bool running : 1;
	bool server : 1;
	bool encrypted : 1;
	bool kexinit_sent : 1;
	bool kexinit_received : 1;
	bool newkeys_sent : 1;
	bool authenticated : 1;

	struct ssh_channel channels[CONFIG_SSH_MAX_CHANNELS];

	uint8_t kex_heap_buf[4096];
	uint8_t tx_buf[2048]; /* TODO: config */
	uint8_t rx_buf[2048];
};

#ifdef CONFIG_SSH_SERVER
#define SSH_SERVER_STACK_SIZE 4096
struct ssh_server {
	struct net_sockaddr_storage bind_addr;
	int host_key_index;
	int sock;
	int eventfd;
	struct ssh_transport transport[CONFIG_SSH_SERVER_MAX_CLIENTS];
	ssh_server_event_callback_t server_callback;
	ssh_transport_event_callback_t transport_callback;
	void *callback_user_data;
	char username[32];
	char password[32];
	int authorized_keys[CONFIG_SSH_MAX_HOST_KEYS];
	size_t authorized_keys_len;
	bool running;
	bool stopping;
	struct k_thread thread;

	K_KERNEL_STACK_MEMBER(thread_stack, SSH_SERVER_STACK_SIZE);
};
#endif

#ifdef CONFIG_SSH_CLIENT
#define SSH_CLIENT_STACK_SIZE 4096
struct ssh_client {
	struct net_sockaddr_storage addr;
	int host_key_index;
	int sock;
	int eventfd;
	struct ssh_transport transport;
	ssh_transport_event_callback_t callback;
	void *callback_user_data;
	char user_name[32]; /* TODO: Max len config */
	bool running;
	bool stopping;
	struct k_thread thread;

	K_KERNEL_STACK_MEMBER(thread_stack, SSH_CLIENT_STACK_SIZE);
};
#endif

int ssh_transport_start(struct ssh_transport *transport, bool server, void *parent,
			int sock, const struct net_sockaddr *addr, int host_key_index,
			ssh_transport_event_callback_t callback, void *user_data);
void ssh_transport_close(struct ssh_transport *transport);
int ssh_transport_input(struct ssh_transport *transport);

int ssh_transport_send_packet(struct ssh_transport *transport, struct ssh_payload *payload);

int ssh_transport_queue_user_request(struct ssh_transport *transport,
				     const struct ssh_transport_user_request *request);

int ssh_transport_wakeup(struct ssh_transport *transport);

int ssh_transport_update(struct ssh_transport *transport);

#ifdef CONFIG_SSH_CLIENT
int ssh_transport_send_service_request(struct ssh_transport *transport,
				       const struct ssh_string *service_name);
#endif
#ifdef CONFIG_SSH_SERVER
int ssh_transport_send_server_sig_algs(struct ssh_transport *transport);
#endif

#endif
