/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <zephyr/types.h>
#include <string.h>
#include <version.h>

#include <soc.h>
#include <toolchain.h>
#include <errno.h>
#include <atomic.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_vs.h>
#include <bluetooth/buf.h>
#include <bluetooth/bluetooth.h>
#include <misc/byteorder.h>
#include <misc/util.h>

#include "util/util.h"
#include "hal/ecb.h"
#include "ll_sw/pdu.h"
#include "ll_sw/ctrl.h"
#include "ll.h"
#include "hci_internal.h"

#if defined(CONFIG_BT_CTLR_DTM_HCI)
#include "ll_sw/ll_test.h"
#endif /* CONFIG_BT_CTLR_DTM_HCI */

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#include "common/log.h"
#include "hal/debug.h"

/* opcode of the HCI command currently being processed. The opcode is stored
 * by hci_cmd_handle() and then used during the creation of cmd complete and
 * cmd status events to avoid passing it up the call chain.
 */
static u16_t _opcode;

#if CONFIG_BT_CTLR_DUP_FILTER_LEN > 0
/* Scan duplicate filter */
struct dup {
	u8_t         mask;
	bt_addr_le_t addr;
};
static struct dup dup_filter[CONFIG_BT_CTLR_DUP_FILTER_LEN];
static s32_t dup_count;
static u32_t dup_curr;
#endif

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
s32_t    hci_hbuf_total;
u32_t    hci_hbuf_sent;
u32_t    hci_hbuf_acked;
u16_t    hci_hbuf_pend[CONFIG_BT_MAX_CONN];
atomic_t hci_state_mask;
static struct k_poll_signal *hbuf_signal;
#endif

#if defined(CONFIG_BT_CONN)
static u32_t conn_count;
#endif

#define DEFAULT_EVENT_MASK           0x1fffffffffff
#define DEFAULT_EVENT_MASK_PAGE_2    0x0
#define DEFAULT_LE_EVENT_MASK 0x1f

static u64_t event_mask = DEFAULT_EVENT_MASK;
static u64_t event_mask_page_2 = DEFAULT_EVENT_MASK_PAGE_2;
static u64_t le_event_mask = DEFAULT_LE_EVENT_MASK;

static void evt_create(struct net_buf *buf, u8_t evt, u8_t len)
{
	struct bt_hci_evt_hdr *hdr;

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->evt = evt;
	hdr->len = len;
}

static void *cmd_complete(struct net_buf **buf, u8_t plen)
{
	struct bt_hci_evt_cmd_complete *cc;

	*buf = bt_buf_get_cmd_complete(K_FOREVER);

	evt_create(*buf, BT_HCI_EVT_CMD_COMPLETE, sizeof(*cc) + plen);

	cc = net_buf_add(*buf, sizeof(*cc));
	cc->ncmd = 1;
	cc->opcode = sys_cpu_to_le16(_opcode);

	return net_buf_add(*buf, plen);
}

#if defined(CONFIG_BT_CONN)
static struct net_buf *cmd_status(u8_t status)
{
	struct bt_hci_evt_cmd_status *cs;
	struct net_buf *buf;

	buf = bt_buf_get_cmd_complete(K_FOREVER);
	evt_create(buf, BT_HCI_EVT_CMD_STATUS, sizeof(*cs));

	cs = net_buf_add(buf, sizeof(*cs));
	cs->status = status;
	cs->ncmd = 1;
	cs->opcode = sys_cpu_to_le16(_opcode);

	return buf;
}
#endif

static void *meta_evt(struct net_buf *buf, u8_t subevt, u8_t melen)
{
	struct bt_hci_evt_le_meta_event *me;

	evt_create(buf, BT_HCI_EVT_LE_META_EVENT, sizeof(*me) + melen);
	me = net_buf_add(buf, sizeof(*me));
	me->subevent = subevt;

	return net_buf_add(buf, melen);
}

