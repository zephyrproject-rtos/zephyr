/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* Notification originator: SNMPv2-Trap-PDUs of RFC 3416 and, for version 1
 * receivers, Trap-PDUs of RFC 1157 translated as RFC 3584 section 3.2
 * describes.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_snmp, CONFIG_SNMP_LOG_LEVEL);

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/snmp.h>
#include <zephyr/sys/util.h>

#include "snmp_ber.h"
#include "snmp_internal.h"

/* snmpTraps of RFC 3418: the standard notifications and the version 1
 * enterprise of their translations.
 */
static const uint32_t snmp_traps[] = {1, 3, 6, 1, 6, 3, 1, 1, 5};
#define STANDARD_TRAP_COUNT 6U

static const uint32_t sys_uptime_instance[] = {1, 3, 6, 1, 2, 1, 1, 3, 0};
static const uint32_t snmp_trap_oid_instance[] = {1, 3, 6, 1, 6, 3, 1, 1, 4, 1, 0};
static const uint32_t cold_start[] = {SNMP_TRAP_COLD_START};
static const uint32_t authentication_failure[] = {SNMP_TRAP_AUTHENTICATION_FAILURE};

#define GENERIC_ENTERPRISE_SPECIFIC 6

struct target {
	struct net_sockaddr_storage addr;
	net_socklen_t addrlen;
	enum snmp_version version;
	char community[CONFIG_SNMP_COMMUNITY_MAX_LEN + 1];
};

static struct target targets[CONFIG_SNMP_NOTIFICATION_TARGETS];
static size_t target_count;
static int32_t request_id;
static uint8_t msg_buf[CONFIG_SNMP_MAX_PDU_SIZE];
static K_MUTEX_DEFINE(lock);

int snmp_agent_add_notification_target(const struct snmp_notification_target *target)
{
	struct target *t;
	size_t len;
	int ret = 0;

	if (target == NULL || target->addr == NULL || target->community == NULL ||
	    target->addrlen > sizeof(t->addr)) {
		return -EINVAL;
	}

	if (target->addr->sa_family != NET_AF_INET && target->addr->sa_family != NET_AF_INET6) {
		return -EINVAL;
	}

	if (target->version != SNMP_VERSION_V1 && target->version != SNMP_VERSION_V2C) {
		return -EINVAL;
	}

	len = strlen(target->community);
	if (len > CONFIG_SNMP_COMMUNITY_MAX_LEN) {
		return -EMSGSIZE;
	}

	k_mutex_lock(&lock, K_FOREVER);

	if (target_count >= ARRAY_SIZE(targets)) {
		ret = -ENOMEM;
	} else {
		t = &targets[target_count++];
		memcpy(&t->addr, target->addr, target->addrlen);
		t->addrlen = target->addrlen;
		t->version = target->version;
		memcpy(t->community, target->community, len + 1U);
	}

	k_mutex_unlock(&lock);

	return ret;
}

void snmp_agent_clear_notification_targets(void)
{
	k_mutex_lock(&lock, K_FOREVER);
	target_count = 0U;
	k_mutex_unlock(&lock);
}

static uint32_t uptime_ticks(void)
{
	return (uint32_t)((k_uptime_get() - snmp_agent_start_uptime()) / 10);
}

/* Common framing: SEQUENCE { version, community, PDU }. Returns the message
 * length, or 0 when it does not fit.
 */
static size_t write_message(struct snmp_ber_writer *w, const struct target *t, uint8_t pdu_tag,
			    size_t pdu_content)
{
	size_t community_len = strlen(t->community);
	size_t msg_content = snmp_ber_sizeof_int32((int32_t)t->version) +
			     snmp_ber_sizeof_octets(community_len) +
			     snmp_ber_sizeof_element(pdu_content);
	int ret;

	ret = snmp_ber_write_header(w, SNMP_BER_TAG_SEQUENCE, msg_content);
	ret |= snmp_ber_write_int32(w, (int32_t)t->version);
	ret |= snmp_ber_write_octets(w, SNMP_BER_TAG_OCTET_STRING, (const uint8_t *)t->community,
				     community_len);
	ret |= snmp_ber_write_header(w, pdu_tag, pdu_content);

	return (ret == 0) ? snmp_ber_sizeof_element(msg_content) : 0U;
}

/* Encoded size of the application's bindings; every one was validated. */
static size_t bindings_size(const struct snmp_varbind *bindings, size_t count)
{
	size_t n = 0U;

	for (size_t i = 0U; i < count; i++) {
		n += snmp_varbind_size(bindings[i].oid, bindings[i].oid_len, &bindings[i].value);
	}

	return n;
}

