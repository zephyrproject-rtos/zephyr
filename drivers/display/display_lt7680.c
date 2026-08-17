/*
 * Copyright (c) 2025 JUMO GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT levetop_lt7680

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/types.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(display_lt7680, CONFIG_DISPLAY_LOG_LEVEL);

/*
 * Levetop LT768x Display Controller Driver - Register definitions
 *
 * LT768 SPI wire protocol - bits[7:6] of the FIRST byte of each transaction
 * encode the access mode (lower 6 bits are irrelevant for the access type):
 *
 *   0x00 (bits[7:6]=00) : select register address   -> 2nd byte = reg addr
 *   0x40 (bits[7:6]=01) : read status               -> 2nd byte clocked back = status
 *   0x80 (bits[7:6]=10) : write data                -> 2nd+ bytes = data
 *   0xC0 (bits[7:6]=11) : read data                 -> 2nd byte clocked back = value
 *
 * Every register write is TWO separate CS transactions:
 *   TX1: [0x00, reg_addr]           (select register)
 *   TX2: [0x80, data_value]         (write data)
 *
 * Every register read is TWO separate CS transactions:
 *   TX1: [0x00, reg_addr]           (select register)
 *   TX2: [0xC0, 0xFF], RX: [_, val] (read data; second RX byte = value)
 *
 * Status read is ONE transaction:
 *   TX: [0x40, 0xFF], RX: [_, status] (second RX byte = status byte)
 *
 * Memory (pixel) burst write is TWO transaction:
 *   TX1: [0x00, LT7680_REG_MRWDP] CONFIG_EMUL  (select memory R/W data port)
 *   TX2: [0x80, p0, p1, p2, ...]     (data-write mode + continuous pixel stream)
 */

/* SPI access-mode bytes */
#define LT7680_SPI_SEL_REG     0x00u
#define LT7680_SPI_STATUS_READ 0x40u
#define LT7680_SPI_DATA_WRITE  0x80u
#define LT7680_SPI_DATA_READ   0xC0u

/* Registers. */
#define LT7680_SRR        0x00
#define LT7680_CCR        0x01
#define LT7680_MCR        0x02
#define LT7680_ICR        0x03
#define LT7680_MRWDP      0x04 /* Memory R/W Data Port */
#define LT7680_PPLLC1     0x05 /* PCLK (pixel clock) PLL control 1 */
#define LT7680_PPLLC2     0x06 /* PCLK (pixel clock) PLL control 2 */
#define LT7680_MPLLC1     0x07 /* MCLK (SDRAM clock) PLL control 1 */
#define LT7680_MPLLC2     0x08 /* MCLK (SDRAM clock) PLL control 2 */
#define LT7680_CPLLC1     0x09 /* CCLK (core clock) PLL control 1 */
#define LT7680_CPLLC2     0x0A /* CCLK (core clock) PLL control 2 */
#define LT7680_DPCR       0x12
#define LT7680_HDCR       0x13
#define LT7680_HDWR       0x14 /* Horizontal display width = (HDWR + 1) * 8 + HDWFTR  pixels */
#define LT7680_HDWFTR     0x15 /* fine tuning */
#define LT7680_HNDR       0x16 /* Horizontal non-display period (back porch) */
#define LT7680_HNDFTR     0x17 /* fine tuning */
#define LT7680_HSTR       0x18 /* HSYNC start position (front porch) */
#define LT7680_HPW        0x19 /* HSYNC pulse width */
/* Vertical display height */
#define LT7680_VDHR0      0x1A
#define LT7680_VDHR1      0x1B
/* Vertical non-display period (back porch) */
#define LT7680_VNDR0      0x1C
#define LT7680_VNDR1      0x1D
#define LT7680_VSTR       0x1E /* VSYNC start position (front porch) */
#define LT7680_VPWR       0x1F /* VSYNC pulse width */
/* Main Image Start Address */
#define LT7680_MISA0      0x20
#define LT7680_MISA1      0x21
#define LT7680_MISA2      0x22
#define LT7680_MISA3      0x23
/* Main Image Width (pixels, must be divisible by 4) */
#define LT7680_MIW0       0x24
#define LT7680_MIW1       0x25
/* Main Window Upper-Left corner XY */
#define LT7680_MWULX0     0x26
#define LT7680_MWULX1     0x27
#define LT7680_MWULY0     0x28
#define LT7680_MWULY1     0x29
/* Canvas Image Start Address */
#define LT7680_CVSSA0     0x50
#define LT7680_CVSSA1     0x51
#define LT7680_CVSSA2     0x52
#define LT7680_CVSSA3     0x53
/* Canvas Image Width */
#define LT7680_CVS_IMWTH0 0x54
#define LT7680_CVS_IMWTH1 0x55
/* Active Window Upper-Left XY */
#define LT7680_AWUL_X0    0x56
#define LT7680_AWUL_X1    0x57
#define LT7680_AWUL_Y0    0x58
#define LT7680_AWUL_Y1    0x59
/* Active Window Width / Height */
#define LT7680_AW_WTH0    0x5A
#define LT7680_AW_WTH1    0x5B
#define LT7680_AW_HT0     0x5C
#define LT7680_AW_HT1     0x5D
#define LT7680_MPWCTR     0x5Eu
/* Graphic Write Cursor (XY in block mode) */
#define LT7680_CURH0      0x5F
#define LT7680_CURH1      0x60
#define LT7680_CURV0      0x61
#define LT7680_CURV1      0x62
/* SDRAM registers */
#define LT7680_SDRC       0xE0u /* SDRAM Control */
#define LT7680_SDRCAS     0xE1u /* SDRAM CAS latency (0x02=CAS2, 0x03=CAS3) */
#define LT7680_SDRITV0    0xE2u /* SDRAM refresh interval [7:0] */
#define LT7680_SDRITV1    0xE3u /* SDRAM refresh interval [9:8] */
#define LT7680_SDRINI     0xE4u /* SDRAM initialise: write 0x01 */

/* Software Reset Register */
#define LT7680_SRR_SWRESET      BIT(1)
#define LT7680_SRR_RECONFIG_PLL BIT(7)