#if defined(CONFIG_BT_CONN)
static void disconnect(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_disconnect *cmd = (void *)buf->data;
	u16_t handle;
	u32_t status;

	handle = sys_le16_to_cpu(cmd->handle);
	status = ll_terminate_ind_send(handle, cmd->reason);

	*evt = cmd_status((!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED);
}

static void read_remote_ver_info(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_read_remote_version_info *cmd = (void *)buf->data;
	u16_t handle;
	u32_t status;

	handle = sys_le16_to_cpu(cmd->handle);
	status = ll_version_ind_send(handle);

	*evt = cmd_status((!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED);
}
#endif /* CONFIG_BT_CONN */

static int link_control_cmd_handle(u16_t  ocf, struct net_buf *cmd,
				   struct net_buf **evt)
{
	switch (ocf) {
#if defined(CONFIG_BT_CONN)
	case BT_OCF(BT_HCI_OP_DISCONNECT):
		disconnect(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_READ_REMOTE_VERSION_INFO):
		read_remote_ver_info(cmd, evt);
		break;
#endif /* CONFIG_BT_CONN */
	default:
		return -EINVAL;
	}

	return 0;
}

static void set_event_mask(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_set_event_mask *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;

	event_mask = sys_get_le64(cmd->events);

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = 0x00;
}

static void set_event_mask_page_2(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_set_event_mask_page_2 *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;

	event_mask_page_2 = sys_get_le64(cmd->events_page_2);

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = 0x00;
}

static void reset(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_evt_cc_status *ccst;

#if CONFIG_BT_CTLR_DUP_FILTER_LEN > 0
	dup_count = -1;
#endif
	/* reset event masks */
	event_mask = DEFAULT_EVENT_MASK;
	event_mask_page_2 = DEFAULT_EVENT_MASK_PAGE_2;
	le_event_mask = DEFAULT_LE_EVENT_MASK;

	if (buf) {
		ll_reset();
		ccst = cmd_complete(evt, sizeof(*ccst));
		ccst->status = 0x00;
	}
#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
	hci_hbuf_total = 0;
	hci_hbuf_sent = 0;
	hci_hbuf_acked = 0;
	memset(hci_hbuf_pend, 0, sizeof(hci_hbuf_pend));
	conn_count = 0;
	if (buf) {
		atomic_set_bit(&hci_state_mask, HCI_STATE_BIT_RESET);
		k_poll_signal(hbuf_signal, 0x0);
	}
#endif
}

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
static void set_ctl_to_host_flow(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_set_ctl_to_host_flow *cmd = (void *)buf->data;
	u8_t flow_enable = cmd->flow_enable;
	struct bt_hci_evt_cc_status *ccst;

	ccst = cmd_complete(evt, sizeof(*ccst));

	/* require host buffer size before enabling flow control, and
	 * disallow if any connections are up
	 */
	if (!hci_hbuf_total || conn_count) {
		ccst->status = BT_HCI_ERR_CMD_DISALLOWED;
		return;
	} else {
		ccst->status = 0x00;
	}

	switch (flow_enable) {
	case BT_HCI_CTL_TO_HOST_FLOW_DISABLE:
		if (hci_hbuf_total < 0) {
			/* already disabled */
			return;
		}
		break;
	case BT_HCI_CTL_TO_HOST_FLOW_ENABLE:
		if (hci_hbuf_total > 0) {
			/* already enabled */
			return;
		}
		break;
	default:
		ccst->status = BT_HCI_ERR_INVALID_PARAM;
		return;
	}

	hci_hbuf_sent = 0;
	hci_hbuf_acked = 0;
	memset(hci_hbuf_pend, 0, sizeof(hci_hbuf_pend));
	hci_hbuf_total = -hci_hbuf_total;
}

static void host_buffer_size(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_host_buffer_size *cmd = (void *)buf->data;
	u16_t acl_pkts = sys_le16_to_cpu(cmd->acl_pkts);
	u16_t acl_mtu = sys_le16_to_cpu(cmd->acl_mtu);
	struct bt_hci_evt_cc_status *ccst;

	ccst = cmd_complete(evt, sizeof(*ccst));

	if (hci_hbuf_total) {
		ccst->status = BT_HCI_ERR_CMD_DISALLOWED;
		return;
	}
	/* fragmentation from controller to host not supported, require
	 * ACL MTU to be at least the LL MTU
	 */
	if (acl_mtu < RADIO_LL_LENGTH_OCTETS_RX_MAX) {
		ccst->status = BT_HCI_ERR_INVALID_PARAM;
		return;
	}

	BT_DBG("FC: host buf size: %d", acl_pkts);
	hci_hbuf_total = -acl_pkts;
}

static void host_num_completed_packets(struct net_buf *buf,
				       struct net_buf **evt)
{
	struct bt_hci_cp_host_num_completed_packets *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;
	u32_t count = 0;
	int i;

	/* special case, no event returned except for error conditions */
	if (hci_hbuf_total <= 0) {
		ccst = cmd_complete(evt, sizeof(*ccst));
		ccst->status = BT_HCI_ERR_CMD_DISALLOWED;
		return;
	} else if (!conn_count) {
		ccst = cmd_complete(evt, sizeof(*ccst));
		ccst->status = BT_HCI_ERR_INVALID_PARAM;
		return;
	}

	/* leave *evt == NULL so no event is generated */
	for (i = 0; i < cmd->num_handles; i++) {
		u16_t h = sys_le16_to_cpu(cmd->h[i].handle);
		u16_t c = sys_le16_to_cpu(cmd->h[i].count);

		if ((h >= ARRAY_SIZE(hci_hbuf_pend)) ||
		    (c > hci_hbuf_pend[h])) {
			ccst = cmd_complete(evt, sizeof(*ccst));
			ccst->status = BT_HCI_ERR_INVALID_PARAM;
			return;
		}

		hci_hbuf_pend[h] -= c;
		count += c;
	}

	BT_DBG("FC: acked: %d", count);
	hci_hbuf_acked += count;
	k_poll_signal(hbuf_signal, 0x0);
}
#endif

#if defined(CONFIG_BT_CTLR_LE_PING)
static void read_auth_payload_timeout(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_read_auth_payload_timeout *cmd = (void *)buf->data;
	struct bt_hci_rp_read_auth_payload_timeout *rp;
	u32_t status;
	u16_t handle;
	u16_t auth_payload_timeout;

	handle = sys_le16_to_cpu(cmd->handle);

	status = ll_apto_get(handle, &auth_payload_timeout);

	rp = cmd_complete(evt, sizeof(*rp));
	rp->status = (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED;
	rp->handle = sys_cpu_to_le16(handle);
	rp->auth_payload_timeout = sys_cpu_to_le16(auth_payload_timeout);
}

static void write_auth_payload_timeout(struct net_buf *buf,
				       struct net_buf **evt)
{
	struct bt_hci_cp_write_auth_payload_timeout *cmd = (void *)buf->data;
	struct bt_hci_rp_write_auth_payload_timeout *rp;
	u32_t status;
	u16_t handle;
	u16_t auth_payload_timeout;

	handle = sys_le16_to_cpu(cmd->handle);
	auth_payload_timeout = sys_le16_to_cpu(cmd->auth_payload_timeout);

	status = ll_apto_set(handle, auth_payload_timeout);

	rp = cmd_complete(evt, sizeof(*rp));
	rp->status = (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED;
	rp->handle = sys_cpu_to_le16(handle);
}
#endif /* CONFIG_BT_CTLR_LE_PING */

static void read_tx_power_level(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_read_tx_power_level *cmd = (void *)buf->data;
	struct bt_hci_rp_read_tx_power_level *rp;
	u32_t status;
	u16_t handle;
	u8_t  type;

	handle = sys_le16_to_cpu(cmd->handle);
	type = cmd->type;

	rp = cmd_complete(evt, sizeof(*rp));

	status = ll_tx_power_level_get(handle, type, &rp->tx_power_level);
	rp->status = (!status) ? 0x00 : BT_HCI_ERR_UNKNOWN_CONN_ID;
	rp->handle = sys_cpu_to_le16(handle);
}

static int ctrl_bb_cmd_handle(u16_t  ocf, struct net_buf *cmd,
			      struct net_buf **evt)
{
	switch (ocf) {
	case BT_OCF(BT_HCI_OP_SET_EVENT_MASK):
		set_event_mask(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_RESET):
		reset(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_SET_EVENT_MASK_PAGE_2):
		set_event_mask_page_2(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_READ_TX_POWER_LEVEL):
		read_tx_power_level(cmd, evt);
		break;

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
	case BT_OCF(BT_HCI_OP_SET_CTL_TO_HOST_FLOW):
		set_ctl_to_host_flow(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_HOST_BUFFER_SIZE):
		host_buffer_size(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_HOST_NUM_COMPLETED_PACKETS):
		host_num_completed_packets(cmd, evt);
		break;
#endif

#if defined(CONFIG_BT_CTLR_LE_PING)
	case BT_OCF(BT_HCI_OP_READ_AUTH_PAYLOAD_TIMEOUT):
		read_auth_payload_timeout(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_WRITE_AUTH_PAYLOAD_TIMEOUT):
		write_auth_payload_timeout(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_LE_PING */

	default:
		return -EINVAL;
	}

	return 0;
}

static void read_local_version_info(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_read_local_version_info *rp;

	rp = cmd_complete(evt, sizeof(*rp));

	rp->status = 0x00;
	rp->hci_version = BT_HCI_VERSION_5_0;
	rp->hci_revision = sys_cpu_to_le16(0);
	rp->lmp_version = RADIO_BLE_VERSION_NUMBER;
	rp->manufacturer = sys_cpu_to_le16(RADIO_BLE_COMPANY_ID);
	rp->lmp_subversion = sys_cpu_to_le16(RADIO_BLE_SUB_VERSION_NUMBER);
}

static void read_supported_commands(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_read_supported_commands *rp;

	rp = cmd_complete(evt, sizeof(*rp));

	rp->status = 0x00;
	memset(&rp->commands[0], 0, sizeof(rp->commands));

	/* Read Remote Version Info. */
	rp->commands[2] |= BIT(7);
	/* Set Event Mask, and Reset. */
	rp->commands[5] |= BIT(6) | BIT(7);
	/* Read TX Power Level. */
	rp->commands[10] |= BIT(2);
#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
	/* Set FC, Host Buffer Size and Host Num Completed */
	rp->commands[10] |= BIT(5) | BIT(6) | BIT(7);
#endif
	/* Read Local Version Info, Read Local Supported Features. */
	rp->commands[14] |= BIT(3) | BIT(5);
	/* Read BD ADDR. */
	rp->commands[15] |= BIT(1);
#if defined(CONFIG_BT_CTLR_CONN_RSSI)
	/* Read RSSI. */
	rp->commands[15] |= BIT(5);
#endif /* CONFIG_BT_CTLR_CONN_RSSI */
	/* Set Event Mask Page 2 */
	rp->commands[22] |= BIT(2);
	/* LE Set Event Mask, LE Read Buffer Size, LE Read Local Supp Feats,
	 * Set Random Addr
	 */
	rp->commands[25] |= BIT(0) | BIT(1) | BIT(2) | BIT(4);
	/* LE Read WL Size, LE Clear WL */
	rp->commands[26] |= BIT(6) | BIT(7);
	/* LE Add Dev to WL, LE Remove Dev from WL */
	rp->commands[27] |= BIT(0) | BIT(1);
	/* LE Encrypt, LE Rand */
	rp->commands[27] |= BIT(6) | BIT(7);
	/* LE Read Supported States */
	rp->commands[28] |= BIT(3);
#if defined(CONFIG_BT_BROADCASTER)
	/* LE Set Adv Params, LE Read Adv Channel TX Power, LE Set Adv Data */
	rp->commands[25] |= BIT(5) | BIT(6) | BIT(7);
	/* LE Set Scan Response Data, LE Set Adv Enable */
	rp->commands[26] |= BIT(0) | BIT(1);
#endif
#if defined(CONFIG_BT_OBSERVER)
	/* LE Set Scan Params, LE Set Scan Enable */
	rp->commands[26] |= BIT(2) | BIT(3);
#endif
#if defined(CONFIG_BT_CENTRAL)
	/* LE Create Connection, LE Create Connection Cancel */
	rp->commands[26] |= BIT(4) | BIT(5);
	/* Set Host Channel Classification */
	rp->commands[27] |= BIT(3);
#if defined(CONFIG_BT_CTLR_LE_ENC)
	/* LE Start Encryption */
	rp->commands[28] |= BIT(0);
#endif /* CONFIG_BT_CTLR_LE_ENC */
#endif /* CONFIG_BT_CENTRAL */
#if defined(CONFIG_BT_PERIPHERAL)
#if defined(CONFIG_BT_CTLR_LE_ENC)
	/* LE LTK Request Reply, LE LTK Request Negative Reply */
	rp->commands[28] |= BIT(1) | BIT(2);
#endif /* CONFIG_BT_CTLR_LE_ENC */
#endif
#if defined(CONFIG_BT_CTLR_DTM_HCI)
	/* LE RX Test, LE TX Test, LE Test End */
	rp->commands[28] |= BIT(4) | BIT(5) | BIT(6);
	/* LE Enhanced RX Test. */
	rp->commands[35] |= BIT(7);
	/* LE Enhanced TX Test. */
	rp->commands[36] |= BIT(0);
#endif /* CONFIG_BT_CTLR_DTM_HCI */
#if defined(CONFIG_BT_CONN)
	/* Disconnect. */
	rp->commands[0] |= BIT(5);
	/* LE Connection Update, LE Read Channel Map, LE Read Remote Features */
	rp->commands[27] |= BIT(2) | BIT(4) | BIT(5);
	/* LE Remote Conn Param Req and Neg Reply */
	rp->commands[33] |= BIT(4) | BIT(5);
#if defined(CONFIG_BT_CTLR_LE_PING)
	/* Read and Write authenticated payload timeout */
	rp->commands[32] |= BIT(4) | BIT(5);
#endif
#endif /* CONFIG_BT_CONN */
#if defined(CONFIG_BT_CTLR_PRIVACY)
	/* LE resolving list commands, LE Read Peer RPA */
	rp->commands[34] |= BIT(3) | BIT(4) | BIT(5) | BIT(6) | BIT(7);
	/* LE Read Local RPA, LE Set AR Enable, Set RPA Timeout */
	rp->commands[35] |= BIT(0) | BIT(1) | BIT(2);
	/* LE Set Privacy Mode */
	rp->commands[39] |= BIT(2);
#endif /* CONFIG_BT_CTLR_PRIVACY */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	/* LE Set Data Length, and LE Read Suggested Data Length. */
	rp->commands[33] |= BIT(6) | BIT(7);
	/* LE Write Suggested Data Length. */
	rp->commands[34] |= BIT(0);
	/* LE Read Maximum Data Length. */
	rp->commands[35] |= BIT(3);
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_HCI_RAW) && defined(CONFIG_BT_TINYCRYPT_ECC)
	/* LE Read Local P256 Public Key and LE Generate DH Key*/
	rp->commands[34] |= BIT(1) | BIT(2);
#endif
	/* LE Read TX Power. */
	rp->commands[38] |= BIT(7);
}

static void read_local_features(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_read_local_features *rp;

	rp = cmd_complete(evt, sizeof(*rp));

	rp->status = 0x00;
	memset(&rp->features[0], 0x00, sizeof(rp->features));
	/* BR/EDR not supported and LE supported */
	rp->features[4] = (1 << 5) | (1 << 6);
}

static void read_bd_addr(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_read_bd_addr *rp;

	rp = cmd_complete(evt, sizeof(*rp));

	rp->status = 0x00;
	ll_addr_get(0, &rp->bdaddr.val[0]);
}

static int info_cmd_handle(u16_t  ocf, struct net_buf *cmd,
			   struct net_buf **evt)
{
	switch (ocf) {
	case BT_OCF(BT_HCI_OP_READ_LOCAL_VERSION_INFO):
		read_local_version_info(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_READ_SUPPORTED_COMMANDS):
		read_supported_commands(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_READ_LOCAL_FEATURES):
		read_local_features(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_READ_BD_ADDR):
		read_bd_addr(cmd, evt);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
static void read_rssi(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_read_rssi *cmd = (void *)buf->data;
	struct bt_hci_rp_read_rssi *rp;
	u32_t status;
	u16_t handle;

	handle = sys_le16_to_cpu(cmd->handle);

	rp = cmd_complete(evt, sizeof(*rp));

	status = ll_rssi_get(handle, &rp->rssi);

	rp->status = (!status) ? 0x00 : BT_HCI_ERR_UNKNOWN_CONN_ID;
	rp->handle = sys_cpu_to_le16(handle);
	/* The Link Layer currently returns RSSI as an absolute value */
	rp->rssi = (!status) ? -rp->rssi : 127;
}
#endif /* CONFIG_BT_CTLR_CONN_RSSI */

static int status_cmd_handle(u16_t  ocf, struct net_buf *cmd,
			     struct net_buf **evt)
{
	switch (ocf) {
#if defined(CONFIG_BT_CTLR_CONN_RSSI)
	case BT_OCF(BT_HCI_OP_READ_RSSI):
		read_rssi(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_CONN_RSSI */

	default:
		return -EINVAL;
	}

	return 0;
}

static void le_set_event_mask(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_set_event_mask *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;

	le_event_mask = sys_get_le64(cmd->events);

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = 0x00;
}

static void le_read_buffer_size(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_le_read_buffer_size *rp;

	rp = cmd_complete(evt, sizeof(*rp));

	rp->status = 0x00;

	rp->le_max_len = sys_cpu_to_le16(RADIO_PACKET_TX_DATA_SIZE);
	rp->le_max_num = RADIO_PACKET_COUNT_TX_MAX;
}

static void le_read_local_features(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_le_read_local_features *rp;

	rp = cmd_complete(evt, sizeof(*rp));

	rp->status = 0x00;

	memset(&rp->features[0], 0x00, sizeof(rp->features));
	rp->features[0] = RADIO_BLE_FEAT & 0xFF;
	rp->features[1] = (RADIO_BLE_FEAT >> 8)  & 0xFF;
	rp->features[2] = (RADIO_BLE_FEAT >> 16)  & 0xFF;
}

static void le_set_random_address(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_random_address *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;

	ll_addr_set(1, &cmd->bdaddr.val[0]);

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = 0x00;
}

static void le_read_wl_size(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_le_read_wl_size *rp;

	rp = cmd_complete(evt, sizeof(*rp));
	rp->status = 0x00;

	rp->wl_size = ll_wl_size_get();
}

static void le_clear_wl(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_evt_cc_status *ccst;

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = ll_wl_clear();
}

static void le_add_dev_to_wl(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_add_dev_to_wl *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;
	u32_t status;

	status = ll_wl_add(&cmd->addr);

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = status;
}

static void le_rem_dev_from_wl(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_rem_dev_from_wl *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;
	u32_t status;

	status = ll_wl_remove(&cmd->addr);

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = status;
}

static void le_encrypt(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_encrypt *cmd = (void *)buf->data;
	struct bt_hci_rp_le_encrypt *rp;
	u8_t enc_data[16];

	ecb_encrypt(cmd->key, cmd->plaintext, enc_data, NULL);

	rp = cmd_complete(evt, sizeof(*rp));

	rp->status = 0x00;
	memcpy(rp->enc_data, enc_data, 16);
}

static void le_rand(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_le_rand *rp;
	u8_t count = sizeof(rp->rand);

	rp = cmd_complete(evt, sizeof(*rp));
	rp->status = 0x00;

	bt_rand(rp->rand, count);
}

static void le_read_supp_states(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_le_read_supp_states *rp;
	u64_t states = 0;

	rp = cmd_complete(evt, sizeof(*rp));
	rp->status = 0x00;

#define ST_ADV (BIT64(0)  | BIT64(1)  | BIT64(8)  | BIT64(9)  | BIT64(12) | \
		BIT64(13) | BIT64(16) | BIT64(17) | BIT64(18) | BIT64(19) | \
		BIT64(20) | BIT64(21))

#define ST_SCA (BIT64(4)  | BIT64(5)  | BIT64(8)  | BIT64(9)  | BIT64(10) | \
		BIT64(11) | BIT64(12) | BIT64(13) | BIT64(14) | BIT64(15) | \
		BIT64(22) | BIT64(23) | BIT64(24) | BIT64(25) | BIT64(26) | \
		BIT64(27) | BIT64(30) | BIT64(31))

#define ST_SLA (BIT64(2)  | BIT64(3)  | BIT64(7)  | BIT64(10) | BIT64(11) | \
		BIT64(14) | BIT64(15) | BIT64(20) | BIT64(21) | BIT64(26) | \
		BIT64(27) | BIT64(29) | BIT64(30) | BIT64(31) | BIT64(32) | \
		BIT64(33) | BIT64(34) | BIT64(35) | BIT64(36) | BIT64(37) | \
		BIT64(38) | BIT64(39) | BIT64(40) | BIT64(41))

#define ST_MAS (BIT64(6)  | BIT64(16) | BIT64(17) | BIT64(18) | BIT64(19) | \
		BIT64(22) | BIT64(23) | BIT64(24) | BIT64(25) | BIT64(28) | \
		BIT64(32) | BIT64(33) | BIT64(34) | BIT64(35) | BIT64(36) | \
		BIT64(37) | BIT64(41))

#if defined(CONFIG_BT_BROADCASTER)
	states |= ST_ADV;
#else
	states &= ~ST_ADV;
#endif
#if defined(CONFIG_BT_OBSERVER)
	states |= ST_SCA;
#else
	states &= ~ST_SCA;
#endif
#if defined(CONFIG_BT_PERIPHERAL)
	states |= ST_SLA;
#else
	states &= ~ST_SLA;
#endif
#if defined(CONFIG_BT_CENTRAL)
	states |= ST_MAS;
#else
	states &= ~ST_MAS;
#endif
	/* All states and combinations supported except:
	 * Initiating State + Passive Scanning
	 * Initiating State + Active Scanning
	 */
	states &= ~(BIT64(22) | BIT64(23));
	BT_DBG("states: 0x%08x%08x", (u32_t)(states >> 32),
				     (u32_t)(states & 0xffffffff));
	sys_put_le64(states, rp->le_states);
}

#if defined(CONFIG_BT_BROADCASTER)
static void le_set_adv_param(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_adv_param *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;
	u16_t min_interval;
	u8_t status;

	min_interval = sys_le16_to_cpu(cmd->min_interval);

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	status = ll_adv_params_set(0, 0, min_interval, cmd->type,
				   cmd->own_addr_type, cmd->direct_addr.type,
				   &cmd->direct_addr.a.val[0], cmd->channel_map,
				   cmd->filter_policy, 0, 0, 0, 0, 0, 0);
#else /* !CONFIG_BT_CTLR_ADV_EXT */
	status = ll_adv_params_set(min_interval, cmd->type,
				   cmd->own_addr_type, cmd->direct_addr.type,
				   &cmd->direct_addr.a.val[0], cmd->channel_map,
				   cmd->filter_policy);
#endif /* !CONFIG_BT_CTLR_ADV_EXT */

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = status;
}

static void le_read_adv_chan_tx_power(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_le_read_chan_tx_power *rp;

	rp = cmd_complete(evt, sizeof(*rp));

	rp->status = 0x00;

	rp->tx_power_level = 0;
}

static void le_set_adv_data(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_adv_data *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;

	ll_adv_data_set(cmd->len, &cmd->data[0]);

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = 0x00;
}

static void le_set_scan_rsp_data(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_scan_rsp_data *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;

	ll_scan_data_set(cmd->len, &cmd->data[0]);

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = 0x00;
}

static void le_set_adv_enable(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_adv_enable *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;
	u32_t status;

	status = ll_adv_enable(cmd->enable);

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED;
}
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_OBSERVER)
static void le_set_scan_param(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_scan_param *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;
	u16_t interval;
	u16_t window;
	u32_t status;

	interval = sys_le16_to_cpu(cmd->interval);
	window = sys_le16_to_cpu(cmd->window);

	status = ll_scan_params_set(cmd->scan_type, interval, window,
				    cmd->addr_type, cmd->filter_policy);

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED;
}

static void le_set_scan_enable(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_scan_enable *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;
	u32_t status;

#if CONFIG_BT_CTLR_DUP_FILTER_LEN > 0
	/* initialize duplicate filtering */
	if (cmd->enable && cmd->filter_dup) {
		dup_count = 0;
		dup_curr = 0;
	} else {
		dup_count = -1;
	}
#endif
	status = ll_scan_enable(cmd->enable);

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED;
}
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_CONN)
#if defined(CONFIG_BT_CENTRAL)
static void le_create_connection(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_create_conn *cmd = (void *)buf->data;
	u16_t supervision_timeout;
	u16_t conn_interval_max;
	u16_t scan_interval;
	u16_t conn_latency;
	u16_t scan_window;
	u32_t status;

	scan_interval = sys_le16_to_cpu(cmd->scan_interval);
	scan_window = sys_le16_to_cpu(cmd->scan_window);
	conn_interval_max = sys_le16_to_cpu(cmd->conn_interval_max);
	conn_latency = sys_le16_to_cpu(cmd->conn_latency);
	supervision_timeout = sys_le16_to_cpu(cmd->supervision_timeout);

	status = ll_create_connection(scan_interval, scan_window,
				      cmd->filter_policy,
				      cmd->peer_addr.type,
				      &cmd->peer_addr.a.val[0],
				      cmd->own_addr_type, conn_interval_max,
				      conn_latency, supervision_timeout);

	*evt = cmd_status((!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED);
}

static void le_create_conn_cancel(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_evt_cc_status *ccst;
	u32_t status;

	status = ll_connect_disable();

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED;
}

static void le_set_host_chan_classif(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_host_chan_classif *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;
	u32_t status;

	status = ll_chm_update(&cmd->ch_map[0]);

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED;
}

#if defined(CONFIG_BT_CTLR_LE_ENC)
static void le_start_encryption(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_start_encryption *cmd = (void *)buf->data;
	u32_t status;
	u16_t handle;

	handle = sys_le16_to_cpu(cmd->handle);
	status = ll_enc_req_send(handle,
				 (u8_t *)&cmd->rand,
				 (u8_t *)&cmd->ediv,
				 &cmd->ltk[0]);

	*evt = cmd_status((!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED);
}
#endif /* CONFIG_BT_CTLR_LE_ENC */
#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_PERIPHERAL)
#if defined(CONFIG_BT_CTLR_LE_ENC)
static void le_ltk_req_reply(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_ltk_req_reply *cmd = (void *)buf->data;
	struct bt_hci_rp_le_ltk_req_reply *rp;
	u32_t status;
	u16_t handle;

	handle = sys_le16_to_cpu(cmd->handle);
	status = ll_start_enc_req_send(handle, 0x00, &cmd->ltk[0]);

	rp = cmd_complete(evt, sizeof(*rp));
	rp->status = (!status) ?  0x00 : BT_HCI_ERR_CMD_DISALLOWED;
	rp->handle = sys_cpu_to_le16(handle);
}

static void le_ltk_req_neg_reply(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_ltk_req_neg_reply *cmd = (void *)buf->data;
	struct bt_hci_rp_le_ltk_req_neg_reply *rp;
	u32_t status;
	u16_t handle;

	handle = sys_le16_to_cpu(cmd->handle);
	status = ll_start_enc_req_send(handle, BT_HCI_ERR_PIN_OR_KEY_MISSING,
				       NULL);

	rp = cmd_complete(evt, sizeof(*rp));
	rp->status = (!status) ?  0x00 : BT_HCI_ERR_CMD_DISALLOWED;
	rp->handle = sys_le16_to_cpu(handle);
}
#endif /* CONFIG_BT_CTLR_LE_ENC */
#endif /* CONFIG_BT_PERIPHERAL */

static void le_read_remote_features(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_read_remote_features *cmd = (void *)buf->data;
	u32_t status;
	u16_t handle;

	handle = sys_le16_to_cpu(cmd->handle);
	status = ll_feature_req_send(handle);

	*evt = cmd_status((!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED);
}
static void le_read_chan_map(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_read_chan_map *cmd = (void *)buf->data;
	struct bt_hci_rp_le_read_chan_map *rp;
	u32_t status;
	u16_t handle;

	handle = sys_le16_to_cpu(cmd->handle);

	rp = cmd_complete(evt, sizeof(*rp));
	status = ll_chm_get(handle, rp->ch_map);

	rp->status = (!status) ?  0x00 : BT_HCI_ERR_UNKNOWN_CONN_ID;
	rp->handle = sys_le16_to_cpu(handle);
}

static void le_conn_update(struct net_buf *buf, struct net_buf **evt)
{
	struct hci_cp_le_conn_update *cmd = (void *)buf->data;
	u16_t supervision_timeout;
	u16_t conn_interval_max;
	u16_t conn_latency;
	u32_t status;
	u16_t handle;

	handle = sys_le16_to_cpu(cmd->handle);
	conn_interval_max = sys_le16_to_cpu(cmd->conn_interval_max);
	conn_latency = sys_le16_to_cpu(cmd->conn_latency);
	supervision_timeout = sys_le16_to_cpu(cmd->supervision_timeout);

	/** @todo if peer supports LE Conn Param Req,
	* use Req cmd (1) instead of Initiate cmd (0).
	*/
	status = ll_conn_update(handle, 0, 0, conn_interval_max,
				conn_latency, supervision_timeout);

	*evt = cmd_status((!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED);
}

static void le_conn_param_req_reply(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_conn_param_req_reply *cmd = (void *)buf->data;
	struct bt_hci_rp_le_conn_param_req_reply *rp;
	u16_t interval_max;
	u16_t latency;
	u16_t timeout;
	u32_t status;
	u16_t handle;

	handle = sys_le16_to_cpu(cmd->handle);
	interval_max = sys_le16_to_cpu(cmd->interval_max);
	latency = sys_le16_to_cpu(cmd->latency);
	timeout = sys_le16_to_cpu(cmd->timeout);

	status = ll_conn_update(handle, 2, 0, interval_max, latency,
				timeout);

	rp = cmd_complete(evt, sizeof(*rp));
	rp->status = (!status) ?  0x00 : BT_HCI_ERR_CMD_DISALLOWED;
	rp->handle = sys_cpu_to_le16(handle);
}

static void le_conn_param_req_neg_reply(struct net_buf *buf,
					struct net_buf **evt)
{
	struct bt_hci_cp_le_conn_param_req_neg_reply *cmd = (void *)buf->data;
	struct bt_hci_rp_le_conn_param_req_neg_reply *rp;
	u32_t status;
	u16_t handle;

	handle = sys_le16_to_cpu(cmd->handle);
	status = ll_conn_update(handle, 2, cmd->reason, 0, 0, 0);

	rp = cmd_complete(evt, sizeof(*rp));
	rp->status = (!status) ?  0x00 : BT_HCI_ERR_CMD_DISALLOWED;
	rp->handle = sys_cpu_to_le16(handle);
}

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
static void le_set_data_len(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_data_len *cmd = (void *)buf->data;
	struct bt_hci_rp_le_set_data_len *rp;
	u32_t status;
	u16_t tx_octets;
	u16_t tx_time;
	u16_t handle;

	handle = sys_le16_to_cpu(cmd->handle);
	tx_octets = sys_le16_to_cpu(cmd->tx_octets);
	tx_time = sys_le16_to_cpu(cmd->tx_time);
	status = ll_length_req_send(handle, tx_octets, tx_time);

	rp = cmd_complete(evt, sizeof(*rp));
	rp->status = (!status) ?  0x00 : BT_HCI_ERR_CMD_DISALLOWED;
	rp->handle = sys_cpu_to_le16(handle);
}

static void le_read_default_data_len(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_le_read_default_data_len *rp;

	rp = cmd_complete(evt, sizeof(*rp));

	ll_length_default_get(&rp->max_tx_octets, &rp->max_tx_time);
	rp->status = 0x00;
}

static void le_write_default_data_len(struct net_buf *buf,
				      struct net_buf **evt)
{
	struct bt_hci_cp_le_write_default_data_len *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;
	u32_t status;

	status = ll_length_default_set(cmd->max_tx_octets, cmd->max_tx_time);

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = (!status) ? 0x00 : BT_HCI_ERR_INVALID_LL_PARAM;
}

static void le_read_max_data_len(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_le_read_max_data_len *rp;

	rp = cmd_complete(evt, sizeof(*rp));

	ll_length_max_get(&rp->max_tx_octets, &rp->max_tx_time,
			  &rp->max_rx_octets, &rp->max_rx_time);
	rp->status = 0x00;
}
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
static void le_read_phy(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_read_phy *cmd = (void *) buf->data;
	struct bt_hci_rp_le_read_phy *rp;
	u16_t handle;
	u32_t status;

	handle = sys_le16_to_cpu(cmd->handle);

	rp = cmd_complete(evt, sizeof(*rp));

	status = ll_phy_get(handle, &rp->tx_phy, &rp->rx_phy);

	rp->status = (!status) ?  0x00 : BT_HCI_ERR_CMD_DISALLOWED;
	rp->handle = sys_cpu_to_le16(handle);
	rp->tx_phy = find_lsb_set(rp->tx_phy);
	rp->rx_phy = find_lsb_set(rp->rx_phy);
}

static void le_set_default_phy(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_default_phy *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;
	u32_t status;

	if (cmd->all_phys & BT_HCI_LE_PHY_TX_ANY) {
		cmd->tx_phys = 0x07;
	}
	if (cmd->all_phys & BT_HCI_LE_PHY_RX_ANY) {
		cmd->rx_phys = 0x07;
	}

	status = ll_phy_default_set(cmd->tx_phys, cmd->rx_phys);

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = (!status) ? 0x00 : BT_HCI_ERR_INVALID_LL_PARAM;
}

static void le_set_phy(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_phy *cmd = (void *)buf->data;
	u32_t status;
	u16_t handle;
	u16_t phy_opts;

	handle = sys_le16_to_cpu(cmd->handle);
	phy_opts = sys_le16_to_cpu(cmd->phy_opts);

	if (cmd->all_phys & BT_HCI_LE_PHY_TX_ANY) {
		cmd->tx_phys = 0x07;
	}
	if (cmd->all_phys & BT_HCI_LE_PHY_RX_ANY) {
		cmd->rx_phys = 0x07;
	}
	if (phy_opts & 0x03) {
		phy_opts -= 1;
		phy_opts &= 1;
	} else {
		phy_opts = 0;
	}

	status = ll_phy_req_send(handle, cmd->tx_phys, phy_opts,
				 cmd->rx_phys);

	*evt = cmd_status((!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED);
}
#endif /* CONFIG_BT_CTLR_PHY */
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_CTLR_PRIVACY)
static void le_add_dev_to_rl(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_add_dev_to_rl *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;
	u32_t status;

	status = ll_rl_add(&cmd->peer_id_addr, cmd->peer_irk, cmd->local_irk);

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = status;
}

static void le_rem_dev_from_rl(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_rem_dev_from_rl *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;
	u32_t status;

	status = ll_rl_remove(&cmd->peer_id_addr);

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = status;
}

static void le_clear_rl(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_evt_cc_status *ccst;
	ccst = cmd_complete(evt, sizeof(*ccst));

	ccst->status = ll_rl_clear();
}

static void le_read_rl_size(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_le_read_rl_size *rp;

	rp = cmd_complete(evt, sizeof(*rp));

	rp->rl_size = ll_rl_size_get();
	rp->status = 0x00;
}

static void le_read_peer_rpa(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_read_peer_rpa *cmd = (void *)buf->data;
	struct bt_hci_rp_le_read_peer_rpa *rp;
	bt_addr_le_t peer_id_addr;

	bt_addr_le_copy(&peer_id_addr, &cmd->peer_id_addr);
	rp = cmd_complete(evt, sizeof(*rp));

	rp->status = ll_rl_crpa_get(&peer_id_addr, &rp->peer_rpa);
}

static void le_read_local_rpa(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_read_local_rpa *cmd = (void *)buf->data;
	struct bt_hci_rp_le_read_local_rpa *rp;
	bt_addr_le_t peer_id_addr;

	bt_addr_le_copy(&peer_id_addr, &cmd->peer_id_addr);
	rp = cmd_complete(evt, sizeof(*rp));

	rp->status = ll_rl_lrpa_get(&peer_id_addr, &rp->local_rpa);
}

static void le_set_addr_res_enable(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_addr_res_enable *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;
	u8_t enable = cmd->enable;

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = ll_rl_enable(enable);
}

static void le_set_rpa_timeout(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_rpa_timeout *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;
	u16_t timeout = sys_le16_to_cpu(cmd->rpa_timeout);

	ll_rl_timeout_set(timeout);

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = 0x00;
}

static void le_set_privacy_mode(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_privacy_mode *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;
	u32_t status;

	status = ll_priv_mode_set(&cmd->id_addr, cmd->mode);

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = status;
}
#endif /* CONFIG_BT_CTLR_PRIVACY */

static void le_read_tx_power(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_le_read_tx_power *rp;

	rp = cmd_complete(evt, sizeof(*rp));
	rp->status = 0x00;
	ll_tx_power_get(&rp->min_tx_power, &rp->max_tx_power);
}

#if defined(CONFIG_BT_CTLR_DTM_HCI)
static void le_rx_test(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_rx_test *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;
	u32_t status;

	status = ll_test_rx(cmd->rx_ch, 0x01, 0);

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = status;
}

static void le_tx_test(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_tx_test *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;
	u32_t status;

	status = ll_test_tx(cmd->tx_ch, cmd->test_data_len, cmd->pkt_payload,
			    0x01);

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = status;
}

static void le_test_end(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_le_test_end *rp;
	u16_t rx_pkt_count;

	ll_test_end(&rx_pkt_count);

	rp = cmd_complete(evt, sizeof(*rp));
	rp->status = 0x00;
	rp->rx_pkt_count = sys_cpu_to_le16(rx_pkt_count);
}

static void le_enh_rx_test(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_enh_rx_test *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;
	u32_t status;

	status = ll_test_rx(cmd->rx_ch, cmd->phy, cmd->mod_index);

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = status;
}

static void le_enh_tx_test(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_enh_tx_test *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;
	u32_t status;

	status = ll_test_tx(cmd->tx_ch, cmd->test_data_len, cmd->pkt_payload,
			    cmd->phy);

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = status;
}
#endif /* CONFIG_BT_CTLR_DTM_HCI */

static int controller_cmd_handle(u16_t  ocf, struct net_buf *cmd,
				 struct net_buf **evt)
{
	switch (ocf) {
	case BT_OCF(BT_HCI_OP_LE_SET_EVENT_MASK):
		le_set_event_mask(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_READ_BUFFER_SIZE):
		le_read_buffer_size(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_READ_LOCAL_FEATURES):
		le_read_local_features(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_RANDOM_ADDRESS):
		le_set_random_address(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_READ_WL_SIZE):
		le_read_wl_size(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_CLEAR_WL):
		le_clear_wl(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_ADD_DEV_TO_WL):
		le_add_dev_to_wl(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_REM_DEV_FROM_WL):
		le_rem_dev_from_wl(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_ENCRYPT):
		le_encrypt(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_RAND):
		le_rand(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_READ_SUPP_STATES):
		le_read_supp_states(cmd, evt);
		break;

#if defined(CONFIG_BT_BROADCASTER)
	case BT_OCF(BT_HCI_OP_LE_SET_ADV_PARAM):
		le_set_adv_param(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_READ_ADV_CHAN_TX_POWER):
		le_read_adv_chan_tx_power(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_ADV_DATA):
		le_set_adv_data(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_SCAN_RSP_DATA):
		le_set_scan_rsp_data(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_ADV_ENABLE):
		le_set_adv_enable(cmd, evt);
		break;
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_OBSERVER)
	case BT_OCF(BT_HCI_OP_LE_SET_SCAN_PARAM):
		le_set_scan_param(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_SCAN_ENABLE):
		le_set_scan_enable(cmd, evt);
		break;
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_CONN)
#if defined(CONFIG_BT_CENTRAL)
	case BT_OCF(BT_HCI_OP_LE_CREATE_CONN):
		le_create_connection(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_CREATE_CONN_CANCEL):
		le_create_conn_cancel(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_HOST_CHAN_CLASSIF):
		le_set_host_chan_classif(cmd, evt);
		break;

#if defined(CONFIG_BT_CTLR_LE_ENC)
	case BT_OCF(BT_HCI_OP_LE_START_ENCRYPTION):
		le_start_encryption(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_LE_ENC */
#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_PERIPHERAL)
#if defined(CONFIG_BT_CTLR_LE_ENC)
	case BT_OCF(BT_HCI_OP_LE_LTK_REQ_REPLY):
		le_ltk_req_reply(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_LTK_REQ_NEG_REPLY):
		le_ltk_req_neg_reply(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_LE_ENC */
#endif /* CONFIG_BT_PERIPHERAL */

	case BT_OCF(BT_HCI_OP_LE_READ_CHAN_MAP):
		le_read_chan_map(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_READ_REMOTE_FEATURES):
		le_read_remote_features(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_CONN_UPDATE):
		le_conn_update(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_CONN_PARAM_REQ_REPLY):
		le_conn_param_req_reply(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_CONN_PARAM_REQ_NEG_REPLY):
		le_conn_param_req_neg_reply(cmd, evt);
		break;

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	case BT_OCF(BT_HCI_OP_LE_SET_DATA_LEN):
		le_set_data_len(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_READ_DEFAULT_DATA_LEN):
		le_read_default_data_len(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_WRITE_DEFAULT_DATA_LEN):
		le_write_default_data_len(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_READ_MAX_DATA_LEN):
		le_read_max_data_len(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
	case BT_OCF(BT_HCI_OP_LE_READ_PHY):
		le_read_phy(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_DEFAULT_PHY):
		le_set_default_phy(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_PHY):
		le_set_phy(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_PHY */
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_CTLR_PRIVACY)
	case BT_OCF(BT_HCI_OP_LE_ADD_DEV_TO_RL):
		le_add_dev_to_rl(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_REM_DEV_FROM_RL):
		le_rem_dev_from_rl(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_CLEAR_RL):
		le_clear_rl(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_READ_RL_SIZE):
		le_read_rl_size(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_READ_PEER_RPA):
		le_read_peer_rpa(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_READ_LOCAL_RPA):
		le_read_local_rpa(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_SET_ADDR_RES_ENABLE):
		le_set_addr_res_enable(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_SET_RPA_TIMEOUT):
		le_set_rpa_timeout(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_SET_PRIVACY_MODE):
		le_set_privacy_mode(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_PRIVACY */

	case BT_OCF(BT_HCI_OP_LE_READ_TX_POWER):
		le_read_tx_power(cmd, evt);
		break;

#if defined(CONFIG_BT_CTLR_DTM_HCI)
	case BT_OCF(BT_HCI_OP_LE_RX_TEST):
		le_rx_test(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_TX_TEST):
		le_tx_test(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_TEST_END):
		le_test_end(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_ENH_RX_TEST):
		le_enh_rx_test(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_ENH_TX_TEST):
		le_enh_tx_test(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_DTM_HCI */

	default:
		return -EINVAL;
	}

	return 0;
}

static void vs_read_version_info(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_vs_read_version_info *rp;

	rp = cmd_complete(evt, sizeof(*rp));

	rp->status = 0x00;
	rp->hw_platform = BT_HCI_VS_HW_PLAT;
	rp->hw_variant = BT_HCI_VS_HW_VAR;

	rp->fw_variant = 0;
	rp->fw_version = (KERNEL_VERSION_MAJOR & 0xff);
	rp->fw_revision = KERNEL_VERSION_MINOR;
	rp->fw_build = (KERNEL_PATCHLEVEL & 0xffff);
}

static void vs_read_supported_commands(struct net_buf *buf,
				       struct net_buf **evt)
{
	struct bt_hci_rp_vs_read_supported_commands *rp;

	rp = cmd_complete(evt, sizeof(*rp));

	rp->status = 0x00;
	memset(&rp->commands[0], 0, sizeof(rp->commands));

	/* Set Version Information, Supported Commands, Supported Features. */
	rp->commands[0] |= BIT(0) | BIT(1) | BIT(2);
#if defined(CONFIG_BT_HCI_VS_EXT)
	/* Write BD_ADDR, Read Build Info */
	rp->commands[0] |= BIT(5) | BIT(7);
	/* Read Static Addresses, Read Key Hierarchy Roots */
	rp->commands[1] |= BIT(0) | BIT(1);
#endif /* CONFIG_BT_HCI_VS_EXT */
}

static void vs_read_supported_features(struct net_buf *buf,
				       struct net_buf **evt)
{
	struct bt_hci_rp_vs_read_supported_features *rp;

	rp = cmd_complete(evt, sizeof(*rp));

	rp->status = 0x00;
	memset(&rp->features[0], 0x00, sizeof(rp->features));
}

#if defined(CONFIG_BT_HCI_VS_EXT)
static void vs_write_bd_addr(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_vs_write_bd_addr *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;

	ll_addr_set(0, &cmd->bdaddr.val[0]);

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = 0x00;
}

static void vs_read_build_info(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_vs_read_build_info *rp;

#define BUILD_TIMESTAMP " " __DATE__ " " __TIME__

#define HCI_VS_BUILD_INFO "Zephyr OS v" \
	KERNEL_VERSION_STRING BUILD_TIMESTAMP CONFIG_BT_CTLR_HCI_VS_BUILD_INFO

	const char build_info[] = HCI_VS_BUILD_INFO;

#define BUILD_INFO_EVT_LEN (sizeof(struct bt_hci_evt_hdr) + \
			    sizeof(struct bt_hci_evt_cmd_complete) + \
			    sizeof(struct bt_hci_rp_vs_read_build_info) + \
			    sizeof(build_info))

	BUILD_ASSERT(CONFIG_BT_RX_BUF_LEN >= BUILD_INFO_EVT_LEN);

	rp = cmd_complete(evt, sizeof(*rp) + sizeof(build_info));
	rp->status = 0x00;
	memcpy(rp->info, build_info, sizeof(build_info));
}

static void vs_read_static_addrs(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_vs_read_static_addrs *rp;

#if defined(CONFIG_SOC_FAMILY_NRF5)
	/* Read address from nRF5-specific storage
	 * Non-initialized FICR values default to 0xFF, skip if no address
	 * present. Also if a public address lives in FICR, do not use in this
	 * function.
	 */
	if (((NRF_FICR->DEVICEADDR[0] != UINT32_MAX) ||
	    ((NRF_FICR->DEVICEADDR[1] & UINT16_MAX) != UINT16_MAX)) &&
	      (NRF_FICR->DEVICEADDRTYPE & 0x01)) {
		struct bt_hci_vs_static_addr *addr;

		rp = cmd_complete(evt, sizeof(*rp) + sizeof(*addr));
		rp->status = 0x00;
		rp->num_addrs = 1;

		addr = &rp->a[0];
		sys_put_le32(NRF_FICR->DEVICEADDR[0], &addr->bdaddr.val[0]);
		sys_put_le16(NRF_FICR->DEVICEADDR[1], &addr->bdaddr.val[4]);
		/* The FICR value is a just a random number, with no knowledge
		 * of the Bluetooth Specification requirements for random
		 * static addresses.
		 */
		BT_ADDR_SET_STATIC(&addr->bdaddr);

		/* Mark IR as invalid */
		memset(addr->ir, 0x00, sizeof(addr->ir));

		return;
	}
#endif /* CONFIG_SOC_FAMILY_NRF5 */

	rp = cmd_complete(evt, sizeof(*rp));
	rp->status = 0x00;
	rp->num_addrs = 0;
}

static void vs_read_key_hierarchy_roots(struct net_buf *buf,
					struct net_buf **evt)
{
	struct bt_hci_rp_vs_read_key_hierarchy_roots *rp;

	rp = cmd_complete(evt, sizeof(*rp));
	rp->status = 0x00;

#if defined(CONFIG_SOC_FAMILY_NRF5)
	/* Fill in IR if present */
	if ((NRF_FICR->IR[0] != UINT32_MAX) &&
	    (NRF_FICR->IR[1] != UINT32_MAX) &&
	    (NRF_FICR->IR[2] != UINT32_MAX) &&
	    (NRF_FICR->IR[3] != UINT32_MAX)) {
		sys_put_le32(NRF_FICR->IR[0], &rp->ir[0]);
		sys_put_le32(NRF_FICR->IR[1], &rp->ir[4]);
		sys_put_le32(NRF_FICR->IR[2], &rp->ir[8]);
		sys_put_le32(NRF_FICR->IR[3], &rp->ir[12]);
	} else {
		/* Mark IR as invalid */
		memset(rp->ir, 0x00, sizeof(rp->ir));
	}

	/* Fill in ER if present */
	if ((NRF_FICR->ER[0] != UINT32_MAX) &&
	    (NRF_FICR->ER[1] != UINT32_MAX) &&
	    (NRF_FICR->ER[2] != UINT32_MAX) &&
	    (NRF_FICR->ER[3] != UINT32_MAX)) {
		sys_put_le32(NRF_FICR->ER[0], &rp->er[0]);
		sys_put_le32(NRF_FICR->ER[1], &rp->er[4]);
		sys_put_le32(NRF_FICR->ER[2], &rp->er[8]);
		sys_put_le32(NRF_FICR->ER[3], &rp->er[12]);
	} else {
		/* Mark ER as invalid */
		memset(rp->er, 0x00, sizeof(rp->er));
	}

	return;
#else
	/* Mark IR as invalid */
	memset(rp->ir, 0x00, sizeof(rp->ir));
	/* Mark ER as invalid */
	memset(rp->er, 0x00, sizeof(rp->er));
#endif /* CONFIG_SOC_FAMILY_NRF5 */
}

#endif /* CONFIG_BT_HCI_VS_EXT */

static int vendor_cmd_handle(u16_t ocf, struct net_buf *cmd,
			     struct net_buf **evt)
{
	switch (ocf) {
	case BT_OCF(BT_HCI_OP_VS_READ_VERSION_INFO):
		vs_read_version_info(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_VS_READ_SUPPORTED_COMMANDS):
		vs_read_supported_commands(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_VS_READ_SUPPORTED_FEATURES):
		vs_read_supported_features(cmd, evt);
		break;

#if defined(CONFIG_BT_HCI_VS_EXT)
	case BT_OCF(BT_HCI_OP_VS_READ_BUILD_INFO):
		vs_read_build_info(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_VS_WRITE_BD_ADDR):
		vs_write_bd_addr(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_VS_READ_STATIC_ADDRS):
		vs_read_static_addrs(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_VS_READ_KEY_HIERARCHY_ROOTS):
		vs_read_key_hierarchy_roots(cmd, evt);
		break;
#endif /* CONFIG_BT_HCI_VS_EXT */

	default:
		return -EINVAL;
	}

	return 0;
}

static void data_buf_overflow(struct net_buf **buf)
{
	struct bt_hci_evt_data_buf_overflow *ep;

	if (!(event_mask & BT_EVT_MASK_DATA_BUFFER_OVERFLOW)) {
		return;
	}

	*buf = bt_buf_get_rx(BT_BUF_EVT, K_FOREVER);
	evt_create(*buf, BT_HCI_EVT_DATA_BUF_OVERFLOW, sizeof(*ep));
	ep = net_buf_add(*buf, sizeof(*ep));

	ep->link_type = BT_OVERFLOW_LINK_ACL;
}

struct net_buf *hci_cmd_handle(struct net_buf *cmd)
{
	struct bt_hci_evt_cc_status *ccst;
	struct bt_hci_cmd_hdr *chdr;
	struct net_buf *evt = NULL;
	u16_t ocf;
	int err;

	if (cmd->len < sizeof(*chdr)) {
		BT_ERR("No HCI Command header");
		return NULL;
	}

	chdr = (void *)cmd->data;
	/* store in a global for later CC/CS event creation */
	_opcode = sys_le16_to_cpu(chdr->opcode);

	if (cmd->len < chdr->param_len) {
		BT_ERR("Invalid HCI CMD packet length");
		return NULL;
	}

	net_buf_pull(cmd, sizeof(*chdr));

	ocf = BT_OCF(_opcode);

	switch (BT_OGF(_opcode)) {
	case BT_OGF_LINK_CTRL:
		err = link_control_cmd_handle(ocf, cmd, &evt);
		break;
	case BT_OGF_BASEBAND:
		err = ctrl_bb_cmd_handle(ocf, cmd, &evt);
		break;
	case BT_OGF_INFO:
		err = info_cmd_handle(ocf, cmd, &evt);
		break;
	case BT_OGF_STATUS:
		err = status_cmd_handle(ocf, cmd, &evt);
		break;
	case BT_OGF_LE:
		err = controller_cmd_handle(ocf, cmd, &evt);
		break;
	case BT_OGF_VS:
		err = vendor_cmd_handle(ocf, cmd, &evt);
		break;
	default:
		err = -EINVAL;
		break;
	}

	if (err == -EINVAL) {
		ccst = cmd_complete(&evt, sizeof(*ccst));
		ccst->status = BT_HCI_ERR_UNKNOWN_CMD;
	}

	return evt;
}

int hci_acl_handle(struct net_buf *buf, struct net_buf **evt)
{
	struct radio_pdu_node_tx *radio_pdu_node_tx;
	struct bt_hci_acl_hdr *acl;
	struct pdu_data *pdu_data;
	u16_t handle;
	u8_t flags;
	u16_t len;

	*evt = NULL;

	if (buf->len < sizeof(*acl)) {
		BT_ERR("No HCI ACL header");
		return -EINVAL;
	}

	acl = (void *)buf->data;
	len = sys_le16_to_cpu(acl->len);
	handle = sys_le16_to_cpu(acl->handle);
	net_buf_pull(buf, sizeof(*acl));

	if (buf->len < len) {
		BT_ERR("Invalid HCI ACL packet length");
		return -EINVAL;
	}

	/* assigning flags first because handle will be overwritten */
	flags = bt_acl_flags(handle);
	handle = bt_acl_handle(handle);

	radio_pdu_node_tx = radio_tx_mem_acquire();
	if (!radio_pdu_node_tx) {
		BT_ERR("Tx Buffer Overflow");
		data_buf_overflow(evt);
		return -ENOBUFS;
	}

	pdu_data = (struct pdu_data *)radio_pdu_node_tx->pdu_data;
	if (flags == BT_ACL_START_NO_FLUSH || flags == BT_ACL_START) {
		pdu_data->ll_id = PDU_DATA_LLID_DATA_START;
	} else {
		pdu_data->ll_id = PDU_DATA_LLID_DATA_CONTINUE;
	}
	pdu_data->len = len;
	memcpy(&pdu_data->payload.lldata[0], buf->data, len);

	if (radio_tx_mem_enqueue(handle, radio_pdu_node_tx)) {
		BT_ERR("Invalid Tx Enqueue");
		radio_tx_mem_release(radio_pdu_node_tx);
		return -EINVAL;
	}

	return 0;
}

#if CONFIG_BT_CTLR_DUP_FILTER_LEN > 0
static inline bool dup_found(struct pdu_adv *adv)
{
	/* check for duplicate filtering */
	if (dup_count >= 0) {
		int i;

		for (i = 0; i < dup_count; i++) {
			if (!memcmp(&adv->payload.adv_ind.addr[0],
				    &dup_filter[i].addr.a.val[0],
				    sizeof(bt_addr_t)) &&
			    adv->tx_addr == dup_filter[i].addr.type) {

				if (dup_filter[i].mask & BIT(adv->type)) {
					/* duplicate found */
					return true;
				}
				/* report different adv types */
				dup_filter[i].mask |= BIT(adv->type);
				return false;
			}
		}

		/* insert into the duplicate filter */
		memcpy(&dup_filter[dup_curr].addr.a.val[0],
		       &adv->payload.adv_ind.addr[0], sizeof(bt_addr_t));
		dup_filter[dup_curr].addr.type = adv->tx_addr;
		dup_filter[dup_curr].mask = BIT(adv->type);

		if (dup_count < CONFIG_BT_CTLR_DUP_FILTER_LEN) {
			dup_count++;
			dup_curr = dup_count;
		} else {
			dup_curr++;
		}

		if (dup_curr == CONFIG_BT_CTLR_DUP_FILTER_LEN) {
			dup_curr = 0;
		}
	}

	return false;
}
#endif /* CONFIG_BT_CTLR_DUP_FILTER_LEN > 0 */

static void le_advertising_report(struct pdu_data *pdu_data, u8_t *b,
				  struct net_buf *buf)
{
	const u8_t c_adv_type[] = { 0x00, 0x01, 0x03, 0xff, 0x04,
				    0xff, 0x02 };
	struct bt_hci_evt_le_advertising_report *sep;
	struct pdu_adv *adv = (struct pdu_adv *)pdu_data;
	struct bt_hci_evt_le_advertising_info *adv_info;
	u8_t data_len;
	u8_t info_len;
	s8_t rssi;
#if defined(CONFIG_BT_CTLR_PRIVACY)
	u8_t rl_idx;
#endif /* CONFIG_BT_CTLR_PRIVACY */
#if defined(CONFIG_BT_CTLR_EXT_SCAN_FP)
	u8_t direct;
#endif /* CONFIG_BT_CTLR_EXT_SCAN_FP */
	s8_t *prssi;

#if defined(CONFIG_BT_CTLR_PRIVACY)
	rl_idx = b[offsetof(struct radio_pdu_node_rx, pdu_data) +
		   offsetof(struct pdu_adv, payload) + adv->len + 1];
	/* Update current RPA */
	if (adv->tx_addr) {
		ll_rl_crpa_set(0x00, NULL, rl_idx,
			       &adv->payload.adv_ind.addr[0]);
	}
#endif

	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT)) {
		return;
	}

#if defined(CONFIG_BT_CTLR_EXT_SCAN_FP)
	direct = b[offsetof(struct radio_pdu_node_rx, pdu_data) +
		   offsetof(struct pdu_adv, payload) + adv->len + 2];

	if ((!direct && !(le_event_mask & BT_EVT_MASK_LE_ADVERTISING_REPORT)) ||
	    (direct && !(le_event_mask & BT_HCI_EVT_LE_DIRECT_ADV_REPORT))) {
		return;
	}
#else
	if (!(le_event_mask & BT_EVT_MASK_LE_ADVERTISING_REPORT)) {
		return;
	}
#endif /* CONFIG_BT_CTLR_EXT_SCAN_FP */


#if CONFIG_BT_CTLR_DUP_FILTER_LEN > 0
	if (dup_found(adv)) {
		return;
	}
#endif /* CONFIG_BT_CTLR_DUP_FILTER_LEN > 0 */

	if (adv->type != PDU_ADV_TYPE_DIRECT_IND) {
		data_len = (adv->len - BDADDR_SIZE);
	} else {
		data_len = 0;
	}

	/* The Link Layer currently returns RSSI as an absolute value */
	rssi = -b[offsetof(struct radio_pdu_node_rx, pdu_data) +
		  offsetof(struct pdu_adv, payload) + adv->len];

#if defined(CONFIG_BT_CTLR_EXT_SCAN_FP)
	if (direct) {
		struct bt_hci_evt_le_direct_adv_report *drp;
		struct bt_hci_evt_le_direct_adv_info *dir_info;

		LL_ASSERT(adv->type == PDU_ADV_TYPE_DIRECT_IND);
		drp = meta_evt(buf, BT_HCI_EVT_LE_DIRECT_ADV_REPORT,
			       sizeof(*drp) + sizeof(*dir_info));

		drp->num_reports = 1;
		dir_info = (void *)(((u8_t *)drp) + sizeof(*drp));

		dir_info->evt_type = c_adv_type[PDU_ADV_TYPE_DIRECT_IND];

#if defined(CONFIG_BT_CTLR_PRIVACY)
		if (rl_idx < ll_rl_size_get()) {
			/* Store identity address */
			ll_rl_id_addr_get(rl_idx, &dir_info->addr.type,
					  &dir_info->addr.a.val[0]);
			/* Mark it as identity address from RPA (0x02, 0x03) */
			dir_info->addr.type += 2;
		} else {
#else
		if (1) {
#endif /* CONFIG_BT_CTLR_PRIVACY */
			dir_info->addr.type = adv->tx_addr;
			memcpy(&dir_info->addr.a.val[0],
			       &adv->payload.direct_ind.adv_addr[0],
			       sizeof(bt_addr_t));
		}

		dir_info->dir_addr.type = 0x1;
		memcpy(&dir_info->dir_addr.a.val[0],
		       &adv->payload.direct_ind.tgt_addr[0], sizeof(bt_addr_t));

		dir_info->rssi = rssi;

		return;
	}
#endif /* CONFIG_BT_CTLR_EXT_SCAN_FP */

	info_len = sizeof(struct bt_hci_evt_le_advertising_info) + data_len +
		   sizeof(*prssi);
	sep = meta_evt(buf, BT_HCI_EVT_LE_ADVERTISING_REPORT,
		       sizeof(*sep) + info_len);

	sep->num_reports = 1;
	adv_info = (void *)(((u8_t *)sep) + sizeof(*sep));

	adv_info->evt_type = c_adv_type[adv->type];

#if defined(CONFIG_BT_CTLR_PRIVACY)
	rl_idx = b[offsetof(struct radio_pdu_node_rx, pdu_data) +
		   offsetof(struct pdu_adv, payload) + adv->len + 1];
	if (rl_idx < ll_rl_size_get()) {
		/* Store identity address */
		ll_rl_id_addr_get(rl_idx, &adv_info->addr.type,
				  &adv_info->addr.a.val[0]);
		/* Mark it as identity address from RPA (0x02, 0x03) */
		adv_info->addr.type += 2;
	} else {
#else
	if (1) {
#endif /* CONFIG_BT_CTLR_PRIVACY */

		adv_info->addr.type = adv->tx_addr;
		memcpy(&adv_info->addr.a.val[0], &adv->payload.adv_ind.addr[0],
		       sizeof(bt_addr_t));
	}

	adv_info->length = data_len;
	memcpy(&adv_info->data[0], &adv->payload.adv_ind.data[0], data_len);
	/* RSSI */
	prssi = &adv_info->data[0] + data_len;
	*prssi = rssi;
}

#if defined(CONFIG_BT_CTLR_ADV_EXT)
static void le_adv_ext_report(struct pdu_data *pdu_data, u8_t *b,
			      struct net_buf *buf, u8_t phy)
{
	struct pdu_adv *adv = (struct pdu_adv *)pdu_data;
	s8_t rssi;

	/* The Link Layer currently returns RSSI as an absolute value */
	rssi = -b[offsetof(struct radio_pdu_node_rx, pdu_data) +
		  offsetof(struct pdu_adv, payload) + adv->len];

	BT_WARN("phy= 0x%x, type= 0x%x, len= %u, tat= %u, rat= %u, rssi=%d dB",
		phy, adv->type, adv->len, adv->tx_addr, adv->rx_addr, rssi);

	if ((adv->type == PDU_ADV_TYPE_EXT_IND) && adv->len) {
		struct pdu_adv_payload_com_ext_adv *p;
		struct ext_adv_hdr *h;
		u8_t *ptr;

		p = (void *)&adv->payload.adv_ext_ind;
		h = (void *)p->ext_hdr_adi_adv_data;
		ptr = (u8_t *)h + sizeof(*h);

		BT_WARN("Ext. adv mode= 0x%x, hdr len= %u", p->adv_mode,
			p->ext_hdr_len);

		if (!p->ext_hdr_len) {
			goto no_ext_hdr;
		}

		if (h->adv_addr) {
			char addr_str[BT_ADDR_LE_STR_LEN];
			bt_addr_le_t addr;

			addr.type = adv->tx_addr;
			memcpy(&addr.a.val[0], ptr, sizeof(bt_addr_t));
			ptr += BDADDR_SIZE;

			bt_addr_le_to_str(&addr, addr_str, sizeof(addr_str));

			BT_WARN("AdvA: %s", addr_str);

		}

		if (h->tx_pwr) {
			s8_t tx_pwr;

			tx_pwr = *(s8_t *)ptr;
			ptr++;

			BT_WARN("Tx pwr= %d dB", tx_pwr);
		}

		/* TODO: length check? */
	}

no_ext_hdr:
	return;
}

static void le_adv_ext_1M_report(struct pdu_data *pdu_data, u8_t *b,
				 struct net_buf *buf)
{
	le_adv_ext_report(pdu_data, b, buf, BIT(0));
}

static void le_adv_ext_coded_report(struct pdu_data *pdu_data, u8_t *b,
				    struct net_buf *buf)
{
	le_adv_ext_report(pdu_data, b, buf, BIT(2));
}
#endif /* CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY)
static void le_scan_req_received(struct pdu_data *pdu_data, u8_t *b,
				 struct net_buf *buf)
{
	struct pdu_adv *adv = (struct pdu_adv *)pdu_data;
	struct bt_hci_evt_le_scan_req_received *sep;

	/* TODO: fill handle when Adv Ext. feature is implemented. */

	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    !(le_event_mask & BT_EVT_MASK_LE_SCAN_REQ_RECEIVED)) {
		char addr_str[BT_ADDR_LE_STR_LEN];
		bt_addr_le_t addr;
		u8_t handle;
		s8_t rssi;

		handle = 0;
		addr.type = adv->tx_addr;
		memcpy(&addr.a.val[0], &adv->payload.scan_req.scan_addr[0],
		       sizeof(bt_addr_t));
		/* The Link Layer currently returns RSSI as an absolute value */
		rssi = -b[offsetof(struct radio_pdu_node_rx, pdu_data) +
			  offsetof(struct pdu_adv, payload) + adv->len];

		bt_addr_le_to_str(&addr, addr_str, sizeof(addr_str));

		BT_WARN("handle: %d, addr: %s, rssi: %d dB.",
			handle, addr_str, rssi);

		return;
	}

	sep = meta_evt(buf, BT_HCI_EVT_LE_SCAN_REQ_RECEIVED, sizeof(*sep));
	sep->handle = 0;
	sep->addr.type = adv->tx_addr;
	memcpy(&sep->addr.a.val[0], &adv->payload.scan_req.scan_addr[0],
	       sizeof(bt_addr_t));
}
#endif /* CONFIG_BT_CTLR_SCAN_REQ_NOTIFY */

#if defined(CONFIG_BT_CONN)
static void le_conn_complete(struct pdu_data *pdu_data, u16_t handle,
			     struct net_buf *buf)
{
	struct bt_hci_evt_le_conn_complete *lecc;
	struct radio_le_conn_cmplt *radio_cc;

	radio_cc = (struct radio_le_conn_cmplt *) (pdu_data->payload.lldata);

#if defined(CONFIG_BT_CTLR_PRIVACY)
	/* Update current RPA */
	ll_rl_crpa_set(radio_cc->peer_addr_type, &radio_cc->peer_addr[0],
		       0xff, &radio_cc->peer_rpa[0]);
#endif
	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    (!(le_event_mask & BT_EVT_MASK_LE_CONN_COMPLETE) &&
#if defined(CONFIG_BT_CTLR_PRIVACY)
	     !(le_event_mask & BT_EVT_MASK_LE_ENH_CONN_COMPLETE))) {
#else
	     1)) {
#endif /* CONFIG_BT_CTLR_PRIVACY */
		return;
	}

	if (!radio_cc->status) {
		conn_count++;
	}

#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (le_event_mask & BT_EVT_MASK_LE_ENH_CONN_COMPLETE) {
		struct bt_hci_evt_le_enh_conn_complete *leecc;

		leecc = meta_evt(buf, BT_HCI_EVT_LE_ENH_CONN_COMPLETE,
				 sizeof(*leecc));

		leecc->status = radio_cc->status;
		leecc->handle = sys_cpu_to_le16(handle);
		leecc->role = radio_cc->role;

		leecc->peer_addr.type = radio_cc->peer_addr_type;
		memcpy(&leecc->peer_addr.a.val[0], &radio_cc->peer_addr[0],
		       BDADDR_SIZE);

		/* Note: this could be an RPA set as the random address by
		 * the Host instead of generated by the controller. That said,
		 * this should make no difference. */
		if ((radio_cc->own_addr_type) &&
		    ((radio_cc->own_addr[5] & 0xc0) == 0x40)) {
			memcpy(&leecc->local_rpa.val[0], &radio_cc->own_addr[0],
			       BDADDR_SIZE);
		} else {
			memset(&leecc->local_rpa.val[0], 0x0, BDADDR_SIZE);
		}

		memcpy(&leecc->peer_rpa.val[0], &radio_cc->peer_rpa[0],
		       BDADDR_SIZE);

		leecc->interval = sys_cpu_to_le16(radio_cc->interval);
		leecc->latency = sys_cpu_to_le16(radio_cc->latency);
		leecc->supv_timeout = sys_cpu_to_le16(radio_cc->timeout);
		leecc->clock_accuracy = radio_cc->mca;
		return;
	}
#endif /* CONFIG_BT_CTLR_PRIVACY */

	lecc = meta_evt(buf, BT_HCI_EVT_LE_CONN_COMPLETE, sizeof(*lecc));

	lecc->status = radio_cc->status;
	lecc->handle = sys_cpu_to_le16(handle);
	lecc->role = radio_cc->role;
	lecc->peer_addr.type = radio_cc->peer_addr_type;
	memcpy(&lecc->peer_addr.a.val[0], &radio_cc->peer_addr[0], BDADDR_SIZE);
	lecc->interval = sys_cpu_to_le16(radio_cc->interval);
	lecc->latency = sys_cpu_to_le16(radio_cc->latency);
	lecc->supv_timeout = sys_cpu_to_le16(radio_cc->timeout);
	lecc->clock_accuracy = radio_cc->mca;
}

static void disconn_complete(struct pdu_data *pdu_data, u16_t handle,
			     struct net_buf *buf)
{
	struct bt_hci_evt_disconn_complete *ep;

	if (!(event_mask & BT_EVT_MASK_DISCONN_COMPLETE)) {
		return;
	}

	evt_create(buf, BT_HCI_EVT_DISCONN_COMPLETE, sizeof(*ep));
	ep = net_buf_add(buf, sizeof(*ep));

	ep->status = 0x00;
	ep->handle = sys_cpu_to_le16(handle);
	ep->reason = *((u8_t *)pdu_data);

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
	/* Clear any pending packets upon disconnection */
	/* Note: This requires linear handle values starting from 0 */
	LL_ASSERT(handle < ARRAY_SIZE(hci_hbuf_pend));
	hci_hbuf_acked += hci_hbuf_pend[handle];
	hci_hbuf_pend[handle] = 0;
#endif /* CONFIG_BT_HCI_ACL_FLOW_CONTROL */
	conn_count--;
}

static void le_conn_update_complete(struct pdu_data *pdu_data, u16_t handle,
				    struct net_buf *buf)
{
	struct bt_hci_evt_le_conn_update_complete *sep;
	struct radio_le_conn_update_cmplt *radio_cu;

	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    !(le_event_mask & BT_EVT_MASK_LE_CONN_UPDATE_COMPLETE)) {
		return;
	}

	radio_cu = (struct radio_le_conn_update_cmplt *)
			(pdu_data->payload.lldata);

	sep = meta_evt(buf, BT_HCI_EVT_LE_CONN_UPDATE_COMPLETE, sizeof(*sep));

	sep->status = radio_cu->status;
	sep->handle = sys_cpu_to_le16(handle);
	sep->interval = sys_cpu_to_le16(radio_cu->interval);
	sep->latency = sys_cpu_to_le16(radio_cu->latency);
	sep->supv_timeout = sys_cpu_to_le16(radio_cu->timeout);
}

#if defined(CONFIG_BT_CTLR_LE_ENC)
static void enc_refresh_complete(struct pdu_data *pdu_data, u16_t handle,
				 struct net_buf *buf)
{
	struct bt_hci_evt_encrypt_key_refresh_complete *ep;

	if (!(event_mask & BT_EVT_MASK_ENCRYPT_KEY_REFRESH_COMPLETE)) {
		return;
	}

	evt_create(buf, BT_HCI_EVT_ENCRYPT_KEY_REFRESH_COMPLETE, sizeof(*ep));
	ep = net_buf_add(buf, sizeof(*ep));

	ep->status = 0x00;
	ep->handle = sys_cpu_to_le16(handle);
}
#endif /* CONFIG_BT_CTLR_LE_ENC */

#if defined(CONFIG_BT_CTLR_LE_PING)
static void auth_payload_timeout_exp(struct pdu_data *pdu_data, u16_t handle,
				     struct net_buf *buf)
{
	struct bt_hci_evt_auth_payload_timeout_exp *ep;

	if (!(event_mask_page_2 & BT_EVT_MASK_AUTH_PAYLOAD_TIMEOUT_EXP)) {
		return;
	}

	evt_create(buf, BT_HCI_EVT_AUTH_PAYLOAD_TIMEOUT_EXP, sizeof(*ep));
	ep = net_buf_add(buf, sizeof(*ep));

	ep->handle = sys_cpu_to_le16(handle);
}
#endif /* CONFIG_BT_CTLR_LE_PING */

#if defined(CONFIG_BT_CTLR_CHAN_SEL_2)
static void le_chan_sel_algo(struct pdu_data *pdu_data, u16_t handle,
			     struct net_buf *buf)
{
	struct bt_hci_evt_le_chan_sel_algo *sep;
	struct radio_le_chan_sel_algo *radio_le_chan_sel_algo;

	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    !(le_event_mask & BT_EVT_MASK_LE_CHAN_SEL_ALGO)) {
		return;
	}

	radio_le_chan_sel_algo = (struct radio_le_chan_sel_algo *)
					pdu_data->payload.lldata;

	sep = meta_evt(buf, BT_HCI_EVT_LE_CHAN_SEL_ALGO, sizeof(*sep));

	sep->handle = sys_cpu_to_le16(handle);
	sep->chan_sel_algo = radio_le_chan_sel_algo->chan_sel_algo;
}
#endif /* CONFIG_BT_CTLR_CHAN_SEL_2 */

#if defined(CONFIG_BT_CTLR_PHY)
static void le_phy_upd_complete(struct pdu_data *pdu_data, u16_t handle,
				struct net_buf *buf)
{
	struct bt_hci_evt_le_phy_update_complete *sep;
	struct radio_le_phy_upd_cmplt *radio_le_phy_upd_cmplt;

	radio_le_phy_upd_cmplt = (struct radio_le_phy_upd_cmplt *)
				 pdu_data->payload.lldata;

	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    !(le_event_mask & BT_EVT_MASK_LE_PHY_UPDATE_COMPLETE)) {
		BT_WARN("handle: 0x%04x, status: %x, tx: %x, rx: %x.", handle,
			radio_le_phy_upd_cmplt->status,
			find_lsb_set(radio_le_phy_upd_cmplt->tx),
			find_lsb_set(radio_le_phy_upd_cmplt->rx));
		return;
	}

	sep = meta_evt(buf, BT_HCI_EVT_LE_PHY_UPDATE_COMPLETE, sizeof(*sep));

	sep->status = radio_le_phy_upd_cmplt->status;
	sep->handle = sys_cpu_to_le16(handle);
	sep->tx_phy = find_lsb_set(radio_le_phy_upd_cmplt->tx);
	sep->rx_phy = find_lsb_set(radio_le_phy_upd_cmplt->rx);
}
#endif /* CONFIG_BT_CTLR_PHY */
#endif /* CONFIG_BT_CONN */

