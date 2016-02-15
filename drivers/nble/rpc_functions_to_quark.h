/*
 * Copyright (c) 2016 Intel Corporation
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

/* declare the list of functions sorted by signature */
#define LIST_FN_SIG_NONE					\
	FN_SIG_NONE(on_nble_up)

#define LIST_FN_SIG_S						\
	FN_SIG_S(on_nble_get_version_rsp,			\
		 const struct nble_version_response *)		\
	FN_SIG_S(on_nble_gap_connect_evt,			\
		 const struct nble_gap_connect_evt *)		\
	FN_SIG_S(on_nble_gap_disconnect_evt,			\
		 const struct nble_gap_disconnect_evt *)	\
	FN_SIG_S(on_nble_gap_conn_update_evt,			\
		 const struct nble_gap_conn_update_evt *)	\
	FN_SIG_S(on_nble_gap_sm_status_evt,			\
		 const struct nble_gap_sm_status_evt *)		\
	FN_SIG_S(on_nble_gap_sm_passkey_display_evt,		\
		 const struct nble_gap_sm_passkey_disp_evt *)	\
	FN_SIG_S(on_nble_gap_sm_passkey_req_evt,		\
		 const struct nble_gap_sm_passkey_req_evt *)	\
	FN_SIG_S(on_nble_gap_to_evt,				\
		 const struct nble_gap_timout_evt *)		\
	FN_SIG_S(on_nble_gap_rssi_evt,				\
		 const struct nble_gap_rssi_evt *)		\
	FN_SIG_S(on_nble_common_rsp,				\
		 const struct nble_response *)			\
	FN_SIG_S(on_nble_gap_connect_rsp,			\
		 const struct nble_response *)			\
	FN_SIG_S(on_nble_gap_cancel_connect_rsp,		\
		 const struct nble_response *)			\
	FN_SIG_S(on_nble_gap_read_bda_rsp,			\
		 const struct nble_service_read_bda_response *)	\
	FN_SIG_S(on_nble_gap_disconnect_rsp,			\
		 const struct nble_response *)			\
	FN_SIG_S(on_nble_gap_sm_config_rsp,			\
		 struct nble_gap_sm_config_rsp *)		\
	FN_SIG_S(on_nble_gap_generic_cmd_rsp,			\
		 const struct nble_response *)			\
	FN_SIG_S(on_nble_gap_conn_update_rsp,			\
		 const struct nble_response *)			\
	FN_SIG_S(on_nble_gap_sm_common_rsp,			\
		 const struct nble_gap_sm_response *)		\
	FN_SIG_S(on_nble_gap_service_write_rsp,			\
		 const struct nble_service_write_response *)	\
	FN_SIG_S(on_nble_set_enable_config_rsp,			\
		 const struct nble_response *)			\
	FN_SIG_S(on_nble_gap_set_rssi_report_rsp,		\
		 const struct nble_response *)			\
	FN_SIG_S(on_nble_gap_dbg_rsp,				\
		 const struct debug_response *)			\
	FN_SIG_S(on_nble_gatts_send_svc_changed_rsp,		\
		 const struct nble_response *)			\
	FN_SIG_S(on_nble_gatts_send_notif_ind_rsp,		\
		 const struct nble_gatt_notif_ind_rsp *)	\
	FN_SIG_S(on_nble_gap_start_advertise_rsp,		\
		 const struct nble_response *)			\
	FN_SIG_S(on_nble_gap_stop_advertise_rsp,		\
		 const struct nble_response *)			\
	FN_SIG_S(on_nble_gatts_read_evt,			\
		 const struct nble_gatt_rd_evt *)		\
	FN_SIG_S(on_nble_gap_scan_start_stop_rsp,		\
		 const struct nble_response *)

#define LIST_FN_SIG_P						\
	FN_SIG_P(on_nble_gap_dtm_init_rsp, void *)

#define LIST_FN_SIG_S_B						\
	FN_SIG_S_B(nble_log, const struct nble_log_s *,		\
		   char *, uint8_t)				\
	FN_SIG_S_B(on_nble_gattc_value_evt,			\
		   const struct nble_gattc_value_evt *,		\
		   uint8_t *, uint8_t)				\
	FN_SIG_S_B(on_nble_gatts_write_evt,			\
		   const struct nble_gatt_wr_evt *,		\
		   const uint8_t *, uint8_t)			\
	FN_SIG_S_B(on_nble_gatts_get_attribute_value_rsp,	\
		   const struct nble_gatts_attribute_rsp *,	\
		   uint8_t *, uint8_t)				\
	FN_SIG_S_B(on_nble_gatt_register_rsp,			\
		   const struct nble_gatt_register_rsp *,	\
		   const struct nble_gatt_attr_handles *,	\
		   uint8_t)					\
	FN_SIG_S_B(on_nble_gattc_discover_rsp,			\
		   const struct nble_gattc_disc_rsp *,		\
		   const uint8_t *, uint8_t)			\
	FN_SIG_S_B(on_nble_gap_adv_report_evt,			\
		   const struct nble_gap_adv_report_evt *,	\
		   const uint8_t *, uint8_t)			\
	FN_SIG_S_B(on_nble_gap_sm_bond_info_rsp,		\
		   const struct nble_gap_sm_bond_info_rsp *,	\
		   const bt_addr_le_t *, uint16_t)

#define LIST_FN_SIG_B_B_P

#define LIST_FN_SIG_S_P						\
	FN_SIG_S_P(on_nble_gattc_write_rsp,			\
		   const struct nble_gattc_write_rsp *, void *)

#define LIST_FN_SIG_S_B_P					\
	FN_SIG_S_B_P(on_nble_gattc_read_rsp,			\
		     const struct nble_gattc_read_rsp *,	\
		     uint8_t *, uint8_t, void *)

#define LIST_FN_SIG_S_B_B_P
