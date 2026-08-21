/*
 * SPDX-FileCopyrightText: Copyright 2026 Sayed Naser Moravej
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/arpa/inet.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_ip.h>

#include <config.h>
#include "srtp.h"
#include "util.h"
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>

#include "srtp_priv.h"
#include "cipher_priv.h"

#include <zephyr/net/socket.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rtpw_sample, LOG_LEVEL_INF);

/*
 * RTP_HEADER_LEN indicates the size of an RTP header
 */
#define RTP_HEADER_LEN 12

/*
 * RTP_MAX_BUF_LEN defines the largest RTP packet
 */
#define RTP_MAX_BUF_LEN 1500

struct rtp_msg_t {
	srtp_hdr_t header;
	char body[RTP_MAX_BUF_LEN];
};

#define MAX_WORD_LEN 128
#define MAX_KEY_LEN  96

char word[MAX_WORD_LEN];
char key[MAX_KEY_LEN];
struct rtp_msg_t message = { 0 };

uint32_t ssrc = 0xdeadbeefu; /* ssrc value hardcoded for now */

__maybe_unused int app_sender(srtp_ctx_t *srtp_ctx, srtp_policy_t policy,
	size_t key_len, int sock_fd, struct sockaddr_storage addr,
	socklen_t addr_len, size_t salt_len)
{

	/* initialize sender's rtp and srtp contexts */
	int ret = 0;
	srtp_err_status_t status;

	uint16_t seq_host;
	uint32_t ts_host;

	/* Host-order sequence/timestamp */
	seq_host = (uint16_t)srtp_cipher_rand_u32_for_tests();
	ts_host = 0;

	/* Initialize RTP header */
	message.header.ts = sys_cpu_to_be32(ts_host);
	message.header.seq = sys_cpu_to_be16(seq_host);
	message.header.ssrc = sys_cpu_to_be32(ssrc);
	message.header.m = 0;
	message.header.pt = 0x1;
	message.header.version = 2;
	message.header.p = 0;
	message.header.x = 0;
	message.header.cc = 0;

	ret = srtp_policy_add_key(policy, key, key_len, key + key_len, salt_len,
				NULL, 0);
	if (ret != srtp_err_status_ok) {
		LOG_ERR("error: failed to set key in policy");
		return ret;
	}

	status = srtp_create(&srtp_ctx, policy);
	if (status) {
		LOG_ERR("srtp_create() failed with code %d.", status);
		return -EINVAL;
	}

	size_t word_len;

	/* Up-count the buffer, then send them off */
	for (int i = 0; i < 1000; i++) {
		snprintf(word, sizeof(word), "SRTP test%d.", i);
		word_len = strlen(word) + 1; /* plus one for null */

		if (word_len > MAX_WORD_LEN) {
			LOG_ERR("word %s too large to send.", word);
		} else {

			size_t bytes_sent;
			srtp_err_status_t stat;
			size_t msg_len = word_len + RTP_HEADER_LEN;
			size_t pkt_len = RTP_HEADER_LEN + RTP_MAX_BUF_LEN;

			/* marshal data */
			memcpy(message.body, word, word_len);

			/* update header */
			seq_host++;
			ts_host++;

			message.header.seq = sys_cpu_to_be16(seq_host);
			message.header.ts = sys_cpu_to_be32(ts_host);

			/* apply srtp */
			stat = srtp_protect(srtp_ctx, (uint8_t *)&message.header, msg_len,
					    (uint8_t *)&message.header, &pkt_len, 0);
			if (stat) {
				LOG_ERR("error: srtp protection failed with code %d\n", stat);
				return -EINVAL;
			}

			bytes_sent = zsock_sendto(sock_fd, (void *)&message, pkt_len, 0,
						   (struct sockaddr *)&addr, addr_len);
			if (bytes_sent != pkt_len) {
				LOG_ERR("error: couldn't send message %s", (char *)word);
			}

			LOG_INF("sending word: %s", word);
		}
		k_msleep(500);
	}

	return 0;
}

