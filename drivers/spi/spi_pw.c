/*
 * Copyright (c) 2023 Intel Corporation.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_penwell_spi

#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(pcie)
BUILD_ASSERT(IS_ENABLED(CONFIG_PCIE), "DT need CONFIG_PCIE");
#include <zephyr/drivers/pcie/pcie.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_pw, CONFIG_SPI_LOG_LEVEL);

#include "spi_pw.h"

static uint32_t spi_pw_reg_read(const struct device *dev, uint32_t offset)
{
	return sys_read32(DEVICE_MMIO_GET(dev) + offset);
}

static void spi_pw_reg_write(const struct device *dev,
			     uint32_t offset,
			     uint32_t val)
{
	return sys_write32(val, DEVICE_MMIO_GET(dev) + offset);
}

static void spi_pw_ssp_reset(const struct device *dev)
{
	/* Bring the controller from reset state in to operational mode */
	spi_pw_reg_write(dev, PW_SPI_REG_RESETS, 0x00);
	spi_pw_reg_write(dev, PW_SPI_REG_RESETS, PW_SPI_INST_RESET);
}

#ifndef CONFIG_SPI_PW_INTERRUPT
static bool is_spi_transfer_ongoing(struct spi_pw_data *spi)
{
	return spi_context_tx_on(&spi->ctx) || spi_context_rx_on(&spi->ctx);
}
#endif

static void spi_pw_enable_cs_hw_ctrl(const struct device *dev)
{
	uint32_t cs_ctrl;

	cs_ctrl = spi_pw_reg_read(dev, PW_SPI_REG_CS_CTRL);
	cs_ctrl &= PW_SPI_CS_CTRL_HW_MODE;
	spi_pw_reg_write(dev, PW_SPI_REG_CS_CTRL, cs_ctrl);
}

static void spi_pw_cs_sw_ctrl(const struct device *dev, bool enable)
{
	uint32_t cs_ctrl;

	cs_ctrl = spi_pw_reg_read(dev, PW_SPI_REG_CS_CTRL);
	cs_ctrl &= ~(PW_SPI_CS_CTRL_CS_MASK);
	/* Enable chip select software control method */
	cs_ctrl |= PW_SPI_CS_CTRL_SW_MODE;

	if (enable) {
		cs_ctrl &= PW_SPI_CS_LOW;
	} else {
		cs_ctrl |= PW_SPI_CS_HIGH;
	}

	spi_pw_reg_write(dev, PW_SPI_REG_CS_CTRL, cs_ctrl);
}

#ifdef CONFIG_SPI_PW_INTERRUPT
static void spi_pw_intr_enable(const struct device *dev, bool rx_mask)
{
	uint32_t ctrlr1;

	ctrlr1 = spi_pw_reg_read(dev, PW_SPI_REG_CTRLR1);
	if (rx_mask) {
		ctrlr1 |= PW_SPI_INTR_BITS;
	} else {
		ctrlr1 |= PW_SPI_INTR_BITS;
		ctrlr1 &= ~(PW_SPI_INTR_MASK_RX);
	}
	spi_pw_reg_write(dev, PW_SPI_REG_CTRLR1, ctrlr1);
}

static void spi_pw_intr_disable(const struct device *dev)
{
	uint32_t ctrlr1;

	ctrlr1 = spi_pw_reg_read(dev, PW_SPI_REG_CTRLR1);
	ctrlr1 &= ~(PW_SPI_INTR_BITS);
	spi_pw_reg_write(dev, PW_SPI_REG_CTRLR1, ctrlr1);
}
#endif

static void spi_pw_ssp_enable(const struct device *dev)
{
	uint32_t ctrlr0;

	ctrlr0 = spi_pw_reg_read(dev, PW_SPI_REG_CTRLR0);
	ctrlr0 |= PW_SPI_CTRLR0_SSE_BIT;
	spi_pw_reg_write(dev, PW_SPI_REG_CTRLR0, ctrlr0);

}

