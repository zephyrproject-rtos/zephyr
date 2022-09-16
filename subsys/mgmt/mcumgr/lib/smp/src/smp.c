/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** SMP - Simple Management Protocol. */

#include <assert.h>
#include <string.h>

#include <zephyr/net/buf.h>
#include <zephyr/mgmt/mcumgr/buf.h>
#include <zephyr/mgmt/mcumgr/smp.h>
#include "mgmt/mgmt.h"
#include <zcbor_common.h>
#include <zcbor_encode.h>
#include "smp/smp.h"
#include "../../../smp_internal.h"

/**
 * Converts a request opcode to its corresponding response opcode.
 */
static uint8_t
smp_rsp_op(uint8_t req_op)
{
	if (req_op == MGMT_OP_READ) {
		return MGMT_OP_READ_RSP;
	} else {
		return MGMT_OP_WRITE_RSP;
	}
}

static void
smp_make_rsp_hdr(const struct mgmt_hdr *req_hdr, struct mgmt_hdr *rsp_hdr, size_t len)
{
	*rsp_hdr = (struct mgmt_hdr) {
		.nh_len = len,
		.nh_flags = 0,
		.nh_op = smp_rsp_op(req_hdr->nh_op),
		.nh_group = req_hdr->nh_group,
		.nh_seq = req_hdr->nh_seq,
		.nh_id = req_hdr->nh_id,
	};
	mgmt_hton_hdr(rsp_hdr);
}

static int
smp_read_hdr(const struct net_buf *nb, struct mgmt_hdr *dst_hdr)
{
	if (nb->len < sizeof(*dst_hdr)) {
		return MGMT_ERR_EINVAL;
	}

	memcpy(dst_hdr, nb->data, sizeof(*dst_hdr));
	return 0;
}

static inline int
smp_write_hdr(struct smp_streamer *streamer, const struct mgmt_hdr *src_hdr)
{
	memcpy(streamer->writer->nb->data, src_hdr, sizeof(*src_hdr));
	return 0;
}

static int
smp_build_err_rsp(struct smp_streamer *streamer, const struct mgmt_hdr *req_hdr, int status,
		  const char *rc_rsn)
{
	struct mgmt_hdr rsp_hdr;
	struct cbor_nb_writer *nbw = streamer->writer;
	zcbor_state_t *zsp = nbw->zs;
	bool ok;

	ok = zcbor_map_start_encode(zsp, 2)		&&
	     zcbor_tstr_put_lit(zsp, "rc")		&&
	     zcbor_int32_put(zsp, status);

#ifdef CONFIG_MGMT_VERBOSE_ERR_RESPONSE
	if (ok && rc_rsn != NULL) {
		ok = zcbor_tstr_put_lit(zsp, "rsn")			&&
		     zcbor_tstr_put_term(zsp, rc_rsn);
	}
#else
	ARG_UNUSED(rc_rsn);
#endif
	ok &= zcbor_map_end_encode(zsp, 2);

	if (!ok) {
		return MGMT_ERR_EMSGSIZE;
	}

	smp_make_rsp_hdr(req_hdr, &rsp_hdr,
			 zsp->payload_mut - nbw->nb->data - MGMT_HDR_SIZE);
	nbw->nb->len = zsp->payload_mut - nbw->nb->data;
	smp_write_hdr(streamer, &rsp_hdr);

	return 0;
}

/**
 * Processes a single SMP request and generates a response payload (i.e.,
 * everything after the management header).  On success, the response payload
 * is written to the supplied cbuf but not transmitted.  On failure, no error
 * response gets written; the caller is expected to build an error response
 * from the return code.
 *
 * @param cbuf		A cbuf containing the request and response buffer.
 * @param req_hdr	The management header belonging to the incoming request (host-byte order).
 *
 * @return A MGMT_ERR_[...] error code.
 */
