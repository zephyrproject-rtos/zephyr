/*
 * Copyright (c) 2024 Daikin Comfort Technologies North America, Inc.
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_eusart_spi

#include <stdbool.h>
#include <stddef.h>

#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_silabs.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/dma/dma_silabs_ldma.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/device_runtime.h>
#include <em_cmu.h>
#include <em_eusart.h>

LOG_MODULE_REGISTER(spi_silabs_eusart, CONFIG_SPI_LOG_LEVEL);

/* Required by spi_context.h */
#include "spi_context.h"

#if defined(CONFIG_SPI_ASYNC) && !defined(CONFIG_SPI_SILABS_EUSART_DMA)
#warning "Silabs eusart SPI driver ASYNC without DMA is not supported"
#endif

#define SPI_WORD_SIZE 8
#ifdef CONFIG_SPI_SILABS_EUSART_DMA
#define SPI_DMA_MAX_DESCRIPTOR_TRANSFER_SIZE (0x800U)

struct dma_channel {
	const struct device *dma_dev;
	uint8_t dma_slot;
	int chan_nb;
	struct dma_block_config dma_descriptors[CONFIG_SPI_SILABS_EUSART_DMA_MAX_BLOCKS];
};
#endif

struct spi_silabs_eusart_data {
	struct spi_context ctx;
#ifdef CONFIG_SPI_SILABS_EUSART_DMA
	struct dma_channel dma_chan_rx;
	struct dma_channel dma_chan_tx;
#endif
};

struct spi_silabs_eusart_config {
	EUSART_TypeDef *base;
	const struct device *clock_dev;
	const struct silabs_clock_control_cmu_config clock_cfg;
	uint32_t clock_frequency;
	const struct pinctrl_dev_config *pcfg;
	uint8_t mosi_overrun;
};

#ifdef CONFIG_SPI_SILABS_EUSART_DMA
static volatile uint8_t empty_buffer;
#endif

static bool spi_silabs_eusart_is_dma_enabled_instance(const struct device *dev)
{
#ifdef CONFIG_SPI_SILABS_EUSART_DMA
	struct spi_silabs_eusart_data *data = dev->data;

	__ASSERT_NO_MSG(!!data->dma_chan_tx.dma_dev == !!data->dma_chan_rx.dma_dev);

	return data->dma_chan_rx.dma_dev != NULL;
#else
	return false;
#endif
}

