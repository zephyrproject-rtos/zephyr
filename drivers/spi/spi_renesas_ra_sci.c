/*
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_spi_sci

#include <r_dtc.h>
#include <r_sci_spi.h>
#include <rp_sci_spi.h>
#include <soc.h>
#include <zephyr/drivers/clock_control/renesas_ra_cgc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/irq.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(renesas_ra_spi_sci);

#include "spi_context.h"

struct renesas_ra_sci_spi_config {
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	const struct clock_control_ra_subsys_cfg clock_subsys;
	void (*irq_configure)(const struct device *dev);
};

struct renesas_ra_sci_spi_data {
	struct spi_context ctx;
	struct spi_config config;
	sci_spi_instance_ctrl_t fsp_ctrl;
	spi_cfg_t fsp_cfg;
	sci_spi_extended_cfg_t fsp_ext_cfg;
	bool is_cs_active_state_same;
#ifdef CONFIG_SPI_RENESAS_RA_SCI_INTERRUPT
	uint32_t data_len;
#endif
#ifdef CONFIG_SPI_RENESAS_RA_SCI_DTC
	/* RX */
	struct st_transfer_instance rx_transfer;
	struct st_dtc_instance_ctrl rx_transfer_ctrl;
	struct st_transfer_info rx_transfer_info DTC_TRANSFER_INFO_ALIGNMENT;
	struct st_transfer_cfg rx_transfer_cfg;
	struct st_dtc_extended_cfg rx_transfer_cfg_extend;

	/* TX */
	struct st_transfer_instance tx_transfer;
	struct st_dtc_instance_ctrl tx_transfer_ctrl;
	struct st_transfer_info tx_transfer_info DTC_TRANSFER_INFO_ALIGNMENT;
	struct st_transfer_cfg tx_transfer_cfg;
	struct st_dtc_extended_cfg tx_transfer_cfg_extend;
#endif /* CONFIG_SPI_RENESAS_RA_SCI_DTC */
};

#ifdef CONFIG_SPI_RENESAS_RA_SCI_INTERRUPT
extern void sci_spi_txi_isr(void);
extern void sci_spi_rxi_isr(void);
extern void sci_spi_tei_isr(void);
extern void sci_spi_eri_isr(void);
#endif

#define CS_GPIO_LOW_WHEN_ACTIVE  true
#define CS_GPIO_HIGH_WHEN_ACTIVE false

#define SCI_RENESAS_RA_IRQ_GET(id, name, cell)                                                     \
	COND_CODE_1(DT_IRQ_HAS_NAME(id, name), (DT_IRQ_BY_NAME(id, name, cell)),                   \
		    ((IRQn_Type) FSP_INVALID_VECTOR))
/*
 * This function help to control the cs gpio when changing the CS GPIO active state in
 * runtime.
 */
static inline void _renesas_ra_spi_context_cs_control(const struct device *dev, bool on,
						      bool force_off)
{
	struct renesas_ra_sci_spi_data *data = dev->data;
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

/*
 * This function should be called by drivers to control the chip select line in master mode
 * in the case of the CS being a GPIO, help to control the cs gpio when changing the CS GPIO
 * active state in runtime.
 */
static inline void renesas_ra_spi_context_cs_control(const struct device *dev, bool on)
{
	_renesas_ra_spi_context_cs_control(dev, on, false);
}

/*
 * Forcefully releases the spi context and removes the owner, allowing taking the lock
 * with spi_context_lock without the previous owner releasing the lock.
 * This function help to control the cs gpio when changing the CS GPIO active state in
 * runtime.
 */
static inline void renesas_ra_spi_context_unlock_unconditionally(const struct device *dev)
{
	struct renesas_ra_sci_spi_data *data = dev->data;
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
static inline bool renesas_ra_sci_context_configured(const struct device *dev,
						     const struct spi_config *config)
{
	struct renesas_ra_sci_spi_data *data = dev->data;

	if ((data->config.frequency == config->frequency) &&
	    (data->config.operation == config->operation) &&
	    (data->config.slave == config->slave)) {
		return true;
	}

	return false;
}

#ifdef CONFIG_SPI_RENESAS_RA_SCI_INTERRUPT
static bool renesas_ra_sci_spi_transfer_ongoing(struct renesas_ra_sci_spi_data *data)
{
	return (spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx));
}

static void renesas_ra_sci_spi_retransmit(struct renesas_ra_sci_spi_data *data)
{
	fsp_err_t fsp_err;

	data->data_len = spi_context_max_continuous_chunk(&data->ctx);

	if (data->ctx.rx_buf == NULL) {
		fsp_err = R_SCI_SPI_Write(&data->fsp_ctrl, data->ctx.tx_buf, data->data_len,
					  SPI_BIT_WIDTH_8_BITS);
	} else if (data->ctx.tx_buf == NULL) {
		fsp_err = R_SCI_SPI_Read(&data->fsp_ctrl, data->ctx.rx_buf, data->data_len,
					 SPI_BIT_WIDTH_8_BITS);
	} else {
		fsp_err = R_SCI_SPI_WriteRead(&data->fsp_ctrl, data->ctx.tx_buf, data->ctx.rx_buf,
					      data->data_len, SPI_BIT_WIDTH_8_BITS);
	}

	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("SCI SPI transfer failed %d", fsp_err);
		return;
	}
}

