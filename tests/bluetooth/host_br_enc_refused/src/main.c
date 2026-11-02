/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/* The Host refuses an encrypted BR/EDR link at Encryption Change or Encryption Key
 * Refresh Complete when the key is too short or the link stays below the required
 * security level, and disconnects it. The peer can send until the link is down, and
 * nothing of that reaches L2CAP.
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/classic/classic.h>
#include <zephyr/bluetooth/classic/l2cap_br.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "host/conn_internal.h"
#include "host/hci_core.h"
#include "host/keys.h"
#include "host/l2cap_internal.h"

#define DT_DRV_COMPAT zephyr_bt_hci_test

/* Large enough for the return parameters of the commands without a handler,
 * which are zeroed.
 */
#define GENERIC_RP_SIZE 64U

#define CONN_HANDLE 0x0001U
#define ACL_PKTS    4U

/* Below CONFIG_BT_BR_MIN_ENC_KEY_SIZE, and a size that a controller can report */
#define KEY_SIZE_SHORT 7U
#define KEY_SIZE_LONG  16U

#define L2CAP_PSM      0x1001U
#define L2CAP_SCID     0x0040U
#define SIG_CID        0x0001U
#define CONNLESS_CID   0x0002U
#define SMP_CID        0x0007U
#define L2CAP_CONN_REQ 0x02U
#define L2CAP_CONN_RSP 0x03U
#define L2CAP_CONF_REQ 0x04U
#define L2CAP_CONF_RSP 0x05U

#define SMP_PAIRING_REQ 0x01U
#define SMP_DIST_ENC    0x01U

/* Long enough for what the fake controller and the Host have to do */
#define SETTLE         K_MSEC(10)
/* Longer than L2CAP_BR_RETRY_INTERVAL_MAX */
#define SIG_RETRY_WAIT K_MSEC(200)

struct cmd_handler {
	uint16_t opcode;
	uint8_t len;
	/* Sets evt to NULL when it sends its events itself, or none */
	void (*handler)(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode);
};

static const bt_addr_t peer = {
	.val = {0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U},
};

/* What the controller answers to HCI Read Encryption Key Size */
static uint8_t key_size;

/* With key_size_hold, the controller keeps the response until the test sends it
 * with key_size_respond().
 */
static bool key_size_hold;
static K_SEM_DEFINE(key_size_sem, 0, 1);

/* What the Host sent */
static uint32_t disconnect_count;
static uint32_t acl_count;
static uint16_t host_cid;
static uint8_t host_conf_ident;

/* With ncp_hold, the controller does not report the completion of ACL packets
 * until ncp_release(). This counts what it holds.
 */
static bool ncp_hold;
static uint32_t ncp_held;

static K_SEM_DEFINE(disconnect_sem, 0, 1);
static K_SEM_DEFINE(accept_sem, 0, 1);

static void evt_create(struct net_buf *buf, uint8_t evt, uint8_t len)
{
	struct bt_hci_evt_hdr *hdr;

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->evt = evt;
	hdr->len = len;
}

static void *cmd_complete(struct net_buf **buf, uint8_t plen, uint16_t opcode)
{
	struct bt_hci_evt_cmd_complete *cc;

	*buf = bt_buf_get_evt(BT_HCI_EVT_CMD_COMPLETE, false, K_FOREVER);
	evt_create(*buf, BT_HCI_EVT_CMD_COMPLETE, sizeof(*cc) + plen);
	cc = net_buf_add(*buf, sizeof(*cc));
	cc->ncmd = 1U;
	cc->opcode = sys_cpu_to_le16(opcode);

	return net_buf_add(*buf, plen);
}

static void cmd_status(uint16_t opcode, uint8_t status)
{
	struct bt_hci_evt_cmd_status *cs;
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_CMD_STATUS, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_CMD_STATUS, sizeof(*cs));
	cs = net_buf_add(buf, sizeof(*cs));
	cs->status = status;
	cs->ncmd = 1U;
	cs->opcode = sys_cpu_to_le16(opcode);
	bt_hci_recv(DEVICE_DT_INST_GET(0), buf);
}

static void generic_success(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	struct bt_hci_evt_cc_status *rp;

	ARG_UNUSED(buf);

	rp = cmd_complete(evt, len, opcode);
	(void)memset(rp, 0, len);
	rp->status = BT_HCI_ERR_SUCCESS;
}

static void read_local_features(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				uint16_t opcode)
{
	struct bt_hci_rp_read_local_features *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->features, 0, sizeof(rp->features));
	/* LE and BR/EDR supported */
	rp->features[4] = BIT(6);
	/* Extended features */
	rp->features[7] = BIT(7);
}

static void read_local_ext_features(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				    uint16_t opcode)
{
	const struct bt_hci_cp_read_local_ext_features *cp = (const void *)buf->data;
	struct bt_hci_rp_read_local_ext_features *rp;
	uint8_t page = cp->page;

	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	(void)memset(rp, 0, sizeof(*rp));
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->page = page;
	rp->max_page = 2U;
	if (page == 2U) {
		/* Secure Connections (Controller Support) */
		rp->ext_features[1] = BIT(0);
	}
}

static void read_supported_commands(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				    uint16_t opcode)
{
	struct bt_hci_rp_read_supported_commands *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->commands, 0xFF, sizeof(rp->commands));
}

