/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include "spi_nxp_s32.h"

extern Spi_Ip_StateStructureType * Spi_Ip_apxStateStructureArray[SPI_INSTANCE_COUNT];

static bool spi_nxp_s32_last_packet(struct spi_nxp_s32_data *data)
{
	struct spi_context *ctx = &data->ctx;

	if (ctx->tx_count <= 1U && ctx->rx_count <= 1U) {
		if (!spi_context_tx_on(ctx) && (data->transfer_len == ctx->rx_len)) {
			return true;
		}

		if (!spi_context_rx_on(ctx) && (data->transfer_len == ctx->tx_len)) {
			return true;
		}

		if ((ctx->rx_len == ctx->tx_len) && (data->transfer_len == ctx->tx_len)) {
			return true;
		}
	}

	return false;
}

static inline bool spi_nxp_s32_transfer_done(struct spi_context *ctx)
{
	return !spi_context_tx_on(ctx) && !spi_context_rx_on(ctx);
}

static int spi_nxp_s32_transfer_next_packet(const struct device *dev)
{
	const struct spi_nxp_s32_config *config = dev->config;
	struct spi_nxp_s32_data *data = dev->data;

	Spi_Ip_StatusType status;
	Spi_Ip_CallbackType data_cb;

#ifdef CONFIG_NXP_S32_SPI_INTERRUPT
	data_cb = config->cb;
#else
	data_cb = NULL;
#endif /* CONFIG_NXP_S32_SPI_INTERRUPT */

	data->transfer_len = spi_context_max_continuous_chunk(&data->ctx);
	data->transfer_len = MIN(data->transfer_len,
					SPI_NXP_S32_MAX_BYTES_PER_PACKAGE(data->bytes_per_frame));

	/*
	 * Keep CS signal asserted until the last package, there is no other way
	 * than directly intervening to internal state of low level driver
	 */
	Spi_Ip_apxStateStructureArray[config->instance]->KeepCs = !spi_nxp_s32_last_packet(data);

	status = Spi_Ip_AsyncTransmit(&data->transfer_cfg, (uint8_t *)data->ctx.tx_buf,
						data->ctx.rx_buf, data->transfer_len, data_cb);

	if (status) {
		LOG_ERR("Transfer could not start");
		return -EIO;
	}

#ifdef CONFIG_NXP_S32_SPI_INTERRUPT
	return 0;
#else

	while (Spi_Ip_GetStatus(config->instance) == SPI_IP_BUSY) {
		Spi_Ip_ManageBuffers(config->instance);
	}

	if (Spi_Ip_GetStatus(config->instance) == SPI_IP_FAULT) {
		return -EIO;
	}

	return 0;
#endif /* CONFIG_NXP_S32_SPI_INTERRUPT */
}

/*
 * The function to get Scaler and Prescaler for corresponding registers
 * to configure the baudrate for the transmission. The real frequency is
 * computated to ensure it will always equal or the nearest approximation
 * lower to the expected one.
 */
static void spi_nxp_s32_getbestfreq(uint32_t clock_frequency,
					uint32_t requested_baud,
					struct spi_nxp_s32_baudrate_param *best_baud)
{
	uint8_t scaler;
	uint8_t prescaler;

	uint32_t low, high;
	uint32_t curr_freq;

	uint32_t best_freq = 0U;

	static const uint8_t prescaler_arr[SPI_NXP_S32_NUM_PRESCALER] = {2U, 3U, 5U, 7U};

	static const uint16_t scaller_arr[SPI_NXP_S32_NUM_SCALER] = {
		2U, 4U, 6U, 8U, 16U, 32U, 64U, 128U, 256U, 512U, 1024U, 2048U,
		4096U, 8192U, 16384U, 32768U
	};

