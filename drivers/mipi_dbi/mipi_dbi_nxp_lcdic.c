/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lcdic

#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/logging/log.h>
#include <soc.h>
#include <zephyr/drivers/dma/dma_mcux_lpc.h>
LOG_MODULE_REGISTER(mipi_dbi_lcdic, CONFIG_MIPI_DBI_LOG_LEVEL);

#include <fsl_inputmux.h>

enum lcdic_data_fmt {
	LCDIC_DATA_FMT_BYTE = 0,
	LCDIC_DATA_FMT_HALFWORD = 1, /* 2 byte */
	LCDIC_DATA_FMT_WORD = 2, /* 4 byte */
};

enum lcdic_cmd_dc {
	LCDIC_COMMAND = 0,
	LCDIC_DATA = 1,
};

enum lcdic_cmd_type {
	LCDIC_RX = 0,
	LCDIC_TX = 1,
};

/* Limit imposed by size of data length field in LCDIC command */
#define LCDIC_MAX_XFER 0x40000
/* Max reset width (in terms of Timer0_Period, see RST_CTRL register) */
#define LCDIC_MAX_RST_WIDTH 0x3F

/* Descriptor for LCDIC command */
union lcdic_trx_cmd {
	struct {
		/* Data length in bytes. LCDIC transfers data_len + 1 */
		uint32_t data_len: 18;
		/* Dummy SCLK cycles between TX and RX (for SPI mode) */
		uint32_t dummy_count: 3;
		uint32_t rsvd: 2;
		/* Use auto repeat mode */
		uint32_t auto_repeat: 1;
		/* Tearing enable sync mode */
		uint32_t te_sync_mode: 2;
		/* TRX command timeout mode */
		uint32_t trx_timeout_mode: 1;
		/* Data format, see lcdic_data_fmt */
		uint32_t data_format: 2;
		/* Enable command done interrupt */
		uint32_t cmd_done_int: 1;
		/* LCD command or LCD data, see lcdic_cmd_dc */
		uint32_t cmd_data: 1;
		/* TX or RX command, see lcdic_cmd_type */
		uint32_t trx: 1;
	} bits;
	uint32_t u32;
};

struct mipi_dbi_lcdic_config {
	LCDIC_Type *base;
	void (*irq_config_func)(const struct device *dev);
	const struct pinctrl_dev_config *pincfg;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	bool swap_bytes;
};

#ifdef CONFIG_MIPI_DBI_NXP_LCDIC_DMA
struct stream {
	const struct device *dma_dev;
	uint32_t channel;
	struct dma_config dma_cfg;
	struct dma_block_config blk_cfg[2];
};
#endif

struct mipi_dbi_lcdic_data {
	/* Tracks number of bytes remaining in command */
	uint32_t cmd_bytes;
	/* Tracks number of bytes remaining in transfer */
	uint32_t xfer_bytes;
	/* Tracks start of transfer buffer */
	const uint8_t *xfer_buf;
	/* When sending data that does not evenly fit into 4 byte chunks,
	 * this is used to store the last unaligned segment of the data.
	 */
	uint32_t unaligned_word __aligned(4);
	/* Tracks lcdic_data_fmt value we should use for pixel data */
	uint8_t pixel_fmt;
	const struct mipi_dbi_config *active_cfg;
	struct k_sem xfer_sem;
	struct k_sem lock;
#ifdef CONFIG_MIPI_DBI_NXP_LCDIC_DMA
	struct stream dma_stream;
#endif
};

#define LCDIC_ALL_INTERRUPTS \
	(LCDIC_ICR_RFIFO_THRES_INTR_CLR_MASK | \
	LCDIC_ICR_RFIFO_UNDERFLOW_INTR_CLR_MASK | \
	LCDIC_ICR_TFIFO_THRES_INTR_CLR_MASK | \
	LCDIC_ICR_TFIFO_OVERFLOW_INTR_CLR_MASK | \
	LCDIC_ICR_TE_TO_INTR_CLR_MASK | \
	LCDIC_ICR_CMD_TO_INTR_CLR_MASK | \
	LCDIC_ICR_CMD_DONE_INTR_CLR_MASK | \
	LCDIC_ICR_RST_DONE_INTR_CLR_MASK)

