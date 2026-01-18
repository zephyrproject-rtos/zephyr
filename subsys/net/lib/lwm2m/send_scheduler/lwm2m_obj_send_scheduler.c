/*
 * Copyright (c) 2025 Clunky Machines
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(lwm2m_send_sched, CONFIG_LWM2M_SEND_SCHEDULER_LOG_LEVEL);

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/net/lwm2m.h>
#include <zephyr/sys/util.h>

#include "lwm2m_engine.h"
#include "lwm2m_object.h"
#include "lwm2m_registry.h"

#include <zephyr/net/lwm2m_send_scheduler.h>
#include "send_scheduler_internal.h"

/* Official OMA object IDs (reserved via OMA issue #858) */
#define SEND_SCHED_CTRL_OBJECT_ID  10523
#define SEND_SCHED_RULES_OBJECT_ID 10524

/* resource IDs */
#define SEND_SCHED_CTRL_RES_PAUSED          0
#define SEND_SCHED_CTRL_RES_MAX_SAMPLES     1
#define SEND_SCHED_CTRL_RES_MAX_AGE         2
#define SEND_SCHED_CTRL_RES_FLUSH           3
#define SEND_SCHED_CTRL_RES_FLUSH_ON_UPDATE 4
#define SEND_SCHED_RULES_RES_PATH           0
#define SEND_SCHED_RULES_RES_RULES          1

#define SEND_SCHED_CTRL_RES_COUNT      5
#define SEND_SCHED_CTRL_RES_INST_COUNT SEND_SCHED_CTRL_RES_COUNT

#define SEND_SCHED_RULES_RES_COUNT      2
#define SEND_SCHED_RULES_RES_INST_COUNT (1 + LWM2M_SEND_SCHED_MAX_RULE_STRINGS)

static struct lwm2m_engine_obj send_sched_ctrl_obj;
static struct lwm2m_engine_obj_field send_sched_ctrl_fields[] = {
	OBJ_FIELD(SEND_SCHED_CTRL_RES_PAUSED, RW, BOOL),
	OBJ_FIELD(SEND_SCHED_CTRL_RES_MAX_SAMPLES, RW, S32),
	OBJ_FIELD(SEND_SCHED_CTRL_RES_MAX_AGE, RW, S32),
	OBJ_FIELD_EXECUTE(SEND_SCHED_CTRL_RES_FLUSH),
	OBJ_FIELD(SEND_SCHED_CTRL_RES_FLUSH_ON_UPDATE, RW, BOOL),
};
static struct lwm2m_engine_res send_sched_ctrl_res[SEND_SCHED_CTRL_RES_COUNT];
static struct lwm2m_engine_res_inst send_sched_ctrl_res_inst[SEND_SCHED_CTRL_RES_INST_COUNT];
static struct lwm2m_engine_obj_inst send_sched_ctrl_inst;

static struct lwm2m_engine_obj send_sched_rules_obj;
static struct lwm2m_engine_obj_field send_sched_rules_fields[] = {
	OBJ_FIELD(SEND_SCHED_RULES_RES_PATH, RW, STRING),
	OBJ_FIELD(SEND_SCHED_RULES_RES_RULES, RW, STRING),
};
static struct lwm2m_engine_res send_sched_rules_res[LWM2M_SEND_SCHED_RULES_MAX_INSTANCES]
						   [SEND_SCHED_RULES_RES_COUNT];
static struct lwm2m_engine_res_inst send_sched_rules_res_inst[LWM2M_SEND_SCHED_RULES_MAX_INSTANCES]
							     [SEND_SCHED_RULES_RES_INST_COUNT];
static struct lwm2m_engine_obj_inst send_sched_rules_inst[LWM2M_SEND_SCHED_RULES_MAX_INSTANCES];

static bool scheduler_max_age_cb_registered;
static bool scheduler_max_samples_cb_registered;

/* Find the internal rule slot used by an object instance */
static int send_sched_rules_index_for_inst(uint16_t obj_inst_id)
{
	for (int idx = 0; idx < LWM2M_SEND_SCHED_RULES_MAX_INSTANCES; idx++) {
		if (send_sched_rules_inst[idx].obj &&
		    send_sched_rules_inst[idx].obj_inst_id == obj_inst_id) {
			return idx;
		}
	}

	return -1;
}

