/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <string.h>
#include <atomic.h>

#include <bluetooth/gatt.h>
/* for bt_security_t */
#include <bluetooth/conn.h>
#include <bluetooth/log.h>

#ifdef CONFIG_PRINTK
#include <misc/printk.h>
#define PRINTK(...) printk(__VA_ARGS__)
#else
#define PRINTK(...)
#endif /* CONFIG_PRINTK */

#include "rpc.h"
#include "gap_internal.h"
#include "gatt_internal.h"

#include "rpc_functions_to_quark.h"

#if !defined(CONFIG_NBLE_DEBUG_RPC)
#undef BT_DBG
#define BT_DBG(fmt, ...)
#endif

/* Build the list of prototypes and check that list are made only of matching
 * signatures
 */
#define FN_SIG_NONE(__fn)	void __fn(void);
LIST_FN_SIG_NONE
#undef FN_SIG_NONE

#define FN_SIG_S(__fn, __s)	void __fn(__s p_s);
LIST_FN_SIG_S
#undef FN_SIG_S

#define FN_SIG_P(__fn, __type)	void __fn(__type priv);
LIST_FN_SIG_P
#undef FN_SIG_P

#define FN_SIG_S_B(__fn, __s, __type, __length)				\
	void __fn(__s p_s, __type buf, __length length);
LIST_FN_SIG_S_B
#undef FN_SIG_S_B

#define FN_SIG_B_B_P(__fn, __type1, __length1, __type2, __length2,	\
		     __type3)						\
	void __fn(__type1 p_buf1, __length1 length1, __type2 p_buf2,	\
		  __length2 length2, __type3 priv);
LIST_FN_SIG_B_B_P
#undef FN_SIG_B_B_P

#define FN_SIG_S_P(__fn, __s, __type)	void __fn(__s p_s, __type priv);
LIST_FN_SIG_S_P
#undef FN_SIG_S_P

#define FN_SIG_S_B_P(__fn, __s,  __type, __length, __type_ptr)		\
	void __fn(__s p_s, __type buf, __length length, __type_ptr priv);
LIST_FN_SIG_S_B_P
#undef FN_SIG_S_B_P

#define FN_SIG_S_B_B_P(__fn, __s,  __type1, __length1, __type2,		\
		       __length2, __type_ptr)				\
	void __fn(__s p_s, __type1 p_buf1, __length1 length1,		\
		  __type2 p_buf2, __length2 length2, __type_ptr priv);
LIST_FN_SIG_S_B_B_P
#undef FN_SIG_S_B_B_P

/* 1 - define the size check arrays */
#define FN_SIG_NONE(__fn)

#define FN_SIG_S(__fn, __s)				sizeof(*((__s)0)),

#define FN_SIG_P(__fn, __type)

#define FN_SIG_S_B(__fn, __s, __type, __length)		sizeof(*((__s)0)),

#define FN_SIG_B_B_P(__fn, __type1, __length1, __type2, __length2,	\
		     __type3)				sizeof(*((__s)0)),

#define FN_SIG_S_P(__fn, __s, __type)			sizeof(*((__s)0)),

#define FN_SIG_S_B_P(__fn, __s, __type, __length, __type_ptr)		\
							sizeof(*((__s)0)),

#define FN_SIG_S_B_B_P(__fn, __s, __type1, __length1, __type2,		\
		       __length2, __type3)		sizeof(*((__s)0)),

static uint8_t m_size_s[] = { LIST_FN_SIG_S };
static uint8_t m_size_s_b[] = { LIST_FN_SIG_S_B };
static uint8_t m_size_s_p[] = { LIST_FN_SIG_S_P };
static uint8_t m_size_s_b_p[] = { LIST_FN_SIG_S_B_P };
static uint8_t m_size_s_b_b_p[] = { LIST_FN_SIG_S_B_B_P };

#undef FN_SIG_NONE
#undef FN_SIG_S
#undef FN_SIG_P
#undef FN_SIG_S_B
#undef FN_SIG_B_B_P
#undef FN_SIG_S_P
#undef FN_SIG_S_B_P
#undef FN_SIG_S_B_B_P

