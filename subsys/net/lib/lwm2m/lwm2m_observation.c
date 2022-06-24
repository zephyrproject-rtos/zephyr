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

#define LOG_MODULE_NAME net_lwm2m_observation
#define LOG_LEVEL	CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include "lwm2m_engine.h"
#include "lwm2m_object.h"
#include "lwm2m_util.h"

#include <ctype.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/init.h>
#include <zephyr/net/http_parser_url.h>
#include <zephyr/net/lwm2m.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/printk.h>
#include <zephyr/types.h>

#include <fcntl.h>

#ifdef CONFIG_LWM2M_RD_CLIENT_SUPPORT
#include "lwm2m_rd_client.h"
#endif
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

#define OBSERVE_COUNTER_START 0U

#if defined(CONFIG_COAP_EXTENDED_OPTIONS_LEN)
#define COAP_OPTION_BUF_LEN (CONFIG_COAP_EXTENDED_OPTIONS_LEN_VALUE + 1)
#else
#define COAP_OPTION_BUF_LEN 13
#endif

/* Resources */
static sys_slist_t obs_obj_path_list;

static struct observe_node observe_node_data[CONFIG_LWM2M_ENGINE_MAX_OBSERVER];

/* External resources */
struct lwm2m_ctx **lwm2m_sock_ctx(void);

int lwm2m_sock_nfds(void);
/* Resource wrappers */
sys_slist_t *lwm2m_obs_obj_path_list(void) { return &obs_obj_path_list; }

struct notification_attrs {
	/* use to determine which value is set */
	double gt;
	double lt;
	double st;
	int32_t pmin;
	int32_t pmax;
	uint8_t flags;
};

/* write-attribute related definitions */
static const char *const LWM2M_ATTR_STR[] = {"pmin", "pmax", "gt", "lt", "st"};
static const uint8_t LWM2M_ATTR_LEN[] = {4, 4, 2, 2, 2};

static struct lwm2m_attr write_attr_pool[CONFIG_LWM2M_NUM_ATTR];

/* Forward declarations */

void lwm2m_engine_free_list(sys_slist_t *path_list, sys_slist_t *free_list);

struct lwm2m_obj_path_list *lwm2m_engine_get_from_list(sys_slist_t *path_list);

static int update_attrs(void *ref, struct notification_attrs *out)
{
	int i;

	for (i = 0; i < CONFIG_LWM2M_NUM_ATTR; i++) {
		if (ref != write_attr_pool[i].ref) {
			continue;
		}

		switch (write_attr_pool[i].type) {
		case LWM2M_ATTR_PMIN:
			out->pmin = write_attr_pool[i].int_val;
			break;
		case LWM2M_ATTR_PMAX:
			out->pmax = write_attr_pool[i].int_val;
			break;
		case LWM2M_ATTR_LT:
			out->lt = write_attr_pool[i].float_val;
			break;
		case LWM2M_ATTR_GT:
			out->gt = write_attr_pool[i].float_val;
			break;
		case LWM2M_ATTR_STEP:
			out->st = write_attr_pool[i].float_val;
			break;
		default:
			LOG_ERR("Unrecognized attr: %d", write_attr_pool[i].type);
			return -EINVAL;
		}

		/* mark as set */
		out->flags |= BIT(write_attr_pool[i].type);
	}

	return 0;
}

void clear_attrs(void *ref)
{
	int i;

	for (i = 0; i < CONFIG_LWM2M_NUM_ATTR; i++) {
		if (ref == write_attr_pool[i].ref) {
			(void)memset(&write_attr_pool[i], 0, sizeof(write_attr_pool[i]));
		}
	}
}

static bool lwm2m_observer_path_compare(struct lwm2m_obj_path *o_p, struct lwm2m_obj_path *p)
{
	/* updated path is deeper than obs node, skip */
	if (p->level > o_p->level) {
		return false;
	}

	/* check obj id matched or not */
	if (p->obj_id != o_p->obj_id) {
		return false;
	}

	if (o_p->level >= LWM2M_PATH_LEVEL_OBJECT_INST) {
		if (p->level >= LWM2M_PATH_LEVEL_OBJECT_INST &&
		    p->obj_inst_id != o_p->obj_inst_id) {
			return false;
		}
	}

	if (o_p->level >= LWM2M_PATH_LEVEL_RESOURCE) {
		if (p->level >= LWM2M_PATH_LEVEL_RESOURCE && p->res_id != o_p->res_id) {
			return false;
		}
	}

	if (IS_ENABLED(CONFIG_LWM2M_VERSION_1_1) && o_p->level == LWM2M_PATH_LEVEL_RESOURCE_INST) {
		if (p->level == LWM2M_PATH_LEVEL_RESOURCE_INST &&
		    p->res_inst_id != o_p->res_inst_id) {
			return false;
		}
	}

	return true;
}

static bool lwm2m_notify_observer_list(sys_slist_t *path_list, struct lwm2m_obj_path *path)
{
	struct lwm2m_obj_path_list *o_p;

	SYS_SLIST_FOR_EACH_CONTAINER(path_list, o_p, node) {
		if (lwm2m_observer_path_compare(&o_p->path, path)) {
			return true;
		}
	}

	return false;
}

int lwm2m_notify_observer(uint16_t obj_id, uint16_t obj_inst_id, uint16_t res_id)
{
	struct lwm2m_obj_path path;

	path.level = LWM2M_PATH_LEVEL_RESOURCE;
	path.obj_id = obj_id;
	path.obj_inst_id = obj_inst_id;
	path.res_id = res_id;

	return lwm2m_notify_observer_path(&path);
}

static int engine_observe_get_attributes(struct lwm2m_obj_path *path,
					 struct notification_attrs *attrs, uint16_t srv_obj_inst)
{
	struct lwm2m_engine_obj *obj;
	struct lwm2m_engine_obj_field *obj_field = NULL;
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	struct lwm2m_engine_res_inst *res_inst = NULL;
	int ret, i;

	/* defaults from server object */
	attrs->pmin = lwm2m_server_get_pmin(srv_obj_inst);
	attrs->pmax = lwm2m_server_get_pmax(srv_obj_inst);
	attrs->flags = BIT(LWM2M_ATTR_PMIN) | BIT(LWM2M_ATTR_PMAX);

	/* check if object exists */
	obj = get_engine_obj(path->obj_id);
	if (!obj) {
		LOG_ERR("unable to find obj: %u", path->obj_id);
		return -ENOENT;
	}

	ret = update_attrs(obj, attrs);
	if (ret < 0) {
		return ret;
	}

