/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2018-2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Uses some original concepts by:
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 *         Joel Hoglund <joel@sics.se>
 */

#define LOG_MODULE_NAME net_lwm2m_registry
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <fcntl.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/http_parser_url.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/lwm2m.h>
#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
#include <zephyr/net/tls_credentials.h>
#endif
#if defined(CONFIG_DNS_RESOLVER)
#include <zephyr/net/dns_resolve.h>
#endif

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include "lwm2m_rw_link_format.h"
#include "lwm2m_rw_plain_text.h"
#include "lwm2m_rw_oma_tlv.h"
#include "lwm2m_util.h"
#if defined(CONFIG_LWM2M_RW_SENML_JSON_SUPPORT)
#include "lwm2m_rw_senml_json.h"
#endif
#ifdef CONFIG_LWM2M_RW_JSON_SUPPORT
#include "lwm2m_rw_json.h"
#endif
#ifdef CONFIG_LWM2M_RW_CBOR_SUPPORT
#include "lwm2m_rw_cbor.h"
#endif
#ifdef CONFIG_LWM2M_RW_SENML_CBOR_SUPPORT
#include "lwm2m_rw_senml_cbor.h"
#endif
#ifdef CONFIG_LWM2M_RD_CLIENT_SUPPORT
#include "lwm2m_rd_client.h"
#endif
#define BINDING_OPT_MAX_LEN	3 /* "UQ" */
#define QUEUE_OPT_MAX_LEN	2 /* "Q" */


/* Resources */
static sys_slist_t engine_obj_list;
static sys_slist_t engine_obj_inst_list;
/* External resources */
/* Resource wrappers */
sys_slist_t *lwm2m_engine_obj_list(void)
{
	return &engine_obj_list;
}

sys_slist_t *lwm2m_engine_obj_inst_list(void)
{
	return &engine_obj_inst_list;
}

/* Forward declarations. */
void engine_remove_observer_by_id(uint16_t obj_id, int32_t obj_inst_id);

void clear_attrs(void *ref);
/* engine object */
void lwm2m_register_obj(struct lwm2m_engine_obj *obj)
{
	sys_slist_append(&engine_obj_list, &obj->node);
}

void lwm2m_unregister_obj(struct lwm2m_engine_obj *obj)
{
	engine_remove_observer_by_id(obj->obj_id, -1);
	sys_slist_find_and_remove(&engine_obj_list, &obj->node);
}

struct lwm2m_engine_obj *get_engine_obj(int obj_id)
{
	struct lwm2m_engine_obj *obj;

	SYS_SLIST_FOR_EACH_CONTAINER(&engine_obj_list, obj, node) {
		if (obj->obj_id == obj_id) {
			return obj;
		}
	}

	return NULL;
}

struct lwm2m_engine_obj_field *
lwm2m_get_engine_obj_field(struct lwm2m_engine_obj *obj, int res_id)
{
	int i;

	if (obj && obj->fields && obj->field_count > 0) {
		for (i = 0; i < obj->field_count; i++) {
			if (obj->fields[i].res_id == res_id) {
				return &obj->fields[i];
			}
		}
	}

	return NULL;
}

struct lwm2m_engine_obj *lwm2m_engine_get_obj(
					const struct lwm2m_obj_path *path)
{
	if (path->level < LWM2M_PATH_LEVEL_OBJECT) {
		return NULL;
	}

	return get_engine_obj(path->obj_id);
}

/* engine object instance */
static void engine_register_obj_inst(struct lwm2m_engine_obj_inst *obj_inst)
{
	sys_slist_append(&engine_obj_inst_list, &obj_inst->node);
}

static void engine_unregister_obj_inst(struct lwm2m_engine_obj_inst *obj_inst)
{
	engine_remove_observer_by_id(
			obj_inst->obj->obj_id, obj_inst->obj_inst_id);
	sys_slist_find_and_remove(&engine_obj_inst_list, &obj_inst->node);
}

struct lwm2m_engine_obj_inst *get_engine_obj_inst(int obj_id,
							 int obj_inst_id)
{
	struct lwm2m_engine_obj_inst *obj_inst;

	SYS_SLIST_FOR_EACH_CONTAINER(&engine_obj_inst_list, obj_inst,
				     node) {
		if (obj_inst->obj->obj_id == obj_id &&
		    obj_inst->obj_inst_id == obj_inst_id) {
			return obj_inst;
		}
	}

	return NULL;
}

struct lwm2m_engine_obj_inst *
next_engine_obj_inst(int obj_id, int obj_inst_id)
{
	struct lwm2m_engine_obj_inst *obj_inst, *next = NULL;

	SYS_SLIST_FOR_EACH_CONTAINER(&engine_obj_inst_list, obj_inst,
				     node) {
		if (obj_inst->obj->obj_id == obj_id &&
		    obj_inst->obj_inst_id > obj_inst_id &&
		    (!next || next->obj_inst_id > obj_inst->obj_inst_id)) {
			next = obj_inst;
		}
	}

	return next;
}

int lwm2m_create_obj_inst(uint16_t obj_id, uint16_t obj_inst_id,
			  struct lwm2m_engine_obj_inst **obj_inst)
{
	struct lwm2m_engine_obj *obj;
	int ret;

	*obj_inst = NULL;
	obj = get_engine_obj(obj_id);
	if (!obj) {
		LOG_ERR("unable to find obj: %u", obj_id);
		return -ENOENT;
	}

	if (!obj->create_cb) {
		LOG_ERR("obj %u has no create_cb", obj_id);
		return -EINVAL;
	}

	if (obj->instance_count + 1 > obj->max_instance_count) {
		LOG_ERR("no more instances available for obj %u", obj_id);
		return -ENOMEM;
	}

	*obj_inst = obj->create_cb(obj_inst_id);
	if (!*obj_inst) {
		LOG_ERR("unable to create obj %u instance %u",
			obj_id, obj_inst_id);
		/*
		 * Already checked for instance count total.
		 * This can only be an error if the object instance exists.
		 */
		return -EEXIST;
	}

	obj->instance_count++;
	(*obj_inst)->obj = obj;
	(*obj_inst)->obj_inst_id = obj_inst_id;
	engine_register_obj_inst(*obj_inst);

	if (obj->user_create_cb) {
		ret = obj->user_create_cb(obj_inst_id);
		if (ret < 0) {
			LOG_ERR("Error in user obj create %u/%u: %d",
				obj_id, obj_inst_id, ret);
			lwm2m_delete_obj_inst(obj_id, obj_inst_id);
			return ret;
		}
	}

	return 0;
}

