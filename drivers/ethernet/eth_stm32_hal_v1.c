/*
 * Copyright (c) 2017 Erwin Rol <erwin@erwinrol.com>
 * Copyright (c) 2020 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 * Copyright (c) 2021 Carbon Robotics
 * Copyright (c) 2025 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/lldp.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>
#include <ethernet/eth_stats.h>

#include <stdbool.h>
#include <errno.h>

#include "eth.h"
#include "eth_stm32_hal_priv.h"

LOG_MODULE_DECLARE(eth_stm32_hal, CONFIG_ETHERNET_LOG_LEVEL);

void eth_stm32_setup_mac_filter(ETH_HandleTypeDef *heth)
{
	__ASSERT_NO_MSG(heth != NULL);
	uint32_t tmp = heth->Instance->MACFFR;

	/* clear all multicast filter bits, resulting in perfect filtering */
	tmp &= ~(ETH_MULTICASTFRAMESFILTER_PERFECTHASHTABLE |
		 ETH_MULTICASTFRAMESFILTER_HASHTABLE |
		 ETH_MULTICASTFRAMESFILTER_PERFECT |
		 ETH_MULTICASTFRAMESFILTER_NONE);

	if (IS_ENABLED(CONFIG_ETH_STM32_MULTICAST_FILTER)) {
		/* enable multicast hash receive filter */
		tmp |= ETH_MULTICASTFRAMESFILTER_HASHTABLE;
	} else {
		/* enable receiving all multicast frames */
		tmp |= ETH_MULTICASTFRAMESFILTER_NONE;
	}

	heth->Instance->MACFFR = tmp;

	/* Wait until the write operation will be taken into account:
	 * at least four TX_CLK/RX_CLK clock cycles
	 */
	tmp = heth->Instance->MACFFR;
	k_sleep(K_MSEC(1));
	heth->Instance->MACFFR = tmp;
}

int eth_stm32_tx(const struct device *dev, struct net_pkt *pkt)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;
	int res;
	size_t total_len;
	uint8_t *dma_buffer;
	__IO ETH_DMADescTypeDef *dma_tx_desc;
	HAL_StatusTypeDef hal_ret = HAL_OK;

	__ASSERT_NO_MSG(pkt != NULL);
	__ASSERT_NO_MSG(pkt->frags != NULL);

	total_len = net_pkt_get_len(pkt);
	if (total_len > (ETH_STM32_TX_BUF_SIZE * ETH_TXBUFNB)) {
		LOG_ERR("PKT too big");
		return -EIO;
	}

	k_mutex_lock(&dev_data->tx_mutex, K_FOREVER);

	dma_tx_desc = heth->TxDesc;
	while (IS_ETH_DMATXDESC_OWN(dma_tx_desc) != (uint32_t)RESET) {
		k_yield();
	}

	dma_buffer = (uint8_t *)(dma_tx_desc->Buffer1Addr);

	if (net_pkt_read(pkt, dma_buffer, total_len)) {
		res = -ENOBUFS;
		goto error;
	}

	hal_ret = HAL_ETH_TransmitFrame(heth, total_len);

	if (hal_ret != HAL_OK) {
		LOG_ERR("HAL_ETH_Transmit: failed!");
		res = -EIO;
		goto error;
	}

	/* When Transmit Underflow flag is set, clear it and issue a
	 * Transmit Poll Demand to resume transmission.
	 */
	if ((heth->Instance->DMASR & ETH_DMASR_TUS) != (uint32_t)RESET) {
		/* Clear TUS ETHERNET DMA flag */
		heth->Instance->DMASR = ETH_DMASR_TUS;
		/* Resume DMA transmission*/
		heth->Instance->DMATPDR = 0;
		res = -EIO;
		goto error;
	}

	res = 0;
error:

	k_mutex_unlock(&dev_data->tx_mutex);

	return res;
}

struct net_pkt *eth_stm32_rx(const struct device *dev)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;
	struct net_pkt *pkt;
	size_t total_len = 0;
	__IO ETH_DMADescTypeDef *dma_rx_desc;
	uint8_t *dma_buffer;
	HAL_StatusTypeDef hal_ret = HAL_OK;

	hal_ret = HAL_ETH_GetReceivedFrame_IT(heth);
	if (hal_ret != HAL_OK) {
		/* no frame available */
		return NULL;
	}

	total_len = heth->RxFrameInfos.length;
	dma_buffer = (uint8_t *)heth->RxFrameInfos.buffer;

	pkt = net_pkt_rx_alloc_with_buffer(dev_data->iface,
					   total_len, AF_UNSPEC, 0, K_MSEC(100));
	if (!pkt) {
		LOG_ERR("Failed to obtain RX buffer");
		goto release_desc;
	}

	if (net_pkt_write(pkt, dma_buffer, total_len)) {
		LOG_ERR("Failed to append RX buffer to context buffer");
		net_pkt_unref(pkt);
		pkt = NULL;
		goto release_desc;
	}

release_desc:
	/* Release descriptors to DMA */
	/* Point to first descriptor */
	dma_rx_desc = heth->RxFrameInfos.FSRxDesc;
	/* Set Own bit in Rx descriptors: gives the buffers back to DMA */
	for (int i = 0; i < heth->RxFrameInfos.SegCount; i++) {
		dma_rx_desc->Status |= ETH_DMARXDESC_OWN;
		dma_rx_desc = (ETH_DMADescTypeDef *)
			(dma_rx_desc->Buffer2NextDescAddr);
	}

	/* Clear Segment_Count */
	heth->RxFrameInfos.SegCount = 0;

	/* When Rx Buffer unavailable flag is set: clear it
	 * and resume reception.
	 */
	if ((heth->Instance->DMASR & ETH_DMASR_RBUS) != (uint32_t)RESET) {
		/* Clear RBUS ETHERNET DMA flag */
		heth->Instance->DMASR = ETH_DMASR_RBUS;
		/* Resume DMA reception */
		heth->Instance->DMARPDR = 0;
	}

	if (!pkt) {
		goto out;
	}

