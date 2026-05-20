/*
 * Copyright (c) 2024 Grant Ramsay <grant.ramsay@hotmail.com>
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ssh, CONFIG_SSH_LOG_LEVEL);

#include "ssh_transport.h"

#include "ssh_kex.h"
#include "ssh_auth.h"
#include "ssh_connection.h"

#include <zephyr/random/random.h>
#include <zephyr/zvfs/eventfd.h>

#include <psa/crypto.h>

#ifdef CONFIG_SSH_IDENTITY_OVERRIDE_ENABLE
#define ZEPHYR_SSH_IDENTITY CONFIG_SSH_IDENTITY_OVERRIDE
#else
#define ZEPHYR_SSH_IDENTITY "SSH-2.0-zephyr " ZEPHYR_SSH_VERSION
#endif

static int update_channels(struct ssh_transport *transport);
static int check_kex_expiry(struct ssh_transport *transport);
static int process_user_requests(struct ssh_transport *transport);
static int process_user_request(struct ssh_transport *transport,
				const struct ssh_transport_user_request *request);

static int process_msg(struct ssh_transport *transport);
static int ssh_transport_process_msg(struct ssh_transport *transport,
				     uint8_t msg_id, struct ssh_payload *rx_pkt);
#ifdef CONFIG_SSH_CLIENT
static int process_server_sig_algs(struct ssh_transport *transport,
				   const struct ssh_string *server_sig_algs_str);
#endif
#ifdef CONFIG_SSH_SERVER
static int send_service_accept(struct ssh_transport *transport,
			       const struct ssh_string *service_name);
#endif
static int mac_compute(struct ssh_transport *transport, psa_key_id_t mac_key,
		       uint32_t seq_num, const void *data, size_t len, uint8_t *mac_tag_out,
		       size_t mac_tag_size, size_t *mac_tag_len);
static int mac_verify(struct ssh_transport *transport, psa_key_id_t mac_key,
		      uint32_t seq_num, const void *data, size_t len,
		      const uint8_t *expected_mac, size_t mac_len);

static const char zephyr_ssh_identity[] = ZEPHYR_SSH_IDENTITY "\r\n";
static const struct ssh_string server_sig_algs_str = SSH_STRING_LITERAL("server-sig-algs");

static const struct ssh_string supported_server_sig_algs[] = {
#ifdef CONFIG_SSH_HOST_KEY_ALG_RSA_SHA2_256
	[SSH_SERVER_SIG_ALG_RSA_SHA2_256] = SSH_STRING_LITERAL("rsa-sha2-256"),
#endif
#ifdef CONFIG_SSH_HOST_KEY_ALG_RSA_SHA2_512
	[SSH_SERVER_SIG_ALG_RSA_SHA2_512] = SSH_STRING_LITERAL("rsa-sha2-512"),
#endif
};
BUILD_ASSERT(ARRAY_SIZE(supported_server_sig_algs) > 0);

static ssize_t sendall(int sock, const void *buf, size_t len)
{
	while (len) {
		ssize_t out_len = zsock_send(sock, buf, len, 0);

		if (out_len < 0) {
			return out_len;
		}

		buf = (const char *)buf + out_len;
		len -= out_len;
	}

	return 0;
}

static int send_identity(struct ssh_transport *transport)
{
	/* "\r\n" already included in zephyr_ssh_identity */;
	const uint32_t identity_len = sizeof(zephyr_ssh_identity) - 1;
	struct ssh_string **local_identity;
	ssize_t send_ret;

	NET_HEXDUMP_DBG(zephyr_ssh_identity, identity_len, "TX:");

	send_ret = sendall(transport->sock, zephyr_ssh_identity, identity_len);
	if (send_ret != 0) {
		NET_DBG("Failed to send %s (%d)", "identity", errno);
		return -errno;
	}

	/* Save local identity for KEX hash */
	local_identity = transport->server ? &transport->server_identity :
					     &transport->client_identity;

	*local_identity = ssh_payload_string_alloc(&transport->kex_heap,
						   zephyr_ssh_identity,
						   identity_len - 2 /* exclude "\r\n" */);
	if (*local_identity == NULL) {
		return -ENOMEM;
	}

	return 0;
}

int ssh_transport_start(struct ssh_transport *transport, bool server, void *parent,
			int sock, const struct net_sockaddr *addr, int host_key_index,
			ssh_transport_event_callback_t callback, void *user_data)
{
	psa_status_t status;
	int ret;

	*transport = (struct ssh_transport) {
		.server = server,
		.parent = parent,
		.sock = sock,
		.host_key_index = host_key_index,
		.callback = callback,
		.callback_user_data = user_data,
		.recv_state = SSH_RECV_STATE_IDENTITY_INIT,
		/* TODO: Make allowed authentication methods configurable */
		.auths_allowed_mask = BIT(SSH_AUTH_NONE) |
				      BIT(SSH_AUTH_PASSWORD) |
				      BIT(SSH_AUTH_PUBKEY)
	};

	if (addr->sa_family == NET_AF_INET) {
		memcpy(&transport->addr, addr, sizeof(struct net_sockaddr_in));
	} else if (addr->sa_family == NET_AF_INET6) {
		memcpy(&transport->addr, addr, sizeof(struct net_sockaddr_in6));
	} else {
		return -EPROTONOSUPPORT;
	}

	k_msgq_init(&transport->user_request_msgq,
		    (void *)transport->user_request_msgq_buf,
		    sizeof(transport->user_request_msgq_buf[0]),
		    ARRAY_SIZE(transport->user_request_msgq_buf));

	sys_heap_init(&transport->kex_heap, transport->kex_heap_buf,
		      sizeof(transport->kex_heap_buf));

	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		NET_ERR("PSA crypto init failed: %d", status);
		return -EIO;
	}

	ret = send_identity(transport);
	if (ret == 0) {
		transport->running = true;
	}

	return ret;
}

