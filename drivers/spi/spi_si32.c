/*
 * Copyright (c) 2024 GARDENA GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_si32_spi

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_silabs_si32);

#include "spi_context.h"

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/sys_io.h>

#include <SI32_CLKCTRL_A_Type.h>
#include <SI32_SPI_A_Type.h>
#include <si32_device.h>

#include <stdbool.h>

struct spi_si32_data {
	struct spi_context ctx;
};

struct spi_si32_config {
	SI32_SPI_A_Type *spi;
	void (*irq_connect)(void);
	unsigned int irq;
	const struct device *clock_dev;
};

static int spi_si32_configure(const struct device *dev, const struct spi_config *config)
{
	const struct spi_si32_config *si32_config = dev->config;
	struct spi_si32_data *data = dev->data;
	uint32_t apb_freq;
	uint32_t word_size;
	int ret;

	if (spi_context_configured(&data->ctx, config)) {
		return 0;
	}

	if (!device_is_ready(si32_config->clock_dev)) {
		LOG_ERR("source clock is not ready");
		return -ENODEV;
	}

	if (SPI_OP_MODE_GET(config->operation) != SPI_OP_MODE_MASTER) {
		LOG_ERR("only master mode is supported right now");
		return -ENOTSUP;
	}
	if (SPI_OP_MODE_GET(config->operation) & ~(SPI_MODE_CPOL | SPI_MODE_CPHA)) {
		LOG_ERR("unsupported mode flags: 0x%x", SPI_OP_MODE_GET(config->operation));
		return -ENOTSUP;
	}

	if ((config->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("unsupported lines config: 0x%x", config->operation & SPI_LINES_MASK);
		return -ENOTSUP;
	}

	word_size = SPI_WORD_SIZE_GET(config->operation);
	if (word_size == 0 || word_size > 16) {
		LOG_ERR("sunsupported word size: %u", word_size);
		return -ENOTSUP;
	}

	ret = clock_control_get_rate(si32_config->clock_dev, NULL, &apb_freq);
	if (ret) {
		LOG_ERR("failed to get source clock rate: %d", ret);
		return ret;
	}

	SI32_SPI_A_set_clock_divisor(si32_config->spi, apb_freq / config->frequency);

	SI32_SPI_A_disable_module(si32_config->spi);

	if (config->cs.gpio.port != NULL) {
		SI32_SPI_A_select_3wire_master_mode(si32_config->spi);
	} else {
		if (config->operation & SPI_CS_ACTIVE_HIGH) {
			SI32_SPI_A_select_4wire_master_mode_nss_low(si32_config->spi);
		} else {
			SI32_SPI_A_select_4wire_master_mode_nss_high(si32_config->spi);
		}
	}

	SI32_SPI_A_set_data_length(si32_config->spi, word_size);

	if (config->operation & SPI_TRANSFER_LSB) {
		SI32_SPI_A_select_direction_lsb_first(si32_config->spi);
	} else {
		SI32_SPI_A_select_direction_msb_first(si32_config->spi);
	}

	if (SPI_MODE_GET(config->operation) & SPI_MODE_CPOL) {
		SI32_SPI_A_select_clock_idle_high(si32_config->spi);
	} else {
		SI32_SPI_A_select_clock_idle_low(si32_config->spi);
	}

	if (SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) {
		SI32_SPI_A_select_data_change_first_edge(si32_config->spi);
	} else {
		SI32_SPI_A_select_data_change_second_edge(si32_config->spi);
	}

	SI32_SPI_A_select_master_mode(si32_config->spi);

	SI32_SPI_A_select_tx_fifo_threshold(si32_config->spi, SI32_SPI_FIFO_THRESHOLD_ONE);
	SI32_SPI_A_select_rx_fifo_threshold(si32_config->spi, SI32_SPI_FIFO_THRESHOLD_ONE);

	SI32_SPI_A_disable_rx_fifo_read_request_interrupt(si32_config->spi);
	SI32_SPI_A_disable_tx_fifo_write_request_interrupt(si32_config->spi);
	SI32_SPI_A_disable_rx_fifo_read_request_interrupt(si32_config->spi);
	SI32_SPI_A_disable_shift_register_empty_interrupt(si32_config->spi);
	SI32_SPI_A_disable_underrun_interrupt(si32_config->spi);
	SI32_SPI_A_enable_rx_fifo_overrun_interrupt(si32_config->spi);
	SI32_SPI_A_enable_tx_fifo_overrun_interrupt(si32_config->spi);
	SI32_SPI_A_enable_mode_fault_interrupt(si32_config->spi);
	SI32_SPI_A_clear_all_interrupts(si32_config->spi);

	SI32_SPI_A_enable_module(si32_config->spi);

	SI32_SPI_A_enable_stall_in_debug_mode(si32_config->spi);

	data->ctx.config = config;

	return 0;
}

static void spi_si32_cs_control_hw(const struct device *dev, bool on, bool force_off)
{
	const struct spi_si32_config *si32_config = dev->config;
	struct spi_si32_data *data = dev->data;
	const struct spi_config *config = data->ctx.config;

	if (on) {
		SI32_SPI_A_clear_nss(si32_config->spi);
	} else {
		if (!force_off && config->operation & SPI_HOLD_ON_CS) {
			return;
		}

		SI32_SPI_A_set_nss(si32_config->spi);
	}
}

static void spi_si32_cs_control(const struct device *dev, bool on)
{
	struct spi_si32_data *data = dev->data;
	const struct spi_config *config = data->ctx.config;

	if (!data->ctx.config) {
		LOG_ERR("can't control CS without config");
		return;
	}

	if (config->cs.gpio.port != NULL) {
		spi_context_cs_control(&data->ctx, on);
		return;
	}

	spi_si32_cs_control_hw(dev, on, false);
}

static int spi_si32_transceive(const struct device *dev, const struct spi_config *config,
			       const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs)
{
	const struct spi_si32_config *si32_config = dev->config;
	struct spi_si32_data *data = dev->data;
	int ret;

	spi_context_lock(&data->ctx, false, NULL, NULL, config);

	ret = spi_si32_configure(dev, config);
	if (ret) {
		goto release;
	}

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	spi_si32_cs_control(dev, true);

	/* Flush SPI FIFOs */
	SI32_SPI_A_flush_rx_fifo(si32_config->spi);
	while (si32_config->spi->CONFIG.RFIFOFL) {
		;
	}
	SI32_SPI_A_flush_tx_fifo(si32_config->spi);
	while (si32_config->spi->CONFIG.TFIFOFL) {
		;
	}

	/* Clear all interrupts */
	SI32_SPI_A_clear_all_interrupts(si32_config->spi);
	NVIC_ClearPendingIRQ(si32_config->irq);

	/* Enable relevant interrupts */
	irq_enable(si32_config->irq);
	SI32_SPI_A_enable_rx_fifo_read_request_interrupt(si32_config->spi);
	SI32_SPI_A_enable_shift_register_empty_interrupt(si32_config->spi);

	ret = spi_context_wait_for_completion(&data->ctx);

	spi_si32_cs_control(dev, false);