	for (prescaler = 0U; prescaler < SPI_NXP_S32_NUM_PRESCALER; prescaler++) {
		low = 0U;
		high = SPI_NXP_S32_NUM_SCALER - 1U;

		/* Implement golden section search algorithm */
		do {
			scaler = (low + high) / 2U;

			curr_freq = clock_frequency * 1U /
					(prescaler_arr[prescaler] * scaller_arr[scaler]);

			/*
			 * If the scaler make current frequency higher than the
			 * expected one, skip the next step
			 */
			if (curr_freq > requested_baud) {
				low = scaler;
				continue;
			} else {
				high = scaler;
			}

			if ((requested_baud - best_freq) > (requested_baud - curr_freq)) {
				best_freq = curr_freq;
				best_baud->prescaler = prescaler;
				best_baud->scaler    = scaler;
			}

			if (best_freq == requested_baud) {
				break;
			}

		} while ((high - low) > 1U);

		if ((high - low) <= 1U) {

			if (high == scaler) {
				/* use low value */
				scaler = low;
			} else {
				scaler = high;
			}

			curr_freq = clock_frequency * 1U /
					(prescaler_arr[prescaler] * scaller_arr[scaler]);

			if (curr_freq <= requested_baud) {

				if ((requested_baud - best_freq) > (requested_baud - curr_freq)) {
					best_freq = curr_freq;
					best_baud->prescaler = prescaler;
					best_baud->scaler    = scaler;
				}
			}
		}

		if (best_freq == requested_baud) {
			break;
		}
	}

	best_baud->frequency = best_freq;
}

/*
 * The function to get Scaler and Prescaler for corresponding registers
 * to configure the delay for the transmission. The real delay is computated
 * to ensure it will always equal or the nearest approximation higher to
 * the expected one. In the worst case, use the delay as much as possible.
 */
static void spi_nxp_s32_getbestdelay(uint32_t clock_frequency, uint32_t requested_delay,
					uint8_t *best_scaler, uint8_t *best_prescaler)
{
	uint32_t current_delay;
	uint8_t scaler, prescaler;
	uint32_t low, high;

	uint32_t best_delay = 0xFFFFFFFFU;

	/* The scaler array is a power of two, so do not need to be defined */
	static const uint8_t prescaler_arr[SPI_NXP_S32_NUM_PRESCALER] = {1U, 3U, 5U, 7U};

	clock_frequency = clock_frequency / MHZ(1);

	for (prescaler = 0; prescaler < SPI_NXP_S32_NUM_PRESCALER; prescaler++) {
		low = 0U;
		high = SPI_NXP_S32_NUM_SCALER - 1U;

		do {
			scaler = (low + high) / 2U;

			current_delay = NSEC_PER_USEC * prescaler_arr[prescaler]
						* (1U << (scaler + 1)) / clock_frequency;

			/*
			 * If the scaler make current delay smaller than
			 * the expected one, skip the next step
			 */
			if (current_delay < requested_delay) {
				low = scaler;
				continue;
			} else {
				high = scaler;
			}

			if ((best_delay - requested_delay) > (current_delay - requested_delay)) {
				best_delay = current_delay;
				*best_prescaler = prescaler;
				*best_scaler = scaler;
			}

			if (best_delay == requested_delay) {
				break;
			}

		} while ((high - low) > 1U);

		if ((high - low) <= 1U) {

			if (high == scaler) {
				/* use low value */
				scaler = low;
			} else {
				scaler = high;
			}

			current_delay = NSEC_PER_USEC * prescaler_arr[prescaler]
						* (1U << (scaler + 1)) / clock_frequency;

			if (current_delay >= requested_delay) {
				if ((best_delay - requested_delay) >
					(current_delay - requested_delay)) {

					best_delay = current_delay;
					*best_prescaler = prescaler;
					*best_scaler = scaler;
				}
			}
		}

		if (best_delay == requested_delay) {
			break;
		}
	}

	if (best_delay == 0xFFFFFFFFU) {
		/* Use the delay as much as possible */
		*best_prescaler = SPI_NXP_S32_NUM_PRESCALER - 1U;
		*best_scaler = SPI_NXP_S32_NUM_SCALER - 1U;
	}
}