static void spi_pw_ssp_disable(const struct device *dev)
{
	uint32_t ctrlr0;

	ctrlr0 = spi_pw_reg_read(dev, PW_SPI_REG_CTRLR0);
	ctrlr0 &= ~(PW_SPI_CTRLR0_SSE_BIT);
	spi_pw_reg_write(dev, PW_SPI_REG_CTRLR0, ctrlr0);
}

static bool is_pw_ssp_busy(const struct device *dev)
{
	uint32_t status;

	status = spi_pw_reg_read(dev, PW_SPI_REG_SSSR);
	return (status & PW_SPI_SSSR_BSY_BIT) ? true : false;
}

static uint8_t spi_pw_get_frame_size(const struct spi_config *config)
{
	uint8_t dfs = SPI_WORD_SIZE_GET(config->operation);

	dfs /= PW_SPI_WIDTH_8BITS;

	if ((dfs == 0) || (dfs > PW_SPI_FRAME_SIZE_4_BYTES)) {
		LOG_WRN("Unsupported dfs, 1-byte size will be used");
		dfs = PW_SPI_FRAME_SIZE_1_BYTE;
	}

	return dfs;
}

void spi_pw_cs_ctrl_enable(const struct device *dev, bool enable)
{
	struct spi_pw_data *spi = dev->data;

	if (enable == true) {
		if (spi->cs_mode == CS_SW_MODE) {
			spi_pw_cs_sw_ctrl(dev, true);
		} else if (spi->cs_mode == CS_GPIO_MODE) {
			spi_context_cs_control(&spi->ctx, true);
		}
	} else {
		if (spi->cs_mode == CS_SW_MODE) {
			spi_pw_cs_sw_ctrl(dev, false);
		} else if (spi->cs_mode == CS_GPIO_MODE) {
			spi_context_cs_control(&spi->ctx, false);
		}
	}
}

static void spi_pw_cs_ctrl_init(const struct device *dev)
{
	uint32_t cs_ctrl;
	struct spi_pw_data *spi = dev->data;

	/* Enable chip select output CS0/CS1 */
	cs_ctrl = spi_pw_reg_read(dev, PW_SPI_REG_CS_CTRL);

	if (spi->cs_output == PW_SPI_CS1_OUTPUT_SELECT) {
		cs_ctrl &= ~(PW_SPI_CS_CTRL_CS_MASK << PW_SPI_CS_EN_SHIFT);
		/* Set chip select CS1 */
		cs_ctrl |= PW_SPI_CS1_SELECT;
	} else {
		/* Set chip select CS0 */
		cs_ctrl &= ~(PW_SPI_CS_CTRL_CS_MASK << PW_SPI_CS_EN_SHIFT);
	}

	spi_pw_reg_write(dev, PW_SPI_REG_CS_CTRL, cs_ctrl);

	if (spi->cs_mode == CS_HW_MODE) {
		spi_pw_enable_cs_hw_ctrl(dev);
	} else if (spi->cs_mode == CS_SW_MODE) {
		spi_pw_cs_sw_ctrl(dev, false);
	} else if (spi->cs_mode == CS_GPIO_MODE) {
		spi_pw_cs_sw_ctrl(dev, false);
	}
}

static void spi_pw_tx_thld_set(const struct device *dev)
{
	uint32_t reg_data;

	/* Tx threshold */
	reg_data = spi_pw_reg_read(dev, PW_SPI_REG_SITF);
	/* mask high water mark bits in tx fifo reg */
	reg_data &= ~(PW_SPI_WM_MASK);
	/* mask low water mark bits in tx fifo reg */
	reg_data &= ~(PW_SPI_WM_MASK << PW_SPI_SITF_LWMTF_SHIFT);
	reg_data |= (PW_SPI_SITF_HIGH_WM_DFLT | PW_SPI_SITF_LOW_WM_DFLT);
	spi_pw_reg_write(dev, PW_SPI_REG_SITF, reg_data);
}

static void spi_pw_rx_thld_set(const struct device *dev,
			       struct spi_pw_data *spi)
{
	uint32_t reg_data;

