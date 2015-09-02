/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* Various utility functions taken from libcoap with this license:

Copyright (c) 2010--2015, Olaf Bergmann
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

  o Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

  o Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

/* tinyDTLS and libcoap have same preprocessor symbols defined in their
 * include files so undef conflicting symbols here.
 */
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_URL
#undef PACKAGE_VERSION
#undef UTHASH_VERSION
#undef HASH_FIND
#undef HASH_ADD
#undef HASH_ADD_KEYPTR
#undef HASH_DELETE
#undef HASH_FIND_STR
#undef HASH_ADD_STR
#undef HASH_BER
#undef HASH_FNV
#undef HASH_JEN
#undef HASH_SFH
#undef HASH_FIND_IN_BKT
#undef HASH_SRT
#undef HASH_CLEAR
#undef UTLIST_VERSION
#undef _NEXT
#undef _NEXTASGN
#undef _PREVASGN
#undef LL_SORT
#undef DL_SORT
#undef CDL_SORT
#undef LL_PREPEND
#undef LL_APPEND
#undef LL_DELETE
#undef LL_APPEND_VS2008
#undef LL_DELETE_VS2008
#undef LL_FOREACH
#undef LL_FOREACH_SAFE
#undef LL_SEARCH_SCALAR
#undef LL_SEARCH
#undef DL_PREPEND
#undef DL_APPEND
#undef DL_DELETE
#undef DL_FOREACH
#undef DL_FOREACH_SAFE
#undef CDL_PREPEND
#undef CDL_DELETE
#undef CDL_FOREACH
#undef CDL_FOREACH_SAFE
#undef CDL_SEARCH_SCALAR
#undef CDL_SEARCH
#define UTHASH_H

#include <coap/coap.h>

typedef struct coap_list_t {
	struct coap_list_t *next;
	char data[];
} coap_list_t;

#ifdef __GNUC__
#define UNUSED_PARAM __attribute__((unused))
#else
#define UNUSED_PARAM
#endif /* __GNUC__ */

#define SERVER_PORT        5683
#define SERVER_SECURE_PORT 5684

#define CLIENT_PORT  8484
#define MAX_BUF_SIZE 1280	/* min IPv6 MTU, the actual data is smaller */
#define MAX_TIMEOUT  6		/* in seconds */

static bool debug;
static int renegotiate = -1;
static session_t session;
static bool no_dtls;
static const char *target;
static int family;

static coap_block_t block = {
	.num = 0,
	.m = 0,
	.szx = 6
};
static unsigned int wait_seconds = MAX_TIMEOUT; /* default timeout in secs */
static coap_tick_t max_wait;
static unsigned int obs_seconds = MAX_TIMEOUT * 2; /* default observe time */
static coap_tick_t obs_wait = 0;	/* timeout for current subscription */
static int ready = 0;
static coap_list_t *optlist;
static unsigned char _token_data[8];
static str the_token = { 0, _token_data };
typedef unsigned char method_t;
static str payload = { 0, NULL }; /* optional payload to send */
static int flags = 0;
#define FLAGS_BLOCK 0x01
static method_t method;

static coap_pdu_t *coap_new_request(coap_context_t *ctx,
				    method_t m,
				    coap_list_t **options,
				    const unsigned char *data,
				    size_t length,
				    int msgtype);

#define ENTRY(desc, entry, expect_result, method, test_data, length, mid,\
	      confirmed)						\
	{								\
		.description = #desc ,					\
		.len = sizeof(entry),					\
		.buf = entry,						\
		.expecting_reply = expect_result,			\
		.data = test_data,					\
		.data_len = length,					\
		.coap_method = COAP_REQUEST_ ## method,			\
		.check_mid = mid,					\
		.message_type = confirmed,				\
	}

#define ENTRY2(desc, entry, expect_result, method, test_data, length, \
	       test_payload, test_payload_len, mid, confirmed)		\
	{								\
		.description = #desc ,					\
		.len = sizeof(entry),					\
		.buf = entry,						\
		.expecting_reply = expect_result,			\
		.data = test_data,					\
		.data_len = length,					\
		.payload = test_payload,				\
		.payload_len = test_payload_len,			\
		.coap_method = COAP_REQUEST_ ## method,			\
		.check_mid = mid,					\
		.message_type = confirmed,				\
	}

#define ENTRY3(desc, entry, expect_result, method, test_data, length, mid,\
	      confirmed)						\
	{								\
		.description = #desc ,					\
		.len = sizeof(entry),					\
		.buf = entry,						\
		.expecting_reply = expect_result,			\
		.data = test_data,					\
		.data_len = length,					\
		.coap_method = COAP_REQUEST_ ## method,			\
		.check_mid = mid,					\
		.message_type = confirmed,				\
		.add_token = true,					\
	}

#define ENTRY4(desc, entry, expect_result, method, test_data, length, \
	       test_payload, test_payload_len, mid, confirmed)		\
	{								\
		.description = #desc ,					\
		.len = sizeof(entry),					\
		.buf = entry,						\
		.expecting_reply = expect_result,			\
		.data = test_data,					\
		.data_len = length,					\
		.payload = test_payload,				\
		.payload_len = test_payload_len,			\
		.coap_method = COAP_REQUEST_ ## method,			\
		.check_mid = mid,					\
		.message_type = confirmed,				\
		.add_token = true,					\
	}

#define GET_CON(desc, e, r, c) ENTRY(desc, e, true, GET, r, sizeof(r), c, \
				 COAP_MESSAGE_CON)
#define POST_CON(desc, e, r, c) ENTRY(desc, e, true, POST, r, sizeof(r), c, \
				  COAP_MESSAGE_CON)
#define PUT_CON(desc, e, d, p, c) ENTRY2(desc, e, true, PUT, d, sizeof(d), \
					p, sizeof(p), c, COAP_MESSAGE_CON)
#define DELETE_CON(desc,e, r, c) ENTRY(desc, e, true, DELETE, r, sizeof(r), c,\
				   COAP_MESSAGE_CON)

#define GET_NON(desc, e, r, c) ENTRY(desc, e, true, GET, r, sizeof(r), c, \
				     COAP_MESSAGE_NON)
