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

#include "spi_context.h"

#if defined(CONFIG_SPI_ASYNC) && !defined(CONFIG_SPI_SILABS_EUSART_DMA)
#warning "Silabs eusart SPI driver ASYNC without DMA is not supported"
#endif

/* Macro Declarations */
#define SPI_WORD_SIZE 8
#ifdef CONFIG_SPI_SILABS_EUSART_DMA
#define SPI_DMA_MAX_DESCRIPTOR_TRANSFER_SIZE (0x800U)

struct dma_channel {
	const struct device *dma_dev;
	uint8_t dma_slot;
	int chan_nb;
	struct dma_config dma_config;
	struct dma_block_config dma_descriptors[CONFIG_SPI_SILABS_EUSART_DMA_MAX_BLOCKS];
};

struct dma_block_iterator {
	struct dma_block_config *blk_array;
	size_t block_idx;
	size_t current_total_transfer_size;
};
#endif

/* Structure Declarations */
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
	volatile uint8_t mosi_overrun;
};

/* Global variable */
#ifdef CONFIG_SPI_SILABS_EUSART_DMA
static volatile uint8_t empty_buffer;
#endif

/* Function declaration */
#ifdef CONFIG_SPI_SILABS_EUSART_DMA
void spi_silabs_dma_rx_callback(const struct device *dev, void *user_data, uint32_t channel,
				int status);
#endif

/* Helper Functions */
static inline bool is_dma_enabled_instance(const struct device *dev)
{
#ifdef CONFIG_SPI_SILABS_EUSART_DMA
	struct spi_silabs_eusart_data *data = dev->data;

	return data->dma_chan_rx.dma_dev != NULL;
#else
	return false;
#endif
}

