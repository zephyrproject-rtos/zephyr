/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <ifaddrs.h>
#include <signal.h>

#include <tinydtls.h>
#include <global.h>
#include <debug.h>
#include <dtls.h>

#ifdef __GNUC__
#define UNUSED_PARAM __attribute__((unused))
#else
#define UNUSED_PARAM
#endif /* __GNUC__ */

#define SERVER_PORT  4242
#define CLIENT_PORT  8484
#define MAX_BUF_SIZE 1280	/* min IPv6 MTU, the actual data is smaller */
#define MAX_TIMEOUT  3		/* in seconds */

static bool debug;
static int renegotiate = -1;
static int packets;

struct server_data {
	int fd;
	int len;
	int packet;
#define MAX_READ_BUF 2000
	uint8 buf[MAX_READ_BUF];
};

static inline void reverse(unsigned char *buf, int len)
{
	int i, last = len - 1;

	for(i = 0; i < len/2; i++) {
		unsigned char tmp = buf[i];
		buf[i] = buf[last - i];
		buf[last - i] = tmp;
	}
}

static int get_ifindex(const char *name)
{
	struct ifreq ifr;
	int sk, err;

	if (!name)
		return -1;

	sk = socket(PF_INET6, SOCK_DGRAM | SOCK_CLOEXEC, 0);
	if (sk < 0)
		return -1;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name) - 1);

	err = ioctl(sk, SIOCGIFINDEX, &ifr);

	close(sk);

	if (err < 0)
		return -1;

	return ifr.ifr_ifindex;
}

static int get_socket(int family)
{
	int fd;

	fd = socket(family, SOCK_DGRAM, IPPROTO_UDP);
	if (fd < 0) {
		perror("socket");
		exit(-errno);
	}
	return fd;
}

static int bind_device(int fd, const char *interface, void *addr, int len)
{
	struct ifreq ifr;
	int ret, val = 1;

	memset(&ifr, 0, sizeof(ifr));
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), interface);

	if (setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE,
		       (void *)&ifr, sizeof(ifr)) < 0) {
		perror("SO_BINDTODEVICE");
		exit(-errno);
	}

	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

	ret = bind(fd, (struct sockaddr *)addr, len);
	if (ret < 0) {
		perror("bind");
		exit(-errno);
	}

	return ret;
}

static int get_address(int ifindex, int family, void *address)
{
	struct ifaddrs *ifaddr, *ifa;
	int err = -ENOENT;
	char name[IF_NAMESIZE];

	if (!if_indextoname(ifindex, name))
		return -EINVAL;

	if (getifaddrs(&ifaddr) < 0) {
		err = -errno;
		fprintf(stderr, "Cannot get addresses err %d/%s",
			err, strerror(-err));
		return err;
	}

	for (ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
		if (!ifa->ifa_addr)
			continue;

		if (strncmp(ifa->ifa_name, name, IF_NAMESIZE) == 0 &&
					ifa->ifa_addr->sa_family == family) {
			if (family == AF_INET) {
				struct sockaddr_in *in4 = (struct sockaddr_in *)
					ifa->ifa_addr;
				if (in4->sin_addr.s_addr == INADDR_ANY)
					continue;
				if ((in4->sin_addr.s_addr & IN_CLASSB_NET) ==
						((in_addr_t) 0xa9fe0000))
					continue;
				memcpy(address, &in4->sin_addr,
							sizeof(struct in_addr));
			} else if (family == AF_INET6) {
				struct sockaddr_in6 *in6 =
					(struct sockaddr_in6 *)ifa->ifa_addr;
				if (memcmp(&in6->sin6_addr, &in6addr_any,
						sizeof(struct in6_addr)) == 0)
					continue;
				if (IN6_IS_ADDR_LINKLOCAL(&in6->sin6_addr))
					continue;

				memcpy(address, &in6->sin6_addr,
						sizeof(struct in6_addr));
			} else {
				err = -EINVAL;
				goto out;
			}

			err = 0;
			break;
		}
	}

out:
	freeifaddrs(ifaddr);
	return err;
}

#define PSK_DEFAULT_IDENTITY "Client_identity"
#define PSK_DEFAULT_KEY      "secretPSK"
#define PSK_OPTIONS          "i:k:"

static bool quit = false;
static dtls_context_t *dtls_context;

static const unsigned char ecdsa_priv_key[] = {
			0x41, 0xC1, 0xCB, 0x6B, 0x51, 0x24, 0x7A, 0x14,
			0x43, 0x21, 0x43, 0x5B, 0x7A, 0x80, 0xE7, 0x14,
			0x89, 0x6A, 0x33, 0xBB, 0xAD, 0x72, 0x94, 0xCA,
			0x40, 0x14, 0x55, 0xA1, 0x94, 0xA9, 0x49, 0xFA};

