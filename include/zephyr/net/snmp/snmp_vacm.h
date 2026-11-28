/*
 * Copyright (c) 2026 StructCore Embedded
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_SNMP_SNMP_VACM_H_
#define ZEPHYR_INCLUDE_NET_SNMP_SNMP_VACM_H_

#include <zephyr/kernel.h>
#include <zephyr/net/snmp/snmp_ber.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup snmp_vacm View-Based Access Control Model (VACM)
 * @ingroup snmp
 * @{
 * @brief RFC 3415 access control tables and MIB view filtering.
 */

/* ---- VACM Access Types ---- */
#define SNMP_VACM_ACCESS_READ   1
#define SNMP_VACM_ACCESS_WRITE  2
#define SNMP_VACM_ACCESS_NOTIFY 3

/* ---- VACM Security Levels (RFC 3411 §3.4.3 / RFC 3415) ---- */
#define SNMP_SEC_LEVEL_NOAUTH_NOPRIV 1
#define SNMP_SEC_LEVEL_AUTH_NOPRIV   2
#define SNMP_SEC_LEVEL_AUTH_PRIV     3

/* ---- VACM Security Models ---- */
#define SNMP_SEC_MODEL_ANY 0
#define SNMP_SEC_MODEL_V1  1
#define SNMP_SEC_MODEL_V2C 2
#define SNMP_SEC_MODEL_USM 3

/* ---- VACM View Tree Family Types (RFC 3415 §4) ---- */
#define SNMP_VACM_VIEW_INCLUDED 1
#define SNMP_VACM_VIEW_EXCLUDED 2

/* ---- Table Capacities & Limits ---- */
#ifndef CONFIG_SNMP_VACM_MAX_CONTEXTS
#define CONFIG_SNMP_VACM_MAX_CONTEXTS 8
#endif

#ifndef CONFIG_SNMP_VACM_MAX_GROUPS
#define CONFIG_SNMP_VACM_MAX_GROUPS 16
#endif

#ifndef CONFIG_SNMP_VACM_MAX_ACCESS_RULES
#define CONFIG_SNMP_VACM_MAX_ACCESS_RULES 16
#endif

#ifndef CONFIG_SNMP_VACM_MAX_VIEW_TREES
#define CONFIG_SNMP_VACM_MAX_VIEW_TREES 32
#endif

#define SNMP_VACM_NAME_MAX_LEN 32
#define SNMP_VACM_MASK_MAX_LEN 16

/* Forward declaration of agent instance */
struct snmp_agent;

/* ---- Table Entry Structures ---- */

struct vacm_context_entry {
	char name[SNMP_VACM_NAME_MAX_LEN];
	bool in_use;
};

struct vacm_group_mapping_entry {
	int security_model;
	char security_name[SNMP_VACM_NAME_MAX_LEN];
	char group_name[SNMP_VACM_NAME_MAX_LEN];
	bool in_use;
};

struct vacm_access_rule_entry {
	char group_name[SNMP_VACM_NAME_MAX_LEN];
	char context_prefix[SNMP_VACM_NAME_MAX_LEN];
	int security_model;
	int min_sec_level;
	char read_view[SNMP_VACM_NAME_MAX_LEN];
	char write_view[SNMP_VACM_NAME_MAX_LEN];
	char notify_view[SNMP_VACM_NAME_MAX_LEN];
	bool in_use;
};

struct vacm_view_tree_entry {
	char view_name[SNMP_VACM_NAME_MAX_LEN];
	struct snmp_oid subtree;
	uint8_t mask[SNMP_VACM_MASK_MAX_LEN];
	size_t mask_len;
	int type; /* SNMP_VACM_VIEW_INCLUDED or SNMP_VACM_VIEW_EXCLUDED */
	bool in_use;
};

/* ---- Instance-based APIs ---- */

