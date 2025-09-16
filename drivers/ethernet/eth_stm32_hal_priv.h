/*
 * Copyright (c) 2017 Erwin Rol <erwin@erwinrol.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ETHERNET_ETH_STM32_HAL_PRIV_H_
#define ZEPHYR_DRIVERS_ETHERNET_ETH_STM32_HAL_PRIV_H_

#include <zephyr/kernel.h>
#include <zephyr/types.h>

#ifdef CONFIG_PTP_CLOCK_STM32_HAL
const struct device *eth_stm32_get_ptp_clock(const struct device *dev);
bool eth_is_ptp_pkt(struct net_if *iface, struct net_pkt *pkt);
#endif /* CONFIG_PTP_CLOCK_STM32_HAL */

#ifdef CONFIG_ETH_STM32_HAL_API_V2

struct eth_stm32_tx_context {
	struct net_pkt *pkt;
	uint16_t first_tx_buffer_index;
	bool used;
};

#endif /* CONFIG_ETH_STM32_HAL_API_V2 */

#define ST_OUI_B0 0x00
#define ST_OUI_B1 0x80
#define ST_OUI_B2 0xE1

#define ETH_STM32_HAL_MTU NET_ETH_MTU
#define ETH_STM32_HAL_FRAME_SIZE_MAX (ETH_STM32_HAL_MTU + 18)

/* Definition of the Ethernet driver buffers size and count */
#define ETH_STM32_RX_BUF_SIZE	ETH_MAX_PACKET_SIZE /* buffer size for receive */
#define ETH_STM32_TX_BUF_SIZE	ETH_MAX_PACKET_SIZE /* buffer size for transmit */

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

#endif /* ZEPHYR_DRIVERS_ETHERNET_ETH_STM32_HAL_PRIV_H_ */