/* Chip Configuration Register */
#define LT7680_CCR_HOST        BIT(0)
#define LT7680_CCR_HOST_8BIT   0
#define LT7680_CCR_HOST_16BIT  1
#define LT7680_CCR_SPI_MASTER  BIT(1)
#define LT7680_CCR_I2C_MASTER  BIT(2)
#define LT7680_CCR_TFT         GENMASK(4, 3)
#define LT7680_CCR_TFT_24BIT   0
#define LT7680_CCR_TFT_18BIT   1
#define LT7680_CCR_TFT_16BIT   2
#define LT7680_CCR_TFT_LVDS    3
#define LT7680_CCR_KEYPAD_SCAN BIT(5)
#define LT7680_CCR_SLEEP       BIT(6)
#define LT7680_CCR_PLL_ENABLE  BIT(7)

/* Memory Configuration Register */
/* Memory write direction */
#define LT7680_MCR_MWRITE                GENMASK(2, 1)
#define LT7680_MCR_MWRITE_LR_TB          0 /* left->right, top->bottom */
#define LT7680_MCR_MWRITE_RL_TB          1 /* right->left, top->bottom */
#define LT7680_MCR_MWRITE_TB_LR          2 /* top->bottom, left->right */
#define LT7680_MCR_MWRITE_BT_LR          3 /* bottom->top, left->right */
#define LT7680_MCR_IMGFMT                GENMASK(7, 6)
#define LT7680_MCR_IMGFMT_DIRECT_WRITE   0
#define LT7680_MCR_IMGFMT_MASK_HIGH_EACH 1
#define LT7680_MCR_IMGFMT_MASK_HIGH_EVEN 2

/* Interface Control Register */
#define LT7680_ICR_MEM             GENMASK(1, 0)
#define LT7680_ICR_MEM_SDRAM       0
#define LT7680_ICR_MEM_GAMMA       1
#define LT7680_ICR_MEM_CURSOR_RAM  2
#define LT7680_ICR_MEM_PALETTE_RAM 3
#define LT7680_ICR_MODE            BIT(2)
#define LT7680_ICR_MODE_GRAPHIC    0
#define LT7680_ICR_MODE_TEXT       1

/* [05h-0Ah] PLL Registers
 * Format for registers 0x05, 0x07, 0x09:
 *   bits[7:6] = lpllOD  (output divider, actual_div = 2^(OD-1), OD=1..3)
 *   bits[4:1] = lpllR   (input divider R, >=1)
 *   bit[0]    = lpllN[8] (MSB of 9-bit multiplier N)
 * Registers 0x06, 0x08, 0x0A hold lpllN[7:0].
 * PLL output frequency:  FOUT = XI_freq * N / (R * 2^(OD-1))
 *   where XI_freq is the crystal frequency, N = 9-bit value,
 *   R is the 4-bit input divider.
 */
#define LT7680_XPLLC1_PLL_DIV_MSB BIT(0)
#define LT7680_XPLLC1_INPUT_DIV   GENMASK(5, 1)
#define LT7680_XPLLC1_OUTPUT_DIV  GENMASK(7, 6)
#define LT7680_XPLLC2_PLLDIV      GENMASK(7, 0)

/* Display Panel Control Register */
#define LT7680_DPCR_PDATA                  GENMASK(2, 0)
#define LT7680_DPCR_PDATA_RGB              0
#define LT7680_DPCR_PDATA_BGR              5
#define LT7680_DPCR_VSCAN                  BIT(3)
#define LT7680_DPCR_VSCAN_T_TO_B           0 /* vertical scan top->bottom */
#define LT7680_DPCR_VSCAN_B_TO_T           1 /* vertical scan bottom->top */
#define LT7680_DPCR_DISPLAY_TEST_COLOR_BAR BIT(5)
#define LT7680_DPCR_DISPLAY_ON             BIT(6)
#define LT7680_DPCR_PCLK                   BIT(7)
#define LT7680_DPCR_PCLK_RISING            0
#define LT7680_DPCR_PCLK_FALLING           1

/* Horizontal/Display Control Register */
#define LT7680_HDCR_PDE            BIT(5)
#define LT7680_HDCR_PDE_HIGH_ACT   0
#define LT7680_HDCR_PDE_LOW_ACT    1
#define LT7680_HDCR_VSYNC          BIT(6)
#define LT7680_HDCR_VSYNC_LOW_ACT  0
#define LT7680_HDCR_VSYNC_HIGH_ACT 1
#define LT7680_HDCR_HSYNC          BIT(7)
#define LT7680_HDCR_HSYNC_LOW_ACT  0
#define LT7680_HDCR_HSYNC_HIGH_ACT 1

/* Cursor and addressing mode control */
#define LT7680_MPWCTR_BPP         GENMASK(1, 0)
#define LT7680_MPWCTR_BPP_8       0
#define LT7680_MPWCTR_BPP_16      1
#define LT7680_MPWCTR_BPP_24      2
#define LT7680_MPWCTR_MODE        BIT(2)
#define LT7680_MPWCTR_MODE_XY     0
#define LT7680_MPWCTR_MODE_LINEAR 1

/* SDRC default value (64 Mbit, 4 banks, row=13, col=9, 16-bit) */
#define LT7680_SDRC_DEFAULT 0x29 /* 32MB SDRAM at 143 MHz */

#define LT7680_SDRCAS_CAS3 0x03

/* Status register bit masks (read from SPI status byte) */
#define LT7680_STATUS_INTERRUPT         BIT(0)
#define LT7680_STATUS_POWER_SAVING      BIT(1)
#define LT7680_STATUS_SDRAM_READY       BIT(2)
#define LT7680_STATUS_2D_BUSY           BIT(3)
#define LT7680_STATUS_MEM_WR_FIFO_EMPTY BIT(6)
#define LT7680_STATUS_MEM_WR_FIFO_FULL  BIT(7)

struct lt7680_config {
	struct spi_dt_spec spi_spec;
	struct gpio_dt_spec reset_gpio;

	uint16_t width;
	uint16_t height;
	uint16_t reset_delay_ms;

	uint32_t clock_frequency;

	uint8_t rotation;
	uint16_t hsync_len;
	uint16_t hback_porch;
	uint16_t hfront_porch;
	uint16_t vsync_len;
	uint16_t vback_porch;
	uint16_t vfront_porch;
	bool hsync_active;
	bool vsync_active;
	bool de_active;
	bool pixelclk_active;
};