static int spi_silabs_eusart_configure(const struct device *dev, const struct spi_config *config)
{
	struct spi_silabs_eusart_data *data = dev->data;
	const struct spi_silabs_eusart_config *eusart_cfg = dev->config;
	uint32_t spi_frequency;

	EUSART_SpiAdvancedInit_TypeDef eusartAdvancedSpiInit = EUSART_SPI_ADVANCED_INIT_DEFAULT;
	EUSART_SpiInit_TypeDef eusartInit = EUSART_SPI_MASTER_INIT_DEFAULT_HF;

	int err;

	if (spi_context_configured(&data->ctx, config)) {
		/* Already configured. No need to do it again, but must re-enable in case
		 * TXEN/RXEN were cleared.
		 */
		EUSART_Enable(eusart_cfg->base, eusartEnable);

		return 0;
	}

	err = clock_control_get_rate(eusart_cfg->clock_dev,
				     (clock_control_subsys_t)&eusart_cfg->clock_cfg,
				     &spi_frequency);
	if (err) {
		return err;
	}
	/* Max supported SPI frequency is half the source clock */
	spi_frequency /= 2;

	if (config->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	if (SPI_WORD_SIZE_GET(config->operation) != SPI_WORD_SIZE) {
		LOG_ERR("Word size must be %d", SPI_WORD_SIZE);
		return -ENOTSUP;
	}

	if (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES) &&
	    (config->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("Only supports single mode");
		return -ENOTSUP;
	}

	if (config->operation & SPI_OP_MODE_SLAVE) {
		LOG_ERR("Slave mode not supported");
		return -ENOTSUP;
	}

	/* Set frequency to the minimum of what the device supports, what the
	 * user has configured the controller to, and the max frequency for the
	 * transaction.
	 */
	if (eusart_cfg->clock_frequency > spi_frequency) {
		LOG_ERR("SPI clock-frequency too high");
		return -EINVAL;
	}
	spi_frequency = MIN(eusart_cfg->clock_frequency, spi_frequency);
	if (config->frequency) {
		spi_frequency = MIN(config->frequency, spi_frequency);
	}
	eusartInit.bitRate = spi_frequency;

	if (config->operation & SPI_MODE_LOOP) {
		eusartInit.loopbackEnable = eusartLoopbackEnable;
	} else {
		eusartInit.loopbackEnable = eusartLoopbackDisable;
	}

	/* Set Clock Mode */
	if (config->operation & SPI_MODE_CPOL) {
		if (config->operation & SPI_MODE_CPHA) {
			eusartInit.clockMode = eusartClockMode3;
		} else {
			eusartInit.clockMode = eusartClockMode2;
		}
	} else {
		if (config->operation & SPI_MODE_CPHA) {
			eusartInit.clockMode = eusartClockMode1;
		} else {
			eusartInit.clockMode = eusartClockMode0;
		}
	}

	if (config->operation & SPI_CS_ACTIVE_HIGH) {
		eusartAdvancedSpiInit.csPolarity = eusartCsActiveHigh;
	} else {
		eusartAdvancedSpiInit.csPolarity = eusartCsActiveLow;
	}

	eusartAdvancedSpiInit.msbFirst = !(config->operation & SPI_TRANSFER_LSB);
	eusartAdvancedSpiInit.autoCsEnable = !spi_cs_is_gpio(config);
	eusartInit.databits = eusartDataBits8;
	eusartInit.advancedSettings = &eusartAdvancedSpiInit;

#ifdef CONFIG_SPI_SILABS_EUSART_DMA
	if (spi_silabs_eusart_is_dma_enabled_instance(dev)) {
		if (!device_is_ready(data->dma_chan_tx.dma_dev)) {
			return -ENODEV;
		}

		eusartAdvancedSpiInit.TxFifoWatermark = eusartTxFiFoWatermark1Frame;
		eusartAdvancedSpiInit.RxFifoWatermark = eusartRxFiFoWatermark1Frame;

		if (data->dma_chan_rx.chan_nb < 0) {
			data->dma_chan_rx.chan_nb =
				dma_request_channel(data->dma_chan_rx.dma_dev, NULL);
		}

		if (data->dma_chan_rx.chan_nb < 0) {
			LOG_ERR("DMA channel request failed");
			return -EAGAIN;
		}

		if (data->dma_chan_tx.chan_nb < 0) {
			data->dma_chan_tx.chan_nb =
				dma_request_channel(data->dma_chan_tx.dma_dev, NULL);
		}

		if (data->dma_chan_tx.chan_nb < 0) {
			dma_release_channel(data->dma_chan_rx.dma_dev, data->dma_chan_rx.chan_nb);
			data->dma_chan_rx.chan_nb = -1;
			LOG_ERR("DMA channel request failed");
			return -EAGAIN;
		}
	}
#endif
	/* Enable EUSART clock */
	err = clock_control_on(eusart_cfg->clock_dev,
			       (clock_control_subsys_t)&eusart_cfg->clock_cfg);
	if (err < 0 && err != -EALREADY) {
		goto exit;
	}

	/* Initialize the EUSART */
	EUSART_SpiInit(eusart_cfg->base, &eusartInit);

	data->ctx.config = config;

	return 0;

exit:
#ifdef CONFIG_SPI_SILABS_EUSART_DMA
	if (spi_silabs_eusart_is_dma_enabled_instance(dev)) {
		dma_release_channel(data->dma_chan_rx.dma_dev, data->dma_chan_rx.chan_nb);
		dma_release_channel(data->dma_chan_tx.dma_dev, data->dma_chan_tx.chan_nb);
		data->dma_chan_rx.chan_nb = -1;
		data->dma_chan_tx.chan_nb = -1;
	}
#endif
	return err;
}

static inline void spi_silabs_eusart_pm_policy_get(const struct device *dev)
{
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
}

static inline void spi_silabs_eusart_pm_policy_put(const struct device *dev)
{
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
}

static int spi_silabs_eusart_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct spi_silabs_eusart_config *eusart_config = dev->config;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = clock_control_on(eusart_config->clock_dev,
				       (clock_control_subsys_t)&eusart_config->clock_cfg);

		if (ret == -EALREADY) {
			ret = 0;
		} else if (ret < 0) {
			break;
		}

		pinctrl_apply_state(eusart_config->pcfg, PINCTRL_STATE_DEFAULT);

		break;
	case PM_DEVICE_ACTION_SUSPEND:
		pinctrl_apply_state(eusart_config->pcfg, PINCTRL_STATE_SLEEP);

		EUSART_Enable(eusart_config->base, eusartDisable);
		ret = clock_control_off(eusart_config->clock_dev,
					(clock_control_subsys_t)&eusart_config->clock_cfg);
		if (ret == -EALREADY) {
			ret = 0;
		}

		break;
	default:
		ret = -ENOTSUP;
	}

	return ret;
}