/* 2- build the enumerations list */
#define FN_SIG_NONE(__fn)				fn_index_##__fn,
#define FN_SIG_S(__fn, __s)				FN_SIG_NONE(__fn)
#define FN_SIG_P(__fn, __type)				FN_SIG_NONE(__fn)
#define FN_SIG_S_B(__fn, __s, __type, __length)		FN_SIG_NONE(__fn)
#define FN_SIG_B_B_P(__fn, __type1, __length1, __type2, __length2,	\
		     __type3)				FN_SIG_NONE(__fn)
#define FN_SIG_S_P(__fn, __s, __type)			FN_SIG_NONE(__fn)
#define FN_SIG_S_B_P(__fn, __s, __type, __length, __type_ptr)		\
							FN_SIG_NONE(__fn)
#define FN_SIG_S_B_B_P(__fn, __s, __type1, __length1, __type2,		\
		       __length2, __type3)		FN_SIG_NONE(__fn)

/* Build the list of function indexes in the deserialization array */
enum { LIST_FN_SIG_NONE fn_none_index_max };
enum { LIST_FN_SIG_S fn_s_index_max };
enum { LIST_FN_SIG_P fn_p_index_max };
enum { LIST_FN_SIG_S_B fn_s_b_index_max };
enum { LIST_FN_SIG_B_B_P fn_b_b_p_index_max };
enum { LIST_FN_SIG_S_P fn_s_p_index_max };
enum { LIST_FN_SIG_S_B_P fn_s_b_p_index_max };
enum { LIST_FN_SIG_S_B_B_P fn_s_b_b_p_index_max };

#undef FN_SIG_NONE
#undef FN_SIG_S
#undef FN_SIG_P
#undef FN_SIG_S_B
#undef FN_SIG_B_B_P
#undef FN_SIG_S_P
#undef FN_SIG_S_B_P
#undef FN_SIG_S_B_B_P

/* 3- build the array */
#define FN_SIG_NONE(__fn)			[fn_index_##__fn] =	\
								(void *)__fn,
#define FN_SIG_S(__fn, __s)			FN_SIG_NONE(__fn)
#define FN_SIG_P(__fn, __type)			FN_SIG_NONE(__fn)
#define FN_SIG_S_B(__fn, __s, __type, __length)				\
						FN_SIG_NONE(__fn)
#define FN_SIG_B_B_P(__fn, __type1, __length1, __type2, __length2,	\
		     __type3)			FN_SIG_NONE(__fn)
#define FN_SIG_S_P(__fn, __s, __type)		FN_SIG_NONE(__fn)
#define FN_SIG_S_B_P(__fn, __s, __type, __length, __type_ptr)		\
						FN_SIG_NONE(__fn)
#define FN_SIG_S_B_B_P(__fn, __s, __type1, __length1, __type2,		\
		       __length2, __type3)	FN_SIG_NONE(__fn)

static void (*m_fct_none[])(void) = { LIST_FN_SIG_NONE };
static void (*m_fct_s[])(void *structure) = { LIST_FN_SIG_S };
static void (*m_fct_p[])(void *pointer) = { LIST_FN_SIG_P };
static void (*m_fct_s_b[])(void *structure, void *buffer,
			   uint8_t length) = { LIST_FN_SIG_S_B };
static void (*m_fct_b_b_p[])(void *buffer1, uint8_t length1,
			     void *buffer2, uint8_t length2,
			     void *pointer) = { LIST_FN_SIG_B_B_P };
static void (*m_fct_s_p[])(void *structure,
			   void *pointer) = { LIST_FN_SIG_S_P };
static void (*m_fct_s_b_p[])(void *structure, void *buffer, uint8_t length,
			     void *pointer) = { LIST_FN_SIG_S_B_P };
static void (*m_fct_s_b_b_p[])(void *structure, void *buffer1, uint8_t length1,
			       void *buffer2, uint8_t length2,
			       void *pointer) = { LIST_FN_SIG_S_B_B_P };

