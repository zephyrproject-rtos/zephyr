/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/tbs.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys_clock.h>

#include "babblekit/testcase.h"

#include "btp/btp.h"

LOG_MODULE_REGISTER(bsim_btp, CONFIG_BSIM_BTTESTER_LOG_LEVEL);

K_FIFO_DEFINE(btp_rsp_fifo);
NET_BUF_POOL_FIXED_DEFINE(btp_rsp_pool, 1, BTP_MTU, 0, NULL);
K_FIFO_DEFINE(btp_evt_fifo);
NET_BUF_POOL_FIXED_DEFINE(btp_evt_pool, 100, BTP_MTU, 0, NULL);

static const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

static bool is_valid_core_packet_len(const struct btp_hdr *hdr, struct net_buf_simple *buf_simple)
{
	switch (hdr->opcode) {
	/* responses */
	case BTP_CORE_READ_SUPPORTED_COMMANDS:
		return buf_simple->len > 0U; /* variable length */
	case BTP_CORE_READ_SUPPORTED_SERVICES:
		return buf_simple->len > 0U; /* variable length */
	case BTP_CORE_REGISTER_SERVICE:
		return buf_simple->len == 0U;
	case BTP_CORE_UNREGISTER_SERVICE:
		return buf_simple->len == 0U;

	/* events */
	case BTP_CORE_EV_IUT_READY:
		return buf_simple->len == 0U;
	default:
		LOG_ERR("Unhandled opcode 0x%02X", hdr->opcode);
		return false;
	}
}

static bool is_valid_gap_packet_len(const struct btp_hdr *hdr, struct net_buf_simple *buf_simple)
{
	switch (hdr->opcode) {
	/* responses */
	case BTP_GAP_READ_SUPPORTED_COMMANDS:
		return buf_simple->len > 0U; /* variable length */
	case BTP_GAP_READ_CONTROLLER_INDEX_LIST:
		if (hdr->len >= sizeof(struct btp_gap_read_controller_index_list_rp)) {
			const struct btp_gap_read_controller_index_list_rp *rp =
				net_buf_simple_pull_mem(
					buf_simple,
					sizeof(struct btp_gap_read_controller_index_list_rp));

			return rp->num == buf_simple->len;
		} else {
			return false;
		}
	case BTP_GAP_READ_CONTROLLER_INFO:
		return buf_simple->len == sizeof(struct btp_gap_read_controller_info_rp);
	case BTP_GAP_RESET:
		return buf_simple->len == sizeof(struct btp_gap_reset_rp);
	case BTP_GAP_SET_POWERED:
		return buf_simple->len == sizeof(struct btp_gap_set_powered_rp);
	case BTP_GAP_SET_CONNECTABLE:
		return buf_simple->len == sizeof(struct btp_gap_set_connectable_rp);
	case BTP_GAP_SET_FAST_CONNECTABLE:
		return buf_simple->len == sizeof(struct btp_gap_set_fast_connectable_rp);
	case BTP_GAP_SET_DISCOVERABLE:
		return buf_simple->len == sizeof(struct btp_gap_set_discoverable_rp);
	case BTP_GAP_SET_BONDABLE:
		return buf_simple->len == sizeof(struct btp_gap_set_bondable_rp);
	case BTP_GAP_START_ADVERTISING:
		return buf_simple->len == sizeof(struct btp_gap_start_advertising_rp);
	case BTP_GAP_STOP_ADVERTISING:
		return buf_simple->len == sizeof(struct btp_gap_stop_advertising_rp);
	case BTP_GAP_START_DISCOVERY:
		return buf_simple->len == 0U;
	case BTP_GAP_STOP_DISCOVERY:
		return buf_simple->len == 0U;
	case BTP_GAP_CONNECT:
		return buf_simple->len == 0U;
	case BTP_GAP_DISCONNECT:
		return buf_simple->len == 0U;
	case BTP_GAP_SET_IO_CAP:
		return buf_simple->len == 0U;
	case BTP_GAP_PAIR:
		return buf_simple->len == 0U;
	case BTP_GAP_UNPAIR:
		return buf_simple->len == 0U;
	case BTP_GAP_PASSKEY_ENTRY:
		return buf_simple->len == 0U;
	case BTP_GAP_PASSKEY_CONFIRM:
		return buf_simple->len == 0U;
	case BTP_GAP_START_DIRECTED_ADV:
		return buf_simple->len == sizeof(struct btp_gap_start_directed_adv_rp);
	case BTP_GAP_CONN_PARAM_UPDATE:
		return buf_simple->len == 0U;
	case BTP_GAP_PAIRING_CONSENT:
		return buf_simple->len == 0U;
	case BTP_GAP_OOB_LEGACY_SET_DATA:
		return buf_simple->len == 0U;
	case BTP_GAP_OOB_SC_GET_LOCAL_DATA:
		return buf_simple->len == sizeof(struct btp_gap_oob_sc_get_local_data_rp);
	case BTP_GAP_OOB_SC_SET_REMOTE_DATA:
		return buf_simple->len == 0U;
	case BTP_GAP_SET_MITM:
		return buf_simple->len == 0U;
	case BTP_GAP_SET_FILTER_LIST:
		return buf_simple->len == 0U;
	case BTP_GAP_SET_EXTENDED_ADVERTISING:
		return buf_simple->len == sizeof(struct btp_gap_set_extended_advertising_rp);
	case BTP_GAP_PADV_CONFIGURE:
		return buf_simple->len == sizeof(struct btp_gap_padv_start_rp);
	case BTP_GAP_PADV_STOP:
		return buf_simple->len == sizeof(struct btp_gap_padv_stop_rp);
	case BTP_GAP_PADV_SET_DATA:
		return buf_simple->len == 0U;
	case BTP_GAP_PADV_CREATE_SYNC:
		return buf_simple->len == 0U;
	case BTP_GAP_PADV_SYNC_TRANSFER_SET_INFO:
		return buf_simple->len == 0U;
	case BTP_GAP_PADV_SYNC_TRANSFER_START:
		return buf_simple->len == 0U;
	case BTP_GAP_PADV_SYNC_TRANSFER_RECV:
		return buf_simple->len == 0U;

	/* events */
	case BTP_GAP_EV_NEW_SETTINGS:
		return buf_simple->len == sizeof(struct btp_gap_new_settings_ev);
	case BTP_GAP_EV_DEVICE_FOUND:
		if (hdr->len >= sizeof(struct btp_gap_device_found_ev)) {
			const struct btp_gap_device_found_ev *ev = net_buf_simple_pull_mem(
				buf_simple, sizeof(struct btp_gap_device_found_ev));

			return ev->eir_data_len == buf_simple->len;
		} else {
			return false;
		}
	case BTP_GAP_EV_DEVICE_CONNECTED:
		return buf_simple->len == sizeof(struct btp_gap_device_connected_ev);
	case BTP_GAP_EV_DEVICE_DISCONNECTED:
		return buf_simple->len == sizeof(struct btp_gap_device_disconnected_ev);
	case BTP_GAP_EV_PASSKEY_DISPLAY:
		return buf_simple->len == sizeof(struct btp_gap_passkey_display_ev);
	case BTP_GAP_EV_PASSKEY_ENTRY_REQ:
		return buf_simple->len == sizeof(struct btp_gap_passkey_entry_req_ev);
	case BTP_GAP_EV_PASSKEY_CONFIRM_REQ:
		return buf_simple->len == sizeof(struct btp_gap_passkey_confirm_req_ev);
	case BTP_GAP_EV_IDENTITY_RESOLVED:
		return buf_simple->len == sizeof(struct btp_gap_identity_resolved_ev);
	case BTP_GAP_EV_CONN_PARAM_UPDATE:
		return buf_simple->len == sizeof(struct btp_gap_conn_param_update_ev);
	case BTP_GAP_EV_SEC_LEVEL_CHANGED:
		return buf_simple->len == sizeof(struct btp_gap_sec_level_changed_ev);
	case BTP_GAP_EV_PAIRING_CONSENT_REQ:
		return buf_simple->len == sizeof(struct btp_gap_pairing_consent_req_ev);
	case BTP_GAP_EV_BOND_LOST:
		return buf_simple->len == sizeof(struct btp_gap_bond_lost_ev);
	case BTP_GAP_EV_PAIRING_FAILED:
		return buf_simple->len == sizeof(struct btp_gap_bond_pairing_failed_ev);
	case BTP_GAP_EV_PERIODIC_SYNC_ESTABLISHED:
		return buf_simple->len == sizeof(struct btp_gap_ev_periodic_sync_established_ev);
	case BTP_GAP_EV_PERIODIC_SYNC_LOST:
		return buf_simple->len == sizeof(struct btp_gap_ev_periodic_sync_lost_ev);
	case BTP_GAP_EV_PERIODIC_REPORT:
		if (hdr->len >= sizeof(struct btp_gap_ev_periodic_report_ev)) {
			const struct btp_gap_ev_periodic_report_ev *ev = net_buf_simple_pull_mem(
				buf_simple, sizeof(struct btp_gap_ev_periodic_report_ev));

			return ev->data_len == buf_simple->len;
		} else {
			return false;
		}
	case BTP_GAP_EV_PERIODIC_TRANSFER_RECEIVED:
		if (hdr->len >= sizeof(struct btp_gap_ev_periodic_transfer_received_ev)) {
			const struct btp_gap_ev_periodic_transfer_received_ev *ev =
				net_buf_simple_pull_mem(
					buf_simple,
					sizeof(struct btp_gap_ev_periodic_transfer_received_ev));

			return ev->data_len == buf_simple->len;
		} else {
			return false;
		}
	default:
		LOG_ERR("Unhandled opcode 0x%02X", hdr->opcode);
		return false;
	}
}

