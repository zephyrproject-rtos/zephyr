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

/*
 * TODO:
 * - Handle Resource ObjLink type
 */

#define LOG_MODULE_NAME net_lwm2m_engine
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <init.h>
#include <sys/printk.h>
#include <net/net_ip.h>
#include <net/http_parser_url.h>
#include <net/socket.h>
#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
#include <net/tls_credentials.h>
#endif
#if defined(CONFIG_DNS_RESOLVER)
#include <net/dns_resolve.h>
#endif

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include "lwm2m_rw_plain_text.h"
#include "lwm2m_rw_oma_tlv.h"
#ifdef CONFIG_LWM2M_RW_JSON_SUPPORT
#include "lwm2m_rw_json.h"
#endif
#ifdef CONFIG_LWM2M_RD_CLIENT_SUPPORT
#include "lwm2m_rd_client.h"
#endif

#define ENGINE_UPDATE_INTERVAL_MS 500

#define WELL_KNOWN_CORE_PATH	"</.well-known/core>"

/*
 * TODO: to implement a way for clients to specify alternate path
 * via Kconfig (LwM2M specification 8.2.2 Alternate Path)
 *
 * For now, in order to inform server we support JSON format, we have to
 * report 'ct=11543' to the server. '</>' is required in order to append
 * content attribute. And resource type attribute is appended because of
 * Eclipse wakaama will reject the registration when 'rt="oma.lwm2m"' is
 * missing.
 */

#define RESOURCE_TYPE		";rt=\"oma.lwm2m\""

#if defined(CONFIG_LWM2M_RW_JSON_SUPPORT)
#define REG_PREFACE		"</>" RESOURCE_TYPE \
				";ct=" STRINGIFY(LWM2M_FORMAT_OMA_JSON)
#else
#define REG_PREFACE		""
#endif

#if defined(CONFIG_COAP_EXTENDED_OPTIONS_LEN)
#define	COAP_OPTION_BUF_LEN	(CONFIG_COAP_EXTENDED_OPTIONS_LEN_VALUE + 1)
#else
#define	COAP_OPTION_BUF_LEN	13
#endif

#define MAX_TOKEN_LEN		8

struct observe_node {
	sys_snode_t node;
	struct lwm2m_ctx *ctx;
	struct lwm2m_obj_path path;
	u8_t  token[MAX_TOKEN_LEN];
	s64_t event_timestamp;
	s64_t last_timestamp;
	u32_t min_period_sec;
	u32_t max_period_sec;
	u32_t counter;
	u16_t format;
	u8_t  tkl;
};

struct notification_attrs {
	/* use to determine which value is set */
	float32_value_t gt;
	float32_value_t lt;
	float32_value_t st;
	s32_t pmin;
	s32_t pmax;
	u8_t flags;
};

static struct observe_node observe_node_data[CONFIG_LWM2M_ENGINE_MAX_OBSERVER];

#define MAX_PERIODIC_SERVICE	10

struct service_node {
	sys_snode_t node;
	k_work_handler_t service_work;
	u32_t min_call_period; /* ms */
	u64_t last_timestamp; /* ms */
};

static struct service_node service_node_data[MAX_PERIODIC_SERVICE];

static sys_slist_t engine_obj_list;
static sys_slist_t engine_obj_inst_list;
static sys_slist_t engine_observer_list;
static sys_slist_t engine_service_list;

static K_THREAD_STACK_DEFINE(engine_thread_stack,
			      CONFIG_LWM2M_ENGINE_STACK_SIZE);
static struct k_thread engine_thread_data;

#define MAX_POLL_FD		CONFIG_NET_SOCKETS_POLL_MAX

static struct lwm2m_ctx *sock_ctx[MAX_POLL_FD];
static struct pollfd sock_fds[MAX_POLL_FD];
static int sock_nfds;

#define NUM_BLOCK1_CONTEXT	CONFIG_LWM2M_NUM_BLOCK1_CONTEXT

/* TODO: figure out what's correct value */
#define TIMEOUT_BLOCKWISE_TRANSFER_MS (MSEC_PER_SEC * 30)

#define GET_BLOCK_NUM(v)	((v) >> 4)
#define GET_BLOCK_SIZE(v)	(((v) & 0x7))
#define GET_MORE(v)		(!!((v) & 0x08))

struct block_context {
	struct coap_block_context ctx;
	s64_t timestamp;
	u8_t token[8];
	u8_t tkl;
};

static struct block_context block1_contexts[NUM_BLOCK1_CONTEXT];

/* write-attribute related definitons */
static const char * const LWM2M_ATTR_STR[] = { "pmin", "pmax",
					       "gt", "lt", "st" };
static const u8_t LWM2M_ATTR_LEN[] = { 4, 4, 2, 2, 2 };

static struct lwm2m_attr write_attr_pool[CONFIG_LWM2M_NUM_ATTR];

static struct lwm2m_engine_obj *get_engine_obj(int obj_id);
static struct lwm2m_engine_obj_inst *get_engine_obj_inst(int obj_id,
							 int obj_inst_id);

/* Shared set of in-flight LwM2M messages */
static struct lwm2m_message messages[CONFIG_LWM2M_ENGINE_MAX_MESSAGES];

/* for debugging: to print IP addresses */
char *lwm2m_sprint_ip_addr(const struct sockaddr *addr)
{
	static char buf[NET_IPV6_ADDR_LEN];

	if (addr->sa_family == AF_INET6) {
		return net_addr_ntop(AF_INET6, &net_sin6(addr)->sin6_addr,
				     buf, sizeof(buf));
	}

	if (addr->sa_family == AF_INET) {
		return net_addr_ntop(AF_INET, &net_sin(addr)->sin_addr,
				     buf, sizeof(buf));
	}

	LOG_ERR("Unknown IP address family:%d", addr->sa_family);
	strcpy(buf, "unk");
	return buf;
}

static u8_t to_hex_digit(u8_t digit)
{
	if (digit >= 10U) {
		return digit - 10U + 'a';
	}

	return digit + '0';
}

static char *sprint_token(const u8_t *token, u8_t tkl)
{
	static char buf[32];
	char *ptr = buf;

	if (token && tkl != LWM2M_MSG_TOKEN_LEN_SKIP) {
		int i;

		tkl = MIN(tkl, sizeof(buf) / 2 - 1);

		for (i = 0; i < tkl; i++) {
			*ptr++ = to_hex_digit(token[i] >> 4);
			*ptr++ = to_hex_digit(token[i] & 0x0F);
		}

		*ptr = '\0';
	} else if (tkl == LWM2M_MSG_TOKEN_LEN_SKIP) {
		strcpy(buf, "[skip-token]");
	} else {
		strcpy(buf, "[no-token]");
	}

	return buf;
}

/* block-wise transfer functions */

enum coap_block_size lwm2m_default_block_size(void)
{
	switch (CONFIG_LWM2M_COAP_BLOCK_SIZE) {
	case 16:
		return COAP_BLOCK_16;
	case 32:
		return COAP_BLOCK_32;
	case 64:
		return COAP_BLOCK_64;
	case 128:
		return COAP_BLOCK_128;
	case 256:
		return COAP_BLOCK_256;
	case 512:
		return COAP_BLOCK_512;
	case 1024:
		return COAP_BLOCK_1024;
	}

	return COAP_BLOCK_256;
}

static int
init_block_ctx(const u8_t *token, u8_t tkl, struct block_context **ctx)
{
	int i;
	s64_t timestamp;

	*ctx = NULL;
	timestamp = k_uptime_get();
	for (i = 0; i < NUM_BLOCK1_CONTEXT; i++) {
		if (block1_contexts[i].tkl == 0U) {
			*ctx = &block1_contexts[i];
			break;
		}

		if (timestamp - block1_contexts[i].timestamp >
		    TIMEOUT_BLOCKWISE_TRANSFER_MS) {
			*ctx = &block1_contexts[i];
			/* TODO: notify application for block
			 * transfer timeout
			 */
			break;
		}
	}

	if (*ctx == NULL) {
		LOG_ERR("Cannot find free block context");
		return -ENOMEM;
	}

	(*ctx)->tkl = tkl;
	memcpy((*ctx)->token, token, tkl);
	coap_block_transfer_init(&(*ctx)->ctx, lwm2m_default_block_size(), 0);
	(*ctx)->timestamp = timestamp;

	return 0;
}

static int
get_block_ctx(const u8_t *token, u8_t tkl, struct block_context **ctx)
{
	int i;

	*ctx = NULL;

	for (i = 0; i < NUM_BLOCK1_CONTEXT; i++) {
		if (block1_contexts[i].tkl == tkl &&
		    memcmp(token, block1_contexts[i].token, tkl) == 0) {
			*ctx = &block1_contexts[i];
			/* refresh timestamp */
			(*ctx)->timestamp = k_uptime_get();
			break;
		}
	}

	if (*ctx == NULL) {
		LOG_ERR("Cannot find block context");
		return -ENOENT;
	}

	return 0;
}

static void free_block_ctx(struct block_context *ctx)
{
	if (ctx == NULL) {
		return;
	}

	ctx->tkl = 0U;
}

/* observer functions */

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
			LOG_ERR("Unrecognize attr: %d",
				write_attr_pool[i].type);
			return -EINVAL;
		}

		/* mark as set */
		out->flags |= BIT(write_attr_pool[i].type);
	}

	return 0;
}

static void clear_attrs(void *ref)
{
	int i;

	for (i = 0; i < CONFIG_LWM2M_NUM_ATTR; i++) {
		if (ref == write_attr_pool[i].ref) {
			(void)memset(&write_attr_pool[i], 0,
				     sizeof(write_attr_pool[i]));
		}
	}
}

int lwm2m_notify_observer(u16_t obj_id, u16_t obj_inst_id, u16_t res_id)
{
	struct observe_node *obs;
	int ret = 0;

	/* look for observers which match our resource */
	SYS_SLIST_FOR_EACH_CONTAINER(&engine_observer_list, obs, node) {
		if (obs->path.obj_id == obj_id &&
		    obs->path.obj_inst_id == obj_inst_id &&
		    (obs->path.level < 3 ||
		     obs->path.res_id == res_id)) {
			/* update the event time for this observer */
			obs->event_timestamp = k_uptime_get();

			LOG_DBG("NOTIFY EVENT %u/%u/%u",
				obj_id, obj_inst_id, res_id);

			ret++;
		}
	}

	return ret;
}

int lwm2m_notify_observer_path(struct lwm2m_obj_path *path)
{
	return lwm2m_notify_observer(path->obj_id, path->obj_inst_id,
				     path->res_id);
}

static int engine_add_observer(struct lwm2m_message *msg,
			       const u8_t *token, u8_t tkl,
			       u16_t format)
{
	struct lwm2m_engine_obj *obj = NULL;
	struct lwm2m_engine_obj_field *obj_field = NULL;
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	struct observe_node *obs;
	struct notification_attrs attrs = {
		.flags = BIT(LWM2M_ATTR_PMIN) | BIT(LWM2M_ATTR_PMAX),
	};
	int i, ret;

	if (!msg || !msg->ctx) {
		LOG_ERR("valid lwm2m message is required");
		return -EINVAL;
	}

	if (!token || (tkl == 0U || tkl > MAX_TOKEN_LEN)) {
		LOG_ERR("token(%p) and token length(%u) must be valid.",
			token, tkl);
		return -EINVAL;
	}

	/* defaults from server object */
	attrs.pmin = lwm2m_server_get_pmin(msg->ctx->sec_obj_inst);
	attrs.pmax = lwm2m_server_get_pmax(msg->ctx->sec_obj_inst);

	/* TODO: observe dup checking */

	/* make sure this observer doesn't exist already */
	SYS_SLIST_FOR_EACH_CONTAINER(&engine_observer_list, obs, node) {
		/* TODO: distinguish server object */
		if (obs->ctx == msg->ctx &&
		    memcmp(&obs->path, &msg->path, sizeof(msg->path)) == 0) {
			/* quietly update the token information */
			memcpy(obs->token, token, tkl);
			obs->tkl = tkl;

			LOG_DBG("OBSERVER DUPLICATE %u/%u/%u(%u) [%s]",
				msg->path.obj_id, msg->path.obj_inst_id,
				msg->path.res_id, msg->path.level,
				log_strdup(
				lwm2m_sprint_ip_addr(&msg->ctx->remote_addr)));

			return 0;
		}
	}

	/* check if object exists */
	obj = get_engine_obj(msg->path.obj_id);
	if (!obj) {
		LOG_ERR("unable to find obj: %u", msg->path.obj_id);
		return -ENOENT;
	}

	ret = update_attrs(obj, &attrs);
	if (ret < 0) {
		return ret;
	}

	/* check if object instance exists */
	if (msg->path.level >= 2U) {
		obj_inst = get_engine_obj_inst(msg->path.obj_id,
					       msg->path.obj_inst_id);
		if (!obj_inst) {
			LOG_ERR("unable to find obj_inst: %u/%u",
				msg->path.obj_id, msg->path.obj_inst_id);
			return -ENOENT;
		}

		ret = update_attrs(obj_inst, &attrs);
		if (ret < 0) {
			return ret;
		}
	}

	/* check if resource exists */
	if (msg->path.level >= 3U) {
		for (i = 0; i < obj_inst->resource_count; i++) {
			if (obj_inst->resources[i].res_id == msg->path.res_id) {
				break;
			}
		}

		if (i == obj_inst->resource_count) {
			LOG_ERR("unable to find res_id: %u/%u/%u",
				msg->path.obj_id, msg->path.obj_inst_id,
				msg->path.res_id);
			return -ENOENT;
		}

		/* load object field data */
		obj_field = lwm2m_get_engine_obj_field(obj,
				obj_inst->resources[i].res_id);
		if (!obj_field) {
			LOG_ERR("unable to find obj_field: %u/%u/%u",
				msg->path.obj_id, msg->path.obj_inst_id,
				msg->path.res_id);
			return -ENOENT;
		}

		/* check for READ permission on matching resource */
		if (!LWM2M_HAS_PERM(obj_field, LWM2M_PERM_R)) {
			return -EPERM;
		}

		ret = update_attrs(&obj_inst->resources[i], &attrs);
		if (ret < 0) {
			return ret;
		}
	}

	/* find an unused observer index node */
	for (i = 0; i < CONFIG_LWM2M_ENGINE_MAX_OBSERVER; i++) {
		if (!observe_node_data[i].ctx) {
			break;
		}
	}

	/* couldn't find an index */
	if (i == CONFIG_LWM2M_ENGINE_MAX_OBSERVER) {
		return -ENOMEM;
	}

	/* copy the values and add it to the list */
	observe_node_data[i].ctx = msg->ctx;
	memcpy(&observe_node_data[i].path, &msg->path, sizeof(msg->path));
	memcpy(observe_node_data[i].token, token, tkl);
	observe_node_data[i].tkl = tkl;
	observe_node_data[i].last_timestamp = k_uptime_get();
	observe_node_data[i].event_timestamp =
			observe_node_data[i].last_timestamp;
	observe_node_data[i].min_period_sec = attrs.pmin;
	observe_node_data[i].max_period_sec = MAX(attrs.pmax, attrs.pmin);
	observe_node_data[i].format = format;
	observe_node_data[i].counter = 1U;
	sys_slist_append(&engine_observer_list,
			 &observe_node_data[i].node);

	LOG_DBG("OBSERVER ADDED %u/%u/%u(%u) token:'%s' addr:%s",
		msg->path.obj_id, msg->path.obj_inst_id,
		msg->path.res_id, msg->path.level,
		log_strdup(sprint_token(token, tkl)),
		log_strdup(lwm2m_sprint_ip_addr(&msg->ctx->remote_addr)));

	return 0;
}

