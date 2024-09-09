/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* @file
 * @brief NRF Wi-Fi util shell module
 */
#include <stdlib.h>
#include "host_rpu_umac_if.h"
#include "fmac_api.h"
#include "fmac_util.h"
#include "fmac_main.h"
#include "wifi_util.h"

extern struct nrf_wifi_drv_priv_zep rpu_drv_priv_zep;
struct nrf_wifi_ctx_zep *ctx = &rpu_drv_priv_zep.rpu_ctx_zep;

static bool check_valid_data_rate(const struct shell *sh,
				  unsigned char rate_flag,
				  unsigned int data_rate)
{
	bool ret = false;

	switch (rate_flag) {
	case RPU_TPUT_MODE_LEGACY:
		if ((data_rate == 1) ||
		    (data_rate == 2) ||
		    (data_rate == 55) ||
		    (data_rate == 11) ||
		    (data_rate == 6) ||
		    (data_rate == 9) ||
		    (data_rate == 12) ||
		    (data_rate == 18) ||
		    (data_rate == 24) ||
		    (data_rate == 36) ||
		    (data_rate == 48) ||
		    (data_rate == 54)) {
			ret = true;
		}
		break;
	case RPU_TPUT_MODE_HT:
	case RPU_TPUT_MODE_HE_SU:
	case RPU_TPUT_MODE_VHT:
		if ((data_rate >= 0) && (data_rate <= 7)) {
			ret = true;
		}
		break;
	case RPU_TPUT_MODE_HE_ER_SU:
		if (data_rate >= 0 && data_rate <= 2) {
			ret = true;
		}
		break;
	default:
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "%s: Invalid rate_flag %d\n",
			      __func__,
			      rate_flag);
		break;
	}

	return ret;
}


int nrf_wifi_util_conf_init(struct rpu_conf_params *conf_params)
{
	if (!conf_params) {
		return -ENOEXEC;
	}

	memset(conf_params, 0, sizeof(*conf_params));

	/* Initialize values which are other than 0 */
	conf_params->he_ltf = -1;
	conf_params->he_gi = -1;
	return 0;
}


static int nrf_wifi_util_set_he_ltf(const struct shell *sh,
				    size_t argc,
				    const char *argv[])
{
	char *ptr = NULL;
	unsigned long he_ltf = 0;

	if (ctx->conf_params.set_he_ltf_gi) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Disable 'set_he_ltf_gi', to set 'he_ltf'\n");
		return -ENOEXEC;
	}

	he_ltf = strtoul(argv[1], &ptr, 10);

	if (he_ltf > 2) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Invalid HE LTF value(%lu).\n",
			      he_ltf);
		shell_help(sh);
		return -ENOEXEC;
	}

	ctx->conf_params.he_ltf = he_ltf;

	return 0;
}


static int nrf_wifi_util_set_he_gi(const struct shell *sh,
				   size_t argc,
				   const char *argv[])
{
	char *ptr = NULL;
	unsigned long he_gi = 0;

	if (ctx->conf_params.set_he_ltf_gi) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Disable 'set_he_ltf_gi', to set 'he_gi'\n");
		return -ENOEXEC;
	}

	he_gi = strtoul(argv[1], &ptr, 10);

	if (he_gi > 2) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Invalid HE GI value(%lu).\n",
			      he_gi);
		shell_help(sh);
		return -ENOEXEC;
	}

	ctx->conf_params.he_gi = he_gi;

	return 0;
}


static int nrf_wifi_util_set_he_ltf_gi(const struct shell *sh,
				       size_t argc,
				       const char *argv[])
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	char *ptr = NULL;
	unsigned long val = 0;

	val = strtoul(argv[1], &ptr, 10);

	if ((val < 0) || (val > 1)) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Invalid value(%lu).\n",
			      val);
		shell_help(sh);
		return -ENOEXEC;
	}

	status = nrf_wifi_fmac_conf_ltf_gi(ctx->rpu_ctx,
					   ctx->conf_params.he_ltf,
					   ctx->conf_params.he_gi,
					   val);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Programming ltf_gi failed\n");
		return -ENOEXEC;
	}

	ctx->conf_params.set_he_ltf_gi = val;

	return 0;
}

#ifdef CONFIG_NRF70_STA_MODE
static int nrf_wifi_util_set_uapsd_queue(const struct shell *sh,
					 size_t argc,
					 const char *argv[])
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	char *ptr = NULL;
	unsigned long val = 0;

	val = strtoul(argv[1], &ptr, 10);

	if ((val < UAPSD_Q_MIN) || (val > UAPSD_Q_MAX)) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Invalid value(%lu).\n",
			      val);
		shell_help(sh);
		return -ENOEXEC;
	}

	if (ctx->conf_params.uapsd_queue != val) {
		status = nrf_wifi_fmac_set_uapsd_queue(ctx->rpu_ctx,
						       0,
						       val);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			shell_fprintf(sh,
				      SHELL_ERROR,
				      "Programming uapsd_queue failed\n");
			return -ENOEXEC;
		}

		ctx->conf_params.uapsd_queue = val;
	}

	return 0;
}
#endif /* CONFIG_NRF70_STA_MODE */