#ifdef CONFIG_SPI_SILABS_EUSART_DMA
static void spi_silabs_dma_rx_callback(const struct device *dev, void *user_data, uint32_t channel,
				       int status)
{
	const struct device *spi_dev = (const struct device *)user_data;
	struct spi_silabs_eusart_data *data = spi_dev->data;
	struct spi_context *instance_ctx = &data->ctx;

	ARG_UNUSED(dev);

	if (status >= 0 && status != DMA_STATUS_COMPLETE) {
		return;
	}

	if (status < 0) {
		dma_stop(data->dma_chan_tx.dma_dev, data->dma_chan_tx.chan_nb);
		dma_stop(data->dma_chan_rx.dma_dev, data->dma_chan_rx.chan_nb);
	}

	spi_context_cs_control(instance_ctx, false);
	spi_silabs_eusart_pm_policy_put(spi_dev);
	spi_context_complete(instance_ctx, spi_dev, status);
}

static void spi_silabs_eusart_clear_txrx_fifos(EUSART_TypeDef *eusart)
{
	sys_write32(EUSART_CMD_CLEARTX, (mem_addr_t)&eusart->CMD_SET);

	while (sys_read32((mem_addr_t)&eusart->STATUS) & EUSART_STATUS_RXFL) {
		(void)sys_read32((mem_addr_t)&eusart->RXDATA);
	}

	while (sys_read32((mem_addr_t)&eusart->STATUS) & EUSART_STATUS_CLEARTXBUSY) {
	}
}

static size_t spi_silabs_longest_transfer_size(struct spi_context *instance_ctx)
{
	uint32_t tx_transfer_size = spi_context_total_tx_len(instance_ctx);
	uint32_t rx_transfer_size = spi_context_total_rx_len(instance_ctx);

	return MAX(tx_transfer_size, rx_transfer_size);
}

static int spi_silabs_dma_config(const struct device *dev,
				 struct dma_channel *channel,
				 uint32_t block_count, bool is_tx)
{
	struct dma_config cfg = {
		.channel_direction = is_tx ? MEMORY_TO_PERIPHERAL : PERIPHERAL_TO_MEMORY,
		.complete_callback_en = 0,
		.source_data_size = 1,
		.dest_data_size = 1,
		.source_burst_length = 1,
		.dest_burst_length = 1,
		.block_count = block_count,
		.head_block = channel->dma_descriptors,
		.dma_slot = channel->dma_slot,
		.dma_callback = !is_tx ? &spi_silabs_dma_rx_callback : NULL,
		.user_data = (void *)dev,
	};

	return dma_config(channel->dma_dev, channel->chan_nb, &cfg);
}

static uint32_t spi_eusart_fill_desc(const struct spi_silabs_eusart_config *cfg,
				     struct dma_block_config *new_blk_cfg, uint8_t *buffer,
				     size_t requested_transaction_size, bool is_tx)
{
	/* Set-up source and destination address with increment behavior */
	if (is_tx) {
		new_blk_cfg->dest_address = (uint32_t)&cfg->base->TXDATA;
		new_blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		if (buffer) {
			new_blk_cfg->source_address = (uint32_t)buffer;
			new_blk_cfg->source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		} else {
			/* Null buffer pointer means sending dummy byte */
			new_blk_cfg->source_address = (uint32_t)&(cfg->mosi_overrun);
			new_blk_cfg->source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		}
	} else {
		new_blk_cfg->source_address = (uint32_t)&cfg->base->RXDATA;
		new_blk_cfg->source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		if (buffer) {
			new_blk_cfg->dest_address = (uint32_t)buffer;
			new_blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		} else {
			/* Null buffer pointer means rx to null byte */
			new_blk_cfg->dest_address = (uint32_t)&empty_buffer;
			new_blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		}
	}
	/* Setup max transfer according to requested transaction size.
	 * Will top if bigger than the maximum transfer size.
	 */
	new_blk_cfg->block_size = MIN(requested_transaction_size,
				      SPI_DMA_MAX_DESCRIPTOR_TRANSFER_SIZE);
	return new_blk_cfg->block_size;
}

