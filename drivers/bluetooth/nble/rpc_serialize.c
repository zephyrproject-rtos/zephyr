/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <atomic.h>

#include <bluetooth/gatt.h>
#include <bluetooth/conn.h>

#include "rpc.h"
#include "gap_internal.h"
#include "gatt_internal.h"

#include "rpc_functions_to_ble_core.h"

/* Build the functions exposed */
/* Define the functions identifiers per signature */
#define FN_SIG_NONE(__fn)				fn_index_##__fn,
#define FN_SIG_S(__fn, __s)				FN_SIG_NONE(__fn)
#define FN_SIG_P(__fn, __type)				FN_SIG_NONE(__fn)
#define FN_SIG_S_B(__fn, __s, __type, __length)		FN_SIG_NONE(__fn)
#define FN_SIG_B_B_P(__fn, __type1, __length1, __type2, __length2, __type3) \
							FN_SIG_NONE(__fn)
#define FN_SIG_S_P(__fn, __s, __type)			FN_SIG_NONE(__fn)
#define FN_SIG_S_B_P(__fn, __s, __type, __length, __type_ptr) \
							FN_SIG_NONE(__fn)
#define FN_SIG_S_B_B_P(__fn, __s, __type1, __length1, __type2, __length2, \
		       __type3)				FN_SIG_NONE(__fn)

/* Build the list of function indexes -> this should match the array at
 * deserialization
 */
enum { LIST_FN_SIG_NONE fn_none_index_max };
enum { LIST_FN_SIG_S fn_s_index_max };
enum { LIST_FN_SIG_P fn_p_index_max };
enum { LIST_FN_SIG_S_B fn_s_b_index_max };
enum { LIST_FN_SIG_B_B_P fn_b_b_p_index_max };
enum { LIST_FN_SIG_S_P fn_s_p_index_max };
enum { LIST_FN_SIG_S_B_P fn_s_b_p_index_max };
enum { LIST_FN_SIG_S_B_B_P fn_s_b_b_p_index_max };

/* Implement the functions using serialization API */
#undef FN_SIG_NONE
#undef FN_SIG_S
#undef FN_SIG_P
#undef FN_SIG_S_B
#undef FN_SIG_B_B_P
#undef FN_SIG_S_P
#undef FN_SIG_S_B_P
#undef FN_SIG_S_B_B_P