static int engine_remove_observer(const u8_t *token, u8_t tkl)
{
	struct observe_node *obs, *found_obj = NULL;
	sys_snode_t *prev_node = NULL;

	if (!token || (tkl == 0U || tkl > MAX_TOKEN_LEN)) {
		LOG_ERR("token(%p) and token length(%u) must be valid.",
			token, tkl);
		return -EINVAL;
	}

	/* find the node index */
	SYS_SLIST_FOR_EACH_CONTAINER(&engine_observer_list, obs, node) {
		if (memcmp(obs->token, token, tkl) == 0) {
			found_obj = obs;
			break;
		}

		prev_node = &obs->node;
	}

	if (!found_obj) {
		return -ENOENT;
	}

	sys_slist_remove(&engine_observer_list, prev_node, &found_obj->node);
	(void)memset(found_obj, 0, sizeof(*found_obj));

	LOG_DBG("observer '%s' removed", log_strdup(sprint_token(token, tkl)));

	return 0;
}

static void engine_remove_observer_by_id(u16_t obj_id, s32_t obj_inst_id)
{
	struct observe_node *obs, *tmp;
	sys_snode_t *prev_node = NULL;

	/* remove observer instances accordingly */
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(
			&engine_observer_list, obs, tmp, node) {
		if (!(obj_id == obs->path.obj_id &&
		      obj_inst_id == obs->path.obj_inst_id)) {
			prev_node = &obs->node;
			continue;
		}

		sys_slist_remove(&engine_observer_list, prev_node, &obs->node);
		(void)memset(obs, 0, sizeof(*obs));
	}
}

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

static struct lwm2m_engine_obj *get_engine_obj(int obj_id)
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

