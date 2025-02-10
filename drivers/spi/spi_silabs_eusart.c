/*
 * Copyright (c) 2024 Daikin Comfort Technologies North America, Inc.
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_eusart_spi

#if defined(CONFIG_SPI_ASYNC)
#define SPI_SILABS_SPI_EUSART_DMA (1U)
#endif

#include <stdbool.h>

#include <zephyr/sys/sys_io.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_silabs.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#ifdef SPI_SILABS_SPI_EUSART_DMA
#include <zephyr/drivers/dma.h>
#endif
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/device_runtime.h>

#include <em_cmu.h>
#include <em_eusart.h>

LOG_MODULE_REGISTER(spi_silabs_eusart, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"

/* Macro Declarations */

#define SPI_WORD_SIZE 8

#if	defined(CONFIG_SPI_ASYNC) && defined(SPI_SILABS_SPI_EUSART_DMA)
#define SPI_DMA_MAX_DESCRIPTOR_PER_CHANNEL   (9U)
#define SPI_DMA_MAX_DESCRIPTOR_TRANSFER_SIZE (0x800U)
#define SPI_TX_DUMMY_BYTE_VALUE              (0xFFU)
#endif

/* Structure Declarations */
struct spi_silabs_eusart_data {
	struct spi_context ctx;
#if defined(CONFIG_SPI_ASYNC) && defined(SPI_SILABS_SPI_EUSART_DMA)
	uint32_t dma_rx_channel;
	uint32_t dma_tx_channel;
	struct dma_config dma_tx_config;
	struct dma_block_config dma_tx_descriptors[SPI_DMA_MAX_DESCRIPTOR_PER_CHANNEL];
	struct dma_config dma_rx_config;
	struct dma_block_config dma_rx_descriptors[SPI_DMA_MAX_DESCRIPTOR_PER_CHANNEL];
#endif
};

struct spi_silabs_eusart_config {
	EUSART_TypeDef *base;
#if defined(CONFIG_SPI_ASYNC) && defined(SPI_SILABS_SPI_EUSART_DMA)
	const struct device *dma_device;
#endif
	const struct device *clock_dev;
	const struct silabs_clock_control_cmu_config clock_cfg;
	uint32_t clock_frequency;
	const struct pinctrl_dev_config *pcfg;
};

/* Global variable */
#if defined(CONFIG_SPI_ASYNC) && defined(SPI_SILABS_SPI_EUSART_DMA)
static volatile uint8_t dummy_byte = SPI_TX_DUMMY_BYTE_VALUE;
static volatile uint8_t empty_buffer = SPI_TX_DUMMY_BYTE_VALUE;
#endif /* CONFIG_SPI_ASYNC */

/* Function declaration */
#if defined(CONFIG_SPI_ASYNC) && defined(SPI_SILABS_SPI_EUSART_DMA)
void spi_silabs_dma_rx_callback(const struct device *dev, void *user_data, uint32_t channel,
				int status);
#endif /* CONFIG_SPI_ASYNC */

