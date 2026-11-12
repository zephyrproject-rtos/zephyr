/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* The snmp group of RFC 3418, 1.3.6.1.2.1.11, and snmpSetSerialNo. The
 * objects of RFC 1213 that RFC 3418 made obsolete are not implemented.
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/net/snmp.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/util.h>

#include "snmp_internal.h"

#define MIB2_SNMP(n)       1, 3, 6, 1, 2, 1, 11, (n)
/* snmpSetSerialNo is { snmpMIBObjects 6 1 } under snmpModules. */
#define SNMP_SET_SERIAL_NO 1, 3, 6, 1, 6, 3, 1, 1, 6, 1

#define AUTHEN_TRAPS_ENABLED  1
#define AUTHEN_TRAPS_DISABLED 2

/* The agent forwards nothing, so snmpProxyDrops never moves. */
static const uint32_t proxy_drops;

static int32_t set_serial_no;
static bool set_serial_no_seeded;

/* RFC 2579: after a re-initialization a TestAndIncr value whose previous
 * value is unknown starts from a pseudo-random value, so that a request
 * prepared before the restart does not pass the check by accident.
 */
static int32_t serial_no(void)
{
	if (!set_serial_no_seeded) {
		set_serial_no = (int32_t)(sys_rand32_get() & INT32_MAX);
		set_serial_no_seeded = true;
	}

	return set_serial_no;
}

/* Getter for the counters; user_data points at the counter. */
static int counter_get(const struct snmp_object *obj, void *row, struct snmp_value *val)
{
	const uint32_t *counter = obj->user_data;

	ARG_UNUSED(row);

	val->type = SNMP_TYPE_COUNTER32;
	val->u32 = *counter;

	return 0;
}

static int authen_traps_get(const struct snmp_object *obj, void *row, struct snmp_value *val)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(row);

	val->type = SNMP_TYPE_INTEGER;
	val->i32 = snmp_authen_traps_enabled() ? AUTHEN_TRAPS_ENABLED : AUTHEN_TRAPS_DISABLED;

	return 0;
}

static int authen_traps_set(const struct snmp_object *obj, void *row, const struct snmp_value *val,
			    enum snmp_set_phase phase)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(row);

	if (val->type != SNMP_TYPE_INTEGER) {
		return -EINVAL;
	}

	if (val->i32 != AUTHEN_TRAPS_ENABLED && val->i32 != AUTHEN_TRAPS_DISABLED) {
		return -ERANGE;
	}

	if (phase == SNMP_SET_COMMIT &&
	    snmp_mib2_snmp_set_authen_traps(val->i32 == AUTHEN_TRAPS_ENABLED) != 0) {
		return -EIO;
	}

	return 0;
}

static int set_serial_no_get(const struct snmp_object *obj, void *row, struct snmp_value *val)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(row);

	val->type = SNMP_TYPE_INTEGER;
	val->i32 = serial_no();

	return 0;
}

/* TestAndIncr of RFC 2579: the value written must equal the current one,
 * which then advances by one and wraps from 2^31-1 to zero.
 */
static int set_serial_no_set(const struct snmp_object *obj, void *row, const struct snmp_value *val,
			     enum snmp_set_phase phase)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(row);

	if (val->type != SNMP_TYPE_INTEGER) {
		return -EINVAL;
	}

	if (val->i32 < 0) {
		return -ERANGE;
	}

	if (val->i32 != serial_no()) {
		return -EBUSY;
	}

	if (phase == SNMP_SET_COMMIT) {
		set_serial_no = (set_serial_no == INT32_MAX) ? 0 : set_serial_no + 1;
	}

	return 0;
}

SNMP_SCALAR_DEFINE(mib2_snmp_in_pkts, counter_get, NULL, &snmp_stats.in_pkts, MIB2_SNMP(1));
SNMP_SCALAR_DEFINE(mib2_snmp_in_bad_versions, counter_get, NULL, &snmp_stats.in_bad_versions,
		   MIB2_SNMP(3));
SNMP_SCALAR_DEFINE(mib2_snmp_in_bad_community_names, counter_get, NULL,
		   &snmp_stats.in_bad_community_names, MIB2_SNMP(4));
SNMP_SCALAR_DEFINE(mib2_snmp_in_bad_community_uses, counter_get, NULL,
		   &snmp_stats.in_bad_community_uses, MIB2_SNMP(5));
SNMP_SCALAR_DEFINE(mib2_snmp_in_asn_parse_errs, counter_get, NULL, &snmp_stats.in_asn_parse_errs,
		   MIB2_SNMP(6));
SNMP_SCALAR_DEFINE(mib2_snmp_enable_authen_traps, authen_traps_get, authen_traps_set, NULL,
		   MIB2_SNMP(30));
SNMP_SCALAR_DEFINE(mib2_snmp_silent_drops, counter_get, NULL, &snmp_stats.silent_drops,
		   MIB2_SNMP(31));
SNMP_SCALAR_DEFINE(mib2_snmp_proxy_drops, counter_get, NULL, (void *)&proxy_drops, MIB2_SNMP(32));
SNMP_SCALAR_DEFINE(mib2_snmp_set_serial_no, set_serial_no_get, set_serial_no_set, NULL,
		   SNMP_SET_SERIAL_NO);
