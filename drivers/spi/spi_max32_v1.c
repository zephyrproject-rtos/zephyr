/*
 * Copyright (c) 2023 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_max32_spi

#include <string.h>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>

/**
 * @ingroup  spi_registers
 * @defgroup SPI_CTRL0 SPI_CTRL0
 * @brief    Register for controlling SPI peripheral.
 * @{
 */
#define MAX32_SPI_CTRL0_EN       BIT(0) /**< CTRL0_EN Mask */
#define MAX32_SPI_CTRL0_MST_MODE BIT(1) /**< CTRL0_MST_MODE Mask */
#define MAX32_SPI_CTRL0_START    BIT(5) /**< CTRL0_START Mask */
#define MAX32_SPI_CTRL0_SS_CTRL  BIT(8) /**< CTRL0_SS_CTRL Mask */

#define MAX32_SPI_CTRL0_SS_ACTIVE_POS   16              /**< CTRL0_SS_ACTIVE Position */
#define MAX32_SPI_CTRL0_SS_ACTIVE       GENMASK(19, 16) /**< CTRL0_SS_ACTIVE Mask */
#define MAX32_S_SPI_CTRL0_SS_ACTIVE_SS0 BIT(16)         /**< CTRL0_SS_ACTIVE_SS0 Setting */
#define MAX32_S_SPI_CTRL0_SS_ACTIVE_SS1 BIT(17)         /**< CTRL0_SS_ACTIVE_SS1 Setting */
#define MAX32_S_SPI_CTRL0_SS_ACTIVE_SS2 BIT(18)         /**< CTRL0_SS_ACTIVE_SS2 Setting */
#define MAX32_S_SPI_CTRL0_SS_ACTIVE_SS3 BIT(19)         /**< CTRL0_SS_ACTIVE_SS3 Setting */

/**@} end of group SPI_CTRL0_Register */

/**
 * @ingroup  spi_registers
 * @defgroup SPI_CTRL1 SPI_CTRL1
 * @brief    Register for controlling SPI peripheral.
 * @{
 */
#define MAX32_SPI_CTRL1_TX_NUM_CHAR GENMASK(15, 0)  /**< CTRL1_TX_NUM_CHAR Mask */
#define MAX32_SPI_CTRL1_RX_NUM_CHAR GENMASK(31, 16) /**< CTRL1_RX_NUM_CHAR Mask */

#define MAX32_SPI_CTRL1_TX_NUM_CHAR_POS 0
#define MAX32_SPI_CTRL1_RX_NUM_CHAR_POS 16

/**@} end of group SPI_CTRL1_Register */

/**
 * @ingroup  spi_registers
 * @defgroup SPI_CTRL2 SPI_CTRL2
 * @brief    Register for controlling SPI peripheral.
 * @{
 */
#define MAX32_SPI_CTRL2_NUMBITS_POS 8              /**< CTRL2_NUMBITS Position */
#define MAX32_SPI_CTRL2_NUMBITS     GENMASK(11, 8) /**< CTRL2_NUMBITS Mask */

#define MAX32_SPI_CTRL2_DATA_WIDTH_POS    12              /**< CTRL2_DATA_WIDTH Position */
#define MAX32_SPI_CTRL2_DATA_WIDTH        GENMASK(13, 12) /**< CTRL2_DATA_WIDTH Mask */
#define MAX32_S_SPI_CTRL2_DATA_WIDTH_MONO 0x00            /**< CTRL2_DATA_WIDTH_MONO Setting */
#define MAX32_S_SPI_CTRL2_DATA_WIDTH_DUAL BIT(1)          /**< CTRL2_DATA_WIDTH_DUAL Setting */
#define MAX32_S_SPI_CTRL2_DATA_WIDTH_QUAD BIT(2)          /**< CTRL2_DATA_WIDTH_QUAD Setting */

#define MAX32_SPI_CTRL2_THREE_WIRE BIT(15) /**< CTRL2_THREE_WIRE Mask */