static struct lwm2m_engine_obj_inst *get_engine_obj_inst(int obj_id,
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

static struct lwm2m_engine_obj_inst *
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

int lwm2m_create_obj_inst(u16_t obj_id, u16_t obj_inst_id,
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

int lwm2m_delete_obj_inst(u16_t obj_id, u16_t obj_inst_id)
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

/* utility functions */

static int get_option_int(const struct coap_packet *cpkt, u8_t opt)
{
	struct coap_option option = {};
	u16_t count = 1U;
	int r;

	r = coap_find_options(cpkt, opt, &option, count);
	if (r <= 0) {
		return -ENOENT;
	}

	return coap_option_value_to_int(&option);
}

static u16_t atou16(u8_t *buf, u16_t buflen, u16_t *len)
{
	u16_t val = 0U;
	u16_t pos = 0U;

	/* we should get a value first - consume all numbers */
	while (pos < buflen && isdigit(buf[pos])) {
		val = val * 10U + (buf[pos] - '0');
		pos++;
	}

	*len = pos;
	return val;
}

static int atof32(const char *input, float32_value_t *out)
{
	char *pos, *end, buf[24];
	long int val;
	s32_t base = 1000000, sign = 1;

	if (!input || !out) {
		return -EINVAL;
	}

	strncpy(buf, input, sizeof(buf) - 1);
	buf[sizeof(buf) - 1] = '\0';

	if (strchr(buf, '-')) {
		sign = -1;
	}

	pos = strchr(buf, '.');
	if (pos) {
		*pos = '\0';
	}

	errno = 0;
	val = strtol(buf, &end, 10);
	if (errno || *end || val < INT_MIN) {
		return -EINVAL;
	}

	out->val1 = (s32_t) val;
	out->val2 = 0;

	if (!pos) {
		return 0;
	}

	while (*(++pos) && base > 1 && isdigit((unsigned char)*pos)) {
		out->val2 = out->val2 * 10 + (*pos - '0');
		base /= 10;
	}

	out->val2 *= sign * base;
	return !*pos || base == 1 ? 0 : -EINVAL;
}

static int coap_options_to_path(struct coap_option *opt, int options_count,
				struct lwm2m_obj_path *path)
{
	u16_t len, *id[4] = { &path->obj_id, &path->obj_inst_id,
			      &path->res_id, &path->res_inst_id };

	path->level = options_count;

	for (int i = 0; i < options_count; i++) {
		*id[i] = atou16(opt[i].value, opt[i].len, &len);
		if (len == 0U || opt[i].len != len) {
			path->level = i;
			break;
		}
	}

	return options_count == path->level ? 0 : -EINVAL;
}

static struct lwm2m_message *find_msg(struct coap_pending *pending,
				      struct coap_reply *reply)
{
	size_t i;

	if (!pending && !reply) {
		return NULL;
	}

	for (i = 0; i < CONFIG_LWM2M_ENGINE_MAX_MESSAGES; i++) {
		if (messages[i].ctx && messages[i].pending == pending) {
			return &messages[i];
		}

		if (messages[i].ctx && messages[i].reply == reply) {
			return &messages[i];
		}
	}

	return NULL;
}

struct lwm2m_message *lwm2m_get_message(struct lwm2m_ctx *client_ctx)
{
	size_t i;

	for (i = 0; i < CONFIG_LWM2M_ENGINE_MAX_MESSAGES; i++) {
		if (!messages[i].ctx) {
			messages[i].ctx = client_ctx;
			return &messages[i];
		}
	}

	return NULL;
}

void lwm2m_reset_message(struct lwm2m_message *msg, bool release)
{
	if (!msg) {
		return;
	}

	if (msg->pending) {
		coap_pending_clear(msg->pending);
	}

	if (msg->reply) {
		/* make sure we want to clear the reply */
		coap_reply_clear(msg->reply);
	}

	if (release) {
		(void)memset(msg, 0, sizeof(*msg));
	} else {
		msg->message_timeout_cb = NULL;
		(void)memset(&msg->cpkt, 0, sizeof(msg->cpkt));
	}
}

int lwm2m_init_message(struct lwm2m_message *msg)
{
	u8_t tokenlen = 0U;
	u8_t *token = NULL;
	int r = 0;

	if (!msg || !msg->ctx) {
		LOG_ERR("LwM2M message is invalid.");
		return -EINVAL;
	}

	/*
	 * msg->tkl == 0 is for a new TOKEN
	 * msg->tkl == LWM2M_MSG_TOKEN_LEN_SKIP means dont set
	 */
	if (msg->tkl == 0U) {
		tokenlen = 0U;
		token = coap_next_token();
	} else if (msg->token && msg->tkl != LWM2M_MSG_TOKEN_LEN_SKIP) {
		tokenlen = msg->tkl;
		token = msg->token;
	}

	r = coap_packet_init(&msg->cpkt, msg->msg_data, sizeof(msg->msg_data),
			     1, msg->type, tokenlen, token, msg->code,
			     (msg->mid > 0 ? msg->mid : coap_next_id()));
	if (r < 0) {
		LOG_ERR("coap packet init error (err:%d)", r);
		goto cleanup;
	}

	/* only TYPE_CON messages need pending tracking / reply handling */
	if (msg->type != COAP_TYPE_CON) {
		return 0;
	}

	msg->pending = coap_pending_next_unused(
				msg->ctx->pendings,
				CONFIG_LWM2M_ENGINE_MAX_PENDING);
	if (!msg->pending) {
		LOG_ERR("Unable to find a free pending to track "
			"retransmissions.");
		r = -ENOMEM;
		goto cleanup;
	}

	r = coap_pending_init(msg->pending, &msg->cpkt, &msg->ctx->remote_addr);
	if (r < 0) {
		LOG_ERR("Unable to initialize a pending "
			"retransmission (err:%d).", r);
		goto cleanup;
	}

	if (msg->reply_cb) {
		msg->reply = coap_reply_next_unused(
				msg->ctx->replies,
				CONFIG_LWM2M_ENGINE_MAX_REPLIES);
		if (!msg->reply) {
			LOG_ERR("No resources for waiting for replies.");
			r = -ENOMEM;
			goto cleanup;
		}

		coap_reply_clear(msg->reply);
		coap_reply_init(msg->reply, &msg->cpkt);
		msg->reply->reply = msg->reply_cb;
	}

	return 0;

cleanup:
	lwm2m_reset_message(msg, true);

	return r;
}

int lwm2m_send_message(struct lwm2m_message *msg)
{
	if (!msg || !msg->ctx) {
		LOG_ERR("LwM2M message is invalid.");
		return -EINVAL;
	}

	if (msg->type == COAP_TYPE_CON) {
		coap_pending_cycle(msg->pending);
	}

	msg->send_attempts++;

	if (send(msg->ctx->sock_fd, msg->cpkt.data, msg->cpkt.offset, 0) < 0) {
		if (msg->type == COAP_TYPE_CON) {
			coap_pending_clear(msg->pending);
		}

		return -errno;
	}

	if (msg->type == COAP_TYPE_CON) {
		s32_t remaining = k_delayed_work_remaining_get(
					&msg->ctx->retransmit_work);

		/* If the item is already pending and its timeout is smaller
		 * than the new one, skip the submission.
		 */
		if (remaining == 0 || remaining > msg->pending->timeout) {
			k_delayed_work_submit(&msg->ctx->retransmit_work,
					      K_MSEC(msg->pending->timeout));
		}
	} else {
		lwm2m_reset_message(msg, true);
	}

	if (IS_ENABLED(CONFIG_LWM2M_RD_CLIENT_SUPPORT) &&
	    IS_ENABLED(CONFIG_LWM2M_QUEUE_MODE_ENABLED)) {
		engine_update_tx_time();
	}

	return 0;
}

u16_t lwm2m_get_rd_data(u8_t *client_data, u16_t size)
{
	struct lwm2m_engine_obj *obj;
	struct lwm2m_engine_obj_inst *obj_inst;
	u8_t temp[32];
	u16_t pos = 0U;
	int len;

	/* Add resource-type/content-type to the registration message */
	memcpy(client_data, REG_PREFACE, sizeof(REG_PREFACE) - 1);
	pos += sizeof(REG_PREFACE) - 1;

	SYS_SLIST_FOR_EACH_CONTAINER(&engine_obj_list, obj, node) {
		/* Security obj MUST NOT be part of registration message */
		if (obj->obj_id == LWM2M_OBJECT_SECURITY_ID) {
			continue;
		}

		/* Only report <OBJ_ID> when no instance available */
		if (obj->instance_count == 0U) {
			len = snprintk(temp, sizeof(temp), "%s</%u>",
				       (pos > 0) ? "," : "", obj->obj_id);
			if (pos + len >= size) {
				/* full buffer -- exit loop */
				break;
			}

			memcpy(&client_data[pos], temp, len);
			pos += len;
			continue;
		}

		SYS_SLIST_FOR_EACH_CONTAINER(&engine_obj_inst_list,
					     obj_inst, node) {
			if (obj_inst->obj->obj_id == obj->obj_id) {
				len = snprintk(temp, sizeof(temp),
					       "%s</%u/%u>",
					       (pos > 0) ? "," : "",
					       obj_inst->obj->obj_id,
					       obj_inst->obj_inst_id);
				/*
				 * TODO: iterate through resources once block
				 * transfer is handled correctly
				 */
				if (pos + len >= size) {
					/* full buffer -- exit loop */
					break;
				}

				memcpy(&client_data[pos], temp, len);
				pos += len;
			}
		}
	}

	client_data[pos] = '\0';
	return pos;
}

/* input / output selection */

static int select_writer(struct lwm2m_output_context *out, u16_t accept)
{
	switch (accept) {

	case LWM2M_FORMAT_APP_LINK_FORMAT:
		/* TODO: rewrite do_discover as content formatter */
		break;

	case LWM2M_FORMAT_PLAIN_TEXT:
	case LWM2M_FORMAT_OMA_PLAIN_TEXT:
		out->writer = &plain_text_writer;
		break;

	case LWM2M_FORMAT_OMA_TLV:
	case LWM2M_FORMAT_OMA_OLD_TLV:
		out->writer = &oma_tlv_writer;
		break;

#ifdef CONFIG_LWM2M_RW_JSON_SUPPORT
	case LWM2M_FORMAT_OMA_JSON:
	case LWM2M_FORMAT_OMA_OLD_JSON:
		out->writer = &json_writer;
		break;
#endif

	default:
		LOG_WRN("Unknown content type %u", accept);
		return -ENOMSG;

	}

	return 0;
}

static int select_reader(struct lwm2m_input_context *in, u16_t format)
{
	switch (format) {

	case LWM2M_FORMAT_APP_OCTET_STREAM:
	case LWM2M_FORMAT_PLAIN_TEXT:
	case LWM2M_FORMAT_OMA_PLAIN_TEXT:
		in->reader = &plain_text_reader;
		break;

	case LWM2M_FORMAT_OMA_TLV:
	case LWM2M_FORMAT_OMA_OLD_TLV:
		in->reader = &oma_tlv_reader;
		break;

#ifdef CONFIG_LWM2M_RW_JSON_SUPPORT
	case LWM2M_FORMAT_OMA_JSON:
	case LWM2M_FORMAT_OMA_OLD_JSON:
		in->reader = &json_reader;
		break;
#endif

	default:
		LOG_WRN("Unknown content type %u", format);
		return -ENOMSG;
	}

	return 0;
}

/* user data setter functions */

static int string_to_path(char *pathstr, struct lwm2m_obj_path *path,
			  char delim)
{
	u16_t value, len;
	int i, tokstart = -1, toklen;
	int end_index = strlen(pathstr) - 1;

	(void)memset(path, 0, sizeof(*path));
	for (i = 0; i <= end_index; i++) {
		/* search for first numeric */
		if (tokstart == -1) {
			if (!isdigit((unsigned char)pathstr[i])) {
				continue;
			}

			tokstart = i;
		}

		/* find delimiter char or end of string */
		if (pathstr[i] == delim || i == end_index) {
			toklen = i - tokstart + 1;

			/* don't process delimiter char */
			if (pathstr[i] == delim) {
				toklen--;
			}

			if (toklen <= 0) {
				continue;
			}

			value = atou16(&pathstr[tokstart], toklen, &len);
			switch (path->level) {

			case 0:
				path->obj_id = value;
				break;

			case 1:
				path->obj_inst_id = value;
				break;

			case 2:
				path->res_id = value;
				break;

			case 3:
				path->res_inst_id = value;
				break;

			default:
				LOG_ERR("invalid level (%d)", path->level);
				return -EINVAL;

			}

			/* increase the path level for each token found */
			path->level++;
			tokstart = -1;
		}
	}

	return 0;
}

static int path_to_objs(const struct lwm2m_obj_path *path,
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
		LOG_ERR("resource %d not found", path->res_id);
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

int lwm2m_engine_create_obj_inst(char *pathstr)
{
	struct lwm2m_obj_path path;
	struct lwm2m_engine_obj_inst *obj_inst;
	int ret = 0;

	LOG_DBG("path:%s", log_strdup(pathstr));

	/* translate path -> path_obj */
	ret = string_to_path(pathstr, &path, '/');
	if (ret < 0) {
		return ret;
	}

	if (path.level != 2U) {
		LOG_ERR("path must have 2 parts");
		return -EINVAL;
	}

	return lwm2m_create_obj_inst(path.obj_id, path.obj_inst_id, &obj_inst);
}

int lwm2m_engine_set_res_data(char *pathstr, void *data_ptr, u16_t data_len,
			      u8_t data_flags)
{
	struct lwm2m_obj_path path;
	struct lwm2m_engine_res_inst *res_inst = NULL;
	int ret = 0;

	/* translate path -> path_obj */
	ret = string_to_path(pathstr, &path, '/');
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
	res_inst->data_ptr = data_ptr;
	res_inst->data_len = data_len;
	res_inst->data_flags = data_flags;

	return ret;
}

static int lwm2m_engine_set(char *pathstr, void *value, u16_t len)
{
	struct lwm2m_obj_path path;
	struct lwm2m_engine_obj_inst *obj_inst;
	struct lwm2m_engine_obj_field *obj_field;
	struct lwm2m_engine_res *res = NULL;
	struct lwm2m_engine_res_inst *res_inst = NULL;
	void *data_ptr = NULL;
	size_t data_len = 0;
	int ret = 0;
	bool changed = false;

	LOG_DBG("path:%s, value:%p, len:%d", log_strdup(pathstr), value, len);

	/* translate path -> path_obj */
	ret = string_to_path(pathstr, &path, '/');
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
	data_len = res_inst->data_len;

	/* allow user to override data elements via callback */
	if (res->pre_write_cb) {
		data_ptr = res->pre_write_cb(obj_inst->obj_inst_id,
					     res->res_id, res_inst->res_inst_id,
					     &data_len);
	}

	if (!data_ptr) {
		LOG_ERR("res instance data pointer is NULL [%u/%u/%u/%u:%u]",
			path.obj_id, path.obj_inst_id, path.res_id,
			path.res_inst_id, path.level);
		return -EINVAL;
	}

	/* check length (note: we add 1 to string length for NULL pad) */
	if (len > res_inst->data_len -
		(obj_field->data_type == LWM2M_RES_TYPE_STRING ? 1 : 0)) {
		LOG_ERR("length %u is too long for res instance %d data",
			len, path.res_id);
		return -ENOMEM;
	}

	if (memcmp(data_ptr, value, len) !=  0) {
		changed = true;
	}

	switch (obj_field->data_type) {

	case LWM2M_RES_TYPE_OPAQUE:
		memcpy((u8_t *)data_ptr, value, len);
		break;

	case LWM2M_RES_TYPE_STRING:
		memcpy((u8_t *)data_ptr, value, len);
		((u8_t *)data_ptr)[len] = '\0';
		break;

	case LWM2M_RES_TYPE_U64:
		*((u64_t *)data_ptr) = *(u64_t *)value;
		break;

	case LWM2M_RES_TYPE_U32:
	case LWM2M_RES_TYPE_TIME:
		*((u32_t *)data_ptr) = *(u32_t *)value;
		break;

	case LWM2M_RES_TYPE_U16:
		*((u16_t *)data_ptr) = *(u16_t *)value;
		break;

	case LWM2M_RES_TYPE_U8:
		*((u8_t *)data_ptr) = *(u8_t *)value;
		break;

	case LWM2M_RES_TYPE_S64:
		*((s64_t *)data_ptr) = *(s64_t *)value;
		break;

	case LWM2M_RES_TYPE_S32:
		*((s32_t *)data_ptr) = *(s32_t *)value;
		break;

	case LWM2M_RES_TYPE_S16:
		*((s16_t *)data_ptr) = *(s16_t *)value;
		break;

	case LWM2M_RES_TYPE_S8:
		*((s8_t *)data_ptr) = *(s8_t *)value;
		break;

	case LWM2M_RES_TYPE_BOOL:
		*((bool *)data_ptr) = *(bool *)value;
		break;

	case LWM2M_RES_TYPE_FLOAT32:
		((float32_value_t *)data_ptr)->val1 =
				((float32_value_t *)value)->val1;
		((float32_value_t *)data_ptr)->val2 =
				((float32_value_t *)value)->val2;
		break;

	case LWM2M_RES_TYPE_FLOAT64:
		((float64_value_t *)data_ptr)->val1 =
				((float64_value_t *)value)->val1;
		((float64_value_t *)data_ptr)->val2 =
				((float64_value_t *)value)->val2;
		break;

	default:
		LOG_ERR("unknown obj data_type %d", obj_field->data_type);
		return -EINVAL;

	}

	if (res->post_write_cb) {
		ret = res->post_write_cb(obj_inst->obj_inst_id,
					 res->res_id, res_inst->res_inst_id,
					 data_ptr, len, false, 0);
	}

	if (changed) {
		NOTIFY_OBSERVER_PATH(&path);
	}

	return ret;
}

int lwm2m_engine_set_opaque(char *pathstr, char *data_ptr, u16_t data_len)
{
	return lwm2m_engine_set(pathstr, data_ptr, data_len);
}

int lwm2m_engine_set_string(char *pathstr, char *data_ptr)
{
	return lwm2m_engine_set(pathstr, data_ptr, strlen(data_ptr));
}

int lwm2m_engine_set_u8(char *pathstr, u8_t value)
{
	return lwm2m_engine_set(pathstr, &value, 1);
}

int lwm2m_engine_set_u16(char *pathstr, u16_t value)
{
	return lwm2m_engine_set(pathstr, &value, 2);
}

int lwm2m_engine_set_u32(char *pathstr, u32_t value)
{
	return lwm2m_engine_set(pathstr, &value, 4);
}

int lwm2m_engine_set_u64(char *pathstr, u64_t value)
{
	return lwm2m_engine_set(pathstr, &value, 8);
}

int lwm2m_engine_set_s8(char *pathstr, s8_t value)
{
	return lwm2m_engine_set(pathstr, &value, 1);
}

int lwm2m_engine_set_s16(char *pathstr, s16_t value)
{
	return lwm2m_engine_set(pathstr, &value, 2);
}

int lwm2m_engine_set_s32(char *pathstr, s32_t value)
{
	return lwm2m_engine_set(pathstr, &value, 4);
}

int lwm2m_engine_set_s64(char *pathstr, s64_t value)
{
	return lwm2m_engine_set(pathstr, &value, 8);
}

int lwm2m_engine_set_bool(char *pathstr, bool value)
{
	u8_t temp = (value != 0 ? 1 : 0);

	return lwm2m_engine_set(pathstr, &temp, 1);
}

int lwm2m_engine_set_float32(char *pathstr, float32_value_t *value)
{
	return lwm2m_engine_set(pathstr, value, sizeof(float32_value_t));
}

int lwm2m_engine_set_float64(char *pathstr, float64_value_t *value)
{
	return lwm2m_engine_set(pathstr, value, sizeof(float64_value_t));
}

/* user data getter functions */

int lwm2m_engine_get_res_data(char *pathstr, void **data_ptr, u16_t *data_len,
			      u8_t *data_flags)
{
	struct lwm2m_obj_path path;
	struct lwm2m_engine_res_inst *res_inst = NULL;
	int ret = 0;

	/* translate path -> path_obj */
	ret = string_to_path(pathstr, &path, '/');
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

	*data_ptr = res_inst->data_ptr;
	*data_len = res_inst->data_len;
	*data_flags = res_inst->data_flags;

	return 0;
}

static int lwm2m_engine_get(char *pathstr, void *buf, u16_t buflen)
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
	ret = string_to_path(pathstr, &path, '/');
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
			strncpy((u8_t *)buf, (u8_t *)data_ptr, buflen);
			break;

		case LWM2M_RES_TYPE_U64:
			*(u64_t *)buf = *(u64_t *)data_ptr;
			break;

		case LWM2M_RES_TYPE_U32:
		case LWM2M_RES_TYPE_TIME:
			*(u32_t *)buf = *(u32_t *)data_ptr;
			break;

		case LWM2M_RES_TYPE_U16:
			*(u16_t *)buf = *(u16_t *)data_ptr;
			break;

		case LWM2M_RES_TYPE_U8:
			*(u8_t *)buf = *(u8_t *)data_ptr;
			break;

		case LWM2M_RES_TYPE_S64:
			*(s64_t *)buf = *(s64_t *)data_ptr;
			break;

		case LWM2M_RES_TYPE_S32:
			*(s32_t *)buf = *(s32_t *)data_ptr;
			break;

		case LWM2M_RES_TYPE_S16:
			*(s16_t *)buf = *(s16_t *)data_ptr;
			break;

		case LWM2M_RES_TYPE_S8:
			*(s8_t *)buf = *(s8_t *)data_ptr;
			break;

		case LWM2M_RES_TYPE_BOOL:
			*(bool *)buf = *(bool *)data_ptr;
			break;

		case LWM2M_RES_TYPE_FLOAT32:
			((float32_value_t *)buf)->val1 =
				((float32_value_t *)data_ptr)->val1;
			((float32_value_t *)buf)->val2 =
				((float32_value_t *)data_ptr)->val2;
			break;

		case LWM2M_RES_TYPE_FLOAT64:
			((float64_value_t *)buf)->val1 =
				((float64_value_t *)data_ptr)->val1;
			((float64_value_t *)buf)->val2 =
				((float64_value_t *)data_ptr)->val2;
			break;

		default:
			LOG_ERR("unknown obj data_type %d",
				obj_field->data_type);
			return -EINVAL;

		}
	}

	return 0;
}

int lwm2m_engine_get_opaque(char *pathstr, void *buf, u16_t buflen)
{
	return lwm2m_engine_get(pathstr, buf, buflen);
}

int lwm2m_engine_get_string(char *pathstr, void *buf, u16_t buflen)
{
	return lwm2m_engine_get(pathstr, buf, buflen);
}

int lwm2m_engine_get_u8(char *pathstr, u8_t *value)
{
	return lwm2m_engine_get(pathstr, value, 1);
}

int lwm2m_engine_get_u16(char *pathstr, u16_t *value)
{
	return lwm2m_engine_get(pathstr, value, 2);
}

int lwm2m_engine_get_u32(char *pathstr, u32_t *value)
{
	return lwm2m_engine_get(pathstr, value, 4);
}

int lwm2m_engine_get_u64(char *pathstr, u64_t *value)
{
	return lwm2m_engine_get(pathstr, value, 8);
}

int lwm2m_engine_get_s8(char *pathstr, s8_t *value)
{
	return lwm2m_engine_get(pathstr, value, 1);
}

int lwm2m_engine_get_s16(char *pathstr, s16_t *value)
{
	return lwm2m_engine_get(pathstr, value, 2);
}

int lwm2m_engine_get_s32(char *pathstr, s32_t *value)
{
	return lwm2m_engine_get(pathstr, value, 4);
}

int lwm2m_engine_get_s64(char *pathstr, s64_t *value)
{
	return lwm2m_engine_get(pathstr, value, 8);
}

int lwm2m_engine_get_bool(char *pathstr, bool *value)
{
	int ret = 0;
	s8_t temp = 0;

	ret = lwm2m_engine_get_s8(pathstr, &temp);
	if (!ret) {
		*value = temp != 0;
	}

	return ret;
}

int lwm2m_engine_get_float32(char *pathstr, float32_value_t *buf)
{
	return lwm2m_engine_get(pathstr, buf, sizeof(float32_value_t));
}

int lwm2m_engine_get_float64(char *pathstr, float64_value_t *buf)
{
	return lwm2m_engine_get(pathstr, buf, sizeof(float64_value_t));
}

int lwm2m_engine_get_resource(char *pathstr, struct lwm2m_engine_res **res)
{
	int ret;
	struct lwm2m_obj_path path;

	ret = string_to_path(pathstr, &path, '/');
	if (ret < 0) {
		return ret;
	}

	if (path.level < 3) {
		LOG_ERR("path must have 3 parts");
		return -EINVAL;
	}

	return path_to_objs(&path, NULL, NULL, res, NULL);
}