static int
smp_handle_single_payload(struct mgmt_ctxt *cbuf, const struct mgmt_hdr *req_hdr,
			  bool *handler_found)
{
	const struct mgmt_handler *handler;
	mgmt_handler_fn handler_fn;
	int rc;

	handler = mgmt_find_handler(req_hdr->nh_group, req_hdr->nh_id);
	if (handler == NULL) {
		return MGMT_ERR_ENOTSUP;
	}

	switch (req_hdr->nh_op) {
	case MGMT_OP_READ:
		handler_fn = handler->mh_read;
		break;

	case MGMT_OP_WRITE:
		handler_fn = handler->mh_write;
		break;

	default:
		return MGMT_ERR_EINVAL;
	}

	if (handler_fn) {
		*handler_found = true;
		zcbor_map_start_encode(cbuf->cnbe->zs, CONFIG_MGMT_MAX_MAIN_MAP_ENTRIES);
		mgmt_evt(MGMT_EVT_OP_CMD_RECV, req_hdr->nh_group, req_hdr->nh_id, NULL);

		MGMT_CTXT_SET_RC_RSN(cbuf, NULL);
		rc = handler_fn(cbuf);

		/* End response payload. */
		if (!zcbor_map_end_encode(cbuf->cnbe->zs, CONFIG_MGMT_MAX_MAIN_MAP_ENTRIES) &&
		    rc == 0) {
			rc = MGMT_ERR_EMSGSIZE;
		}
	} else {
		rc = MGMT_ERR_ENOTSUP;
	}

	return rc;
}

/**
 * Processes a single SMP request and generates a complete response (i.e.,
 * header and payload).  On success, the response is written using the supplied
 * streamer but not transmitted.  On failure, no error response gets written;
 * the caller is expected to build an error response from the return code.
 *
 * @param streamer	The SMP streamer to use for reading the request and writing the response.
 * @param req_hdr	The management header belonging to the incoming request (host-byte order).
 *
 * @return A MGMT_ERR_[...] error code.
 */
static int
smp_handle_single_req(struct smp_streamer *streamer, const struct mgmt_hdr *req_hdr,
		      bool *handler_found, const char **rsn)
{
	struct mgmt_ctxt cbuf;
	struct mgmt_hdr rsp_hdr;
	struct cbor_nb_writer *nbw = streamer->writer;
	struct cbor_nb_reader *nbr = streamer->reader;
	zcbor_state_t *zsp = nbw->zs;
	int rc;

	cbuf.cnbe = nbw;
	cbuf.cnbd = nbr;

	/* Process the request and write the response payload. */
	rc = smp_handle_single_payload(&cbuf, req_hdr, handler_found);
	if (rc != 0) {
		*rsn = MGMT_CTXT_RC_RSN(&cbuf);
		return rc;
	}

	smp_make_rsp_hdr(req_hdr, &rsp_hdr,
			 zsp->payload_mut - nbw->nb->data - MGMT_HDR_SIZE);
	nbw->nb->len = zsp->payload_mut - nbw->nb->data;
	smp_write_hdr(streamer, &rsp_hdr);

	return 0;
}

/**
 * Attempts to transmit an SMP error response.  This function consumes both
 * supplied buffers.
 *
 * @param streamer	The SMP streamer for building and transmitting the response.
 * @param req_hdr	The header of the request which elicited the error.
 * @param req		The buffer holding the request.
 * @param rsp		The buffer holding the response, or NULL if none was allocated.
 * @param status	The status to indicate in the error response.
 * @param rsn		The text explanation to @status encoded as "rsn" into CBOR
 *			response.
 */