/* Check whether the attribute expects an integer value */
static bool send_sched_attribute_requires_integer(const char *attr)
{
	return (!strcmp(attr, "pmin") || !strcmp(attr, "pmax") || !strcmp(attr, "epmin") ||
		!strcmp(attr, "epmax"));
}

/* Check whether the attribute expects a floating-point value */
static bool send_sched_attribute_requires_float(const char *attr)
{
	return (!strcmp(attr, "gt") || !strcmp(attr, "lt") || !strcmp(attr, "st"));
}

/* Validate that the string can be parsed as a decimal integer */
static bool send_sched_is_valid_integer(const char *value)
{
	char *end = NULL;
	long parsed;

	if (value == NULL || *value == '\0') {
		return false;
	}

	errno = 0;
	parsed = strtol(value, &end, 10);
	if (errno == ERANGE) {
		return false;
	}

	if (end == value || (end && *end != '\0')) {
		return false;
	}

	ARG_UNUSED(parsed);

	return true;
}

/* Validate that the string can be parsed as a floating-point number */
static bool send_sched_is_valid_float(const char *value)
{
	char *end = NULL;
	double parsed;

	if (value == NULL || *value == '\0') {
		return false;
	}

	errno = 0;
	parsed = strtod(value, &end);
	if (errno == ERANGE) {
		return false;
	}

	if (end == value || (end && *end != '\0')) {
		return false;
	}

	ARG_UNUSED(parsed);

	return true;
}

/* Determine whether the attribute is supported by the scheduler */
static bool send_sched_attribute_is_allowed(const char *attr)
{
	return send_sched_attribute_requires_integer(attr) ||
	       send_sched_attribute_requires_float(attr);
}

/* Ensure the configured path string references a resource */
static int send_sched_validate_path(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
				    uint8_t *data, uint16_t data_len, bool last_block,
				    size_t total_size, size_t offset)
{
	char path_buf[LWM2M_SEND_SCHED_RULE_STRING_SIZE];
	size_t copy_len;
	int segments = 0;

	ARG_UNUSED(res_id);
	ARG_UNUSED(res_inst_id);
	ARG_UNUSED(last_block);
	ARG_UNUSED(total_size);
	ARG_UNUSED(offset);

	if (!data || data_len == 0) {
		LOG_WRN("Sampling rule path cannot be empty");
		return -EINVAL;
	}

	if (data_len >= sizeof(path_buf)) {
		LOG_WRN("Sampling rule path too long (%u)", data_len);
		return -ENOBUFS;
	}

	int entry_idx = send_sched_rules_index_for_inst(obj_inst_id);

	if (entry_idx >= 0) {
		send_sched_cancel_pmax_timer(&rule_entries[entry_idx]);
		rule_entries[entry_idx].pmax_deadline_ms = 0;
		rule_entries[entry_idx].has_cached_path = false;
		rule_entries[entry_idx].has_configured_path = false;
		rule_entries[entry_idx].has_last_reported = false;
		rule_entries[entry_idx].has_last_observed = false;
		send_sched_schedule_age_check();
	}

	copy_len = MIN((size_t)data_len, sizeof(path_buf) - 1U);
	memcpy(path_buf, data, copy_len);
	path_buf[copy_len] = '\0';

	if (path_buf[0] != '/') {
		LOG_WRN("Sampling rule path must start with '/'");
		return -EINVAL;
	}

	for (char *cursor = path_buf + 1; *cursor != '\0';) {
		char *next = strchr(cursor, '/');
		size_t seg_len = next ? (size_t)(next - cursor) : strlen(cursor);

		if (seg_len == 0) {
			LOG_WRN("Sampling rule path contains empty segment");
			return -EINVAL;
		}

		for (size_t idx = 0; idx < seg_len; idx++) {
			if (!isdigit((unsigned char)cursor[idx])) {
				LOG_WRN("Sampling rule path segment must be numeric");
				return -EINVAL;
			}
		}

		segments++;
		if (!next) {
			break;
		}
		cursor = next + 1;
	}

	if (segments != 3) {
		LOG_WRN("Sampling rule path must reference a resource (/obj/inst/res)");
		return -EINVAL;
	}

	if (entry_idx >= 0) {
		struct lwm2m_obj_path parsed_path;
		int ret = send_sched_parse_path(path_buf, &parsed_path);

		if (ret < 0) {
			LOG_WRN("Sampling rule path failed to parse (%d)", ret);
			return ret;
		}

		rule_entries[entry_idx].configured_path = parsed_path;
		rule_entries[entry_idx].has_configured_path = true;
	}

	return 0;
}

