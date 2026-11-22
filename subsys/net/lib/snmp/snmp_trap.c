/*
 * Copyright (c) 2026 StructCore Embedded
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/snmp/snmp_trap.h>
#include <zephyr/net/snmp/snmp_mib.h>
#include <zephyr/net/snmp/snmp_core.h>
#include "snmp_internal.h"
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if (defined(CONFIG_SNMP) || defined(CONFIG_SNMP_AGENT)) && defined(CONFIG_SNMP_TRAP_ENABLED)

#ifdef CONFIG_SNMP_VERSION_3
#include <zephyr/net/snmp/snmp_usm.h>
#endif

#ifndef CONFIG_SNMP_TRAP_PORT
#define CONFIG_SNMP_TRAP_PORT 162
#endif

#if defined(CONFIG_NET_IPV4)
static bool snmp_trap_iface_ipv4(struct net_if *iface, struct net_in_addr *addr)
{
	if (!iface || !net_if_is_up(iface) || !iface->config.ip.ipv4) {
		return false;
	}

	for (int i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		if (iface->config.ip.ipv4->unicast[i].ipv4.is_used &&
		    iface->config.ip.ipv4->unicast[i].ipv4.address.in_addr.s_addr != 0) {
			if (addr) {
				*addr = iface->config.ip.ipv4->unicast[i].ipv4.address.in_addr;
			}
			return true;
		}
	}

	return false;
}
#endif

/*
 * Is the v3 notification builder reachable from snmp_trap_send()?
 *
 * That function picks the wire format with an #if/#elif ladder in which the explicit
 * SNMP_TRAP_FORMAT_V1/V2C choices and the SNMP_VERSION_2C fallback are tested before
 * the SNMP_VERSION_3 branch, so enabling v3 while selecting a v2c trap format (the
 * Kconfig default whenever v2c is enabled) leaves the builder unreferenced. Compiling
 * it anyway trips -Wunused-function, so it is compiled exactly when the ladder can
 * reach it. This mirrors the ladder below; keep the two in sync.
 */
#if defined(CONFIG_SNMP_VERSION_3)
#if !defined(CONFIG_SNMP_TRAP_FORMAT_V1) && !defined(CONFIG_SNMP_TRAP_FORMAT_V2C) &&               \
	!defined(CONFIG_SNMP_VERSION_2C)
#define SNMP_TRAP_V3_REACHED 1
#endif
#if defined(CONFIG_SNMP_TRAP_FORMAT_V3) || defined(SNMP_TRAP_V3_REACHED)
#define SNMP_TRAP_HAVE_V3 1
#endif
#endif

