/*
 * Copyright (c) 2017 Erwin Rol <erwin@erwinrol.com>
 * Copyright (c) 2020 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 * Copyright (c) 2021 Carbon Robotics
 * Copyright (c) 2025 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>
#include <ethernet/eth_stats.h>
#include <zephyr/linker/devicetree_regions.h>

#include <errno.h>
#include <stdbool.h>

#include "eth.h"
#include "eth_stm32_hal_priv.h"

#define DT_DRV_COMPAT st_stm32_ethernet

LOG_MODULE_DECLARE(eth_stm32_hal, CONFIG_ETHERNET_LOG_LEVEL);

#define ETH_DMA_TX_TIMEOUT_MS	20U  /* transmit timeout in milliseconds */

/* We separate the cases where HAL API uses heth or not */
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32mp13_ethernet)
#define ETH_STM32_HAL_CB_HAS_HETH
#define STM32_ETH_ARGS(heth, ...) heth, __VA_ARGS__
#else
#define STM32_ETH_ARGS(heth, ...) __VA_ARGS__
#endif

#ifndef ETH_STM32_HAL_CB_HAS_HETH
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1,
	     "Multiple Ethernet instances are not supported on this platform");
#endif

#ifdef ETH_STM32_HAL_CB_HAS_HETH
#define ETH_STM32_HAL_GET_CB_DEVDATA(_heth) \
	CONTAINER_OF(_heth, struct eth_stm32_hal_dev_data, heth)
#else
#define ETH_STM32_HAL_GET_CB_DEVDATA(_heth) \
	((struct eth_stm32_hal_dev_data *)DEVICE_DT_INST_GET(0)->data)
#endif

/* Pointer to an array of ETH_STM32_RX_BUF_SIZE uint8_t's */
typedef uint8_t (*RxBufferPtr)[ETH_STM32_RX_BUF_SIZE];

void HAL_ETH_RxAllocateCallback(STM32_ETH_ARGS(ETH_HandleTypeDef *heth, uint8_t **buf))
{
	struct eth_stm32_hal_dev_data *dev_data = ETH_STM32_HAL_GET_CB_DEVDATA(heth);
	const struct eth_stm32_hal_dev_cfg *cfg;
	const struct device *dev;

	dev = net_if_get_device(dev_data->iface);
	cfg = dev->config;

	for (size_t i = 0; i < ETH_RXBUFNB; ++i) {
		if (!dev_data->rx_buffer_header[i].used) {
			dev_data->rx_buffer_header[i].next = NULL;
			dev_data->rx_buffer_header[i].size = 0U;
			dev_data->rx_buffer_header[i].used = true;
			*buf = cfg->dma_buf->rx_buf[i];
			return;
		}
	}

	*buf = NULL;
}

/* called by HAL_ETH_ReadData() */
void HAL_ETH_RxLinkCallback(STM32_ETH_ARGS(ETH_HandleTypeDef *heth, void **pStart,
					   void **pEnd, uint8_t *buff, uint16_t length))
{
	struct eth_stm32_hal_dev_data *dev_data = ETH_STM32_HAL_GET_CB_DEVDATA(heth);
	struct eth_stm32_rx_buffer_header *header;
	const struct eth_stm32_hal_dev_cfg *cfg;
	const struct device *dev;
	size_t index;

	dev = net_if_get_device(dev_data->iface);
	cfg = dev->config;

	index = (RxBufferPtr)buff - &cfg->dma_buf->rx_buf[0];

	__ASSERT_NO_MSG(index < ETH_RXBUFNB);

	header = &dev_data->rx_buffer_header[index];
	header->size = length;

	if (!*pStart) {
		*pStart = header;
		*pEnd = header;
	} else {
		__ASSERT_NO_MSG(*pEnd != NULL);
		((struct eth_stm32_rx_buffer_header *)*pEnd)->next = header;
		*pEnd = header;
	}
}