static void read_bd_addr(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	struct bt_hci_rp_read_bd_addr *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(&rp->bdaddr, 0x11, sizeof(rp->bdaddr));
}

static void read_buffer_size(struct net_buf *buf, struct net_buf **evt, uint8_t len,
			     uint16_t opcode)
{
	struct bt_hci_rp_read_buffer_size *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	(void)memset(rp, 0, sizeof(*rp));
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->acl_max_len = sys_cpu_to_le16(255U);
	rp->acl_max_num = sys_cpu_to_le16(ACL_PKTS);
}

static void accept_conn_req(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(evt);
	ARG_UNUSED(len);

	cmd_status(opcode, BT_HCI_ERR_SUCCESS);
	k_sem_give(&accept_sem);
}

static void read_remote_features(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				 uint16_t opcode)
{
	struct bt_hci_evt_remote_features *rf;
	struct net_buf *rf_buf;

	ARG_UNUSED(buf);
	ARG_UNUSED(evt);
	ARG_UNUSED(len);

	cmd_status(opcode, BT_HCI_ERR_SUCCESS);

	rf_buf = bt_buf_get_evt(BT_HCI_EVT_REMOTE_FEATURES, false, K_FOREVER);
	evt_create(rf_buf, BT_HCI_EVT_REMOTE_FEATURES, sizeof(*rf));
	rf = net_buf_add(rf_buf, sizeof(*rf));
	(void)memset(rf, 0, sizeof(*rf));
	rf->handle = sys_cpu_to_le16(CONN_HANDLE);
	/* Extended features */
	rf->features[7] = BIT(7);
	bt_hci_recv(DEVICE_DT_INST_GET(0), rf_buf);
}

static void read_remote_ext_features(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				     uint16_t opcode)
{
	struct bt_hci_evt_remote_ext_features *rf;
	struct net_buf *rf_buf;

	ARG_UNUSED(buf);
	ARG_UNUSED(evt);
	ARG_UNUSED(len);

	cmd_status(opcode, BT_HCI_ERR_SUCCESS);

	rf_buf = bt_buf_get_evt(BT_HCI_EVT_REMOTE_EXT_FEATURES, false, K_FOREVER);
	evt_create(rf_buf, BT_HCI_EVT_REMOTE_EXT_FEATURES, sizeof(*rf));
	rf = net_buf_add(rf_buf, sizeof(*rf));
	(void)memset(rf, 0, sizeof(*rf));
	rf->handle = sys_cpu_to_le16(CONN_HANDLE);
	rf->page = 1U;
	rf->max_page = 1U;
	/* Secure Simple Pairing (Host Support) */
	rf->features[0] = BIT(0);
	bt_hci_recv(DEVICE_DT_INST_GET(0), rf_buf);
}

static struct net_buf *key_size_rsp(void)
{
	struct bt_hci_rp_read_encryption_key_size *rp;
	struct net_buf *buf;

	rp = cmd_complete(&buf, sizeof(*rp), BT_HCI_OP_READ_ENCRYPTION_KEY_SIZE);
	(void)memset(rp, 0, sizeof(*rp));
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->handle = sys_cpu_to_le16(CONN_HANDLE);
	rp->key_size = key_size;

	return buf;
}

static void read_key_size(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	ARG_UNUSED(opcode);

	if (key_size_hold) {
		*evt = NULL;
		k_sem_give(&key_size_sem);
		return;
	}

	*evt = key_size_rsp();
}

/* Send the response that the controller holds */
static void key_size_respond(void)
{
	bt_hci_recv(DEVICE_DT_INST_GET(0), key_size_rsp());
}

static void disconn_complete(uint8_t reason)
{
	struct bt_hci_evt_disconn_complete *evt;
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_DISCONN_COMPLETE, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_DISCONN_COMPLETE, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	evt->status = BT_HCI_ERR_SUCCESS;
	evt->handle = sys_cpu_to_le16(CONN_HANDLE);
	evt->reason = reason;
	bt_hci_recv(DEVICE_DT_INST_GET(0), buf);
}

/* With disconnect_hold, the controller keeps the Command Status until the test sends it
 * with disconnect_respond().
 */
static bool disconnect_hold;

static void disconnect_respond(void)
{
	disconnect_hold = false;
	cmd_status(BT_HCI_OP_DISCONNECT, BT_HCI_ERR_SUCCESS);
}

/* The test reports the disconnection when it wants to */
static void disconnect(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(evt);
	ARG_UNUSED(len);

	disconnect_count++;
	k_sem_give(&disconnect_sem);

	if (!disconnect_hold) {
		cmd_status(opcode, BT_HCI_ERR_SUCCESS);
	}
}

/* No event for this one */
static void host_num_completed_packets(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				       uint16_t opcode)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(evt);
	ARG_UNUSED(len);
	ARG_UNUSED(opcode);
}

static void create_connection(struct net_buf *buf, struct net_buf **evt, uint8_t len,
			      uint16_t opcode)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(evt);
	ARG_UNUSED(len);

	cmd_status(opcode, BT_HCI_ERR_SUCCESS);
}