	/* Rx threshold */
	reg_data = spi_pw_reg_read(dev, PW_SPI_REG_SIRF);
	reg_data &= (uint32_t) ~(PW_SPI_WM_MASK);
	reg_data |= PW_SPI_SIRF_WM_DFLT;
	if (spi->ctx.rx_len && spi->ctx.rx_len < spi->fifo_depth) {
		reg_data = spi->ctx.rx_len - 1;
	}
	spi_pw_reg_write(dev, PW_SPI_REG_SIRF, reg_data);
}

static int spi_pw_set_data_size(const struct device *dev,
				const struct spi_config *config)
{
	uint32_t ctrlr0;

	ctrlr0 = spi_pw_reg_read(dev, PW_SPI_REG_CTRLR0);

	/* Full duplex mode */
	ctrlr0 &= ~(PW_SPI_CTRLR0_MOD_BIT);

	ctrlr0 &= PW_SPI_CTRLR0_DATA_MASK;
	ctrlr0 &= PW_SPI_CTRLR0_EDSS_MASK;

	/* Set the word size */
	if (SPI_WORD_SIZE_GET(config->operation) == 4) {
		ctrlr0 |= PW_SPI_DATA_SIZE_4_BIT;
	} else if (SPI_WORD_SIZE_GET(config->operation) == 8) {
		ctrlr0 |= PW_SPI_DATA_SIZE_8_BIT;
	} else if (SPI_WORD_SIZE_GET(config->operation) == 16) {
		ctrlr0 |= PW_SPI_DATA_SIZE_16_BIT;
	} else if (SPI_WORD_SIZE_GET(config->operation) == 32) {
		ctrlr0 |= PW_SPI_DATA_SIZE_32_BIT;
	} else {
		LOG_ERR("Invalid word size");
		return -ENOTSUP;
	}

	spi_pw_reg_write(dev, PW_SPI_REG_CTRLR0, ctrlr0);

	return 0;
}

static void spi_pw_config_phase_polarity(const struct device *dev,
					 const struct spi_config *config)
{
	uint8_t mode;
	uint32_t ctrlr1;

	ctrlr1 = spi_pw_reg_read(dev, PW_SPI_REG_CTRLR1);

	mode = (SPI_MODE_GET(config->operation) & SPI_MODE_CPOL) |
	       (SPI_MODE_GET(config->operation) & SPI_MODE_CPHA);

	LOG_DBG("mode: 0x%x", (mode >> 1));
	switch (mode >> 1) {
	case SPI_PW_MODE0:
		ctrlr1 &= ~(PW_SPI_CTRL1_SPO_SPH_MASK);
		ctrlr1 &= ~(PW_SPI_CTRL1_SPO_BIT);
		ctrlr1 &= ~(PW_SPI_CTRL1_SPH_BIT);
		break;
	case SPI_PW_MODE1:
		ctrlr1 &= ~(PW_SPI_CTRL1_SPO_SPH_MASK);
		ctrlr1 |= PW_SPI_CTRL1_SPO_BIT;
		ctrlr1 &= ~(PW_SPI_CTRL1_SPH_BIT);
		break;
	case SPI_PW_MODE2:
		ctrlr1 &= ~(PW_SPI_CTRL1_SPO_SPH_MASK);
		ctrlr1 &= ~(PW_SPI_CTRL1_SPO_BIT);
		ctrlr1 |= PW_SPI_CTRL1_SPH_BIT;
		break;
	case SPI_PW_MODE3:
		ctrlr1 |= PW_SPI_CTRL1_SPO_BIT;
		ctrlr1 |= PW_SPI_CTRL1_SPH_BIT;
		break;
	}

	/* Set Polarity & Phase  */
	spi_pw_reg_write(dev, PW_SPI_REG_CTRLR1, ctrlr1);
}

static void spi_pw_enable_clk(const struct device *dev)
{
	uint32_t clks;

	/*Update M:N value & enable clock */
	clks = spi_pw_reg_read(dev, PW_SPI_REG_CLKS);
	clks &= ~(PW_SPI_CLKS_MVAL_MASK);
	clks &= ~(PW_SPI_CLKS_NVAL_MASK);
	clks |= (PW_SPI_CLKS_MVAL | PW_SPI_CLKS_NVAL |
		 PW_SPI_CLKS_EN_BIT | PW_SPI_CLKS_UPDATE_BIT);
	spi_pw_reg_write(dev, PW_SPI_REG_CLKS, clks);
}

