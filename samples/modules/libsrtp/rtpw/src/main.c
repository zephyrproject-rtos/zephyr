/*
 * Copyright 2025 Sayed Naser Moravej <seyednasermoravej@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/posix/sys/socket.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/arpa/inet.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_ip.h>

#include <config.h>
#include "srtp.h"
#include "rtp.h"
#include "util.h"

#define MAX_WORD_LEN 128
#define MAX_KEY_LEN  96

LOG_MODULE_REGISTER(rtpw_sample, LOG_LEVEL_INF);

int main(void)
{
	char word[MAX_WORD_LEN];
	int sock, ret;
	struct in_addr rcvr_addr;
	struct sockaddr_in name;
	char key[MAX_KEY_LEN];
	srtp_policy_t policy;
	srtp_err_status_t status;
	uint32_t ssrc = 0xdeadbeefu; /* ssrc value hardcoded for now */

	memset(&policy, 0x0, sizeof(srtp_policy_t));

	LOG_INF("Using %s [0x%x]\n", srtp_get_version_string(), srtp_get_version());

	/* initialize srtp library */
	status = srtp_init();
	if (status) {
		LOG_ERR("srtp initialization failed with error code %d.", status);
		return 0;
	}

	/* set address */
	LOG_INF("peer IPv4 address: %s.", CONFIG_NET_CONFIG_PEER_IPV4_ADDR);
	LOG_INF("my IPv4 address: %s.", CONFIG_NET_CONFIG_MY_IPV4_ADDR);

	if (inet_pton(AF_INET, CONFIG_NET_CONFIG_PEER_IPV4_ADDR, &rcvr_addr) != 1) {
		LOG_ERR("Invalid IPv4 address: %s.", CONFIG_NET_CONFIG_PEER_IPV4_ADDR);
		return 0;
	}

	/* open socket */
	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		LOG_ERR("couldn't open socket.");
		return 0;
	}

	memset(&name, 0, sizeof(struct sockaddr_in));
	name.sin_addr = rcvr_addr;
	name.sin_family = PF_INET;
	name.sin_port = htons(CONFIG_NET_SAMPLE_SRTP_SERVER_PORT);

	/* set up the srtp policy and master key
	 * create policy structure, using the default mechanisms.
	 */
	srtp_crypto_policy_set_rtp_default(&policy.rtp);
	srtp_crypto_policy_set_rtcp_default(&policy.rtcp);
	policy.ssrc.type = ssrc_specific;
	policy.ssrc.value = ssrc;
	policy.key = (uint8_t *)key;
	policy.next = NULL;
	policy.window_size = 128;
	policy.allow_repeat_tx = 0;
	policy.rtp.sec_serv = sec_serv_conf_and_auth;
	policy.rtcp.sec_serv = sec_serv_none; /* we don't do RTCP anyway */
	size_t key_len = hex2bin(CONFIG_NET_SAMPLE_SRTP_KEY, strlen(CONFIG_NET_SAMPLE_SRTP_KEY),
				 key, sizeof(key));

	/* check that hex string is the right length */
	if (key_len != policy.rtp.cipher_key_len) {
		LOG_ERR("wrong number of digits in key (should be %d digits, found %d).",
			2 * policy.rtp.cipher_key_len, 2 * key_len);
		return 0;
	}

#ifdef CONFIG_NET_SAMPLE_SENDER
	/* initialize sender's rtp and srtp contexts */
	rtp_sender_t snd;

	snd = rtp_sender_alloc();
	if (snd == NULL) {
		LOG_ERR("malloc() failed.");
		return 0;
	}
	rtp_sender_init(snd, sock, name, ssrc);
	status = rtp_sender_init_srtp(snd, &policy);
	if (status) {
		LOG_ERR("srtp_create() failed with code %d.", status);
		return 0;
	}

	size_t word_len;
	/* Up-count the buffer, then send them off */
	for (int i = 0; i < 1000; i++) {
		snprintf(word, sizeof(word), "SRTP test%d.", i);
		word_len = strlen(word) + 1; /* plus one for null */

		if (word_len > MAX_WORD_LEN) {
			LOG_ERR("word %s too large to send.", word);
		} else {
			rtp_sendto(snd, word, word_len);
			LOG_INF("sending word: %s", word);
		}
		k_msleep(500);
	}

	rtp_sender_deinit_srtp(snd);
	rtp_sender_dealloc(snd);

#elif defined(CONFIG_NET_SAMPLE_RECEIVER)
	rtp_receiver_t rcvr;

	if (bind(sock, (struct sockaddr *)&name, sizeof(name)) < 0) {
		close(sock);
		LOG_ERR("socket bind error.");
		return 0;
	}

	rcvr = rtp_receiver_alloc();
	if (rcvr == NULL) {
		LOG_ERR("malloc() failed.");
		return 0;
	}
	rtp_receiver_init(rcvr, sock, name, ssrc);
	status = rtp_receiver_init_srtp(rcvr, &policy);
	if (status) {
		LOG_ERR("srtp_create() failed with code %d.", status);
		return 0;
	}

	/* get next word and loop */

	while (true) {
		size_t word_len = MAX_WORD_LEN;

		if (rtp_recvfrom(rcvr, word, &word_len) > -1) {
			LOG_INF("receiving word: %s", word);
		}
	}

	rtp_receiver_deinit_srtp(rcvr);
	rtp_receiver_dealloc(rcvr);
#else
#error "Either SENDER or RECEIVER should be defined."
#endif
	ret = close(sock);
	if (ret < 0) {
		LOG_ERR("Failed to close socket");
	}

	status = srtp_shutdown();
	if (status) {
		LOG_ERR("srtp shutdown failed with error code %d.", status);
		return 0;
	}
	return 0;
}