static const struct cmd_handler cmds[] = {
	{
		BT_HCI_OP_READ_LOCAL_FEATURES,
		sizeof(struct bt_hci_rp_read_local_features),
		read_local_features,
	},
	{
		BT_HCI_OP_READ_LOCAL_EXT_FEATURES,
		sizeof(struct bt_hci_rp_read_local_ext_features),
		read_local_ext_features,
	},
	{
		BT_HCI_OP_READ_SUPPORTED_COMMANDS,
		sizeof(struct bt_hci_rp_read_supported_commands),
		read_supported_commands,
	},
	{
		BT_HCI_OP_READ_BD_ADDR,
		sizeof(struct bt_hci_rp_read_bd_addr),
		read_bd_addr,
	},
	{
		BT_HCI_OP_READ_BUFFER_SIZE,
		sizeof(struct bt_hci_rp_read_buffer_size),
		read_buffer_size,
	},
	{
		BT_HCI_OP_CONNECT,
		0U,
		create_connection,
	},
	{
		BT_HCI_OP_ACCEPT_CONN_REQ,
		0U,
		accept_conn_req,
	},
	{
		BT_HCI_OP_READ_REMOTE_FEATURES,
		0U,
		read_remote_features,
	},
	{
		BT_HCI_OP_READ_REMOTE_EXT_FEATURES,
		0U,
		read_remote_ext_features,
	},
	{
		BT_HCI_OP_READ_ENCRYPTION_KEY_SIZE,
		0U,
		read_key_size,
	},
	{
		BT_HCI_OP_DISCONNECT,
		0U,
		disconnect,
	},
	{
		BT_HCI_OP_HOST_NUM_COMPLETED_PACKETS,
		0U,
		host_num_completed_packets,
	},
};

static void cmd_handle(const struct device *dev, struct net_buf *cmd)
{
	struct net_buf *evt = NULL;
	struct bt_hci_cmd_hdr *chdr;
	uint16_t opcode;

	chdr = net_buf_pull_mem(cmd, sizeof(*chdr));
	opcode = sys_le16_to_cpu(chdr->opcode);

	ARRAY_FOR_EACH_PTR(cmds, handler) {
		if (handler->opcode == opcode) {
			handler->handler(cmd, &evt, handler->len, opcode);
			if (evt != NULL) {
				bt_hci_recv(dev, evt);
			}

			return;
		}
	}

	generic_success(cmd, &evt, GENERIC_RP_SIZE, opcode);
	bt_hci_recv(dev, evt);
}

/* Takes note of what the Host says about the channel that the test opens */
static void acl_handle(struct net_buf *buf)
{
	struct bt_l2cap_hdr *hdr;
	uint8_t code;
	uint8_t ident;

	(void)net_buf_pull_mem(buf, sizeof(struct bt_hci_acl_hdr));
	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	if (sys_le16_to_cpu(hdr->cid) != SIG_CID) {
		return;
	}

	code = net_buf_pull_u8(buf);
	ident = net_buf_pull_u8(buf);
	(void)net_buf_pull_le16(buf);

	if (code == L2CAP_CONN_RSP) {
		host_cid = net_buf_pull_le16(buf);
	} else if (code == L2CAP_CONF_REQ) {
		host_conf_ident = ident;
	}
}

static void num_completed_packet(void)
{
	struct bt_hci_evt_num_completed_packets *evt;
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_NUM_COMPLETED_PACKETS, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_NUM_COMPLETED_PACKETS, sizeof(*evt) + sizeof(evt->h[0]));
	evt = net_buf_add(buf, sizeof(*evt));
	evt->num_handles = 1U;
	net_buf_add_le16(buf, CONN_HANDLE);
	net_buf_add_le16(buf, 1U);
	bt_hci_recv(DEVICE_DT_INST_GET(0), buf);
}

static void ncp_release(void)
{
	ncp_hold = false;

	while (ncp_held > 0U) {
		ncp_held--;
		num_completed_packet();
	}
}

