/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* The system group, 1.3.6.1.2.1.1: the seven scalars of RFC 1213 plus
 * sysORLastChange and sysORTable of RFC 3418. The table lists the
 * compliance statements this build satisfies, all present from the start,
 * so sysORLastChange stays at zero.
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/snmp.h>
#include <zephyr/sys/util.h>
#include <zephyr/version.h>

#include "snmp_internal.h"

#define MIB2_SYSTEM(n)       1, 3, 6, 1, 2, 1, 1, (n)
#define MIB2_SYS_OR_ENTRY(n) MIB2_SYSTEM(9), 1, (n)

#define SYS_DESCR_DEFAULT "Zephyr " KERNEL_VERSION_STRING " " CONFIG_BOARD
#define SYS_DESCR ((sizeof(CONFIG_SNMP_SYS_DESCR) > 1U) ? CONFIG_SNMP_SYS_DESCR : SYS_DESCR_DEFAULT)

#define SYS_STRING_SIZE (CONFIG_SNMP_SYS_STRING_MAX_LEN + 1)

BUILD_ASSERT(sizeof(CONFIG_SNMP_SYS_NAME) <= SYS_STRING_SIZE,
	     "CONFIG_SNMP_SYS_NAME is longer than CONFIG_SNMP_SYS_STRING_MAX_LEN");
BUILD_ASSERT(sizeof(CONFIG_SNMP_SYS_CONTACT) <= SYS_STRING_SIZE,
	     "CONFIG_SNMP_SYS_CONTACT is longer than CONFIG_SNMP_SYS_STRING_MAX_LEN");
BUILD_ASSERT(sizeof(CONFIG_SNMP_SYS_LOCATION) <= SYS_STRING_SIZE,
	     "CONFIG_SNMP_SYS_LOCATION is longer than CONFIG_SNMP_SYS_STRING_MAX_LEN");

/* The writable strings, guarded by lock against concurrent SET requests and
 * application calls.
 */
static char sys_name[SYS_STRING_SIZE] = CONFIG_SNMP_SYS_NAME;
static char sys_contact[SYS_STRING_SIZE] = CONFIG_SNMP_SYS_CONTACT;
static char sys_location[SYS_STRING_SIZE] = CONFIG_SNMP_SYS_LOCATION;
static uint32_t sys_object_id[SNMP_OID_MAX_LEN] = {0U, 0U};
static size_t sys_object_id_len = 2U;
static K_MUTEX_DEFINE(lock);

/* Snapshot handed to the agent, taken under lock so that a value being
 * changed is never encoded half-way. Only the agent's context uses it, and
 * it encodes the value before invoking the next getter.
 */
static char scratch[SYS_STRING_SIZE];
static uint32_t scratch_oid[SNMP_OID_MAX_LEN];

/* DisplayString of RFC 2579: NVT ASCII, where CR may only start CR LF or
 * CR NUL. The values are stored NUL terminated, so a NUL is rejected too and
 * a CR must be followed by LF.
 */
static bool display_string_valid(const uint8_t *data, size_t len)
{
	for (size_t i = 0U; i < len; i++) {
		if (data[i] > 0x7FU || data[i] == 0U) {
			return false;
		}

		if (data[i] == '\r' && (i + 1U >= len || data[i + 1U] != '\n')) {
			return false;
		}
	}

	return true;
}

static int set_string(char *dst, const char *src)
{
	size_t len;

	if (src == NULL) {
		return -EINVAL;
	}

	len = strlen(src);
	if (len > CONFIG_SNMP_SYS_STRING_MAX_LEN) {
		return -EMSGSIZE;
	}

	if (!display_string_valid((const uint8_t *)src, len)) {
		return -EINVAL;
	}

	k_mutex_lock(&lock, K_FOREVER);
	memcpy(dst, src, len + 1U);
	k_mutex_unlock(&lock);

	return 0;
}

int snmp_mib2_system_set_object_id(const uint32_t *arcs, size_t len)
{
	if (arcs == NULL || len < 2U) {
		return -EINVAL;
	}

	/* The first two arcs share one encoded sub-identifier, X.690 8.19.4. */
	if (arcs[0] > 2U || (arcs[0] < 2U && arcs[1] >= 40U) ||
	    (arcs[0] == 2U && arcs[1] > UINT32_MAX - 80U)) {
		return -EINVAL;
	}

	if (len > SNMP_OID_MAX_LEN) {
		return -EMSGSIZE;
	}

	k_mutex_lock(&lock, K_FOREVER);
	memcpy(sys_object_id, arcs, len * sizeof(uint32_t));
	sys_object_id_len = len;
	k_mutex_unlock(&lock);

	return 0;
}