/* RX and TX FIFO thresholds */
#ifdef CONFIG_MIPI_DBI_NXP_LCDIC_DMA
#define LCDIC_RX_FIFO_THRESH 0x0
#define LCDIC_TX_FIFO_THRESH 0x0
#else
#define LCDIC_RX_FIFO_THRESH 0x0
#define LCDIC_TX_FIFO_THRESH 0x3
#endif

/* Timer0 and Timer1 bases. We choose a longer timer0 base to enable
 * long reset periods
 */
#define LCDIC_TIMER0_RATIO 0xF
#define LCDIC_TIMER1_RATIO 0x9

/* After LCDIC is enabled or disabled, there should be a wait longer than
 * 5x the module clock before other registers are read
 */
static inline void mipi_dbi_lcdic_reset_delay(void)
{
	k_busy_wait(1);
}

/* Resets state of the LCDIC TX/RX FIFO */
static inline void mipi_dbi_lcdic_reset_state(const struct device *dev)
{
	const struct mipi_dbi_lcdic_config *config = dev->config;
	LCDIC_Type *base = config->base;

	base->CTRL &= ~LCDIC_CTRL_LCDIC_EN_MASK;
	mipi_dbi_lcdic_reset_delay();
	base->CTRL |= LCDIC_CTRL_LCDIC_EN_MASK;
	mipi_dbi_lcdic_reset_delay();
}


#ifdef CONFIG_MIPI_DBI_NXP_LCDIC_DMA

/* Start DMA to send data using LCDIC TX FIFO */
static int mipi_dbi_lcdic_start_dma(const struct device *dev)
{
	const struct mipi_dbi_lcdic_config *config = dev->config;
	struct mipi_dbi_lcdic_data *data = dev->data;
	struct stream *stream = &data->dma_stream;
	uint32_t aligned_len = data->cmd_bytes & (~0x3);
	uint32_t unaligned_len = data->cmd_bytes & 0x3;
	int ret;

	stream->dma_cfg.head_block = &stream->blk_cfg[0];
	if (aligned_len == 0) {
		/* Only unaligned data exists, send it in the first block */
		/* First DMA block configuration is used to send aligned data */
		stream->blk_cfg[0].source_address = (uint32_t)&data->unaligned_word;
		stream->blk_cfg[0].dest_address = (uint32_t)&config->base->TFIFO_WDATA;
		/* Block size should be the aligned portion of the transfer */
		stream->blk_cfg[0].block_size = sizeof(uint32_t);
		stream->dma_cfg.block_count = 1;
		stream->blk_cfg[0].next_block = NULL;
	} else {
		/* First DMA block configuration is used to send aligned data */
		stream->blk_cfg[0].source_address = (uint32_t)data->xfer_buf;
		stream->blk_cfg[0].dest_address = (uint32_t)&config->base->TFIFO_WDATA;
		/* Block size should be the aligned portion of the transfer */
		stream->blk_cfg[0].block_size = aligned_len;
		/* Second DMA block configuration sends unaligned block */
		if (unaligned_len) {
			stream->dma_cfg.block_count = 2;
			stream->blk_cfg[0].next_block =
						&stream->blk_cfg[1];
			stream->blk_cfg[1].source_address =
				(uint32_t)&data->unaligned_word;
			stream->blk_cfg[1].dest_address =
				(uint32_t)&config->base->TFIFO_WDATA;
			stream->blk_cfg[1].block_size = sizeof(uint32_t);
		} else {
			stream->dma_cfg.block_count = 1;
			stream->blk_cfg[0].next_block = NULL;
		}
	}

	ret = dma_config(stream->dma_dev, stream->channel, &stream->dma_cfg);
	if (ret) {
		return ret;
	}
	/* Enable DMA channel before we set up DMA request. This way,
	 * the hardware DMA trigger does not fire until the DMA
	 * start function has initialized the DMA.
	 */
	ret = dma_start(stream->dma_dev, stream->channel);
	if (ret) {
		return ret;
	}
	/* Enable DMA request */
	config->base->CTRL |= LCDIC_CTRL_DMA_EN_MASK;
	return ret;
}

