/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/toolchain.h>

#ifdef CONFIG_MODEM_CELLULAR
#include <zephyr/modem/chat.h>
#include <zephyr/drivers/modem/modem_cellular.h>

/* Validate the board-init registration macro expands, links, and binds a
 * board-supplied chat script to a modem instance.
 */
MODEM_CHAT_MATCH_DEFINE(board_init_ok_match, "OK", "", NULL);
MODEM_CHAT_SCRIPT_CMDS_DEFINE(board_init_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT", board_init_ok_match));
MODEM_CHAT_SCRIPT_NO_ABORT_DEFINE(board_init_script, board_init_script_cmds, NULL, 4);
MODEM_CELLULAR_BOARD_INIT_DEFINE(DT_NODELABEL(test_quectel_eg25_g), &board_init_script);
#elif defined(CONFIG_MODEM_HL78XX)
#include <zephyr/drivers/modem/hl78xx_apis.h>
#endif /* CONFIG_MODEM_CELLULAR */

int main(void)
{
#ifdef CONFIG_MODEM_CELLULAR
	/* Validate drivers compiled in */
	__maybe_unused const struct device *swir_hl7800 = DEVICE_DT_GET_ONE(swir_hl7800);
	__maybe_unused const struct device *wnc_m14a2a = DEVICE_DT_GET_ONE(wnc_m14a2a);
	__maybe_unused const struct device *u_blox_sara_r4 = DEVICE_DT_GET_ONE(u_blox_sara_r4);
	__maybe_unused const struct device *simcom_sim7080 = DEVICE_DT_GET_ONE(simcom_sim7080);
	__maybe_unused const struct device *quectel_bg9x = DEVICE_DT_GET_ONE(quectel_bg9x);
	__maybe_unused const struct device *quectel_eg25_g = DEVICE_DT_GET_ONE(quectel_eg25_g);
	__maybe_unused const struct device *telit_me910g1 = DEVICE_DT_GET_ONE(telit_me910g1);
	__maybe_unused const struct device *nordic_nrf91_slm = DEVICE_DT_GET_ONE(nordic_nrf91_slm);
	__maybe_unused const struct device *sqn_gm02s = DEVICE_DT_GET_ONE(sqn_gm02s);
	__maybe_unused const struct device *st_st87mxx = DEVICE_DT_GET_ONE(st_st87mxx);
	__maybe_unused const struct device *trasna_lexi_r10 = DEVICE_DT_GET_ONE(trasna_lexi_r10);

#elif defined(CONFIG_MODEM_HL78XX)
	/** Validate driver compiled in. */

#if DT_HAS_COMPAT_STATUS_OKAY(swir_hl7800)
	__maybe_unused const struct device *swir_hl78xx = DEVICE_DT_GET_ONE(swir_hl7800);
#elif DT_HAS_COMPAT_STATUS_OKAY(swir_hl7802)
	__maybe_unused const struct device *swir_hl78xx = DEVICE_DT_GET_ONE(swir_hl7802);
#elif DT_HAS_COMPAT_STATUS_OKAY(swir_hl7810)
	__maybe_unused const struct device *swir_hl78xx = DEVICE_DT_GET_ONE(swir_hl7810);
#elif DT_HAS_COMPAT_STATUS_OKAY(swir_hl7812)
	__maybe_unused const struct device *swir_hl78xx = DEVICE_DT_GET_ONE(swir_hl7812);
#else
#error "CONFIG_MODEM_HL78XX enabled, but no supported SWIR HL78XX devicetree node has status okay"
#endif

#endif /* CONFIG_MODEM_CELLULAR */
	return 0;
}

#ifdef CONFIG_CONNECTIVITY_WIFI_MGMT_APPLICATION

#include <zephyr/net/conn_mgr_connectivity_impl.h>
#include <zephyr/net/conn_mgr/connectivity_wifi_mgmt.h>

/* Bind L2 connectity APIs. */
static struct conn_mgr_conn_api conn_api = {0};

CONN_MGR_CONN_DEFINE(CONNECTIVITY_WIFI_MGMT, &conn_api);

#endif /* CONFIG_CONNECTIVITY_WIFI_MGMT_APPLICATION */
