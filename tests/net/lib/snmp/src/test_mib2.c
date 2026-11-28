/*
 * Copyright (c) 2026 StructCore Embedded
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * MIB-II Network Subsystem Unit Tests (RFC 1213 / RFC 2863)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <zephyr/ztest.h>
#include <zephyr/net/snmp/snmp.h>
#include <zephyr/net/snmp/snmp_mib2.h>

#define ASSERT(c)          zassert_true(c, #c)
#define ASSERT_EQ(a, b)    zassert_equal(a, b, NULL)
#define ASSERT_NE(a, b)    zassert_not_equal(a, b, NULL)
#define ASSERT_GT(a, b)    zassert_true((a) > (b), NULL)
#define ASSERT_LT(a, b)    zassert_true((a) < (b), NULL)
#define ASSERT_STREQ(a, b) zassert_mem_equal(a, b, strlen(b), NULL)

#if defined(__ZEPHYR__)
struct snmp_mock_iface {
	uint32_t index;
	const char *descr;
	uint32_t type;
	uint32_t mtu;
	uint32_t speed;
	uint8_t mac[6];
	uint32_t admin_status;
	uint32_t oper_status;
	uint32_t in_octets;
	uint32_t in_ucast;
	uint32_t in_errors;
	uint32_t out_octets;
	uint32_t out_ucast;
	uint32_t out_errors;
};

void snmp_mib2_mock_set_ifaces(const struct snmp_mock_iface *ifaces, size_t count);
#endif

static struct snmp_agent s_agent;

static const struct snmp_mock_iface s_test_ifaces[2] = {
	{
		.index = 1,
		.descr = "eth0",
		.type = 6, /* ethernetCsmacd */
		.mtu = 1500,
		.speed = 100000000,
		.mac = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55},
		.admin_status = 1, /* up */
		.oper_status = 1,  /* up */
		.in_octets = 1000,
		.in_ucast = 10,
		.in_errors = 1,
		.out_octets = 2000,
		.out_ucast = 20,
		.out_errors = 2,
	},
	{
		.index = 2,
		.descr = "lo0",
		.type = 24, /* softwareLoopback */
		.mtu = 65535,
		.speed = 10000000,
		.mac = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		.admin_status = 1,
		.oper_status = 1,
		.in_octets = 500,
		.in_ucast = 5,
		.in_errors = 0,
		.out_octets = 500,
		.out_ucast = 5,
		.out_errors = 0,
	},
};