out:
	if (!pkt) {
		eth_stats_update_errors_rx(dev_data->iface);
	}

	return pkt;
}

int eth_stm32_hal_init(const struct device *dev)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;
	HAL_StatusTypeDef hal_ret = HAL_OK;

	if (!ETH_STM32_AUTO_NEGOTIATION_ENABLE) {
		struct phy_link_state state;

		phy_get_link_state(eth_stm32_phy_dev, &state);

		heth->Init.DuplexMode = PHY_LINK_IS_FULL_DUPLEX(state.speed) ? ETH_MODE_FULLDUPLEX
									     : ETH_MODE_HALFDUPLEX;
		heth->Init.Speed =
			PHY_LINK_IS_SPEED_100M(state.speed) ? ETH_SPEED_100M : ETH_SPEED_10M;
	}

	hal_ret = HAL_ETH_Init(heth);
	if (hal_ret == HAL_TIMEOUT) {
		/* HAL Init time out. This could be linked to */
		/* a recoverable error. Log the issue and continue */
		/* driver initialisation */
		LOG_WRN("HAL_ETH_Init timed out (cable not connected?)");
	} else if (hal_ret != HAL_OK) {
		LOG_ERR("HAL_ETH_Init failed: %d", hal_ret);
		return -EINVAL;
	}

	/* Initialize semaphores */
	k_mutex_init(&dev_data->tx_mutex);
	k_sem_init(&dev_data->rx_int_sem, 0, K_SEM_MAX_LIMIT);

	if (HAL_ETH_DMATxDescListInit(heth, dma_tx_desc_tab,
				      &dma_tx_buffer[0][0], ETH_TXBUFNB) != HAL_OK) {
		return -EIO;
	}
	if (HAL_ETH_DMARxDescListInit(heth, dma_rx_desc_tab,
				      &dma_rx_buffer[0][0], ETH_RXBUFNB) != HAL_OK) {
		return -EIO;
	}

	return 0;
}

void eth_stm32_set_mac_config(const struct device *dev, struct phy_link_state *state)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;
	HAL_StatusTypeDef hal_ret = HAL_OK;

	heth->Init.DuplexMode =
		PHY_LINK_IS_FULL_DUPLEX(state->speed) ? ETH_MODE_FULLDUPLEX : ETH_MODE_HALFDUPLEX;

	heth->Init.Speed = PHY_LINK_IS_SPEED_100M(state->speed) ? ETH_SPEED_100M : ETH_SPEED_10M;

	hal_ret = HAL_ETH_ConfigMAC(heth, NULL);
	if (hal_ret != HAL_OK) {
		LOG_ERR("HAL_ETH_ConfigMAC: failed: %d", hal_ret);
	}
}

int eth_stm32_hal_start(const struct device *dev)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;
	HAL_StatusTypeDef hal_ret = HAL_OK;

	LOG_DBG("Starting ETH HAL driver");

	hal_ret = HAL_ETH_Start(heth);

	if (hal_ret != HAL_OK) {
		LOG_ERR("HAL_ETH_Start{_IT} failed");
	}

	return 0;
}

int eth_stm32_hal_stop(const struct device *dev)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;
	HAL_StatusTypeDef hal_ret = HAL_OK;

	LOG_DBG("Stopping ETH HAL driver");

	hal_ret = HAL_ETH_Stop(heth);

	if (hal_ret != HAL_OK) {
		/* HAL_ETH_Stop{_IT} returns HAL_ERROR only if ETH is already stopped */
		LOG_DBG("HAL_ETH_Stop{_IT} returned error (Ethernet is already stopped)");
	}

	return 0;
}

int eth_stm32_hal_set_config(const struct device *dev,
				    enum ethernet_config_type type,
				    const struct ethernet_config *config)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(dev_data->mac_addr, config->mac_address.addr, 6);
		heth->Instance->MACA0HR = (dev_data->mac_addr[5] << 8) |
			dev_data->mac_addr[4];
		heth->Instance->MACA0LR = (dev_data->mac_addr[3] << 24) |
			(dev_data->mac_addr[2] << 16) |
			(dev_data->mac_addr[1] << 8) |
			dev_data->mac_addr[0];
		net_if_set_link_addr(dev_data->iface, dev_data->mac_addr,
				     sizeof(dev_data->mac_addr),
				     NET_LINK_ETHERNET);
		return 0;
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		if (config->promisc_mode) {
			heth->Instance->MACFFR |= ETH_MACFFR_PM;
		} else {
			heth->Instance->MACFFR &= ~ETH_MACFFR_PM;
		}
		return 0;
#endif /* CONFIG_NET_PROMISCUOUS_MODE */
#if defined(CONFIG_ETH_STM32_MULTICAST_FILTER)
	case ETHERNET_CONFIG_TYPE_FILTER:
		eth_stm32_mcast_filter(dev, &config->filter);
		return 0;
#endif /* CONFIG_ETH_STM32_MULTICAST_FILTER */
	default:
		break;
	}

	return -ENOTSUP;
}