/**
 * @brief Initialize VACM for an agent instance with the RFC 3415 default views,
 *        groups, access rules and context entries.
 * @param inst Target agent instance.
 * @return 0 on success, -EINVAL if @p inst is NULL.
 */
int snmp_vacm_init(struct snmp_agent *inst);

/**
 * @brief Register a context name in the instance's vacmContextTable.
 * @param inst Target agent instance.
 * @param context_name Context to add; the empty string is always valid.
 * @return 0 on success, -EINVAL on bad arguments, -ENOMEM if the table is full.
 */
int snmp_vacm_add_context(struct snmp_agent *inst, const char *context_name);

/**
 * @brief Map security model + security name to a group (vacmSecurityToGroupTable).
 * @param inst Target agent instance.
 * @param security_model One of SNMP_SEC_MODEL_*.
 * @param security_name Security name (community or USM user).
 * @param group_name Group name.
 * @return 0 on success, -EINVAL on bad arguments, -ENOMEM if the table is full.
 */
int snmp_vacm_add_group_mapping(struct snmp_agent *inst, int security_model,
				const char *security_name, const char *group_name);

/**
 * @brief Add a row to the instance's vacmAccessTable.
 * @param inst Target agent instance.
 * @param group_name Group the rule applies to.
 * @param context_prefix Context prefix ("" matches the default context).
 * @param security_model One of SNMP_SEC_MODEL_*, or SNMP_SEC_MODEL_ANY.
 * @param min_sec_level Minimum SNMP_SEC_LEVEL_* required.
 * @param read_view Name of the readable view.
 * @param write_view Name of the writable view.
 * @param notify_view Name of the notification view.
 * @return 0 on success, -EINVAL on bad arguments, -ENOMEM if the table is full.
 */
int snmp_vacm_add_access_rule(struct snmp_agent *inst, const char *group_name,
			      const char *context_prefix, int security_model, int min_sec_level,
			      const char *read_view, const char *write_view,
			      const char *notify_view);

/**
 * @brief Add a subtree to vacmViewTreeFamilyTable.
 * @param inst Target agent instance.
 * @param view_name View the entry belongs to.
 * @param subtree Subtree OID.
 * @param mask Optional bitmask, NULL for exact/prefix matching.
 * @param mask_len Length of @p mask.
 * @param type_included SNMP_VACM_VIEW_INCLUDED or SNMP_VACM_VIEW_EXCLUDED.
 * @return 0 on success, -EINVAL on bad arguments, -ENOMEM if the table is full.
 */
int snmp_vacm_add_view_tree(struct snmp_agent *inst, const char *view_name,
			    const struct snmp_oid *subtree, const uint8_t *mask, size_t mask_len,
			    int type_included);

/**
 * @brief RFC 3415 section 4 access decision for one target OID.
 * @param inst Target agent instance.
 * @param context_name Context of the request.
 * @param security_model One of SNMP_SEC_MODEL_*.
 * @param security_name Security name of the requester.
 * @param sec_level Requested SNMP_SEC_LEVEL_*.
 * @param access_type SNMP_VACM_ACCESS_READ, _WRITE or _NOTIFY.
 * @param target_oid OID to test, NULL for a whole-context decision.
 * @return 0 when access is granted, -EACCES otherwise.
 */
int snmp_vacm_check_access_ex(struct snmp_agent *inst, const char *context_name, int security_model,
			      const char *security_name, int sec_level, int access_type,
			      const struct snmp_oid *target_oid);

/**
 * @brief Check every varbind of an incoming v3 message against the instance's tables.
 * @param inst Target agent instance.
 * @param msg Incoming v3 message.
 * @param is_write True when the scoped PDU is a SET request.
 * @return 0 when all varbinds are accessible, -EACCES on the first denial.
 */
int snmp_vacm_check_access(struct snmp_agent *inst, const struct snmp_v3_msg *msg, bool is_write);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_SNMP_SNMP_VACM_H_ */