#define MAX32_SPI_CTRL2_SS_POL_POS     16 /**< CTRL2_SS_POL Position */
#define MAX32_SPI_CTRL2_THREE_WIRE_POS 15
/**@} end of group SPI_CTRL2_Register */

/**
 * @ingroup  spi_registers
 * @defgroup SPI_SSTIME SPI_SSTIME
 * @brief    Register for controlling SPI peripheral/Slave Select Timing.
 * @{
 */
#define MAX32_SPI_SSTIME_PRE       GENMASK(7, 0)   /**< SSTIME_PRE Mask */
#define MAX32_SPI_SSTIME_POST      GENMASK(15, 8)  /**< SSTIME_POST Mask */
#define MAX32_SPI_SSTIME_INACT     GENMASK(23, 16) /**< SSTIME_INACT Mask */
#define MAX32_SPI_SSTIME_PRE_POS   0
#define MAX32_SPI_SSTIME_POST_POS  8
#define MAX32_SPI_SSTIME_INACT_POS 16
/**@} end of group SPI_SSTIME_Register */

/**
 * @ingroup  spi_registers
 * @defgroup SPI_CLKCTRL SPI_CLKCTRL
 * @brief    Register for controlling SPI clock rate.
 * @{
 */
#define MAX32_SPI_CLKCTRL_LO     GENMASK(7, 0)   /**< CLKCTRL_LO Mask */
#define MAX32_SPI_CLKCTRL_HI     GENMASK(15, 8)  /**< CLKCTRL_HI Mask */
#define MAX32_SPI_CLKCTRL_CLKDIV GENMASK(19, 16) /**< CLKCTRL_CLKDIV Mask */

#define MAX32_SPI_CLKCTRL_LO_POS     0
#define MAX32_SPI_CLKCTRL_HI_POS     8
#define MAX32_SPI_CLKCTRL_CLKDIV_POS 16

/**@} end of group SPI_CLKCTRL_Register */

/**
 * @ingroup  spi_registers
 * @defgroup SPI_DMA SPI_DMA
 * @brief    Register for controlling DMA.
 * @{
 */
#define MAX32_REVA_DMA_TX_THD_VAL_POS     0               /**< DMA_TX_THD_VAL Position */
#define MAX32_REVA_DMA_TX_THD_VAL         BIT(0)          /**< DMA_TX_THD_VAL Mask */
#define MAX32_SPI_DMA_TX_FIFO_EN          BIT(6)          /**< DMA_TX_FIFO_EN Mask */
#define MAX32_SPI_DMA_TX_FLUSH            BIT(7)          /**< DMA_TX_FLUSH Mask */
#define MAX32_SPI_DMA_TX_LVL_POS          8               /**< DMA_TX_LVL Position */
#define MAX32_SPI_DMA_TX_LVL              GENMASK(13, 8)  /**< DMA_TX_LVL Mask */
#define MXC_F_SPI_REVA_DMA_RX_THD_VAL_POS 16              /**< DMA_RX_THD_VAL Position */
#define MXC_F_SPI_REVA_DMA_RX_THD_VAL     BIT(16)         /**< DMA_RX_THD_VAL Mask */
#define MAX32_SPI_DMA_RX_FIFO_EN          BIT(22)         /**< DMA_RX_FIFO_EN Mask */
#define MAX32_SPI_DMA_RX_FLUSH            BIT(23)         /**< DMA_RX_FLUSH Mask */
#define MAX32_SPI_DMA_RX_LVL              GENMASK(25, 24) /**< DMA_RX_LVL Mask */
#define MAX32_SPI_DMA_RX_LVL_POS          24

/**@} end of group SPI_DMA_Register */

/**
 * @ingroup  spi_registers
 * @defgroup SPI_INTFL SPI_INTFL
 * @brief    Register for reading and clearing interrupt flags. All bits are write 1 to
 *           clear.
 * @{
 */
#define MAX32_SPI_INTFL_MST_DONE BIT(11) /**< INTFL_MST_DONE Mask */

/**@} end of group SPI_INTFL_Register */