	/* check if object instance exists */
	if (path->level >= LWM2M_PATH_LEVEL_OBJECT_INST) {
		obj_inst = get_engine_obj_inst(path->obj_id, path->obj_inst_id);
		if (!obj_inst) {
			attrs->pmax = 0;
			attrs->pmin = 0;
			return 0;
		}

		ret = update_attrs(obj_inst, attrs);
		if (ret < 0) {
			return ret;
		}
	}

	/* check if resource exists */
	if (path->level >= LWM2M_PATH_LEVEL_RESOURCE) {
		for (i = 0; i < obj_inst->resource_count; i++) {
			if (obj_inst->resources[i].res_id == path->res_id) {
				break;
			}
		}

		if (i == obj_inst->resource_count) {
			LOG_ERR("unable to find res_id: %u/%u/%u", path->obj_id, path->obj_inst_id,
				path->res_id);
			return -ENOENT;
		}

		/* load object field data */
		obj_field = lwm2m_get_engine_obj_field(obj, obj_inst->resources[i].res_id);
		if (!obj_field) {
			LOG_ERR("unable to find obj_field: %u/%u/%u", path->obj_id,
				path->obj_inst_id, path->res_id);
			return -ENOENT;
		}

		/* check for READ permission on matching resource */
		if (!LWM2M_HAS_PERM(obj_field, LWM2M_PERM_R)) {
			return -EPERM;
		}

		ret = update_attrs(&obj_inst->resources[i], attrs);
		if (ret < 0) {
			return ret;
		}
	}

	/* check if resource instance exists */
	if (IS_ENABLED(CONFIG_LWM2M_VERSION_1_1) && path->level == LWM2M_PATH_LEVEL_RESOURCE_INST) {
		ret = path_to_objs(path, NULL, NULL, NULL, &res_inst);
		if (ret < 0) {
			return ret;
		}

		if (res_inst == NULL) {
			return -ENOENT;
		}

		ret = update_attrs(res_inst, attrs);
		if (ret < 0) {
			return ret;
		}
	}

	attrs->pmax = (attrs->pmax >= attrs->pmin) ? (uint32_t)attrs->pmax : 0UL;

	return 0;
}

int engine_observe_attribute_list_get(sys_slist_t *path_list, struct notification_attrs *nattrs,
				      uint16_t server_obj_inst)
{
	struct lwm2m_obj_path_list *o_p;
	/* Temporary compare values */
	int32_t pmin = 0;
	int32_t pmax = 0;
	int ret;

	SYS_SLIST_FOR_EACH_CONTAINER(path_list, o_p, node) {
		nattrs->pmin = 0;
		nattrs->pmax = 0;

		ret = engine_observe_get_attributes(&o_p->path, nattrs, server_obj_inst);
		if (ret < 0) {
			return ret;
		}

		if (nattrs->pmin) {
			if (pmin == 0) {
				pmin = nattrs->pmin;
			} else {
				pmin = MIN(pmin, nattrs->pmin);
			}
		}

		if (nattrs->pmax) {
			if (pmax == 0) {
				pmax = nattrs->pmax;
			} else {
				pmax = MIN(pmax, nattrs->pmax);
			}
		}
	}

	nattrs->pmin = pmin;
	nattrs->pmax = pmax;
	return 0;
}

int lwm2m_notify_observer_path(struct lwm2m_obj_path *path)
{
	struct observe_node *obs;
	struct notification_attrs nattrs = {0};
	int64_t timestamp;
	int ret = 0;
	int i;
	struct lwm2m_ctx **sock_ctx = lwm2m_sock_ctx();

	if (path->level < LWM2M_PATH_LEVEL_RESOURCE) {
		return 0;
	}

	/* look for observers which match our resource */
	for (i = 0; i < lwm2m_sock_nfds(); ++i) {
		SYS_SLIST_FOR_EACH_CONTAINER(&sock_ctx[i]->observer, obs, node) {
			if (lwm2m_notify_observer_list(&obs->path_list, path)) {
				/* update the event time for this observer */
				ret = engine_observe_attribute_list_get(&obs->path_list, &nattrs,
									sock_ctx[i]->srv_obj_inst);
				if (ret < 0) {
					return ret;
				}

				if (nattrs.pmin) {
					timestamp =
						obs->last_timestamp + MSEC_PER_SEC * nattrs.pmin;
				} else {
					/* Trig immediately */
					timestamp = k_uptime_get();
				}

				if (!obs->event_timestamp || obs->event_timestamp > timestamp) {
					obs->resource_update = true;
					obs->event_timestamp = timestamp;
				}

				LOG_DBG("NOTIFY EVENT %u/%u/%u", path->obj_id, path->obj_inst_id,
					path->res_id);
				ret++;
			}
		}
	}

	return ret;
}

static struct observe_node *engine_allocate_observer(sys_slist_t *path_list, bool composite)
{
	int i;
	struct lwm2m_obj_path_list *entry, *tmp;
	struct observe_node *obs = NULL;

	/* find an unused observer index node */
	for (i = 0; i < CONFIG_LWM2M_ENGINE_MAX_OBSERVER; i++) {
		if (!observe_node_data[i].tkl) {

			obs = &observe_node_data[i];
			break;
		}
	}

	if (!obs) {
		return NULL;
	}

	sys_slist_init(&obs->path_list);
	obs->composite = composite;

	/* Allocate and copy path */
	SYS_SLIST_FOR_EACH_CONTAINER(path_list, tmp, node) {
		/* Allocate path entry */
		entry = lwm2m_engine_get_from_list(&obs_obj_path_list);
		if (!entry) {
			/* Free list */
			lwm2m_engine_free_list(&obs->path_list, &obs_obj_path_list);
			return NULL;
		}

		/* copy the values and add it to the list */
		memcpy(&entry->path, &tmp->path, sizeof(tmp->path));
		/* Add to last by keeping already sorted order */
		sys_slist_append(&obs->path_list, &entry->node);
	}

	return obs;
}