/* Check rule syntax and enforce per-instance attribute uniqueness */
static int send_sched_validate_rule(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
				    uint8_t *data, uint16_t data_len, bool last_block,
				    size_t total_size, size_t offset)
{
	char rule_buf[LWM2M_SEND_SCHED_RULE_STRING_SIZE];
	char *eq = NULL;
	const char *attr;
	const char *value;
	size_t attr_len;
	int entry_idx;
	int current_slot = -1;
	struct send_sched_rule_entry *entry;

	ARG_UNUSED(res_id);
	ARG_UNUSED(last_block);
	ARG_UNUSED(total_size);
	ARG_UNUSED(offset);

	entry_idx = send_sched_rules_index_for_inst(obj_inst_id);
	if (entry_idx < 0) {
		LOG_ERR("Sampling rule instance %u not found", obj_inst_id);
		return -ENOENT;
	}

	entry = &rule_entries[entry_idx];

	if (res_inst_id < LWM2M_SEND_SCHED_MAX_RULE_STRINGS) {
		current_slot = res_inst_id;
	}

	if (current_slot < 0) {
		for (int idx = 0; idx < LWM2M_SEND_SCHED_MAX_RULE_STRINGS; idx++) {
			if (&entry->rules[idx][0] == (char *)data) {
				current_slot = idx;
				break;
			}
		}
	}

	if (current_slot < 0 || current_slot >= LWM2M_SEND_SCHED_MAX_RULE_STRINGS) {
		LOG_ERR("Sampling rule index out of range (%d)", current_slot);
		return -EINVAL;
	}

	if (!data) {
		return -EINVAL;
	}

	if (data_len == 0U) {
		const char *existing = entry->rules[current_slot];

		if (existing[0] != '\0') {
			int32_t tmp;

			if (send_sched_rule_parse_int(existing, "pmin", &tmp)) {
				entry->pmin_waiting = false;
				entry->pmin_deadline_ms = 0;
				entry->has_pmin = false;
				entry->pmin_seconds = 0;
			}

			if (send_sched_rule_parse_int(existing, "pmax", &tmp)) {
				entry->pmax_seconds = 0;
				entry->pmax_deadline_ms = 0;
				send_sched_cancel_pmax_timer(entry);
			}
		}

		entry->rules[current_slot][0] = '\0';
		send_sched_rules_res_inst[entry_idx][current_slot].data_len = 0U;
		entry->has_last_reported = false;
		entry->has_last_observed = false;
		entry->rules_dirty = true;
		return 0;
	}

	if (data_len >= sizeof(rule_buf)) {
		LOG_WRN("Sampling rule string too long (%u)", data_len);
		return -ENOBUFS;
	}

	memcpy(rule_buf, data, data_len);
	rule_buf[data_len] = '\0';

	eq = strchr(rule_buf, '=');
	if (!eq || strchr(eq + 1, '=')) {
		LOG_WRN("Sampling rule must be formatted as attribute=value");
		return -EINVAL;
	}

	*eq = '\0';
	attr = rule_buf;
	value = eq + 1;
	attr_len = strlen(attr);

	if (attr_len == 0U || *value == '\0') {
		LOG_WRN("Sampling rule requires both attribute and value");
		return -EINVAL;
	}

	for (size_t idx = 0; idx < attr_len; idx++) {
		if (!islower((unsigned char)attr[idx])) {
			LOG_WRN("Sampling rule attribute contains invalid characters");
			return -EINVAL;
		}
	}

	if (!send_sched_attribute_is_allowed(attr)) {
		LOG_WRN("Sampling rule attribute '%s' is not supported", attr);
		return -EINVAL;
	}

	if (send_sched_attribute_requires_integer(attr)) {
		if (!send_sched_is_valid_integer(value)) {
			LOG_WRN("Sampling rule attribute '%s' expects integer value", attr);
			return -EINVAL;
		}
	} else if (send_sched_attribute_requires_float(attr)) {
		if (!send_sched_is_valid_float(value)) {
			LOG_WRN("Sampling rule attribute '%s' expects floating-point value", attr);
			return -EINVAL;
		}
	}

	for (int idx = 0; idx < LWM2M_SEND_SCHED_MAX_RULE_STRINGS; idx++) {
		const struct lwm2m_engine_res_inst *res_inst;
		const char *existing_eq;

		if (idx == current_slot) {
			continue;
		}

		res_inst = &send_sched_rules_res_inst[entry_idx][idx];
		if (res_inst->res_inst_id == RES_INSTANCE_NOT_CREATED || res_inst->data_len == 0U) {
			continue;
		}

		existing_eq = strchr(entry->rules[idx], '=');
		if (!existing_eq) {
			continue;
		}

		if ((size_t)(existing_eq - entry->rules[idx]) == attr_len &&
		    strncmp(entry->rules[idx], attr, attr_len) == 0) {
			LOG_WRN("Sampling rule attribute '%s' already defined", attr);
			return -EEXIST;
		}
	}

	entry->rules_dirty = true;
	return 0;
}