/* Helper Functions */
static int spi_silabs_eusart_configure(const struct device *dev, const struct spi_config *config,
				       uint16_t *control)
{
	struct spi_silabs_eusart_data *data = dev->data;
	const struct spi_silabs_eusart_config *eusart_config = dev->config;
	uint32_t spi_frequency;

	EUSART_SpiAdvancedInit_TypeDef eusartAdvancedSpiInit = EUSART_SPI_ADVANCED_INIT_DEFAULT;
	EUSART_SpiInit_TypeDef eusartInit = EUSART_SPI_MASTER_INIT_DEFAULT_HF;

	int err;

	if (spi_context_configured(&data->ctx, config)) {
		/* Runtime auto management disables and already configured.
		 * No need to do it again, but must re-enable in case
		 * TXEN/RXEN were cleared due to deep sleep.
		 */
#if !defined(CONFIG_PM_DEVICE_RUNTIME)
		EUSART_Enable(eusart_config->base, eusartEnable);
#endif
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

#if defined(CONFIG_SPI_ASYNC) && defined(SPI_SILABS_SPI_EUSART_DMA)
	if (!device_is_ready(eusart_config->dma_device)) {
		return -ENODEV;
	}

	eusartAdvancedSpiInit.TxFifoWatermark = eusartTxFiFoWatermark1Frame;
	eusartAdvancedSpiInit.RxFifoWatermark = eusartRxFiFoWatermark1Frame;

	data->dma_rx_channel = dma_request_channel(eusart_config->dma_device, NULL);

	if (data->dma_rx_channel < 0) {
		LOG_ERR("DMA channel request failed");
		return -EINVAL;
	}

	data->dma_tx_channel = dma_request_channel(eusart_config->dma_device, NULL);

	if (data->dma_tx_channel < 0) {
		dma_release_channel(eusart_config->dma_device, data->dma_rx_channel);
		LOG_ERR("DMA channel request failed");
		return -EINVAL;
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
	pm_device_runtime_get(dev);
}

static inline void spi_silabs_eusart_pm_policy_put(const struct device *dev)
{
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	pm_device_runtime_put(dev);
}

#if defined(CONFIG_PM_DEVICE)
static int spi_silabs_eusart_power_management_action(const struct device *dev,
						     enum pm_device_action action)
{
	int ret;

	const struct spi_silabs_eusart_config *eusart_config = dev->config;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = clock_control_on(eusart_config->clock_dev,
				       (clock_control_subsys_t)&eusart_config->clock_cfg);
		if (ret == -EALREADY) {
			/* ignore if clock is already on */
			ret = 0U;
		} else if (ret < 0) {
			return ret;
		}

		pinctrl_apply_state(eusart_config->pcfg, PINCTRL_STATE_DEFAULT);

		EUSART_Enable(eusart_config->base, eusartEnable);

		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = pinctrl_apply_state(eusart_config->pcfg, PINCTRL_STATE_SLEEP);

		EUSART_Enable(eusart_config->base, eusartDisable);
		ret = clock_control_off(eusart_config->clock_dev,
					(clock_control_subsys_t)&eusart_config->clock_cfg);

		if (ret == -EALREADY) {
			/* ignore if clock is already on */
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
#endif

#if defined(CONFIG_SPI_ASYNC) && defined(SPI_SILABS_SPI_EUSART_DMA)
static inline void spi_silabs_eusart_clear_txrx_fifos(EUSART_TypeDef *eusart)
{
	eusart->CMD_SET = EUSART_CMD_CLEARTX;

	while (EUSART_StatusGet(eusart) & EUSART_STATUS_RXFL) {
		(void)eusart->RXDATA;
	}

	while (eusart->STATUS & EUSART_STATUS_CLEARTXBUSY) {
	}
}

static inline size_t spi_silabs_longuest_transfer_size(struct spi_context *instance_ctx)
{
	uint32_t tx_transfer_size = spi_context_total_tx_len(instance_ctx);
	uint32_t rx_transfer_size = spi_context_total_rx_len(instance_ctx);

	return MAX(tx_transfer_size, rx_transfer_size);
}

static uint32_t
spi_silabs_build_dma_transfer_block(const struct spi_silabs_eusart_config *instance_config,
				    struct dma_block_config *new_blk_cfg, uint8_t *buffer,
				    size_t requested_transaction_size, bool is_tx)
{
	memset(new_blk_cfg, 0, sizeof(*new_blk_cfg));
	size_t remaining_transaction_size = 0U;

	/* Set-up source and destination address with increment behavior. */
	if (is_tx) {
		new_blk_cfg->dest_address = (uint32_t)&instance_config->base->TXDATA;
		new_blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		if (buffer) {
			new_blk_cfg->source_address = (uint32_t)buffer;
			new_blk_cfg->source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		} else {
			/* Null buffer pointer means sending dummy byte. */
			new_blk_cfg->source_address = (uint32_t)&dummy_byte;
			new_blk_cfg->source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		}
	} else {
		new_blk_cfg->source_address = (uint32_t)&instance_config->base->RXDATA;
		new_blk_cfg->source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		if (buffer) {
			new_blk_cfg->dest_address = (uint32_t)buffer;
			new_blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		} else {
			/* Null buffer pointer means rx to null byte. */
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

	/* Next block ptr. */
	if (remaining_transaction_size > 0U) {
		new_blk_cfg->next_block = &(new_blk_cfg[1]);
	} else {
		new_blk_cfg->next_block = NULL;
	}

	/* Return how much of the requested transaction is still required to be processed. */
	return remaining_transaction_size;
}

static void spi_silabs_build_dma_config(const struct device *dev,
					struct dma_config *current_dma_config,
					const struct dma_block_config *blk_cfg,
					uint32_t block_count, bool is_tx)
{
	memset(current_dma_config, 0, sizeof(*current_dma_config));

	if (is_tx) {
		/* Memory to peripheral. */
		current_dma_config->channel_direction = MEMORY_TO_PERIPHERAL;
	} else {
		/* Peripheral to memory. */
		current_dma_config->channel_direction = PERIPHERAL_TO_MEMORY;
	}

	/* invoke callback at the end of transfer list. */
	current_dma_config->complete_callback_en = 0x00U;

	current_dma_config->source_data_size = 0x01U; /* 1 byte. */
	current_dma_config->dest_data_size = 0x01U;   /* 1 byte. */

	current_dma_config->source_burst_length = 0x01U;
	current_dma_config->dest_burst_length = 0x01U;

	current_dma_config->block_count = block_count;

	current_dma_config->head_block = (struct dma_block_config *)blk_cfg;

	current_dma_config->user_data = (void *)dev;

	if (!is_tx) {
		/* Only enable the end transaction callback. Only SPI controller mode is supported.
		 */
		current_dma_config->dma_callback = &spi_silabs_dma_rx_callback;
	}
}

#define LDMA_SIGNAL_TO_DMA_SLOT_CONFIG(slot)                                                       \
	((slot & DMA_SLOT_SOURCESEL_MASK) << 13 | (slot & DMA_SLOT_SIGSEL_MASK))

static inline uint8_t spi_silabs_get_eusart_source_signal(uint32_t eusart_num, uint32_t dma_channel,
							  bool is_tx)
{
	volatile uint32_t source_signal = 0U;

	/* TX source/signal selection is at [X][0] of the 2d array and RX is at [X][1]. */
	const static struct {
		uint32_t source;
		uint32_t signal;
	} eusart_config[EUSART_COUNT][2] = {
#if defined(LDMAXBAR_CH_REQSEL_SOURCESEL_EUSART0)
		{{_LDMAXBAR_CH_REQSEL_SOURCESEL_EUSART0, _LDMAXBAR_CH_REQSEL_SIGSEL_EUSART0TXFL},
		 {_LDMAXBAR_CH_REQSEL_SOURCESEL_EUSART0, _LDMAXBAR_CH_REQSEL_SIGSEL_EUSART0RXFL}},
#endif
#if defined(LDMAXBAR_CH_REQSEL_SOURCESEL_EUSART1)
		{{_LDMAXBAR_CH_REQSEL_SOURCESEL_EUSART1, _LDMAXBAR_CH_REQSEL_SIGSEL_EUSART1TXFL},
		 {_LDMAXBAR_CH_REQSEL_SOURCESEL_EUSART1, _LDMAXBAR_CH_REQSEL_SIGSEL_EUSART1RXFL}},
#endif
#if defined(LDMAXBAR_CH_REQSEL_SOURCESEL_EUSART2)
		{{_LDMAXBAR_CH_REQSEL_SOURCESEL_EUSART2, _LDMAXBAR_CH_REQSEL_SIGSEL_EUSART2TXFL},
		 {_LDMAXBAR_CH_REQSEL_SOURCESEL_EUSART2, _LDMAXBAR_CH_REQSEL_SIGSEL_EUSART2RXFL}},
#endif
	};

	source_signal = eusart_config[eusart_num][0].source << 3U;

	if (is_tx) {
		source_signal |= eusart_config[eusart_num][0].signal;
	} else {
		source_signal |= eusart_config[eusart_num][1].signal;
	}

	return (uint8_t)(source_signal & 0xFFU);
}

static int spi_silabs_prepare_dma_transfer(const struct device *dev,
					   size_t total_padded_transaction_size, bool is_tx)
{
	struct dma_block_config *current_blk_cfg = NULL;
	const struct spi_buf *current_buffer = NULL;
	struct spi_silabs_eusart_data *instance_data = dev->data;
	const struct spi_silabs_eusart_config *instance_conf = dev->config;

	struct dma_config dma_cfg;

	size_t current_total_transfer_size = 0U;
	size_t padding_size = 0U;
	size_t buffer_count = 0U;
	size_t dma_block_config_idx = 0U;
	int ret = 0U;

	if (is_tx) {
		current_buffer = &instance_data->ctx.current_tx[0];
		buffer_count = instance_data->ctx.tx_count;
		current_blk_cfg = &instance_data->dma_tx_descriptors[0];
	} else {
		current_buffer = &instance_data->ctx.current_rx[0];
		buffer_count = instance_data->ctx.rx_count;
		current_blk_cfg = &instance_data->dma_rx_descriptors[0];
	}

	/* For each buffer in the instance context buffer set. */
	for (size_t i = 0U; i < buffer_count; i++) {
		size_t buffer_remaining_count = current_buffer->len;

		current_total_transfer_size += current_buffer->len;

		if (buffer_remaining_count == 0U) {
			/* Skip zero length buffers */
			continue;
		}

		/* Create the required number of dma transfer block for this buffer. */
		while (dma_block_config_idx < SPI_DMA_MAX_DESCRIPTOR_PER_CHANNEL) {
			uint32_t index_in_buffer = current_buffer->len - buffer_remaining_count;

			buffer_remaining_count = spi_silabs_build_dma_transfer_block(
				instance_conf, &(current_blk_cfg[dma_block_config_idx]),
				&(((uint8_t *)current_buffer->buf)[index_in_buffer]),
				buffer_remaining_count, is_tx);

			dma_block_config_idx++;

			if (dma_block_config_idx > SPI_DMA_MAX_DESCRIPTOR_PER_CHANNEL) {
				LOG_ERR("DMA channel transfer blocks exausted. Can't prepare the "
					"transaction.");
				return -ENOMEM;
			}

			if (buffer_remaining_count == 0U) {
				current_buffer++;
				break;
			}
		}
	}

	padding_size = total_padded_transaction_size - current_total_transfer_size;

	while (padding_size) {

		padding_size = spi_silabs_build_dma_transfer_block(
			instance_conf, &current_blk_cfg[dma_block_config_idx], NULL, padding_size,
			is_tx);

		if (++dma_block_config_idx >= SPI_DMA_MAX_DESCRIPTOR_PER_CHANNEL) {
			LOG_ERR("DMA transfer blocks exausted. Can't configure the transaction.");
			return -ENOMEM;
		}
	}

	spi_silabs_build_dma_config(dev, &dma_cfg,
				    (const struct dma_block_config *)&current_blk_cfg[0],
				    dma_block_config_idx, is_tx);

	uint32_t dma_channel =
		is_tx ? instance_data->dma_tx_channel : instance_data->dma_rx_channel;

	dma_cfg.dma_slot = spi_silabs_get_eusart_source_signal(EUSART_NUM(instance_conf->base),
							       dma_channel, is_tx);

	ret = dma_config(instance_conf->dma_device, dma_channel, &dma_cfg);

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
		/** nothing to do */
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
	if (status == DMA_STATUS_COMPLETE || status < 0U) {
		const struct device *spi_dev = (const struct device *)user_data;
		struct spi_silabs_eusart_data *instance_data = spi_dev->data;
		const struct spi_silabs_eusart_config *instance_conf = spi_dev->config;
		struct spi_context *instance_ctx = &instance_data->ctx;

		ARG_UNUSED(dev);

		if (status < 0U) {
			dma_stop(instance_conf->dma_device, instance_data->dma_tx_channel);
			dma_stop(instance_conf->dma_device, instance_data->dma_rx_channel);
		}

		spi_context_cs_control(instance_ctx, false);

		spi_silabs_eusart_pm_policy_put(spi_dev);

		spi_context_complete(instance_ctx, spi_dev, status);
	}
}

#else

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

#endif

static int spi_silabs_eusart_xfer(const struct device *dev, const struct spi_config *config)
{
	int ret;
	struct spi_silabs_eusart_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	const struct spi_silabs_eusart_config *eusart_config = dev->config;

#if defined(CONFIG_SPI_ASYNC) && defined(SPI_SILABS_SPI_EUSART_DMA)
	size_t total_padded_transaction_size = spi_silabs_longuest_transfer_size(ctx);

	spi_silabs_eusart_clear_txrx_fifos(eusart_config->base);

	ret = spi_silabs_prepare_dma_transaction(dev, total_padded_transaction_size);

	if (ret) {
		return ret;
	}

	spi_silabs_eusart_pm_policy_get(dev);

	spi_context_cs_control(ctx, true);

	if (total_padded_transaction_size == 0U) {
		goto end_transaction;
	}

	/* RX channel needs to be ready before TX channel actually starts. */
	ret = dma_start(eusart_config->dma_device, data->dma_rx_channel);

	if (ret) {
		goto end_transaction;
	}

	ret = dma_start(eusart_config->dma_device, data->dma_tx_channel);

	if (ret) {
		goto end_transaction;
	}

	ret = spi_context_wait_for_completion(&data->ctx);

end_transaction:
	spi_context_cs_control(ctx, false);
#else
	spi_silabs_eusart_pm_policy_get(dev);
	spi_context_cs_control(ctx, true);

	do {
		ret = spi_silabs_eusart_shift_frames(eusart_config->base, data);
	} while (!ret && spi_silabs_eusart_transfer_ongoing(data));

	spi_context_cs_control(ctx, false);
	spi_context_complete(ctx, dev, 0);
#endif

	spi_silabs_eusart_pm_policy_put(dev);
	return ret;
}

/* API Functions */
static int spi_silabs_eusart_init(const struct device *dev)
{
	int err;
	const struct spi_silabs_eusart_config *eusart_config = dev->config;
	struct spi_silabs_eusart_data *data = dev->data;

	err = clock_control_on(eusart_config->clock_dev, (clock_control_subsys_t)&eusart_config->clock_cfg);
	if (err < 0) {
		return err;
	}

	err = pinctrl_apply_state(eusart_config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0 && (err != -ENOENT)) {
		/** The return number ENOENT means that no sleep state is defined by the user in the
		 *  DT.*/
		return err;
	}

	err = spi_context_cs_configure_all(&data->ctx);
	if (err < 0) {
		return err;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	pm_device_init_off(dev);

	err = pm_device_runtime_enable(dev);

	return err;
}

static int spi_silabs_eusart_transceive(const struct device *dev, const struct spi_config *config,
					const struct spi_buf_set *tx_bufs,
					const struct spi_buf_set *rx_bufs)
{
	struct spi_silabs_eusart_data *data = dev->data;
	uint16_t control = 0;
	int ret;

	spi_context_lock(&data->ctx, false, NULL, NULL, config);

	ret = spi_silabs_eusart_configure(dev, config, &control);

	if (ret) {
		spi_context_release(&data->ctx, ret);
		return ret;
	}

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);
	ret = spi_silabs_eusart_xfer(dev, config);

	spi_context_release(&data->ctx, ret);

	return ret;
}

#ifdef CONFIG_SPI_ASYNC

static int spi_silabs_eusart_transceive_async(const struct device *dev,
					      const struct spi_config *config,
					      const struct spi_buf_set *tx_bufs,
					      const struct spi_buf_set *rx_bufs, spi_callback_t cb,
					      void *userdata)
{
	struct spi_silabs_eusart_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	uint16_t control = 0;
	int ret = 0U;

	/* Validate if the alignment of the buffers are valid. */
	spi_context_lock(ctx, true, cb, userdata, config);

	ret = spi_silabs_eusart_configure(dev, config, &control);

	if (ret) {
		spi_context_release(ctx, ret);
		return ret;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);
	spi_silabs_eusart_xfer(dev, config);

	ret = spi_context_wait_for_completion(ctx);

	spi_context_release(&data->ctx, ret);

	return ret;
}

#endif /* CONFIG_SPI_ASYNC */

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
	.transceive = spi_silabs_eusart_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_silabs_eusart_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
	.release = spi_silabs_eusart_release,
};

#if defined(CONFIG_SPI_ASYNC) && defined(SPI_SILABS_SPI_EUSART_DMA)
#define SPI_DMA_DEVICE_INSTANCE .dma_device = (const struct device *)DEVICE_DT_GET_ONE(silabs_ldma),
#else
#define SPI_DMA_DEVICE_INSTANCE
#endif

#define SPI_INIT(n)                                                                                \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static struct spi_silabs_eusart_data spi_silabs_eusart_data_##n = {                        \
		SPI_CONTEXT_INIT_LOCK(spi_silabs_eusart_data_##n, ctx),                            \
		SPI_CONTEXT_INIT_SYNC(spi_silabs_eusart_data_##n, ctx),                            \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)};                             \
	static struct spi_silabs_eusart_config spi_silabs_eusart_cfg_##n = {                       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.base = (EUSART_TypeDef *)DT_INST_REG_ADDR(n),                                     \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_cfg = SILABS_DT_INST_CLOCK_CFG(n),                                          \
		.clock_frequency = DT_INST_PROP_OR(n, clock_frequency, 1000000),                   \
		SPI_DMA_DEVICE_INSTANCE};                                                          \
	PM_DEVICE_DT_INST_DEFINE(n, spi_silabs_eusart_power_management_action);                    \
	SPI_DEVICE_DT_INST_DEFINE(n, spi_silabs_eusart_init, PM_DEVICE_DT_INST_GET(n),             \
				  &spi_silabs_eusart_data_##n, &spi_silabs_eusart_cfg_##n,         \
				  POST_KERNEL, CONFIG_SPI_INIT_PRIORITY, &spi_silabs_eusart_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_INIT)