void lwm2m_engine_get_binding(char *binding)
{
	if (IS_ENABLED(CONFIG_LWM2M_QUEUE_MODE_ENABLED)) {
		strcpy(binding, "UQ");
	} else {
		/* Defaults to UDP. */
		strcpy(binding, "U");
	}
}

int lwm2m_engine_create_res_inst(char *pathstr)
{
	int ret, i;
	struct lwm2m_engine_res *res = NULL;
	struct lwm2m_engine_res_inst *res_inst = NULL;
	struct lwm2m_obj_path path;

	ret = string_to_path(pathstr, &path, '/');
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

	if (!res->res_instances || res->res_inst_count == 0) {
		LOG_ERR("no available res instances");
		return -ENOMEM;
	}

	for (i = 0; i < res->res_inst_count; i++) {
		if (res->res_instances[i].res_inst_id ==
		    RES_INSTANCE_NOT_CREATED) {
			break;
		}
	}

	if (i >= res->res_inst_count) {
		LOG_ERR("no available res instances");
		return -ENOMEM;
	}

	res->res_instances[i].res_inst_id = path.res_inst_id;
	return 0;
}

int lwm2m_engine_delete_res_inst(char *pathstr)
{
	int ret;
	struct lwm2m_engine_res_inst *res_inst = NULL;
	struct lwm2m_obj_path path;

	ret = string_to_path(pathstr, &path, '/');
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
	res_inst->data_len = 0U;
	res_inst->res_inst_id = RES_INSTANCE_NOT_CREATED;

	return 0;
}

