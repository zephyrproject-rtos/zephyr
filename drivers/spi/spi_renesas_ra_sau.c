/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_spi_sau

#include <zephyr/drivers/clock_control/renesas_ra_cgc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <soc.h>
#include <r_dtc.h>
#include <r_sau_spi.h>
#include <rp_sau_spi.h>

LOG_MODULE_REGISTER(renesas_ra_sau_spi, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"

#define CS_GPIO_LOW_WHEN_ACTIVE  true
#define CS_GPIO_HIGH_WHEN_ACTIVE false

/* Frames sent once the transmit buffer is exhausted, as the SPI API describes the overrun
 * character. FSP sends 0xFF from its own dummy byte, so the read interfaces are not used.
 */
#define SPI_RENESAS_RA_SAU_NOP_FRAME 0x00U

struct renesas_ra_sau_spi_config {
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	const struct clock_control_ra_subsys_cfg clock_subsys;
	void (*irq_configure)(void);
};

struct renesas_ra_sau_spi_data {
	struct spi_context ctx;
	struct spi_config config;
	sau_spi_instance_ctrl_t fsp_ctrl;
	spi_cfg_t fsp_cfg;
	sau_spi_extended_cfg_t fsp_ext_cfg;
	spi_bit_width_t bit_width;
	size_t data_len;
	bool is_cs_active_state_same;
	/*
	 * Set while a transfer of ours is waiting for its callback. FSP raises
	 * SPI_EVENT_ERR_READ_OVERFLOW whenever OVF is latched at the time the interrupt is taken,
	 * including when no transfer is outstanding, and fsp_ctrl.transfer_in_progress cannot be
	 * used to tell the two apart because this driver clears it to chain the next chunk.
	 */
	bool transfer_active;
#ifdef CONFIG_SPI_RENESAS_RA_SAU_DTC
	/*
	 * A single instance carries both directions: p_info[0] receives SDRmn into the
	 * destination buffer and p_info[1] sends the source buffer to SDRmn, chained by FSP.
	 */
	struct st_transfer_instance tx_transfer;
	struct st_dtc_instance_ctrl tx_transfer_ctrl;
	struct st_transfer_info tx_transfer_info[2] DTC_TRANSFER_INFO_ALIGNMENT;
	struct st_transfer_cfg tx_transfer_cfg;
	struct st_dtc_extended_cfg tx_transfer_cfg_extend;
#endif /* CONFIG_SPI_RENESAS_RA_SAU_DTC */
};

void sau_spi_txrxi_isr(void);

static const uint8_t renesas_ra_sau_spi_nop_frames[64] = {
	[0 ... 63] = SPI_RENESAS_RA_SAU_NOP_FRAME,
};

/*
 * The devicetree flag and SPI_CS_ACTIVE_HIGH describe the same thing and the SPI API expects
 * them to agree, but a device may invert the chip select for part of its protocol, such as the
 * sdhc_spi card initialisation. Drive the level spi_config asks for, leaving the pin
 * configuration as devicetree set it, so that the line only changes when a transfer needs it.
 */
static inline void _renesas_ra_spi_context_cs_control(const struct device *dev, bool on,
						      bool force_off)
{
	struct renesas_ra_sau_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	if ((ctx->config) && spi_cs_is_gpio(ctx->config)) {
		if (on) {
			gpio_pin_set_dt(&ctx->config->cs.gpio,
					(data->is_cs_active_state_same) ? 1 : 0);
			k_busy_wait(ctx->config->cs.delay);
		} else {
			if ((!force_off) && (ctx->config->operation & SPI_HOLD_ON_CS)) {
				return;
			}
			k_busy_wait(ctx->config->cs.delay);
			gpio_pin_set_dt(&ctx->config->cs.gpio,
					(data->is_cs_active_state_same) ? 0 : 1);
		}
	}
}

static inline void renesas_ra_spi_context_cs_control(const struct device *dev, bool on)
{
	_renesas_ra_spi_context_cs_control(dev, on, false);
}

static inline void renesas_ra_spi_context_unlock_unconditionally(const struct device *dev)
{
	struct renesas_ra_sau_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	/* Forcing CS to go to inactive status */
	_renesas_ra_spi_context_cs_control(dev, false, true);

#ifdef CONFIG_MULTITHREADING
	if (!k_sem_count_get(&ctx->lock)) {
		ctx->owner = NULL;
		k_sem_give(&ctx->lock);
	}
#endif /* CONFIG_MULTITHREADING */
}

/* Check whether the configuration is similar */
static inline bool renesas_ra_sau_context_configured(const struct device *dev,
						     const struct spi_config *config)
{
	struct renesas_ra_sau_spi_data *data = dev->data;

	if ((data->fsp_ctrl.open != 0U) && (data->config.frequency == config->frequency) &&
	    (data->config.operation == config->operation) &&
	    (data->config.peripheral == config->peripheral)) {
		return true;
	}

	return false;
}

static fsp_err_t renesas_ra_sau_spi_start_chunk(struct renesas_ra_sau_spi_data *data)
{
	struct spi_context *ctx = &data->ctx;

	if (IS_ENABLED(CONFIG_SPI_PERIPHERAL) && spi_context_is_peripheral(ctx)) {
		/*
		 * A peripheral is given the whole set in a single transfer. It cannot be re-armed
		 * between buffers: starting a transfer stops and restarts the channel, and the
		 * frames the controller keeps clocking over that window are lost. The SCI and
		 * SCI_B drivers size a peripheral transfer the same way.
		 */
		if (ctx->rx_len == 0U) {
			data->data_len = spi_context_total_tx_len(ctx);
		} else if (ctx->tx_len == 0U) {
			data->data_len = spi_context_total_rx_len(ctx);
		} else {
			data->data_len =
				MAX(spi_context_total_tx_len(ctx), spi_context_total_rx_len(ctx));
		}
	} else {
		data->data_len = spi_context_max_continuous_chunk(ctx);
	}

#ifdef CONFIG_SPI_RENESAS_RA_SAU_DTC
	/* FSP only asserts the DTC length limit when parameter checking is enabled */
	data->data_len = MIN(data->data_len, UINT16_MAX);
#endif /* CONFIG_SPI_RENESAS_RA_SAU_DTC */

	if (ctx->tx_buf == NULL) {
		/* The overrun character is sent from a buffer, so the chunk is bounded by it */
		data->data_len = MIN(data->data_len, sizeof(renesas_ra_sau_spi_nop_frames));

		if (ctx->rx_buf == NULL) {
			return R_SAU_SPI_Write(&data->fsp_ctrl, renesas_ra_sau_spi_nop_frames,
					       data->data_len, data->bit_width);
		}

		return R_SAU_SPI_WriteRead(&data->fsp_ctrl, renesas_ra_sau_spi_nop_frames,
					   ctx->rx_buf, data->data_len, data->bit_width);
	}

	if (ctx->rx_buf == NULL) {
		return R_SAU_SPI_Write(&data->fsp_ctrl, ctx->tx_buf, data->data_len,
				       data->bit_width);
	}

	return R_SAU_SPI_WriteRead(&data->fsp_ctrl, ctx->tx_buf, ctx->rx_buf, data->data_len,
				   data->bit_width);
}

/*
 * Advance the context over the chunk that just finished. In peripheral mode
 * spi_context_update_rx() counts every frame the controller clocked as received, but a chunk
 * without a receive buffer is discarded, and the SPI API reports the number of frames that
 * reached the caller, so those frames are taken back out of the count.
 */
static void renesas_ra_sau_spi_update_context(struct renesas_ra_sau_spi_data *data)
{
	struct spi_context *ctx = &data->ctx;
#ifdef CONFIG_SPI_PERIPHERAL
	bool frames_kept = (ctx->rx_buf != NULL);
#endif /* CONFIG_SPI_PERIPHERAL */

	spi_context_update_tx(ctx, 1, data->data_len);
	spi_context_update_rx(ctx, 1, data->data_len);

#ifdef CONFIG_SPI_PERIPHERAL
	if (spi_context_is_peripheral(ctx) && !frames_kept) {
		ctx->recv_frames -= data->data_len;
	}
#endif /* CONFIG_SPI_PERIPHERAL */
}

#ifdef CONFIG_SPI_RENESAS_RA_SAU_DTC
/*
 * The DTC leaves FSP waiting for two interrupts one frame apart: the one the DTC raises as it
 * arms the last frame, and the one that frame raises when it completes. The transfer end
 * interrupt has a single pending bit, so the second is lost whenever the first is not taken
 * within a frame time, and FSP then waits for a frame it has already received. A short chunk at
 * a high bit rate closes that window while the calling thread is still parking itself, so the
 * received frame is drained here instead.
 */
static void renesas_ra_sau_spi_isr(const struct device *dev)
{
	struct renesas_ra_sau_spi_data *data = dev->data;
	R_SAU0_Type *sau_reg = (data->fsp_ext_cfg.sau_unit == 0U) ? R_SAU0 : R_SAU1;
	uint8_t channel = data->fsp_cfg.channel;

	sau_spi_txrxi_isr();

	if (data->fsp_ctrl.transfer_in_progress &&
	    ((sau_reg->SSR[channel] & R_SAU0_SSR_BFF_Msk) != 0U)) {
		sau_spi_txrxi_isr();

		/* The drained frame may have pended the interrupt that was just serviced */
		NVIC_ClearPendingIRQ(data->fsp_cfg.tei_irq);
	}
}
#endif /* CONFIG_SPI_RENESAS_RA_SAU_DTC */

static void renesas_ra_sau_spi_callback(spi_callback_args_t *p_args)
{
	const struct device *dev = p_args->p_context;
	struct renesas_ra_sau_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	fsp_err_t fsp_err;
	int ret = 0;

	if (!data->transfer_active) {
		/*
		 * A stray overflow between transfers would otherwise complete the context: that
		 * stores the error in sync_status and gives the completion semaphore, and the
		 * count it leaves behind is taken by the next transfer, which then reports the
		 * error instead of waiting for its own result. FSP has already cleared OVF.
		 */
		return;
	}

	switch (p_args->event) {
	case SPI_EVENT_TRANSFER_COMPLETE:
#ifdef CONFIG_SPI_PERIPHERAL
		if (spi_context_is_peripheral(ctx)) {
			/*
			 * The whole set was covered by the transfer that just finished, so the
			 * context is not advanced over it and no next chunk is started. Only the
			 * number of frames that reached the caller is reported.
			 */
			if ((ctx->tx_buf != NULL) && (ctx->rx_buf != NULL)) {
				ctx->recv_frames = MIN(spi_context_total_tx_len(ctx),
						       spi_context_total_rx_len(ctx));
			} else if (ctx->tx_buf == NULL) {
				ctx->recv_frames = (int)data->data_len;
			}

			break;
		}
#endif /* CONFIG_SPI_PERIPHERAL */

		renesas_ra_sau_spi_update_context(data);

		if (spi_context_tx_on(ctx) || spi_context_rx_on(ctx)) {
			/*
			 * FSP clears transfer_in_progress after this callback returns, so it is
			 * cleared here for the next chunk to be accepted, and FSP then clears it
			 * again over the chunk that was just armed. It therefore reads false for
			 * the rest of the transaction, which the spi_context lock serialises
			 * instead.
			 */
			data->fsp_ctrl.transfer_in_progress = false;

			fsp_err = renesas_ra_sau_spi_start_chunk(data);
			if (fsp_err == FSP_SUCCESS) {
				return;
			}

			LOG_ERR("Failed to start transfer: %d", fsp_err);
			ret = -EIO;
		}
		break;
	case SPI_EVENT_ERR_READ_OVERFLOW:
		/* FSP leaves transfer_in_progress set after an overflow */
		data->fsp_ctrl.transfer_in_progress = false;
		ret = -EIO;
		break;
	default:
		return;
	}

	data->transfer_active = false;
	renesas_ra_spi_context_cs_control(dev, false);
	spi_context_complete(ctx, dev, ret);
}

/*
 * FSP Open clears the direction bit of sck_pin_settings to make SCK an input in peripheral
 * mode. A pinctrl state does not name its pins, so the first entry of pinctrl-0 is taken as
 * the clock: the board states list SCK first in the output group.
 */
static int renesas_ra_sau_spi_sck_pin_get(const struct device *dev,
					  sau_spi_pin_settings_t *pin_settings)
{
	const struct renesas_ra_sau_spi_config *config = dev->config;
	const struct pinctrl_state *state;
	int ret;

	ret = pinctrl_lookup_state(config->pcfg, PINCTRL_STATE_DEFAULT, &state);
	if (ret < 0) {
		return ret;
	}

	if (state->pin_cnt == 0U) {
		return -EINVAL;
	}

	pin_settings->pin =
		(bsp_io_port_pin_t)((state->pins[0].port_num << 8U) | state->pins[0].pin_num);
	pin_settings->cfg = state->pins[0].cfg;

	return 0;
}

/*
 * The SCK input is sampled with the channel operation clock CKm0, so it runs at most at a sixth
 * of it. FSP only writes SPS in controller mode, so CKm0 may run faster than the devicetree
 * rate used here, which makes the check conservative. A zero frequency skips the check.
 */
static int renesas_ra_sau_spi_peripheral_frequency_check(const struct device *dev,
							 uint32_t frequency)
{
	const struct renesas_ra_sau_spi_config *config = dev->config;
	uint32_t rate;
	int ret;

	if (frequency == 0U) {
		return 0;
	}

	ret = clock_control_get_rate(config->clock_dev,
				     (clock_control_subsys_t)&config->clock_subsys, &rate);
	if (ret < 0) {
		return ret;
	}

	if (frequency > (rate / 6U)) {
		LOG_ERR("Frequency %u Hz exceeds the peripheral maximum %u Hz", frequency,
			rate / 6U);
		return -EINVAL;
	}

	return 0;
}

static int renesas_ra_sau_spi_configure(const struct device *dev, const struct spi_config *config)
{
	const struct renesas_ra_sau_spi_config *dev_config = dev->config;
	struct renesas_ra_sau_spi_data *data = dev->data;
	uint32_t word_size = SPI_WORD_SIZE_GET(config->operation);
	fsp_err_t fsp_err;
	int ret;

	/* The chip select is an input driven by the controller, SSI00 on SPI00 only */
	if ((SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_PERIPHERAL) &&
	    spi_cs_is_gpio(config)) {
		LOG_ERR("CS GPIO is not supported in peripheral mode");
		return -EINVAL;
	}

	/* Only the chip select may differ, and it is not part of the FSP configuration */
	if (renesas_ra_sau_context_configured(dev, config)) {
		data->config = *config;
		return 0;
	}

	if ((SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_PERIPHERAL) &&
	    !IS_ENABLED(CONFIG_SPI_PERIPHERAL)) {
		LOG_ERR("Peripheral mode requires CONFIG_SPI_PERIPHERAL");
		return -ENOTSUP;
	}

	if ((config->operation & SPI_FRAME_FORMAT_TI) == SPI_FRAME_FORMAT_TI) {
		LOG_ERR("TI frame format is not supported");
		return -ENOTSUP;
	}

	if ((SPI_MODE_GET(config->operation) & SPI_MODE_LOOP) != 0U) {
		LOG_ERR("Loopback mode is not supported");
		return -ENOTSUP;
	}

	if ((config->operation & SPI_HALF_DUPLEX) != 0U) {
		LOG_ERR("Half-duplex is not supported");
		return -ENOTSUP;
	}

	if (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES) &&
	    ((config->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE)) {
		LOG_ERR("Only single line mode is supported");
		return -ENOTSUP;
	}

	if ((word_size != 7U) && (word_size != 8U)) {
		LOG_ERR("Word size %u is not supported", word_size);
		return -ENOTSUP;
	}

	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_PERIPHERAL) {
		/* The controller provides the clock, so no bitrate is calculated */
		ret = renesas_ra_sau_spi_peripheral_frequency_check(dev, config->frequency);
		if (ret != 0) {
			return ret;
		}

		ret = renesas_ra_sau_spi_sck_pin_get(dev, &data->fsp_ext_cfg.sck_pin_settings);
		if (ret != 0) {
			LOG_ERR("No pin in the pinctrl state");
			return ret;
		}

		data->fsp_cfg.operating_mode = SPI_MODE_SLAVE;
	} else {
		if (config->frequency == 0U) {
			LOG_ERR("Invalid frequency");
			return -EINVAL;
		}

		fsp_err = R_SAU_SPI_CalculateBitrate(config->frequency, &data->fsp_ext_cfg.clk_div,
						     data->fsp_ext_cfg.sau_unit,
						     data->fsp_cfg.channel);
		/* A zero STCLK divider (half the operation clock) is only allowed on SPI00 */
		if ((fsp_err != FSP_SUCCESS) ||
		    ((data->fsp_ext_cfg.clk_div.stclk == 0U) &&
		     ((data->fsp_ext_cfg.sau_unit != 0U) || (data->fsp_cfg.channel != 0U)))) {
			LOG_ERR("Frequency %u Hz is not achievable", config->frequency);
			return -EINVAL;
		}

		data->fsp_cfg.operating_mode = SPI_MODE_MASTER;
	}

	data->fsp_cfg.bit_order = ((config->operation & SPI_TRANSFER_LSB) != 0U)
					  ? SPI_BIT_ORDER_LSB_FIRST
					  : SPI_BIT_ORDER_MSB_FIRST;
	/* The SCK idle level is high with the non-reverse clock phase */
	data->fsp_ext_cfg.clock_phase = ((SPI_MODE_GET(config->operation) & SPI_MODE_CPOL) != 0U)
						? SAU_SPI_CLOCK_PHASE_NON_REVERSE
						: SAU_SPI_CLOCK_PHASE_REVERSE;
	/* Data output half a clock cycle before the first edge samples on the first edge */
	data->fsp_ext_cfg.data_phase = ((SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) != 0U)
					       ? SAU_SPI_DATA_PHASE_START
					       : SAU_SPI_DATA_PHASE_HALF_CYCLE_START;
	data->bit_width = (word_size == 7U) ? SPI_BIT_WIDTH_7_BITS : SPI_BIT_WIDTH_8_BITS;

	if (data->fsp_ctrl.open != 0U) {
		fsp_err = R_SAU_SPI_Close(&data->fsp_ctrl);
		if (fsp_err != FSP_SUCCESS) {
			return -EIO;
		}

		memset(&data->config, 0, sizeof(data->config));
	}

	/* SCK keeps the direction the last Open left it in, so the state is applied again */
	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_PERIPHERAL) {
		/* FSP Open writes the clock pin itself, so the state is selected beforehand */
		ret = pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			LOG_ERR("Failed to apply the pinctrl state: %d", ret);
			return ret;
		}
	}

	fsp_err = R_SAU_SPI_Open(&data->fsp_ctrl, &data->fsp_cfg);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("Failed to apply SPI configuration: %d", fsp_err);

		if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_PERIPHERAL) {
			/* Restore the clock pin to the direction controller mode needs */
			(void)pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_DEFAULT);
		}

		return -EIO;
	}

	if (SPI_OP_MODE_GET(config->operation) != SPI_OP_MODE_PERIPHERAL) {
		/*
		 * Open drives the configured idle level onto the clock pin, so the pins are only
		 * selected once that level is in place, or the change is seen on the bus as a
		 * clock edge by a peripheral that is already waiting for one.
		 */
		ret = pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			LOG_ERR("Failed to apply the pinctrl state: %d", ret);
			(void)R_SAU_SPI_Close(&data->fsp_ctrl);
			return ret;
		}
	}

	data->config = *config;
	data->ctx.config = &data->config;

	return 0;
}

