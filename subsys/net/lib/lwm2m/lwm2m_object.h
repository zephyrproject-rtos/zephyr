/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2018-2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Copyright (c) 2015, Yanzi Networks AB.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Original Authors:
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */

#ifndef LWM2M_OBJECT_H_
#define LWM2M_OBJECT_H_

/* stdint conversions */
#include <zephyr/types.h>
#include <stddef.h>
#include <kernel.h>

#include <net/net_ip.h>
#include <sys/printk.h>
#include <sys/util.h>
#include <sys/types.h>

#include <net/coap.h>
#include <net/lwm2m.h>

#include "buf_util.h"

/* #####/###/#####/### + NULL */
#define MAX_RESOURCE_LEN	20

/* operations / permissions */
/* values from 0 to 7 can be used as permission checks */
#define LWM2M_OP_READ		0
#define LWM2M_OP_WRITE		1
#define LWM2M_OP_CREATE		2
#define LWM2M_OP_DELETE		3
#define LWM2M_OP_EXECUTE	4
#define LWM2M_FLAG_OPTIONAL	7
/* values >7 aren't used for permission checks */
#define LWM2M_OP_DISCOVER	8
#define LWM2M_OP_WRITE_ATTR	9

/* resource permissions */
#define LWM2M_PERM_R		BIT(LWM2M_OP_READ)
#define LWM2M_PERM_R_OPT	(BIT(LWM2M_OP_READ) | \
				 BIT(LWM2M_FLAG_OPTIONAL))
#define LWM2M_PERM_W		(BIT(LWM2M_OP_WRITE) | \
				 BIT(LWM2M_OP_CREATE))
#define LWM2M_PERM_W_OPT	(BIT(LWM2M_OP_WRITE) | \
				 BIT(LWM2M_OP_CREATE) | \
				 BIT(LWM2M_FLAG_OPTIONAL))
#define LWM2M_PERM_X		BIT(LWM2M_OP_EXECUTE)
#define LWM2M_PERM_X_OPT	(BIT(LWM2M_OP_EXECUTE) | \
				 BIT(LWM2M_FLAG_OPTIONAL))
#define LWM2M_PERM_RW		(BIT(LWM2M_OP_READ) | \
				 BIT(LWM2M_OP_WRITE) | \
				 BIT(LWM2M_OP_CREATE))
#define LWM2M_PERM_RW_OPT	(BIT(LWM2M_OP_READ) | \
				 BIT(LWM2M_OP_WRITE) | \
				 BIT(LWM2M_OP_CREATE) | \
				 BIT(LWM2M_FLAG_OPTIONAL))
#define LWM2M_PERM_RWX		(BIT(LWM2M_OP_READ) | \
				 BIT(LWM2M_OP_WRITE) | \
				 BIT(LWM2M_OP_CREATE) | \
				 BIT(LWM2M_OP_EXECUTE))
#define LWM2M_PERM_RWX_OPT	(BIT(LWM2M_OP_READ) | \
				 BIT(LWM2M_OP_WRITE) | \
				 BIT(LWM2M_OP_CREATE) | \
				 BIT(LWM2M_OP_EXECUTE) | \
				 BIT(LWM2M_FLAG_OPTIONAL))

#define LWM2M_HAS_PERM(of, p)	(((of)->permissions & p) == p)

/* resource types */
#define LWM2M_RES_TYPE_NONE	0
#define LWM2M_RES_TYPE_OPAQUE	1
#define LWM2M_RES_TYPE_STRING	2
#define LWM2M_RES_TYPE_UINT64	3
#define LWM2M_RES_TYPE_U64	3
#define LWM2M_RES_TYPE_UINT	4
#define LWM2M_RES_TYPE_U32	4
#define LWM2M_RES_TYPE_U16	5
#define LWM2M_RES_TYPE_U8	6
#define LWM2M_RES_TYPE_INT64	7
#define LWM2M_RES_TYPE_S64	7
#define LWM2M_RES_TYPE_INT	8
#define LWM2M_RES_TYPE_S32	8
#define LWM2M_RES_TYPE_S16	9
#define LWM2M_RES_TYPE_S8	10
#define LWM2M_RES_TYPE_BOOL	11
#define LWM2M_RES_TYPE_TIME	12
#define LWM2M_RES_TYPE_FLOAT32	13
#define LWM2M_RES_TYPE_FLOAT64	14
#define LWM2M_RES_TYPE_OBJLNK	15