static void engine_observe_node_init(struct observe_node *obs, const uint8_t *token,
				     struct lwm2m_ctx *ctx, uint8_t tkl, uint16_t format,
				     int32_t att_pmax)
{
	struct lwm2m_obj_path_list *tmp;

	memcpy(obs->token, token, tkl);
	obs->tkl = tkl;

	obs->last_timestamp = k_uptime_get();
	if (att_pmax) {
		obs->event_timestamp = obs->last_timestamp + MSEC_PER_SEC * att_pmax;
	} else {
		obs->event_timestamp = 0;
	}
	obs->resource_update = false;
	obs->active_tx_operation = false;
	obs->format = format;
	obs->counter = OBSERVE_COUNTER_START;
	sys_slist_append(&ctx->observer, &obs->node);

	SYS_SLIST_FOR_EACH_CONTAINER(&obs->path_list, tmp, node) {
		LOG_DBG("OBSERVER ADDED %u/%u/%u/%u(%u)", tmp->path.obj_id, tmp->path.obj_inst_id,
			tmp->path.res_id, tmp->path.res_inst_id, tmp->path.level);

		if (ctx->observe_cb) {
			ctx->observe_cb(LWM2M_OBSERVE_EVENT_OBSERVER_ADDED, &tmp->path, NULL);
		}
	}

	LOG_DBG("token:'%s' addr:%s", sprint_token(token, tkl),
		lwm2m_sprint_ip_addr(&ctx->remote_addr));
}

static void remove_observer_path_from_list(struct lwm2m_ctx *ctx, struct observe_node *obs,
					   struct lwm2m_obj_path_list *o_p, sys_snode_t *prev_node)
{
	char buf[LWM2M_MAX_PATH_STR_LEN];

	LOG_DBG("Removing observer %p for path %s", obs, lwm2m_path_log_buf(buf, &o_p->path));
	if (ctx->observe_cb) {
		ctx->observe_cb(LWM2M_OBSERVE_EVENT_OBSERVER_REMOVED, &o_p->path, NULL);
	}
	/* Remove from the list and add to free list */
	sys_slist_remove(&obs->path_list, prev_node, &o_p->node);
	sys_slist_append(&obs_obj_path_list, &o_p->node);
}

static void engine_observe_single_path_id_remove(struct lwm2m_ctx *ctx, struct observe_node *obs,
						 uint16_t obj_id, int32_t obj_inst_id)
{
	struct lwm2m_obj_path_list *o_p, *tmp;
	sys_snode_t *prev_node = NULL;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&obs->path_list, o_p, tmp, node) {
		if (o_p->path.obj_id != obj_id && o_p->path.obj_inst_id) {
			prev_node = &o_p->node;
			continue;
		}

		if (obj_inst_id == -1 || o_p->path.obj_inst_id == obj_inst_id) {
			remove_observer_path_from_list(ctx, obs, o_p, prev_node);
		} else {
			prev_node = &o_p->node;
		}
	}
}

static bool engine_compare_obs_path_list(sys_slist_t *obs_path_list, sys_slist_t *path_list,
					 int list_length)
{
	sys_snode_t *obs_ptr, *comp_ptr;
	struct lwm2m_obj_path_list *obs_path, *comp_path;

	obs_ptr = sys_slist_peek_head(obs_path_list);
	comp_ptr = sys_slist_peek_head(path_list);
	while (list_length--) {
		obs_path = (struct lwm2m_obj_path_list *)obs_ptr;
		comp_path = (struct lwm2m_obj_path_list *)comp_ptr;
		if (memcmp(&obs_path->path, &comp_path->path, sizeof(struct lwm2m_obj_path))) {
			return false;
		}
		/* Read Next Info from list entry*/
		obs_ptr = sys_slist_peek_next_no_check(obs_ptr);
		comp_ptr = sys_slist_peek_next_no_check(comp_ptr);
	}

	return true;
}

static int engine_path_list_size(sys_slist_t *lwm2m_path_list)
{
	int list_size = 0;
	struct lwm2m_obj_path_list *entry;

	SYS_SLIST_FOR_EACH_CONTAINER(lwm2m_path_list, entry, node) {
		list_size++;
	}
	return list_size;
}

struct observe_node *engine_observe_node_discover(sys_slist_t *observe_node_list,
						  sys_snode_t **prev_node,
						  sys_slist_t *lwm2m_path_list,
						  const uint8_t *token, uint8_t tkl)
{
	struct observe_node *obs;
	int obs_list_size, path_list_size;

	if (lwm2m_path_list) {
		path_list_size = engine_path_list_size(lwm2m_path_list);
	}

	*prev_node = NULL;

	SYS_SLIST_FOR_EACH_CONTAINER(observe_node_list, obs, node) {

		if (lwm2m_path_list) {
			/* Validate Path for discovery */
			obs_list_size = engine_path_list_size(&obs->path_list);

			if (obs_list_size != path_list_size) {
				*prev_node = &obs->node;
				continue;
			}

			if (!engine_compare_obs_path_list(&obs->path_list, lwm2m_path_list,
							  obs_list_size)) {
				*prev_node = &obs->node;
				continue;
			}
		}

		if (token && memcmp(obs->token, token, tkl)) {
			/* Token not match */
			*prev_node = &obs->node;
			continue;
		}
		return obs;
	}
	return NULL;
}

static int engine_add_observer(struct lwm2m_message *msg, const uint8_t *token, uint8_t tkl,
			       uint16_t format)
{
	struct observe_node *obs;
	struct notification_attrs attrs;
	struct lwm2m_obj_path_list obs_path_list_buf;
	sys_slist_t lwm2m_path_list;
	sys_snode_t *prev_node = NULL;
	int ret;

	if (!msg || !msg->ctx) {
		LOG_ERR("valid lwm2m message is required");
		return -EINVAL;
	}

	if (!token || (tkl == 0U || tkl > MAX_TOKEN_LEN)) {
		LOG_ERR("token(%p) and token length(%u) must be valid.", token, tkl);
		return -EINVAL;
	}

	/* Create 1 entry linked list for message path */
	memcpy(&obs_path_list_buf.path, &msg->path, sizeof(struct lwm2m_obj_path));
	sys_slist_init(&lwm2m_path_list);
	sys_slist_append(&lwm2m_path_list, &obs_path_list_buf.node);

	obs = engine_observe_node_discover(&msg->ctx->observer, &prev_node, &lwm2m_path_list, NULL,
					   0);
	if (obs) {
		memcpy(obs->token, token, tkl);
		obs->tkl = tkl;

		LOG_DBG("OBSERVER DUPLICATE %u/%u/%u(%u) [%s]", msg->path.obj_id,
			msg->path.obj_inst_id, msg->path.res_id, msg->path.level,
			lwm2m_sprint_ip_addr(&msg->ctx->remote_addr));

		return 0;
	}

	/* Read attributes and allocate new entry */
	ret = engine_observe_get_attributes(&msg->path, &attrs, msg->ctx->srv_obj_inst);
	if (ret < 0) {
		return ret;
	}

	obs = engine_allocate_observer(&lwm2m_path_list, false);
	if (!obs) {
		return -ENOMEM;
	}

	engine_observe_node_init(obs, token, msg->ctx, tkl, format, attrs.pmax);
	return 0;
}