static bool is_valid_gatt_packet_len(const struct btp_hdr *hdr, struct net_buf_simple *buf_simple)
{
	switch (hdr->opcode) {
	/* responses */
	case BTP_GATT_READ_SUPPORTED_COMMANDS:
		return buf_simple->len > 0U; /* variable length */
	case BTP_GATT_ADD_SERVICE:
		return buf_simple->len == sizeof(struct btp_gatt_add_service_rp);
	case BTP_GATT_ADD_CHARACTERISTIC:
		return buf_simple->len == sizeof(struct btp_gatt_add_characteristic_rp);
	case BTP_GATT_ADD_DESCRIPTOR:
		return buf_simple->len == sizeof(struct btp_gatt_add_descriptor_rp);
	case BTP_GATT_ADD_INCLUDED_SERVICE:
		return buf_simple->len == sizeof(struct btp_gatt_add_included_service_rp);
	case BTP_GATT_SET_VALUE:
		return buf_simple->len == 0U;
	case BTP_GATT_START_SERVER:
		return buf_simple->len == sizeof(struct btp_gatt_start_server_rp);
	case BTP_GATT_RESET_SERVER:
		return buf_simple->len == 0U;
	case BTP_GATT_SET_ENC_KEY_SIZE:
		return buf_simple->len == 0U;
	case BTP_GATT_EXCHANGE_MTU:
		return buf_simple->len == 0U;
	case BTP_GATT_DISC_ALL_PRIM:
		if (hdr->len >= sizeof(struct btp_gatt_disc_all_prim_rp)) {
			const struct btp_gatt_disc_all_prim_rp *rp = net_buf_simple_pull_mem(
				buf_simple, sizeof(struct btp_gatt_disc_all_prim_rp));

			for (uint8_t i = 0U; i < rp->services_count; i++) {
				const struct btp_gatt_service *svc;

				if (buf_simple->len < sizeof(*svc)) {
					return false;
				}

				svc = net_buf_simple_pull_mem(buf_simple, sizeof(*svc));
				if (buf_simple->len < svc->uuid_length) {
					return false;
				}
			}

			return buf_simple->len == 0U;
		} else {
			return false;
		}
	case BTP_GATT_DISC_PRIM_UUID:
		if (hdr->len >= sizeof(struct btp_gatt_disc_prim_rp)) {
			const struct btp_gatt_disc_prim_rp *rp = net_buf_simple_pull_mem(
				buf_simple, sizeof(struct btp_gatt_disc_prim_rp));

			for (uint8_t i = 0U; i < rp->services_count; i++) {
				const struct btp_gatt_service *svc;

				if (buf_simple->len < sizeof(*svc)) {
					return false;
				}

				svc = net_buf_simple_pull_mem(buf_simple, sizeof(*svc));
				if (buf_simple->len < svc->uuid_length) {
					return false;
				}
			}

			return buf_simple->len == 0U;
		} else {
			return false;
		}
	case BTP_GATT_FIND_INCLUDED:
		if (hdr->len >= sizeof(struct btp_gatt_find_included_rp)) {
			const struct btp_gatt_find_included_rp *rp = net_buf_simple_pull_mem(
				buf_simple, sizeof(struct btp_gatt_find_included_rp));

			for (uint8_t i = 0U; i < rp->services_count; i++) {
				const struct btp_gatt_included *incl;

				if (buf_simple->len < sizeof(*incl)) {
					return false;
				}

				incl = net_buf_simple_pull_mem(buf_simple, sizeof(*incl));
				if (buf_simple->len < incl->service.uuid_length) {
					return false;
				}
			}

			return buf_simple->len == 0U;
		} else {
			return false;
		}
	case BTP_GATT_DISC_ALL_CHRC:
		if (hdr->len >= sizeof(struct btp_gatt_disc_chrc_rp)) {
			const struct btp_gatt_disc_chrc_rp *rp = net_buf_simple_pull_mem(
				buf_simple, sizeof(struct btp_gatt_disc_chrc_rp));

			for (uint8_t i = 0U; i < rp->characteristics_count; i++) {
				const struct btp_gatt_characteristic *chrc;

				if (buf_simple->len < sizeof(*chrc)) {
					return false;
				}

				chrc = net_buf_simple_pull_mem(buf_simple, sizeof(*chrc));
				if (buf_simple->len < chrc->uuid_length) {
					return false;
				}
			}

			return buf_simple->len == 0U;
		} else {
			return false;
		}
	case BTP_GATT_DISC_ALL_DESC:
		if (hdr->len >= sizeof(struct btp_gatt_disc_all_desc_rp)) {
			const struct btp_gatt_disc_all_desc_rp *rp = net_buf_simple_pull_mem(
				buf_simple, sizeof(struct btp_gatt_disc_all_desc_rp));

			for (uint8_t i = 0U; i < rp->descriptors_count; i++) {
				const struct btp_gatt_descriptor *desc;

				if (buf_simple->len < sizeof(*desc)) {
					return false;
				}

				desc = net_buf_simple_pull_mem(buf_simple, sizeof(*desc));
				if (buf_simple->len < desc->uuid_length) {
					return false;
				}
			}

			return buf_simple->len == 0U;
		} else {
			return false;
		}
	case BTP_GATT_READ:
		if (hdr->len >= sizeof(struct btp_gatt_read_rp)) {
			const struct btp_gatt_read_rp *rp = net_buf_simple_pull_mem(
				buf_simple, sizeof(struct btp_gatt_read_rp));

			return buf_simple->len == rp->data_length;
		} else {
			return false;
		}
	case BTP_GATT_READ_UUID:
		if (hdr->len >= sizeof(struct btp_gatt_read_uuid_rp)) {
			const struct btp_gatt_read_uuid_rp *rp = net_buf_simple_pull_mem(
				buf_simple, sizeof(struct btp_gatt_read_uuid_rp));

			for (uint8_t i = 0U; i < rp->values_count; i++) {
				const struct btp_gatt_char_value *value;

				if (buf_simple->len < sizeof(*value)) {
					return false;
				}

				value = net_buf_simple_pull_mem(buf_simple, sizeof(*value));
				if (buf_simple->len < value->data_len) {
					return false;
				}
			}

			return buf_simple->len == 0U;
		} else {
			return false;
		}
	case BTP_GATT_READ_LONG:
		if (hdr->len >= sizeof(struct btp_gatt_read_long_rp)) {
			const struct btp_gatt_read_long_rp *rp = net_buf_simple_pull_mem(
				buf_simple, sizeof(struct btp_gatt_read_long_rp));

			return buf_simple->len == rp->data_length;
		} else {
			return false;
		}
	case BTP_GATT_READ_MULTIPLE:
		if (hdr->len >= sizeof(struct btp_gatt_read_multiple_rp)) {
			const struct btp_gatt_read_multiple_rp *rp = net_buf_simple_pull_mem(
				buf_simple, sizeof(struct btp_gatt_read_multiple_rp));

			return buf_simple->len == rp->data_length;
		} else {
			return false;
		}
	case BTP_GATT_WRITE_WITHOUT_RSP:
		return buf_simple->len == 0U;
	case BTP_GATT_SIGNED_WRITE_WITHOUT_RSP:
		return buf_simple->len == 0U;
	case BTP_GATT_WRITE:
		return buf_simple->len == sizeof(struct btp_gatt_write_rp);
	case BTP_GATT_WRITE_LONG:
		return buf_simple->len == sizeof(struct btp_gatt_write_long_rp);
	case BTP_GATT_RELIABLE_WRITE:
		return buf_simple->len == sizeof(struct btp_gatt_reliable_write_rp);
	case BTP_GATT_GET_ATTRIBUTES:
		if (hdr->len >= sizeof(struct btp_gatt_get_attributes_rp)) {
			const struct btp_gatt_get_attributes_rp *rp = net_buf_simple_pull_mem(
				buf_simple, sizeof(struct btp_gatt_get_attributes_rp));

			return buf_simple->len == rp->attrs_count;
		} else {
			return false;
		}
	case BTP_GATT_GET_ATTRIBUTE_VALUE:
		if (hdr->len >= sizeof(struct btp_gatt_get_attribute_value_rp)) {
			const struct btp_gatt_get_attribute_value_rp *rp = net_buf_simple_pull_mem(
				buf_simple, sizeof(struct btp_gatt_get_attribute_value_rp));

			return buf_simple->len == rp->value_length;
		} else {
			return false;
		}
	case BTP_GATT_CHANGE_DB:
		return buf_simple->len == 0U;
	case BTP_GATT_EATT_CONNECT:
		return buf_simple->len == 0U;
	case BTP_GATT_READ_MULTIPLE_VAR:
		if (hdr->len >= sizeof(struct btp_gatt_read_multiple_var_rp)) {
			const struct btp_gatt_read_multiple_var_rp *rp = net_buf_simple_pull_mem(
				buf_simple, sizeof(struct btp_gatt_read_multiple_var_rp));

			return buf_simple->len == rp->data_length;
		} else {
			return false;
		}
	case BTP_GATT_NOTIFY_MULTIPLE:
		return buf_simple->len == 0U;

	/* events */
	case BTP_GATT_EV_NOTIFICATION:
		if (hdr->len >= sizeof(struct btp_gatt_notification_ev)) {
			const struct btp_gatt_notification_ev *ev = net_buf_simple_pull_mem(
				buf_simple, sizeof(struct btp_gatt_notification_ev));

			return ev->data_length == buf_simple->len;
		} else {
			return false;
		}
	case BTP_GATT_EV_ATTR_VALUE_CHANGED:
		if (hdr->len >= sizeof(struct btp_gatt_attr_value_changed_ev)) {
			const struct btp_gatt_attr_value_changed_ev *ev = net_buf_simple_pull_mem(
				buf_simple, sizeof(struct btp_gatt_attr_value_changed_ev));

			return ev->data_length == buf_simple->len;
		} else {
			return false;
		}

	default:
		LOG_ERR("Unhandled opcode 0x%02X", hdr->opcode);
		return false;
	}
}

static bool is_valid_l2cap_packet_len(const struct btp_hdr *hdr, struct net_buf_simple *buf_simple)
{
	switch (hdr->opcode) {
	/* responses */
	case BTP_L2CAP_READ_SUPPORTED_COMMANDS:
		return buf_simple->len > 0U; /* variable length */
	case BTP_L2CAP_CONNECT:
		if (hdr->len >= sizeof(struct btp_l2cap_connect_rp)) {
			const struct btp_l2cap_connect_rp *rp = net_buf_simple_pull_mem(
				buf_simple, sizeof(struct btp_l2cap_connect_rp));

			return rp->num == buf_simple->len;
		} else {
			return false;
		}
	case BTP_L2CAP_DISCONNECT:
		return buf_simple->len == 0U;
	case BTP_L2CAP_SEND_DATA:
		return buf_simple->len == 0U;
	case BTP_L2CAP_LISTEN:
		return buf_simple->len == 0U;
	case BTP_L2CAP_ACCEPT_CONNECTION:
		return buf_simple->len == 0U;
	case BTP_L2CAP_RECONFIGURE:
		return buf_simple->len == 0U;
	case BTP_L2CAP_CREDITS:
		return buf_simple->len == 0U;
	case BTP_L2CAP_DISCONNECT_EATT_CHANS:
		return buf_simple->len == 0U;

	/* events */
	case BTP_L2CAP_EV_CONNECTION_REQ:
		return buf_simple->len == sizeof(struct btp_l2cap_connection_req_ev);
	case BTP_L2CAP_EV_CONNECTED:
		return buf_simple->len == sizeof(struct btp_l2cap_connected_ev);
	case BTP_L2CAP_EV_DISCONNECTED:
		return buf_simple->len == sizeof(struct btp_l2cap_disconnected_ev);
	case BTP_L2CAP_EV_DATA_RECEIVED:
		if (hdr->len >= sizeof(struct btp_gatt_attr_value_changed_ev)) {
			const struct btp_gatt_attr_value_changed_ev *ev = net_buf_simple_pull_mem(
				buf_simple, sizeof(struct btp_gatt_attr_value_changed_ev));

			return ev->data_length == buf_simple->len;
		} else {
			return false;
		}
	case BTP_L2CAP_EV_RECONFIGURED:
		return buf_simple->len == sizeof(struct btp_l2cap_reconfigured_ev);
	default:
		LOG_ERR("Unhandled opcode 0x%02X", hdr->opcode);
		return false;
	}
}