struct dma_block_config *spi_eusart_fill_data_desc(const struct spi_silabs_eusart_config *cfg,
						   struct dma_block_config *desc,
						   const struct spi_buf buffers[],
						   int buffer_count,
						   size_t transaction_len,
						   bool is_tx)
{
	__ASSERT(transaction_len > 0, "Not supported");

	size_t offset = 0;
	int i = 0;
	uint8_t *buffer = NULL;

	while (i != buffer_count) {
		if (!buffers[i].len) {
			i++;
			continue;
		}
		if (!desc) {
			return NULL;
		}
		buffer = buffers[i].buf ? (uint8_t *)buffers[i].buf + offset : NULL;
		offset += spi_eusart_fill_desc(cfg, desc,
					       buffer,
					       buffers[i].len - offset,
					       is_tx);
		if (offset == buffers[i].len) {
			transaction_len -= offset;
			offset = 0;
			i++;
		}
		if (transaction_len) {
			desc = desc->next_block;
		}
	}

	while (transaction_len) {
		if (!desc) {
			return NULL;
		}

		transaction_len -= spi_eusart_fill_desc(cfg, desc, NULL, transaction_len, is_tx);
		if (transaction_len) {
			desc = desc->next_block;
		}
	}

	desc->next_block = NULL;
	return desc;
}

static void spi_eusart_reset_desc(struct dma_channel *channel)
{
	int i;

	memset(channel->dma_descriptors, 0, sizeof(channel->dma_descriptors));
	for (i = 0; i < ARRAY_SIZE(channel->dma_descriptors) - 1; i++) {
		channel->dma_descriptors[i].next_block = &channel->dma_descriptors[i + 1];
	}
}

static int spi_eusart_prepare_dma_channel(const struct device *spi_dev,
					  const struct spi_buf *buffer,
					  size_t buffer_count,
					  struct dma_channel *channel,
					  size_t padded_transaction_size,
					  bool is_tx)
{
	const struct spi_silabs_eusart_config *cfg = spi_dev->config;
	struct dma_block_config *desc;
	int ret = 0;

	spi_eusart_reset_desc(channel);
	desc = spi_eusart_fill_data_desc(cfg, channel->dma_descriptors,
					 buffer, buffer_count, padded_transaction_size, is_tx);
	if (!desc) {
		return -ENOMEM;
	}

	ret = spi_silabs_dma_config(spi_dev, channel,
				    ARRAY_INDEX(channel->dma_descriptors, desc),
				    is_tx);

	return ret;
}

static int spi_eusart_prepare_dma_transaction(const struct device *dev,
					      size_t padded_transaction_size)
{
	int ret;
	struct spi_silabs_eusart_data *data = dev->data;

	if (padded_transaction_size == 0) {
		/* Nothing to do */
		return 0;
	}

	ret = spi_eusart_prepare_dma_channel(dev, data->ctx.current_tx, data->ctx.tx_count,
					     &data->dma_chan_tx, padded_transaction_size,
					     true);
	if (ret) {
		return ret;
	}

	ret = spi_eusart_prepare_dma_channel(dev, data->ctx.current_rx, data->ctx.rx_count,
					     &data->dma_chan_rx, padded_transaction_size, false);
	return ret;
}

#endif

static void spi_silabs_eusart_send(EUSART_TypeDef *eusart, uint8_t frame)
{
	/* Write frame to register */
	EUSART_Tx(eusart, frame);

	/* Wait until the transfer ends */
	while (!(eusart->STATUS & EUSART_STATUS_TXC)) {
	}
}

static uint8_t spi_silabs_eusart_recv(EUSART_TypeDef *eusart)
{
	/* Return data inside rx register */
	return EUSART_Rx(eusart);
}

static bool spi_silabs_eusart_transfer_ongoing(struct spi_silabs_eusart_data *data)
{
	return spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx);
}