static void spi_pw_config_clk(const struct device *dev,
			      const struct spi_pw_config *info,
			      const struct spi_config *config)
{
	uint32_t ctrlr0, scr;

	/* Update scr control bits */
	if (!config->frequency) {
		scr = PW_SPI_BR_2MHZ;
	} else if (config->frequency > PW_SPI_BR_MAX_FRQ) {
		scr = (info->clock_freq / PW_SPI_BR_MAX_FRQ) - 1;
	} else {
		scr = (info->clock_freq / config->frequency) - 1;
	}
	ctrlr0 = spi_pw_reg_read(dev, PW_SPI_REG_CTRLR0);

	ctrlr0 &= ~(PW_SPI_SCR_MASK);
	ctrlr0 |= (scr << PW_SPI_SCR_SHIFT);
	spi_pw_reg_write(dev, PW_SPI_REG_CTRLR0, ctrlr0);
}

static void spi_pw_completed(const struct device *dev, int err)
{
	struct spi_pw_data *spi = dev->data;

	if (!err && (spi_context_tx_on(&spi->ctx) ||
		     spi_context_rx_on(&spi->ctx))) {
		return;
	}

	/* need to give time for FIFOs to drain before issuing more commands */
	while (is_pw_ssp_busy(dev)) {
	}

#ifdef CONFIG_SPI_PW_INTERRUPT
	/* Disabling interrupts */
	spi_pw_intr_disable(dev);
#endif

	/* Disabling the controller operation, which also clear's all status bits
	 * in status register
	 */
	spi_pw_ssp_disable(dev);

	spi_pw_cs_ctrl_enable(dev, false);

	LOG_DBG("SPI transaction completed %s error\n",
		err ? "with" : "without");

	spi_context_complete(&spi->ctx, dev, err);
}

static void spi_pw_clear_intr(const struct device *dev)
{
	uint32_t sssr;

	sssr = spi_pw_reg_read(dev, PW_SPI_REG_SSSR);
	sssr &= ~(PW_SPI_INTR_ERRORS_MASK);
	spi_pw_reg_write(dev, PW_SPI_REG_SSSR, sssr);
}

static int spi_pw_get_tx_fifo_level(const struct device *dev)
{
	uint32_t tx_fifo_level;

	tx_fifo_level = spi_pw_reg_read(dev, PW_SPI_REG_SITF);

	tx_fifo_level = ((tx_fifo_level & PW_SPI_SITF_SITFL_MASK) >>
			 PW_SPI_SITF_SITFL_SHIFT);

	return tx_fifo_level;
}

static int spi_pw_get_rx_fifo_level(const struct device *dev)
{
	uint32_t rx_fifo_level;

	rx_fifo_level = spi_pw_reg_read(dev, PW_SPI_REG_SIRF);
	rx_fifo_level = ((rx_fifo_level & PW_SPI_SIRF_SIRFL_MASK) >>
			 PW_SPI_SIRF_SIRFL_SHIFT);

	return rx_fifo_level;
}

static void spi_pw_reset_tx_fifo_level(const struct device *dev)
{
	uint32_t tx_fifo_level;

	tx_fifo_level = spi_pw_reg_read(dev, PW_SPI_REG_SITF);
	tx_fifo_level &= ~(PW_SPI_SITF_SITFL_MASK);
	spi_pw_reg_write(dev, PW_SPI_REG_SITF, tx_fifo_level);

}

static void spi_pw_update_rx_fifo_level(uint32_t len,
					const struct device *dev)
{
	uint32_t rx_fifo_level;

	rx_fifo_level = spi_pw_reg_read(dev, PW_SPI_REG_SIRF);
	rx_fifo_level &= ~(PW_SPI_SIRF_SIRFL_MASK);
	rx_fifo_level |= (len << PW_SPI_SIRF_SIRFL_SHIFT);
	spi_pw_reg_write(dev, PW_SPI_REG_SIRF, rx_fifo_level);
}

