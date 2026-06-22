/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/toolchain.h>

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
#endif /* CONFIG_MODEM_CELLULAR */
	return 0;
}

#ifdef CONFIG_CONNECTIVITY_WIFI_MGMT_APPLICATION

#include <zephyr/net/conn_mgr_connectivity_impl.h>
#include <zephyr/net/conn_mgr/connectivity_wifi_mgmt.h>

/* Bind L2 connectity APIs. */
static struct conn_mgr_conn_api conn_api = { 0 };

CONN_MGR_CONN_DEFINE(CONNECTIVITY_WIFI_MGMT, &conn_api);

#endif /* CONFIG_CONNECTIVITY_WIFI_MGMT_APPLICATION */
