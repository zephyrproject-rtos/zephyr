/** @file
 *  @brief Bluetooth Mesh statistic APIs.
 */

/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_MESH_STATISTIC_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_MESH_STATISTIC_H_

#include <stdint.h>

/**
 * @brief Statistic
 * @defgroup bt_mesh_stat Statistic
 * @ingroup bt_mesh
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** The structure that keeps statistics of mesh frames handling. */
struct bt_mesh_statistic {
	/** All received frames passed basic validation and decryption. */
	/** Received frames over advertiser. */
	uint32_t rx_adv;
	/** Received frames over loopback. */
	uint32_t rx_loopback;
	/** Received frames over proxy. */
	uint32_t rx_proxy;
	/** Received over unknown interface. */
	uint32_t rx_uknown;
	/** Counter of frames that were initiated to relay over advertiser bearer. */
	uint32_t tx_adv_relay_planned;
	/** Counter of frames that succeeded relaying over advertiser bearer. */
	uint32_t tx_adv_relay_succeeded;
	/** Counter of frames that were initiated to send over advertiser bearer locally. */
	uint32_t tx_local_planned;
	/** Counter of frames that succeeded to send over advertiser bearer locally. */
	uint32_t tx_local_succeeded;
	/** Counter of frames that were initiated to send over friend bearer. */
	uint32_t tx_friend_planned;
	/** Counter of frames that succeeded to send over friend bearer. */
	uint32_t tx_friend_succeeded;
};

/** @brief Get mesh frame handling statistic.
 *
 *  @param st   Bluetooth Mesh statistic.
 */
void bt_mesh_stat_get(struct bt_mesh_statistic *st);

/** @brief Reset mesh frame handling statistic.
 */
void bt_mesh_stat_reset(void);

/** Measured LPN friendship timing derived from timestamps.
 *
 *  Values are computed by timestamping protocol events on the LPN side:
 *  - T1: Poll TX complete
 *  - T2: Scanner enabled (ReceiveDelay elapsed)
 *  - T3: Friend response received
 *
 *  Measured ReceiveDelay = T2 - T1 (actual time before LPN starts listening).
 *  Measured ReceiveWindow = T3 - T2 (actual listening time until response).
 */
struct bt_mesh_lpn_timing {
	/** Measured ReceiveDelay for the last poll cycle in microseconds.
	 *  Time from poll TX end to scanner enable.
	 */
	uint32_t recv_delay_us;
	/** Minimum measured ReceiveDelay in microseconds. */
	uint32_t recv_delay_min_us;
	/** Maximum measured ReceiveDelay in microseconds. */
	uint32_t recv_delay_max_us;
	/** Measured ReceiveWindow for the last poll cycle in microseconds.
	 *  Time from scanner enable to scanner disable after Friend response RX.
	 *  Retains the last successful measurement; not updated on timeout.
	 */
	uint32_t recv_win_us;
	/** Minimum measured ReceiveWindow in microseconds. */
	uint32_t recv_win_min_us;
	/** Maximum measured ReceiveWindow in microseconds. */
	uint32_t recv_win_max_us;
	/** Expected ReceiveWindow in microseconds. Time from scanner enable
	 *  to scanner disable when no Friend response arrived (full window).
	 *  Zero if no timeout has occurred yet.
	 */
	uint32_t recv_win_expected_us;
	/** Total number of completed poll-response cycles. */
	uint32_t cnt;
	/** Total number of failed poll-response cycles (no response within window). */
	uint32_t cnt_failed;
};

/** @brief Get measured LPN friendship poll timing.
 *
 *  Returns timestamp-derived measurements of actual ReceiveDelay and
 *  ReceiveWindow observed on-air. Timestamps are captured using
 *  k_uptime_get() at poll TX completion, scanner enable, and response RX.
 *
 *  @param timing  Pointer to structure to populate.
 *
 *  @retval 0        Success.
 *  @retval -ENOTSUP LPN or statistic feature not enabled.
 *  @retval -EINVAL  NULL pointer.
 */
int bt_mesh_stat_lpn_timing_get(struct bt_mesh_lpn_timing *timing);

/** @brief Reset measured LPN friendship poll timing statistics.
 */
void bt_mesh_stat_lpn_timing_reset(void);

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_MESH_STATISTIC_H_ */