static int write_bindings(struct snmp_ber_writer *w, const struct snmp_varbind *bindings,
			  size_t count)
{
	int ret = 0;

	for (size_t i = 0U; i < count; i++) {
		ret |= snmp_write_varbind(w, bindings[i].oid, bindings[i].oid_len,
					  &bindings[i].value, 0U);
	}

	return ret;
}

static bool bindings_have_counter64(const struct snmp_varbind *bindings, size_t count)
{
	for (size_t i = 0U; i < count; i++) {
		if (bindings[i].value.type == SNMP_TYPE_COUNTER64) {
			return true;
		}
	}

	return false;
}

static size_t build_v2c(struct snmp_ber_writer *w, const struct target *t,
			const uint32_t *notification, size_t len,
			const struct snmp_varbind *bindings, size_t count, uint32_t ticks)
{
	struct snmp_value uptime = {.type = SNMP_TYPE_TIMETICKS, .u32 = ticks};
	struct snmp_value trap_oid = {.type = SNMP_TYPE_OID, .oid = {notification, len}};
	size_t list =
		snmp_varbind_size(sys_uptime_instance, ARRAY_SIZE(sys_uptime_instance), &uptime) +
		snmp_varbind_size(snmp_trap_oid_instance, ARRAY_SIZE(snmp_trap_oid_instance),
				  &trap_oid) +
		bindings_size(bindings, count);
	size_t pdu = snmp_ber_sizeof_int32(request_id) + 2U * snmp_ber_sizeof_int32(0) +
		     snmp_ber_sizeof_element(list);
	size_t total;
	int ret;

	total = write_message(w, t, SNMP_BER_TAG_TRAP_V2, pdu);
	if (total == 0U) {
		return 0U;
	}

	ret = snmp_ber_write_int32(w, request_id);
	ret |= snmp_ber_write_int32(w, 0);
	ret |= snmp_ber_write_int32(w, 0);
	ret |= snmp_ber_write_header(w, SNMP_BER_TAG_SEQUENCE, list);
	ret |= snmp_write_varbind(w, sys_uptime_instance, ARRAY_SIZE(sys_uptime_instance), &uptime,
				  0U);
	ret |= snmp_write_varbind(w, snmp_trap_oid_instance, ARRAY_SIZE(snmp_trap_oid_instance),
				  &trap_oid, 0U);
	ret |= write_bindings(w, bindings, count);

	return (ret == 0) ? total : 0U;
}

/* RFC 3584 section 3.2, steps 1, 3 and 4. */
static void translate_v1(const uint32_t *notification, size_t len, const uint32_t **enterprise,
			 size_t *enterprise_len, int32_t *generic, int32_t *specific)
{
	if (len == ARRAY_SIZE(snmp_traps) + 1U &&
	    snmp_oid_has_prefix(notification, len, snmp_traps, ARRAY_SIZE(snmp_traps)) &&
	    notification[len - 1U] >= 1U && notification[len - 1U] <= STANDARD_TRAP_COUNT) {
		*enterprise = snmp_traps;
		*enterprise_len = ARRAY_SIZE(snmp_traps);
		*generic = (int32_t)notification[len - 1U] - 1;
		*specific = 0;
		return;
	}

	*enterprise = notification;
	*enterprise_len = (notification[len - 2U] == 0U) ? len - 2U : len - 1U;
	*generic = GENERIC_ENTERPRISE_SPECIFIC;
	*specific = (int32_t)notification[len - 1U];
}