static void renesas_ra_sci_spi_callback(spi_callback_args_t *p_args)
{
	struct device *dev = (struct device *)p_args->p_context;
	struct renesas_ra_sci_spi_data *data = dev->data;
	uint32_t data_receive_len;

	switch (p_args->event) {
	case SPI_EVENT_TRANSFER_COMPLETE:
		if (!spi_context_is_slave(&data->ctx)) {
			if (data->fsp_ctrl.rx_count == data->fsp_ctrl.count ||
			    data->fsp_ctrl.tx_count == data->fsp_ctrl.count) {

				data_receive_len = !!data->fsp_ctrl.rx_count
							   ? data->fsp_ctrl.rx_count
							   : data->ctx.rx_len;

				spi_context_update_rx(&data->ctx, 1, data_receive_len);
			}

			if (data->fsp_ctrl.tx_count == data->fsp_ctrl.count) {
				spi_context_update_tx(&data->ctx, 1, data->data_len);
			}

			if (renesas_ra_sci_spi_transfer_ongoing(data)) {
				renesas_ra_sci_spi_retransmit(data);
				return;
			}
		}
#ifdef CONFIG_SPI_SLAVE
		else if (data->fsp_ctrl.rx_count == data->fsp_ctrl.count) {
			if (data->ctx.rx_buf != NULL && data->ctx.tx_buf != NULL) {
				data->ctx.recv_frames = MIN(spi_context_total_tx_len(&data->ctx),
							    spi_context_total_rx_len(&data->ctx));
			} else if (data->ctx.tx_buf == NULL) {
				data->ctx.recv_frames = data->data_len;
			} else {
				/* Do nothing */
			}
		}
#endif /* CONFIG_SPI_SLAVE */
		renesas_ra_spi_context_cs_control(dev, false);
		spi_context_complete(&data->ctx, dev, 0);
		break;
	case SPI_EVENT_ERR_READ_OVERFLOW:
		renesas_ra_spi_context_cs_control(dev, false);
		spi_context_complete(&data->ctx, dev, -EIO);
		break;
	default:
		break;
	}
}
#endif

