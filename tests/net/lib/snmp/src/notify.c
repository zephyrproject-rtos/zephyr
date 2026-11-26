/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/net/snmp.h>
#include <zephyr/net/socket.h>
#include <zephyr/ztest.h>

#include "helpers.h"
#include "test_objects.h"

/* Two receivers on loopback, one per protocol version */
#define RECEIVER_V2C_PORT  16162
#define RECEIVER_V1_PORT   16163
#define RECEIVE_TIMEOUT_MS 500

#define OID_LEN_OF(arr) ARRAY_SIZE(arr)

static const uint32_t snmp_traps[] = {1, 3, 6, 1, 6, 3, 1, 1, 5};
static const uint32_t cold_start[] = {SNMP_TRAP_COLD_START};
static const uint32_t auth_failure[] = {SNMP_TRAP_AUTHENTICATION_FAILURE};
static const uint32_t sys_uptime_inst[] = {1, 3, 6, 1, 2, 1, 1, 3, 0};
static const uint32_t snmp_trap_oid_inst[] = {1, 3, 6, 1, 6, 3, 1, 1, 4, 1, 0};

static int receiver_v2c = -1;
static int receiver_v1 = -1;

/* Decoded fields of a received trap of either version */
struct trap {
	int32_t version;
	char community[32];
	uint8_t pdu_tag;
	/* SNMPv2-Trap */
	uint32_t uptime;
	uint32_t oid[SNMP_OID_MAX_LEN];
	size_t oid_len;
	/* Trap-PDU */
	uint32_t enterprise[SNMP_OID_MAX_LEN];
	size_t enterprise_len;
	uint8_t agent_addr[4];
	int32_t generic;
	int32_t specific;
	size_t binding_count;
	/* First binding after the standard ones of an SNMPv2-Trap */
	uint32_t vb_oid[SNMP_OID_MAX_LEN];
	size_t vb_oid_len;
	uint8_t vb_tag;
	int32_t vb_i32;
};

static int open_receiver(uint16_t port)
{
	struct net_sockaddr_in addr = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(port),
	};
	int sock;

	zassert_equal(zsock_inet_pton(NET_AF_INET, "127.0.0.1", &addr.sin_addr), 1);
	sock = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(sock >= 0, "Cannot create receiver (%d)", -errno);
	zassert_ok(zsock_bind(sock, (struct net_sockaddr *)&addr, sizeof(addr)),
		   "Cannot bind receiver (%d)", -errno);

	return sock;
}

/* Register a receiver; returns what snmp_agent_add_notification_target() does. */
static int add_target_to(const struct net_sockaddr *addr, net_socklen_t addrlen,
			 enum snmp_version version, const char *community)
{
	const struct snmp_notification_target target = {
		.addr = addr,
		.addrlen = addrlen,
		.version = version,
		.community = community,
	};

	return snmp_agent_add_notification_target(&target);
}

static void add_target(uint16_t port, enum snmp_version version, const char *community)
{
	struct net_sockaddr_in addr = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(port),
	};

	zassert_equal(zsock_inet_pton(NET_AF_INET, "127.0.0.1", &addr.sin_addr), 1);
	zassert_ok(add_target_to((struct net_sockaddr *)&addr, sizeof(addr), version, community));
}

/* Wait for one datagram; returns its length or -1 on timeout. */
static int receive(int sock, uint8_t *buf, size_t size)
{
	struct zsock_pollfd fds[] = {{.fd = sock, .events = ZSOCK_POLLIN}};
	int ret;

	ret = zsock_poll(fds, 1, RECEIVE_TIMEOUT_MS);
	if (ret <= 0) {
		return -1;
	}

	ret = zsock_recv(sock, buf, size, 0);
	zassert_true(ret >= 0, "Receiving failed (%d)", -errno);

	return ret;
}

/* Decode the name and, for an INTEGER, the value of the first application
 * binding; count the rest.
 */