int lwm2m_delete_obj_inst(uint16_t obj_id, uint16_t obj_inst_id)
{
	int i, ret = 0;
	struct lwm2m_engine_obj *obj;
	struct lwm2m_engine_obj_inst *obj_inst;

	obj = get_engine_obj(obj_id);
	if (!obj) {
		return -ENOENT;
	}

	obj_inst = get_engine_obj_inst(obj_id, obj_inst_id);
	if (!obj_inst) {
		return -ENOENT;
	}

	if (obj->user_delete_cb) {
		ret = obj->user_delete_cb(obj_inst_id);
		if (ret < 0) {
			LOG_ERR("Error in user obj delete %u/%u: %d",
				obj_id, obj_inst_id, ret);
			/* don't return error */
		}
	}

	engine_unregister_obj_inst(obj_inst);
	obj->instance_count--;

	if (obj->delete_cb) {
		ret = obj->delete_cb(obj_inst_id);
	}

	/* reset obj_inst and res_inst data structure */
	for (i = 0; i < obj_inst->resource_count; i++) {
		clear_attrs(&obj_inst->resources[i]);
		(void)memset(obj_inst->resources + i, 0,
			     sizeof(struct lwm2m_engine_res));
	}

	clear_attrs(obj_inst);
	(void)memset(obj_inst, 0, sizeof(struct lwm2m_engine_obj_inst));
	return ret;
}

int path_to_objs(const struct lwm2m_obj_path *path,
			struct lwm2m_engine_obj_inst **obj_inst,
			struct lwm2m_engine_obj_field **obj_field,
			struct lwm2m_engine_res **res,
			struct lwm2m_engine_res_inst **res_inst)
{
	struct lwm2m_engine_obj_inst *oi;
	struct lwm2m_engine_obj_field *of;
	struct lwm2m_engine_res *r = NULL;
	struct lwm2m_engine_res_inst *ri = NULL;
	int i;

	if (!path) {
		return -EINVAL;
	}

	oi = get_engine_obj_inst(path->obj_id, path->obj_inst_id);
	if (!oi) {
		LOG_ERR("obj instance %d/%d not found",
			path->obj_id, path->obj_inst_id);
		return -ENOENT;
	}

	if (!oi->resources || oi->resource_count == 0U) {
		LOG_ERR("obj instance has no resources");
		return -EINVAL;
	}

	of = lwm2m_get_engine_obj_field(oi->obj, path->res_id);
	if (!of) {
		LOG_ERR("obj field %d not found", path->res_id);
		return -ENOENT;
	}

	for (i = 0; i < oi->resource_count; i++) {
		if (oi->resources[i].res_id == path->res_id) {
			r = &oi->resources[i];
			break;
		}
	}

	if (!r) {
		if (LWM2M_HAS_PERM(of, BIT(LWM2M_FLAG_OPTIONAL))) {
			LOG_DBG("resource %d not found", path->res_id);
		} else {
			LOG_ERR("resource %d not found", path->res_id);
		}

		return -ENOENT;
	}

	for (i = 0; i < r->res_inst_count; i++) {
		if (r->res_instances[i].res_inst_id == path->res_inst_id) {
			ri = &r->res_instances[i];
			break;
		}
	}

	/* specifically don't complain about missing resource instance */

	if (obj_inst) {
		*obj_inst = oi;
	}

	if (obj_field) {
		*obj_field = of;
	}

	if (res) {
		*res = r;
	}

	if (ri && res_inst) {
		*res_inst = ri;
	}

	return 0;
}

int lwm2m_engine_create_obj_inst(const char *pathstr)
{
	struct lwm2m_obj_path path;
	struct lwm2m_engine_obj_inst *obj_inst;
	int ret = 0;

	LOG_DBG("path:%s", log_strdup(pathstr));

	/* translate path -> path_obj */
	ret = lwm2m_string_to_path(pathstr, &path, '/');
	if (ret < 0) {
		return ret;
	}

	if (path.level != 2U) {
		LOG_ERR("path must have 2 parts");
		return -EINVAL;
	}

	ret = lwm2m_create_obj_inst(path.obj_id, path.obj_inst_id, &obj_inst);
	if (ret < 0) {
		return ret;
	}

#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT)
	engine_trigger_update(true);
#endif

	return 0;
}

int lwm2m_engine_delete_obj_inst(const char *pathstr)
{
	struct lwm2m_obj_path path;
	int ret = 0;

	LOG_DBG("path: %s", log_strdup(pathstr));

	/* translate path -> path_obj */
	ret = lwm2m_string_to_path(pathstr, &path, '/');
	if (ret < 0) {
		return ret;
	}

	if (path.level != 2U) {
		LOG_ERR("path must have 2 parts");
		return -EINVAL;
	}

	ret = lwm2m_delete_obj_inst(path.obj_id, path.obj_inst_id);
	if (ret < 0) {
		return ret;
	}

#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT)
	engine_trigger_update(true);
#endif

	return 0;
}

struct lwm2m_engine_obj_inst *lwm2m_engine_get_obj_inst(
					const struct lwm2m_obj_path *path)
{
	if (path->level < LWM2M_PATH_LEVEL_OBJECT_INST) {
		return NULL;
	}

	return get_engine_obj_inst(path->obj_id, path->obj_inst_id);
}

/* user data setter functions */
int lwm2m_engine_set_res_buf(const char *pathstr, void *buffer_ptr,
				  uint16_t buffer_len, uint16_t data_len, uint8_t data_flags)
{
	struct lwm2m_obj_path path;
	struct lwm2m_engine_res_inst *res_inst = NULL;
	int ret = 0;

	/* translate path -> path_obj */
	ret = lwm2m_string_to_path(pathstr, &path, '/');
	if (ret < 0) {
		return ret;
	}

	if (path.level < 3) {
		LOG_ERR("path must have at least 3 parts");
		return -EINVAL;
	}

	/* look up resource obj */
	ret = path_to_objs(&path, NULL, NULL, NULL, &res_inst);
	if (ret < 0) {
		return ret;
	}

	if (!res_inst) {
		LOG_ERR("res instance %d not found", path.res_inst_id);
		return -ENOENT;
	}

	/* assign data elements */
	res_inst->data_ptr = buffer_ptr;
	res_inst->data_len = data_len;
	res_inst->max_data_len = buffer_len;
	res_inst->data_flags = data_flags;

	return ret;
}

int lwm2m_engine_set_res_data(const char *pathstr, void *data_ptr, uint16_t data_len,
			      uint8_t data_flags)
{
	return lwm2m_engine_set_res_buf(pathstr, data_ptr, data_len, data_len, data_flags);
}