release:
	spi_context_release(&data->ctx, ret);
	return ret;
}

static int spi_si32_release(const struct device *dev, const struct spi_config *config)
{
	ARG_UNUSED(config);

	struct spi_si32_data *data = dev->data;

	/* spi_context_unlock_unconditionally handles the software path already */
	spi_si32_cs_control_hw(dev, false, true);

	spi_context_unlock_unconditionally(&data->ctx);
	return 0;
}

static bool spi_si32_transfer_ongoing(struct spi_si32_data *data)
{
	return spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx);
}

static void spi_si32_irq_handler(const struct device *dev)
{
	const struct spi_si32_config *config = dev->config;
	struct spi_si32_data *data = dev->data;
	uint8_t byte = 0;
	int err = 0;

	if (SI32_SPI_A_is_rx_fifo_read_request_interrupt_pending(config->spi)) {
		byte = SI32_SPI_A_read_rx_fifo_u8(config->spi);

		if (spi_context_rx_buf_on(&data->ctx)) {
			UNALIGNED_PUT(byte, (uint8_t *)data->ctx.rx_buf);
		}

		spi_context_update_rx(&data->ctx, 1, 1);
	}

	else if (SI32_SPI_A_is_shift_register_empty_interrupt_pending(config->spi)) {
		if (spi_context_tx_buf_on(&data->ctx)) {
			byte = UNALIGNED_GET((uint8_t *)(data->ctx.tx_buf));
		}

		SI32_SPI_A_write_tx_fifo_u8(config->spi, byte);

		spi_context_update_tx(&data->ctx, 1, 1);
	}

	if (SI32_SPI_A_is_rx_fifo_overrun_interrupt_pending(config->spi)) {
		LOG_ERR("RX FIFO overrun");
		err = -EIO;
	}
	if (SI32_SPI_A_is_tx_fifo_overrun_interrupt_pending(config->spi)) {
		LOG_ERR("TX FIFO overrun");
		err = -EIO;
	}
	if (SI32_SPI_A_is_mode_fault_interrupt_pending(config->spi)) {
		LOG_ERR("mode fault");
		err = -EIO;
	}
	if (SI32_SPI_A_is_illegal_rx_fifo_access_interrupt_pending(config->spi)) {
		LOG_ERR("illegal RX FIFO access");
		err = -EIO;
	}
	if (SI32_SPI_A_is_illegal_tx_fifo_access_interrupt_pending(config->spi)) {
		LOG_ERR("illegal TX FIFO access");
		err = -EIO;
	}

	SI32_SPI_A_clear_all_interrupts(config->spi);

	if (err || !spi_si32_transfer_ongoing(data)) {
		SI32_SPI_A_disable_rx_fifo_read_request_interrupt(config->spi);
		SI32_SPI_A_disable_shift_register_empty_interrupt(config->spi);
		irq_disable(config->irq);
		SI32_SPI_A_clear_all_interrupts(config->spi);
		NVIC_ClearPendingIRQ(config->irq);

		spi_context_complete(&data->ctx, dev, err);
	}
}

