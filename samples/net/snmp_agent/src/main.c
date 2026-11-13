/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(snmp_sample, LOG_LEVEL_INF);

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/snmp.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/util.h>

/* The sample's objects live under the experimental subtree of RFC 2578. A
 * product registers its enterprise number with IANA and places its objects
 * under 1.3.6.1.4.1.<enterprise>.
 */
#define SAMPLE_BASE          1, 3, 6, 1, 3, 9
#define SAMPLE_NOTIFICATIONS SAMPLE_BASE, 0
#define SAMPLE_OBJECTS       SAMPLE_BASE, 1
#define SAMPLE_CAPABILITIES  SAMPLE_BASE, 2

#define LEVEL_MIN 0
#define LEVEL_MAX 100

/* Reported as sysObjectID */
static const uint32_t sample_object_id[] = {SAMPLE_BASE};

static int32_t level = 50;

/* sampleUpTime: seconds since boot, a read-only Counter32 */
static int uptime_get(const struct snmp_object *obj, void *row, struct snmp_value *val)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(row);

	val->type = SNMP_TYPE_COUNTER32;
	val->u32 = (uint32_t)(k_uptime_get() / MSEC_PER_SEC);

	return 0;
}

/* sampleLevel: a writable INTEGER in the range 0..100 */
static int level_get(const struct snmp_object *obj, void *row, struct snmp_value *val)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(row);

	val->type = SNMP_TYPE_INTEGER;
	val->i32 = level;

	return 0;
}

static int level_set(const struct snmp_object *obj, void *row, const struct snmp_value *val,
		     enum snmp_set_phase phase)
{
	/* sampleLevelChanged, sent when a manager changes sampleLevel, carries
	 * the new value of sampleLevel.0 as its OBJECTS clause says.
	 */
	static const uint32_t level_changed[] = {SAMPLE_NOTIFICATIONS, 1};
	static const uint32_t level_instance[] = {SAMPLE_OBJECTS, 2, 0};
	const struct snmp_varbind bindings[] = {
		{.oid = level_instance,
		 .oid_len = ARRAY_SIZE(level_instance),
		 .value = {.type = SNMP_TYPE_INTEGER, .i32 = val->i32}},
	};
	int ret;

	ARG_UNUSED(obj);
	ARG_UNUSED(row);

	/* The check phase validates; nothing may change here. */
	if (val->type != SNMP_TYPE_INTEGER) {
		return -EINVAL;
	}

	if (val->i32 < LEVEL_MIN || val->i32 > LEVEL_MAX) {
		return -ERANGE;
	}

	if (phase == SNMP_SET_CHECK) {
		return 0;
	}

	level = val->i32;
	LOG_INF("sampleLevel set to %d", level);

	/* Tell the receivers; failing to reach them does not fail the SET. */
	ret = snmp_agent_notify(level_changed, ARRAY_SIZE(level_changed), bindings,
				ARRAY_SIZE(bindings));
	if (ret != 0) {
		LOG_WRN("sampleLevelChanged not sent (%d)", ret);
	}

	return 0;
}

SNMP_SCALAR_DEFINE(sample_uptime, uptime_get, NULL, NULL, SAMPLE_OBJECTS, 1);
SNMP_SCALAR_DEFINE(sample_level, level_get, level_set, NULL, SAMPLE_OBJECTS, 2);

/* Listed in sysORTable next to the library's own compliance statement */
SNMP_CAPABILITY_DEFINE(sample_capabilities, "Objects of the Zephyr SNMP agent sample",
		       SAMPLE_CAPABILITIES, 1);

static int add_notification_receiver(void)
{
	const char *host = CONFIG_NET_SAMPLE_SNMP_TRAP_RECEIVER;
	struct net_sockaddr_in addr4 = {.sin_family = NET_AF_INET,
					.sin_port = net_htons(SNMP_TRAP_PORT)};
	struct net_sockaddr_in6 addr6 = {.sin6_family = NET_AF_INET6,
					 .sin6_port = net_htons(SNMP_TRAP_PORT)};
	struct snmp_notification_target target = {
		.version = IS_ENABLED(CONFIG_NET_SAMPLE_SNMP_TRAP_V1) ? SNMP_VERSION_V1
								      : SNMP_VERSION_V2C,
		.community = CONFIG_NET_SAMPLE_SNMP_TRAP_COMMUNITY,
	};

	if (host[0] == '\0') {
		return 0;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) &&
	    zsock_inet_pton(NET_AF_INET, host, &addr4.sin_addr) == 1) {
		target.addr = (struct net_sockaddr *)&addr4;
		target.addrlen = sizeof(addr4);
	} else if (IS_ENABLED(CONFIG_NET_IPV6) &&
		   zsock_inet_pton(NET_AF_INET6, host, &addr6.sin6_addr) == 1) {
		target.addr = (struct net_sockaddr *)&addr6;
		target.addrlen = sizeof(addr6);
	} else {
		LOG_ERR("Cannot parse notification receiver %s", host);
		return -EINVAL;
	}

	return snmp_agent_add_notification_target(&target);
}

int main(void)
{
	int ret;

	ret = snmp_agent_set_community(SNMP_COMMUNITY_READ, CONFIG_NET_SAMPLE_SNMP_READ_COMMUNITY);
	if (ret == 0) {
		ret = snmp_agent_set_community(SNMP_COMMUNITY_WRITE,
					       CONFIG_NET_SAMPLE_SNMP_WRITE_COMMUNITY);
	}

	if (ret != 0) {
		LOG_ERR("Cannot set community strings (%d)", ret);
		return 0;
	}

	ret = snmp_mib2_system_set_object_id(sample_object_id, ARRAY_SIZE(sample_object_id));
	if (ret != 0) {
		LOG_ERR("Cannot set sysObjectID (%d)", ret);
		return 0;
	}

	/* Receivers added before the start get the coldStart notification. */
	ret = add_notification_receiver();
	if (ret != 0) {
		LOG_ERR("Cannot add notification receiver (%d)", ret);
		return 0;
	}

	ret = snmp_agent_start();
	if (ret != 0) {
		LOG_ERR("Cannot start the SNMP agent (%d)", ret);
		return 0;
	}

	LOG_INF("SNMP agent listening on UDP port %d", CONFIG_SNMP_PORT);

	/* The agent answers from the socket service thread; main is done. */
	return 0;
}