static int lwm2m_engine_set(const char *pathstr, void *value, uint16_t len)
{
	struct lwm2m_obj_path path;
	struct lwm2m_engine_obj_inst *obj_inst;
	struct lwm2m_engine_obj_field *obj_field;
	struct lwm2m_engine_res *res = NULL;
	struct lwm2m_engine_res_inst *res_inst = NULL;
	void *data_ptr = NULL;
	size_t max_data_len = 0;
	int ret = 0;
	bool changed = false;

	LOG_DBG("path:%s, value:%p, len:%d", log_strdup(pathstr), value, len);

	/* translate path -> path_obj */
	ret = lwm2m_string_to_path(pathstr, &path, '/');
	if (ret < 0) {
		return ret;
	}

	if (path.level < 3) {
		LOG_ERR("path must have at least 3 parts");
		return -EINVAL;
	}

	/* look up resource obj */
	ret = path_to_objs(&path, &obj_inst, &obj_field, &res, &res_inst);
	if (ret < 0) {
		return ret;
	}

	if (!res_inst) {
		LOG_ERR("res instance %d not found", path.res_inst_id);
		return -ENOENT;
	}

	if (LWM2M_HAS_RES_FLAG(res_inst, LWM2M_RES_DATA_FLAG_RO)) {
		LOG_ERR("res instance data pointer is read-only "
			"[%u/%u/%u/%u:%u]", path.obj_id, path.obj_inst_id,
			path.res_id, path.res_inst_id, path.level);
		return -EACCES;
	}

	/* setup initial data elements */
	data_ptr = res_inst->data_ptr;
	max_data_len = res_inst->max_data_len;

	/* allow user to override data elements via callback */
	if (res->pre_write_cb) {
		data_ptr = res->pre_write_cb(obj_inst->obj_inst_id,
					     res->res_id, res_inst->res_inst_id,
					     &max_data_len);
	}

	if (!data_ptr) {
		LOG_ERR("res instance data pointer is NULL [%u/%u/%u/%u:%u]",
			path.obj_id, path.obj_inst_id, path.res_id,
			path.res_inst_id, path.level);
		return -EINVAL;
	}

	/* check length (note: we add 1 to string length for NULL pad) */
	if (len > max_data_len -
		(obj_field->data_type == LWM2M_RES_TYPE_STRING ? 1 : 0)) {
		LOG_ERR("length %u is too long for res instance %d data",
			len, path.res_id);
		return -ENOMEM;
	}

	if (memcmp(data_ptr, value, len) !=  0) {
		changed = true;
	}

#if CONFIG_LWM2M_ENGINE_VALIDATION_BUFFER_SIZE > 0
	if (res->validate_cb) {
		ret = res->validate_cb(obj_inst->obj_inst_id, res->res_id,
				       res_inst->res_inst_id, value,
				       len, false, 0);
		if (ret < 0) {
			return -EINVAL;
		}
	}
#endif /* CONFIG_LWM2M_ENGINE_VALIDATION_BUFFER_SIZE > 0 */

	switch (obj_field->data_type) {

	case LWM2M_RES_TYPE_OPAQUE:
		memcpy((uint8_t *)data_ptr, value, len);
		break;

	case LWM2M_RES_TYPE_STRING:
		memcpy((uint8_t *)data_ptr, value, len);
		((uint8_t *)data_ptr)[len] = '\0';
		break;

	case LWM2M_RES_TYPE_U32:
	case LWM2M_RES_TYPE_TIME:
		*((uint32_t *)data_ptr) = *(uint32_t *)value;
		break;

	case LWM2M_RES_TYPE_U16:
		*((uint16_t *)data_ptr) = *(uint16_t *)value;
		break;

	case LWM2M_RES_TYPE_U8:
		*((uint8_t *)data_ptr) = *(uint8_t *)value;
		break;

	case LWM2M_RES_TYPE_S64:
		*((int64_t *)data_ptr) = *(int64_t *)value;
		break;

	case LWM2M_RES_TYPE_S32:
		*((int32_t *)data_ptr) = *(int32_t *)value;
		break;

	case LWM2M_RES_TYPE_S16:
		*((int16_t *)data_ptr) = *(int16_t *)value;
		break;

	case LWM2M_RES_TYPE_S8:
		*((int8_t *)data_ptr) = *(int8_t *)value;
		break;

	case LWM2M_RES_TYPE_BOOL:
		*((bool *)data_ptr) = *(bool *)value;
		break;

	case LWM2M_RES_TYPE_FLOAT:
		*(double *)data_ptr = *(double *)value;
		break;

	case LWM2M_RES_TYPE_OBJLNK:
		*((struct lwm2m_objlnk *)data_ptr) =
				*(struct lwm2m_objlnk *)value;
		break;

	default:
		LOG_ERR("unknown obj data_type %d", obj_field->data_type);
		return -EINVAL;

	}

	res_inst->data_len = len;

	if (res->post_write_cb) {
		ret = res->post_write_cb(obj_inst->obj_inst_id,
					 res->res_id, res_inst->res_inst_id,
					 data_ptr, len, false, 0);
	}

	if (changed && LWM2M_HAS_PERM(obj_field, LWM2M_PERM_R)) {
		NOTIFY_OBSERVER_PATH(&path);
	}

	return ret;
}

int lwm2m_engine_set_opaque(const char *pathstr, char *data_ptr, uint16_t data_len)
{
	return lwm2m_engine_set(pathstr, data_ptr, data_len);
}

int lwm2m_engine_set_string(const char *pathstr, char *data_ptr)
{
	return lwm2m_engine_set(pathstr, data_ptr, strlen(data_ptr));
}

int lwm2m_engine_set_u8(const char *pathstr, uint8_t value)
{
	return lwm2m_engine_set(pathstr, &value, 1);
}

int lwm2m_engine_set_u16(const char *pathstr, uint16_t value)
{
	return lwm2m_engine_set(pathstr, &value, 2);
}

int lwm2m_engine_set_u32(const char *pathstr, uint32_t value)
{
	return lwm2m_engine_set(pathstr, &value, 4);
}

int lwm2m_engine_set_u64(const char *pathstr, uint64_t value)
{
	return lwm2m_engine_set(pathstr, &value, 8);
}

int lwm2m_engine_set_s8(const char *pathstr, int8_t value)
{
	return lwm2m_engine_set(pathstr, &value, 1);
}

int lwm2m_engine_set_s16(const char *pathstr, int16_t value)
{
	return lwm2m_engine_set(pathstr, &value, 2);
}

int lwm2m_engine_set_s32(const char *pathstr, int32_t value)
{
	return lwm2m_engine_set(pathstr, &value, 4);
}

int lwm2m_engine_set_s64(const char *pathstr, int64_t value)
{
	return lwm2m_engine_set(pathstr, &value, 8);
}

int lwm2m_engine_set_bool(const char *pathstr, bool value)
{
	uint8_t temp = (value != 0 ? 1 : 0);

	return lwm2m_engine_set(pathstr, &temp, 1);
}

int lwm2m_engine_set_float(const char *pathstr, double *value)
{
	return lwm2m_engine_set(pathstr, value, sizeof(double));
}