static bool is_valid_mesh_packet_len(const struct btp_hdr *hdr, struct net_buf_simple *buf_simple)
{
	switch (hdr->opcode) {
	/* responses */
	case BTP_MESH_READ_SUPPORTED_COMMANDS:
		return buf_simple->len > 0U; /* variable length */
	case BTP_MESH_CONFIG_PROVISIONING:
		return buf_simple->len == 0U;
	case BTP_MESH_PROVISION_NODE:
		return buf_simple->len == 0U;
	case BTP_MESH_INIT:
		return buf_simple->len == 0U;
	case BTP_MESH_INPUT_STRING:
		return buf_simple->len == 0U;
	case BTP_MESH_IVU_TEST_MODE:
		return buf_simple->len == 0U;
	case BTP_MESH_IVU_TOGGLE_STATE:
		return buf_simple->len == 0U;
	case BTP_MESH_NET_SEND:
		return buf_simple->len == 0U;
	case BTP_MESH_HEALTH_GENERATE_FAULTS:
		if (hdr->len >= sizeof(struct btp_mesh_health_generate_faults_rp)) {
			const struct btp_mesh_health_generate_faults_rp *rp =
				net_buf_simple_pull_mem(
					buf_simple,
					sizeof(struct btp_mesh_health_generate_faults_rp));

			return (rp->cur_faults_count + rp->reg_faults_count) == buf_simple->len;
		} else {
			return false;
		}
	case BTP_MESH_HEALTH_CLEAR_FAULTS:
		return buf_simple->len == 0U;
	case BTP_MESH_LPN:
		return buf_simple->len == 0U;
	case BTP_MESH_LPN_POLL:
		return buf_simple->len == 0U;
	case BTP_MESH_MODEL_SEND:
		return buf_simple->len == 0U;
	case BTP_MESH_LPN_SUBSCRIBE:
		return buf_simple->len == 0U;
	case BTP_MESH_LPN_UNSUBSCRIBE:
		return buf_simple->len == 0U;
	case BTP_MESH_RPL_CLEAR:
		return buf_simple->len == 0U;
	case BTP_MESH_PROXY_IDENTITY:
		return buf_simple->len == 0U;
	case BTP_MESH_COMP_DATA_GET:
		return buf_simple->len > 0U; /* variable length */
	case BTP_MESH_CFG_BEACON_GET:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_beacon_get_rp);
	case BTP_MESH_CFG_BEACON_SET:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_beacon_set_rp);
	case BTP_MESH_CFG_DEFAULT_TTL_GET:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_default_ttl_get_rp);
	case BTP_MESH_CFG_DEFAULT_TTL_SET:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_default_ttl_set_rp);
	case BTP_MESH_CFG_GATT_PROXY_GET:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_gatt_proxy_get_rp);
	case BTP_MESH_CFG_GATT_PROXY_SET:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_gatt_proxy_set_rp);
	case BTP_MESH_CFG_FRIEND_GET:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_friend_get_rp);
	case BTP_MESH_CFG_FRIEND_SET:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_friend_set_rp);
	case BTP_MESH_CFG_RELAY_GET:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_relay_get_rp);
	case BTP_MESH_CFG_RELAY_SET:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_relay_set_rp);
	case BTP_MESH_CFG_MODEL_PUB_GET:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_model_pub_get_rp);
	case BTP_MESH_CFG_MODEL_PUB_SET:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_model_pub_set_rp);
	case BTP_MESH_CFG_MODEL_SUB_ADD:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_model_sub_add_rp);
	case BTP_MESH_CFG_MODEL_SUB_DEL:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_model_sub_del_rp);
	case BTP_MESH_CFG_NETKEY_ADD:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_netkey_add_rp);
	case BTP_MESH_CFG_NETKEY_GET:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_netkey_get_rp);
	case BTP_MESH_CFG_NETKEY_DEL:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_netkey_del_rp);
	case BTP_MESH_CFG_APPKEY_ADD:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_appkey_add_rp);
	case BTP_MESH_CFG_APPKEY_DEL:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_appkey_del_rp);
	case BTP_MESH_CFG_APPKEY_GET:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_appkey_get_rp);
	case BTP_MESH_CFG_MODEL_APP_BIND:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_model_app_bind_rp);
	case BTP_MESH_CFG_MODEL_APP_UNBIND:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_model_app_unbind_rp);
	case BTP_MESH_CFG_MODEL_APP_GET:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_model_app_get_rp);
	case BTP_MESH_CFG_MODEL_APP_VND_GET:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_model_app_vnd_get_rp);
	case BTP_MESH_CFG_HEARTBEAT_PUB_SET:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_heartbeat_pub_set_rp);
	case BTP_MESH_CFG_HEARTBEAT_PUB_GET:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_heartbeat_pub_get_rp);
	case BTP_MESH_CFG_HEARTBEAT_SUB_SET:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_heartbeat_sub_set_rp);
	case BTP_MESH_CFG_HEARTBEAT_SUB_GET:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_heartbeat_sub_get_rp);
	case BTP_MESH_CFG_NET_TRANS_GET:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_net_trans_get_rp);
	case BTP_MESH_CFG_NET_TRANS_SET:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_net_trans_set_rp);
	case BTP_MESH_CFG_MODEL_SUB_OVW:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_model_sub_ovw_rp);
	case BTP_MESH_CFG_MODEL_SUB_DEL_ALL:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_model_sub_del_all_rp);
	case BTP_MESH_CFG_MODEL_SUB_GET:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_model_sub_get_rp);
	case BTP_MESH_CFG_MODEL_SUB_GET_VND:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_model_sub_get_vnd_rp);
	case BTP_MESH_CFG_MODEL_SUB_VA_ADD:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_model_sub_va_add_rp);
	case BTP_MESH_CFG_MODEL_SUB_VA_DEL:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_model_sub_va_del_rp);
	case BTP_MESH_CFG_MODEL_SUB_VA_OVW:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_model_sub_va_ovw_rp);
	case BTP_MESH_CFG_NETKEY_UPDATE:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_netkey_update_rp);
	case BTP_MESH_CFG_APPKEY_UPDATE:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_appkey_update_rp);
	case BTP_MESH_CFG_NODE_IDT_SET:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_node_idt_set_rp);
	case BTP_MESH_CFG_NODE_IDT_GET:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_node_idt_get_rp);
	case BTP_MESH_CFG_NODE_RESET:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_node_reset_rp);
	case BTP_MESH_CFG_LPN_TIMEOUT_GET:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_lpn_timeout_rp);
	case BTP_MESH_CFG_MODEL_PUB_VA_SET:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_model_pub_va_set_rp);
	case BTP_MESH_CFG_MODEL_APP_BIND_VND:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_model_app_bind_vnd_rp);
	case BTP_MESH_HEALTH_FAULT_GET:
		return buf_simple->len == 0U;
	case BTP_MESH_HEALTH_FAULT_CLEAR:
		return buf_simple->len == sizeof(struct btp_mesh_health_fault_clear_rp);
	case BTP_MESH_HEALTH_FAULT_TEST:
		return buf_simple->len == sizeof(struct btp_mesh_health_fault_test_rp);
	case BTP_MESH_HEALTH_PERIOD_GET:
		return buf_simple->len == 0U;
	case BTP_MESH_HEALTH_PERIOD_SET:
		return buf_simple->len == sizeof(struct btp_mesh_health_period_set_rp);
	case BTP_MESH_HEALTH_ATTENTION_GET:
		return buf_simple->len == 0U;
	case BTP_MESH_HEALTH_ATTENTION_SET:
		return buf_simple->len == sizeof(struct btp_mesh_health_attention_set_rp);
	case BTP_MESH_PROVISION_ADV:
		return buf_simple->len == 0U;
	case BTP_MESH_CFG_KRP_GET:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_krp_get_rp);
	case BTP_MESH_CFG_KRP_SET:
		return buf_simple->len == sizeof(struct btp_mesh_cfg_krp_set_rp);
	case BTP_MESH_VA_ADD:
		return buf_simple->len == sizeof(struct btp_mesh_va_add_rp);
	case BTP_MESH_PROXY_CONNECT:
		return buf_simple->len == 0U;
	case BTP_MESH_SAR_TRANSMITTER_GET:
		return buf_simple->len == 0U;
	case BTP_MESH_SAR_TRANSMITTER_SET:
		return buf_simple->len == 0U;
	case BTP_MESH_SAR_RECEIVER_GET:
		return buf_simple->len == 0U;
	case BTP_MESH_SAR_RECEIVER_SET:
		return buf_simple->len == 0U;
	case BTP_MESH_LARGE_COMP_DATA_GET:
		return true /* variable length */;
	case BTP_MESH_MODELS_METADATA_GET:
		return true /* variable length */;
	case BTP_MESH_OPCODES_AGGREGATOR_INIT:
		return buf_simple->len == 0U;
	case BTP_MESH_OPCODES_AGGREGATOR_SEND:
		return buf_simple->len == 0U;
	case BTP_MESH_COMP_CHANGE_PREPARE:
		return buf_simple->len == 0U;
	case BTP_MESH_RPR_SCAN_START:
		return buf_simple->len == 0U;
	case BTP_MESH_RPR_EXT_SCAN_START:
		return buf_simple->len == 0U;
	case BTP_MESH_RPR_SCAN_CAPS_GET:
		return buf_simple->len == 0U;
	case BTP_MESH_RPR_SCAN_GET:
		return buf_simple->len == 0U;
	case BTP_MESH_RPR_SCAN_STOP:
		return buf_simple->len == 0U;
	case BTP_MESH_RPR_LINK_GET:
		return buf_simple->len == 0U;
	case BTP_MESH_RPR_LINK_CLOSE:
		return buf_simple->len == 0U;
	case BTP_MESH_RPR_PROV_REMOTE:
		return buf_simple->len == 0U;
	case BTP_MESH_RPR_REPROV_REMOTE:
		return buf_simple->len == 0U;
	case BTP_MESH_SUBNET_BRIDGE_GET:
		return buf_simple->len == 0U;
	case BTP_MESH_SUBNET_BRIDGE_SET:
		return buf_simple->len == 0U;
	case BTP_MESH_BRIDGING_TABLE_ADD:
		return buf_simple->len == 0U;
	case BTP_MESH_BRIDGING_TABLE_REMOVE:
		return buf_simple->len == 0U;
	case BTP_MESH_BRIDGED_SUBNETS_GET:
		return buf_simple->len == 0U;
	case BTP_MESH_BRIDGING_TABLE_GET:
		return buf_simple->len == 0U;
	case BTP_MESH_BRIDGING_TABLE_SIZE_GET:
		return buf_simple->len == 0U;
	case BTP_MESH_PRIV_BEACON_GET:
		return buf_simple->len == 0U;
	case BTP_MESH_PRIV_BEACON_SET:
		return buf_simple->len == 0U;
	case BTP_MESH_PRIV_GATT_PROXY_GET:
		return buf_simple->len == 0U;
	case BTP_MESH_PRIV_GATT_PROXY_SET:
		return buf_simple->len == 0U;
	case BTP_MESH_PRIV_NODE_ID_GET:
		return buf_simple->len == 0U;
	case BTP_MESH_PRIV_NODE_ID_SET:
		return buf_simple->len == 0U;
	case BTP_MESH_PROXY_PRIVATE_IDENTITY:
		return buf_simple->len == 0U;
	case BTP_MESH_OD_PRIV_PROXY_GET:
		return buf_simple->len == 0U;
	case BTP_MESH_OD_PRIV_PROXY_SET:
		return buf_simple->len == 0U;
	case BTP_MESH_SRPL_CLEAR:
		return buf_simple->len == 0U;
	case BTP_MESH_PROXY_SOLICIT:
		return buf_simple->len == 0U;
	case BTP_MESH_START:
		return buf_simple->len == 0U;

	/* events */
	case BTP_MESH_EV_OUT_NUMBER_ACTION:
		return buf_simple->len == sizeof(struct btp_mesh_out_number_action_ev);
	case BTP_MESH_EV_OUT_STRING_ACTION:
		if (hdr->len >= sizeof(struct btp_mesh_out_string_action_ev)) {
			const struct btp_mesh_out_string_action_ev *ev = net_buf_simple_pull_mem(
				buf_simple, sizeof(struct btp_mesh_out_string_action_ev));

			return ev->string_len == buf_simple->len;
		} else {
			return false;
		}
	case BTP_MESH_EV_IN_ACTION:
		return buf_simple->len == sizeof(struct btp_mesh_in_action_ev);
	case BTP_MESH_EV_PROVISIONED:
		return buf_simple->len == 0U;
	case BTP_MESH_EV_PROV_LINK_OPEN:
		return buf_simple->len == sizeof(struct btp_mesh_prov_link_open_ev);
	case BTP_MESH_EV_NET_RECV:
		if (hdr->len >= sizeof(struct btp_mesh_net_recv_ev)) {
			const struct btp_mesh_net_recv_ev *ev = net_buf_simple_pull_mem(
				buf_simple, sizeof(struct btp_mesh_net_recv_ev));

			return ev->payload_len == buf_simple->len;
		} else {
			return false;
		}
	case BTP_MESH_EV_INVALID_BEARER:
		return buf_simple->len == sizeof(struct btp_mesh_invalid_bearer_ev);
	case BTP_MESH_EV_INCOMP_TIMER_EXP:
		return buf_simple->len == 0U;
	case BTP_MESH_EV_FRND_ESTABLISHED:
		return buf_simple->len == sizeof(struct btp_mesh_frnd_established_ev);
	case BTP_MESH_EV_FRND_TERMINATED:
		return buf_simple->len == sizeof(struct btp_mesh_frnd_terminated_ev);
	case BTP_MESH_EV_LPN_ESTABLISHED:
		return buf_simple->len == sizeof(struct btp_mesh_lpn_established_ev);
	case BTP_MESH_EV_LPN_TERMINATED:
		return buf_simple->len == sizeof(struct btp_mesh_lpn_terminated_ev);
	case BTP_MESH_EV_LPN_POLLED:
		return buf_simple->len == sizeof(struct btp_mesh_lpn_polled_ev);
	case BTP_MESH_EV_PROV_NODE_ADDED:
		return buf_simple->len == sizeof(struct btp_mesh_prov_node_added_ev);
	case BTP_MESH_EV_MODEL_RECV:
		return buf_simple->len == sizeof(struct btp_mesh_model_recv_ev);
	case MESH_EV_BLOB_LOST_TARGET:
		return buf_simple->len == 0U;
	default:
		LOG_ERR("Unhandled opcode 0x%02X", hdr->opcode);
		return false;
	}
}

