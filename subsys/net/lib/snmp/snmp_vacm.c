/*
 * Copyright (c) 2026 StructCore Embedded
 * SPDX-License-Identifier: Apache-2.0
 *
 * SNMPv3 View-based Access Control Model (RFC 3415).
 * Full implementation supporting vacmContextTable, vacmSecurityToGroupTable,
 * vacmAccessTable, and vacmViewTreeFamilyTable with wildcard mask matching.
 */

#include <zephyr/net/snmp/snmp_vacm.h>
#include <zephyr/net/snmp/snmp_ber.h>
#include <zephyr/net/snmp/snmp_mib.h>
#include <zephyr/net/snmp/snmp_core.h>
#include <string.h>
#include <errno.h>

#if (defined(CONFIG_SNMP) || defined(CONFIG_SNMP_AGENT)) && defined(CONFIG_SNMP_VERSION_3)

/* ---- Internal Helpers ---- */

static void safe_strncpy(char *dst, const char *src, size_t dst_sz)
{
	if (!dst || dst_sz == 0) {
		return;
	}
	if (!src) {
		dst[0] = '\0';
		return;
	}
	strncpy(dst, src, dst_sz - 1);
	dst[dst_sz - 1] = '\0';
}

static bool vacm_context_is_valid(const struct snmp_agent *inst, const char *context_name)
{
	if (!inst) {
		return false;
	}
	const char *target = context_name ? context_name : "";

	for (size_t i = 0; i < CONFIG_SNMP_VACM_MAX_CONTEXTS; i++) {
		if (inst->contexts[i].in_use && strcmp(inst->contexts[i].name, target) == 0) {
			return true;
		}
	}
	return false;
}

static const char *vacm_resolve_group(const struct snmp_agent *inst, int security_model,
				      const char *security_name)
{
	if (!inst || !security_name) {
		return NULL;
	}

	/* Pass 1: exact security model match */
	for (size_t i = 0; i < CONFIG_SNMP_VACM_MAX_GROUPS; i++) {
		if (inst->groups[i].in_use && inst->groups[i].security_model == security_model &&
		    strcmp(inst->groups[i].security_name, security_name) == 0) {
			return inst->groups[i].group_name;
		}
	}

	/* Pass 2: wildcard security model (ANY) */
	for (size_t i = 0; i < CONFIG_SNMP_VACM_MAX_GROUPS; i++) {
		if (inst->groups[i].in_use &&
		    inst->groups[i].security_model == SNMP_SEC_MODEL_ANY &&
		    strcmp(inst->groups[i].security_name, security_name) == 0) {
			return inst->groups[i].group_name;
		}
	}

	return NULL;
}

static const struct vacm_access_rule_entry *vacm_find_access_rule(const struct snmp_agent *inst,
								  const char *group_name,
								  const char *context_name,
								  int security_model, int sec_level)
{
	if (!inst || !group_name) {
		return NULL;
	}

	const char *ctx = context_name ? context_name : "";
	const struct vacm_access_rule_entry *best_rule = NULL;
	int best_score = -1;

	for (size_t i = 0; i < CONFIG_SNMP_VACM_MAX_ACCESS_RULES; i++) {
		const struct vacm_access_rule_entry *rule = &inst->access_rules[i];

		if (!rule->in_use) {
			continue;
		}
		if (strcmp(rule->group_name, group_name) != 0) {
			continue;
		}
		if (rule->security_model != SNMP_SEC_MODEL_ANY &&
		    rule->security_model != security_model) {
			continue;
		}

		/* Context prefix matching */
		size_t prefix_len = strlen(rule->context_prefix);
		if (prefix_len > 0) {

			if (strncmp(ctx, rule->context_prefix, prefix_len) != 0) {
				continue;
			}
		}

		/* Security level check: request must meet minimum level */
		if (sec_level < rule->min_sec_level) {
			continue;
		}

		/* Scoring for most specific match:
		 * model match (100) + prefix length (10) + min_sec_level
		 */
		int score = 0;
		if (rule->security_model == security_model) {

			score += 100;
		}
		score += (int)prefix_len * 10;
		score += rule->min_sec_level;

		if (score > best_score) {
			best_score = score;
			best_rule = rule;
		}
	}

	return best_rule;
}