struct lt7680_data {
	enum display_pixel_format current_pixel_format;
	bool display_on;
	bool blanking;
	enum display_orientation orientation;
};

static int lt7680_read_status(const struct device *dev, uint8_t *status)
{
	const struct lt7680_config *cfg = dev->config;
	int ret;

	uint8_t tx[] = {LT7680_SPI_STATUS_READ, 0xFF};
	uint8_t rx[] = {0, 0};
	const struct spi_buf tx_buf = {.buf = tx, .len = sizeof(tx)};
	const struct spi_buf rx_buf = {.buf = rx, .len = sizeof(rx)};
	const struct spi_buf_set tx_set = {.buffers = &tx_buf, .count = 1};
	const struct spi_buf_set rx_set = {.buffers = &rx_buf, .count = 1};

	ret = spi_transceive_dt(&cfg->spi_spec, &tx_set, &rx_set);
	if (ret >= 0) {
		*status = rx[1];
	}

	return ret;
}

static int lt7680_read_reg(const struct device *dev, uint8_t reg, uint8_t *val)
{
	const struct lt7680_config *cfg = dev->config;
	int ret;

	uint8_t sel[] = {LT7680_SPI_SEL_REG, reg};
	const struct spi_buf sel_buf = {.buf = sel, .len = sizeof(sel)};
	const struct spi_buf_set sel_set = {.buffers = &sel_buf, .count = 1};

	uint8_t tx[] = {LT7680_SPI_DATA_READ, 0xFF};
	uint8_t rx[] = {0, 0};
	const struct spi_buf tx_buf = {.buf = tx, .len = sizeof(tx)};
	const struct spi_buf rx_buf = {.buf = rx, .len = sizeof(rx)};
	const struct spi_buf_set tx_set = {.buffers = &tx_buf, .count = 1};
	const struct spi_buf_set rx_set = {.buffers = &rx_buf, .count = 1};

	ret = spi_write_dt(&cfg->spi_spec, &sel_set);
	if (ret < 0) {
		return ret;
	}

	ret = spi_transceive_dt(&cfg->spi_spec, &tx_set, &rx_set);
	if (ret == 0) {
		*val = rx[1];
	}

	return ret;
}

static int lt7680_write_reg(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct lt7680_config *cfg = dev->config;
	int ret;

	uint8_t sel[] = {LT7680_SPI_SEL_REG, reg};
	uint8_t wr[] = {LT7680_SPI_DATA_WRITE, val};
	const struct spi_buf sel_buf = {.buf = sel, .len = sizeof(sel)};
	const struct spi_buf wr_buf = {.buf = wr, .len = sizeof(wr)};
	const struct spi_buf_set sel_set = {.buffers = &sel_buf, .count = 1};
	const struct spi_buf_set wr_set = {.buffers = &wr_buf, .count = 1};

	ret = spi_write_dt(&cfg->spi_spec, &sel_set);
	if (ret < 0) {
		return ret;
	}

	return spi_write_dt(&cfg->spi_spec, &wr_set);
}

/** @brief  Write two consecutive registers (16-bit value, LSB first). */
static int lt7680_write_reg16(const struct device *dev, uint8_t reg, uint16_t val)
{
	int ret;

	ret = lt7680_write_reg(dev, reg, val & 0xFF);
	if (ret < 0) {
		return ret;
	}

	return lt7680_write_reg(dev, reg + 1, val >> 8);
}

/** @brief  Write four consecutive registers (32-bit value, LSB first). */
static int lt7680_write_reg32(const struct device *dev, uint8_t reg, uint32_t val)
{
	int ret;

	ret = lt7680_write_reg(dev, reg, val & 0xFF);
	if (ret < 0) {
		return ret;
	}

	ret = lt7680_write_reg(dev, reg + 1, (val >> 8) & 0xFF);
	if (ret < 0) {
		return ret;
	}

	ret = lt7680_write_reg(dev, reg + 2, (val >> 16) & 0xFF);
	if (ret < 0) {
		return ret;
	}

	return lt7680_write_reg(dev, reg + 3, (val >> 24) & 0xFF);
}

static int lt7680_write_reg_div8sub1(const struct device *dev, uint8_t reg, uint16_t val)
{
	if (val >= 8) {
		return lt7680_write_reg(dev, reg, val / 8 - 1);
	} else {
		return lt7680_write_reg(dev, reg, 0);
	}
}

static int lt7680_write_reg16_div8sub1(const struct device *dev, uint8_t reg, uint16_t val)
{
	int ret;

	ret = lt7680_write_reg_div8sub1(dev, reg, val);
	if (ret) {
		return ret;
	}

	return lt7680_write_reg(dev, reg + 1, val % 8);
}

static int lt7680_wait_for_status(const struct device *dev, uint8_t mask, bool present)
{
	uint8_t status;
	int ret;

	for (size_t i = 0; i < 200; i++) {
		ret = lt7680_read_status(dev, &status);
		if (ret < 0) {
			return ret;
		}

		if ((!!(status & mask)) == present) {
			return 0;
		}

		k_msleep(1);
	}

	return -ETIMEDOUT;
}

/** @brief  Set active window + move memory-write cursor to (x, y). */
static int lt7680_set_window(const struct device *dev, uint16_t x, uint16_t y, uint16_t w,
			     uint16_t h)
{
	int ret;
	const struct lt7680_config *cfg = dev->config;
	struct lt7680_data *data = dev->data;
	uint16_t windowx = x;
	uint16_t windowy = y;
	uint16_t cursorx = x;
	uint16_t cursory = y;

	if (data->orientation == DISPLAY_ORIENTATION_ROTATED_180) {
		windowx = cfg->width - x - w;
		cursorx = cfg->width - 1 - x;
	}

	/* Active window origin */
	ret = lt7680_write_reg16(dev, LT7680_AWUL_X0, windowx);
	if (ret < 0) {
		return ret;
	}
	ret = lt7680_write_reg16(dev, LT7680_AWUL_Y0, windowy);
	if (ret < 0) {
		return ret;
	}

	/* Active window size */
	ret = lt7680_write_reg16(dev, LT7680_AW_WTH0, w);
	if (ret < 0) {
		return ret;
	}
	ret = lt7680_write_reg16(dev, LT7680_AW_HT0, h);
	if (ret < 0) {
		return ret;
	}

	/* Move write cursor to (x, y) */
	ret = lt7680_write_reg16(dev, LT7680_CURH0, cursorx);
	if (ret < 0) {
		return ret;
	}

	return lt7680_write_reg16(dev, LT7680_CURV0, cursory);
}

