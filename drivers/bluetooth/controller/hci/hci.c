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

#include "defines.h"
#include "ticker.h"
#include "mem.h"
#include "ecb.h"
#include "ccm.h"
#include "radio.h"
#include "pdu.h"
#include "ctrl.h"
#include "ll.h"

#include <bluetooth/hci.h>
#include "hci.h"

#include "debug.h"

#define HCI_PACKET_SIZE_MAX 255

enum {
	HCI_CMD = 0x01,
	HCI_DATA = 0x02,
	HCI_EVT = 0x04,
};

/*****************************************************************************
 * HCI EVENTS
*****************************************************************************/

enum {
	HCI_EVT_CODE_DISCONNECTION_COMPLETE = 0x05,
	HCI_EVT_CODE_ENCRYPTION_CHANGE = 0x08,
	HCI_EVT_CODE_READ_REMOTE_VERSION_INFO_COMPLETE = 0x0C,
	HCI_EVT_CODE_COMMAND_COMPLETE = 0x0E,
	HCI_EVT_CODE_COMMAND_STATUS = 0x0F,
	HCI_EVT_CODE_NUM_COMPLETE = 0x13,
	HCI_EVT_CODE_ENCRYPTION_KEY_REFRESH_COMPLETE = 0x30,
	HCI_EVT_CODE_LE_META = 0x3E,
	HCI_EVT_CODE_APTO_EXPIRED = 0x57,
};

enum {
	HCI_EVT_ERROR_CODE_SUCCESS = 0x00,
	HCI_EVT_ERROR_CODE_UNKNOWN_HCI_COMMAND = 0x01,
	HCI_EVT_ERROR_CODE_PIN_OR_KEY_MISSING = 0x06,
	HCI_EVT_ERROR_CODE_MEM_CAPACITY_EXCEEDED = 0x07,
	HCI_EVT_ERROR_CODE_COMMAND_DISALLOWED = 0x0C,
};

struct __packed hci_evt_cmd_cmplt_unknown_hci_command {
	uint8_t status;
};

struct __packed hci_evt_disconnect_cmplt {
	uint8_t status;
	uint16_t conn_handle;
	uint8_t reason;
};

struct __packed hci_evt_encryption_change {
	uint8_t status;
	uint16_t conn_handle;
	uint8_t enabled;
};

struct __packed hci_evt_read_remote_version_info_cmplt {
	uint8_t status;
	uint16_t conn_handle;
	uint8_t version_number;
	uint16_t company_id;
	uint16_t sub_version_number;
};

struct __packed hci_evt_num_cmplt {
	uint8_t num_handles;
	uint8_t handles_nums[1];
};

struct __packed hci_evt_encryption_key_refresh_cmplt {
	uint8_t status;
	uint16_t conn_handle;
};

enum {
	HCI_EVT_LE_META_CONNECTION_COMPLETE = 0x01,
	HCI_EVT_LE_META_ADV_REPORT,
	HCI_EVT_LE_META_CONNECTION_UPDATE_COMPLETE,
	HCI_EVT_LE_META_READ_REMOTE_USED_FEATURE_COMPLETE,
	HCI_EVT_LE_META_LONG_TERM_KEY_REQUEST,
	HCI_EVT_LE_META_REMOTE_CONNECTION_PARAMETER_REQUEST,
	HCI_EVT_LE_META_LENGTH_CHANGE,
};

struct __packed hci_evt_le_meta_conn_complete {
	uint8_t status;
	uint16_t conn_handle;
	uint8_t role;
	uint8_t addr_type;
	uint8_t addr[BDADDR_SIZE];
	uint16_t interval;
	uint16_t latency;
	uint16_t timeout;
	uint8_t mca;
};

struct __packed hci_evt_le_meta_adv_report {
	uint8_t num_reports;
	uint8_t reports[1];
};

struct __packed hci_evt_le_meta_conn_update_complete {
	uint8_t status;
	uint16_t conn_handle;
	uint16_t interval;
	uint16_t latency;
	uint16_t timeout;
};

struct __packed hci_evt_le_meta_read_remote_used_features {
	uint8_t status;
	uint16_t conn_handle;
	uint8_t features[8];
};

struct __packed hci_evt_le_meta_long_term_key_request {
	uint16_t conn_handle;
	uint8_t rand[8];
	uint8_t ediv[2];
};

struct __packed hci_evt_le_meta_remote_conn_param_request {
	uint16_t conn_handle;
	uint16_t interval_min;
	uint16_t interval_max;
	uint16_t latency;
	uint16_t timeout;
};

struct __packed hci_evt_le_meta_length_change {
	uint16_t conn_handle;
	uint16_t max_tx_octets;
	uint16_t max_tx_time;
	uint16_t max_rx_octets;
	uint16_t max_rx_time;
};

struct __packed hci_evt_le_meta {
	uint8_t subevent_code;
	union __packed {
		struct hci_evt_le_meta_conn_complete conn_cmplt;
		struct hci_evt_le_meta_adv_report adv_report;
		struct hci_evt_le_meta_conn_update_complete conn_update_cmplt;

		struct hci_evt_le_meta_read_remote_used_features
			remote_used_features;

		struct hci_evt_le_meta_long_term_key_request
			long_term_key_request;

		struct hci_evt_le_meta_remote_conn_param_request
			remote_conn_param_request;
		struct hci_evt_le_meta_length_change length_change;
	} subevent;
};

struct __packed hci_evt_apto_expired {
	uint16_t conn_handle;
};

struct __packed hci_evt {
	uint8_t code;
	uint8_t len;
	union __packed {
		struct hci_evt_disconnect_cmplt disconnect_cmplt;
		struct hci_evt_encryption_change encryption_change;

		struct hci_evt_read_remote_version_info_cmplt
			read_remote_version_info_cmplt;
		struct hci_evt_num_cmplt num_cmplt;

		struct hci_evt_encryption_key_refresh_cmplt
			encryption_key_refresh_cmplt;
		struct hci_evt_le_meta le_meta;
		struct hci_evt_apto_expired apto_expired;
	} params;
};