/* DMA completion callback */
static void mipi_dbi_lcdic_dma_callback(const struct device *dma_dev,
	     void *user_data, uint32_t channel, int status)
{
	if (status < 0) {
		LOG_ERR("DMA callback with error %d", status);
	}
}

#endif /* CONFIG_MIPI_DBI_NXP_LCDIC_DMA */

/* Configure LCDIC */
static int mipi_dbi_lcdic_configure(const struct device *dev,
				    const struct mipi_dbi_config  *dbi_config)
{
	const struct mipi_dbi_lcdic_config *config = dev->config;
	struct mipi_dbi_lcdic_data *data = dev->data;
	const struct spi_config *spi_cfg = &dbi_config->config;
	LCDIC_Type *base = config->base;
	int ret;
	uint32_t reg;

	if (dbi_config == data->active_cfg) {
		return 0;
	}

	/* Clear all interrupt flags */
	base->ICR = LCDIC_ALL_INTERRUPTS;
	/* Mask all interrupts */
	base->IMR = LCDIC_ALL_INTERRUPTS;

	/* Set LCDIC clock frequency */
	ret = clock_control_set_rate(config->clock_dev, config->clock_subsys,
			(clock_control_subsys_rate_t)spi_cfg->frequency);
	if (ret) {
		LOG_ERR("Invalid clock frequency %d", spi_cfg->frequency);
		return ret;
	}
	if (!(spi_cfg->operation & SPI_HALF_DUPLEX)) {
		LOG_ERR("LCDIC only supports half duplex operation");
		return -ENOTSUP;
	}
	if (spi_cfg->slave != 0) {
		/* Only one slave select line */
		return -ENOTSUP;
	}
	if (SPI_WORD_SIZE_GET(spi_cfg->operation) > 8) {
		LOG_ERR("Unsupported word size");
		return -ENOTSUP;
	}

	reg = base->CTRL;
	/* Disable LCD module during configuration */
	reg &= ~LCDIC_CTRL_LCDIC_EN_MASK;
	/* Select SPI mode */
	reg &= ~LCDIC_CTRL_LCDIC_MD_MASK;
	/* Select 3 or 4 wire mode based on config selection */
	if (dbi_config->mode == MIPI_DBI_MODE_SPI_4WIRE) {
		reg |= LCDIC_CTRL_SPI_MD_MASK;
	} else {
		reg &= ~LCDIC_CTRL_SPI_MD_MASK;
	}
	/* Enable byte swapping if user requested it */
	reg = (reg & ~LCDIC_CTRL_DAT_ENDIAN_MASK) |
		LCDIC_CTRL_DAT_ENDIAN(!config->swap_bytes);
	/* Disable DMA */
	reg &= ~LCDIC_CTRL_DMA_EN_MASK;
	base->CTRL = reg;
	mipi_dbi_lcdic_reset_delay();

	/* Setup SPI CPOL and CPHA selections */
	reg = base->SPI_CTRL;
	reg = (reg & ~LCDIC_SPI_CTRL_SDAT_ENDIAN_MASK) |
		LCDIC_SPI_CTRL_SDAT_ENDIAN((spi_cfg->operation &
					    SPI_TRANSFER_LSB) ? 1 : 0);
	reg = (reg & ~LCDIC_SPI_CTRL_CPHA_MASK) |
		LCDIC_SPI_CTRL_CPHA((spi_cfg->operation & SPI_MODE_CPHA) ? 1 : 0);
	reg = (reg & ~LCDIC_SPI_CTRL_CPOL_MASK) |
		LCDIC_SPI_CTRL_CPOL((spi_cfg->operation & SPI_MODE_CPOL) ? 1 : 0);
	base->SPI_CTRL = reg;

	/* Enable the module */
	base->CTRL |= LCDIC_CTRL_LCDIC_EN_MASK;
	mipi_dbi_lcdic_reset_delay();

	data->active_cfg = dbi_config;

	return 0;
}