/* Called by HAL_ETH_ReleaseTxPacket */
void HAL_ETH_TxFreeCallback(STM32_ETH_ARGS(ETH_HandleTypeDef *heth, uint32_t *buff))
{
	struct eth_stm32_hal_dev_data *dev_data = ETH_STM32_HAL_GET_CB_DEVDATA(heth);
	struct eth_stm32_tx_buffer_header *buffer_header;
	struct eth_stm32_tx_context *ctx;

	ctx = (struct eth_stm32_tx_context *)buff;
	buffer_header = &dev_data->tx_buffer_header[ctx->first_tx_buffer_index];

	while (buffer_header != NULL) {
		buffer_header->used = false;
		if (buffer_header->tx_buff.next != NULL) {
			buffer_header = CONTAINER_OF(buffer_header->tx_buff.next,
						     struct eth_stm32_tx_buffer_header,
						     tx_buff);
		} else {
			buffer_header = NULL;
		}
	}

	ctx->used = false;
}

/* allocate a tx buffer and mark it as used */
static inline uint16_t allocate_tx_buffer(struct eth_stm32_hal_dev_data *dev_data)
{
	for (;;) {
		for (uint16_t index = 0U; index < ETH_TXBUFNB; index++) {
			if (!dev_data->tx_buffer_header[index].used) {
				dev_data->tx_buffer_header[index].used = true;
				return index;
			}
		}
		k_yield();
	}
}

#if defined(CONFIG_ETH_STM32_HAL_TX_ASYNC)
/* allocate a tx context and mark it as used, the first tx buffer is also allocated */
static struct eth_stm32_tx_context *
allocate_tx_context_async(struct eth_stm32_hal_dev_data *dev_data, struct net_pkt *pkt)
{
	int tx_index;

	for (uint16_t index = 0U; index < ETH_TX_DESC_CNT; index++) {
		if (!dev_data->tx_context[index].used) {
			dev_data->tx_context[index].used = true;
			dev_data->tx_context[index].pkt = pkt;
			tx_index = allocate_tx_buffer(dev_data);
			dev_data->tx_context[index].first_tx_buffer_index = tx_index;
			return &dev_data->tx_context[index];
		}
	}

	return NULL;
}

int eth_stm32_tx(const struct device *dev, struct net_pkt *pkt)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;
	int res = 0;
	size_t total_len;
	size_t remaining_read;
	struct eth_stm32_tx_context *ctx = NULL;
	struct eth_stm32_tx_buffer_header *buf_header = NULL;
	uint16_t next_buffer_id;
	HAL_StatusTypeDef hal_ret;
	ETH_TxPacketConfigTypeDef tx_config = {
		.Attributes = ETH_TX_PACKETS_FEATURES_CSUM | ETH_TX_PACKETS_FEATURES_CRCPAD,
		.ChecksumCtrl = IS_ENABLED(CONFIG_ETH_STM32_HW_CHECKSUM) ?
			ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC : ETH_CHECKSUM_DISABLE,
		.CRCPadCtrl = ETH_CRC_PAD_INSERT,
	};

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

	while (ctx == NULL) {
		ctx = allocate_tx_context_async(dev_data, pkt);
		if (ctx == NULL) {
			k_sem_take(&dev_data->tx_int_sem, K_MSEC(ETH_DMA_TX_TIMEOUT_MS));
			hal_ret = HAL_ETH_ReleaseTxPacket(heth);
			__ASSERT_NO_MSG(hal_ret == HAL_OK);
		}
	}
	buf_header = &dev_data->tx_buffer_header[ctx->first_tx_buffer_index];

#if defined(CONFIG_PTP_CLOCK_STM32_HAL)
	timestamped_frame = eth_stm32_is_ptp_pkt(net_pkt_iface(pkt), pkt) ||
		net_pkt_is_tx_timestamping(pkt);
	if (timestamped_frame) {
		/* Enable transmit timestamp */
		if (HAL_ETH_PTP_InsertTxTimestamp(heth) != HAL_OK) {
			res = -EIO;
			goto error;
		}
	}
#endif /* CONFIG_PTP_CLOCK_STM32_HAL */

	remaining_read = total_len;
	/* fill and allocate buffer until remaining data fits in one buffer */
	while (remaining_read > ETH_STM32_TX_BUF_SIZE) {
		res = net_pkt_read(pkt, buf_header->tx_buff.buffer, ETH_STM32_TX_BUF_SIZE);
		if (res < 0) {
			goto error;
		}

		next_buffer_id = allocate_tx_buffer(dev_data);

		buf_header->tx_buff.len = ETH_STM32_TX_BUF_SIZE;
		/* append new buffer to the linked list */
		buf_header->tx_buff.next = &dev_data->tx_buffer_header[next_buffer_id].tx_buff;
		/* and adjust tail pointer */
		buf_header = &dev_data->tx_buffer_header[next_buffer_id];
		remaining_read -= ETH_STM32_TX_BUF_SIZE;
	}
	res = net_pkt_read(pkt, buf_header->tx_buff.buffer, remaining_read);
	if (res < 0) {
		goto error;
	}
	buf_header->tx_buff.len = remaining_read;
	buf_header->tx_buff.next = NULL;

	tx_config.Length = total_len;
	tx_config.pData = ctx;
	tx_config.TxBuffer =
		&dev_data->tx_buffer_header[ctx->first_tx_buffer_index].tx_buff;

	if (HAL_ETH_Transmit_IT(heth, &tx_config) != HAL_OK) {
		LOG_ERR("HAL_ETH_Transmit: failed!");
		res = -EIO;
	}