/* Build debug table to help development with this "robust" macro stuff */

#if defined(CONFIG_NBLE_DEBUG_RPC)

#undef FN_SIG_NONE
#undef FN_SIG_S
#undef FN_SIG_P
#undef FN_SIG_S_B
#undef FN_SIG_B_B_P
#undef FN_SIG_S_P
#undef FN_SIG_S_B_P
#undef FN_SIG_S_B_B_P

#define FN_SIG_NONE(__fn)			#__fn,
#define FN_SIG_S(__fn, __s)			FN_SIG_NONE(__fn)
#define FN_SIG_P(__fn, __type)			FN_SIG_NONE(__fn)
#define FN_SIG_S_B(__fn, __s, __type, __length)				\
						FN_SIG_NONE(__fn)
#define FN_SIG_B_B_P(__fn, __type1, __length1, __type2, __length2,	\
		     __type3)			FN_SIG_NONE(__fn)
#define FN_SIG_S_P(__fn, __s, __type)		FN_SIG_NONE(__fn)
#define FN_SIG_S_B_P(__fn, __s, __type, __length, __type_ptr)		\
						FN_SIG_NONE(__fn)
#define FN_SIG_S_B_B_P(__fn, __s, __type1, __length1, __type2,		\
		       __length2, __type3)	FN_SIG_NONE(__fn)

static char *debug_func_none[] = { LIST_FN_SIG_NONE };
static char *debug_func_s[] = { LIST_FN_SIG_S };
static char *debug_func_p[] = { LIST_FN_SIG_P };
static char *debug_func_s_b[] = { LIST_FN_SIG_S_B };
static char *debug_func_b_b_p[] = { LIST_FN_SIG_B_B_P };
static char *debug_func_s_p[] = { LIST_FN_SIG_S_P };
static char *debug_func_s_b_p[] = { LIST_FN_SIG_S_B_P };
static char *debug_func_s_b_b_p[] = { LIST_FN_SIG_S_B_B_P};

#endif

#undef FN_SIG_NONE
#undef FN_SIG_S
#undef FN_SIG_P
#undef FN_SIG_S_B
#undef FN_SIG_B_B_P
#undef FN_SIG_S_P
#undef FN_SIG_S_B_P
#undef FN_SIG_S_B_B_P

#define DJB2_HASH(__h, __v) ((((__h) << 5) + (__h)) + (__v))

#define FN_SIG_NONE(__fn)						\
		hash = DJB2_HASH(hash, 1);

#define FN_SIG_S(__fn, __s)						\
	do {								\
		hash = DJB2_HASH(hash, 2);				\
		hash = DJB2_HASH(hash, sizeof(*((__s)0)));		\
	} while (0);

#define FN_SIG_P(__fn, __type)						\
		hash = DJB2_HASH(hash, 3);

#define FN_SIG_S_B(__fn, __s, __type, __length)				\
	do {								\
		hash = DJB2_HASH(hash, 4);				\
		hash = DJB2_HASH(hash, sizeof(*((__s)0)));		\
	} while (0);

#define FN_SIG_B_B_P(__fn, __type1, __length1, __type2, __length2,	\
		     __type3)						\
	do {								\
		hash = DJB2_HASH(hash, 5);				\
		hash = DJB2_HASH(hash, sizeof(*((__s)0)));		\
	} while (0);

#define FN_SIG_S_P(__fn, __s, __type)					\
	do {								\
		hash = DJB2_HASH(hash, 6);				\
		hash = DJB2_HASH(hash, sizeof(*((__s)0)));		\
	} while (0);

#define FN_SIG_S_B_P(__fn, __s, __type, __length, __type_ptr)		\
	do {								\
		hash = DJB2_HASH(hash, 7);				\
		hash = DJB2_HASH(hash, sizeof(*((__s)0)));		\
	} while (0);

#define FN_SIG_S_B_B_P(__fn, __s, __type1, __length1, __type2,		\
		       __length2, __type3)				\
	do {								\
		hash = DJB2_HASH(hash, 8);				\
		hash = DJB2_HASH(hash, sizeof(*((__s)0)));		\
	} while (0);