void ssh_transport_close(struct ssh_transport *transport)
{
	for (int i = 0; i < ARRAY_SIZE(transport->channels); i++) {
		struct ssh_channel *channel = &transport->channels[i];

		if (!channel->in_use) {
			continue;
		}

		ssh_connection_free_channel(channel);
	}

	if (transport->encrypted) {
		psa_cipher_abort(&transport->tx_cipher.op);
		psa_cipher_abort(&transport->rx_cipher.op);
		psa_destroy_key(transport->tx_cipher.key_id);
		psa_destroy_key(transport->rx_cipher.key_id);
		psa_destroy_key(transport->tx_mac_key_id);
		psa_destroy_key(transport->rx_mac_key_id);
	}

	if (transport->client_identity != NULL) {
		sys_heap_free(&transport->kex_heap, transport->client_identity);
		transport->client_identity = NULL;
	}

	if (transport->server_identity != NULL) {
		sys_heap_free(&transport->kex_heap, transport->server_identity);
		transport->server_identity = NULL;
	}

	if (transport->client_kexinit != NULL) {
		sys_heap_free(&transport->kex_heap, transport->client_kexinit);
		transport->client_kexinit = NULL;
	}

	if (transport->server_kexinit != NULL) {
		sys_heap_free(&transport->kex_heap, transport->server_kexinit);
		transport->server_kexinit = NULL;
	}

	if (transport->shared_secret != NULL) {
		sys_heap_free(&transport->kex_heap, transport->shared_secret);
		transport->shared_secret = NULL;
	}

	if (transport->exchange_hash != NULL) {
		sys_heap_free(&transport->kex_heap, transport->exchange_hash);
		transport->exchange_hash = NULL;
	}

	if (transport->session_id != NULL) {
		sys_heap_free(&transport->kex_heap, transport->session_id);
		transport->session_id = NULL;
	}

	transport->running = false;
}

