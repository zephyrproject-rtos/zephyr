/*
 * Copyright (c) 2025 Clunky Machines
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME lwm2m_send_sched
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_LWM2M_SEND_SCHEDULER_LOG_LEVEL);

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <zephyr/kernel.h>
#include <zephyr/net/lwm2m.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "lwm2m_engine.h"
#include "lwm2m_object.h"
#include "lwm2m_registry.h"

#include <zephyr/net/lwm2m_send_scheduler.h>
#include "send_scheduler_internal.h"

BUILD_ASSERT(CONFIG_LWM2M_COMPOSITE_PATH_LIST_SIZE >= LWM2M_SEND_SCHED_RULES_MAX_INSTANCES,
	     "Composite path list too small for send scheduler rules");

bool scheduler_paused;
int32_t scheduler_max_samples;
int32_t scheduler_max_age;
bool send_sched_flush_on_update = true;

static struct k_work_delayable scheduler_age_work;
static bool scheduler_age_work_initialized;
static int32_t scheduler_accumulated_samples;

struct send_sched_rule_entry rule_entries[LWM2M_SEND_SCHED_RULES_MAX_INSTANCES];

static int send_sched_collect_paths(struct lwm2m_obj_path *paths, size_t max_paths);
static const struct lwm2m_obj_path *
send_sched_get_configured_path(struct send_sched_rule_entry *entry);
static int send_sched_find_rule_entry(const struct lwm2m_obj_path *path,
				      struct lwm2m_obj_path *parsed_path);
void send_sched_pmax_work_handler(struct k_work *work);
static void send_sched_record_cached_sample(void);
static void send_sched_maybe_flush_on_full(struct send_sched_rule_entry *entry);
static void send_sched_ensure_age_work_initialized(void);
static bool send_sched_find_oldest_timestamp(time_t *out_ts);
static void send_sched_age_work_handler(struct k_work *work);
static void send_sched_clear_cached_rules(struct send_sched_rule_entry *entry);
static void send_sched_refresh_cached_rules(struct send_sched_rule_entry *entry);

static void send_sched_log_decision(const char *verb, const char *path_str, const char *reason)
{
	LOG_DBG("%s %s: %s", verb, path_str, reason);
}

/* Compare two LwM2M paths for equality */
static bool send_sched_paths_equal(const struct lwm2m_obj_path *lhs,
				   const struct lwm2m_obj_path *rhs)
{
	return lhs->obj_id == rhs->obj_id && lhs->obj_inst_id == rhs->obj_inst_id &&
	       lhs->res_id == rhs->res_id && lhs->res_inst_id == rhs->res_inst_id &&
	       lhs->level == rhs->level;
}

/* Parse a textual object path into a LwM2M obj path structure */
int send_sched_parse_path(const char *path, struct lwm2m_obj_path *out)
{
	const char *cursor = path;
	unsigned long segments[3];
	char *end = NULL;

	if (!path || path[0] != '/') {
		return -EINVAL;
	}

	cursor++;

	for (int idx = 0; idx < ARRAY_SIZE(segments); idx++) {
		unsigned long value;

		if (*cursor == '\0') {
			return -EINVAL;
		}

		errno = 0;
		value = strtoul(cursor, &end, 10);
		if (errno == ERANGE || value > UINT16_MAX) {
			return -ERANGE;
		}

		if (end == cursor) {
			return -EINVAL;
		}

		segments[idx] = value;

		if (idx < (ARRAY_SIZE(segments) - 1)) {
			if (*end != '/') {
				return -EINVAL;
			}

			cursor = end + 1;
		} else if (*end != '\0') {
			return -EINVAL;
		}
	}

	*out = LWM2M_OBJ((uint16_t)segments[0], (uint16_t)segments[1], (uint16_t)segments[2]);

	return 0;
}