/**
 * @ingroup  spi_registers
 * @defgroup SPI_INTEN SPI_INTEN
 * @brief    Register for enabling interrupts.
 * @{
 */
#define MAX32_SPI_INTEN_TX_THD BIT(0) /**< INTEN_TX_THD Mask */
#define MAX32_SPI_INTEN_RX_THD BIT(2) /**< INTEN_RX_THD Mask */

/**@} end of group SPI_INTEN_Register */

/**
 * @ingroup  spi_registers
 * @defgroup SPI_STAT SPI_STAT
 * @brief    SPI Status register.
 * @{
 */
#define MAX32_SPI_STAT_BUSY BIT(0) /**< STAT_BUSY Mask */

/**@} end of group SPI_STAT_Register */

#define MAX32_SPI_FIFO_DEPTH 32

struct max32_spi_regs {
	union {
		uint32_t fifo32;    /**< <tt>\b 0x00:</tt> SPI FIFO32 Register */
		uint16_t fifo16[2]; /**< <tt>\b 0x00:</tt> SPI FIFO16 Register */
		uint8_t fifo8[4];   /**< <tt>\b 0x00:</tt> SPI FIFO8 Register */
	};
	uint32_t ctrl0;   /**< <tt>\b 0x04:</tt> SPI CTRL0 Register */
	uint32_t ctrl1;   /**< <tt>\b 0x08:</tt> SPI CTRL1 Register */
	uint32_t ctrl2;   /**< <tt>\b 0x0C:</tt> SPI CTRL2 Register */
	uint32_t sstime;  /**< <tt>\b 0x10:</tt> SPI SSTIME Register */
	uint32_t clkctrl; /**< <tt>\b 0x14:</tt> SPI CLKCTRL Register */
	uint32_t rsv_0x18;
	uint32_t dma;   /**< <tt>\b 0x1C:</tt> SPI DMA Register */
	uint32_t intfl; /**< <tt>\b 0x20:</tt> SPI INTFL Register */
	uint32_t inten; /**< <tt>\b 0x24:</tt> SPI INTEN Register */
	uint32_t wkfl;  /**< <tt>\b 0x28:</tt> SPI WKFL Register */
	uint32_t wken;  /**< <tt>\b 0x2C:</tt> SPI WKEN Register */
	uint32_t stat;  /**< <tt>\b 0x30:</tt> SPI STAT Register */
};

struct max32_spi_req_t {
	int ssIdx;
	int ssDeassert;
	uint8_t *txData;
	uint8_t *rxData;
	uint32_t txLen;
	uint32_t rxLen;
	uint32_t txCnt;
	uint32_t rxCnt;
};

struct max32_spi_config {
	volatile struct max32_spi_regs *spi;
	const struct pinctrl_dev_config *pctrl;
	const struct device *clock;
	uint32_t clock_bus;
	uint32_t clock_bit;
};

struct max32_spi_data {
	struct max32_spi_req_t req;
};

#define MAX32_SETFIELD(reg, mask, setting) (reg = (reg & ~mask) | (setting & mask))

#define DT_GCR_CLOCK_SOURCE DT_CLOCKS_CTLR(DT_NODELABEL(gcr))

typedef struct {
	struct max32_spi_req_t *req;
	int started;
	unsigned last_size;
	unsigned ssDeassert;
	unsigned defaultTXData;
	int channelTx;
	int channelRx;
	bool txrx_req;
	uint8_t req_done;
} spi_req_state_t;

/* states whether to use call back or not */
uint8_t async;

static spi_req_state_t states[3];
typedef enum {
	SPI_WIDTH_3WIRE,
	SPI_WIDTH_STANDARD,
	SPI_WIDTH_DUAL,
	SPI_WIDTH_QUAD,
} max32_spi_width_t;