static void decode_bindings(struct snmp_ber_reader *list, struct trap *t)
{
	if (!snmp_ber_reader_at_end(list)) {
		struct snmp_ber_reader vb;

		zassert_ok(snmp_ber_read_sequence(list, SNMP_BER_TAG_SEQUENCE, &vb));
		zassert_ok(
			snmp_ber_read_oid(&vb, t->vb_oid, ARRAY_SIZE(t->vb_oid), &t->vb_oid_len));
		zassert_ok(snmp_ber_peek_tag(&vb, &t->vb_tag));
		if (t->vb_tag == SNMP_BER_TAG_INTEGER) {
			zassert_ok(snmp_ber_read_int32(&vb, &t->vb_i32));
		} else {
			zassert_ok(snmp_ber_skip(&vb));
		}
		zassert_true(snmp_ber_reader_at_end(&vb), "Trailing bytes in binding");
		t->binding_count++;
	}

	while (!snmp_ber_reader_at_end(list)) {
		zassert_ok(snmp_ber_skip(list));
		t->binding_count++;
	}
}

static void decode_trap(const uint8_t *data, size_t len, struct trap *t)
{
	struct snmp_ber_reader outer, msg, pdu, list, vb;
	const uint8_t *octets;
	size_t octets_len, pdu_len;

	memset(t, 0, sizeof(*t));

	snmp_ber_reader_init(&outer, data, len);
	zassert_ok(snmp_ber_read_sequence(&outer, SNMP_BER_TAG_SEQUENCE, &msg));
	zassert_true(snmp_ber_reader_at_end(&outer));
	zassert_ok(snmp_ber_read_int32(&msg, &t->version));
	zassert_ok(snmp_ber_read_octets(&msg, SNMP_BER_TAG_OCTET_STRING, &octets, &octets_len));
	zassert_true(octets_len < sizeof(t->community));
	memcpy(t->community, octets, octets_len);
	zassert_ok(snmp_ber_read_header(&msg, &t->pdu_tag, &pdu_len));
	snmp_ber_reader_init(&pdu, &msg.data[msg.pos], pdu_len);

	if (t->pdu_tag == SNMP_BER_TAG_TRAP_V2) {
		int32_t i32;
		uint32_t name[SNMP_OID_MAX_LEN];
		size_t name_len;

		zassert_ok(snmp_ber_read_int32(&pdu, &i32), "request-id");
		zassert_ok(snmp_ber_read_int32(&pdu, &i32));
		zassert_equal(i32, 0, "error-status");
		zassert_ok(snmp_ber_read_int32(&pdu, &i32));
		zassert_equal(i32, 0, "error-index");
		zassert_ok(snmp_ber_read_sequence(&pdu, SNMP_BER_TAG_SEQUENCE, &list));

		zassert_ok(snmp_ber_read_sequence(&list, SNMP_BER_TAG_SEQUENCE, &vb));
		zassert_ok(snmp_ber_read_oid(&vb, name, ARRAY_SIZE(name), &name_len));
		zassert_equal(
			snmp_oid_cmp(name, name_len, sys_uptime_inst, ARRAY_SIZE(sys_uptime_inst)),
			0, "First binding is not sysUpTime.0");
		zassert_ok(snmp_ber_read_uint32(&vb, SNMP_BER_TAG_TIMETICKS, &t->uptime));

		zassert_ok(snmp_ber_read_sequence(&list, SNMP_BER_TAG_SEQUENCE, &vb));
		zassert_ok(snmp_ber_read_oid(&vb, name, ARRAY_SIZE(name), &name_len));
		zassert_equal(snmp_oid_cmp(name, name_len, snmp_trap_oid_inst,
					   ARRAY_SIZE(snmp_trap_oid_inst)),
			      0, "Second binding is not snmpTrapOID.0");
		zassert_ok(snmp_ber_read_oid(&vb, t->oid, ARRAY_SIZE(t->oid), &t->oid_len));
		t->binding_count = 2;
		decode_bindings(&list, t);
	} else if (t->pdu_tag == SNMP_BER_TAG_TRAP_V1) {
		zassert_ok(snmp_ber_read_oid(&pdu, t->enterprise, ARRAY_SIZE(t->enterprise),
					     &t->enterprise_len));
		zassert_ok(
			snmp_ber_read_octets(&pdu, SNMP_BER_TAG_IPADDRESS, &octets, &octets_len));
		zassert_equal(octets_len, 4);
		memcpy(t->agent_addr, octets, 4);
		zassert_ok(snmp_ber_read_int32(&pdu, &t->generic));
		zassert_ok(snmp_ber_read_int32(&pdu, &t->specific));
		zassert_ok(snmp_ber_read_uint32(&pdu, SNMP_BER_TAG_TIMETICKS, &t->uptime));
		zassert_ok(snmp_ber_read_sequence(&pdu, SNMP_BER_TAG_SEQUENCE, &list));
		decode_bindings(&list, t);
	} else {
		zassert_unreachable("PDU tag 0x%02x", t->pdu_tag);
	}

	zassert_true(snmp_ber_reader_at_end(&pdu), "Trailing bytes in PDU");
}