#define POST_NON(desc, e, r, c) ENTRY(desc, e, true, POST, r, sizeof(r), c, \
				      COAP_MESSAGE_NON)
#define PUT_NON(desc, e, d, p, c) ENTRY2(desc, e, true, PUT, d, sizeof(d), \
					 p, sizeof(p), c, COAP_MESSAGE_NON)
#define DELETE_NON(desc,e, r, c) ENTRY(desc, e, true, DELETE, r, sizeof(r), c,\
				       COAP_MESSAGE_NON)

#define GET_CON_TOKEN(desc, e, r, c) ENTRY3(desc, e, true, GET, r, sizeof(r), \
					    c, COAP_MESSAGE_CON)
#define POST_CON_TOKEN(desc, e, r, c) ENTRY3(desc, e, true, POST, r,	\
					     sizeof(r), c, COAP_MESSAGE_CON)
#define PUT_CON_TOKEN(desc, e, d, p, c) ENTRY4(desc, e, true, PUT, d, \
					       sizeof(d), p, sizeof(p), c,\
					       COAP_MESSAGE_CON)
#define DELETE_CON_TOKEN(desc,e, r, c) ENTRY3(desc, e, true, DELETE, r, \
					      sizeof(r), c, COAP_MESSAGE_CON)

#define GET_CON_PAYLOAD(desc, e, d, p, c) ENTRY2(desc, e, true, GET, \
						 d, sizeof(d), \
						 p, sizeof(p), c, \
						 COAP_MESSAGE_CON)

#define RES(var, path)	\
	static const char res_ ## var [] = path ;

#define DATA_ARRAY(var, data, ...) \
	static const char var [] = { data, __VA_ARGS__ } ;

#define DATA_STRING(var, data) \
	static const char var [] = data;

RES(core, ".well-known/core");
RES(test, "test");
RES(seg, "seg1/seg2/seg3");
RES(query, "query");

/* See this test specification document for different test descriptions
 * http://www.etsi.org/plugtests/CoAP/Document/CoAP_TestDescriptions_v015.pdf
 */
DATA_STRING(get_con, "Type: 0\nCode: 1\n");
DATA_STRING(post_con, "Type: 0\nCode: 2\n");
DATA_STRING(put_con, "Type: 0\nCode: 3\n");
DATA_STRING(delete_con, "Type: 0\nCode: 4\n");
DATA_STRING(get_non, "Type: 1\nCode: 1\n");
DATA_STRING(post_non, "Type: 1\nCode: 2\n");
DATA_STRING(put_non, "Type: 1\nCode: 3\n");
DATA_STRING(delete_non, "Type: 1\nCode: 4\n");

/* Generated by http://www.lipsum.com/
 * 1202 bytes of Lorem Ipsum.
 *
 * This is the maximum we can send with encryption.
 */
static const char lorem_ipsum[] =
	"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin congue orci et lectus ultricies, sed elementum urna finibus. Nam bibendum, massa id sollicitudin finibus, massa ante pharetra lacus, nec semper felis metus eu massa. Curabitur gravida, neque a pulvinar suscipit, felis massa maximus neque, eu sagittis felis enim nec justo. Suspendisse sit amet sem a magna aliquam tincidunt. Mauris consequat ante in consequat auctor. Nam eu congue mauris, congue aliquet metus. Etiam elit ipsum, vehicula et lectus at, dignissim accumsan turpis. Sed magna nisl, tempor ut dolor sed, feugiat pharetra velit. Nulla sed purus at elit dapibus lobortis. In hac habitasse platea dictumst. Praesent quis libero id enim aliquet viverra eleifend non urna. Vivamus metus justo, dignissim eget libero molestie, tincidunt pellentesque purus. Quisque pulvinar, nisi sed egestas vestibulum, ante felis elementum justo, ut viverra nisl est sagittis leo. Curabitur pharetra eros at felis ultricies efficitur."
	"\n"
	"Ut rutrum urna vitae neque rhoncus, id dictum ex dictum. Suspendisse venenatis vel mauris sed maximus. Sed malesuada elit vel neque hendrerit, in accumsan odio sodales. Aliquam erat volutpat. Praesent non situ.\n";

static const char lorem_ipsum_short[] = "Lorem ipsum dolor sit amet.";

static struct data {
	const char *description;
	int len;
	method_t coap_method;
	const unsigned char *buf;
	bool expecting_reply;
	const unsigned char *data; /* data to be sent */
	int data_len;
	const unsigned char *payload; /* possible payload to be sent */
	int payload_len;
	bool check_mid;
	int expected_mid; /* message id we sent */
	int message_type;
	bool add_token;
} data[] = {
	GET_CON(TD_COAP_CORE_01, res_test, get_con, true),
	POST_CON(TD_COAP_CORE_02, res_test, post_con, false),
	PUT_CON(TD_COAP_CORE_03, res_test, put_con, lorem_ipsum, false),
	DELETE_CON(TD_COAP_CORE_04, res_test, delete_con, false),

	GET_NON(TD_COAP_CORE_05, res_test, get_non, true),
	POST_NON(TD_COAP_CORE_06, res_test, post_non, false),
	PUT_NON(TD_COAP_CORE_07, res_test, put_non, lorem_ipsum, false),
	DELETE_NON(TD_COAP_CORE_08, res_test, delete_non, false),

	/* /separate not supported atm */
	//GET_CON(TD_COAP_CORE_09, res_separate, get_con, true),

	GET_CON_TOKEN(TD_COAP_CORE_10, res_test, get_con, true),

	/* Isn't test case 11 same as test case 01? */
	GET_CON(TD_COAP_CORE_11, res_test, get_con, true),

	GET_CON(TD_COAP_CORE_12, res_seg, get_con, true),
	GET_CON(TD_COAP_CORE_13, res_query, get_con, true),

	/* Test 14 & 15 are for testing gateway scenario so not used here */
	// TD_COAP_CORE_14
	// TD_COAP_CORE_15

	/* /separate not supported atm */
	//GET_NON(TD_COAP_CORE_16, res_separate, get_con, true),

#if TO_BE_DONE
	GET_CON(TD_COAP_LINK_01, res_core, get_con, true),
	//TD_COAP_LINK_02

	// TD_COAP_BLOCK_01
	// TD_COAP_BLOCK_02
	// TD_COAP_BLOCK_03
	// TD_COAP_BLOCK_04

	// TD_COAP_OBS_01
	// TD_COAP_OBS_02
	// TD_COAP_OBS_03
	// TD_COAP_OBS_04
	// TD_COAP_OBS_05
#endif

	{ 0, 0 }
};