/* Gets unaligned word data from array. Return value will be a 4 byte
 * value containing the last unaligned section of the array data
 */
static uint32_t mipi_dbi_lcdic_get_unaligned(const uint8_t *bytes,
					     uint32_t buf_len)
{
	uint32_t word = 0U;
	uint8_t unaligned_len = buf_len & 0x3;
	uint32_t aligned_len = buf_len - unaligned_len;

	while ((unaligned_len--)) {
		word <<= 8U;
		word |= bytes[aligned_len + unaligned_len];
	}
	return word;
}

/* Fills the TX fifo with data. Returns number of bytes written. */
static int mipi_dbi_lcdic_fill_tx(LCDIC_Type *base, const uint8_t *buf,
				  uint32_t buf_len, uint32_t last_word)
{
	uint32_t *word_buf = (uint32_t *)buf;
	uint32_t bytes_written = 0U;
	uint32_t write_len;

	/* TX FIFO consumes 4 bytes on each write, so we can write up
	 * to buf_len / 4 times before we send all data.
	 * Write to FIFO it overflows or we send entire buffer.
	 */
	while (buf_len) {
		if (buf_len < 4) {
			/* Send last bytes */
			base->TFIFO_WDATA = last_word;
			write_len = buf_len;
		} else {
			/* Otherwise, write one word */
			base->TFIFO_WDATA = word_buf[bytes_written >> 2];
			write_len = 4;
		}
		if (base->IRSR & LCDIC_IRSR_TFIFO_OVERFLOW_RAW_INTR_MASK) {
			/* TX FIFO has overflowed, last word write did not
			 * complete. Return current number of bytes written.
			 */
			base->ICR |= LCDIC_ICR_TFIFO_OVERFLOW_INTR_CLR_MASK;
			return bytes_written;
		}
		bytes_written += write_len;
		buf_len -= write_len;
	}
	return bytes_written;
}

/* Writes command word */
static void mipi_dbi_lcdic_set_cmd(LCDIC_Type *base,
				   enum lcdic_cmd_type dir,
				   enum lcdic_cmd_dc dc,
				   enum lcdic_data_fmt data_fmt,
				   uint32_t buf_len)
{
	union lcdic_trx_cmd cmd = {0};


	/* TX FIFO will be clear, write command word */
	cmd.bits.data_len = buf_len - 1;
	cmd.bits.cmd_data = dc;
	cmd.bits.trx = dir;
	cmd.bits.cmd_done_int = true;
	cmd.bits.data_format = data_fmt;
	/* Write command */
	base->TFIFO_WDATA = cmd.u32;
}

static int mipi_dbi_lcdic_write_display(const struct device *dev,
					const struct mipi_dbi_config *dbi_config,
					const uint8_t *framebuf,
					struct display_buffer_descriptor *desc,
					enum display_pixel_format pixfmt)
{
	const struct mipi_dbi_lcdic_config *config = dev->config;
	struct mipi_dbi_lcdic_data *dev_data = dev->data;
	LCDIC_Type *base = config->base;
	int ret;
	uint32_t interrupts = 0U;

	ret = k_sem_take(&dev_data->lock, K_FOREVER);
	if (ret) {
		goto out;
	}

	ret = mipi_dbi_lcdic_configure(dev, dbi_config);
	if (ret) {
		goto out;
	}

	/* State reset is required before transfer */
	mipi_dbi_lcdic_reset_state(dev);

