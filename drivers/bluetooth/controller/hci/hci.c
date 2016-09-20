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

#define HCI_PACKET_SIZE_MAX 255

enum {
	HCI_CMD = 0x01,
	HCI_DATA = 0x02,
	HCI_EVT = 0x04,
};

static struct {
	uint8_t tx[HCI_PACKET_SIZE_MAX];
} hci_context;

/* event packet length */
#define HCI_EVT_LEN(evt) ((uint8_t)(1 + sizeof(struct bt_hci_evt_hdr) + \
			  evt->len))

/* data packet length */
#define HCI_DATA_LEN(data_hdr) ((uint8_t)(1 + sizeof(struct bt_hci_acl_hdr) + \
				data_hdr->len))

/* command complete event length */
#define HCI_CC_LEN(type) ((uint8_t)(sizeof(struct bt_hci_evt_cmd_complete) + \
			  sizeof(type)))

/* le meta event length */
#define HCI_ME_LEN(type) ((uint8_t)(sizeof(struct bt_hci_evt_le_meta_event) + \
			  sizeof(type)))

/* direct access to the event parameters */
#define HCI_EVTP(evt_hdr) ((void *)((uint8_t *)evt_hdr + \
			   sizeof(struct bt_hci_evt_hdr)))

/* direct access to the command status event parameters */
#define HCI_CS(evt_hdr) ((struct bt_hci_evt_cmd_status *)HCI_EVTP(evt_hdr))

/* direct access to the command complete event parameters */
#define HCI_CC(evt_hdr) ((struct bt_hci_evt_cmd_complete *)HCI_EVTP(evt_hdr))

/* direct access to the command complete event return parameters */
#define HCI_CC_RP(evt_hdr) ((void *)(((uint8_t *)HCI_EVTP(evt_hdr)) + \
			    sizeof(struct bt_hci_evt_cmd_complete)))

/* direct access to the command complete status event parameters */
#define HCI_CC_ST(evt_hdr) ((struct bt_hci_evt_cc_status *)(HCI_CC_RP(evt_hdr)))

/* direct access to the LE meta event parameters */
#define HCI_ME(evt_hdr) ((struct bt_hci_evt_le_meta_event *)HCI_EVTP(evt_hdr))

/* direct access to the meta event subevent */
#define HCI_SE(evt_hdr) ((void *)(((uint8_t *)HCI_EVTP(evt_hdr)) + \
			 sizeof(struct bt_hci_evt_le_meta_event)))