/**
 * @brief  Push raw pixel bytes to the LT768 frame-buffer.
 *
 * SPI TX: [0x80 (data-write mode), pixel_data...] - single burst transaction.
 * The LT768 auto-increments the write address within the active window.
 */
static int lt7680_push_pixels(const struct device *dev, const uint8_t *data, size_t len)
{
	const struct lt7680_config *cfg = dev->config;
	int ret;

	uint8_t sel[] = {LT7680_SPI_SEL_REG, LT7680_MRWDP};
	const struct spi_buf sel_buf = {.buf = sel, .len = sizeof(sel)};
	const struct spi_buf_set sel_set = {.buffers = &sel_buf, .count = 1};

	/* Transaction 1: select register */
	ret = spi_write_dt(&cfg->spi_spec, &sel_set);
	if (ret < 0) {
		return ret;
	}

	if (IS_ENABLED(CONFIG_LT7680_TRANSMIT_DATA_AS_BURST)) {
		/* Transaction 2: write pixel bytes as burst */
		uint8_t cmd = LT7680_SPI_DATA_WRITE;
		const struct spi_buf tx_bufs[] = {
			{.buf = &cmd, .len = 1},
			{.buf = (void *)data, .len = len},
		};
		const struct spi_buf_set tx_set = {
			.buffers = tx_bufs,
			.count = ARRAY_SIZE(tx_bufs),
		};

		ret = spi_write_dt(&cfg->spi_spec, &tx_set);
		if (ret < 0) {
			return ret;
		}
	} else {
		/* Transaction 2: write pixel pytes as single bytes with fifo-Check */
		for (size_t i = 0; i < len; i++) {
			uint8_t tx[] = {LT7680_SPI_DATA_WRITE, data[i]};
			const struct spi_buf tx_buf = {.buf = tx, .len = sizeof(tx)};
			const struct spi_buf_set tx_set = {.buffers = &tx_buf, .count = 1};

			/* Wait for MEM_WR_FIFO to have space for at least one more byte */
			ret = lt7680_wait_for_status(dev, LT7680_STATUS_MEM_WR_FIFO_FULL, false);
			if (ret < 0) {
				return ret;
			}

			/* Transaction 2: write pixel byte */
			ret = spi_write_dt(&cfg->spi_spec, &tx_set);
			if (ret < 0) {
				return ret;
			}
		}
	}

	return 0;
}

/** @brief  Write a rectangular region of pixels. */
static int lt7680_write(const struct device *dev, const uint16_t x, const uint16_t y,
			const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct lt7680_config *cfg = dev->config;
	struct lt7680_data *data = dev->data;
	int ret;

	if ((buf == NULL) || (desc == NULL)) {
		return -EINVAL;
	}

	if (((x + desc->width) > cfg->width) || ((y + desc->height) > cfg->height)) {
		LOG_ERR("Write (%u,%u)+(%ux%u) exceeds display (%ux%u)", x, y, desc->width,
			desc->height, cfg->width, cfg->height);
		return -EINVAL;
	}

	if (data->blanking) {
		/* discard while blanked */
		return 0;
	}

	ret = lt7680_set_window(dev, x, y, desc->width, desc->height);
	if (ret < 0) {
		LOG_ERR("set_window failed: %d", ret);
		return ret;
	}

	size_t px_size = (data->current_pixel_format == PIXEL_FORMAT_RGB_888) ? 3 : 2;
	size_t byte_len = (size_t)desc->width * (size_t)desc->height * px_size;

	ret = lt7680_push_pixels(dev, buf, byte_len);
	if (ret < 0) {
		LOG_ERR("pixel push failed: %d", ret);
	}

	return ret;
}

/**
 * @brief  Read a rectangular region of pixels from the frame-buffer.
 *
 * @note   Read-back requires the SPI bus to support half-duplex / MISO.
 *         Verify your MIPI DBI host controller supports command_read.
 */
static int lt7680_read(const struct device *dev, const uint16_t x, const uint16_t y,
		       const struct display_buffer_descriptor *desc, void *buf)
{
	const struct lt7680_config *cfg = dev->config;
	struct lt7680_data *data = dev->data;
	size_t px_size;
	size_t byte_len;
	uint8_t *out;
	int ret;

	uint8_t sel[] = {LT7680_SPI_SEL_REG, LT7680_MRWDP};
	const struct spi_buf sel_buf = {.buf = sel, .len = sizeof(sel)};
	const struct spi_buf_set sel_set = {.buffers = &sel_buf, .count = 1};

	if ((buf == NULL) || (desc == NULL)) {
		return -EINVAL;
	}

	if (((x + desc->width) > cfg->width) || ((y + desc->height) > cfg->height)) {
		return -EINVAL;
	}

	ret = lt7680_set_window(dev, x, y, desc->width, desc->height);
	if (ret < 0) {
		LOG_ERR("set_window failed: %d", ret);
		return ret;
	}

	px_size = (data->current_pixel_format == PIXEL_FORMAT_RGB_888) ? 3 : 2;
	byte_len = (size_t)desc->width * (size_t)desc->height * px_size;

	/* Transaction 1: select MRWDP register, identical to lt7680_push_pixels */
	ret = spi_write_dt(&cfg->spi_spec, &sel_set);
	if (ret < 0) {
		return ret;
	}

	if (IS_ENABLED(CONFIG_LT7680_READ_DATA_AS_BURST)) {
		uint8_t tx_cmd[] = {LT7680_SPI_DATA_READ, 0xFFu};
		uint8_t rx_dummy[] = {0, 0};
		const struct spi_buf tx_bufs[] = {
			{.buf = tx_cmd, .len = sizeof(tx_cmd)},
			{.buf = buf, .len = byte_len},
		};
		const struct spi_buf rx_bufs[] = {
			{.buf = rx_dummy, .len = sizeof(rx_dummy)},
			{.buf = buf, .len = byte_len},
		};
		const struct spi_buf_set tx_set = {
			.buffers = tx_bufs,
			.count = ARRAY_SIZE(tx_bufs),
		};
		const struct spi_buf_set rx_set = {
			.buffers = rx_bufs,
			.count = ARRAY_SIZE(rx_bufs),
		};

		/* Transaction 2: read buffer as burst. */
		ret = spi_transceive_dt(&cfg->spi_spec, &tx_set, &rx_set);
		if (ret < 0) {
			return ret;
		}
	} else {
		uint8_t tx[] = {LT7680_SPI_DATA_READ, 0xFFu};
		uint8_t rx[] = {0, 0};
		const struct spi_buf tx_buf = {.buf = tx, .len = sizeof(tx)};
		const struct spi_buf rx_buf = {.buf = rx, .len = sizeof(rx)};
		const struct spi_buf_set tx_set = {.buffers = &tx_buf, .count = 1};
		const struct spi_buf_set rx_set = {.buffers = &rx_buf, .count = 1};

		/* Transaction 2..N: read one byte per 2-byte transceive - identical to
		 * lt7680_read_reg
		 */
		out = (uint8_t *)buf;
		for (size_t i = 0; i < byte_len; i++) {
			ret = spi_transceive_dt(&cfg->spi_spec, &tx_set, &rx_set);
			if (ret < 0) {
				return ret;
			}

			out[i] = rx[1];
		}
	}

	return 0;
}