int lwm2m_engine_set_objlnk(const char *pathstr, struct lwm2m_objlnk *value)
{
	return lwm2m_engine_set(pathstr, value, sizeof(struct lwm2m_objlnk));
}

int lwm2m_engine_set_res_data_len(const char *pathstr, uint16_t data_len)
{
	int ret;
	void *buffer_ptr;
	uint16_t buffer_len;
	uint16_t old_len;
	uint8_t data_flags;

	ret = lwm2m_engine_get_res_buf(pathstr, &buffer_ptr, &buffer_len, &old_len, &data_flags);
	if (ret) {
		return ret;
	}
	return lwm2m_engine_set_res_buf(pathstr, buffer_ptr, buffer_len, data_len, data_flags);
}
/* user data getter functions */
int lwm2m_engine_get_res_buf(const char *pathstr, void **buffer_ptr, uint16_t *buffer_len,
				  uint16_t *data_len, uint8_t *data_flags)
{
	struct lwm2m_obj_path path;
	struct lwm2m_engine_res_inst *res_inst = NULL;
	int ret = 0;

	/* translate path -> path_obj */
	ret = lwm2m_string_to_path(pathstr, &path, '/');
	if (ret < 0) {
		return ret;
	}

	if (path.level < 3) {
		LOG_ERR("path must have at least 3 parts");
		return -EINVAL;
	}

	/* look up resource obj */
	ret = path_to_objs(&path, NULL, NULL, NULL, &res_inst);
	if (ret < 0) {
		return ret;
	}

	if (!res_inst) {
		LOG_ERR("res instance %d not found", path.res_inst_id);
		return -ENOENT;
	}

	if (buffer_ptr) {
		*buffer_ptr = res_inst->data_ptr;
	}
	if (buffer_len) {
		*buffer_len = res_inst->max_data_len;
	}
	if (data_len) {
		*data_len = res_inst->data_len;
	}
	if (data_flags) {
		*data_flags = res_inst->data_flags;
	}

	return 0;
}

size_t lwm2m_engine_get_opaque_more(struct lwm2m_input_context *in,
				    uint8_t *buf, size_t buflen,
				    struct lwm2m_opaque_context *opaque,
				    bool *last_block)
{
	uint32_t in_len = opaque->remaining;
	uint16_t remaining = in->in_cpkt->max_len - in->offset;

	if (in_len > buflen) {
		in_len = buflen;
	}

	if (in_len > remaining) {
		in_len = remaining;
	}

	opaque->remaining -= in_len;
	remaining -= in_len;
	if (opaque->remaining == 0U || remaining == 0) {
		*last_block = true;
	}

	if (buf_read(buf, in_len, CPKT_BUF_READ(in->in_cpkt),
		     &in->offset) < 0) {
		*last_block = true;
		return 0;
	}

	return (size_t)in_len;
}

void lwm2m_engine_get_queue_mode(char *queue)
{
	if (IS_ENABLED(CONFIG_LWM2M_QUEUE_MODE_ENABLED)) {
		strncpy(queue, "Q", QUEUE_OPT_MAX_LEN);
	} else {
		strncpy(queue, "", QUEUE_OPT_MAX_LEN);
	}
}

void lwm2m_engine_get_binding(char *binding)
{
	/* Defaults to UDP. */
	strncpy(binding, "U", BINDING_OPT_MAX_LEN);
#if CONFIG_LWM2M_VERSION_1_0
	/* In LwM2M 1.0 binding and queue mode are in same parameter */
	char queue[QUEUE_OPT_MAX_LEN];

	lwm2m_engine_get_queue_mode(queue);
	strncat(binding, queue, QUEUE_OPT_MAX_LEN);
#endif
}

int lwm2m_engine_get_res_data(const char *pathstr, void **data_ptr, uint16_t *data_len,
			      uint8_t *data_flags)
{
	return lwm2m_engine_get_res_buf(pathstr, data_ptr, NULL, data_len, data_flags);
}

static int lwm2m_engine_get(const char *pathstr, void *buf, uint16_t buflen)
{
	int ret = 0;
	struct lwm2m_obj_path path;
	struct lwm2m_engine_obj_inst *obj_inst;
	struct lwm2m_engine_obj_field *obj_field;
	struct lwm2m_engine_res *res = NULL;
	struct lwm2m_engine_res_inst *res_inst = NULL;
	void *data_ptr = NULL;
	size_t data_len = 0;

	LOG_DBG("path:%s, buf:%p, buflen:%d", log_strdup(pathstr), buf, buflen);

	/* translate path -> path_obj */
	ret = lwm2m_string_to_path(pathstr, &path, '/');
	if (ret < 0) {
		return ret;
	}

	if (path.level < 3) {
		LOG_ERR("path must have at least 3 parts");
		return -EINVAL;
	}

	/* look up resource obj */
	ret = path_to_objs(&path, &obj_inst, &obj_field, &res, &res_inst);
	if (ret < 0) {
		return ret;
	}

	if (!res_inst) {
		LOG_ERR("res instance %d not found", path.res_inst_id);
		return -ENOENT;
	}

	/* setup initial data elements */
	data_ptr = res_inst->data_ptr;
	data_len = res_inst->data_len;

	/* allow user to override data elements via callback */
	if (res->read_cb) {
		data_ptr = res->read_cb(obj_inst->obj_inst_id,
					res->res_id, res_inst->res_inst_id,
					&data_len);
	}

	/* TODO: handle data_len > buflen case */

	if (data_ptr && data_len > 0) {
		switch (obj_field->data_type) {

		case LWM2M_RES_TYPE_OPAQUE:
			if (data_len > buflen) {
				return -ENOMEM;
			}

			memcpy(buf, data_ptr, data_len);
			break;

		case LWM2M_RES_TYPE_STRING:
			strncpy((uint8_t *)buf, (uint8_t *)data_ptr, buflen);
			break;

		case LWM2M_RES_TYPE_U32:
		case LWM2M_RES_TYPE_TIME:
			*(uint32_t *)buf = *(uint32_t *)data_ptr;
			break;

		case LWM2M_RES_TYPE_U16:
			*(uint16_t *)buf = *(uint16_t *)data_ptr;
			break;

		case LWM2M_RES_TYPE_U8:
			*(uint8_t *)buf = *(uint8_t *)data_ptr;
			break;

		case LWM2M_RES_TYPE_S64:
			*(int64_t *)buf = *(int64_t *)data_ptr;
			break;

		case LWM2M_RES_TYPE_S32:
			*(int32_t *)buf = *(int32_t *)data_ptr;
			break;

		case LWM2M_RES_TYPE_S16:
			*(int16_t *)buf = *(int16_t *)data_ptr;
			break;

		case LWM2M_RES_TYPE_S8:
			*(int8_t *)buf = *(int8_t *)data_ptr;
			break;

		case LWM2M_RES_TYPE_BOOL:
			*(bool *)buf = *(bool *)data_ptr;
			break;

		case LWM2M_RES_TYPE_FLOAT:
			*(double *)buf = *(double *)data_ptr;
			break;

		case LWM2M_RES_TYPE_OBJLNK:
			*(struct lwm2m_objlnk *)buf =
				*(struct lwm2m_objlnk *)data_ptr;
			break;

		default:
			LOG_ERR("unknown obj data_type %d",
				obj_field->data_type);
			return -EINVAL;

		}
	}

	return 0;
}