static void encode_control(struct radio_pdu_node_rx *node_rx,
			   struct pdu_data *pdu_data, struct net_buf *buf)
{
	u8_t *b = (u8_t *)node_rx;
	u16_t handle;

	handle = node_rx->hdr.handle;

	switch (node_rx->hdr.type) {
	case NODE_RX_TYPE_REPORT:
		le_advertising_report(pdu_data, b, buf);
		break;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	case NODE_RX_TYPE_EXT_1M_REPORT:
		le_adv_ext_1M_report(pdu_data, b, buf);
		break;

	case NODE_RX_TYPE_EXT_CODED_REPORT:
		le_adv_ext_coded_report(pdu_data, b, buf);
		break;
#endif /* CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY)
	case NODE_RX_TYPE_SCAN_REQ:
		le_scan_req_received(pdu_data, b, buf);
		break;
#endif /* CONFIG_BT_CTLR_SCAN_REQ_NOTIFY */

#if defined(CONFIG_BT_CONN)
	case NODE_RX_TYPE_CONNECTION:
		le_conn_complete(pdu_data, handle, buf);
		break;

	case NODE_RX_TYPE_TERMINATE:
		disconn_complete(pdu_data, handle, buf);
		break;

	case NODE_RX_TYPE_CONN_UPDATE:
		le_conn_update_complete(pdu_data, handle, buf);
		break;

#if defined(CONFIG_BT_CTLR_LE_ENC)
	case NODE_RX_TYPE_ENC_REFRESH:
		enc_refresh_complete(pdu_data, handle, buf);
		break;
#endif /* CONFIG_BT_CTLR_LE_ENC */

#if defined(CONFIG_BT_CTLR_LE_PING)
	case NODE_RX_TYPE_APTO:
		auth_payload_timeout_exp(pdu_data, handle, buf);
		break;
#endif /* CONFIG_BT_CTLR_LE_PING */

#if defined(CONFIG_BT_CTLR_CHAN_SEL_2)
	case NODE_RX_TYPE_CHAN_SEL_ALGO:
		le_chan_sel_algo(pdu_data, handle, buf);
		break;
#endif /* CONFIG_BT_CTLR_CHAN_SEL_2 */

#if defined(CONFIG_BT_CTLR_PHY)
	case NODE_RX_TYPE_PHY_UPDATE:
		le_phy_upd_complete(pdu_data, handle, buf);
		return;
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
	case NODE_RX_TYPE_RSSI:
		BT_INFO("handle: 0x%04x, rssi: -%d dB.", handle,
			pdu_data->payload.rssi);
		return;
#endif /* CONFIG_BT_CTLR_CONN_RSSI */
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_CTLR_ADV_INDICATION)
	case NODE_RX_TYPE_ADV_INDICATION:
		BT_INFO("Advertised.");
		return;
#endif /* CONFIG_BT_CTLR_ADV_INDICATION */

#if defined(CONFIG_BT_CTLR_PROFILE_ISR)
	case NODE_RX_TYPE_PROFILE:
		BT_INFO("l: %d, %d, %d; t: %d, %d, %d.",
			pdu_data->payload.profile.lcur,
			pdu_data->payload.profile.lmin,
			pdu_data->payload.profile.lmax,
			pdu_data->payload.profile.cur,
			pdu_data->payload.profile.min,
			pdu_data->payload.profile.max);
		return;
#endif /* CONFIG_BT_CTLR_PROFILE_ISR */