static int renesas_ra_sci_spi_configure(const struct device *dev, const struct spi_config *config)
{
	struct renesas_ra_sci_spi_data *data = dev->data;
	fsp_err_t fsp_err;

	/* Check whether the congiguration is changed */
	if (renesas_ra_sci_context_configured(dev, config)) {
		return 0;
	}

	if ((config->operation & SPI_FRAME_FORMAT_TI) == SPI_FRAME_FORMAT_TI) {
		LOG_ERR("TI frame format is not supported");
		return -ENOTSUP;
	}

	if (SPI_MODE_GET(config->operation) & SPI_MODE_LOOP) {
		LOG_ERR("Internal hardware loopback is not supported");
		return -ENOTSUP;
	}

	if (SPI_WORD_SIZE_GET(config->operation) != 8) {
		LOG_ERR("Word sizes other than 8 bits are not supported");
		return -ENOTSUP;
	}

	if ((config->operation & SPI_OP_MODE_SLAVE) && !IS_ENABLED(CONFIG_SPI_SLAVE)) {
		LOG_ERR("Kconfig for enable SPI in slave mode is not enabled");
		return -ENOTSUP;
	}

	if (config->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	if ((SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_MASTER) &&
	    (config->frequency == 0)) {
		LOG_ERR("Invalid frequency value");
		return -EINVAL;
	}

	if (config->frequency > 2500000) {
		LOG_ERR("Frequencies more than 2,5 MHz are not supported");
		return -EINVAL;
	}

	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_SLAVE) {
		data->fsp_cfg.operating_mode = SPI_MODE_SLAVE;
	} else {
		data->fsp_cfg.operating_mode = SPI_MODE_MASTER;
	}

	if (SPI_MODE_GET(config->operation) & SPI_MODE_CPOL) {
		data->fsp_cfg.clk_polarity = SPI_CLK_POLARITY_HIGH;
	} else {
		data->fsp_cfg.clk_polarity = SPI_CLK_POLARITY_LOW;
	}

	if (SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) {
		data->fsp_cfg.clk_phase = SPI_CLK_PHASE_EDGE_EVEN;
	} else {
		data->fsp_cfg.clk_phase = SPI_CLK_PHASE_EDGE_ODD;
	}

	if (config->operation & SPI_TRANSFER_LSB) {
		data->fsp_cfg.bit_order = SPI_BIT_ORDER_LSB_FIRST;
	} else {
		data->fsp_cfg.bit_order = SPI_BIT_ORDER_MSB_FIRST;
	}

	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_MASTER) {
		fsp_err = R_SCI_SPI_CalculateBitrate(config->frequency, &data->fsp_ext_cfg.clk_div,
						     true);
		if (fsp_err != FSP_SUCCESS) {
			return -EINVAL;
		}
	}

	data->fsp_cfg.p_extend = &data->fsp_ext_cfg;
#ifdef CONFIG_SPI_RENESAS_RA_SCI_INTERRUPT
	data->fsp_cfg.p_callback = renesas_ra_sci_spi_callback;
#else
	data->fsp_cfg.p_callback = NULL;
#endif /* CONFIG_SPI_RENESAS_RA_SCI_INTERRUPT */
	data->fsp_cfg.p_context = (void *)dev;

	if (data->fsp_ctrl.open != 0) {
		fsp_err = R_SCI_SPI_Close(&data->fsp_ctrl);
		if (fsp_err != FSP_SUCCESS) {
			return -EIO;
		}

		memset(&data->config, 0, sizeof(struct spi_config));
	}

	fsp_err = R_SCI_SPI_Open(&data->fsp_ctrl, &data->fsp_cfg);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("Failed to apply spi configuration");
		return -EINVAL;
	}

	memcpy(&data->config, config, sizeof(struct spi_config));
	data->ctx.config = &data->config;

	return 0;
}