int do_composite_observe_read_path_op(struct lwm2m_message *msg, uint16_t content_format,
				      sys_slist_t *lwm2m_path_list,
				      sys_slist_t *lwm2m_path_free_list)
{
	switch (content_format) {
#if defined(CONFIG_LWM2M_RW_SENML_JSON_SUPPORT)
	case LWM2M_FORMAT_APP_SEML_JSON:
		return do_composite_observe_parse_path_senml_json(msg, lwm2m_path_list,
								  lwm2m_path_free_list);
#endif

#if defined(CONFIG_LWM2M_RW_SENML_CBOR_SUPPORT)
	case LWM2M_FORMAT_APP_SENML_CBOR:
		return do_composite_observe_parse_path_senml_cbor(msg, lwm2m_path_list,
								  lwm2m_path_free_list);
#endif

	default:
		LOG_ERR("Unsupported content-format: %u", content_format);
		return -ENOMSG;
	}
}

static int engine_add_composite_observer(struct lwm2m_message *msg, const uint8_t *token,
					 uint8_t tkl, uint16_t format)
{
	struct observe_node *obs;
	struct notification_attrs attrs;
	struct lwm2m_obj_path_list lwm2m_path_list_buf[CONFIG_LWM2M_COMPOSITE_PATH_LIST_SIZE];
	sys_slist_t lwm2m_path_list;
	sys_slist_t lwm2m_path_free_list;
	sys_snode_t *prev_node = NULL;
	int ret;

	if (!msg || !msg->ctx) {
		LOG_ERR("valid lwm2m message is required");
		return -EINVAL;
	}

	if (!token || (tkl == 0U || tkl > MAX_TOKEN_LEN)) {
		LOG_ERR("token(%p) and token length(%u) must be valid.", token, tkl);
		return -EINVAL;
	}

	/* Init list */
	lwm2m_engine_path_list_init(&lwm2m_path_list, &lwm2m_path_free_list, lwm2m_path_list_buf,
				    CONFIG_LWM2M_COMPOSITE_PATH_LIST_SIZE);

	/* Read attributes and allocate new entry */
	ret = do_composite_observe_read_path_op(msg, format, &lwm2m_path_list,
						&lwm2m_path_free_list);
	if (ret < 0) {
		return ret;
	}

	obs = engine_observe_node_discover(&msg->ctx->observer, &prev_node, &lwm2m_path_list, NULL,
					   0);
	if (obs) {
		memcpy(obs->token, token, tkl);
		obs->tkl = tkl;

		LOG_DBG("OBSERVER Composite DUPLICATE [%s]",
			lwm2m_sprint_ip_addr(&msg->ctx->remote_addr));

		return do_composite_read_op_for_parsed_list(msg, format, &lwm2m_path_list);
	}

	ret = engine_observe_attribute_list_get(&lwm2m_path_list, &attrs, msg->ctx->srv_obj_inst);
	if (ret < 0) {
		return ret;
	}

	obs = engine_allocate_observer(&lwm2m_path_list, true);
	if (!obs) {
		return -ENOMEM;
	}
	engine_observe_node_init(obs, token, msg->ctx, tkl, format, attrs.pmax);
	return do_composite_read_op_for_parsed_list(msg, format, &lwm2m_path_list);
}

void remove_observer_from_list(struct lwm2m_ctx *ctx, sys_snode_t *prev_node,
			       struct observe_node *obs)
{
	struct lwm2m_obj_path_list *o_p, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&obs->path_list, o_p, tmp, node) {
		remove_observer_path_from_list(ctx, obs, o_p, NULL);
	}
	sys_slist_remove(&ctx->observer, prev_node, &obs->node);
	(void)memset(obs, 0, sizeof(*obs));
}

int engine_remove_observer_by_token(struct lwm2m_ctx *ctx, const uint8_t *token, uint8_t tkl)
{
	struct observe_node *obs;
	sys_snode_t *prev_node = NULL;

	if (!token || (tkl == 0U || tkl > MAX_TOKEN_LEN)) {
		LOG_ERR("token(%p) and token length(%u) must be valid.", token, tkl);
		return -EINVAL;
	}

	obs = engine_observe_node_discover(&ctx->observer, &prev_node, NULL, token, tkl);
	if (!obs) {
		return -ENOENT;
	}

	remove_observer_from_list(ctx, prev_node, obs);

	LOG_DBG("observer '%s' removed", sprint_token(token, tkl));

	return 0;
}

static int engine_remove_composite_observer(struct lwm2m_message *msg, const uint8_t *token,
					    uint8_t tkl, uint16_t format)
{
	struct observe_node *obs;
	sys_snode_t *prev_node = NULL;
	struct lwm2m_obj_path_list lwm2m_path_list_buf[CONFIG_LWM2M_COMPOSITE_PATH_LIST_SIZE];
	sys_slist_t lwm2m_path_list;
	sys_slist_t lwm2m_path_free_list;
	int ret;

	if (!token || (tkl == 0U || tkl > MAX_TOKEN_LEN)) {
		LOG_ERR("token(%p) and token length(%u) must be valid.", token, tkl);
		return -EINVAL;
	}

	/* Init list */
	lwm2m_engine_path_list_init(&lwm2m_path_list, &lwm2m_path_free_list, lwm2m_path_list_buf,
				    CONFIG_LWM2M_COMPOSITE_PATH_LIST_SIZE);

	ret = do_composite_observe_read_path_op(msg, format, &lwm2m_path_list,
						&lwm2m_path_free_list);
	if (ret < 0) {
		return ret;
	}

	obs = engine_observe_node_discover(&msg->ctx->observer, &prev_node, &lwm2m_path_list, token,
					   tkl);
	if (!obs) {
		return -ENOENT;
	}

	remove_observer_from_list(msg->ctx, prev_node, obs);

	LOG_DBG("observer '%s' removed", sprint_token(token, tkl));

	return do_composite_read_op_for_parsed_list(msg, format, &lwm2m_path_list);
}

#if defined(CONFIG_LOG)
char *lwm2m_path_log_buf(char *buf, struct lwm2m_obj_path *path)
{
	size_t cur;

	if (!path) {
		sprintf(buf, "/");
		return buf;
	}

	cur = sprintf(buf, "%u", path->obj_id);

	if (path->level > 1) {
		cur += sprintf(buf + cur, "/%u", path->obj_inst_id);
	}
	if (path->level > 2) {
		cur += sprintf(buf + cur, "/%u", path->res_id);
	}
	if (path->level > 3) {
		cur += sprintf(buf + cur, "/%u", path->res_inst_id);
	}

	return buf;
}
#endif /* CONFIG_LOG */

