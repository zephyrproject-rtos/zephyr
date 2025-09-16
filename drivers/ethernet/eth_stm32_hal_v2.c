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

LOG_MODULE_DECLARE(eth_stm32_hal, CONFIG_ETHERNET_LOG_LEVEL);

#define ETH_DMA_TX_TIMEOUT_MS	20U  /* transmit timeout in milliseconds */

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32mp13_ethernet)
#define STM32_ETH_ARGS(heth, ...)  heth, __VA_ARGS__
#else
#define STM32_ETH_ARGS(heth, ...)  __VA_ARGS__
#endif

struct eth_stm32_rx_buffer_header {
	struct eth_stm32_rx_buffer_header *next;
	uint16_t size;
	bool used;
};

struct eth_stm32_tx_buffer_header {
	ETH_BufferTypeDef tx_buff;
	bool used;
};

static ETH_TxPacketConfigTypeDef tx_config;

static struct eth_stm32_rx_buffer_header dma_rx_buffer_header[ETH_RXBUFNB];
static struct eth_stm32_tx_buffer_header dma_tx_buffer_header[ETH_TXBUFNB];
static struct eth_stm32_tx_context dma_tx_context[ETH_TX_DESC_CNT];

/* Pointer to an array of ETH_STM32_RX_BUF_SIZE uint8_t's */
typedef uint8_t (*RxBufferPtr)[ETH_STM32_RX_BUF_SIZE];

void HAL_ETH_RxAllocateCallback(STM32_ETH_ARGS(ETH_HandleTypeDef *heth, uint8_t **buf))
{
	for (size_t i = 0; i < ETH_RXBUFNB; ++i) {
		if (!dma_rx_buffer_header[i].used) {
			dma_rx_buffer_header[i].next = NULL;
			dma_rx_buffer_header[i].size = 0;
			dma_rx_buffer_header[i].used = true;
			*buf = dma_rx_buffer[i];
			return;
		}
	}
	*buf = NULL;
}

/* called by HAL_ETH_ReadData() */
void HAL_ETH_RxLinkCallback(STM32_ETH_ARGS(ETH_HandleTypeDef *heth, void **pStart,
					   void **pEnd, uint8_t *buff, uint16_t Length))
{
	/* buff points to the begin on one of the rx buffers,
	 * so we can compute the index of the given buffer
	 */
	size_t index = (RxBufferPtr)buff - &dma_rx_buffer[0];
	struct eth_stm32_rx_buffer_header *header = &dma_rx_buffer_header[index];

	__ASSERT_NO_MSG(index < ETH_RXBUFNB);

	header->size = Length;

	if (!*pStart) {
		/* first packet, set head pointer of linked list */
		*pStart = header;
		*pEnd = header;
	} else {
		__ASSERT_NO_MSG(*pEnd != NULL);
		/* not the first packet, add to list and adjust tail pointer */
		((struct eth_stm32_rx_buffer_header *)*pEnd)->next = header;
		*pEnd = header;
	}
}

/* Called by HAL_ETH_ReleaseTxPacket */
void HAL_ETH_TxFreeCallback(STM32_ETH_ARGS(ETH_HandleTypeDef *heth, uint32_t *buff))
{
	__ASSERT_NO_MSG(buff != NULL);

	/* buff is the user context in tx_config.pData */
	struct eth_stm32_tx_context *ctx = (struct eth_stm32_tx_context *)buff;
	struct eth_stm32_tx_buffer_header *buffer_header =
		&dma_tx_buffer_header[ctx->first_tx_buffer_index];

	while (buffer_header != NULL) {
		buffer_header->used = false;
		if (buffer_header->tx_buff.next != NULL) {
			buffer_header = CONTAINER_OF(buffer_header->tx_buff.next,
				struct eth_stm32_tx_buffer_header, tx_buff);
		} else {
			buffer_header = NULL;
		}
	}
	ctx->used = false;
}

/* allocate a tx buffer and mark it as used */
static inline uint16_t allocate_tx_buffer(void)
{
	for (;;) {
		for (uint16_t index = 0; index < ETH_TXBUFNB; index++) {
			if (!dma_tx_buffer_header[index].used) {
				dma_tx_buffer_header[index].used = true;
				return index;
			}
		}
		k_yield();
	}
}

#if defined(CONFIG_ETH_STM32_HAL_TX_ASYNC)
/* allocate a tx context and mark it as used, the first tx buffer is also allocated */
static struct eth_stm32_tx_context *allocate_tx_context_async(struct net_pkt *pkt)
{
	int tx_index;

