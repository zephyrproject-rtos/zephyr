/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <string.h>
#include <sys/util.h>
#include <sys/byteorder.h>
#include <net/buf.h>
#include "mgmt/mcumgr/buf.h"

extern struct net_buf_simple ut_mcumgr_req_buf;
extern struct net_buf_simple ut_mcumgr_rsp_buf;
NET_BUF_SIMPLE_DEFINE(ut_mcumgr_req_buf, CONFIG_MCUMGR_BUF_SIZE);
NET_BUF_SIMPLE_DEFINE(ut_mcumgr_rsp_buf, CONFIG_MCUMGR_BUF_SIZE);

struct cbor_decoder_reader ut_cbor_reader;
struct cbor_encoder_writer ut_cbor_writer;

static uint8_t
ut_cbor_reader_get8(struct cbor_decoder_reader *d, int offset)
{
	__ASSERT(offset >= 0, "Offset can not be negative");
	return ut_mcumgr_req_buf.data[offset];
}

static uint16_t
ut_cbor_reader_get16(struct cbor_decoder_reader *d, int offset)
{
	return sys_get_be16(&ut_mcumgr_req_buf.data[offset]);
}

static uint32_t
ut_cbor_reader_get32(struct cbor_decoder_reader *d, int offset)
{
	return sys_get_be32(&ut_mcumgr_req_buf.data[offset]);
}

static uint64_t
ut_cbor_reader_get64(struct cbor_decoder_reader *d, int offset)
{
	return sys_get_be64(&ut_mcumgr_req_buf.data[offset]);
}

static uintptr_t
ut_cbor_reader_cmp(struct cbor_decoder_reader *d, char *buf, int offset,
		   size_t len)
{
	return memcmp(&ut_mcumgr_req_buf.data[offset], buf, len);
}

static uintptr_t
ut_cbor_reader_cpy(struct cbor_decoder_reader *d, char *dst, int offset,
		   size_t len)
{
	return (uintptr_t)memcpy(dst, &ut_mcumgr_req_buf.data[offset], len);
}

void ut_cbor_reader_init(void)
{
	ut_cbor_reader.get8 = &ut_cbor_reader_get8;
	ut_cbor_reader.get16 = &ut_cbor_reader_get16;
	ut_cbor_reader.get32 = &ut_cbor_reader_get32;
	ut_cbor_reader.get64 = &ut_cbor_reader_get64;
	ut_cbor_reader.cmp = &ut_cbor_reader_cmp;
	ut_cbor_reader.cpy = &ut_cbor_reader_cpy;

	ut_cbor_reader.message_size = ut_mcumgr_req_buf.len;
}

static int ut_cbor_write(struct cbor_encoder_writer *cew, const char *data, int len)
{
	ARG_UNUSED(cew);
	__ASSERT(net_buf_simple_tailroom(&ut_mcumgr_rsp_buf) >= len,
		 "Not enough space in ut_mcumgr_rsp_buf for %d len bytes", len);

	net_buf_simple_add_mem(&ut_mcumgr_rsp_buf, data, len);
	ut_cbor_writer.bytes_written += len;

	return CborNoError;
}

void ut_cbor_writer_init(void)
{
	ut_cbor_writer.bytes_written = 0;
	ut_cbor_writer.write = &ut_cbor_write;
}

void ut_cbor_reader_trim_front(size_t len)
{
	net_buf_simple_pull(&ut_mcumgr_req_buf, len);
}

void *ut_cbor_writer_alloc_rsp(void)
{
	return &ut_mcumgr_rsp_buf;
}

void ut_cbor_reader_free_buf(void *p)
{
	/* Note: free should probably always receive non-NULL buffer, there is no reason
	 * for logic that would possibly call free in case where it fails allocation,
	 * as it could be avoided.
	 */
	if (p != NULL) {
		net_buf_simple_reset(p);
	} else {
		TC_PRINT("??: ut_cbor_writer_alloc_rsp called for NULL\n");
	}
}

void ut_cbor_writer_reset_buf(struct net_buf_simple *p)
{
	ARG_UNUSED(p);
	net_buf_simple_reset(&ut_mcumgr_rsp_buf);
}

/* Returns total number of bytes rsp buffer */
int ut_cbor_writer_write_at(size_t off, const void *p, size_t len)
{
	/* Hidden logic: if write would cross ut_mcumgr_rsp_buf.len then the len will be modified
	 * to have value off + len of written data.
	 */
	__ASSERT(off <= ut_mcumgr_rsp_buf.len, "off(%d) < ut_mcumgr_rsp_buf.len(%d)", off, len);
	__ASSERT((off + len) <=
		   (ut_mcumgr_rsp_buf.size - net_buf_simple_headroom(&ut_mcumgr_rsp_buf)),
		 "off(%d) + len(%d) will not fit into buffer (ut_mcumgr_rsp_buf.size(%d) - "
		  "net_buf_simple_headroom(&ut_mcumgr_rsp_buf)(%d))", off, len,
		   ut_mcumgr_rsp_buf.size, net_buf_simple_headroom(&ut_mcumgr_rsp_buf));

	memcpy(&(ut_mcumgr_rsp_buf.data[off]), p, len);
	if (ut_mcumgr_rsp_buf.len < off + len) {
		ut_mcumgr_rsp_buf.len = off + len;
	}

	return 0;
}