#if defined(CONFIG_LWM2M_CANCEL_OBSERVE_BY_PATH)
static int engine_remove_observer_by_path(struct lwm2m_ctx *ctx, struct lwm2m_obj_path *path)
{
	char buf[LWM2M_MAX_PATH_STR_LEN];
	struct observe_node *obs;
	struct lwm2m_obj_path_list obs_path_list_buf;
	sys_slist_t lwm2m_path_list;
	sys_snode_t *prev_node = NULL;

	/* Create 1 entry linked list for message path */
	memcpy(&obs_path_list_buf.path, path, sizeof(struct lwm2m_obj_path));
	sys_slist_init(&lwm2m_path_list);
	sys_slist_append(&lwm2m_path_list, &obs_path_list_buf.node);

	obs = engine_observe_node_discover(&ctx->observer, &prev_node, &lwm2m_path_list, NULL, 0);
	if (!obs) {
		return -ENOENT;
	}

	LOG_INF("Removing observer for path %s", lwm2m_path_log_buf(buf, path));

	remove_observer_from_list(ctx, prev_node, obs);

	return 0;
}
#endif /* CONFIG_LWM2M_CANCEL_OBSERVE_BY_PATH */

void engine_remove_observer_by_id(uint16_t obj_id, int32_t obj_inst_id)
{
	struct observe_node *obs, *tmp;
	sys_snode_t *prev_node = NULL;
	int i;
	struct lwm2m_ctx **sock_ctx = lwm2m_sock_ctx();

	/* remove observer instances accordingly */
	for (i = 0; i < lwm2m_sock_nfds(); ++i) {
		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&sock_ctx[i]->observer, obs, tmp, node) {
			engine_observe_single_path_id_remove(sock_ctx[i], obs, obj_id, obj_inst_id);

			if (sys_slist_is_empty(&obs->path_list)) {
				remove_observer_from_list(sock_ctx[i], prev_node, obs);
			} else {
				prev_node = &obs->node;
			}
		}
	}
}

static int lwm2m_update_or_allocate_attribute(void *ref, uint8_t type, void *data)
{
	struct lwm2m_attr *attr;
	int i;

	/* find matching attributes */
	for (i = 0; i < CONFIG_LWM2M_NUM_ATTR; i++) {
		if (ref != write_attr_pool[i].ref || write_attr_pool[i].type != type) {
			continue;
		}

		attr = write_attr_pool + i;
		type = attr->type;

		if (type <= LWM2M_ATTR_PMAX) {
			attr->int_val = *(int32_t *)data;
			LOG_DBG("Update %s to %d", LWM2M_ATTR_STR[type], attr->int_val);
		} else {
			attr->float_val = *(double *)data;
			LOG_DBG("Update %s to %f", LWM2M_ATTR_STR[type], attr->float_val);
		}
		return 0;
	}

	/* add attribute to obj/obj_inst/res/res_inst */
	/* grab an entry for newly added attribute */
	for (i = 0; i < CONFIG_LWM2M_NUM_ATTR; i++) {
		if (!write_attr_pool[i].ref) {
			break;
		}
	}

	if (i == CONFIG_LWM2M_NUM_ATTR) {
		return -ENOMEM;
	}

	attr = write_attr_pool + i;
	attr->type = type;
	attr->ref = ref;

	if (type <= LWM2M_ATTR_PMAX) {
		attr->int_val = *(int32_t *)data;
		LOG_DBG("Add %s to %d", LWM2M_ATTR_STR[type], attr->int_val);
	} else {
		attr->float_val = *(double *)data;
		LOG_DBG("Add %s to %f", LWM2M_ATTR_STR[type], attr->float_val);
	}
	return 0;
}

const char *lwm2m_engine_get_attr_name(const struct lwm2m_attr *attr)
{
	if (attr->type >= NR_LWM2M_ATTR) {
		return NULL;
	}

	return LWM2M_ATTR_STR[attr->type];
}

static int lwm2m_engine_observer_timestamp_update(sys_slist_t *observer,
						  struct lwm2m_obj_path *path,
						  uint16_t srv_obj_inst)
{
	struct observe_node *obs;
	struct notification_attrs nattrs = {0};
	int ret;
	int64_t timestamp;

	/* update observe_node accordingly */
	SYS_SLIST_FOR_EACH_CONTAINER(observer, obs, node) {
		if (!obs->resource_update) {
			/* Resource Update on going skip this*/
			continue;
		}
		/* Compare Observation node path to updated one */
		if (!lwm2m_notify_observer_list(&obs->path_list, path)) {
			continue;
		}

		/* Read Attributes after validation Path */
		ret = engine_observe_attribute_list_get(&obs->path_list, &nattrs, srv_obj_inst);
		if (ret < 0) {
			return ret;
		}

		/* Update based on by PMax */
		if (nattrs.pmax) {
			/* Update Current */
			timestamp = obs->last_timestamp + MSEC_PER_SEC * nattrs.pmax;
		} else {
			/* Disable Automatic Notify */
			timestamp = 0;
		}
		obs->event_timestamp = timestamp;

		(void)memset(&nattrs, 0, sizeof(nattrs));
	}
	return 0;
}

/* input / output selection */

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
		obj_inst = get_engine_obj_inst(path->obj_id, path->obj_inst_id);
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

int lwm2m_engine_update_observer_min_period(struct lwm2m_ctx *client_ctx, const char *pathstr,
					    uint32_t period_s)
{
	int ret;
	struct lwm2m_obj_path path;
	struct notification_attrs nattrs = {0};
	struct notification_attrs attrs = {0};
	void *ref;

	ret = lwm2m_string_to_path(pathstr, &path, '/');
	if (ret < 0) {
		return ret;
	}

	/* Read Reference pointer to attribute */
	ret = lwm2m_get_path_reference_ptr(NULL, &path, &ref);
	if (ret < 0) {
		return ret;
	}
	/* retrieve existing attributes */
	ret = update_attrs(ref, &nattrs);
	if (ret < 0) {
		return ret;
	}

	if (nattrs.flags & BIT(LWM2M_ATTR_PMIN) && nattrs.pmin == period_s) {
		/* No need to change */
		return 0;
	}

	/* Read Pmin & Pmax values for path */
	ret = engine_observe_get_attributes(&path, &attrs, client_ctx->srv_obj_inst);
	if (ret < 0) {
		return ret;
	}

	if (period_s && attrs.pmax && attrs.pmax < period_s) {
		LOG_DBG("New pmin (%d) > pmax (%d)", period_s, attrs.pmax);
		return -EEXIST;
	}

	return lwm2m_update_or_allocate_attribute(ref, LWM2M_ATTR_PMIN, &period_s);
}