/* Gather unique rule paths into the provided array */
static int send_sched_collect_paths(struct lwm2m_obj_path *paths, size_t max_paths)
{
	int count = 0;

	if (!paths || max_paths == 0U) {
		return 0;
	}

	for (int idx = 0; idx < LWM2M_SEND_SCHED_RULES_MAX_INSTANCES; idx++) {
		const struct lwm2m_obj_path *candidate;
		bool duplicate = false;

		if (count >= (int)max_paths) {
			LOG_WRN("Flush path list full (%zu entries)", max_paths);
			break;
		}

		if (rule_entries[idx].path[0] == '\0') {
			continue;
		}

		candidate = send_sched_get_configured_path(&rule_entries[idx]);
		if (!candidate) {
			LOG_WRN("Skipping invalid rule path '%s'", rule_entries[idx].path);
			continue;
		}

		struct lwm2m_time_series_resource *cache_entry =
			lwm2m_cache_entry_get_by_object(candidate);

		if (!cache_entry || lwm2m_cache_size(cache_entry) == 0) {
			LOG_DBG("No cached samples for %s, skipping", rule_entries[idx].path);
			continue;
		}

		for (int j = 0; j < count; j++) {
			if (send_sched_paths_equal(&paths[j], candidate)) {
				duplicate = true;
				break;
			}
		}

		if (duplicate) {
			continue;
		}

		paths[count++] = *candidate;
	}

	return count;
}

/* Locate the configured path for a rule entry (parsing on demand) */
static const struct lwm2m_obj_path *
send_sched_get_configured_path(struct send_sched_rule_entry *entry)
{
	struct lwm2m_obj_path parsed;

	if (!entry || entry->path[0] == '\0') {
		return NULL;
	}

	if (entry->has_configured_path) {
		return &entry->configured_path;
	}

	if (send_sched_parse_path(entry->path, &parsed) < 0) {
		return NULL;
	}

	entry->configured_path = parsed;
	entry->has_configured_path = true;

	return &entry->configured_path;
}

/* Locate the rule entry matching the given path */
static int send_sched_find_rule_entry(const struct lwm2m_obj_path *path,
				      struct lwm2m_obj_path *parsed_path)
{
	for (int idx = 0; idx < LWM2M_SEND_SCHED_RULES_MAX_INSTANCES; idx++) {
		struct send_sched_rule_entry *entry = &rule_entries[idx];
		const struct lwm2m_obj_path *candidate;

		if (entry->path[0] == '\0') {
			continue;
		}

		candidate = send_sched_get_configured_path(entry);
		if (!candidate) {
			continue;
		}

		if (send_sched_paths_equal(path, candidate)) {
			if (parsed_path) {
				*parsed_path = *candidate;
			}
			return idx;
		}
	}

	return -ENOENT;
}

/* Extract a floating-point value from a rule string */
static bool send_sched_rule_parse_double(const char *rule, const char *attr, double *out_value)
{
	size_t attr_len;
	char *end = NULL;
	double value;

	if (!rule || !attr || !out_value) {
		return false;
	}

	attr_len = strlen(attr);
	if (strncmp(rule, attr, attr_len) != 0) {
		return false;
	}

	if (rule[attr_len] != '=') {
		return false;
	}

	errno = 0;
	value = strtod(&rule[attr_len + 1], &end);
	if (errno == ERANGE) {
		return false;
	}

	if (end == &rule[attr_len + 1] || (end && *end != '\0')) {
		return false;
	}

	*out_value = value;

	return true;
}

/* Extract an integer value from a rule string */
bool send_sched_rule_parse_int(const char *rule, const char *attr, int32_t *out_value)
{
	size_t attr_len;
	char *end = NULL;
	long value;

	if (!rule || !attr || !out_value) {
		return false;
	}

	attr_len = strlen(attr);
	if (strncmp(rule, attr, attr_len) != 0) {
		return false;
	}

	if (rule[attr_len] != '=') {
		return false;
	}

	errno = 0;
	value = strtol(&rule[attr_len + 1], &end, 10);
	if (errno == ERANGE || value < INT32_MIN || value > INT32_MAX) {
		return false;
	}

	if (end == &rule[attr_len + 1] || (end && *end != '\0')) {
		return false;
	}

	*out_value = (int32_t)value;

	return true;
}

