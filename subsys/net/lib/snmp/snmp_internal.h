/*
 * Copyright (c) 2026 StructCore Embedded
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module-internal declarations for the SNMP agent.
 *
 * NOT INSTALLED: this header lives under src/ and is never part of the public API
 * surface (include/snmp/). It exists for symbols that must cross translation units
 * and therefore cannot be static, yet must not be reachable by applications.
 */

#ifndef ZEPHYR_SUBSYS_NET_LIB_SNMP_INTERNAL_H_
#define ZEPHYR_SUBSYS_NET_LIB_SNMP_INTERNAL_H_

#include <zephyr/net/snmp/snmp_core.h>
#include <string.h>

#if defined(CONFIG_SNMP) || defined(CONFIG_SNMP_AGENT)

/*
 * Bounded string length. C11 strnlen() is not declared by every libc configuration
 * Zephyr builds with (native_sim's host libc only exposes it behind POSIX feature
 * macros, and -Werror turns the implicit declaration into a build failure), so the
 * module carries its own trivial implementation instead of depending on feature
 * test macros.
 *
 * @param s   NUL-terminated (or at least @p max bytes long) buffer.
 * @param max Maximum number of bytes to examine.
 * @return Number of bytes before the first NUL, or @p max when there is none.
 */
static inline size_t snmp_strnlen(const char *s, size_t max)
{
	const char *nul = memchr(s, '\0', max);

	return nul != NULL ? (size_t)(nul - s) : max;
}

/*
 * SNMPv1 notification parameters (RFC 3584 section 3.2, originally RFC 2576).
 *
 * Notifications are produced in the v2 form, i.e. as a single snmpTrapOID value, so the
 * Trap-PDU fields have to be derived from it:
 *
 *  - one of the six standard traps: generic-trap 0..5, enterprise = snmpTraps
 *    (1.3.6.1.6.3.1.1.5), specific-trap 0;
 *  - anything else: generic-trap 6 (enterpriseSpecific), specific-trap = last
 *    sub-identifier of snmpTrapOID, and enterprise = snmpTrapOID with the last
 *    sub-identifier removed - or with the last two removed when the next-to-last
 *    sub-identifier is 0, which is the "<enterprise>.0.<specific>" form an SMIv1
 *    TRAP-TYPE takes once converted to SMIv2.
 *
 * The result is invertible on purpose: a receiver reconstructs the v2 notification OID
 * by appending specific-trap to enterprise. Keeping it in one pure function also lets
 * the host unit tests cover the mapping without a socket.
 *
 * @param pdu      Trap-PDU under construction; generic_trap, enterprise and
 *                 specific_trap are written.
 * @param trap_oid Notification OID from the v2/v3 side.
 */
static inline void snmp_v1_trap_parameters(struct snmp_pdu *pdu, const struct snmp_oid *trap_oid)
{
	static const struct snmp_oid snmp_traps = {.len = 9, .ids = {1, 3, 6, 1, 6, 3, 1, 1, 5}};
	size_t ent_len;

	if (pdu == NULL || trap_oid == NULL) {
		return;
	}

	if (trap_oid->len == 10 && trap_oid->ids[0] == 1 && trap_oid->ids[1] == 3 &&
	    trap_oid->ids[2] == 6 && trap_oid->ids[3] == 1 && trap_oid->ids[4] == 6 &&
	    trap_oid->ids[5] == 3 && trap_oid->ids[6] == 1 && trap_oid->ids[7] == 1 &&
	    trap_oid->ids[8] == 5 && trap_oid->ids[9] >= 1 && trap_oid->ids[9] <= 6) {
		pdu->generic_trap = (int32_t)trap_oid->ids[9] - 1;
		pdu->enterprise = snmp_traps;
		pdu->specific_trap = 0;
		return;
	}

	pdu->generic_trap = SNMP_TRAP_ENTERPRISESPEC;

	if (trap_oid->len < 2) {
		pdu->enterprise = *trap_oid;
		pdu->specific_trap = 0;
		return;
	}

	pdu->specific_trap = (int32_t)trap_oid->ids[trap_oid->len - 1];
	pdu->enterprise = *trap_oid;

	ent_len = trap_oid->ids[trap_oid->len - 2] == 0 ? (size_t)trap_oid->len - 2
							: (size_t)trap_oid->len - 1;

	/* A Trap-PDU enterprise is an OBJECT IDENTIFIER, so it needs at least two arcs. */
	pdu->enterprise.len = ent_len >= 2 ? ent_len : trap_oid->len;
}