error:
	if (res < 0 && ctx) {
		/* We need to release the tx context and its buffers */
		HAL_ETH_TxFreeCallback(STM32_ETH_ARGS(heth, (uint32_t *)ctx));
	}

	return res;
}
#else

/* allocate a tx context and mark it as used, the first tx buffer is also allocated */
static inline struct eth_stm32_tx_context *
allocate_tx_context(struct eth_stm32_hal_dev_data *dev_data, struct net_pkt *pkt)
{
	for (;;) {
		for (uint16_t index = 0; index < ETH_TX_DESC_CNT; index++) {
			if (!dev_data->tx_context[index].used) {
				dev_data->tx_context[index].used = true;
				dev_data->tx_context[index].pkt = pkt;
				dev_data->tx_context[index].first_tx_buffer_index =
					allocate_tx_buffer(dev_data);
				return &dev_data->tx_context[index];
			}
		}
		k_yield();
	}
}

int eth_stm32_tx(const struct device *dev, struct net_pkt *pkt)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;
	int res;
	size_t total_len;
	size_t remaining_read;
	struct eth_stm32_tx_context *ctx = NULL;
	struct eth_stm32_tx_buffer_header *buf_header = NULL;
	HAL_StatusTypeDef hal_ret;
	ETH_TxPacketConfigTypeDef tx_config = {
		.Attributes = ETH_TX_PACKETS_FEATURES_CSUM | ETH_TX_PACKETS_FEATURES_CRCPAD,
		.ChecksumCtrl = IS_ENABLED(CONFIG_ETH_STM32_HW_CHECKSUM) ?
			ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC : ETH_CHECKSUM_DISABLE,
		.CRCPadCtrl = ETH_CRC_PAD_INSERT,
	};
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

	ctx = allocate_tx_context(dev_data, pkt);
	buf_header = &dev_data->tx_buffer_header[ctx->first_tx_buffer_index];

#if defined(CONFIG_PTP_CLOCK_STM32_HAL)
	timestamped_frame = eth_stm32_is_ptp_pkt(net_pkt_iface(pkt), pkt) ||
			    net_pkt_is_tx_timestamping(pkt);
	if (timestamped_frame) {
		/* Enable transmit timestamp */
		if (HAL_ETH_PTP_InsertTxTimestamp(heth) != HAL_OK) {
			return -EIO;
		}
	}