static const unsigned char ecdsa_pub_key_x[] = {
			0x36, 0xDF, 0xE2, 0xC6, 0xF9, 0xF2, 0xED, 0x29,
			0xDA, 0x0A, 0x9A, 0x8F, 0x62, 0x68, 0x4E, 0x91,
			0x63, 0x75, 0xBA, 0x10, 0x30, 0x0C, 0x28, 0xC5,
			0xE4, 0x7C, 0xFB, 0xF2, 0x5F, 0xA5, 0x8F, 0x52};

static const unsigned char ecdsa_pub_key_y[] = {
			0x71, 0xA0, 0xD4, 0xFC, 0xDE, 0x1A, 0xB8, 0x78,
			0x5A, 0x3C, 0x78, 0x69, 0x35, 0xA7, 0xCF, 0xAB,
			0xE9, 0x3F, 0x98, 0x72, 0x09, 0xDA, 0xED, 0x0B,
			0x4F, 0xAB, 0xC3, 0x6F, 0xC7, 0x72, 0xF8, 0x29};

#ifdef DTLS_PSK
/* The PSK information for DTLS */
#define PSK_ID_MAXLEN 256
#define PSK_MAXLEN 256
static unsigned char psk_id[PSK_ID_MAXLEN];
static size_t psk_id_length = 0;
static unsigned char psk_key[PSK_MAXLEN];
static size_t psk_key_length = 0;

/* This function is the "key store" for tinyDTLS. It is called to
 * retrieve a key for the given identity within this particular
 * session. */
static int get_psk_info(struct dtls_context_t *ctx UNUSED_PARAM,
			const session_t *session UNUSED_PARAM,
			dtls_credentials_type_t type,
			const unsigned char *id, size_t id_len,
			unsigned char *result, size_t result_length)
{
	switch (type) {
	case DTLS_PSK_IDENTITY:
		if (id_len) {
			dtls_debug("got psk_identity_hint: '%.*s'\n", id_len,
				   id);
		}

		if (result_length < psk_id_length) {
			dtls_warn("cannot set psk_identity -- buffer too small\n");
			return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
		}

		memcpy(result, psk_id, psk_id_length);
		return psk_id_length;

	case DTLS_PSK_KEY:
		if (id_len != psk_id_length || memcmp(psk_id, id, id_len) != 0) {
			dtls_warn("PSK for unknown id requested, exiting\n");
			return dtls_alert_fatal_create(DTLS_ALERT_ILLEGAL_PARAMETER);
		} else if (result_length < psk_key_length) {
			dtls_warn("cannot set psk -- buffer too small\n");
			return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
		}

		memcpy(result, psk_key, psk_key_length);
		return psk_key_length;

	default:
		dtls_warn("unsupported request type: %d\n", type);
	}

	return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
}
#endif /* DTLS_PSK */

#ifdef DTLS_ECC
static int get_ecdsa_key(struct dtls_context_t *ctx,
			 const session_t *session,
			 const dtls_ecdsa_key_t **result)
{
	static const dtls_ecdsa_key_t ecdsa_key = {
		.curve = DTLS_ECDH_CURVE_SECP256R1,
		.priv_key = ecdsa_priv_key,
		.pub_key_x = ecdsa_pub_key_x,
		.pub_key_y = ecdsa_pub_key_y
	};

	*result = &ecdsa_key;
	return 0;
}

static int verify_ecdsa_key(struct dtls_context_t *ctx,
			    const session_t *session,
			    const unsigned char *other_pub_x,
			    const unsigned char *other_pub_y,
			    size_t key_size)
{
	return 0;
}
#endif /* DTLS_ECC */

static void print_data(const unsigned char *packet, int length)
{
	int n = 0;

	while (length--) {
		if (n % 16 == 0)
			printf("%X: ", n);

		printf("%X ", *packet++);

		n++;
		if (n % 8 == 0) {
			if (n % 16 == 0)
				printf("\n");
			else
				printf(" ");
		}
	}
	printf("\n");
}

static int read_from_peer(struct dtls_context_t *ctx,
			  session_t *session,
			  uint8 *read_data, size_t read_len)
{
	struct server_data *user_data =
			(struct server_data *)dtls_get_app_data(ctx);
	int ret;

	printf("Read from peer %d bytes\n", read_len);

	reverse(read_data, read_len);

	if (debug)
		print_data(read_data, read_len);

	ret = dtls_write(ctx, session, read_data, read_len);
	if (ret < 0) {
		/* Failure */
		quit = true;
		return ret;
	}

	user_data->packet++;
	if (user_data->packet == renegotiate) {
		printf("Starting to renegotiate keys\n");
		dtls_renegotiate(ctx, session);
		return 0;
	}

	return 0;
}