	if (desc->buf_size != 0) {
		dev_data->xfer_bytes = desc->buf_size;
		/* Cap command to max transfer size */
		dev_data->cmd_bytes = MIN(desc->buf_size, LCDIC_MAX_XFER);
		dev_data->xfer_buf = framebuf;
		/* If the length of the transfer is not divisible by
		 * 4, save the unaligned portion of the transfer into
		 * a temporary buffer
		 */
		if (dev_data->cmd_bytes & 0x3) {
			dev_data->unaligned_word = mipi_dbi_lcdic_get_unaligned(
							dev_data->xfer_buf,
							dev_data->cmd_bytes);
		}

		/* Save pixel format */
		if (DISPLAY_BITS_PER_PIXEL(pixfmt) == 32) {
			dev_data->pixel_fmt = LCDIC_DATA_FMT_WORD;
		} else if (DISPLAY_BITS_PER_PIXEL(pixfmt) == 16) {
			dev_data->pixel_fmt = LCDIC_DATA_FMT_HALFWORD;
		} else if (DISPLAY_BITS_PER_PIXEL(pixfmt) == 8) {
			dev_data->pixel_fmt = LCDIC_DATA_FMT_BYTE;
		} else {
			if (config->swap_bytes) {
				LOG_WRN("Unsupported pixel format, byte swapping disabled");
			}
		}
		/* Use pixel format data width, so we can byte swap
		 * if needed
		 */
		mipi_dbi_lcdic_set_cmd(base, LCDIC_TX, LCDIC_DATA,
				       dev_data->pixel_fmt,
				       dev_data->cmd_bytes);
#ifdef CONFIG_MIPI_DBI_NXP_LCDIC_DMA
		/* Enable command complete interrupt */
		interrupts |= LCDIC_IMR_CMD_DONE_INTR_MSK_MASK;
		/* Write interrupt mask */
		base->IMR &= ~interrupts;
		/* Configure DMA to send data */
		ret = mipi_dbi_lcdic_start_dma(dev);
		if (ret) {
			LOG_ERR("Could not start DMA (%d)", ret);
			goto out;
		}
#else
		/* Enable TX FIFO threshold interrupt. This interrupt
		 * should fire once enabled, which will kick off
		 * the transfer
		 */
		interrupts |= LCDIC_IMR_TFIFO_THRES_INTR_MSK_MASK;
		/* Enable command complete interrupt */
		interrupts |= LCDIC_IMR_CMD_DONE_INTR_MSK_MASK;
		/* Write interrupt mask */
		base->IMR &= ~interrupts;
#endif
		ret = k_sem_take(&dev_data->xfer_sem, K_FOREVER);
	}
out:
	k_sem_give(&dev_data->lock);
	return ret;

}