int ssh_transport_input(struct ssh_transport *transport)
{
	struct ssh_payload *rx_pkt = &transport->rx_pkt;
	uint32_t rx_cipher_block_size = 1;
	uint32_t mac_len = 0;
	size_t read_len;
	ssize_t ret;
	void *buf;

	if (transport->encrypted) {
		rx_cipher_block_size = 16; /* AES-128 */
		mac_len = 32; /* AES-256 */
	}

	switch (transport->recv_state) {
	case SSH_RECV_STATE_IDENTITY_INIT: {
		*rx_pkt = (struct ssh_payload) {
			.size = MIN(sizeof(transport->rx_buf),
				    SSH_IDENTITY_MAX_LEN),
			.len = 0,
			.data = transport->rx_buf
		};

		transport->recv_state = SSH_RECV_STATE_IDENTITY;
		__fallthrough;
	}

	case SSH_RECV_STATE_IDENTITY:
		/* TODO: Currently just reading one byte at a time.
		 *  Could read 5 bytes at a time during identity?
		 *  Or (say) ~128 bytes every time and just "process data"?
		 *  ZSOCK_MSG_PEEK isn't well supported
		 *  ZSOCK_MSG_DONTWAIT could be an option?
		 * TODO: The server MAY send other lines of data before sending the version string.
		 */
		read_len = 1;
		break;

	case SSH_RECV_STATE_PKT_INIT: {
		uint32_t header_read_len;

		header_read_len = MAX(rx_cipher_block_size, 8);
		if (header_read_len > sizeof(transport->rx_buf)) {
			return -1;
		}

		*rx_pkt = (struct ssh_payload) {
			.size = header_read_len,
			.len = 0,
			.data = transport->rx_buf,
		};

		transport->recv_state = SSH_RECV_STATE_PKT_HEADER;
		__fallthrough;
	}

	case SSH_RECV_STATE_PKT_HEADER:
	case SSH_RECV_STATE_PKT_PAYLOAD:
		read_len = rx_pkt->size - rx_pkt->len;
		break;
	default:
		/* Bad state */
		return -1;
	}

	buf = &rx_pkt->data[rx_pkt->len];

	ret = zsock_recv(transport->sock, buf, read_len, 0);
	if (ret <= 0) {
		char str[NET_INET6_ADDRSTRLEN];
		void *addr_ptr = transport->addr.ss_family == NET_AF_INET ?
			(void *)&net_sin(net_sad(&transport->addr))->sin_addr :
			(void *)&net_sin6(net_sad(&transport->addr))->sin6_addr;

		zsock_inet_ntop(transport->addr.ss_family, addr_ptr, str, sizeof(str));

		if (ret == 0) {
			NET_DBG("Connection closed %s%s%s:%d",
				transport->addr.ss_family == NET_AF_INET6 ? "[" : "",
				str,
				transport->addr.ss_family == NET_AF_INET6 ? "]" : "",
				net_ntohs(net_sin(net_sad(&transport->addr))->sin_port));
		} else {
			NET_DBG("recv error (%d) %s%s%s:%d",
				errno,
				transport->addr.ss_family == NET_AF_INET6 ? "[" : "",
				str,
				transport->addr.ss_family == NET_AF_INET6 ? "]" : "",
				net_ntohs(net_sin(net_sad(&transport->addr))->sin_port));
		}

		return -1;
	}

	switch (transport->recv_state) {
	case SSH_RECV_STATE_IDENTITY: {
		rx_pkt->len += ret;
		if (rx_pkt->len > 2 && rx_pkt->data[rx_pkt->len-2] == '\r' &&
		    rx_pkt->data[rx_pkt->len-1] == '\n') {
			struct ssh_string **remote_identity;

			/* Truncate "\r\n" */
			rx_pkt->len -= 2;

			NET_DBG("Remote identity: \"%.*s\"", rx_pkt->len, rx_pkt->data);

			/* Save remote identity for KEX hash */
			remote_identity = transport->server ? &transport->client_identity :
							      &transport->server_identity;

			*remote_identity = ssh_payload_string_alloc(&transport->kex_heap,
								    rx_pkt->data, rx_pkt->len);
			if (*remote_identity == NULL) {
				return -ENOMEM;
			}

			transport->recv_state = SSH_RECV_STATE_PKT_INIT;
			return ssh_kex_send_kexinit(transport);
		} else if (rx_pkt->len == rx_pkt->size - 1) {
			return -1;
		}
		break;
	}
	case SSH_RECV_STATE_PKT_HEADER: {
		uint32_t packet_len;
		uint32_t total_len;

		rx_pkt->len += ret;
		if (rx_pkt->len < rx_pkt->size) {
			break;
		}

		if (transport->encrypted) {
			/* Decrypt */
			size_t decrypt_len = rx_cipher_block_size;
			uint8_t *decrypt_buf = &rx_pkt->data[SSH_PKT_LEN_OFFSET];
			struct ssh_cipher_aes128_ctr *cipher = &transport->rx_cipher;
			size_t output_len;
			psa_status_t psa_ret;

			psa_ret = psa_cipher_update(&cipher->op, decrypt_buf, decrypt_len,
						    decrypt_buf, decrypt_len, &output_len);
			if (psa_ret != PSA_SUCCESS) {
				NET_ERR("Decrypt error");
				return -1;
			}

			NET_HEXDUMP_DBG(decrypt_buf, decrypt_len, "RX hdr decrypt:");
		}

		packet_len = sys_get_be32(&rx_pkt->data[SSH_PKT_LEN_OFFSET]);
		NET_DBG("RX: Packet len %u", packet_len);

		total_len = SSH_PKT_LEN_SIZE + packet_len;

		/* TODO: Encrypt Then Mac (ETM) MAC algorithms pad the packet without
		 * the packet length field
		 */
		if (total_len < packet_len || /* wrapped */
		    total_len < SSH_MIN_PACKET_LEN ||
		    total_len > SSH_MAX_PACKET_LEN ||
		    (total_len % MAX(rx_cipher_block_size, 8)) != 0) {
			NET_ERR("Length error");
			return -1;
		}

		if (total_len + mac_len > sizeof(transport->rx_buf)) {
			NET_ERR("Length error");
			return -1;
		}

		rx_pkt->size = total_len + mac_len;
		transport->recv_state = SSH_RECV_STATE_PKT_PAYLOAD;

		/* It's possible we have received the full packet, fallthrough to check */
		ret = 0;

		__fallthrough;
	}

	case SSH_RECV_STATE_PKT_PAYLOAD: {
		uint32_t total_len;

		rx_pkt->len += ret;
		if (rx_pkt->len < rx_pkt->size) {
			break;
		}

		total_len = rx_pkt->len - mac_len;

		if (transport->encrypted) {
			/* Decrypt */
			size_t decrypt_len = total_len - rx_cipher_block_size;
			uint8_t *decrypt_buf = &rx_pkt->data[rx_cipher_block_size];

			if (decrypt_len > 0) {
				struct ssh_cipher_aes128_ctr *cipher = &transport->rx_cipher;
				size_t output_len;
				psa_status_t psa_ret;

				psa_ret = psa_cipher_update(&cipher->op, decrypt_buf, decrypt_len,
							    decrypt_buf, decrypt_len, &output_len);
				if (psa_ret != PSA_SUCCESS) {
					NET_ERR("Decrypt error");
					return -1;
				}

				NET_HEXDUMP_DBG(decrypt_buf, decrypt_len, "RX decrypt:");
			}
		}

		if (mac_len > 0) {
			const uint8_t *mac_tag = &rx_pkt->data[total_len];

			ret = mac_verify(transport, transport->rx_mac_key_id,
					 transport->rx_seq_num, rx_pkt->data, total_len,
					 mac_tag, mac_len);
			if (ret != 0) {
				NET_ERR("MAC incorrect");
				return -1;
			}

			/* Strip off the MAC before processing */
			rx_pkt->size = total_len;
			rx_pkt->len = total_len;
		}

		transport->rx_bytes_since_kex += total_len + mac_len;
		transport->recv_state = SSH_RECV_STATE_PKT_INIT;
		return process_msg(transport);
	}
	default:
		/* Bad state */
		return -1;
	}

	return 0;
}