int lwm2m_engine_get_opaque(const char *pathstr, void *buf, uint16_t buflen)
{
	return lwm2m_engine_get(pathstr, buf, buflen);
}

int lwm2m_engine_get_string(const char *pathstr, void *buf, uint16_t buflen)
{
	return lwm2m_engine_get(pathstr, buf, buflen);
}

int lwm2m_engine_get_u8(const char *pathstr, uint8_t *value)
{
	return lwm2m_engine_get(pathstr, value, 1);
}

int lwm2m_engine_get_u16(const char *pathstr, uint16_t *value)
{
	return lwm2m_engine_get(pathstr, value, 2);
}

int lwm2m_engine_get_u32(const char *pathstr, uint32_t *value)
{
	return lwm2m_engine_get(pathstr, value, 4);
}

int lwm2m_engine_get_u64(const char *pathstr, uint64_t *value)
{
	return lwm2m_engine_get(pathstr, value, 8);
}

int lwm2m_engine_get_s8(const char *pathstr, int8_t *value)
{
	return lwm2m_engine_get(pathstr, value, 1);
}

int lwm2m_engine_get_s16(const char *pathstr, int16_t *value)
{
	return lwm2m_engine_get(pathstr, value, 2);
}

int lwm2m_engine_get_s32(const char *pathstr, int32_t *value)
{
	return lwm2m_engine_get(pathstr, value, 4);
}

int lwm2m_engine_get_s64(const char *pathstr, int64_t *value)
{
	return lwm2m_engine_get(pathstr, value, 8);
}

int lwm2m_engine_get_bool(const char *pathstr, bool *value)
{
	int ret = 0;
	int8_t temp = 0;

	ret = lwm2m_engine_get_s8(pathstr, &temp);
	if (!ret) {
		*value = temp != 0;
	}

	return ret;
}

int lwm2m_engine_get_float(const char *pathstr, double *buf)
{
	return lwm2m_engine_get(pathstr, buf, sizeof(double));
}

int lwm2m_engine_get_objlnk(const char *pathstr, struct lwm2m_objlnk *buf)
{
	return lwm2m_engine_get(pathstr, buf, sizeof(struct lwm2m_objlnk));
}

int lwm2m_engine_get_resource(const char *pathstr, struct lwm2m_engine_res **res)
{
	int ret;
	struct lwm2m_obj_path path;

	ret = lwm2m_string_to_path(pathstr, &path, '/');
	if (ret < 0) {
		return ret;
	}

	if (path.level < 3) {
		LOG_ERR("path must have 3 parts");
		return -EINVAL;
	}

	return path_to_objs(&path, NULL, NULL, res, NULL);
}

int lwm2m_get_path_reference_ptr(struct lwm2m_engine_obj *obj, struct lwm2m_obj_path *path,
				      void **ref)
{
	struct lwm2m_engine_obj_inst *obj_inst;
	struct lwm2m_engine_res *res;
	struct lwm2m_engine_res_inst *res_inst;
	int ret;

	if (!obj) {
		/* Discover Object */
		obj = get_engine_obj(path->obj_id);
		if (!obj) {
			/* No matching object found - ignore request */
			return -ENOENT;
		}
	}

	if (path->level == LWM2M_PATH_LEVEL_OBJECT) {
		*ref = obj;
	} else if (path->level == LWM2M_PATH_LEVEL_OBJECT_INST) {
		obj_inst = get_engine_obj_inst(path->obj_id,
					       path->obj_inst_id);
		if (!obj_inst) {
			return -ENOENT;
		}

		*ref = obj_inst;
	} else if (path->level == LWM2M_PATH_LEVEL_RESOURCE) {
		ret = path_to_objs(path, NULL, NULL, &res, NULL);
		if (ret < 0) {
			return ret;
		}

		*ref = res;
	} else if (IS_ENABLED(CONFIG_LWM2M_VERSION_1_1) &&
		   path->level == LWM2M_PATH_LEVEL_RESOURCE_INST) {

		ret = path_to_objs(path, NULL, NULL, NULL, &res_inst);
		if (ret < 0) {
			return ret;
		}

		*ref = res_inst;
	} else {
		/* bad request */
		return -EEXIST;
	}

	return 0;
}

/* Engine resource instance */
static int lwm2m_engine_allocate_resource_instance(struct lwm2m_engine_res *res,
						   struct lwm2m_engine_res_inst **res_inst,
						   uint8_t resource_instance_id)
{
	int i;

	if (!res->res_instances || res->res_inst_count == 0) {
		return -ENOMEM;
	}

	for (i = 0; i < res->res_inst_count; i++) {
		if (res->res_instances[i].res_inst_id ==
		    RES_INSTANCE_NOT_CREATED) {
			break;
		}
	}

	if (i >= res->res_inst_count) {
		return -ENOMEM;
	}

	res->res_instances[i].res_inst_id = resource_instance_id;
	*res_inst = &res->res_instances[i];
	return 0;
}

int lwm2m_engine_get_create_res_inst(struct lwm2m_obj_path *path, struct lwm2m_engine_res **res,
				     struct lwm2m_engine_res_inst **res_inst)
{
	int ret;
	struct lwm2m_engine_res *r = NULL;
	struct lwm2m_engine_res_inst *r_i = NULL;

	ret = path_to_objs(path, NULL, NULL, &r, &r_i);
	if (ret < 0) {
		return ret;
	}

	if (!r) {
		return -ENOENT;
	}
	/* Store resource pointer */
	*res = r;

	if (!r_i) {
		if (path->level < LWM2M_PATH_LEVEL_RESOURCE_INST) {
			return -EINVAL;
		}

		ret = lwm2m_engine_allocate_resource_instance(r, &r_i, path->res_inst_id);
		if (ret < 0) {
			return ret;
		}
	}

	/* Store resource instance pointer */
	*res_inst = r_i;
	return 0;
}