static int vacm_evaluate_view(const struct snmp_agent *inst, const char *view_name,
			      const struct snmp_oid *target_oid)
{
	if (!inst || !view_name || view_name[0] == '\0') {
		return -EACCES;
	}

	if (!target_oid) {
		/* General view presence check */
		for (size_t i = 0; i < CONFIG_SNMP_VACM_MAX_VIEW_TREES; i++) {
			if (inst->views[i].in_use &&
			    strcmp(inst->views[i].view_name, view_name) == 0 &&
			    inst->views[i].type == SNMP_VACM_VIEW_INCLUDED) {
				return 0;
			}
		}
		return -EACCES;
	}

	int best_match_len = -1;
	int best_match_type = SNMP_VACM_VIEW_EXCLUDED;

	/* RFC 3415 §4: evaluate target OID against view tree family entries */
	for (size_t i = 0; i < CONFIG_SNMP_VACM_MAX_VIEW_TREES; i++) {
		const struct vacm_view_tree_entry *entry = &inst->views[i];

		if (!entry->in_use) {
			continue;
		}
		if (strcmp(entry->view_name, view_name) != 0) {
			continue;
		}
		if (target_oid->len < entry->subtree.len) {
			continue;
		}

		/* Check each sub-identifier against subtree and mask */
		bool matches = true;
		for (size_t j = 0; j < entry->subtree.len; j++) {

			size_t byte_idx = j / 8;
			uint8_t bit_mask = 0x80 >> (j % 8);
			bool check_exact = true;

			if (byte_idx < entry->mask_len) {
				check_exact = (entry->mask[byte_idx] & bit_mask) != 0;
			}

			if (check_exact && (target_oid->ids[j] != entry->subtree.ids[j])) {
				matches = false;
				break;
			}
		}

		if (matches) {
			/* Longest match selection per RFC 3415 §4.2 */
			if ((int)entry->subtree.len > best_match_len) {
				best_match_len = (int)entry->subtree.len;
				best_match_type = entry->type;
			}
		}
	}

	if (best_match_len < 0) {
		return -EACCES; /* No matching subtree found in view */
	}

	return (best_match_type == SNMP_VACM_VIEW_INCLUDED) ? 0 : -EACCES;
}

/* ---- Instance-based APIs ---- */

