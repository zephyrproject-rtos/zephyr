/*
 * Copyright (c) 2017 Erwin Rol <erwin@erwinrol.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ETHERNET_ETH_STM32_HAL_PRIV_H_
#define ZEPHYR_DRIVERS_ETHERNET_ETH_STM32_HAL_PRIV_H_

#include <zephyr/kernel.h>
#include <zephyr/types.h>

/* Naming of the  ETH PTP Config Status changes depending on the stm32 serie */
#if defined(CONFIG_SOC_SERIES_STM32F4X)
#define ETH_STM32_PTP_CONFIGURED HAL_ETH_PTP_CONFIGURATED
#define ETH_STM32_PTP_NOT_CONFIGURED HAL_ETH_PTP_NOT_CONFIGURATED
#else
#define ETH_STM32_PTP_CONFIGURED HAL_ETH_PTP_CONFIGURED
#define ETH_STM32_PTP_NOT_CONFIGURED HAL_ETH_PTP_NOT_CONFIGURED
#endif /* stm32F7x or sm32F4x */

#define ST_OUI_B0 0x00
#define ST_OUI_B1 0x80
#define ST_OUI_B2 0xE1

#define ETH_STM32_HAL_MTU NET_ETH_MTU
#define ETH_STM32_HAL_FRAME_SIZE_MAX (ETH_STM32_HAL_MTU + 18)

/* Definition of the Ethernet driver buffers size and count */
#define ETH_STM32_RX_BUF_SIZE	ETH_MAX_PACKET_SIZE /* buffer size for receive */
#define ETH_STM32_TX_BUF_SIZE	ETH_MAX_PACKET_SIZE /* buffer size for transmit */

#if defined(CONFIG_ETH_STM32_HAL_USE_DTCM_FOR_DMA_BUFFER) && \
	    DT_NODE_HAS_STATUS_OKAY(DT_CHOSEN(zephyr_dtcm))
#define __eth_stm32_desc __dtcm_noinit_section
#define __eth_stm32_buf  __dtcm_noinit_section
#elif defined(CONFIG_SOC_SERIES_STM32H7X)
#define __eth_stm32_desc __attribute__((section(".eth_stm32_desc")))
#define __eth_stm32_buf  __attribute__((section(".eth_stm32_buf")))
#elif defined(CONFIG_NOCACHE_MEMORY)
#define __eth_stm32_desc __nocache __aligned(4)
#define __eth_stm32_buf  __nocache __aligned(4)
#else
#define __eth_stm32_desc __aligned(4)
#define __eth_stm32_buf  __aligned(4)
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet)
#define IS_ETH_DMATXDESC_OWN(dma_tx_desc)	(dma_tx_desc->DESC3 & \
							ETH_DMATXNDESCRF_OWN)

#define ETH_RXBUFNB	ETH_RX_DESC_CNT
#define ETH_TXBUFNB	ETH_TX_DESC_CNT

/* Only one tx_buffer is sufficient to pass only 1 dma_buffer */
#define ETH_TXBUF_DEF_NB	1U
#else

#define IS_ETH_DMATXDESC_OWN(dma_tx_desc)	(dma_tx_desc->Status & \
							ETH_DMATXDESC_OWN)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet) */

/* Device constant configuration parameters */
struct eth_stm32_hal_dev_cfg {
	void (*config_func)(void);
	struct stm32_pclken pclken;
	struct stm32_pclken pclken_rx;
	struct stm32_pclken pclken_tx;
#if DT_INST_CLOCKS_HAS_NAME(0, mac_clk_ptp)
	struct stm32_pclken pclken_ptp;
#endif
	const struct pinctrl_dev_config *pcfg;
};

/* Device run time data */
struct eth_stm32_hal_dev_data {
	struct net_if *iface;
	uint8_t mac_addr[6];
	ETH_HandleTypeDef heth;
	struct k_mutex tx_mutex;
	struct k_sem rx_int_sem;
#if defined(CONFIG_ETH_STM32_HAL_API_V2)
	struct k_sem tx_int_sem;
#endif /* CONFIG_ETH_STM32_HAL_API_V2 */
	K_KERNEL_STACK_MEMBER(rx_thread_stack,
		CONFIG_ETH_STM32_HAL_RX_THREAD_STACK_SIZE);
	struct k_thread rx_thread;
#if defined(CONFIG_ETH_STM32_MULTICAST_FILTER)
	uint8_t hash_index_cnt[64];
#endif /* CONFIG_ETH_STM32_MULTICAST_FILTER */
#if defined(CONFIG_PTP_CLOCK_STM32_HAL)
	const struct device *ptp_clock;
	float clk_ratio;
	float clk_ratio_adj;
#endif /* CONFIG_PTP_CLOCK_STM32_HAL */
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	struct net_stats_eth stats;
#endif
};

const struct device *eth_stm32_hal_get_phy(const struct device *dev);
struct net_if *get_iface(struct eth_stm32_hal_dev_data *ctx);

#if defined(CONFIG_ETH_STM32_HAL_API_V2)
int eth_tx_v2(const struct device *dev, struct net_pkt *pkt);
struct net_pkt *eth_rx_v2(const struct device *dev);
void setup_mac_filter_v2(ETH_HandleTypeDef *heth);
void set_mac_config_v2(const struct device *dev, struct phy_link_state *state);
int eth_init_api_v2(const struct device *dev);
#else
int eth_tx_v1(const struct device *dev, struct net_pkt *pkt);
struct net_pkt *eth_rx_v1(const struct device *dev);
void setup_mac_filter_v1(ETH_HandleTypeDef *heth);
void set_mac_config_v1(const struct device *dev, struct phy_link_state *state);
int eth_init_api_v1(const struct device *dev);
#endif /* CONFIG_ETH_STM32_HAL_API_V2 */


#endif /* ZEPHYR_DRIVERS_ETHERNET_ETH_STM32_HAL_PRIV_H_ */