__maybe_unused int app_receiver(srtp_ctx_t *srtp_ctx, srtp_policy_t policy,
	size_t key_len, int sock_fd, struct sockaddr_storage addr,
	socklen_t addr_len, size_t salt_len)
{

	/* initialize receiver's rtp and srtp contexts */
	int ret = 0;
	srtp_err_status_t status;

	message.header.seq = 0;
	message.header.ts = 0;
	message.header.ssrc = sys_cpu_to_be32(ssrc);
	message.header.m = 0;
	message.header.pt = 0x1;
	message.header.version = 2;
	message.header.p = 0;
	message.header.x = 0;
	message.header.cc = 0;

	ret = srtp_policy_add_key(policy, key, key_len, key + key_len, salt_len,
					NULL, 0);
	if (ret != srtp_err_status_ok) {
		LOG_ERR("error: failed to set key in policy");
		return ret;
	}

	status = srtp_create(&srtp_ctx, policy);
	if (status) {
		LOG_ERR("srtp_create() failed with code %d.", status);
		return -EINVAL;
	}

#if defined(CONFIG_NET_IPV6)
	struct sockaddr_in6 bind_addr = {0};

	bind_addr.sin6_family = AF_INET6;
	bind_addr.sin6_port = sys_cpu_to_be16((CONFIG_NET_SAMPLE_SRTP_SERVER_PORT));
	bind_addr.sin6_addr = in6addr_any;
	ret = zsock_bind(sock_fd, (struct sockaddr *)&bind_addr, sizeof(bind_addr));
#else
	struct sockaddr_in bind_addr = {0};

	bind_addr.sin_family = AF_INET;
	bind_addr.sin_port = sys_cpu_to_be16((CONFIG_NET_SAMPLE_SRTP_SERVER_PORT));
	bind_addr.sin_addr.s_addr = INADDR_ANY;
	ret = zsock_bind(sock_fd, (struct sockaddr *)&bind_addr, sizeof(bind_addr));
#endif
	if (ret < 0) {
		zsock_close(sock_fd);
		LOG_ERR("socket bind error.");
		return ret;
	}

	/* get next word and loop */
	while (true) {
		size_t word_len = MAX_WORD_LEN;
		ssize_t bytes_recvd;
		srtp_err_status_t stat;

		bytes_recvd = zsock_recvfrom(sock_fd, (void *)&message, word_len, 0,
					      (struct sockaddr *)NULL, 0);

		if (bytes_recvd < 0) {
			word_len = 0;
			return bytes_recvd;
		}

		/* verify rtp header */
		if (message.header.version != 2) {
			word_len = 0;
			return -ENOMSG;
		}

		LOG_DBG("%zu bytes received from SSRC %u\n", bytes_recvd, message.header.ssrc);

		/* apply srtp */
		stat = srtp_unprotect(srtp_ctx, (uint8_t *)&message.header, bytes_recvd,
				      (uint8_t *)&message.header, &bytes_recvd);
		if (stat) {
			LOG_ERR("error: srtp unprotection failed with code %d%s\n", stat,
				stat == srtp_err_status_replay_fail ? " (replay check failed)"
				: stat == srtp_err_status_auth_fail ? " (auth check failed)"
								    : "");
			return -ENODATA;
		}
		strncpy(word, message.body, bytes_recvd - RTP_HEADER_LEN);
		LOG_INF("receiving word: %s", word);
	}

	return 0;
}