static int driver_open(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int driver_close(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int driver_send(const struct device *dev, struct net_buf *buf)
{
	uint8_t type = net_buf_pull_u8(buf);

	if (type == BT_HCI_H4_CMD) {
		cmd_handle(dev, buf);
	} else if (type == BT_HCI_H4_ACL) {
		acl_count++;
		acl_handle(buf);
		if (ncp_hold) {
			ncp_held++;
		} else {
			num_completed_packet();
		}
	}

	net_buf_unref(buf);

	return 0;
}

static DEVICE_API(bt_hci, driver_api) = {
	.open = driver_open,
	.close = driver_close,
	.send = driver_send,
};

#define TEST_DEVICE_INIT(inst)                                                                     \
	static struct bt_hci_driver_data driver_data_##inst = {0};                                 \
	static const struct bt_hci_driver_config driver_config_##inst =                            \
		BT_DT_HCI_DRIVER_CONFIG_INST_GET(inst);                                            \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &driver_data_##inst, &driver_config_##inst,        \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &driver_api)

DT_INST_FOREACH_STATUS_OKAY(TEST_DEVICE_INIT)

static void conn_request(void)
{
	struct bt_hci_evt_conn_request *evt;
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_CONN_REQUEST, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_CONN_REQUEST, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	(void)memset(evt, 0, sizeof(*evt));
	bt_addr_copy(&evt->bdaddr, &peer);
	evt->link_type = BT_HCI_ACL;
	bt_hci_recv(DEVICE_DT_INST_GET(0), buf);
}

static void conn_complete(void)
{
	struct bt_hci_evt_conn_complete *evt;
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_CONN_COMPLETE, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_CONN_COMPLETE, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	evt->status = BT_HCI_ERR_SUCCESS;
	evt->handle = sys_cpu_to_le16(CONN_HANDLE);
	bt_addr_copy(&evt->bdaddr, &peer);
	evt->link_type = BT_HCI_ACL;
	evt->encr_enabled = 0U;
	bt_hci_recv(DEVICE_DT_INST_GET(0), buf);
}

static void link_key_req(void)
{
	struct bt_hci_evt_link_key_req *evt;
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_LINK_KEY_REQ, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_LINK_KEY_REQ, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	bt_addr_copy(&evt->bdaddr, &peer);
	bt_hci_recv(DEVICE_DT_INST_GET(0), buf);
}

static void encrypt_change(uint8_t encrypt)
{
	struct bt_hci_evt_encrypt_change *evt;
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_ENCRYPT_CHANGE, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_ENCRYPT_CHANGE, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	evt->status = BT_HCI_ERR_SUCCESS;
	evt->handle = sys_cpu_to_le16(CONN_HANDLE);
	evt->encrypt = encrypt;
	bt_hci_recv(DEVICE_DT_INST_GET(0), buf);
}

static void key_refresh_complete(void)
{
	struct bt_hci_evt_encrypt_key_refresh_complete *evt;
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_ENCRYPT_KEY_REFRESH_COMPLETE, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_ENCRYPT_KEY_REFRESH_COMPLETE, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	evt->status = BT_HCI_ERR_SUCCESS;
	evt->handle = sys_cpu_to_le16(CONN_HANDLE);
	bt_hci_recv(DEVICE_DT_INST_GET(0), buf);
}

/* The pool of what the peer sends */
static struct net_buf_pool *acl_in_pool;

/* An L2CAP PDU of the peer */
static void peer_send(uint16_t cid, const void *data, uint16_t len)
{
	struct net_buf *buf;

	buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
	acl_in_pool = net_buf_pool_get(buf->pool_id);
	net_buf_add_le16(buf, bt_acl_handle_pack(CONN_HANDLE, BT_ACL_START));
	net_buf_add_le16(buf, sizeof(struct bt_l2cap_hdr) + len);
	net_buf_add_le16(buf, len);
	net_buf_add_le16(buf, cid);
	net_buf_add_mem(buf, data, len);
	bt_hci_recv(DEVICE_DT_INST_GET(0), buf);
}

static void peer_conn_req(uint16_t scid)
{
	const struct {
		uint8_t code;
		uint8_t ident;
		uint16_t len;
		uint16_t psm;
		uint16_t scid;
	} __packed req = {
		.code = L2CAP_CONN_REQ,
		.ident = 1U,
		.len = sys_cpu_to_le16(4U),
		.psm = sys_cpu_to_le16(L2CAP_PSM),
		.scid = sys_cpu_to_le16(scid),
	};

	peer_send(SIG_CID, &req, sizeof(req));
}

static void peer_conf_rsp(void)
{
	const struct {
		uint8_t code;
		uint8_t ident;
		uint16_t len;
		uint16_t scid;
		uint16_t flags;
		uint16_t result;
	} __packed rsp = {
		.code = L2CAP_CONF_RSP,
		.ident = host_conf_ident,
		.len = sys_cpu_to_le16(6U),
		.scid = sys_cpu_to_le16(host_cid),
	};

	peer_send(SIG_CID, &rsp, sizeof(rsp));
}

static void peer_conf_req(void)
{
	const struct {
		uint8_t code;
		uint8_t ident;
		uint16_t len;
		uint16_t dcid;
		uint16_t flags;
	} __packed req = {
		.code = L2CAP_CONF_REQ,
		.ident = 2U,
		.len = sys_cpu_to_le16(4U),
		.dcid = sys_cpu_to_le16(host_cid),
	};

	peer_send(SIG_CID, &req, sizeof(req));
}

/* A Pairing Request that has the LE LTK derived from the link key */
static void peer_smp_pairing_req(uint8_t max_key_size)
{
	const uint8_t req[] = {
		SMP_PAIRING_REQ, 0U, 0U, 0U, max_key_size, SMP_DIST_ENC, SMP_DIST_ENC,
	};

	peer_send(SMP_CID, req, sizeof(req));
}

static void peer_connless_data(void)
{
	const uint8_t data[] = {
		L2CAP_PSM & 0xFFU,
		L2CAP_PSM >> 8,
		0x55U,
	};

	peer_send(CONNLESS_CID, data, sizeof(data));
}

/* For a channel that does not exist */
static void peer_chan_data_unknown(void)
{
	const uint8_t data[] = {
		0x55U,
	};

	peer_send(0x0077U, data, sizeof(data));
}

static void peer_chan_data(void)
{
	const uint8_t data[] = {
		0x55U,
	};

	peer_send(host_cid, data, sizeof(data));
}

/* What reached the services */
static uint32_t accept_count;
static uint32_t chan_recv_count;
static uint32_t connless_recv_count;
static uint32_t pairing_complete_count;
static uint32_t security_changed_count;

/* With accept_chan the server takes the connection request */
static bool accept_chan;

static K_SEM_DEFINE(chan_connected_sem, 0, 1);
static K_SEM_DEFINE(connected_sem, 0, 1);
static K_SEM_DEFINE(disconnected_sem, 0, 1);

static void chan_connected(struct bt_l2cap_chan *chan)
{
	ARG_UNUSED(chan);

	k_sem_give(&chan_connected_sem);
}

static void chan_disconnected(struct bt_l2cap_chan *chan)
{
	ARG_UNUSED(chan);
}

static int chan_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	ARG_UNUSED(chan);
	ARG_UNUSED(buf);

	chan_recv_count++;

	return 0;
}

static int server_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
			 struct bt_l2cap_chan **chan)
{
	static const struct bt_l2cap_chan_ops ops = {
		.connected = chan_connected,
		.disconnected = chan_disconnected,
		.recv = chan_recv,
	};
	static struct bt_l2cap_br_chan br_chan;