	default:
		LL_ASSERT(0);
		return;
	}
}

#if defined(CONFIG_BT_CTLR_LE_ENC)
static void le_ltk_request(struct pdu_data *pdu_data, u16_t handle,
			   struct net_buf *buf)
{
	struct bt_hci_evt_le_ltk_request *sep;

	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    !(le_event_mask & BT_EVT_MASK_LE_LTK_REQUEST)) {
		return;
	}

	sep = meta_evt(buf, BT_HCI_EVT_LE_LTK_REQUEST, sizeof(*sep));

	sep->handle = sys_cpu_to_le16(handle);
	memcpy(&sep->rand, pdu_data->payload.llctrl.ctrldata.enc_req.rand,
	       sizeof(u64_t));
	memcpy(&sep->ediv, pdu_data->payload.llctrl.ctrldata.enc_req.ediv,
	       sizeof(u16_t));
}

static void encrypt_change(u8_t err, u16_t handle,
			   struct net_buf *buf)
{
	struct bt_hci_evt_encrypt_change *ep;

	if (!(event_mask & BT_EVT_MASK_ENCRYPT_CHANGE)) {
		return;
	}

	evt_create(buf, BT_HCI_EVT_ENCRYPT_CHANGE, sizeof(*ep));
	ep = net_buf_add(buf, sizeof(*ep));

