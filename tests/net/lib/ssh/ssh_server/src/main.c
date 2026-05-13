/*
 * Copyright (c) 2024 Grant Ramsay <grant.ramsay@hotmail.com>
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/net/socket.h>
#include <zephyr/ztest.h>

#include <zephyr/net/ssh/client.h>
#include <zephyr/net/ssh/keygen.h>
#include <zephyr/net/ssh/server.h>

#define MY_IPV4_ADDR "127.0.0.1"
#define SSH_PORT 22

/* SSH message IDs */
#define SSH_MSG_KEXINIT        0x14
#define SSH_MSG_KEX_ECDH_REPLY 0x1f
#define SSH_MSG_NEWKEYS        0x15

/*
 * Kexinit packet layout (for the current algorithm set):
 *   bytes 0-3:     packet_length (deterministic: 0x9c = 156)
 *   byte 4:        padding_length (deterministic: 0x0a = 10)
 *   byte 5:        msg_id SSH_MSG_KEXINIT (0x14)
 *   bytes 6-21:    16-byte random cookie
 *   bytes 22-149:  algorithm name-lists + trailing fields (deterministic)
 *   bytes 150-159: random padding
 *   Total: 160 bytes
 */
#define KEXINIT_SIZE             160
#define KEXINIT_COOKIE_OFFSET    6
#define KEXINIT_COOKIE_LEN       16
#define KEXINIT_NAMELISTS_OFFSET (KEXINIT_COOKIE_OFFSET + KEXINIT_COOKIE_LEN)
#define KEXINIT_NAMELISTS_LEN    128

/* Newkeys packet: 4 (pkt_len) + 1 (pad_len) + 1 (msg_id) + 10 (padding) = 16 */
#define NEWKEYS_SIZE 16

static int ssh_server_event_callback(struct ssh_server *sshd,
				     const struct ssh_server_event *event,
				     void *user_data)
{
	ARG_UNUSED(sshd);
	ARG_UNUSED(event);
	ARG_UNUSED(user_data);

	return 0;
}

static int ssh_server_transport_event_callback(struct ssh_transport *transport,
					       const struct ssh_transport_event *event,
					       void *user_data)
{
	ARG_UNUSED(transport);
	ARG_UNUSED(event);
	ARG_UNUSED(user_data);

	return 0;
}

/* Receive exactly 'needed' bytes from socket */
static void recv_exact(int fd, unsigned char *buf, size_t needed)
{
	size_t total = 0;
	int ret;

	while (total < needed) {
		ret = zsock_recv(fd, buf + total, needed - total, 0);
		zassert_true(ret > 0, "recv() failed (%d)", errno);
		total += ret;
	}
}