int snmp_trap_add_destination_port(struct snmp_agent *inst, const char *addr, uint16_t port)
{
	if (!inst || !addr) {
		return -EINVAL;
	}

	char ip_buf[INET_ADDRSTRLEN + 8];
	const char *ip_start = addr;
	size_t ip_len = 0;
	const char *port_str = NULL;
	uint16_t target_port = port;

	if (*addr == '[') {
		const char *close_bracket = strchr(addr, ']');

		if (!close_bracket) {
			return -EINVAL;
		}
		ip_start = addr + 1;
		ip_len = (size_t)(close_bracket - ip_start);
		if (close_bracket[1] == ':') {
			port_str = close_bracket + 2;
		} else if (close_bracket[1] != '\0') {
			return -EINVAL;
		}
	} else {
		const char *colon = strchr(addr, ':');

		if (colon) {
			if (strchr(colon + 1, ':') != NULL) {
				return -EINVAL;
			}
			ip_start = addr;
			ip_len = (size_t)(colon - ip_start);
			port_str = colon + 1;
		} else {
			ip_start = addr;
			ip_len = strlen(addr);
		}
	}

	if (ip_len == 0 || ip_len >= sizeof(ip_buf)) {
		return -EINVAL;
	}

	memcpy(ip_buf, ip_start, ip_len);
	ip_buf[ip_len] = '\0';

	if (port_str) {
		char *endptr = NULL;
		unsigned long parsed_p = strtoul(port_str, &endptr, 10);

		if (endptr == port_str || *endptr != '\0' || parsed_p == 0 || parsed_p > 65535) {
			return -EINVAL;
		}
		if (port != 0 && port != (uint16_t)parsed_p) {
			return -EINVAL;
		}
		target_port = (uint16_t)parsed_p;
	} else if (target_port == 0) {
		target_port = CONFIG_SNMP_TRAP_PORT;
	}

	if (target_port == 0) {
		return -EINVAL;
	}

	struct sockaddr_in dest;

	memset(&dest, 0, sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_port = htons(target_port);

	int ret = zsock_inet_pton(AF_INET, ip_buf, &dest.sin_addr);

	if (ret <= 0) {
		return -EINVAL;
	}

	k_mutex_lock(&inst->lock, K_FOREVER);
	if (inst->trap_dest_cnt >= CONFIG_SNMP_TRAP_DEST_COUNT) {
		k_mutex_unlock(&inst->lock);
		return -ENOMEM;
	}

	inst->trap_dests[inst->trap_dest_cnt] = dest;
	inst->trap_dest_cnt++;
	k_mutex_unlock(&inst->lock);

	return 0;
}

int snmp_trap_add_destination(struct snmp_agent *inst, const char *addr)
{
	return snmp_trap_add_destination_port(inst, addr, 0);
}

int snmp_trap_clear_destinations(struct snmp_agent *inst)
{
	if (!inst) {
		return -EINVAL;
	}

	k_mutex_lock(&inst->lock, K_FOREVER);
	inst->trap_dest_cnt = 0;
	memset(inst->trap_dests, 0, sizeof(inst->trap_dests));
	k_mutex_unlock(&inst->lock);

	return 0;
}

#if defined(SNMP_TRAP_HAVE_V3)
static const struct snmp_usm_user *trap_find_user(const struct snmp_agent *inst, const char *name)
{
	for (size_t i = 0; i < CONFIG_SNMP_V3_MAX_USERS; i++) {
		if (inst->users[i].in_use && strcmp(inst->users[i].user_name, name) == 0) {
			return &inst->users[i];
		}
	}
	return NULL;
}

static int snmp_trap_send_v3(struct snmp_agent *inst, const struct snmp_oid *trap_oid,
			     const struct snmp_varbind *varbinds, uint8_t varbind_cnt,
			     bool is_inform)
{
	struct snmp_v3_msg msg;
	size_t tx_len = 0;
	int sock;
	int err;
	int ret;

	if (inst == NULL || trap_oid == NULL) {
		return -EINVAL;
	}

	/* Two mandatory varbinds (sysUpTime.0, snmpTrapOID.0) plus the caller's. */
	if (varbind_cnt > CONFIG_SNMP_MAX_VARBINDS - 2) {
		return -E2BIG;
	}

#if defined(CONFIG_NET_IPV4)
	struct net_if *iface = inst->bound_iface ? inst->bound_iface : net_if_get_default();
	if (!snmp_trap_iface_ipv4(iface, NULL)) {

		return -ENETDOWN;
	}
#endif

	/*
	 * Uniform locking (contract 9.1): the whole notification build reads shared
	 * instance state (the security name of the first USM user, the engine ID, the
	 * destination list) and then uses the instance tx_buf, so it runs with
	 * inst->lock held, exactly like the v1/v2c path below. Zephyr's k_mutex is
	 * recursive for its owner (kernel/mutex.c, lock_count), so an authFailure trap
	 * raised from inside snmp_agent_process_pdu() - which the service callback
	 * already calls under this same lock - cannot self-deadlock.
	 */
	k_mutex_lock(&inst->lock, K_FOREVER);

	/* A notification without a destination is a configuration error, not a broadcast:
	 * blasting traps to 255.255.255.255 leaks device identity to the whole segment.
	 */
	if (inst->trap_dest_cnt == 0) {
		k_mutex_unlock(&inst->lock);
		return -EDESTADDRREQ;
	}

	memset(&msg, 0, sizeof(msg));

	msg.global.msg_id = (int32_t)k_uptime_get_32();
	msg.global.msg_max_size = CONFIG_SNMP_MAX_PDU_SIZE;
	msg.global.msg_security_model = 3;

	/*
	 * Select the notification user explicitly. Using users[0] silently changed the
	 * sender's identity whenever the table order changed; Kconfig lets the app name
	 * the account that is allowed to notify.
	 */
#ifdef CONFIG_SNMP_TRAP_V3_USER
	const char *user_name = CONFIG_SNMP_TRAP_V3_USER;
#else
	const char *user_name = "";
#endif

	if (user_name[0] == '\0') {
		user_name =
			inst->user_cnt > 0 ? inst->users[0].user_name : CONFIG_SNMP_V3_USER_NAME;
	}

	size_t u_len = snmp_strnlen(user_name, sizeof(msg.usm.user_name));
	if (u_len == 0 || u_len >= sizeof(msg.usm.user_name)) {

		k_mutex_unlock(&inst->lock);
		return -EACCES;
	}
	memcpy(msg.usm.user_name, user_name, u_len);
	msg.usm.user_name_len = u_len;

	const struct snmp_usm_user *user = trap_find_user(inst, user_name);
	if (user == NULL) {

		k_mutex_unlock(&inst->lock);
		return -EACCES;
	}

	/* Traps and Informs use the strongest level this user supports (RFC 3414 §3.4). */
	if (user->priv_protocol != SNMP_USM_PRIV_NONE) {
		msg.sec_level = SNMP_SEC_LEVEL_AUTH_PRIV;
	} else if (user->auth_protocol != SNMP_USM_AUTH_NONE) {
		msg.sec_level = SNMP_SEC_LEVEL_AUTH_NOPRIV;
	} else {
		msg.sec_level = SNMP_SEC_LEVEL_NOAUTH_NOPRIV;
	}

	msg.scoped.pdu.version = 3;
	msg.scoped.pdu.type = is_inform ? SNMP_PDU_INFORM_REQ : SNMP_PDU_TRAP_V2;
	msg.scoped.pdu.request_id = (int32_t)k_uptime_get_32();
	msg.scoped.pdu.error_status = 0;
	msg.scoped.pdu.error_index = 0;

	static const uint32_t uptime_oid[9] = {1, 3, 6, 1, 2, 1, 1, 3, 0};
	static const uint32_t trap_oid_spec[11] = {1, 3, 6, 1, 6, 3, 1, 1, 4, 1, 0};

	msg.scoped.pdu.varbinds[0].oid.len = 9;
	memcpy(msg.scoped.pdu.varbinds[0].oid.ids, uptime_oid, sizeof(uptime_oid));
	msg.scoped.pdu.varbinds[0].type = SNMP_TAG_TIMETICKS;
	msg.scoped.pdu.varbinds[0].val.uint_val = (uint32_t)(k_uptime_get() / 10);

	msg.scoped.pdu.varbinds[1].oid.len = 11;
	memcpy(msg.scoped.pdu.varbinds[1].oid.ids, trap_oid_spec, sizeof(trap_oid_spec));
	msg.scoped.pdu.varbinds[1].type = ASN1_TAG_OID;
	msg.scoped.pdu.varbinds[1].val.oid = *trap_oid;

	msg.scoped.pdu.varbind_cnt = 2;

	if (varbinds && varbind_cnt > 0) {
		for (uint8_t i = 0;
		     i < varbind_cnt && msg.scoped.pdu.varbind_cnt < CONFIG_SNMP_MAX_VARBINDS;
		     i++) {
			msg.scoped.pdu.varbinds[msg.scoped.pdu.varbind_cnt++] = varbinds[i];
		}
	}

	uint8_t eng_len = sizeof(msg.scoped.context_engine_id);
	if (snmp_usm_get_engine_id(inst, msg.scoped.context_engine_id, &eng_len) == 0) {

		msg.scoped.context_engine_id_len = eng_len;
	}
	msg.scoped.context_name_len = 0;

	ret = snmp_usm_build_outgoing(inst, &msg);
	if (ret < 0) {
		k_mutex_unlock(&inst->lock);
		return ret;
	}

	/*
	 * Encode into the instance's own tx_buf instead of the previous second
	 * CONFIG_SNMP_MAX_PDU_SIZE stack buffer: this path already needs several KiB of
	 * stack for struct snmp_v3_msg, so dropping the extra 1 KiB matters for
	 * CONFIG_NET_SOCKETS_SERVICE_STACK_SIZE / main-stack sizing (contract 8.3).
	 */
	ret = snmp_ber_encode_v3_msg(inst->tx_buf, sizeof(inst->tx_buf), &tx_len, &msg);
	if (ret < 0) {
		k_mutex_unlock(&inst->lock);
		return ret;
	}

	sock = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		err = -errno;
		k_mutex_unlock(&inst->lock);
		return err;
	}

	/* Multi-homed egress: leave through the interface this agent is bound to. */
	ret = snmp_sock_bind_to_iface(sock, inst->bound_iface);
	if (ret < 0) {
		zsock_close(sock);
		k_mutex_unlock(&inst->lock);
		return ret;
	}

	for (uint8_t d = 0; d < inst->trap_dest_cnt; d++) {
		(void)zsock_sendto(sock, inst->tx_buf, tx_len, 0,
				   (struct sockaddr *)&inst->trap_dests[d],
				   sizeof(inst->trap_dests[d]));
	}

	/* One PDU sent = one increment, on the sending agent (contract 6.2). */
	snmp_counter_inc(&inst->counters.snmpOutTraps);
	zsock_close(sock);
	k_mutex_unlock(&inst->lock);
	return 0;
}
#endif /* SNMP_TRAP_HAVE_V3 */