/* remember that we have already output a value - can be between two block's */
#define WRITER_OUTPUT_VALUE      1
#define WRITER_RESOURCE_INSTANCE 2

#define MAX_PACKET_SIZE		(CONFIG_LWM2M_COAP_BLOCK_SIZE + \
				 CONFIG_LWM2M_ENGINE_MESSAGE_HEADER_SIZE)

/* buffer util macros */
#define CPKT_BUF_WRITE(cpkt)	(cpkt)->data, &(cpkt)->offset, (cpkt)->max_len
#define CPKT_BUF_READ(cpkt)	(cpkt)->data, (cpkt)->max_len

struct lwm2m_engine_obj;
struct lwm2m_message;

#define LWM2M_PATH_LEVEL_NONE 0
#define LWM2M_PATH_LEVEL_OBJECT 1
#define LWM2M_PATH_LEVEL_OBJECT_INST 2
#define LWM2M_PATH_LEVEL_RESOURCE 3
#define LWM2M_PATH_LEVEL_RESOURCE_INST 4

/* path representing object instances */
struct lwm2m_obj_path {
	uint16_t obj_id;
	uint16_t obj_inst_id;
	uint16_t res_id;
	uint16_t res_inst_id;
	uint8_t  level;  /* 0/1/2/3/4 (4 = resource instance) */
};

#define OBJ_FIELD(_id, _perm, _type) \
	{ .res_id = _id, \
	  .permissions = LWM2M_PERM_ ## _perm, \
	  .data_type = LWM2M_RES_TYPE_ ## _type, }

/* Keep OBJ_FIELD_DATA around for historical reasons */
#define OBJ_FIELD_DATA(res_id, perm, type) \
	OBJ_FIELD(res_id, perm, type)

#define OBJ_FIELD_EXECUTE(res_id) \
	OBJ_FIELD(res_id, X, NONE)

#define OBJ_FIELD_EXECUTE_OPT(res_id) \
	OBJ_FIELD(res_id, X_OPT, NONE)

struct lwm2m_engine_obj_field {
	uint16_t  res_id;
	uint8_t   permissions;
	uint8_t   data_type;
};

typedef struct lwm2m_engine_obj_inst *
	(*lwm2m_engine_obj_create_cb_t)(uint16_t obj_inst_id);

struct lwm2m_engine_obj {
	/* object list */
	sys_snode_t node;

	/* object field definitions */
	struct lwm2m_engine_obj_field *fields;

	/* object event callbacks */
	lwm2m_engine_obj_create_cb_t create_cb;
	lwm2m_engine_user_cb_t delete_cb;
	lwm2m_engine_user_cb_t user_create_cb;
	lwm2m_engine_user_cb_t user_delete_cb;

	/* object member data */
	uint16_t obj_id;
	uint16_t field_count;
	uint16_t instance_count;
	uint16_t max_instance_count;

	/* Object version information. */
	uint8_t version_major;
	uint8_t version_minor;

	/* Object is a core object (defined in the official LwM2M spec.) */
	bool is_core : 1;
};

/* Resource instances with this value are considered "not created" yet */
#define RES_INSTANCE_NOT_CREATED 65535

/* Resource macros */

#if CONFIG_LWM2M_ENGINE_VALIDATION_BUFFER_SIZE > 0
#define _INIT_OBJ_RES(_id, _r_ptr, _r_idx, _ri_ptr, _ri_count, _multi_ri, \
		      _r_cb, _pre_w_cb, _val_cb, _post_w_cb, _ex_cb) \
	_r_ptr[_r_idx].res_id = _id; \
	_r_ptr[_r_idx].res_instances = _ri_ptr; \
	_r_ptr[_r_idx].res_inst_count = _ri_count; \
	_r_ptr[_r_idx].multi_res_inst = _multi_ri; \
	_r_ptr[_r_idx].read_cb = _r_cb; \
	_r_ptr[_r_idx].pre_write_cb = _pre_w_cb; \
	_r_ptr[_r_idx].validate_cb = _val_cb; \
	_r_ptr[_r_idx].post_write_cb = _post_w_cb; \
	_r_ptr[_r_idx].execute_cb = _ex_cb