int lwm2m_engine_register_read_callback(char *pathstr,
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

int lwm2m_engine_register_pre_write_callback(char *pathstr,
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

int lwm2m_engine_register_post_write_callback(char *pathstr,
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

int lwm2m_engine_register_exec_callback(char *pathstr,
					lwm2m_engine_user_cb_t cb)
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

int lwm2m_engine_register_create_callback(u16_t obj_id,
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

int lwm2m_engine_register_delete_callback(u16_t obj_id,
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

static int lwm2m_read_handler(struct lwm2m_engine_obj_inst *obj_inst,
			      struct lwm2m_engine_res *res,
			      struct lwm2m_engine_obj_field *obj_field,
			      struct lwm2m_message *msg)
{
	int i, loop_max = 1, found_values = 0;
	u16_t res_inst_id_tmp = 0U;
	void *data_ptr = NULL;
	size_t data_len = 0;

	if (!obj_inst || !res || !obj_field || !msg) {
		return -EINVAL;
	}

	loop_max = res->res_inst_count;
	if (loop_max > 1) {
		/* search for valid resource instances */
		for (i = 0; i < loop_max; i++) {
			if (res->res_instances[i].res_inst_id !=
			    RES_INSTANCE_NOT_CREATED) {
				found_values = 1;
				break;
			}
		}

		if (!found_values) {
			return -ENOENT;
		}

		engine_put_begin_ri(&msg->out, &msg->path);
		res_inst_id_tmp = msg->path.res_inst_id;
	}

	for (i = 0; i < loop_max; i++) {
		if (res->res_instances[i].res_inst_id ==
		    RES_INSTANCE_NOT_CREATED) {
			continue;
		}

		if (res->res_inst_count > 1) {
			msg->path.res_inst_id =
				res->res_instances[i].res_inst_id;
		}

		/* setup initial data elements */
		data_ptr = res->res_instances[i].data_ptr;
		data_len = res->res_instances[i].data_len;

		/* allow user to override data elements via callback */
		if (res->read_cb) {
			data_ptr = res->read_cb(obj_inst->obj_inst_id,
					res->res_id,
					res->res_instances[i].res_inst_id,
					&data_len);
		}

		if (!data_ptr || data_len == 0) {
			return -ENOENT;
		}

		switch (obj_field->data_type) {

		case LWM2M_RES_TYPE_OPAQUE:
			engine_put_opaque(&msg->out, &msg->path,
					  (u8_t *)data_ptr,
					  data_len);
			break;

		case LWM2M_RES_TYPE_STRING:
			engine_put_string(&msg->out, &msg->path,
					  (u8_t *)data_ptr,
					  strlen((u8_t *)data_ptr));
			break;

		case LWM2M_RES_TYPE_U64:
			engine_put_s64(&msg->out, &msg->path,
				       (s64_t)*(u64_t *)data_ptr);
			break;

		case LWM2M_RES_TYPE_U32:
		case LWM2M_RES_TYPE_TIME:
			engine_put_s32(&msg->out, &msg->path,
				       (s32_t)*(u32_t *)data_ptr);
			break;

		case LWM2M_RES_TYPE_U16:
			engine_put_s16(&msg->out, &msg->path,
				       (s16_t)*(u16_t *)data_ptr);
			break;

		case LWM2M_RES_TYPE_U8:
			engine_put_s8(&msg->out, &msg->path,
				      (s8_t)*(u8_t *)data_ptr);
			break;

		case LWM2M_RES_TYPE_S64:
			engine_put_s64(&msg->out, &msg->path,
				       *(s64_t *)data_ptr);
			break;

		case LWM2M_RES_TYPE_S32:
			engine_put_s32(&msg->out, &msg->path,
				       *(s32_t *)data_ptr);
			break;

		case LWM2M_RES_TYPE_S16:
			engine_put_s16(&msg->out, &msg->path,
				       *(s16_t *)data_ptr);
			break;

		case LWM2M_RES_TYPE_S8:
			engine_put_s8(&msg->out, &msg->path,
				      *(s8_t *)data_ptr);
			break;

		case LWM2M_RES_TYPE_BOOL:
			engine_put_bool(&msg->out, &msg->path,
					*(bool *)data_ptr);
			break;

		case LWM2M_RES_TYPE_FLOAT32:
			engine_put_float32fix(&msg->out, &msg->path,
				(float32_value_t *)data_ptr);
			break;

		case LWM2M_RES_TYPE_FLOAT64:
			engine_put_float64fix(&msg->out, &msg->path,
				(float64_value_t *)data_ptr);
			break;

		default:
			LOG_ERR("unknown obj data_type %d",
				obj_field->data_type);
			return -EINVAL;

		}
	}

	if (res->res_inst_count > 1) {
		engine_put_end_ri(&msg->out, &msg->path);
		msg->path.res_inst_id = res_inst_id_tmp;
	}

	return 0;
}

size_t lwm2m_engine_get_opaque_more(struct lwm2m_input_context *in,
				    u8_t *buf, size_t buflen, bool *last_block)
{
	u16_t in_len = in->opaque_len;

	if (in_len > buflen) {
		in_len = buflen;
	}

	in->opaque_len -= in_len;
	if (in->opaque_len == 0U) {
		*last_block = true;
	}

	if (buf_read(buf, in_len, CPKT_BUF_READ(in->in_cpkt),
		     &in->offset) < 0) {
		*last_block = true;
		return 0;
	}

	return (size_t)in_len;
}

static int lwm2m_write_handler_opaque(struct lwm2m_engine_obj_inst *obj_inst,
				      struct lwm2m_engine_res *res,
				      struct lwm2m_engine_res_inst *res_inst,
				      struct lwm2m_input_context *in,
				      void *data_ptr, size_t data_len,
				      bool last_block, size_t total_size)
{
	size_t len = 1;
	bool last_pkt_block = false, first_read = true;
	int ret = 0;

	while (!last_pkt_block && len > 0) {
		if (first_read) {
			len = engine_get_opaque(in, (u8_t *)data_ptr,
						data_len, &last_pkt_block);
			if (len == 0) {
				/* ignore empty content and continue */
				return 0;
			}

			first_read = false;
		} else {
			len = lwm2m_engine_get_opaque_more(in, (u8_t *)data_ptr,
							   data_len,
							   &last_pkt_block);
		}

		if (len == 0) {
			return -EINVAL;
		}

		if (res->post_write_cb) {
			ret = res->post_write_cb(obj_inst->obj_inst_id,
						 res->res_id,
						 res_inst->res_inst_id,
						 data_ptr, len,
						 last_pkt_block && last_block,
						 total_size);
			if (ret < 0) {
				return ret;
			}
		}
	}

	return ret;
}

/* This function is exposed for the content format writers */
int lwm2m_write_handler(struct lwm2m_engine_obj_inst *obj_inst,
			struct lwm2m_engine_res *res,
			struct lwm2m_engine_res_inst *res_inst,
			struct lwm2m_engine_obj_field *obj_field,
			struct lwm2m_message *msg)
{
	struct block_context *block_ctx = NULL;
	void *data_ptr = NULL;
	size_t data_len = 0;
	size_t len = 0;
	size_t total_size = 0;
	s64_t temp64 = 0;
	s32_t temp32 = 0;
	int ret = 0;
	u8_t token[8];
	u8_t tkl = 0U;
	bool last_block = true;

	if (!obj_inst || !res || !res_inst || !obj_field || !msg) {
		return -EINVAL;
	}

	if (LWM2M_HAS_RES_FLAG(res_inst, LWM2M_RES_DATA_FLAG_RO)) {
		return -EACCES;
	}

	/* setup initial data elements */
	data_ptr = res_inst->data_ptr;
	data_len = res_inst->data_len;

	/* allow user to override data elements via callback */
	if (res->pre_write_cb) {
		data_ptr = res->pre_write_cb(obj_inst->obj_inst_id,
					     res->res_id, res_inst->res_inst_id,
					     &data_len);
	}

	if (res->post_write_cb) {
		/* Get block1 option for checking MORE block flag */
		ret = get_option_int(msg->in.in_cpkt, COAP_OPTION_BLOCK1);
		if (ret >= 0) {
			last_block = !GET_MORE(ret);

			/* Get block_ctx for total_size (might be zero) */
			tkl = coap_header_get_token(msg->in.in_cpkt, token);
			if (tkl && !get_block_ctx(token, tkl, &block_ctx)) {
				total_size = block_ctx->ctx.total_size;
				LOG_DBG("BLOCK1: total:%zu current:%zu"
					" last:%u",
					block_ctx->ctx.total_size,
					block_ctx->ctx.current,
					last_block);
			}
		}
	}

	if (data_ptr && data_len > 0) {
		switch (obj_field->data_type) {

		case LWM2M_RES_TYPE_OPAQUE:
			ret = lwm2m_write_handler_opaque(obj_inst, res,
							 res_inst, &msg->in,
							 data_ptr, data_len,
							 last_block,
							 total_size);
			if (ret < 0) {
				return ret;
			}

			break;

		case LWM2M_RES_TYPE_STRING:
			engine_get_string(&msg->in, (u8_t *)data_ptr, data_len);
			len = strlen((char *)data_ptr);
			break;

		case LWM2M_RES_TYPE_U64:
			engine_get_s64(&msg->in, &temp64);
			*(u64_t *)data_ptr = temp64;
			len = 8;
			break;

		case LWM2M_RES_TYPE_U32:
		case LWM2M_RES_TYPE_TIME:
			engine_get_s32(&msg->in, &temp32);
			*(u32_t *)data_ptr = temp32;
			len = 4;
			break;

		case LWM2M_RES_TYPE_U16:
			engine_get_s32(&msg->in, &temp32);
			*(u16_t *)data_ptr = temp32;
			len = 2;
			break;

		case LWM2M_RES_TYPE_U8:
			engine_get_s32(&msg->in, &temp32);
			*(u8_t *)data_ptr = temp32;
			len = 1;
			break;

		case LWM2M_RES_TYPE_S64:
			engine_get_s64(&msg->in, (s64_t *)data_ptr);
			len = 8;
			break;

		case LWM2M_RES_TYPE_S32:
			engine_get_s32(&msg->in, (s32_t *)data_ptr);
			len = 4;
			break;

		case LWM2M_RES_TYPE_S16:
			engine_get_s32(&msg->in, &temp32);
			*(s16_t *)data_ptr = temp32;
			len = 2;
			break;

		case LWM2M_RES_TYPE_S8:
			engine_get_s32(&msg->in, &temp32);
			*(s8_t *)data_ptr = temp32;
			len = 1;
			break;

		case LWM2M_RES_TYPE_BOOL:
			engine_get_bool(&msg->in, (bool *)data_ptr);
			len = 1;
			break;

		case LWM2M_RES_TYPE_FLOAT32:
			engine_get_float32fix(&msg->in,
					      (float32_value_t *)data_ptr);
			len = sizeof(float32_value_t);
			break;

		case LWM2M_RES_TYPE_FLOAT64:
			engine_get_float64fix(&msg->in,
					      (float64_value_t *)data_ptr);
			len = sizeof(float64_value_t);
			break;

		default:
			LOG_ERR("unknown obj data_type %d",
				obj_field->data_type);
			return -EINVAL;

		}
	} else {
		return -ENOENT;
	}

	if (res->post_write_cb &&
	    obj_field->data_type != LWM2M_RES_TYPE_OPAQUE) {
		ret = res->post_write_cb(obj_inst->obj_inst_id,
					 res->res_id, res_inst->res_inst_id,
					 data_ptr, len,
					 last_block, total_size);
	}

	NOTIFY_OBSERVER_PATH(&msg->path);

	return ret;
}

static int lwm2m_write_attr_handler(struct lwm2m_engine_obj *obj,
				    struct lwm2m_message *msg)
{
	bool update_observe_node = false;
	char opt_buf[COAP_OPTION_BUF_LEN];
	int nr_opt, i, ret = 0;
	struct coap_option options[NR_LWM2M_ATTR];
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	struct lwm2m_engine_res *res = NULL;
	struct lwm2m_attr *attr;
	struct notification_attrs nattrs = { 0 };
	struct observe_node *obs;
	u8_t type = 0U;
	void *nattr_ptrs[NR_LWM2M_ATTR] = {
		&nattrs.pmin, &nattrs.pmax, &nattrs.gt, &nattrs.lt, &nattrs.st
	};
	void *ref;

	if (!obj || !msg) {
		return -EINVAL;
	}

	/* do not expose security obj */
	if (obj->obj_id == LWM2M_OBJECT_SECURITY_ID) {
		return -ENOENT;
	}

	nr_opt = coap_find_options(msg->in.in_cpkt, COAP_OPTION_URI_QUERY,
				   options, NR_LWM2M_ATTR);
	if (nr_opt <= 0) {
		LOG_ERR("No attribute found!");
		/* translate as bad request */
		return -EEXIST;
	}

	/* get lwm2m_attr slist */
	if (msg->path.level == 3U) {
		ret = path_to_objs(&msg->path, NULL, NULL, &res, NULL);
		if (ret < 0) {
			return ret;
		}

		ref = res;
	} else if (msg->path.level == 1U) {
		ref = obj;
	} else if (msg->path.level == 2U) {
		obj_inst = get_engine_obj_inst(msg->path.obj_id,
					       msg->path.obj_inst_id);
		if (!obj_inst) {
			return -ENOENT;
		}

		ref = obj_inst;
	} else {
		/* bad request */
		return -EEXIST;
	}

	/* retrieve existing attributes */
	ret = update_attrs(ref, &nattrs);
	if (ret < 0) {
		return ret;
	}

	/* loop through options to parse attribute */
	for (i = 0; i < nr_opt; i++) {
		int limit = MIN(options[i].len, 5), plen = 0, vlen;
		float32_value_t val = { 0 };
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
			    !memcmp(options[i].value, LWM2M_ATTR_STR[type],
				    LWM2M_ATTR_LEN[type])) {
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
				     type <= LWM2M_ATTR_PMAX ? sizeof(s32_t) :
				     sizeof(float32_value_t));
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
			long int v;

			/* pmin/pmax: integer (sec 5.1.2)
			 * however, negative is non-sense
			 */
			errno = 0;
			v = strtol(opt_buf, &end, 10);
			if (errno || *end || v < 0) {
				ret = -EINVAL;
			}

			val.val1 = v;
		} else {
			/* gt/lt/st: type float */
			ret = atof32(opt_buf, &val);
		}

		if (ret < 0) {
			LOG_ERR("invalid attr[%s] value",
				log_strdup(LWM2M_ATTR_STR[type]));
			/* bad request */
			return -EEXIST;
		}

		if (type <= LWM2M_ATTR_PMAX) {
			*(s32_t *)nattr_ptrs[type] = val.val1;
		} else {
			memcpy(nattr_ptrs[type], &val, sizeof(float32_value_t));
		}

		nattrs.flags |= BIT(type);
	}

	if ((nattrs.flags & (BIT(LWM2M_ATTR_PMIN) | BIT(LWM2M_ATTR_PMAX))) &&
	    nattrs.pmin > nattrs.pmax) {
		LOG_DBG("pmin (%d) > pmax (%d)", nattrs.pmin, nattrs.pmax);
		return -EEXIST;
	}

	if (nattrs.flags & (BIT(LWM2M_ATTR_LT) | BIT(LWM2M_ATTR_GT))) {
		if (!((nattrs.lt.val1 < nattrs.gt.val1) ||
		      (nattrs.lt.val2 < nattrs.gt.val2))) {
			LOG_DBG("lt > gt");
			return -EEXIST;
		}

		if (nattrs.flags & BIT(LWM2M_ATTR_STEP)) {
			s32_t st1 = nattrs.st.val1 * 2 +
				    nattrs.st.val2 * 2 / 1000000;
			s32_t st2 = nattrs.st.val2 * 2 % 1000000;
			if (!(((nattrs.lt.val1 + st1) < nattrs.gt.val1) ||
			      ((nattrs.lt.val2 + st2) < nattrs.gt.val2))) {
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
			LOG_DBG("Unset attr %s",
				log_strdup(LWM2M_ATTR_STR[type]));
			(void)memset(attr, 0, sizeof(*attr));

			if (type <= LWM2M_ATTR_PMAX) {
				update_observe_node = true;
			}

			continue;
		}

		nattrs.flags &= ~BIT(type);

		if (type <= LWM2M_ATTR_PMAX) {
			if (attr->int_val == *(s32_t *)nattr_ptrs[type]) {
				continue;
			}

			attr->int_val = *(s32_t *)nattr_ptrs[type];
			update_observe_node = true;
		} else {
			if (!memcmp(&attr->float_val, nattr_ptrs[type],
				    sizeof(float32_value_t))) {
				continue;
			}

			memcpy(&attr->float_val, nattr_ptrs[type],
			       sizeof(float32_value_t));
		}

		LOG_DBG("Update %s to %d.%06d",
			log_strdup(LWM2M_ATTR_STR[type]),
			attr->float_val.val1, attr->float_val.val2);
	}

	/* add attribute to obj/obj_inst/res */
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
			attr->int_val = *(s32_t *)nattr_ptrs[type];
			update_observe_node = true;
		} else {
			memcpy(&attr->float_val, nattr_ptrs[type],
			       sizeof(float32_value_t));
		}

		nattrs.flags &= ~BIT(type);
		LOG_DBG("Add %s to %d.%06d", log_strdup(LWM2M_ATTR_STR[type]),
			attr->float_val.val1, attr->float_val.val2);
	}

	/* check only pmin/pmax */
	if (!update_observe_node) {
		return 0;
	}

	/* update observe_node accordingly */
	SYS_SLIST_FOR_EACH_CONTAINER(&engine_observer_list, obs, node) {
		/* updated path is deeper than obs node, skip */
		if (msg->path.level > obs->path.level) {
			continue;
		}

		/* check obj id matched or not */
		if (msg->path.obj_id != obs->path.obj_id) {
			continue;
		}

		/* defaults from server object */
		nattrs.pmin = lwm2m_server_get_pmin(msg->ctx->sec_obj_inst);
		nattrs.pmax = lwm2m_server_get_pmax(msg->ctx->sec_obj_inst);

		ret = update_attrs(obj, &nattrs);
		if (ret < 0) {
			return ret;
		}

		if (obs->path.level > 1) {
			if (msg->path.level > 1 &&
			    msg->path.obj_inst_id != obs->path.obj_inst_id) {
				continue;
			}

			/* get obj_inst */
			if (!obj_inst || obj_inst->obj_inst_id !=
					 obs->path.obj_inst_id) {
				obj_inst = get_engine_obj_inst(
						obs->path.obj_id,
						obs->path.obj_inst_id);
				if (!obj_inst) {
					return -ENOENT;
				}
			}

			ret = update_attrs(obj_inst, &nattrs);
			if (ret < 0) {
				return ret;
			}
		}

		if (obs->path.level > 2) {
			if (msg->path.level > 2 &&
			    msg->path.res_id != obs->path.res_id) {
				continue;
			}

			if (!res || res->res_id != obs->path.res_id) {
				ret = path_to_objs(&obs->path, NULL, NULL,
						   &res, NULL);
				if (ret < 0) {
					return ret;
				}
			}

			ret = update_attrs(res, &nattrs);
			if (ret < 0) {
				return ret;
			}
		}

		LOG_DBG("%d/%d/%d(%d) updated from %d/%d to %u/%u",
			obs->path.obj_id, obs->path.obj_inst_id,
			obs->path.res_id, obs->path.level,
			obs->min_period_sec, obs->max_period_sec,
			nattrs.pmin, MAX(nattrs.pmin, nattrs.pmax));
		obs->min_period_sec = (u32_t)nattrs.pmin;
		obs->max_period_sec = (u32_t)MAX(nattrs.pmin, nattrs.pmax);
		(void)memset(&nattrs, 0, sizeof(nattrs));
	}

	return 0;
}

static int lwm2m_exec_handler(struct lwm2m_message *msg)
{
	struct lwm2m_engine_obj_inst *obj_inst;
	struct lwm2m_engine_res *res = NULL;
	int ret;

	if (!msg) {
		return -EINVAL;
	}

	ret = path_to_objs(&msg->path, &obj_inst, NULL, &res, NULL);
	if (ret < 0) {
		return ret;
	}

	if (res->execute_cb) {
		return res->execute_cb(obj_inst->obj_inst_id);
	}

	/* TODO: something else to handle for execute? */
	return -ENOENT;
}

static int lwm2m_delete_handler(struct lwm2m_message *msg)
{
	int ret;

	if (!msg) {
		return -EINVAL;
	}

	ret = lwm2m_delete_obj_inst(msg->path.obj_id, msg->path.obj_inst_id);
#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT)
	if (!ret && !msg->ctx->bootstrap_mode) {
		engine_trigger_update();
	}
#endif

	return ret;
}

static int do_read_op(struct lwm2m_message *msg, u16_t content_format)
{
	switch (content_format) {

	case LWM2M_FORMAT_APP_OCTET_STREAM:
	case LWM2M_FORMAT_PLAIN_TEXT:
	case LWM2M_FORMAT_OMA_PLAIN_TEXT:
		return do_read_op_plain_text(msg, content_format);

	case LWM2M_FORMAT_OMA_TLV:
	case LWM2M_FORMAT_OMA_OLD_TLV:
		return do_read_op_tlv(msg, content_format);

#if defined(CONFIG_LWM2M_RW_JSON_SUPPORT)
	case LWM2M_FORMAT_OMA_JSON:
	case LWM2M_FORMAT_OMA_OLD_JSON:
		return do_read_op_json(msg, content_format);
#endif

	default:
		LOG_ERR("Unsupported content-format: %u", content_format);
		return -ENOMSG;

	}
}

int lwm2m_perform_read_op(struct lwm2m_message *msg, u16_t content_format)
{
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	struct lwm2m_engine_res *res = NULL;
	struct lwm2m_engine_obj_field *obj_field;
	struct lwm2m_obj_path temp_path;
	int ret = 0, index;
	u8_t num_read = 0U;

	if (msg->path.level >= 2U) {
		obj_inst = get_engine_obj_inst(msg->path.obj_id,
					       msg->path.obj_inst_id);
	} else if (msg->path.level == 1U) {
		/* find first obj_inst with path's obj_id */
		obj_inst = next_engine_obj_inst(msg->path.obj_id, -1);
	}

	if (!obj_inst) {
		return -ENOENT;
	}

	/* set output content-format */
	ret = coap_append_option_int(msg->out.out_cpkt,
				     COAP_OPTION_CONTENT_FORMAT,
				     content_format);
	if (ret < 0) {
		LOG_ERR("Error setting response content-format: %d", ret);
		return ret;
	}

	ret = coap_packet_append_payload_marker(msg->out.out_cpkt);
	if (ret < 0) {
		LOG_ERR("Error appending payload marker: %d", ret);
		return ret;
	}

	/* store original path values so we can change them during processing */
	memcpy(&temp_path, &msg->path, sizeof(temp_path));
	engine_put_begin(&msg->out, &msg->path);

	while (obj_inst) {
		if (!obj_inst->resources || obj_inst->resource_count == 0U) {
			goto move_forward;
		}

		/* update the obj_inst_id as we move through the instances */
		msg->path.obj_inst_id = obj_inst->obj_inst_id;

		if (msg->path.level <= 1U) {
			/* start instance formatting */
			engine_put_begin_oi(&msg->out, &msg->path);
		}

		for (index = 0; index < obj_inst->resource_count; index++) {
			if (msg->path.level > 2 &&
			    msg->path.res_id !=
					obj_inst->resources[index].res_id) {
				continue;
			}

			res = &obj_inst->resources[index];

			/*
			 * On an entire object instance, we need to set path's
			 * res_id for lwm2m_read_handler to read this specific
			 * resource.
			 */
			msg->path.res_id = res->res_id;
			obj_field = lwm2m_get_engine_obj_field(obj_inst->obj,
							       res->res_id);
			if (!obj_field) {
				ret = -ENOENT;
			} else if (!LWM2M_HAS_PERM(obj_field, LWM2M_PERM_R)) {
				ret = -EPERM;
			} else {
				/* start resource formatting */
				engine_put_begin_r(&msg->out, &msg->path);

				/* perform read operation on this resource */
				ret = lwm2m_read_handler(obj_inst, res,
							 obj_field, msg);
				if (ret < 0) {
					/* ignore errors unless single read */
					if (msg->path.level > 2 &&
					    !LWM2M_HAS_PERM(obj_field,
						BIT(LWM2M_FLAG_OPTIONAL))) {
						LOG_ERR("READ OP: %d", ret);
					}
				} else {
					num_read += 1U;
				}

				/* end resource formatting */
				engine_put_end_r(&msg->out, &msg->path);
			}

			/* on single read break if errors */
			if (ret < 0 && msg->path.level > 2) {
				break;
			}

			/* when reading multiple resources ignore return code */
			ret = 0;
		}

move_forward:
		if (msg->path.level <= 1U) {
			/* end instance formatting */
			engine_put_end_oi(&msg->out, &msg->path);
		}

		if (msg->path.level <= 1U) {
			/* advance to the next object instance */
			obj_inst = next_engine_obj_inst(msg->path.obj_id,
							obj_inst->obj_inst_id);
		} else {
			obj_inst = NULL;
		}
	}

	engine_put_end(&msg->out, &msg->path);

	/* restore original path values */
	memcpy(&msg->path, &temp_path, sizeof(temp_path));

	/* did not read anything even if we should have - on single item */
	if (ret == 0 && num_read == 0U && msg->path.level == 3U) {
		return -ENOENT;
	}

	return ret;
}

static int print_attr(struct lwm2m_output_context *out,
		      u8_t *buf, u16_t buflen, void *ref)
{
	struct lwm2m_attr *attr;
	int i, used, base, ret;
	u8_t digit;
	s32_t fraction;

	for (i = 0; i < CONFIG_LWM2M_NUM_ATTR; i++) {
		if (ref != write_attr_pool[i].ref) {
			continue;
		}

		attr = write_attr_pool + i;

		/* assuming integer will have float_val.val2 set as 0 */

		used = snprintk(buf, buflen, ";%s=%s%d%s",
				LWM2M_ATTR_STR[attr->type],
				attr->float_val.val1 == 0 &&
				attr->float_val.val2 < 0 ? "-" : "",
				attr->float_val.val1,
				attr->float_val.val2 != 0 ? "." : "");

		base = 100000;
		fraction = attr->float_val.val2 < 0 ?
			   -attr->float_val.val2 : attr->float_val.val2;
		while (fraction && used < buflen && base > 0) {
			digit = fraction / base;
			buf[used++] = '0' + digit;
			fraction -= digit * base;
			base /= 10;
		}

		ret = buf_append(CPKT_BUF_WRITE(out->out_cpkt), buf, used);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int do_discover_op(struct lwm2m_message *msg, bool well_known)
{
	static char disc_buf[24];
	struct lwm2m_engine_obj *obj;
	struct lwm2m_engine_obj_inst *obj_inst;
	int ret;
	bool reported = false;

	/* object ID is required unless it's bootstrap discover or it's
	 * a ".well-known/core" discovery
	 * ref: lwm2m spec 20170208-A table 11
	 */
	if (!msg->ctx->bootstrap_mode && !well_known &&
	    (msg->path.level == 0U ||
	     (msg->path.level > 0 &&
	      msg->path.obj_id == LWM2M_OBJECT_SECURITY_ID))) {
		return -EPERM;
	}

	/* set output content-format */
	ret = coap_append_option_int(msg->out.out_cpkt,
				     COAP_OPTION_CONTENT_FORMAT,
				     LWM2M_FORMAT_APP_LINK_FORMAT);
	if (ret < 0) {
		LOG_ERR("Error setting response content-format: %d", ret);
		return ret;
	}

	ret = coap_packet_append_payload_marker(msg->out.out_cpkt);
	if (ret < 0) {
		return ret;
	}

	/* Handle CoAP .well-known/core discover */
	if (well_known) {
		/* </.well-known/core> */
		ret = buf_append(CPKT_BUF_WRITE(msg->out.out_cpkt),
				 WELL_KNOWN_CORE_PATH,
				 strlen(WELL_KNOWN_CORE_PATH));
		if (ret < 0) {
			return ret;
		}

		SYS_SLIST_FOR_EACH_CONTAINER(&engine_obj_list, obj, node) {
			snprintk(disc_buf, sizeof(disc_buf), ",</%u>",
				 obj->obj_id);

			ret = buf_append(CPKT_BUF_WRITE(msg->out.out_cpkt),
					 disc_buf, strlen(disc_buf));
			if (ret < 0) {
				return ret;
			}
		}

		return 0;
	}

	/*
	 * lwm2m spec 20170208-A sec 5.2.7.3 bootstrap discover on "/"
	 * - (TODO) prefixed w/ lwm2m enabler version. e.g. lwm2m="1.0"
	 * - returns object and object instances only
	 */

	SYS_SLIST_FOR_EACH_CONTAINER(&engine_obj_inst_list, obj_inst, node) {
		/*
		 * - Avoid discovery for security object (5.2.7.3) unless
		 *   Bootstrap discover
		 * - Skip reporting unrelated object
		 */
		if ((!msg->ctx->bootstrap_mode &&
		     obj_inst->obj->obj_id == LWM2M_OBJECT_SECURITY_ID) ||
		    obj_inst->obj->obj_id != msg->path.obj_id) {
			continue;
		}

		if (msg->path.level == 1U) {
			snprintk(disc_buf, sizeof(disc_buf), "%s</%u>",
				 reported ? "," : "",
				 obj_inst->obj->obj_id);

			ret = buf_append(CPKT_BUF_WRITE(msg->out.out_cpkt),
					 disc_buf, strlen(disc_buf));
			if (ret < 0) {
				return ret;
			}

			/* report object attrs (5.4.2) */
			ret = print_attr(&msg->out, disc_buf, sizeof(disc_buf),
					 obj_inst->obj);
			if (ret < 0) {
				return ret;
			}

			reported = true;
		}

		/* skip unrelated object instance */
		if (msg->path.level > 1 &&
		    msg->path.obj_inst_id != obj_inst->obj_inst_id) {
			continue;
		}

		if (msg->path.level == 2U) {
			snprintk(disc_buf, sizeof(disc_buf), "%s</%u/%u>",
				 reported ? "," : "",
				 obj_inst->obj->obj_id, obj_inst->obj_inst_id);

			ret = buf_append(CPKT_BUF_WRITE(msg->out.out_cpkt),
					 disc_buf, strlen(disc_buf));
			if (ret < 0) {
				return ret;
			}

			/* report object instance attrs (5.4.2) */
			ret = print_attr(&msg->out, disc_buf, sizeof(disc_buf),
					 obj_inst);
			if (ret < 0) {
				return ret;
			}

			reported = true;
		}

		/* don't return resource info for bootstrap discovery */
		if (msg->ctx->bootstrap_mode) {
			continue;
		}

		for (int i = 0; i < obj_inst->resource_count; i++) {
			/* skip unrelated resources */
			if (msg->path.level == 3U &&
			    msg->path.res_id != obj_inst->resources[i].res_id) {
				continue;
			}

			snprintk(disc_buf, sizeof(disc_buf),
				 "%s</%u/%u/%u>",
				 reported ? "," : "",
				 obj_inst->obj->obj_id,
				 obj_inst->obj_inst_id,
				 obj_inst->resources[i].res_id);

			ret = buf_append(CPKT_BUF_WRITE(msg->out.out_cpkt),
					 disc_buf, strlen(disc_buf));
			if (ret < 0) {
				return ret;
			}

			/* report resource attrs when path > 1 (5.4.2) */
			if (msg->path.level > 1) {
				ret = print_attr(&msg->out,
						 disc_buf, sizeof(disc_buf),
						 &obj_inst->resources[i]);
				if (ret < 0) {
					return ret;
				}
			}

			reported = true;
		}
	}

	return reported ? 0 : -ENOENT;
}

int lwm2m_get_or_create_engine_obj(struct lwm2m_message *msg,
				   struct lwm2m_engine_obj_inst **obj_inst,
				   u8_t *created)
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
			engine_trigger_update();
		}
#endif
	}

	return ret;
}

static int do_write_op(struct lwm2m_message *msg,
		       u16_t format)
{
	switch (format) {

	case LWM2M_FORMAT_APP_OCTET_STREAM:
	case LWM2M_FORMAT_PLAIN_TEXT:
	case LWM2M_FORMAT_OMA_PLAIN_TEXT:
		return do_write_op_plain_text(msg);

	case LWM2M_FORMAT_OMA_TLV:
	case LWM2M_FORMAT_OMA_OLD_TLV:
		return do_write_op_tlv(msg);

#ifdef CONFIG_LWM2M_RW_JSON_SUPPORT
	case LWM2M_FORMAT_OMA_JSON:
	case LWM2M_FORMAT_OMA_OLD_JSON:
		return do_write_op_json(msg);
#endif

	default:
		LOG_ERR("Unsupported format: %u", format);
		return -ENOMSG;

	}
}

#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)
static int bootstrap_delete(void)
{
	struct lwm2m_engine_obj_inst *obj_inst, *tmp;
	int ret = 0;

	/* delete SECURITY instances > 0 */
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&engine_obj_inst_list,
					  obj_inst, tmp, node) {
		if (obj_inst->obj->obj_id == LWM2M_OBJECT_SECURITY_ID &&
		    obj_inst->obj_inst_id > 0) {
			ret = lwm2m_delete_obj_inst(obj_inst->obj->obj_id,
						    obj_inst->obj_inst_id);
			if (ret < 0) {
				return ret;
			}
		}
	}

	return ret;
}
#endif