static bool is_valid_mesh_mdl_packet_len(const struct btp_hdr *hdr,
					 struct net_buf_simple *buf_simple)
{
	switch (hdr->opcode) {
	case BTP_MMDL_DFU_INFO_GET:
		return buf_simple->len == 0U;
	case BTP_MMDL_BLOB_INFO_GET:
		return buf_simple->len == 0U;
	case BTP_MMDL_DFU_UPDATE_METADATA_CHECK:
		return buf_simple->len == sizeof(struct btp_mmdl_dfu_metadata_check_rp);
	case BTP_MMDL_DFU_FIRMWARE_UPDATE_GET:
		return buf_simple->len == 0U;
	case BTP_MMDL_DFU_FIRMWARE_UPDATE_CANCEL:
		return buf_simple->len == 0U;
	case BTP_MMDL_DFU_FIRMWARE_UPDATE_START:
		return buf_simple->len == sizeof(struct btp_mmdl_dfu_firmware_update_rp);
	case BTP_MMDL_BLOB_TRANSFER_START:
		return buf_simple->len == 0U;
	case BTP_MMDL_BLOB_TRANSFER_CANCEL:
		return buf_simple->len == 0U;
	case BTP_MMDL_BLOB_TRANSFER_GET:
		return buf_simple->len == 0U;
	case BTP_MMDL_BLOB_SRV_CANCEL:
		return buf_simple->len == 0U;
	case BTP_MMDL_DFU_FIRMWARE_UPDATE_APPLY:
		return buf_simple->len == 0U;
	case BTP_MMDL_DFU_SRV_APPLY:
		return buf_simple->len == 0U;
	default:
		LOG_ERR("Unhandled opcode 0x%02X", hdr->opcode);
		return false;
	}
}

static bool is_valid_vcs_packet_len(const struct btp_hdr *hdr, struct net_buf_simple *buf_simple)
{
	switch (hdr->opcode) {
	/* responses */
	case BTP_VCS_READ_SUPPORTED_COMMANDS:
		return buf_simple->len > 0U; /* variable length */
	case BTP_VCS_SET_VOL:
		return buf_simple->len == 0U;
	case BTP_VCS_VOL_UP:
		return buf_simple->len == 0U;
	case BTP_VCS_VOL_DOWN:
		return buf_simple->len == 0U;
	case BTP_VCS_MUTE:
		return buf_simple->len == 0U;
	case BTP_VCS_UNMUTE:
		return buf_simple->len == 0U;

	/* no events */
	default:
		LOG_ERR("Unhandled opcode 0x%02X", hdr->opcode);
		return false;
	}
}

static bool is_valid_ias_packet_len(const struct btp_hdr *hdr, struct net_buf_simple *buf_simple)
{
	switch (hdr->opcode) {
	/* no responses */

	/* events */
	case BTP_IAS_EV_OUT_ALERT_ACTION:
		return buf_simple->len == sizeof(struct btp_ias_alert_action_ev);
	default:
		LOG_ERR("Unhandled opcode 0x%02X", hdr->opcode);
		return false;
	}
}

static bool is_valid_aics_packet_len(const struct btp_hdr *hdr, struct net_buf_simple *buf_simple)
{
	switch (hdr->opcode) {
	/* responses */
	case BTP_AICS_READ_SUPPORTED_COMMANDS:
		return buf_simple->len > 0U; /* variable length */
	case BTP_AICS_SET_GAIN:
		return buf_simple->len == 0U;
	case BTP_AICS_MUTE:
		return buf_simple->len == 0U;
	case BTP_AICS_UNMUTE:
		return buf_simple->len == 0U;
	case BTP_AICS_MAN_GAIN_SET:
		return buf_simple->len == 0U;
	case BTP_AICS_AUTO_GAIN_SET:
		return buf_simple->len == 0U;
	case BTP_AICS_SET_MAN_GAIN_ONLY:
		return buf_simple->len == 0U;
	case BTP_AICS_SET_AUTO_GAIN_ONLY:
		return buf_simple->len == 0U;
	case BTP_AICS_AUDIO_DESCRIPTION_SET:
		return buf_simple->len == 0U;
	case BTP_AICS_MUTE_DISABLE:
		return buf_simple->len == 0U;
	case BTP_AICS_GAIN_SETTING_PROP_GET:
		return buf_simple->len == 0U;
	case BTP_AICS_TYPE_GET:
		return buf_simple->len == 0U;
	case BTP_AICS_STATUS_GET:
		return buf_simple->len == 0U;
	case BTP_AICS_STATE_GET:
		return buf_simple->len == 0U;
	case BTP_AICS_DESCRIPTION_GET:
		return buf_simple->len == 0U;

	/* events */
	case BTP_AICS_STATE_EV:
		return buf_simple->len == sizeof(struct btp_aics_state_ev);
	case BTP_GAIN_SETTING_PROPERTIES_EV:
		return buf_simple->len == sizeof(struct btp_gain_setting_properties_ev);
	case BTP_AICS_INPUT_TYPE_EV:
		return buf_simple->len == sizeof(struct btp_aics_input_type_ev);
	case BTP_AICS_DESCRIPTION_EV:
		if (hdr->len >= sizeof(struct btp_aics_description_ev)) {
			const struct btp_aics_description_ev *ev = net_buf_simple_pull_mem(
				buf_simple, sizeof(struct btp_aics_description_ev));

			return ev->data_len == buf_simple->len;
		} else {
			return false;
		}
	case BTP_AICS_PROCEDURE_EV:
		return buf_simple->len == sizeof(struct btp_aics_procedure_ev);
	default:
		LOG_ERR("Unhandled opcode 0x%02X", hdr->opcode);
		return false;
	}
}

static bool is_valid_vocs_packet_len(const struct btp_hdr *hdr, struct net_buf_simple *buf_simple)
{
	switch (hdr->opcode) {
	/* responses */
	case BTP_VOCS_READ_SUPPORTED_COMMANDS:
		return buf_simple->len > 0U; /* variable length */
	case BTP_VOCS_UPDATE_LOC:
		return buf_simple->len == 0U;
	case BTP_VOCS_UPDATE_DESC:
		return buf_simple->len == 0U;
	case BTP_VOCS_STATE_GET:
		return buf_simple->len == 0U;
	case BTP_AICS_DESCRIPTION_GET:
		return buf_simple->len == 0U;
	case BTP_VOCS_LOCATION_GET:
		return buf_simple->len == 0U;
	case BTP_VOCS_OFFSET_STATE_SET:
		return buf_simple->len == 0U;

	/* events */
	case BTP_VOCS_OFFSET_STATE_EV:
		return buf_simple->len == sizeof(struct btp_vocs_offset_state_ev);
	case BTP_VOCS_AUDIO_LOCATION_EV:
		return buf_simple->len == sizeof(struct btp_vocs_audio_location_ev);
	case BTP_VOCS_PROCEDURE_EV:
		return buf_simple->len == sizeof(struct btp_vocs_procedure_ev);
	default:
		LOG_ERR("Unhandled opcode 0x%02X", hdr->opcode);
		return false;
	}
}