#endif /* CONFIG_PTP_CLOCK_STM32_HAL */

	remaining_read = total_len;
	/* fill and allocate buffer until remaining data fits in one buffer */
	while (remaining_read > ETH_STM32_TX_BUF_SIZE) {
		if (net_pkt_read(pkt, buf_header->tx_buff.buffer, ETH_STM32_TX_BUF_SIZE)) {
			res = -ENOBUFS;
			goto error;
		}
		const uint16_t next_buffer_id = allocate_tx_buffer(dev_data);

		buf_header->tx_buff.len = ETH_STM32_TX_BUF_SIZE;
		/* append new buffer to the linked list */
		buf_header->tx_buff.next = &dev_data->tx_buffer_header[next_buffer_id].tx_buff;
		/* and adjust tail pointer */
		buf_header = &dev_data->tx_buffer_header[next_buffer_id];
		remaining_read -= ETH_STM32_TX_BUF_SIZE;
	}
	if (net_pkt_read(pkt, buf_header->tx_buff.buffer, remaining_read)) {
		res = -ENOBUFS;
		goto error;
	}
	buf_header->tx_buff.len = remaining_read;
	buf_header->tx_buff.next = NULL;

	tx_config.Length = total_len;
	tx_config.pData = ctx;
	tx_config.TxBuffer =
		&dev_data->tx_buffer_header[ctx->first_tx_buffer_index].tx_buff;

	/* Reset TX complete interrupt semaphore before TX request*/
	k_sem_reset(&dev_data->tx_int_sem);

	/* tx_buffer is allocated on function stack, we need */
	/* to wait for the transfer to complete */
	/* So it is not freed before the interrupt happens */
	hal_ret = HAL_ETH_Transmit_IT(heth, &tx_config);

	if (hal_ret != HAL_OK) {
		LOG_ERR("HAL_ETH_Transmit: failed!");
		res = -EIO;
		goto error;
	}

	/* the tx context is now owned by the HAL */
	ctx = NULL;

	/* Wait for end of TX buffer transmission */
	/* If the semaphore timeout breaks, it means */
	/* an error occurred or IT was not fired */
	if (k_sem_take(&dev_data->tx_int_sem,
			K_MSEC(ETH_DMA_TX_TIMEOUT_MS)) != 0) {

		LOG_ERR("HAL_ETH_TransmitIT tx_int_sem take timeout");
		res = -EIO;

		/* Check for errors */
		/* Ethernet device was put in error state */
		/* Error state is unrecoverable ? */
		if (HAL_ETH_GetState(heth) == HAL_ETH_STATE_ERROR) {
			LOG_ERR("%s: ETH in error state: errorcode:%x",
				__func__,
				HAL_ETH_GetError(heth));
			/* TODO recover from error state by restarting eth */
		}

		/* Check for DMA errors */
		if (HAL_ETH_GetDMAError(heth) != 0U) {
			LOG_ERR("%s: ETH DMA error: dmaerror:%x",
				__func__,
				HAL_ETH_GetDMAError(heth));
			/* DMA fatal bus errors are putting in error state*/
			/* TODO recover from this */
		}

		/* Check for MAC errors */
		if (HAL_ETH_GetMACError(heth) != 0U) {
			LOG_ERR("%s: ETH MAC error: macerror:%x",
				__func__,
				HAL_ETH_GetMACError(heth));
			/* MAC errors are putting in error state*/
			/* TODO recover from this */
		}

		goto error;
	}

	res = 0;
error:

	if (!ctx) {
		/* The HAL owns the tx context */
		if (HAL_ETH_ReleaseTxPacket(heth) != HAL_OK) {
			return -EIO;
		}
	} else {
		/* We need to release the tx context and its buffers */
		HAL_ETH_TxFreeCallback(STM32_ETH_ARGS(heth, (uint32_t *)ctx));
	}

	return res;
}
#endif /* ETH_STM32_HAL_TX_ASYNC */

struct net_pkt *eth_stm32_rx(const struct device *dev)
{
	const struct eth_stm32_hal_dev_cfg *cfg = dev->config;
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;
	struct net_pkt *pkt;
	size_t total_len = 0;
	void *appbuf = NULL;
	struct eth_stm32_rx_buffer_header *rx_header;

#if defined(CONFIG_PTP_CLOCK_STM32_HAL)
	struct net_ptp_time timestamp;
	ETH_TimeStampTypeDef ts_registers;
	/* Default to invalid value. */
	timestamp.second = UINT64_MAX;
	timestamp.nanosecond = UINT32_MAX;
#endif /* CONFIG_PTP_CLOCK_STM32_HAL */

	if (HAL_ETH_ReadData(heth, &appbuf) != HAL_OK) {
		/* no frame available */
		return NULL;
	}

	/* computing total length */
	for (rx_header = (struct eth_stm32_rx_buffer_header *)appbuf;
			rx_header; rx_header = rx_header->next) {
		total_len += rx_header->size;
	}

#if defined(CONFIG_PTP_CLOCK_STM32_HAL)
	if (HAL_ETH_PTP_GetRxTimestamp(heth, &ts_registers) == HAL_OK) {
		timestamp.second = ts_registers.TimeStampHigh;
		timestamp.nanosecond = ts_registers.TimeStampLow;
	}
#endif /* CONFIG_PTP_CLOCK_STM32_HAL */

	pkt = net_pkt_rx_alloc_with_buffer(dev_data->iface,
					   total_len, NET_AF_UNSPEC, 0, K_MSEC(100));
	if (!pkt) {
		LOG_ERR("Failed to obtain RX buffer");
		goto release_desc;
	}