static size_t build_v1(struct snmp_ber_writer *w, const struct target *t,
		       const uint32_t *notification, size_t len,
		       const struct snmp_varbind *bindings, size_t count, uint32_t ticks)
{
	const uint32_t *enterprise;
	size_t enterprise_len;
	int32_t generic, specific;
	uint8_t agent_addr[4] = {0};
	size_t list = bindings_size(bindings, count);
	size_t pdu, total;
	int ret;

	translate_v1(notification, len, &enterprise, &enterprise_len, &generic, &specific);

	/* RFC 3584 section 3.2 step 2: the agent's IP address, which only an
	 * IPv4 receiver can be told about.
	 */
	if (IS_ENABLED(CONFIG_NET_IPV4) && t->addr.ss_family == NET_AF_INET) {
		const struct net_in_addr *src = net_if_ipv4_select_src_addr(
			NULL, &net_sin((const struct net_sockaddr *)&t->addr)->sin_addr);

		if (src != NULL) {
			memcpy(agent_addr, src, sizeof(agent_addr));
		}
	}

	pdu = snmp_ber_sizeof_oid(enterprise, enterprise_len) +
	      snmp_ber_sizeof_octets(sizeof(agent_addr)) + snmp_ber_sizeof_int32(generic) +
	      snmp_ber_sizeof_int32(specific) + snmp_ber_sizeof_uint32(ticks) +
	      snmp_ber_sizeof_element(list);

	total = write_message(w, t, SNMP_BER_TAG_TRAP_V1, pdu);
	if (total == 0U) {
		return 0U;
	}

	ret = snmp_ber_write_oid(w, enterprise, enterprise_len);
	ret |= snmp_ber_write_octets(w, SNMP_BER_TAG_IPADDRESS, agent_addr, sizeof(agent_addr));
	ret |= snmp_ber_write_int32(w, generic);
	ret |= snmp_ber_write_int32(w, specific);
	ret |= snmp_ber_write_uint32(w, SNMP_BER_TAG_TIMETICKS, ticks);
	ret |= snmp_ber_write_header(w, SNMP_BER_TAG_SEQUENCE, list);
	ret |= write_bindings(w, bindings, count);

	return (ret == 0) ? total : 0U;
}

/*
 * Reject what no receiver could be sent, so that a message the builders
 * cannot produce afterwards can only be one that does not fit.
 */
static int validate_notification(const uint32_t *notification, size_t len,
				 const struct snmp_varbind *bindings, size_t count)
{
	const uint32_t *enterprise;
	size_t enterprise_len;
	int32_t generic, specific;

	if (notification == NULL || len < 2U || (bindings == NULL && count > 0U)) {
		return -EINVAL;
	}

	if (snmp_ber_sizeof_oid(notification, len) == 0U) {
		return -EINVAL;
	}

	/* A version 1 enterprise is an object identifier of its own. */
	translate_v1(notification, len, &enterprise, &enterprise_len, &generic, &specific);
	if (enterprise_len < 2U) {
		return -EINVAL;
	}

	for (size_t i = 0U; i < count; i++) {
		if (bindings[i].oid == NULL ||
		    snmp_varbind_size(bindings[i].oid, bindings[i].oid_len, &bindings[i].value) ==
			    0U) {
			return -EINVAL;
		}
	}

	return 0;
}

int snmp_agent_notify(const uint32_t *notification, size_t len, const struct snmp_varbind *bindings,
		      size_t count)
{
	uint32_t ticks = uptime_ticks();
	bool counter64;
	int ret;

	ret = validate_notification(notification, len, bindings, count);
	if (ret != 0) {
		return ret;
	}

	/* RFC 3584 section 3.2 step 6: not expressible in version 1. */
	counter64 = bindings_have_counter64(bindings, count);

	k_mutex_lock(&lock, K_FOREVER);

	for (size_t i = 0U; i < target_count; i++) {
		const struct target *t = &targets[i];
		struct snmp_ber_writer w;
		size_t total;
		int err;

		if (t->version == SNMP_VERSION_V1 && counter64) {
			LOG_DBG("Counter64 binding, skipping version 1 receiver");
			continue;
		}

		snmp_ber_writer_init(&w, msg_buf, sizeof(msg_buf));
		total = (t->version == SNMP_VERSION_V1)
				? build_v1(&w, t, notification, len, bindings, count, ticks)
				: build_v2c(&w, t, notification, len, bindings, count, ticks);
		if (total == 0U) {
			LOG_WRN("Notification does not fit the message buffer");
			err = -EMSGSIZE;
		} else {
			err = snmp_transport_send((const struct net_sockaddr *)&t->addr, t->addrlen,
						  msg_buf, total);
		}

		if (err != 0 && ret == 0) {
			ret = err;
		}
	}

	request_id++;
	k_mutex_unlock(&lock);

	return ret;
}

void snmp_notify_cold_start(void)
{
	int ret = snmp_agent_notify(cold_start, ARRAY_SIZE(cold_start), NULL, 0U);

	if (ret != 0) {
		LOG_DBG("coldStart not sent (%d)", ret);
	}
}

void snmp_notify_authentication_failure(void)
{
	int ret;

	if (!snmp_authen_traps_enabled()) {
		return;
	}

	ret = snmp_agent_notify(authentication_failure, ARRAY_SIZE(authentication_failure), NULL,
				0U);
	if (ret != 0) {
		LOG_DBG("authenticationFailure not sent (%d)", ret);
	}
}