static void spi_pw_tx_data(const struct device *dev)
{
	struct spi_pw_data *spi = dev->data;
	uint32_t data = 0U;
	int32_t fifo_len;

	if (spi_context_rx_on(&spi->ctx)) {
		fifo_len = spi->fifo_depth -
			   spi_pw_get_tx_fifo_level(dev) -
			   spi_pw_get_rx_fifo_level(dev);
		if (fifo_len < 0) {
			fifo_len = 0U;
		}
	} else {
		fifo_len = spi->fifo_depth - spi_pw_get_tx_fifo_level(dev);
	}

	while (fifo_len > 0) {
		if (spi_context_tx_buf_on(&spi->ctx)) {
			switch (spi->dfs) {
			case 1:
				data = UNALIGNED_GET((uint8_t *)
						     (spi->ctx.tx_buf));
				break;
			case 2:
				data = UNALIGNED_GET((uint16_t *)
						     (spi->ctx.tx_buf));
				break;
			case 4:
				data = UNALIGNED_GET((uint32_t *)
						     (spi->ctx.tx_buf));
				break;
			}
		} else if (spi_context_rx_on(&spi->ctx)) {
			if ((int)(spi->ctx.rx_len - spi->fifo_diff) <= 0) {
				break;
			}

			data = 0U;
		} else if (spi_context_tx_on(&spi->ctx)) {
			data = 0U;
		} else {
			break;
		}

		spi_pw_reg_write(dev, PW_SPI_REG_SSDR, data);

		spi_context_update_tx(&spi->ctx, spi->dfs, 1);
		spi->fifo_diff++;
		fifo_len--;
	}

	if (!spi_context_tx_on(&spi->ctx)) {
		spi_pw_reset_tx_fifo_level(dev);
	}
}

static void spi_pw_rx_data(const struct device *dev)
{
	struct spi_pw_data *spi = dev->data;

	while (spi_pw_get_rx_fifo_level(dev)) {
		uint32_t data = spi_pw_reg_read(dev, PW_SPI_REG_SSDR);

		if (spi_context_rx_buf_on(&spi->ctx)) {
			switch (spi->dfs) {
			case 1:
				UNALIGNED_PUT(data,
					      (uint8_t *)spi->ctx.rx_buf);
				break;
			case 2:
				UNALIGNED_PUT(data,
					      (uint16_t *)spi->ctx.rx_buf);
				break;
			case 4:
				UNALIGNED_PUT(data,
					      (uint32_t *)spi->ctx.rx_buf);
				break;
			}
		}

		spi_context_update_rx(&spi->ctx, spi->dfs, 1);
		spi->fifo_diff--;
	}

	if (!spi->ctx.rx_len && spi->ctx.tx_len < spi->fifo_depth) {
		spi_pw_update_rx_fifo_level(spi->ctx.tx_len - 1, dev);
	} else if (spi_pw_get_rx_fifo_level(dev) >= spi->ctx.rx_len) {
		spi_pw_update_rx_fifo_level(spi->ctx.rx_len - 1, dev);
	}
}

static int spi_pw_transfer(const struct device *dev)
{
	uint32_t intr_status;
	int err;

	intr_status = spi_pw_reg_read(dev, PW_SPI_REG_SSSR);

	if (intr_status & PW_SPI_SSSR_ROR_BIT) {
		LOG_ERR("Receive FIFO overrun");
		err = -EIO;
		goto out;
	}

	if (intr_status & PW_SPI_SSSR_TUR_BIT) {
		LOG_ERR("Transmit FIFO underrun");
		err = -EIO;
		goto out;
	}

	if (intr_status & PW_SPI_SSSR_TINT_BIT) {
		LOG_ERR("Receiver timeout interrupt");
		err = -EIO;
		goto out;
	}

	err = 0;

	if (intr_status & PW_SPI_SSSR_RNE_BIT) {
		spi_pw_rx_data(dev);
	}

	if (intr_status & PW_SPI_SSSR_TNF_BIT) {
		spi_pw_tx_data(dev);
	}

out:
	if (err) {
		spi_pw_clear_intr(dev);
	}

	return err;
}