static int nrf_wifi_util_show_cfg(const struct shell *sh,
				  size_t argc,
				  const char *argv[])
{
	struct rpu_conf_params *conf_params = NULL;

	conf_params = &ctx->conf_params;

	shell_fprintf(sh,
		      SHELL_INFO,
		      "************* Configured Parameters ***********\n");
	shell_fprintf(sh,
		      SHELL_INFO,
		      "\n");

	shell_fprintf(sh,
		      SHELL_INFO,
		      "he_ltf = %d\n",
		      conf_params->he_ltf);

	shell_fprintf(sh,
		      SHELL_INFO,
		      "he_gi = %u\n",
		      conf_params->he_gi);

	shell_fprintf(sh,
		      SHELL_INFO,
		      "set_he_ltf_gi = %d\n",
		      conf_params->set_he_ltf_gi);

	shell_fprintf(sh,
		      SHELL_INFO,
		      "uapsd_queue = %d\n",
		      conf_params->uapsd_queue);

	shell_fprintf(sh,
		      SHELL_INFO,
		      "rate_flag = %d,  rate_val = %d\n",
		      ctx->conf_params.tx_pkt_tput_mode,
		      ctx->conf_params.tx_pkt_rate);
	return 0;
}

#ifdef CONFIG_NRF70_STA_MODE
static int nrf_wifi_util_tx_stats(const struct shell *sh,
				  size_t argc,
				  const char *argv[])
{
	int vif_index = -1;
	int peer_index = 0;
	int max_vif_index = MAX(MAX_NUM_APS, MAX_NUM_STAS);
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	void *queue = NULL;
	unsigned int tx_pending_pkts = 0;
	struct nrf_wifi_fmac_dev_ctx_def *def_dev_ctx = NULL;

	vif_index = atoi(argv[1]);
	if ((vif_index < 0) || (vif_index >= max_vif_index)) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Invalid vif index(%d).\n",
			      vif_index);
		shell_help(sh);
		return -ENOEXEC;
	}

	fmac_dev_ctx = ctx->rpu_ctx;
	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	/* TODO: Get peer_index from shell once AP mode is supported */
	shell_fprintf(sh,
		SHELL_INFO,
		"************* Tx Stats: vif(%d) peer(0) ***********\n",
		vif_index);

	for (int i = 0; i < NRF_WIFI_FMAC_AC_MAX ; i++) {
		queue = def_dev_ctx->tx_config.data_pending_txq[peer_index][i];
		tx_pending_pkts = nrf_wifi_utils_q_len(queue);

		shell_fprintf(
			sh,
			SHELL_INFO,
			"Outstanding tokens: ac: %d -> %d (pending_q_len: %d)\n",
			i,
			def_dev_ctx->tx_config.outstanding_descs[i],
			tx_pending_pkts);
	}

	return 0;
}
#endif /* CONFIG_NRF70_STA_MODE */


static int nrf_wifi_util_tx_rate(const struct shell *sh,
				 size_t argc,
				 const char *argv[])
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	char *ptr = NULL;
	long rate_flag = -1;
	long data_rate = -1;

	rate_flag = strtol(argv[1], &ptr, 10);

	if (rate_flag >= RPU_TPUT_MODE_MAX) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Invalid value %ld for rate_flags\n",
			      rate_flag);
		shell_help(sh);
		return -ENOEXEC;
	}


	if (rate_flag == RPU_TPUT_MODE_HE_TB) {
		data_rate = -1;
	} else {
		if (argc < 3) {
			shell_fprintf(sh,
				      SHELL_ERROR,
				      "rate_val needed for rate_flag = %ld\n",
				      rate_flag);
			shell_help(sh);
			return -ENOEXEC;
		}

		data_rate = strtol(argv[2], &ptr, 10);

		if (!(check_valid_data_rate(sh,
					    rate_flag,
					    data_rate))) {
			shell_fprintf(sh,
				      SHELL_ERROR,
				      "Invalid data_rate %ld for rate_flag %ld\n",
				      data_rate,
				      rate_flag);
			return -ENOEXEC;
		}

	}

	status = nrf_wifi_fmac_set_tx_rate(ctx->rpu_ctx,
					   rate_flag,
					   data_rate);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Programming tx_rate failed\n");
		return -ENOEXEC;
	}

	ctx->conf_params.tx_pkt_tput_mode = rate_flag;
	ctx->conf_params.tx_pkt_rate = data_rate;

	return 0;
}


