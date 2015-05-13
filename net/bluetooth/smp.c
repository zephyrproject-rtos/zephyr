/**
 * @file smp.c
 * Security Manager Protocol implementation
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <nanokernel.h>
#include <stddef.h>

#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>

#include "hci_core.h"
#include "conn.h"
#include "l2cap.h"
#include "smp.h"

#if !defined(CONFIG_BLUETOOTH_DEBUG_SMP)
#undef BT_DBG
#define BT_DBG(fmt, ...)
#endif

struct bt_buf *bt_smp_create_pdu(struct bt_conn *conn, uint8_t op, size_t len)
{
	struct bt_smp_hdr *hdr;
	struct bt_buf *buf;

	buf = bt_l2cap_create_pdu(conn, BT_L2CAP_CID_SMP, sizeof(*hdr) + len);
	if (!buf)
		return NULL;

	hdr = (void *)bt_buf_add(buf, sizeof(*hdr));
	hdr->code = op;

	return buf;
}

static void send_err_rsp(struct bt_conn *conn, uint8_t reason)
{
	struct bt_smp_pairing_fail *rsp;
	struct bt_buf *buf;

	buf = bt_smp_create_pdu(conn, BT_SMP_CMD_PAIRING_FAIL, sizeof(*rsp));
	if (!buf)
		return;

	rsp = (void *)bt_buf_add(buf, sizeof(*rsp));
	rsp->reason = reason;

	bt_conn_send(conn, buf);
}

static int smp_pairing_req(struct bt_conn *conn, struct bt_buf *buf)
{
	struct bt_smp_pairing *req = (void *)buf->data;
	struct bt_smp_pairing *rsp;
	struct bt_buf *rsp_buf;

	BT_DBG("\n");

	if (buf->len != sizeof(*req))
		return BT_SMP_ERR_INVALID_PARAMS;

	rsp_buf = bt_smp_create_pdu(conn, BT_SMP_CMD_PAIRING_RSP, sizeof(*rsp));
	if (!rsp_buf)
		return BT_SMP_ERR_UNSPECIFIED;

	if ((req->max_key_size > BT_SMP_MAX_ENC_KEY_SIZE) ||
	    (req->max_key_size < BT_SMP_MIN_ENC_KEY_SIZE)) {
		return BT_SMP_ERR_ENC_KEY_SIZE;
	}

	rsp = (void *)bt_buf_add(rsp_buf, sizeof(*rsp));

	/* For JustWorks pairing simplify rsp parameters.
	 * TODO: needs to be reworked later on
	 */
	rsp->auth_req = req->auth_req;
	rsp->io_capability = BT_SMP_IO_NO_INPUT_OUTPUT;
	rsp->oob_flag = BT_SMP_OOB_NOT_PRESENT;
	rsp->max_key_size = req->max_key_size;
	rsp->init_key_dist = 0;
	rsp->resp_key_dist = 0;

	bt_conn_send(conn, rsp_buf);

	return 0;
}

static int smp_pairing_confirm(struct bt_conn *conn, struct bt_buf *buf)
{
	struct bt_smp_pairing_confirm *req = (void *)buf->data;

	BT_DBG("\n");

	if (buf->len != sizeof(*req))
		return BT_SMP_ERR_INVALID_PARAMS;

	/* TODO: Send pairing_confirm(Sconfirm) back */

	return 0;
}

void bt_smp_recv(struct bt_conn *conn, struct bt_buf *buf)
{
	struct bt_smp_hdr *hdr = (void *)buf->data;
	int err;

	if (buf->len < sizeof(*hdr)) {
		BT_ERR("Too small SMP PDU received\n");
		goto done;
	}

	BT_DBG("Received SMP code %u len %u\n", hdr->code, buf->len);

	bt_buf_pull(buf, sizeof(*hdr));

	switch (hdr->code) {
	case BT_SMP_CMD_PAIRING_REQ:
		err = smp_pairing_req(conn, buf);
		break;
	case BT_SMP_CMD_PAIRING_CONFIRM:
		err = smp_pairing_confirm(conn, buf);
		break;
	default:
		BT_WARN("Unhandled SMP code %u\n", hdr->code);
		err = BT_SMP_ERR_CMD_NOTSUPP;
		break;
	}

	if (err) {
		send_err_rsp(conn, err);
	}

done:
	bt_buf_put(buf);
}