static int transceive(const struct device *dev, const struct spi_config *config,
		      const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
		      bool asynchronous, spi_callback_t cb, void *userdata)
{
	struct renesas_ra_sau_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	bool cs_gpio_logic_when_active;
	bool cs_active_logic_config;
	fsp_err_t fsp_err;
	int ret;

	if ((tx_bufs == NULL) && (rx_bufs == NULL)) {
		return 0;
	}

	spi_context_lock(ctx, asynchronous, cb, userdata, config);

	ret = renesas_ra_sau_spi_configure(dev, config);
	if (ret != 0) {
		goto release;
	}

	/* SAU frames are at most 8 bits wide, so one byte per frame */
	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);

	if (!spi_context_tx_on(ctx) && !spi_context_rx_on(ctx)) {
		if (asynchronous) {
			/* Report completion and release the lock taken for this call */
			spi_context_complete(ctx, dev, 0);
		}
		goto release;
	}

	/*
	 * The GPIO flags GPIO_ACTIVE_LOW/GPIO_ACTIVE_HIGH should be equivalent
	 * to SPI_CS_ACTIVE_HIGH/SPI_CS_ACTIVE_LOW options in struct spi_config.
	 * In runtime, there are some peripherals that need the CS level contrast
	 * to the CS defined in the device tree to make some actions such as
	 * initialization. Ex: PMOD SD_CARD
	 */
	cs_gpio_logic_when_active = !!(data->ctx.config->cs.gpio.dt_flags & GPIO_ACTIVE_LOW)
					    ? CS_GPIO_LOW_WHEN_ACTIVE
					    : CS_GPIO_HIGH_WHEN_ACTIVE;
	cs_active_logic_config = !!(data->ctx.config->operation & SPI_CS_ACTIVE_HIGH)
					 ? CS_GPIO_HIGH_WHEN_ACTIVE
					 : CS_GPIO_LOW_WHEN_ACTIVE;
	if (cs_gpio_logic_when_active == cs_active_logic_config) {
		data->is_cs_active_state_same = true;
	} else {
		data->is_cs_active_state_same = false;
	}

	renesas_ra_spi_context_cs_control(dev, true);