static bool is_valid_pacs_packet_len(const struct btp_hdr *hdr, struct net_buf_simple *buf_simple)
{
	switch (hdr->opcode) {
	/* responses */
	case BTP_PACS_READ_SUPPORTED_COMMANDS:
		return buf_simple->len > 0U; /* variable length */
	case BTP_PACS_UPDATE_CHARACTERISTIC:
		return buf_simple->len == 0U;
	case BTP_PACS_SET_LOCATION:
		return buf_simple->len == 0U;
	case BTP_PACS_SET_AVAILABLE_CONTEXTS:
		return buf_simple->len == 0U;
	case BTP_PACS_SET_SUPPORTED_CONTEXTS:

	/* No events */
	default:
		LOG_ERR("Unhandled opcode 0x%02X", hdr->opcode);
		return false;
	}
}

static bool is_valid_ascs_packet_len(const struct btp_hdr *hdr, struct net_buf_simple *buf_simple)
{
	switch (hdr->opcode) {
	/* responses */
	case BTP_ASCS_READ_SUPPORTED_COMMANDS:
		return buf_simple->len > 0U; /* variable length */
	case BTP_ASCS_CONFIGURE_CODEC:
		return buf_simple->len == 0U;
	case BTP_ASCS_CONFIGURE_QOS:
		return buf_simple->len == 0U;
	case BTP_ASCS_ENABLE:
		return buf_simple->len == 0U;
	case BTP_ASCS_RECEIVER_START_READY:
		return buf_simple->len == 0U;
	case BTP_ASCS_RECEIVER_STOP_READY:
		return buf_simple->len == 0U;
	case BTP_ASCS_DISABLE:
		return buf_simple->len == 0U;
	case BTP_ASCS_RELEASE:
		return buf_simple->len == 0U;
	case BTP_ASCS_UPDATE_METADATA:
		return buf_simple->len == 0U;
	case BTP_ASCS_ADD_ASE_TO_CIS:
		return buf_simple->len == 0U;
	case BTP_ASCS_PRECONFIGURE_QOS:
		return buf_simple->len == 0U;

	/* events */
	case BTP_ASCS_EV_OPERATION_COMPLETED:
		return buf_simple->len == sizeof(struct btp_ascs_operation_completed_ev);
	case BTP_ASCS_EV_CHARACTERISTIC_SUBSCRIBED:
		return buf_simple->len == 0U;
	case BTP_ASCS_EV_ASE_STATE_CHANGED:
		return buf_simple->len == sizeof(struct btp_ascs_ase_state_changed_ev);
	default:
		LOG_ERR("Unhandled opcode 0x%02X", hdr->opcode);
		return false;
	}
}

static bool is_valid_bap_packet_len(const struct btp_hdr *hdr, struct net_buf_simple *buf_simple)
{
	switch (hdr->opcode) {
	/* responses */
	case BTP_BAP_READ_SUPPORTED_COMMANDS:
		return buf_simple->len > 0U; /* variable length */
	case BTP_BAP_DISCOVER:
		return buf_simple->len == 0U;
	case BTP_BAP_SEND:
		return buf_simple->len == sizeof(struct btp_bap_send_rp);
	case BTP_BAP_BROADCAST_SOURCE_SETUP:
		return buf_simple->len == sizeof(struct btp_bap_broadcast_source_setup_rp);
	case BTP_BAP_BROADCAST_SOURCE_RELEASE:
		return buf_simple->len == 0U;
	case BTP_BAP_BROADCAST_ADV_START:
		return buf_simple->len == 0U;
	case BTP_BAP_BROADCAST_ADV_STOP:
		return buf_simple->len == 0U;
	case BTP_BAP_BROADCAST_SOURCE_START:
		return buf_simple->len == 0U;
	case BTP_BAP_BROADCAST_SOURCE_STOP:
		return buf_simple->len == 0U;
	case BTP_BAP_BROADCAST_SINK_SETUP:
		return buf_simple->len == 0U;
	case BTP_BAP_BROADCAST_SINK_RELEASE:
		return buf_simple->len == 0U;
	case BTP_BAP_BROADCAST_SCAN_START:
		return buf_simple->len == 0U;
	case BTP_BAP_BROADCAST_SCAN_STOP:
		return buf_simple->len == 0U;
	case BTP_BAP_BROADCAST_SINK_SYNC:
		return buf_simple->len == 0U;
	case BTP_BAP_BROADCAST_SINK_STOP:
		return buf_simple->len == 0U;
	case BTP_BAP_BROADCAST_SINK_BIS_SYNC:
		return buf_simple->len == 0U;
	case BTP_BAP_DISCOVER_SCAN_DELEGATORS:
		return buf_simple->len == 0U;
	case BTP_BAP_BROADCAST_ASSISTANT_SCAN_START:
		return buf_simple->len == 0U;
	case BTP_BAP_BROADCAST_ASSISTANT_SCAN_STOP:
		return buf_simple->len == 0U;
	case BTP_BAP_ADD_BROADCAST_SRC:
		return buf_simple->len == 0U;
	case BTP_BAP_REMOVE_BROADCAST_SRC:
		return buf_simple->len == 0U;
	case BTP_BAP_MODIFY_BROADCAST_SRC:
		return buf_simple->len == 0U;
	case BTP_BAP_SET_BROADCAST_CODE:
		return buf_simple->len == 0U;
	case BTP_BAP_SEND_PAST:
		return buf_simple->len == 0U;

	/* events */
	case BTP_BAP_EV_DISCOVERY_COMPLETED:
		return buf_simple->len == sizeof(struct btp_bap_discovery_completed_ev);
	case BTP_BAP_EV_CODEC_CAP_FOUND:
		return buf_simple->len == sizeof(struct btp_bap_codec_cap_found_ev);
	case BTP_BAP_EV_ASE_FOUND:
		return buf_simple->len == sizeof(struct btp_bap_ase_found_ev);
	case BTP_BAP_EV_STREAM_RECEIVED:
		if (hdr->len >= sizeof(struct btp_bap_stream_received_ev)) {
			const struct btp_bap_stream_received_ev *ev = net_buf_simple_pull_mem(
				buf_simple, sizeof(struct btp_bap_stream_received_ev));

			return ev->data_len == buf_simple->len;
		} else {
			return false;
		}
	case BTP_BAP_EV_BAA_FOUND:
		return buf_simple->len == sizeof(struct btp_bap_baa_found_ev);
	case BTP_BAP_EV_BIS_FOUND:
		if (hdr->len >= sizeof(struct btp_bap_bis_found_ev)) {
			const struct btp_bap_bis_found_ev *ev = net_buf_simple_pull_mem(
				buf_simple, sizeof(struct btp_bap_bis_found_ev));

			return ev->cc_ltvs_len == buf_simple->len;
		} else {
			return false;
		}
	case BTP_BAP_EV_BIS_STREAM_RECEIVED:
		if (hdr->len >= sizeof(struct btp_bap_stream_received_ev)) {
			const struct btp_bap_bis_stream_received_ev *ev = net_buf_simple_pull_mem(
				buf_simple, sizeof(struct btp_bap_bis_stream_received_ev));

			return ev->data_len == buf_simple->len;
		} else {
			return false;
		}
	case BTP_BAP_EV_SCAN_DELEGATOR_FOUND:
		return buf_simple->len == sizeof(struct btp_bap_scan_delegator_found_ev);
	case BTP_BAP_EV_BROADCAST_RECEIVE_STATE:
		if (hdr->len >= sizeof(struct btp_bap_broadcast_receive_state_ev)) {
			const struct btp_bap_broadcast_receive_state_ev *ev =
				net_buf_simple_pull_mem(
					buf_simple,
					sizeof(struct btp_bap_broadcast_receive_state_ev));

			for (uint8_t i = 0U; i < ev->num_subgroups; i++) {
				/* Each subgroup consists of 4 octets of BIS sync, 1 octets
				 * of metadata length, and then the metadata
				 */
				uint8_t metadata_len;
				uint32_t bis_sync;

				if (buf_simple->len <= (sizeof(bis_sync) + sizeof(metadata_len))) {
					return false;
				}

				(void)net_buf_simple_pull_le32(buf_simple);
				metadata_len = net_buf_simple_pull_u8(buf_simple);
				if (buf_simple->len < metadata_len) {
					return false;
				}
				(void)net_buf_simple_pull_mem(buf_simple, metadata_len);
			}

			return buf_simple->len == 0U;
		} else {
			return false;
		}
	case BTP_BAP_EV_PA_SYNC_REQ:
		return buf_simple->len == sizeof(struct btp_bap_pa_sync_req_ev);
	default:
		LOG_ERR("Unhandled opcode 0x%02X", hdr->opcode);
		return false;
	}
}

static bool is_valid_has_packet_len(const struct btp_hdr *hdr, struct net_buf_simple *buf_simple)
{
	switch (hdr->opcode) {
	/* responses */
	case BTP_HAS_READ_SUPPORTED_COMMANDS:
		return buf_simple->len > 0U; /* variable length */
	case BTP_HAS_SET_ACTIVE_INDEX:
		return buf_simple->len == 0U;
	case BTP_HAS_SET_PRESET_NAME:
		return buf_simple->len == 0U;
	case BTP_HAS_REMOVE_PRESET:
		return buf_simple->len == 0U;
	case BTP_HAS_ADD_PRESET:
		return buf_simple->len == 0U;
	case BTP_HAS_SET_PROPERTIES:
		return buf_simple->len == 0U;

	/* events */
	case BTP_HAS_EV_OPERATION_COMPLETED:
		return buf_simple->len == sizeof(struct btp_has_operation_completed_ev);
	default:
		LOG_ERR("Unhandled opcode 0x%02X", hdr->opcode);
		return false;
	}
}

static bool is_valid_micp_packet_len(const struct btp_hdr *hdr, struct net_buf_simple *buf_simple)
{
	switch (hdr->opcode) {
	/* responses */
	case BTP_MICP_READ_SUPPORTED_COMMANDS:
		return buf_simple->len > 0U; /* variable length */
	case BTP_MICP_CTLR_DISCOVER:
		return buf_simple->len == 0U;
	case BTP_MICP_CTLR_MUTE_READ:
		return buf_simple->len == 0U;
	case BTP_MICP_CTLR_MUTE:
		return buf_simple->len == 0U;

	/* events */
	case BTP_MICP_DISCOVERED_EV:
		return buf_simple->len == sizeof(struct btp_micp_discovered_ev);
	case BTP_MICP_MUTE_STATE_EV:
		return buf_simple->len == sizeof(struct btp_micp_mute_state_ev);
	default:
		LOG_ERR("Unhandled opcode 0x%02X", hdr->opcode);
		return false;
	}
}