int lwm2m_engine_create_res_inst(const char *pathstr)
{
	int ret;
	struct lwm2m_engine_res *res = NULL;
	struct lwm2m_engine_res_inst *res_inst = NULL;
	struct lwm2m_obj_path path;

	ret = lwm2m_string_to_path(pathstr, &path, '/');
	if (ret < 0) {
		return ret;
	}

	if (path.level < 4) {
		LOG_ERR("path must have 4 parts");
		return -EINVAL;
	}

	ret = path_to_objs(&path, NULL, NULL, &res, &res_inst);
	if (ret < 0) {
		return ret;
	}

	if (!res) {
		LOG_ERR("resource %u not found", path.res_id);
		return -ENOENT;
	}

	if (res_inst && res_inst->res_inst_id != RES_INSTANCE_NOT_CREATED) {
		LOG_ERR("res instance %u already exists", path.res_inst_id);
		return -EINVAL;
	}

	return lwm2m_engine_allocate_resource_instance(res, &res_inst, path.res_inst_id);
}

int lwm2m_engine_delete_res_inst(const char *pathstr)
{
	int ret;
	struct lwm2m_engine_res_inst *res_inst = NULL;
	struct lwm2m_obj_path path;

	ret = lwm2m_string_to_path(pathstr, &path, '/');
	if (ret < 0) {
		return ret;
	}

	if (path.level < 4) {
		LOG_ERR("path must have 4 parts");
		return -EINVAL;
	}

	ret = path_to_objs(&path, NULL, NULL, NULL, &res_inst);
	if (ret < 0) {
		return ret;
	}

	if (!res_inst) {
		LOG_ERR("res instance %u not found", path.res_inst_id);
		return -ENOENT;
	}

	res_inst->data_ptr = NULL;
	res_inst->max_data_len = 0U;
	res_inst->data_len = 0U;
	res_inst->res_inst_id = RES_INSTANCE_NOT_CREATED;

	return 0;
}

/* Register callbacks */
int lwm2m_engine_register_read_callback(const char *pathstr,
					lwm2m_engine_get_data_cb_t cb)
{
	int ret;
	struct lwm2m_engine_res *res = NULL;

	ret = lwm2m_engine_get_resource(pathstr, &res);
	if (ret < 0) {
		return ret;
	}

	res->read_cb = cb;
	return 0;
}

int lwm2m_engine_register_pre_write_callback(const char *pathstr,
					     lwm2m_engine_get_data_cb_t cb)
{
	int ret;
	struct lwm2m_engine_res *res = NULL;

	ret = lwm2m_engine_get_resource(pathstr, &res);
	if (ret < 0) {
		return ret;
	}

	res->pre_write_cb = cb;
	return 0;
}

int lwm2m_engine_register_validate_callback(const char *pathstr,
					    lwm2m_engine_set_data_cb_t cb)
{
#if CONFIG_LWM2M_ENGINE_VALIDATION_BUFFER_SIZE > 0
	int ret;
	struct lwm2m_engine_res *res = NULL;

	ret = lwm2m_engine_get_resource(pathstr, &res);
	if (ret < 0) {
		return ret;
	}

	res->validate_cb = cb;
	return 0;
#else
	ARG_UNUSED(pathstr);
	ARG_UNUSED(cb);

	LOG_ERR("Validation disabled. Set "
		"CONFIG_LWM2M_ENGINE_VALIDATION_BUFFER_SIZE > 0 to "
		"enable validation support.");
	return -ENOTSUP;
#endif /* CONFIG_LWM2M_ENGINE_VALIDATION_BUFFER_SIZE > 0 */
}

int lwm2m_engine_register_post_write_callback(const char *pathstr,
					 lwm2m_engine_set_data_cb_t cb)
{
	int ret;
	struct lwm2m_engine_res *res = NULL;

	ret = lwm2m_engine_get_resource(pathstr, &res);
	if (ret < 0) {
		return ret;
	}

	res->post_write_cb = cb;
	return 0;
}

int lwm2m_engine_register_exec_callback(const char *pathstr,
					lwm2m_engine_execute_cb_t cb)
{
	int ret;
	struct lwm2m_engine_res *res = NULL;

	ret = lwm2m_engine_get_resource(pathstr, &res);
	if (ret < 0) {
		return ret;
	}

	res->execute_cb = cb;
	return 0;
}

int lwm2m_engine_register_create_callback(uint16_t obj_id,
					  lwm2m_engine_user_cb_t cb)
{
	struct lwm2m_engine_obj *obj = NULL;

	obj = get_engine_obj(obj_id);
	if (!obj) {
		LOG_ERR("unable to find obj: %u", obj_id);
		return -ENOENT;
	}

	obj->user_create_cb = cb;
	return 0;
}

int lwm2m_engine_register_delete_callback(uint16_t obj_id,
					  lwm2m_engine_user_cb_t cb)
{
	struct lwm2m_engine_obj *obj = NULL;

	obj = get_engine_obj(obj_id);
	if (!obj) {
		LOG_ERR("unable to find obj: %u", obj_id);
		return -ENOENT;
	}

	obj->user_delete_cb = cb;
	return 0;
}

/* generic data handlers */
static int lwm2m_write_handler_opaque(struct lwm2m_engine_obj_inst *obj_inst,
				      struct lwm2m_engine_res *res,
				      struct lwm2m_engine_res_inst *res_inst,
				      struct lwm2m_message *msg,
				      void *data_ptr, size_t data_len)
{
	int len = 1;
	bool last_pkt_block = false;
	int ret = 0;
	bool last_block = true;
	struct lwm2m_opaque_context opaque_ctx = { 0 };
	void *write_buf;
	size_t write_buf_len;

	if (msg->in.block_ctx != NULL) {
		last_block = msg->in.block_ctx->last_block;

		/* Restore the opaque context from the block context, if used.
		*/
		opaque_ctx = msg->in.block_ctx->opaque;
	}

#if CONFIG_LWM2M_ENGINE_VALIDATION_BUFFER_SIZE > 0
	/* In case validation callback is present, write data to the temporary
	 * buffer first, for validation. Otherwise, write to the data buffer
	 * directly.
	 */
	if (res->validate_cb) {
		write_buf = msg->ctx->validate_buf;
		write_buf_len = sizeof(msg->ctx->validate_buf);
	} else
#endif /* CONFIG_LWM2M_ENGINE_VALIDATION_BUFFER_SIZE > 0 */
	{
		write_buf = data_ptr;
		write_buf_len = data_len;
	}

	while (!last_pkt_block && len > 0) {
		len = engine_get_opaque(&msg->in, write_buf,
					MIN(data_len, write_buf_len),
					&opaque_ctx, &last_pkt_block);
		if (len <= 0) {
			return len;
		}

#if CONFIG_LWM2M_ENGINE_VALIDATION_BUFFER_SIZE > 0
		if (res->validate_cb) {
			ret = res->validate_cb(
				obj_inst->obj_inst_id, res->res_id,
				res_inst->res_inst_id, write_buf, len,
				last_pkt_block && last_block, opaque_ctx.len);
			if (ret < 0) {
				/* -EEXIST will generate Bad Request LWM2M response. */
				return -EEXIST;
			}

			memcpy(data_ptr, write_buf, len);
		}
#endif /* CONFIG_LWM2M_ENGINE_VALIDATION_BUFFER_SIZE > 0 */

		if (res->post_write_cb) {
			ret = res->post_write_cb(
				obj_inst->obj_inst_id, res->res_id,
				res_inst->res_inst_id, data_ptr, len,
				last_pkt_block && last_block, opaque_ctx.len);
			if (ret < 0) {
				return ret;
			}
		}
	}

	if (msg->in.block_ctx != NULL) {
		msg->in.block_ctx->opaque = opaque_ctx;
	}

	return opaque_ctx.len;
}