static void *lt7680_get_framebuffer(const struct device *dev)
{
	return NULL;
}

static int lt7680_set_brightness(const struct device *dev, const uint8_t brightness)
{
	return -ENOTSUP;
}

static int lt7680_set_contrast(const struct device *dev, const uint8_t contrast)
{
	return -ENOTSUP;
}

static int lt7680_blanking_on(const struct device *dev)
{
	struct lt7680_data *data = dev->data;
	uint8_t val;
	int ret;

	ret = lt7680_read_reg(dev, LT7680_DPCR, &val);
	if (ret < 0) {
		return ret;
	}

	val &= ~LT7680_DPCR_DISPLAY_ON;

	ret = lt7680_write_reg(dev, LT7680_DPCR, val);
	if (ret == 0) {
		data->blanking = true;
	}

	return ret;
}

static int lt7680_blanking_off(const struct device *dev)
{
	struct lt7680_data *data = dev->data;
	uint8_t val;
	int ret;

	ret = lt7680_read_reg(dev, LT7680_DPCR, &val);
	if (ret < 0) {
		return ret;
	}

	val |= LT7680_DPCR_DISPLAY_ON;

	ret = lt7680_write_reg(dev, LT7680_DPCR, val);
	if (ret == 0) {
		data->blanking = false;
	}

	return ret;
}

static void lt7680_get_capabilities(const struct device *dev, struct display_capabilities *caps)
{
	const struct lt7680_config *cfg = dev->config;
	const struct lt7680_data *data = dev->data;

	*caps = (struct display_capabilities){
		.x_resolution = cfg->width,
		.y_resolution = cfg->height,
		.supported_pixel_formats = PIXEL_FORMAT_RGB_565 | PIXEL_FORMAT_RGB_888,
		.current_pixel_format = data->current_pixel_format,
		.current_orientation = data->orientation,
	};
}

static int lt7680_set_pixel_format(const struct device *dev, const enum display_pixel_format fmt)
{
	struct lt7680_data *data = dev->data;
	uint8_t mode_bpp;
	uint8_t val;
	int ret;

	switch (fmt) {
	case PIXEL_FORMAT_RGB_565:
		mode_bpp = LT7680_MPWCTR_BPP_16;
		break;
	case PIXEL_FORMAT_RGB_888:
		mode_bpp = LT7680_MPWCTR_BPP_24;
		break;
	default:
		LOG_ERR("Unsupported pixel format: %d", fmt);
		return -ENOTSUP;
	}

	ret = lt7680_read_reg(dev, LT7680_MPWCTR, &val);
	if (ret < 0) {
		return ret;
	}

	val &= ~LT7680_MPWCTR_BPP;
	val |= FIELD_PREP(LT7680_MPWCTR_BPP, mode_bpp);

	ret = lt7680_write_reg(dev, LT7680_MPWCTR, val);
	if (ret == 0) {
		data->current_pixel_format = fmt;
	}

	return ret;
}

static int lt7680_set_orientation(const struct device *dev,
				  const enum display_orientation orientation)
{
	struct lt7680_data *data = dev->data;
	uint8_t mcr;
	uint8_t dpcr;
	uint8_t memory_direction = 0;
	uint8_t vscan_direction = 0;
	int ret;

	switch (orientation) {
	case DISPLAY_ORIENTATION_NORMAL:
		memory_direction = LT7680_MCR_MWRITE_LR_TB;
		vscan_direction = LT7680_DPCR_VSCAN_T_TO_B;
		break;
	case DISPLAY_ORIENTATION_ROTATED_180:
		memory_direction = LT7680_MCR_MWRITE_RL_TB;
		vscan_direction = LT7680_DPCR_VSCAN_B_TO_T;
		break;
	default:
		return -ENOTSUP;
	}

	ret = lt7680_read_reg(dev, LT7680_MCR, &mcr);
	if (ret < 0) {
		return ret;
	}

	mcr &= ~LT7680_MCR_MWRITE;
	mcr |= FIELD_PREP(LT7680_MCR_MWRITE, memory_direction);

	ret = lt7680_write_reg(dev, LT7680_MCR, mcr);
	if (ret < 0) {
		return ret;
	}

	ret = lt7680_read_reg(dev, LT7680_DPCR, &dpcr);
	if (ret < 0) {
		return ret;
	}

	dpcr &= ~LT7680_DPCR_VSCAN;
	dpcr |= FIELD_PREP(LT7680_DPCR_VSCAN, vscan_direction);

	ret = lt7680_write_reg(dev, LT7680_DPCR, dpcr);
	if (ret < 0) {
		return ret;
	}

	data->orientation = orientation;

	return 0;
}

/**
 * @brief  Toggle the optional hardware reset GPIO.
 *
 * When no reset GPIO is configured, a power-on stabilisation delay is used.
 */