static inline uint8_t spi_silabs_eusart_next_tx(struct spi_silabs_eusart_data *data)
{
	uint8_t tx_frame = 0;

	if (spi_context_tx_buf_on(&data->ctx)) {
		tx_frame = UNALIGNED_GET((uint8_t *)(data->ctx.tx_buf));
	}

	return tx_frame;
}

static int spi_silabs_eusart_shift_frames(EUSART_TypeDef *eusart,
					  struct spi_silabs_eusart_data *data)
{
	uint8_t tx_frame;
	uint8_t rx_frame;

	tx_frame = spi_silabs_eusart_next_tx(data);
	spi_silabs_eusart_send(eusart, tx_frame);
	spi_context_update_tx(&data->ctx, 1, 1);

	rx_frame = spi_silabs_eusart_recv(eusart);

	if (spi_context_rx_buf_on(&data->ctx)) {
		UNALIGNED_PUT(rx_frame, (uint8_t *)data->ctx.rx_buf);
	}

	spi_context_update_rx(&data->ctx, 1, 1);

	return 0;
}

static int spi_silabs_eusart_xfer_dma(const struct device *dev, const struct spi_config *config)
{
#ifdef CONFIG_SPI_SILABS_EUSART_DMA
	const struct spi_silabs_eusart_config *eusart_config = dev->config;
	struct spi_silabs_eusart_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret = 0;

	size_t padded_transaction_size = spi_silabs_longest_transfer_size(ctx);

	if (padded_transaction_size == 0) {
		return -EINVAL;
	}

	spi_silabs_eusart_clear_txrx_fifos(eusart_config->base);

	ret = spi_eusart_prepare_dma_transaction(dev, padded_transaction_size);
	if (ret) {
		return ret;
	}

	spi_silabs_eusart_pm_policy_get(dev);

	spi_context_cs_control(ctx, true);

	/* RX channel needs to be ready before TX channel actually starts */
	ret = dma_start(data->dma_chan_rx.dma_dev, data->dma_chan_rx.chan_nb);
	if (ret) {
		goto force_transaction_close;
	}

	ret = dma_start(data->dma_chan_tx.dma_dev, data->dma_chan_tx.chan_nb);
	if (ret) {

		goto force_transaction_close;
	}

	ret = spi_context_wait_for_completion(&data->ctx);
	if (ret < 0) {
		goto force_transaction_close;
	}

	/* Successful transaction. DMA transfer done interrupt ended the transaction. */
	return 0;
force_transaction_close:
	dma_stop(data->dma_chan_rx.dma_dev, data->dma_chan_rx.chan_nb);
	dma_stop(data->dma_chan_tx.dma_dev, data->dma_chan_tx.chan_nb);
	spi_context_cs_control(ctx, false);
	spi_silabs_eusart_pm_policy_put(dev);
	return ret;
#else
	return -ENOTSUP;
#endif
}

static int spi_silabs_eusart_xfer_polling(const struct device *dev,
					  const struct spi_config *config)
{
	const struct spi_silabs_eusart_config *eusart_config = dev->config;
	struct spi_silabs_eusart_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret;

	spi_silabs_eusart_pm_policy_get(dev);
	spi_context_cs_control(ctx, true);

	ret = 0;
	while (!ret && spi_silabs_eusart_transfer_ongoing(data)) {
		ret = spi_silabs_eusart_shift_frames(eusart_config->base, data);
	}

	spi_context_cs_control(ctx, false);
	spi_context_complete(ctx, dev, 0);

	spi_silabs_eusart_pm_policy_put(dev);
	return ret;
}

static int spi_silabs_eusart_transceive(const struct device *dev,
					const struct spi_config *config,
					const struct spi_buf_set *tx_bufs,
					const struct spi_buf_set *rx_bufs,
					bool asynchronous,
					spi_callback_t cb,
					void *userdata)
{
	struct spi_silabs_eusart_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret;

	spi_context_lock(ctx, asynchronous, cb, userdata, config);

	ret = spi_silabs_eusart_configure(dev, config);
	if (ret) {
		goto out;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);

	if (spi_silabs_eusart_is_dma_enabled_instance(dev)) {
		/* DMA transfer handle a/synchronous transfers */
		ret = spi_silabs_eusart_xfer_dma(dev, config);
	} else if (!asynchronous) {
		ret = spi_silabs_eusart_xfer_polling(dev, config);
	} else {
		/* Asynchronous transfers without DMA is not implemented,
		 * please configure the device tree
		 * instance with the proper DMA configuration.
		 */
		ret = -ENOTSUP;
	}

out:
	spi_context_release(ctx, ret);

	return ret;
}

