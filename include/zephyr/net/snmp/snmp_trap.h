/*
 * Copyright (c) 2026 StructCore Embedded
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_SNMP_SNMP_TRAP_H_
#define ZEPHYR_INCLUDE_NET_SNMP_SNMP_TRAP_H_

#include <zephyr/kernel.h>
#include <zephyr/net/snmp/snmp_ber.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup snmp_trap Trap & Inform Notification Engine
 * @ingroup snmp
 * @{
 * @brief SNMPv1 trap and SNMPv2c/v3 notification generation and delivery.
 */

#ifndef CONFIG_SNMP_TRAP_DEST_COUNT
#define CONFIG_SNMP_TRAP_DEST_COUNT 4
#endif

/* Forward declaration of agent instance */
struct snmp_agent;

/* ---- Instance-based APIs ---- */

/**
 * @brief Add a destination IPv4 address and custom port for trap notifications on one agent.
 *
 * @param inst Target agent instance.
 * @param addr Dotted-quad destination IPv4 address (supports optional ":port" or "[ip]:port"
 * syntax).
 * @param port Destination UDP port (1..65535). If 0 and no port in @p addr, CONFIG_SNMP_TRAP_PORT
 * is used.
 * @return 0 on success, -EINVAL on bad arguments/port format, -ENOMEM if the destination table
 *         (CONFIG_SNMP_TRAP_DEST_COUNT) is full.
 */
int snmp_trap_add_destination_port(struct snmp_agent *inst, const char *addr, uint16_t port);

/**
 * @brief Add a destination IPv4 address for trap notifications on one agent.
 *
 * Destination port defaults to CONFIG_SNMP_TRAP_PORT, or can be specified inline
 * in @p addr (e.g. "192.168.1.100:1162").
 *
 * @param inst Target agent instance.
 * @param addr Dotted-quad destination (e.g. "192.168.1.100" or "192.168.1.100:1162").
 * @return 0 on success, -EINVAL on bad arguments, -ENOMEM if the destination table
 *         (CONFIG_SNMP_TRAP_DEST_COUNT) is full.
 */
int snmp_trap_add_destination(struct snmp_agent *inst, const char *addr);

/**
 * @brief Clear all configured trap destinations on one agent.
 *
 * @param inst Target agent instance.
 * @return 0 on success, -EINVAL on bad arguments.
 */
int snmp_trap_clear_destinations(struct snmp_agent *inst);

/**
 * @brief Send an SNMP Trap or Inform notification from one agent.
 *
 * The wire format is selected by Kconfig (v1 / v2c / v3). Thread context required:
 * takes the agent lock, so never call from an ISR and never from a MIB callback.
 *
 * @param inst Target agent instance.
 * @param trap_oid Notification OID (becomes snmpTrapOID.0).
 * @param varbinds Optional extra varbinds (may be NULL).
 * @param varbind_cnt Number of entries in @p varbinds.
 * @param is_inform True to send an Inform request instead of a Trap.
 * @return 0 on success, -EINVAL on bad arguments, -ENETDOWN when the bound interface
 *         has no address, other negative errno on socket failure.
 */
int snmp_trap_send(struct snmp_agent *inst, const struct snmp_oid *trap_oid,
		   const struct snmp_varbind *varbinds, uint8_t varbind_cnt, bool is_inform);

/**
 * @brief Send the standard coldStart (1.3.6.1.6.3.1.1.5.1) notification from one agent.
 * @param inst Target agent instance.
 * @return 0 on success, negative errno as for snmp_trap_send().
 */
int snmp_trap_send_coldstart(struct snmp_agent *inst);

/**
 * @brief Send the standard authFailure (1.3.6.1.6.3.1.1.5.5) notification from one agent.
 *
 * Silently returns 0 when the instance's snmpEnableAuthenTraps counter is not enabled
 * (RFC 3418).
 *
 * @param inst Target agent instance.
 * @return 0 when sent or suppressed, negative errno on socket failure.
 */
int snmp_trap_send_auth_failure(struct snmp_agent *inst);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_SNMP_SNMP_TRAP_H_ */
