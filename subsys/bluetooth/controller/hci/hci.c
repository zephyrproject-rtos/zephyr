/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <toolchain.h>
#include <errno.h>
#include <misc/byteorder.h>
#include <bluetooth/hci.h>
#include <bluetooth/buf.h>
#include <bluetooth/bluetooth.h>

#include "defines.h"
#include "ticker.h"
#include "mem.h"
#include "rand.h"
#include "cpu.h"
#include "ecb.h"
#include "ccm.h"
#include "radio.h"
#include "pdu.h"
#include "ctrl.h"
#include "ll.h"
#include "hci_internal.h"

#include "debug.h"

/* opcode of the HCI command currently being processed. The opcode is stored
 * by hci_cmd_handle() and then used during the creation of cmd complete and
 * cmd status events to avoid passing it up the call chain.
 */
static uint16_t _opcode;

static void evt_create(struct net_buf *buf, uint8_t evt, uint8_t len)
{
	struct bt_hci_evt_hdr *hdr;

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->evt = evt;
	hdr->len = len;
}

static void *cmd_complete(struct net_buf *buf, uint8_t plen)
{
	struct bt_hci_evt_cmd_complete *cc;

	evt_create(buf, BT_HCI_EVT_CMD_COMPLETE, sizeof(*cc) + plen);

	cc = net_buf_add(buf, sizeof(*cc));
	cc->ncmd = 1;
	cc->opcode = sys_cpu_to_le16(_opcode);
	return net_buf_add(buf, plen);
}

static void cmd_status(struct net_buf *buf, uint8_t status)
{
	struct bt_hci_evt_cmd_status *cs;

	evt_create(buf, BT_HCI_EVT_CMD_STATUS, sizeof(*cs));

	cs = net_buf_add(buf, sizeof(*cs));
	cs->status = status;
	cs->ncmd = 1;
	cs->opcode = sys_cpu_to_le16(_opcode);
}

static void *meta_evt(struct net_buf *buf, uint8_t subevt, uint8_t melen)
{
	struct bt_hci_evt_le_meta_event *me;

	evt_create(buf, BT_HCI_EVT_LE_META_EVENT, sizeof(*me) + melen);
	me = net_buf_add(buf, sizeof(*me));
	me->subevent = subevt;

	return net_buf_add(buf, melen);
}