static void lt7680_hw_reset(const struct device *dev)
{
	const struct lt7680_config *cfg = dev->config;

	if (cfg->reset_gpio.port) {
		gpio_pin_set_dt(&cfg->reset_gpio, 0);
		k_msleep(cfg->reset_delay_ms);

		gpio_pin_set_dt(&cfg->reset_gpio, 1);
		k_msleep(cfg->reset_delay_ms);

		gpio_pin_set_dt(&cfg->reset_gpio, 0);
		k_msleep(cfg->reset_delay_ms);
	}
}

static uint32_t lt7680_calculate_sclk_mhz(const struct device *dev)
{
	const struct lt7680_config *cfg = dev->config;

	uint32_t htotal =
		(uint32_t)cfg->width + cfg->hback_porch + cfg->hfront_porch + cfg->hsync_len;
	uint32_t vtotal =
		(uint32_t)cfg->height + cfg->vback_porch + cfg->vfront_porch + cfg->vsync_len;

	uint32_t sclk_mhz = 0;
	uint32_t sclk_hz = htotal * vtotal * 60;
	uint32_t round_tmp = (sclk_hz % 1000000) / 100000;

	if (round_tmp > 5) {
		sclk_mhz = sclk_hz / 1000000 + 1;
	} else {
		sclk_mhz = sclk_hz / 1000000;
	}

	return sclk_mhz;
}

