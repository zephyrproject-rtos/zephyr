/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/direction.h>
#include <zephyr/net_buf.h>

/* Performs initialization of Direction Finding in Host */
int le_df_init(void);

int hci_df_prepare_connectionless_iq_report(struct net_buf *buf,
					    struct bt_df_per_adv_sync_iq_samples_report *report,
					    struct bt_le_per_adv_sync **per_adv_sync_to_report);
int hci_df_vs_prepare_connectionless_iq_report(struct net_buf *buf,
					       struct bt_df_per_adv_sync_iq_samples_report *report,
					       struct bt_le_per_adv_sync **per_adv_sync_to_report);
int hci_df_prepare_connection_iq_report(struct net_buf *buf,
					struct bt_df_conn_iq_samples_report *report,
					struct bt_conn **conn_to_report);
int hci_df_vs_prepare_connection_iq_report(struct net_buf *buf,
					   struct bt_df_conn_iq_samples_report *report,
					   struct bt_conn **conn_to_report);
int hci_df_prepare_conn_cte_req_failed(struct net_buf *buf,
				       struct bt_df_conn_iq_samples_report *report,
				       struct bt_conn **conn_to_report);