int ssh_transport_send_packet(struct ssh_transport *transport, struct ssh_payload *payload)
{
	int ret;
	uint32_t total_len = payload->len;
	uint32_t len_to_pad = total_len;
	uint8_t padding_align = 8;
	uint8_t padding_len;
	uint32_t packet_len;
	uint8_t mac_tag[32];
	size_t mac_tag_len;
	ssize_t send_ret;

	if (transport->encrypted) {
		/* Encrypt Then Mac (ETM) MAC algorithms pad the packet without
		 * the packet length field.
		 */
/*		len_to_pad -= SSH_BIN_PKT_LEN_SIZE; */
		padding_align = 16; /* AES-128 */
	}

	/* Padding (4 byte minimum, 8 or more byte aligned) */
	padding_len = ROUND_UP(len_to_pad + 4, padding_align) - len_to_pad;
	total_len += padding_len;

	if (total_len < SSH_MIN_PACKET_LEN) {
		padding_len += padding_align;
		total_len += padding_align;
	}

	/* Check that there is enough room for the random padding */
	if (payload->size < total_len) {
		return -ENOBUFS;
	}

	/* Prepend header */
	packet_len = total_len - SSH_PKT_LEN_SIZE;
	sys_put_be32(packet_len, &payload->data[SSH_PKT_LEN_OFFSET]);
	payload->data[SSH_PKT_PADDING_OFFSET] = padding_len;
	sys_rand_get(&payload->data[total_len - padding_len], padding_len);

	NET_HEXDUMP_DBG(payload->data, total_len, "TX:");

	if (transport->encrypted) {
		struct ssh_cipher_aes128_ctr *cipher;
		size_t encrypt_len;
		uint8_t *encrypt_buf;
		size_t output_len;
		psa_status_t psa_ret;

		ret = mac_compute(transport, transport->tx_mac_key_id,
				  transport->tx_seq_num, payload->data, total_len,
				  mac_tag, sizeof(mac_tag), &mac_tag_len);
		if (ret != 0) {
			return ret;
		}

		/* Encrypt */
		encrypt_len = total_len;
		encrypt_buf = payload->data;
		cipher = &transport->tx_cipher;

		psa_ret = psa_cipher_update(&cipher->op, encrypt_buf, encrypt_len,
					    encrypt_buf, encrypt_len, &output_len);
		if (psa_ret != PSA_SUCCESS) {
			NET_ERR("Encrypt error");
			return -1;
		}

		NET_HEXDUMP_DBG(encrypt_buf, encrypt_len, "TX encrypt:");
	}

	send_ret = sendall(transport->sock, payload->data, total_len);
	if (send_ret < 0) {
		NET_DBG("Failed to send packet to socket %d (%d)", transport->sock, -errno);
		return -errno;
	}

	transport->tx_bytes_since_kex += total_len;
	if (transport->encrypted) {
		NET_HEXDUMP_DBG(mac_tag, mac_tag_len, "TX digest:");

		send_ret = sendall(transport->sock, mac_tag, mac_tag_len);
		if (send_ret < 0) {
			NET_DBG("Failed to send %s (%d)", "mac_tag", -errno);
			return -errno;
		}

		transport->tx_bytes_since_kex += mac_tag_len;
	}

	transport->tx_seq_num++;

	return 0;
}

int ssh_transport_queue_user_request(struct ssh_transport *transport,
				     const struct ssh_transport_user_request *request)
{
	int ret;

	if (!transport->running) {
		return -EINVAL;
	}

	ret = k_msgq_put(&transport->user_request_msgq, request, K_NO_WAIT);
	if (ret != 0) {
		return ret;
	}

	return ssh_transport_wakeup(transport);
}

int ssh_transport_wakeup(struct ssh_transport *transport)
{
	zvfs_eventfd_t value = 1;
	int eventfd = -1;

	if (transport->server) {
#ifdef CONFIG_SSH_SERVER
		eventfd = transport->sshd->eventfd;
#endif
	} else {
#ifdef CONFIG_SSH_CLIENT
		eventfd = transport->sshc->eventfd;
#endif
	}

	return zvfs_eventfd_write(eventfd, value);
}