static bool is_valid_csis_packet_len(const struct btp_hdr *hdr, struct net_buf_simple *buf_simple)
{
	switch (hdr->opcode) {
	/* responses */
	case BTP_CSIS_READ_SUPPORTED_COMMANDS:
		return buf_simple->len > 0U; /* variable length */
	case BTP_CSIS_SET_MEMBER_LOCK:
		return buf_simple->len == 0U;
	case BTP_CSIS_GET_MEMBER_RSI:
		return buf_simple->len == sizeof(struct btp_csis_get_member_rsi_rp);
	case BTP_CSIS_ENC_SIRK_TYPE:
		return buf_simple->len == 0U;

	/* No events */
	default:
		LOG_ERR("Unhandled opcode 0x%02X", hdr->opcode);
		return false;
	}
}

static bool is_valid_mics_packet_len(const struct btp_hdr *hdr, struct net_buf_simple *buf_simple)
{
	switch (hdr->opcode) {
	/* responses */
	case BTP_MICS_READ_SUPPORTED_COMMANDS:
		return buf_simple->len > 0U; /* variable length */
	case BTP_MICS_DEV_MUTE_DISABLE:
		return buf_simple->len == 0U;
	case BTP_MICS_DEV_MUTE_READ:
		return buf_simple->len == 0U;
	case BTP_MICS_DEV_MUTE:
		return buf_simple->len == 0U;
	case BTP_MICS_DEV_UNMUTE:
		return buf_simple->len == 0U;

	/* events */
	case BTP_MICS_MUTE_STATE_EV:
		return buf_simple->len == sizeof(struct btp_mics_mute_state_ev);
	default:
		LOG_ERR("Unhandled opcode 0x%02X", hdr->opcode);
		return false;
	}
}

static bool is_valid_ccp_packet_len(const struct btp_hdr *hdr, struct net_buf_simple *buf_simple)
{
	switch (hdr->opcode) {
	/* responses */
	case BTP_CCP_READ_SUPPORTED_COMMANDS:
		return buf_simple->len > 0U; /* variable length */
	case BTP_CCP_DISCOVER_TBS:
		return buf_simple->len == 0U;
	case BTP_CCP_ACCEPT_CALL:
		return buf_simple->len == 0U;
	case BTP_CCP_TERMINATE_CALL:
		return buf_simple->len == 0U;
	case BTP_CCP_ORIGINATE_CALL:
		return buf_simple->len == 0U;
	case BTP_CCP_READ_CALL_STATE:
		return buf_simple->len == 0U;
	case BTP_CCP_READ_BEARER_NAME:
		return buf_simple->len == 0U;
	case BTP_CCP_READ_BEARER_UCI:
		return buf_simple->len == 0U;
	case BTP_CCP_READ_BEARER_TECH:
		return buf_simple->len == 0U;
	case BTP_CCP_READ_URI_LIST:
		return buf_simple->len == 0U;
	case BTP_CCP_READ_SIGNAL_STRENGTH:
		return buf_simple->len == 0U;
	case BTP_CCP_READ_SIGNAL_INTERVAL:
		return buf_simple->len == 0U;
	case BTP_CCP_READ_CURRENT_CALLS:
		return buf_simple->len == 0U;
	case BTP_CCP_READ_CCID:
		return buf_simple->len == 0U;
	case BTP_CCP_READ_CALL_URI:
		return buf_simple->len == 0U;
	case BTP_CCP_READ_STATUS_FLAGS:
		return buf_simple->len == 0U;
	case BTP_CCP_READ_OPTIONAL_OPCODES:
		return buf_simple->len == 0U;
	case BTP_CCP_READ_FRIENDLY_NAME:
		return buf_simple->len == 0U;
	case BTP_CCP_READ_REMOTE_URI:
		return buf_simple->len == 0U;
	case BTP_CCP_SET_SIGNAL_INTERVAL:
		return buf_simple->len == 0U;
	case BTP_CCP_HOLD_CALL:
		return buf_simple->len == 0U;
	case BTP_CCP_RETRIEVE_CALL:
		return buf_simple->len == 0U;
	case BTP_CCP_JOIN_CALLS:
		return buf_simple->len == 0U;

	/* events */
	case BTP_CCP_EV_DISCOVERED:
		return buf_simple->len == sizeof(struct btp_ccp_discovered_ev);
	case BTP_CCP_EV_CALL_STATES:
		if (hdr->len >= sizeof(struct btp_ccp_call_states_ev)) {
			const struct btp_ccp_call_states_ev *ev = net_buf_simple_pull_mem(
				buf_simple, sizeof(struct btp_ccp_call_states_ev));

			return (ev->call_count * sizeof(struct bt_tbs_client_call_state)) ==
			       buf_simple->len;
		} else {
			return false;
		}
	case BTP_CCP_EV_CHRC_HANDLES:
		return buf_simple->len == sizeof(struct btp_ccp_chrc_handles_ev);
	case BTP_CCP_EV_CHRC_VAL:
		return buf_simple->len == sizeof(struct btp_ccp_chrc_val_ev);
	case BTP_CCP_EV_CHRC_STR:
		if (hdr->len >= sizeof(struct btp_ccp_chrc_str_ev)) {
			const struct btp_ccp_chrc_str_ev *ev = net_buf_simple_pull_mem(
				buf_simple, sizeof(struct btp_ccp_chrc_str_ev));

			return ev->data_len == buf_simple->len;
		} else {
			return false;
		}
	case BTP_CCP_EV_CP:
		return buf_simple->len == sizeof(struct btp_ccp_cp_ev);
	case BTP_CCP_EV_CURRENT_CALLS:
		return buf_simple->len == sizeof(struct btp_ccp_current_calls_ev);
	default:
		LOG_ERR("Unhandled opcode 0x%02X", hdr->opcode);
		return false;
	}
}

static bool is_valid_vcp_packet_len(const struct btp_hdr *hdr, struct net_buf_simple *buf_simple)
{
	switch (hdr->opcode) {
	/* responses */
	case BTP_VCP_READ_SUPPORTED_COMMANDS:
		return buf_simple->len > 0U; /* variable length */
	case BTP_VCP_VOL_CTLR_DISCOVER:
		return buf_simple->len == 0U;
	case BTP_VCP_VOL_CTLR_STATE_READ:
		return buf_simple->len == 0U;
	case BTP_VCP_VOL_CTLR_FLAGS_READ:
		return buf_simple->len == 0U;
	case BTP_VCP_VOL_CTLR_VOL_DOWN:
		return buf_simple->len == 0U;
	case BTP_VCP_VOL_CTLR_VOL_UP:
		return buf_simple->len == 0U;
	case BTP_VCP_VOL_CTLR_UNMUTE_VOL_DOWN:
		return buf_simple->len == 0U;
	case BTP_VCP_VOL_CTLR_UNMUTE_VOL_UP:
		return buf_simple->len == 0U;
	case BTP_VCP_VOL_CTLR_SET_VOL:
		return buf_simple->len == 0U;
	case BTP_VCP_VOL_CTLR_UNMUTE:
		return buf_simple->len == 0U;
	case BTP_VCP_VOL_CTLR_MUTE:
		return buf_simple->len == 0U;

	/* events */
	case BTP_VCP_DISCOVERED_EV:
		return buf_simple->len == sizeof(struct btp_vcp_discovered_ev);
	case BTP_VCP_STATE_EV:
		return buf_simple->len == sizeof(struct btp_vcp_state_ev);
	case BTP_VCP_FLAGS_EV:
		return buf_simple->len == sizeof(struct btp_vcp_volume_flags_ev);
	case BTP_VCP_PROCEDURE_EV:
		return buf_simple->len == sizeof(struct btp_vcp_procedure_ev);
	default:
		LOG_ERR("Unhandled opcode 0x%02X", hdr->opcode);
		return false;
	}
}

static bool is_valid_cas_packet_len(const struct btp_hdr *hdr, struct net_buf_simple *buf_simple)
{
	switch (hdr->opcode) {
	/* responses */
	case BTP_CAS_READ_SUPPORTED_COMMANDS:
		return buf_simple->len > 0U; /* variable length */
	case BTP_CAS_SET_MEMBER_LOCK:
		return buf_simple->len == 0U;
	case BTP_CAS_GET_MEMBER_RSI:
		return buf_simple->len == sizeof(struct btp_cas_get_member_rsi_rp);

	/* No events */
	default:
		LOG_ERR("Unhandled opcode 0x%02X", hdr->opcode);
		return false;
	}
}

