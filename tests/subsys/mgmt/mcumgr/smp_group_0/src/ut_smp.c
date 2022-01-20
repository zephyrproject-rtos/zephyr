/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <sys/byteorder.h>
#include <net/buf.h>
#include <mgmt/mcumgr/smp.h>
#include <mgmt/mcumgr/buf.h>
#include <mgmt/mgmt.h>
#include "os_mgmt/os_mgmt.h"
#include "../../../../../../subsys/mgmt/mcumgr/lib/smp/include/smp/smp.h"

typedef int (*mcumgr_rsp_callback)(void);

/* CBOR encoder/decoder streams that are used by CBOR lib to write/read data */
extern struct cbor_decoder_reader ut_cbor_reader;
extern struct cbor_encoder_writer ut_cbor_writer;

/* Request/response buffers */
extern struct net_buf_simple ut_mcumgr_req_buf;
extern struct net_buf_simple ut_mcumgr_rsp_buf;

/* Provided by ut_cbor_stream.c */
void ut_cbor_reader_init(void);
void ut_cbor_writer_init(void);
void ut_cbor_writer_init(void);
void ut_cbor_reader_trim_front(size_t len);
void *ut_cbor_writer_alloc_rsp(void);
void ut_cbor_reader_free_buf(void *buf);
void ut_cbor_writer_reset_buf(struct net_buf_simple *p);
int ut_cbor_writer_write_at(size_t off, const void *p, size_t len);

static void *ut_smp_alloc_rsp(const void *rsp, void *arg)
{
	return ut_cbor_writer_alloc_rsp();
}

static void ut_smp_free_buf(void *buf, void *arg)
{
	ut_cbor_reader_free_buf(buf);
}

static void ut_smp_reader_trim_front(void *buf, size_t len, void *arg)
{
	ut_cbor_reader_trim_front(len);
}

static void ut_smp_writer_reset_buf(void *buf, void *arg)
{
	ut_cbor_writer_reset_buf(buf);
}

static int ut_smp_writer_write_at(struct cbor_encoder_writer *writer,
		size_t offset, const void *data, size_t len, void *arg)
{
	/* The call will return error or number of bytes in output, or rather
	 * index of lst byte in output from the beginning of buffer.
	 */
	ut_cbor_writer_write_at(offset, data, len);
	writer->bytes_written = ut_mcumgr_rsp_buf.len;
	return 0;
}

static int
ut_smp_reader_init(struct cbor_decoder_reader *r, void *buf, void *arg)
{
	ut_cbor_reader_init();

	return 0;
}

static int
ut_smp_writer_init(struct cbor_encoder_writer *w, void *buf, void *arg)
{
	ut_cbor_writer_init();

	return 0;
}

static const struct mgmt_streamer_cfg ut_smp_mgmt_streamer_cfg = {
		.alloc_rsp = ut_smp_alloc_rsp,
		.trim_front = ut_smp_reader_trim_front,
		.reset_buf = ut_smp_writer_reset_buf,
		.write_at = ut_smp_writer_write_at,
		.init_reader = ut_smp_reader_init,
		.init_writer = ut_smp_writer_init,
		.free_buf = ut_smp_free_buf,
};

static int ut_smp_rsp_callback(struct smp_streamer *ns, void *rsp, void *arg)
{
	mcumgr_rsp_callback rsp_cb = (mcumgr_rsp_callback)arg;

	printk("Making callback\n");
	return rsp_cb();
}

int ut_smp_req_to_mcumgr(struct net_buf_simple *nb, mcumgr_rsp_callback rsp_cb)
{
	struct smp_streamer streamer;
	int rc;

	streamer = (struct smp_streamer) {
		.mgmt_stmr = {
			.cfg = &ut_smp_mgmt_streamer_cfg,
			.reader = &ut_cbor_reader,
			.writer = &ut_cbor_writer,
			.cb_arg = rsp_cb,
		},
		.tx_rsp_cb = ut_smp_rsp_callback,
	};

	rc = smp_process_request_packet(&streamer, nb);
	return rc;
}