static int lt7680_set_pll_control(const struct device *dev, uint8_t reg, uint8_t od, uint8_t r,
				  uint16_t n)
{
	int ret;
	uint8_t c1;
	uint8_t c2;

	c1 = FIELD_PREP(LT7680_XPLLC1_OUTPUT_DIV, od);
	c1 |= FIELD_PREP(LT7680_XPLLC1_INPUT_DIV, r);
	c1 |= FIELD_PREP(LT7680_XPLLC1_PLL_DIV_MSB, (n >> 8));
	ret = lt7680_write_reg(dev, reg, c1);
	if (ret < 0) {
		return ret;
	}

	c2 = FIELD_PREP(LT7680_XPLLC2_PLLDIV, (n & 0xFF));
	ret = lt7680_write_reg(dev, reg + 1, c2);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

/**
 * @brief  Configure the three on-chip PLLs (SCLK, MCLK, CCLK).
 *
 * Follows the algorithm from LT768_Lib.c::LT768_PLL_Initial():
 *   SCLK = pixel clock derived from panel timing * 60 Hz
 *   MCLK = CCLK = 3 * SCLK  (capped at 100 MHz)
 *   SCLK capped at 65 MHz
 */
static int lt7680_pll_init(const struct device *dev)
{
	const struct lt7680_config *cfg = dev->config;
	uint32_t xi_mhz;
	uint32_t sclk_mhz;
	uint32_t mclk_mhz;
	uint32_t cclk_mhz;
	int ret;

	xi_mhz = cfg->clock_frequency / 1000000u;
	if (xi_mhz != 10) {
		LOG_ERR("Unsupported clock frequency: %u kHz (only 10 MHz supported)",
			cfg->clock_frequency);
		return -ENOTSUP;
	}

	sclk_mhz = lt7680_calculate_sclk_mhz(dev);

	/* MCLK = CCLK = 3 * SCLK, capped at 100 MHz */
	mclk_mhz = sclk_mhz * 3;
	if (mclk_mhz > 100) {
		mclk_mhz = 100;
	}

	cclk_mhz = mclk_mhz;
	if (sclk_mhz > 65) {
		sclk_mhz = 65;
	}

	LOG_DBG("LT768 PLL: XI=%u MHz, SCLK=%u MHz, MCLK=%u MHz, CCLK=%u MHz", xi_mhz, sclk_mhz,
		mclk_mhz, cclk_mhz);

	ret = lt7680_set_pll_control(dev, LT7680_PPLLC1, 3, 5, 2 * sclk_mhz);
	if (ret < 0) {
		return ret;
	}

	ret = lt7680_set_pll_control(dev, LT7680_MPLLC1, 2, 5, mclk_mhz);
	if (ret < 0) {
		return ret;
	}

	ret = lt7680_set_pll_control(dev, LT7680_CPLLC1, 2, 5, cclk_mhz);
	if (ret < 0) {
		return ret;
	}

	ret = lt7680_write_reg(dev, LT7680_SRR, LT7680_SRR_RECONFIG_PLL);
	if (ret < 0) {
		return ret;
	}

	/* wait for PLLs to lock */
	k_msleep(1);

	return 0;
}

/**
 * @brief  Initialise the on-chip SDRAM.
 *
 * SDRAM refresh interval formula (from LT768_Lib.c):
 *   sdram_itv = (64_000_000 / 8192) / (1000 / MCLK_MHz) - 2
 *             = MCLK_kHz * 64 / 8192 - 2
 */
static int lt7680_sdram_init(const struct device *dev)
{
	uint16_t sdram_itv;
	uint32_t mclk_mhz;
	int ret;

	uint32_t sclk_mhz = lt7680_calculate_sclk_mhz(dev);

	mclk_mhz = MIN(sclk_mhz * 3, 100);

	if (sclk_mhz > 65) {
		sclk_mhz = 65;
	}

	sdram_itv = ((sclk_mhz * 1000u * 1000000u) / (8192u * 64u)) - 2u;

	ret = lt7680_write_reg(dev, LT7680_SDRC, LT7680_SDRC_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	ret = lt7680_write_reg(dev, LT7680_SDRCAS, LT7680_SDRCAS_CAS3);
	if (ret < 0) {
		return ret;
	}

	ret = lt7680_write_reg16(dev, LT7680_SDRITV0, sdram_itv);
	if (ret < 0) {
		return ret;
	}

	ret = lt7680_write_reg(dev, LT7680_SDRINI, 1);
	if (ret < 0) {
		return ret;
	}

	ret = lt7680_wait_for_status(dev, LT7680_STATUS_SDRAM_READY, true);
	if (ret < 0) {
		LOG_ERR("LT768: SDRAM init timeout");
	}

	return ret;
}

/**
 * @brief  Configure the panel control registers from the DT configuration.
 *
 * All horizontal values are in units of 8 pixels; vertical in lines.
 *
 * @note  If hsync_len == 0, detailed timing setup
 *        is skipped and the LT768 retains its power-on defaults.
 */
static int lt7680_panel_init(const struct device *dev)
{
	const struct lt7680_config *cfg = dev->config;
	const struct lt7680_data *data = dev->data;
	uint8_t reg_val = 0;
	uint8_t mcr;
	uint8_t icr;
	uint8_t dpcr;
	uint8_t hdcr;
	int ret;

	ret = lt7680_read_reg(dev, LT7680_CCR, &reg_val);
	if (ret < 0) {
		return ret;
	}

	reg_val &= ~LT7680_CCR_SPI_MASTER;
	reg_val &= ~LT7680_CCR_I2C_MASTER;

	reg_val &= ~LT7680_CCR_HOST;
	reg_val |= FIELD_PREP(LT7680_CCR_HOST, LT7680_CCR_HOST_8BIT);

	reg_val &= ~LT7680_CCR_TFT;
	reg_val |= FIELD_PREP(LT7680_CCR_TFT, LT7680_CCR_TFT_16BIT);

	ret = lt7680_write_reg(dev, LT7680_CCR, reg_val);
	if (ret < 0) {
		return ret;
	}

	mcr = FIELD_PREP(LT7680_MCR_MWRITE, LT7680_MCR_MWRITE_LR_TB);
	if (data->current_pixel_format == PIXEL_FORMAT_RGB_888) {
		mcr |= LT7680_MCR_IMGFMT_DIRECT_WRITE;
	} else {
		mcr |= LT7680_MCR_IMGFMT_MASK_HIGH_EACH;
	}
	ret = lt7680_write_reg(dev, LT7680_MCR, mcr);
	if (ret < 0) {
		return ret;
	}

	icr = FIELD_PREP(LT7680_ICR_MEM, LT7680_ICR_MEM_SDRAM);
	icr |= FIELD_PREP(LT7680_ICR_MODE, LT7680_ICR_MODE_GRAPHIC);
	ret = lt7680_write_reg(dev, LT7680_ICR, icr);
	if (ret < 0) {
		return ret;
	}

	/* PCLK edge, display OFF (enabled later), scan dir, RGB */
	dpcr = FIELD_PREP(LT7680_DPCR_VSCAN, LT7680_DPCR_VSCAN_T_TO_B);
	dpcr |= FIELD_PREP(LT7680_DPCR_PDATA, LT7680_DPCR_PDATA_RGB);
	if (!cfg->pixelclk_active) {
		dpcr |= FIELD_PREP(LT7680_DPCR_PCLK, LT7680_DPCR_PCLK_FALLING);
	}
	ret = lt7680_write_reg(dev, LT7680_DPCR, dpcr);
	if (ret < 0) {
		return ret;
	}

	hdcr = 0;
	if (cfg->de_active) {
		hdcr |= FIELD_PREP(LT7680_HDCR_PDE, LT7680_HDCR_PDE_HIGH_ACT);
	} else {
		hdcr |= FIELD_PREP(LT7680_HDCR_PDE, LT7680_HDCR_PDE_LOW_ACT);
	}
	if (cfg->vsync_active) {
		hdcr |= FIELD_PREP(LT7680_HDCR_VSYNC, LT7680_HDCR_VSYNC_HIGH_ACT);
	} else {
		hdcr |= FIELD_PREP(LT7680_HDCR_VSYNC, LT7680_HDCR_VSYNC_LOW_ACT);
	}
	if (cfg->hsync_active) {
		hdcr |= FIELD_PREP(LT7680_HDCR_HSYNC, LT7680_HDCR_HSYNC_HIGH_ACT);
	} else {
		hdcr |= FIELD_PREP(LT7680_HDCR_HSYNC, LT7680_HDCR_HSYNC_LOW_ACT);
	}
	ret = lt7680_write_reg(dev, LT7680_HDCR, hdcr);
	if (ret < 0) {
		return ret;
	}

	/* Skip detailed timing if not provided in DT */
	if (cfg->hsync_len == 0 || cfg->vsync_len == 0) {
		LOG_DBG("LT768: panel timing not configured, skipping");
		return 0;
	}

	/* Horizontal display width */
	ret = lt7680_write_reg16_div8sub1(dev, LT7680_HDWR, cfg->width);
	if (ret < 0) {
		return ret;
	}

	/* Vertical display height */
	ret = lt7680_write_reg16(dev, LT7680_VDHR0, cfg->height - 1);
	if (ret < 0) {
		return ret;
	}

	/* Horizontal non-display / back porch */
	ret = lt7680_write_reg16_div8sub1(dev, LT7680_HNDR, cfg->hback_porch);
	if (ret < 0) {
		return ret;
	}

	/* HSYNC start position / front porch */
	ret = lt7680_write_reg_div8sub1(dev, LT7680_HSTR, cfg->hfront_porch);
	if (ret < 0) {
		return ret;
	}

	/* HSYNC pulse width */
	ret = lt7680_write_reg_div8sub1(dev, LT7680_HPW, cfg->hsync_len);
	if (ret < 0) {
		return ret;
	}

	/* Vertical non-display / back porch */
	ret = lt7680_write_reg16(dev, LT7680_VNDR0, cfg->vback_porch - 1);
	if (ret < 0) {
		return ret;
	}

	/* VSYNC start position / front porch */
	ret = lt7680_write_reg(dev, LT7680_VSTR, cfg->vfront_porch - 1);
	if (ret < 0) {
		return ret;
	}

	/* VSYNC pulse width */
	return lt7680_write_reg(dev, LT7680_VPWR, cfg->vsync_len - 1);
}

/** @brief  Configure the main image layer and canvas for full-screen use. */
static int lt7680_layer_init(const struct device *dev)
{
	const struct lt7680_config *cfg = dev->config;
	const struct lt7680_data *data = dev->data;
	uint8_t mpw;
	int ret;

	/* Main image: origin at SDRAM address 0 */
	ret = lt7680_write_reg32(dev, LT7680_MISA0, 0);
	if (ret < 0) {
		return ret;
	}

	/* Main image width */
	ret = lt7680_write_reg16(dev, LT7680_MIW0, cfg->width);
	if (ret < 0) {
		return ret;
	}

	/* Main window upper-left at (0, 0) */
	ret = lt7680_write_reg16(dev, LT7680_MWULX0, 0);
	if (ret < 0) {
		return ret;
	}
	ret = lt7680_write_reg16(dev, LT7680_MWULY0, 0);
	if (ret < 0) {
		return ret;
	}

	/* Canvas: same frame-buffer, same width */
	ret = lt7680_write_reg32(dev, LT7680_CVSSA0, 0);
	if (ret < 0) {
		return ret;
	}
	ret = lt7680_write_reg16(dev, LT7680_CVS_IMWTH0, cfg->width);
	if (ret < 0) {
		return ret;
	}

	/* Active window = full display */
	ret = lt7680_set_window(dev, 0, 0, cfg->width, cfg->height);
	if (ret < 0) {
		return ret;
	}

	mpw = FIELD_PREP(LT7680_MPWCTR_MODE, LT7680_MPWCTR_MODE_XY);
	if (data->current_pixel_format == PIXEL_FORMAT_RGB_888) {
		mpw |= FIELD_PREP(LT7680_MPWCTR_BPP, LT7680_MPWCTR_BPP_24);
	} else {
		mpw |= FIELD_PREP(LT7680_MPWCTR_BPP, LT7680_MPWCTR_BPP_16);
	}
	return lt7680_write_reg(dev, LT7680_MPWCTR, mpw);
}

static int lt7680_init(const struct device *dev)
{
	const struct lt7680_config *cfg = dev->config;
	struct lt7680_data *data = dev->data;
	enum display_orientation orientation;
	int ret;

	if (cfg->rotation == 0) {
		orientation = DISPLAY_ORIENTATION_NORMAL;
	} else if (cfg->rotation == 180) {
		orientation = DISPLAY_ORIENTATION_ROTATED_180;
	} else {
		return -ENOTSUP;
	}

	if (!spi_is_ready_dt(&cfg->spi_spec)) {
		LOG_ERR("SPI bus not ready");
		return -ENODEV;
	}

	if (cfg->reset_gpio.port) {
		if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
			LOG_ERR("Reset GPIO not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Reset GPIO config failed: %d", ret);
			return ret;
		}
	}

	lt7680_hw_reset(dev);

	ret = lt7680_wait_for_status(dev, LT7680_STATUS_POWER_SAVING, false);
	if (ret < 0) {
		LOG_ERR("Power-saving wait failed: %d", ret);
		return ret;
	}

	ret = lt7680_pll_init(dev);
	if (ret < 0) {
		LOG_ERR("PLL init failed: %d", ret);
		return ret;
	}

	ret = lt7680_sdram_init(dev);
	if (ret < 0) {
		LOG_ERR("SDRAM init failed: %d", ret);
		return ret;
	}

	ret = lt7680_panel_init(dev);
	if (ret < 0) {
		LOG_ERR("Panel init failed: %d", ret);
		return ret;
	}

	ret = lt7680_layer_init(dev);
	if (ret < 0) {
		LOG_ERR("Layer init failed: %d", ret);
		return ret;
	}

	ret = lt7680_blanking_off(dev);
	if (ret < 0) {
		LOG_ERR("Display enable failed: %d", ret);
		return ret;
	}

	ret = lt7680_set_orientation(dev, orientation);
	if (ret < 0) {
		return ret;
	}

	data->display_on = true;
	data->blanking = false;

	LOG_INF("LT768 %ux%u initialised (SPI)", cfg->width, cfg->height);
	return 0;
}

static DEVICE_API(display, lt7680_api) = {
	.blanking_on = lt7680_blanking_on,
	.blanking_off = lt7680_blanking_off,
	.write = lt7680_write,
	.read = lt7680_read,
	.get_framebuffer = lt7680_get_framebuffer,
	.set_brightness = lt7680_set_brightness,
	.set_contrast = lt7680_set_contrast,
	.get_capabilities = lt7680_get_capabilities,
	.set_pixel_format = lt7680_set_pixel_format,
	.set_orientation = lt7680_set_orientation,
};

#define LT7680_INIT(inst)                                                                          \
	static struct lt7680_data lt7680_data_##inst = {                                           \
		.current_pixel_format = PIXEL_FORMAT_RGB_565,                                      \
		.display_on = false,                                                               \
		.blanking = false,                                                                 \
		.orientation = DISPLAY_ORIENTATION_NORMAL,                                         \
	};                                                                                         \
                                                                                                   \
	static const struct lt7680_config lt7680_config_##inst = {                                 \
		.spi_spec =                                                                        \
			SPI_DT_SPEC_GET(DT_INST(inst, levetop_lt7680),                             \
					SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8)),  \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                    \
		.width = DT_INST_PROP(inst, width),                                                \
		.height = DT_INST_PROP(inst, height),                                              \
		.reset_delay_ms = DT_INST_PROP(inst, reset_delay_ms),                              \
		.clock_frequency = DT_PROP_OR(DT_INST_CHILD(inst, display_timings),                \
					      clock_frequency, 10000000),                          \
		.rotation = DT_INST_PROP(inst, rotation),                                          \
		.hsync_len = DT_PROP(DT_INST_CHILD(inst, display_timings), hsync_len),             \
		.hback_porch = DT_PROP(DT_INST_CHILD(inst, display_timings), hback_porch),         \
		.hfront_porch = DT_PROP(DT_INST_CHILD(inst, display_timings), hfront_porch),       \
		.vsync_len = DT_PROP(DT_INST_CHILD(inst, display_timings), vsync_len),             \
		.vback_porch = DT_PROP(DT_INST_CHILD(inst, display_timings), vback_porch),         \
		.vfront_porch = DT_PROP(DT_INST_CHILD(inst, display_timings), vfront_porch),       \
		.hsync_active = DT_PROP(DT_INST_CHILD(inst, display_timings), hsync_active),       \
		.vsync_active = DT_PROP(DT_INST_CHILD(inst, display_timings), vsync_active),       \
		.de_active = DT_PROP(DT_INST_CHILD(inst, display_timings), de_active),             \
		.pixelclk_active = DT_PROP(DT_INST_CHILD(inst, display_timings), pixelclk_active), \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, lt7680_init, NULL, &lt7680_data_##inst, &lt7680_config_##inst, \
			      POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &lt7680_api);

DT_INST_FOREACH_STATUS_OKAY(LT7680_INIT)
