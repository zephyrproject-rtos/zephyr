/*
 * Copyright (c) 2018-2021 mcumgr authors
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** SMP - Simple Management Protocol. */

#include <zephyr/sys/byteorder.h>
#include <zephyr/net/buf.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>
#include <assert.h>
#include <string.h>

#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>

#include <mgmt/mcumgr/transport/smp_internal.h>

#ifdef CONFIG_MCUMGR_MGMT_NOTIFICATION_HOOKS
#include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>
#endif

static void cbor_nb_reader_init(struct cbor_nb_reader *cnr, struct net_buf *nb)
{
	/* Skip the smp_hdr */
	void *new_ptr = net_buf_pull(nb, sizeof(struct smp_hdr));

	cnr->nb = nb;
	zcbor_new_decode_state(cnr->zs, ARRAY_SIZE(cnr->zs), new_ptr,
			       cnr->nb->len, 1);
}

static void cbor_nb_writer_init(struct cbor_nb_writer *cnw, struct net_buf *nb)
{
	net_buf_reset(nb);
	cnw->nb = nb;
	cnw->nb->len = sizeof(struct smp_hdr);
	zcbor_new_encode_state(cnw->zs, 2, nb->data + sizeof(struct smp_hdr),
			       net_buf_tailroom(nb), 0);
}

/**
 * Converts a request opcode to its corresponding response opcode.
 */
static uint8_t smp_rsp_op(uint8_t req_op)
{
	if (req_op == MGMT_OP_READ) {
		return MGMT_OP_READ_RSP;
	} else {
		return MGMT_OP_WRITE_RSP;
	}
}

static void smp_make_rsp_hdr(const struct smp_hdr *req_hdr, struct smp_hdr *rsp_hdr, size_t len)
{
	*rsp_hdr = (struct smp_hdr) {
		.nh_len = sys_cpu_to_be16(len),
		.nh_flags = 0,
		.nh_op = smp_rsp_op(req_hdr->nh_op),
		.nh_group = sys_cpu_to_be16(req_hdr->nh_group),
		.nh_seq = req_hdr->nh_seq,
		.nh_id = req_hdr->nh_id,
	};
}

static int smp_read_hdr(const struct net_buf *nb, struct smp_hdr *dst_hdr)
{
	if (nb->len < sizeof(*dst_hdr)) {
		return MGMT_ERR_EINVAL;
	}

	memcpy(dst_hdr, nb->data, sizeof(*dst_hdr));
	dst_hdr->nh_len = sys_be16_to_cpu(dst_hdr->nh_len);
	dst_hdr->nh_group = sys_be16_to_cpu(dst_hdr->nh_group);

	return 0;
}

static inline int smp_write_hdr(struct smp_streamer *streamer, const struct smp_hdr *src_hdr)
{
	memcpy(streamer->writer->nb->data, src_hdr, sizeof(*src_hdr));
	return 0;
}