static void disconnect(struct net_buf *buf, struct net_buf *evt)
{
	struct bt_hci_cp_disconnect *cmd = (void *)buf->data;
	uint16_t handle;
	uint32_t status;

	handle = sys_le16_to_cpu(cmd->handle);
	status = radio_terminate_ind_send(handle, cmd->reason);

	cmd_status(evt, (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED);
}

static void read_remote_ver_info(struct net_buf *buf, struct net_buf *evt)
{
	struct bt_hci_cp_read_remote_version_info *cmd = (void *)buf->data;
	uint16_t handle;
	uint32_t status;

	handle = sys_le16_to_cpu(cmd->handle);
	status = radio_version_ind_send(handle);

	cmd_status(evt, (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED);
}

static int link_control_cmd_handle(uint8_t ocf, struct net_buf *cmd,
				   struct net_buf *evt)
{
	switch (ocf) {
	case BT_OCF(BT_HCI_OP_DISCONNECT):
		disconnect(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_READ_REMOTE_VERSION_INFO):
		read_remote_ver_info(cmd, evt);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static void set_event_mask(struct net_buf *buf, struct net_buf *evt)
{
	struct bt_hci_evt_cc_status *ccst;

	/** TODO */

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = 0x00;
}

static void reset(struct net_buf *buf, struct net_buf *evt)
{
	struct bt_hci_evt_cc_status *ccst;

	/** TODO */

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = 0x00;
}

static int ctrl_bb_cmd_handle(uint8_t ocf, struct net_buf *cmd,
			      struct net_buf *evt)
{
	switch (ocf) {
	case BT_OCF(BT_HCI_OP_SET_EVENT_MASK):
		set_event_mask(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_RESET):
		reset(cmd, evt);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static void read_local_version_info(struct net_buf *buf, struct net_buf *evt)
{
	struct bt_hci_rp_read_local_version_info *rp;

	rp = cmd_complete(evt, sizeof(*rp));

	rp->status = 0x00;
	rp->hci_version = BT_HCI_VERSION_4_2;
	rp->hci_revision = sys_cpu_to_le16(0);
	rp->lmp_version = RADIO_BLE_VERSION_NUMBER;
	rp->manufacturer = sys_cpu_to_le16(RADIO_BLE_COMPANY_ID);
	rp->lmp_subversion = sys_cpu_to_le16(RADIO_BLE_SUB_VERSION_NUMBER);
}

static void read_supported_commands(struct net_buf *buf, struct net_buf *evt)
{
	struct bt_hci_rp_read_supported_commands *rp;

	rp = cmd_complete(evt, sizeof(*rp));

	rp->status = 0x00;
	memset(&rp->commands[0], 0, sizeof(rp->commands));
	/* Disconnect. */
	rp->commands[0] = (1 << 5);
	/* Set Event Mask, and Reset. */
	rp->commands[5] = (1 << 6) | (1 << 7);
	/* Read Local Version Info, Read Local Supported Features. */
	rp->commands[14] = (1 << 3) | (1 << 5);
	/* Read BD ADDR. */
	rp->commands[15] = (1 << 1);
	/* All LE commands in this octet. */
	rp->commands[25] = 0xF7;
	/* All LE commands in this octet. */
	rp->commands[26] = 0xFF;
	/* All LE commands in this octet,
	 */
	rp->commands[27] = 0xFF;
	/* LE Start Encryption, LE Long Term Key Req Reply,
	 * LE Long Term Key Req Neg Reply. and
	 * LE Read Supported States.
	 */
	rp->commands[28] = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3);
	/* LE Remote Conn Param Req and Neg Reply, LE Set Data Length,
	 * and LE Read Suggested Data Length.
	 */
	rp->commands[33] = (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7);
	/* LE Write Suggested Data Length. */
	rp->commands[34] = (1 << 0);
	/* LE Read Maximum Data Length. */
	rp->commands[35] = (1 << 3);
}

static void read_local_features(struct net_buf *buf, struct net_buf *evt)
{
	struct bt_hci_rp_read_local_features *rp;

	rp = cmd_complete(evt, sizeof(*rp));

	rp->status = 0x00;
	memset(&rp->features[0], 0x00, sizeof(rp->features));
	/* BR/EDR not supported and LE supported */
	rp->features[4] = (1 << 5) | (1 << 6);
}

static void read_bd_addr(struct net_buf *buf, struct net_buf *evt)
{
	struct bt_hci_rp_read_bd_addr *rp;

	rp = cmd_complete(evt, sizeof(*rp));

	rp->status = 0x00;
	ll_address_get(0, &rp->bdaddr.val[0]);
}

static int info_cmd_handle(uint8_t ocf, struct net_buf *cmd,
			   struct net_buf *evt)
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

static void le_set_event_mask(struct net_buf *buf, struct net_buf *evt)
{
	struct bt_hci_evt_cc_status *ccst;

	/** TODO */

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = 0x00;
}

static void le_read_buffer_size(struct net_buf *buf, struct net_buf *evt)
{
	struct bt_hci_rp_le_read_buffer_size *rp;

	rp = cmd_complete(evt, sizeof(*rp));

	rp->status = 0x00;

	rp->le_max_len = sys_cpu_to_le16(RADIO_LL_LENGTH_OCTETS_RX_MAX);
	rp->le_max_num = RADIO_PACKET_COUNT_TX_MAX;
}

static void le_read_local_features(struct net_buf *buf, struct net_buf *evt)
{
	struct bt_hci_rp_le_read_local_features *rp;

	rp = cmd_complete(evt, sizeof(*rp));

	rp->status = 0x00;

	memset(&rp->features[0], 0x00, sizeof(rp->features));
	rp->features[0] = RADIO_BLE_FEATURES;
}

static void le_set_random_address(struct net_buf *buf, struct net_buf *evt)
{
	struct bt_hci_cp_le_set_random_address *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;

	ll_address_set(1, &cmd->bdaddr.val[0]);

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = 0x00;
}

static void le_set_adv_param(struct net_buf *buf, struct net_buf *evt)
{
	struct bt_hci_cp_le_set_adv_param *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;
	uint8_t const c_adv_type[] = {
		PDU_ADV_TYPE_ADV_IND, PDU_ADV_TYPE_DIRECT_IND,
		PDU_ADV_TYPE_SCAN_IND, PDU_ADV_TYPE_NONCONN_IND };
	uint16_t min_interval;

	min_interval = sys_le16_to_cpu(cmd->min_interval);

	ll_adv_params_set(min_interval, c_adv_type[cmd->type],
			  cmd->own_addr_type, cmd->direct_addr.type,
			  &cmd->direct_addr.a.val[0], cmd->channel_map,
			  cmd->filter_policy);

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = 0x00;
}

static void le_read_adv_ch_tx_power(struct net_buf *buf, struct net_buf *evt)
{
	struct bt_hci_rp_le_read_ch_tx_power *rp;

	rp = cmd_complete(evt, sizeof(*rp));

	rp->status = 0x00;

	rp->tx_power_level = 0;
}

static void le_set_adv_data(struct net_buf *buf, struct net_buf *evt)
{
	struct bt_hci_cp_le_set_adv_data *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;

	ll_adv_data_set(cmd->len, &cmd->data[0]);

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = 0x00;
}

static void le_set_scan_rsp_data(struct net_buf *buf, struct net_buf *evt)
{
	struct bt_hci_cp_le_set_scan_rsp_data *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;

	ll_scan_data_set(cmd->len, &cmd->data[0]);

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = 0x00;
}

static void le_set_adv_enable(struct net_buf *buf, struct net_buf *evt)
{
	struct bt_hci_cp_le_set_adv_enable *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;
	uint32_t status;

	status = ll_adv_enable(cmd->enable);

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED;
}

static void le_set_scan_params(struct net_buf *buf, struct net_buf *evt)
{
	struct bt_hci_cp_le_set_scan_params *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;
	uint16_t interval;
	uint16_t window;

	interval = sys_le16_to_cpu(cmd->interval);
	window = sys_le16_to_cpu(cmd->window);

	ll_scan_params_set(cmd->scan_type, interval, window, cmd->addr_type,
			   cmd->filter_policy);

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = 0x00;
}

static void le_set_scan_enable(struct net_buf *buf, struct net_buf *evt)
{
	struct bt_hci_cp_le_set_scan_enable *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;
	uint32_t status;

	status = ll_scan_enable(cmd->enable);

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED;
}

static void le_create_connection(struct net_buf *buf, struct net_buf *evt)
{
	struct bt_hci_cp_le_create_conn *cmd = (void *)buf->data;
	uint16_t supervision_timeout;
	uint16_t conn_interval_max;
	uint16_t scan_interval;
	uint16_t conn_latency;
	uint16_t scan_window;
	uint32_t status;

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

	cmd_status(evt, (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED);
}

static void le_create_conn_cancel(struct net_buf *buf, struct net_buf *evt)
{
	struct bt_hci_evt_cc_status *ccst;
	uint32_t status;

	status = radio_connect_disable();

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED;
}

static void le_read_wl_size(struct net_buf *buf, struct net_buf *evt)
{
	struct bt_hci_rp_le_read_wl_size *rp;

	rp = cmd_complete(evt, sizeof(*rp));
	rp->status = 0x00;

	rp->wl_size = 8;
}

static void le_clear_wl(struct net_buf *buf, struct net_buf *evt)
{
	struct bt_hci_evt_cc_status *ccst;

	radio_filter_clear();

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = 0x00;
}

static void le_add_dev_to_wl(struct net_buf *buf, struct net_buf *evt)
{
	struct bt_hci_cp_le_add_dev_to_wl *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;
	uint32_t status;

	status = radio_filter_add(cmd->addr.type, &cmd->addr.a.val[0]);

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = (!status) ? 0x00 : BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
}

static void le_rem_dev_from_wl(struct net_buf *buf, struct net_buf *evt)
{
	struct bt_hci_cp_le_rem_dev_from_wl *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;
	uint32_t status;

	status = radio_filter_remove(cmd->addr.type, &cmd->addr.a.val[0]);

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED;
}

static void le_conn_update(struct net_buf *buf, struct net_buf *evt)
{
	struct hci_cp_le_conn_update *cmd = (void *)buf->data;
	uint16_t supervision_timeout;
	uint16_t conn_interval_max;
	uint16_t conn_latency;
	uint32_t status;
	uint16_t handle;

	handle = sys_le16_to_cpu(cmd->handle);
	conn_interval_max = sys_le16_to_cpu(cmd->conn_interval_max);
	conn_latency = sys_le16_to_cpu(cmd->conn_latency);
	supervision_timeout = sys_le16_to_cpu(cmd->supervision_timeout);

	/** @todo if peer supports LE Conn Param Req,
	* use Req cmd (1) instead of Initiate cmd (0).
	*/
	status = radio_conn_update(handle, 0, 0, conn_interval_max,
				   conn_latency, supervision_timeout);

	cmd_status(evt, (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED);
}

static void le_set_host_ch_classif(struct net_buf *buf, struct net_buf *evt)
{
	struct bt_hci_cp_le_set_host_ch_classif *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;
	uint32_t status;

	status = radio_chm_update(&cmd->ch_map[0]);

	ccst = cmd_complete(evt, sizeof(*ccst));
	ccst->status = (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED;
}

static void le_read_remote_features(struct net_buf *buf, struct net_buf *evt)
{
	struct bt_hci_cp_le_read_remote_features *cmd = (void *)buf->data;
	uint32_t status;
	uint16_t handle;

	handle = sys_le16_to_cpu(cmd->handle);
	status = radio_feature_req_send(handle);

	cmd_status(evt, (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED);
}

static void le_encrypt(struct net_buf *buf, struct net_buf *evt)
{
	struct bt_hci_cp_le_encrypt *cmd = (void *)buf->data;
	struct bt_hci_rp_le_encrypt *rp;

	rp = cmd_complete(evt, sizeof(*rp));

	ecb_encrypt(&cmd->key[0], &cmd->plaintext[0], &rp->enc_data[0], 0);

	rp->status = 0x00;
}

static void le_rand(struct net_buf *buf, struct net_buf *evt)
{
	struct bt_hci_rp_le_rand *rp;
	uint8_t count = sizeof(rp->rand);

	rp = cmd_complete(evt, sizeof(*rp));
	rp->status = 0x00;

	while (count) {
		count = rand_get(count, rp->rand);
		if (count) {
			cpu_sleep();
		}
	}
}

static void le_start_encryption(struct net_buf *buf, struct net_buf *evt)
{
	struct bt_hci_cp_le_start_encryption *cmd = (void *)buf->data;
	uint32_t status;
	uint16_t handle;

	handle = sys_le16_to_cpu(cmd->handle);
	status = radio_enc_req_send(handle,
			       (uint8_t *)&cmd->rand,
			       (uint8_t *)&cmd->ediv,
			       &cmd->ltk[0]);

	cmd_status(evt, (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED);
}

static void le_ltk_req_reply(struct net_buf *buf, struct net_buf *evt)
{
	struct bt_hci_cp_le_ltk_req_reply *cmd = (void *)buf->data;
	struct bt_hci_rp_le_ltk_req_reply *rp;
	uint32_t status;
	uint16_t handle;

	handle = sys_le16_to_cpu(cmd->handle);
	status = radio_start_enc_req_send(handle, 0x00, &cmd->ltk[0]);

	rp = cmd_complete(evt, sizeof(*rp));
	rp->status = (!status) ?  0x00 : BT_HCI_ERR_CMD_DISALLOWED;
	rp->handle = cmd->handle;
}

static void le_ltk_req_neg_reply(struct net_buf *buf, struct net_buf *evt)
{
	struct bt_hci_cp_le_ltk_req_neg_reply *cmd = (void *)buf->data;
	struct bt_hci_rp_le_ltk_req_neg_reply *rp;
	uint32_t status;
	uint16_t handle;

	handle = sys_le16_to_cpu(cmd->handle);
	status = radio_start_enc_req_send(handle, BT_HCI_ERR_PIN_OR_KEY_MISSING,
					  NULL);

	rp = cmd_complete(evt, sizeof(*rp));
	rp->status = (!status) ?  0x00 : BT_HCI_ERR_CMD_DISALLOWED;
	rp->handle = cmd->handle;
}

static void le_read_supp_states(struct net_buf *buf, struct net_buf *evt)
{
	struct bt_hci_rp_le_read_supp_states *rp;

	rp = cmd_complete(evt, sizeof(*rp));
	rp->status = 0x00;

	sys_put_le64(0x000003ffffffffff, rp->le_states);
}

static void le_conn_param_req_reply(struct net_buf *buf, struct net_buf *evt)
{
	struct bt_hci_cp_le_conn_param_req_reply *cmd = (void *)buf->data;
	struct bt_hci_rp_le_conn_param_req_reply *rp;
	uint16_t interval_max;
	uint16_t latency;
	uint16_t timeout;
	uint32_t status;
	uint16_t handle;

	handle = sys_le16_to_cpu(cmd->handle);
	interval_max = sys_le16_to_cpu(cmd->interval_max);
	latency = sys_le16_to_cpu(cmd->latency);
	timeout = sys_le16_to_cpu(cmd->timeout);

	status = radio_conn_update(handle, 2, 0, interval_max, latency,
				   timeout);

	rp = cmd_complete(evt, sizeof(*rp));
	rp->status = (!status) ?  0x00 : BT_HCI_ERR_CMD_DISALLOWED;
	rp->handle = cmd->handle;
}

static void le_conn_param_req_neg_reply(struct net_buf *buf,
					struct net_buf *evt)
{
	struct bt_hci_cp_le_conn_param_req_neg_reply *cmd = (void *)buf->data;
	struct bt_hci_rp_le_conn_param_req_neg_reply *rp;
	uint32_t status;
	uint16_t handle;

	handle = sys_le16_to_cpu(cmd->handle);
	status = radio_conn_update(handle, 2, cmd->reason, 0, 0, 0);

	rp = cmd_complete(evt, sizeof(*rp));
	rp->status = (!status) ?  0x00 : BT_HCI_ERR_CMD_DISALLOWED;
	rp->handle = cmd->handle;
}

static void le_set_data_len(struct net_buf *buf, struct net_buf *evt)
{
	struct bt_hci_cp_le_set_data_len *cmd = (void *)buf->data;
	struct bt_hci_rp_le_set_data_len *rp;
	uint32_t status;
	uint16_t tx_octets;
	uint16_t handle;

	handle = sys_le16_to_cpu(cmd->handle);
	tx_octets = sys_le16_to_cpu(cmd->tx_octets);
	/** @todo add reject_ext_ind support in ctrl.c */
	status = radio_length_req_send(handle, tx_octets);

	rp = cmd_complete(evt, sizeof(*rp));
	rp->status = (!status) ?  0x00 : BT_HCI_ERR_CMD_DISALLOWED;
	rp->handle = cmd->handle;
}

static int controller_cmd_handle(uint8_t ocf, struct net_buf *cmd,
				 struct net_buf *evt)
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

	case BT_OCF(BT_HCI_OP_LE_SET_ADV_PARAM):
		le_set_adv_param(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_READ_ADV_CH_TX_POWER):
		le_read_adv_ch_tx_power(cmd, evt);
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

	case BT_OCF(BT_HCI_OP_LE_SET_SCAN_PARAMS):
		le_set_scan_params(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_SCAN_ENABLE):
		le_set_scan_enable(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_CREATE_CONN):
		le_create_connection(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_CREATE_CONN_CANCEL):
		le_create_conn_cancel(cmd, evt);
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

	case BT_OCF(BT_HCI_OP_LE_CONN_UPDATE):
		le_conn_update(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_HOST_CH_CLASSIF):
		le_set_host_ch_classif(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_READ_REMOTE_FEATURES):
		le_read_remote_features(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_ENCRYPT):
		le_encrypt(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_RAND):
		le_rand(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_START_ENCRYPTION):
		le_start_encryption(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_LTK_REQ_REPLY):
		le_ltk_req_reply(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_LTK_REQ_NEG_REPLY):
		le_ltk_req_neg_reply(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_READ_SUPP_STATES):
		le_read_supp_states(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_CONN_PARAM_REQ_REPLY):
		le_conn_param_req_reply(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_CONN_PARAM_REQ_NEG_REPLY):
		le_conn_param_req_neg_reply(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_DATA_LEN):
		le_set_data_len(cmd, evt);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

int hci_cmd_handle(struct net_buf *cmd, struct net_buf *evt)
{
	struct bt_hci_evt_cc_status *ccst;
	struct bt_hci_cmd_hdr *chdr;
	uint8_t ocf;
	int err;

	if (cmd->len < sizeof(*chdr)) {
		BT_ERR("No HCI Command header");
		return -EINVAL;
	}

	chdr = (void *)cmd->data;
	/* store in a global for later CC/CS event creation */
	_opcode = sys_le16_to_cpu(chdr->opcode);

	if (cmd->len < chdr->param_len) {
		BT_ERR("Invalid HCI CMD packet length");
		return -EINVAL;
	}

	net_buf_pull(cmd, sizeof(*chdr));

	ocf = BT_OCF(_opcode);

	switch (BT_OGF(_opcode)) {
	case BT_OGF_LINK_CTRL:
		err = link_control_cmd_handle(ocf, cmd, evt);
		break;
	case BT_OGF_BASEBAND:
		err = ctrl_bb_cmd_handle(ocf, cmd, evt);
		break;
	case BT_OGF_INFO:
		err = info_cmd_handle(ocf, cmd, evt);
		break;
	case BT_OGF_LE:
		err = controller_cmd_handle(ocf, cmd, evt);
		break;
	case BT_OGF_VS:
		err = -EINVAL;
		break;
	default:
		err = -EINVAL;
		break;
	}

	if (err == -EINVAL) {
		ccst = cmd_complete(evt, sizeof(*ccst));
		ccst->status = BT_HCI_ERR_UNKNOWN_CMD;
	}

	return 0;
}

int hci_acl_handle(struct net_buf *buf)
{
	struct radio_pdu_node_tx *radio_pdu_node_tx;
	struct bt_hci_acl_hdr *acl;
	uint16_t handle;
	uint8_t flags;
	uint16_t len;

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
	if (radio_pdu_node_tx) {
		struct pdu_data *pdu_data;

		pdu_data = (struct pdu_data *)radio_pdu_node_tx->pdu_data;
		if (flags == BT_ACL_START_NO_FLUSH || flags == BT_ACL_START) {
			pdu_data->ll_id = PDU_DATA_LLID_DATA_START;
		} else {
			pdu_data->ll_id = PDU_DATA_LLID_DATA_CONTINUE;
		}
		pdu_data->len = len;
		memcpy(&pdu_data->payload.lldata[0], buf->data, len);
		if (radio_tx_mem_enqueue(handle, radio_pdu_node_tx)) {
			radio_tx_mem_release(radio_pdu_node_tx);
		}
	}

	return 0;
}

static void le_advertising_report(struct pdu_data *pdu_data, uint8_t *b,
				  struct net_buf *buf)
{
	const uint8_t c_adv_type[] = { 0x00, 0x01, 0x03, 0xff, 0x04,
				       0xff, 0x02 };
	struct bt_hci_ev_le_advertising_report *sep;
	struct pdu_adv *adv = (struct pdu_adv *)pdu_data;
	struct bt_hci_ev_le_advertising_info *adv_info;
	uint8_t data_len;
	uint8_t *rssi;
	uint8_t info_len;

	if (adv->type != PDU_ADV_TYPE_DIRECT_IND) {
		data_len = (adv->len - BDADDR_SIZE);
	} else {
		data_len = 0;
	}

	info_len = sizeof(struct bt_hci_ev_le_advertising_info) + data_len +
		   sizeof(*rssi);
	sep = meta_evt(buf, BT_HCI_EVT_LE_ADVERTISING_REPORT,
		       sizeof(*sep) + info_len);

	sep->num_reports = 1;
	adv_info = (void *)(((uint8_t *)sep) + sizeof(*sep));

	adv_info->evt_type = c_adv_type[adv->type];
	adv_info->addr.type = adv->tx_addr;
	memcpy(&adv_info->addr.a.val[0], &adv->payload.adv_ind.addr[0],
	       sizeof(bt_addr_t));

	adv_info->length = data_len;
	memcpy(&adv_info->data[0], &adv->payload.adv_ind.data[0], data_len);
	/* RSSI */
	rssi = &adv_info->data[0] + data_len;
	*rssi = b[offsetof(struct radio_pdu_node_rx, pdu_data) +
		  offsetof(struct pdu_adv, payload) + adv->len];

}

static void le_conn_complete(struct pdu_data *pdu_data, uint16_t handle,
			     struct net_buf *buf)
{
	struct bt_hci_evt_le_conn_complete *sep;
	struct radio_le_conn_cmplt *radio_cc;

	radio_cc = (struct radio_le_conn_cmplt *) (pdu_data->payload.lldata);

	sep = meta_evt(buf, BT_HCI_EVT_LE_CONN_COMPLETE, sizeof(*sep));

	sep->status = radio_cc->status;
	sep->handle = sys_cpu_to_le16(handle);
	sep->role = radio_cc->role;
	sep->peer_addr.type = radio_cc->peer_addr_type;
	memcpy(&sep->peer_addr.a.val[0], &radio_cc->peer_addr[0], BDADDR_SIZE);
	sep->interval = sys_cpu_to_le16(radio_cc->interval);
	sep->latency = sys_cpu_to_le16(radio_cc->latency);
	sep->supv_timeout = sys_cpu_to_le16(radio_cc->timeout);
	sep->clock_accuracy = radio_cc->mca;
}

static void disconn_complete(struct pdu_data *pdu_data, uint16_t handle,
			     struct net_buf *buf)
{
	struct bt_hci_evt_disconn_complete *ep;

	evt_create(buf, BT_HCI_EVT_DISCONN_COMPLETE, sizeof(*ep));
	ep = net_buf_add(buf, sizeof(*ep));

	ep->status = 0x00;
	ep->handle = sys_cpu_to_le16(handle);
	ep->reason = *((uint8_t *)pdu_data);
}

static void le_conn_update_complete(struct pdu_data *pdu_data, uint16_t handle,
				    struct net_buf *buf)
{
	struct bt_hci_evt_le_conn_update_complete *sep;
	struct radio_le_conn_update_cmplt *radio_cu;

	radio_cu = (struct radio_le_conn_update_cmplt *)
			(pdu_data->payload.lldata);

	sep = meta_evt(buf, BT_HCI_EVT_LE_CONN_UPDATE_COMPLETE, sizeof(*sep));

	sep->status = radio_cu->status;
	sep->handle = sys_cpu_to_le16(handle);
	sep->interval = sys_cpu_to_le16(radio_cu->interval);
	sep->latency = sys_cpu_to_le16(radio_cu->latency);
	sep->supv_timeout = sys_cpu_to_le16(radio_cu->timeout);
}

static void enc_refresh_complete(struct pdu_data *pdu_data, uint16_t handle,
				 struct net_buf *buf)
{
	struct bt_hci_evt_encrypt_key_refresh_complete *ep;

	evt_create(buf, BT_HCI_EVT_ENCRYPT_KEY_REFRESH_COMPLETE, sizeof(*ep));
	ep = net_buf_add(buf, sizeof(*ep));

	ep->status = 0x00;
	ep->handle = sys_cpu_to_le16(handle);
}

static void auth_payload_timeout_exp(struct pdu_data *pdu_data, uint16_t handle,
				     struct net_buf *buf)
{
	struct bt_hci_evt_auth_payload_timeout_exp *ep;

	evt_create(buf, BT_HCI_EVT_AUTH_PAYLOAD_TIMEOUT_EXP, sizeof(*ep));
	ep = net_buf_add(buf, sizeof(*ep));

	ep->handle = sys_cpu_to_le16(handle);
}

static void encode_control(struct radio_pdu_node_rx *node_rx,
			   struct pdu_data *pdu_data, struct net_buf *buf)
{
	uint8_t *b = (uint8_t *)node_rx;
	uint16_t handle;

	handle = node_rx->hdr.handle;

	switch (node_rx->hdr.type) {
	case NODE_RX_TYPE_REPORT:
		le_advertising_report(pdu_data, b, buf);
		break;

	case NODE_RX_TYPE_CONNECTION:
		le_conn_complete(pdu_data, handle, buf);
		break;

	case NODE_RX_TYPE_TERMINATE:
		disconn_complete(pdu_data, handle, buf);
		break;

	case NODE_RX_TYPE_CONN_UPDATE:
		le_conn_update_complete(pdu_data, handle, buf);
		break;

	case NODE_RX_TYPE_ENC_REFRESH:
		enc_refresh_complete(pdu_data, handle, buf);
		break;

	case NODE_RX_TYPE_APTO:
		auth_payload_timeout_exp(pdu_data, handle, buf);
		break;

	case NODE_RX_TYPE_RSSI:
		/** @todo */
		return;

	case NODE_RX_TYPE_PROFILE:
		/** @todo */
		return;

	default:
		LL_ASSERT(0);
		return;
	}
}

static void le_ltk_request(struct pdu_data *pdu_data, uint16_t handle,
				    struct net_buf *buf)
{
	struct bt_hci_evt_le_ltk_request *sep;

	sep = meta_evt(buf, BT_HCI_EVT_LE_LTK_REQUEST, sizeof(*sep));

	sep->handle = sys_cpu_to_le16(handle);
	memcpy(&sep->rand, pdu_data->payload.llctrl.ctrldata.enc_req.rand,
	       sizeof(uint64_t));
	memcpy(&sep->ediv, pdu_data->payload.llctrl.ctrldata.enc_req.ediv,
	       sizeof(uint16_t));
}

static void encrypt_change(uint8_t err, uint16_t handle,
			   struct net_buf *buf)
{
	struct bt_hci_evt_encrypt_change *ep;

	evt_create(buf, BT_HCI_EVT_ENCRYPT_CHANGE, sizeof(*ep));
	ep = net_buf_add(buf, sizeof(*ep));

	ep->status = err;
	ep->handle = sys_cpu_to_le16(handle);
	ep->encrypt = !err ? 1 : 0;
}

static void le_remote_feat_complete(uint8_t status, struct pdu_data *pdu_data,
				    uint16_t handle, struct net_buf *buf)
{
	struct bt_hci_ev_le_remote_feat_complete *sep;

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

static void le_unknown_rsp(struct pdu_data *pdu_data, uint16_t handle,
			   struct net_buf *buf)
{

	switch (pdu_data->payload.llctrl.ctrldata.unknown_rsp.type) {
	case PDU_DATA_LLCTRL_TYPE_SLAVE_FEATURE_REQ:
		le_remote_feat_complete(BT_HCI_ERR_UNSUPP_REMOTE_FEATURE,
					    NULL, handle, buf);
		break;

	default:
		BT_ASSERT(0);
		break;
	}
}

static void remote_version_info(struct pdu_data *pdu_data, uint16_t handle,
				struct net_buf *buf)
{
	struct bt_hci_evt_remote_version_info *ep;

	evt_create(buf, BT_HCI_EVT_REMOTE_VERSION_INFO, sizeof(*ep));
	ep = net_buf_add(buf, sizeof(*ep));

	ep->status = 0x00;
	ep->handle = sys_cpu_to_le16(handle);
	ep->version =
	      pdu_data->payload.llctrl.ctrldata.version_ind.version_number;
	ep->manufacturer =
		pdu_data->payload.llctrl.ctrldata.version_ind.company_id;
	ep->subversion =
	      pdu_data->payload.llctrl.ctrldata.version_ind.sub_version_number;
}

static void le_conn_param_req(struct pdu_data *pdu_data, uint16_t handle,
			      struct net_buf *buf)
{
	struct bt_hci_evt_le_conn_param_req *sep;

	sep = meta_evt(buf, BT_HCI_EVT_LE_CONN_PARAM_REQ, sizeof(*sep));

	sep->handle = sys_cpu_to_le16(handle);
	sep->interval_min =
		pdu_data->payload.llctrl.ctrldata.conn_param_req.interval_min;
	sep->interval_max =
		pdu_data->payload.llctrl.ctrldata.conn_param_req.interval_max;
	sep->latency = pdu_data->payload.llctrl.ctrldata.conn_param_req.latency;
	sep->timeout = pdu_data->payload.llctrl.ctrldata.conn_param_req.timeout;
}

static void le_data_len_change(struct pdu_data *pdu_data, uint16_t handle,
			       struct net_buf *buf)
{
	struct bt_hci_evt_le_data_len_change *sep;

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
	uint16_t handle = node_rx->hdr.handle;

	switch (pdu_data->payload.llctrl.opcode) {
	case PDU_DATA_LLCTRL_TYPE_ENC_REQ:
		le_ltk_request(pdu_data, handle, buf);
		break;

	case PDU_DATA_LLCTRL_TYPE_START_ENC_RSP:
		encrypt_change(0x00, handle, buf);
		break;

	case PDU_DATA_LLCTRL_TYPE_FEATURE_RSP:
		le_remote_feat_complete(0x00, pdu_data, handle, buf);
		break;

	case PDU_DATA_LLCTRL_TYPE_VERSION_IND:
		remote_version_info(pdu_data, handle, buf);
		break;

	case PDU_DATA_LLCTRL_TYPE_REJECT_IND:
		encrypt_change(pdu_data->payload.llctrl.ctrldata.reject_ind.
			       error_code,
			       handle, buf);
		break;

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

void hci_acl_encode(struct radio_pdu_node_rx *node_rx, struct net_buf *buf)
{
	struct bt_hci_acl_hdr *acl;
	struct pdu_data *pdu_data;
	uint16_t handle_flags;
	uint16_t handle;
	uint8_t *data;

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
		break;

	default:
		LL_ASSERT(0);
		break;
	}

}

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

void hci_num_cmplt_encode(struct net_buf *buf, uint16_t handle, uint8_t num)
{
	struct bt_hci_evt_num_completed_packets *ep;
	struct bt_hci_handle_count *hc;
	uint8_t num_handles;
	uint8_t len;

	num_handles = 1;

	len = (sizeof(*ep) + (sizeof(*hc) * num_handles));
	evt_create(buf, BT_HCI_EVT_NUM_COMPLETED_PACKETS, len);

	ep = net_buf_add(buf, len);
	ep->num_handles = num_handles;
	hc = &ep->h[0];
	hc->handle = sys_cpu_to_le16(handle);
	hc->count = sys_cpu_to_le16(num);
}