static int spi_silabs_eusart_configure(const struct device *dev, const struct spi_config *config)
{
	struct spi_silabs_eusart_data *data = dev->data;
	const struct spi_silabs_eusart_config *eusart_config = dev->config;
	uint32_t spi_frequency;

	EUSART_SpiAdvancedInit_TypeDef eusartAdvancedSpiInit = EUSART_SPI_ADVANCED_INIT_DEFAULT;
	EUSART_SpiInit_TypeDef eusartInit = EUSART_SPI_MASTER_INIT_DEFAULT_HF;

	int err;

	if (spi_context_configured(&data->ctx, config)) {
		return 0;
	}

	err = clock_control_get_rate(eusart_config->clock_dev,
				     (clock_control_subsys_t)&eusart_config->clock_cfg,
				     &spi_frequency);
	if (err) {
		return err;
	}

	/* Max supported SPI frequency is half the source clock */
	spi_frequency /= 2;

	if (spi_context_configured(&data->ctx, config)) {
		/* Already configured. No need to do it again, but must re-enable in case
		 * TXEN/RXEN were cleared.
		 */
		EUSART_Enable(eusart_config->base, eusartEnable);

		return 0;
	}

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
	if (eusart_config->clock_frequency > spi_frequency) {
		LOG_ERR("SPI clock-frequency too high");
		return -EINVAL;
	}
	spi_frequency = MIN(eusart_config->clock_frequency, spi_frequency);
	if (config->frequency) {
		spi_frequency = MIN(config->frequency, spi_frequency);
	}
	eusartInit.bitRate = spi_frequency;

	if (config->operation & SPI_MODE_LOOP) {
		eusartInit.loopbackEnable = eusartLoopbackEnable;
	} else {
		eusartInit.loopbackEnable = eusartLoopbackDisable;
	}

	/* Set clock mode */
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
	if (is_dma_enabled_instance(dev)) {
		if (!device_is_ready(data->dma_chan_tx.dma_dev)
		    || data->dma_chan_tx.dma_dev == NULL) {
			return -ENODEV;
		}

		eusartAdvancedSpiInit.TxFifoWatermark = eusartTxFiFoWatermark1Frame;
		eusartAdvancedSpiInit.RxFifoWatermark = eusartRxFiFoWatermark1Frame;

		if (data->dma_chan_rx.chan_nb < 0) {
			data->dma_chan_rx.chan_nb = dma_request_channel(data->dma_chan_rx.dma_dev,
									      NULL);
		}

		if (data->dma_chan_rx.chan_nb < 0) {
			LOG_ERR("DMA channel request failed");
			return -EAGAIN;
		}

		if (data->dma_chan_tx.chan_nb < 0) {
			data->dma_chan_tx.chan_nb = dma_request_channel(data->dma_chan_tx.dma_dev,
									      NULL);
		}

		if (data->dma_chan_tx.chan_nb < 0) {
			dma_release_channel(data->dma_chan_rx.dma_dev,
					    data->dma_chan_rx.chan_nb);
			LOG_ERR("DMA channel request failed");
			return -EAGAIN;
		}
	}
#endif
	/* Enable EUSART clock */
	err = clock_control_on(eusart_config->clock_dev,
			       (clock_control_subsys_t)&eusart_config->clock_cfg);

	if (err < 0 && err != -EALREADY) {
		return err;
	}

	/* Initialize the EUSART */
	EUSART_SpiInit(eusart_config->base, &eusartInit);

	data->ctx.config = config;

	return 0;
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

static int spi_silabs_eusart_pm_action(const struct device *dev,
						     enum pm_device_action action)
{
	const struct spi_silabs_eusart_config *eusart_config = dev->config;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = clock_control_on(eusart_config->clock_dev,
				       (clock_control_subsys_t)&eusart_config->clock_cfg);

		if (ret == -EALREADY) {
			/* Ignore if clock is already on */
			ret = 0U;
		} else if (ret < 0) {
			return ret;
		}

		pinctrl_apply_state(eusart_config->pcfg, PINCTRL_STATE_DEFAULT);

		break;
	case PM_DEVICE_ACTION_SUSPEND:
		pinctrl_apply_state(eusart_config->pcfg, PINCTRL_STATE_SLEEP);

		EUSART_Enable(eusart_config->base, eusartDisable);
		ret = clock_control_off(eusart_config->clock_dev,
					(clock_control_subsys_t)&eusart_config->clock_cfg);

		if (ret == -EALREADY) {
			/* Ignore if clock is already on */
			ret = 0U;
		} else if (ret < 0) {
			return ret;
		}

		break;
	default:
		ret = -ENOTSUP;
	}

	return ret;
}

#ifdef CONFIG_SPI_SILABS_EUSART_DMA
static inline void spi_silabs_eusart_clear_txrx_fifos(EUSART_TypeDef *eusart)
{
	eusart->CMD_SET = EUSART_CMD_CLEARTX;

	while (sys_read32((mem_addr_t)&(eusart->STATUS)) & EUSART_STATUS_RXFL) {
		(void)sys_read32((mem_addr_t)&(eusart->RXDATA));
	}

	while (sys_read32((mem_addr_t)&(eusart->STATUS)) & EUSART_STATUS_CLEARTXBUSY) {
	}
}

static inline size_t spi_silabs_longest_transfer_size(struct spi_context *instance_ctx)
{
	uint32_t tx_transfer_size = spi_context_total_tx_len(instance_ctx);
	uint32_t rx_transfer_size = spi_context_total_rx_len(instance_ctx);

	return MAX(tx_transfer_size, rx_transfer_size);
}

static void link_dma_transfer_blocks(struct dma_channel *channel, size_t block_count)
{
	struct dma_block_config *cur_blk = &(channel->dma_descriptors[0U]);

	for (size_t i = 0U; i < CONFIG_SPI_SILABS_EUSART_DMA_MAX_BLOCKS; i++) {
		/* If has a next descriptor */
		if ((i + 1U) < block_count) {
			cur_blk->next_block = &(channel->dma_descriptors[i + 1U]);
		} else {
			/* Is last dma descriptor */
			cur_blk->next_block = NULL;
		}

		cur_blk++;
	}
}