int snmp_mib2_system_set_name(const char *name)
{
	return set_string(sys_name, name);
}

int snmp_mib2_system_set_contact(const char *contact)
{
	return set_string(sys_contact, contact);
}

int snmp_mib2_system_set_location(const char *location)
{
	return set_string(sys_location, location);
}

static void value_from_cstr(struct snmp_value *val, const char *s)
{
	val->type = SNMP_TYPE_OCTET_STRING;
	val->octets.data = (const uint8_t *)s;
	val->octets.len = strlen(s);
}

/* Getter for the writable strings; user_data points at the buffer. */
static int string_get(const struct snmp_object *obj, void *row, struct snmp_value *val)
{
	const char *src = obj->user_data;

	ARG_UNUSED(row);

	k_mutex_lock(&lock, K_FOREVER);
	memcpy(scratch, src, strlen(src) + 1U);
	k_mutex_unlock(&lock);

	value_from_cstr(val, scratch);

	return 0;
}

/* Setter for the writable strings. */
static int string_set(const struct snmp_object *obj, void *row, const struct snmp_value *val,
		      enum snmp_set_phase phase)
{
	char *dst = obj->user_data;

	ARG_UNUSED(row);

	if (val->type != SNMP_TYPE_OCTET_STRING) {
		return -EINVAL;
	}

	if (val->octets.len > CONFIG_SNMP_SYS_STRING_MAX_LEN) {
		return -EMSGSIZE;
	}

	if (!display_string_valid(val->octets.data, val->octets.len)) {
		return -ERANGE;
	}

	if (phase == SNMP_SET_CHECK) {
		return 0;
	}

	k_mutex_lock(&lock, K_FOREVER);
	memcpy(dst, val->octets.data, val->octets.len);
	dst[val->octets.len] = '\0';
	k_mutex_unlock(&lock);

	return 0;
}

static int sys_descr_get(const struct snmp_object *obj, void *row, struct snmp_value *val)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(row);

	value_from_cstr(val, SYS_DESCR);

	return 0;
}

static int sys_object_id_get(const struct snmp_object *obj, void *row, struct snmp_value *val)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(row);

	k_mutex_lock(&lock, K_FOREVER);
	memcpy(scratch_oid, sys_object_id, sys_object_id_len * sizeof(uint32_t));
	val->oid.len = sys_object_id_len;
	k_mutex_unlock(&lock);

	val->type = SNMP_TYPE_OID;
	val->oid.arcs = scratch_oid;

	return 0;
}

static int sys_uptime_get(const struct snmp_object *obj, void *row, struct snmp_value *val)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(row);

	/* Hundredths of a second since the agent was last started, wrapping
	 * after about 497 days as the TimeTicks type prescribes.
	 */
	val->type = SNMP_TYPE_TIMETICKS;
	val->u32 = (uint32_t)((k_uptime_get() - snmp_agent_start_uptime()) / 10);

	return 0;
}

static int sys_services_get(const struct snmp_object *obj, void *row, struct snmp_value *val)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(row);

	val->type = SNMP_TYPE_INTEGER;
	val->i32 = CONFIG_SNMP_SYS_SERVICES;

	return 0;
}

static int sys_or_last_change_get(const struct snmp_object *obj, void *row, struct snmp_value *val)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(row);

	/* Every entry of sysORTable exists since the agent started. */
	val->type = SNMP_TYPE_TIMETICKS;
	val->u32 = 0U;

	return 0;
}

/* sysORTable rows are the snmp_capability entries of the build. The
 * library contributes snmpBasicComplianceRev2 of RFC 3418 when the snmp
 * group, snmpSetSerialNo and the coldStart and authenticationFailure
 * notifications are built besides this group.
 */
#if defined(CONFIG_SNMP_MIB2_SNMP) && defined(CONFIG_SNMP_NOTIFICATIONS)
SNMP_CAPABILITY_DEFINE(snmp_basic_compliance_rev2, "The MIB module for SNMP entities", 1, 3, 6, 1,
		       6, 3, 1, 2, 1, 3);