static int send_sched_flush_cb(uint16_t obj_inst_id, uint8_t *args, uint16_t args_len)
{
	ARG_UNUSED(obj_inst_id);
	ARG_UNUSED(args);
	ARG_UNUSED(args_len);

	LOG_DBG("Manual flush requested");
	return send_sched_flush_all();
}

static struct lwm2m_engine_obj_inst *send_sched_ctrl_create(uint16_t obj_inst_id)
{
	static bool created;
	int i = 0;
	int j = 0;

	if (created || obj_inst_id != 0U) {
		LOG_WRN("Scheduler control instance %u already exists or not 0", obj_inst_id);
		return NULL;
	}

	created = true;

	(void)memset(&send_sched_ctrl_inst, 0, sizeof(send_sched_ctrl_inst));
	init_res_instance(send_sched_ctrl_res_inst, ARRAY_SIZE(send_sched_ctrl_res_inst));
	(void)memset(send_sched_ctrl_res, 0, sizeof(send_sched_ctrl_res));

	INIT_OBJ_RES_DATA(SEND_SCHED_CTRL_RES_PAUSED, send_sched_ctrl_res, i,
			  send_sched_ctrl_res_inst, j, &scheduler_paused, sizeof(scheduler_paused));
	INIT_OBJ_RES_DATA(SEND_SCHED_CTRL_RES_MAX_SAMPLES, send_sched_ctrl_res, i,
			  send_sched_ctrl_res_inst, j, &scheduler_max_samples,
			  sizeof(scheduler_max_samples));
	INIT_OBJ_RES_DATA(SEND_SCHED_CTRL_RES_MAX_AGE, send_sched_ctrl_res, i,
			  send_sched_ctrl_res_inst, j, &scheduler_max_age,
			  sizeof(scheduler_max_age));
	INIT_OBJ_RES_EXECUTE(SEND_SCHED_CTRL_RES_FLUSH, send_sched_ctrl_res, i,
			     send_sched_flush_cb);
	INIT_OBJ_RES_DATA(SEND_SCHED_CTRL_RES_FLUSH_ON_UPDATE, send_sched_ctrl_res, i,
			  send_sched_ctrl_res_inst, j, &send_sched_flush_on_update,
			  sizeof(send_sched_flush_on_update));

	send_sched_ctrl_inst.resources = send_sched_ctrl_res;
	send_sched_ctrl_inst.resource_count = i;

	return &send_sched_ctrl_inst;
}

static int send_sched_ctrl_delete(uint16_t obj_inst_id)
{
	ARG_UNUSED(obj_inst_id);
	LOG_WRN("Scheduler control object cannot be deleted");
	return -EBUSY;
}