int main(void)
{
	int sock_fd, ret;
	struct sockaddr_storage addr = { 0 };
	socklen_t addr_len;
	srtp_policy_t policy;
	srtp_err_status_t status;
	size_t key_size = 128;
	size_t tag_size = 8;
	srtp_ctx_t *srtp_ctx = NULL;

	LOG_INF("Using %s [0x%x]\n", srtp_get_version_string(), srtp_get_version());

	/* initialize srtp library */
	status = srtp_init();
	if (status) {
		LOG_ERR("srtp initialization failed with error code %d.", status);
		return -ECANCELED;
	}

	/* set address */
#if defined(CONFIG_NET_IPV6)
	struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&addr;

	LOG_INF("Peer IPv6 address: %s.", CONFIG_NET_CONFIG_PEER_IPV6_ADDR);
	LOG_INF("My IPv6 address: %s.", CONFIG_NET_CONFIG_MY_IPV6_ADDR);

	if (zsock_inet_pton(AF_INET6, CONFIG_NET_CONFIG_PEER_IPV6_ADDR, &addr6->sin6_addr) != 1) {
		LOG_ERR("Invalid IPv6 address: %s.", CONFIG_NET_CONFIG_PEER_IPV6_ADDR);
		return -EFAULT;
	}

	/* open socket */
	sock_fd = zsock_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	addr6->sin6_family = AF_INET6;
	addr6->sin6_port = sys_cpu_to_be16(CONFIG_NET_SAMPLE_SRTP_SERVER_PORT);
	addr_len = sizeof(struct sockaddr_in6);
#else
	struct sockaddr_in *addr4 = (struct sockaddr_in *)&addr;

	LOG_INF("Peer IPv4 address: %s.", CONFIG_NET_CONFIG_PEER_IPV4_ADDR);
	LOG_INF("My IPv4 address: %s.", CONFIG_NET_CONFIG_MY_IPV4_ADDR);

	if (zsock_inet_pton(AF_INET, CONFIG_NET_CONFIG_PEER_IPV4_ADDR, &addr4->sin_addr) != 1) {
		LOG_ERR("Invalid IPv4 address: %s.", CONFIG_NET_CONFIG_PEER_IPV4_ADDR);
		return -EFAULT;
	}

	/* open socket */
	sock_fd = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	addr4->sin_family = AF_INET;
	addr4->sin_port = sys_cpu_to_be16(CONFIG_NET_SAMPLE_SRTP_SERVER_PORT);
	addr_len = sizeof(struct sockaddr_in);
#endif
	if (sock_fd < 0) {
		LOG_ERR("couldn't open socket.");
		return sock_fd;
	}

	/* set up the srtp policy and master key
	 * create policy structure, using the default mechanisms.
	 */
	policy_params_t params;

	params.sec_servs = sec_serv_conf_and_auth;
	params.gcm_on = false;
	params.key_size = key_size;
	params.tag_size = tag_size;

	ret = create_policy_from_params(&policy, &params);
	if (ret != srtp_err_status_ok) {
		LOG_ERR("failed to create policy from parameters");
		return ret;
	}

	ret = srtp_policy_set_ssrc(policy, (srtp_ssrc_t){ ssrc_specific, ssrc });
	if (ret != srtp_err_status_ok) {
		LOG_ERR("failed to set SSRC in policy");
		return ret;
	}

	srtp_profile_t profile;

	ret = srtp_policy_get_profile(policy, &profile);
	if (ret != srtp_err_status_ok) {
		LOG_ERR("failed to get profile from policy");
		return ret;
	}

	size_t key_len = srtp_profile_get_master_key_length(profile);
	size_t salt_len = srtp_profile_get_master_salt_length(profile);
	size_t input_key_len = key_len + salt_len;
	size_t net_sample_key_len = hex2bin(CONFIG_NET_SAMPLE_SRTP_KEY,
		strlen(CONFIG_NET_SAMPLE_SRTP_KEY), key, sizeof(key));

	/* check that hex string is the right length */
	if (net_sample_key_len != input_key_len) {
		LOG_ERR("wrong number of digits in key (should be %d digits, found %d).",
			2 * input_key_len, 2 * net_sample_key_len);
		return -EBADMSG;
	}
#if CONFIG_NET_SAMPLE_SENDER
	ret = app_sender(srtp_ctx, policy, key_len, sock_fd, addr, addr_len, salt_len);
#elif defined(CONFIG_NET_SAMPLE_RECEIVER)
	ret = app_receiver(srtp_ctx, policy, key_len, sock_fd, addr, addr_len, salt_len);
#else
#error "Either SENDER or RECEIVER should be defined."
#endif
	ret = srtp_dealloc(srtp_ctx);
	if (ret) {
		LOG_ERR("Failed to dealloc SRTP");
		return ret;
	}

	ret = zsock_close(sock_fd);
	if (ret < 0) {
		LOG_ERR("Failed to close socket");
		return ret;
	}

	status = srtp_shutdown();
	if (status) {
		LOG_ERR("srtp shutdown failed with error code %d.", status);
		return -ECANCELED;
	}

	return 0;
}