static int spi_max32_set_frequency(const struct device *dev, unsigned int hz)
{
	const struct max32_spi_config *const cfg = dev->config;
	volatile struct max32_spi_regs *spi = cfg->spi;
	int hi_clk, lo_clk, scale;
	uint32_t freq_div;

#if DT_SAME_NODE(DT_GCR_CLOCK_SOURCE, DT_NODELABEL(clk_lpo))
	freq_div = DT_PROP(DT_NODELABEL(clk_lpo), clock_frequency);
#elif DT_SAME_NODE(DT_GCR_CLOCK_SOURCE, DT_NODELABEL(clk_hso))
	freq_div = DT_PROP(DT_NODELABEL(clk_hso), clock_frequency);
#else DT_SAME_NODE(DT_GCR_CLOCK_SOURCE, DT_NODELABEL(clk_obrc))
	freq_div = DT_PROP(DT_NODELABEL(clk_obrc), clock_frequency);
#endif
	freq_div = (freq_div / hz);

	hi_clk = freq_div / 2;
	lo_clk = freq_div / 2;
	scale = 0;

	if (freq_div % 2) {
		hi_clk += 1;
	}

	while (hi_clk >= 16 && scale < 8) {
		hi_clk /= 2;
		lo_clk /= 2;
		scale++;
	}

	if (scale == 8) {
		lo_clk = 15;
		hi_clk = 15;
	}

	MAX32_SETFIELD(spi->clkctrl, MAX32_SPI_CLKCTRL_LO, (lo_clk << MAX32_SPI_CLKCTRL_LO_POS));
	MAX32_SETFIELD(spi->clkctrl, MAX32_SPI_CLKCTRL_HI, (hi_clk << MAX32_SPI_CLKCTRL_HI_POS));
	MAX32_SETFIELD(spi->clkctrl, MAX32_SPI_CLKCTRL_CLKDIV,
		       (scale << MAX32_SPI_CLKCTRL_CLKDIV_POS));

	return 0;
}

static int spi_max32_set_slave(const struct device *dev, int ssIdx)
{
	volatile struct max32_spi_regs *spi = dev->data;
	if (!(spi->ctrl0 & MAX32_SPI_CTRL0_MST_MODE)) {
		return -ENXIO;
	}
	/* Setup the slave select */
	/* Activate chosen SS pin */
	spi->ctrl0 |= (1 << ssIdx) << MAX32_SPI_CTRL0_SS_ACTIVE_POS;
	/* Deactivate all unchosen pins */
	spi->ctrl0 &= ~MAX32_SPI_CTRL0_SS_ACTIVE | ((1 << ssIdx) << MAX32_SPI_CTRL0_SS_ACTIVE_POS);
	return 0;
}

static max32_spi_width_t spi_max32_get_width(const struct device *dev)
{
	const struct max32_spi_config *const cfg = dev->config;
	volatile struct max32_spi_regs *spi = cfg->spi;

	if (spi->ctrl2 & MAX32_SPI_CTRL2_THREE_WIRE) {
		return SPI_WIDTH_3WIRE;
	}

	if (spi->ctrl2 & MAX32_S_SPI_CTRL2_DATA_WIDTH_DUAL) {
		return SPI_WIDTH_DUAL;
	}

	if (spi->ctrl2 & MAX32_S_SPI_CTRL2_DATA_WIDTH_QUAD) {
		return SPI_WIDTH_QUAD;
	}

	return SPI_WIDTH_STANDARD;
}

static int spi_max32_get_datasize(const struct device *dev)
{
	const struct max32_spi_config *const cfg = dev->config;
	volatile struct max32_spi_regs *spi = cfg->spi;

	if (!(spi->ctrl2 & MAX32_SPI_CTRL2_NUMBITS)) {
		return 16;
	}

	return (spi->ctrl2 & MAX32_SPI_CTRL2_NUMBITS) >> MAX32_SPI_CTRL2_NUMBITS_POS;
}