static int send_sched_ctrl_max_age_post_write_cb(uint16_t obj_inst_id, uint16_t res_id,
						 uint16_t res_inst_id, uint8_t *data,
						 uint16_t data_len, bool last_block,
						 size_t total_size, size_t offset)
{
	ARG_UNUSED(obj_inst_id);
	ARG_UNUSED(res_id);
	ARG_UNUSED(res_inst_id);
	ARG_UNUSED(data);
	ARG_UNUSED(data_len);
	ARG_UNUSED(last_block);
	ARG_UNUSED(total_size);
	ARG_UNUSED(offset);

	send_sched_process_max_age(true);

	return 0;
}

static int send_sched_ctrl_max_samples_post_write_cb(uint16_t obj_inst_id, uint16_t res_id,
						     uint16_t res_inst_id, uint8_t *data,
						     uint16_t data_len, bool last_block,
						     size_t total_size, size_t offset)
{
	ARG_UNUSED(obj_inst_id);
	ARG_UNUSED(res_id);
	ARG_UNUSED(res_inst_id);
	ARG_UNUSED(data);
	ARG_UNUSED(data_len);
	ARG_UNUSED(last_block);
	ARG_UNUSED(total_size);
	ARG_UNUSED(offset);

	if (scheduler_max_samples <= 0) {
		send_sched_reset_accumulated_samples();
		return 0;
	}

	send_sched_enforce_max_sample_limit();

	return 0;
}

/* Create a new rules object instance and wire resources */
static struct lwm2m_engine_obj_inst *send_sched_rules_create(uint16_t obj_inst_id)
{
	int avail = -1;
	int i = 0;
	int j = 0;

	for (int idx = 0; idx < LWM2M_SEND_SCHED_RULES_MAX_INSTANCES; idx++) {
		if (send_sched_rules_inst[idx].obj &&
		    send_sched_rules_inst[idx].obj_inst_id == obj_inst_id) {
			LOG_WRN("Sampling rules instance %u already exists", obj_inst_id);
			return NULL;
		}

		if (avail < 0 && send_sched_rules_inst[idx].obj == NULL) {
			avail = idx;
		}
	}

	if (avail < 0) {
		LOG_WRN("No slot available for sampling rules instance %u", obj_inst_id);
		return NULL;
	}

	(void)memset(&send_sched_rules_res[avail], 0, sizeof(send_sched_rules_res[avail]));
	(void)memset(&rule_entries[avail], 0, sizeof(rule_entries[avail]));
	(void)memset(&send_sched_rules_inst[avail], 0, sizeof(send_sched_rules_inst[avail]));

	k_work_init_delayable(&rule_entries[avail].pmax_work, send_sched_pmax_work_handler);

	init_res_instance(send_sched_rules_res_inst[avail],
			  ARRAY_SIZE(send_sched_rules_res_inst[avail]));

	INIT_OBJ_RES_LEN(SEND_SCHED_RULES_RES_PATH, send_sched_rules_res[avail], i,
			 send_sched_rules_res_inst[avail], j, 1U, false, true,
			 rule_entries[avail].path, sizeof(rule_entries[avail].path), 0, NULL, NULL,
			 send_sched_validate_path, NULL, NULL);

	INIT_OBJ_RES_LEN(SEND_SCHED_RULES_RES_RULES, send_sched_rules_res[avail], i,
			 send_sched_rules_res_inst[avail], j, LWM2M_SEND_SCHED_MAX_RULE_STRINGS,
			 true, false, rule_entries[avail].rules,
			 sizeof(rule_entries[avail].rules[0]), 0, NULL, NULL,
			 send_sched_validate_rule, NULL, NULL);

	send_sched_rules_inst[avail].resources = send_sched_rules_res[avail];
	send_sched_rules_inst[avail].resource_count = i;
	send_sched_rules_inst[avail].obj = &send_sched_rules_obj;
	send_sched_rules_inst[avail].obj_inst_id = obj_inst_id;

	return &send_sched_rules_inst[avail];
}