static int handle_request(struct coap_packet *request,
			  struct lwm2m_message *msg)
{
	int r;
	u8_t code;
	struct coap_option options[4];
	struct lwm2m_engine_obj *obj = NULL;
	u8_t token[8];
	u8_t tkl = 0U;
	u16_t format = LWM2M_FORMAT_NONE, accept;
	int observe = -1; /* default to -1, 0 = ENABLE, 1 = DISABLE */
	bool well_known = false;
	struct block_context *block_ctx = NULL;
	enum coap_block_size block_size;
	u16_t payload_len = 0U;
	bool last_block = false;

	/* set CoAP request / message */
	msg->in.in_cpkt = request;
	msg->out.out_cpkt = &msg->cpkt;

	/* set default reader/writer */
	msg->in.reader = &plain_text_reader;
	msg->out.writer = &plain_text_writer;

	code = coap_header_get_code(msg->in.in_cpkt);

	/* setup response token */
	tkl = coap_header_get_token(msg->in.in_cpkt, token);
	if (tkl) {
		msg->tkl = tkl;
		msg->token = token;
	}

	/* parse the URL path into components */
	r = coap_find_options(msg->in.in_cpkt, COAP_OPTION_URI_PATH, options,
			      ARRAY_SIZE(options));
	if (r <= 0) {
		switch (code & COAP_REQUEST_MASK) {
#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)
		case COAP_METHOD_DELETE:
			if (msg->ctx->bootstrap_mode) {
				r = bootstrap_delete();
				if (r < 0) {
					goto error;
				}

				msg->code = COAP_RESPONSE_CODE_DELETED;
				r = lwm2m_init_message(msg);
			} else {
				r = -EPERM;
			}

			if (r < 0) {
				goto error;
			}

			return 0;
#endif
		default:
			r = -EPERM;
			goto error;
		}
	}

	/* check for .well-known/core URI query (DISCOVER) */
	if (r == 2 &&
	    (options[0].len == 11U &&
	     strncmp(options[0].value, ".well-known", 11) == 0) &&
	    (options[1].len == 4U &&
	     strncmp(options[1].value, "core", 4) == 0)) {
		if ((code & COAP_REQUEST_MASK) != COAP_METHOD_GET) {
			r = -EPERM;
			goto error;
		}

		well_known = true;
#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)
	/* check for bootstrap-finish */
	} else if ((code & COAP_REQUEST_MASK) == COAP_METHOD_POST && r == 1 && \
		   strncmp(options[0].value, "bs", options[0].len) == 0) {
		engine_bootstrap_finish();
		return 0;
#endif
	} else {
		r = coap_options_to_path(options, r, &msg->path);
		if (r < 0) {
			r = -ENOENT;
			goto error;
		}
	}

	/* read Content Format / setup in.reader */
	r = coap_find_options(msg->in.in_cpkt, COAP_OPTION_CONTENT_FORMAT,
			      options, 1);
	if (r > 0) {
		format = coap_option_value_to_int(&options[0]);
		r = select_reader(&msg->in, format);
		if (r < 0) {
			goto error;
		}
	}

	/* read Accept / setup out.writer */
	r = coap_find_options(msg->in.in_cpkt, COAP_OPTION_ACCEPT, options, 1);
	if (r > 0) {
		accept = coap_option_value_to_int(&options[0]);
	} else {
		LOG_DBG("No accept option given. Assume OMA TLV.");
		accept = LWM2M_FORMAT_OMA_TLV;
	}

	r = select_writer(&msg->out, accept);
	if (r < 0) {
		goto error;
	}

	if (!well_known) {
		/* find registered obj */
		obj = get_engine_obj(msg->path.obj_id);
		if (!obj) {
			/* No matching object found - ignore request */
			r = -ENOENT;
			goto error;
		}
	}

	/* set the operation */
	switch (code & COAP_REQUEST_MASK) {

	case COAP_METHOD_GET:
		/*
		 * LwM2M V1_0_1-20170704-A, table 25,
		 * Discover: CoAP GET + accept=LWM2M_FORMAT_APP_LINK_FORMAT
		 */
		if (well_known || accept == LWM2M_FORMAT_APP_LINK_FORMAT) {
			msg->operation = LWM2M_OP_DISCOVER;
			accept = LWM2M_FORMAT_APP_LINK_FORMAT;
		} else {
			msg->operation = LWM2M_OP_READ;
		}

		/* check for observe */
		observe = get_option_int(msg->in.in_cpkt, COAP_OPTION_OBSERVE);
		msg->code = COAP_RESPONSE_CODE_CONTENT;
		break;

	case COAP_METHOD_POST:
		if (msg->path.level == 1U) {
			/* create an object instance */
			msg->operation = LWM2M_OP_CREATE;
			msg->code = COAP_RESPONSE_CODE_CREATED;
		} else if (msg->path.level == 2U) {
			/* write values to an object instance */
			msg->operation = LWM2M_OP_WRITE;
			msg->code = COAP_RESPONSE_CODE_CHANGED;
		} else {
			msg->operation = LWM2M_OP_EXECUTE;
			msg->code = COAP_RESPONSE_CODE_CHANGED;
		}

		break;

	case COAP_METHOD_PUT:
		/* write attributes if content-format is absent */
		if (format == LWM2M_FORMAT_NONE) {
			msg->operation = LWM2M_OP_WRITE_ATTR;
		} else {
			msg->operation = LWM2M_OP_WRITE;
		}

		msg->code = COAP_RESPONSE_CODE_CHANGED;
		break;

	case COAP_METHOD_DELETE:
		msg->operation = LWM2M_OP_DELETE;
		msg->code = COAP_RESPONSE_CODE_DELETED;
		break;

	default:
		break;
	}

	/* setup incoming data */
	msg->in.offset = msg->in.in_cpkt->hdr_len + msg->in.in_cpkt->opt_len;
	coap_packet_get_payload(msg->in.in_cpkt, &payload_len);

	/* Check for block transfer */
	r = get_option_int(msg->in.in_cpkt, COAP_OPTION_BLOCK1);
	if (r > 0) {
		last_block = !GET_MORE(r);

		/* RFC7252: 4.6. Message Size */
		block_size = GET_BLOCK_SIZE(r);
		if (!last_block &&
		    coap_block_size_to_bytes(block_size) > payload_len) {
			LOG_DBG("Trailing payload is discarded!");
			r = -EFBIG;
			goto error;
		}

		if (GET_BLOCK_NUM(r) == 0) {
			r = init_block_ctx(token, tkl, &block_ctx);
		} else {
			r = get_block_ctx(token, tkl, &block_ctx);
		}

		if (r < 0) {
			goto error;
		}

		r = coap_update_from_block(msg->in.in_cpkt, &block_ctx->ctx);
		if (r < 0) {
			LOG_ERR("Error from block update: %d", r);
			goto error;
		}

		/* Handle blockwise 1 (Part 1): Set response code */
		if (!last_block) {
			msg->code = COAP_RESPONSE_CODE_CONTINUE;
		}
	}