static int spi_max32_trans_setup(const struct device *dev)
{
	const struct max32_spi_config *const cfg = dev->config;
	volatile struct max32_spi_regs *spi = cfg->spi;
	struct max32_spi_req_t *req = dev->data;
	int spi_num;
	uint8_t bits;

	if ((!req) || ((req->txData == NULL) && (req->rxData == NULL))) {
		return -ENXIO;
	}

	/* Setup the number of characters to transact */
	if (req->txLen > (MAX32_SPI_CTRL1_TX_NUM_CHAR >> MAX32_SPI_CTRL1_TX_NUM_CHAR_POS)) {
		return -ENXIO;
	}

	spi_num = 0;
	bits = spi_max32_get_datasize(dev);
	req->txCnt = 0;
	req->rxCnt = 0;

	states[spi_num].req = req;
	states[spi_num].started = 0;
	states[spi_num].req_done = 0;

	/* HW requires disabling/renabling SPI block at end of each transaction (when SS is
	 inactive). */
	if (states[spi_num].ssDeassert == 1) {
		spi->ctrl0 &= ~(MAX32_SPI_CTRL0_EN);
	}

	/* if master */
	if (spi->ctrl0 & MAX32_SPI_CTRL0_MST_MODE) {
		/* Setup the slave select */
		spi_max32_set_slave(dev, req->ssIdx);
	}

	if (req->rxData != NULL && req->rxLen > 0) {
		MAX32_SETFIELD(spi->ctrl1, MAX32_SPI_CTRL1_RX_NUM_CHAR,
			       req->rxLen << MAX32_SPI_CTRL1_RX_NUM_CHAR_POS);
		spi->dma |= MAX32_SPI_DMA_RX_FIFO_EN;
	} else {
		spi->ctrl1 &= ~(MAX32_SPI_CTRL1_RX_NUM_CHAR);
		spi->dma &= ~(MAX32_SPI_DMA_RX_FIFO_EN);
	}

	/* Must use TXFIFO and NUM in full duplex//start editing here */
	if ((max32_spi_width_t)spi_max32_get_width(dev) == SPI_WIDTH_STANDARD &&
	    !((spi->ctrl2 & MAX32_SPI_CTRL2_THREE_WIRE) >> MAX32_SPI_CTRL2_THREE_WIRE_POS)) {
		if (req->txData == NULL) {
			/* Must have something to send, so we'll use the rx_data buffer initialized
			 to 0. */
			memset(req->rxData, states[spi_num].defaultTXData,
			       (bits > 8 ? req->rxLen << 1 : req->rxLen));
			req->txData = req->rxData;
			req->txLen = req->rxLen;
		}
	}

	if (req->txData != NULL && req->txLen > 0) {
		MAX32_SETFIELD(spi->ctrl1, MAX32_SPI_CTRL1_TX_NUM_CHAR,
			       req->txLen << MAX32_SPI_CTRL1_TX_NUM_CHAR_POS);
		spi->dma |= MAX32_SPI_DMA_TX_FIFO_EN;
	} else {
		spi->ctrl1 &= ~(MAX32_SPI_CTRL1_TX_NUM_CHAR);
		spi->dma &= ~(MAX32_SPI_DMA_TX_FIFO_EN);
	}

	if ((req->txData != NULL && req->txLen) && (req->rxData != NULL && req->rxLen)) {
		states[spi_num].txrx_req = true;
	} else {
		states[spi_num].txrx_req = false;
	}

	spi->dma |= (MAX32_SPI_DMA_TX_FLUSH | MAX32_SPI_DMA_RX_FLUSH);
	spi->ctrl0 |= (MAX32_SPI_CTRL0_EN);

	states[spi_num].ssDeassert = req->ssDeassert;
	/* Clear master done flag */
	spi->intfl = MAX32_SPI_INTFL_MST_DONE;

	return 0;
}

static unsigned int spi_max32_get_rx_fifo_available(const struct device *dev)
{
	const struct max32_spi_config *const cfg = dev->config;
	volatile struct max32_spi_regs *spi = cfg->spi;

	return (spi->dma & MAX32_SPI_DMA_RX_LVL) >> MAX32_SPI_DMA_RX_LVL_POS;
}

