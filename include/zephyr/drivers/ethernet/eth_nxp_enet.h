/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for NXP ENET Ethernet driver internal definitions.
 * @ingroup ethernet
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ETH_NXP_ENET_H__
#define ZEPHYR_INCLUDE_DRIVERS_ETH_NXP_ENET_H__

/*
 * This header is for NXP ENET driver development
 * and has definitions for internal implementations
 * not to be used by application
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>

#include <fsl_enet.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

struct nxp_enet_mac_data {
	DEVICE_MMIO_RAM;
	ENET_Type *base;
	struct net_if *iface;
	uint8_t mac_addr[6];
	enet_handle_t enet_handle;
	struct k_sem tx_buf_sem;
	struct k_work rx_work;
	struct k_work_delayable rx_resume_work;
	const struct device *dev;
	struct k_sem rx_thread_sem;
	struct k_mutex rx_frame_buf_mutex;
#ifdef CONFIG_MDIO_NXP_ENET
	/* Given by the MAC ISR on MDIO transfer completion */
	struct k_sem mdio_done;
#endif
#ifdef CONFIG_PTP_CLOCK_NXP_ENET
	struct k_sem ptp_ts_sem;
	struct k_mutex ptp_mutex;
#endif
	uint8_t *tx_frame_buf;
	uint8_t *rx_frame_buf;
};

/** INTERNAL_HIDDEN @endcond */

#ifdef __cplusplus
}
#endif


#endif /* ZEPHYR_INCLUDE_DRIVERS_ETH_NXP_ENET_H__ */