#ifdef CONFIG_SPI_RENESAS_RA_SAU_INTERRUPT
	data->transfer_active = true;

	fsp_err = renesas_ra_sau_spi_start_chunk(data);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("Failed to start transfer: %d", fsp_err);
		ret = -EIO;
		data->transfer_active = false;
		renesas_ra_spi_context_cs_control(dev, false);
		goto release;
	}

	ret = spi_context_wait_for_completion(ctx);
	if (ret == -ETIMEDOUT) {
		data->transfer_active = false;
		/* Abort the transfer; the next call opens the channel again */
		(void)R_SAU_SPI_Close(&data->fsp_ctrl);
		renesas_ra_spi_context_cs_control(dev, false);
		memset(&data->config, 0, sizeof(data->config));
	}
#else
	fsp_err = RP_SAU_SPI_StartTransferPolling(&data->fsp_ctrl, data->bit_width);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("Failed to start transfer: %d", fsp_err);
		ret = -EIO;
		renesas_ra_spi_context_cs_control(dev, false);
		goto release;
	}

	while (spi_context_tx_on(ctx) || spi_context_rx_on(ctx)) {
		data->data_len = spi_context_max_continuous_chunk(ctx);

		if (ctx->tx_buf == NULL) {
			data->data_len = MIN(data->data_len, sizeof(renesas_ra_sau_spi_nop_frames));
		}

		if (ctx->rx_buf != NULL) {
			fsp_err = RP_SAU_SPI_WriteReadPolling(
				&data->fsp_ctrl,
				(ctx->tx_buf != NULL) ? ctx->tx_buf : renesas_ra_sau_spi_nop_frames,
				ctx->rx_buf, data->data_len);
		} else {
			fsp_err = RP_SAU_SPI_WritePolling(
				&data->fsp_ctrl,
				(ctx->tx_buf != NULL) ? ctx->tx_buf : renesas_ra_sau_spi_nop_frames,
				data->data_len);
		}

		if (fsp_err != FSP_SUCCESS) {
			LOG_ERR("Transfer failed: %d", fsp_err);
			ret = -EIO;
			break;
		}

		renesas_ra_sau_spi_update_context(data);
	}

	fsp_err = RP_SAU_SPI_EndTransferPolling(&data->fsp_ctrl);
	if ((fsp_err != FSP_SUCCESS) && (ret == 0)) {
		LOG_ERR("Failed to end transfer: %d", fsp_err);
		ret = -EIO;
	}

	renesas_ra_spi_context_cs_control(dev, false);
	spi_context_complete(ctx, dev, ret);

	if (IS_ENABLED(CONFIG_SPI_PERIPHERAL) && (ret == 0)) {
		/* Peripheral mode returns the number of frames received */
		ret = spi_context_wait_for_completion(ctx);
	}