bool lwm2m_engine_bootstrap_override(struct lwm2m_ctx *client_ctx, struct lwm2m_obj_path *path)
{
	if (!client_ctx->bootstrap_mode) {
		/* Bootstrap is not active override is not possible then */
		return false;
	}

	if (path->obj_id == LWM2M_OBJECT_SECURITY_ID || path->obj_id == LWM2M_OBJECT_SERVER_ID) {
		/* Bootstrap server have a access to Security and Server object */
		return true;
	}

	return false;
}

/* This function is exposed for the content format writers */
int lwm2m_write_handler(struct lwm2m_engine_obj_inst *obj_inst,
			struct lwm2m_engine_res *res,
			struct lwm2m_engine_res_inst *res_inst,
			struct lwm2m_engine_obj_field *obj_field,
			struct lwm2m_message *msg)
{
	void *data_ptr = NULL;
	size_t data_len = 0;
	size_t len = 0;
	size_t total_size = 0;
	int64_t temp64 = 0;
	int32_t temp32 = 0;
	int ret = 0;
	bool last_block = true;
	void *write_buf;
	size_t write_buf_len;

	if (!obj_inst || !res || !res_inst || !obj_field || !msg) {
		return -EINVAL;
	}

	if (LWM2M_HAS_RES_FLAG(res_inst, LWM2M_RES_DATA_FLAG_RO)) {
		return -EACCES;
	}

	/* setup initial data elements */
	data_ptr = res_inst->data_ptr;
	data_len = res_inst->max_data_len;

	/* allow user to override data elements via callback */
	if (res->pre_write_cb) {
		data_ptr = res->pre_write_cb(obj_inst->obj_inst_id,
					     res->res_id, res_inst->res_inst_id,
					     &data_len);
	}

	if (res->post_write_cb
#if CONFIG_LWM2M_ENGINE_VALIDATION_BUFFER_SIZE > 0
	    || res->validate_cb
#endif
	) {
		if (msg->in.block_ctx != NULL) {
			/* Get block_ctx for total_size (might be zero) */
			total_size = msg->in.block_ctx->ctx.total_size;
			LOG_DBG("BLOCK1: total:%zu current:%zu"
				" last:%u",
				msg->in.block_ctx->ctx.total_size,
				msg->in.block_ctx->ctx.current,
				msg->in.block_ctx->last_block);
		}
	}

#if CONFIG_LWM2M_ENGINE_VALIDATION_BUFFER_SIZE > 0
	/* In case validation callback is present, write data to the temporary
	 * buffer first, for validation. Otherwise, write to the data buffer
	 * directly.
	 */
	if (res->validate_cb) {
		write_buf = msg->ctx->validate_buf;
		write_buf_len = sizeof(msg->ctx->validate_buf);
	} else
#endif /* CONFIG_LWM2M_ENGINE_VALIDATION_BUFFER_SIZE > 0 */
	{
		write_buf = data_ptr;
		write_buf_len = data_len;
	}

	if (data_ptr && data_len > 0) {
		switch (obj_field->data_type) {

		case LWM2M_RES_TYPE_OPAQUE:
			ret = lwm2m_write_handler_opaque(obj_inst, res,
							 res_inst, msg,
							 data_ptr, data_len);
			len = ret;
			break;

		case LWM2M_RES_TYPE_STRING:
			ret = engine_get_string(&msg->in, write_buf,
						write_buf_len);
			if (ret < 0) {
				break;
			}

			len = strlen((char *)write_buf);
			break;

		case LWM2M_RES_TYPE_TIME:
			ret = engine_get_time(&msg->in, &temp64);
			if (ret < 0) {
				break;
			}
			*(uint32_t *)write_buf = temp64;
			len = 4;
			break;

		case LWM2M_RES_TYPE_U32:
			ret = engine_get_s64(&msg->in, &temp64);
			if (ret < 0) {
				break;
			}

			*(uint32_t *)write_buf = temp64;
			len = 4;
			break;

		case LWM2M_RES_TYPE_U16:
			ret = engine_get_s32(&msg->in, &temp32);
			if (ret < 0) {
				break;
			}

			*(uint16_t *)write_buf = temp32;
			len = 2;
			break;

		case LWM2M_RES_TYPE_U8:
			ret = engine_get_s32(&msg->in, &temp32);
			if (ret < 0) {
				break;
			}

			*(uint8_t *)write_buf = temp32;
			len = 1;
			break;

		case LWM2M_RES_TYPE_S64:
			ret = engine_get_s64(&msg->in, (int64_t *)write_buf);
			len = 8;
			break;

		case LWM2M_RES_TYPE_S32:
			ret = engine_get_s32(&msg->in, (int32_t *)write_buf);
			len = 4;
			break;

		case LWM2M_RES_TYPE_S16:
			ret = engine_get_s32(&msg->in, &temp32);
			if (ret < 0) {
				break;
			}

			*(int16_t *)write_buf = temp32;
			len = 2;
			break;

		case LWM2M_RES_TYPE_S8:
			ret = engine_get_s32(&msg->in, &temp32);
			if (ret < 0) {
				break;
			}

			*(int8_t *)write_buf = temp32;
			len = 1;
			break;

		case LWM2M_RES_TYPE_BOOL:
			ret = engine_get_bool(&msg->in, (bool *)write_buf);
			len = 1;
			break;

		case LWM2M_RES_TYPE_FLOAT:
			ret = engine_get_float(&msg->in, (double *)write_buf);
			len = sizeof(double);
			break;

		case LWM2M_RES_TYPE_OBJLNK:
			ret = engine_get_objlnk(&msg->in,
						(struct lwm2m_objlnk *)write_buf);
			len = sizeof(struct lwm2m_objlnk);
			break;

		default:
			LOG_ERR("unknown obj data_type %d",
				obj_field->data_type);
			return -EINVAL;

		}

		if (ret < 0) {
			return ret;
		}
	} else {
		return -ENOENT;
	}