#ifdef CONFIG_NRF_WIFI_LOW_POWER
static int nrf_wifi_util_show_host_rpu_ps_ctrl_state(const struct shell *sh,
						     size_t argc,
						     const char *argv[])
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	int rpu_ps_state = -1;

	status = nrf_wifi_fmac_get_host_rpu_ps_ctrl_state(ctx->rpu_ctx,
							  &rpu_ps_state);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Failed to get PS state\n");
		return -ENOEXEC;
	}

	shell_fprintf(sh,
		      SHELL_INFO,
		      "RPU sleep status = %s\n", rpu_ps_state ? "AWAKE" : "SLEEP");
	return 0;
}
#endif /* CONFIG_NRF_WIFI_LOW_POWER */


static int nrf_wifi_util_show_vers(const struct shell *sh,
				  size_t argc,
				  const char *argv[])
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	unsigned int fw_ver;

	fmac_dev_ctx = ctx->rpu_ctx;

	shell_fprintf(sh, SHELL_INFO, "Driver version: %s\n",
				  NRF70_DRIVER_VERSION);

	status = nrf_wifi_fmac_ver_get(fmac_dev_ctx, &fw_ver);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		shell_fprintf(sh,
		 SHELL_INFO,
		 "Failed to get firmware version\n");
		return -ENOEXEC;
	}

	shell_fprintf(sh, SHELL_INFO,
		 "Firmware version: %d.%d.%d.%d\n",
		  NRF_WIFI_UMAC_VER(fw_ver),
		  NRF_WIFI_UMAC_VER_MAJ(fw_ver),
		  NRF_WIFI_UMAC_VER_MIN(fw_ver),
		  NRF_WIFI_UMAC_VER_EXTRA(fw_ver));

	return status;
}