	ep->status = err;
	ep->handle = sys_cpu_to_le16(handle);
	ep->encrypt = !err ? 1 : 0;
}
#endif /* CONFIG_BT_CTLR_LE_ENC */

static void le_remote_feat_complete(u8_t status, struct pdu_data *pdu_data,
				    u16_t handle, struct net_buf *buf)
{
	struct bt_hci_evt_le_remote_feat_complete *sep;

	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    !(le_event_mask & BT_EVT_MASK_LE_REMOTE_FEAT_COMPLETE)) {
		return;
	}

	sep = meta_evt(buf, BT_HCI_EV_LE_REMOTE_FEAT_COMPLETE, sizeof(*sep));

	sep->status = status;
	sep->handle = sys_cpu_to_le16(handle);
	if (!status) {
		memcpy(&sep->features[0],
		       &pdu_data->payload.llctrl.ctrldata.feature_rsp.features[0],
		       sizeof(sep->features));
	} else {
		memset(&sep->features[0], 0x00, sizeof(sep->features));
	}
}

static void le_unknown_rsp(struct pdu_data *pdu_data, u16_t handle,
			   struct net_buf *buf)
{

	switch (pdu_data->payload.llctrl.ctrldata.unknown_rsp.type) {
	case PDU_DATA_LLCTRL_TYPE_SLAVE_FEATURE_REQ:
		le_remote_feat_complete(BT_HCI_ERR_UNSUPP_REMOTE_FEATURE,
					    NULL, handle, buf);
		break;

	default:
		BT_WARN("type: 0x%02x",
			pdu_data->payload.llctrl.ctrldata.unknown_rsp.type);
		break;
	}
}