static void expect_trap(int sock, struct trap *t)
{
	uint8_t buf[256];
	int len = receive(sock, buf, sizeof(buf));

	zassert_true(len > 0, "No notification received");
	decode_trap(buf, (size_t)len, t);
}

static void expect_silence(int sock)
{
	uint8_t buf[256];

	zassert_equal(receive(sock, buf, sizeof(buf)), -1, "Unexpected notification");
}

/* bindings counts the application's, after sysUpTime.0 and snmpTrapOID.0 */
static void expect_v2c(const struct trap *t, const uint32_t *oid, size_t len, size_t bindings)
{
	zassert_equal(t->version, SNMP_VERSION_V2C);
	zassert_equal(t->pdu_tag, SNMP_BER_TAG_TRAP_V2);
	zassert_str_equal(t->community, "trapv2");
	zassert_equal(snmp_oid_cmp(t->oid, t->oid_len, oid, len), 0, "Wrong snmpTrapOID");
	zassert_equal(t->binding_count, 2 + bindings);
	zassert_true(t->uptime < 100, "sysUpTime %u", t->uptime);
}

static void expect_v1(const struct trap *t, const uint32_t *enterprise, size_t enterprise_len,
		      int32_t generic, int32_t specific, size_t bindings)
{
	static const uint8_t loopback[4] = {127, 0, 0, 1};

	zassert_equal(t->version, SNMP_VERSION_V1);
	zassert_equal(t->pdu_tag, SNMP_BER_TAG_TRAP_V1);
	zassert_str_equal(t->community, "trapv1");
	zassert_equal(snmp_oid_cmp(t->enterprise, t->enterprise_len, enterprise, enterprise_len), 0,
		      "Wrong enterprise");
	zassert_mem_equal(t->agent_addr, loopback, 4, "agent-addr");
	zassert_equal(t->generic, generic, "generic-trap %d", t->generic);
	zassert_equal(t->specific, specific, "specific-trap %d", t->specific);
	zassert_equal(t->binding_count, bindings);
	zassert_true(t->uptime < 100, "time-stamp %u", t->uptime);
}

static void expect_int32_binding(const struct trap *t, const uint32_t *oid, size_t len,
				 int32_t value)
{
	zassert_equal(snmp_oid_cmp(t->vb_oid, t->vb_oid_len, oid, len), 0, "Wrong binding name");
	zassert_equal(t->vb_tag, SNMP_BER_TAG_INTEGER, "Binding tag 0x%02x", t->vb_tag);
	zassert_equal(t->vb_i32, value, "Binding value %d", t->vb_i32);
}

/* Send a request to the running agent through the network. */
static void send_request(int32_t version, const char *community)
{
	const struct req_vb vbs[] = {VB_NULL(TEST_OID_A, 0)};
	struct net_sockaddr_in agent = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(CONFIG_SNMP_PORT),
	};
	size_t start, len;
	int sock;

	zassert_equal(zsock_inet_pton(NET_AF_INET, "127.0.0.1", &agent.sin_addr), 1);
	len = build_request(version, community, SNMP_BER_TAG_GET_REQUEST, 0, 0, vbs,
			    ARRAY_SIZE(vbs), &start);
	sock = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(sock >= 0);
	zassert_equal(zsock_sendto(sock, &req_buf[start], len, 0, (struct net_sockaddr *)&agent,
				   sizeof(agent)),
		      (int)len);
	(void)zsock_close(sock);
}