static unsigned int spi_max32_get_tx_fifo_available(const struct device *dev)
{
	const struct max32_spi_config *const cfg = dev->config;
	volatile struct max32_spi_regs *spi = cfg->spi;

	return MAX32_SPI_FIFO_DEPTH -
	       ((spi->dma & MAX32_SPI_DMA_TX_LVL) >> MAX32_SPI_DMA_TX_LVL_POS);
}

static unsigned int spi_max32_write_tx_fifo(const struct device *dev, unsigned char *bytes,
					    unsigned int len)
{
	const struct max32_spi_config *const cfg = dev->config;
	volatile struct max32_spi_regs *spi = cfg->spi;

	unsigned tx_avail, bits;

	if (!bytes || !len) {
		return 0;
	}

	tx_avail = spi_max32_get_tx_fifo_available(dev);
	bits = spi_max32_get_datasize(dev);

	if (len > tx_avail) {
		len = tx_avail;
	}

	if (bits > 8) {
		len &= ~(unsigned)0x1;
	}

	unsigned cnt = 0;

	while (len) {
		if (len > 3) {
			memcpy((void *)&spi->fifo32, &((uint8_t *)bytes)[cnt], 4);

			len -= 4;
			cnt += 4;

		} else if (len > 1) {
			memcpy((void *)&spi->fifo16[0], &((uint8_t *)bytes)[cnt], 2);

			len -= 2;
			cnt += 2;

		} else if (bits <= 8) {
			spi->fifo8[0] = ((uint8_t *)bytes)[cnt++];
			len--;
		}
	}

	return cnt;
}

static unsigned int spi_max32_read_rx_fifo(const struct device *dev, unsigned char *bytes,
					   unsigned int len)
{
	const struct max32_spi_config *const cfg = dev->config;
	volatile struct max32_spi_regs *spi = cfg->spi;

	unsigned rx_avail, bits;

	if (!bytes || !len) {
		return 0;
	}

	rx_avail = spi_max32_get_rx_fifo_available(dev);
	bits = spi_max32_get_datasize(dev);

	if (len > rx_avail) {
		len = rx_avail;
	}

	if (bits > 8) {
		len &= ~(unsigned)0x1;
	}

	unsigned cnt = 0;

	if (bits <= 8 || len >= 2) {
		/* Read from the FIFO */
		while (len) {
			if (len > 3) {
				memcpy(&((uint8_t *)bytes)[cnt], (void *)&spi->fifo32, 4);
				len -= 4;
				cnt += 4;
			} else if (len > 1) {
				memcpy(&((uint8_t *)bytes)[cnt], (void *)&spi->fifo16[0], 2);
				len -= 2;
				cnt += 2;

			} else {
				((uint8_t *)bytes)[cnt++] = spi->fifo8[0];
				len -= 1;
			}

			/* Don't read less than 2 bytes if we are using greater than 8 bit
			   characters */
			if (len == 1 && bits > 8) {
				break;
			}
		}
	}

	return cnt;
}

static int spi_max32_set_rx_threshold(const struct device *dev, unsigned int numBytes)
{
	const struct max32_spi_config *const cfg = dev->config;
	volatile struct max32_spi_regs *spi = cfg->spi;

	if (numBytes > 32) {
		return -ENXIO;
	}

	MAX32_SETFIELD(spi->dma, MAX32_REVA_DMA_TX_THD_VAL,
		       numBytes << MAX32_REVA_DMA_TX_THD_VAL_POS);

	return 0;
}

static int spi_max32_set_tx_threshold(const struct device *dev, unsigned int numBytes)
{
	const struct max32_spi_config *const cfg = dev->config;
	volatile struct max32_spi_regs *spi = cfg->spi;

	if (numBytes > 32) {
		return -ENXIO;
	}

	MAX32_SETFIELD(spi->dma, MAX32_REVA_DMA_TX_THD_VAL,
		       numBytes << MAX32_REVA_DMA_TX_THD_VAL_POS);

	return 0;
}

