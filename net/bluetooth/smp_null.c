/**
 * @file smp_null.c
 * Security Manager Protocol stub
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

#include <bluetooth/bluetooth.h>
#include <errno.h>
#include "l2cap.h"
#include "smp.h"

int bt_smp_sign_verify(struct bt_conn *conn, struct bt_buf *buf)
{
	return -ENOTSUP;
}

static void bt_smp_recv(struct bt_conn *conn, struct bt_buf *buf)
{
	struct bt_smp_pairing_fail *rsp;
	struct bt_smp_hdr *hdr;

	bt_buf_put(buf);

	/* If a device does not support pairing then it shall respond with
	 * a Pairing Failed command with the reason set to “Pairing Not
	 * Supported” when any command is received.
	 * Core Specification Vol. 3, Part H, 3.3
	 */

	buf = bt_l2cap_create_pdu(conn);
	if (!buf) {
		return;
	}

	hdr = bt_buf_add(buf, sizeof(*hdr));
	hdr->code = BT_SMP_CMD_PAIRING_FAIL;

	rsp = bt_buf_add(buf, sizeof(*rsp));
	rsp->reason = BT_SMP_ERR_PAIRING_NOTSUPP;

	bt_l2cap_send(conn, BT_L2CAP_CID_SMP, buf);
}

int bt_smp_init(void)
{
	static struct bt_l2cap_chan chan = {
		.cid	= BT_L2CAP_CID_SMP,
		.recv	= bt_smp_recv,
	};

	bt_l2cap_chan_register(&chan);

	return 0;
}