static int mipi_dbi_lcdic_write_cmd(const struct device *dev,
				    const struct mipi_dbi_config *dbi_config,
				    uint8_t cmd,
				    const uint8_t *data,
				    size_t data_len)
{
	const struct mipi_dbi_lcdic_config *config = dev->config;
	struct mipi_dbi_lcdic_data *dev_data = dev->data;
	LCDIC_Type *base = config->base;
	int ret;
	uint32_t interrupts = 0U;

	ret = k_sem_take(&dev_data->lock, K_FOREVER);
	if (ret) {
		goto out;
	}

	ret = mipi_dbi_lcdic_configure(dev, dbi_config);
	if (ret) {
		goto out;
	}

	/* State reset is required before transfer */
	mipi_dbi_lcdic_reset_state(dev);

	/* Write command */
	mipi_dbi_lcdic_set_cmd(base, LCDIC_TX, LCDIC_COMMAND,
			       LCDIC_DATA_FMT_BYTE, 1);
	/* Use standard byte writes */
	dev_data->pixel_fmt = LCDIC_DATA_FMT_BYTE;
	base->TFIFO_WDATA = cmd;
	/* Wait for command completion */
	while ((base->IRSR & LCDIC_IRSR_CMD_DONE_RAW_INTR_MASK) == 0) {
		/* Spin */
	}
	base->ICR |= LCDIC_ICR_CMD_DONE_INTR_CLR_MASK;

	if (data_len != 0) {
		dev_data->xfer_bytes = data_len;
		/* Cap command to max transfer size */
		dev_data->cmd_bytes = MIN(data_len, LCDIC_MAX_XFER);
		dev_data->xfer_buf = data;
		/* If the length of the transfer is not divisible by
		 * 4, save the unaligned portion of the transfer into
		 * a temporary buffer
		 */
		if (dev_data->cmd_bytes & 0x3) {
			dev_data->unaligned_word = mipi_dbi_lcdic_get_unaligned(
							dev_data->xfer_buf,
							dev_data->cmd_bytes);
		}
		if (cmd == MIPI_DCS_WRITE_MEMORY_START) {
			/* Use pixel format data width, so we can byte swap
			 * if needed
			 */
			mipi_dbi_lcdic_set_cmd(base, LCDIC_TX, LCDIC_DATA,
					       dev_data->pixel_fmt,
					       dev_data->cmd_bytes);
		} else {
			mipi_dbi_lcdic_set_cmd(base, LCDIC_TX, LCDIC_DATA,
					       LCDIC_DATA_FMT_BYTE,
					       dev_data->cmd_bytes);
		}
#ifdef CONFIG_MIPI_DBI_NXP_LCDIC_DMA
		if (((((uint32_t)dev_data->xfer_buf) & 0x3) == 0) ||
		    (dev_data->cmd_bytes < 4)) {
			/* Data is aligned, we can use DMA */
			/* Enable command complete interrupt */
			interrupts |= LCDIC_IMR_CMD_DONE_INTR_MSK_MASK;
			/* Write interrupt mask */
			base->IMR &= ~interrupts;
			/* Configure DMA to send data */
			ret = mipi_dbi_lcdic_start_dma(dev);
			if (ret) {
				LOG_ERR("Could not start DMA (%d)", ret);
				goto out;
			}
		} else /* Data is not aligned */
#endif
		{
			/* Enable TX FIFO threshold interrupt. This interrupt
			 * should fire once enabled, which will kick off
			 * the transfer
			 */
			interrupts |= LCDIC_IMR_TFIFO_THRES_INTR_MSK_MASK;
			/* Enable command complete interrupt */
			interrupts |= LCDIC_IMR_CMD_DONE_INTR_MSK_MASK;
			/* Write interrupt mask */
			base->IMR &= ~interrupts;
		}
		ret = k_sem_take(&dev_data->xfer_sem, K_FOREVER);
	}
out:
	k_sem_give(&dev_data->lock);
	return ret;
}

static int mipi_dbi_lcdic_reset(const struct device *dev, k_timeout_t delay)
{
	const struct mipi_dbi_lcdic_config *config = dev->config;
	LCDIC_Type *base = config->base;
	uint32_t lcdic_freq;
	uint8_t rst_width, pulse_cnt;
	uint32_t delay_ms = k_ticks_to_ms_ceil32(delay);

	/* Calculate delay based off timer0 ratio. Formula given
	 * by RM is as follows:
	 * Reset pulse width = (RST_WIDTH + 1) * Timer0_Period
	 * Timer0_Period = 2^(TIMER_RATIO0) / LCDIC_Clock_Freq
	 */
	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
				   &lcdic_freq)) {
		return -EIO;
	}
	rst_width = (delay_ms * (lcdic_freq)) / ((1 << LCDIC_TIMER0_RATIO) * MSEC_PER_SEC);
	/* If rst_width is larger than max value supported by hardware,
	 * increase the pulse count (rounding up)
	 */
	pulse_cnt = ((rst_width + (LCDIC_MAX_RST_WIDTH - 1)) / LCDIC_MAX_RST_WIDTH);
	rst_width = MIN(LCDIC_MAX_RST_WIDTH, rst_width);

	/* Start the reset signal */
	base->RST_CTRL = LCDIC_RST_CTRL_RST_WIDTH(rst_width - 1) |
			LCDIC_RST_CTRL_RST_SEQ_NUM(pulse_cnt - 1) |
			LCDIC_RST_CTRL_RST_START_MASK;
	/* Wait for reset to complete */
	while ((base->IRSR & LCDIC_IRSR_RST_DONE_RAW_INTR_MASK) == 0) {
		/* Spin */
	}
	base->ICR |= LCDIC_ICR_RST_DONE_INTR_CLR_MASK;
	return 0;
}