#define FN_SIG_NONE(__fn)						\
	void __fn(void)							\
	{								\
		rpc_serialize_none(fn_index_##__fn);			\
	}

#define FN_SIG_S(__fn, __s)						\
	void __fn(__s p_s)						\
	{								\
		rpc_serialize_s(fn_index_##__fn, p_s, sizeof(*p_s));	\
	}

#define FN_SIG_P(__fn, __type)						\
	void __fn(__type p_priv)					\
	{								\
		rpc_serialize_p(fn_index_##__fn, p_priv);		\
	}

#define FN_SIG_S_B(__fn, __s, __type, __length)				\
	void __fn(__s p_s, __type p_buf, __length length)		\
	{								\
		rpc_serialize_s_b(fn_index_##__fn, p_s, sizeof(*p_s),	\
				  p_buf, length);			\
	}

#define FN_SIG_B_B_P(__fn, __type1, __length1, __type2, __length2,	\
		     __type3)						\
	void __fn(__type1 p_buf1, __length1 length1, __type2 p_buf2,	\
		  __length2 length2, __type3 p_priv)			\
	{								\
		rpc_serialize_b_b_p(fn_index_##__fn, p_buf1, length1,	\
				    p_buf2, length2, p_priv);		\
	}

#define FN_SIG_S_P(__fn, __s, __type)					\
	void __fn(__s p_s, __type p_priv)				\
	{								\
		rpc_serialize_s_p(fn_index_##__fn, p_s, sizeof(*p_s),	\
				  p_priv);				\
	}

#define FN_SIG_S_B_P(__fn, __s, __type, __length, __type_ptr)		\
	void __fn(__s p_s, __type p_buf, __length length,		\
		  __type_ptr p_priv)					\
	{								\
		rpc_serialize_s_b_p(fn_index_##__fn, p_s, sizeof(*p_s),	\
				    p_buf, length, p_priv);		\
	}

#define FN_SIG_S_B_B_P(__fn, __s, __type1, __length1, __type2,		\
		       __length2, __type3)				\
	void __fn(__s p_s, __type1 p_buf1, __length1 length1,		\
		  __type2 p_buf2, __length2 length2, __type3 p_priv)	\
	{								\
		rpc_serialize_s_b_b_p(fn_index_##__fn, p_s,		\
				      sizeof(*p_s), p_buf1, length1,	\
				      p_buf2, length2, p_priv);		\
	}


/* Build the functions */
LIST_FN_SIG_NONE
LIST_FN_SIG_S
LIST_FN_SIG_P
LIST_FN_SIG_S_B
LIST_FN_SIG_B_B_P
LIST_FN_SIG_S_P
LIST_FN_SIG_S_B_P
LIST_FN_SIG_S_B_B_P

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

uint32_t rpc_serialize_hash(void)
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

#define SIG_TYPE_SIZE		1
#define FN_INDEX_SIZE		1
#define POINTER_SIZE		4

static void _send(struct net_buf *buf)
{
	rpc_transmit_cb(buf);
}

static uint16_t encoded_structlen(uint8_t structlen)
{
	return 1 + structlen;
}

static void serialize_struct(struct net_buf *buf, const uint8_t *struct_data,
			     uint8_t struct_length)
{
	net_buf_add_u8(buf, struct_length);
	net_buf_add_mem(buf, struct_data, struct_length);
}

static uint16_t encoded_buflen(const uint8_t *buf, uint16_t buflen)
{
	if (!buf) {
		return 1;
	}

	if (buflen < (1 << 7)) {
		return 1 + buflen;
	} else {
		return 2 + buflen;
	}
}

static void serialize_buf(struct net_buf *buf, const uint8_t *data,
			  uint16_t len)
{
	uint16_t varint;
	uint8_t *p;

	if (!data) {
		len = 0;
	}

	varint = len;

	p = net_buf_add_u8(buf, (varint & 0x7f));
	if (varint >= (1 << 7)) {
		*p |= 0x80;
		net_buf_add_u8(buf, (varint >> 7));
	}

	net_buf_add_mem(buf, data, len);
}

static void serialize_p(struct net_buf *buf, void *ptr)
{
	uintptr_t val = (uintptr_t)ptr;

	net_buf_add_mem(buf, &val, sizeof(val));
}

void rpc_serialize_none(uint8_t fn_index)
{
	struct net_buf *buf;

	buf = rpc_alloc_cb(SIG_TYPE_SIZE + FN_INDEX_SIZE);

	net_buf_add_u8(buf, SIG_TYPE_NONE);
	net_buf_add_u8(buf, fn_index);

	_send(buf);
}

void rpc_serialize_s(uint8_t fn_index, const void *struct_data,
		     uint8_t struct_length)
{
	struct net_buf *buf;

	buf = rpc_alloc_cb(SIG_TYPE_SIZE + FN_INDEX_SIZE +
			   encoded_structlen(struct_length));

	net_buf_add_u8(buf, SIG_TYPE_S);
	net_buf_add_u8(buf, fn_index);

	serialize_struct(buf, struct_data, struct_length);

	_send(buf);
}

void rpc_serialize_p(uint8_t fn_index, void *priv)
{
	struct net_buf *buf;

	buf = rpc_alloc_cb(SIG_TYPE_SIZE + FN_INDEX_SIZE + POINTER_SIZE);

	net_buf_add_u8(buf, SIG_TYPE_P);
	net_buf_add_u8(buf, fn_index);
	serialize_p(buf, priv);

	_send(buf);
}

void rpc_serialize_s_b(uint8_t fn_index, const void *struct_data,
		       uint8_t struct_length, const void *vbuf,
		       uint16_t vbuf_length)
{
	struct net_buf *buf;

	buf = rpc_alloc_cb(SIG_TYPE_SIZE + FN_INDEX_SIZE +
			   encoded_structlen(struct_length) +
			   encoded_buflen(vbuf, vbuf_length));

	net_buf_add_u8(buf, SIG_TYPE_S_B);
	net_buf_add_u8(buf, fn_index);

	serialize_struct(buf, struct_data, struct_length);
	serialize_buf(buf, vbuf, vbuf_length);

	_send(buf);
}

void rpc_serialize_b_b_p(uint8_t fn_index, const void *vbuf1,
			 uint16_t vbuf1_length, const void *vbuf2,
			 uint16_t vbuf2_length, void *priv)
{
	struct net_buf *buf;

	buf = rpc_alloc_cb(SIG_TYPE_SIZE + FN_INDEX_SIZE +
			   encoded_buflen(vbuf1, vbuf1_length) +
			   encoded_buflen(vbuf2, vbuf2_length) + POINTER_SIZE);

	net_buf_add_u8(buf, SIG_TYPE_B_B_P);
	net_buf_add_u8(buf, fn_index);

	serialize_buf(buf, vbuf1, vbuf1_length);
	serialize_buf(buf, vbuf2, vbuf2_length);
	serialize_p(buf, priv);

	_send(buf);
}

void rpc_serialize_s_p(uint8_t fn_index, const void *struct_data,
		       uint8_t struct_length, void *priv)
{
	struct net_buf *buf;

	buf = rpc_alloc_cb(SIG_TYPE_SIZE + FN_INDEX_SIZE +
			   encoded_structlen(struct_length) + POINTER_SIZE);

	net_buf_add_u8(buf, SIG_TYPE_S_P);
	net_buf_add_u8(buf, fn_index);

	serialize_struct(buf, struct_data, struct_length);
	serialize_p(buf, priv);

	_send(buf);
}

void rpc_serialize_s_b_p(uint8_t fn_index, const void *struct_data,
			 uint8_t struct_length, const void *vbuf,
			 uint16_t vbuf_length, void *priv)
{
	struct net_buf *buf;

	buf = rpc_alloc_cb(SIG_TYPE_SIZE + FN_INDEX_SIZE +
			   encoded_structlen(struct_length) +
			   encoded_buflen(vbuf, vbuf_length) + POINTER_SIZE);

	net_buf_add_u8(buf, SIG_TYPE_S_B_P);
	net_buf_add_u8(buf, fn_index);

	serialize_struct(buf, struct_data, struct_length);
	serialize_buf(buf, vbuf, vbuf_length);
	serialize_p(buf, priv);

	_send(buf);
}

void rpc_serialize_s_b_b_p(uint8_t fn_index, const void *struct_data,
			   uint8_t struct_length, const void *vbuf1,
			   uint16_t vbuf1_length, const void *vbuf2,
			   uint16_t vbuf2_length, void *priv)
{
	struct net_buf *buf;

	buf = rpc_alloc_cb(SIG_TYPE_SIZE + FN_INDEX_SIZE +
			   encoded_structlen(struct_length) +
			   encoded_buflen(vbuf1, vbuf1_length) +
			   encoded_buflen(vbuf2, vbuf2_length) + POINTER_SIZE);

	net_buf_add_u8(buf, SIG_TYPE_S_B_B_P);
	net_buf_add_u8(buf, fn_index);

	serialize_struct(buf, struct_data, struct_length);
	serialize_buf(buf, vbuf1, vbuf1_length);
	serialize_buf(buf, vbuf2, vbuf2_length);
	serialize_p(buf, priv);

	_send(buf);
}

void rpc_init(uint32_t version)
{
	struct net_buf *buf;
	struct {
		uint32_t version;
		uint32_t ser_hash;
		uint32_t des_hash;
	} struct_data;

	struct_data.version = version;
	struct_data.ser_hash = rpc_serialize_hash();
	struct_data.des_hash = rpc_deserialize_hash();
	buf = rpc_alloc_cb(SIG_TYPE_SIZE + FN_INDEX_SIZE +
			   encoded_structlen(sizeof(struct_data)));

	net_buf_add_u8(buf, SIG_TYPE_CONTROL);
	net_buf_add_u8(buf, 0);
	serialize_struct(buf, (uint8_t *)&struct_data, sizeof(struct_data));
	_send(buf);
}
