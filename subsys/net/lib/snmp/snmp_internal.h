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

#endif /* ZEPHYR_SUBSYS_NET_LIB_SNMP_SNMP_INTERNAL_H_ */