	for (rx_header = (struct eth_stm32_rx_buffer_header *)appbuf;
			rx_header; rx_header = rx_header->next) {
		const size_t index = rx_header - &dev_data->rx_buffer_header[0];

		__ASSERT_NO_MSG(index < ETH_RXBUFNB);
		if (net_pkt_write(pkt, cfg->dma_buf->rx_buf[index], rx_header->size)) {
			LOG_ERR("Failed to append RX buffer to context buffer");
			net_pkt_unref(pkt);
			pkt = NULL;
			goto release_desc;
		}
	}

release_desc:
	for (rx_header = (struct eth_stm32_rx_buffer_header *)appbuf;
			rx_header; rx_header = rx_header->next) {
		rx_header->used = false;
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
		eth_stats_update_errors_rx(dev_data->iface);
	}

	return pkt;
}

void HAL_ETH_TxCpltCallback(ETH_HandleTypeDef *heth_handle)
{
	struct eth_stm32_hal_dev_data *dev_data;

	__ASSERT_NO_MSG(heth_handle != NULL);

	dev_data = CONTAINER_OF(heth_handle, struct eth_stm32_hal_dev_data, heth);

	__ASSERT_NO_MSG(dev_data != NULL);

	k_sem_give(&dev_data->tx_int_sem);

}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
static void eth_stm32_update_dma_error(struct eth_stm32_hal_dev_data *dev_data, uint32_t dma_error)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet)
	if ((dma_error & ETH_DMA_RX_WATCHDOG_TIMEOUT_FLAG) ||
	    (dma_error & ETH_DMA_RX_PROCESS_STOPPED_FLAG) ||
	    (dma_error & ETH_DMA_RX_BUFFER_UNAVAILABLE_FLAG)) {
		eth_stats_update_errors_rx(dev_data->iface);
	}
	if ((dma_error & ETH_DMA_EARLY_TX_IT_FLAG) ||
	    (dma_error & ETH_DMA_TX_PROCESS_STOPPED_FLAG)) {
		eth_stats_update_errors_tx(dev_data->iface);
	}
#else
	if ((dma_error & ETH_DMASR_RWTS) || (dma_error & ETH_DMASR_RPSS) ||
	    (dma_error & ETH_DMASR_RBUS)) {
		eth_stats_update_errors_rx(dev_data->iface);
	}
	if ((dma_error & ETH_DMASR_ETS) || (dma_error & ETH_DMASR_TPSS) ||
	    (dma_error & ETH_DMASR_TJTS)) {
		eth_stats_update_errors_tx(dev_data->iface);
	}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet) */
}

static void eth_stm32_update_rx_error_details(ETH_HandleTypeDef *heth,
					      struct eth_stm32_hal_dev_data *dev_data)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32mp13_ethernet)
	dev_data->stats.error_details.rx_crc_errors = heth->Instance->MMCRXCRCEPR;
	dev_data->stats.error_details.rx_align_errors = heth->Instance->MMCRXAEPR;
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet)
	dev_data->stats.error_details.rx_crc_errors = heth->Instance->MMCRCRCEPR;
	dev_data->stats.error_details.rx_align_errors = heth->Instance->MMCRAEPR;
#else
	dev_data->stats.error_details.rx_crc_errors = heth->Instance->MMCRFCECR;
	dev_data->stats.error_details.rx_align_errors = heth->Instance->MMCRFAECR;
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32mp13_ethernet) */
}
#endif /* CONFIG_NET_STATISTICS_ETHERNET */

void HAL_ETH_ErrorCallback(ETH_HandleTypeDef *heth)
{
	/* Do not log errors. If errors are reported due to high traffic,
	 * logging errors will only increase traffic issues
	 */
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	__ASSERT_NO_MSG(heth != NULL);

	uint32_t dma_error;
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet)
	uint32_t mac_error;
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet) */
	const uint32_t error_code = HAL_ETH_GetError(heth);

	struct eth_stm32_hal_dev_data *dev_data =
		CONTAINER_OF(heth, struct eth_stm32_hal_dev_data, heth);

	switch (error_code) {
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32n6_ethernet)
	case HAL_ETH_ERROR_DMA_CH0:
	case HAL_ETH_ERROR_DMA_CH1:
#else
	case HAL_ETH_ERROR_DMA:
#endif
		dma_error = HAL_ETH_GetDMAError(heth);

		eth_stm32_update_dma_error(dev_data, dma_error);

		break;

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet)
	case HAL_ETH_ERROR_MAC:
		mac_error = HAL_ETH_GetMACError(heth);

		if (mac_error & ETH_RECEIVE_WATCHDOG_TIMEOUT) {
			eth_stats_update_errors_rx(dev_data->iface);
		}

		if ((mac_error & ETH_EXECESSIVE_COLLISIONS)  ||
			(mac_error & ETH_LATE_COLLISIONS)        ||
			(mac_error & ETH_EXECESSIVE_DEFERRAL)    ||
			(mac_error & ETH_TRANSMIT_JABBR_TIMEOUT) ||
			(mac_error & ETH_LOSS_OF_CARRIER)        ||
			(mac_error & ETH_NO_CARRIER)) {
			eth_stats_update_errors_tx(dev_data->iface);
		}
		break;
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet) */
	}

	eth_stm32_update_rx_error_details(heth, dev_data);

#endif /* CONFIG_NET_STATISTICS_ETHERNET */
}

int eth_stm32_hal_init(const struct device *dev)
{
	const struct eth_stm32_hal_dev_cfg *cfg = dev->config;
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;
	HAL_StatusTypeDef hal_ret;
	__maybe_unused uint8_t *desc_uncached_addr;

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32n6_ethernet)
	for (int ch = 0; ch < ETH_DMA_CH_CNT; ch++) {
		heth->Init.TxDesc[ch] = &cfg->dma_desc->tx_desc[ch][0];
		heth->Init.RxDesc[ch] = &cfg->dma_desc->rx_desc[ch][0];
	}
#else
	heth->Init.TxDesc = cfg->dma_desc->tx_desc;
	heth->Init.RxDesc = cfg->dma_desc->rx_desc;
#endif
	heth->Init.RxBuffLen = ETH_STM32_RX_BUF_SIZE;

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32mp13_ethernet)
	/* Map memory region for DMA descriptor and buffer as non cacheable */
	k_mem_map_phys_bare(&desc_uncached_addr,
			    DT_REG_ADDR(ETH_DMA_REGION),
			    DT_REG_SIZE(ETH_DMA_REGION),
			    K_MEM_PERM_RW | K_MEM_DIRECT_MAP | K_MEM_ARM_NORMAL_NC);
#endif

	hal_ret = HAL_ETH_Init(heth);
	if (hal_ret != HAL_OK) {
		LOG_ERR("HAL_ETH_Init failed: %d", hal_ret);
		return -EINVAL;
	}

#if defined(CONFIG_SOC_SERIES_STM32F4X) || defined(CONFIG_SOC_SERIES_STM32F7X)
	/* Workaround for F4x and F7x as the HAL_ETH_Init function
	 * does not set back the MDIO clock range after resetting
	 * the MAC for these series.
	 */
	HAL_ETH_SetMDIOClockRange(heth);

#endif /* CONFIG_SOC_SERIES_STM32F4X || CONFIG_SOC_SERIES_STM32F7X */
#if defined(CONFIG_PTP_CLOCK_STM32_HAL)
	/* Enable timestamping of RX packets. We enable all packets to be
	 * timestamped to cover both IEEE 1588 and gPTP.
	 */
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet)
	heth->Instance->MACTSCR |= ETH_MACTSCR_TSENALL;
#else
	heth->Instance->PTPTSCR |= ETH_PTPTSCR_TSSARFE;
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet) */
#endif /* CONFIG_PTP_CLOCK_STM32_HAL */

	for (uint16_t i = 0; i < ETH_TXBUFNB; ++i) {
		dev_data->tx_buffer_header[i].tx_buff.buffer = cfg->dma_buf->tx_buf[i];
	}

	return 0;
}