static int transceive(const struct device *dev, const struct spi_config *config,
		      const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
		      bool asynchronous, spi_callback_t cb, void *userdata)
{
	struct renesas_ra_sci_spi_data *data = dev->data;
	bool cs_gpio_logic_when_active;
	bool cs_active_logic_config;
	int ret;
	fsp_err_t fsp_err;
#ifndef CONFIG_SPI_RENESAS_RA_SCI_INTERRUPT
	size_t len;
#endif

	if (!tx_bufs && !rx_bufs) {
		return 0;
	}

#ifndef CONFIG_SPI_RENESAS_RA_SCI_INTERRUPT
	if (asynchronous) {
		return -ENOTSUP;
	}
#endif /* CONFIG_SPI_RENESAS_RA_SCI_INTERRUPT */

	spi_context_lock(&data->ctx, asynchronous, cb, userdata, config);

	ret = renesas_ra_sci_spi_configure(dev, config);
	if (ret) {
		goto release;
	}

	/*
	 * For SCI SPI, the hardware only supports 8-bit frames,
	 * so the data frame size must be 1 byte
	 */
	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

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

	/* If current buffer has no data, do nothing */
	if (!spi_context_tx_buf_on(&data->ctx) && !spi_context_rx_buf_on(&data->ctx)) {
		goto release;
	}

#ifdef CONFIG_SPI_RENESAS_RA_SCI_INTERRUPT
	if (data->ctx.rx_len == 0) {
		data->data_len = spi_context_is_slave(&data->ctx)
					 ? spi_context_total_tx_len(&data->ctx)
					 : data->ctx.tx_len;
	} else if (data->ctx.tx_len == 0) {
		data->data_len = spi_context_is_slave(&data->ctx)
					 ? spi_context_total_rx_len(&data->ctx)
					 : data->ctx.rx_len;
	} else {
		data->data_len = spi_context_is_slave(&data->ctx)
					 ? MAX(spi_context_total_tx_len(&data->ctx),
					       spi_context_total_rx_len(&data->ctx))
					 : MIN(data->ctx.tx_len, data->ctx.rx_len);
	}

	if (data->ctx.rx_buf == NULL) {
		fsp_err = R_SCI_SPI_Write(&data->fsp_ctrl, data->ctx.tx_buf, data->data_len,
					  SPI_BIT_WIDTH_8_BITS);
	} else if (data->ctx.tx_buf == NULL) {
		fsp_err = R_SCI_SPI_Read(&data->fsp_ctrl, data->ctx.rx_buf, data->data_len,
					 SPI_BIT_WIDTH_8_BITS);
	} else {
		fsp_err = R_SCI_SPI_WriteRead(&data->fsp_ctrl, data->ctx.tx_buf, data->ctx.rx_buf,
					      data->data_len, SPI_BIT_WIDTH_8_BITS);
	}

	if (fsp_err != FSP_SUCCESS) {
		ret = -EIO;
		goto release;
	}

	ret = spi_context_wait_for_completion(&data->ctx);
#else
	fsp_err = RP_SCI_SPI_StartTransferPolling(&data->fsp_ctrl);
	if (fsp_err != FSP_SUCCESS) {
		ret = -EIO;
		goto release;
	}

	while (spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx)) {
		size_t tx_len = data->ctx.tx_len;
		size_t rx_len = data->ctx.rx_len;

		len = MIN(tx_len, rx_len);
		if (len > 0) {
			fsp_err = RP_SCI_SPI_WriteReadPolling(&data->fsp_ctrl, data->ctx.tx_buf,
							      data->ctx.rx_buf, len);
			if (fsp_err != FSP_SUCCESS) {
				ret = -EIO;
				break;
			}

			if (spi_context_tx_buf_on(&data->ctx)) {
				spi_context_update_tx(&data->ctx, 1, len);
			}

			if (spi_context_rx_on(&data->ctx)) {
				spi_context_update_rx(&data->ctx, 1, len);
			}
		}

		if (spi_context_tx_on(&data->ctx) && !spi_context_rx_on(&data->ctx)) {
			fsp_err = RP_SCI_SPI_WritePolling(&data->fsp_ctrl, data->ctx.tx_buf,
							  data->ctx.tx_len);
			if (fsp_err != FSP_SUCCESS) {
				ret = -EIO;
				break;
			}

			spi_context_update_tx(&data->ctx, 1, data->ctx.tx_len);
		}

		if (spi_context_rx_on(&data->ctx) && !spi_context_tx_on(&data->ctx)) {
			fsp_err = RP_SCI_SPI_ReadPolling(&data->fsp_ctrl, data->ctx.rx_buf,
							 data->ctx.rx_len);
			if (fsp_err != FSP_SUCCESS) {
				ret = -EIO;
				break;
			}

			spi_context_update_rx(&data->ctx, 1, data->ctx.rx_len);
		}
	}

	fsp_err = RP_SCI_SPI_EndTransferPolling(&data->fsp_ctrl);
	if (fsp_err != FSP_SUCCESS) {
		ret = -EIO;
	}

	spi_context_complete(&data->ctx, dev, ret);
#endif /* CONFIG_SPI_RENESAS_RA_SCI_INTERRUPT */

#ifdef CONFIG_SPI_SLAVE
	if (spi_context_is_slave(&data->ctx) && !ret) {
		ret = data->ctx.recv_frames;
	}
#endif /* CONFIG_SPI_SLAVE */