int lwm2m_engine_update_observer_max_period(struct lwm2m_ctx *client_ctx, const char *pathstr,
					    uint32_t period_s)
{
	int ret;
	struct lwm2m_obj_path path;
	void *ref;
	struct notification_attrs nattrs = {0};
	struct notification_attrs attrs = {0};

	ret = lwm2m_string_to_path(pathstr, &path, '/');
	if (ret < 0) {
		return ret;
	}

	/* Read Reference pointer to attribute */
	ret = lwm2m_get_path_reference_ptr(NULL, &path, &ref);
	if (ret < 0) {
		return ret;
	}
	/* retrieve existing attributes */
	ret = update_attrs(ref, &nattrs);
	if (ret < 0) {
		return ret;
	}

	if (nattrs.flags & BIT(LWM2M_ATTR_PMAX) && nattrs.pmax == period_s) {
		/* No need to change */
		return 0;
	}

	/* Read Pmin & Pmax values for path */
	ret = engine_observe_get_attributes(&path, &attrs, client_ctx->srv_obj_inst);
	if (ret < 0) {
		return ret;
	}

	if (period_s && attrs.pmin > period_s) {
		LOG_DBG("pmin (%d) > new pmax (%d)", attrs.pmin, period_s);
		return -EEXIST;
	}

	/* Update or allocate new */
	ret = lwm2m_update_or_allocate_attribute(ref, LWM2M_ATTR_PMAX, &period_s);
	if (ret < 0) {
		return ret;
	}

	/* Update Observer timestamp */
	return lwm2m_engine_observer_timestamp_update(&client_ctx->observer, &path,
						      client_ctx->srv_obj_inst);
}

struct lwm2m_attr *lwm2m_engine_get_next_attr(const void *ref, struct lwm2m_attr *prev)
{
	struct lwm2m_attr *iter = (prev == NULL) ? write_attr_pool : prev + 1;
	struct lwm2m_attr *result = NULL;

	if (!PART_OF_ARRAY(write_attr_pool, iter)) {
		return NULL;
	}

	while (iter < &write_attr_pool[ARRAY_SIZE(write_attr_pool)]) {
		if (ref == iter->ref) {
			result = iter;
			break;
		}

		++iter;
	}

	return result;
}