	ARG_UNUSED(conn);
	ARG_UNUSED(server);

	accept_count++;

	if (!accept_chan) {
		return -ENOMEM;
	}

	(void)memset(&br_chan, 0, sizeof(br_chan));
	br_chan.chan.ops = &ops;
	br_chan.rx.mtu = 48U;
	*chan = &br_chan.chan;

	return 0;
}

static void connless_recv(struct bt_conn *conn, uint16_t psm, struct net_buf *buf)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(psm);
	ARG_UNUSED(buf);

	connless_recv_count++;
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	ARG_UNUSED(conn);

	zassert_equal(err, BT_HCI_ERR_SUCCESS, "Connected with 0x%02x", err);
	k_sem_give(&connected_sem);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(reason);

	k_sem_give(&disconnected_sem);
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(level);
	ARG_UNUSED(err);

	security_changed_count++;
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(bonded);

	pairing_complete_count++;
}

/* The connection of the test, also for what a failed test left */
static struct bt_conn *test_conn;

/* The link up without encryption, and the peer known to support SSP */
static void link_up(void)
{
	test_conn = bt_conn_create_br(&peer, BT_BR_CONN_PARAM_DEFAULT);
	zassert_not_null(test_conn, "Creating the connection failed");

	conn_complete();
	zassert_ok(k_sem_take(&connected_sem, K_SECONDS(1)), "No connected callback");
	k_sleep(SETTLE);
	zassert_true(BT_FEAT_HOST_SSP(test_conn->br.features), "Remote features not read");
}

static void link_key_add(uint8_t flags)
{
	struct bt_keys_link_key *key;

	key = bt_keys_get_link_key(&peer);
	zassert_not_null(key, "No link key");
	key->flags = flags;

	link_key_req();
	k_sleep(SETTLE);
}

static void expect_kept(void)
{
	k_sleep(SETTLE);
	zassert_equal(disconnect_count, 0U, "The link was disconnected");
}

static void expect_refused(void)
{
	zassert_ok(k_sem_take(&disconnect_sem, K_SECONDS(1)), "The link was not disconnected");
}

/* The application learns of the end of the link, and lets go of it */
static void link_gone(void)
{
	zassert_ok(k_sem_take(&disconnected_sem, K_SECONDS(1)), "No disconnected callback");
	bt_conn_unref(test_conn);
	test_conn = NULL;
}

/* The controller reports the end of the link */
static void link_down(void)
{
	disconn_complete(BT_HCI_ERR_LOCALHOST_TERM_CONN);
	link_gone();
}

/* Open a channel to the server of the test */
static void chan_open(void)
{
	accept_chan = true;

	peer_conn_req(L2CAP_SCID);
	k_sleep(SETTLE);
	zassert_not_equal(host_cid, 0U, "No connection response");
	zassert_not_equal(host_conf_ident, 0U, "No configuration request");

	peer_conf_rsp();
	peer_conf_req();
	zassert_ok(k_sem_take(&chan_connected_sem, K_SECONDS(1)), "Channel not connected");

	peer_chan_data();
	k_sleep(SETTLE);
	zassert_equal(chan_recv_count, 1U, "%u PDUs received", chan_recv_count);

	accept_count = 0U;
	chan_recv_count = 0U;
}

/* The link at the end of it refused at Encryption Change */
static void refused_at_encrypt_change(void)
{
	link_up();
	key_size = KEY_SIZE_SHORT;
	encrypt_change(BT_HCI_ENCRYPTION_ON_BR_E0);
	expect_refused();
}

/* The default TX pool, held by the test to have the Host run out of buffers */
static struct net_buf *held[CONFIG_BT_BUF_ACL_TX_COUNT];

static void held_release(void)
{
	ARRAY_FOR_EACH(held, i) {
		if (held[i] != NULL) {
			net_buf_unref(held[i]);
			held[i] = NULL;
		}
	}
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	key_size = KEY_SIZE_LONG;
	key_size_hold = false;
	k_sem_reset(&key_size_sem);
	disconnect_hold = false;
	disconnect_count = 0U;
	acl_count = 0U;
	host_cid = 0U;
	host_conf_ident = 0U;
	ncp_hold = false;
	ncp_held = 0U;
	k_sem_reset(&disconnect_sem);
	k_sem_reset(&accept_sem);
	accept_count = 0U;
	chan_recv_count = 0U;
	connless_recv_count = 0U;
	pairing_complete_count = 0U;
	security_changed_count = 0U;
	accept_chan = false;

	zassert_ok(bt_enable(NULL), "Bluetooth init failed");
	zassert_ok(bt_unpair(BT_ID_DEFAULT, NULL), "Removing the keys failed");

	k_sem_reset(&chan_connected_sem);
	k_sem_reset(&connected_sem);
	k_sem_reset(&disconnected_sem);
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	held_release();

	if (disconnect_hold) {
		disconnect_respond();
	}

	zassert_ok(bt_disable(), "Bluetooth disable failed");

	if (test_conn != NULL) {
		bt_conn_unref(test_conn);
		test_conn = NULL;
	}
}

