#include <nanokernel.h>
#include <toolchain.h>
#include <string.h>
#include <errno.h>
#include <misc/byteorder.h>

#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>

#include "hci_core.h"
#include "conn.h"
#include "l2cap.h"

#define LE_CONN_MIN_INTERVAL	0x0028
#define LE_CONN_MAX_INTERVAL	0x0038
#define LE_CONN_LATENCY		0x0000
#define LE_CONN_TIMEOUT		0x002a

static uint8_t get_ident(struct bt_conn *conn)
{
	conn->l2_ident++;

	/* handle integer overflow (0 is not valid) */
	if (!conn->l2_ident)
		conn->l2_ident++;

	return conn->l2_ident;
}

struct bt_buf *bt_l2cap_create_pdu(struct bt_conn *conn, uint16_t cid,
				   size_t len)
{
	struct bt_l2cap_hdr *hdr;
	struct bt_buf *buf;

	buf = bt_conn_create_pdu(conn, sizeof(*hdr) + len);
	if (!buf)
		return NULL;

	hdr = (void *)bt_buf_add(buf, sizeof(*hdr));
	hdr->len = sys_cpu_to_le16(len);
	hdr->cid = sys_cpu_to_le16(cid);

	return buf;
}

static void rej_not_understood(struct bt_conn *conn, uint8_t ident)
{
	struct bt_l2cap_cmd_reject *rej;
	struct bt_l2cap_sig_hdr *hdr;
	struct bt_buf *buf;

	buf = bt_l2cap_create_pdu(conn, BT_L2CAP_CID_LE_SIG,
				  sizeof(*hdr) + sizeof(*rej));
	if (!buf)
		return;

	hdr = (void *)bt_buf_add(buf, sizeof(*hdr));
	hdr->code = BT_L2CAP_CMD_REJECT;
	hdr->ident = ident;
	hdr->len = sys_cpu_to_le16(sizeof(*rej));

	rej = (void *)bt_buf_add(buf, sizeof(*rej));
	rej->reason = sys_cpu_to_le16(BT_L2CAP_REJ_NOT_UNDERSTOOD);

	bt_conn_send(conn, buf);
}

static void le_conn_param_rsp(struct bt_conn *conn, struct bt_buf *buf)
{
	struct bt_l2cap_conn_param_rsp *rsp = (void *)buf->data;

	if (buf->len < sizeof(*rsp)) {
		BT_ERR("Too small LE conn param rsp\n");
		return;
	}

	BT_DBG("LE conn param rsp result %u\n", sys_le16_to_cpu(rsp->result));
}

static void le_sig(struct bt_conn *conn, struct bt_buf *buf)
{
	struct bt_l2cap_sig_hdr *hdr = (void *)buf->data;
	uint16_t len;

	if (buf->len < sizeof(*hdr)) {
		BT_ERR("Too small L2CAP LE signaling PDU\n");
		goto drop;
	}

	len = sys_le16_to_cpu(hdr->len);
	bt_buf_pull(buf, sizeof(*hdr));

	BT_DBG("LE signaling code %u ident %u len %u\n", hdr->code,
	       hdr->ident, len);

	if (buf->len != len) {
		BT_ERR("L2CAP length mismatch (%u != %u)\n", buf->len, len);
		goto drop;
	}

	if (!hdr->ident) {
		BT_ERR("Invalid ident value in L2CAP PDU\n");
		goto drop;
	}

	switch (hdr->code) {
	case BT_L2CAP_CONN_PARAM_RSP:
		le_conn_param_rsp(conn, buf);
		break;
	default:
		BT_ERR("Unknown L2CAP PDU code %u\n", hdr->code);
		rej_not_understood(conn, hdr->ident);
		break;
	}

drop:
	bt_buf_put(buf);
}

void bt_l2cap_recv(struct bt_conn *conn, struct bt_buf *buf)
{
	struct bt_l2cap_hdr *hdr = (void *)buf->data;
	uint16_t cid;

	if (buf->len < sizeof(*hdr)) {
		BT_ERR("Too small L2CAP PDU received\n");
		bt_buf_put(buf);
		return;
	}

	cid = sys_le16_to_cpu(hdr->cid);
	bt_buf_pull(buf, sizeof(*hdr));

	BT_DBG("Packet for CID %u len %u\n", cid, buf->len);

	switch (cid) {
	case BT_L2CAP_CID_LE_SIG:
		le_sig(conn, buf);
		break;
	default:
		BT_ERR("Ignoring data for unknown CID %u\n", cid);
		bt_buf_put(buf);
		break;
	}
}

void bt_l2cap_update_conn_param(struct bt_conn *conn)
{
	struct bt_l2cap_sig_hdr *hdr;
	struct bt_l2cap_conn_param_req *req;
	struct bt_buf *buf;

	/* Check if we need to update anything */
	if (conn->le_conn_interval >= LE_CONN_MIN_INTERVAL &&
	    conn->le_conn_interval <= LE_CONN_MAX_INTERVAL)
		return;

	buf = bt_l2cap_create_pdu(conn, BT_L2CAP_CID_LE_SIG,
				  sizeof(*hdr) + sizeof(*req));
	if (!buf)
		return;

	hdr = (void *)bt_buf_add(buf, sizeof(*hdr));
	hdr->code = BT_L2CAP_CONN_PARAM_REQ;
	hdr->ident = get_ident(conn);
	hdr->len = sys_cpu_to_le16(sizeof(*req));

	req = (void *)bt_buf_add(buf, sizeof(*req));
	req->min_interval = sys_cpu_to_le16(LE_CONN_MIN_INTERVAL);
	req->max_interval = sys_cpu_to_le16(LE_CONN_MAX_INTERVAL);
	req->latency = sys_cpu_to_le16(LE_CONN_LATENCY);
	req->timeout = sys_cpu_to_le16(LE_CONN_TIMEOUT);

	bt_conn_send(conn, buf);
}