#else
#define _INIT_OBJ_RES(_id, _r_ptr, _r_idx, _ri_ptr, _ri_count, _multi_ri, \
		      _r_cb, _pre_w_cb, _val_cb, _post_w_cb, _ex_cb) \
	_r_ptr[_r_idx].res_id = _id; \
	_r_ptr[_r_idx].res_instances = _ri_ptr; \
	_r_ptr[_r_idx].res_inst_count = _ri_count; \
	_r_ptr[_r_idx].multi_res_inst = _multi_ri; \
	_r_ptr[_r_idx].read_cb = _r_cb; \
	_r_ptr[_r_idx].pre_write_cb = _pre_w_cb; \
	_r_ptr[_r_idx].post_write_cb = _post_w_cb; \
	_r_ptr[_r_idx].execute_cb = _ex_cb
#endif /* CONFIG_LWM2M_ENGINE_VALIDATION_BUFFER_SIZE > 0 */

#define _INIT_OBJ_RES_INST(_ri_ptr, _ri_idx, _ri_count, _ri_create, \
			   _data_ptr, _data_len) \
	do { \
		if (_ri_ptr != NULL && _ri_count > 0) { \
			for (int _i = 0; _i < _ri_count; _i++) { \
				_ri_ptr[_ri_idx + _i].data_ptr = \
						(_data_ptr + _i); \
				_ri_ptr[_ri_idx + _i].max_data_len = \
						_data_len; \
				_ri_ptr[_ri_idx + _i].data_len = \
						_data_len; \
				if (_ri_create) { \
					_ri_ptr[_ri_idx + _i].res_inst_id = \
						_i; \
				} else { \
					_ri_ptr[_ri_idx + _i].res_inst_id = \
						RES_INSTANCE_NOT_CREATED; \
				} \
			} \
		} \
		_ri_idx += _ri_count; \
	} while (false)

#define _INIT_OBJ_RES_INST_OPT(_ri_ptr, _ri_idx, _ri_count, _ri_create) \
	do { \
		if (_ri_count > 0) { \
			for (int _i = 0; _i < _ri_count; _i++) { \
				_ri_ptr[_ri_idx + _i].data_ptr = NULL; \
				_ri_ptr[_ri_idx + _i].max_data_len = 0; \
				_ri_ptr[_ri_idx + _i].data_len = 0; \
				if (_ri_create) { \
					_ri_ptr[_ri_idx + _i].res_inst_id = \
						_i; \
				} else { \
					_ri_ptr[_ri_idx + _i].res_inst_id = \
						RES_INSTANCE_NOT_CREATED; \
				} \
			} \
		} \
		_ri_idx += _ri_count; \
	} while (false)

#define INIT_OBJ_RES(_id, _r_ptr, _r_idx, \
		     _ri_ptr, _ri_idx, _ri_count, _multi_ri, _ri_create, \
		     _data_ptr, _data_len, \
		     _r_cb, _pre_w_cb, _val_cb, _post_w_cb, _ex_cb) \
	do { \
		_INIT_OBJ_RES(_id, _r_ptr, _r_idx, \
			      (_ri_ptr + _ri_idx), _ri_count, _multi_ri, \
			      _r_cb, _pre_w_cb, _val_cb, _post_w_cb, _ex_cb); \
		_INIT_OBJ_RES_INST(_ri_ptr, _ri_idx, _ri_count, _ri_create, \
				   _data_ptr, _data_len); \
	++_r_idx; \
	} while (false)


#define INIT_OBJ_RES_OPT(_id, _r_ptr, _r_idx, \
			 _ri_ptr, _ri_idx, _ri_count, _multi_ri, _ri_create, \
			 _r_cb, _pre_w_cb, _val_cb, _post_w_cb, _ex_cb) \
	do { \
		_INIT_OBJ_RES(_id, _r_ptr, _r_idx, \
			      (_ri_ptr + _ri_idx), _ri_count, _multi_ri, \
			      _r_cb, _pre_w_cb, _val_cb, _post_w_cb, _ex_cb); \
		_INIT_OBJ_RES_INST_OPT(_ri_ptr, _ri_idx, _ri_count, _ri_create); \
		++_r_idx; \
	} while (false)