	/* render CoAP packet header */
	r = lwm2m_init_message(msg);
	if (r < 0) {
		goto error;
	}

	switch (msg->operation) {

	case LWM2M_OP_READ:
		if (observe == 0) {
			/* add new observer */
			if (msg->token) {
				r = coap_append_option_int(msg->out.out_cpkt,
							   COAP_OPTION_OBSERVE,
							   1);
				if (r < 0) {
					LOG_ERR("OBSERVE option error: %d", r);
					goto error;
				}

				r = engine_add_observer(msg, token, tkl,
							accept);
				if (r < 0) {
					LOG_ERR("add OBSERVE error: %d", r);
					goto error;
				}
			} else {
				LOG_ERR("OBSERVE request missing token");
				r = -EINVAL;
				goto error;
			}
		} else if (observe == 1) {
			/* remove observer */
			r = engine_remove_observer(token, tkl);
			if (r < 0) {
				LOG_ERR("remove observe error: %d", r);
			}
		}

		r = do_read_op(msg, accept);
		break;

	case LWM2M_OP_DISCOVER:
		r = do_discover_op(msg, well_known);
		break;

	case LWM2M_OP_WRITE:
	case LWM2M_OP_CREATE:
		r = do_write_op(msg, format);
		break;

	case LWM2M_OP_WRITE_ATTR:
		r = lwm2m_write_attr_handler(obj, msg);
		break;

	case LWM2M_OP_EXECUTE:
		r = lwm2m_exec_handler(msg);
		break;

	case LWM2M_OP_DELETE:
		r = lwm2m_delete_handler(msg);
		break;

	default:
		LOG_ERR("Unknown operation: %u", msg->operation);
		r = -EINVAL;
	}

	if (r < 0) {
		goto error;
	}

	/* Handle blockwise 1 (Part 2): Append BLOCK1 option / free context */
	if (block_ctx) {
		if (!last_block) {
			/* More to come, ack with correspond block # */
			r = coap_append_block1_option(msg->out.out_cpkt,
						      &block_ctx->ctx);
			if (r < 0) {
				/* report as internal server error */
				LOG_ERR("Fail adding block1 option: %d", r);
				r = -EINVAL;
				goto error;
			}
		} else {
			/* Free context when finished */
			free_block_ctx(block_ctx);
		}
	}

	return 0;

error:
	lwm2m_reset_message(msg, false);
	if (r == -ENOENT) {
		msg->code = COAP_RESPONSE_CODE_NOT_FOUND;
	} else if (r == -EPERM) {
		msg->code = COAP_RESPONSE_CODE_NOT_ALLOWED;
	} else if (r == -EEXIST) {
		msg->code = COAP_RESPONSE_CODE_BAD_REQUEST;
	} else if (r == -EFAULT) {
		msg->code = COAP_RESPONSE_CODE_INCOMPLETE;
	} else if (r == -EFBIG) {
		msg->code = COAP_RESPONSE_CODE_REQUEST_TOO_LARGE;
	} else if (r == -ENOTSUP) {
		msg->code = COAP_RESPONSE_CODE_NOT_IMPLEMENTED;
	} else if (r == -ENOMSG) {
		msg->code = COAP_RESPONSE_CODE_UNSUPPORTED_CONTENT_FORMAT;
	} else {
		/* Failed to handle the request */
		msg->code = COAP_RESPONSE_CODE_INTERNAL_ERROR;
	}

	r = lwm2m_init_message(msg);
	if (r < 0) {
		LOG_ERR("Error recreating message: %d", r);
	}

	/* Free block context when error happened */
	free_block_ctx(block_ctx);

	return 0;
}

static void lwm2m_udp_receive(struct lwm2m_ctx *client_ctx,
			      u8_t *buf, u16_t buf_len,
			      struct sockaddr *from_addr,
			      udp_request_handler_cb_t udp_request_handler)
{
	struct lwm2m_message *msg = NULL;
	struct coap_pending *pending;
	struct coap_reply *reply;
	struct coap_packet response;
	int r;
	u8_t token[8];
	u8_t tkl;

	r = coap_packet_parse(&response, buf, buf_len, NULL, 0);
	if (r < 0) {
		LOG_ERR("Invalid data received (err:%d)", r);
		return;
	}

	tkl = coap_header_get_token(&response, token);
	pending = coap_pending_received(&response, client_ctx->pendings,
					CONFIG_LWM2M_ENGINE_MAX_PENDING);
	/*
	 * Clear pending pointer because coap_pending_received() calls
	 * coap_pending_clear, and later when we call lwm2m_reset_message()
	 * it will try and call coap_pending_clear() again if msg->pending
	 * is != NULL.
	 */
	if (pending) {
		msg = find_msg(pending, NULL);
	}

	LOG_DBG("checking for reply from [%s]",
		log_strdup(lwm2m_sprint_ip_addr(from_addr)));
	reply = coap_response_received(&response, from_addr,
				       client_ctx->replies,
				       CONFIG_LWM2M_ENGINE_MAX_REPLIES);
	if (reply) {
		/*
		 * Separate response is composed of 2 messages, empty ACK with
		 * no token and an additional message with a matching token id
		 * (based on the token used by the CON request).
		 *
		 * Since the ACK received by the notify CON messages are also
		 * empty with no token (consequence of always using the same
		 * token id for all notifications), we have to use an
		 * additional flag to decide when to clear the reply callback.
		 */
		if (client_ctx->handle_separate_response && !tkl &&
			coap_header_get_type(&response) == COAP_TYPE_ACK) {
			LOG_DBG("separated response, not removing reply");
			return;
		}

		if (!msg) {
			msg = find_msg(pending, reply);
		}
	}

	if (reply || pending) {
		/* skip release if reply->user_data has error condition */
		if (reply && reply->user_data != COAP_REPLY_STATUS_NONE) {
			/* reset reply->user_data for next time */
			reply->user_data = (void *)COAP_REPLY_STATUS_NONE;
			LOG_DBG("reply %p NOT removed", reply);
			return;
		}

		/* free up msg resources */
		if (msg) {
			lwm2m_reset_message(msg, true);
		}

		LOG_DBG("reply %p handled and removed", reply);
		return;
	}

	/*
	 * If no normal response handler is found, then this is
	 * a new request coming from the server.  Let's look
	 * at registered objects to find a handler.
	 */
	if (udp_request_handler &&
	    coap_header_get_type(&response) == COAP_TYPE_CON) {
		msg = lwm2m_get_message(client_ctx);
		if (!msg) {
			LOG_ERR("Unable to get a lwm2m message!");
			return;
		}

		/* Create a response message if we reach this point */
		msg->type = COAP_TYPE_ACK;
		msg->code = coap_header_get_code(&response);
		msg->mid = coap_header_get_id(&response);
		/* skip token generation by default */
		msg->tkl = LWM2M_MSG_TOKEN_LEN_SKIP;

		/* process the response to this request */
		r = udp_request_handler(&response, msg);
		if (r < 0) {
			return;
		}

		r = lwm2m_send_message(msg);
		if (r < 0) {
			LOG_ERR("Err sending response: %d", r);
			lwm2m_reset_message(msg, true);
		}
	} else {
		LOG_ERR("No handler for response");
	}
}

static void retransmit_request(struct k_work *work)
{
	struct lwm2m_ctx *client_ctx;
	struct lwm2m_message *msg;
	struct coap_pending *pending;
	s32_t remaining;

	client_ctx = CONTAINER_OF(work, struct lwm2m_ctx, retransmit_work);
	pending = coap_pending_next_to_expire(client_ctx->pendings,
					      CONFIG_LWM2M_ENGINE_MAX_PENDING);
	if (!pending) {
		return;
	}

	remaining = pending->t0 + pending->timeout - k_uptime_get_32();
	if (remaining > 0) {
		/* First message to expire was removed from the list,
		 * schedule next.
		 */
		goto next;
	}

	msg = find_msg(pending, NULL);
	if (!msg) {
		LOG_ERR("pending has no valid LwM2M message!");
		coap_pending_clear(pending);
		goto next;
	}

	if (!coap_pending_cycle(pending)) {
		/* pending request has expired */
		if (msg->message_timeout_cb) {
			msg->message_timeout_cb(msg);
		}

		/*
		 * coap_pending_clear() is called in lwm2m_reset_message()
		 * which balances the ref we made in coap_pending_cycle()
		 */
		lwm2m_reset_message(msg, true);
		goto next;
	}

	LOG_INF("Resending message: %p", msg);
	msg->send_attempts++;
	if (send(msg->ctx->sock_fd, msg->cpkt.data, msg->cpkt.offset, 0) < 0) {
		LOG_ERR("Error sending lwm2m message: %d", -errno);
		/* don't error here, retry until timeout */
	}

next:
	pending = coap_pending_next_to_expire(client_ctx->pendings,
					      CONFIG_LWM2M_ENGINE_MAX_PENDING);
	if (!pending) {
		return;
	}

	remaining = pending->t0 + pending->timeout - k_uptime_get_32();
	if (remaining < 0) {
		remaining = 0;
	}

	k_delayed_work_submit(&client_ctx->retransmit_work, K_MSEC(remaining));
}

static int notify_message_reply_cb(const struct coap_packet *response,
				   struct coap_reply *reply,
				   const struct sockaddr *from)
{
	int ret = 0;
	u8_t type, code;

	type = coap_header_get_type(response);
	code = coap_header_get_code(response);

	LOG_DBG("NOTIFY ACK type:%u code:%d.%d reply_token:'%s'",
		type,
		COAP_RESPONSE_CODE_CLASS(code),
		COAP_RESPONSE_CODE_DETAIL(code),
		log_strdup(sprint_token(reply->token, reply->tkl)));

	/* remove observer on COAP_TYPE_RESET */
	if (type == COAP_TYPE_RESET) {
		if (reply->tkl > 0) {
			ret = engine_remove_observer(reply->token, reply->tkl);
			if (ret) {
				LOG_ERR("remove observe error: %d", ret);
			}
		} else {
			LOG_ERR("notify reply missing token -- ignored.");
		}
	}

	return 0;
}

static int generate_notify_message(struct observe_node *obs,
				   bool manual_trigger)
{
	struct lwm2m_message *msg;
	struct lwm2m_engine_obj_inst *obj_inst;
	int ret = 0;

	if (!obs->ctx) {
		LOG_ERR("observer has no valid LwM2M ctx!");
		return -EINVAL;
	}

	msg = lwm2m_get_message(obs->ctx);
	if (!msg) {
		LOG_ERR("Unable to get a lwm2m message!");
		return -ENOMEM;
	}

	/* copy path */
	memcpy(&msg->path, &obs->path, sizeof(struct lwm2m_obj_path));
	msg->operation = LWM2M_OP_READ;

	LOG_DBG("[%s] NOTIFY MSG START: %u/%u/%u(%u) token:'%s' [%s] %lld",
		manual_trigger ? "MANUAL" : "AUTO",
		obs->path.obj_id,
		obs->path.obj_inst_id,
		obs->path.res_id,
		obs->path.level,
		log_strdup(sprint_token(obs->token, obs->tkl)),
		log_strdup(lwm2m_sprint_ip_addr(&obs->ctx->remote_addr)),
		k_uptime_get());

	obj_inst = get_engine_obj_inst(obs->path.obj_id,
				       obs->path.obj_inst_id);
	if (!obj_inst) {
		LOG_ERR("unable to get engine obj for %u/%u",
			obs->path.obj_id,
			obs->path.obj_inst_id);
		ret = -EINVAL;
		goto cleanup;
	}

	msg->type = COAP_TYPE_CON;
	msg->code = COAP_RESPONSE_CODE_CONTENT;
	msg->mid = 0U;
	msg->token = obs->token;
	msg->tkl = obs->tkl;
	msg->reply_cb = notify_message_reply_cb;
	msg->out.out_cpkt = &msg->cpkt;

	ret = lwm2m_init_message(msg);
	if (ret < 0) {
		LOG_ERR("Unable to init lwm2m message! (err: %d)", ret);
		goto cleanup;
	}

	/* each notification should increment the obs counter */
	obs->counter++;
	ret = coap_append_option_int(&msg->cpkt, COAP_OPTION_OBSERVE,
				     obs->counter);
	if (ret < 0) {
		LOG_ERR("OBSERVE option error: %d", ret);
		goto cleanup;
	}

	/* set the output writer */
	select_writer(&msg->out, obs->format);

	ret = do_read_op(msg, obs->format);
	if (ret < 0) {
		LOG_ERR("error in multi-format read (err:%d)", ret);
		goto cleanup;
	}

	ret = lwm2m_send_message(msg);
	if (ret < 0) {
		LOG_ERR("Error sending LWM2M packet (err:%d).", ret);
		goto cleanup;
	}

	LOG_DBG("NOTIFY MSG: SENT");
	return 0;

cleanup:
	lwm2m_reset_message(msg, true);
	return ret;
}

s32_t engine_next_service_timeout_ms(u32_t max_timeout)
{
	struct service_node *srv;
	u64_t time_left_ms, timestamp = k_uptime_get();
	u32_t timeout = max_timeout;

	SYS_SLIST_FOR_EACH_CONTAINER(&engine_service_list, srv, node) {
		time_left_ms = srv->last_timestamp + srv->min_call_period;

		/* service is due */
		if (time_left_ms < timestamp) {
			return 0;
		}

		/* service timeout is less than the current timeout */
		time_left_ms -= timestamp;
		if (time_left_ms < timeout) {
			timeout = time_left_ms;
		}
	}

	return timeout;
}

int lwm2m_engine_add_service(k_work_handler_t service, u32_t period_ms)
{
	int i;

	/* find an unused service index node */
	for (i = 0; i < MAX_PERIODIC_SERVICE; i++) {
		if (!service_node_data[i].service_work) {
			break;
		}
	}

	if (i == MAX_PERIODIC_SERVICE) {
		return -ENOMEM;
	}

	service_node_data[i].service_work = service;
	service_node_data[i].min_call_period = period_ms;
	service_node_data[i].last_timestamp = 0U;

	sys_slist_append(&engine_service_list,
			 &service_node_data[i].node);

	return 0;
}