#if !defined(CONFIG_NRF70_RADIO_TEST) && !defined(CONFIG_NRF70_OFFLOADED_RAW_TX)
static int nrf_wifi_util_dump_rpu_stats(const struct shell *sh,
					size_t argc,
					const char *argv[])
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct rpu_op_stats stats;
	enum rpu_stats_type stats_type = RPU_STATS_TYPE_ALL;

	if (argc == 2) {
		const char *type  = argv[1];

		if (!strcmp(type, "umac")) {
			stats_type = RPU_STATS_TYPE_UMAC;
		} else if (!strcmp(type, "lmac")) {
			stats_type = RPU_STATS_TYPE_LMAC;
		} else if (!strcmp(type, "phy")) {
			stats_type = RPU_STATS_TYPE_PHY;
		} else if (!strcmp(type, "all")) {
			stats_type = RPU_STATS_TYPE_ALL;
		} else {
			shell_fprintf(sh,
				      SHELL_ERROR,
				      "Invalid stats type %s\n",
				      type);
			return -ENOEXEC;
		}
	}

	fmac_dev_ctx = ctx->rpu_ctx;

	memset(&stats, 0, sizeof(struct rpu_op_stats));
	status = nrf_wifi_fmac_stats_get(fmac_dev_ctx, 0, &stats);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Failed to get stats\n");
		return -ENOEXEC;
	}

	if (stats_type == RPU_STATS_TYPE_UMAC || stats_type == RPU_STATS_TYPE_ALL) {
		struct rpu_umac_stats *umac = &stats.fw.umac;

		shell_fprintf(sh, SHELL_INFO,
				  "UMAC TX debug stats:\n"
				  "======================\n"
				  "tx_cmd: %u\n"
				  "tx_non_coalesce_pkts_rcvd_from_host: %u\n"
				  "tx_coalesce_pkts_rcvd_from_host: %u\n"
				  "tx_max_coalesce_pkts_rcvd_from_host: %u\n"
				  "tx_cmds_max_used: %u\n"
				  "tx_cmds_currently_in_use: %u\n"
				  "tx_done_events_send_to_host: %u\n"
				  "tx_done_success_pkts_to_host: %u\n"
				  "tx_done_failure_pkts_to_host: %u\n"
				  "tx_cmds_with_crypto_pkts_rcvd_from_host: %u\n"
				  "tx_cmds_with_non_crypto_pkts_rcvd_from_host: %u\n"
				  "tx_cmds_with_broadcast_pkts_rcvd_from_host: %u\n"
				  "tx_cmds_with_multicast_pkts_rcvd_from_host: %u\n"
				  "tx_cmds_with_unicast_pkts_rcvd_from_host: %u\n"
				  "xmit: %u\n"
				  "send_addba_req: %u\n"
				  "addba_resp: %u\n"
				  "softmac_tx: %u\n"
				  "internal_pkts: %u\n"
				  "external_pkts: %u\n"
				  "tx_cmds_to_lmac: %u\n"
				  "tx_dones_from_lmac: %u\n"
				  "total_cmds_to_lmac: %u\n"
				  "tx_packet_data_count: %u\n"
				  "tx_packet_mgmt_count: %u\n"
				  "tx_packet_beacon_count: %u\n"
				  "tx_packet_probe_req_count: %u\n"
				  "tx_packet_auth_count: %u\n"
				  "tx_packet_deauth_count: %u\n"
				  "tx_packet_assoc_req_count: %u\n"
				  "tx_packet_disassoc_count: %u\n"
				  "tx_packet_action_count: %u\n"
				  "tx_packet_other_mgmt_count: %u\n"
				  "tx_packet_non_mgmt_data_count: %u\n\n",
				  umac->tx_dbg_params.tx_cmd,
				  umac->tx_dbg_params.tx_non_coalesce_pkts_rcvd_from_host,
				  umac->tx_dbg_params.tx_coalesce_pkts_rcvd_from_host,
				  umac->tx_dbg_params.tx_max_coalesce_pkts_rcvd_from_host,
				  umac->tx_dbg_params.tx_cmds_max_used,
				  umac->tx_dbg_params.tx_cmds_currently_in_use,
				  umac->tx_dbg_params.tx_done_events_send_to_host,
				  umac->tx_dbg_params.tx_done_success_pkts_to_host,
				  umac->tx_dbg_params.tx_done_failure_pkts_to_host,
				  umac->tx_dbg_params.tx_cmds_with_crypto_pkts_rcvd_from_host,
				  umac->tx_dbg_params.tx_cmds_with_non_crypto_pkts_rcvd_from_host,
				  umac->tx_dbg_params.tx_cmds_with_broadcast_pkts_rcvd_from_host,
				  umac->tx_dbg_params.tx_cmds_with_multicast_pkts_rcvd_from_host,
				  umac->tx_dbg_params.tx_cmds_with_unicast_pkts_rcvd_from_host,
				  umac->tx_dbg_params.xmit,
				  umac->tx_dbg_params.send_addba_req,
				  umac->tx_dbg_params.addba_resp,
				  umac->tx_dbg_params.softmac_tx,
				  umac->tx_dbg_params.internal_pkts,
				  umac->tx_dbg_params.external_pkts,
				  umac->tx_dbg_params.tx_cmds_to_lmac,
				  umac->tx_dbg_params.tx_dones_from_lmac,
				  umac->tx_dbg_params.total_cmds_to_lmac,
				  umac->tx_dbg_params.tx_packet_data_count,
				  umac->tx_dbg_params.tx_packet_mgmt_count,
				  umac->tx_dbg_params.tx_packet_beacon_count,
				  umac->tx_dbg_params.tx_packet_probe_req_count,
				  umac->tx_dbg_params.tx_packet_auth_count,
				  umac->tx_dbg_params.tx_packet_deauth_count,
				  umac->tx_dbg_params.tx_packet_assoc_req_count,
				  umac->tx_dbg_params.tx_packet_disassoc_count,
				  umac->tx_dbg_params.tx_packet_action_count,
				  umac->tx_dbg_params.tx_packet_other_mgmt_count,
				  umac->tx_dbg_params.tx_packet_non_mgmt_data_count);

		shell_fprintf(sh, SHELL_INFO,
				  "UMAC RX debug stats\n"
				  "======================\n"
				  "lmac_events: %u\n"
				  "rx_events: %u\n"
				  "rx_coalesce_events: %u\n"
				  "total_rx_pkts_from_lmac: %u\n"
				  "max_refill_gap: %u\n"
				  "current_refill_gap: %u\n"
				  "out_of_order_mpdus: %u\n"
				  "reorder_free_mpdus: %u\n"
				  "umac_consumed_pkts: %u\n"
				  "host_consumed_pkts: %u\n"
				  "rx_mbox_post: %u\n"
				  "rx_mbox_receive: %u\n"
				  "reordering_ampdu: %u\n"
				  "timer_mbox_post: %u\n"
				  "timer_mbox_rcv: %u\n"
				  "work_mbox_post: %u\n"
				  "work_mbox_rcv: %u\n"
				  "tasklet_mbox_post: %u\n"
				  "tasklet_mbox_rcv: %u\n"
				  "userspace_offload_frames: %u\n"
				  "alloc_buf_fail: %u\n"
				  "rx_packet_total_count: %u\n"
				  "rx_packet_data_count: %u\n"
				  "rx_packet_qos_data_count: %u\n"
				  "rx_packet_protected_data_count: %u\n"
				  "rx_packet_mgmt_count: %u\n"
				  "rx_packet_beacon_count: %u\n"
				  "rx_packet_probe_resp_count: %u\n"
				  "rx_packet_auth_count: %u\n"
				  "rx_packet_deauth_count: %u\n"
				  "rx_packet_assoc_resp_count: %u\n"
				  "rx_packet_disassoc_count: %u\n"
				  "rx_packet_action_count: %u\n"
				  "rx_packet_probe_req_count: %u\n"
				  "rx_packet_other_mgmt_count: %u\n"
				  "max_coalesce_pkts: %d\n"
				  "null_skb_pointer_from_lmac: %u\n"
				  "unexpected_mgmt_pkt: %u\n\n",
				  umac->rx_dbg_params.lmac_events,
				  umac->rx_dbg_params.rx_events,
				  umac->rx_dbg_params.rx_coalesce_events,
				  umac->rx_dbg_params.total_rx_pkts_from_lmac,
				  umac->rx_dbg_params.max_refill_gap,
				  umac->rx_dbg_params.current_refill_gap,
				  umac->rx_dbg_params.out_of_order_mpdus,
				  umac->rx_dbg_params.reorder_free_mpdus,
				  umac->rx_dbg_params.umac_consumed_pkts,
				  umac->rx_dbg_params.host_consumed_pkts,
				  umac->rx_dbg_params.rx_mbox_post,
				  umac->rx_dbg_params.rx_mbox_receive,
				  umac->rx_dbg_params.reordering_ampdu,
				  umac->rx_dbg_params.timer_mbox_post,
				  umac->rx_dbg_params.timer_mbox_rcv,
				  umac->rx_dbg_params.work_mbox_post,
				  umac->rx_dbg_params.work_mbox_rcv,
				  umac->rx_dbg_params.tasklet_mbox_post,
				  umac->rx_dbg_params.tasklet_mbox_rcv,
				  umac->rx_dbg_params.userspace_offload_frames,
				  umac->rx_dbg_params.alloc_buf_fail,
				  umac->rx_dbg_params.rx_packet_total_count,
				  umac->rx_dbg_params.rx_packet_data_count,
				  umac->rx_dbg_params.rx_packet_qos_data_count,
				  umac->rx_dbg_params.rx_packet_protected_data_count,
				  umac->rx_dbg_params.rx_packet_mgmt_count,
				  umac->rx_dbg_params.rx_packet_beacon_count,
				  umac->rx_dbg_params.rx_packet_probe_resp_count,
				  umac->rx_dbg_params.rx_packet_auth_count,
				  umac->rx_dbg_params.rx_packet_deauth_count,
				  umac->rx_dbg_params.rx_packet_assoc_resp_count,
				  umac->rx_dbg_params.rx_packet_disassoc_count,
				  umac->rx_dbg_params.rx_packet_action_count,
				  umac->rx_dbg_params.rx_packet_probe_req_count,
				  umac->rx_dbg_params.rx_packet_other_mgmt_count,
				  umac->rx_dbg_params.max_coalesce_pkts,
				  umac->rx_dbg_params.null_skb_pointer_from_lmac,
				  umac->rx_dbg_params.unexpected_mgmt_pkt);

		shell_fprintf(sh, SHELL_INFO,
				  "UMAC control path stats\n"
				  "======================\n"
				  "cmd_init: %u\n"
				  "event_init_done: %u\n"
				  "cmd_rf_test: %u\n"
				  "cmd_connect: %u\n"
				  "cmd_get_stats: %u\n"
				  "event_ps_state: %u\n"
				  "cmd_set_reg: %u\n"
				  "cmd_get_reg: %u\n"
				  "cmd_req_set_reg: %u\n"
				  "cmd_trigger_scan: %u\n"
				  "event_scan_done: %u\n"
				  "cmd_get_scan: %u\n"
				  "umac_scan_req: %u\n"
				  "umac_scan_complete: %u\n"
				  "umac_scan_busy: %u\n"
				  "cmd_auth: %u\n"
				  "cmd_assoc: %u\n"
				  "cmd_deauth: %u\n"
				  "cmd_register_frame: %u\n"
				  "cmd_frame: %u\n"
				  "cmd_del_key: %u\n"
				  "cmd_new_key: %u\n"
				  "cmd_set_key: %u\n"
				  "cmd_get_key: %u\n"
				  "event_beacon_hint: %u\n"
				  "event_reg_change: %u\n"
				  "event_wiphy_reg_change: %u\n"
				  "cmd_set_station: %u\n"
				  "cmd_new_station: %u\n"
				  "cmd_del_station: %u\n"
				  "cmd_new_interface: %u\n"
				  "cmd_set_interface: %u\n"
				  "cmd_get_interface: %u\n"
				  "cmd_set_ifflags: %u\n"
				  "cmd_set_ifflags_done: %u\n"
				  "cmd_set_bss: %u\n"
				  "cmd_set_wiphy: %u\n"
				  "cmd_start_ap: %u\n"
				  "LMAC_CMD_PS: %u\n"
				  "CURR_STATE: %u\n\n",
				  umac->cmd_evnt_dbg_params.cmd_init,
				  umac->cmd_evnt_dbg_params.event_init_done,
				  umac->cmd_evnt_dbg_params.cmd_rf_test,
				  umac->cmd_evnt_dbg_params.cmd_connect,
				  umac->cmd_evnt_dbg_params.cmd_get_stats,
				  umac->cmd_evnt_dbg_params.event_ps_state,
				  umac->cmd_evnt_dbg_params.cmd_set_reg,
				  umac->cmd_evnt_dbg_params.cmd_get_reg,
				  umac->cmd_evnt_dbg_params.cmd_req_set_reg,
				  umac->cmd_evnt_dbg_params.cmd_trigger_scan,
				  umac->cmd_evnt_dbg_params.event_scan_done,
				  umac->cmd_evnt_dbg_params.cmd_get_scan,
				  umac->cmd_evnt_dbg_params.umac_scan_req,
				  umac->cmd_evnt_dbg_params.umac_scan_complete,
				  umac->cmd_evnt_dbg_params.umac_scan_busy,
				  umac->cmd_evnt_dbg_params.cmd_auth,
				  umac->cmd_evnt_dbg_params.cmd_assoc,
				  umac->cmd_evnt_dbg_params.cmd_deauth,
				  umac->cmd_evnt_dbg_params.cmd_register_frame,
				  umac->cmd_evnt_dbg_params.cmd_frame,
				  umac->cmd_evnt_dbg_params.cmd_del_key,
				  umac->cmd_evnt_dbg_params.cmd_new_key,
				  umac->cmd_evnt_dbg_params.cmd_set_key,
				  umac->cmd_evnt_dbg_params.cmd_get_key,
				  umac->cmd_evnt_dbg_params.event_beacon_hint,
				  umac->cmd_evnt_dbg_params.event_reg_change,
				  umac->cmd_evnt_dbg_params.event_wiphy_reg_change,
				  umac->cmd_evnt_dbg_params.cmd_set_station,
				  umac->cmd_evnt_dbg_params.cmd_new_station,
				  umac->cmd_evnt_dbg_params.cmd_del_station,
				  umac->cmd_evnt_dbg_params.cmd_new_interface,
				  umac->cmd_evnt_dbg_params.cmd_set_interface,
				  umac->cmd_evnt_dbg_params.cmd_get_interface,
				  umac->cmd_evnt_dbg_params.cmd_set_ifflags,
				  umac->cmd_evnt_dbg_params.cmd_set_ifflags_done,
				  umac->cmd_evnt_dbg_params.cmd_set_bss,
				  umac->cmd_evnt_dbg_params.cmd_set_wiphy,
				  umac->cmd_evnt_dbg_params.cmd_start_ap,
				  umac->cmd_evnt_dbg_params.LMAC_CMD_PS,
				  umac->cmd_evnt_dbg_params.CURR_STATE);

			shell_fprintf(sh, SHELL_INFO,
				  "UMAC interface stats\n"
				  "======================\n"
				  "tx_unicast_pkt_count: %u\n"
				  "tx_multicast_pkt_count: %u\n"
				  "tx_broadcast_pkt_count: %u\n"
				  "tx_bytes: %u\n"
				  "rx_unicast_pkt_count: %u\n"
				  "rx_multicast_pkt_count: %u\n"
				  "rx_broadcast_pkt_count: %u\n"
				  "rx_beacon_success_count: %u\n"
				  "rx_beacon_miss_count: %u\n"
				  "rx_bytes: %u\n"
				  "rx_checksum_error_count: %u\n\n"
				  "replay_attack_drop_cnt: %u\n\n",
				  umac->interface_data_stats.tx_unicast_pkt_count,
				  umac->interface_data_stats.tx_multicast_pkt_count,
				  umac->interface_data_stats.tx_broadcast_pkt_count,
				  umac->interface_data_stats.tx_bytes,
				  umac->interface_data_stats.rx_unicast_pkt_count,
				  umac->interface_data_stats.rx_multicast_pkt_count,
				  umac->interface_data_stats.rx_broadcast_pkt_count,
				  umac->interface_data_stats.rx_beacon_success_count,
				  umac->interface_data_stats.rx_beacon_miss_count,
				  umac->interface_data_stats.rx_bytes,
				  umac->interface_data_stats.rx_checksum_error_count,
				  umac->interface_data_stats.replay_attack_drop_cnt);
	}

	if (stats_type == RPU_STATS_TYPE_LMAC || stats_type == RPU_STATS_TYPE_ALL) {
		struct rpu_lmac_stats *lmac = &stats.fw.lmac;

		shell_fprintf(sh, SHELL_INFO,
			      "LMAC stats\n"
				  "======================\n"
				  "reset_cmd_cnt: %u\n"
				  "reset_complete_event_cnt: %u\n"
				  "unable_gen_event: %u\n"
				  "ch_prog_cmd_cnt: %u\n"
				  "channel_prog_done: %u\n"
				  "tx_pkt_cnt: %u\n"
				  "tx_pkt_done_cnt: %u\n"
				  "scan_pkt_cnt: %u\n"
				  "internal_pkt_cnt: %u\n"
				  "internal_pkt_done_cnt: %u\n"
				  "ack_resp_cnt: %u\n"
				  "tx_timeout: %u\n"
				  "deagg_isr: %u\n"
				  "deagg_inptr_desc_empty: %u\n"
				  "deagg_circular_buffer_full: %u\n"
				  "lmac_rxisr_cnt: %u\n"
				  "rx_decryptcnt: %u\n"
				  "process_decrypt_fail: %u\n"
				  "prepa_rx_event_fail: %u\n"
				  "rx_core_pool_full_cnt: %u\n"
				  "rx_mpdu_crc_success_cnt: %u\n"
				  "rx_mpdu_crc_fail_cnt: %u\n"
				  "rx_ofdm_crc_success_cnt: %u\n"
				  "rx_ofdm_crc_fail_cnt: %u\n"
				  "rxDSSSCrcSuccessCnt: %u\n"
				  "rxDSSSCrcFailCnt: %u\n"
				  "rx_crypto_start_cnt: %u\n"
				  "rx_crypto_done_cnt: %u\n"
				  "rx_event_buf_full: %u\n"
				  "rx_extram_buf_full: %u\n"
				  "scan_req: %u\n"
				  "scan_complete: %u\n"
				  "scan_abort_req: %u\n"
				  "scan_abort_complete: %u\n"
				  "internal_buf_pool_null: %u\n"
				  "rpu_hw_lockup_count: %u\n"
				  "rpu_hw_lockup_recovery_done: %u\n\n",
				  lmac->reset_cmd_cnt,
				  lmac->reset_complete_event_cnt,
				  lmac->unable_gen_event,
				  lmac->ch_prog_cmd_cnt,
				  lmac->channel_prog_done,
				  lmac->tx_pkt_cnt,
				  lmac->tx_pkt_done_cnt,
				  lmac->scan_pkt_cnt,
				  lmac->internal_pkt_cnt,
				  lmac->internal_pkt_done_cnt,
				  lmac->ack_resp_cnt,
				  lmac->tx_timeout,
				  lmac->deagg_isr,
				  lmac->deagg_inptr_desc_empty,
				  lmac->deagg_circular_buffer_full,
				  lmac->lmac_rxisr_cnt,
				  lmac->rx_decryptcnt,
				  lmac->process_decrypt_fail,
				  lmac->prepa_rx_event_fail,
				  lmac->rx_core_pool_full_cnt,
				  lmac->rx_mpdu_crc_success_cnt,
				  lmac->rx_mpdu_crc_fail_cnt,
				  lmac->rx_ofdm_crc_success_cnt,
				  lmac->rx_ofdm_crc_fail_cnt,
				  lmac->rxDSSSCrcSuccessCnt,
				  lmac->rxDSSSCrcFailCnt,
				  lmac->rx_crypto_start_cnt,
				  lmac->rx_crypto_done_cnt,
				  lmac->rx_event_buf_full,
				  lmac->rx_extram_buf_full,
				  lmac->scan_req,
				  lmac->scan_complete,
				  lmac->scan_abort_req,
				  lmac->scan_abort_complete,
				  lmac->internal_buf_pool_null,
				  lmac->rpu_hw_lockup_count,
				  lmac->rpu_hw_lockup_recovery_done);
	}

	if (stats_type == RPU_STATS_TYPE_PHY || stats_type == RPU_STATS_TYPE_ALL) {
		struct rpu_phy_stats *phy = &stats.fw.phy;

		shell_fprintf(sh, SHELL_INFO,
			      "PHY stats\n"
				  "======================\n"
				  "rssi_avg: %d\n"
				  "pdout_val: %u\n"
				  "ofdm_crc32_pass_cnt: %u\n"
				  "ofdm_crc32_fail_cnt: %u\n"
				  "dsss_crc32_pass_cnt: %u\n"
				  "dsss_crc32_fail_cnt: %u\n\n",
				  phy->rssi_avg,
				  phy->pdout_val,
				  phy->ofdm_crc32_pass_cnt,
				  phy->ofdm_crc32_fail_cnt,
				  phy->dsss_crc32_pass_cnt,
				  phy->dsss_crc32_fail_cnt);
	}

	return 0;
}
#endif /* !CONFIG_NRF70_RADIO_TEST && !CONFIG_NRF70_OFFLOADED_RAW_TX */