	for (uint16_t index = 0; index < ETH_TX_DESC_CNT; index++) {
		if (!dma_tx_context[index].used) {
			dma_tx_context[index].used = true;
			dma_tx_context[index].pkt = pkt;
			tx_index = allocate_tx_buffer();
			if (tx_index < 0) {
				return NULL;
			}
			dma_tx_context[index].first_tx_buffer_index = tx_index;
			return &dma_tx_context[index];
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

	while (ctx == NULL) {
		ctx = allocate_tx_context_async(pkt);
		if (ctx == NULL) {
			k_sem_take(&dev_data->tx_int_sem, K_MSEC(ETH_DMA_TX_TIMEOUT_MS));
			hal_ret = HAL_ETH_ReleaseTxPacket(heth);
			__ASSERT_NO_MSG(hal_ret == HAL_OK);
		}
	}
	buf_header = &dma_tx_buffer_header[ctx->first_tx_buffer_index];

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

		const uint16_t next_buffer_id = allocate_tx_buffer();

		buf_header->tx_buff.len = ETH_STM32_TX_BUF_SIZE;
		/* append new buffer to the linked list */
		buf_header->tx_buff.next = &dma_tx_buffer_header[next_buffer_id].tx_buff;
		/* and adjust tail pointer */
		buf_header = &dma_tx_buffer_header[next_buffer_id];
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
	tx_config.TxBuffer = &dma_tx_buffer_header[ctx->first_tx_buffer_index].tx_buff;

	if (HAL_ETH_Transmit_IT(heth, &tx_config) != HAL_OK) {
		LOG_ERR("HAL_ETH_Transmit: failed!");
		res = -EIO;
	}

error:
	if (res < 0 && ctx) {
		/* We need to release the tx context and its buffers */
		HAL_ETH_TxFreeCallback((uint32_t *)ctx);
	}

	k_mutex_unlock(&dev_data->tx_mutex);

	return res;
}
#else

/* allocate a tx context and mark it as used, the first tx buffer is also allocated */
static inline struct eth_stm32_tx_context *allocate_tx_context(struct net_pkt *pkt)
{
	for (;;) {
		for (uint16_t index = 0; index < ETH_TX_DESC_CNT; index++) {
			if (!dma_tx_context[index].used) {
				dma_tx_context[index].used = true;
				dma_tx_context[index].pkt = pkt;
				dma_tx_context[index].first_tx_buffer_index = allocate_tx_buffer();
				return &dma_tx_context[index];
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

	ctx = allocate_tx_context(pkt);
	buf_header = &dma_tx_buffer_header[ctx->first_tx_buffer_index];

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
		const uint16_t next_buffer_id = allocate_tx_buffer();

		buf_header->tx_buff.len = ETH_STM32_TX_BUF_SIZE;
		/* append new buffer to the linked list */
		buf_header->tx_buff.next = &dma_tx_buffer_header[next_buffer_id].tx_buff;
		/* and adjust tail pointer */
		buf_header = &dma_tx_buffer_header[next_buffer_id];
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
	tx_config.TxBuffer = &dma_tx_buffer_header[ctx->first_tx_buffer_index].tx_buff;

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

	k_mutex_unlock(&dev_data->tx_mutex);

	return res;
}
#endif /* ETH_STM32_HAL_TX_ASYNC */

struct net_pkt *eth_stm32_rx(const struct device *dev)
{
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
					   total_len, AF_UNSPEC, 0, K_MSEC(100));
	if (!pkt) {
		LOG_ERR("Failed to obtain RX buffer");
		goto release_desc;
	}

	for (rx_header = (struct eth_stm32_rx_buffer_header *)appbuf;
			rx_header; rx_header = rx_header->next) {
		const size_t index = rx_header - &dma_rx_buffer_header[0];

		__ASSERT_NO_MSG(index < ETH_RXBUFNB);
		if (net_pkt_write(pkt, dma_rx_buffer[index], rx_header->size)) {
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
	__ASSERT_NO_MSG(heth_handle != NULL);

	struct eth_stm32_hal_dev_data *dev_data =
		CONTAINER_OF(heth_handle, struct eth_stm32_hal_dev_data, heth);

	__ASSERT_NO_MSG(dev_data != NULL);

	k_sem_give(&dev_data->tx_int_sem);

}

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

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet)
		if ((dma_error & ETH_DMA_RX_WATCHDOG_TIMEOUT_FLAG)   ||
			(dma_error & ETH_DMA_RX_PROCESS_STOPPED_FLAG)    ||
			(dma_error & ETH_DMA_RX_BUFFER_UNAVAILABLE_FLAG)) {
			eth_stats_update_errors_rx(dev_data->iface);
		}
		if ((dma_error & ETH_DMA_EARLY_TX_IT_FLAG) ||
			(dma_error & ETH_DMA_TX_PROCESS_STOPPED_FLAG)) {
			eth_stats_update_errors_tx(dev_data->iface);
		}
#else
		if ((dma_error & ETH_DMASR_RWTS) ||
			(dma_error & ETH_DMASR_RPSS) ||
			(dma_error & ETH_DMASR_RBUS)) {
			eth_stats_update_errors_rx(dev_data->iface);
		}
		if ((dma_error & ETH_DMASR_ETS)  ||
			(dma_error & ETH_DMASR_TPSS) ||
			(dma_error & ETH_DMASR_TJTS)) {
			eth_stats_update_errors_tx(dev_data->iface);
		}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet) */
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

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32mp13_ethernet)
	dev_data->stats.error_details.rx_crc_errors = heth->Instance->MMCRXCRCEPR;
	dev_data->stats.error_details.rx_align_errors = heth->Instance->MMCRXAEPR;
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet)
	dev_data->stats.error_details.rx_crc_errors = heth->Instance->MMCRCRCEPR;
	dev_data->stats.error_details.rx_align_errors = heth->Instance->MMCRAEPR;
#else
	dev_data->stats.error_details.rx_crc_errors = heth->Instance->MMCRFCECR;
	dev_data->stats.error_details.rx_align_errors = heth->Instance->MMCRFAECR;
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet) */

#endif /* CONFIG_NET_STATISTICS_ETHERNET */
}