#endif

STRUCT_SECTION_START_EXTERN(snmp_capability);

static void sys_or_foreach_row(const struct snmp_table *tbl, snmp_row_cb_t cb, void *user_data)
{
	ARG_UNUSED(tbl);

	STRUCT_SECTION_FOREACH(snmp_capability, cap) {
		cb((void *)cap, user_data);
	}
}

static int sys_or_row_index(const struct snmp_table *tbl, void *row, uint32_t *arcs,
			    size_t max_arcs)
{
	const struct snmp_capability *cap = row;

	ARG_UNUSED(tbl);

	if (max_arcs < 1U) {
		return -EMSGSIZE;
	}

	/* sysORIndex is the position in the section, counting from one. */
	arcs[0] = (uint32_t)(cap - STRUCT_SECTION_START(snmp_capability)) + 1U;

	return 1;
}

static void *sys_or_row_lookup(const struct snmp_table *tbl, const uint32_t *arcs, size_t len)
{
	const struct snmp_capability *cap;
	size_t count;

	ARG_UNUSED(tbl);

	STRUCT_SECTION_COUNT(snmp_capability, &count);

	if (len != 1U || arcs[0] < 1U || arcs[0] > count) {
		return NULL;
	}

	STRUCT_SECTION_GET(snmp_capability, arcs[0] - 1U, &cap);

	return (void *)cap;
}

static const struct snmp_table sys_or_table = {
	.foreach_row = sys_or_foreach_row,
	.row_index = sys_or_row_index,
	.row_lookup = sys_or_row_lookup,
};

static int sys_or_id_get(const struct snmp_object *obj, void *row, struct snmp_value *val)
{
	const struct snmp_capability *cap = row;

	ARG_UNUSED(obj);

	val->type = SNMP_TYPE_OID;
	val->oid.arcs = cap->oid;
	val->oid.len = cap->oid_len;

	return 0;
}

static int sys_or_descr_get(const struct snmp_object *obj, void *row, struct snmp_value *val)
{
	const struct snmp_capability *cap = row;

	ARG_UNUSED(obj);

	value_from_cstr(val, cap->descr);

	return 0;
}

static int sys_or_uptime_get(const struct snmp_object *obj, void *row, struct snmp_value *val)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(row);

	val->type = SNMP_TYPE_TIMETICKS;
	val->u32 = 0U;

	return 0;
}

SNMP_SCALAR_DEFINE(mib2_sys_descr, sys_descr_get, NULL, NULL, MIB2_SYSTEM(1));
SNMP_SCALAR_DEFINE(mib2_sys_object_id, sys_object_id_get, NULL, NULL, MIB2_SYSTEM(2));
SNMP_SCALAR_DEFINE(mib2_sys_uptime, sys_uptime_get, NULL, NULL, MIB2_SYSTEM(3));
SNMP_SCALAR_DEFINE(mib2_sys_contact, string_get, string_set, sys_contact, MIB2_SYSTEM(4));
SNMP_SCALAR_DEFINE(mib2_sys_name, string_get, string_set, sys_name, MIB2_SYSTEM(5));
SNMP_SCALAR_DEFINE(mib2_sys_location, string_get, string_set, sys_location, MIB2_SYSTEM(6));
SNMP_SCALAR_DEFINE(mib2_sys_services, sys_services_get, NULL, NULL, MIB2_SYSTEM(7));
SNMP_SCALAR_DEFINE(mib2_sys_or_last_change, sys_or_last_change_get, NULL, NULL, MIB2_SYSTEM(8));
/* sysORIndex is not-accessible and therefore not registered. */
SNMP_COLUMN_DEFINE(mib2_sys_or_id, sys_or_table, sys_or_id_get, NULL, NULL, MIB2_SYS_OR_ENTRY(2));
SNMP_COLUMN_DEFINE(mib2_sys_or_descr, sys_or_table, sys_or_descr_get, NULL, NULL,
		   MIB2_SYS_OR_ENTRY(3));
SNMP_COLUMN_DEFINE(mib2_sys_or_uptime, sys_or_table, sys_or_uptime_get, NULL, NULL,
		   MIB2_SYS_OR_ENTRY(4));