static struct spi_driver_api spi_si32_api = {
	.transceive = spi_si32_transceive,
	.release = spi_si32_release,
};

static int spi_si32_init(const struct device *dev)
{
	int err;
	const struct spi_si32_config *config = dev->config;
	struct spi_si32_data *data = dev->data;

	if (config->spi == SI32_SPI_0) {
		SI32_CLKCTRL_A_enable_apb_to_modules_0(SI32_CLKCTRL_0,
						       SI32_CLKCTRL_A_APBCLKG0_SPI0);
	} else if (config->spi == SI32_SPI_2) {
		SI32_CLKCTRL_A_enable_apb_to_modules_0(SI32_CLKCTRL_0,
						       SI32_CLKCTRL_A_APBCLKG0_SPI2);
	} else {
		LOG_ERR("unsupported SPI device");
		return -ENOTSUP;
	}

	irq_disable(config->irq);
	config->irq_connect();

	err = spi_context_cs_configure_all(&data->ctx);
	if (err < 0) {
		return err;
	}

	/* Make sure the context is unlocked */
	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#define SI32_SPI_INIT(n)                                                                           \
	static void irq_connect_##n(void)                                                          \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), spi_si32_irq_handler,       \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
	static struct spi_si32_data spi_si32_data_##n = {                                          \
		SPI_CONTEXT_INIT_LOCK(spi_si32_data_##n, ctx),                                     \
		SPI_CONTEXT_INIT_SYNC(spi_si32_data_##n, ctx),                                     \
	};                                                                                         \
	static struct spi_si32_config spi_si32_cfg_##n = {                                         \
		.spi = (SI32_SPI_A_Type *)DT_INST_REG_ADDR(n),                                     \
		.irq_connect = irq_connect_##n,                                                    \
		.irq = DT_INST_IRQN(n),                                                            \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, spi_si32_init, NULL, &spi_si32_data_##n, &spi_si32_cfg_##n,       \
			      POST_KERNEL, CONFIG_SPI_INIT_PRIORITY, &spi_si32_api);

DT_INST_FOREACH_STATUS_OKAY(SI32_SPI_INIT)