#define INIT_OBJ_RES_MULTI_DATA(_id, _r_ptr, _r_idx, \
				_ri_ptr, _ri_idx, _ri_count, _ri_create, \
				_data_ptr, _data_len) \
	INIT_OBJ_RES(_id, _r_ptr, _r_idx, \
		     _ri_ptr, _ri_idx, _ri_count, true, _ri_create, \
		     _data_ptr, _data_len, NULL, NULL, NULL, NULL, NULL)

#define INIT_OBJ_RES_MULTI_OPTDATA(_id, _r_ptr, _r_idx, \
				   _ri_ptr, _ri_idx, _ri_count, _ri_create) \
	INIT_OBJ_RES_OPT(_id, _r_ptr, _r_idx, \
			 _ri_ptr, _ri_idx, _ri_count, true, _ri_create, \
			 NULL, NULL, NULL, NULL, NULL)

#define INIT_OBJ_RES_DATA(_id, _r_ptr, _r_idx, _ri_ptr, _ri_idx, \
			  _data_ptr, _data_len) \
	INIT_OBJ_RES(_id, _r_ptr, _r_idx, _ri_ptr, _ri_idx, 1U, false, true, \
		     _data_ptr, _data_len, NULL, NULL, NULL, NULL, NULL)

#define INIT_OBJ_RES_OPTDATA(_id, _r_ptr, _r_idx, _ri_ptr, _ri_idx) \
	INIT_OBJ_RES_OPT(_id, _r_ptr, _r_idx, _ri_ptr, _ri_idx, 1U, false, \
			 true, NULL, NULL, NULL, NULL, NULL)

#define INIT_OBJ_RES_EXECUTE(_id, _r_ptr, _r_idx, _ex_cb) \
	do { \
		_INIT_OBJ_RES(_id, _r_ptr, _r_idx, NULL, 0, false, \
			      NULL, NULL, NULL, NULL, _ex_cb); \
		++_r_idx; \
	} while (false)


#define LWM2M_ATTR_PMIN	0
#define LWM2M_ATTR_PMAX	1
#define LWM2M_ATTR_GT	2
#define LWM2M_ATTR_LT	3
#define LWM2M_ATTR_STEP	4
#define NR_LWM2M_ATTR	5

/* TODO: support multiple server (sec 5.4.2) */
struct lwm2m_attr {
	void *ref;

	/* values */
	union {
		float32_value_t float_val;
		int32_t int_val;
	};

	uint8_t type;
};

struct lwm2m_engine_res_inst {
	void  *data_ptr;
	uint16_t max_data_len;
	uint16_t data_len;
	uint16_t res_inst_id; /* 65535 == not "created" */
	uint8_t  data_flags;
};

struct lwm2m_engine_res {
	lwm2m_engine_get_data_cb_t		read_cb;
	lwm2m_engine_get_data_cb_t		pre_write_cb;
#if CONFIG_LWM2M_ENGINE_VALIDATION_BUFFER_SIZE > 0
	lwm2m_engine_set_data_cb_t		validate_cb;
#endif
	lwm2m_engine_set_data_cb_t		post_write_cb;
	lwm2m_engine_execute_cb_t		execute_cb;

	struct lwm2m_engine_res_inst *res_instances;
	uint16_t res_id;
	uint8_t  res_inst_count;
	bool multi_res_inst;
};

struct lwm2m_engine_obj_inst {
	/* instance list */
	sys_snode_t node;

	struct lwm2m_engine_obj *obj;
	struct lwm2m_engine_res *resources;

	/* object instance member data */
	uint16_t obj_inst_id;
	uint16_t resource_count;
};

/* Initialize resource instances prior to use */
static inline void init_res_instance(struct lwm2m_engine_res_inst *ri,
				     size_t ri_len)
{
	size_t i;

	memset(ri, 0, sizeof(*ri) * ri_len);
	for (i = 0; i < ri_len; i++) {
		ri[i].res_inst_id = RES_INSTANCE_NOT_CREATED;
	}
}