struct __packed hci_data {
	uint16_t handle:12;
	uint16_t pb:2;
	uint16_t bc:2;
	uint16_t len;
	uint8_t data[1];
};

static struct {
	uint16_t rx_len;
	uint8_t rx[HCI_PACKET_SIZE_MAX];
	uint8_t tx[HCI_PACKET_SIZE_MAX];
} hci_context;

#define HCI_EVT_LEN(evt) ((uint8_t)(1 + sizeof(struct bt_hci_evt_hdr) + \
			evt->len))

#define HCI_DATA_LEN(dat) ((uint8_t)(1 + offsetof(struct hci_data, data) + \
			       dat->len))

#define _HCI_CC_LEN(st) ((uint8_t)(sizeof(struct bt_hci_evt_cmd_complete) + \
			sizeof(st)))

#define HCI_CC_LEN(stn) (_HCI_CC_LEN(struct stn))

#define HCI_EVTP(evt_hdr) (void *)((uint8_t *)evt_hdr + \
			sizeof(struct bt_hci_evt_hdr))

/* direct access to the command status event parameters */
#define HCI_CS(evt_hdr) ((struct bt_hci_evt_cmd_status *)HCI_EVTP(evt_hdr))

/* direct access to the command complete event parameters */
#define HCI_CC(evt_hdr) ((struct bt_hci_evt_cmd_complete *)HCI_EVTP(evt_hdr))

/* direct access to the command complete event return parameters */
#define HCI_CC_RP(evt_hdr) ((void *)(((uint8_t *)HCI_EVTP(evt_hdr)) + \
				     sizeof(struct bt_hci_evt_cmd_complete)))

/* direct access to the command complete status event parameters */
#define HCI_CC_ST(evt_hdr) ((struct bt_hci_evt_cc_status *)(HCI_CC_RP(evt_hdr)))