int lwm2m_write_attr_handler(struct lwm2m_engine_obj *obj, struct lwm2m_message *msg)
{
	bool update_observe_node = false;
	char opt_buf[COAP_OPTION_BUF_LEN];
	int nr_opt, i, ret = 0;
	struct coap_option options[NR_LWM2M_ATTR];
	struct lwm2m_attr *attr;
	struct notification_attrs nattrs = {0};
	uint8_t type = 0U;
	void *nattr_ptrs[NR_LWM2M_ATTR] = {&nattrs.pmin, &nattrs.pmax, &nattrs.gt, &nattrs.lt,
					   &nattrs.st};
	void *ref;

	if (!obj || !msg) {
		return -EINVAL;
	}

	/* do not expose security obj */
	if (obj->obj_id == LWM2M_OBJECT_SECURITY_ID) {
		return -ENOENT;
	}

	nr_opt = coap_find_options(msg->in.in_cpkt, COAP_OPTION_URI_QUERY, options, NR_LWM2M_ATTR);
	if (nr_opt <= 0) {
		LOG_ERR("No attribute found!");
		/* translate as bad request */
		return -EEXIST;
	}

	/* get lwm2m_attr slist */
	ret = lwm2m_get_path_reference_ptr(obj, &msg->path, &ref);
	if (ret < 0) {
		return ret;
	}

	/* retrieve existing attributes */
	ret = update_attrs(ref, &nattrs);
	if (ret < 0) {
		return ret;
	}

	/* loop through options to parse attribute */
	for (i = 0; i < nr_opt; i++) {
		int limit = MIN(options[i].len, 5), plen = 0, vlen;
		struct lwm2m_attr val = {0};

		type = 0U;

		/* search for '=' */
		while (plen < limit && options[i].value[plen] != '=') {
			plen += 1;
		}

		/* either length = 2(gt/lt/st) or = 4(pmin/pmax) */
		if (plen != 2 && plen != 4) {
			continue;
		}

		/* matching attribute name */
		for (type = 0U; type < NR_LWM2M_ATTR; type++) {
			if (LWM2M_ATTR_LEN[type] == plen &&
			    !memcmp(options[i].value, LWM2M_ATTR_STR[type], LWM2M_ATTR_LEN[type])) {
				break;
			}
		}

		/* unrecognized attribute */
		if (type == NR_LWM2M_ATTR) {
			continue;
		}

		/* unset attribute when no value's given */
		if (options[i].len == plen) {
			nattrs.flags &= ~BIT(type);

			(void)memset(nattr_ptrs[type], 0,
				     type <= LWM2M_ATTR_PMAX ? sizeof(int32_t) : sizeof(double));
			continue;
		}

		/* gt/lt/st cannot be assigned to obj/obj_inst unless unset */
		if (plen == 2 && msg->path.level <= 2U) {
			return -EEXIST;
		}

		vlen = options[i].len - plen - 1;
		memcpy(opt_buf, options[i].value + plen + 1, vlen);
		opt_buf[vlen] = '\0';

		/* convert value to integer or float */
		if (plen == 4) {
			char *end;
			long v;

			/* pmin/pmax: integer (sec 5.1.2)
			 * however, negative is non-sense
			 */
			errno = 0;
			v = strtol(opt_buf, &end, 10);
			if (errno || *end || v < 0) {
				ret = -EINVAL;
			}

			val.int_val = v;
		} else {
			/* gt/lt/st: type float */
			ret = lwm2m_atof(opt_buf, &val.float_val);
		}

		if (ret < 0) {
			LOG_ERR("invalid attr[%s] value", LWM2M_ATTR_STR[type]);
			/* bad request */
			return -EEXIST;
		}

		if (type <= LWM2M_ATTR_PMAX) {
			*(int32_t *)nattr_ptrs[type] = val.int_val;
		} else {
			memcpy(nattr_ptrs[type], &val.float_val, sizeof(val.float_val));
		}

		nattrs.flags |= BIT(type);
	}

	if (((nattrs.flags & (BIT(LWM2M_ATTR_PMIN) | BIT(LWM2M_ATTR_PMAX))) ==
	     (BIT(LWM2M_ATTR_PMIN) | BIT(LWM2M_ATTR_PMAX))) &&
	    nattrs.pmin > nattrs.pmax) {
		LOG_DBG("pmin (%d) > pmax (%d)", nattrs.pmin, nattrs.pmax);
		return -EEXIST;
	}

	if ((nattrs.flags & (BIT(LWM2M_ATTR_LT) | BIT(LWM2M_ATTR_GT))) ==
	    (BIT(LWM2M_ATTR_LT) | BIT(LWM2M_ATTR_GT))) {
		if (nattrs.lt > nattrs.gt) {
			LOG_DBG("lt > gt");
			return -EEXIST;
		}

		if (nattrs.flags & BIT(LWM2M_ATTR_STEP)) {
			if (nattrs.lt + 2 * nattrs.st > nattrs.gt) {
				LOG_DBG("lt + 2*st > gt");
				return -EEXIST;
			}
		}
	}

	/* find matching attributes */
	for (i = 0; i < CONFIG_LWM2M_NUM_ATTR; i++) {
		if (ref != write_attr_pool[i].ref) {
			continue;
		}

		attr = write_attr_pool + i;
		type = attr->type;

		if (!(BIT(type) & nattrs.flags)) {
			LOG_DBG("Unset attr %s", LWM2M_ATTR_STR[type]);
			(void)memset(attr, 0, sizeof(*attr));

			if (type <= LWM2M_ATTR_PMAX) {
				update_observe_node = true;
			}

			continue;
		}

		nattrs.flags &= ~BIT(type);

		if (type <= LWM2M_ATTR_PMAX) {
			if (attr->int_val == *(int32_t *)nattr_ptrs[type]) {
				continue;
			}

			attr->int_val = *(int32_t *)nattr_ptrs[type];
			update_observe_node = true;

			LOG_DBG("Update %s to %d", LWM2M_ATTR_STR[type], attr->int_val);
		} else {
			if (attr->float_val == *(double *)nattr_ptrs[type]) {
				continue;
			}

			attr->float_val = *(double *)nattr_ptrs[type];

			LOG_DBG("Update %s to %f", LWM2M_ATTR_STR[type], attr->float_val);
		}
	}

	/* add attribute to obj/obj_inst/res/res_inst */
	for (type = 0U; nattrs.flags && type < NR_LWM2M_ATTR; type++) {
		if (!(BIT(type) & nattrs.flags)) {
			continue;
		}

		/* grab an entry for newly added attribute */
		for (i = 0; i < CONFIG_LWM2M_NUM_ATTR; i++) {
			if (!write_attr_pool[i].ref) {
				break;
			}
		}

		if (i == CONFIG_LWM2M_NUM_ATTR) {
			return -ENOMEM;
		}

		attr = write_attr_pool + i;
		attr->type = type;
		attr->ref = ref;

		if (type <= LWM2M_ATTR_PMAX) {
			attr->int_val = *(int32_t *)nattr_ptrs[type];
			update_observe_node = true;

			LOG_DBG("Add %s to %d", LWM2M_ATTR_STR[type], attr->int_val);
		} else {
			attr->float_val = *(double *)nattr_ptrs[type];

			LOG_DBG("Add %s to %f", LWM2M_ATTR_STR[type], attr->float_val);
		}

		nattrs.flags &= ~BIT(type);
	}

	/* check only pmin/pmax */
	if (!update_observe_node) {
		return 0;
	}

	lwm2m_engine_observer_timestamp_update(&msg->ctx->observer, &msg->path,
					       msg->ctx->srv_obj_inst);

	return 0;
}

bool lwm2m_engine_path_is_observed(const char *pathstr)
{
	struct observe_node *obs;
	struct lwm2m_obj_path path;
	int ret;
	int i;
	struct lwm2m_ctx **sock_ctx = lwm2m_sock_ctx();

	ret = lwm2m_string_to_path(pathstr, &path, '/');
	if (ret < 0) {
		return false;
	}

	for (i = 0; i < lwm2m_sock_nfds(); ++i) {
		SYS_SLIST_FOR_EACH_CONTAINER(&sock_ctx[i]->observer, obs, node) {

			if (lwm2m_notify_observer_list(&obs->path_list, &path)) {
				return true;
			}
		}
	}
	return false;
}

int lwm2m_engine_observation_handler(struct lwm2m_message *msg, int observe, uint16_t accept,
				     bool composite)
{
	int r;

	if (observe == 0) {
		/* add new observer */
		r = coap_append_option_int(msg->out.out_cpkt, COAP_OPTION_OBSERVE,
					   OBSERVE_COUNTER_START);
		if (r < 0) {
			LOG_ERR("OBSERVE option error: %d", r);
			return r;
		}

		if (composite) {
			r = engine_add_composite_observer(msg, msg->token, msg->tkl, accept);
		} else {
			r = engine_add_observer(msg, msg->token, msg->tkl, accept);
		}

		if (r < 0) {
			LOG_ERR("add OBSERVE error: %d", r);
		}
	} else if (observe == 1) {
		/* remove observer */
		if (composite) {
			r = engine_remove_composite_observer(msg, msg->token, msg->tkl, accept);
		} else {
			r = engine_remove_observer_by_token(msg->ctx, msg->token, msg->tkl);
			if (r < 0) {
#if defined(CONFIG_LWM2M_CANCEL_OBSERVE_BY_PATH)
				r = engine_remove_observer_by_path(msg->ctx, &msg->path);
				if (r < 0)
#endif /* CONFIG_LWM2M_CANCEL_OBSERVE_BY_PATH */
				{
					LOG_ERR("remove observe error: %d", r);
					r = 0;
				}
			}
		}

	} else {
		r = -EINVAL;
	}
	return r;
}

int64_t engine_observe_shedule_next_event(struct observe_node *obs, uint16_t srv_obj_inst,
					  const int64_t timestamp)
{
	struct notification_attrs attrs;
	int64_t t_s = 0;
	int ret;

	ret = engine_observe_attribute_list_get(&obs->path_list, &attrs, srv_obj_inst);
	if (ret < 0) {
		return 0;
	}

	if (attrs.pmax) {
		t_s = timestamp + MSEC_PER_SEC * attrs.pmax;
	}

	return t_s;
}