/* Cancel any pending pmax timer */
void send_sched_cancel_pmax_timer(struct send_sched_rule_entry *entry)
{
	if (!entry) {
		return;
	}

	if (entry->pmax_timer_active) {
		(void)k_work_cancel_delayable(&entry->pmax_work);
		entry->pmax_timer_active = false;
	}
}

/* Arm (or re-arm) the pmax timer if configured */
static void send_sched_arm_pmax_timer(struct send_sched_rule_entry *entry)
{
	if (!entry) {
		return;
	}

	if (entry->pmax_seconds <= 0) {
		send_sched_cancel_pmax_timer(entry);
		return;
	}

	send_sched_cancel_pmax_timer(entry);

	int64_t now_ms = k_uptime_get();
	int64_t deadline_ms = entry->pmax_deadline_ms;
	int64_t required_ms = (int64_t)entry->pmax_seconds * 1000LL;

	if (deadline_ms <= 0) {
		deadline_ms = now_ms + required_ms;
		entry->pmax_deadline_ms = deadline_ms;
	}

	int64_t delay_ms = deadline_ms - now_ms;

	if (delay_ms < 0) {
		delay_ms = 0;
	}

	k_timeout_t timeout = K_MSEC((uint32_t)MIN(delay_ms, INT32_MAX));

	if (k_work_schedule(&entry->pmax_work, timeout) < 0) {
		LOG_WRN("Failed to schedule pmax timer for %s", entry->path);
		entry->pmax_timer_active = false;
		return;
	}

	entry->pmax_timer_active = true;
}

/* Work handler that forces a SEND when pmax expires */
void send_sched_pmax_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = CONTAINER_OF(work, struct k_work_delayable, work);
	struct send_sched_rule_entry *entry =
		CONTAINER_OF(dwork, struct send_sched_rule_entry, pmax_work);
	struct lwm2m_obj_path path;
	struct lwm2m_time_series_resource *cache_entry;
	int ret;
	int64_t now_ms;
	char path_buf[LWM2M_MAX_PATH_STR_SIZE];

	entry->pmax_timer_active = false;

	if (entry->path[0] == '\0') {
		return;
	}

	ret = send_sched_parse_path(entry->path, &path);
	if (ret < 0) {
		LOG_WRN("Skipping pmax cache refresh for invalid path '%s' (%d)", entry->path, ret);
		return;
	}

	cache_entry = lwm2m_cache_entry_get_by_object(&path);
	if (!cache_entry) {
		LOG_WRN("No cache entry available for %s when pmax expired", entry->path);
		return;
	}

	now_ms = k_uptime_get();

	if (entry->has_last_reported) {
		struct lwm2m_time_series_elem elem = entry->last_reported;
		const char *path_str = lwm2m_path_log_buf(path_buf, &path);
		char reason[64];
		time_t ts = time(NULL);

		if (ts <= 0) {
			LOG_WRN("time() unavailable for pmax cache refresh on %s", entry->path);
		} else {
			elem.t = ts;
		}

		if (!lwm2m_cache_write(cache_entry, &elem)) {
			LOG_WRN("Failed to append cached sample for %s on pmax expiry",
				entry->path);
		} else {
			entry->last_reported = elem;
			entry->has_last_reported = true;
			snprintk(reason, sizeof(reason), "pmax %d expired (cached)",
				 entry->pmax_seconds);
			send_sched_log_decision("Cache", path_str, reason);
			send_sched_schedule_age_check();
		}
	} else {
		LOG_DBG("pmax timer fired for %s before any sample cached", entry->path);
	}

	entry->last_accept_ms = now_ms;
	entry->has_last_accept_ms = true;

	if (entry->has_pmin && entry->pmin_seconds > 0) {
		entry->pmin_deadline_ms = now_ms + ((int64_t)entry->pmin_seconds * 1000LL);
		entry->pmin_waiting = false;
	} else {
		entry->pmin_waiting = false;
		entry->pmin_deadline_ms = 0;
	}

	if (entry->pmax_seconds > 0) {
		entry->pmax_deadline_ms = now_ms + ((int64_t)entry->pmax_seconds * 1000LL);
		send_sched_arm_pmax_timer(entry);
	} else {
		entry->pmax_deadline_ms = 0;
	}
}