#if defined(CONFIG_SNMP_VERSION_3)
/**
 * @brief USM incoming-message check for one agent (RFC 3414 section 2.2): engine
 *        discovery, timeliness, key-localization-aware HMAC verification and, when the
 *        privacy flag is set, AES-128-CFB decryption of the scopedPDU.
 *
 * Cross-TU caller: src/snmp_core.c. Increments the receiving agent's counters only.
 *
 * @param inst    Target agent instance (owner of the local keys table).
 * @param msg     Decoded incoming message; filled in with the plaintext scopedPDU.
 * @param raw_buf Raw datagram (needed to locate the ciphertext).
 * @param raw_len Length of @p raw_buf.
 * @return 0 to continue processing, -EACCES to drop silently, other negative errno on
 *         parse failure.
 */
int snmp_usm_verify_incoming(struct snmp_agent *inst, struct snmp_v3_msg *msg,
			     const uint8_t *raw_buf, size_t raw_len);

/**
 * @brief USM outgoing-message preparation for one agent: fills the security parameters,
 *        encrypts the scopedPDU when privacy is in use and stamps the msgAuthentication
 *        Parameters (RFC 3414 section 7).
 *
 * Cross-TU callers: src/snmp_core.c, src/snmp_trap.c.
 *
 * @param inst Target agent instance.
 * @param msg  Message to secure in place.
 * @return 0 on success, negative errno otherwise.
 */
int snmp_usm_build_outgoing(struct snmp_agent *inst, struct snmp_v3_msg *msg);
#endif /* CONFIG_SNMP_VERSION_3 */

/* --------------------------------------------------------------------------
 * Transport adapter (src/snmp_service.c)
 *
 * Driven only by snmp_agent_start()/snmp_agent_stop() in snmp_core.c. The adapter
 * reserves one pre-allocated socket-service descriptor slot per RUNNING instance,
 * binds that agent's sockets to agent->port / agent->bound_iface and registers them
 * with the agent as user_data, so the service callback recovers its owner from the
 * event copy instead of scanning descriptors.
 * ------------------------------------------------------------------------ */

/**
 * @brief Reserve a slot, create and bind the agent's sockets and register them.
 * @param agent Instance to attach (svc_slot, sock_v4/sock_v6 are written).
 * @return 0 on success, -EBUSY if all slots are taken, -EADDRINUSE/-ENETDOWN or
 *         a socket errno otherwise.
 */
int snmp_service_attach(struct snmp_agent *agent);

/**
 * @brief Tear the transport down with the documented quiesce ordering.
 *
 * Deregister the descriptor, then take agent->lock as the barrier that drains any
 * in-flight callback, then close the sockets, release the slot and publish
 * SNMP_AGENT_STOPPED.
 *
 * @param agent Instance to detach.
 * @return 0 on success, -EINVAL if @p agent is NULL.
 */
int snmp_service_detach(struct snmp_agent *agent);

/* --------------------------------------------------------------------------
 * Socket helpers (implemented by the adapter, shared with the trap engine)
 * ------------------------------------------------------------------------ */

/**
 * @brief Restrict a socket to an interface with SO_BINDTODEVICE (struct net_ifreq).
 *
 * Used by both the RX listener and trap egress so multi-homed notifications leave on
 * the same interface the agent listens on (contract 4.5).
 *
 * @param sock Socket file descriptor.
 * @param iface Interface to bind to, may be NULL (then a no-op returning 0).
 * @return 0 on success, negative errno otherwise (-ENOTSUP on configs without
 *         SO_BINDTODEVICE, where callers fall back to address binding).
 */
int snmp_sock_bind_to_iface(int sock, struct net_if *iface);

#endif /* CONFIG_SNMP_AGENT */

#endif /* ZEPHYR_SUBSYS_NET_LIB_SNMP_INTERNAL_H_ */