int snmp_vacm_init(struct snmp_agent *inst)
{
	if (!inst) {
		return -EINVAL;
	}

	memset(inst->contexts, 0, sizeof(inst->contexts));
	inst->context_cnt = 0;
	memset(inst->groups, 0, sizeof(inst->groups));
	inst->group_cnt = 0;
	memset(inst->access_rules, 0, sizeof(inst->access_rules));
	inst->access_rule_cnt = 0;
	memset(inst->views, 0, sizeof(inst->views));
	inst->view_cnt = 0;

	/* Default context: empty string ("") */
	int ret = snmp_vacm_add_context(inst, "");
	if (ret < 0) {

		return ret;
	}

	/* Group "admin": mapped from USM user CONFIG_SNMP_V3_USER_NAME */
	ret = snmp_vacm_add_group_mapping(inst, SNMP_SEC_MODEL_USM, CONFIG_SNMP_V3_USER_NAME,
					  "admin");
	if (ret < 0) {
		return ret;
	}

	/* Access rule: "admin" gets "fullView" for Read, Write, Notify with authNoPriv+ */
	ret = snmp_vacm_add_access_rule(inst, "admin", "", SNMP_SEC_MODEL_USM,
					SNMP_SEC_LEVEL_AUTH_NOPRIV, "fullView", "fullView",
					"fullView");
	if (ret < 0) {
		return ret;
	}

	/* View "fullView": includes internet subtree (1.3.6.1) */
	struct snmp_oid internet_oid = {.len = 4, .ids = {1, 3, 6, 1}};
	ret = snmp_vacm_add_view_tree(inst, "fullView", &internet_oid, NULL, 0,

				      SNMP_VACM_VIEW_INCLUDED);
	if (ret < 0) {
		return ret;
	}

	/* Group "readonly": mapped from user "monitor" */
	ret = snmp_vacm_add_group_mapping(inst, SNMP_SEC_MODEL_USM, "monitor", "readonly");
	if (ret < 0) {
		return ret;
	}

	/* Access rule: "readonly" gets "restrictedView" for Read, Notify with noAuthNoPriv+ */
	ret = snmp_vacm_add_access_rule(inst, "readonly", "", SNMP_SEC_MODEL_USM,
					SNMP_SEC_LEVEL_NOAUTH_NOPRIV, "restrictedView", "",
					"restrictedView");
	if (ret < 0) {
		return ret;
	}

	/* View "restrictedView": includes system group (1.3.6.1.2.1.1) and snmpEngine
	 * (1.3.6.1.6.3.10.2.1) */
	struct snmp_oid system_oid = {.len = 7, .ids = {1, 3, 6, 1, 2, 1, 1}};
	ret = snmp_vacm_add_view_tree(inst, "restrictedView", &system_oid, NULL, 0,

				      SNMP_VACM_VIEW_INCLUDED);
	if (ret < 0) {
		return ret;
	}

	struct snmp_oid engine_oid = {.len = 9, .ids = {1, 3, 6, 1, 6, 3, 10, 2, 1}};
	ret = snmp_vacm_add_view_tree(inst, "restrictedView", &engine_oid, NULL, 0,

				      SNMP_VACM_VIEW_INCLUDED);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

int snmp_vacm_add_context(struct snmp_agent *inst, const char *context_name)
{
	if (!inst) {
		return -EINVAL;
	}
	const char *name = context_name ? context_name : "";

	if (strlen(name) >= SNMP_VACM_NAME_MAX_LEN) {
		return -EINVAL;
	}

	for (size_t i = 0; i < CONFIG_SNMP_VACM_MAX_CONTEXTS; i++) {
		if (inst->contexts[i].in_use && strcmp(inst->contexts[i].name, name) == 0) {
			return 0; /* Already registered */
		}
	}

	for (size_t i = 0; i < CONFIG_SNMP_VACM_MAX_CONTEXTS; i++) {
		if (!inst->contexts[i].in_use) {
			safe_strncpy(inst->contexts[i].name, name, sizeof(inst->contexts[i].name));
			inst->contexts[i].in_use = true;
			inst->context_cnt++;
			return 0;
		}
	}

	return -ENOMEM;
}

int snmp_vacm_add_group_mapping(struct snmp_agent *inst, int security_model,
				const char *security_name, const char *group_name)
{
	if (!inst || !security_name || !group_name) {
		return -EINVAL;
	}
	if (strlen(security_name) >= SNMP_VACM_NAME_MAX_LEN ||
	    strlen(group_name) >= SNMP_VACM_NAME_MAX_LEN) {
		return -EINVAL;
	}

	/* Update existing mapping if already present */
	for (size_t i = 0; i < CONFIG_SNMP_VACM_MAX_GROUPS; i++) {
		if (inst->groups[i].in_use && inst->groups[i].security_model == security_model &&
		    strcmp(inst->groups[i].security_name, security_name) == 0) {
			safe_strncpy(inst->groups[i].group_name, group_name,
				     sizeof(inst->groups[i].group_name));
			return 0;
		}
	}

	/* Find empty slot */
	for (size_t i = 0; i < CONFIG_SNMP_VACM_MAX_GROUPS; i++) {
		if (!inst->groups[i].in_use) {
			inst->groups[i].security_model = security_model;
			safe_strncpy(inst->groups[i].security_name, security_name,
				     sizeof(inst->groups[i].security_name));
			safe_strncpy(inst->groups[i].group_name, group_name,
				     sizeof(inst->groups[i].group_name));
			inst->groups[i].in_use = true;
			inst->group_cnt++;
			return 0;
		}
	}

	return -ENOMEM;
}

int snmp_vacm_add_access_rule(struct snmp_agent *inst, const char *group_name,
			      const char *context_prefix, int security_model, int min_sec_level,
			      const char *read_view, const char *write_view,
			      const char *notify_view)
{
	if (!inst || !group_name) {
		return -EINVAL;
	}
	if (min_sec_level < SNMP_SEC_LEVEL_NOAUTH_NOPRIV ||
	    min_sec_level > SNMP_SEC_LEVEL_AUTH_PRIV) {
		return -EINVAL;
	}

	const char *prefix = context_prefix ? context_prefix : "";
	if (strlen(group_name) >= SNMP_VACM_NAME_MAX_LEN ||

	    strlen(prefix) >= SNMP_VACM_NAME_MAX_LEN) {
		return -EINVAL;
	}

	/* Check for existing rule to update */
	for (size_t i = 0; i < CONFIG_SNMP_VACM_MAX_ACCESS_RULES; i++) {
		struct vacm_access_rule_entry *rule = &inst->access_rules[i];

		if (rule->in_use && rule->security_model == security_model &&
		    rule->min_sec_level == min_sec_level &&
		    strcmp(rule->group_name, group_name) == 0 &&
		    strcmp(rule->context_prefix, prefix) == 0) {
			safe_strncpy(rule->read_view, read_view, sizeof(rule->read_view));
			safe_strncpy(rule->write_view, write_view, sizeof(rule->write_view));
			safe_strncpy(rule->notify_view, notify_view, sizeof(rule->notify_view));
			return 0;
		}
	}

	/* Find empty slot */
	for (size_t i = 0; i < CONFIG_SNMP_VACM_MAX_ACCESS_RULES; i++) {
		struct vacm_access_rule_entry *rule = &inst->access_rules[i];

		if (!rule->in_use) {
			safe_strncpy(rule->group_name, group_name, sizeof(rule->group_name));
			safe_strncpy(rule->context_prefix, prefix, sizeof(rule->context_prefix));
			rule->security_model = security_model;
			rule->min_sec_level = min_sec_level;
			safe_strncpy(rule->read_view, read_view, sizeof(rule->read_view));
			safe_strncpy(rule->write_view, write_view, sizeof(rule->write_view));
			safe_strncpy(rule->notify_view, notify_view, sizeof(rule->notify_view));
			rule->in_use = true;
			inst->access_rule_cnt++;
			return 0;
		}
	}

	return -ENOMEM;
}

int snmp_vacm_add_view_tree(struct snmp_agent *inst, const char *view_name,
			    const struct snmp_oid *subtree, const uint8_t *mask, size_t mask_len,
			    int type_included)
{
	if (!inst || !view_name || !subtree) {
		return -EINVAL;
	}
	if (strlen(view_name) >= SNMP_VACM_NAME_MAX_LEN) {
		return -EINVAL;
	}
	if (mask_len > SNMP_VACM_MASK_MAX_LEN) {
		return -EINVAL;
	}
	if (type_included != SNMP_VACM_VIEW_INCLUDED && type_included != SNMP_VACM_VIEW_EXCLUDED) {
		return -EINVAL;
	}

	/* Check for existing entry to update */
	for (size_t i = 0; i < CONFIG_SNMP_VACM_MAX_VIEW_TREES; i++) {
		struct vacm_view_tree_entry *entry = &inst->views[i];

		if (entry->in_use && strcmp(entry->view_name, view_name) == 0 &&
		    snmp_oid_equal(&entry->subtree, subtree)) {
			entry->type = type_included;
			entry->mask_len = mask_len;
			if (mask && mask_len > 0) {
				memcpy(entry->mask, mask, mask_len);
			} else {
				memset(entry->mask, 0, sizeof(entry->mask));
			}
			return 0;
		}
	}

	/* Find empty slot */
	for (size_t i = 0; i < CONFIG_SNMP_VACM_MAX_VIEW_TREES; i++) {
		struct vacm_view_tree_entry *entry = &inst->views[i];

		if (!entry->in_use) {
			safe_strncpy(entry->view_name, view_name, sizeof(entry->view_name));
			entry->subtree = *subtree;
			entry->type = type_included;
			entry->mask_len = mask_len;
			if (mask && mask_len > 0) {
				memcpy(entry->mask, mask, mask_len);
			} else {
				memset(entry->mask, 0, sizeof(entry->mask));
			}
			entry->in_use = true;
			inst->view_cnt++;
			return 0;
		}
	}

	return -ENOMEM;
}

int snmp_vacm_check_access_ex(struct snmp_agent *inst, const char *context_name, int security_model,
			      const char *security_name, int sec_level, int access_type,
			      const struct snmp_oid *target_oid)
{
	if (!inst || !security_name) {
		return -EINVAL;
	}

	const char *ctx = context_name ? context_name : "";

	/* 1. Context validation per vacmContextTable */
	if (!vacm_context_is_valid(inst, ctx)) {
		return -EACCES;
	}

	/* 2. Security-to-Group mapping */
	const char *group = vacm_resolve_group(inst, security_model, security_name);
	if (!group) {

		return -EACCES;
	}

	/* 3. Access table lookup & security level verification */
	const struct vacm_access_rule_entry *rule =
		vacm_find_access_rule(inst, group, ctx, security_model, sec_level);
	if (!rule) {
		return -EACCES;
	}

	/* 4. Select view by access type */
	const char *view = NULL;
	switch (access_type) {

	case SNMP_VACM_ACCESS_READ:
		view = rule->read_view;
		break;
	case SNMP_VACM_ACCESS_WRITE:
		view = rule->write_view;
		break;
	case SNMP_VACM_ACCESS_NOTIFY:
		view = rule->notify_view;
		break;
	default:
		return -EINVAL;
	}

	if (!view || view[0] == '\0') {
		return -EACCES;
	}

	/* 5. View Tree Family OID matching per RFC 3415 §4 */
	return vacm_evaluate_view(inst, view, target_oid);
}

int snmp_vacm_check_access(struct snmp_agent *inst, const struct snmp_v3_msg *msg, bool is_write)
{
	if (inst == NULL || msg == NULL) {
		return -EINVAL;
	}

	int sec_level = (int)msg->sec_level;
	if (sec_level == 0) {
		/* Fallback if sec_level was not filled by USM verification (e.g. mock caller) */
		if ((msg->global.msg_flags & 0x02U) != 0U) {
			sec_level = SNMP_SEC_LEVEL_AUTH_PRIV;
		} else if ((msg->global.msg_flags & 0x01U) != 0U) {
			sec_level = SNMP_SEC_LEVEL_AUTH_NOPRIV;
		} else {
			sec_level = SNMP_SEC_LEVEL_NOAUTH_NOPRIV;
		}
	}

	int access_type = is_write ? SNMP_VACM_ACCESS_WRITE : SNMP_VACM_ACCESS_READ;

	char user_name[sizeof(msg->usm.user_name) + 1];
	size_t name_len = msg->usm.user_name_len < sizeof(msg->usm.user_name)
				  ? msg->usm.user_name_len
				  : sizeof(msg->usm.user_name);
	memcpy(user_name, msg->usm.user_name, name_len);
	user_name[name_len] = '\0';

	if (msg->scoped.pdu.varbind_cnt == 0) {
		return snmp_vacm_check_access_ex(inst, (const char *)msg->scoped.context_name,
						 msg->global.msg_security_model, user_name,
						 sec_level, access_type, NULL);
	}

	for (size_t i = 0; i < msg->scoped.pdu.varbind_cnt; i++) {
		int ret = snmp_vacm_check_access_ex(inst, (const char *)msg->scoped.context_name,
						    msg->global.msg_security_model, user_name,
						    sec_level, access_type,
						    &msg->scoped.pdu.varbinds[i].oid);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

#endif /* CONFIG_SNMP_AGENT && CONFIG_SNMP_VERSION_3 */