	if (obj_field->data_type != LWM2M_RES_TYPE_OPAQUE) {
#if CONFIG_LWM2M_ENGINE_VALIDATION_BUFFER_SIZE > 0
		if (res->validate_cb) {
			ret = res->validate_cb(
				obj_inst->obj_inst_id, res->res_id,
				res_inst->res_inst_id, write_buf, len,
				last_block, total_size);
			if (ret < 0) {
				/* -EEXIST will generate Bad Request LWM2M response. */
				return -EEXIST;
			}

			if (len > data_len) {
				LOG_ERR("Received data won't fit into provided "
					"bufffer");
				return -ENOMEM;
			}

			if (obj_field->data_type == LWM2M_RES_TYPE_STRING) {
				strncpy(data_ptr, write_buf, data_len);
			} else {
				memcpy(data_ptr, write_buf, len);
			}
		}
#endif /* CONFIG_LWM2M_ENGINE_VALIDATION_BUFFER_SIZE > 0 */

		if (res->post_write_cb) {
			ret = res->post_write_cb(
				obj_inst->obj_inst_id, res->res_id,
				res_inst->res_inst_id, data_ptr, len,
				last_block, total_size);
		}
	}

	res_inst->data_len = len;

	if (LWM2M_HAS_PERM(obj_field, LWM2M_PERM_R)) {
		NOTIFY_OBSERVER_PATH(&msg->path);
	}

	return ret;
}

int lwm2m_exec_handler(struct lwm2m_message *msg)
{
	struct lwm2m_engine_obj_inst *obj_inst;
	struct lwm2m_engine_res *res = NULL;
	int ret;
	uint8_t *args;
	uint16_t args_len;

	if (!msg) {
		return -EINVAL;
	}

	ret = path_to_objs(&msg->path, &obj_inst, NULL, &res, NULL);
	if (ret < 0) {
		return ret;
	}

	args = (uint8_t *)coap_packet_get_payload(msg->in.in_cpkt, &args_len);

	if (res->execute_cb) {
		return res->execute_cb(obj_inst->obj_inst_id, args, args_len);
	}

	/* TODO: something else to handle for execute? */
	return -ENOENT;
}

int lwm2m_get_or_create_engine_obj(struct lwm2m_message *msg,
				   struct lwm2m_engine_obj_inst **obj_inst,
				   uint8_t *created)
{
	int ret = 0;

	if (created) {
		*created = 0U;
	}

	*obj_inst = get_engine_obj_inst(msg->path.obj_id,
					msg->path.obj_inst_id);
	if (!*obj_inst) {
		ret = lwm2m_create_obj_inst(msg->path.obj_id,
					    msg->path.obj_inst_id,
					    obj_inst);
		if (ret < 0) {
			return ret;
		}

		/* set created flag to one */
		if (created) {
			*created = 1U;
		}

#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT)
		if (!msg->ctx->bootstrap_mode) {
			engine_trigger_update(true);
		}
#endif
	}

	return ret;
}

int lwm2m_engine_validate_write_access(struct lwm2m_message *msg,
				       struct lwm2m_engine_obj_inst *obj_inst,
				       struct lwm2m_engine_obj_field **obj_field)
{
	struct lwm2m_engine_obj_field *o_f;

	o_f = lwm2m_get_engine_obj_field(obj_inst->obj, msg->path.res_id);
	if (!o_f) {
		return -ENOENT;
	}

	*obj_field = o_f;

	if (!LWM2M_HAS_PERM(o_f, LWM2M_PERM_W) &&
	    !lwm2m_engine_bootstrap_override(msg->ctx, &msg->path)) {
		return -EPERM;
	}

	if (!obj_inst->resources || obj_inst->resource_count == 0U) {
		return -EINVAL;
	}

	return 0;
}

struct lwm2m_engine_res *lwm2m_engine_get_res(
					const struct lwm2m_obj_path *path)
{
	struct lwm2m_engine_res *res = NULL;
	int ret;

	if (path->level < LWM2M_PATH_LEVEL_RESOURCE) {
		return NULL;
	}

	ret = path_to_objs(path, NULL, NULL, &res, NULL);
	if (ret < 0) {
		return NULL;
	}

	return res;
}

struct lwm2m_engine_res_inst *lwm2m_engine_get_res_inst(
					const struct lwm2m_obj_path *path)
{
	struct lwm2m_engine_res_inst *res_inst = NULL;
	int ret;

	if (path->level != LWM2M_PATH_LEVEL_RESOURCE_INST) {
		return NULL;
	}

	ret = path_to_objs(path, NULL, NULL, NULL, &res_inst);
	if (ret < 0) {
		return NULL;
	}

	return res_inst;
}

bool lwm2m_engine_shall_report_obj_version(const struct lwm2m_engine_obj *obj)
{
	if (obj->is_core) {
		return obj->version_major != LWM2M_PROTOCOL_VERSION_MAJOR ||
		       obj->version_minor != LWM2M_PROTOCOL_VERSION_MINOR;
	}

	return obj->version_major != 1 || obj->version_minor != 0;
}

#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)
static bool bootstrap_delete_allowed(int obj_id, int obj_inst_id)
{
	char pathstr[MAX_RESOURCE_LEN];
	bool bootstrap_server;
	int ret;

	if (obj_id == LWM2M_OBJECT_SECURITY_ID) {
		snprintk(pathstr, sizeof(pathstr), "%d/%d/1",
			 LWM2M_OBJECT_SECURITY_ID, obj_inst_id);
		ret = lwm2m_engine_get_bool(pathstr, &bootstrap_server);
		if (ret < 0) {
			return false;
		}

		if (bootstrap_server) {
			return false;
		}
	}

	if (obj_id == LWM2M_OBJECT_DEVICE_ID) {
		return false;
	}

	return true;
}


static int bootstrap_delete(struct lwm2m_message *msg)
{
	struct lwm2m_engine_obj_inst *obj_inst, *tmp;
	int ret = 0;

	if (msg->path.level > 2) {
		return -EPERM;
	}

	if (msg->path.level == 2) {
		if (!bootstrap_delete_allowed(msg->path.obj_id,
					      msg->path.obj_inst_id)) {
			return -EPERM;
		}

		return lwm2m_delete_obj_inst(msg->path.obj_id,
					     msg->path.obj_inst_id);
	}

	/* DELETE all instances of a specific object or all object instances if
	 * not specified, excluding the following exceptions (according to the
	 * LwM2M specification v1.0.2, ch 5.2.7.5):
	 * - LwM2M Bootstrap-Server Account (Bootstrap Security object, ID 0)
	 * - Device object (ID 3)
	 */
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&engine_obj_inst_list,
					  obj_inst, tmp, node) {
		if (msg->path.level == 1 &&
		    obj_inst->obj->obj_id != msg->path.obj_id) {
			continue;
		}

		if (!bootstrap_delete_allowed(obj_inst->obj->obj_id,
					      obj_inst->obj_inst_id)) {
			continue;
		}

		ret = lwm2m_delete_obj_inst(obj_inst->obj->obj_id,
					    obj_inst->obj_inst_id);
		if (ret < 0) {
			return ret;
		}
	}

	return ret;
}
#endif
