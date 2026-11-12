/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_NET_LIB_SNMP_SNMP_INTERNAL_H_
#define ZEPHYR_SUBSYS_NET_LIB_SNMP_SNMP_INTERNAL_H_

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

/*
 * Process one received message and build the response in out. Returns the
 * length of the response, which starts at out[*out_start], or 0 when the
 * message is dropped: malformed, unsupported version, unknown community or
 * a response that cannot fit.
 */
size_t snmp_process_message(const uint8_t *in, size_t in_len, uint8_t *out, size_t out_size,
			    size_t *out_start);

#endif /* ZEPHYR_SUBSYS_NET_LIB_SNMP_SNMP_INTERNAL_H_ */