static void
smp_on_err(struct smp_streamer *streamer, const struct mgmt_hdr *req_hdr,
		   void *req, void *rsp, int status, const char *rsn)
{
	int rc;

	/* Prefer the response buffer for holding the error response.  If no
	 * response buffer was allocated, use the request buffer instead.
	 */
	if (rsp == NULL) {
		rsp = req;
		req = NULL;
	}

	/* Clear the partial response from the buffer, if any. */
	cbor_nb_writer_init(streamer->writer, rsp);

	/* Build and transmit the error response. */
	rc = smp_build_err_rsp(streamer, req_hdr, status, rsn);
	if (rc == 0) {
		streamer->smpt->zst_output(rsp);
		rsp = NULL;
	}

	/* Free any extra buffers. */
	zephyr_smp_free_buf(req, streamer->smpt);
	zephyr_smp_free_buf(rsp, streamer->smpt);
}

/**
 * Processes all SMP requests in an incoming packet.  Requests are processed
 * sequentially from the start of the packet to the end.  Each response is sent
 * individually in its own packet.  If a request elicits an error response,
 * processing of the packet is aborted.  This function consumes the supplied
 * request buffer regardless of the outcome.
 * The function will return MGMT_ERR_EOK (0) when given an empty input stream,
 * and will also release the buffer from the stream; it does not return
 * MTMT_ERR_ECORRUPT, or any other MGMT error, because there was no error while
 * processing of the input stream, it is callers fault that an empty stream has
 * been passed to the function.
 *
 * @param streamer	The streamer to use for reading, writing, and transmitting.
 * @param req		A buffer containing the request packet.
 *
 * @return 0 on success or when input stream is empty;
 *         MGMT_ERR_ECORRUPT if buffer starts with non SMP data header or there
 *         is not enough bytes to process header, or other MGMT_ERR_[...] code on
 *         failure.
 */
int
smp_process_request_packet(struct smp_streamer *streamer, void *vreq)
{
	struct mgmt_hdr req_hdr;
	struct mgmt_evt_op_cmd_done_arg cmd_done_arg;
	void *rsp;
	struct net_buf *req = vreq;
	bool valid_hdr = false;
	bool handler_found = false;
	int rc = 0;
	const char *rsn = NULL;

	rsp = NULL;

	while (req->len > 0) {
		handler_found = false;
		valid_hdr = false;

		/* Read the management header and strip it from the request. */
		rc = smp_read_hdr(req, &req_hdr);
		if (rc != 0) {
			rc = MGMT_ERR_ECORRUPT;
			break;
		} else {
			valid_hdr = true;
		}
		mgmt_ntoh_hdr(&req_hdr);
		/* Does buffer contain whole message? */
		if (req->len < (req_hdr.nh_len + MGMT_HDR_SIZE)) {
			rc = MGMT_ERR_ECORRUPT;
			break;
		}

		rsp = zephyr_smp_alloc_rsp(req, streamer->smpt);
		if (rsp == NULL) {
			rc = MGMT_ERR_ENOMEM;
			break;
		}

		cbor_nb_reader_init(streamer->reader, req);
		cbor_nb_writer_init(streamer->writer, rsp);

		/* Process the request payload and build the response. */
		rc = smp_handle_single_req(streamer, &req_hdr, &handler_found, &rsn);
		if (rc != 0) {
			break;
		}

		/* Send the response. */
		rc = streamer->smpt->zst_output(rsp);
		rsp = NULL;
		if (rc != 0) {
			break;
		}

		/* Trim processed request to free up space for subsequent responses. */
		net_buf_pull(req, req_hdr.nh_len);

		cmd_done_arg.err = MGMT_ERR_EOK;
		mgmt_evt(MGMT_EVT_OP_CMD_DONE, req_hdr.nh_group, req_hdr.nh_id,
				 &cmd_done_arg);
	}

	if (rc != 0 && valid_hdr) {
		smp_on_err(streamer, &req_hdr, req, rsp, rc, rsn);

		if (handler_found) {
			cmd_done_arg.err = rc;
			mgmt_evt(MGMT_EVT_OP_CMD_DONE, req_hdr.nh_group, req_hdr.nh_id,
				 &cmd_done_arg);
		}

		return rc;
	}

	zephyr_smp_free_buf(req, streamer->smpt);
	zephyr_smp_free_buf(rsp, streamer->smpt);

	return rc;
}