static void *suite_setup(void)
{
	receiver_v2c = open_receiver(RECEIVER_V2C_PORT);
	receiver_v1 = open_receiver(RECEIVER_V1_PORT);

	return NULL;
}

static void suite_teardown(void *fixture)
{
	ARG_UNUSED(fixture);

	snmp_agent_clear_notification_targets();
	(void)snmp_agent_stop();
	(void)zsock_close(receiver_v2c);
	(void)zsock_close(receiver_v1);
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_ok(snmp_agent_set_community(SNMP_COMMUNITY_READ, "public"));
	snmp_mib2_snmp_set_authen_traps(true);
	snmp_agent_clear_notification_targets();
}

ZTEST(snmp_notify, test_targets)
{
	struct net_sockaddr_in addr = {.sin_family = NET_AF_INET};
	struct net_sockaddr_in6 addr6 = {.sin6_family = NET_AF_INET6};
	struct net_sockaddr bad = {.sa_family = NET_AF_UNSPEC};
	char too_long[CONFIG_SNMP_COMMUNITY_MAX_LEN + 2];

	memset(too_long, 'c', sizeof(too_long) - 1);
	too_long[sizeof(too_long) - 1] = '\0';

	zassert_equal(snmp_agent_add_notification_target(NULL), -EINVAL);
	zassert_equal(add_target_to(NULL, sizeof(addr), SNMP_VERSION_V2C, "x"), -EINVAL);
	zassert_equal(
		add_target_to((struct net_sockaddr *)&addr, sizeof(addr), SNMP_VERSION_V2C, NULL),
		-EINVAL);
	zassert_equal(add_target_to(&bad, sizeof(bad), SNMP_VERSION_V2C, "x"), -EINVAL);
	zassert_equal(add_target_to((struct net_sockaddr *)&addr, sizeof(addr),
				    (enum snmp_version)3, "x"),
		      -EINVAL);
	zassert_equal(add_target_to((struct net_sockaddr *)&addr, sizeof(addr), SNMP_VERSION_V2C,
				    too_long),
		      -EMSGSIZE);

	for (int i = 0; i < CONFIG_SNMP_NOTIFICATION_TARGETS; i++) {
		zassert_ok(add_target_to((struct net_sockaddr *)&addr6, sizeof(addr6),
					 SNMP_VERSION_V1, "x"));
	}
	zassert_equal(
		add_target_to((struct net_sockaddr *)&addr, sizeof(addr), SNMP_VERSION_V2C, "x"),
		-ENOMEM);

	snmp_agent_clear_notification_targets();
	zassert_ok(
		add_target_to((struct net_sockaddr *)&addr, sizeof(addr), SNMP_VERSION_V2C, "x"));

	/* Nothing can be sent while the agent is stopped */
	zassert_equal(snmp_agent_notify(cold_start, ARRAY_SIZE(cold_start), NULL, 0), -ENOTCONN);
	zassert_equal(snmp_agent_notify(NULL, 2, NULL, 0), -EINVAL);
	zassert_equal(snmp_agent_notify(cold_start, 1, NULL, 0), -EINVAL);
}

