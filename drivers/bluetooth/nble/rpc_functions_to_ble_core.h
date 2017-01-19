/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* declare the list of functions sorted by signature */
#define LIST_FN_SIG_NONE						\
	FN_SIG_NONE(nble_gap_start_adv_req)				\
	FN_SIG_NONE(nble_gap_stop_scan_req)				\
	FN_SIG_NONE(nble_panic_req)

#define LIST_FN_SIG_S							\
	FN_SIG_S(nble_gap_set_adv_data_req,				\
		 struct nble_gap_set_adv_data_req *)			\
	FN_SIG_S(nble_gap_set_adv_params_req,				\
		 struct nble_gap_set_adv_params_req *)			\
	FN_SIG_S(nble_gap_start_scan_req,				\
		 const struct nble_gap_start_scan_req *)		\
	FN_SIG_S(nble_sm_passkey_reply_req,				\
		 const struct nble_sm_passkey_reply_req *)		\
	FN_SIG_S(nble_sm_bond_info_req,					\
		 const struct nble_sm_bond_info_req *)			\
	FN_SIG_S(nble_sm_security_req,					\
		 const struct nble_sm_security_req *)			\
	FN_SIG_S(nble_sm_clear_bonds_req,				\
		 const struct nble_sm_clear_bonds_req *)		\
	FN_SIG_S(nble_set_bda_req, const struct nble_set_bda_req *)	\
	FN_SIG_S(nble_get_bda_req, const struct nble_get_bda_req *)	\
	FN_SIG_S(nble_gap_conn_update_req,				\
		 const struct nble_gap_conn_update_req *)		\
	FN_SIG_S(nble_gattc_discover_req,				\
		 const struct nble_gattc_discover_req *)		\
	FN_SIG_S(nble_uas_rssi_calibrate_req,				\
		 const struct nble_uas_rssi_calibrate_req *)		\
	FN_SIG_S(nble_gap_service_req,					\
		 const struct nble_gap_service_req *)			\
	FN_SIG_S(nble_gap_disconnect_req,				\
		 const struct nble_gap_disconnect_req *)		\
	FN_SIG_S(nble_gattc_read_req,					\
		 const struct nble_gattc_read_req *)			\
	FN_SIG_S(nble_gap_set_tx_power_req,				\
		 const struct nble_gap_set_tx_power_req *)		\
	FN_SIG_S(nble_dbg_req, const struct nble_dbg_req *)		\
	FN_SIG_S(nble_sm_pairing_response_req,				\
		 const struct nble_sm_pairing_response_req *)		\
	FN_SIG_S(nble_sm_error_req,					\
		 const struct nble_sm_error_req *)


#define LIST_FN_SIG_P							\
	FN_SIG_P(nble_gap_dtm_init_req, void *)				\
	FN_SIG_P(nble_gap_stop_adv_req, void *)				\
	FN_SIG_P(nble_get_version_req, ble_get_version_cb_t)		\
	FN_SIG_P(nble_gap_cancel_connect_req, void *)

#define LIST_FN_SIG_S_B							\
	FN_SIG_S_B(nble_gatts_register_req,				\
		   const struct nble_gatts_register_req *,		\
		   uint8_t *, uint16_t)					\
	FN_SIG_S_B(nble_gatts_notify_req,				\
		   const struct nble_gatts_notify_req *,		\
		   const uint8_t *, uint16_t)				\
	FN_SIG_S_B(nble_gatts_indicate_req,				\
		   const struct nble_gatts_indicate_req *,		\
		   const uint8_t *, uint8_t)				\
	FN_SIG_S_B(nble_gatts_read_reply_req,				\
		   const struct nble_gatts_read_reply_req *,		\
		   uint8_t *, uint16_t)					\
	FN_SIG_S_B(nble_gattc_write_req,				\
		   const struct nble_gattc_write_req *,			\
		   const uint8_t *, uint16_t)				\
	FN_SIG_S_B(nble_gattc_read_multi_req,				\
		   const struct nble_gattc_read_multi_req *,		\
		   const uint16_t *, uint16_t)				\
	FN_SIG_S_B(nble_uart_test_req,					\
		   const struct nble_uart_test_req *,			\
		   const uint8_t *, uint8_t)				\
	FN_SIG_S_B(nble_gatts_write_reply_req,				\
		   const struct nble_gatts_write_reply_req *,		\
		   const uint8_t *, uint8_t)

#define LIST_FN_SIG_B_B_P

#define LIST_FN_SIG_S_P							\
	FN_SIG_S_P(nble_gap_connect_req,				\
		   const struct nble_gap_connect_req *, void *)		\
	FN_SIG_S_P(nble_gap_set_rssi_report_req,			\
		   const struct nble_gap_set_rssi_report_req *, void *)

#define LIST_FN_SIG_S_B_P

#define LIST_FN_SIG_S_B_B_P