uint32_t rpc_deserialize_hash(void)
{
	uint32_t hash = 5381;

	LIST_FN_SIG_NONE;
	LIST_FN_SIG_S;
	LIST_FN_SIG_P;
	LIST_FN_SIG_S_B;
	LIST_FN_SIG_B_B_P;
	LIST_FN_SIG_S_P;
	LIST_FN_SIG_S_B_P;
	LIST_FN_SIG_S_B_B_P;

	return hash;
}

static void panic(int err)
{
	PRINTK("panic: errcode %d", err);

	while (true) {
	}
}

static void deserialize_struct(struct net_buf *buf, const uint8_t **struct_ptr,
			       uint8_t *struct_length)
{
	*struct_length = net_buf_pull_u8(buf);
	*struct_ptr = buf->data;
	net_buf_pull(buf, *struct_length);
}

static void deserialize_buf(struct net_buf *buf, const uint8_t **buf_ptr,
			    uint16_t *buf_len)
{
	uint8_t b;

	/* Get the current byte */
	b = net_buf_pull_u8(buf);
	*buf_len = b & 0x7F;
	if (b & 0x80) {
		/* Get the current byte */
		b = net_buf_pull_u8(buf);
		*buf_len += (uint16_t)b << 7;
	}

	/* Return the values */
	*buf_ptr = buf->data;

	net_buf_pull(buf, *buf_len);
}

static void deserialize_ptr(struct net_buf *buf, uintptr_t *priv)
{
	memcpy(priv, buf->data, sizeof(*priv));
	net_buf_pull(buf, sizeof(*priv));
}

static void deserialize_none(uint8_t fn_index, struct net_buf *buf)
{
	(void)buf;

	if (buf->len != 0) {
		panic(-1);
	}

	m_fct_none[fn_index]();
}

static void deserialize_s(uint8_t fn_index, struct net_buf *buf)
{
	const uint8_t *struct_ptr;
	uint8_t struct_length;

	deserialize_struct(buf, &struct_ptr, &struct_length);

	if (struct_length != m_size_s[fn_index]) {
		panic(-1);
	} else {
		/* Always align structures on word boundary */
		uintptr_t struct_data[(struct_length +
				(sizeof(uintptr_t) - 1))/(sizeof(uintptr_t))];

		memcpy(struct_data, struct_ptr, struct_length);

		m_fct_s[fn_index](struct_data);
	}
}

static void deserialize_p(uint8_t fn_index, struct net_buf *buf)
{
	uintptr_t priv;

	if (buf->len != sizeof(priv)) {
		panic(-1);
	}

	deserialize_ptr(buf, &priv);

	m_fct_p[fn_index]((void *)priv);
}

static void deserialize_s_b(uint8_t fn_index, struct net_buf *buf)
{
	const uint8_t *p_struct_data;
	uint8_t struct_length;
	const uint8_t *p_vbuf;
	uint16_t vbuf_length;

	deserialize_struct(buf, &p_struct_data, &struct_length);
	deserialize_buf(buf, &p_vbuf, &vbuf_length);

	if (struct_length != m_size_s_b[fn_index]) {
		panic(-1);
	} else {
		/* Always align structures on word boundary */
		uintptr_t struct_data[(struct_length +
				(sizeof(uintptr_t) - 1))/(sizeof(uintptr_t))];
		uintptr_t vbuf[(vbuf_length +
				(sizeof(uintptr_t) - 1))/(sizeof(uintptr_t))];
		void *buf = NULL;

		memcpy(struct_data, p_struct_data, struct_length);

		if (vbuf_length) {
			memcpy(vbuf, p_vbuf, vbuf_length);
			buf = vbuf;
		}

		m_fct_s_b[fn_index](struct_data, buf, vbuf_length);
	}
}