static int spi_nxp_s32_configure(const struct device *dev,
				const struct spi_config *spi_cfg)
{
	const struct spi_nxp_s32_config *config = dev->config;
	struct spi_nxp_s32_data *data = dev->data;

	bool clk_phase, clk_polarity;
	bool lsb, hold_cs;
	bool slave_mode, cs_active_high;

	uint8_t frame_size;

	struct spi_nxp_s32_baudrate_param best_baud = {0};

	if (spi_context_configured(&data->ctx, spi_cfg)) {
		/* This configuration is already in use */
		return 0;
	}

	clk_phase	= !!(SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPHA);
	clk_polarity	= !!(SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPOL);

	hold_cs		= !!(spi_cfg->operation & SPI_HOLD_ON_CS);
	lsb		= !!(spi_cfg->operation & SPI_TRANSFER_LSB);

	slave_mode	= !!(SPI_OP_MODE_GET(spi_cfg->operation));
	frame_size	= SPI_WORD_SIZE_GET(spi_cfg->operation);
	cs_active_high	= !!(spi_cfg->operation & SPI_CS_ACTIVE_HIGH);

	if (slave_mode == (!!(config->spi_hw_cfg->Mcr & SPI_MCR_MSTR_MASK))) {
		LOG_ERR("SPI mode (master/slave) must be same as configured in DT");
		return -ENOTSUP;
	}

	if (slave_mode && !IS_ENABLED(CONFIG_SPI_SLAVE)) {
		LOG_ERR("Kconfig for enable SPI in slave mode is not enabled");
		return -ENOTSUP;
	}

	if (slave_mode && lsb) {
		LOG_ERR("SPI does not support to shifting out with LSB in slave mode");
		return -ENOTSUP;
	}

	if (spi_cfg->slave >= config->num_cs) {
		LOG_ERR("Slave %d excess the allowed maximum value (%d)",
			spi_cfg->slave, config->num_cs - 1);
		return -ENOTSUP;
	}

	if (frame_size > 32U) {
		LOG_ERR("Unsupported frame size %d bits", frame_size);
		return -ENOTSUP;
	}

	if ((spi_cfg->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("Only single line mode is supported");
		return -ENOTSUP;
	}

	if (spi_cfg->operation & SPI_MODE_LOOP) {
		LOG_ERR("Loopback mode is not supported");
		return -ENOTSUP;
	}

	if (cs_active_high && (spi_cfg->cs == NULL)) {
		LOG_ERR("For CS has active state is high, a GPIO pin must be used to"
			" control CS line instead");
		return -ENOTSUP;
	}

	if (!slave_mode) {

		if ((spi_cfg->frequency < SPI_NXP_S32_MIN_FREQ) ||
			(spi_cfg->frequency > SPI_NXP_S32_MAX_FREQ)) {

			LOG_ERR("The frequency is out of range");
			return -ENOTSUP;
		}

		spi_nxp_s32_getbestfreq(config->clock_frequency, spi_cfg->frequency, &best_baud);

		data->transfer_cfg.Ctar &= ~(SPI_CTAR_BR_MASK | SPI_CTAR_PBR_MASK);
		data->transfer_cfg.Ctar |= SPI_CTAR_BR(best_baud.scaler) |
						SPI_CTAR_PBR(best_baud.prescaler);

		data->transfer_cfg.PushrCmd &= ~((SPI_PUSHR_CONT_MASK | SPI_PUSHR_PCS_MASK) >> 16U);

		if (spi_cfg->cs == NULL) {
			/* Use inner CS signal from SPI module */
			data->transfer_cfg.PushrCmd |= hold_cs << 15U;
			data->transfer_cfg.PushrCmd |= (1U << spi_cfg->slave);
		}
	}

	data->transfer_cfg.Ctar &= ~(SPI_CTAR_CPHA_MASK | SPI_CTAR_CPOL_MASK);
	data->transfer_cfg.Ctar |= SPI_CTAR_CPHA(clk_phase) | SPI_CTAR_CPOL(clk_polarity);

	Spi_Ip_UpdateFrameSize(&data->transfer_cfg, frame_size);
	Spi_Ip_UpdateLsb(&data->transfer_cfg, lsb);

	data->ctx.config	= spi_cfg;
	data->bytes_per_frame	= SPI_NXP_S32_BYTE_PER_FRAME(frame_size);

	if (slave_mode) {
		LOG_DBG("SPI configuration: cpol = %u, cpha = %u,"
			" lsb = %u, frame_size = %u, mode: slave",
			clk_polarity, clk_phase, lsb, frame_size);
	} else {
		LOG_DBG("SPI configuration: frequency = %uHz, cpol = %u,"
			" cpha = %u, lsb = %u, hold_cs = %u, frame_size = %u,"
			" mode: master, CS = %u\n",
			best_baud.frequency, clk_polarity, clk_phase,
			lsb, hold_cs, frame_size, spi_cfg->slave);
	}

	return 0;
}

static int transceive(const struct device *dev,
			const struct spi_config *spi_cfg,
			const struct spi_buf_set *tx_bufs,
			const struct spi_buf_set *rx_bufs,
			bool asynchronous,
			spi_callback_t cb,
			void *userdata)
{
	struct spi_nxp_s32_data *data = dev->data;
	struct spi_context *context = &data->ctx;
	int ret;

	if (!tx_bufs && !rx_bufs) {
		return 0;
	}

#ifndef CONFIG_NXP_S32_SPI_INTERRUPT
	if (asynchronous) {
		return -ENOTSUP;
	}
#endif /* CONFIG_NXP_S32_SPI_INTERRUPT */

	spi_context_lock(context, asynchronous, cb, userdata, spi_cfg);

	ret = spi_nxp_s32_configure(dev, spi_cfg);
	if (ret) {
		LOG_ERR("An error occurred in the SPI configuration");
		spi_context_release(context, ret);
		return ret;
	}

	spi_context_buffers_setup(context, tx_bufs, rx_bufs, 1U);

	if (spi_nxp_s32_transfer_done(context)) {
		spi_context_release(context, 0);
		return 0;
	}

	spi_context_cs_control(context, true);

#ifdef CONFIG_NXP_S32_SPI_INTERRUPT
	ret = spi_nxp_s32_transfer_next_packet(dev);

	if (!ret) {
		ret = spi_context_wait_for_completion(context);
	} else {
		spi_context_cs_control(context, false);
	}
#else
	do {
		ret = spi_nxp_s32_transfer_next_packet(dev);

		if (!ret) {
			spi_context_update_tx(context, 1U, data->transfer_len);
			spi_context_update_rx(context, 1U, data->transfer_len);
		}
	} while (!ret && !spi_nxp_s32_transfer_done(context));

	spi_context_cs_control(context, false);

#ifdef CONFIG_SPI_SLAVE
	if (spi_context_is_slave(context) && !ret) {
		ret = data->ctx.recv_frames;
	}
#endif /* CONFIG_SPI_SLAVE */
#endif /* CONFIG_NXP_S32_SPI_INTERRUPT */

	spi_context_release(context, ret);

	return ret;
}

static int spi_nxp_s32_transceive(const struct device *dev,
				const struct spi_config *spi_cfg,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, false, NULL, NULL);
}
#ifdef CONFIG_SPI_ASYNC
static int spi_nxp_s32_transceive_async(const struct device *dev,
				const struct spi_config *spi_cfg,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs,
				spi_callback_t callback,
				void *userdata)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, true, callback, userdata);
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_nxp_s32_release(const struct device *dev,
				const struct spi_config *spi_cfg)
{
	struct spi_nxp_s32_data *data = dev->data;

	(void)spi_cfg;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static int spi_nxp_s32_init(const struct device *dev)
{
	const struct spi_nxp_s32_config *config = dev->config;
	struct spi_nxp_s32_data *data = dev->data;

	uint8_t scaler, prescaler;

	uint32_t ctar = 0;
	int ret = 0;

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	if (Spi_Ip_Init(config->spi_hw_cfg)) {
		return -EBUSY;
	}

#ifdef CONFIG_NXP_S32_SPI_INTERRUPT
	if (Spi_Ip_UpdateTransferMode(config->instance, SPI_IP_INTERRUPT)) {
		return -EBUSY;
	}

	config->irq_config_func(dev);
#endif /* CONFIG_NXP_S32_SPI_INTERRUPT */

	/*
	 * Update the delay timings configuration that are
	 * applied for all inner CS signals of SPI module.
	 */
	spi_nxp_s32_getbestdelay(config->clock_frequency,
					config->sck_cs_delay, &scaler, &prescaler);

	ctar |= SPI_CTAR_ASC(scaler) | SPI_CTAR_PASC(prescaler);

	spi_nxp_s32_getbestdelay(config->clock_frequency,
					config->cs_sck_delay, &scaler, &prescaler);

	ctar |= SPI_CTAR_CSSCK(scaler) | SPI_CTAR_PCSSCK(prescaler);

	spi_nxp_s32_getbestdelay(config->clock_frequency,
					config->cs_cs_delay, &scaler, &prescaler);

	ctar |= SPI_CTAR_DT(scaler) | SPI_CTAR_PDT(prescaler);

	data->transfer_cfg.Ctar |= ctar;
	data->transfer_cfg.DeviceParams = &data->transfer_params;

	ret = spi_context_cs_configure_all(&data->ctx);
	if (ret < 0) {
		return ret;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}


#ifdef CONFIG_NXP_S32_SPI_INTERRUPT
static void spi_nxp_s32_transfer_callback(const struct device *dev, Spi_Ip_EventType event)
{
	struct spi_nxp_s32_data *data = dev->data;
	int ret = 0;

	if (event == SPI_IP_EVENT_END_TRANSFER) {
		spi_context_update_tx(&data->ctx, 1U, data->transfer_len);
		spi_context_update_rx(&data->ctx, 1U, data->transfer_len);

		if (spi_nxp_s32_transfer_done(&data->ctx)) {
			spi_context_complete(&data->ctx, dev, 0);
			spi_context_cs_control(&data->ctx, false);
		} else {
			ret = spi_nxp_s32_transfer_next_packet(dev);
		}
	} else {
		LOG_ERR("Failing in transfer_callback");
		ret = -EIO;
	}

	if (ret) {
		spi_context_complete(&data->ctx, dev, ret);
		spi_context_cs_control(&data->ctx, false);
	}
}
#endif /*CONFIG_NXP_S32_SPI_INTERRUPT*/

static const struct spi_driver_api spi_nxp_s32_driver_api = {
	.transceive = spi_nxp_s32_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_nxp_s32_transceive_async,
#endif
	.release = spi_nxp_s32_release,
};

#define SPI_NXP_S32_NODE(n)		DT_NODELABEL(spi##n)
#define SPI_NXP_S32_NUM_CS(n)		DT_PROP(SPI_NXP_S32_NODE(n), num_cs)
#define SPI_NXP_S32_IS_MASTER(n)	!DT_PROP(SPI_NXP_S32_NODE(n), slave)

#ifdef CONFIG_SPI_SLAVE
#define SPI_NXP_S32_SET_SLAVE(n)	.SlaveMode = DT_PROP(SPI_NXP_S32_NODE(n), slave),
#else
#define SPI_NXP_S32_SET_SLAVE(n)
#endif

#ifdef CONFIG_NXP_S32_SPI_INTERRUPT

#define SPI_NXP_S32_CONFIG_INTERRUPT_FUNC(n)						\
	.irq_config_func = spi_nxp_s32_config_func_##n,

#define SPI_NXP_S32_INTERRUPT_DEFINE(n)							\
	extern void Spi_Ip_SPI_##n##_IRQHandler(void);					\
	static void spi_nxp_s32_config_func_##n(const struct device *dev)		\
	{										\
		IRQ_CONNECT(DT_IRQN(SPI_NXP_S32_NODE(n)),				\
			DT_IRQ(SPI_NXP_S32_NODE(n), priority),				\
			Spi_Ip_SPI_##n##_IRQHandler,					\
			DEVICE_DT_GET(SPI_NXP_S32_NODE(n)),				\
			DT_IRQ(SPI_NXP_S32_NODE(n), flags));				\
		irq_enable(DT_IRQN(SPI_NXP_S32_NODE(n)));				\
	}

#define SPI_NXP_S32_CONFIG_CALLBACK_FUNC(n)						\
	.cb = spi_nxp_s32_##n##_callback,

#define SPI_NXP_S32_CALLBACK_DEFINE(n)							\
	static void spi_nxp_s32_##n##_callback(uint8 instance, Spi_Ip_EventType event)	\
	{										\
		ARG_UNUSED(instance);							\
		const struct device *dev = DEVICE_DT_GET(SPI_NXP_S32_NODE(n));		\
											\
		spi_nxp_s32_transfer_callback(dev, event);				\
	}
#else
#define SPI_NXP_S32_CONFIG_INTERRUPT_FUNC(n)
#define SPI_NXP_S32_INTERRUPT_DEFINE(n)
#define SPI_NXP_S32_CONFIG_CALLBACK_FUNC(n)
#define SPI_NXP_S32_CALLBACK_DEFINE(n)
#endif /*CONFIG_NXP_S32_SPI_INTERRUPT*/

/*
 * Declare the default configuration for SPI driver, no DMA
 * support, all inner module Chip Selects are active low.
 */
#define SPI_NXP_S32_INSTANCE_CONFIG(n)							\
	static const Spi_Ip_ConfigType spi_nxp_s32_default_config_##n = {		\
		.Instance = n,								\
		.Mcr = (SPI_MCR_MSTR(SPI_NXP_S32_IS_MASTER(n)) |			\
			SPI_MCR_CONT_SCKE(0U) |	SPI_MCR_FRZ(0U) |			\
			SPI_MCR_MTFE(0U) | SPI_MCR_SMPL_PT(0U) |			\
			SPI_MCR_PCSIS(BIT_MASK(SPI_NXP_S32_NUM_CS(n))) |		\
			SPI_MCR_MDIS(0U) | SPI_MCR_XSPI(1U) | SPI_MCR_HALT(1U)),	\
		.TransferMode = SPI_IP_POLLING,						\
		.StateIndex   = n,							\
		SPI_NXP_S32_SET_SLAVE(n)						\
	}

#define SPI_NXP_S32_TRANSFER_CONFIG(n)							\
	.transfer_cfg = {								\
		.Instance = n,								\
		.Ctare = SPI_CTARE_FMSZE(0U) | SPI_CTARE_DTCP(1U),			\
	}

#define SPI_NXP_S32_DEVICE(n)								\
	PINCTRL_DT_DEFINE(SPI_NXP_S32_NODE(n));						\
	SPI_NXP_S32_CALLBACK_DEFINE(n)							\
	SPI_NXP_S32_INTERRUPT_DEFINE(n)							\
	SPI_NXP_S32_INSTANCE_CONFIG(n);							\
	static const struct spi_nxp_s32_config spi_nxp_s32_config_##n = {		\
		.instance     = n,							\
		.num_cs	      = SPI_NXP_S32_NUM_CS(n),					\
		.clock_frequency = DT_PROP(SPI_NXP_S32_NODE(n), clock_frequency),	\
		.sck_cs_delay = DT_PROP_OR(SPI_NXP_S32_NODE(n), spi_sck_cs_delay, 0U),	\
		.cs_sck_delay = DT_PROP_OR(SPI_NXP_S32_NODE(n), spi_cs_sck_delay, 0U),	\
		.cs_cs_delay  = DT_PROP_OR(SPI_NXP_S32_NODE(n), spi_cs_cs_delay, 0U),	\
		.spi_hw_cfg   = (Spi_Ip_ConfigType *)&spi_nxp_s32_default_config_##n,	\
		.pincfg = PINCTRL_DT_DEV_CONFIG_GET(SPI_NXP_S32_NODE(n)),		\
		SPI_NXP_S32_CONFIG_CALLBACK_FUNC(n)					\
		SPI_NXP_S32_CONFIG_INTERRUPT_FUNC(n)					\
	};										\
	static struct spi_nxp_s32_data spi_nxp_s32_data_##n = {				\
		SPI_NXP_S32_TRANSFER_CONFIG(n),						\
		SPI_CONTEXT_BASE_INIT(spi_nxp_s32_data_##n, ctx),			\
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(SPI_NXP_S32_NODE(n), ctx)		\
	};										\
	DEVICE_DT_DEFINE(SPI_NXP_S32_NODE(n),						\
			&spi_nxp_s32_init, NULL,					\
			&spi_nxp_s32_data_##n, &spi_nxp_s32_config_##n,			\
			POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,				\
			&spi_nxp_s32_driver_api);

