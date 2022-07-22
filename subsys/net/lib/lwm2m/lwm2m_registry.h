/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef LWM2M_REGISTRY_H
#define LWM2M_REGISTRY_H
#include "lwm2m_object.h"
int path_to_objs(const struct lwm2m_obj_path *path, struct lwm2m_engine_obj_inst **obj_inst,
		 struct lwm2m_engine_obj_field **obj_field, struct lwm2m_engine_res **res,
		 struct lwm2m_engine_res_inst **res_inst);

struct lwm2m_engine_obj_field *lwm2m_get_engine_obj_field(struct lwm2m_engine_obj *obj, int res_id);
int lwm2m_create_obj_inst(uint16_t obj_id, uint16_t obj_inst_id,
			  struct lwm2m_engine_obj_inst **obj_inst);
int lwm2m_delete_obj_inst(uint16_t obj_id, uint16_t obj_inst_id);
int lwm2m_get_or_create_engine_obj(struct lwm2m_message *msg,
				   struct lwm2m_engine_obj_inst **obj_inst, uint8_t *created);

void lwm2m_register_obj(struct lwm2m_engine_obj *obj);
void lwm2m_unregister_obj(struct lwm2m_engine_obj *obj);

/* Create or Allocate resource instance. */
int lwm2m_engine_get_create_res_inst(struct lwm2m_obj_path *path, struct lwm2m_engine_res **res,
				     struct lwm2m_engine_res_inst **res_inst);

struct lwm2m_engine_obj *lwm2m_engine_get_obj(const struct lwm2m_obj_path *path);
struct lwm2m_engine_obj_inst *lwm2m_engine_get_obj_inst(const struct lwm2m_obj_path *path);
struct lwm2m_engine_res *lwm2m_engine_get_res(const struct lwm2m_obj_path *path);
struct lwm2m_engine_res_inst *lwm2m_engine_get_res_inst(const struct lwm2m_obj_path *path);

struct lwm2m_engine_obj *get_engine_obj(int obj_id);
struct lwm2m_engine_obj_inst *get_engine_obj_inst(int obj_id, int obj_inst_id);

struct lwm2m_engine_obj_inst *next_engine_obj_inst(int obj_id, int obj_inst_id);

bool lwm2m_engine_shall_report_obj_version(const struct lwm2m_engine_obj *obj);

int lwm2m_engine_get_resource(const char *pathstr, struct lwm2m_engine_res **res);

void lwm2m_engine_get_binding(char *binding);

void lwm2m_engine_get_queue_mode(char *queue);

size_t lwm2m_engine_get_opaque_more(struct lwm2m_input_context *in, uint8_t *buf, size_t buflen,
				    struct lwm2m_opaque_context *opaque, bool *last_block);

/* Resources */
sys_slist_t *lwm2m_engine_obj_list(void);
sys_slist_t *lwm2m_engine_obj_inst_list(void);
#endif /* LWM2M_REGISTRY_H */