static void disconnect(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_disconnect *cmd = (void *)cp;
	uint32_t status;

	status = radio_terminate_ind_send(cmd->handle, cmd->reason);

	evt->evt = BT_HCI_EVT_CMD_STATUS;
	evt->len = sizeof(struct bt_hci_evt_cmd_status);

	HCI_CS(evt)->status = (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED;
}

static void read_remote_ver_info(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_read_remote_version_info *cmd = (void *)cp;
	uint32_t status;

	status = radio_version_ind_send(cmd->handle);

	evt->evt = BT_HCI_EVT_CMD_STATUS;
	evt->len = sizeof(struct bt_hci_evt_cmd_status);

	HCI_CS(evt)->status = (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED;
}

static int link_control_cmd_handle(uint8_t ocf, uint8_t *cp,
				   uint8_t *len, struct bt_hci_evt_hdr *evt)
{
	switch (ocf) {
	case BT_OCF(BT_HCI_OP_DISCONNECT):
		disconnect(cp, evt);
		break;
	case BT_OCF(BT_HCI_OP_READ_REMOTE_VERSION_INFO):
		read_remote_ver_info(cp, evt);
		break;
	default:
		return -EINVAL;
	}

	*len = HCI_EVT_LEN(evt);

	return 0;
}

static void set_event_mask(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	/** TODO */

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(bt_hci_evt_cc_status);

	HCI_CC_ST(evt)->status = 0x00;
}

static void reset(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	/** TODO */

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(bt_hci_evt_cc_status);

	HCI_CC_ST(evt)->status = 0x00;
}

static int ctrl_bb_cmd_handle(uint8_t ocf, uint8_t *cp, uint8_t *len,
			      struct bt_hci_evt_hdr *evt)
{
	switch (ocf) {
	case BT_OCF(BT_HCI_OP_SET_EVENT_MASK):
		set_event_mask(cp, evt);
		break;

	case BT_OCF(BT_HCI_OP_RESET):
		reset(cp, evt);
		break;

	default:
		return -EINVAL;
	}

	*len = HCI_EVT_LEN(evt);

	return 0;
}

static void read_local_version_info(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_rp_read_local_version_info *rp = HCI_CC_RP(evt);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = _HCI_CC_LEN(*rp);

	rp->status = 0x00;
	rp->hci_version = 0;
	rp->hci_revision = 0;
	rp->lmp_version = RADIO_BLE_VERSION_NUMBER;
	rp->manufacturer = RADIO_BLE_COMPANY_ID;
	rp->lmp_subversion = RADIO_BLE_SUB_VERSION_NUMBER;
}

static void read_supported_commands(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_rp_read_supported_commands *rp = HCI_CC_RP(evt);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = _HCI_CC_LEN(*rp);

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

static void read_local_features(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_rp_read_local_features *rp = HCI_CC_RP(evt);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = _HCI_CC_LEN(*rp);

	rp->status = 0x00;
	memset(&rp->features[0], 0x00, sizeof(rp->features));
	/* BR/EDR not supported and LE supported */
	rp->features[4] = (1 << 5) | (1 << 6);
}

static void read_bd_addr(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_rp_read_bd_addr *rp = HCI_CC_RP(evt);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = _HCI_CC_LEN(*rp);

	rp->status = 0x00;
	ll_address_get(0, &rp->bdaddr.val[0]);
}

static int info_cmd_handle(uint8_t ocf, uint8_t *cp, uint8_t *len,
			   struct bt_hci_evt_hdr *evt)
{
	switch (ocf) {
	case BT_OCF(BT_HCI_OP_READ_LOCAL_VERSION_INFO):
		read_local_version_info(cp, evt);
		break;

	case BT_OCF(BT_HCI_OP_READ_SUPPORTED_COMMANDS):
		read_supported_commands(cp, evt);
		break;

	case BT_OCF(BT_HCI_OP_READ_LOCAL_FEATURES):
		read_local_features(cp, evt);
		break;

	case BT_OCF(BT_HCI_OP_READ_BD_ADDR):
		read_bd_addr(cp, evt);
		break;

	default:
		return -EINVAL;
	}

	*len = HCI_EVT_LEN(evt);

	return 0;
}

static void le_set_event_mask(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	/** TODO */

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(bt_hci_evt_cc_status);

	HCI_CC_ST(evt)->status = 0x00;
}

static void le_read_buffer_size(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_rp_le_read_buffer_size *rp = HCI_CC_RP(evt);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = _HCI_CC_LEN(*rp);

	rp->status = 0x00;

	rp->le_max_len = RADIO_LL_LENGTH_OCTETS_RX_MAX;
	rp->le_max_num = RADIO_PACKET_COUNT_TX_MAX;
}

static void le_read_local_features(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_rp_le_read_local_features *rp = HCI_CC_RP(evt);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = _HCI_CC_LEN(*rp);

	rp->status = 0x00;

	memset(&rp->features[0], 0x00, sizeof(rp->features));
	rp->features[0] = RADIO_BLE_FEATURES;
}

static void le_set_random_address(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_le_set_random_address *cmd = (void *)cp;

	ll_address_set(1, &cmd->bdaddr.val[0]);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(bt_hci_evt_cc_status);

	HCI_CC_ST(evt)->status = 0x00;
}

static void le_set_adv_param(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_le_set_adv_param *cmd = (void *)cp;
	uint8_t const c_adv_type[] = {
		PDU_ADV_TYPE_ADV_IND, PDU_ADV_TYPE_DIRECT_IND,
		PDU_ADV_TYPE_SCAN_IND, PDU_ADV_TYPE_NONCONN_IND };

	ll_adv_params_set(cmd->min_interval, c_adv_type[cmd->type],
			  cmd->own_addr_type, cmd->direct_addr.type,
			  &cmd->direct_addr.a.val[0], cmd->channel_map,
			  cmd->filter_policy);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(bt_hci_evt_cc_status);

	HCI_CC_ST(evt)->status = 0x00;
}

static void le_read_adv_ch_tx_power(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_rp_le_read_ch_tx_power *rp = HCI_CC_RP(evt);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = _HCI_CC_LEN(*rp);

	rp->status = 0x00;

	rp->tx_power_level = 0;
}

static void le_set_adv_data(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_le_set_adv_data *cmd = (void *)cp;

	ll_adv_data_set(cmd->len, &cmd->data[0]);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(bt_hci_evt_cc_status);

	HCI_CC_ST(evt)->status = 0x00;
}

static void le_set_scan_rsp_data(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_le_set_scan_rsp_data *cmd = (void *)cp;

	ll_scan_data_set(cmd->len, &cmd->data[0]);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(bt_hci_evt_cc_status);

	HCI_CC_ST(evt)->status = 0x00;
}

static void le_set_adv_enable(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_le_set_adv_enable *cmd = (void *)cp;
	uint32_t status;

	status = ll_adv_enable(cmd->enable);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(bt_hci_evt_cc_status);

	HCI_CC_ST(evt)->status = (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED;
}

static void le_set_scan_params(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_le_set_scan_params *cmd = (void *)cp;

	ll_scan_params_set(cmd->scan_type, cmd->interval, cmd->window,
			   cmd->addr_type, cmd->filter_policy);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(bt_hci_evt_cc_status);

	HCI_CC_ST(evt)->status = 0x00;
}

static void le_set_scan_enable(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_le_set_scan_enable *cmd = (void *)cp;
	uint32_t status;

	status = ll_scan_enable(cmd->enable);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(bt_hci_evt_cc_status);

	HCI_CC_ST(evt)->status = (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED;
}

static void le_create_connection(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_le_create_conn *cmd = (void *)cp;
	uint32_t status;

	status = ll_create_connection(cmd->scan_interval,
				      cmd->scan_window,
				      cmd->filter_policy,
				      cmd->peer_addr.type,
				      &cmd->peer_addr.a.val[0],
				      cmd->own_addr_type,
				      cmd->conn_interval_max,
				      cmd->conn_latency,
				      cmd->supervision_timeout);

	evt->evt = BT_HCI_EVT_CMD_STATUS;
	evt->len = sizeof(struct bt_hci_evt_cmd_status);

	HCI_CS(evt)->status = (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED;
}

static void le_create_conn_cancel(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	uint32_t status;

	status = radio_connect_disable();

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(bt_hci_evt_cc_status);

	HCI_CC_ST(evt)->status = (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED;
}

static void le_read_wl_size(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_rp_le_read_wl_size *rp = HCI_CC_RP(evt);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = _HCI_CC_LEN(*rp);

	rp->status = 0x00;

	rp->wl_size = 8;
}

static void le_clear_wl(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	radio_filter_clear();

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(bt_hci_evt_cc_status);

	HCI_CC_ST(evt)->status = 0x00;
}

static void le_add_dev_to_wl(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_le_add_dev_to_wl *cmd = (void *)cp;
	uint32_t status;

	status = radio_filter_add(cmd->addr.type, &cmd->addr.a.val[0]);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(bt_hci_evt_cc_status);

	HCI_CC_ST(evt)->status = (!status) ? 0x00 :
					     BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
}

static void le_conn_update(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	struct hci_cp_le_conn_update *cmd = (void *)cp;
	uint32_t status;

	/** @todo if peer supports LE Conn Param Req,
	* use Req cmd (1) instead of Initiate cmd (0).
	*/
	status = radio_conn_update(cmd->handle, 0, 0, cmd->conn_interval_max,
				   cmd->conn_latency, cmd->supervision_timeout);

	evt->evt = BT_HCI_EVT_CMD_STATUS;
	evt->len = sizeof(struct bt_hci_evt_cmd_status);

	HCI_CS(evt)->status = (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED;
}

static void le_set_host_ch_classif(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_le_set_host_ch_classif *cmd = (void *)cp;
	uint32_t status;

	status = radio_chm_update(&cmd->ch_map[0]);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = HCI_CC_LEN(bt_hci_evt_cc_status);

	HCI_CC_ST(evt)->status = (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED;
}

static void le_read_remote_features(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_le_read_remote_features *cmd = (void *)cp;
	uint32_t status;

	status = radio_feature_req_send(cmd->handle);

	evt->evt = BT_HCI_EVT_CMD_STATUS;
	evt->len = sizeof(struct bt_hci_evt_cmd_status);

	HCI_CS(evt)->status = (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED;
}

static void le_encrypt(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_le_encrypt *cmd = (void *)cp;
	struct bt_hci_rp_le_encrypt *rp = HCI_CC_RP(evt);

	ecb_encrypt(&cmd->key[0], &cmd->plaintext[0], &rp->enc_data[0], 0);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = _HCI_CC_LEN(*rp);

	rp->status = 0x00;
}

static void le_rand(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_rp_le_rand *rp = HCI_CC_RP(evt);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = _HCI_CC_LEN(*rp);

	rp->status = 0x00;

	/** TODO fill rand */
}

static void le_start_encryption(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_le_start_encryption *cmd = (void *)cp;
	uint32_t status;

	status = radio_enc_req_send(cmd->handle,
			       (uint8_t *)&cmd->rand,
			       (uint8_t *)&cmd->ediv,
			       &cmd->ltk[0]);

	evt->evt = BT_HCI_EVT_CMD_STATUS;
	evt->len = sizeof(struct bt_hci_evt_cmd_status);

	HCI_CS(evt)->status = (!status) ? 0x00 : BT_HCI_ERR_CMD_DISALLOWED;
}

static void le_ltk_req_reply(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_le_ltk_req_reply *cmd = (void *)cp;
	struct bt_hci_rp_le_ltk_req_reply *rp = HCI_CC_RP(evt);
	uint32_t status;

	status = radio_start_enc_req_send(cmd->handle, 0x00, &cmd->ltk[0]);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = _HCI_CC_LEN(*rp);
	rp->status = (!status) ?  0x00 : BT_HCI_ERR_CMD_DISALLOWED;
	rp->handle = cmd->handle;
}

static void le_ltk_req_neg_reply(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_le_ltk_req_neg_reply *cmd = (void *)cp;
	struct bt_hci_rp_le_ltk_req_neg_reply *rp = HCI_CC_RP(evt);
	uint32_t status;

	status = radio_start_enc_req_send(cmd->handle,
					  BT_HCI_ERR_PIN_OR_KEY_MISSING, NULL);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = _HCI_CC_LEN(*rp);
	rp->status = (!status) ?  0x00 : BT_HCI_ERR_CMD_DISALLOWED;
	rp->handle = cmd->handle;
}

static void le_read_supp_states(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_rp_le_read_supp_states *rp = HCI_CC_RP(evt);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = _HCI_CC_LEN(*rp);

	rp->status = 0x00;

	sys_put_le64(0x000003ffffffffff, rp->le_states);
}

static void le_conn_param_req_reply(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_le_conn_param_req_reply *cmd = (void *)cp;
	struct bt_hci_rp_le_conn_param_req_reply *rp = HCI_CC_RP(evt);
	uint32_t status;

	status = radio_conn_update(cmd->handle, 2, 0,
				   cmd->interval_max,
				   cmd->latency,
				   cmd->timeout);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = _HCI_CC_LEN(*rp);
	rp->status = (!status) ?  0x00 : BT_HCI_ERR_CMD_DISALLOWED;
	rp->handle = cmd->handle;
}

static void le_conn_param_req_neg_reply(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_le_conn_param_req_neg_reply *cmd = (void *)cp;
	struct bt_hci_rp_le_conn_param_req_neg_reply *rp = HCI_CC_RP(evt);
	uint32_t status;

	status = radio_conn_update(cmd->handle, 2, cmd->reason, 0, 0, 0);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = _HCI_CC_LEN(*rp);
	rp->status = (!status) ?  0x00 : BT_HCI_ERR_CMD_DISALLOWED;
	rp->handle = cmd->handle;
}

static void le_set_data_len(uint8_t *cp, struct bt_hci_evt_hdr *evt)
{
	struct bt_hci_cp_le_set_data_len *cmd = (void *)cp;
	struct bt_hci_rp_le_set_data_len *rp = HCI_CC_RP(evt);
	uint32_t status;

	/** @todo add reject_ext_ind support in ctrl.c */
	status = radio_length_req_send(cmd->handle, cmd->tx_octets);

	evt->evt = BT_HCI_EVT_CMD_COMPLETE;
	evt->len = _HCI_CC_LEN(*rp);
	rp->status = (!status) ?  0x00 : BT_HCI_ERR_CMD_DISALLOWED;
	rp->handle = cmd->handle;
}

static int controller_cmd_handle(uint8_t ocf, uint8_t *cp, uint8_t *len,
				 struct bt_hci_evt_hdr *evt)
{

	switch (ocf) {
	case BT_OCF(BT_HCI_OP_LE_SET_EVENT_MASK):
		le_set_event_mask(cp, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_READ_BUFFER_SIZE):
		le_read_buffer_size(cp, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_READ_LOCAL_FEATURES):
		le_read_local_features(cp, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_RANDOM_ADDRESS):
		le_set_random_address(cp, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_ADV_PARAM):
		le_set_adv_param(cp, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_READ_ADV_CH_TX_POWER):
		le_read_adv_ch_tx_power(cp, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_ADV_DATA):
		le_set_adv_data(cp, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_SCAN_RSP_DATA):
		le_set_scan_rsp_data(cp, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_ADV_ENABLE):
		le_set_adv_enable(cp, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_SCAN_PARAMS):
		le_set_scan_params(cp, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_SCAN_ENABLE):
		le_set_scan_enable(cp, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_CREATE_CONN):
		le_create_connection(cp, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_CREATE_CONN_CANCEL):
		le_create_conn_cancel(cp, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_READ_WL_SIZE):
		le_read_wl_size(cp, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_CLEAR_WL):
		le_clear_wl(cp, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_ADD_DEV_TO_WL):
		le_add_dev_to_wl(cp, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_CONN_UPDATE):
		le_conn_update(cp, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_HOST_CH_CLASSIF):
		le_set_host_ch_classif(cp, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_READ_REMOTE_FEATURES):
		le_read_remote_features(cp, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_ENCRYPT):
		le_encrypt(cp, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_RAND):
		le_rand(cp, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_START_ENCRYPTION):
		le_start_encryption(cp, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_LTK_REQ_REPLY):
		le_ltk_req_reply(cp, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_LTK_REQ_NEG_REPLY):
		le_ltk_req_neg_reply(cp, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_READ_SUPP_STATES):
		le_read_supp_states(cp, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_CONN_PARAM_REQ_REPLY):
		le_conn_param_req_reply(cp, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_CONN_PARAM_REQ_NEG_REPLY):
		le_conn_param_req_neg_reply(cp, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_DATA_LEN):
		le_set_data_len(cp, evt);
		break;

	default:
		return -EINVAL;
	}

	*len = HCI_EVT_LEN(evt);

	return 0;
}

static void hci_cmd_handle(struct bt_hci_cmd_hdr *cmd, uint8_t *len,
			   uint8_t **out)
{
	struct bt_hci_evt_hdr *evt;
	struct bt_hci_evt_cmd_complete *cc;
	struct bt_hci_evt_cmd_status *cs;
	int err;
	uint16_t opcode;
	uint8_t ocf;
	uint8_t *cp;

	*out = &hci_context.tx[0];
	hci_context.tx[0] = HCI_EVT;
	evt = (void *)&hci_context.tx[1];
	cc = HCI_CC(evt);
	cs = HCI_CS(evt);

	opcode = sys_le16_to_cpu(cmd->opcode);
	ocf = BT_OCF(opcode);
	cp = ((uint8_t *)cmd) + sizeof(struct bt_hci_cmd_hdr);

	switch (BT_OGF(opcode)) {
	case BT_OGF_LINK_CTRL:
		err = link_control_cmd_handle(ocf, cp, len, evt);
		break;
	case BT_OGF_BASEBAND:
		err = ctrl_bb_cmd_handle(ocf, cp, len, evt);
		break;
	case BT_OGF_INFO:
		err = info_cmd_handle(ocf, cp, len, evt);
		break;
	case BT_OGF_LE:
		err = controller_cmd_handle(ocf, cp, len, evt);
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
		evt->len = HCI_CC_LEN(bt_hci_evt_cc_status);
		HCI_CC_ST(evt)->status = BT_HCI_ERR_UNKNOWN_CMD;
		*len = HCI_EVT_LEN(evt);
	}

	switch (evt->evt) {
	case BT_HCI_EVT_CMD_COMPLETE:
		cc->ncmd = 1;
		cc->opcode = opcode;
		break;

	case BT_HCI_EVT_CMD_STATUS:
		cs->ncmd = 1;
		cs->opcode = opcode;
		break;
	default:
		break;
	}
}

static void hci_data_handle(void)
{
	struct hci_data *data;

	if (!(hci_context.rx_len > offsetof(struct hci_data, data))) {
		return;
	}

	data = (struct hci_data *)&hci_context.rx[1];
	if (!(hci_context.rx_len >=
	    (1 + offsetof(struct hci_data, data) +
	     data->len))) {
		return;
	}

	struct radio_pdu_node_tx *radio_pdu_node_tx;

	radio_pdu_node_tx = radio_tx_mem_acquire();
	if (radio_pdu_node_tx) {
		struct pdu_data *pdu_data;

		pdu_data = (struct pdu_data *)radio_pdu_node_tx->pdu_data;
		if (data->pb == 0x00 || data->pb == 0x02) {
			pdu_data->ll_id = PDU_DATA_LLID_DATA_START;
		} else {
			pdu_data->ll_id = PDU_DATA_LLID_DATA_CONTINUE;
		}
		pdu_data->len = data->len;
		memcpy(&pdu_data->payload.lldata[0],
			 &data->data[0],
			 data->len);
		if (radio_tx_mem_enqueue(data->handle, radio_pdu_node_tx)) {
			radio_tx_mem_release
			    (radio_pdu_node_tx);
		}
	}

	hci_context.rx_len = 0;
}

void hci_handle(uint8_t x, uint8_t *len, uint8_t **out)
{
	struct bt_hci_cmd_hdr *cmd;

	hci_context.rx[hci_context.rx_len++] = x;

	*len = 0;
	*out = 0;
	if (!(hci_context.rx_len > 0)) {
		return;
	}

	switch (hci_context.rx[0]) {
	case HCI_CMD:
		/* include 1 + for H4 packet type */
		if (hci_context.rx_len < (1 + sizeof(struct bt_hci_cmd_hdr))) {
			break;
		}

		cmd = (struct bt_hci_cmd_hdr *)&hci_context.rx[1];
		if (hci_context.rx_len >=
		    /* include 1 + for H4 packet type */
		    (1 + sizeof(struct bt_hci_cmd_hdr) + cmd->param_len)) {
			/* packet fully received, process it */
			hci_cmd_handle(cmd, len, out);
			hci_context.rx_len = 0;
		}
		break;

	case HCI_DATA:
		hci_data_handle();
		break;

	default:
		hci_context.rx_len = 0;
		break;
	}
}


static void encode_control(uint8_t *buf, uint8_t *len, uint8_t **out)
{
	struct hci_evt *evt;
	struct pdu_adv *adv;
	struct pdu_data *pdu_data;
	uint8_t *report;
	uint8_t data_len;
	const uint8_t c_adv_type[] = { 0x00, 0x01, 0x03, 0xff, 0x04,
				     0xff, 0x02 };
	uint16_t instance;
	struct radio_le_conn_update_cmplt *le_conn_update_cmplt;
	struct radio_le_conn_cmplt *radio_le_conn_cmplt;
	struct radio_pdu_node_rx *radio_pdu_node_rx;

	radio_pdu_node_rx = (struct radio_pdu_node_rx *)buf;
	instance = radio_pdu_node_rx->hdr.handle;

	switch (radio_pdu_node_rx->hdr.type) {
	case NODE_RX_TYPE_REPORT:
		adv = (struct pdu_adv *)radio_pdu_node_rx->pdu_data;

		hci_context.tx[0] = HCI_EVT;

		evt = (struct hci_evt *)&hci_context.tx[1];
		evt->code = HCI_EVT_CODE_LE_META;
		evt->len = offsetof(struct hci_evt_le_meta,
				    subevent.adv_report.reports);

		evt->params.le_meta.subevent_code = HCI_EVT_LE_META_ADV_REPORT;
		evt->params.le_meta.subevent.adv_report.num_reports = 1;
		report = &evt->params.le_meta.subevent.adv_report.reports[0];

		*report++ = c_adv_type[adv->type];
		*report++ = adv->tx_addr;
		memcpy(&report[0], &adv->payload.adv_ind.addr[0],
			 BDADDR_SIZE);
		report += BDADDR_SIZE;
		if (adv->type != PDU_ADV_TYPE_DIRECT_IND) {
			data_len = (adv->len - BDADDR_SIZE);
		} else {
			data_len = 0;
		}
		*report++ = data_len;
		memcpy(&report[0], &adv->payload.adv_ind.data[0], data_len);
		report += data_len;
		/* RSSI */
		*report++ = buf[offsetof(struct radio_pdu_node_rx, pdu_data) +
			offsetof(struct pdu_adv, payload) + adv->len];

		evt->len += (report -
			     &evt->params.le_meta.subevent.adv_report.
			     reports[0]);

		*len = HCI_EVT_LEN(evt);
		*out = &hci_context.tx[0];
		break;

	case NODE_RX_TYPE_CONNECTION:
		pdu_data = (struct pdu_data *)radio_pdu_node_rx->pdu_data;
		radio_le_conn_cmplt = (struct radio_le_conn_cmplt *)
				       (pdu_data->payload.lldata);

		hci_context.tx[0] = HCI_EVT;

		evt = (struct hci_evt *)&hci_context.tx[1];
		evt->code = HCI_EVT_CODE_LE_META;
		evt->len = (offsetof(struct hci_evt_le_meta, subevent) +
			 sizeof(struct hci_evt_le_meta_conn_complete));
		evt->params.le_meta.subevent_code =
		    HCI_EVT_LE_META_CONNECTION_COMPLETE;
		evt->params.le_meta.subevent.conn_cmplt.status =
		    radio_le_conn_cmplt->status;
		evt->params.le_meta.subevent.conn_cmplt.conn_handle = instance;
		evt->params.le_meta.subevent.conn_cmplt.role =
		    radio_le_conn_cmplt->role;
		evt->params.le_meta.subevent.conn_cmplt.addr_type =
		    radio_le_conn_cmplt->peer_addr_type;
		memcpy(&evt->params.le_meta.subevent.conn_cmplt.addr[0],
		       &radio_le_conn_cmplt->peer_addr[0],
		       BDADDR_SIZE);
		evt->params.le_meta.subevent.conn_cmplt.interval =
		    radio_le_conn_cmplt->interval;
		evt->params.le_meta.subevent.conn_cmplt.latency =
		    radio_le_conn_cmplt->latency;
		evt->params.le_meta.subevent.conn_cmplt.timeout =
		    radio_le_conn_cmplt->timeout;
		evt->params.le_meta.subevent.conn_cmplt.mca =
		    radio_le_conn_cmplt->mca;

		*len = HCI_EVT_LEN(evt);
		*out = &hci_context.tx[0];
		break;

	case NODE_RX_TYPE_TERMINATE:
		hci_context.tx[0] = HCI_EVT;

		evt = (struct hci_evt *)&hci_context.tx[1];
		evt->code = HCI_EVT_CODE_DISCONNECTION_COMPLETE;
		evt->len = sizeof(struct hci_evt_disconnect_cmplt);
		evt->params.disconnect_cmplt.status =
		    HCI_EVT_ERROR_CODE_SUCCESS;
		evt->params.disconnect_cmplt.conn_handle = instance;
		evt->params.disconnect_cmplt.reason =
		    *((uint8_t *)radio_pdu_node_rx->pdu_data);

		*len = HCI_EVT_LEN(evt);
		*out = &hci_context.tx[0];
		break;

	case NODE_RX_TYPE_CONN_UPDATE:
		pdu_data = (struct pdu_data *)radio_pdu_node_rx->pdu_data;
		le_conn_update_cmplt = (struct radio_le_conn_update_cmplt *)
			(pdu_data->payload.lldata);
		hci_context.tx[0] = HCI_EVT;

		evt = (struct hci_evt *)&hci_context.tx[1];
		evt->code = HCI_EVT_CODE_LE_META;
		evt->len = (offsetof(struct hci_evt_le_meta, subevent) +
			 sizeof(struct hci_evt_le_meta_conn_update_complete));
		evt->params.le_meta.subevent_code =
			HCI_EVT_LE_META_CONNECTION_UPDATE_COMPLETE;
		evt->params.le_meta.subevent.conn_update_cmplt.status =
			le_conn_update_cmplt->status;
		evt->params.le_meta.subevent.conn_update_cmplt.conn_handle =
			instance;
		evt->params.le_meta.subevent.conn_update_cmplt.interval =
			le_conn_update_cmplt->interval;
		evt->params.le_meta.subevent.conn_update_cmplt.latency =
			le_conn_update_cmplt->latency;
		evt->params.le_meta.subevent.conn_update_cmplt.timeout =
			le_conn_update_cmplt->timeout;

		*len = HCI_EVT_LEN(evt);
		*out = &hci_context.tx[0];
		break;

	case NODE_RX_TYPE_ENC_REFRESH:
		hci_context.tx[0] = HCI_EVT;

		evt = (struct hci_evt *)&hci_context.tx[1];
		evt->code = HCI_EVT_CODE_ENCRYPTION_KEY_REFRESH_COMPLETE;
		evt->len = sizeof(struct hci_evt_encryption_key_refresh_cmplt);
		evt->params.encryption_key_refresh_cmplt.status =
			HCI_EVT_ERROR_CODE_SUCCESS;
		evt->params.encryption_key_refresh_cmplt.conn_handle = instance;

		*len = HCI_EVT_LEN(evt);
		*out = &hci_context.tx[0];
		break;

	case NODE_RX_TYPE_APTO:
		hci_context.tx[0] = HCI_EVT;

		evt = (struct hci_evt *)&hci_context.tx[1];
		evt->code = HCI_EVT_CODE_APTO_EXPIRED;
		evt->len = sizeof(struct hci_evt_apto_expired);
		evt->params.apto_expired.conn_handle = instance;

		*len = HCI_EVT_LEN(evt);
		*out = &hci_context.tx[0];
		break;

	case NODE_RX_TYPE_RSSI:
  /** @todo */
		break;

	case NODE_RX_TYPE_PROFILE:
  /** @todo */
		break;

	default:
		BT_ASSERT(0);
		break;
	}
}

static void encode_data_ctrl(struct radio_pdu_node_rx *radio_pdu_node_rx,
		uint8_t *len, uint8_t **out)
{
	uint16_t instance;
	struct hci_evt *evt;
	struct hci_evt_le_meta_read_remote_used_features *rem_used_feats;
	struct hci_evt_le_meta_long_term_key_request *long_term_key_req;
	struct hci_evt_le_meta_remote_conn_param_request *rem_cp_req;
	struct hci_evt_le_meta_length_change *len_change;
	struct pdu_data *pdu_data;

	pdu_data = (struct pdu_data *)radio_pdu_node_rx->pdu_data;
	instance = radio_pdu_node_rx->hdr.handle;

	evt = (struct hci_evt *)&hci_context.tx[1];

	switch (pdu_data->payload.llctrl.opcode) {
	case PDU_DATA_LLCTRL_TYPE_ENC_REQ:
		hci_context.tx[0] = HCI_EVT;
		long_term_key_req =
			&evt->params.le_meta.subevent.long_term_key_request;

		evt->code = HCI_EVT_CODE_LE_META;
		evt->len = (offsetof(struct hci_evt_le_meta, subevent) +
			sizeof(struct hci_evt_le_meta_long_term_key_request));
		evt->params.le_meta.subevent_code =
		    HCI_EVT_LE_META_LONG_TERM_KEY_REQUEST;
		long_term_key_req->conn_handle = instance;
		memcpy(&long_term_key_req->rand[0],
			 &pdu_data->payload.llctrl.ctrldata.enc_req.rand[0],
			 sizeof(long_term_key_req->rand));
		long_term_key_req->ediv[0] =
		    pdu_data->payload.llctrl.ctrldata.enc_req.ediv[0];
		long_term_key_req->ediv[1] =
		    pdu_data->payload.llctrl.ctrldata.enc_req.ediv[1];

		*len = HCI_EVT_LEN(evt);
		*out = &hci_context.tx[0];
		break;

	case PDU_DATA_LLCTRL_TYPE_START_ENC_RSP:
		hci_context.tx[0] = HCI_EVT;

		evt->code = HCI_EVT_CODE_ENCRYPTION_CHANGE;
		evt->len = sizeof(struct hci_evt_encryption_change);
		evt->params.encryption_change.status =
		    HCI_EVT_ERROR_CODE_SUCCESS;
		evt->params.encryption_change.conn_handle = instance;
		evt->params.encryption_change.enabled = 1;

		*len = HCI_EVT_LEN(evt);
		*out = &hci_context.tx[0];
		break;

	case PDU_DATA_LLCTRL_TYPE_FEATURE_RSP:
		hci_context.tx[0] = HCI_EVT;
		rem_used_feats =
			&evt->params.le_meta.subevent.remote_used_features;

		evt->code = HCI_EVT_CODE_LE_META;
		evt->len = (offsetof(struct hci_evt_le_meta, subevent) +
				sizeof(
		struct hci_evt_le_meta_read_remote_used_features));
		evt->params.le_meta.subevent_code =
		    HCI_EVT_LE_META_READ_REMOTE_USED_FEATURE_COMPLETE;
		rem_used_feats->status = HCI_EVT_ERROR_CODE_SUCCESS;
		rem_used_feats->conn_handle = instance;
		memcpy(&rem_used_feats->features[0],
		       &pdu_data->payload.llctrl.ctrldata.feature_rsp.
		       features[0],
		       sizeof(rem_used_feats->features));

		*len = HCI_EVT_LEN(evt);
		*out = &hci_context.tx[0];
		break;

	case PDU_DATA_LLCTRL_TYPE_VERSION_IND:
		hci_context.tx[0] = HCI_EVT;

		evt->code = HCI_EVT_CODE_READ_REMOTE_VERSION_INFO_COMPLETE;
		evt->len = sizeof(struct
				  hci_evt_read_remote_version_info_cmplt);
		evt->params.read_remote_version_info_cmplt.status =
			HCI_EVT_ERROR_CODE_SUCCESS;
		evt->params.read_remote_version_info_cmplt.conn_handle =
			instance;
		evt->params.read_remote_version_info_cmplt.version_number =
			pdu_data->payload.llctrl.ctrldata.
			version_ind.version_number;
		evt->params.read_remote_version_info_cmplt.company_id =
			pdu_data->payload.llctrl.ctrldata.
			version_ind.company_id;
		evt->params.read_remote_version_info_cmplt.sub_version_number =
			pdu_data->payload.llctrl.ctrldata.
			version_ind.sub_version_number;

		*len = HCI_EVT_LEN(evt);
		*out = &hci_context.tx[0];
		break;

	case PDU_DATA_LLCTRL_TYPE_REJECT_IND:
		hci_context.tx[0] = HCI_EVT;

		evt->code = HCI_EVT_CODE_ENCRYPTION_CHANGE;
		evt->len = sizeof(struct hci_evt_encryption_change);
		evt->params.encryption_change.status =
		    pdu_data->payload.llctrl.ctrldata.reject_ind.error_code;
		evt->params.encryption_change.conn_handle = instance;
		evt->params.encryption_change.enabled = 0;

		*len = HCI_EVT_LEN(evt);
		*out = &hci_context.tx[0];
		break;

	case PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ:
		hci_context.tx[0] = HCI_EVT;
		rem_cp_req =
			&evt->params.le_meta.subevent.remote_conn_param_request;

		evt->code = HCI_EVT_CODE_LE_META;
		evt->len = (offsetof(struct hci_evt_le_meta,
			    subevent) + sizeof(struct
			    hci_evt_le_meta_remote_conn_param_request));
		evt->params.le_meta.subevent_code =
			HCI_EVT_LE_META_REMOTE_CONNECTION_PARAMETER_REQUEST;
		rem_cp_req->conn_handle = instance;
		rem_cp_req->interval_min =
			pdu_data->payload.llctrl.ctrldata.conn_param_req.
			interval_min;
		rem_cp_req->interval_max =
			pdu_data->payload.llctrl.ctrldata.conn_param_req.
			interval_max;
		rem_cp_req->latency =
			pdu_data->payload.llctrl.ctrldata.conn_param_req.
			latency;
		rem_cp_req->timeout =
			pdu_data->payload.llctrl.ctrldata.conn_param_req.
			timeout;

		*len = HCI_EVT_LEN(evt);
		*out = &hci_context.tx[0];
		break;

	case PDU_DATA_LLCTRL_TYPE_LENGTH_REQ:
	case PDU_DATA_LLCTRL_TYPE_LENGTH_RSP:
		hci_context.tx[0] = HCI_EVT;
		len_change = &evt->params.le_meta.subevent.length_change;

		evt->code = HCI_EVT_CODE_LE_META;
		evt->len = (offsetof(struct hci_evt_le_meta, subevent) +
			    sizeof(struct hci_evt_le_meta_length_change));
		evt->params.le_meta.subevent_code =
		    HCI_EVT_LE_META_LENGTH_CHANGE;
		len_change->conn_handle = instance;
		len_change->max_tx_octets =
		    pdu_data->payload.llctrl.ctrldata.length_rsp.max_tx_octets;
		len_change->max_tx_time = pdu_data->payload.llctrl.ctrldata.
		    length_rsp.max_tx_time;
		len_change->max_rx_octets = pdu_data->payload.llctrl.ctrldata.
		    length_rsp.max_rx_octets;
		len_change->max_rx_time = pdu_data->payload.llctrl.ctrldata.
		    length_rsp.max_rx_time;

		*len = HCI_EVT_LEN(evt);
		*out = &hci_context.tx[0];

#if (TEST_DATA_LENGTH && TEST_TX)
		{
			extern uint16_t g_data_length;

			g_data_length =
			    pdu_data->payload.llctrl.ctrldata.
			    length_rsp.max_tx_octets;
		}
#endif
		break;

	default:
		BT_ASSERT(0);
		break;
	}

}

static void encode_data(uint8_t *buf, uint8_t *len, uint8_t **out)
{

	uint16_t instance;
	struct hci_data *data;
	struct radio_pdu_node_rx *radio_pdu_node_rx;
	struct pdu_data *pdu_data;

	radio_pdu_node_rx = (struct radio_pdu_node_rx *)buf;
	pdu_data = (struct pdu_data *)radio_pdu_node_rx->pdu_data;
	instance = radio_pdu_node_rx->hdr.handle;

	switch (pdu_data->ll_id) {
	case PDU_DATA_LLID_DATA_CONTINUE:
	case PDU_DATA_LLID_DATA_START:
#if !TEST_DROP_RX
		hci_context.tx[0] = HCI_DATA;

		data = (struct hci_data *)&hci_context.tx[1];
		data->handle = instance;
		if (pdu_data->ll_id == PDU_DATA_LLID_DATA_START) {
			data->pb = 0x02;
		} else {
			data->pb = 0x01;
		}
		data->bc = 0;
		data->len = pdu_data->len;
		memcpy(&data->data[0], &pdu_data->payload.lldata[0],
			 pdu_data->len);

		*len = HCI_DATA_LEN(data);
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

	case PDU_DATA_LLID_CTRL:
		encode_data_ctrl(radio_pdu_node_rx, len, out);
		break;

	default:
		BT_ASSERT(0);
		break;
	}

}

void hci_encode(uint8_t *buf, uint8_t *len, uint8_t **out)
{
	struct radio_pdu_node_rx *radio_pdu_node_rx;

	radio_pdu_node_rx = (struct radio_pdu_node_rx *)buf;
	*len = 0;
	*out = 0;

	if (radio_pdu_node_rx->hdr.type != NODE_RX_TYPE_DC_PDU) {
		encode_control(buf, len, out);
	} else {
		encode_data(buf, len, out);
	}
}

void hci_encode_num_cmplt(uint16_t instance, uint8_t num, uint8_t *len,
			  uint8_t **out)
{
	struct hci_evt *evt;
	uint8_t *handles_nums;
	uint8_t num_handles;

	num_handles = 1;

	hci_context.tx[0] = HCI_EVT;

	evt = (struct hci_evt *)&hci_context.tx[1];
	evt->code = HCI_EVT_CODE_NUM_COMPLETE;
	evt->len = (offsetof(struct hci_evt_num_cmplt, handles_nums) +
		    (sizeof(uint16_t) * 2 * num_handles));

	evt->params.num_cmplt.num_handles = num_handles;
	handles_nums = &evt->params.num_cmplt.handles_nums[0];
	handles_nums[0] = instance & 0xFF;
	handles_nums[1] = (instance >> 8) & 0xFF;
	handles_nums[2] = num & 0xFF;
	handles_nums[3] = (num >> 8) & 0xFF;

	*len = HCI_EVT_LEN(evt);
	*out = &hci_context.tx[0];
}