#if DT_NODE_HAS_STATUS(SPI_NXP_S32_NODE(0), okay)
SPI_NXP_S32_DEVICE(0);
#endif

#if DT_NODE_HAS_STATUS(SPI_NXP_S32_NODE(1), okay)
SPI_NXP_S32_DEVICE(1);
#endif

#if DT_NODE_HAS_STATUS(SPI_NXP_S32_NODE(2), okay)
SPI_NXP_S32_DEVICE(2);
#endif

#if DT_NODE_HAS_STATUS(SPI_NXP_S32_NODE(3), okay)
SPI_NXP_S32_DEVICE(3);
#endif

#if DT_NODE_HAS_STATUS(SPI_NXP_S32_NODE(4), okay)
SPI_NXP_S32_DEVICE(4);
#endif

#if DT_NODE_HAS_STATUS(SPI_NXP_S32_NODE(5), okay)
SPI_NXP_S32_DEVICE(5);
#endif

#if DT_NODE_HAS_STATUS(SPI_NXP_S32_NODE(6), okay)
SPI_NXP_S32_DEVICE(6);
#endif

#if DT_NODE_HAS_STATUS(SPI_NXP_S32_NODE(7), okay)
SPI_NXP_S32_DEVICE(7);
#endif

#if DT_NODE_HAS_STATUS(SPI_NXP_S32_NODE(8), okay)
SPI_NXP_S32_DEVICE(8);
#endif

#if DT_NODE_HAS_STATUS(SPI_NXP_S32_NODE(9), okay)
SPI_NXP_S32_DEVICE(9);
#endif