static int smp_build_err_rsp(struct smp_streamer *streamer, const struct smp_hdr *req_hdr,
			     int status, const char *rc_rsn)
{
	struct smp_hdr rsp_hdr;
	struct cbor_nb_writer *nbw = streamer->writer;
	zcbor_state_t *zsp = nbw->zs;
	bool ok;

	ok = zcbor_map_start_encode(zsp, 2)		&&
	     zcbor_tstr_put_lit(zsp, "rc")		&&
	     zcbor_int32_put(zsp, status);

#ifdef CONFIG_MCUMGR_SMP_VERBOSE_ERR_RESPONSE
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
static int smp_handle_single_payload(struct smp_streamer *cbuf, const struct smp_hdr *req_hdr,
				     bool *handler_found)
{
	const struct mgmt_handler *handler;
	mgmt_handler_fn handler_fn;
	int rc;
#if defined(CONFIG_MCUMGR_SMP_COMMAND_STATUS_HOOKS)
	struct mgmt_evt_op_cmd_arg cmd_recv;
#endif

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
		zcbor_map_start_encode(cbuf->writer->zs,
				       CONFIG_MCUMGR_SMP_CBOR_MAX_MAIN_MAP_ENTRIES);

#if defined(CONFIG_MCUMGR_SMP_COMMAND_STATUS_HOOKS)
		cmd_recv.group = req_hdr->nh_group;
		cmd_recv.id = req_hdr->nh_id;
		cmd_recv.err = MGMT_ERR_EOK;

		(void)mgmt_callback_notify(MGMT_EVT_OP_CMD_RECV, &cmd_recv, sizeof(cmd_recv));
#endif

		MGMT_CTXT_SET_RC_RSN(cbuf, NULL);
		rc = handler_fn(cbuf);

		/* End response payload. */
		if (!zcbor_map_end_encode(cbuf->writer->zs,
					  CONFIG_MCUMGR_SMP_CBOR_MAX_MAIN_MAP_ENTRIES) &&
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
static int smp_handle_single_req(struct smp_streamer *streamer, const struct smp_hdr *req_hdr,
				 bool *handler_found, const char **rsn)
{
	struct smp_hdr rsp_hdr;
	struct cbor_nb_writer *nbw = streamer->writer;
	zcbor_state_t *zsp = nbw->zs;
	int rc;

	/* Process the request and write the response payload. */
	rc = smp_handle_single_payload(streamer, req_hdr, handler_found);
	if (rc != 0) {
		*rsn = MGMT_CTXT_RC_RSN(streamer);
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
static void smp_on_err(struct smp_streamer *streamer, const struct smp_hdr *req_hdr,
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
		streamer->smpt->output(rsp);
		rsp = NULL;
	}

	/* Free any extra buffers. */
	smp_free_buf(req, streamer->smpt);
	smp_free_buf(rsp, streamer->smpt);
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
int smp_process_request_packet(struct smp_streamer *streamer, void *vreq)
{
	struct smp_hdr req_hdr;
	void *rsp;
	struct net_buf *req = vreq;
	bool valid_hdr = false;
	bool handler_found = false;
	int rc = 0;
	const char *rsn = NULL;

#if defined(CONFIG_MCUMGR_SMP_COMMAND_STATUS_HOOKS)
	struct mgmt_evt_op_cmd_arg cmd_done_arg;
#endif

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
		/* Does buffer contain whole message? */
		if (req->len < (req_hdr.nh_len + MGMT_HDR_SIZE)) {
			rc = MGMT_ERR_ECORRUPT;
			break;
		}

		rsp = smp_alloc_rsp(req, streamer->smpt);
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
		rc = streamer->smpt->output(rsp);
		rsp = NULL;
		if (rc != 0) {
			break;
		}

		/* Trim processed request to free up space for subsequent responses. */
		net_buf_pull(req, req_hdr.nh_len);

#if defined(CONFIG_MCUMGR_SMP_COMMAND_STATUS_HOOKS)
		cmd_done_arg.group = req_hdr.nh_group;
		cmd_done_arg.id = req_hdr.nh_id;
		cmd_done_arg.err = MGMT_ERR_EOK;

		(void)mgmt_callback_notify(MGMT_EVT_OP_CMD_DONE, &cmd_done_arg,
					   sizeof(cmd_done_arg));
#endif
	}

	if (rc != 0 && valid_hdr) {
		smp_on_err(streamer, &req_hdr, req, rsp, rc, rsn);

		if (handler_found) {
#if defined(CONFIG_MCUMGR_SMP_COMMAND_STATUS_HOOKS)
			cmd_done_arg.group = req_hdr.nh_group;
			cmd_done_arg.id = req_hdr.nh_id;
			cmd_done_arg.err = rc;

			(void)mgmt_callback_notify(MGMT_EVT_OP_CMD_DONE, &cmd_done_arg,
						   sizeof(cmd_done_arg));
#endif
		}

		return rc;
	}

	smp_free_buf(req, streamer->smpt);
	smp_free_buf(rsp, streamer->smpt);

	return rc;
}