static int spi_pw_configure(const struct device *dev,
			    const struct spi_pw_config *info,
			    struct spi_pw_data *spi,
			    const struct spi_config *config)
{
	int err;

	/* At this point, it's mandatory to set this on the context! */
	spi->ctx.config = config;

	if (!spi_cs_is_gpio(spi->ctx.config)) {
		if (spi->cs_mode == CS_GPIO_MODE) {
			LOG_DBG("cs gpio is NULL, switch to hw mode");
			spi->cs_mode = CS_HW_MODE;
			spi_pw_enable_cs_hw_ctrl(dev);
		}
	}

	if (config->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	/* Verify if requested op mode is relevant to this controller */
	if (config->operation & SPI_OP_MODE_SLAVE) {
		LOG_ERR("Slave mode not supported");
		return -ENOTSUP;
	}

	if ((config->operation & SPI_TRANSFER_LSB) ||
	    (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES) &&
	     (config->operation & (SPI_LINES_DUAL |
				   SPI_LINES_QUAD |
				   SPI_LINES_OCTAL)))) {
		LOG_ERR("Extended mode Unsupported configuration");
		return -EINVAL;
	}

	if (config->operation & SPI_FRAME_FORMAT_TI) {
		LOG_ERR("TI frame format not supported");
		return -ENOTSUP;
	}

	if (config->operation & SPI_HOLD_ON_CS) {
		LOG_ERR("Chip select hold not supported");
		return -ENOTSUP;
	}

	/* Set mode & data size */
	err = spi_pw_set_data_size(dev, config);

	if (err) {
		LOG_ERR("Invalid data size");
		return -ENOTSUP;
	}

	/* Set Polarity & Phase  */
	spi_pw_config_phase_polarity(dev, config);

	/* enable clock */
	spi_pw_enable_clk(dev);

	/* configure */
	spi_pw_config_clk(dev, info, config);

	return 0;
}

static int transceive(const struct device *dev,
		      const struct spi_config *config,
		      const struct spi_buf_set *tx_bufs,
		      const struct spi_buf_set *rx_bufs,
		      bool asynchronous,
		      spi_callback_t cb,
		      void *userdata)
{
	const struct spi_pw_config *info = dev->config;
	struct spi_pw_data *spi = dev->data;
	int err;

	if (!tx_bufs && !rx_bufs) {
		LOG_ERR(" Tx & Rx buff null");
		return 0;
	}

	if (asynchronous) {
		LOG_ERR("Async not supported");
		return -ENOTSUP;
	}

	spi_context_lock(&spi->ctx, asynchronous, cb, userdata, config);

	/* Configure */
	err = spi_pw_configure(dev, info, spi, config);
	if (err) {
		LOG_ERR("spi pw config fail");
		goto out;
	}

	/* Frame size in number of data bytes */
	spi->dfs = spi_pw_get_frame_size(config);
	spi_context_buffers_setup(&spi->ctx, tx_bufs, rx_bufs,
				  spi->dfs);

	spi->fifo_diff = 0U;

	/* Tx threshold */
	spi_pw_tx_thld_set(dev);

	/* Rx threshold */
	spi_pw_rx_thld_set(dev, spi);

	spi_pw_cs_ctrl_enable(dev, true);

	/* Enable ssp operation */
	spi_pw_ssp_enable(dev);

#ifdef CONFIG_SPI_PW_INTERRUPT
	LOG_DBG("Interrupt Mode");

	/* Enable interrupts */
	if (rx_bufs) {
		spi_pw_intr_enable(dev, true);
	} else {
		spi_pw_intr_enable(dev, false);
	}

	err = spi_context_wait_for_completion(&spi->ctx);
#else
	LOG_DBG("Polling Mode");

	do {
		err = spi_pw_transfer(dev);
	} while ((!err) && is_spi_transfer_ongoing(spi));

	spi_pw_completed(dev, err);
#endif

out:
	spi_context_release(&spi->ctx, err);
	return err;
}