ZTEST(snmp_mib2, test_mib2_interfaces_group)
{
	int ret;
	struct snmp_varbind vb;

	ret = snmp_agent_init(&s_agent, NULL);
	ASSERT_EQ(ret, 0);

	snmp_mib2_mock_set_ifaces(s_test_ifaces, 2);

	ret = snmp_mib2_register_interfaces(&s_agent);
	ASSERT_EQ(ret, 0);

	/* 1. Test ifNumber.0 (1.3.6.1.2.1.2.1.0) */
	struct snmp_oid oid_ifnumber = {
		.len = 9,
		.ids = {1, 3, 6, 1, 2, 1, 2, 1, 0},
	};
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get(&s_agent, &oid_ifnumber, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, ASN1_TAG_INTEGER);
	ASSERT_EQ(vb.val.int_val, 2);

	/* 2. Test ifTable GET col 1 (index), row 1: 1.3.6.1.2.1.2.2.1.1.1 */
	struct snmp_oid oid_c1_r1 = {
		.len = 11,
		.ids = {1, 3, 6, 1, 2, 1, 2, 2, 1, 1, 1},
	};
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get(&s_agent, &oid_c1_r1, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, ASN1_TAG_INTEGER);
	ASSERT_EQ(vb.val.int_val, 1);

	/* 3. Test ifTable GET col 2 (descr "eth0"), row 1: 1.3.6.1.2.1.2.2.1.2.1 */
	struct snmp_oid oid_c2_r1 = {
		.len = 11,
		.ids = {1, 3, 6, 1, 2, 1, 2, 2, 1, 2, 1},
	};
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get(&s_agent, &oid_c2_r1, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, ASN1_TAG_OCTET_STRING);
	ASSERT_EQ(vb.val.octet_str.len, 4);
	ASSERT_EQ(memcmp(vb.val.octet_str.data, "eth0", 4), 0);

	/* 4. Test ifTable GET col 6 (MAC), row 1: 1.3.6.1.2.1.2.2.1.6.1 */
	struct snmp_oid oid_c6_r1 = {
		.len = 11,
		.ids = {1, 3, 6, 1, 2, 1, 2, 2, 1, 6, 1},
	};
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get(&s_agent, &oid_c6_r1, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, ASN1_TAG_OCTET_STRING);
	ASSERT_EQ(vb.val.octet_str.len, 6);
	ASSERT_EQ(memcmp(vb.val.octet_str.data, s_test_ifaces[0].mac, 6), 0);

	/* 5. Test ifTable GET col 10 (in_octets), row 1: 1.3.6.1.2.1.2.2.1.10.1 */
	struct snmp_oid oid_c10_r1 = {
		.len = 11,
		.ids = {1, 3, 6, 1, 2, 1, 2, 2, 1, 10, 1},
	};
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get(&s_agent, &oid_c10_r1, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, SNMP_TAG_COUNTER32);
	ASSERT_EQ(vb.val.uint_val, 1000);

	/* 6. Test row 2: col 1 (index 2), col 2 ("lo0") */
	struct snmp_oid oid_c1_r2 = {
		.len = 11,
		.ids = {1, 3, 6, 1, 2, 1, 2, 2, 1, 1, 2},
	};
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get(&s_agent, &oid_c1_r2, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, ASN1_TAG_INTEGER);
	ASSERT_EQ(vb.val.int_val, 2);

	struct snmp_oid oid_c2_r2 = {
		.len = 11,
		.ids = {1, 3, 6, 1, 2, 1, 2, 2, 1, 2, 2},
	};
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get(&s_agent, &oid_c2_r2, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, ASN1_TAG_OCTET_STRING);
	ASSERT_EQ(vb.val.octet_str.len, 3);
	ASSERT_EQ(memcmp(vb.val.octet_str.data, "lo0", 3), 0);

	/* 7. Test GETNEXT progression across columns and rows */
	/* From 1.3.6.1.2.1.2.2.1 -> next should be col 1, row 1 */
	struct snmp_oid cur_oid = {
		.len = 9,
		.ids = {1, 3, 6, 1, 2, 1, 2, 2, 1},
	};
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get_next(&s_agent, &cur_oid, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(snmp_oid_equal(&vb.oid, &oid_c1_r1), 1);
	ASSERT_EQ(vb.val.int_val, 1);

	/* Next from col 1, row 1 -> col 1, row 2 */
	cur_oid = vb.oid;
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get_next(&s_agent, &cur_oid, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(snmp_oid_equal(&vb.oid, &oid_c1_r2), 1);
	ASSERT_EQ(vb.val.int_val, 2);

	/* Next from col 1, row 2 -> col 2, row 1 */
	cur_oid = vb.oid;
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get_next(&s_agent, &cur_oid, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(snmp_oid_equal(&vb.oid, &oid_c2_r1), 1);
	ASSERT_EQ(vb.val.octet_str.len, 4);

	/* Next from col 2, row 1 -> col 2, row 2 */
	cur_oid = vb.oid;
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get_next(&s_agent, &cur_oid, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(snmp_oid_equal(&vb.oid, &oid_c2_r2), 1);
	ASSERT_EQ(vb.val.octet_str.len, 3);
}

ZTEST(snmp_mib2, test_mib2_ip_group)
{
	int ret;
	struct snmp_varbind vb;

	ret = snmp_agent_init(&s_agent, NULL);
	ASSERT_EQ(ret, 0);

	ret = snmp_mib2_register_ip(&s_agent);
	ASSERT_EQ(ret, 0);

	/* ipForwarding.0 (1.3.6.1.2.1.4.1.0) */
	struct snmp_oid oid_fwd = {
		.len = 9,
		.ids = {1, 3, 6, 1, 2, 1, 4, 1, 0},
	};
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get(&s_agent, &oid_fwd, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, ASN1_TAG_INTEGER);
	ASSERT_EQ(vb.val.int_val, 2);

	/* ipDefaultTTL.0 (1.3.6.1.2.1.4.2.0) */
	struct snmp_oid oid_ttl = {
		.len = 9,
		.ids = {1, 3, 6, 1, 2, 1, 4, 2, 0},
	};
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get(&s_agent, &oid_ttl, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, ASN1_TAG_INTEGER);
	ASSERT_EQ(vb.val.int_val, 64);

	/* ipInReceives.0 (1.3.6.1.2.1.4.3.0) */
	struct snmp_oid oid_in_recv = {
		.len = 9,
		.ids = {1, 3, 6, 1, 2, 1, 4, 3, 0},
	};
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get(&s_agent, &oid_in_recv, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, SNMP_TAG_COUNTER32);
	ASSERT_EQ(vb.val.uint_val, 100);

	/* ipInDelivers.0 (1.3.6.1.2.1.4.9.0) */
	struct snmp_oid oid_in_deliv = {
		.len = 9,
		.ids = {1, 3, 6, 1, 2, 1, 4, 9, 0},
	};
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get(&s_agent, &oid_in_deliv, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, SNMP_TAG_COUNTER32);
	ASSERT_EQ(vb.val.uint_val, 100);

	/* ipOutRequests.0 (1.3.6.1.2.1.4.10.0) */
	struct snmp_oid oid_out_req = {
		.len = 9,
		.ids = {1, 3, 6, 1, 2, 1, 4, 10, 0},
	};
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get(&s_agent, &oid_out_req, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, SNMP_TAG_COUNTER32);
	ASSERT_EQ(vb.val.uint_val, 50);
}

ZTEST(snmp_mib2, test_mib2_icmp_group)
{
	int ret;
	struct snmp_varbind vb;

	ret = snmp_agent_init(&s_agent, NULL);
	ASSERT_EQ(ret, 0);

	ret = snmp_mib2_register_icmp(&s_agent);
	ASSERT_EQ(ret, 0);

	/* icmpInMsgs.0 (1.3.6.1.2.1.5.1.0) */
	struct snmp_oid oid_in_msgs = {
		.len = 9,
		.ids = {1, 3, 6, 1, 2, 1, 5, 1, 0},
	};
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get(&s_agent, &oid_in_msgs, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, SNMP_TAG_COUNTER32);
	ASSERT_EQ(vb.val.uint_val, 10);

	/* icmpInErrors.0 (1.3.6.1.2.1.5.2.0) */
	struct snmp_oid oid_in_err = {
		.len = 9,
		.ids = {1, 3, 6, 1, 2, 1, 5, 2, 0},
	};
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get(&s_agent, &oid_in_err, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, SNMP_TAG_COUNTER32);
	ASSERT_EQ(vb.val.uint_val, 0);

	/* icmpInDestUnreachs.0 (1.3.6.1.2.1.5.3.0) */
	struct snmp_oid oid_in_unreach = {
		.len = 9,
		.ids = {1, 3, 6, 1, 2, 1, 5, 3, 0},
	};
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get(&s_agent, &oid_in_unreach, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, SNMP_TAG_COUNTER32);
	ASSERT_EQ(vb.val.uint_val, 1);

	/* icmpInEchos.0 (1.3.6.1.2.1.5.8.0) */
	struct snmp_oid oid_in_echo = {
		.len = 9,
		.ids = {1, 3, 6, 1, 2, 1, 5, 8, 0},
	};
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get(&s_agent, &oid_in_echo, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, SNMP_TAG_COUNTER32);
	ASSERT_EQ(vb.val.uint_val, 8);

	/* icmpOutMsgs.0 (1.3.6.1.2.1.5.14.0) */
	struct snmp_oid oid_out_msgs = {
		.len = 9,
		.ids = {1, 3, 6, 1, 2, 1, 5, 14, 0},
	};
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get(&s_agent, &oid_out_msgs, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, SNMP_TAG_COUNTER32);
	ASSERT_EQ(vb.val.uint_val, 12);

	/* icmpOutEchoReps.0 (1.3.6.1.2.1.5.21.0) */
	struct snmp_oid oid_out_echorep = {
		.len = 9,
		.ids = {1, 3, 6, 1, 2, 1, 5, 21, 0},
	};
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get(&s_agent, &oid_out_echorep, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, SNMP_TAG_COUNTER32);
	ASSERT_EQ(vb.val.uint_val, 8);
}

ZTEST(snmp_mib2, test_mib2_tcp_udp_groups)
{
	int ret;
	struct snmp_varbind vb;

	ret = snmp_agent_init(&s_agent, NULL);
	ASSERT_EQ(ret, 0);

	ret = snmp_mib2_register_tcp(&s_agent);
	ASSERT_EQ(ret, 0);

	ret = snmp_mib2_register_udp(&s_agent);
	ASSERT_EQ(ret, 0);

	/* tcpRtoAlgorithm.0 (1.3.6.1.2.1.6.1.0) */
	struct snmp_oid oid_rto_alg = {
		.len = 9,
		.ids = {1, 3, 6, 1, 2, 1, 6, 1, 0},
	};
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get(&s_agent, &oid_rto_alg, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, ASN1_TAG_INTEGER);
	ASSERT_EQ(vb.val.int_val, 4);

	/* tcpInSegs.0 (1.3.6.1.2.1.6.10.0) */
	struct snmp_oid oid_tcp_in = {
		.len = 9,
		.ids = {1, 3, 6, 1, 2, 1, 6, 10, 0},
	};
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get(&s_agent, &oid_tcp_in, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, SNMP_TAG_COUNTER32);
	ASSERT_EQ(vb.val.uint_val, 500);

	/* tcpOutSegs.0 (1.3.6.1.2.1.6.11.0) */
	struct snmp_oid oid_tcp_out = {
		.len = 9,
		.ids = {1, 3, 6, 1, 2, 1, 6, 11, 0},
	};
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get(&s_agent, &oid_tcp_out, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, SNMP_TAG_COUNTER32);
	ASSERT_EQ(vb.val.uint_val, 450);

	/* udpInDatagrams.0 (1.3.6.1.2.1.7.1.0) */
	struct snmp_oid oid_udp_in = {
		.len = 9,
		.ids = {1, 3, 6, 1, 2, 1, 7, 1, 0},
	};
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get(&s_agent, &oid_udp_in, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, SNMP_TAG_COUNTER32);
	ASSERT_EQ(vb.val.uint_val, 200);

	/* udpNoPorts.0 (1.3.6.1.2.1.7.2.0) */
	struct snmp_oid oid_udp_noports = {
		.len = 9,
		.ids = {1, 3, 6, 1, 2, 1, 7, 2, 0},
	};
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get(&s_agent, &oid_udp_noports, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, SNMP_TAG_COUNTER32);
	ASSERT_EQ(vb.val.uint_val, 3);

	/* udpInErrors.0 (1.3.6.1.2.1.7.3.0) */
	struct snmp_oid oid_udp_err = {
		.len = 9,
		.ids = {1, 3, 6, 1, 2, 1, 7, 3, 0},
	};
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get(&s_agent, &oid_udp_err, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, SNMP_TAG_COUNTER32);
	ASSERT_EQ(vb.val.uint_val, 1);

	/* udpOutDatagrams.0 (1.3.6.1.2.1.7.4.0) */
	struct snmp_oid oid_udp_out = {
		.len = 9,
		.ids = {1, 3, 6, 1, 2, 1, 7, 4, 0},
	};
	memset(&vb, 0, sizeof(vb));
	ret = snmp_mib_get(&s_agent, &oid_udp_out, &vb);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(vb.type, SNMP_TAG_COUNTER32);
	ASSERT_EQ(vb.val.uint_val, 150);
}
