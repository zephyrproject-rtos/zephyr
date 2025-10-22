/*
 * Copyright (c) 2017 Erwin Rol <erwin@erwinrol.com>
 * Copyright (c) 2025 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ETHERNET_ETH_STM32_HAL_PRIV_H_
#define ZEPHYR_DRIVERS_ETHERNET_ETH_STM32_HAL_PRIV_H_

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/phy.h>
#include <zephyr/types.h>
#include <soc.h>

#define DT_DRV_COMPAT st_stm32_ethernet

extern const struct device *eth_stm32_phy_dev;

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet)
#define IS_ETH_DMATXDESC_OWN(dma_tx_desc) (dma_tx_desc->DESC3 & ETH_DMATXNDESCRF_OWN)
#define ETH_RXBUFNB	ETH_RX_DESC_CNT
#define ETH_TXBUFNB	ETH_TX_DESC_CNT
/* Only one tx_buffer is sufficient to pass only 1 dma_buffer */
#define ETH_TXBUF_DEF_NB	1U
#else
#define IS_ETH_DMATXDESC_OWN(dma_tx_desc) (dma_tx_desc->Status & ETH_DMATXDESC_OWN)
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet) */

#if defined(CONFIG_ETH_STM32_HAL_USE_DTCM_FOR_DMA_BUFFER) && \
	    DT_NODE_HAS_STATUS_OKAY(DT_CHOSEN(zephyr_dtcm))
#define __eth_stm32_desc __dtcm_noinit_section
#define __eth_stm32_buf  __dtcm_noinit_section
#elif defined(CONFIG_SOC_SERIES_STM32H7X) || defined(CONFIG_SOC_SERIES_STM32H7RSX)
#define __eth_stm32_desc __attribute__((section(".eth_stm32_desc")))
#define __eth_stm32_buf  __attribute__((section(".eth_stm32_buf")))
#elif defined(CONFIG_NOCACHE_MEMORY)
#define __eth_stm32_desc __nocache __aligned(4)
#define __eth_stm32_buf  __nocache __aligned(4)
#else
#define __eth_stm32_desc __aligned(4)
#define __eth_stm32_buf  __aligned(4)
#endif

#if defined(CONFIG_ETH_STM32_HAL_API_V1)

#define ETH_MII_MODE	ETH_MEDIA_INTERFACE_MII
#define ETH_RMII_MODE	ETH_MEDIA_INTERFACE_RMII

#define ETH_STM32_AUTO_NEGOTIATION_ENABLE                                                          \
	UTIL_NOT(DT_NODE_HAS_PROP(DT_INST_PHANDLE(0, phy_handle), fixed_link))

#else /* CONFIG_ETH_STM32_HAL_API_V2 */

#define ETH_MII_MODE	HAL_ETH_MII_MODE
#define ETH_RMII_MODE	HAL_ETH_RMII_MODE

struct eth_stm32_tx_context {
	struct net_pkt *pkt;
	uint16_t first_tx_buffer_index;
	bool used;
};

#endif /* CONFIG_ETH_STM32_HAL_API_V2 */

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32n6_ethernet)

#define ETH_GMII_MODE	HAL_ETH_GMII_MODE
#define ETH_RGMII_MODE	HAL_ETH_RGMII_MODE

#define STM32_ETH_PHY_MODE(inst) \
	((DT_INST_ENUM_HAS_VALUE(inst, phy_connection_type, rgmii) ? ETH_RGMII_MODE : \
	 (DT_INST_ENUM_HAS_VALUE(inst, phy_connection_type, gmii) ? ETH_GMII_MODE : \
	 (DT_INST_ENUM_HAS_VALUE(inst, phy_connection_type, mii) ? ETH_MII_MODE : \
								ETH_RMII_MODE))))
#else
#define STM32_ETH_PHY_MODE(inst) \
	(DT_INST_ENUM_HAS_VALUE(inst, phy_connection_type, mii) ? \
		ETH_MII_MODE : ETH_RMII_MODE)
#endif

/* Definition of the Ethernet driver buffers size and count */
#define ETH_STM32_RX_BUF_SIZE	ETH_MAX_PACKET_SIZE /* buffer size for receive */
#define ETH_STM32_TX_BUF_SIZE	ETH_MAX_PACKET_SIZE /* buffer size for transmit */

BUILD_ASSERT(ETH_STM32_RX_BUF_SIZE % 4 == 0, "Rx buffer size must be a multiple of 4");

extern uint8_t dma_rx_buffer[ETH_RXBUFNB][ETH_STM32_RX_BUF_SIZE];
extern uint8_t dma_tx_buffer[ETH_TXBUFNB][ETH_STM32_TX_BUF_SIZE];

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32n6_ethernet)
extern ETH_DMADescTypeDef dma_rx_desc_tab[ETH_DMA_RX_CH_CNT][ETH_RXBUFNB];
extern ETH_DMADescTypeDef dma_tx_desc_tab[ETH_DMA_TX_CH_CNT][ETH_TXBUFNB];
#else
extern ETH_DMADescTypeDef dma_rx_desc_tab[ETH_RXBUFNB];
extern ETH_DMADescTypeDef dma_tx_desc_tab[ETH_TXBUFNB];
#endif

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
#endif /* CONFIG_PTP_CLOCK_STM32_HAL */
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	struct net_stats_eth stats;
#endif
};

void eth_stm32_setup_mac_filter(ETH_HandleTypeDef *heth);
void eth_stm32_set_mac_config(const struct device *dev, struct phy_link_state *state);
int eth_stm32_tx(const struct device *dev, struct net_pkt *pkt);
struct net_pkt *eth_stm32_rx(const struct device *dev);
int eth_stm32_hal_init(const struct device *dev);
int eth_stm32_hal_start(const struct device *dev);
int eth_stm32_hal_stop(const struct device *dev);
int eth_stm32_hal_set_config(const struct device *dev,
			     enum ethernet_config_type type,
			     const struct ethernet_config *config);
struct net_if *eth_stm32_get_iface(struct eth_stm32_hal_dev_data *ctx);

#if defined(CONFIG_ETH_STM32_MULTICAST_FILTER)
void eth_stm32_mcast_filter(const struct device *dev,
			    const struct ethernet_filter *filter);
#endif /* CONFIG_ETH_STM32_MULTICAST_FILTER */

#ifdef CONFIG_PTP_CLOCK_STM32_HAL
const struct device *eth_stm32_get_ptp_clock(const struct device *dev);
bool eth_stm32_is_ptp_pkt(struct net_if *iface, struct net_pkt *pkt);
#endif /* CONFIG_PTP_CLOCK_STM32_HAL */

#endif /* ZEPHYR_DRIVERS_ETHERNET_ETH_STM32_HAL_PRIV_H_ */