static int spi_pw_transceive(const struct device *dev,
			     const struct spi_config *config,
			     const struct spi_buf_set *tx_bufs,
			     const struct spi_buf_set *rx_bufs)
{
	LOG_DBG("%p, %p, %p\n", dev, tx_bufs, rx_bufs);
	return transceive(dev, config, tx_bufs, rx_bufs,
			  false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_pw_transceive_async(const struct device *dev,
				   const struct spi_config *config,
				   const struct spi_buf_set *tx_bufs,
				   const struct spi_buf_set *rx_bufs,
				   spi_callback_t cb,
				   void *userdata)
{
	LOG_DBG("%p, %p, %p, %p, %p\n", dev, tx_bufs, rx_bufs,
					cb, userdata);

	return transceive(dev, config, tx_bufs, rx_bufs, true,
			  cb, userdata);
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_pw_release(const struct device *dev,
			  const struct spi_config *config)
{
	struct spi_pw_data *spi = dev->data;

	if (!spi_context_configured(&spi->ctx, config)) {
		return -EINVAL;
	}

	spi_context_unlock_unconditionally(&spi->ctx);

	return 0;
}

#ifdef CONFIG_SPI_PW_INTERRUPT
static void spi_pw_isr(const void *arg)
{
	const struct device *dev = (const struct device *)arg;
	int err;

	err = spi_pw_transfer(dev);
	spi_pw_completed(dev, err);
}
#endif

static const struct spi_driver_api pw_spi_api = {
	.transceive = spi_pw_transceive,
	.release = spi_pw_release,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_pw_transceive_async,
#endif  /* CONFIG_SPI_ASYNC */
};

static int spi_pw_init(const struct device *dev)
{
	const struct spi_pw_config *info = dev->config;
	struct spi_pw_data *spi = dev->data;
	int err;

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(pcie)
	if (info->pcie) {
		struct pcie_bar mbar;

		if (info->pcie->bdf == PCIE_BDF_NONE) {
			LOG_ERR("Cannot probe PCI device");
			return -ENODEV;
		}

		if (!pcie_probe_mbar(info->pcie->bdf, 0, &mbar)) {
			LOG_ERR("MBAR not found");
			return -EINVAL;
		}

		pcie_set_cmd(info->pcie->bdf, PCIE_CONF_CMDSTAT_MEM,
			     true);

		device_map(DEVICE_MMIO_RAM_PTR(dev), mbar.phys_addr,
			   mbar.size, K_MEM_CACHE_NONE);

		pcie_set_cmd(info->pcie->bdf,
			     PCIE_CONF_CMDSTAT_MASTER,
			     true);

	} else {
		DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	}
#else
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
#endif

	/* Bring ssp out of reset */
	spi_pw_ssp_reset(dev);

	/* Disable ssp operation */
	spi_pw_ssp_disable(dev);

	/* Chip select control */
	spi_pw_cs_ctrl_init(dev);

#if defined(CONFIG_SPI_PW_INTERRUPT)
	/* Mask interrupts */
	spi_pw_intr_disable(dev);

	/* Init and connect IRQ */
	info->irq_config(dev);
#endif

	if (spi->cs_mode == CS_GPIO_MODE) {
		err = spi_context_cs_configure_all(&spi->ctx);
		if (err < 0) {
			LOG_ERR("Failed to configure CS pins: %d", err);
			return err;
		}
	}

	spi_context_unlock_unconditionally(&spi->ctx);

	LOG_DBG("SPI pw init success");

	return 0;
}

#define INIT_PCIE0(n)
#define INIT_PCIE1(n) DEVICE_PCIE_INST_INIT(n, pcie),
#define INIT_PCIE(n) _CONCAT(INIT_PCIE, DT_INST_ON_BUS(n, pcie))(n)

#define DEFINE_PCIE0(n)
#define DEFINE_PCIE1(n) DEVICE_PCIE_INST_DECLARE(n)
#define SPI_PCIE_DEFINE(n) _CONCAT(DEFINE_PCIE, DT_INST_ON_BUS(n, pcie))(n)

#ifdef CONFIG_SPI_PW_INTERRUPT

#define SPI_INTEL_IRQ_FLAGS_SENSE0(n) 0
#define SPI_INTEL_IRQ_FLAGS_SENSE1(n) DT_INST_IRQ(n, sense)
#define SPI_INTEL_IRQ_FLAGS(n) \
	_CONCAT(SPI_INTEL_IRQ_FLAGS_SENSE, DT_INST_IRQ_HAS_CELL(n, sense))(n)

#define SPI_INTEL_IRQ_INIT(n)						     \
	BUILD_ASSERT(IS_ENABLED(CONFIG_DYNAMIC_INTERRUPTS),		     \
		     "SPI PCIe requires dynamic interrupts");		     \
	static void spi_##n##_irq_init(const struct device *dev)	     \
	{								     \
		const struct spi_pw_config *info = dev->config;		     \
		unsigned int irq;					     \
		if (DT_INST_IRQN(n) == PCIE_IRQ_DETECT) {		     \
			irq = pcie_alloc_irq(info->pcie->bdf);		     \
			if (irq == PCIE_CONF_INTR_IRQ_NONE) {		     \
				return;					     \
			}						     \
		} else {						     \
			irq = DT_INST_IRQN(n);				     \
			pcie_conf_write(info->pcie->bdf,		     \
					PCIE_CONF_INTR, irq);		     \
		}							     \
		pcie_connect_dynamic_irq(info->pcie->bdf, irq,		     \
					 DT_INST_IRQ(n, priority),	     \
					 (void (*)(const void *))spi_pw_isr, \
					 DEVICE_DT_INST_GET(n),		     \
					 SPI_INTEL_IRQ_FLAGS(n));	     \
		pcie_irq_enable(info->pcie->bdf, irq);			     \
		LOG_DBG("lpass spi Configure irq %d", irq);		     \
	}

#define SPI_PW_DEV_INIT(n)					     \
	static struct spi_pw_data spi_##n##_data = {		     \
		SPI_CONTEXT_INIT_LOCK(spi_##n##_data, ctx),	     \
		SPI_CONTEXT_INIT_SYNC(spi_##n##_data, ctx),	     \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx) \
		.cs_mode = DT_INST_PROP(n, pw_cs_mode),		     \
		.cs_output = DT_INST_PROP(n, pw_cs_output),	     \
		.fifo_depth = DT_INST_PROP(n, pw_fifo_depth),	     \
	};							     \
	SPI_PCIE_DEFINE(n);					     \
	SPI_INTEL_IRQ_INIT(n)					     \
	static const struct spi_pw_config spi_##n##_config = {	     \
		.irq_config = spi_##n##_irq_init,		     \
		.clock_freq = DT_INST_PROP(n, clock_frequency),	     \
		INIT_PCIE(n)					     \
	};							     \
	DEVICE_DT_INST_DEFINE(n, spi_pw_init, NULL,		     \
			      &spi_##n##_data, &spi_##n##_config,    \
			      POST_KERNEL, CONFIG_SPI_INIT_PRIORITY, \
			      &pw_spi_api);
#else

#define SPI_PW_DEV_INIT(n)					     \
	static struct spi_pw_data spi_##n##_data = {		     \
		SPI_CONTEXT_INIT_LOCK(spi_##n##_data, ctx),	     \
		SPI_CONTEXT_INIT_SYNC(spi_##n##_data, ctx),	     \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx) \
		.cs_mode = DT_INST_PROP(n, pw_cs_mode),		     \
		.cs_output = DT_INST_PROP(n, pw_cs_output),	     \
		.fifo_depth = DT_INST_PROP(n, pw_fifo_depth),	     \
	};							     \
	SPI_PCIE_DEFINE(n);					     \
	static const struct spi_pw_config spi_##n##_config = {	     \
		.clock_freq = DT_INST_PROP(n, clock_frequency),	     \
		INIT_PCIE(n)					     \
	};							     \
	DEVICE_DT_INST_DEFINE(n, spi_pw_init, NULL,		     \
			      &spi_##n##_data, &spi_##n##_config,    \
			      POST_KERNEL, CONFIG_SPI_INIT_PRIORITY, \
			      &pw_spi_api);

#endif

DT_INST_FOREACH_STATUS_OKAY(SPI_PW_DEV_INIT)