static uint32_t spi_max32_trans_handler(const struct device *dev)
{
	const struct max32_spi_config *const cfg = dev->config;
	volatile struct max32_spi_regs *spi = cfg->spi;
	volatile struct max32_spi_req_t *req = dev->data;
	int remain, spi_num;
	uint32_t int_en = 0;
	uint32_t tx_length = 0, rx_length = 0;
	uint8_t bits;

	spi_num = 0;

	bits = spi_max32_get_datasize(dev);

	/* MAX32_SPI_CTRL2_NUMBITS data bits
	   Read/write 2x number of bytes if larger character size */
	if (bits > 8) {
		tx_length = req->txLen * 2;
		rx_length = req->rxLen * 2;
	} else {
		tx_length = req->txLen;
		rx_length = req->rxLen;
	}

	if (req->txData != NULL) {

		req->txCnt += spi_max32_write_tx_fifo(dev, &(req->txData[req->txCnt]),
						      tx_length - req->txCnt);
	}

	remain = tx_length - req->txCnt;

	/* Write the FIFO */
	if (remain) {
		if (remain > MAX32_SPI_FIFO_DEPTH) {
			spi_max32_set_tx_threshold(dev, MAX32_SPI_FIFO_DEPTH);
		} else {
			spi_max32_set_tx_threshold(dev, remain);
		}

		int_en |= MAX32_SPI_INTEN_TX_THD;
	}
	/* Break out if we've transmitted all the bytes and not receiving */
	if ((req->rxData == NULL) && (req->txCnt == tx_length)) {
		spi->inten = 0;
		int_en = 0;
	}

	/* Read the RX FIFO */
	if (req->rxData != NULL) {
		req->rxCnt += spi_max32_read_rx_fifo(dev, &(req->rxData[req->rxCnt]),
						     rx_length - req->rxCnt);

		remain = rx_length - req->rxCnt;

		if (remain) {
			if (remain > MAX32_SPI_FIFO_DEPTH) {
				spi_max32_set_rx_threshold(dev, 2);
			} else {
				spi_max32_set_rx_threshold(dev, remain - 1);
			}

			int_en |= MAX32_SPI_INTEN_RX_THD;
		}

		/* Break out if we've received all the bytes and we're not transmitting */
		if ((req->txData == NULL) && (req->rxCnt == rx_length)) {
			spi->inten = 0;
			int_en = 0;
		}
	}
	/* Break out once we've transmitted and received all of the data */
	if ((req->rxCnt == rx_length) && (req->txCnt == tx_length)) {
		spi->inten = 0;
		int_en = 0;
	}

	return int_en;
}

static int spi_max32_get_active(const struct device *dev)
{
	const struct max32_spi_config *const cfg = dev->config;
	volatile struct max32_spi_regs *spi = cfg->spi;

	if (spi->stat & MAX32_SPI_STAT_BUSY) {
		return -ENXIO;
	}

	return 0;
}

static int spi_max32_start_transmission(const struct device *dev)
{
	const struct max32_spi_config *const cfg = dev->config;
	volatile struct max32_spi_regs *spi = cfg->spi;

	if (spi_max32_get_active(dev)) {
		return -ENXIO;
	}

	spi->ctrl0 |= MAX32_SPI_CTRL0_START;

	return 0;
}

static uint32_t spi_max32_master_trans_handler(const struct device *dev)
{
	const struct max32_spi_config *const cfg = dev->config;
	volatile struct max32_spi_regs *spi = cfg->spi;
	volatile struct max32_spi_req_t *req = dev->data;
	uint32_t retval;
	int spi_num = 0;

	/* Leave slave select asserted at the end of the transaction */
	if (!req->ssDeassert) {
		spi->ctrl0 |= MAX32_SPI_CTRL0_SS_CTRL;
	}

	retval = spi_max32_trans_handler(dev);

	if (!states[spi_num].started) {
		spi_max32_start_transmission(dev);
		states[spi_num].started = 1;
	}

	/* Deassert slave select at the end of the transaction */
	if (req->ssDeassert) {
		spi->ctrl0 &= ~MAX32_SPI_CTRL0_SS_CTRL;
	}

	return retval;
}