static void deserialize_b_b_p(uint8_t fn_index, struct net_buf *buf)
{
	const uint8_t *p_vbuf1;
	uint16_t vbuf1_length;
	const uint8_t *p_vbuf2;
	uint16_t vbuf2_length;
	uintptr_t priv;

	deserialize_buf(buf, &p_vbuf1, &vbuf1_length);
	deserialize_buf(buf, &p_vbuf2, &vbuf2_length);
	deserialize_ptr(buf, &priv);

	{
		/* Always align structures on word boundary */
		uintptr_t vbuf1[(vbuf1_length +
				(sizeof(uintptr_t) - 1))/(sizeof(uintptr_t))];
		uintptr_t vbuf2[(vbuf2_length +
				(sizeof(uintptr_t) - 1))/(sizeof(uintptr_t))];
		void *buf1 = NULL;
		void *buf2 = NULL;

		if (vbuf1_length) {
			memcpy(vbuf1, p_vbuf1, vbuf1_length);
			buf1 = vbuf1;
		}

		if (vbuf2_length) {
			memcpy(vbuf2, p_vbuf2, vbuf2_length);
			buf2 = vbuf2;
		}

		m_fct_b_b_p[fn_index](buf1, vbuf1_length, buf2, vbuf2_length,
				      (void *)priv);
	}
}

static void deserialize_s_p(uint8_t fn_index, struct net_buf *buf)
{
	const uint8_t *p_struct_data;
	uint8_t struct_length;
	uintptr_t priv;

	deserialize_struct(buf, &p_struct_data, &struct_length);
	deserialize_ptr(buf, &priv);

	if (struct_length != m_size_s_p[fn_index]) {
		panic(-1);
	} else {
		/* Always align structures on word boundary */
		uintptr_t struct_data[(struct_length +
				(sizeof(uintptr_t) - 1))/(sizeof(uintptr_t))];

		memcpy(struct_data, p_struct_data, struct_length);

		m_fct_s_p[fn_index](struct_data, (void *)priv);
	}
}

static void deserialize_s_b_p(uint8_t fn_index, struct net_buf *buf)
{
	const uint8_t *p_struct_data;
	uint8_t struct_length;
	const uint8_t *p_vbuf;
	uint16_t vbuf_length;
	uintptr_t priv;

	deserialize_struct(buf, &p_struct_data, &struct_length);
	deserialize_buf(buf, &p_vbuf, &vbuf_length);
	deserialize_ptr(buf, &priv);

	if (struct_length != m_size_s_b_p[fn_index]) {
		panic(-1);
	} else {
		/* Always align structures on word boundary */
		uintptr_t struct_data[(struct_length +
				(sizeof(uintptr_t) - 1))/(sizeof(uintptr_t))];
		uintptr_t vbuf[(vbuf_length +
				(sizeof(uintptr_t) - 1))/(sizeof(uintptr_t))];
		void *buf = NULL;

		memcpy(struct_data, p_struct_data, struct_length);

		if (vbuf_length) {
			memcpy(vbuf, p_vbuf, vbuf_length);
			buf = vbuf;
		}

		m_fct_s_b_p[fn_index](struct_data, buf, vbuf_length,
				      (void *)priv);
	}
}

static void deserialize_s_b_b_p(uint8_t fn_index, struct net_buf *buf)
{
	const uint8_t *p_struct_data;
	uint8_t struct_length;
	const uint8_t *p_vbuf1;
	uint16_t vbuf1_length;
	const uint8_t *p_vbuf2;
	uint16_t vbuf2_length;
	uintptr_t priv;

	deserialize_struct(buf, &p_struct_data, &struct_length);
	deserialize_buf(buf, &p_vbuf1, &vbuf1_length);
	deserialize_buf(buf, &p_vbuf2, &vbuf2_length);
	deserialize_ptr(buf, &priv);

	if (struct_length != m_size_s_b_b_p[fn_index]) {
		panic(-1);
	} else {
		/* Always align structures on word boundary */
		uintptr_t struct_data[(struct_length +
				(sizeof(uintptr_t) - 1))/(sizeof(uintptr_t))];
		uintptr_t vbuf1[(vbuf1_length +
				(sizeof(uintptr_t) - 1))/(sizeof(uintptr_t))];
		uintptr_t vbuf2[(vbuf2_length +
				(sizeof(uintptr_t) - 1))/(sizeof(uintptr_t))];
		void *buf1 = NULL;
		void *buf2 = NULL;

		memcpy(struct_data, p_struct_data, struct_length);

		if (vbuf1_length) {
			memcpy(vbuf1, p_vbuf1, vbuf1_length);
			buf1 = vbuf1;
		}

		if (vbuf2_length) {
			memcpy(vbuf2, p_vbuf2, vbuf2_length);
			buf2 = vbuf2;
		}

		m_fct_s_b_b_p[fn_index](struct_data, buf1, vbuf1_length, buf2,
					vbuf2_length, (void *)priv);
	}
}