#endif /* CONFIG_SPI_RENESAS_RA_SAU_INTERRUPT */

release:
	spi_context_release(ctx, ret);

	return ret;
}

static int renesas_ra_sau_spi_transceive(const struct device *dev, const struct spi_config *config,
					 const struct spi_buf_set *tx_bufs,
					 const struct spi_buf_set *rx_bufs)
{
	return transceive(dev, config, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int renesas_ra_sau_spi_transceive_async(const struct device *dev,
					       const struct spi_config *config,
					       const struct spi_buf_set *tx_bufs,
					       const struct spi_buf_set *rx_bufs, spi_callback_t cb,
					       void *userdata)
{
	return transceive(dev, config, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif /* CONFIG_SPI_ASYNC */

/*
 * Releases the lock and the chip select only, as the SPI API describes. A transfer that FSP
 * has already armed keeps running; the next configure() closes the channel to abandon it.
 */
static int renesas_ra_sau_spi_release(const struct device *dev, const struct spi_config *config)
{
	ARG_UNUSED(config);

	renesas_ra_spi_context_unlock_unconditionally(dev);

	return 0;
}

static DEVICE_API(spi, renesas_ra_sau_spi_driver_api) = {
	.transceive = renesas_ra_sau_spi_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = renesas_ra_sau_spi_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
	.release = renesas_ra_sau_spi_release,
};

static int renesas_ra_sau_spi_init(const struct device *dev)
{
	const struct renesas_ra_sau_spi_config *config = dev->config;
	struct renesas_ra_sau_spi_data *data = dev->data;
	int ret;

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	ret = clock_control_on(config->clock_dev, (clock_control_subsys_t)&config->clock_subsys);
	if (ret < 0) {
		return ret;
	}

	/*
	 * The pins keep their reset state until the first transfer selects one. Driving them
	 * here would put the clock pin on the bus before the operating mode and the idle level
	 * are known, which a peripheral sharing the bus sees as a clock edge.
	 */

	ret = spi_context_cs_configure_all(&data->ctx);
	if (ret < 0) {
		return ret;
	}

	config->irq_configure();

	renesas_ra_spi_context_unlock_unconditionally(dev);

	return 0;
}

#define SPI_RENESAS_RA_SAU_CHANNEL(index) DT_INST_PHANDLE(index, sau_channel)

#ifdef CONFIG_SPI_RENESAS_RA_SAU_DTC
#define SPI_RENESAS_RA_SAU_ISR renesas_ra_sau_spi_isr
#else
#define SPI_RENESAS_RA_SAU_ISR sau_spi_txrxi_isr
#endif /* CONFIG_SPI_RENESAS_RA_SAU_DTC */

#ifdef CONFIG_SPI_RENESAS_RA_SAU_INTERRUPT
#define SPI_RENESAS_RA_SAU_IRQ_CONFIG_FUNC(index)                                                  \
	static void renesas_ra_sau_spi_irq_configure_##index(void)                                 \
	{                                                                                          \
		IRQ_CONNECT(DT_IRQ(SPI_RENESAS_RA_SAU_CHANNEL(index), irq),                        \
			    DT_IRQ(SPI_RENESAS_RA_SAU_CHANNEL(index), priority),                   \
			    SPI_RENESAS_RA_SAU_ISR, DEVICE_DT_INST_GET(index), 0);                 \
	}
#else
/* The polling interface masks the transfer end interrupt, so no handler is connected */
#define SPI_RENESAS_RA_SAU_IRQ_CONFIG_FUNC(index)                                                  \
	static void renesas_ra_sau_spi_irq_configure_##index(void)                                 \
	{                                                                                          \
	}
#endif /* CONFIG_SPI_RENESAS_RA_SAU_INTERRUPT */

#ifdef CONFIG_SPI_RENESAS_RA_SAU_DTC
#define SPI_RENESAS_RA_SAU_DTC_TRANSFER(index) (&renesas_ra_sau_spi_data_##index.tx_transfer)
#define SPI_RENESAS_RA_SAU_DTC_STRUCT_INIT(index)                                                  \
	.tx_transfer_cfg_extend =                                                                  \
		{                                                                                  \
			.activation_source = DT_IRQ(SPI_RENESAS_RA_SAU_CHANNEL(index), irq),       \
		},                                                                                 \
	.tx_transfer_cfg =                                                                         \
		{                                                                                  \
			.p_info = renesas_ra_sau_spi_data_##index.tx_transfer_info,                \
			.p_extend = &renesas_ra_sau_spi_data_##index.tx_transfer_cfg_extend,       \
		},                                                                                 \
	.tx_transfer =                                                                             \
		{                                                                                  \
			.p_ctrl = &renesas_ra_sau_spi_data_##index.tx_transfer_ctrl,               \
			.p_cfg = &renesas_ra_sau_spi_data_##index.tx_transfer_cfg,                 \
			.p_api = &g_transfer_on_dtc,                                               \
		},
#else
#define SPI_RENESAS_RA_SAU_DTC_TRANSFER(index) NULL
#define SPI_RENESAS_RA_SAU_DTC_STRUCT_INIT(index)
#endif /* CONFIG_SPI_RENESAS_RA_SAU_DTC */

#define SPI_RENESAS_RA_SAU_INIT(index)                                                             \
	BUILD_ASSERT(                                                                              \
		DT_NODE_HAS_COMPAT(SPI_RENESAS_RA_SAU_CHANNEL(index), renesas_ra_sau_channel) &&   \
			DT_NODE_HAS_COMPAT(DT_PARENT(SPI_RENESAS_RA_SAU_CHANNEL(index)),           \
					   renesas_ra_sau),                                        \
		"sau-channel must reference a channel of a renesas,ra-sau node");                  \
	BUILD_ASSERT(DT_IRQ_HAS_IDX(SPI_RENESAS_RA_SAU_CHANNEL(index), 0),                         \
		     "sau-channel must have an interrupt");                                        \
                                                                                                   \
	SPI_RENESAS_RA_SAU_IRQ_CONFIG_FUNC(index)                                                  \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	static const struct renesas_ra_sau_spi_config renesas_ra_sau_spi_config_##index = {        \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(SPI_RENESAS_RA_SAU_CHANNEL(index))),     \
		.clock_subsys =                                                                    \
			{                                                                          \
				.mstp = DT_CLOCKS_CELL(SPI_RENESAS_RA_SAU_CHANNEL(index), mstp),   \
				.stop_bit = DT_CLOCKS_CELL(SPI_RENESAS_RA_SAU_CHANNEL(index),      \
							   stop_bit),                              \
			},                                                                         \
		.irq_configure = renesas_ra_sau_spi_irq_configure_##index,                         \
	};                                                                                         \
                                                                                                   \
	static struct renesas_ra_sau_spi_data renesas_ra_sau_spi_data_##index = {                  \
		.is_cs_active_state_same = true,                                                   \
		SPI_CONTEXT_INIT_LOCK(renesas_ra_sau_spi_data_##index, ctx),                       \
		SPI_CONTEXT_INIT_SYNC(renesas_ra_sau_spi_data_##index, ctx),                       \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(index), ctx)                           \
		SPI_RENESAS_RA_SAU_DTC_STRUCT_INIT(index)                                          \
		.fsp_ext_cfg =                                                                     \
			{                                                                          \
				.transfer_mode = SAU_SPI_TRANSFER_MODE_SINGLE,                     \
				.sau_unit = DT_PROP(DT_PARENT(SPI_RENESAS_RA_SAU_CHANNEL(index)),  \
						    unit),                                         \
			},                                                                         \
		.fsp_cfg =                                                                         \
			{                                                                          \
				.channel = DT_REG_ADDR(SPI_RENESAS_RA_SAU_CHANNEL(index)),         \
				.rxi_irq = FSP_INVALID_VECTOR,                                     \
				.txi_irq = FSP_INVALID_VECTOR,                                     \
				.tei_irq = DT_IRQ(SPI_RENESAS_RA_SAU_CHANNEL(index), irq),         \
				.eri_irq = FSP_INVALID_VECTOR,                                     \
				.rxi_ipl = BSP_IRQ_DISABLED,                                       \
				.txi_ipl = BSP_IRQ_DISABLED,                                       \
				.tei_ipl = DT_IRQ(SPI_RENESAS_RA_SAU_CHANNEL(index), priority),    \
				.eri_ipl = BSP_IRQ_DISABLED,                                       \
				.p_transfer_tx = SPI_RENESAS_RA_SAU_DTC_TRANSFER(index),           \
				.p_transfer_rx = NULL,                                             \
				.p_callback = renesas_ra_sau_spi_callback,                         \
				.p_context = (void *)DEVICE_DT_INST_GET(index),                    \
				.p_extend = &renesas_ra_sau_spi_data_##index.fsp_ext_cfg,          \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	SPI_DEVICE_DT_INST_DEFINE(index, renesas_ra_sau_spi_init, NULL,                            \
				  &renesas_ra_sau_spi_data_##index,                                \
				  &renesas_ra_sau_spi_config_##index, POST_KERNEL,                 \
				  CONFIG_SPI_INIT_PRIORITY, &renesas_ra_sau_spi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_RENESAS_RA_SAU_INIT)