static void remote_version_info(struct pdu_data *pdu_data, u16_t handle,
				struct net_buf *buf)
{
	struct pdu_data_llctrl_version_ind *ver_ind;
	struct bt_hci_evt_remote_version_info *ep;

	if (!(event_mask & BT_EVT_MASK_REMOTE_VERSION_INFO)) {
		return;
	}

	evt_create(buf, BT_HCI_EVT_REMOTE_VERSION_INFO, sizeof(*ep));
	ep = net_buf_add(buf, sizeof(*ep));

	ver_ind = &pdu_data->payload.llctrl.ctrldata.version_ind;
	ep->status = 0x00;
	ep->handle = sys_cpu_to_le16(handle);
	ep->version = ver_ind->version_number;
	ep->manufacturer = sys_cpu_to_le16(ver_ind->company_id);
	ep->subversion = sys_cpu_to_le16(ver_ind->sub_version_number);
}

static void le_conn_param_req(struct pdu_data *pdu_data, u16_t handle,
			      struct net_buf *buf)
{
	struct bt_hci_evt_le_conn_param_req *sep;

	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    !(le_event_mask & BT_EVT_MASK_LE_CONN_PARAM_REQ)) {
		/* event masked, reject the conn param req */
		ll_conn_update(handle, 2, BT_HCI_ERR_UNSUPP_REMOTE_FEATURE,
			       0, 0, 0);

		return;
	}

	sep = meta_evt(buf, BT_HCI_EVT_LE_CONN_PARAM_REQ, sizeof(*sep));

	sep->handle = sys_cpu_to_le16(handle);
	sep->interval_min =
		pdu_data->payload.llctrl.ctrldata.conn_param_req.interval_min;
	sep->interval_max =
		pdu_data->payload.llctrl.ctrldata.conn_param_req.interval_max;
	sep->latency = pdu_data->payload.llctrl.ctrldata.conn_param_req.latency;
	sep->timeout = pdu_data->payload.llctrl.ctrldata.conn_param_req.timeout;
}