release:
	if ((ret < 0) || !IS_ENABLED(CONFIG_SPI_RENESAS_RA_SCI_INTERRUPT)) {
		renesas_ra_spi_context_cs_control(dev, false);
	}

	spi_context_release(&data->ctx, ret);

	return ret;
}

static int renesas_ra_sci_spi_transceive(const struct device *dev, const struct spi_config *config,
					 const struct spi_buf_set *tx_bufs,
					 const struct spi_buf_set *rx_bufs)
{
	return transceive(dev, config, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int renesas_ra_sci_spi_transceive_async(const struct device *dev,
					       const struct spi_config *config,
					       const struct spi_buf_set *tx_bufs,
					       const struct spi_buf_set *rx_bufs, spi_callback_t cb,
					       void *userdata)
{
	return transceive(dev, config, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif /* CONFIG_SPI_ASYNC */

static int renesas_ra_sci_spi_release(const struct device *dev, const struct spi_config *config)
{
	ARG_UNUSED(config);
	renesas_ra_spi_context_unlock_unconditionally(dev);

	return 0;
}

static int renesas_ra_sci_spi_init(const struct device *dev)
{
	const struct renesas_ra_sci_spi_config *config = dev->config;
	struct renesas_ra_sci_spi_data *data = dev->data;
	const struct device *clock_dev = config->clock_dev;
	int ret;
#ifdef DT_SPI_CTX_HAS_NO_CS_GPIOS
	ARG_UNUSED(data);
#endif

#ifdef CONFIG_SPI_RENESAS_RA_SCI_DTC
	data->fsp_cfg.p_transfer_rx = &data->rx_transfer;
	data->fsp_cfg.p_transfer_tx = &data->tx_transfer;
#endif

	if (!device_is_ready(clock_dev)) {
		return -ENODEV;
	}

	ret = clock_control_on(config->clock_dev, (clock_control_subsys_t)&config->clock_subsys);
	if (ret < 0) {
		return ret;
	}

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	ret = spi_context_cs_configure_all(&data->ctx);
	if (ret < 0) {
		return ret;
	}

	config->irq_configure(dev);

	renesas_ra_spi_context_unlock_unconditionally(dev);

	return 0;
}

static DEVICE_API(spi, renesas_ra_sci_spi_driver_api) = {
	.transceive = renesas_ra_sci_spi_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = renesas_ra_sci_spi_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
	.release = renesas_ra_sci_spi_release,
};

#define EVENT_SCI_RXI(channel) BSP_PRV_IELS_ENUM(CONCAT(EVENT_SCI, channel, _RXI))
#define EVENT_SCI_TXI(channel) BSP_PRV_IELS_ENUM(CONCAT(EVENT_SCI, channel, _TXI))
#define EVENT_SCI_TEI(channel) BSP_PRV_IELS_ENUM(CONCAT(EVENT_SCI, channel, _TEI))
#define EVENT_SCI_ERI(channel) BSP_PRV_IELS_ENUM(CONCAT(EVENT_SCI, channel, _ERI))

#ifdef CONFIG_SPI_RENESAS_RA_SCI_INTERRUPT

#define RENESAS_RA_IRQ_CONFIG_FUNC(index)                                                          \
	static void sci_spi_config_func_##index(const struct device *dev)                          \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
                                                                                                   \
		R_ICU->IELSR[DT_IRQ_BY_NAME(DT_INST_PARENT(index), rxi, irq)] =                    \
			EVENT_SCI_RXI(DT_INST_PROP(index, channel));                               \
		R_ICU->IELSR[DT_IRQ_BY_NAME(DT_INST_PARENT(index), txi, irq)] =                    \
			EVENT_SCI_TXI(DT_INST_PROP(index, channel));                               \
		R_ICU->IELSR[DT_IRQ_BY_NAME(DT_INST_PARENT(index), tei, irq)] =                    \
			EVENT_SCI_TEI(DT_INST_PROP(index, channel));                               \
		R_ICU->IELSR[DT_IRQ_BY_NAME(DT_INST_PARENT(index), eri, irq)] =                    \
			EVENT_SCI_ERI(DT_INST_PROP(index, channel));                               \
                                                                                                   \
		IRQ_CONNECT(DT_IRQ_BY_NAME(DT_INST_PARENT(index), rxi, irq),                       \
			    DT_IRQ_BY_NAME(DT_INST_PARENT(index), rxi, priority), sci_spi_rxi_isr, \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		IRQ_CONNECT(DT_IRQ_BY_NAME(DT_INST_PARENT(index), txi, irq),                       \
			    DT_IRQ_BY_NAME(DT_INST_PARENT(index), txi, priority), sci_spi_txi_isr, \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		IRQ_CONNECT(DT_IRQ_BY_NAME(DT_INST_PARENT(index), tei, irq),                       \
			    DT_IRQ_BY_NAME(DT_INST_PARENT(index), tei, priority), sci_spi_tei_isr, \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		IRQ_CONNECT(DT_IRQ_BY_NAME(DT_INST_PARENT(index), eri, irq),                       \
			    DT_IRQ_BY_NAME(DT_INST_PARENT(index), eri, priority), sci_spi_eri_isr, \
			    DEVICE_DT_INST_GET(index), 0);                                         \
                                                                                                   \
		irq_enable(DT_IRQ_BY_NAME(DT_INST_PARENT(index), rxi, irq));                       \
		irq_enable(DT_IRQ_BY_NAME(DT_INST_PARENT(index), txi, irq));                       \
		irq_enable(DT_IRQ_BY_NAME(DT_INST_PARENT(index), eri, irq));                       \
		irq_enable(DT_IRQ_BY_NAME(DT_INST_PARENT(index), tei, irq));                       \
	}
#else

#define RENESAS_RA_IRQ_CONFIG_FUNC(index)                                                          \
	static void sci_spi_config_func_##index(const struct device *dev)                          \
	{                                                                                          \
	}
#endif

/* clang-format off */

#ifndef CONFIG_SPI_RENESAS_RA_SCI_DTC
#define RA_SCI_SPI_DTC_STRUCT_INIT(index)
#else
#define RA_SCI_SPI_DTC_STRUCT_INIT(index)                                                          \
	.rx_transfer_info = {                                                                      \
		.transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED,         \
		.transfer_settings_word_b.repeat_area = TRANSFER_REPEAT_AREA_DESTINATION,          \
		.transfer_settings_word_b.irq = TRANSFER_IRQ_END,                                  \
		.transfer_settings_word_b.chain_mode = TRANSFER_CHAIN_MODE_DISABLED,               \
		.transfer_settings_word_b.src_addr_mode = TRANSFER_ADDR_MODE_FIXED,                \
		.transfer_settings_word_b.size = TRANSFER_SIZE_1_BYTE,                             \
		.transfer_settings_word_b.mode = TRANSFER_MODE_NORMAL,                             \
		.p_dest = (void *)NULL,                                                            \
		.p_src = (void const *)NULL,                                                       \
		.num_blocks = 0,                                                                   \
		.length = 0,                                                                       \
	},                                                                                         \
	.rx_transfer_cfg_extend = {                                                                \
		.activation_source = DT_IRQ_BY_NAME(DT_INST_PARENT(index), rxi, irq),              \
	},                                                                                         \
	.rx_transfer_cfg = {                                                                       \
		.p_info = &renesas_ra_sci_spi_data_##index.rx_transfer_info,                       \
		.p_extend = &renesas_ra_sci_spi_data_##index.rx_transfer_cfg_extend,               \
	},                                                                                         \
	.rx_transfer = {                                                                           \
		.p_ctrl = &renesas_ra_sci_spi_data_##index.rx_transfer_ctrl,                       \
		.p_cfg = &renesas_ra_sci_spi_data_##index.rx_transfer_cfg,                         \
		.p_api = &g_transfer_on_dtc,                                                       \
	},                                                                                         \
	.tx_transfer_info = {                                                                      \
		.transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_FIXED,               \
		.transfer_settings_word_b.repeat_area = TRANSFER_REPEAT_AREA_SOURCE,               \
		.transfer_settings_word_b.irq = TRANSFER_IRQ_END,                                  \
		.transfer_settings_word_b.chain_mode = TRANSFER_CHAIN_MODE_DISABLED,               \
		.transfer_settings_word_b.src_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED,          \
		.transfer_settings_word_b.size = TRANSFER_SIZE_1_BYTE,                             \
		.transfer_settings_word_b.mode = TRANSFER_MODE_NORMAL,                             \
		.p_dest = (void *)NULL,                                                            \
		.p_src = (void const *)NULL,                                                       \
		.num_blocks = 0,                                                                   \
		.length = 0,                                                                       \
	},                                                                                         \
	.tx_transfer_cfg_extend = {                                                                \
		.activation_source = DT_IRQ_BY_NAME(DT_INST_PARENT(index), txi, irq),              \
	},                                                                                         \
	.tx_transfer_cfg = {                                                                       \
		.p_info = &renesas_ra_sci_spi_data_##index.tx_transfer_info,                       \
		.p_extend = &renesas_ra_sci_spi_data_##index.tx_transfer_cfg_extend,               \
	},                                                                                         \
	.tx_transfer = {                                                                           \
		.p_ctrl = &renesas_ra_sci_spi_data_##index.tx_transfer_ctrl,                       \
		.p_cfg = &renesas_ra_sci_spi_data_##index.tx_transfer_cfg,                         \
		.p_api = &g_transfer_on_dtc,                                                       \
	},
#endif

#define RENESAS_RA_SPI_SCI_INIT(index)                                                             \
	RENESAS_RA_IRQ_CONFIG_FUNC(index)                                                          \
	PINCTRL_DT_DEFINE(DT_INST_PARENT(index));                                                  \
                                                                                                   \
	static const struct renesas_ra_sci_spi_config renesas_ra_sci_spi_config_##index = {        \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(DT_INST_PARENT(index)),                          \
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_INST_PARENT(index))),                 \
		.clock_subsys = {                                                                  \
			.mstp = DT_CLOCKS_CELL_BY_IDX(DT_INST_PARENT(index), 0, mstp),             \
			.stop_bit = DT_CLOCKS_CELL_BY_IDX(DT_INST_PARENT(index), 0, stop_bit),     \
		},                                                                                 \
		.irq_configure = sci_spi_config_func_##index,                                      \
	};                                                                                         \
                                                                                                   \
	static struct renesas_ra_sci_spi_data renesas_ra_sci_spi_data_##index = {                  \
		.is_cs_active_state_same = true,                                                   \
		.fsp_cfg = {                                                                       \
			.channel = DT_INST_PROP(index, channel),                                   \
			.rxi_ipl = SCI_RENESAS_RA_IRQ_GET(DT_INST_PARENT(index), rxi, priority),   \
			.rxi_irq = SCI_RENESAS_RA_IRQ_GET(DT_INST_PARENT(index), rxi, irq),        \
			.txi_ipl = SCI_RENESAS_RA_IRQ_GET(DT_INST_PARENT(index), txi, priority),   \
			.txi_irq = SCI_RENESAS_RA_IRQ_GET(DT_INST_PARENT(index), txi, irq),        \
			.tei_ipl = SCI_RENESAS_RA_IRQ_GET(DT_INST_PARENT(index), tei, priority),   \
			.tei_irq = SCI_RENESAS_RA_IRQ_GET(DT_INST_PARENT(index), tei, irq),        \
			.eri_ipl = SCI_RENESAS_RA_IRQ_GET(DT_INST_PARENT(index), eri, priority),   \
			.eri_irq = SCI_RENESAS_RA_IRQ_GET(DT_INST_PARENT(index), eri, irq),        \
		},                                                                                 \
		SPI_CONTEXT_INIT_LOCK(renesas_ra_sci_spi_data_##index, ctx),                       \
		SPI_CONTEXT_INIT_SYNC(renesas_ra_sci_spi_data_##index, ctx),                       \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(index), ctx)                           \
		RA_SCI_SPI_DTC_STRUCT_INIT(index)                                                  \
	};                                                                                         \
	                                                                                           \
	SPI_DEVICE_DT_INST_DEFINE(index, renesas_ra_sci_spi_init, NULL,                            \
				  &renesas_ra_sci_spi_data_##index,                                \
				  &renesas_ra_sci_spi_config_##index, POST_KERNEL,                 \
				  CONFIG_SPI_INIT_PRIORITY, &renesas_ra_sci_spi_driver_api);

/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(RENESAS_RA_SPI_SCI_INIT)