struct lwm2m_opaque_context {
	size_t len;
	size_t remaining;
};

struct lwm2m_block_context {
	struct coap_block_context ctx;
	struct lwm2m_opaque_context opaque;
	int64_t timestamp;
	uint32_t expected;
	uint8_t token[8];
	uint8_t tkl;
	bool last_block : 1;
	uint16_t res_id;
};

struct lwm2m_output_context {
	const struct lwm2m_writer *writer;
	struct coap_packet *out_cpkt;

	/* private output data */
	void *user_data;
};

struct lwm2m_input_context {
	const struct lwm2m_reader *reader;
	struct coap_packet *in_cpkt;

	/* current position in buffer */
	uint16_t offset;

	/* Corresponding block context. NULL if block transfer is not used. */
	struct lwm2m_block_context *block_ctx;

	/* private output data */
	void *user_data;
};

/* Establish a message timeout callback */
typedef void (*lwm2m_message_timeout_cb_t)(struct lwm2m_message *msg);

/* Internal LwM2M message structure to track in-flight messages. */
struct lwm2m_message {
	sys_snode_t node;

	/** LwM2M context related to this message */
	struct lwm2m_ctx *ctx;

	/** Incoming / outgoing contexts */
	struct lwm2m_input_context in;
	struct lwm2m_output_context out;

	/** Incoming path */
	struct lwm2m_obj_path path;

	/** CoAP packet data related to the outgoing message */
	struct coap_packet cpkt;

	/** Buffer data related outgoing message */
	uint8_t msg_data[MAX_PACKET_SIZE];

	/** Message transmission handling for TYPE_CON */
	struct coap_pending *pending;
	struct coap_reply *reply;

	/** Message configuration */
	uint8_t *token;
	coap_reply_t reply_cb;
	lwm2m_message_timeout_cb_t message_timeout_cb;
	uint16_t mid;
	uint8_t type;
	uint8_t code;
	uint8_t tkl;

	/** Incoming message action */
	uint8_t operation;

	/* Information whether the message was acknowledged. */
	bool acknowledged : 1;
};

/* LWM2M format writer for the various formats supported */
struct lwm2m_writer {
	size_t (*put_begin)(struct lwm2m_output_context *out,
			    struct lwm2m_obj_path *path);
	size_t (*put_end)(struct lwm2m_output_context *out,
			  struct lwm2m_obj_path *path);
	size_t (*put_begin_oi)(struct lwm2m_output_context *out,
			       struct lwm2m_obj_path *path);
	size_t (*put_end_oi)(struct lwm2m_output_context *out,
			     struct lwm2m_obj_path *path);
	size_t (*put_begin_r)(struct lwm2m_output_context *out,
			      struct lwm2m_obj_path *path);
	size_t (*put_end_r)(struct lwm2m_output_context *out,
			    struct lwm2m_obj_path *path);
	size_t (*put_begin_ri)(struct lwm2m_output_context *out,
			       struct lwm2m_obj_path *path);
	size_t (*put_end_ri)(struct lwm2m_output_context *out,
			     struct lwm2m_obj_path *path);
	size_t (*put_s8)(struct lwm2m_output_context *out,
			 struct lwm2m_obj_path *path,
			 int8_t value);
	size_t (*put_s16)(struct lwm2m_output_context *out,
			  struct lwm2m_obj_path *path,
			  int16_t value);
	size_t (*put_s32)(struct lwm2m_output_context *out,
			  struct lwm2m_obj_path *path,
			  int32_t value);
	size_t (*put_s64)(struct lwm2m_output_context *out,
			  struct lwm2m_obj_path *path,
			  int64_t value);
	size_t (*put_string)(struct lwm2m_output_context *out,
			     struct lwm2m_obj_path *path,
			     char *buf, size_t buflen);
	size_t (*put_float32fix)(struct lwm2m_output_context *out,
				 struct lwm2m_obj_path *path,
				 float32_value_t *value);
	size_t (*put_float64fix)(struct lwm2m_output_context *out,
				 struct lwm2m_obj_path *path,
				 float64_value_t *value);
	size_t (*put_bool)(struct lwm2m_output_context *out,
			   struct lwm2m_obj_path *path,
			   bool value);
	size_t (*put_opaque)(struct lwm2m_output_context *out,
			     struct lwm2m_obj_path *path,
			     char *buf, size_t buflen);
	size_t (*put_objlnk)(struct lwm2m_output_context *out,
			     struct lwm2m_obj_path *path,
			     struct lwm2m_objlnk *value);
	ssize_t (*put_corelink)(struct lwm2m_output_context *out,
				const struct lwm2m_obj_path *path);
};