static void le_data_len_change(struct pdu_data *pdu_data, u16_t handle,
			       struct net_buf *buf)
{
	struct bt_hci_evt_le_data_len_change *sep;

	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    !(le_event_mask & BT_EVT_MASK_LE_DATA_LEN_CHANGE)) {
		return;
	}

	sep = meta_evt(buf, BT_HCI_EVT_LE_DATA_LEN_CHANGE, sizeof(*sep));

	sep->handle = sys_cpu_to_le16(handle);
	sep->max_tx_octets =
		pdu_data->payload.llctrl.ctrldata.length_rsp.max_tx_octets;
	sep->max_tx_time =
		pdu_data->payload.llctrl.ctrldata.length_rsp.max_tx_time;
	sep->max_rx_octets =
		pdu_data->payload.llctrl.ctrldata.length_rsp.max_rx_octets;
	sep->max_rx_time =
		pdu_data->payload.llctrl.ctrldata.length_rsp.max_rx_time;
}

static void encode_data_ctrl(struct radio_pdu_node_rx *node_rx,
			     struct pdu_data *pdu_data, struct net_buf *buf)
{
	u16_t handle = node_rx->hdr.handle;

	switch (pdu_data->payload.llctrl.opcode) {

#if defined(CONFIG_BT_CTLR_LE_ENC)
	case PDU_DATA_LLCTRL_TYPE_ENC_REQ:
		le_ltk_request(pdu_data, handle, buf);
		break;

	case PDU_DATA_LLCTRL_TYPE_START_ENC_RSP:
		encrypt_change(0x00, handle, buf);
		break;
#endif /* CONFIG_BT_CTLR_LE_ENC */

	case PDU_DATA_LLCTRL_TYPE_FEATURE_RSP:
		le_remote_feat_complete(0x00, pdu_data, handle, buf);
		break;

	case PDU_DATA_LLCTRL_TYPE_VERSION_IND:
		remote_version_info(pdu_data, handle, buf);
		break;

#if defined(CONFIG_BT_CTLR_LE_ENC)
	case PDU_DATA_LLCTRL_TYPE_REJECT_IND:
		encrypt_change(pdu_data->payload.llctrl.ctrldata.reject_ind.
			       error_code,
			       handle, buf);
		break;
#endif /* CONFIG_BT_CTLR_LE_ENC */

	case PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ:
		le_conn_param_req(pdu_data, handle, buf);
		break;

	case PDU_DATA_LLCTRL_TYPE_LENGTH_REQ:
	case PDU_DATA_LLCTRL_TYPE_LENGTH_RSP:
		le_data_len_change(pdu_data, handle, buf);
		break;

	case PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP:
		le_unknown_rsp(pdu_data, handle, buf);
		break;

	default:
		LL_ASSERT(0);
		return;
	}
}

