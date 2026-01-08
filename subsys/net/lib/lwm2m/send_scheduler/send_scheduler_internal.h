/*
 * Copyright (c) 2025 Clunky Machines
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LWM2M_SEND_SCHEDULER_INTERNAL_H_
#define ZEPHYR_LWM2M_SEND_SCHEDULER_INTERNAL_H_

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/net/lwm2m.h>

/* Provide a small buffer for rule definitions that the engine can mutate */
#define LWM2M_SEND_SCHED_MAX_RULE_STRINGS    4
#define LWM2M_SEND_SCHED_RULE_STRING_SIZE    64
#define LWM2M_SEND_SCHED_RULES_MAX_INSTANCES 4

/* Aggregated bookkeeping for one scheduler rule instance */
struct send_sched_rule_entry {
	char path[LWM2M_SEND_SCHED_RULE_STRING_SIZE]; /* LwM2M path string (/obj/inst/res) */
	struct lwm2m_obj_path cached_path;            /* Parsed path for cache lookups */
	bool has_cached_path;                         /* Guard for cached_path validity */
	struct lwm2m_obj_path configured_path;        /* Parsed resource path from */
						      /* /20001/X/0 */
	bool has_configured_path;                     /* Guard for configured_path validity */
	char rules[LWM2M_SEND_SCHED_MAX_RULE_STRINGS][LWM2M_SEND_SCHED_RULE_STRING_SIZE];
	/* Raw rule strings (gt/lt/st/pmin/pmax) */
	double last_observed;                        /* Most recent sample seen, even if dropped */
	bool has_last_observed;                      /* Guard for last_observed validity */
	struct lwm2m_time_series_elem last_reported; /* Last sample committed */
	bool has_last_reported;                      /* Guard for last_reported validity */
	int64_t last_accept_ms;            /* Monotonic timestamp (ms) of last accepted sample */
	bool has_last_accept_ms;           /* Guard for last_accept_ms */
	int64_t pmin_deadline_ms;          /* Next time pmin allows a sample */
	bool pmin_waiting;                 /* True when we are deferring because of pmin */
	bool has_pmin;                     /* pmin is configured (>0) */
	int32_t pmin_seconds;              /* Cached pmin seconds value */
	int64_t pmax_deadline_ms;          /* Next time pmax requires a cached refresh */
	int32_t pmax_seconds;              /* Cached pmax seconds value */
	struct k_work_delayable pmax_work; /* Work item used to enforce pmax */
	bool pmax_timer_active;            /* True if the pmax timer is scheduled */
	bool rules_dirty;                  /* True when parsed rule cache needs refresh */
	bool has_rule_gt;
	bool has_rule_lt;
	bool has_rule_st;
	bool has_rule_pmin;
	bool has_rule_pmax;
	double rule_gt_value;
	double rule_lt_value;
	double rule_st_value;
	int32_t rule_pmin_seconds;
	int32_t rule_pmax_seconds;
};

extern bool scheduler_paused;
extern int32_t scheduler_max_samples;
extern int32_t scheduler_max_age;
extern bool send_sched_flush_on_update;
extern struct send_sched_rule_entry rule_entries[LWM2M_SEND_SCHED_RULES_MAX_INSTANCES];

int send_sched_parse_path(const char *path, struct lwm2m_obj_path *out);
bool send_sched_rule_parse_int(const char *rule, const char *attr, int32_t *out_value);
void send_sched_cancel_pmax_timer(struct send_sched_rule_entry *entry);
void send_sched_pmax_work_handler(struct k_work *work);
int send_sched_flush_all(void);
void send_sched_schedule_age_check(void);
void send_sched_process_max_age(bool allow_flush);
void send_sched_enforce_max_sample_limit(void);
void send_sched_reset_accumulated_samples(void);
void lwm2m_send_sched_handle_registration_event(void);

#endif /* ZEPHYR_LWM2M_SEND_SCHEDULER_INTERNAL_H_ */