static uint32_t
spi_silabs_build_dma_block(const struct spi_silabs_eusart_config *instance_config,
				    struct dma_block_config *new_blk_cfg, uint8_t *buffer,
				    size_t requested_transaction_size, bool is_tx)
{
	memset(new_blk_cfg, 0, sizeof(*new_blk_cfg));
	size_t remaining_transaction_size = 0U;

	/* Set-up source and destination address with increment behavior */
	if (is_tx) {
		new_blk_cfg->dest_address = (uint32_t)&instance_config->base->TXDATA;
		new_blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		if (buffer) {
			new_blk_cfg->source_address = (uint32_t)buffer;
			new_blk_cfg->source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		} else {
			/* Null buffer pointer means sending dummy byte */
			new_blk_cfg->source_address = (uint32_t)&(instance_config->mosi_overrun);
			new_blk_cfg->source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		}
	} else {
		new_blk_cfg->source_address = (uint32_t)&instance_config->base->RXDATA;
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
	if (requested_transaction_size <= SPI_DMA_MAX_DESCRIPTOR_TRANSFER_SIZE) {
		new_blk_cfg->block_size = requested_transaction_size;
		remaining_transaction_size = 0U;
	} else {
		new_blk_cfg->block_size = SPI_DMA_MAX_DESCRIPTOR_TRANSFER_SIZE;
		remaining_transaction_size = requested_transaction_size - new_blk_cfg->block_size;
	}

	/* Return how much of the requested transaction is still required to be processed */
	return remaining_transaction_size;
}

static void spi_silabs_build_dma_config(const struct device *dev, struct dma_channel *channel,
					uint32_t block_count, bool is_tx)
{
	struct dma_config *current_dma_config = &(channel->dma_config);

	memset(current_dma_config, 0, sizeof(channel->dma_config));

	if (is_tx) {
		/* Memory to peripheral */
		current_dma_config->channel_direction = MEMORY_TO_PERIPHERAL;
	} else {
		/* Peripheral to memory */
		current_dma_config->channel_direction = PERIPHERAL_TO_MEMORY;
	}

	/* Invoke callback at the end of transfer list */
	current_dma_config->complete_callback_en = 0U;

	current_dma_config->source_data_size = 1U; /* 1 byte */
	current_dma_config->dest_data_size = 1U;   /* 1 byte */

	current_dma_config->source_burst_length = 1U;
	current_dma_config->dest_burst_length = 1U;

	current_dma_config->block_count = block_count;

	current_dma_config->head_block = (struct dma_block_config *)channel->dma_descriptors;

	current_dma_config->user_data = (void *)dev;

	current_dma_config->dma_slot = channel->dma_slot;

	link_dma_transfer_blocks(channel, block_count);

	if (!is_tx) {
		/* Only enable the end transaction callback. 
		 * Only SPI controller mode is supported.
		 */
		current_dma_config->dma_callback = &spi_silabs_dma_rx_callback;
	}
}

static int spi_silabs_create_dma_blocks_from_buffer(const struct spi_buf *buffer,
					 const struct spi_silabs_eusart_config *instance_conf,
					 struct dma_block_iterator *out_blk_it,
					 bool is_tx)
{
	size_t buf_remaining_cnt = buffer->len;

	if (buf_remaining_cnt == 0U) {
		/* Skip zero length buffers */
		return 0;
	}

	out_blk_it->current_total_transfer_size += buffer->len;

	/* Create the required number of dma transfer block for this buffer */
	while (out_blk_it->block_idx <= CONFIG_SPI_SILABS_EUSART_DMA_MAX_BLOCKS) {
		if (out_blk_it->block_idx >= CONFIG_SPI_SILABS_EUSART_DMA_MAX_BLOCKS) {
			LOG_ERR("DMA channel transfer blocks exausted. Can't prepare the "
				"transaction.");
			return -ENOMEM;
		}

		uint32_t buf_index = buffer->len - buf_remaining_cnt;
		uint8_t *buffer_start_address = buffer->buf ? 
						&(((uint8_t *)buffer->buf)[buf_index])
						: NULL;
		struct dma_block_config *blk_cfg = &(out_blk_it->blk_array[out_blk_it->block_idx]);

		buf_remaining_cnt = spi_silabs_build_dma_block(instance_conf,
							       blk_cfg,
							       buffer_start_address,
							       buf_remaining_cnt,
							       is_tx);

		out_blk_it->block_idx++;

		if (buf_remaining_cnt == 0U) {
			break;
		}
	}

	return 0;
}

static int spi_silabs_create_padding_dma_blocks(const struct spi_silabs_eusart_config *instance_conf,
			      size_t total_padded_transaction_size,
			      struct dma_block_iterator *out_blk_it,
			      bool is_tx)
{
	total_padded_transaction_size -= out_blk_it->current_total_transfer_size;

	while (total_padded_transaction_size) {
		if (out_blk_it->block_idx >= CONFIG_SPI_SILABS_EUSART_DMA_MAX_BLOCKS) {
			LOG_ERR("DMA channel transfer blocks exausted. Can't prepare the "
				"transaction.");
			return -ENOMEM;
		}

		struct dma_block_config *blk_cfg = &(out_blk_it->blk_array[out_blk_it->block_idx]);
		total_padded_transaction_size = spi_silabs_build_dma_block(instance_conf,
									   blk_cfg,
									   NULL,
									   total_padded_transaction_size,
									   is_tx);
		out_blk_it->block_idx++;
	}

	return 0;
}

static int spi_silabs_prepare_dma_transfer(const struct device *spi_dev,
					   size_t total_padded_transaction_size,
					   bool is_tx)
{
	struct spi_silabs_eusart_data *instance_data = spi_dev->data;
	const struct spi_silabs_eusart_config *instance_conf = spi_dev->config;
	struct dma_channel *channel = is_tx ? &(instance_data->dma_chan_tx) :
					      &(instance_data->dma_chan_rx);
	struct dma_block_iterator blk_it = { .blk_array = &(channel->dma_descriptors[0]),
					     .block_idx = 0U,
					     .current_total_transfer_size = 0U };
	const struct spi_buf *buffer = NULL;
	size_t buffer_count = 0U;
	int ret = 0;

	if (is_tx) {
		buffer = &instance_data->ctx.current_tx[0];
		buffer_count = instance_data->ctx.tx_count;
	} else {
		buffer = &instance_data->ctx.current_rx[0];
		buffer_count = instance_data->ctx.rx_count;
	}

	/* For each buffer in the instance context buffer set */
	for (size_t i = 0U; i < buffer_count; i++) {
		ret = spi_silabs_create_dma_blocks_from_buffer(&(buffer[i]),
							       spi_dev->config,
							       &blk_it,
							       is_tx);
		if (ret < 0) {
			return ret;
		}
	}

	ret = spi_silabs_create_padding_dma_blocks(instance_conf,
						   total_padded_transaction_size,
						   &blk_it,
						   is_tx);

	if (ret < 0) {
		return ret;
	}

	spi_silabs_build_dma_config(spi_dev, channel, blk_it.block_idx, is_tx);

	ret = dma_config(channel->dma_dev, channel->chan_nb, &(channel->dma_config));

	if (ret == -ENOMEM) {
		LOG_ERR("DMA driver failed allocation.");
		return ret;
	}

	return ret;
}

static int spi_silabs_prepare_dma_transaction(const struct device *dev,
					      size_t total_padded_transaction_size)
{
	int ret;

	if (total_padded_transaction_size == 0) {
		/* Nothing to do */
		return 0U;
	}

	ret = spi_silabs_prepare_dma_transfer(dev, total_padded_transaction_size, true);

	if (ret) {
		return ret;
	}

	ret = spi_silabs_prepare_dma_transfer(dev, total_padded_transaction_size, false);

	return ret;
}

void spi_silabs_dma_rx_callback(const struct device *dev, void *user_data, uint32_t channel,
				int status)
{
	ARG_UNUSED(dev);

	if (status >= 0 && status != DMA_STATUS_COMPLETE) {
		return;
	}

	const struct device *spi_dev = (const struct device *)user_data;
	struct spi_silabs_eusart_data *instance_data = spi_dev->data;
	struct spi_context *instance_ctx = &instance_data->ctx;

	if (status < 0U) {
		dma_stop(instance_data->dma_chan_tx.dma_dev,
				instance_data->dma_chan_tx.chan_nb);
		dma_stop(instance_data->dma_chan_rx.dma_dev,
				instance_data->dma_chan_rx.chan_nb);
	}

	spi_context_cs_control(instance_ctx, false);
	spi_silabs_eusart_pm_policy_put(spi_dev);
	spi_context_complete(instance_ctx, spi_dev, status);
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
	uint8_t tx_frame = 0U;

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
	int ret;

	size_t total_padded_transaction_size = spi_silabs_longest_transfer_size(ctx);

	spi_silabs_eusart_clear_txrx_fifos(eusart_config->base);

	ret = spi_silabs_prepare_dma_transaction(dev, total_padded_transaction_size);

	if (ret) {
		return ret;
	}

	spi_silabs_eusart_pm_policy_get(dev);

	spi_context_cs_control(ctx, true);

	if (total_padded_transaction_size == 0U) {
		goto force_transaction_close;
	}

	/* RX channel needs to be ready before TX channel actually starts */
	ret = dma_start(data->dma_chan_rx.dma_dev, data->dma_chan_rx.chan_nb);

	if (ret) {
		goto force_transaction_close;
	}

	ret = dma_start(data->dma_chan_tx.dma_dev, data->dma_chan_tx.chan_nb);

	if (ret) {
		dma_stop(data->dma_chan_rx.dma_dev, data->dma_chan_rx.chan_nb);
		goto force_transaction_close;
	}

	ret = spi_context_wait_for_completion(&data->ctx);

	if (ret >= 0) {
		/* Successful transaction. DMA transfer done interrupt ended the transaction. */
		return 0;
	}

force_transaction_close:
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
	int ret = 0;

	spi_silabs_eusart_pm_policy_get(dev);
	spi_context_cs_control(ctx, true);

	if (spi_silabs_eusart_transfer_ongoing(data)) {
		do {
			ret = spi_silabs_eusart_shift_frames(eusart_config->base, data);
		} while (!ret && spi_silabs_eusart_transfer_ongoing(data));
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

	if (is_dma_enabled_instance(dev)) {
		/* DMA transfer handle a/synchronous transfers */
		ret = spi_silabs_eusart_xfer_dma(dev, config);
	} else {
		if (asynchronous) {
			/* Asynchronous transfers without DMA is not implemented,
			 * please configure the device tree
			 * instance with the proper DMA configuration.
			 */
			ret = -ENOTSUP;
			goto out;
		} else {
			ret = spi_silabs_eusart_xfer_polling(dev, config);
		}
	}

out:
	spi_context_release(ctx, ret);

	return ret;
}

/* API Functions */
static int spi_silabs_eusart_init(const struct device *dev)
{
	const struct spi_silabs_eusart_config *eusart_config = dev->config;
	struct spi_silabs_eusart_data *data = dev->data;
	int err;

	err = clock_control_on(eusart_config->clock_dev,
			       (clock_control_subsys_t)&eusart_config->clock_cfg);

	if (err < 0) {
		return err;
	}

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
	return spi_silabs_eusart_transceive(dev,
					    config,
					    tx_bufs,
					    rx_bufs,
					    true,
					    cb,
					    userdata);
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
#define SPI_SILABS_EUSART_DMA_CHANNEL_INIT(index, dir) \
	.dma_chan_##dir = { \
		.chan_nb = -1, \
		.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(index, dir)), \
		.dma_slot = SILABS_LDMA_REQSEL_TO_SLOT(DT_INST_DMAS_CELL_BY_NAME(index, \
										 dir, \
										 slot)), \
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