void eth_stm32_set_mac_config(const struct device *dev, struct phy_link_state *state)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;
	HAL_StatusTypeDef hal_ret;
	ETH_MACConfigTypeDef mac_config = {0};

	hal_ret = HAL_ETH_GetMACConfig(heth, &mac_config);
	if (hal_ret != HAL_OK) {
		LOG_ERR("HAL_ETH_GetMACConfig failed: %d", hal_ret);
		__ASSERT_NO_MSG(0);
		return;
	}

	mac_config.DuplexMode =
		PHY_LINK_IS_FULL_DUPLEX(state->speed) ? ETH_FULLDUPLEX_MODE : ETH_HALFDUPLEX_MODE;

	mac_config.Speed =
		IF_ENABLED(DT_HAS_COMPAT_STATUS_OKAY(st_stm32n6_ethernet),
			PHY_LINK_IS_SPEED_1000M(state->speed) ? ETH_SPEED_1000M :)
		PHY_LINK_IS_SPEED_100M(state->speed) ? ETH_SPEED_100M : ETH_SPEED_10M;

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32n6_ethernet)
	mac_config.PortSelect = PHY_LINK_IS_SPEED_1000M(state->speed) ? DISABLE : ENABLE;
#endif

	/* Always disable hardware source address replacement.
	 * Zephyr network stack sets the source MAC address and
	 * therefore hardware replacement should not be enabled,
	 * since it may affect bridging applications, for example.
	 */
	mac_config.SourceAddrControl = ETH_SOURCEADDRESS_DISABLE;

	hal_ret = HAL_ETH_SetMACConfig(heth, &mac_config);
	if (hal_ret != HAL_OK) {
		LOG_ERR("HAL_ETH_SetMACConfig failed: %d", hal_ret);
		__ASSERT_NO_MSG(0);
	}
}

void eth_stm32_setup_mac_filter(ETH_HandleTypeDef *heth)
{
	ETH_MACFilterConfigTypeDef MACFilterConf;
	HAL_StatusTypeDef __maybe_unused hal_ret;

	__ASSERT_NO_MSG(heth != NULL);

	hal_ret = HAL_ETH_GetMACFilterConfig(heth, &MACFilterConf);
	__ASSERT_NO_MSG(hal_ret == HAL_OK);

	MACFilterConf.HashMulticast =
		IS_ENABLED(CONFIG_ETH_STM32_MULTICAST_FILTER) ? ENABLE : DISABLE;
	MACFilterConf.PassAllMulticast =
		IS_ENABLED(CONFIG_ETH_STM32_MULTICAST_FILTER) ? DISABLE : ENABLE;
	MACFilterConf.HachOrPerfectFilter = DISABLE;

	hal_ret = HAL_ETH_SetMACFilterConfig(heth, &MACFilterConf);
	__ASSERT_NO_MSG(hal_ret == HAL_OK);

	k_sleep(K_MSEC(1));
}

int eth_stm32_hal_start(const struct device *dev, struct net_if *iface __unused)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;
	HAL_StatusTypeDef hal_ret;

	LOG_DBG("Starting ETH HAL driver");

	hal_ret = HAL_ETH_Start_IT(heth);

	if (hal_ret != HAL_OK) {
		LOG_ERR("HAL_ETH_Start{_IT} failed");
	}

	return 0;
}

int eth_stm32_hal_stop(const struct device *dev, struct net_if *iface __unused)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;
	HAL_StatusTypeDef hal_ret;

	LOG_DBG("Stopping ETH HAL driver");

	hal_ret = HAL_ETH_Stop_IT(heth);

	if (hal_ret != HAL_OK) {
		/* HAL_ETH_Stop{_IT} returns HAL_ERROR only if ETH is already stopped */
		LOG_DBG("HAL_ETH_Stop{_IT} returned error (Ethernet is already stopped)");
	}

	return 0;
}

int eth_stm32_hal_set_config(const struct device *dev,
			     struct net_if *iface __unused,
			     enum ethernet_config_type type,
			     const struct ethernet_config *config)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(dev_data->mac_addr, config->mac_address.addr,
		       sizeof(dev_data->mac_addr));
		heth->Instance->MACA0HR = (dev_data->mac_addr[5] << 8) |
			dev_data->mac_addr[4];
		heth->Instance->MACA0LR = (dev_data->mac_addr[3] << 24) |
			(dev_data->mac_addr[2] << 16) |
			(dev_data->mac_addr[1] << 8) |
			dev_data->mac_addr[0];
		return 0;
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		ETH_MACFilterConfigTypeDef MACFilterConf;

		if (HAL_ETH_GetMACFilterConfig(heth, &MACFilterConf) != HAL_OK) {
			return -EIO;
		}

		MACFilterConf.PromiscuousMode = config->promisc_mode ? ENABLE : DISABLE;

		if (HAL_ETH_SetMACFilterConfig(heth, &MACFilterConf) != HAL_OK) {
			return -EIO;
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