struct lwm2m_obj_path_list *lwm2m_engine_get_from_list(sys_slist_t *path_list)
{
	sys_snode_t *path_node = sys_slist_get(path_list);
	struct lwm2m_obj_path_list *entry;

	if (!path_node) {
		return NULL;
	}

	entry = SYS_SLIST_CONTAINER(path_node, entry, node);
	if (entry) {
		memset(entry, 0, sizeof(struct lwm2m_obj_path_list));
	}
	return entry;
}

void lwm2m_engine_free_list(sys_slist_t *path_list, sys_slist_t *free_list)
{
	sys_snode_t *node;

	while (NULL != (node = sys_slist_get(path_list))) {
		/* Add to free list */
		sys_slist_append(free_list, node);
	}
}

static bool lwm2m_path_object_compare(struct lwm2m_obj_path *path,
				      struct lwm2m_obj_path *compare_path)
{
	if (path->level != compare_path->level || path->obj_id != compare_path->obj_id ||
	    path->obj_inst_id != compare_path->obj_inst_id ||
	    path->res_id != compare_path->res_id ||
	    path->res_inst_id != compare_path->res_inst_id) {
		return false;
	}
	return true;
}

void lwm2m_engine_path_list_init(sys_slist_t *lwm2m_path_list, sys_slist_t *lwm2m_free_list,
				 struct lwm2m_obj_path_list path_object_buf[],
				 uint8_t path_object_size)
{
	/* Init list */
	sys_slist_init(lwm2m_path_list);
	sys_slist_init(lwm2m_free_list);

	/* Put buffer elements to free list */
	for (int i = 0; i < path_object_size; i++) {
		sys_slist_append(lwm2m_free_list, &path_object_buf[i].node);
	}
}

int lwm2m_engine_add_path_to_list(sys_slist_t *lwm2m_path_list, sys_slist_t *lwm2m_free_list,
				  struct lwm2m_obj_path *path)
{
	struct lwm2m_obj_path_list *prev = NULL;
	struct lwm2m_obj_path_list *entry;
	struct lwm2m_obj_path_list *new_entry;
	bool add_before_current = false;

	if (path->level == LWM2M_PATH_LEVEL_NONE) {
		/* Clear the list if we are adding the root path which includes all */
		lwm2m_engine_free_list(lwm2m_path_list, lwm2m_free_list);
	}

	/* Check is it at list already here */
	new_entry = lwm2m_engine_get_from_list(lwm2m_free_list);
	if (!new_entry) {
		return -1;
	}

	new_entry->path = *path;
	if (!sys_slist_is_empty(lwm2m_path_list)) {

		/* Keep list Ordered by Object ID/ Object instance/ resource ID */
		SYS_SLIST_FOR_EACH_CONTAINER(lwm2m_path_list, entry, node) {
			if (entry->path.level == LWM2M_PATH_LEVEL_NONE ||
			    lwm2m_path_object_compare(&entry->path, &new_entry->path)) {
				/* Already Root request at list or current path is at list */
				sys_slist_append(lwm2m_free_list, &new_entry->node);
				return 0;
			}

			if (entry->path.obj_id > path->obj_id) {
				/* New entry have smaller Object ID */
				add_before_current = true;
			} else if (entry->path.obj_id == path->obj_id &&
				   entry->path.level > path->level) {
				add_before_current = true;
			} else if (entry->path.obj_id == path->obj_id &&
				   entry->path.level == path->level) {
				if (path->level >= LWM2M_PATH_LEVEL_OBJECT_INST &&
				    entry->path.obj_inst_id > path->obj_inst_id) {
					/*
					 * New have same Object ID
					 * but smaller Object Instance ID
					 */
					add_before_current = true;
				} else if (path->level >= LWM2M_PATH_LEVEL_RESOURCE &&
					   entry->path.obj_inst_id == path->obj_inst_id &&
					   entry->path.res_id > path->res_id) {
					/*
					 * Object ID and Object Instance id same
					 * but Resource ID is smaller
					 */
					add_before_current = true;
				} else if (path->level >= LWM2M_PATH_LEVEL_RESOURCE_INST &&
					   entry->path.obj_inst_id == path->obj_inst_id &&
					   entry->path.res_id == path->res_id &&
					   entry->path.res_inst_id > path->res_inst_id) {
					/*
					 * Object ID, Object Instance id & Resource ID same
					 * but Resource instance ID is smaller
					 */
					add_before_current = true;
				}
			}

			if (add_before_current) {
				if (prev) {
					sys_slist_insert(lwm2m_path_list, &prev->node,
							 &new_entry->node);
				} else {
					sys_slist_prepend(lwm2m_path_list, &new_entry->node);
				}
				return 0;
			}
			prev = entry;
		}
	}

	/* Add First or new tail entry */
	sys_slist_append(lwm2m_path_list, &new_entry->node);
	return 0;
}

void lwm2m_engine_clear_duplicate_path(sys_slist_t *lwm2m_path_list, sys_slist_t *lwm2m_free_list)
{
	struct lwm2m_obj_path_list *prev = NULL;
	struct lwm2m_obj_path_list *entry, *tmp;
	bool remove_entry;

	if (sys_slist_is_empty(lwm2m_path_list)) {
		return;
	}

	/* Keep list Ordered but remove if shorter path is similar */
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(lwm2m_path_list, entry, tmp, node) {

		if (prev && prev->path.level < entry->path.level) {
			if (prev->path.level == LWM2M_PATH_LEVEL_OBJECT &&
			    entry->path.obj_id == prev->path.obj_id) {
				remove_entry = true;
			} else if (prev->path.level == LWM2M_PATH_LEVEL_OBJECT_INST &&
				   entry->path.obj_id == prev->path.obj_id &&
				   entry->path.obj_inst_id == prev->path.obj_inst_id) {
				/* Remove current from the list */
				remove_entry = true;
			} else if (prev->path.level == LWM2M_PATH_LEVEL_RESOURCE &&
				   entry->path.obj_id == prev->path.obj_id &&
				   entry->path.obj_inst_id == prev->path.obj_inst_id &&
				   entry->path.res_id == prev->path.res_id) {
				/* Remove current from the list */
				remove_entry = true;
			} else {
				remove_entry = false;
			}

			if (remove_entry) {
				/* Remove Current entry */
				sys_slist_remove(lwm2m_path_list, &prev->node, &entry->node);
				sys_slist_append(lwm2m_free_list, &entry->node);
			} else {
				prev = entry;
			}
		} else {
			prev = entry;
		}
	}
}