/* Trigger a composite SEND for cached resources */
int send_sched_flush_all(void)
{
	struct lwm2m_ctx *ctx;
	struct lwm2m_obj_path path_list[LWM2M_SEND_SCHED_RULES_MAX_INSTANCES];
	int path_count;
	int ret;

	ctx = lwm2m_rd_client_ctx();
	if (!ctx) {
		LOG_WRN("Cannot flush caches: LwM2M context unavailable");
		return -ENODEV;
	}

	path_count = send_sched_collect_paths(path_list, ARRAY_SIZE(path_list));
	if (path_count <= 0) {
		LOG_WRN("No cached resources registered for flush");
		return -ENOENT;
	}

	if (path_count > CONFIG_LWM2M_COMPOSITE_PATH_LIST_SIZE) {
		LOG_WRN("Limiting flush to %d path(s)", CONFIG_LWM2M_COMPOSITE_PATH_LIST_SIZE);
		path_count = CONFIG_LWM2M_COMPOSITE_PATH_LIST_SIZE;
	}

	ret = lwm2m_send_cb(ctx, path_list, (uint8_t)path_count, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to flush cached data (%d)", ret);
	} else {
		LOG_INF("Triggered LwM2M send for %d cached path(s)", path_count);
	}

	send_sched_reset_accumulated_samples();
	send_sched_schedule_age_check();

	return ret;
}

void send_sched_reset_accumulated_samples(void)
{
	scheduler_accumulated_samples = 0;
}

void send_sched_enforce_max_sample_limit(void)
{
	if (scheduler_max_samples <= 0) {
		return;
	}

	if (scheduler_accumulated_samples >= scheduler_max_samples) {
		LOG_INF("Accumulated %d samples (limit %d), forcing SEND",
			scheduler_accumulated_samples, scheduler_max_samples);
		scheduler_accumulated_samples = 0;
		(void)send_sched_flush_all();
	}
}

static void send_sched_record_cached_sample(void)
{
	if (scheduler_accumulated_samples < INT32_MAX) {
		scheduler_accumulated_samples++;
	}

	send_sched_enforce_max_sample_limit();
}

static void send_sched_maybe_flush_on_full(struct send_sched_rule_entry *entry)
{
	int slots;

	if (!entry || !entry->has_cached_path) {
		return;
	}

	slots = lwm2m_cache_free_slots_get(&entry->cached_path);
	if (slots < 0) {
		/* No cache entry or API failure, nothing to do */
		return;
	}

	if (slots == 0) {
		LOG_DBG("Cache full for %s, triggering global SEND", entry->path);
		(void)send_sched_flush_all();
	}
}

static void send_sched_ensure_age_work_initialized(void)
{
	if (!scheduler_age_work_initialized) {
		k_work_init_delayable(&scheduler_age_work, send_sched_age_work_handler);
		scheduler_age_work_initialized = true;
	}
}

static bool send_sched_find_oldest_timestamp(time_t *out_ts)
{
	bool found = false;
	time_t oldest = 0;

	for (int idx = 0; idx < LWM2M_SEND_SCHED_RULES_MAX_INSTANCES; idx++) {
		struct send_sched_rule_entry *entry = &rule_entries[idx];
		struct lwm2m_obj_path path;
		struct lwm2m_time_series_resource *cache_entry;
		struct lwm2m_time_series_elem elem;

		if (entry->path[0] == '\0') {
			continue;
		}

		if (entry->has_cached_path) {
			path = entry->cached_path;
		} else if (send_sched_parse_path(entry->path, &path) < 0) {
			continue;
		}

		cache_entry = lwm2m_cache_entry_get_by_object(&path);
		if (!cache_entry || ring_buf_is_empty(&cache_entry->rb)) {
			continue;
		}

		if (ring_buf_peek(&cache_entry->rb, (uint8_t *)&elem, sizeof(elem)) !=
		    sizeof(elem)) {
			continue;
		}

		if (!found || elem.t < oldest) {
			oldest = elem.t;
			found = true;
		}
	}

	if (found && out_ts) {
		*out_ts = oldest;
	}

	return found;
}