static inline void sleep_ms(int ms)
{
	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = ms * 1000;

	select(1, NULL, NULL, NULL, &tv);
}

static int send_to_peer(struct dtls_context_t *ctx,
			session_t *session,
			uint8 *data, size_t len)
{
	struct server_data *user_data =
			(struct server_data *)dtls_get_app_data(ctx);

	/* The Qemu uart driver can loose chars if sent too fast.
	 * So before sending more data, sleep a while.
	 */
	sleep_ms(200);

	printf("Sending to peer data %p len %d\n", data, len);
	return sendto(user_data->fd, data, len, 0,
		      &session->addr.sa, session->size);
}

static int handle_event(struct dtls_context_t *ctx, session_t *session,
			dtls_alert_level_t level, unsigned short code)
{
	dtls_debug("event: level %d code %d\n", level, code);

	if (level > 0) {
		/* alert code, quit */
	} else if (level == 0) {
		/* internal event */
		if (code == DTLS_EVENT_CONNECTED)
			printf("*** Connected ***\n");
	}

	return 0;
}

static dtls_handler_t cb = {
	.write = send_to_peer,
	.read  = read_from_peer,
	.event = handle_event,
#ifdef DTLS_PSK
	.get_psk_info = get_psk_info,
#endif /* DTLS_PSK */
#ifdef DTLS_ECC
	.get_ecdsa_key = get_ecdsa_key,
	.verify_ecdsa_key = verify_ecdsa_key
#endif /* DTLS_ECC */
};

void signal_handler(int signum)
{
	switch (signum) {
	case SIGINT:
	case SIGTERM:
		quit = true;
		break;
	}
}

static int dtls_handle_read(struct dtls_context_t *ctx,
			    session_t *session,
			    struct server_data *user_data)
{
	user_data->len = recvfrom(user_data->fd,
				  user_data->buf, sizeof(user_data->buf),
				  MSG_TRUNC,
				  &session->addr.sa, &session->size);

	if (user_data->len < 0) {
		perror("recvfrom");
		return -1;
	} else {
		dtls_dsrv_log_addr(DTLS_LOG_DEBUG, "peer", session);
		dtls_debug_dump("bytes from peer", user_data->buf,
				user_data->len);

		if (sizeof(user_data->buf) < user_data->len) {
			dtls_warn("packet was truncated (%d bytes lost)\n",
				  user_data->len - sizeof(user_data->buf));
		}
	}

	return dtls_handle_message(ctx, session,
				   user_data->buf, user_data->len);
}

static int receive_and_reply(dtls_context_t *ctx, session_t *session,
			     fd_set *rfds, int fd)
{
	if (FD_ISSET(fd, rfds)) {
		struct server_data *user_data = dtls_get_app_data(ctx);
		struct sockaddr from;
		socklen_t fromlen;
		int ret;

		user_data->fd = fd;

		ret = dtls_handle_read(dtls_context, session, user_data);
		if (ret < 0)
			return ret;
	}

	return 0;
}

extern int optind, opterr, optopt;
extern char *optarg;

/* The application returns:
 *    < 0 : connection or similar error
 *      0 : no errors, all tests passed
 *    > 0 : could not send all the data to server
 */
