/*
 * Copyright (c) 2026 StructCore Embedded
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Trap Destination & Custom Port Unit Tests
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <zephyr/ztest.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/snmp/snmp.h>
#include <zephyr/net/snmp/snmp_core.h>
#include <zephyr/net/snmp/snmp_trap.h>

#define ASSERT(c)       zassert_true(c, #c)
#define ASSERT_EQ(a, b) zassert_equal(a, b, NULL)
#define ASSERT_NE(a, b) zassert_not_equal(a, b, NULL)

static struct snmp_agent s_agent;

ZTEST(snmp_trap, test_trap_default_port)
{
	int ret = snmp_agent_init(&s_agent, NULL);
	ASSERT_EQ(ret, 0);

	ret = snmp_trap_add_destination(&s_agent, "192.168.1.100");
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(s_agent.trap_dest_cnt, 1);
	ASSERT_EQ(ntohs(s_agent.trap_dests[0].sin_port), CONFIG_SNMP_TRAP_PORT);

	struct in_addr exp;
	ret = zsock_inet_pton(AF_INET, "192.168.1.100", &exp);
	ASSERT_EQ(ret, 1);
	ASSERT_EQ(s_agent.trap_dests[0].sin_addr.s_addr, exp.s_addr);
}

ZTEST(snmp_trap, test_trap_custom_port_arg)
{
	int ret = snmp_agent_init(&s_agent, NULL);
	ASSERT_EQ(ret, 0);

	ret = snmp_trap_add_destination_port(&s_agent, "192.168.1.101", 1162);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(s_agent.trap_dest_cnt, 1);
	ASSERT_EQ(ntohs(s_agent.trap_dests[0].sin_port), 1162);

	struct in_addr exp;
	ret = zsock_inet_pton(AF_INET, "192.168.1.101", &exp);
	ASSERT_EQ(ret, 1);
	ASSERT_EQ(s_agent.trap_dests[0].sin_addr.s_addr, exp.s_addr);
}

ZTEST(snmp_trap, test_trap_inline_port_syntax)
{
	int ret = snmp_agent_init(&s_agent, NULL);
	ASSERT_EQ(ret, 0);

	ret = snmp_trap_add_destination(&s_agent, "192.168.1.102:2162");
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(s_agent.trap_dest_cnt, 1);
	ASSERT_EQ(ntohs(s_agent.trap_dests[0].sin_port), 2162);

	ret = snmp_trap_add_destination(&s_agent, "[192.168.1.103]:3162");
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(s_agent.trap_dest_cnt, 2);
	ASSERT_EQ(ntohs(s_agent.trap_dests[1].sin_port), 3162);

	struct in_addr exp1;
	struct in_addr exp2;
	zsock_inet_pton(AF_INET, "192.168.1.102", &exp1);
	zsock_inet_pton(AF_INET, "192.168.1.103", &exp2);
	ASSERT_EQ(s_agent.trap_dests[0].sin_addr.s_addr, exp1.s_addr);
	ASSERT_EQ(s_agent.trap_dests[1].sin_addr.s_addr, exp2.s_addr);
}

ZTEST(snmp_trap, test_trap_four_independent_servers)
{
	int ret = snmp_agent_init(&s_agent, NULL);
	ASSERT_EQ(ret, 0);

	/* Server 1: default port 162 */
	ret = snmp_trap_add_destination(&s_agent, "10.0.0.1");
	ASSERT_EQ(ret, 0);

	/* Server 2: custom port 1162 via parameter */
	ret = snmp_trap_add_destination_port(&s_agent, "10.0.0.2", 1162);
	ASSERT_EQ(ret, 0);

	/* Server 3: custom port 2162 via inline syntax */
	ret = snmp_trap_add_destination(&s_agent, "10.0.0.3:2162");
	ASSERT_EQ(ret, 0);

	/* Server 4: custom port 3162 via bracket syntax */
	ret = snmp_trap_add_destination_port(&s_agent, "[10.0.0.4]:3162", 3162);
	ASSERT_EQ(ret, 0);

	ASSERT_EQ(s_agent.trap_dest_cnt, 4);
	ASSERT_EQ(ntohs(s_agent.trap_dests[0].sin_port), 162);
	ASSERT_EQ(ntohs(s_agent.trap_dests[1].sin_port), 1162);
	ASSERT_EQ(ntohs(s_agent.trap_dests[2].sin_port), 2162);
	ASSERT_EQ(ntohs(s_agent.trap_dests[3].sin_port), 3162);

	/* 5th server exceeds CONFIG_SNMP_TRAP_DEST_COUNT (4) */
	ret = snmp_trap_add_destination(&s_agent, "10.0.0.5");
	ASSERT_EQ(ret, -ENOMEM);
	ASSERT_EQ(s_agent.trap_dest_cnt, 4);
}

ZTEST(snmp_trap, test_trap_clear_destinations)
{
	int ret = snmp_agent_init(&s_agent, NULL);
	ASSERT_EQ(ret, 0);

	ret = snmp_trap_add_destination(&s_agent, "10.0.0.1");
	ASSERT_EQ(ret, 0);
	ret = snmp_trap_add_destination(&s_agent, "10.0.0.2");
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(s_agent.trap_dest_cnt, 2);

	ret = snmp_trap_clear_destinations(&s_agent);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(s_agent.trap_dest_cnt, 0);

	/* Can add 4 destinations again */
	ASSERT_EQ(snmp_trap_add_destination(&s_agent, "10.0.0.1"), 0);
	ASSERT_EQ(snmp_trap_add_destination(&s_agent, "10.0.0.2"), 0);
	ASSERT_EQ(snmp_trap_add_destination(&s_agent, "10.0.0.3"), 0);
	ASSERT_EQ(snmp_trap_add_destination(&s_agent, "10.0.0.4"), 0);
	ASSERT_EQ(s_agent.trap_dest_cnt, 4);
}

ZTEST(snmp_trap, test_trap_validation_errors)
{
	int ret = snmp_agent_init(&s_agent, NULL);
	ASSERT_EQ(ret, 0);

	ASSERT_EQ(snmp_trap_add_destination(NULL, "10.0.0.1"), -EINVAL);
	ASSERT_EQ(snmp_trap_add_destination(&s_agent, NULL), -EINVAL);
	ASSERT_EQ(snmp_trap_clear_destinations(NULL), -EINVAL);

	/* Bad IP */
	ASSERT_EQ(snmp_trap_add_destination(&s_agent, "not.an.ip"), -EINVAL);
	ASSERT_EQ(snmp_trap_add_destination(&s_agent, "256.1.1.1"), -EINVAL);

	/* Bad ports */
	ASSERT_EQ(snmp_trap_add_destination(&s_agent, "10.0.0.1:0"), -EINVAL);
	ASSERT_EQ(snmp_trap_add_destination(&s_agent, "10.0.0.1:70000"), -EINVAL);
	ASSERT_EQ(snmp_trap_add_destination(&s_agent, "10.0.0.1:abc"), -EINVAL);

	/* Conflict */
	ASSERT_EQ(snmp_trap_add_destination_port(&s_agent, "10.0.0.1:1162", 2162), -EINVAL);

	/* Bad brackets */
	ASSERT_EQ(snmp_trap_add_destination(&s_agent, "[10.0.0.1"), -EINVAL);
	ASSERT_EQ(snmp_trap_add_destination(&s_agent, "[10.0.0.1]extra"), -EINVAL);
}