/* Reset rule bookkeeping when the instance is deleted */
static int send_sched_rules_delete(uint16_t obj_inst_id)
{
	int idx = send_sched_rules_index_for_inst(obj_inst_id);
	struct send_sched_rule_entry *entry;

	if (idx < 0) {
		return -ENOENT;
	}

	entry = &rule_entries[idx];
	send_sched_cancel_pmax_timer(entry);

	memset(entry, 0, sizeof(*entry));
	k_work_init_delayable(&entry->pmax_work, send_sched_pmax_work_handler);

	memset(&send_sched_rules_res[idx], 0, sizeof(send_sched_rules_res[idx]));
	memset(&send_sched_rules_inst[idx], 0, sizeof(send_sched_rules_inst[idx]));
	init_res_instance(send_sched_rules_res_inst[idx],
			  ARRAY_SIZE(send_sched_rules_res_inst[idx]));

	send_sched_schedule_age_check();

	return 0;
}

/* Register the scheduler objects and instantiate defaults */
int lwm2m_send_sched_init(void)
{
	static bool registered;
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	int ret;

	if (!registered) {
		send_sched_ctrl_obj.obj_id = SEND_SCHED_CTRL_OBJECT_ID;
		send_sched_ctrl_obj.version_major = 1;
		send_sched_ctrl_obj.version_minor = 0;
		send_sched_ctrl_obj.is_core = false;
		send_sched_ctrl_obj.fields = send_sched_ctrl_fields;
		send_sched_ctrl_obj.field_count = ARRAY_SIZE(send_sched_ctrl_fields);
		send_sched_ctrl_obj.max_instance_count = 1U;
		send_sched_ctrl_obj.create_cb = send_sched_ctrl_create;
		send_sched_ctrl_obj.delete_cb = send_sched_ctrl_delete;
		lwm2m_register_obj(&send_sched_ctrl_obj);

		send_sched_rules_obj.obj_id = SEND_SCHED_RULES_OBJECT_ID;
		send_sched_rules_obj.version_major = 1;
		send_sched_rules_obj.version_minor = 0;
		send_sched_rules_obj.is_core = false;
		send_sched_rules_obj.fields = send_sched_rules_fields;
		send_sched_rules_obj.field_count = ARRAY_SIZE(send_sched_rules_fields);
		send_sched_rules_obj.max_instance_count = LWM2M_SEND_SCHED_RULES_MAX_INSTANCES;
		send_sched_rules_obj.create_cb = send_sched_rules_create;
		send_sched_rules_obj.delete_cb = send_sched_rules_delete;
		lwm2m_register_obj(&send_sched_rules_obj);

		registered = true;
	} else {
		LOG_DBG("already registered send scheduler objects");
	}

	ret = lwm2m_create_obj_inst(SEND_SCHED_CTRL_OBJECT_ID, 0, &obj_inst);
	if (ret < 0 && ret != -EEXIST) {
		LOG_ERR("Failed to instantiate scheduler control object (%d)", ret);
		return ret;
	}

	if (!scheduler_max_samples_cb_registered) {
		int cb_ret = lwm2m_register_post_write_callback(
			&LWM2M_OBJ(SEND_SCHED_CTRL_OBJECT_ID, 0, SEND_SCHED_CTRL_RES_MAX_SAMPLES),
			send_sched_ctrl_max_samples_post_write_cb);
		if (cb_ret < 0) {
			LOG_ERR("Failed to register max-samples callback (%d)", cb_ret);
			return cb_ret;
		}
		scheduler_max_samples_cb_registered = true;
	}

	if (!scheduler_max_age_cb_registered) {
		int cb_ret = lwm2m_register_post_write_callback(
			&LWM2M_OBJ(SEND_SCHED_CTRL_OBJECT_ID, 0, SEND_SCHED_CTRL_RES_MAX_AGE),
			send_sched_ctrl_max_age_post_write_cb);
		if (cb_ret < 0) {
			LOG_ERR("Failed to register max-age callback (%d)", cb_ret);
			return cb_ret;
		}
		scheduler_max_age_cb_registered = true;
	}

	return 0;
}

LWM2M_OBJ_INIT(lwm2m_send_sched_init);