static int lwm2m_engine_service(void)
{
	struct observe_node *obs;
	struct service_node *srv;
	s64_t timestamp, service_due_timestamp;

	/*
	 * 1. scan the observer list
	 * 2. For each notify event found, scan the observer list
	 * 3. For each observer match, generate a NOTIFY message,
	 *    attaching the notify response handler
	 */
	timestamp = k_uptime_get();
	SYS_SLIST_FOR_EACH_CONTAINER(&engine_observer_list, obs, node) {
		/*
		 * manual notify requirements:
		 * - event_timestamp > last_timestamp
		 * - current timestamp > last_timestamp + min_period_sec
		 */
		if (obs->event_timestamp > obs->last_timestamp &&
		    timestamp > obs->last_timestamp +
				MSEC_PER_SEC * obs->min_period_sec) {
			obs->last_timestamp = k_uptime_get();
			generate_notify_message(obs, true);

		/*
		 * automatic time-based notify requirements:
		 * - current timestamp > last_timestamp + max_period_sec
		 */
		} else if (timestamp > obs->last_timestamp +
				MSEC_PER_SEC * obs->max_period_sec) {
			obs->last_timestamp = k_uptime_get();
			generate_notify_message(obs, false);
		}

	}

	timestamp = k_uptime_get();
	SYS_SLIST_FOR_EACH_CONTAINER(&engine_service_list, srv, node) {
		service_due_timestamp = srv->last_timestamp +
					srv->min_call_period;
		/* service is due */
		if (timestamp >= service_due_timestamp) {
			srv->last_timestamp = k_uptime_get();
			srv->service_work(NULL);
		}
	}

	/* calculate how long to sleep till the next service */
	return engine_next_service_timeout_ms(ENGINE_UPDATE_INTERVAL_MS);
}

int lwm2m_engine_context_close(struct lwm2m_ctx *client_ctx)
{
	struct observe_node *obs, *tmp;
	sys_snode_t *prev_node = NULL;
	int sock_fd = client_ctx->sock_fd;

	/* Cancel pending retransmit work */
	k_delayed_work_cancel(&client_ctx->retransmit_work);

	/* Remove observes for this context */
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&engine_observer_list,
					  obs, tmp, node) {
		if (obs->ctx == client_ctx) {
			sys_slist_remove(&engine_observer_list, prev_node,
					 &obs->node);
			(void)memset(obs, 0, sizeof(*obs));
		} else {
			prev_node = &obs->node;
		}
	}

	lwm2m_socket_del(client_ctx);
	client_ctx->sock_fd = -1;
	if (sock_fd >= 0) {
		return close(sock_fd);
	} else {
		return 0;
	}
}

void lwm2m_engine_context_init(struct lwm2m_ctx *client_ctx)
{
	k_delayed_work_init(&client_ctx->retransmit_work, retransmit_request);
}

/* LwM2M Socket Integration */

int lwm2m_socket_add(struct lwm2m_ctx *ctx)
{
	int i;

	if (sock_nfds < MAX_POLL_FD) {
		i = sock_nfds++;
	} else {
		for (i = 0; i < MAX_POLL_FD; i++) {
			if (sock_ctx[i] == NULL) {
				goto found;
			}
		}

		return -ENOMEM;
	}

found:
	sock_ctx[i] = ctx;
	sock_fds[i].fd = ctx->sock_fd;
	sock_fds[i].events = POLLIN;
	return 0;
}

void lwm2m_socket_del(struct lwm2m_ctx *ctx)
{
	for (int i = 0; i < sock_nfds; i++) {
		if (sock_ctx[i] == ctx) {
			sock_ctx[i] = NULL;
			sock_fds[i].fd = -1;
			sock_nfds--;
			break;
		}
	}
}

/* LwM2M main work loop */

static void socket_receive_loop(void)
{
	static u8_t in_buf[NET_IPV6_MTU];
	static struct sockaddr from_addr;
	socklen_t from_addr_len;
	ssize_t len;
	int i;

	from_addr_len = sizeof(from_addr);
	while (1) {
		/* wait for sockets */
		if (sock_nfds < 1) {
			k_msleep(lwm2m_engine_service());
			continue;
		}

		/*
		 * FIXME: Currently we timeout and restart poll in case fds
		 *        were modified.
		 */
		if (poll(sock_fds, sock_nfds, lwm2m_engine_service()) < 0) {
			LOG_ERR("Error in poll:%d", errno);
			errno = 0;
			k_msleep(ENGINE_UPDATE_INTERVAL_MS);
			continue;
		}

		for (i = 0; i < sock_nfds; i++) {
			if (sock_fds[i].revents & POLLERR) {
				LOG_ERR("Error in poll.. waiting a moment.");
				k_msleep(ENGINE_UPDATE_INTERVAL_MS);
				continue;
			}

			if (!(sock_fds[i].revents & POLLIN) ||
			    sock_ctx[i] == NULL) {
				sock_fds[i].revents = 0;
				continue;
			}

			sock_fds[i].revents = 0;
			len = recvfrom(sock_ctx[i]->sock_fd, in_buf,
				       sizeof(in_buf) - 1, 0,
				       &from_addr, &from_addr_len);

			if (len < 0) {
				LOG_ERR("Error reading response: %d", errno);
				continue;
			}

			if (len == 0) {
				LOG_ERR("Zero length recv");
				continue;
			}

			in_buf[len] = 0U;

			lwm2m_udp_receive(sock_ctx[i], in_buf, len, &from_addr,
					  handle_request);
		}
	}
}

#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
static int load_tls_credential(struct lwm2m_ctx *client_ctx, u16_t res_id,
			       enum tls_credential_type type)
{
	int ret = 0;
	void *cred = NULL;
	u16_t cred_len;
	u8_t cred_flags;
	char pathstr[MAX_RESOURCE_LEN];

	/* ignore error value */
	tls_credential_delete(client_ctx->tls_tag, type);

	snprintk(pathstr, sizeof(pathstr), "0/%d/%u", client_ctx->sec_obj_inst,
		 res_id);

	ret = lwm2m_engine_get_res_data(pathstr, &cred, &cred_len, &cred_flags);
	if (ret < 0) {
		LOG_ERR("Unable to get resource data for '%s'",
			log_strdup(pathstr));
		return ret;
	}

	/* Set correct PSK_ID length */
	if (type == TLS_CREDENTIAL_PSK_ID) {
		cred_len = strlen(cred);
	}

	ret = tls_credential_add(client_ctx->tls_tag, type, cred, cred_len);
	if (ret < 0) {
		LOG_ERR("Error setting cred tag %d type %d: Error %d",
			client_ctx->tls_tag, type, ret);
	}

	return ret;
}
#endif /* CONFIG_LWM2M_DTLS_SUPPORT */

int lwm2m_socket_start(struct lwm2m_ctx *client_ctx)
{
#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
	int ret;

	if (client_ctx->load_credentials) {
		ret = client_ctx->load_credentials(client_ctx);
		if (ret < 0) {
			return ret;
		}
	} else {
		ret = load_tls_credential(client_ctx, 3, TLS_CREDENTIAL_PSK_ID);
		if (ret < 0) {
			return ret;
		}

		ret = load_tls_credential(client_ctx, 5, TLS_CREDENTIAL_PSK);
		if (ret < 0) {
			return ret;
		}
	}

	if (client_ctx->use_dtls) {
		client_ctx->sock_fd = socket(client_ctx->remote_addr.sa_family,
					     SOCK_DGRAM, IPPROTO_DTLS_1_2);
	} else
#endif /* CONFIG_LWM2M_DTLS_SUPPORT */
	{
		client_ctx->sock_fd = socket(client_ctx->remote_addr.sa_family,
					     SOCK_DGRAM, IPPROTO_UDP);
	}

	if (client_ctx->sock_fd < 0) {
		LOG_ERR("Failed to create socket: %d", errno);
		return -errno;
	}

#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
	if (client_ctx->use_dtls) {
		sec_tag_t tls_tag_list[] = {
			client_ctx->tls_tag,
		};

		ret = setsockopt(client_ctx->sock_fd, SOL_TLS, TLS_SEC_TAG_LIST,
				 tls_tag_list, sizeof(tls_tag_list));
		if (ret < 0) {
			LOG_ERR("Failed to set TLS_SEC_TAG_LIST option: %d",
				errno);
			lwm2m_engine_context_close(client_ctx);
			return -errno;
		}
	}
#endif /* CONFIG_LWM2M_DTLS_SUPPORT */

	if (connect(client_ctx->sock_fd, &client_ctx->remote_addr,
		    NET_SOCKADDR_MAX_SIZE) < 0) {
		LOG_ERR("Cannot connect UDP (-%d)", errno);
		lwm2m_engine_context_close(client_ctx);
		return -errno;
	}

	lwm2m_socket_add(client_ctx);
	return 0;
}

int lwm2m_parse_peerinfo(char *url, struct sockaddr *addr, bool *use_dtls)
{
	struct http_parser_url parser;
#if defined(CONFIG_LWM2M_DNS_SUPPORT)
	struct addrinfo *res, hints = { 0 };
#endif
	int ret;
	u16_t off, len;
	u8_t tmp;

	LOG_DBG("Parse url: %s", log_strdup(url));

	http_parser_url_init(&parser);
	ret = http_parser_parse_url(url, strlen(url), 0, &parser);
	if (ret < 0) {
		LOG_ERR("Invalid url: %s", log_strdup(url));
		return -ENOTSUP;
	}

	off = parser.field_data[UF_SCHEMA].off;
	len = parser.field_data[UF_SCHEMA].len;

	/* check for supported protocol */
	if (strncmp(url + off, "coaps", len) != 0) {
		return -EPROTONOSUPPORT;
	}

	/* check for DTLS requirement */
	*use_dtls = false;
	if (len == 5U && strncmp(url + off, "coaps", len) == 0) {
#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
		*use_dtls = true;
#else
		return -EPROTONOSUPPORT;
#endif /* CONFIG_LWM2M_DTLS_SUPPORT */
	}

	if (!(parser.field_set & (1 << UF_PORT))) {
		/* Set to default port of CoAP */
		parser.port = CONFIG_LWM2M_PEER_PORT;
	}

	off = parser.field_data[UF_HOST].off;
	len = parser.field_data[UF_HOST].len;

	/* truncate host portion */
	tmp = url[off + len];
	url[off + len] = '\0';

	/* initialize addr */
	(void)memset(addr, 0, sizeof(*addr));

	/* try and set IP address directly */
	addr->sa_family = AF_INET6;
	ret = net_addr_pton(AF_INET6, url + off,
			    &((struct sockaddr_in6 *)addr)->sin6_addr);
	/* Try to parse again using AF_INET */
	if (ret < 0) {
		addr->sa_family = AF_INET;
		ret = net_addr_pton(AF_INET, url + off,
				    &((struct sockaddr_in *)addr)->sin_addr);
	}

	if (ret < 0) {
#if defined(CONFIG_LWM2M_DNS_SUPPORT)
#if defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_IPV4)
		hints.ai_family = AF_UNSPEC;
#elif defined(CONFIG_NET_IPV6)
		hints.ai_family = AF_INET6;
#elif defined(CONFIG_NET_IPV4)
		hints.ai_family = AF_INET;
#else
		hints.ai_family = AF_UNSPEC;
#endif /* defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_IPV4) */
		hints.ai_socktype = SOCK_DGRAM;
		ret = getaddrinfo(url + off, NULL, &hints, &res);
		if (ret != 0) {
			LOG_ERR("Unable to resolve address");
			/* DNS error codes don't align with normal errors */
			ret = -ENOENT;
			goto cleanup;
		}

		memcpy(addr, res->ai_addr, sizeof(*addr));
		addr->sa_family = res->ai_family;
		freeaddrinfo(res);
#else
		goto cleanup;
#endif /* CONFIG_LWM2M_DNS_SUPPORT */
	}

	/* set port */
	if (addr->sa_family == AF_INET6) {
		net_sin6(addr)->sin6_port = htons(parser.port);
	} else if (addr->sa_family == AF_INET) {
		net_sin(addr)->sin_port = htons(parser.port);
	} else {
		ret = -EPROTONOSUPPORT;
	}

cleanup:
	/* restore host separator */
	url[off + len] = tmp;
	return ret;
}

int lwm2m_engine_start(struct lwm2m_ctx *client_ctx)
{
	char pathstr[MAX_RESOURCE_LEN];
	char *url;
	u16_t url_len;
	u8_t url_data_flags;
	int ret = 0U;

	/* get the server URL */
	snprintk(pathstr, sizeof(pathstr), "0/%d/0", client_ctx->sec_obj_inst);
	ret = lwm2m_engine_get_res_data(pathstr, (void **)&url, &url_len,
					&url_data_flags);
	if (ret < 0) {
		return ret;
	}

	url[url_len] = '\0';
	ret = lwm2m_parse_peerinfo(url, &client_ctx->remote_addr,
				   &client_ctx->use_dtls);
	if (ret < 0) {
		return ret;
	}

	lwm2m_engine_context_init(client_ctx);
	return lwm2m_socket_start(client_ctx);
}

static int lwm2m_engine_init(struct device *dev)
{
	int ret = 0;

	(void)memset(block1_contexts, 0,
		     sizeof(struct block_context) * NUM_BLOCK1_CONTEXT);

	/* start sock receive thread */
	k_thread_create(&engine_thread_data,
			&engine_thread_stack[0],
			K_THREAD_STACK_SIZEOF(engine_thread_stack),
			(k_thread_entry_t) socket_receive_loop,
			NULL, NULL, NULL,
			/* Lowest priority cooperative thread */
			K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1),
			0, K_NO_WAIT);
	k_thread_name_set(&engine_thread_data, "lwm2m-sock-recv");
	LOG_DBG("LWM2M engine socket receive thread started");

	return ret;
}

SYS_INIT(lwm2m_engine_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
