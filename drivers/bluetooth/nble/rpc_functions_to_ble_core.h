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
#define LIST_FN_SIG_NONE						\
	FN_SIG_NONE(nble_gap_start_adv_req)				\
	FN_SIG_NONE(nble_gap_stop_scan_req)

#define LIST_FN_SIG_S							\
	FN_SIG_S(nble_gap_set_adv_data_req,				\
		 struct nble_gap_ad_data_params *)			\
	FN_SIG_S(nble_gap_set_adv_params_req,				\
		 struct nble_gap_adv_params *)				\
	FN_SIG_S(nble_gap_start_scan_req,				\
		 const struct nble_gap_scan_params *)			\
	FN_SIG_S(nble_gap_sm_config_req,				\
		 const struct nble_gap_sm_config_params *)		\
	FN_SIG_S(nble_gap_sm_passkey_reply_req,				\
		 const struct nble_gap_sm_key_reply_req_params *)	\
	FN_SIG_S(nble_gap_sm_bond_info_req,				\
		 const struct nble_gap_sm_bond_info_param *)		\
	FN_SIG_S(nble_gap_sm_security_req,				\
		 const struct nble_gap_sm_security_params *)		\
	FN_SIG_S(nble_gap_sm_clear_bonds_req,				\
		 const struct nble_gap_sm_clear_bond_req_params *)	\
	FN_SIG_S(nble_set_bda_req, const struct nble_set_bda_params *)	\
	FN_SIG_S(nble_gap_conn_update_req,				\
		 const struct nble_gap_connect_update_params *)		\
	FN_SIG_S(nble_gattc_discover_req,				\
		 const struct nble_discover_params *)			\
	FN_SIG_S(nble_gatts_wr_reply_req,				\
		 const struct nble_gatts_wr_reply_params *)		\
	FN_SIG_S(nble_uas_rssi_calibrate_req,				\
		 const struct nble_uas_rssi_calibrate *)		\
	FN_SIG_S(nble_gap_service_write_req,				\
		 const struct nble_gap_service_write_params *)		\
	FN_SIG_S(nble_gap_disconnect_req,				\
		 const struct nble_gap_disconnect_req_params *)		\
	FN_SIG_S(nble_gattc_read_req,					\
		 const struct nble_gattc_read_params *)			\
	FN_SIG_S(nble_gap_tx_power_req,					\
		 const struct nble_gap_tx_power_params *)

#define LIST_FN_SIG_P							\
	FN_SIG_P(nble_get_version_req, void *)				\
	FN_SIG_P(nble_gap_dtm_init_req, void *)				\
	FN_SIG_P(nble_gap_read_bda_req, void *)				\
	FN_SIG_P(nble_gap_stop_adv_req, void *)				\
	FN_SIG_P(nble_gap_cancel_connect_req, void *)

#define LIST_FN_SIG_S_B							\
	FN_SIG_S_B(nble_gatt_register_req,				\
		   const struct nble_gatt_register_req *,		\
		   uint8_t *, uint16_t)					\
	FN_SIG_S_B(nble_gatt_send_notif_req,				\
		   const struct nble_gatt_send_notif_params *,		\
		   const uint8_t *, uint16_t)				\
	FN_SIG_S_B(nble_gatt_send_ind_req,				\
		   const struct nble_gatt_send_ind_params *,		\
		   const uint8_t *, uint8_t)				\
	FN_SIG_S_B(nble_gatts_rd_reply_req,				\
		   const struct nble_gatts_rd_reply_params *,		\
		   uint8_t *, uint16_t)					\
	FN_SIG_S_B(nble_gattc_write_req,				\
		   const struct nble_gattc_write_params *,		\
		   const uint8_t *, uint8_t)				\
	FN_SIG_S_B(nble_gattc_read_multiple_req,			\
		   const struct nble_gattc_read_multiple_params *,	\
		   const uint16_t *, uint16_t)

#define LIST_FN_SIG_B_B_P

#define LIST_FN_SIG_S_P							\
	FN_SIG_S_P(nble_gap_connect_req,				\
		   const struct nble_gap_connect_req_params *, void *)	\
	FN_SIG_S_P(nble_gap_set_rssi_report_req,			\
		   const struct nble_rssi_report_params *, void *)	\
	FN_SIG_S_P(nble_gap_dbg_req, const struct nble_debug_params *,	\
		   void *)

#define LIST_FN_SIG_S_B_P

#define LIST_FN_SIG_S_B_B_P