static void *setup(void)
{
	static struct bt_conn_cb conn_cb = {
		.connected = connected,
		.disconnected = disconnected,
		.security_changed = security_changed,
	};
	static struct bt_l2cap_server server = {
		.psm = L2CAP_PSM,
		.sec_level = BT_SECURITY_L2,
		.accept = server_accept,
	};
	static struct bt_l2cap_br_connless_cb connless_cb = {
		.psm = L2CAP_PSM,
		.sec_level = BT_SECURITY_L2,
		.recv = connless_recv,
	};
	static struct bt_conn_auth_info_cb auth_info_cb = {
		.pairing_complete = pairing_complete,
	};

	zassert_ok(bt_conn_cb_register(&conn_cb), "Registering the callbacks failed");
	zassert_ok(bt_l2cap_br_server_register(&server), "Registering the L2CAP server failed");
	zassert_ok(bt_l2cap_br_connless_register(&connless_cb), "Registering connless failed");
	zassert_ok(bt_conn_auth_info_cb_register(&auth_info_cb), "Registering auth info failed");

	return NULL;
}

ZTEST_SUITE(br_enc_refused, NULL, setup, before, after, NULL);

/* A key that is long enough: L2CAP takes what the peer sends */
ZTEST(br_enc_refused, test_accepted)
{
	link_up();
	encrypt_change(BT_HCI_ENCRYPTION_ON_BR_E0);
	expect_kept();

	peer_conn_req(L2CAP_SCID);
	k_sleep(SETTLE);
	zexpect_equal(accept_count, 1U, "%u L2CAP accepts", accept_count);

	peer_connless_data();
	k_sleep(SETTLE);
	zexpect_equal(connless_recv_count, 1U, "%u connectionless PDUs", connless_recv_count);

	link_down();
}

/* A key that is too short at Encryption Change */
ZTEST(br_enc_refused, test_short_key_encrypt_change)
{
	refused_at_encrypt_change();

	peer_conn_req(L2CAP_SCID);
	peer_connless_data();
	k_sleep(SETTLE);
	zexpect_equal(accept_count, 0U, "%u L2CAP accepts", accept_count);
	zexpect_equal(connless_recv_count, 0U, "%u connectionless PDUs", connless_recv_count);

	link_down();
}

/* A key that is too short at Encryption Key Refresh Complete, with a channel open
 * already
 */
ZTEST(br_enc_refused, test_short_key_key_refresh)
{
	uint32_t acl_count_before;

	link_up();
	encrypt_change(BT_HCI_ENCRYPTION_ON_BR_E0);
	expect_kept();
	chan_open();
	acl_count_before = acl_count;

	key_size = KEY_SIZE_SHORT;
	key_refresh_complete();
	expect_refused();

	peer_chan_data();
	peer_conn_req(L2CAP_SCID + 1U);
	peer_connless_data();
	k_sleep(SETTLE);
	zexpect_equal(chan_recv_count, 0U, "%u PDUs received", chan_recv_count);
	zexpect_equal(accept_count, 0U, "%u L2CAP accepts", accept_count);
	zexpect_equal(connless_recv_count, 0U, "%u connectionless PDUs", connless_recv_count);
	zexpect_equal(acl_count, acl_count_before, "Data sent");

	link_down();
}

/* A link that stays below the security level that the Host asked for */
ZTEST(br_enc_refused, test_required_level)
{
	link_up();
	test_conn->required_sec_level = BT_SECURITY_L3;
	encrypt_change(BT_HCI_ENCRYPTION_ON_BR_E0);
	expect_refused();

	peer_conn_req(L2CAP_SCID);
	k_sleep(SETTLE);
	zexpect_equal(accept_count, 0U, "%u L2CAP accepts", accept_count);

	link_down();
}

/* What the peer sends right after the encryption, while the Host reads the key size */
ZTEST(br_enc_refused, test_data_during_key_size_read)
{
	link_up();
	key_size = KEY_SIZE_SHORT;
	key_size_hold = true;
	encrypt_change(BT_HCI_ENCRYPTION_ON_BR_E0);
	zassert_ok(k_sem_take(&key_size_sem, K_SECONDS(1)), "The key size was not read");

	peer_conn_req(L2CAP_SCID);
	k_sleep(SETTLE);
	zexpect_equal(accept_count, 0U, "%u L2CAP accepts", accept_count);

	key_size_respond();
	expect_refused();
	k_sleep(SETTLE);
	zexpect_equal(accept_count, 0U, "%u L2CAP accepts", accept_count);

	link_down();
}