#ifdef CONFIG_NRF_WIFI_RPU_RECOVERY
static int nrf_wifi_util_trigger_rpu_recovery(const struct shell *sh,
					      size_t argc,
					      const char *argv[])
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	if (!ctx || !ctx->rpu_ctx) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "RPU context not initialized\n");
		return -ENOEXEC;
	}

	fmac_dev_ctx = ctx->rpu_ctx;

	status = nrf_wifi_fmac_rpu_recovery_callback(fmac_dev_ctx, NULL, 0);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		shell_fprintf(sh,
			      SHELL_ERROR,
			      "Failed to trigger RPU recovery\n");
		return -ENOEXEC;
	}

	shell_fprintf(sh,
		      SHELL_INFO,
		      "RPU recovery triggered\n");

	return 0;
}
#endif /* CONFIG_NRF_WIFI_RPU_RECOVERY */

SHELL_STATIC_SUBCMD_SET_CREATE(
	nrf_wifi_util_subcmds,
	SHELL_CMD_ARG(he_ltf,
		      NULL,
		      "0 - 1x HE LTF\n"
		      "1 - 2x HE LTF\n"
		      "2 - 4x HE LTF                                        ",
		      nrf_wifi_util_set_he_ltf,
		      2,
		      0),
	SHELL_CMD_ARG(he_gi,
		      NULL,
		      "0 - 0.8 us\n"
		      "1 - 1.6 us\n"
		      "2 - 3.2 us                                           ",
		      nrf_wifi_util_set_he_gi,
		      2,
		      0),
	SHELL_CMD_ARG(set_he_ltf_gi,
		      NULL,
		      "0 - Disable\n"
		      "1 - Enable",
		      nrf_wifi_util_set_he_ltf_gi,
		      2,
		      0),
#ifdef CONFIG_NRF70_STA_MODE
	SHELL_CMD_ARG(uapsd_queue,
		      NULL,
		      "<val> - 0 to 15",
		      nrf_wifi_util_set_uapsd_queue,
		      2,
		      0),
#endif /* CONFIG_NRF70_STA_MODE */
	SHELL_CMD_ARG(show_config,
		      NULL,
		      "Display the current configuration values",
		      nrf_wifi_util_show_cfg,
		      1,
		      0),
#ifdef CONFIG_NRF70_STA_MODE
	SHELL_CMD_ARG(tx_stats,
		      NULL,
		      "Displays transmit statistics\n"
			  "vif_index: 0 - 1\n",
		      nrf_wifi_util_tx_stats,
		      2,
		      0),
#endif /* CONFIG_NRF70_STA_MODE */
	SHELL_CMD_ARG(tx_rate,
		      NULL,
		      "Sets TX data rate to either a fixed value or AUTO\n"
		      "Parameters:\n"
		      "    <rate_flag> : The TX data rate type to be set, where:\n"
		      "        0 - LEGACY\n"
		      "        1 - HT\n"
		      "        2 - VHT\n"
		      "        3 - HE_SU\n"
		      "        4 - HE_ER_SU\n"
		      "        5 - AUTO\n"
		      "    <rate_val> : The TX data rate value to be set, valid values are:\n"
		      "        Legacy : <1, 2, 55, 11, 6, 9, 12, 18, 24, 36, 48, 54>\n"
		      "        Non-legacy: <MCS index value between 0 - 7>\n"
		      "        AUTO: <No value needed>\n",
		      nrf_wifi_util_tx_rate,
		      2,
		      1),
#ifdef CONFIG_NRF_WIFI_LOW_POWER
	SHELL_CMD_ARG(sleep_state,
		      NULL,
		      "Display current sleep status",
		      nrf_wifi_util_show_host_rpu_ps_ctrl_state,
		      1,
		      0),
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
	SHELL_CMD_ARG(show_vers,
		      NULL,
		      "Display the driver and the firmware versions",
		      nrf_wifi_util_show_vers,
		      1,
		      0),
#if !defined(CONFIG_NRF70_RADIO_TEST) && !defined(CONFIG_NRF70_OFFLOADED_RAW_TX)
	SHELL_CMD_ARG(rpu_stats,
		      NULL,
		      "Display RPU stats "
		      "Parameters: umac or lmac or phy or all (default)",
		      nrf_wifi_util_dump_rpu_stats,
		      1,
		      1),
#endif /* !CONFIG_NRF70_RADIO_TEST && !CONFIG_NRF70_OFFLOADED_RAW_TX*/
#ifdef CONFIG_NRF_WIFI_RPU_RECOVERY
	SHELL_CMD_ARG(rpu_recovery_test,
		      NULL,
		      "Trigger RPU recovery",
		      nrf_wifi_util_trigger_rpu_recovery,
		      1,
		      0),
#endif /* CONFIG_NRF_WIFI_RPU_RECOVERY */
	SHELL_SUBCMD_SET_END);


SHELL_CMD_REGISTER(wifi_util,
		   &nrf_wifi_util_subcmds,
		   "nRF Wi-Fi utility shell commands",
		   NULL);


static int nrf_wifi_util_init(void)
{

	if (nrf_wifi_util_conf_init(&ctx->conf_params) < 0) {
		return -1;
	}

	return 0;
}


SYS_INIT(nrf_wifi_util_init,
	 APPLICATION,
	 CONFIG_APPLICATION_INIT_PRIORITY);