struct lwm2m_reader {
	size_t (*get_s32)(struct lwm2m_input_context *in,
			  int32_t *value);
	size_t (*get_s64)(struct lwm2m_input_context *in,
			  int64_t *value);
	size_t (*get_string)(struct lwm2m_input_context *in,
			     uint8_t *buf, size_t buflen);
	size_t (*get_float32fix)(struct lwm2m_input_context *in,
				 float32_value_t *value);
	size_t (*get_float64fix)(struct lwm2m_input_context *in,
				 float64_value_t *value);
	size_t (*get_bool)(struct lwm2m_input_context *in,
			   bool *value);
	size_t (*get_opaque)(struct lwm2m_input_context *in,
			     uint8_t *buf, size_t buflen,
			     struct lwm2m_opaque_context *opaque,
			     bool *last_block);
	size_t (*get_objlnk)(struct lwm2m_input_context *in,
			     struct lwm2m_objlnk *value);
};

/* output user_data management functions */

static inline void engine_set_out_user_data(struct lwm2m_output_context *out,
					    void *user_data)
{
	out->user_data = user_data;
}

static inline void *engine_get_out_user_data(struct lwm2m_output_context *out)
{
	return out->user_data;
}

static inline void
engine_clear_out_user_data(struct lwm2m_output_context *out)
{
	out->user_data = NULL;
}

static inline void engine_set_in_user_data(struct lwm2m_input_context *in,
					   void *user_data)
{
	in->user_data = user_data;
}

static inline void *engine_get_in_user_data(struct lwm2m_input_context *in)
{
	return in->user_data;
}

static inline void
engine_clear_in_user_data(struct lwm2m_input_context *in)
{
	in->user_data = NULL;
}

/* inline multi-format write / read functions */

static inline size_t engine_put_begin(struct lwm2m_output_context *out,
				      struct lwm2m_obj_path *path)
{
	if (out->writer->put_begin) {
		return out->writer->put_begin(out, path);
	}

	return 0;
}

static inline size_t engine_put_end(struct lwm2m_output_context *out,
				    struct lwm2m_obj_path *path)
{
	if (out->writer->put_end) {
		return out->writer->put_end(out, path);
	}

	return 0;
}

static inline size_t engine_put_begin_oi(struct lwm2m_output_context *out,
					 struct lwm2m_obj_path *path)
{
	if (out->writer->put_begin_oi) {
		return out->writer->put_begin_oi(out, path);
	}

	return 0;
}

static inline size_t engine_put_end_oi(struct lwm2m_output_context *out,
				       struct lwm2m_obj_path *path)
{
	if (out->writer->put_end_oi) {
		return out->writer->put_end_oi(out, path);
	}

	return 0;
}

static inline size_t engine_put_begin_r(struct lwm2m_output_context *out,
					struct lwm2m_obj_path *path)
{
	if (out->writer->put_begin_r) {
		return out->writer->put_begin_r(out, path);
	}

	return 0;
}

static inline size_t engine_put_end_r(struct lwm2m_output_context *out,
				      struct lwm2m_obj_path *path)
{
	if (out->writer->put_end_r) {
		return out->writer->put_end_r(out, path);
	}

	return 0;
}

static inline size_t engine_put_begin_ri(struct lwm2m_output_context *out,
					 struct lwm2m_obj_path *path)
{
	if (out->writer->put_begin_ri) {
		return out->writer->put_begin_ri(out, path);
	}

	return 0;
}