static int spi_max32_master_transaction(const struct device *dev)
{
	int error;
	const struct max32_spi_config *const cfg = dev->config;
	volatile struct max32_spi_regs *spi = cfg->spi;

	if ((error = spi_max32_trans_setup(dev)) != 0) {
		return error;
	}

	async = 0;

	/* call master transHandler */
	while (spi_max32_master_trans_handler(dev) != 0) {
		;
	}

	while (!(spi->intfl & MAX32_SPI_INTFL_MST_DONE)) {
		;
	}

	return 0;
}

/* API implementation: init */
static int spi_max32_init(const struct device *dev)
{
	const struct max32_spi_config *const cfg = dev->config;
	volatile struct max32_spi_regs *spi = cfg->spi;
	uint32_t clkcfg;
	int ret;

	if (!device_is_ready(cfg->clock)) {
		return -ENODEV;
	}

	/* enable clock */
	clkcfg = (cfg->clock_bus << 16) | cfg->clock_bit;

	ret = clock_control_on(cfg->clock, (clock_control_subsys_t)clkcfg);
	if (ret) {
		return ret;
	}

	ret = pinctrl_apply_state(cfg->pctrl, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}

	spi->ctrl0 = (MAX32_SPI_CTRL0_EN);

	spi->sstime = ((0x1 << MAX32_SPI_SSTIME_PRE_POS) | (0x1 << MAX32_SPI_SSTIME_POST_POS) |
		       (0x1 << MAX32_SPI_SSTIME_INACT_POS));

	spi->ctrl0 |= MAX32_SPI_CTRL0_MST_MODE;

	/* Set frequency */
	spi_max32_set_frequency(dev, 48000);

	spi->ctrl2 |= (0 << MAX32_SPI_CTRL2_SS_POL_POS); // polarity

	/* Clear the interrupts */
	spi->intfl = spi->intfl;

	/* set for one slave */
	spi->ctrl0 |= MAX32_S_SPI_CTRL0_SS_ACTIVE_SS0;

	return 0;
}

/* API implementation: transceive */
static int spi_max32_transceive(const struct device *dev, const struct spi_config *config,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs)
{
	struct max32_spi_req_t *req = dev->data;

	req->txData = (uint8_t *)tx_bufs->buffers->buf;
	req->rxData = (uint8_t *)rx_bufs->buffers->buf;
	req->txLen = tx_bufs->buffers->len;
	req->rxLen = rx_bufs->buffers->len;
	req->ssIdx = 0;
	req->ssDeassert = 1;
	req->txCnt = tx_bufs->count;
	req->rxCnt = rx_bufs->count;

	if (spi_max32_master_transaction(dev)) {
		return -EIO;
	}

	return 0;
}

/* API implementation: release */
static int spi_max32_release(const struct device *dev, const struct spi_config *config)
{
	return 0;
}

/* SPI driver APIs structure */
static struct spi_driver_api spi_max32_api = {
	.transceive = spi_max32_transceive,
	.release = spi_max32_release,
};

/* SPI driver registration */
#define SPI_MAX32_INIT(_num)                                                                       \
	PINCTRL_DT_INST_DEFINE(_num);                                                              \
	static const struct max32_spi_config max32_spi_config_##_num = {                           \
		.spi = (void *)DT_INST_REG_ADDR(_num),                                             \
		.pctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(_num),                                     \
		.clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(_num)),                                 \
		.clock_bus = DT_INST_CLOCKS_CELL(_num, offset),                                    \
		.clock_bit = DT_INST_CLOCKS_CELL(_num, bit),                                       \
	};                                                                                         \
	static struct max32_spi_data max32_spi_data_##num;                                         \
	DEVICE_DT_INST_DEFINE(_num, spi_max32_init, NULL, &max32_spi_data_##num,                   \
			      &max32_spi_config_##_num, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,     \
			      &spi_max32_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_MAX32_INIT)
