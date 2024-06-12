/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Header containing network stack interface specific declarations for
 * the Zephyr OS layer of the Wi-Fi driver.
 */

#ifndef __ZEPHYR_NET_IF_H__
#define __ZEPHYR_NET_IF_H__
#include <zephyr/device.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <fmac_structs.h>
#include <zephyr/net/wifi_mgmt.h>

#define UNICAST_MASK GENMASK(7, 1)
#define LOCAL_BIT BIT(1)

void nrf_wifi_if_init_zep(struct net_if *iface);

int nrf_wifi_if_start_zep(const struct device *dev);

int nrf_wifi_if_stop_zep(const struct device *dev);

int nrf_wifi_if_set_config_zep(const struct device *dev,
			       enum ethernet_config_type type,
			       const struct ethernet_config *config);

int nrf_wifi_if_get_config_zep(const struct device *dev,
			       enum ethernet_config_type type,
			       struct ethernet_config *config);

enum ethernet_hw_caps nrf_wifi_if_caps_get(const struct device *dev);

int nrf_wifi_if_send(const struct device *dev,
		     struct net_pkt *pkt);

void nrf_wifi_if_rx_frm(void *os_vif_ctx,
			void *frm);

#if defined(CONFIG_NRF70_RAW_DATA_RX) || defined(CONFIG_NRF70_PROMISC_DATA_RX)
void nrf_wifi_if_sniffer_rx_frm(void *os_vif_ctx,
				void *frm,
				struct raw_rx_pkt_header *raw_rx_hdr,
				bool pkt_free);
#endif /* CONFIG_NRF70_RAW_DATA_RX || CONFIG_NRF70_PROMISC_DATA_RX */

enum nrf_wifi_status nrf_wifi_if_carr_state_chg(void *os_vif_ctx,
						enum nrf_wifi_fmac_if_carr_state carr_state);

int nrf_wifi_stats_get(const struct device *dev,
		       struct net_stats_wifi *stats);

struct net_stats_eth *nrf_wifi_eth_stats_get(const struct device *dev);

void nrf_wifi_set_iface_event_handler(void *os_vif_ctx,
						struct nrf_wifi_umac_event_set_interface *event,
						unsigned int event_len);
#endif /* __ZEPHYR_NET_IF_H__ */