/* Initializes LCDIC peripheral */
static int mipi_dbi_lcdic_init(const struct device *dev)
{
	const struct mipi_dbi_lcdic_config *config = dev->config;
	struct mipi_dbi_lcdic_data *data = dev->data;
	LCDIC_Type *base = config->base;
	int ret;

	ret = clock_control_on(config->clock_dev, config->clock_subsys);
	if (ret) {
		return ret;
	}

	/* Set initial clock rate of 10 MHz */
	ret = clock_control_set_rate(config->clock_dev, config->clock_subsys,
			(clock_control_subsys_rate_t)MHZ(10));
	if (ret) {
		return ret;
	}

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}
	ret = k_sem_init(&data->xfer_sem, 0, 1);
	if (ret) {
		return ret;
	}
	ret = k_sem_init(&data->lock, 1, 1);
	if (ret) {
		return ret;
	}
	/* Clear all interrupt flags */
	base->ICR = LCDIC_ALL_INTERRUPTS;
	/* Mask all interrupts */
	base->IMR = LCDIC_ALL_INTERRUPTS;

	/* Enable interrupts */
	config->irq_config_func(dev);

	/* Setup RX and TX fifo thresholds */
	base->FIFO_CTRL = LCDIC_FIFO_CTRL_RFIFO_THRES(LCDIC_RX_FIFO_THRESH) |
			LCDIC_FIFO_CTRL_TFIFO_THRES(LCDIC_TX_FIFO_THRESH);
	/* Disable command timeouts */
	base->TO_CTRL &= ~(LCDIC_TO_CTRL_CMD_LONG_TO_MASK |
			LCDIC_TO_CTRL_CMD_SHORT_TO_MASK);

	/* Ensure LCDIC timer ratios are at reset values */
	base->TIMER_CTRL = LCDIC_TIMER_CTRL_TIMER_RATIO1(LCDIC_TIMER1_RATIO) |
			LCDIC_TIMER_CTRL_TIMER_RATIO0(LCDIC_TIMER0_RATIO);

#ifdef CONFIG_MIPI_DBI_NXP_LCDIC_DMA
	/* Attach the LCDIC DMA request signal to the DMA channel we will
	 * use with hardware triggering.
	 */
	INPUTMUX_AttachSignal(INPUTMUX, data->dma_stream.channel,
			kINPUTMUX_LcdTxRegToDmaSingleToDma0);
	INPUTMUX_EnableSignal(INPUTMUX,
			kINPUTMUX_Dmac0InputTriggerLcdTxRegToDmaSingleEna, true);
#endif

	return 0;
}

static struct mipi_dbi_driver_api mipi_dbi_lcdic_driver_api = {
	.command_write = mipi_dbi_lcdic_write_cmd,
	.write_display = mipi_dbi_lcdic_write_display,
	.reset = mipi_dbi_lcdic_reset,
};