void send_sched_process_max_age(bool allow_flush)
{
	if (scheduler_max_age <= 0) {
		if (scheduler_age_work_initialized) {
			k_work_cancel_delayable(&scheduler_age_work);
		}
		return;
	}

	send_sched_ensure_age_work_initialized();

	time_t now = time(NULL);
	time_t oldest_ts;

	if (now <= 0 || !send_sched_find_oldest_timestamp(&oldest_ts)) {
		if (scheduler_age_work_initialized) {
			k_work_cancel_delayable(&scheduler_age_work);
		}
		return;
	}

	int64_t age = (int64_t)now - (int64_t)oldest_ts;

	if (age < 0) {
		age = 0;
	}

	if (allow_flush && age >= scheduler_max_age) {
		LOG_INF("Oldest cached sample age %llds exceeds max %ds, forcing SEND",
			(long long)age, scheduler_max_age);
		(void)send_sched_flush_all();
		send_sched_schedule_age_check();
		return;
	}

	int64_t remaining = (int64_t)scheduler_max_age - age;

	if (remaining < 1) {
		remaining = 1;
	}

	int64_t delay_ms = remaining * 1000LL;

	if (delay_ms > INT32_MAX) {
		delay_ms = INT32_MAX;
	}

	k_work_reschedule(&scheduler_age_work, K_MSEC((uint32_t)delay_ms));
}

void send_sched_schedule_age_check(void)
{
	if (scheduler_max_age <= 0) {
		if (scheduler_age_work_initialized) {
			k_work_cancel_delayable(&scheduler_age_work);
		}
		return;
	}

	send_sched_process_max_age(false);
}

static void send_sched_age_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	send_sched_process_max_age(true);
}

static void send_sched_clear_cached_rules(struct send_sched_rule_entry *entry)
{
	entry->has_rule_gt = false;
	entry->has_rule_lt = false;
	entry->has_rule_st = false;
	entry->has_rule_pmin = false;
	entry->has_rule_pmax = false;
	entry->rule_gt_value = 0.0;
	entry->rule_lt_value = 0.0;
	entry->rule_st_value = 0.0;
	entry->rule_pmin_seconds = 0;
	entry->rule_pmax_seconds = 0;
	entry->rules_dirty = false;
}

static void send_sched_refresh_cached_rules(struct send_sched_rule_entry *entry)
{
	send_sched_clear_cached_rules(entry);

	for (int idx = 0; idx < LWM2M_SEND_SCHED_MAX_RULE_STRINGS; idx++) {
		const char *rule = entry->rules[idx];

		if (rule[0] == '\0') {
			continue;
		}

		if (!entry->has_rule_gt &&
		    send_sched_rule_parse_double(rule, "gt", &entry->rule_gt_value)) {
			entry->has_rule_gt = true;
			continue;
		}

		if (!entry->has_rule_lt &&
		    send_sched_rule_parse_double(rule, "lt", &entry->rule_lt_value)) {
			entry->has_rule_lt = true;
			continue;
		}

		if (!entry->has_rule_st &&
		    send_sched_rule_parse_double(rule, "st", &entry->rule_st_value)) {
			entry->has_rule_st = true;
			continue;
		}

		if (!entry->has_rule_pmin &&
		    send_sched_rule_parse_int(rule, "pmin", &entry->rule_pmin_seconds)) {
			if (entry->rule_pmin_seconds < 0) {
				entry->rule_pmin_seconds = 0;
			}
			entry->has_rule_pmin = true;
			continue;
		}

		if (!entry->has_rule_pmax &&
		    send_sched_rule_parse_int(rule, "pmax", &entry->rule_pmax_seconds)) {
			if (entry->rule_pmax_seconds < 0) {
				entry->rule_pmax_seconds = 0;
			}
			entry->has_rule_pmax = true;
			continue;
		}
	}

	entry->rules_dirty = false;
}

void lwm2m_send_sched_handle_registration_event(void)
{
	if (!send_sched_flush_on_update) {
		return;
	}

	(void)send_sched_flush_all();
}