static inline size_t engine_put_end_ri(struct lwm2m_output_context *out,
				       struct lwm2m_obj_path *path)
{
	if (out->writer->put_end_ri) {
		return out->writer->put_end_ri(out, path);
	}

	return 0;
}

static inline size_t engine_put_s8(struct lwm2m_output_context *out,
				   struct lwm2m_obj_path *path,
				   int8_t value)
{
	return out->writer->put_s8(out, path, value);
}

static inline size_t engine_put_s16(struct lwm2m_output_context *out,
				    struct lwm2m_obj_path *path,
				    int16_t value)
{
	return out->writer->put_s16(out, path, value);
}

static inline size_t engine_put_s32(struct lwm2m_output_context *out,
				    struct lwm2m_obj_path *path,
				    int32_t value)
{
	return out->writer->put_s32(out, path, value);
}

static inline size_t engine_put_s64(struct lwm2m_output_context *out,
				    struct lwm2m_obj_path *path,
				    int64_t value)
{
	return out->writer->put_s64(out, path, value);
}

static inline size_t engine_put_string(struct lwm2m_output_context *out,
				       struct lwm2m_obj_path *path,
				       char *buf, size_t buflen)
{
	return out->writer->put_string(out, path, buf, buflen);
}

static inline size_t engine_put_float32fix(struct lwm2m_output_context *out,
					   struct lwm2m_obj_path *path,
					   float32_value_t *value)
{
	return out->writer->put_float32fix(out, path, value);
}

static inline size_t engine_put_float64fix(struct lwm2m_output_context *out,
					   struct lwm2m_obj_path *path,
					   float64_value_t *value)
{
	return out->writer->put_float64fix(out, path, value);
}

static inline size_t engine_put_bool(struct lwm2m_output_context *out,
				     struct lwm2m_obj_path *path,
				     bool value)
{
	return out->writer->put_bool(out, path, value);
}

static inline size_t engine_put_opaque(struct lwm2m_output_context *out,
				       struct lwm2m_obj_path *path,
				       char *buf, size_t buflen)
{
	if (out->writer->put_opaque) {
		return out->writer->put_opaque(out, path, buf, buflen);
	}

	return 0;
}

static inline size_t engine_put_objlnk(struct lwm2m_output_context *out,
				       struct lwm2m_obj_path *path,
				       struct lwm2m_objlnk *value)
{
	return out->writer->put_objlnk(out, path, value);
}

static inline ssize_t engine_put_corelink(struct lwm2m_output_context *out,
					  const struct lwm2m_obj_path *path)
{
	if (out->writer->put_corelink) {
		return out->writer->put_corelink(out, path);
	}

	return -ENOTSUP;
}

static inline size_t engine_get_s32(struct lwm2m_input_context *in,
				    int32_t *value)
{
	return in->reader->get_s32(in, value);
}

static inline size_t engine_get_s64(struct lwm2m_input_context *in,
				    int64_t *value)
{
	return in->reader->get_s64(in, value);
}

static inline size_t engine_get_string(struct lwm2m_input_context *in,
				       uint8_t *buf, size_t buflen)
{
	return in->reader->get_string(in, buf, buflen);
}

static inline size_t engine_get_float32fix(struct lwm2m_input_context *in,
					   float32_value_t *value)
{
	return in->reader->get_float32fix(in, value);
}

static inline size_t engine_get_float64fix(struct lwm2m_input_context *in,
					   float64_value_t *value)
{
	return in->reader->get_float64fix(in, value);
}

static inline size_t engine_get_bool(struct lwm2m_input_context *in,
				     bool *value)
{
	return in->reader->get_bool(in, value);
}

static inline size_t engine_get_opaque(struct lwm2m_input_context *in,
				       uint8_t *buf, size_t buflen,
				       struct lwm2m_opaque_context *opaque,
				       bool *last_block)
{
	if (in->reader->get_opaque) {
		return in->reader->get_opaque(in, buf, buflen,
					      opaque, last_block);
	}

	return 0;
}

static inline size_t engine_get_objlnk(struct lwm2m_input_context *in,
				       struct lwm2m_objlnk *value)
{
	return in->reader->get_objlnk(in, value);
}

#endif /* LWM2M_OBJECT_H_ */
