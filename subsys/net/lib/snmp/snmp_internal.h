/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_NET_LIB_SNMP_SNMP_INTERNAL_H_
#define ZEPHYR_SUBSYS_NET_LIB_SNMP_SNMP_INTERNAL_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/net/snmp.h>

/* Outcome of resolving an instance identifier for GET and SET. */
enum snmp_lookup_result {
	/* obj and row identify the instance. */
	SNMP_LOOKUP_FOUND,
	/* No registered object is a prefix of the name. */
	SNMP_LOOKUP_NO_SUCH_OBJECT,
	/* An object matches but has no such instance. */
	SNMP_LOOKUP_NO_SUCH_INSTANCE,
};

/*
 * Resolve an instance identifier. On SNMP_LOOKUP_FOUND, *obj is the entry and
 * *row the row handle, NULL for a scalar. On SNMP_LOOKUP_NO_SUCH_INSTANCE,
 * *obj is the entry whose OID is a prefix of the name; otherwise it is NULL.
 */
enum snmp_lookup_result snmp_registry_lookup(const uint32_t *name, size_t name_len,
					     const struct snmp_object **obj, void **row);

/*
 * Find the lexicographically smallest instance greater than target across all
 * registered objects. Fills inst, *inst_len and *row and returns the entry,
 * or NULL when target is at or past the end of the MIB view.
 */
const struct snmp_object *snmp_registry_next(const uint32_t *target, size_t target_len,
					     uint32_t *inst, size_t *inst_len, void **row);

/* Whether authenticationFailure notifications may be sent, snmpEnableAuthenTraps. */
bool snmp_authen_traps_enabled(void);

/* Values of the version field, RFC 3584 section 5.1. */
#define SNMP_VERSION_1  0
#define SNMP_VERSION_2C 1

/* Error status values of the response PDU, RFC 3416 section 3. */
#define SNMP_ERR_NO_ERROR             0
#define SNMP_ERR_TOO_BIG              1
#define SNMP_ERR_NO_SUCH_NAME         2
#define SNMP_ERR_BAD_VALUE            3
#define SNMP_ERR_READ_ONLY            4
#define SNMP_ERR_GEN_ERR              5
#define SNMP_ERR_NO_ACCESS            6
#define SNMP_ERR_WRONG_TYPE           7
#define SNMP_ERR_WRONG_LENGTH         8
#define SNMP_ERR_WRONG_ENCODING       9
#define SNMP_ERR_WRONG_VALUE          10
#define SNMP_ERR_NO_CREATION          11
#define SNMP_ERR_INCONSISTENT_VALUE   12
#define SNMP_ERR_RESOURCE_UNAVAILABLE 13
#define SNMP_ERR_COMMIT_FAILED        14
#define SNMP_ERR_UNDO_FAILED          15
#define SNMP_ERR_AUTHORIZATION_ERROR  16
#define SNMP_ERR_NOT_WRITABLE         17
#define SNMP_ERR_INCONSISTENT_NAME    18

struct snmp_ber_writer;

/*
 * Size of one encoded variable binding, header included, or 0 when the name
 * or the value cannot be encoded.
 */
size_t snmp_varbind_size(const uint32_t *name, size_t name_len, const struct snmp_value *val);

/*
 * Append one variable binding. With a non-zero exception tag the value is
 * the SNMPv2c exception instead of val. Nothing is written on error: -EINVAL
 * when the name or the value cannot be encoded, -EMSGSIZE when it does not
 * fit.
 */
int snmp_write_varbind(struct snmp_ber_writer *w, const uint32_t *name, size_t name_len,
		       const struct snmp_value *val, uint8_t exception);

/*
 * Process one received message and build the response in out. Returns the
 * length of the response, which starts at out[*out_start], or 0 when the
 * message is dropped: malformed, unsupported version, unknown community or
 * a response that cannot fit.
 */
size_t snmp_process_message(const uint8_t *in, size_t in_len, uint8_t *out, size_t out_size,
			    size_t *out_start);

/*
 * Kernel uptime in milliseconds at the last snmp_agent_start(), zero before
 * the first: the re-initialization epoch that sysUpTime counts from.
 */
int64_t snmp_agent_start_uptime(void);

/*
 * Counters of the snmp group of RFC 3418, maintained by the message
 * processing and read by the MIB objects, both from the agent's context.
 */
struct snmp_counters {
	uint32_t in_pkts;
	uint32_t in_bad_versions;
	uint32_t in_bad_community_names;
	uint32_t in_bad_community_uses;
	uint32_t in_asn_parse_errs;
	uint32_t silent_drops;
};

extern struct snmp_counters snmp_stats;

/*
 * Send a datagram from the agent's socket of the destination's family.
 * Returns 0, -ENOTCONN when the agent is not running, -EAFNOSUPPORT for a
 * family without a socket, or the negative errno of the send.
 */
int snmp_transport_send(const struct net_sockaddr *dst, net_socklen_t dst_len, const void *data,
			size_t len);

/* Notifications the agent itself originates; no-ops without receivers. */
void snmp_notify_cold_start(void);
void snmp_notify_authentication_failure(void);

#endif /* ZEPHYR_SUBSYS_NET_LIB_SNMP_SNMP_INTERNAL_H_ */