/* What cannot be encoded is refused before any receiver is considered. */
ZTEST(snmp_notify, test_invalid_notifications)
{
	static const uint32_t bad_first_arc[] = {3, 1, 5};
	static const uint32_t no_enterprise[] = {1, 3};
	const struct snmp_varbind no_name = {
		.oid = NULL, .oid_len = 0, .value = {.type = SNMP_TYPE_INTEGER, .i32 = 1}};
	const struct snmp_varbind bad_name = {.oid = bad_first_arc,
					      .oid_len = ARRAY_SIZE(bad_first_arc),
					      .value = {.type = SNMP_TYPE_INTEGER, .i32 = 1}};
	const struct snmp_varbind bad_value = {.oid = cold_start,
					       .oid_len = ARRAY_SIZE(cold_start),
					       .value = {.type = (enum snmp_type)99}};

	add_target(RECEIVER_V2C_PORT, SNMP_VERSION_V2C, "trapv2");
	zassert_ok(snmp_agent_start());
	expect_trap(receiver_v2c, &(struct trap){0});

	zassert_equal(snmp_agent_notify(bad_first_arc, ARRAY_SIZE(bad_first_arc), NULL, 0),
		      -EINVAL);
	zassert_equal(snmp_agent_notify(no_enterprise, ARRAY_SIZE(no_enterprise), NULL, 0),
		      -EINVAL);
	zassert_equal(snmp_agent_notify(cold_start, ARRAY_SIZE(cold_start), NULL, 1), -EINVAL);
	zassert_equal(snmp_agent_notify(cold_start, ARRAY_SIZE(cold_start), &no_name, 1), -EINVAL);
	zassert_equal(snmp_agent_notify(cold_start, ARRAY_SIZE(cold_start), &bad_name, 1), -EINVAL);
	zassert_equal(snmp_agent_notify(cold_start, ARRAY_SIZE(cold_start), &bad_value, 1),
		      -EINVAL);
	expect_silence(receiver_v2c);

	zassert_ok(snmp_agent_stop());
}

ZTEST(snmp_notify, test_cold_start_and_authentication_failure)
{
	struct trap t;

	add_target(RECEIVER_V2C_PORT, SNMP_VERSION_V2C, "trapv2");
	add_target(RECEIVER_V1_PORT, SNMP_VERSION_V1, "trapv1");

	zassert_ok(snmp_agent_start());

	expect_trap(receiver_v2c, &t);
	expect_v2c(&t, cold_start, ARRAY_SIZE(cold_start), 0);
	expect_trap(receiver_v1, &t);
	expect_v1(&t, snmp_traps, ARRAY_SIZE(snmp_traps), 0, 0, 0);

	/* An unknown community raises authenticationFailure ... */
	send_request(V2C, "wrong");
	expect_trap(receiver_v2c, &t);
	expect_v2c(&t, auth_failure, ARRAY_SIZE(auth_failure), 0);
	expect_trap(receiver_v1, &t);
	expect_v1(&t, snmp_traps, ARRAY_SIZE(snmp_traps), 4, 0, 0);

	/* ... unless snmpEnableAuthenTraps says otherwise ... */
	snmp_mib2_snmp_set_authen_traps(false);
	send_request(V1, "wrong");
	expect_silence(receiver_v2c);
	expect_silence(receiver_v1);

	/* ... and a known community never does */
	snmp_mib2_snmp_set_authen_traps(true);
	send_request(V2C, "public");
	expect_silence(receiver_v2c);

	zassert_ok(snmp_agent_stop());
}

/* RFC 3584 section 3.2: enterprise-specific notifications in version 1. */
ZTEST(snmp_notify, test_enterprise_notification)
{
	static const uint32_t with_zero[] = {1, 3, 6, 1, 4, 1, 99999, 0, 7};
	static const uint32_t without_zero[] = {1, 3, 6, 1, 4, 1, 99999, 3};
	static const uint32_t enterprise[] = {1, 3, 6, 1, 4, 1, 99999};
	struct trap t;

	add_target(RECEIVER_V2C_PORT, SNMP_VERSION_V2C, "trapv2");
	add_target(RECEIVER_V1_PORT, SNMP_VERSION_V1, "trapv1");

	zassert_ok(snmp_agent_start());
	expect_trap(receiver_v2c, &t);
	expect_trap(receiver_v1, &t);

	zassert_ok(snmp_agent_notify(with_zero, ARRAY_SIZE(with_zero), NULL, 0));
	expect_trap(receiver_v2c, &t);
	expect_v2c(&t, with_zero, ARRAY_SIZE(with_zero), 0);
	expect_trap(receiver_v1, &t);
	expect_v1(&t, enterprise, ARRAY_SIZE(enterprise), 6, 7, 0);

	zassert_ok(snmp_agent_notify(without_zero, ARRAY_SIZE(without_zero), NULL, 0));
	expect_trap(receiver_v2c, &t);
	expect_trap(receiver_v1, &t);
	expect_v1(&t, enterprise, ARRAY_SIZE(enterprise), 6, 3, 0);

	/* Without receivers there is nothing to do, and no error */
	snmp_agent_clear_notification_targets();
	zassert_ok(snmp_agent_notify(with_zero, ARRAY_SIZE(with_zero), NULL, 0));
	expect_silence(receiver_v2c);

	zassert_ok(snmp_agent_stop());
}