#if defined(CONFIG_BT_CONN)
void hci_acl_encode(struct radio_pdu_node_rx *node_rx, struct net_buf *buf)
{
	struct bt_hci_acl_hdr *acl;
	struct pdu_data *pdu_data;
	u16_t handle_flags;
	u16_t handle;
	u8_t *data;

	pdu_data = (struct pdu_data *)node_rx->pdu_data;
	handle = node_rx->hdr.handle;

	switch (pdu_data->ll_id) {
	case PDU_DATA_LLID_DATA_CONTINUE:
	case PDU_DATA_LLID_DATA_START:
		acl = (void *)net_buf_add(buf, sizeof(*acl));
		if (pdu_data->ll_id == PDU_DATA_LLID_DATA_START) {
			handle_flags = bt_acl_handle_pack(handle, BT_ACL_START);
		} else {
			handle_flags = bt_acl_handle_pack(handle, BT_ACL_CONT);
		}
		acl->handle = sys_cpu_to_le16(handle_flags);
		acl->len = sys_cpu_to_le16(pdu_data->len);
		data = (void *)net_buf_add(buf, pdu_data->len);
		memcpy(data, &pdu_data->payload.lldata[0], pdu_data->len);
#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
		if (hci_hbuf_total > 0) {
			LL_ASSERT((hci_hbuf_sent - hci_hbuf_acked) <
				  hci_hbuf_total);
			hci_hbuf_sent++;
			/* Note: This requires linear handle values starting
			 * from 0
			 */
			LL_ASSERT(handle < ARRAY_SIZE(hci_hbuf_pend));
			hci_hbuf_pend[handle]++;
		}
#endif
		break;

	default:
		LL_ASSERT(0);
		break;
	}

}
#endif

void hci_evt_encode(struct radio_pdu_node_rx *node_rx, struct net_buf *buf)
{
	struct pdu_data *pdu_data;

	pdu_data = (struct pdu_data *)node_rx->pdu_data;

	if (node_rx->hdr.type != NODE_RX_TYPE_DC_PDU) {
		encode_control(node_rx, pdu_data, buf);
	} else {
		encode_data_ctrl(node_rx, pdu_data, buf);
	}
}

void hci_num_cmplt_encode(struct net_buf *buf, u16_t handle, u8_t num)
{
	struct bt_hci_evt_num_completed_packets *ep;
	struct bt_hci_handle_count *hc;
	u8_t num_handles;
	u8_t len;

	num_handles = 1;

	len = (sizeof(*ep) + (sizeof(*hc) * num_handles));
	evt_create(buf, BT_HCI_EVT_NUM_COMPLETED_PACKETS, len);

	ep = net_buf_add(buf, len);
	ep->num_handles = num_handles;
	hc = &ep->h[0];
	hc->handle = sys_cpu_to_le16(handle);
	hc->count = sys_cpu_to_le16(num);
}

s8_t hci_get_class(struct radio_pdu_node_rx *node_rx)
{
	struct pdu_data *pdu_data;

	pdu_data = (struct pdu_data *)node_rx->pdu_data;

	if (node_rx->hdr.type != NODE_RX_TYPE_DC_PDU) {

		switch (node_rx->hdr.type) {
		case NODE_RX_TYPE_REPORT:
#if defined(CONFIG_BT_CTLR_ADV_EXT)
		case NODE_RX_TYPE_EXT_1M_REPORT:
		case NODE_RX_TYPE_EXT_CODED_REPORT:
#endif /* CONFIG_BT_CTLR_ADV_EXT */
#if defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY)
		case NODE_RX_TYPE_SCAN_REQ:
#endif /* CONFIG_BT_CTLR_SCAN_REQ_NOTIFY */
#if defined(CONFIG_BT_CTLR_ADV_INDICATION)
		case NODE_RX_TYPE_ADV_INDICATION:
#endif
#if defined(CONFIG_BT_CTLR_PROFILE_ISR)
		case NODE_RX_TYPE_PROFILE:
#endif
			return HCI_CLASS_EVT_DISCARDABLE;
		case NODE_RX_TYPE_CONNECTION:
			return HCI_CLASS_EVT_REQUIRED;
		case NODE_RX_TYPE_TERMINATE:
		case NODE_RX_TYPE_CONN_UPDATE:

#if defined(CONFIG_BT_CTLR_LE_ENC)
		case NODE_RX_TYPE_ENC_REFRESH:
#endif /* CONFIG_BT_CTLR_LE_ENC */

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
		case NODE_RX_TYPE_RSSI:
#endif
#if defined(CONFIG_BT_CTLR_LE_PING)
		case NODE_RX_TYPE_APTO:
#endif
#if defined(CONFIG_BT_CTLR_CHAN_SEL_2)
		case NODE_RX_TYPE_CHAN_SEL_ALGO:
#endif
#if defined(CONFIG_BT_CTLR_PHY)
		case NODE_RX_TYPE_PHY_UPDATE:
#endif /* CONFIG_BT_CTLR_PHY */
			return HCI_CLASS_EVT_CONNECTION;
		default:
			return -1;
		}

	} else if (pdu_data->ll_id == PDU_DATA_LLID_CTRL) {
		return HCI_CLASS_EVT_CONNECTION;
	} else {
		return HCI_CLASS_ACL_DATA;
	}
}

void hci_init(struct k_poll_signal *signal_host_buf)
{
#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
	hbuf_signal = signal_host_buf;
#endif
	reset(NULL, NULL);
}
