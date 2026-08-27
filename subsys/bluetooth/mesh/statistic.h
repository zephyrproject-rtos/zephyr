/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_MESH_STATISTIC_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_MESH_STATISTIC_H_

void bt_mesh_stat_planned_count(struct bt_mesh_adv_ctx *ctx);
void bt_mesh_stat_succeeded_count(struct bt_mesh_adv_ctx *ctx);
void bt_mesh_stat_rx(enum bt_mesh_net_if net_if);

#if defined(CONFIG_BT_MESH_LOW_POWER)
void bt_mesh_stat_lpn_timing_update_poll_tx(void);
void bt_mesh_stat_lpn_timing_update_scan_start(void);
void bt_mesh_stat_lpn_timing_update_response_rx(void);
void bt_mesh_stat_lpn_timing_update_win_expired(void);
#endif

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_MESH_STATISTIC_H_ */