/* The OBJECTS of a notification travel in both versions, RFC 3416 4.2.6 and
 * RFC 3584 3.2 step 6, except for Counter64 values, which version 1 cannot
 * carry.
 */
ZTEST(snmp_notify, test_notification_bindings)
{
	static const uint32_t notification[] = {1, 3, 6, 1, 4, 1, 99999, 0, 1};
	static const uint32_t enterprise[] = {1, 3, 6, 1, 4, 1, 99999};
	static const uint32_t level_inst[] = {1, 3, 6, 1, 4, 1, 99999, 1, 2, 0};
	static const uint32_t name_inst[] = {1, 3, 6, 1, 4, 1, 99999, 1, 3, 0};
	static const uint32_t octets_inst[] = {1, 3, 6, 1, 4, 1, 99999, 1, 4, 0};
	const struct snmp_varbind bindings[] = {
		{.oid = level_inst,
		 .oid_len = ARRAY_SIZE(level_inst),
		 .value = {.type = SNMP_TYPE_INTEGER, .i32 = -42}},
		{.oid = name_inst,
		 .oid_len = ARRAY_SIZE(name_inst),
		 .value = {.type = SNMP_TYPE_OCTET_STRING,
			   .octets = {.data = (const uint8_t *)"zephyr", .len = 6}}},
	};
	const struct snmp_varbind counter64 = {
		.oid = octets_inst,
		.oid_len = ARRAY_SIZE(octets_inst),
		.value = {.type = SNMP_TYPE_COUNTER64, .u64 = UINT64_MAX}};
	struct trap t;

	add_target(RECEIVER_V2C_PORT, SNMP_VERSION_V2C, "trapv2");
	add_target(RECEIVER_V1_PORT, SNMP_VERSION_V1, "trapv1");

	zassert_ok(snmp_agent_start());
	expect_trap(receiver_v2c, &t);
	expect_trap(receiver_v1, &t);

	zassert_ok(snmp_agent_notify(notification, ARRAY_SIZE(notification), bindings,
				     ARRAY_SIZE(bindings)));
	expect_trap(receiver_v2c, &t);
	expect_v2c(&t, notification, ARRAY_SIZE(notification), ARRAY_SIZE(bindings));
	expect_int32_binding(&t, level_inst, ARRAY_SIZE(level_inst), -42);
	expect_trap(receiver_v1, &t);
	expect_v1(&t, enterprise, ARRAY_SIZE(enterprise), 6, 1, ARRAY_SIZE(bindings));
	expect_int32_binding(&t, level_inst, ARRAY_SIZE(level_inst), -42);

	/* A Counter64 reaches the version 2c receiver only */
	zassert_ok(snmp_agent_notify(notification, ARRAY_SIZE(notification), &counter64, 1));
	expect_trap(receiver_v2c, &t);
	expect_v2c(&t, notification, ARRAY_SIZE(notification), 1);
	zassert_equal(t.vb_tag, SNMP_BER_TAG_COUNTER64);
	expect_silence(receiver_v1);

	/* The message buffer bounds what a notification can carry */
	{
		uint8_t big[CONFIG_SNMP_MAX_PDU_SIZE];
		const struct snmp_varbind huge = {
			.oid = octets_inst,
			.oid_len = ARRAY_SIZE(octets_inst),
			.value = {.type = SNMP_TYPE_OCTET_STRING,
				  .octets = {.data = big, .len = sizeof(big)}}};

		memset(big, 'x', sizeof(big));
		zassert_equal(snmp_agent_notify(notification, ARRAY_SIZE(notification), &huge, 1),
			      -EMSGSIZE);
		expect_silence(receiver_v2c);
	}

	zassert_ok(snmp_agent_stop());
}

ZTEST_SUITE(snmp_notify, NULL, suite_setup, before, NULL, suite_teardown);