int main(int argc, char**argv)
{
	int c, ret, fd4, fd6, timeout = 0;
	struct sockaddr_in6 addr6_send = { 0 }, addr6_recv = { 0 };
	struct sockaddr_in addr4_send = { 0 }, addr4_recv = { 0 };
	struct sockaddr *addr_send, *addr_recv;
	int addr_len;
	char addr_buf[INET6_ADDRSTRLEN];
	const struct in6_addr any = IN6ADDR_ANY_INIT;
	const char *interface = NULL;
	fd_set rfds;
	struct timeval tv = {};
	int ifindex = -1, on, port;
	void *address = NULL;
	struct server_data user_data;
	session_t session;
	bool do_renegotiate = false;

#ifdef DTLS_PSK
	psk_id_length = strlen(PSK_DEFAULT_IDENTITY);
	psk_key_length = strlen(PSK_DEFAULT_KEY);
	memcpy(psk_id, PSK_DEFAULT_IDENTITY, psk_id_length);
	memcpy(psk_key, PSK_DEFAULT_KEY, psk_key_length);
#endif /* DTLS_PSK */

	opterr = 0;

	while ((c = getopt(argc, argv, "i:Drp:")) != -1) {
		switch (c) {
		case 'i':
			interface = optarg;
			break;
		case 'p':
			packets = atoi(optarg);
			break;
		case 'r':
			/* Do a renegotiate once during the test run. */
			do_renegotiate = true;
			break;
		case 'D':
			debug = true;
			break;
		}
	}

	if (!interface) {
		printf("usage: %s [-D] [-r] [-p count] -i <iface>\n", argv[0]);
		printf("\t-i Use this network interface.\n");
		printf("\t-p How many packets to wait before quitting.\n");
		printf("\t-r Renegoating keys once during the test run.\n");
		printf("\t-D Activate debugging.\n");
		exit(-EINVAL);
	}

	ifindex = get_ifindex(interface);
	if (ifindex < 0) {
		printf("Invalid interface %s\n", interface);
		exit(-EINVAL);
	}

	if (do_renegotiate) {
		if (packets > 0) {
			renegotiate = random() % packets;
			printf("Renegotating after %d messages.\n",
			       renegotiate);
		} else {
			printf("You need to give packet count, "
			       "-r option ignored.\n");
		}
	}

	printf("Interface %s\n", interface);

	addr4_recv.sin_family = AF_INET;
	addr4_recv.sin_port = htons(SERVER_PORT);

	/* We want to bind to global unicast address here so that
	 * we can listen correct addresses. We do not want to listen
	 * link local addresses in this test.
	 */
	get_address(ifindex, AF_INET, &addr4_recv.sin_addr);
	printf("IPv4: binding to %s\n",
	       inet_ntop(AF_INET, &addr4_recv.sin_addr,
			 addr_buf, sizeof(addr_buf)));

	addr6_recv.sin6_family = AF_INET6;
	addr6_recv.sin6_port = htons(SERVER_PORT);

	/* Bind to global unicast address instead of ll address */
	get_address(ifindex, AF_INET6, &addr6_recv.sin6_addr);
	printf("IPv6: binding to %s\n",
	       inet_ntop(AF_INET6, &addr6_recv.sin6_addr,
			 addr_buf, sizeof(addr_buf)));

	fd4 = get_socket(AF_INET);
	fd6 = get_socket(AF_INET6);

	bind_device(fd4, interface, &addr4_recv, sizeof(addr4_recv));
	bind_device(fd6, interface, &addr6_recv, sizeof(addr6_recv));

	on = 1;
#ifdef IPV6_RECVPKTINFO
	if (setsockopt(fd6, IPPROTO_IPV6, IPV6_RECVPKTINFO, &on,
		       sizeof(on) ) < 0) {
#else /* IPV6_RECVPKTINFO */
	if (setsockopt(fd6, IPPROTO_IPV6, IPV6_PKTINFO, &on,
		       sizeof(on) ) < 0) {
#endif /* IPV6_RECVPKTINFO */
		printf("setsockopt IPV6_PKTINFO: %s\n", strerror(errno));
	}

	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	dtls_init();

	dtls_session_init(&session);

	memset(&user_data, 0, sizeof(user_data));

	dtls_context = dtls_new_context(&user_data);
	if (!dtls_context) {
		dtls_emerg("cannot create context\n");
		exit(-EINVAL);
	}

	dtls_set_handler(dtls_context, &cb);

	if (debug)
		dtls_set_log_level(DTLS_LOG_DEBUG);

#define MAX(a,b) (((a) > (b)) ? (a) : (b))

	do {
		int fd = MAX(fd4, fd6);

		FD_ZERO(&rfds);
		FD_SET(fd4, &rfds);
		FD_SET(fd6, &rfds);

		tv.tv_sec = MAX_TIMEOUT;
		tv.tv_usec = 0;

		ret = select(fd + 1, &rfds, NULL, NULL, &tv);
		if (ret < 0) {
			perror("select");
			break;
		} else if (ret == 0) {
			if (quit)
				break;
		} else if (!FD_ISSET(fd, &rfds)) {
			fprintf(stderr, "Invalid fd in read, quitting.\n");
			ret = -EINVAL;
			break;
		}

		if (receive_and_reply(dtls_context, &session, &rfds, fd4) < 0)
			break;

		if (receive_and_reply(dtls_context, &session, &rfds, fd6) < 0)
			break;

		if (packets && user_data.packet >= packets) {
			printf("Received %d packets, quitting.\n", packets);
			break;
		}

	} while(!quit);

	close(fd4);
	close(fd6);

	printf("\n");

	dtls_close(dtls_context, &session);

	dtls_free_context(dtls_context);

	exit(ret);
}