static bool is_valid_mcp_packet_len(const struct btp_hdr *hdr, struct net_buf_simple *buf_simple)
{
	switch (hdr->opcode) {
	/* responses */
	case BTP_MCP_READ_SUPPORTED_COMMANDS:
		return buf_simple->len > 0U; /* variable length */
	case BTP_MCP_DISCOVER:
		return buf_simple->len == 0U;
	case BTP_MCP_TRACK_DURATION_READ:
		return buf_simple->len == 0U;
	case BTP_MCP_TRACK_POSITION_READ:
		return buf_simple->len == 0U;
	case BTP_MCP_TRACK_POSITION_SET:
		return buf_simple->len == 0U;
	case BTP_MCP_PLAYBACK_SPEED_READ:
		return buf_simple->len == 0U;
	case BTP_MCP_PLAYBACK_SPEED_SET:
		return buf_simple->len == 0U;
	case BTP_MCP_SEEKING_SPEED_READ:
		return buf_simple->len == 0U;
	case BTP_MCP_ICON_OBJ_ID_READ:
		return buf_simple->len == 0U;
	case BTP_MCP_NEXT_TRACK_OBJ_ID_READ:
		return buf_simple->len == 0U;
	case BTP_MCP_NEXT_TRACK_OBJ_ID_SET:
		return buf_simple->len == 0U;
	case BTP_MCP_PARENT_GROUP_OBJ_ID_READ:
		return buf_simple->len == 0U;
	case BTP_MCP_CURRENT_GROUP_OBJ_ID_READ:
		return buf_simple->len == 0U;
	case BTP_MCP_CURRENT_GROUP_OBJ_ID_SET:
		return buf_simple->len == 0U;
	case BTP_MCP_PLAYING_ORDER_READ:
		return buf_simple->len == 0U;
	case BTP_MCP_PLAYING_ORDER_SET:
		return buf_simple->len == 0U;
	case BTP_MCP_PLAYING_ORDERS_SUPPORTED_READ:
		return buf_simple->len == 0U;
	case BTP_MCP_MEDIA_STATE_READ:
		return buf_simple->len == 0U;
	case BTP_MCP_OPCODES_SUPPORTED_READ:
		return buf_simple->len == 0U;
	case BTP_MCP_CONTENT_CONTROL_ID_READ:
		return buf_simple->len == 0U;
	case BTP_MCP_SEGMENTS_OBJ_ID_READ:
		return buf_simple->len == 0U;
	case BTP_MCP_CURRENT_TRACK_OBJ_ID_READ:
		return buf_simple->len == 0U;
	case BTP_MCP_CURRENT_TRACK_OBJ_ID_SET:
		return buf_simple->len == 0U;
	case BTP_MCP_CMD_SEND:
		return buf_simple->len == 0U;
	case BTP_MCP_CMD_SEARCH:
		return buf_simple->len == 0U;

	/* events */
	case BTP_MCP_DISCOVERED_EV:
		return buf_simple->len == sizeof(struct btp_mcp_discovered_ev);
	case BTP_MCP_TRACK_DURATION_EV:
		return buf_simple->len == sizeof(struct btp_mcp_track_duration_ev);
	case BTP_MCP_TRACK_POSITION_EV:
		return buf_simple->len == sizeof(struct btp_mcp_track_position_ev);
	case BTP_MCP_PLAYBACK_SPEED_EV:
		return buf_simple->len == sizeof(struct btp_mcp_playback_speed_ev);
	case BTP_MCP_SEEKING_SPEED_EV:
		return buf_simple->len == sizeof(struct btp_mcp_seeking_speed_ev);
	case BTP_MCP_ICON_OBJ_ID_EV:
		return buf_simple->len == sizeof(struct btp_mcp_icon_obj_id_ev);
	case BTP_MCP_NEXT_TRACK_OBJ_ID_EV:
		return buf_simple->len == sizeof(struct btp_mcp_next_track_obj_id_ev);
	case BTP_MCP_PARENT_GROUP_OBJ_ID_EV:
		return buf_simple->len == sizeof(struct btp_mcp_parent_group_obj_id_ev);
	case BTP_MCP_CURRENT_GROUP_OBJ_ID_EV:
		return buf_simple->len == sizeof(struct btp_mcp_current_group_obj_id_ev);
	case BTP_MCP_PLAYING_ORDER_EV:
		return buf_simple->len == sizeof(struct btp_mcp_playing_order_ev);
	case BTP_MCP_PLAYING_ORDERS_SUPPORTED_EV:
		return buf_simple->len == sizeof(struct btp_mcp_playing_orders_supported_ev);
	case BTP_MCP_MEDIA_STATE_EV:
		return buf_simple->len == sizeof(struct btp_mcp_media_state_ev);
	case BTP_MCP_OPCODES_SUPPORTED_EV:
		return buf_simple->len == sizeof(struct btp_mcp_opcodes_supported_ev);
	case BTP_MCP_CONTENT_CONTROL_ID_EV:
		return buf_simple->len == sizeof(struct btp_mcp_content_control_id_ev);
	case BTP_MCP_SEGMENTS_OBJ_ID_EV:
		return buf_simple->len == sizeof(struct btp_mcp_segments_obj_id_ev);
	case BTP_MCP_CURRENT_TRACK_OBJ_ID_EV:
		return buf_simple->len == sizeof(struct btp_mcp_current_track_obj_id_ev);
	case BTP_MCP_MEDIA_CP_EV:
		return buf_simple->len == sizeof(struct btp_mcp_media_cp_ev);
	case BTP_MCP_SEARCH_CP_EV:
		return buf_simple->len == sizeof(struct btp_mcp_search_cp_ev);
	case BTP_MCP_NTF_EV:
		return buf_simple->len == sizeof(struct btp_mcp_cmd_ntf_ev);
	case BTP_SCP_NTF_EV:
		return buf_simple->len == sizeof(struct btp_scp_cmd_ntf_ev);
	default:
		LOG_ERR("Unhandled opcode 0x%02X", hdr->opcode);
		return false;
	}
}

static bool is_valid_gmcs_packet_len(const struct btp_hdr *hdr, struct net_buf_simple *buf_simple)
{
	switch (hdr->opcode) {
	/* No responses */
	/* No events */
	default:
		LOG_ERR("Unhandled opcode 0x%02X", hdr->opcode);
		return false;
	}
}

static bool is_valid_hap_packet_len(const struct btp_hdr *hdr, struct net_buf_simple *buf_simple)
{
	switch (hdr->opcode) {
	/* responses */
	case BTP_HAP_READ_SUPPORTED_COMMANDS:
		return buf_simple->len > 0U; /* variable length */
	case BTP_HAP_HA_INIT:
		return buf_simple->len == 0U;
	case BTP_HAP_HARC_INIT:
		return buf_simple->len == 0U;
	case BTP_HAP_HAUC_INIT:
		return buf_simple->len == 0U;
	case BTP_HAP_IAC_INIT:
		return buf_simple->len == 0U;
	case BTP_HAP_IAC_DISCOVER:
		return buf_simple->len == 0U;
	case BTP_HAP_IAC_SET_ALERT:
		return buf_simple->len == 0U;
	case BTP_HAP_HAUC_DISCOVER:
		return buf_simple->len == 0U;

	/* events */
	case BT_HAP_EV_IAC_DISCOVERY_COMPLETE:
		return buf_simple->len == sizeof(struct btp_hap_iac_discovery_complete_ev);
	case BT_HAP_EV_HAUC_DISCOVERY_COMPLETE:
		return buf_simple->len == sizeof(struct btp_hap_hauc_discovery_complete_ev);
	default:
		LOG_ERR("Unhandled opcode 0x%02X", hdr->opcode);
		return false;
	}
}

static bool is_valid_csip_packet_len(const struct btp_hdr *hdr, struct net_buf_simple *buf_simple)
{
	switch (hdr->opcode) {
	/* responses */
	case BTP_CSIP_READ_SUPPORTED_COMMANDS:
		return buf_simple->len > 0U; /* variable length */
	case BTP_CSIP_DISCOVER:
		return buf_simple->len == 0U;
	case BTP_CSIP_START_ORDERED_ACCESS:
		return buf_simple->len == 0U;
	case BTP_CSIP_SET_COORDINATOR_LOCK:
		return buf_simple->len == 0U;
	case BTP_CSIP_SET_COORDINATOR_RELEASE:
		return buf_simple->len == 0U;

	/* events */
	case BTP_CSIP_DISCOVERED_EV:
		return buf_simple->len == sizeof(struct btp_csip_discovered_ev);
	case BTP_CSIP_SIRK_EV:
		return buf_simple->len == sizeof(struct btp_csip_sirk_ev);
	case BTP_CSIP_LOCK_EV:
		return buf_simple->len == sizeof(struct btp_csip_lock_ev);
	default:
		LOG_ERR("Unhandled opcode 0x%02X", hdr->opcode);
		return false;
	}
}

static bool is_valid_cap_packet_len(const struct btp_hdr *hdr, struct net_buf_simple *buf_simple)
{
	switch (hdr->opcode) {
	/* responses */
	case BTP_CAP_READ_SUPPORTED_COMMANDS:
		return buf_simple->len > 0U; /* variable length */
	case BTP_CAP_DISCOVER:
		return buf_simple->len == 0U;
	case BTP_CAP_UNICAST_SETUP_ASE:
		return buf_simple->len == 0U;
	case BTP_CAP_UNICAST_AUDIO_START:
		return buf_simple->len == 0U;
	case BTP_CAP_UNICAST_AUDIO_UPDATE:
		return buf_simple->len == 0U;
	case BTP_CAP_UNICAST_AUDIO_STOP:
		return buf_simple->len == 0U;
	case BTP_CAP_BROADCAST_SOURCE_SETUP_STREAM:
		return buf_simple->len == 0U;
	case BTP_CAP_BROADCAST_SOURCE_SETUP_SUBGROUP:
		return buf_simple->len == 0U;
	case BTP_CAP_BROADCAST_SOURCE_SETUP:
		return buf_simple->len == sizeof(struct btp_cap_broadcast_source_setup_rp);
	case BTP_CAP_BROADCAST_SOURCE_RELEASE:
		return buf_simple->len == 0U;
	case BTP_CAP_BROADCAST_ADV_START:
		return buf_simple->len == 0U;
	case BTP_CAP_BROADCAST_ADV_STOP:
		return buf_simple->len == 0U;
	case BTP_CAP_BROADCAST_SOURCE_START:
		return buf_simple->len == 0U;
	case BTP_CAP_BROADCAST_SOURCE_STOP:
		return buf_simple->len == 0U;
	case BTP_CAP_BROADCAST_SOURCE_UPDATE:
		return buf_simple->len == 0U;

	/* events */
	case BTP_CAP_EV_DISCOVERY_COMPLETED:
		return buf_simple->len == sizeof(struct btp_cap_discovery_completed_ev);
	case BTP_CAP_EV_UNICAST_START_COMPLETED:
		return buf_simple->len == sizeof(struct btp_cap_unicast_start_completed_ev);
	case BTP_CAP_EV_UNICAST_STOP_COMPLETED:
		return buf_simple->len == sizeof(struct btp_cap_unicast_stop_completed_ev);
	default:
		LOG_ERR("Unhandled opcode 0x%02X", hdr->opcode);
		return false;
	}
}

static bool is_valid_tbs_packet_len(const struct btp_hdr *hdr, struct net_buf_simple *buf_simple)
{
	switch (hdr->opcode) {
	/* responses */
	case BTP_TBS_READ_SUPPORTED_COMMANDS:
		return buf_simple->len > 0U; /* variable length */
	case BTP_TBS_REMOTE_INCOMING:
		return buf_simple->len == 0U;
	case BTP_TBS_HOLD:
		return buf_simple->len == 0U;
	case BTP_TBS_SET_BEARER_NAME:
		return buf_simple->len == 0U;
	case BTP_TBS_SET_TECHNOLOGY:
		return buf_simple->len == 0U;
	case BTP_TBS_SET_URI_SCHEME:
		return buf_simple->len == 0U;
	case BTP_TBS_SET_STATUS_FLAGS:
		return buf_simple->len == 0U;
	case BTP_TBS_REMOTE_HOLD:
		return buf_simple->len == 0U;
	case BTP_TBS_ORIGINATE:
		return buf_simple->len == 0U;
	case BTP_TBS_SET_SIGNAL_STRENGTH:
		return buf_simple->len == 0U;

	/* No events */
	default:
		LOG_ERR("Unhandled opcode 0x%02X", hdr->opcode);
		return false;
	}
}

static bool is_valid_tmap_packet_len(const struct btp_hdr *hdr, struct net_buf_simple *buf_simple)
{
	switch (hdr->opcode) {
	/* responses */
	case BTP_TMAP_READ_SUPPORTED_COMMANDS:
		return buf_simple->len > 0U; /* variable length */
	case BTP_TMAP_DISCOVER:
		return buf_simple->len == 0U;

	/* events */
	case BT_TMAP_EV_DISCOVERY_COMPLETE:
		return buf_simple->len == sizeof(struct btp_tmap_discovery_complete_ev);
	default:
		LOG_ERR("Unhandled opcode 0x%02X", hdr->opcode);
		return false;
	}
}

static bool is_valid_ots_packet_len(const struct btp_hdr *hdr, struct net_buf_simple *buf_simple)
{
	switch (hdr->opcode) {
	/* responses */
	case BTP_OTS_READ_SUPPORTED_COMMANDS:
		return buf_simple->len > 0U; /* variable length */
	case BTP_OTS_REGISTER_OBJECT:
		return buf_simple->len == sizeof(struct btp_ots_register_object_rp);

	/* No events */
	default:
		LOG_ERR("Unhandled opcode 0x%02X", hdr->opcode);
		return false;
	}
}