/* API Functions */
static int spi_silabs_eusart_init(const struct device *dev)
{
	struct spi_silabs_eusart_data *data = dev->data;
	int err;

	err = spi_context_cs_configure_all(&data->ctx);
	if (err < 0) {
		return err;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return pm_device_driver_init(dev, spi_silabs_eusart_pm_action);
}

static int spi_silabs_eusart_transceive_sync(const struct device *dev,
					     const struct spi_config *config,
					     const struct spi_buf_set *tx_bufs,
					     const struct spi_buf_set *rx_bufs)
{
	return spi_silabs_eusart_transceive(dev,
					    config,
					    tx_bufs,
					    rx_bufs,
					    false,
					    NULL,
					    NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_silabs_eusart_transceive_async(const struct device *dev,
					      const struct spi_config *config,
					      const struct spi_buf_set *tx_bufs,
					      const struct spi_buf_set *rx_bufs,
					      spi_callback_t cb,
					      void *userdata)
{
	return spi_silabs_eusart_transceive(dev, config, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif

static int spi_silabs_eusart_release(const struct device *dev, const struct spi_config *config)
{
	const struct spi_silabs_eusart_config *eusart_config = dev->config;
	struct spi_silabs_eusart_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	if (!(eusart_config->base->STATUS & EUSART_STATUS_TXIDLE)) {
		return -EBUSY;
	}

	return 0;
}

/* Device Instantiation */
static DEVICE_API(spi, spi_silabs_eusart_api) = {
	.transceive = spi_silabs_eusart_transceive_sync,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_silabs_eusart_transceive_async,
#endif
	.release = spi_silabs_eusart_release,
};

#ifdef CONFIG_SPI_SILABS_EUSART_DMA
#define SPI_SILABS_EUSART_DMA_CHANNEL_INIT(index, dir)						\
	.dma_chan_##dir = {									\
		.chan_nb = -1,									\
		.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(index, dir)),		\
		.dma_slot =									\
			SILABS_LDMA_REQSEL_TO_SLOT(DT_INST_DMAS_CELL_BY_NAME(index, dir, slot)),\
	},
#define SPI_SILABS_EUSART_DMA_CHANNEL(index, dir) \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(index, dmas), \
		    (SPI_SILABS_EUSART_DMA_CHANNEL_INIT(index, dir)), ())
#else
#define SPI_SILABS_EUSART_DMA_CHANNEL(index, dir)
#endif

#define SPI_INIT(n) \
	PINCTRL_DT_INST_DEFINE(n); \
	static struct spi_silabs_eusart_data spi_silabs_eusart_data_##n = { \
		SPI_CONTEXT_INIT_LOCK(spi_silabs_eusart_data_##n, ctx), \
		SPI_CONTEXT_INIT_SYNC(spi_silabs_eusart_data_##n, ctx), \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx) \
		SPI_SILABS_EUSART_DMA_CHANNEL(n, rx) \
		SPI_SILABS_EUSART_DMA_CHANNEL(n, tx) \
	}; \
	static struct spi_silabs_eusart_config spi_silabs_eusart_cfg_##n = { \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n), \
		.base = (EUSART_TypeDef *)DT_INST_REG_ADDR(n), \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)), \
		.clock_cfg = SILABS_DT_INST_CLOCK_CFG(n), \
		.mosi_overrun = (uint8_t)SPI_MOSI_OVERRUN_DT(n), \
		.clock_frequency = DT_INST_PROP_OR(n, clock_frequency, 1000000), \
	}; \
	PM_DEVICE_DT_INST_DEFINE(n, spi_silabs_eusart_pm_action); \
	SPI_DEVICE_DT_INST_DEFINE(n, spi_silabs_eusart_init, PM_DEVICE_DT_INST_GET(n), \
				  &spi_silabs_eusart_data_##n, &spi_silabs_eusart_cfg_##n, \
				  POST_KERNEL, CONFIG_SPI_INIT_PRIORITY, &spi_silabs_eusart_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_INIT)