static void mipi_dbi_lcdic_isr(const struct device *dev)
{
	const struct mipi_dbi_lcdic_config *config = dev->config;
	struct mipi_dbi_lcdic_data *data = dev->data;
	LCDIC_Type *base = config->base;
	uint32_t bytes_written, isr_status;

	isr_status = base->ISR;
	/* Clear pending interrupts */
	base->ICR |= isr_status;

	if (isr_status & LCDIC_ISR_CMD_DONE_INTR_MASK) {
		if (config->base->CTRL & LCDIC_CTRL_DMA_EN_MASK) {
			/* DMA completed. Update buffer tracking data */
			data->xfer_bytes -= data->cmd_bytes;
			data->xfer_buf += data->cmd_bytes;
			/* Disable DMA request */
			config->base->CTRL &= ~LCDIC_CTRL_DMA_EN_MASK;
		}
		if (data->xfer_bytes == 0) {
			/* Disable interrupts */
			base->IMR |= LCDIC_ALL_INTERRUPTS;
			/* All data has been sent. */
			k_sem_give(&data->xfer_sem);
		} else {
			/* Command done. Queue next command */
			data->cmd_bytes = MIN(data->xfer_bytes, LCDIC_MAX_XFER);
			mipi_dbi_lcdic_set_cmd(base, LCDIC_TX, LCDIC_DATA,
					       LCDIC_DATA_FMT_BYTE,
					       data->cmd_bytes);
			if (data->cmd_bytes & 0x3) {
				/* Save unaligned portion of transfer into
				 * a temporary buffer
				 */
				data->unaligned_word = mipi_dbi_lcdic_get_unaligned(
								data->xfer_buf,
								data->cmd_bytes);
			}
#ifdef CONFIG_MIPI_DBI_NXP_LCDIC_DMA
			if (((((uint32_t)data->xfer_buf) & 0x3) == 0) ||
			    (data->cmd_bytes < 4)) {
				/* Data is aligned. We can use DMA */
				mipi_dbi_lcdic_start_dma(dev);
			} else
#endif
			{
				/* We must refill the FIFO here in order to continue
				 * the next transfer, since the TX FIFO threshold
				 * interrupt may have already fired.
				 */
				bytes_written = mipi_dbi_lcdic_fill_tx(base, data->xfer_buf,
								       data->cmd_bytes,
								       data->unaligned_word);
				if (bytes_written > 0) {
					data->xfer_buf += bytes_written;
					data->cmd_bytes -= bytes_written;
					data->xfer_bytes -= bytes_written;
				}
			}
		}
	} else if (isr_status & LCDIC_ISR_TFIFO_THRES_INTR_MASK) {
		/* If command is not done, continue filling TX FIFO from
		 * current transfer buffer
		 */
		bytes_written = mipi_dbi_lcdic_fill_tx(base, data->xfer_buf,
						      data->cmd_bytes,
						      data->unaligned_word);
		if (bytes_written > 0) {
			data->xfer_buf += bytes_written;
			data->cmd_bytes -= bytes_written;
			data->xfer_bytes -= bytes_written;
		}
	}
}

#ifdef CONFIG_MIPI_DBI_NXP_LCDIC_DMA
#define LCDIC_DMA_CHANNELS(n)						\
	.dma_stream = {							\
		.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR(n)),		\
		.channel = DT_INST_DMAS_CELL_BY_IDX(n, 0, channel),	\
		.dma_cfg = {						\
			.dma_slot = LPC_DMA_HWTRIG_EN |			\
				LPC_DMA_TRIGPOL_HIGH_RISING |		\
				LPC_DMA_TRIGBURST,			\
			.channel_direction = MEMORY_TO_PERIPHERAL,	\
			.dma_callback = mipi_dbi_lcdic_dma_callback,	\
			.source_data_size = 4,				\
			.dest_data_size = 4,				\
			.user_data = (void *)DEVICE_DT_INST_GET(n),	\
		},							\
	},
#else
#define LCDIC_DMA_CHANNELS(n)
#endif


#define MIPI_DBI_LCDIC_INIT(n)						\
	static void mipi_dbi_lcdic_config_func_##n(			\
			const struct device *dev)			\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),	\
			    mipi_dbi_lcdic_isr,				\
			    DEVICE_DT_INST_GET(n), 0);			\
									\
		irq_enable(DT_INST_IRQN(n));				\
	}								\
									\
	PINCTRL_DT_INST_DEFINE(n);					\
	static const struct mipi_dbi_lcdic_config			\
	    mipi_dbi_lcdic_config_##n = {				\
		.base = (LCDIC_Type *)DT_INST_REG_ADDR(n),		\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),	\
		.clock_subsys = (clock_control_subsys_t)		\
		    DT_INST_CLOCKS_CELL(n, name),			\
		.irq_config_func = mipi_dbi_lcdic_config_func_##n,	\
		.swap_bytes = DT_INST_PROP(n, nxp_swap_bytes),		\
	};								\
	static struct mipi_dbi_lcdic_data mipi_dbi_lcdic_data_##n = {	\
		LCDIC_DMA_CHANNELS(n)					\
	};								\
	DEVICE_DT_INST_DEFINE(n, mipi_dbi_lcdic_init, NULL,		\
			&mipi_dbi_lcdic_data_##n,			\
			&mipi_dbi_lcdic_config_##n,			\
			POST_KERNEL,					\
			CONFIG_MIPI_DBI_INIT_PRIORITY,			\
			&mipi_dbi_lcdic_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MIPI_DBI_LCDIC_INIT)