static void test_ssh_server(void)
{
	static const char *expected_identity = "SSH-2.0-zephyr\r\n";
	static const char *test_identity = "SSH-2.0-ztest\r\n";
	int ret;
	int client_fd;
	char *ptr;
	size_t len;
	struct net_sockaddr_in sa;
	struct net_sockaddr_in bind_addr;
	static unsigned char buf[4096];
	int host_key_index = 0;
	uint16_t key_size_bits = 2048;
	uint32_t pkt_len;
	struct ssh_server *sshd;
	int *authorized_keys = NULL;

	/*
	 * Expected deterministic part of kexinit: algorithm name-lists and
	 * trailing fields (first_kex_packet_follows + reserved). The random
	 * cookie and padding bytes are not compared, so this test works
	 * regardless of the entropy source or crypto backend.
	 */
	static const uint8_t expected_kexinit_namelists[] = {
		/* kex: "curve25519-sha256" */
		0x00, 0x00, 0x00, 0x11, 0x63, 0x75, 0x72, 0x76,
		0x65, 0x32, 0x35, 0x35, 0x31, 0x39, 0x2d, 0x73,
		0x68, 0x61, 0x32, 0x35, 0x36,
		/* host_key: "rsa-sha2-256" */
		0x00, 0x00, 0x00, 0x0c, 0x72, 0x73, 0x61, 0x2d,
		0x73, 0x68, 0x61, 0x32, 0x2d, 0x32, 0x35, 0x36,
		/* enc_c2s: "aes128-ctr" */
		0x00, 0x00, 0x00, 0x0a, 0x61, 0x65, 0x73, 0x31,
		0x32, 0x38, 0x2d, 0x63, 0x74, 0x72,
		/* enc_s2c: "aes128-ctr" */
		0x00, 0x00, 0x00, 0x0a, 0x61, 0x65, 0x73, 0x31,
		0x32, 0x38, 0x2d, 0x63, 0x74, 0x72,
		/* mac_c2s: "hmac-sha2-256" */
		0x00, 0x00, 0x00, 0x0d, 0x68, 0x6d, 0x61, 0x63,
		0x2d, 0x73, 0x68, 0x61, 0x32, 0x2d, 0x32, 0x35,
		0x36,
		/* mac_s2c: "hmac-sha2-256" */
		0x00, 0x00, 0x00, 0x0d, 0x68, 0x6d, 0x61, 0x63,
		0x2d, 0x73, 0x68, 0x61, 0x32, 0x2d, 0x32, 0x35,
		0x36,
		/* comp_c2s: "none" */
		0x00, 0x00, 0x00, 0x04, 0x6e, 0x6f, 0x6e, 0x65,
		/* comp_s2c: "none" */
		0x00, 0x00, 0x00, 0x04, 0x6e, 0x6f, 0x6e, 0x65,
		/* lang_c2s: "" */
		0x00, 0x00, 0x00, 0x00,
		/* lang_s2c: "" */
		0x00, 0x00, 0x00, 0x00,
		/* first_kex_packet_follows=0, reserved=0 */
		0x00, 0x00, 0x00, 0x00, 0x00
	};

	BUILD_ASSERT(sizeof(expected_kexinit_namelists) == KEXINIT_NAMELISTS_LEN);

	/* Client kex ecdh init (deterministic client ephemeral key) */
	const uint8_t client_kex_ecdh_init[] = {
		0x00, 0x00, 0x00, 0x2c, 0x06, 0x1e, 0x00, 0x00, 0x00,
		0x20, 0x20, 0x3d, 0x64, 0x36, 0xd4, 0x4a, 0x9b, 0xd2,
		0x14, 0x18, 0x64, 0x3e, 0xaf, 0x85, 0xad, 0x24, 0x0c,
		0x8b, 0x4e, 0x89, 0x40, 0xef, 0xff, 0x05, 0x88, 0x01,
		0x58, 0x0a, 0xbb, 0xa8, 0x4f, 0x37, 0xa2, 0xac, 0xf8,
		0x14, 0xfd, 0x71
	};

	zassert_ok(ssh_keygen(host_key_index, SSH_HOST_KEY_TYPE_RSA, key_size_bits),
		   "Failed to generate server host key");

	bind_addr.sin_family = NET_AF_INET;
	bind_addr.sin_port = net_htons(SSH_PORT);
	zsock_inet_pton(NET_AF_INET, MY_IPV4_ADDR, &bind_addr.sin_addr);

	sshd = ssh_server_instance(0);
	zassert_not_null(sshd);

	zassert_ok(ssh_server_start(sshd, (struct net_sockaddr *)&bind_addr, host_key_index,
				    NULL, NULL, authorized_keys, 0, ssh_server_event_callback,
				    ssh_server_transport_event_callback, NULL),
		   "Failed to start the server");

	k_sleep(K_MSEC(1));

	ret = zsock_socket(NET_AF_INET, NET_SOCK_STREAM, NET_IPPROTO_TCP);
	zassert_not_equal(ret, -1, "failed to create client socket (%d)", errno);
	client_fd = ret;

	sa.sin_family = NET_AF_INET;
	sa.sin_port = net_htons(SSH_PORT);

	ret = zsock_inet_pton(NET_AF_INET, MY_IPV4_ADDR, &sa.sin_addr.s_addr);
	zassert_not_equal(-1, ret, "inet_pton() failed (%d)", errno);
	zassert_not_equal(0, ret, "%s is not a valid IPv4 address", MY_IPV4_ADDR);
	zassert_equal(1, ret, "inet_pton() failed to convert %s", MY_IPV4_ADDR);

	memset(buf, '\0', sizeof(buf));
	ptr = (char *)zsock_inet_ntop(NET_AF_INET, &sa.sin_addr, buf, sizeof(buf));
	zassert_not_equal(ptr, NULL, "inet_ntop() failed (%d)", errno);

	ret = zsock_connect(client_fd, (struct net_sockaddr *)&sa, sizeof(sa));
	zassert_not_equal(ret, -1, "failed to connect (%s/%d)", strerror(errno), errno);

	/* Receive server identity */
	len = strlen(expected_identity);
	recv_exact(client_fd, buf, len);
	buf[len] = '\0';
	zassert_str_equal(expected_identity, buf, "Incorrect server identity");

	/* Send client identity */
	ret = zsock_send(client_fd, test_identity, strlen(test_identity), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	/* Receive server kexinit */
	recv_exact(client_fd, buf, KEXINIT_SIZE);
	zassert_equal(buf[5], SSH_MSG_KEXINIT,
		      "Wrong msg id: expected 0x%02x, got 0x%02x",
		      SSH_MSG_KEXINIT, buf[5]);
	zassert_mem_equal(expected_kexinit_namelists,
			  buf + KEXINIT_NAMELISTS_OFFSET,
			  KEXINIT_NAMELISTS_LEN,
			  "Incorrect server kexinit algorithms");

	/* Send received kexinit back as client kexinit (algorithms match) */
	ret = zsock_send(client_fd, buf, KEXINIT_SIZE, 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	/* Send client kex ecdh init */
	ret = zsock_send(client_fd, client_kex_ecdh_init, sizeof(client_kex_ecdh_init), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	/*
	 * Receive server kex ecdh reply. The content depends on the randomly
	 * generated host key and ephemeral ECDH key, so we only verify the
	 * message type and that the packet has a reasonable length.
	 */
	recv_exact(client_fd, buf, 4);
	pkt_len = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) |
		  ((uint32_t)buf[2] << 8) | (uint32_t)buf[3];
	zassert_true(pkt_len > 50 && pkt_len < 2000,
		     "Unreasonable kex ecdh reply packet length: %u", pkt_len);
	recv_exact(client_fd, buf + 4, pkt_len);
	zassert_equal(buf[5], SSH_MSG_KEX_ECDH_REPLY,
		      "Wrong msg id: expected 0x%02x, got 0x%02x",
		      SSH_MSG_KEX_ECDH_REPLY, buf[5]);

	/* Receive server newkeys */
	recv_exact(client_fd, buf, NEWKEYS_SIZE);
	zassert_equal(buf[5], SSH_MSG_NEWKEYS,
		      "Wrong msg id: expected 0x%02x, got 0x%02x",
		      SSH_MSG_NEWKEYS, buf[5]);

	/* Send client newkeys (echo back the received packet) */
	ret = zsock_send(client_fd, buf, NEWKEYS_SIZE, 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	k_sleep(K_SECONDS(1));

	zassert_ok(ssh_server_stop(sshd), "Failed to stop the server");

	k_sleep(K_SECONDS(1));
}

ZTEST(ssh_server, test_ssh_server)
{
	test_ssh_server();
}

ZTEST_SUITE(ssh_server, NULL, NULL, NULL, NULL, NULL);