static bool is_valid_pbp_packet_len(const struct btp_hdr *hdr, struct net_buf_simple *buf_simple)
{
	switch (hdr->opcode) {
	/* responses */
	case BTP_PBP_READ_SUPPORTED_COMMANDS:
		return buf_simple->len > 0U; /* variable length */
	case BTP_PBP_SET_PUBLIC_BROADCAST_ANNOUNCEMENT:
		return buf_simple->len == 0U;
	case BTP_PBP_SET_BROADCAST_NAME:
		return buf_simple->len == 0U;
	case BTP_PBP_BROADCAST_SCAN_START:
		return buf_simple->len == 0U;
	case BTP_PBP_BROADCAST_SCAN_STOP:
		return buf_simple->len == 0U;

	/* events */
	case BTP_PBP_EV_PUBLIC_BROADCAST_ANNOUNCEMENT_FOUND:
		return buf_simple->len ==
		       sizeof(struct btp_pbp_ev_public_broadcast_announcement_found_ev);
	default:
		LOG_ERR("Unhandled opcode 0x%02X", hdr->opcode);
		return false;
	}
}

/**
 * Tests whether the incoming packet from @p buf is valid in terms of packet length
 *
 * It verifies responses and events, but not commands
 */
static bool is_valid_packet_len(uint8_t buf[], size_t len)
{
	struct net_buf_simple buf_simple;
	const struct btp_hdr *hdr;

	TEST_ASSERT(buf != NULL);
	LOG_HEXDUMP_DBG(buf, len, "evt");

	/* Make a simple copy of the data so we can pull the data without affecting the original */
	net_buf_simple_init_with_data(&buf_simple, buf, len);
	TEST_ASSERT(buf_simple.len >= sizeof(*hdr));

	hdr = net_buf_simple_pull_mem(&buf_simple, sizeof(*hdr));

	/* For all events it shall be true that after getting the header, the header len field and
	 * the remaining buffer length shall be the same
	 */
	if (hdr->len != buf_simple.len) {
		return false;
	}

	switch (hdr->service) {
	case BTP_SERVICE_ID_CORE:
		return is_valid_core_packet_len(hdr, &buf_simple);
	case BTP_SERVICE_ID_GAP:
		return is_valid_gap_packet_len(hdr, &buf_simple);
	case BTP_SERVICE_ID_GATT:
		return is_valid_gatt_packet_len(hdr, &buf_simple);
	case BTP_SERVICE_ID_L2CAP:
		return is_valid_l2cap_packet_len(hdr, &buf_simple);
	case BTP_SERVICE_ID_MESH:
		return is_valid_mesh_packet_len(hdr, &buf_simple);
	case BTP_SERVICE_ID_MESH_MDL:
		return is_valid_mesh_mdl_packet_len(hdr, &buf_simple);
	case BTP_SERVICE_GATT_CLIENT:
		break; /* Not implemented in Zephyr */
	case BTP_SERVICE_GATT_SERVER:
		break; /* Not implemented in Zephyr */
	case BTP_SERVICE_ID_VCS:
		return is_valid_vcs_packet_len(hdr, &buf_simple);
	case BTP_SERVICE_ID_IAS:
		return is_valid_ias_packet_len(hdr, &buf_simple);
	case BTP_SERVICE_ID_AICS:
		return is_valid_aics_packet_len(hdr, &buf_simple);
	case BTP_SERVICE_ID_VOCS:
		return is_valid_vocs_packet_len(hdr, &buf_simple);
	case BTP_SERVICE_ID_PACS:
		return is_valid_pacs_packet_len(hdr, &buf_simple);
	case BTP_SERVICE_ID_ASCS:
		return is_valid_ascs_packet_len(hdr, &buf_simple);
	case BTP_SERVICE_ID_BAP:
		return is_valid_bap_packet_len(hdr, &buf_simple);
	case BTP_SERVICE_ID_HAS:
		return is_valid_has_packet_len(hdr, &buf_simple);
	case BTP_SERVICE_ID_MICP:
		return is_valid_micp_packet_len(hdr, &buf_simple);
	case BTP_SERVICE_ID_CSIS:
		return is_valid_csis_packet_len(hdr, &buf_simple);
	case BTP_SERVICE_ID_MICS:
		return is_valid_mics_packet_len(hdr, &buf_simple);
	case BTP_SERVICE_ID_CCP:
		return is_valid_ccp_packet_len(hdr, &buf_simple);
	case BTP_SERVICE_ID_VCP:
		return is_valid_vcp_packet_len(hdr, &buf_simple);
	case BTP_SERVICE_ID_CAS:
		return is_valid_cas_packet_len(hdr, &buf_simple);
	case BTP_SERVICE_ID_MCP:
		return is_valid_mcp_packet_len(hdr, &buf_simple);
	case BTP_SERVICE_ID_GMCS:
		return is_valid_gmcs_packet_len(hdr, &buf_simple);
	case BTP_SERVICE_ID_HAP:
		return is_valid_hap_packet_len(hdr, &buf_simple);
	case BTP_SERVICE_ID_CSIP:
		return is_valid_csip_packet_len(hdr, &buf_simple);
	case BTP_SERVICE_ID_CAP:
		return is_valid_cap_packet_len(hdr, &buf_simple);
	case BTP_SERVICE_ID_TBS:
		return is_valid_tbs_packet_len(hdr, &buf_simple);
	case BTP_SERVICE_ID_TMAP:
		return is_valid_tmap_packet_len(hdr, &buf_simple);
	case BTP_SERVICE_ID_OTS:
		return is_valid_ots_packet_len(hdr, &buf_simple);
	case BTP_SERVICE_ID_PBP:
		return is_valid_pbp_packet_len(hdr, &buf_simple);
	}

	LOG_ERR("Unhandled service 0x%02x", hdr->service);
	return false;
}

void bsim_btp_wait_for_evt(uint8_t service, uint8_t opcode, struct net_buf **out_buf)
{
	LOG_DBG("Waiting for evt with service 0x%02X and opcode 0x%02X", service, opcode);

	while (true) {
		const struct btp_hdr *hdr;
		struct net_buf *buf;

		buf = k_fifo_get(&btp_evt_fifo, K_FOREVER);
		TEST_ASSERT(buf != NULL);

		hdr = net_buf_pull_mem(buf, sizeof(struct btp_hdr));

		/* TODO: Verify length of event based on the service and opcode */
		if (hdr->service == service && hdr->opcode == opcode) {
			if (out_buf != NULL) {
				/* If the caller provides an out_buf, they are responsible
				 * for unref'ing it
				 */
				*out_buf = net_buf_ref(buf);
			}

			net_buf_unref(buf);

			LOG_DBG("Got evt with service 0x%02X and opcode 0x%02X", service, opcode);

			return;
		}

		net_buf_unref(buf);
	}
}

static bool recv_cb(uint8_t buf[], size_t buf_len)
{
	struct btp_hdr *hdr = (void *)buf;
	uint16_t len;

	if (buf_len < sizeof(*hdr)) {
		return false;
	}

	len = sys_le16_to_cpu(hdr->len);
	TEST_ASSERT(len <= BTP_MTU - sizeof(*hdr), "Invalid packet length %zu", len);

	if (buf_len < sizeof(*hdr) + len) {
		return false;
	}

	TEST_ASSERT(is_valid_packet_len(buf, buf_len),
		    "Header len %u does not match expected packet length for "
		    "service 0x%02X and opcode 0x%02X",
		    hdr->len, hdr->service, hdr->opcode);

	if (hdr->opcode < BTP_EVENT_OPCODE) {
		struct net_buf *net_buf = net_buf_alloc(&btp_rsp_pool, K_NO_WAIT);

		TEST_ASSERT(buf != NULL);

		net_buf_add_mem(net_buf, buf, buf_len);
		k_fifo_put(&btp_rsp_fifo, net_buf);
	} else {
		struct net_buf *net_buf = net_buf_alloc(&btp_evt_pool, K_NO_WAIT);

		if (net_buf == NULL) {
			/* Discard the oldest event */
			net_buf = k_fifo_get(&btp_evt_fifo, K_NO_WAIT);
			TEST_ASSERT(net_buf != NULL);
			net_buf_unref(net_buf);

			net_buf = net_buf_alloc(&btp_evt_pool, K_NO_WAIT);
			TEST_ASSERT(net_buf != NULL);
		}

		net_buf_add_mem(net_buf, buf, buf_len);
		k_fifo_put(&btp_evt_fifo, net_buf);
	}

	buf = NULL;

	return true;
}

static void timer_expiry_cb(struct k_timer *timer)
{
	static uint8_t uart_recv_buf[BTP_MTU];
	static size_t len;

	while (uart_poll_in(dev, &uart_recv_buf[len]) == 0) {
		TEST_ASSERT(len < ARRAY_SIZE(uart_recv_buf));
		len++;
		if (recv_cb(uart_recv_buf, len)) {
			len = 0;
		}
	}
}

K_TIMER_DEFINE(timer, timer_expiry_cb, NULL);

/* Uart Poll */
void bsim_btp_uart_init(void)
{
	TEST_ASSERT(device_is_ready(dev));

	k_timer_start(&timer, K_MSEC(10), K_MSEC(10));
}

static void wait_for_response(const struct btp_hdr *cmd_hdr)
{
	const struct btp_hdr *rsp_hdr;
	struct net_buf *buf;

	buf = k_fifo_get(&btp_rsp_fifo, K_SECONDS(1));
	TEST_ASSERT(buf != NULL);

	rsp_hdr = (struct btp_hdr *)buf->data;
	TEST_ASSERT(rsp_hdr->len <= BTP_MTU, "len %u > %d", rsp_hdr->len, BTP_MTU);

	LOG_DBG("rsp service 0x%02X and opcode 0x%02X len %u", cmd_hdr->service, cmd_hdr->opcode,
		rsp_hdr->len);

	TEST_ASSERT(rsp_hdr->service == cmd_hdr->service && rsp_hdr->opcode == cmd_hdr->opcode);

	net_buf_unref(buf);
}

/* BTP communication is inspired from `uart_pipe_rx` to achieve a similar API */
void bsim_btp_send_to_tester(const uint8_t *data, size_t len)
{
	const struct btp_hdr *cmd_hdr;

	TEST_ASSERT(data != NULL);
	TEST_ASSERT(len <= BTP_MTU, "len %zu > %d", len, BTP_MTU);
	TEST_ASSERT(len >= sizeof(*cmd_hdr), "len %zu <= %zu", len, sizeof(*cmd_hdr));

	cmd_hdr = (const struct btp_hdr *)data;
	LOG_DBG("cmd service 0x%02X and opcode 0x%02X", cmd_hdr->service, cmd_hdr->opcode);

	for (size_t i = 0U; i < len; i++) {
		uart_poll_out(dev, data[i]);
	}

	wait_for_response(cmd_hdr);
}
