/*
 * Copyright (c) 2017 Erwin Rol <erwin@erwinrol.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ETHERNET_ETH_STM32_HAL_PRIV_H_
#define ZEPHYR_DRIVERS_ETHERNET_ETH_STM32_HAL_PRIV_H_

#include <kernel.h>
#include <zephyr/types.h>

#define ST_OUI_B0 0x00
#define ST_OUI_B1 0x80
#define ST_OUI_B2 0xE1

#define ETH_STM32_HAL_MTU NET_ETH_MTU
#define ETH_STM32_HAL_FRAME_SIZE_MAX (ETH_STM32_HAL_MTU + 18)

/* Definition of the Ethernet driver buffers size and count */
#define ETH_RX_BUF_SIZE	ETH_MAX_PACKET_SIZE /* buffer size for receive */
#define ETH_TX_BUF_SIZE	ETH_MAX_PACKET_SIZE /* buffer size for transmit */

/* Device constant configuration parameters */
struct eth_stm32_hal_dev_cfg {
	void (*config_func)(void);
	struct stm32_pclken pclken;
	struct stm32_pclken pclken_rx;
	struct stm32_pclken pclken_tx;
#if !defined(CONFIG_SOC_SERIES_STM32H7X)
	struct stm32_pclken pclken_ptp;
#endif /* !defined(CONFIG_SOC_SERIES_STM32H7X) */
};

/* Device run time data */
struct eth_stm32_hal_dev_data {
	struct net_if *iface;
	uint8_t mac_addr[6];
	ETH_HandleTypeDef heth;
	/* clock device */
	struct device *clock;
	struct k_mutex tx_mutex;
	struct k_sem rx_int_sem;
	K_KERNEL_STACK_MEMBER(rx_thread_stack,
		CONFIG_ETH_STM32_HAL_RX_THREAD_STACK_SIZE);
	struct k_thread rx_thread;
	bool link_up;
};

#define DEV_CFG(dev) \
	((const struct eth_stm32_hal_dev_cfg *)(dev)->config)
#define DEV_DATA(dev) \
	((struct eth_stm32_hal_dev_data *)(dev)->data)

#endif /* ZEPHYR_DRIVERS_ETHERNET_ETH_STM32_HAL_PRIV_H_ */