int eth_stm32_hal_init(const struct device *dev)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;
	HAL_StatusTypeDef hal_ret = HAL_OK;
	__maybe_unused uint8_t *desc_uncached_addr;

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32n6_ethernet)
	for (int ch = 0; ch < ETH_DMA_CH_CNT; ch++) {
		heth->Init.TxDesc[ch] = dma_tx_desc_tab[ch];
		heth->Init.RxDesc[ch] = dma_rx_desc_tab[ch];
	}
#else
	heth->Init.TxDesc = dma_tx_desc_tab;
	heth->Init.RxDesc = dma_rx_desc_tab;
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
	if (hal_ret == HAL_TIMEOUT) {
		/* HAL Init time out. This could be linked to */
		/* a recoverable error. Log the issue and continue */
		/* driver initialisation */
		LOG_ERR("HAL_ETH_Init Timed out");
	} else if (hal_ret != HAL_OK) {
		LOG_ERR("HAL_ETH_Init failed: %d", hal_ret);
		return -EINVAL;
	}

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

	/* Initialize semaphores */
	k_mutex_init(&dev_data->tx_mutex);
	k_sem_init(&dev_data->rx_int_sem, 0, K_SEM_MAX_LIMIT);
	k_sem_init(&dev_data->tx_int_sem, 0, 1);

	/* Tx config init: */
	tx_config.Attributes = ETH_TX_PACKETS_FEATURES_CSUM |
				ETH_TX_PACKETS_FEATURES_CRCPAD;
	tx_config.ChecksumCtrl = IS_ENABLED(CONFIG_ETH_STM32_HW_CHECKSUM) ?
			ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC : ETH_CHECKSUM_DISABLE;
	tx_config.CRCPadCtrl = ETH_CRC_PAD_INSERT;

	/* prepare tx buffer header */
	for (uint16_t i = 0; i < ETH_TXBUFNB; ++i) {
		dma_tx_buffer_header[i].tx_buff.buffer = dma_tx_buffer[i];
	}

	return 0;
}

void eth_stm32_set_mac_config(const struct device *dev, struct phy_link_state *state)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;
	HAL_StatusTypeDef hal_ret = HAL_OK;
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

int eth_stm32_hal_start(const struct device *dev)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;
	HAL_StatusTypeDef hal_ret = HAL_OK;

	LOG_DBG("Starting ETH HAL driver");

	hal_ret = HAL_ETH_Start_IT(heth);

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

	hal_ret = HAL_ETH_Stop_IT(heth);

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