/* Decide whether a sample should be cached for the configured path */
bool lwm2m_send_sched_cache_filter(const struct lwm2m_obj_path *path,
				   const struct lwm2m_time_series_elem *element)
{
	int entry_idx;
	struct lwm2m_obj_path entry_path;
	struct send_sched_rule_entry *entry;
	double sample_value;
	bool trigger = false;
	bool trigger_due_to_pmin_expiry = false;
	char path_buf[LWM2M_MAX_PATH_STR_SIZE];
	struct lwm2m_obj_path path_copy;
	const char *path_str = "unknown";
	char keep_reason[96] = {0};
	char drop_reason[96] = {0};
	bool drop_reason_set = false;
	int64_t now_ms;

	if (!path || !element) {
		return true;
	}

	path_copy = *path;
	path_str = lwm2m_path_log_buf(path_buf, &path_copy);
	sample_value = element->f;
	now_ms = k_uptime_get();

	if (scheduler_paused) {
		send_sched_log_decision("Drop", path_str, "scheduler paused");
		return false;
	}

	entry_idx = send_sched_find_rule_entry(path, &entry_path);
	if (entry_idx < 0) {
		send_sched_log_decision("Drop", path_str, "no rule entry");
		return false;
	}

	entry = &rule_entries[entry_idx];

	if (!entry->has_cached_path || !send_sched_paths_equal(&entry->cached_path, &entry_path)) {
		entry->cached_path = entry_path;
		entry->has_cached_path = true;
		entry->has_last_reported = false;
		entry->has_last_observed = false;
	}

	if (entry->rules_dirty) {
		send_sched_refresh_cached_rules(entry);
	}

	bool has_gt = entry->has_rule_gt;
	bool has_lt = entry->has_rule_lt;
	bool has_st = entry->has_rule_st;
	bool has_pmin_rule = entry->has_rule_pmin;
	bool has_pmax_rule = entry->has_rule_pmax;
	double gt_value = entry->rule_gt_value;
	double lt_value = entry->rule_lt_value;
	double st_value = entry->rule_st_value;
	int32_t pmin_seconds = entry->rule_pmin_seconds;
	int32_t pmax_seconds = entry->rule_pmax_seconds;

	bool effective_has_pmin = (has_pmin_rule && pmin_seconds > 0);
	bool effective_has_pmax = (has_pmax_rule && pmax_seconds > 0);
	int64_t pmin_required_ms = 0;
	int64_t pmax_required_ms = 0;

	if (effective_has_pmin) {
		pmin_required_ms = (int64_t)pmin_seconds * 1000LL;
	}

	if (effective_has_pmax && effective_has_pmin && pmax_seconds <= pmin_seconds) {
		LOG_WRN("Ignoring pmax <= pmin for path %s", entry->path);
		effective_has_pmax = false;
	}

	entry->has_pmin = effective_has_pmin;
	entry->pmin_seconds = effective_has_pmin ? pmin_seconds : 0;
	if (!effective_has_pmin) {
		entry->pmin_waiting = false;
		entry->pmin_deadline_ms = 0;
	}

	entry->pmax_seconds = effective_has_pmax ? pmax_seconds : 0;
	if (effective_has_pmax) {
		pmax_required_ms = (int64_t)pmax_seconds * 1000LL;
		if (entry->has_last_accept_ms) {
			entry->pmax_deadline_ms = entry->last_accept_ms + pmax_required_ms;
		} else if (entry->pmax_deadline_ms == 0) {
			entry->pmax_deadline_ms = now_ms + pmax_required_ms;
		}
		send_sched_arm_pmax_timer(entry);
	} else {
		send_sched_cancel_pmax_timer(entry);
		entry->pmax_deadline_ms = 0;
	}

	if (effective_has_pmin && entry->pmin_waiting && now_ms >= entry->pmin_deadline_ms) {
		trigger = true;
		trigger_due_to_pmin_expiry = true;
		entry->pmin_waiting = false;
		snprintk(keep_reason, sizeof(keep_reason), "pmin %d expired", pmin_seconds);
	}

	if (effective_has_pmax && entry->pmax_deadline_ms > 0 &&
	    now_ms >= entry->pmax_deadline_ms) {
		trigger = true;
		if (keep_reason[0] == '\0') {
			snprintk(keep_reason, sizeof(keep_reason), "pmax %d expired", pmax_seconds);
		}
	}

	if (!has_gt && !has_lt && !has_st) {
		trigger = true;
		snprintk(keep_reason, sizeof(keep_reason), "no threshold rules configured");
	}

	if (has_gt) {
		if (sample_value > gt_value) {
			if (!entry->has_last_observed || entry->last_observed <= gt_value) {
				trigger = true;
				snprintk(keep_reason, sizeof(keep_reason), "crossed gt threshold");
			} else {
				snprintk(drop_reason, sizeof(drop_reason),
					 "above gt threshold but already above");
				drop_reason_set = true;
			}
		}
	}

	if (!trigger && has_lt) {
		if (sample_value < lt_value) {
			if (!entry->has_last_observed || entry->last_observed >= lt_value) {
				trigger = true;
				snprintk(keep_reason, sizeof(keep_reason), "crossed lt threshold");
			} else if (!drop_reason_set) {
				snprintk(drop_reason, sizeof(drop_reason),
					 "below lt threshold but already below");
				drop_reason_set = true;
			}
		}
	}

	if (!trigger && has_st) {
		if (!entry->has_last_reported) {
			trigger = true;
			snprintk(keep_reason, sizeof(keep_reason), "no prior sample, st rule set");
		} else {
			double delta = sample_value - entry->last_reported.f;

			if (delta < 0.0) {
				delta = -delta;
			}

			if (delta >= st_value) {
				trigger = true;
				snprintk(keep_reason, sizeof(keep_reason),
					 "delta exceeded st threshold");
			} else if (!drop_reason_set) {
				snprintk(drop_reason, sizeof(drop_reason),
					 "delta below st threshold");
				drop_reason_set = true;
			}
		}
	}

	entry->last_observed = sample_value;
	entry->has_last_observed = true;

	if (!trigger) {
		if (!drop_reason_set) {
			snprintk(drop_reason, sizeof(drop_reason), "no rule triggered");
		}
		send_sched_log_decision("Drop", path_str, drop_reason);
		return false;
	}

	if (effective_has_pmin && entry->has_last_accept_ms && !trigger_due_to_pmin_expiry) {
		int64_t elapsed_ms = now_ms - entry->last_accept_ms;

		if (elapsed_ms < pmin_required_ms) {
			int64_t remaining_ms = pmin_required_ms - elapsed_ms;

			entry->pmin_waiting = true;
			entry->pmin_deadline_ms = entry->last_accept_ms + pmin_required_ms;

			snprintk(drop_reason, sizeof(drop_reason),
				 "pmin %d active (%lld ms remaining)", pmin_seconds,
				 (long long)remaining_ms);
			send_sched_log_decision("Defer", path_str, drop_reason);
			return false;
		}
	}

	entry->last_reported = *element;
	entry->has_last_reported = true;
	entry->last_accept_ms = now_ms;
	entry->has_last_accept_ms = true;

	if (effective_has_pmin) {
		entry->pmin_deadline_ms = entry->last_accept_ms + pmin_required_ms;
		entry->pmin_waiting = false;
	} else {
		entry->pmin_waiting = false;
		entry->pmin_deadline_ms = 0;
	}

	if (entry->pmax_seconds > 0) {
		entry->pmax_deadline_ms =
			entry->last_accept_ms + ((int64_t)entry->pmax_seconds * 1000LL);
		send_sched_arm_pmax_timer(entry);
	} else {
		send_sched_cancel_pmax_timer(entry);
		entry->pmax_deadline_ms = 0;
	}

	if (keep_reason[0] == '\0') {
		snprintk(keep_reason, sizeof(keep_reason), "rule triggered");
	}

	send_sched_log_decision("Keep", path_str, keep_reason);
	send_sched_record_cached_sample();
	send_sched_maybe_flush_on_full(entry);
	send_sched_schedule_age_check();

	return true;
}