/* Disconnection Complete overtakes the data, so the state of the connection is no
 * longer the disconnecting state when the data is handled.
 */
ZTEST(br_enc_refused, test_data_overtaken_by_disconnection)
{
	link_up();
	key_size = KEY_SIZE_SHORT;
	key_size_hold = true;
	encrypt_change(BT_HCI_ENCRYPTION_ON_BR_E0);
	zassert_ok(k_sem_take(&key_size_sem, K_SECONDS(1)), "The key size was not read");

	peer_conn_req(L2CAP_SCID);
	disconn_complete(BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	key_size_respond();

	link_gone();
	zexpect_equal(accept_count, 0U, "%u L2CAP accepts", accept_count);
}

/* The LE keys stay what they are, and nothing with the refused key is derived */
ZTEST(br_enc_refused, test_short_key_smp)
{
	bt_addr_le_t le_addr;
	struct bt_keys *keys;

	bt_addr_le_copy_addr(&le_addr, &peer, BT_ADDR_LE_PUBLIC);
	keys = bt_keys_get_type(BT_KEYS_LTK_P256, BT_ID_DEFAULT, &le_addr);
	zassert_not_null(keys, "No LE keys");
	keys->enc_size = KEY_SIZE_LONG;

	link_up();
	link_key_add(BT_LINK_KEY_AUTHENTICATED);
	key_size = KEY_SIZE_SHORT;
	encrypt_change(BT_HCI_ENCRYPTION_ON_BR_AES_CCM);
	expect_refused();

	peer_smp_pairing_req(KEY_SIZE_SHORT);
	k_sleep(SETTLE);

	keys = bt_keys_find_addr(BT_ID_DEFAULT, &le_addr);
	zassert_not_null(keys, "The LE keys are gone");
	zexpect_equal(keys->enc_size, KEY_SIZE_LONG, "LE key size %u", keys->enc_size);
	zexpect_equal(pairing_complete_count, 0U, "Pairing completed");

	link_down();
}

/* The same request on a link that is not refused does derive the LTK */
ZTEST(br_enc_refused, test_long_key_smp)
{
	bt_addr_le_t le_addr;
	struct bt_keys *keys;

	link_up();
	link_key_add(BT_LINK_KEY_AUTHENTICATED);
	encrypt_change(BT_HCI_ENCRYPTION_ON_BR_AES_CCM);
	expect_kept();

	peer_smp_pairing_req(KEY_SIZE_LONG);
	k_sleep(SETTLE);

	bt_addr_le_copy_addr(&le_addr, &peer, BT_ADDR_LE_PUBLIC);
	keys = bt_keys_find_addr(BT_ID_DEFAULT, &le_addr);
	zassert_not_null(keys, "No LE keys derived");
	zexpect_equal(keys->enc_size, KEY_SIZE_LONG, "LE key size %u", keys->enc_size);
	zexpect_equal(pairing_complete_count, 1U, "%u pairings", pairing_complete_count);

	link_down();
}

/* The TX buffers used up, and a signaling PDU of the peer waiting for one */
static void signaling_pending(void)
{
	uint32_t accept_count_before;
	uint16_t scid = L2CAP_SCID;

	ARRAY_FOR_EACH(held, i) {
		held[i] = bt_conn_create_pdu_timeout(NULL, 0U, K_NO_WAIT);
	}

	/* Responses use up the controller buffers, and the last one the signaling pool */
	ncp_hold = true;
	for (uint32_t i = 0U; i < ACL_PKTS + CONFIG_BT_L2CAP_SIG_TX_BUF_COUNT; i++) {
		peer_conn_req(scid++);
		k_sleep(SETTLE);
	}
	zassert_true(accept_count > 0U, "The peer reached no server");

	accept_count_before = accept_count;
	peer_conn_req(scid);
	k_sleep(SETTLE);
	zassert_equal(accept_count, accept_count_before, "The request did not wait for a buffer");
}

/* With a buffer available the PDU is handled */
ZTEST(br_enc_refused, test_pending_signaling_accepted)
{
	uint32_t accept_count_before;

	link_up();
	encrypt_change(BT_HCI_ENCRYPTION_ON_BR_E0);
	expect_kept();

	signaling_pending();
	accept_count_before = accept_count;

	held_release();
	k_sleep(SIG_RETRY_WAIT);
	zexpect_true(accept_count > accept_count_before, "The request was not handled");

	link_down();
}

/* On a link that the Host refuses, the PDU is not handled and its buffer returns
 * without the link having to go down.
 */
ZTEST(br_enc_refused, test_pending_signaling_refused)
{
	uint32_t accept_count_before;
	atomic_val_t acl_in_free;

	link_up();
	encrypt_change(BT_HCI_ENCRYPTION_ON_BR_E0);
	expect_kept();

	peer_chan_data_unknown();
	k_sleep(SETTLE);
	acl_in_free = atomic_get(&acl_in_pool->avail_count);

	signaling_pending();
	accept_count_before = accept_count;

	key_size = KEY_SIZE_SHORT;
	key_refresh_complete();
	expect_refused();

	held_release();
	k_sleep(SIG_RETRY_WAIT);
	zexpect_equal(accept_count, accept_count_before, "%u more L2CAP accepts",
		      accept_count - accept_count_before);
	zexpect_equal(atomic_get(&acl_in_pool->avail_count), acl_in_free, "%d ACL buffers free",
		      (int)atomic_get(&acl_in_pool->avail_count));

	link_down();
}

/* A refusal is final: a later event does not tell anyone that the security is fine */
ZTEST(br_enc_refused, test_event_after_refusal)
{
	bt_security_t level;

	refused_at_encrypt_change();
	level = bt_conn_get_security(test_conn);
	security_changed_count = 0U;

	key_size = KEY_SIZE_LONG;
	key_refresh_complete();
	k_sleep(SETTLE);
	encrypt_change(BT_HCI_ENCRYPTION_ON_BR_AES_CCM);
	k_sleep(SETTLE);
	zexpect_equal(security_changed_count, 0U, "%u security changes", security_changed_count);
	zexpect_equal(bt_conn_get_security(test_conn), level, "Security level %d",
		      bt_conn_get_security(test_conn));

	link_down();
}

/* A channel with only the completion of its Configuration Response missing */
static void config_rsp_sent(void)
{
	link_up();
	encrypt_change(BT_HCI_ENCRYPTION_ON_BR_E0);
	expect_kept();

	accept_chan = true;
	peer_conn_req(L2CAP_SCID);
	k_sleep(SETTLE);
	peer_conf_rsp();
	ncp_hold = true;
	peer_conf_req();
	k_sleep(SETTLE);
	zassert_not_equal(ncp_held, 0U, "No configuration response");
}

/* The Configuration Response of the Host completes while the Host waits for the
 * controller to take the disconnection: the channel stays closed.
 */
ZTEST(br_enc_refused, test_config_rsp_sent_while_refusing)
{
	config_rsp_sent();

	key_size = KEY_SIZE_SHORT;
	disconnect_hold = true;
	key_refresh_complete();
	expect_refused();

	ncp_release();
	k_sleep(SETTLE);
	zexpect_not_equal(k_sem_take(&chan_connected_sem, K_NO_WAIT), 0, "Channel connected");

	disconnect_respond();
	link_down();
}

/* And after that */
ZTEST(br_enc_refused, test_config_rsp_sent_after_refusal)
{
	config_rsp_sent();

	key_size = KEY_SIZE_SHORT;
	key_refresh_complete();
	expect_refused();

	ncp_release();
	k_sleep(SETTLE);
	zexpect_not_equal(k_sem_take(&chan_connected_sem, K_NO_WAIT), 0, "Channel connected");

	link_down();
}

/* The Host only tells apart a link that it refused: data for an open channel still
 * arrives on a link that the application is disconnecting.
 */
ZTEST(br_enc_refused, test_local_disconnection)
{
	link_up();
	encrypt_change(BT_HCI_ENCRYPTION_ON_BR_E0);
	expect_kept();
	chan_open();

	zassert_ok(bt_conn_disconnect(test_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN),
		   "Disconnect failed");
	zassert_ok(k_sem_take(&disconnect_sem, K_SECONDS(1)), "The link was not disconnected");

	peer_chan_data();
	k_sleep(SETTLE);
	zexpect_equal(chan_recv_count, 1U, "%u PDUs received", chan_recv_count);

	link_down();
}

/* And so does what the peer sent before it disconnected */
ZTEST(br_enc_refused, test_peer_disconnection)
{
	link_up();
	encrypt_change(BT_HCI_ENCRYPTION_ON_BR_E0);
	expect_kept();
	chan_open();

	key_size_hold = true;
	key_refresh_complete();
	zassert_ok(k_sem_take(&key_size_sem, K_SECONDS(1)), "The key size was not read");

	peer_chan_data();
	disconn_complete(BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	key_size_respond();

	link_gone();
	zexpect_equal(chan_recv_count, 1U, "%u PDUs received", chan_recv_count);
}

/* Nor when the application keeps the connection object and the peer comes back */
ZTEST(br_enc_refused, test_next_connection_kept_object)
{
	refused_at_encrypt_change();
	disconn_complete(BT_HCI_ERR_LOCALHOST_TERM_CONN);
	zassert_ok(k_sem_take(&disconnected_sem, K_SECONDS(1)), "No disconnected callback");

	conn_request();
	zassert_ok(k_sem_take(&accept_sem, K_SECONDS(1)), "The connection was not accepted");
	conn_complete();
	zassert_ok(k_sem_take(&connected_sem, K_SECONDS(1)), "No connected callback");
	k_sleep(SETTLE);

	key_size = KEY_SIZE_LONG;
	encrypt_change(BT_HCI_ENCRYPTION_ON_BR_E0);
	k_sleep(SETTLE);

	peer_conn_req(L2CAP_SCID);
	k_sleep(SETTLE);
	zexpect_equal(accept_count, 1U, "%u L2CAP accepts", accept_count);
}

/* The refusal does not outlast the connection */
ZTEST(br_enc_refused, test_next_connection)
{
	refused_at_encrypt_change();
	link_down();

	key_size = KEY_SIZE_LONG;
	disconnect_count = 0U;
	link_up();
	encrypt_change(BT_HCI_ENCRYPTION_ON_BR_E0);
	expect_kept();

	peer_conn_req(L2CAP_SCID);
	k_sleep(SETTLE);
	zexpect_equal(accept_count, 1U, "%u L2CAP accepts", accept_count);

	link_down();
}
