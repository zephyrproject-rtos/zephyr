/*
 * Copyright (c) 2017 Erwin Rol <erwin@erwinrol.com>
 * Copyright (c) 2020 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 * Copyright (c) 2021 Carbon Robotics
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/crc.h>
#include <errno.h>
#include <stdbool.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/phy.h>
#include <ethernet/eth_stats.h>
#include <soc.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/net/lldp.h>
#include <zephyr/drivers/hwinfo.h>

#if defined(CONFIG_NET_DSA_DEPRECATED)
#include <zephyr/net/dsa.h>
#endif

#if defined(CONFIG_PTP_CLOCK_STM32_HAL)
#include <zephyr/drivers/ptp_clock.h>
#endif /* CONFIG_PTP_CLOCK_STM32_HAL */

#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eth_stm32_hal_v1);

#include "eth.h"
#include "eth_stm32_hal_priv.h"

static ETH_DMADescTypeDef dma_rx_desc_tab[ETH_RXBUFNB] __eth_stm32_desc;
static ETH_DMADescTypeDef dma_tx_desc_tab[ETH_TXBUFNB] __eth_stm32_desc;

static uint8_t dma_rx_buffer[ETH_RXBUFNB][ETH_STM32_RX_BUF_SIZE] __eth_stm32_buf;
static uint8_t dma_tx_buffer[ETH_TXBUFNB][ETH_STM32_TX_BUF_SIZE] __eth_stm32_buf;

int eth_tx_v1(const struct device *dev, struct net_pkt *pkt)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;
	int res;
	size_t total_len;
	uint8_t *dma_buffer;
	__IO ETH_DMADescTypeDef *dma_tx_desc;
	HAL_StatusTypeDef hal_ret = HAL_OK;
#if defined(CONFIG_PTP_CLOCK_STM32_HAL)
	bool timestamped_frame;
#endif /* CONFIG_PTP_CLOCK_STM32_HAL */

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

#if defined(CONFIG_PTP_CLOCK_STM32_HAL)
	timestamped_frame = eth_is_ptp_pkt(net_pkt_iface(pkt), pkt) ||
			    net_pkt_is_tx_timestamping(pkt);
	if (timestamped_frame) {
		/* Enable transmit timestamp */
		HAL_ETH_PTP_InsertTxTimestamp(heth);
	}
#endif /* CONFIG_PTP_CLOCK_STM32_HAL */

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

struct net_pkt *eth_rx_v1(const struct device *dev)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;
	struct net_pkt *pkt;
	size_t total_len = 0;
	__IO ETH_DMADescTypeDef *dma_rx_desc;
	uint8_t *dma_buffer;

	HAL_StatusTypeDef hal_ret = HAL_OK;
#if defined(CONFIG_PTP_CLOCK_STM32_HAL)
	struct net_ptp_time timestamp;
	ETH_TimeStampTypeDef ts_registers;
	/* Default to invalid value. */
	timestamp.second = UINT64_MAX;
	timestamp.nanosecond = UINT32_MAX;
#endif /* CONFIG_PTP_CLOCK_STM32_HAL */

	hal_ret = HAL_ETH_GetReceivedFrame_IT(heth);
	if (hal_ret != HAL_OK) {
		/* no frame available */
		return NULL;
	}

	total_len = heth->RxFrameInfos.length;
	dma_buffer = (uint8_t *)heth->RxFrameInfos.buffer;

#if defined(CONFIG_PTP_CLOCK_STM32_HAL)
	if (HAL_ETH_PTP_GetRxTimestamp(heth, &ts_registers) == HAL_OK) {
		timestamp.second = ts_registers.TimeStampHigh;
		timestamp.nanosecond = ts_registers.TimeStampLow;
	}
#endif /* CONFIG_PTP_CLOCK_STM32_HAL */

	pkt = net_pkt_rx_alloc_with_buffer(get_iface(dev_data),
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

#if defined(CONFIG_PTP_CLOCK_STM32_HAL)
	pkt->timestamp.second = timestamp.second;
	pkt->timestamp.nanosecond = timestamp.nanosecond;
	if (timestamp.second != UINT64_MAX) {
		net_pkt_set_rx_timestamping(pkt, true);
	}
#endif /* CONFIG_PTP_CLOCK_STM32_HAL */

out:
	if (!pkt) {
		eth_stats_update_errors_rx(get_iface(dev_data));
	}

	return pkt;
}

void setup_mac_filter_v1(ETH_HandleTypeDef *heth)
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

void set_mac_config_v1(const struct device *dev, struct phy_link_state *state)
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

int eth_init_api_v1(const struct device *dev)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;
	HAL_StatusTypeDef hal_ret = HAL_OK;

	if (heth->Init.AutoNegotiation == 0) {
		struct phy_link_state state;

		phy_get_link_state(eth_stm32_hal_get_phy(dev), &state);

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

	HAL_ETH_DMATxDescListInit(heth, dma_tx_desc_tab,
		&dma_tx_buffer[0][0], ETH_TXBUFNB);
	HAL_ETH_DMARxDescListInit(heth, dma_rx_desc_tab,
		&dma_rx_buffer[0][0], ETH_RXBUFNB);

	return 0;
}