int ssh_transport_update(struct ssh_transport *transport)
{
	int ret;

	/* Don't try and do anything during key exchange */
	if (transport->kexinit_sent) {
		return 0;
	}

	/* Process any pending events */
	ret = process_user_requests(transport);
	if (ret != 0) {
		return ret;
	}

	/* Send pending data, update RX window */
	ret = update_channels(transport);
	if (ret != 0) {
		return ret;
	}

	/* Check if it is time to for key re-exchange */
	ret = check_kex_expiry(transport);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

int update_channels(struct ssh_transport *transport)
{
	int ret = 0;

	if (!transport->authenticated || transport->kexinit_sent ||
	    transport->kexinit_received) {
		return 0;
	}

	for (int i = 0; i < ARRAY_SIZE(transport->channels); i++) {
		struct ssh_channel *channel;

		channel = &transport->channels[i];
		if (!channel->in_use) {
			continue;
		}

		/* Send any pending data */
		while (channel->tx_window_rem > 0 && !ring_buf_is_empty(&channel->tx_ring_buf)) {
			uint8_t *data;
			uint32_t len = MIN(channel->tx_mtu, channel->tx_window_rem);
			struct ssh_channel_event event;

			/* Assuming up to 256 bytes overhead for headers and random padding */
			BUILD_ASSERT(sizeof(transport->tx_buf) > 256);

			len = MIN(len, sizeof(transport->tx_buf) - 256);
			len = ring_buf_get_claim(&channel->tx_ring_buf, &data, len);
			channel->tx_window_rem -= len;

			ret = ssh_connection_send_channel_data(
				transport, channel->remote_channel, data, len);
			ring_buf_get_finish(&channel->tx_ring_buf, len);
			if (ret != 0) {
				/* Close channel? */
				break;
			}

			event.type = SSH_CHANNEL_EVENT_TX_DATA_READY;
			if (channel->callback != NULL) {
				channel->callback(channel, &event, channel->user_data);
			}
		}

		while (channel->tx_window_rem > 0 &&
		       !ring_buf_is_empty(&channel->tx_stderr_ring_buf)) {
			uint8_t *data;
			uint32_t len = MIN(channel->tx_mtu, channel->tx_window_rem);
			struct ssh_channel_event event;

			/* Assuming up to 256 bytes overhead for headers and random padding */
			BUILD_ASSERT(sizeof(transport->tx_buf) > 256);

			len = MIN(len, sizeof(transport->tx_buf) - 256);
			len = ring_buf_get_claim(&channel->tx_stderr_ring_buf, &data, len);
			channel->tx_window_rem -= len;

			ret = ssh_connection_send_channel_extended_data(
				transport, channel->remote_channel,
				SSH_EXTENDED_DATA_STDERR, data, len);
			ring_buf_get_finish(&channel->tx_stderr_ring_buf, len);
			if (ret != 0) {
				/* Close channel? */
				break;
			}

			event.type = SSH_CHANNEL_EVENT_TX_DATA_READY;
			if (channel->callback != NULL) {
				channel->callback(channel, &event, channel->user_data);
			}
		}

		/* Adjust RX window */
		if (channel->rx_window_rem == 0) {
			uint32_t available_space;

			available_space = MIN(ring_buf_space_get(&channel->rx_ring_buf),
					      ring_buf_space_get(&channel->rx_stderr_ring_buf));
			if (available_space > 0) {
				channel->rx_window_rem = available_space;
				ret = ssh_connection_send_window_adjust(
					transport, channel->remote_channel, available_space);
				if (ret != 0) {
					/* Close channel? */
					break;
				}
			}
		}
	}

	return ret;
}

int check_kex_expiry(struct ssh_transport *transport)
{
	if (!transport->encrypted || transport->kexinit_sent) {
		return 0;
	}

	if (sys_timepoint_expired(transport->kex_expiry) ||
	    transport->tx_bytes_since_kex >= GB(1) ||
	    transport->rx_bytes_since_kex >= GB(1)) {
		return ssh_kex_send_kexinit(transport);
	}

	return 0;
}

static int process_user_requests(struct ssh_transport *transport)
{
	int ret;

	if (transport->kexinit_sent || transport->kexinit_received) {
		return 0;
	}

	/* Process any pending requests */
	while (true) {
		struct ssh_transport_user_request request;

		ret = k_msgq_get(&transport->user_request_msgq, &request, K_NO_WAIT);
		if (ret != 0) {
			return 0;
		}

		ret = process_user_request(transport, &request);
		if (ret != 0) {
			return ret;
		}
	}
}

#ifdef CONFIG_SSH_CLIENT
const char *ssh_transport_client_user_name(struct ssh_transport *transport)
{
	if (transport == NULL || transport->server) {
		return NULL;
	}

	return transport->sshc->user_name;
}

int ssh_transport_auth_password(struct ssh_transport *transport,
				const char *user_name, const char *password)
{
	struct ssh_transport_user_request request;
	size_t user_name_len;
	size_t password_len;

	if (transport == NULL || !transport->running || !transport->encrypted ||
	    transport->server) {
		return -EINVAL;
	}

	request.type = SSH_TRANSPORT_USER_REQUEST_AUTHENTICATE;
	request.authenticate.type = SSH_AUTH_PASSWORD;

	user_name_len = strlen(user_name);
	if (user_name_len + 1 >= ARRAY_SIZE(request.authenticate.user_name)) {
		return -EINVAL;
	}

	memcpy(request.authenticate.user_name, user_name, user_name_len + 1);

	password_len = strlen(password);
	if (password_len + 1 >= ARRAY_SIZE(request.authenticate.password)) {
		return -EINVAL;
	}

	memcpy(request.authenticate.password, password, password_len + 1);

	return ssh_transport_queue_user_request(transport, &request);
}

int ssh_transport_channel_open(struct ssh_transport *transport,
			       ssh_channel_event_callback_t callback, void *user_data)
{
	struct ssh_transport_user_request request;

	if (transport == NULL || !transport->running || !transport->authenticated ||
	    transport->server) {
		return -EINVAL;
	}

	request.type = SSH_TRANSPORT_USER_REQUEST_OPEN_CHANNEL;
	request.open_channel.callback = callback;
	request.open_channel.user_data = user_data;

	return ssh_transport_queue_user_request(transport, &request);
}
#endif

static int process_user_request(struct ssh_transport *transport,
				const struct ssh_transport_user_request *request)
{
	switch (request->type) {
#ifdef CONFIG_SSH_SERVER
	case SSH_TRANSPORT_USER_REQUEST_OPEN_CHANNEL_RESULT: {
		struct ssh_channel *channel;
		int ret = -1;

		channel = request->open_channel_result.channel;
		channel->callback = request->open_channel_result.callback;
		channel->user_data = request->open_channel_result.user_data;

		if (request->open_channel_result.success) {
			ret = ssh_connection_send_channel_open_confirmation(
				transport, channel->remote_channel, channel->local_channel,
				channel->rx_window_rem, channel->rx_mtu);
			if (ret == 0) {
				channel->open = true;
			}
		}

		/* TODO: else send channel open failure */
		if (ret != 0) {
			ssh_connection_free_channel(channel);
		}

		return ret;
	}
#endif
	case SSH_TRANSPORT_USER_REQUEST_CHANNEL_REQUEST_RESULT: {
		struct ssh_channel *channel;
		bool success;
		int ret;

		channel = request->channel_request_result.channel;
		success = request->channel_request_result.success;

		ret = ssh_connection_send_channel_result(transport, success,
							 channel->remote_channel);
		if (ret != 0) {
			ssh_connection_free_channel(channel);
		}

		return ret;
	}
#ifdef CONFIG_SSH_CLIENT
	case SSH_TRANSPORT_USER_REQUEST_AUTHENTICATE:
		if (!transport->encrypted || transport->authenticated) {
			return -1;
		}

		switch (request->authenticate.type) {
		case SSH_AUTH_NONE:
			return ssh_auth_send_userauth_request_none(
				transport, request->authenticate.user_name);

		case SSH_AUTH_PASSWORD:
			return ssh_auth_send_userauth_request_password(
				transport, request->authenticate.user_name,
				request->authenticate.password);

		default:
			return -1;
		}
		break;

	case SSH_TRANSPORT_USER_REQUEST_OPEN_CHANNEL: {
		const struct ssh_string channel_type = SSH_STRING_LITERAL("session");
		struct ssh_channel *channel;
		uint32_t sender_channel;
		uint32_t initial_window_size;
		uint32_t maximum_packet_size;
		int ret;

		if (!transport->authenticated) {
			return -1;
		}

		channel = ssh_connection_allocate_channel(transport);
		if (channel == NULL) {
			NET_ERR("Failed to alloc channel");
			return -ENOBUFS;
		}

		channel->callback = request->open_channel.callback;
		channel->user_data = request->open_channel.user_data;

		sender_channel = channel->local_channel;
		channel->rx_window_rem = sizeof(channel->rx_ring_buf_data);
		/* TODO: MTU = sizeof(transport->rx_buf) - HEADER_LEN - MAX_PADDING - MAC_LEN? */
		channel->rx_mtu = sizeof(channel->rx_ring_buf_data);

		initial_window_size = channel->rx_window_rem;
		maximum_packet_size = channel->rx_mtu;

		ret = ssh_connection_send_channel_open(
			transport, &channel_type, sender_channel,
			initial_window_size, maximum_packet_size);
		if (ret != 0) {
			ssh_connection_free_channel(channel);
		}

		return ret;
	}

	case SSH_TRANSPORT_USER_REQUEST_CHANNEL_REQUEST: {
		static const struct ssh_string channel_type_shell =
			SSH_STRING_LITERAL("shell");
		const struct ssh_string *channel_type;
		struct ssh_channel *channel;
		bool want_reply;

		if (!transport->authenticated) {
			return -1;
		}

		channel = request->channel_request.channel;

		switch (request->channel_request.type) {
		case SSH_CHANNEL_REQUEST_SHELL: {
			/* TODO: Hack: sending pseudo terminal request here too... */
			(void)ssh_connection_send_channel_request_pseudo_terminal(
				transport, channel->remote_channel, true);

			channel_type = &channel_type_shell;
			break;
		}

		default:
			return -1;
		}

		want_reply = request->channel_request.want_reply;

		return ssh_connection_send_channel_request(
			transport, channel->remote_channel,
			channel_type, want_reply);
	}
#endif
	default:
		return -1;
	}

	return -1;
}

static int process_msg(struct ssh_transport *transport)
{
	int ret;
	uint8_t msg_id;
	struct ssh_payload *rx_pkt = &transport->rx_pkt;

	/* Strip off the random padding and MAC before processing */
	uint32_t packet_len = rx_pkt->size - SSH_PKT_LEN_SIZE;
	uint8_t padding_len = rx_pkt->data[SSH_PKT_LEN_SIZE];

	/* At least 1 byte for message ID */
	if (packet_len < SSH_PKT_PADDING_SIZE + SSH_PKT_MSG_ID_SIZE + padding_len) {
		NET_ERR("Length error");
		return -ERANGE;
	}

	/* Strip off the random padding */
	rx_pkt->size -= padding_len;

	rx_pkt->len = SSH_PKT_MSG_ID_OFFSET;
	msg_id = rx_pkt->data[rx_pkt->len++];

	NET_DBG("RX: Packet len %u, padding len %u, msg ID %u",
		packet_len, padding_len, msg_id);

	if (IN_RANGE(msg_id, 1, 49)) {
		ret = ssh_transport_process_msg(transport, msg_id, rx_pkt);

	} else if (IN_RANGE(msg_id, 50, 79)) {
		if (!transport->encrypted || transport->kexinit_received) {
			return -ERANGE;
		}

		ret = ssh_auth_process_msg(transport, msg_id, rx_pkt);

	} else if (IN_RANGE(msg_id, 80, 100)) {
		if (!transport->encrypted || !transport->authenticated ||
		    transport->kexinit_received) {
			return -ERANGE;
		}

		ret = ssh_connection_process_msg(transport, msg_id, rx_pkt);

	} else {
		return -EINVAL;
	}

	if (rx_pkt->data != NULL) {
		mbedtls_platform_zeroize(rx_pkt->data, rx_pkt->len);
	}

	transport->rx_seq_num++;

	if (ret != 0 && msg_id != SSH_MSG_DISCONNECT) {
		NET_ERR("Error processing message (%d/%u)", ret, msg_id);
		return -EINVAL;
	}

	return 0;
}

static int ssh_transport_process_msg(struct ssh_transport *transport,
				     uint8_t msg_id,
				     struct ssh_payload *rx_pkt)
{
	int ret = -1;

	if (IN_RANGE(msg_id, 30, 49)) {
		return ssh_kex_process_msg(transport, msg_id, rx_pkt);
	}

	switch (msg_id) {
	case SSH_MSG_DISCONNECT: {
		uint32_t reason_code;
		struct ssh_string description, language_tag;

		NET_DBG("DISCONNECT");

		if (!ssh_payload_read_u32(rx_pkt, &reason_code) ||
		    !ssh_payload_read_string(rx_pkt, &description) ||
		    !ssh_payload_read_string(rx_pkt, &language_tag) ||
		    !ssh_payload_read_complete(rx_pkt)) {
			NET_ERR("Length error");
			return -1;
		}

		NET_DBG("DISCONNECT: reason %u \"%.*s\"", reason_code,
			description.len, description.data);
		return -1;
	}

	case SSH_MSG_UNIMPLEMENTED: {
		uint32_t rejected_seq_num;

		NET_DBG("UNIMPLEMENTED");

		if (!ssh_payload_read_u32(rx_pkt, &rejected_seq_num) ||
		    !ssh_payload_read_complete(rx_pkt)) {
			NET_ERR("Length error");
			return -1;
		}
		return -1;
	}

	case SSH_MSG_IGNORE: {
		struct ssh_string ignore_data;

		NET_DBG("IGNORE");

		if (!ssh_payload_read_string(rx_pkt, &ignore_data) ||
		    !ssh_payload_read_complete(rx_pkt)) {
			NET_ERR("Length error");
			return -1;
		}
		ret = 0;
		break;
	}

	case SSH_MSG_DEBUG: {
		bool always_display;
		struct ssh_string dbg_msg, language_tag;

		NET_DBG("DEBUG");

		if (!ssh_payload_read_bool(rx_pkt, &always_display) ||
		    !ssh_payload_read_string(rx_pkt, &dbg_msg) ||
		    !ssh_payload_read_string(rx_pkt, &language_tag) ||
		    !ssh_payload_read_complete(rx_pkt)) {
			NET_ERR("Length error");
			return -1;
		}

		if (always_display) {
			NET_INFO("Remote dbg: %.*s", dbg_msg.len, dbg_msg.data);
		} else {
			NET_DBG("Remote dbg: %.*s", dbg_msg.len, dbg_msg.data);
		}
		ret = 0;
		break;
	}

#ifdef CONFIG_SSH_SERVER
	case SSH_MSG_SERVICE_REQUEST: {
		static const struct ssh_string supported_service_name =
			SSH_STRING_LITERAL("ssh-userauth");
		struct ssh_string service_name;

		/* Server only */
		if (!transport->server || !transport->encrypted ||
		    transport->kexinit_received) {
			break;
		}

		NET_DBG("SERVICE_REQUEST");

		if (!ssh_payload_read_string(rx_pkt, &service_name) ||
		    !ssh_payload_read_complete(rx_pkt)) {
			NET_ERR("Length error");
			return -1;
		}

		if (!ssh_strings_equal(&service_name, &supported_service_name)) {
			NET_DBG("Unsupported service");
			return -1;
		}

		ret = send_service_accept(transport, &service_name);
		break;
	}
#endif
#ifdef CONFIG_SSH_CLIENT
	case SSH_MSG_SERVICE_ACCEPT: {
		struct ssh_string service_name;
		struct ssh_client *sshc;

		NET_DBG("SERVICE_ACCEPT");

		/* Client only */
		if (transport->server || !transport->encrypted ||
		    transport->kexinit_received) {
			break;
		}

		if (!ssh_payload_read_string(rx_pkt, &service_name) ||
		    !ssh_payload_read_complete(rx_pkt)) {
			NET_ERR("Length error");
			return -1;
		}

		/* Start by sending a userauth "none" request.
		 * This may succeed, but more generally it's used to get a list
		 * of available authentication methods in the failure reply.
		 */
		sshc = transport->sshc;

		return ssh_auth_send_userauth_request_none(transport,
							   sshc->user_name);
	}

	case SSH_MSG_EXT_INFO: {
		uint32_t num_extensions;

		NET_DBG("EXT_INFO");

		/* Client only */
		if (transport->server || !transport->encrypted || transport->kexinit_received) {
			break;
		}

		if (!ssh_payload_read_u32(rx_pkt, &num_extensions)) {
			NET_ERR("Length error");
			return -1;
		}

		for (uint32_t i = 0; i < num_extensions; i++) {
			struct ssh_string extension_name;
			struct ssh_string extension_value;

			if (!ssh_payload_read_string(rx_pkt, &extension_name) ||
			    !ssh_payload_read_string(rx_pkt, &extension_value)) {
				NET_ERR("Length error");
				return -1;
			}

			NET_DBG("Extension: %.*s", extension_name.len, extension_name.data);
			NET_HEXDUMP_DBG(extension_value.data, extension_value.len, "Value");

			if (ssh_strings_equal(&extension_name, &server_sig_algs_str)) {
				ret = process_server_sig_algs(transport, &extension_value);
				if (ret != 0) {
					return ret;
				}
			}

		}

		if (!ssh_payload_read_complete(rx_pkt)) {
			NET_ERR("Length error");
			return -1;
		}

		ret = 0;
		break;
	}
#endif
	case SSH_MSG_KEXINIT:
		return ssh_kex_process_kexinit(transport, rx_pkt);

	case SSH_MSG_NEWKEYS:
		return ssh_kex_process_newkeys(transport, rx_pkt);

	default:
		break;
	}

	return ret;
}

#ifdef CONFIG_SSH_CLIENT
int ssh_transport_send_service_request(struct ssh_transport *transport,
				       const struct ssh_string *service_name)
{
	int ret;

	SSH_PAYLOAD_BUF(payload, transport->tx_buf);

	if (!ssh_payload_skip_bytes(&payload, SSH_PKT_MSG_ID_OFFSET) ||
	    !ssh_payload_write_byte(&payload, SSH_MSG_SERVICE_REQUEST) ||
	    !ssh_payload_write_string(&payload, service_name)) {
		return -ENOBUFS;
	}

	ret = ssh_transport_send_packet(transport, &payload);
	if (ret < 0) {
		NET_DBG("Failed to send %s (%d)", "SERVICE_REQUEST", ret);
	}

	return ret;
}

static int process_server_sig_algs(struct ssh_transport *transport,
				   const struct ssh_string *server_sig_algs)
{
	struct ssh_string alg_name;
	struct ssh_payload alg_name_list = {
		.size = server_sig_algs->len,
		.len = 0,
		.data = (void *)server_sig_algs->data
	};

	NET_DBG("server-sig-algs: %.*s", server_sig_algs->len, server_sig_algs->data);

	transport->sig_algs_mask = 0;

	while (ssh_payload_name_list_iter(&alg_name_list, &alg_name)) {
		for (int j = 0; j < ARRAY_SIZE(supported_server_sig_algs); j++) {
			if (ssh_strings_equal(&supported_server_sig_algs[j], &alg_name)) {
				NET_DBG("Adding supported server-sig-alg: %.*s",
					alg_name.len, alg_name.data);
				transport->sig_algs_mask |= BIT(j);
			}
		}
	}

	return 0;
}
#endif

#ifdef CONFIG_SSH_SERVER
int ssh_transport_send_server_sig_algs(struct ssh_transport *transport)
{
	int ret;
	uint32_t num_extensions = 1;

	SSH_PAYLOAD_BUF(payload, transport->tx_buf);

	if (!ssh_payload_skip_bytes(&payload, SSH_PKT_MSG_ID_OFFSET) ||
	    !ssh_payload_write_byte(&payload, SSH_MSG_EXT_INFO) ||
	    !ssh_payload_write_u32(&payload, num_extensions) ||
	    !ssh_payload_write_string(&payload, &server_sig_algs_str) ||
	    !ssh_payload_write_name_list(&payload, supported_server_sig_algs,
					 ARRAY_SIZE(supported_server_sig_algs))) {
		return -ENOBUFS;
	}

	ret = ssh_transport_send_packet(transport, &payload);
	if (ret < 0) {
		NET_DBG("Failed to send %s (%d)", "EXT_INFO", ret);
	}

	return ret;
}

static int send_service_accept(struct ssh_transport *transport,
			       const struct ssh_string *service_name)
{
	int ret;

	SSH_PAYLOAD_BUF(payload, transport->tx_buf);

	if (!ssh_payload_skip_bytes(&payload, SSH_PKT_MSG_ID_OFFSET) ||
	    !ssh_payload_write_byte(&payload, SSH_MSG_SERVICE_ACCEPT) ||
	    !ssh_payload_write_string(&payload, service_name)) {
		return -ENOBUFS;
	}

	ret = ssh_transport_send_packet(transport, &payload);
	if (ret < 0) {
		NET_DBG("Failed to send %s (%d)", "SERVICE_ACCEPT", ret);
	}

	return ret;
}
#endif

static int mac_compute(struct ssh_transport *transport, psa_key_id_t mac_key,
		       uint32_t seq_num, const void *data, size_t len, uint8_t *mac_tag_out,
		       size_t mac_tag_size, size_t *mac_tag_len)
{
	psa_mac_operation_t op = PSA_MAC_OPERATION_INIT;
	psa_status_t status;
	uint8_t tmp[4];

	ARG_UNUSED(transport);

	status = psa_mac_sign_setup(&op, mac_key, PSA_ALG_HMAC(PSA_ALG_SHA_256));
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	sys_put_be32(seq_num, tmp);

	status = psa_mac_update(&op, tmp, sizeof(tmp));
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = psa_mac_update(&op, data, len);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = psa_mac_sign_finish(&op, mac_tag_out, mac_tag_size, mac_tag_len);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	return 0;

exit:
	psa_mac_abort(&op);
	return -EIO;
}

static int mac_verify(struct ssh_transport *transport, psa_key_id_t mac_key,
		      uint32_t seq_num, const void *data, size_t len,
		      const uint8_t *expected_mac, size_t mac_len)
{
	psa_mac_operation_t op = PSA_MAC_OPERATION_INIT;
	psa_status_t status;
	uint8_t tmp[4];

	ARG_UNUSED(transport);

	status = psa_mac_verify_setup(&op, mac_key, PSA_ALG_HMAC(PSA_ALG_SHA_256));
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	sys_put_be32(seq_num, tmp);

	status = psa_mac_update(&op, tmp, sizeof(tmp));
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = psa_mac_update(&op, data, len);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = psa_mac_verify_finish(&op, expected_mac, mac_len);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	return 0;

exit:
	psa_mac_abort(&op);
	return -EIO;
}