struct client_data {
	bool fail;
	int fd;
	int index; /* position in data[] */
	int ifindex; /* network interface index */
	coap_context_t *coap_ctx;
	coap_address_t coap_dst;
	int len;
#define MAX_READ_BUF 2000
	uint8 buf[MAX_READ_BUF];
};
static struct client_data *test_context;

static void set_coap_timeout(coap_tick_t *timer, const unsigned int seconds)
{
	coap_ticks(timer);
	*timer += seconds * COAP_TICKS_PER_SECOND;
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

static coap_list_t *new_option_node(unsigned short key,
				    unsigned int length,
				    unsigned char *data)
{
	coap_list_t *node;

	node = coap_malloc(sizeof(coap_list_t) + sizeof(coap_option) + length);

	if (node) {
		coap_option *option = (coap_option *)(node->data);

		COAP_OPTION_KEY(*option) = key;
		COAP_OPTION_LENGTH(*option) = length;
		memcpy(COAP_OPTION_DATA(*option), data, length);
	} else {
		coap_log(LOG_DEBUG, "new_option_node: malloc\n");
	}

	return node;
}

static int coap_insert(coap_list_t **head, coap_list_t *node)
{
	if (!node) {
		coap_log(LOG_WARNING, "cannot create option Proxy-Uri\n");
	} else {
		LL_APPEND((*head), node);
	}

	return node != NULL;
}

static int coap_delete(coap_list_t *node)
{
	if (node) {
		coap_free(node);
	}
	return 1;
}

static void coap_delete_list(coap_list_t *queue)
{
	coap_list_t *elt, *tmp;

	if (!queue)
		return;

	LL_FOREACH_SAFE(queue, elt, tmp) {
		coap_delete(elt);
	}
}

static coap_list_t *parse_uri(const char *arg)
{
	coap_uri_t uri;
	coap_list_t *opts = NULL;
#define BUFSIZE 40
	unsigned char _buf[BUFSIZE];
	unsigned char *buf = _buf;
	size_t buflen;
	int res;

	coap_split_uri((unsigned char *)arg, strlen(arg), &uri );

	if (uri.path.length) {
		buflen = BUFSIZE;
		res = coap_split_path(uri.path.s, uri.path.length,
				      buf, &buflen);

		while (res--) {
			coap_insert(&opts,
				    new_option_node(COAP_OPTION_URI_PATH,
						    COAP_OPT_LENGTH(buf),
						    COAP_OPT_VALUE(buf)));

			buf += COAP_OPT_SIZE(buf);
		}
	}

	if (uri.query.length) {
		buflen = BUFSIZE;
		buf = _buf;
		res = coap_split_query(uri.query.s, uri.query.length,
				       buf, &buflen);

		while (res--) {
			coap_insert(&opts,
				    new_option_node(COAP_OPTION_URI_QUERY,
						    COAP_OPT_LENGTH(buf),
						    COAP_OPT_VALUE(buf)));

			buf += COAP_OPT_SIZE(buf);
		}
	}

	return opts;
}

static coap_list_t *clone_option(coap_list_t *item)
{
	coap_list_t *node;
	coap_option *option = (coap_option *)(item->data);

	node = coap_malloc(sizeof(coap_list_t) + sizeof(coap_option) +
			   option->length);
	if (node) {
		memcpy(node->data, option,
		       sizeof(coap_option) + option->length);
		return node;
	} else
		return NULL;
}

static char *create_uri(char *uri, int len, const char *target,
			struct client_data *user_data)
{
	snprintf(uri, len, "coap://%s%s%s/%s",
		 family == AF_INET6 ? "[" : "",
		 target,
		 family == AF_INET6 ? "]" : "",
		 data[user_data->index].buf);

	if (debug)
		print_data(uri, data[user_data->index].len);

	return uri;
}

static coap_pdu_t *create_pdu(struct client_data *user_data)
{
	coap_pdu_t *pdu;
	coap_list_t *opts;
	coap_list_t *elt, *tmp;
#define MAX_URI 256
	char uri_buf[MAX_URI], *uri;
	const unsigned char *send_data = NULL;
	int send_data_len = 0;

	uri = create_uri(uri_buf, MAX_URI, target, user_data);
	opts = parse_uri(uri);

	LL_FOREACH_SAFE(optlist, elt, tmp) {
		coap_insert(&opts,
			    clone_option(elt));
	}

	if (data[user_data->index].data) {
		send_data = data[user_data->index].data;
		send_data_len = data[user_data->index].data_len;
		if (send_data[send_data_len] == '\0')
			send_data_len--; /* skip the null at the end */
	}
	pdu = coap_new_request(user_data->coap_ctx,
			       data[user_data->index].coap_method,
			       &opts,
			       send_data, send_data_len,
			       data[user_data->index].message_type);
	if (pdu) {
		if (data[user_data->index].check_mid)
			data[user_data->index].expected_mid = ntohs(pdu->hdr->id);
	}
	coap_delete_list(opts);

	return pdu;
}

static void send_packets(struct client_data *user_data)
{
	int ret;
	coap_pdu_t *pdu;
	coap_tid_t tid;

	printf("%s: sending [%d] %d bytes\n",
	       data[user_data->index].description,
	       user_data->index, data[user_data->index].len);

	test_context = user_data;

	pdu = create_pdu(user_data);
	if (!pdu) {
		/* Failure */
		quit = true;
		user_data->fail = true;
		printf("Cannot allocate pdu\n");
		return;
	}

	method = data[user_data->index].coap_method;

	if (coap_get_log_level() >= LOG_DEBUG) {
		printf("sending CoAP request:\n");
		coap_show_pdu(pdu);
	}

	if (pdu->hdr->type == COAP_MESSAGE_CON)
		tid = coap_send_confirmed(user_data->coap_ctx,
					  user_data->coap_ctx->endpoint,
					  &user_data->coap_dst,
					  pdu);
	else
		tid = coap_send(user_data->coap_ctx,
				user_data->coap_ctx->endpoint,
				&user_data->coap_dst,
				pdu);

	if (pdu->hdr->type != COAP_MESSAGE_CON || tid == COAP_INVALID_TID)
		coap_delete_pdu(pdu);

	set_coap_timeout(&max_wait, wait_seconds);

	if (debug)
		printf("timeout is set to %d seconds\n", wait_seconds);
}

static void try_send(struct dtls_context_t *ctx)
{
	struct client_data *user_data =
			(struct client_data *)dtls_get_app_data(ctx);
	int ret;
	coap_pdu_t *pdu;
	coap_tid_t tid;

	printf("%s: sending [%d] %d bytes\n",
	       data[user_data->index].description,
	       user_data->index, data[user_data->index].len);

	test_context = user_data;

	pdu = create_pdu(user_data);
	if (!pdu) {
		/* Failure */
		quit = true;
		user_data->fail = true;
		printf("Cannot allocate pdu\n");
		return;
	}

	method = data[user_data->index].coap_method;

	if (coap_get_log_level() >= LOG_DEBUG) {
		printf("sending CoAP request:\n");
		coap_show_pdu(pdu);
	}

	if (pdu->hdr->type == COAP_MESSAGE_CON)
		tid = coap_send_confirmed(user_data->coap_ctx,
					  user_data->coap_ctx->endpoint,
					  &user_data->coap_dst,
					  pdu);
	else
		tid = coap_send(user_data->coap_ctx,
				user_data->coap_ctx->endpoint,
				&user_data->coap_dst,
				pdu);

	if (pdu->hdr->type != COAP_MESSAGE_CON || tid == COAP_INVALID_TID)
		coap_delete_pdu(pdu);

	set_coap_timeout(&max_wait, wait_seconds);

	if (debug)
		printf("timeout is set to %d seconds\n", wait_seconds);
}

static int dispatch_data(char *buf, ssize_t bytes_read,
			 coap_context_t *ctx, coap_address_t *src)
{
	coap_hdr_t *pdu;
	coap_queue_t *node;

	pdu = (coap_hdr_t *)buf;

	if ((size_t)bytes_read < sizeof(coap_hdr_t)) {
		printf("coap_read: discarded invalid frame\n");
		goto error_early;
	}

	if (pdu->version != COAP_DEFAULT_VERSION) {
		printf("coap_read: unknown protocol version\n");
		goto error_early;
	}

	node = coap_new_node();
	if (!node)
		goto error_early;

	node->pdu = coap_pdu_init(0, 0, 0, bytes_read);

	if (!node->pdu)
		goto error;

	coap_ticks(&node->t);
	memcpy(&node->local_if, ctx->endpoint, sizeof(coap_endpoint_t));
	memcpy(&node->remote, src, sizeof(coap_address_t));

	if (!coap_pdu_parse((unsigned char *)buf, bytes_read, node->pdu)) {
		printf("discard malformed PDU");
		goto error;
	}

	coap_transaction_id(&node->remote, node->pdu, &node->id);

	if (coap_get_log_level() >= LOG_DEBUG) {
		unsigned char addr[INET6_ADDRSTRLEN+8];

		if (coap_print_addr(src, addr, INET6_ADDRSTRLEN+8))
			printf("** received %d bytes from %s:\n",
			       (int)bytes_read, addr);
		coap_show_pdu(node->pdu);
	}

	coap_dispatch(ctx, node);
	return 0;

error:
	coap_delete_node(node);
error_early:
	return -1;
}

static int read_from_peer(struct dtls_context_t *ctx,
			  session_t *session,
			  uint8 *read_data, size_t read_len)
{
	struct client_data *user_data =
			(struct client_data *)dtls_get_app_data(ctx);
	coap_address_t addr;

	printf("%s: read [%d] from peer %d bytes\n",
	       data[user_data->index].description, user_data->index, read_len);

	memcpy(&(addr.addr), &(session->addr), session->size);
	addr.size = session->size;

	/* Process the received CoAP packet */
	if (dispatch_data(read_data, read_len, user_data->coap_ctx,
			  &addr) < 0) {
		printf("Cannot parse received CoAP packet.\n");
		quit = true;
		user_data->fail = true;
		return 0;
	}

	if (debug)
		print_data(read_data, read_len);

	if (!data[user_data->index + 1].buf) {
		/* last entry, just bail out */
		quit = true;
		return 0;
	}

	if (user_data->index == renegotiate) {
		printf("Starting to renegotiate keys\n");
		dtls_renegotiate(ctx, session);
		return;
	}

	try_send(ctx);

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
	struct client_data *user_data =
			(struct client_data *)dtls_get_app_data(ctx);

	/* The Qemu uart driver can loose chars if sent too fast.
	 * So before sending more data, sleep a while.
	 */
	sleep_ms(200);

	printf("Sending to peer encrypted data %p len %d\n", data, len);

	return sendto(user_data->fd, data, len, 0,
		      &session->addr.sa, session->size);
}

static int dtls_handle_read(struct dtls_context_t *ctx)
{
	struct client_data *user_data;

	user_data = (struct client_data *)dtls_get_app_data(ctx);

	if (!user_data || !user_data->fd)
		return -1;

	memset(&session, 0, sizeof(session_t));
	session.size = sizeof(session.addr);
	user_data->len = recvfrom(user_data->fd, user_data->buf, MAX_READ_BUF,
				  0, &session.addr.sa, &session.size);
	if (user_data->len < 0) {
		perror("recvfrom");
		return -1;
	} else {
		dtls_dsrv_log_addr(DTLS_LOG_DEBUG, "peer", &session);
		dtls_debug_dump("bytes from peer", user_data->buf,
				user_data->len);
	}

	return dtls_handle_message(ctx, &session, user_data->buf,
				   user_data->len);
}

static int handle_event(struct dtls_context_t *ctx, session_t *session,
			dtls_alert_level_t level, unsigned short code)
{
	struct client_data *user_data;

	user_data = (struct client_data *)dtls_get_app_data(ctx);

	if (debug)
		printf("event: level %d code %d\n", level, code);

	if (level > 0) {
		/* alert code, quit */
		quit = true;
		user_data->fail = true;
	} else if (level == 0) {
		/* internal event */
		if (code == DTLS_EVENT_CONNECTED) {
			/* We can send data now */
			try_send(ctx);
		}
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

static int read_data_from_net(struct client_data *user_data)
{
	coap_packet_t *packet;
	coap_address_t addr;

	if (!user_data || !user_data->fd)
		return -1;

	return coap_read(user_data->coap_ctx);
}

void signal_handler(int signum)
{
	switch (signum) {
	case SIGINT:
	case SIGTERM:
		quit = true;
		break;
	}
}

static inline int check_token(coap_pdu_t *received)
{
	return received->hdr->token_length == the_token.length &&
		memcmp(received->hdr->token, the_token.s,
		       the_token.length) == 0;
}

static bool is_our_own(coap_context_t *coap_ctx,
		       const coap_address_t *remote,
		       coap_pdu_t *sent,
		       coap_pdu_t *received)
{
	if (data[test_context->index].add_token && !check_token(received)) {
		if (!sent &&
		    (received->hdr->type == COAP_MESSAGE_CON ||
		     received->hdr->type == COAP_MESSAGE_NON))
			coap_send_rst(coap_ctx, coap_ctx->endpoint, remote,
				      received);
		return false;
	}

	return true;
}

static void add_options(coap_pdu_t *pdu)
{
	coap_list_t *option;

	for (option = optlist; option; option = option->next ) {
		switch (COAP_OPTION_KEY(*(coap_option *)option->data)) {
		case COAP_OPTION_URI_HOST :
		case COAP_OPTION_URI_PORT :
		case COAP_OPTION_URI_PATH :
		case COAP_OPTION_CONTENT_FORMAT :
		case COAP_OPTION_URI_QUERY :
			coap_add_option(pdu,
				COAP_OPTION_KEY(*(coap_option *)option->data),
				COAP_OPTION_LENGTH(*(coap_option *)option->data),
				COAP_OPTION_DATA(*(coap_option *)option->data));
			break;
		default:
			break;
		}
	}
}

static int order_opts(void *a, void *b)
{
	coap_option *o1, *o2;

	if (!a || !b)
		return a < b ? -1 : 1;

	o1 = (coap_option *)(((coap_list_t *)a)->data);
	o2 = (coap_option *)(((coap_list_t *)b)->data);

	return (COAP_OPTION_KEY(*o1) < COAP_OPTION_KEY(*o2)) ? -1
		: (COAP_OPTION_KEY(*o1) != COAP_OPTION_KEY(*o2));
}

static coap_pdu_t *coap_new_request(coap_context_t *ctx,
				    method_t m,
				    coap_list_t **options,
				    const unsigned char *req,
				    size_t length,
				    int msgtype)
{
	coap_pdu_t *pdu;
	coap_list_t *opt;

	if (!(pdu = coap_new_pdu()))
		return NULL;

	pdu->hdr->type = msgtype;
	pdu->hdr->id = coap_new_message_id(ctx);
	pdu->hdr->code = m;

	if (data[test_context->index].add_token) {
		pdu->hdr->token_length = the_token.length;
		if (!coap_add_token(pdu, the_token.length, the_token.s)) {
			printf("cannot add token to request\n");
			test_context->fail = true;
			quit = true;
			return NULL;
		}
	}

	coap_show_pdu(pdu);

	if (options) {
		LL_SORT((*options), order_opts);

		LL_FOREACH((*options), opt) {
			coap_option *o = (coap_option *)(opt->data);
			coap_add_option(pdu,
					COAP_OPTION_KEY(*o),
					COAP_OPTION_LENGTH(*o),
					COAP_OPTION_DATA(*o));
		}
	}

	if (length) {
		if ((flags & FLAGS_BLOCK) == 0)
			coap_add_data(pdu, length, req);
		else
			coap_add_block(pdu, length, req,
				       block.num, block.szx);
	}

	return pdu;
}

static coap_pdu_t *read_blocks(coap_context_t *coap_ctx,
			       const coap_endpoint_t *endpoint,
			       const coap_address_t *remote,
			       coap_opt_iterator_t *opt_iter,
			       coap_pdu_t *received,
			       coap_opt_t *block_opt)
{
	unsigned short blktype = opt_iter->type;
	size_t len;
	unsigned char *databuf;
	coap_pdu_t *pdu = NULL;
	unsigned char buf[4];
	coap_tid_t tid;

	if (coap_get_data(received, &len, &databuf))
		printf("Received: databuf %p len %d\n", databuf, len);

	if (COAP_OPT_BLOCK_MORE(block_opt)) {
		printf("found the M bit, block size is %u, block nr. %u\n",
		       COAP_OPT_BLOCK_SZX(block_opt),
		       coap_opt_block_num(block_opt));

		pdu = coap_new_request(coap_ctx, method, NULL, NULL, 0,
				       COAP_MESSAGE_CON);
		if (pdu) {
			add_options(pdu);

			printf("query block %d\n",
			       (coap_opt_block_num(block_opt) + 1));

			coap_add_option(pdu, blktype,
				coap_encode_var_bytes(buf,
				   ((coap_opt_block_num(block_opt) + 1) << 4) |
			             COAP_OPT_BLOCK_SZX(block_opt)), buf);

			if (received->hdr->type == COAP_MESSAGE_CON)
				tid = coap_send_confirmed(coap_ctx,
							  endpoint,
							  remote,
							  pdu);
			else
				tid = coap_send(coap_ctx,
						endpoint,
						remote,
						pdu);

			if (tid == COAP_INVALID_TID) {
				printf("error sending new request\n");
				coap_delete_pdu(pdu);
			} else {
				set_coap_timeout(&max_wait, wait_seconds);
				if (received->hdr->type != COAP_MESSAGE_CON) {
					coap_delete_pdu(pdu);
					return NULL;
				}
			}
		}
	}

	return pdu;
}

static  coap_opt_t *get_block(coap_pdu_t *pdu, coap_opt_iterator_t *opt_iter)
{
	coap_opt_filter_t f;

	memset(f, 0, sizeof(coap_opt_filter_t));
	coap_option_setb(f, COAP_OPTION_BLOCK1);
	coap_option_setb(f, COAP_OPTION_BLOCK2);

	coap_option_iterator_init(pdu, opt_iter, f);
	return coap_option_next(opt_iter);
}

static bool check_mid(const unsigned char *databuf, int len,
		      int test_case)
{
	unsigned char buf[2048];

	snprintf(buf, sizeof(buf), "%sMID: %d", data[test_case].data,
		 data[test_case].expected_mid);
	if (!memcmp(databuf, buf, strlen(buf)))
		return true;
	else {
		printf("FAIL:\n  received %d bytes\n>>>>\n%.*s\n<<<<\n", len,
		       len, databuf);
		printf(" expected %d bytes\n>>>>\n%s\n<<<<\n", strlen(buf), buf);
		return false;
	}
}

static void receive_data(coap_pdu_t *received)
{
	size_t len;
	unsigned char *databuf;

	if (coap_get_data(received, &len, &databuf)) {
		if (!test_context)
			return;

		if (data[test_context->index].check_mid &&
		    !check_mid(databuf, len, test_context->index)) {
			printf("Test %d failed\n",
			       test_context->index);
			test_context->fail = true;
			quit = true;
		} else {
			if (!data[test_context->index + 1].buf) {
				/* last entry, just bail out */
				quit = true;
			} else
				test_context->index++;
		}
	} else {
		if (!test_context)
			return;

		if (data[test_context->index].check_mid) {
			printf("Packet error\n");
			quit = true;
			test_context->fail = true;
		} else {
			if (!data[test_context->index + 1].buf) {
				/* last entry, just bail out */
				quit = true;
			} else
				test_context->index++;
		}
	}
}

static void coap_message_handler(struct coap_context_t *coap_ctx,
				 const coap_endpoint_t *endpoint,
				 const coap_address_t *remote,
				 coap_pdu_t *sent,
				 coap_pdu_t *received,
				 const coap_tid_t id)
{
	coap_pdu_t *pdu = NULL;
	coap_opt_t *block_opt;
	coap_opt_iterator_t opt_iter;
	size_t len;
	unsigned char *databuf;

	if (coap_get_log_level() >= LOG_DEBUG) {
		printf("process incoming %d.%02d response:\n",
		      (received->hdr->code >> 5), received->hdr->code & 0x1F);
		coap_show_pdu(received);
	}

	if (!is_our_own(coap_ctx, remote, sent, received))
		return;

	switch (received->hdr->type) {
	case COAP_MESSAGE_CON:
		coap_send_ack(coap_ctx, endpoint, remote, received);
		break;
	case COAP_MESSAGE_RST:
		printf("CoAP RST\n");
		return;
	default:
		break;
	}

	if (COAP_RESPONSE_CLASS(received->hdr->code) == 2) {

		if (sent && coap_check_option(received,
					      COAP_OPTION_SUBSCRIPTION,
					      &opt_iter)) {
			printf("observation timeout set to %d\n", obs_seconds);
			set_coap_timeout(&obs_wait, obs_seconds);
		}

		block_opt = get_block(received, &opt_iter);
		if (block_opt)
			pdu = read_blocks(coap_ctx,
					  endpoint,
					  remote,
					  &opt_iter,
					  received,
					  block_opt);
		else {
			block_opt = coap_check_option(received,
						      COAP_OPTION_BLOCK1,
						      &opt_iter);
			if (block_opt) { /* handle Block1 */
				unsigned char buf[4];
				coap_tid_t tid;

				block.szx = COAP_OPT_BLOCK_SZX(block_opt);
				block.num = coap_opt_block_num(block_opt);

				debug("found Block1, block size is %u, "
				      "block nr. %u\n",
				      block.szx, block.num);

				if (payload.length <= (block.num+1) *
				    (1 << (block.szx + 4))) {
					debug("upload ready\n");
					ready = 1;
					return;
				}

				/* create pdu with request for next block */
				pdu = coap_new_request(coap_ctx, method, NULL,
						       NULL, 0, COAP_MESSAGE_CON);
				if (pdu) {
					/* add URI components from optlist */
					add_options(pdu);

					block.num++;
					block.m = ((block.num+1) *
						   (1 << (block.szx + 4)) <
						   payload.length);

					debug("send block %d\n", block.num);
					coap_add_option(pdu,
							COAP_OPTION_BLOCK1,
							coap_encode_var_bytes(buf,
							      (block.num << 4) |
							      (block.m << 3) |
							      block.szx), buf);

					coap_add_block(pdu,
						       payload.length,
						       payload.s,
						       block.num,
						       block.szx);
					coap_show_pdu(pdu);
					if (pdu->hdr->type == COAP_MESSAGE_CON)
						tid = coap_send_confirmed(coap_ctx,
						  endpoint, remote, pdu);
					else
						tid = coap_send(coap_ctx,
						  endpoint, remote, pdu);

					if (tid == COAP_INVALID_TID) {
						debug("message_handler: error "
						      "sending new request");
						coap_delete_pdu(pdu);
					} else {
						set_coap_timeout(&max_wait, wait_seconds);
						if (pdu->hdr->type != COAP_MESSAGE_CON)
							coap_delete_pdu(pdu);
					}

					return;
				}
			} else {
				/* There is no block option set, just read the data and we are done. */
				receive_data(received);
			}
		}
	} else {
		if (COAP_RESPONSE_CLASS(received->hdr->code) >= 4) {
			fprintf(stderr, "ERROR %d.%02d",
				(received->hdr->code >> 5),
				received->hdr->code & 0x1F);
			if (coap_get_data(received, &len, &databuf)) {
				fprintf(stderr, " ");
				while(len--)
					fprintf(stderr, "%c", *databuf++);
			}
			fprintf(stderr, "\n");

			if (test_context)
				test_context->fail = true;
			quit = true;
		}
	}

	if (pdu && coap_send(coap_ctx, endpoint,
			     remote, pdu) == COAP_INVALID_TID)
		printf("error sending response\n");

	coap_delete_pdu(pdu);

	ready = coap_check_option(received, COAP_OPTION_SUBSCRIPTION,
				  &opt_iter) == NULL;
}

static void set_blocksize(void)
{
	static unsigned char buf[4];
	unsigned short opt;
	unsigned int opt_length;

	if (method != COAP_REQUEST_DELETE) {
		opt = method == COAP_REQUEST_GET ? COAP_OPTION_BLOCK2 :
							COAP_OPTION_BLOCK1;

		block.m = (opt == COAP_OPTION_BLOCK1) &&
			((1u << (block.szx + 4)) < payload.length);

		opt_length = coap_encode_var_bytes(buf,
				(block.num << 4 | block.m << 3 | block.szx));

		coap_insert(&optlist, new_option_node(opt, opt_length, buf));
	}
}

static ssize_t my_coap_network_send(struct coap_context_t *ctx,
				    const coap_endpoint_t *endpoint,
				    const coap_address_t *dst,
				    unsigned char *data,
				    size_t datalen)
{
	ssize_t bytes_written = dtls_write(dtls_context, &session,
					   data, datalen);
	if (bytes_written <= 0) {
		coap_log(LOG_CRIT, "coap_send: sendto\n");
	}

	return bytes_written;
}

static ssize_t my_coap_network_read(coap_endpoint_t *endpoint,
				    coap_packet_t **packet)
{
	printf("*** We should not get here if we are using DTLS ***\n");

	return -EINVAL;
}

static coap_context_t *get_coap_context(struct sockaddr *ipaddr, int ipaddrlen)
{
	coap_address_t addr;
	coap_context_t *ctx;

	coap_address_init(&addr);
	addr.size = ipaddrlen;
	memcpy(&addr.addr, ipaddr, ipaddrlen);

	ctx = coap_new_context(&addr);
	if (!ctx)
		return NULL;

	if (!no_dtls) {
		ctx->network_send = my_coap_network_send;
		ctx->network_read = my_coap_network_read;
	}

	return ctx;
}

static inline void set_token(char *arg)
{
	strncpy((char *)the_token.s, arg, min(sizeof(_token_data),
					      strlen(arg)));
	the_token.length = strlen(arg);
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
	int c, ret, fd, timeout = 0;
	struct sockaddr_in6 addr6_send = { 0 }, addr6_recv = { 0 };
	struct sockaddr_in addr4_send = { 0 }, addr4_recv = { 0 };
	struct sockaddr *addr_send, *addr_recv;
	int addr_len;
	const struct in6_addr any = IN6ADDR_ANY_INIT;
	const char *interface = NULL;
	fd_set rfds;
	struct timeval tv = {};
	int ifindex = -1, on, port;
	void *address = NULL;
	session_t dst;
	struct client_data user_data;

	coap_tid_t tid = COAP_INVALID_TID;
	coap_context_t *coap_ctx;
	coap_tick_t now;
	coap_queue_t *nextpdu;
	coap_pdu_t  *pdu;

#ifdef DTLS_PSK
	psk_id_length = strlen(PSK_DEFAULT_IDENTITY);
	psk_key_length = strlen(PSK_DEFAULT_KEY);
	memcpy(psk_id, PSK_DEFAULT_IDENTITY, psk_id_length);
	memcpy(psk_key, PSK_DEFAULT_KEY, psk_key_length);
#endif /* DTLS_PSK */

	opterr = 0;

	while ((c = getopt(argc, argv, "i:Drb:n")) != -1) {
		switch (c) {
		case 'i':
			interface = optarg;
			break;
		case 'n':
			no_dtls = true;
			break;
		case 'r':
			/* Do a renegotiate once during the test run. */
			srandom(time(0));
			renegotiate = random() %
				(sizeof(data) / sizeof(struct data) - 1);
			printf("Renegotating after %d messages.\n", renegotiate);
			break;
		case 'D':
			debug = true;
			break;
		case 'h':
			goto usage;
		}
	}

	if (optind < argc)
		target = argv[optind];

	if (!target) {
	usage:
		printf("usage: %s [-i tun0] [-D] [-r] [-n]"
		       "<IPv{6|4} address of the dtls-server>\n",
		       argv[0]);
		printf("\t-i Use this network interface.\n");
		printf("\t-n Do not use DTLS\n");
		printf("\t-r Renegoating keys once during the test run.\n");
		printf("\t-D Activate debugging.\n");
		exit(-EINVAL);
	}

	if (inet_pton(AF_INET6, target, &addr6_send.sin6_addr) != 1) {
		if (inet_pton(AF_INET, target, &addr4_send.sin_addr) != 1) {
			printf("Invalid address family\n");
			exit(-EINVAL);
		} else {
			addr_send = (struct sockaddr *)&addr4_send;
			addr_recv = (struct sockaddr *)&addr4_recv;
			if (no_dtls)
				addr4_send.sin_port = port = htons(SERVER_PORT);
			else
				addr4_send.sin_port = port = htons(SERVER_SECURE_PORT);
			addr4_recv.sin_family = AF_INET;
			addr4_recv.sin_addr.s_addr = INADDR_ANY;
			addr4_recv.sin_port = htons(CLIENT_PORT);
			family = AF_INET;
			addr_len = sizeof(addr4_send);
			address = &addr4_recv.sin_addr;
		}
	} else {
		addr_send = (struct sockaddr *)&addr6_send;
		addr_recv = (struct sockaddr *)&addr6_recv;
		if (no_dtls)
			addr6_send.sin6_port = port = htons(SERVER_PORT);
		else
			addr6_send.sin6_port = port = htons(SERVER_SECURE_PORT);
		addr6_recv.sin6_family = AF_INET6;
		addr6_recv.sin6_addr = any;
		addr6_recv.sin6_port = htons(CLIENT_PORT);
		family = AF_INET6;
		addr_len = sizeof(addr6_send);
		address = &addr6_recv.sin6_addr;
	}

	addr_send->sa_family = family;
	addr_recv->sa_family = family;

	fd = socket(family, SOCK_DGRAM, IPPROTO_UDP);
	if (fd < 0) {
		perror("socket");
		exit(-errno);
	}

	if (interface) {
		struct ifreq ifr;
		char addr_buf[INET6_ADDRSTRLEN];

		memset(&ifr, 0, sizeof(ifr));
		snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), interface);

		if (setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE,
			       (void *)&ifr, sizeof(ifr)) < 0) {
			perror("SO_BINDTODEVICE");
			exit(-errno);
		}

		ifindex = get_ifindex(interface);
		if (ifindex < 0) {
			printf("Invalid interface %s\n", interface);
			exit(-EINVAL);
		}

		get_address(ifindex, family, address);

		printf("Binding to %s\n", inet_ntop(family, address,
					    addr_buf, sizeof(addr_buf)));
	}

	ret = bind(fd, addr_recv, addr_len);
	if (ret < 0) {
		perror("bind");
		exit(-errno);
	}

	on = 1;
#ifdef IPV6_RECVPKTINFO
	if (setsockopt(fd, IPPROTO_IPV6, IPV6_RECVPKTINFO, &on,
		       sizeof(on) ) < 0) {
#else /* IPV6_RECVPKTINFO */
	if (setsockopt(fd, IPPROTO_IPV6, IPV6_PKTINFO, &on,
		       sizeof(on) ) < 0) {
#endif /* IPV6_RECVPKTINFO */
		printf("setsockopt IPV6_PKTINFO: %s\n", strerror(errno));
	}
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) ) < 0) {
		printf("setsockopt SO_REUSEADDR: %s\n", strerror(errno));
	}

	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	user_data.fd = fd;
	user_data.index = 0;
	user_data.ifindex = ifindex;

	coap_ctx = get_coap_context(addr_recv, addr_len);
	user_data.coap_ctx = coap_ctx;
	coap_ctx->endpoint->handle.fd = fd;

	coap_address_init(&user_data.coap_dst);
	memcpy(&user_data.coap_dst.addr, addr_send, addr_len);
	user_data.coap_dst.size = addr_len;

	if (debug)
		coap_set_log_level(LOG_DEBUG);

	coap_register_option(coap_ctx, COAP_OPTION_BLOCK2);
	coap_register_response_handler(coap_ctx, coap_message_handler);
	set_coap_timeout(&max_wait, wait_seconds);
	printf("CoAP timeout is set to %d seconds\n", wait_seconds);

	coap_insert(&optlist,
                    new_option_node(COAP_OPTION_URI_HOST,
				    strlen(target),
				    (char *)target));

	if (port) {
		unsigned char p[2];
		p[0] = ntohs(port) >> 8;
		p[1] = ntohs(port) & 0xff;
		coap_insert(&optlist,
			    new_option_node(COAP_OPTION_URI_PORT,
					    2, p));
	}

	printf("Connecting to port %d\n", ntohs(port));

	if (!no_dtls) {
		dtls_init();

		memset(&dst, 0, sizeof(dst));
		memcpy(&dst.addr.sa, addr_send, addr_len);
		dst.size = addr_len;

		dtls_context = dtls_new_context(&user_data);
		if (!dtls_context) {
			dtls_emerg("cannot create context\n");
			exit(-EINVAL);
		}

		dtls_set_handler(dtls_context, &cb);

		if (debug)
			dtls_set_log_level(DTLS_LOG_DEBUG);

		dtls_connect(dtls_context, &dst);
	}

	set_token("07162534");

	do {
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		tv.tv_sec = MAX_TIMEOUT;
		tv.tv_usec = 0;

		if (no_dtls)
			send_packets(&user_data);

		if (quit || user_data.fail)
			break;

		nextpdu = coap_peek_next(coap_ctx);

		coap_ticks(&now);
		while (nextpdu && nextpdu->t <=
		       (now - coap_ctx->sendqueue_basetime)) {
			coap_retransmit(coap_ctx, coap_pop_next(coap_ctx));
			nextpdu = coap_peek_next(coap_ctx);
		}

		if (nextpdu && nextpdu->t < min(obs_wait ? obs_wait : max_wait,
						max_wait) - now) {
			tv.tv_usec = ((nextpdu->t) % COAP_TICKS_PER_SECOND) *
				1000000 / COAP_TICKS_PER_SECOND;
			tv.tv_sec = (nextpdu->t) / COAP_TICKS_PER_SECOND;
		} else {
			if (obs_wait && obs_wait < max_wait) {
				tv.tv_usec = ((obs_wait - now) %
					    COAP_TICKS_PER_SECOND) * 1000000 /
							COAP_TICKS_PER_SECOND;
				tv.tv_sec = (obs_wait - now) /
							COAP_TICKS_PER_SECOND;
			} else {
				tv.tv_usec = ((max_wait - now) %
					    COAP_TICKS_PER_SECOND) * 1000000 /
							COAP_TICKS_PER_SECOND;
				tv.tv_sec = (max_wait - now) /
							COAP_TICKS_PER_SECOND;
			}
		}

		ret = select(fd + 1, &rfds, NULL, NULL, &tv);
		if (ret < 0) {
			perror("select");
			break;
		} else if (ret == 0) {
			if (quit)
				break;

			if (user_data.index >
			    (sizeof(data) / sizeof(struct data)) - 1)
				break;

			if (!data[user_data.index].expecting_reply) {
				printf("Did not expect a reply, send next entry.\n");
				user_data.index++;
				if (!data[user_data.index].buf) {
					user_data.fail = true;
					break;
				}
				continue;
			}

			fprintf(stderr,	"Timeout [%d] while waiting len %d\n",
				user_data.index, data[user_data.index].len);
			ret = user_data.index + 1;
			user_data.fail = true;
			break;
		} else if (!FD_ISSET(fd, &rfds)) {
			fprintf(stderr, "Invalid fd in read, quitting.\n");
			ret = user_data.index + 1;
			user_data.fail = true;
			break;
		}

		if (!no_dtls) {
			if (dtls_handle_read(dtls_context) < 0) {
				fprintf(stderr, "Peer connection failed.\n");
				ret = user_data.index + 1;
				user_data.fail = true;
				break;
			}
		} else {
			user_data.fd = fd;
			if (read_data_from_net(&user_data) < 0) {
				fprintf(stderr, "Packet read error.\n");
				ret = user_data.index + 1;
				user_data.fail = true;
				break;
			}
		}

	} while(!quit);

	printf("\n");

	if (!no_dtls)
		dtls_close(dtls_context, &dst);

	dtls_free_context(dtls_context);

	close(fd);

	if (!user_data.fail) {
		if (debug)
			printf("Weird: No failures but error code (%d) "
			       "indicates error\n", ret);
		ret = 0;
	}

	exit(ret);
}