/*
 * Which wire format does the ladder below actually build? Kept as one macro so the
 * v1 helper is compiled exactly when it is used (an unused static function is a
 * -Wunused-function warning, and Zephyr sample builds treat that as noise we do not
 * want). The ladder order itself is unchanged.
 */
#if defined(CONFIG_SNMP_TRAP_FORMAT_V1)
#define SNMP_TRAP_IS_V1 1
#elif defined(CONFIG_SNMP_TRAP_FORMAT_V2C) || defined(CONFIG_SNMP_TRAP_FORMAT_V3) ||               \
	defined(CONFIG_SNMP_VERSION_2C)
#define SNMP_TRAP_IS_V1 0
#elif defined(CONFIG_SNMP_VERSION_1)
#define SNMP_TRAP_IS_V1 1
#else
#define SNMP_TRAP_IS_V1 0
#endif

int snmp_trap_send(struct snmp_agent *inst, const struct snmp_oid *trap_oid,
		   const struct snmp_varbind *varbinds, uint8_t varbind_cnt, bool is_inform)
{
	if (inst == NULL || trap_oid == NULL) {
		return -EINVAL;
	}

	if (varbind_cnt > CONFIG_SNMP_MAX_VARBINDS - 2) {
		return -E2BIG;
	}

#if defined(CONFIG_SNMP_TRAP_FORMAT_V3)
	return snmp_trap_send_v3(inst, trap_oid, varbinds, varbind_cnt, is_inform);
#elif defined(CONFIG_SNMP_TRAP_FORMAT_V1)
	/* Proceed with SNMPv1 trap */
#elif defined(CONFIG_SNMP_TRAP_FORMAT_V2C)
	/* Proceed with SNMPv2c trap */
#elif defined(CONFIG_SNMP_VERSION_2C)
	/* Fallback: proceed with SNMPv2c trap */
#elif defined(CONFIG_SNMP_VERSION_3)
	return snmp_trap_send_v3(inst, trap_oid, varbinds, varbind_cnt, is_inform);
#elif defined(CONFIG_SNMP_VERSION_1)
	/* Fallback: proceed with SNMPv1 trap */
#else
	return -ENOTSUP;
#endif

#if defined(CONFIG_NET_IPV4)
	struct net_if *iface = inst->bound_iface ? inst->bound_iface : net_if_get_default();
	if (!snmp_trap_iface_ipv4(iface, NULL)) {

		return -ENETDOWN;
	}
#endif

	k_mutex_lock(&inst->lock, K_FOREVER);

	/* Never fall back to a broadcast: an unconfigured destination is an error. */
	if (inst->trap_dest_cnt == 0) {
		k_mutex_unlock(&inst->lock);
		return -EDESTADDRREQ;
	}

	int sock = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {

		k_mutex_unlock(&inst->lock);
		return -errno;
	}

	/* Multi-homed egress: leave through the interface this agent is bound to. */
	int bind_ret = snmp_sock_bind_to_iface(sock, inst->bound_iface);
	if (bind_ret < 0) {

		zsock_close(sock);
		k_mutex_unlock(&inst->lock);
		return bind_ret;
	}

	memset(&inst->current_pdu, 0, sizeof(inst->current_pdu));

	const char *comm = (inst->read_community[0] != '\0') ? inst->read_community :
#ifdef CONFIG_SNMP_COMMUNITY_READ
							     CONFIG_SNMP_COMMUNITY_READ;
#else
							     "public";
#endif

#if SNMP_TRAP_IS_V1
	inst->current_pdu.version = 0;
	inst->current_pdu.type = SNMP_PDU_TRAP_V1;
	/* generic-trap / enterprise / specific-trap from snmpTrapOID (RFC 3584 §3.2). */
	snmp_v1_trap_parameters(&inst->current_pdu, trap_oid);
	inst->current_pdu.timestamp = (uint32_t)(k_uptime_get() / 10);

#if defined(CONFIG_NET_IPV4)
	struct net_if *iface_v1 = inst->bound_iface ? inst->bound_iface : net_if_get_default();
	struct net_in_addr agent_addr;
	if (snmp_trap_iface_ipv4(iface_v1, &agent_addr)) {

		memcpy(inst->current_pdu.agent_addr, &agent_addr, sizeof(agent_addr));
	}
#endif
	strncpy((char *)inst->current_pdu.community, comm, sizeof(inst->current_pdu.community) - 1);
	inst->current_pdu.community_len = strlen((char *)inst->current_pdu.community);
#else
	inst->current_pdu.version = 1;
	inst->current_pdu.type = is_inform ? SNMP_PDU_INFORM_REQ : SNMP_PDU_TRAP_V2;
	inst->current_pdu.request_id = (int32_t)k_uptime_get_32();
	inst->current_pdu.error_status = 0;
	inst->current_pdu.error_index = 0;
	strncpy((char *)inst->current_pdu.community, comm, sizeof(inst->current_pdu.community) - 1);
	inst->current_pdu.community_len = strlen((char *)inst->current_pdu.community);

	/* sysUpTime.0 (1.3.6.1.2.1.1.3.0) and snmpTrapOID.0 (1.3.6.1.6.3.1.1.4.1.0) */
	static const uint32_t uptime_oid[9] = {1, 3, 6, 1, 2, 1, 1, 3, 0};
	static const uint32_t trap_oid_spec[11] = {1, 3, 6, 1, 6, 3, 1, 1, 4, 1, 0};

	inst->current_pdu.varbinds[0].oid.len = 9;
	memcpy(inst->current_pdu.varbinds[0].oid.ids, uptime_oid, sizeof(uptime_oid));
	inst->current_pdu.varbinds[0].type = SNMP_TAG_TIMETICKS;
	inst->current_pdu.varbinds[0].val.uint_val = (uint32_t)(k_uptime_get() / 10);

	inst->current_pdu.varbinds[1].oid.len = 11;
	memcpy(inst->current_pdu.varbinds[1].oid.ids, trap_oid_spec, sizeof(trap_oid_spec));
	inst->current_pdu.varbinds[1].type = ASN1_TAG_OID;
	inst->current_pdu.varbinds[1].val.oid = *trap_oid;

	inst->current_pdu.varbind_cnt = 2;
#endif

	if (varbinds && varbind_cnt > 0) {
		for (uint8_t i = 0;
		     i < varbind_cnt && inst->current_pdu.varbind_cnt < CONFIG_SNMP_MAX_VARBINDS;
		     i++) {
			inst->current_pdu.varbinds[inst->current_pdu.varbind_cnt++] = varbinds[i];
		}
	}

	size_t tx_len = 0;
	int ret = snmp_ber_encode_pdu(inst->tx_buf, sizeof(inst->tx_buf), &tx_len,
				      &inst->current_pdu);
	if (ret < 0) {
		zsock_close(sock);
		k_mutex_unlock(&inst->lock);
		return ret;
	}

	/* Send only to explicitly configured destinations (checked at entry). */
	for (uint8_t d = 0; d < inst->trap_dest_cnt; d++) {
		zsock_sendto(sock, inst->tx_buf, tx_len, 0, (struct sockaddr *)&inst->trap_dests[d],
			     sizeof(inst->trap_dests[d]));
	}

	/* One PDU sent = one increment, on the sending agent (contract 6.2). */
	snmp_counter_inc(&inst->counters.snmpOutTraps);
	zsock_close(sock);
	k_mutex_unlock(&inst->lock);
	return 0;
}

int snmp_trap_send_coldstart(struct snmp_agent *inst)
{
	static const struct snmp_oid cold_start_oid = {.len = 10,
						       .ids = {1, 3, 6, 1, 6, 3, 1, 1, 5, 1}};
	return snmp_trap_send(inst, &cold_start_oid, NULL, 0, false);
}

int snmp_trap_send_auth_failure(struct snmp_agent *inst)
{
	if (!inst) {
		return -EINVAL;
	}
	/* Gate on the receiving agent only: the global mirror is gone (contract 6.2). */
	if (inst->counters.snmpEnableAuthenTraps != 1) {
		return 0; /* Traps disabled for this agent */
	}
	static const struct snmp_oid auth_fail_oid = {.len = 10,
						      .ids = {1, 3, 6, 1, 6, 3, 1, 1, 5, 5}};
	return snmp_trap_send(inst, &auth_fail_oid, NULL, 0, false);
}

#endif /* CONFIG_SNMP_AGENT && CONFIG_SNMP_TRAP_ENABLED */
