/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int main(void)
{
	return 0;
}

#ifdef CONFIG_CONNECTIVITY_WIFI_MGMT_APPLICATION

#include <zephyr/net/conn_mgr_connectivity_impl.h>
#include <zephyr/net/conn_mgr/connectivity_wifi_mgmt.h>

/* Bind L2 connectity APIs. */
static struct conn_mgr_conn_api conn_api = { 0 };

CONN_MGR_CONN_DEFINE(CONNECTIVITY_WIFI_MGMT, &conn_api);

#endif /* CONFIG_CONNECTIVITY_WIFI_MGMT_APPLICATION */