static void deserialize_control(uint8_t fn_index, struct net_buf *buf)
{
	const uint8_t *p_struct_data;
	uint8_t struct_length;
	struct {
		uint32_t version;
		uint32_t ser_hash;
		uint32_t des_hash;
	} struct_data;

	switch (fn_index) {
	case 0:
		deserialize_struct(buf, &p_struct_data, &struct_length);

		if (struct_length != sizeof(struct_data))
			panic(-1);
		memcpy(&struct_data, p_struct_data, struct_length);
		if (struct_data.ser_hash != rpc_deserialize_hash() ||
		    struct_data.des_hash != rpc_serialize_hash()) {
			rpc_init_cb(struct_data.version, false);
		} else {
			rpc_init_cb(struct_data.version, true);
		}
		break;
	break;
		panic(-1);
	}
}

void rpc_deserialize(struct net_buf *buf)
{

	uint8_t fn_index;
	uint8_t sig_type;

	sig_type = buf->data[0];
	fn_index = buf->data[1];

	net_buf_pull(buf, 2);

	switch (sig_type) {
	case SIG_TYPE_NONE:
		if (fn_index < ARRAY_SIZE(m_fct_none)) {
			BT_DBG("%s", debug_func_none[fn_index]);
			deserialize_none(fn_index, buf);
		}
		break;
	case SIG_TYPE_S:
		if (fn_index < ARRAY_SIZE(m_fct_s)) {
			BT_DBG("%s", debug_func_s[fn_index]);
			deserialize_s(fn_index, buf);
		}
		break;
	case SIG_TYPE_P:
		if (fn_index < ARRAY_SIZE(m_fct_p)) {
			BT_DBG("%s", debug_func_p[fn_index]);
			deserialize_p(fn_index, buf);
		}
		break;
	case SIG_TYPE_S_B:
		if (fn_index < ARRAY_SIZE(m_fct_s_b)) {
			BT_DBG("%s", debug_func_s_b[fn_index]);
			deserialize_s_b(fn_index, buf);
		}
		break;
	case SIG_TYPE_B_B_P:
		if (fn_index < ARRAY_SIZE(m_fct_b_b_p)) {
			BT_DBG("%s", debug_func_b_b_p[fn_index]);
			deserialize_b_b_p(fn_index, buf);
		}
		break;
	case SIG_TYPE_S_P:
		if (fn_index < ARRAY_SIZE(m_fct_s_p)) {
			BT_DBG("%s", debug_func_s_p[fn_index]);
			deserialize_s_p(fn_index, buf);
		}
		break;
	case SIG_TYPE_S_B_P:
		if (fn_index < ARRAY_SIZE(m_fct_s_b_p)) {
			BT_DBG("%s", debug_func_s_b_p[fn_index]);
			deserialize_s_b_p(fn_index, buf);
		}
		break;
	case SIG_TYPE_S_B_B_P:
		if (fn_index < ARRAY_SIZE(m_fct_s_b_b_p)) {
			BT_DBG("%s", debug_func_s_b_b_p[fn_index]);
			deserialize_s_b_b_p(fn_index, buf);
		}
		break;
	case SIG_TYPE_CONTROL:
		deserialize_control(fn_index, buf);
		break;
	default:
		panic(-1);
		break;
	}
}

__weak
void rpc_init_cb(uint32_t version, bool compatible)
{
}