static void disconnect(struct net_buf *buf, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_disconnect *cmd = (void *)buf->data;
	uint16_t handle;
	uint32_t status;

	handle = sys_le16_to_cpu(cmd->handle);
	status = radio_terminate_ind_send(handle, cmd->reason);

	evt->evt = BT_HCI_EVT_CMD_STATUS;
	evt->len = sizeof(struct bt_hci_evt_cmd_status);

	HCI_CS(evt)->status = (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED;
}

static void read_remote_ver_info(struct net_buf *buf,
				 struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_read_remote_version_info *cmd = (void *)buf->data;
	uint16_t handle;
	uint32_t status;

	handle = sys_le16_to_cpu(cmd->handle);
	status = radio_version_ind_send(handle);

	evt->evt = BT_HCI_EVT_CMD_STATUS;
	evt->len = sizeof(struct bt_hci_evt_cmd_status);

	HCI_CS(evt)->status = (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED;
}

static int link_control_cmd_handle(uint8_t ocf, struct net_buf *buf,
				   uint8_t *len, struct bt_hci_evt_hdr *evt)
{
	switch (ocf) {
	case BT_OCF(BT_HCI_OP_DISCONNECT):
		disconnect(buf, evt);
		break;
	case BT_OCF(BT_HCI_OP_READ_REMOTE_VERSION_INFO):
		read_remote_ver_info(buf, evt);
		break;
	default:
		return -EINVAL;
	}

	*len = HCI_EVT_LEN(evt);

	return 0;
}

static void set_event_mask(struct net_buf *buf, struct bt_hci_evt_hdr *evt)
{
	/** TODO */

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(struct bt_hci_evt_cc_status);

	HCI_CC_ST(evt)->status = 0x00;
}

static void reset(struct net_buf *buf, struct bt_hci_evt_hdr *evt)
{
	/** TODO */

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(struct bt_hci_evt_cc_status);

	HCI_CC_ST(evt)->status = 0x00;
}

static int ctrl_bb_cmd_handle(uint8_t ocf, struct net_buf *buf, uint8_t *len,
			      struct bt_hci_evt_hdr *evt)
{
	switch (ocf) {
	case BT_OCF(BT_HCI_OP_SET_EVENT_MASK):
		set_event_mask(buf, evt);
		break;

	case BT_OCF(BT_HCI_OP_RESET):
		reset(buf, evt);
		break;

	default:
		return -EINVAL;
	}

	*len = HCI_EVT_LEN(evt);

	return 0;
}

static void read_local_version_info(struct net_buf *buf,
				    struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_rp_read_local_version_info *rp = HCI_CC_RP(evt);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(*rp);

	rp->status = 0x00;
	rp->hci_version = 0;
	rp->hci_revision = sys_cpu_to_le16(0);
	rp->lmp_version = RADIO_BLE_VERSION_NUMBER;
	rp->manufacturer = sys_cpu_to_le16(RADIO_BLE_COMPANY_ID);
	rp->lmp_subversion = sys_cpu_to_le16(RADIO_BLE_SUB_VERSION_NUMBER);
}

static void read_supported_commands(struct net_buf *buf,
				    struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_rp_read_supported_commands *rp = HCI_CC_RP(evt);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(*rp);

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
	 * except LE Remove Device From White List
	 */
	rp->commands[27] = 0xFD;
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

static void read_local_features(struct net_buf *buf, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_rp_read_local_features *rp = HCI_CC_RP(evt);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(*rp);

	rp->status = 0x00;
	memset(&rp->features[0], 0x00, sizeof(rp->features));
	/* BR/EDR not supported and LE supported */
	rp->features[4] = (1 << 5) | (1 << 6);
}

static void read_bd_addr(struct net_buf *buf, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_rp_read_bd_addr *rp = HCI_CC_RP(evt);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(*rp);

	rp->status = 0x00;
	ll_address_get(0, &rp->bdaddr.val[0]);
}

static int info_cmd_handle(uint8_t ocf, struct net_buf *buf, uint8_t *len,
			   struct bt_hci_evt_hdr *evt)
{
	switch (ocf) {
	case BT_OCF(BT_HCI_OP_READ_LOCAL_VERSION_INFO):
		read_local_version_info(buf, evt);
		break;

	case BT_OCF(BT_HCI_OP_READ_SUPPORTED_COMMANDS):
		read_supported_commands(buf, evt);
		break;

	case BT_OCF(BT_HCI_OP_READ_LOCAL_FEATURES):
		read_local_features(buf, evt);
		break;

	case BT_OCF(BT_HCI_OP_READ_BD_ADDR):
		read_bd_addr(buf, evt);
		break;

	default:
		return -EINVAL;
	}

	*len = HCI_EVT_LEN(evt);

	return 0;
}

static void le_set_event_mask(struct net_buf *buf, struct bt_hci_evt_hdr *evt)
{
	/** TODO */

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(struct bt_hci_evt_cc_status);

	HCI_CC_ST(evt)->status = 0x00;
}

static void le_read_buffer_size(struct net_buf *buf, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_rp_le_read_buffer_size *rp = HCI_CC_RP(evt);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(*rp);

	rp->status = 0x00;

	rp->le_max_len = sys_cpu_to_le16(RADIO_LL_LENGTH_OCTETS_RX_MAX);
	rp->le_max_num = RADIO_PACKET_COUNT_TX_MAX;
}

static void le_read_local_features(struct net_buf *buf,
				   struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_rp_le_read_local_features *rp = HCI_CC_RP(evt);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(*rp);

	rp->status = 0x00;

	memset(&rp->features[0], 0x00, sizeof(rp->features));
	rp->features[0] = RADIO_BLE_FEATURES;
}

static void le_set_random_address(struct net_buf *buf,
				  struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_le_set_random_address *cmd = (void *)buf->data;

	ll_address_set(1, &cmd->bdaddr.val[0]);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(struct bt_hci_evt_cc_status);

	HCI_CC_ST(evt)->status = 0x00;
}

static void le_set_adv_param(struct net_buf *buf, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_le_set_adv_param *cmd = (void *)buf->data;
	uint8_t const c_adv_type[] = {
		PDU_ADV_TYPE_ADV_IND, PDU_ADV_TYPE_DIRECT_IND,
		PDU_ADV_TYPE_SCAN_IND, PDU_ADV_TYPE_NONCONN_IND };
	uint16_t min_interval;

	min_interval = sys_le16_to_cpu(cmd->min_interval);

	ll_adv_params_set(min_interval, c_adv_type[cmd->type],
			  cmd->own_addr_type, cmd->direct_addr.type,
			  &cmd->direct_addr.a.val[0], cmd->channel_map,
			  cmd->filter_policy);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(struct bt_hci_evt_cc_status);

	HCI_CC_ST(evt)->status = 0x00;
}

static void le_read_adv_ch_tx_power(struct net_buf *buf,
				    struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_rp_le_read_ch_tx_power *rp = HCI_CC_RP(evt);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(*rp);

	rp->status = 0x00;

	rp->tx_power_level = 0;
}

static void le_set_adv_data(struct net_buf *buf, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_le_set_adv_data *cmd = (void *)buf->data;

	ll_adv_data_set(cmd->len, &cmd->data[0]);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(struct bt_hci_evt_cc_status);

	HCI_CC_ST(evt)->status = 0x00;
}

static void le_set_scan_rsp_data(struct net_buf *buf,
				 struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_le_set_scan_rsp_data *cmd = (void *)buf->data;

	ll_scan_data_set(cmd->len, &cmd->data[0]);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(struct bt_hci_evt_cc_status);

	HCI_CC_ST(evt)->status = 0x00;
}

static void le_set_adv_enable(struct net_buf *buf, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_le_set_adv_enable *cmd = (void *)buf->data;
	uint32_t status;

	status = ll_adv_enable(cmd->enable);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(struct bt_hci_evt_cc_status);

	HCI_CC_ST(evt)->status = (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED;
}

static void le_set_scan_params(struct net_buf *buf, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_le_set_scan_params *cmd = (void *)buf->data;
	uint16_t interval;
	uint16_t window;

	interval = sys_le16_to_cpu(cmd->interval);
	window = sys_le16_to_cpu(cmd->window);

	ll_scan_params_set(cmd->scan_type, interval, window, cmd->addr_type,
			   cmd->filter_policy);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(struct bt_hci_evt_cc_status);

	HCI_CC_ST(evt)->status = 0x00;
}

static void le_set_scan_enable(struct net_buf *buf, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_le_set_scan_enable *cmd = (void *)buf->data;
	uint32_t status;

	status = ll_scan_enable(cmd->enable);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(struct bt_hci_evt_cc_status);

	HCI_CC_ST(evt)->status = (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED;
}

static void le_create_connection(struct net_buf *buf,
				 struct bt_hci_evt_hdr *evt)
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

	evt->evt = BT_HCI_EVT_CMD_STATUS;
	evt->len = sizeof(struct bt_hci_evt_cmd_status);

	HCI_CS(evt)->status = (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED;
}

static void le_create_conn_cancel(struct net_buf *buf,
				  struct bt_hci_evt_hdr *evt)
{
	uint32_t status;

	status = radio_connect_disable();

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(struct bt_hci_evt_cc_status);

	HCI_CC_ST(evt)->status = (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED;
}

static void le_read_wl_size(struct net_buf *buf, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_rp_le_read_wl_size *rp = HCI_CC_RP(evt);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(*rp);

	rp->status = 0x00;

	rp->wl_size = 8;
}

static void le_clear_wl(struct net_buf *buf, struct bt_hci_evt_hdr *evt)
{
	radio_filter_clear();

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(struct bt_hci_evt_cc_status);

	HCI_CC_ST(evt)->status = 0x00;
}

static void le_add_dev_to_wl(struct net_buf *buf, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_le_add_dev_to_wl *cmd = (void *)buf->data;
	uint32_t status;

	status = radio_filter_add(cmd->addr.type, &cmd->addr.a.val[0]);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(struct bt_hci_evt_cc_status);

	HCI_CC_ST(evt)->status = (!status) ? 0x00 :
					     BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
}

static void le_conn_update(struct net_buf *buf, struct bt_hci_evt_hdr *evt)
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

	evt->evt = BT_HCI_EVT_CMD_STATUS;
	evt->len = sizeof(struct bt_hci_evt_cmd_status);

	HCI_CS(evt)->status = (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED;
}

static void le_set_host_ch_classif(struct net_buf *buf,
				   struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_le_set_host_ch_classif *cmd = (void *)buf->data;
	uint32_t status;

	status = radio_chm_update(&cmd->ch_map[0]);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(struct bt_hci_evt_cc_status);

	HCI_CC_ST(evt)->status = (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED;
}

static void le_read_remote_features(struct net_buf *buf,
				    struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_le_read_remote_features *cmd = (void *)buf->data;
	uint32_t status;
	uint16_t handle;

	handle = sys_le16_to_cpu(cmd->handle);
	status = radio_feature_req_send(handle);

	evt->evt = BT_HCI_EVT_CMD_STATUS;
	evt->len = sizeof(struct bt_hci_evt_cmd_status);

	HCI_CS(evt)->status = (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED;
}

static void le_encrypt(struct net_buf *buf, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_le_encrypt *cmd = (void *)buf->data;
	struct bt_hci_rp_le_encrypt *rp = HCI_CC_RP(evt);

	ecb_encrypt(&cmd->key[0], &cmd->plaintext[0], &rp->enc_data[0], 0);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(*rp);

	rp->status = 0x00;
}

static void le_rand(struct net_buf *buf, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_rp_le_rand *rp = HCI_CC_RP(evt);
	uint8_t count = sizeof(rp->rand);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(*rp);

	rp->status = 0x00;

	while (count) {
		count = rand_get(count, rp->rand);
		if (count) {
			cpu_sleep();
		}
	}
}

static void le_start_encryption(struct net_buf *buf, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_le_start_encryption *cmd = (void *)buf->data;
	uint32_t status;
	uint16_t handle;

	handle = sys_le16_to_cpu(cmd->handle);
	status = radio_enc_req_send(handle,
			       (uint8_t *)&cmd->rand,
			       (uint8_t *)&cmd->ediv,
			       &cmd->ltk[0]);

	evt->evt = BT_HCI_EVT_CMD_STATUS;
	evt->len = sizeof(struct bt_hci_evt_cmd_status);

	HCI_CS(evt)->status = (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED;
}

static void le_ltk_req_reply(struct net_buf *buf, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_le_ltk_req_reply *cmd = (void *)buf->data;
	struct bt_hci_rp_le_ltk_req_reply *rp = HCI_CC_RP(evt);
	uint32_t status;
	uint16_t handle;

	handle = sys_le16_to_cpu(cmd->handle);
	status = radio_start_enc_req_send(handle, 0x00, &cmd->ltk[0]);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(*rp);
	rp->status = (!status) ?  0x00 : BT_HCI_ERR_CMD_DISALLOWED;
	rp->handle = cmd->handle;
}

static void le_ltk_req_neg_reply(struct net_buf *buf,
				 struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_le_ltk_req_neg_reply *cmd = (void *)buf->data;
	struct bt_hci_rp_le_ltk_req_neg_reply *rp = HCI_CC_RP(evt);
	uint32_t status;
	uint16_t handle;

	handle = sys_le16_to_cpu(cmd->handle);
	status = radio_start_enc_req_send(handle, BT_HCI_ERR_PIN_OR_KEY_MISSING,
					  NULL);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(*rp);
	rp->status = (!status) ?  0x00 : BT_HCI_ERR_CMD_DISALLOWED;
	rp->handle = cmd->handle;
}

static void le_read_supp_states(struct net_buf *buf, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_rp_le_read_supp_states *rp = HCI_CC_RP(evt);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(*rp);

	rp->status = 0x00;

	sys_put_le64(0x000003ffffffffff, rp->le_states);
}

static void le_conn_param_req_reply(struct net_buf *buf,
				    struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_le_conn_param_req_reply *cmd = (void *)buf->data;
	struct bt_hci_rp_le_conn_param_req_reply *rp = HCI_CC_RP(evt);
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

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(*rp);
	rp->status = (!status) ?  0x00 : BT_HCI_ERR_CMD_DISALLOWED;
	rp->handle = cmd->handle;
}

static void le_conn_param_req_neg_reply(struct net_buf *buf,
					struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_le_conn_param_req_neg_reply *cmd = (void *)buf->data;
	struct bt_hci_rp_le_conn_param_req_neg_reply *rp = HCI_CC_RP(evt);
	uint32_t status;
	uint16_t handle;

	handle = sys_le16_to_cpu(cmd->handle);
	status = radio_conn_update(handle, 2, cmd->reason, 0, 0, 0);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(*rp);
	rp->status = (!status) ?  0x00 : BT_HCI_ERR_CMD_DISALLOWED;
	rp->handle = cmd->handle;
}

static void le_set_data_len(struct net_buf *buf, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_le_set_data_len *cmd = (void *)buf->data;
	struct bt_hci_rp_le_set_data_len *rp = HCI_CC_RP(evt);
	uint32_t status;
	uint16_t tx_octets;
	uint16_t handle;

	handle = sys_le16_to_cpu(cmd->handle);
	tx_octets = sys_le16_to_cpu(cmd->tx_octets);
	/** @todo add reject_ext_ind support in ctrl.c */
	status = radio_length_req_send(handle, tx_octets);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(*rp);
	rp->status = (!status) ?  0x00 : BT_HCI_ERR_CMD_DISALLOWED;
	rp->handle = cmd->handle;
}

static int controller_cmd_handle(uint8_t ocf, struct net_buf *buf, uint8_t *len,
				 struct bt_hci_evt_hdr *evt)
{

	switch (ocf) {
	case BT_OCF(BT_HCI_OP_LE_SET_EVENT_MASK):
		le_set_event_mask(buf, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_READ_BUFFER_SIZE):
		le_read_buffer_size(buf, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_READ_LOCAL_FEATURES):
		le_read_local_features(buf, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_RANDOM_ADDRESS):
		le_set_random_address(buf, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_ADV_PARAM):
		le_set_adv_param(buf, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_READ_ADV_CH_TX_POWER):
		le_read_adv_ch_tx_power(buf, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_ADV_DATA):
		le_set_adv_data(buf, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_SCAN_RSP_DATA):
		le_set_scan_rsp_data(buf, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_ADV_ENABLE):
		le_set_adv_enable(buf, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_SCAN_PARAMS):
		le_set_scan_params(buf, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_SCAN_ENABLE):
		le_set_scan_enable(buf, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_CREATE_CONN):
		le_create_connection(buf, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_CREATE_CONN_CANCEL):
		le_create_conn_cancel(buf, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_READ_WL_SIZE):
		le_read_wl_size(buf, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_CLEAR_WL):
		le_clear_wl(buf, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_ADD_DEV_TO_WL):
		le_add_dev_to_wl(buf, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_CONN_UPDATE):
		le_conn_update(buf, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_HOST_CH_CLASSIF):
		le_set_host_ch_classif(buf, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_READ_REMOTE_FEATURES):
		le_read_remote_features(buf, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_ENCRYPT):
		le_encrypt(buf, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_RAND):
		le_rand(buf, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_START_ENCRYPTION):
		le_start_encryption(buf, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_LTK_REQ_REPLY):
		le_ltk_req_reply(buf, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_LTK_REQ_NEG_REPLY):
		le_ltk_req_neg_reply(buf, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_READ_SUPP_STATES):
		le_read_supp_states(buf, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_CONN_PARAM_REQ_REPLY):
		le_conn_param_req_reply(buf, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_CONN_PARAM_REQ_NEG_REPLY):
		le_conn_param_req_neg_reply(buf, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_DATA_LEN):
		le_set_data_len(buf, evt);
		break;

	default:
		return -EINVAL;
	}

	*len = HCI_EVT_LEN(evt);

	return 0;
}

static int hci_cmd_handle(struct net_buf *buf, uint8_t *len,
			   uint8_t **out)
{
	struct bt_hci_evt_cmd_complete *cc;
	struct bt_hci_evt_cmd_status *cs;
	struct bt_hci_cmd_hdr *cmd;
	struct bt_hci_evt_hdr *evt;
	uint16_t opcode;
	uint8_t ocf;
	int err;

	if (buf->len < sizeof(*cmd)) {
		BT_ERR("No HCI Command header");
		return -EINVAL;
	}

	cmd = (void *)buf->data;
	opcode = sys_le16_to_cpu(cmd->opcode);
	net_buf_pull(buf, sizeof(*cmd));

	if (buf->len < cmd->param_len) {
		BT_ERR("Invalid HCI CMD packet length");
		return -EINVAL;
	}

	*out = &hci_context.tx[0];
	hci_context.tx[0] = HCI_EVT;
	evt = (void *)&hci_context.tx[1];
	cc = HCI_CC(evt);
	cs = HCI_CS(evt);

	ocf = BT_OCF(opcode);

	switch (BT_OGF(opcode)) {
	case BT_OGF_LINK_CTRL:
		err = link_control_cmd_handle(ocf, buf, len, evt);
		break;
	case BT_OGF_BASEBAND:
		err = ctrl_bb_cmd_handle(ocf, buf, len, evt);
		break;
	case BT_OGF_INFO:
		err = info_cmd_handle(ocf, buf, len, evt);
		break;
	case BT_OGF_LE:
		err = controller_cmd_handle(ocf, buf, len, evt);
		break;
	case BT_OGF_VS:
		err = -EINVAL;
		break;
	default:
		err = -EINVAL;
		break;
	}

	if (err == -EINVAL) {
		evt->evt = BT_HCI_EVT_CMD_COMPLETE;
		evt->len = HCI_CC_LEN(struct bt_hci_evt_cc_status);
		HCI_CC_ST(evt)->status = BT_HCI_ERR_UNKNOWN_CMD;
		*len = HCI_EVT_LEN(evt);
	}

	switch (evt->evt) {
	case BT_HCI_EVT_CMD_COMPLETE:
		cc->ncmd = 1;
		cc->opcode = sys_cpu_to_le16(opcode);
		break;

	case BT_HCI_EVT_CMD_STATUS:
		cs->ncmd = 1;
		cs->opcode = sys_cpu_to_le16(opcode);
		break;
	default:
		break;
	}

	return 0;
}

static int hci_data_handle(struct net_buf *buf)
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

int hci_handle(struct net_buf *buf, uint8_t *len, uint8_t **out)
{
	uint8_t type;

	*len = 0;
	*out = NULL;

	if (!buf->len) {
		BT_ERR("Empty HCI packet");
		return -EINVAL;
	}

	type = bt_buf_get_type(buf);
	switch (type) {
	case BT_BUF_ACL_OUT:
		return hci_data_handle(buf);
	case BT_BUF_CMD:
		return hci_cmd_handle(buf, len, out);
	default:
		BT_ERR("Unknown HCI type %u", type);
		return -EINVAL;
	}
}

static void le_advertising_report(struct pdu_data *pdu_data, uint8_t *buf,
				  struct bt_hci_evt_hdr *evt)
{
	const uint8_t c_adv_type[] = { 0x00, 0x01, 0x03, 0xff, 0x04,
				       0xff, 0x02 };
	struct bt_hci_ev_le_advertising_report *sep = HCI_SE(evt);
	struct pdu_adv *adv = (struct pdu_adv *)pdu_data;
	struct bt_hci_ev_le_advertising_info *adv_info;
	uint8_t data_len;
	uint8_t *rssi;

	adv_info = (void *)(((uint8_t *)sep) + sizeof(*sep));

	evt->evt = BT_HCI_EVT_LE_META_EVENT;
	evt->len = HCI_ME_LEN(*sep);

	HCI_ME(evt)->subevent = BT_HCI_EVT_LE_ADVERTISING_REPORT;

	sep->num_reports = 1;

	adv_info->evt_type = c_adv_type[adv->type];
	adv_info->addr.type = adv->tx_addr;
	memcpy(&adv_info->addr.a.val[0], &adv->payload.adv_ind.addr[0],
	       sizeof(bt_addr_t));
	if (adv->type != PDU_ADV_TYPE_DIRECT_IND) {
		data_len = (adv->len - BDADDR_SIZE);
	} else {
		data_len = 0;
	}
	adv_info->length = data_len;
	memcpy(&adv_info->data[0], &adv->payload.adv_ind.data[0], data_len);
	/* RSSI */
	rssi = &adv_info->data[0] + data_len;
	*rssi = buf[offsetof(struct radio_pdu_node_rx, pdu_data) +
		offsetof(struct pdu_adv, payload) + adv->len];

	evt->len += sizeof(struct bt_hci_ev_le_advertising_info) + data_len + 1;
}

static void le_conn_complete(struct pdu_data *pdu_data, uint16_t handle,
			     struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_evt_le_conn_complete *sep = HCI_SE(evt);
	struct radio_le_conn_cmplt *radio_cc;

	radio_cc = (struct radio_le_conn_cmplt *) (pdu_data->payload.lldata);

	evt->evt = BT_HCI_EVT_LE_META_EVENT;
	evt->len = HCI_ME_LEN(*sep);
	HCI_ME(evt)->subevent = BT_HCI_EVT_LE_CONN_COMPLETE;

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
			     struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_evt_disconn_complete *ep = HCI_EVTP(evt);

	evt->evt = BT_HCI_EVT_DISCONN_COMPLETE;
	evt->len = sizeof(*ep);

	ep->status = 0x00;
	ep->handle = sys_cpu_to_le16(handle);
	ep->reason = *((uint8_t *)pdu_data);
}

static void le_conn_update_complete(struct pdu_data *pdu_data, uint16_t handle,
				    struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_evt_le_conn_update_complete *sep = HCI_SE(evt);
	struct radio_le_conn_update_cmplt *radio_cu;

	radio_cu = (struct radio_le_conn_update_cmplt *)
			(pdu_data->payload.lldata);

	evt->evt = BT_HCI_EVT_LE_META_EVENT;
	evt->len = HCI_ME_LEN(*sep);
	HCI_ME(evt)->subevent = BT_HCI_EVT_LE_CONN_UPDATE_COMPLETE;

	sep->status = radio_cu->status;
	sep->handle = sys_cpu_to_le16(handle);
	sep->interval = sys_cpu_to_le16(radio_cu->interval);
	sep->latency = sys_cpu_to_le16(radio_cu->latency);
	sep->supv_timeout = sys_cpu_to_le16(radio_cu->timeout);
}

static void enc_refresh_complete(struct pdu_data *pdu_data, uint16_t handle,
				 struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_evt_encrypt_key_refresh_complete *ep = HCI_EVTP(evt);

	evt->evt = BT_HCI_EVT_ENCRYPT_KEY_REFRESH_COMPLETE;
	evt->len = sizeof(*ep);

	ep->status = 0x00;
	ep->handle = sys_cpu_to_le16(handle);
}

static void auth_payload_timeout_exp(struct pdu_data *pdu_data, uint16_t handle,
				     struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_evt_auth_payload_timeout_exp *ep = HCI_EVTP(evt);

	evt->evt = BT_HCI_EVT_AUTH_PAYLOAD_TIMEOUT_EXP;
	evt->len = sizeof(*ep);

	ep->handle = sys_cpu_to_le16(handle);
}

static void encode_control(struct radio_pdu_node_rx *radio_pdu_node_rx,
			   struct pdu_data *pdu_data, uint8_t *len,
			   struct bt_hci_evt_hdr *evt)
{
	uint8_t *buf = (uint8_t *)radio_pdu_node_rx;
	uint16_t handle;

	handle = radio_pdu_node_rx->hdr.handle;

	switch (radio_pdu_node_rx->hdr.type) {
	case NODE_RX_TYPE_REPORT:
		le_advertising_report(pdu_data, buf, evt);
		break;

	case NODE_RX_TYPE_CONNECTION:
		le_conn_complete(pdu_data, handle, evt);
		break;

	case NODE_RX_TYPE_TERMINATE:
		disconn_complete(pdu_data, handle, evt);
		break;

	case NODE_RX_TYPE_CONN_UPDATE:
		le_conn_update_complete(pdu_data, handle, evt);
		break;

	case NODE_RX_TYPE_ENC_REFRESH:
		enc_refresh_complete(pdu_data, handle, evt);
		break;

	case NODE_RX_TYPE_APTO:
		auth_payload_timeout_exp(pdu_data, handle, evt);
		break;

	case NODE_RX_TYPE_RSSI:
		/** @todo */
		return;

	case NODE_RX_TYPE_PROFILE:
		/** @todo */
		return;

	default:
		BT_ASSERT(0);
		return;
	}

	*len = HCI_EVT_LEN(evt);
}

static void le_ltk_request(struct pdu_data *pdu_data, uint16_t handle,
				    struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_evt_le_ltk_request *sep = HCI_SE(evt);

	evt->evt = BT_HCI_EVT_LE_META_EVENT;
	evt->len = HCI_ME_LEN(*sep);
	HCI_ME(evt)->subevent = BT_HCI_EVT_LE_LTK_REQUEST;

	sep->handle = sys_cpu_to_le16(handle);
	memcpy(&sep->rand, pdu_data->payload.llctrl.ctrldata.enc_req.rand,
	       sizeof(uint64_t));
	memcpy(&sep->ediv, pdu_data->payload.llctrl.ctrldata.enc_req.ediv,
	       sizeof(uint16_t));
}

static void encrypt_change(uint8_t err, uint16_t handle,
			   struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_evt_encrypt_change *ep = HCI_EVTP(evt);

	evt->evt = BT_HCI_EVT_ENCRYPT_CHANGE;
	evt->len = sizeof(*ep);

	ep->status = err;
	ep->handle = sys_cpu_to_le16(handle);
	ep->encrypt = !err ? 1 : 0;
}

static void le_remote_feat_complete(struct pdu_data *pdu_data, uint16_t handle,
				    struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_ev_le_remote_feat_complete *sep = HCI_SE(evt);

	evt->evt = BT_HCI_EVT_LE_META_EVENT;
	evt->len = HCI_ME_LEN(*sep);
	HCI_ME(evt)->subevent = BT_HCI_EV_LE_REMOTE_FEAT_COMPLETE;

	sep->status = 0x00;
	sep->handle = sys_cpu_to_le16(handle);
	memcpy(&sep->features[0],
	       &pdu_data->payload.llctrl.ctrldata.feature_rsp.features[0],
	       sizeof(sep->features));
}

static void remote_version_info(struct pdu_data *pdu_data, uint16_t handle,
				struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_evt_remote_version_info *ep = HCI_EVTP(evt);

	evt->evt = BT_HCI_EVT_REMOTE_VERSION_INFO;
	evt->len = sizeof(*ep);

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
			      struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_evt_le_conn_param_req *sep = HCI_SE(evt);

	evt->evt = BT_HCI_EVT_LE_META_EVENT;
	evt->len = HCI_ME_LEN(*sep);
	HCI_ME(evt)->subevent = BT_HCI_EVT_LE_CONN_PARAM_REQ;

	sep->handle = sys_cpu_to_le16(handle);
	sep->interval_min =
		pdu_data->payload.llctrl.ctrldata.conn_param_req.interval_min;
	sep->interval_max =
		pdu_data->payload.llctrl.ctrldata.conn_param_req.interval_max;
	sep->latency = pdu_data->payload.llctrl.ctrldata.conn_param_req.latency;
	sep->timeout = pdu_data->payload.llctrl.ctrldata.conn_param_req.timeout;
}

static void le_data_len_change(struct pdu_data *pdu_data, uint16_t handle,
			       struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_evt_le_data_len_change *sep = HCI_SE(evt);

	evt->evt = BT_HCI_EVT_LE_META_EVENT;
	evt->len = HCI_ME_LEN(*sep);
	HCI_ME(evt)->subevent =  BT_HCI_EVT_LE_DATA_LEN_CHANGE;

	sep->handle = sys_cpu_to_le16(handle);
	sep->max_tx_octets =
		pdu_data->payload.llctrl.ctrldata.length_rsp.max_tx_octets;
	sep->max_tx_time =
		pdu_data->payload.llctrl.ctrldata.length_rsp.max_tx_time;
	sep->max_rx_octets =
		pdu_data->payload.llctrl.ctrldata.length_rsp.max_rx_octets;
	sep->max_rx_time =
		pdu_data->payload.llctrl.ctrldata.length_rsp.max_rx_time;
#if (TEST_DATA_LENGTH && TEST_TX)
	{
		extern uint16_t g_data_length;

		g_data_length = pdu_data->payload.llctrl.ctrldata.
						       length_rsp.max_tx_octets;
	}
#endif

}

static void encode_data_ctrl(struct radio_pdu_node_rx *radio_pdu_node_rx,
			     struct pdu_data *pdu_data, uint8_t *len,
			     struct bt_hci_evt_hdr *evt)
{
	uint16_t handle = radio_pdu_node_rx->hdr.handle;

	switch (pdu_data->payload.llctrl.opcode) {
	case PDU_DATA_LLCTRL_TYPE_ENC_REQ:
		le_ltk_request(pdu_data, handle, evt);
		break;

	case PDU_DATA_LLCTRL_TYPE_START_ENC_RSP:
		encrypt_change(0x00, handle, evt);
		break;

	case PDU_DATA_LLCTRL_TYPE_FEATURE_RSP:
		le_remote_feat_complete(pdu_data, handle, evt);
		break;

	case PDU_DATA_LLCTRL_TYPE_VERSION_IND:
		remote_version_info(pdu_data, handle, evt);
		break;

	case PDU_DATA_LLCTRL_TYPE_REJECT_IND:
		encrypt_change(pdu_data->payload.llctrl.ctrldata.reject_ind.
			       error_code,
			       handle, evt);
		break;

	case PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ:
		le_conn_param_req(pdu_data, handle, evt);
		break;

	case PDU_DATA_LLCTRL_TYPE_LENGTH_REQ:
	case PDU_DATA_LLCTRL_TYPE_LENGTH_RSP:
		le_data_len_change(pdu_data, handle, evt);
		break;

	default:
		BT_ASSERT(0);
		return;
	}

	*len = HCI_EVT_LEN(evt);
}

static void encode_data(uint8_t *buf, uint8_t *len, uint8_t **out)
{
	struct radio_pdu_node_rx *radio_pdu_node_rx;
	struct bt_hci_acl_hdr *acl;
	struct pdu_data *pdu_data;
	uint16_t handle_flags;
	uint16_t handle;

	radio_pdu_node_rx = (struct radio_pdu_node_rx *)buf;
	pdu_data = (struct pdu_data *)radio_pdu_node_rx->pdu_data;
	handle = radio_pdu_node_rx->hdr.handle;

	switch (pdu_data->ll_id) {
	case PDU_DATA_LLID_DATA_CONTINUE:
	case PDU_DATA_LLID_DATA_START:
#if !TEST_DROP_RX
		hci_context.tx[0] = HCI_DATA;

		acl = (struct bt_hci_acl_hdr *)&hci_context.tx[1];
		if (pdu_data->ll_id == PDU_DATA_LLID_DATA_START) {
			handle_flags = bt_acl_handle_pack(handle, BT_ACL_START);
		} else {
			handle_flags = bt_acl_handle_pack(handle, BT_ACL_CONT);
		}
		acl->handle = sys_cpu_to_le16(handle_flags);
		acl->len = sys_cpu_to_le16(pdu_data->len);
		memcpy(((uint8_t *)acl) + sizeof(struct bt_hci_acl_hdr),
		       &pdu_data->payload.lldata[0], pdu_data->len);

		*len = HCI_DATA_LEN(acl);
		*out = &hci_context.tx[0];
#else
		if (s_rx_cnt != pdu_data->payload.lldata[0]) {
			s_rx_cnt = pdu_data->payload.lldata[0];

			BT_ASSERT(0);
		} else {
			uint8_t index;

			for (index = 0; index < pdu_data->len; index++) {
				BT_ASSERT(pdu_data->payload.lldata[index] ==
					  (uint8_t)(s_rx_cnt + index));
			}

			s_rx_cnt++;
		}
#endif
		break;

	default:
		BT_ASSERT(0);
		break;
	}

}

void hcic_encode(uint8_t *buf, uint8_t *len, uint8_t **out)
{
	struct radio_pdu_node_rx *radio_pdu_node_rx;
	struct pdu_data *pdu_data;
	struct bt_hci_evt_hdr *evt;

	radio_pdu_node_rx = (struct radio_pdu_node_rx *)buf;
	pdu_data = (struct pdu_data *)radio_pdu_node_rx->pdu_data;
	*len = 0;
	*out = NULL;

	/* Check if we need to generate an HCI event or ACL data */
	if (radio_pdu_node_rx->hdr.type != NODE_RX_TYPE_DC_PDU ||
	    pdu_data->ll_id == PDU_DATA_LLID_CTRL) {
		/* generate an HCI event */
		hci_context.tx[0] = HCI_EVT;
		evt = (void *)&hci_context.tx[1];

		if (radio_pdu_node_rx->hdr.type != NODE_RX_TYPE_DC_PDU) {
			encode_control(radio_pdu_node_rx, pdu_data, len, evt);
		} else {
			encode_data_ctrl(radio_pdu_node_rx, pdu_data, len, evt);
		}
		*out = &hci_context.tx[0];
	} else {
		/* generate ACL data */
		encode_data(buf, len, out);
	}
}

void hcic_encode_num_cmplt(uint16_t handle, uint8_t num, uint8_t *len,
			   uint8_t **out)
{
	struct bt_hci_evt_num_completed_packets *ep;
	struct bt_hci_handle_count *hc;
	struct bt_hci_evt_hdr *evt;
	uint8_t num_handles;

	num_handles = 1;

	hci_context.tx[0] = HCI_EVT;

	evt = (struct bt_hci_evt_hdr *)&hci_context.tx[1];
	ep = HCI_EVTP(evt);
	evt->evt = BT_HCI_EVT_NUM_COMPLETED_PACKETS;
	evt->len = (sizeof(struct bt_hci_evt_num_completed_packets) +
		    (sizeof(struct bt_hci_handle_count) * num_handles));

	ep->num_handles = num_handles;
	hc = &ep->h[0];
	hc->handle = sys_cpu_to_le16(handle);
	hc->count = sys_cpu_to_le16(num);

	*len = HCI_EVT_LEN(evt);
	*out = &hci_context.tx[0];
}
