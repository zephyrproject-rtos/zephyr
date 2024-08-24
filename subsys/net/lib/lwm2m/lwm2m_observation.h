/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef LWM2M_OBSERVATION_H
#define LWM2M_OBSERVATION_H
#include "lwm2m_object.h"

int lwm2m_notify_observer(uint16_t obj_id, uint16_t obj_inst_id, uint16_t res_id);
int lwm2m_notify_observer_path(const struct lwm2m_obj_path *path);

#define MAX_TOKEN_LEN 8

struct observe_node {
	sys_snode_t node;
	sys_slist_t path_list;               /* List of Observation path */
	uint8_t token[MAX_TOKEN_LEN];        /* Observation Token */
	int64_t event_timestamp;             /* Timestamp for trig next Notify  */
	int64_t last_timestamp;	             /* Timestamp from last Notify */
	struct lwm2m_message *active_notify; /* Currently active notification */
	uint32_t counter;
	uint16_t format;
	uint8_t tkl;
	bool resource_update : 1;            /* Resource is updated */
	bool composite : 1;                  /* Composite Observation */
};
/* Attribute handling. */

struct lwm2m_attr *lwm2m_engine_get_next_attr(const void *ref, struct lwm2m_attr *prev);
const char *lwm2m_engine_get_attr_name(const struct lwm2m_attr *attr);

const char *const lwm2m_attr_to_str(uint8_t type);

void clear_attrs(void *ref);

int64_t engine_observe_shedule_next_event(struct observe_node *obs, uint16_t srv_obj_inst,
					  const int64_t timestamp);

void remove_observer_from_list(struct lwm2m_ctx *ctx, sys_snode_t *prev_node,
			       struct observe_node *obs);

struct observe_node *engine_observe_node_discover(sys_slist_t *observe_node_list,
						  sys_snode_t **prev_node,
						  sys_slist_t *lwm2m_path_list,
						  const uint8_t *token, uint8_t tkl);

int engine_remove_observer_by_token(struct lwm2m_ctx *ctx, const uint8_t *token, uint8_t tkl);

int lwm2m_write_attr_handler(struct lwm2m_engine_obj *obj, struct lwm2m_message *msg);

int lwm2m_engine_observation_handler(struct lwm2m_message *msg, int observe, uint16_t accept,
				     bool composite);

void engine_remove_observer_by_id(uint16_t obj_id, int32_t obj_inst_id);

/* path object list */
struct lwm2m_obj_path_list {
	sys_snode_t node;
	struct lwm2m_obj_path path;
};
/* Initialize path list */
void lwm2m_engine_path_list_init(sys_slist_t *lwm2m_path_list, sys_slist_t *lwm2m_free_list,
				 struct lwm2m_obj_path_list path_object_buf[],
				 uint8_t path_object_size);
/**
 * Add new path to the list while maintaining hierarchical sort order
 *
 * @param lwm2m_path_list sorted path list
 * @param lwm2m_free_list free list
 * @param path path to be added
 * @return 0 on success or a negative error code
 */
int lwm2m_engine_add_path_to_list(sys_slist_t *lwm2m_path_list, sys_slist_t *lwm2m_free_list,
				  const struct lwm2m_obj_path *path);

int lwm2m_get_path_reference_ptr(struct lwm2m_engine_obj *obj, const struct lwm2m_obj_path *path,
				 void **ref);

/**
 * Remove paths when parent already exists in the list
 *
 * @note Path list must be sorted
 * @see lwm2m_engine_add_path_to_list()
 *
 * @param lwm2m_path_list sorted path list
 * @param lwm2m_free_list free list
 */
void lwm2m_engine_clear_duplicate_path(sys_slist_t *lwm2m_path_list, sys_slist_t *lwm2m_free_list);

/* Resources */
sys_slist_t *lwm2m_obs_obj_path_list(void);
#endif /* LWM2M_OBSERVATION_H */
