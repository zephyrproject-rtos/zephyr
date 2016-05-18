/*
 * Copyright (c) 2016, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL CORPORATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "qm_spi.h"

/* SPI FIFO size defaults */
#define SPI_DEFAULT_TX_THRESHOLD (0x05)
#define SPI_DEFAULT_RX_THRESHOLD (0x05)
#define SPI_FIFOS_DEPTH (8)

/* SPI DMA transmit watermark level. When the number of valid data entries in
 * the transmit FIFO is equal to or below this field value, dma_tx_req is
 * generated. The burst length has to fit in the remaining space of the transmit
 * FIFO, i.e. the burst length cannot be bigger than (16 - watermark level). */
#define SPI_DMATDLR_DMATDL (0x03)
#define SPI_DMA_WRITE_BURST_LENGTH QM_DMA_BURST_TRANS_LENGTH_4

/* SPI DMA receive watermark level. When the number of valid data entries in the
 * receive FIFO is equal to or above this field value + 1, dma_rx_req is
 * generated. The burst length has to match the watermark level so that the
 * exact number of data entries fit one burst, and therefore only some values
 * are allowed:
 * DMARDL	DMA read burst length
 *   0			1
 *   3			4
 *   7 (highest)	8
 */
#define SPI_DMARDLR_DMARDL (0x03)
#define SPI_DMA_READ_BURST_LENGTH QM_DMA_BURST_TRANS_LENGTH_4

/* Arbitrary byte sent in RX-only mode. */
#define SPI_RX_ONLY_DUMMY_BYTE (0xf0)

/* DMA transfer information, relevant on callback invocations from the DMA
 * driver. */
typedef struct {
	qm_spi_t spi_id;		    /**< SPI controller identifier. */
	qm_dma_channel_id_t dma_channel_id; /**< Used DMA channel. */
	volatile bool cb_pending; /**< True if waiting for DMA calllback. */
} dma_context_t;

/**
 * Extern qm_spi_reg_t* array declared at qm_soc_regs.h .
 */
#ifndef UNIT_TEST
#if (QUARK_SE)
qm_spi_reg_t *qm_spi_controllers[QM_SPI_NUM] = {
    (qm_spi_reg_t *)QM_SPI_MST_0_BASE, (qm_spi_reg_t *)QM_SPI_MST_1_BASE};
#elif(QUARK_D2000)
qm_spi_reg_t *qm_spi_controllers[QM_SPI_NUM] = {
    (qm_spi_reg_t *)QM_SPI_MST_0_BASE};
#endif
#endif

static const qm_spi_async_transfer_t *spi_async_transfer[QM_SPI_NUM];
static volatile uint16_t tx_counter[QM_SPI_NUM], rx_counter[QM_SPI_NUM];
static uint8_t dfs[QM_SPI_NUM];
static const uint32_t tx_dummy_frame = SPI_RX_ONLY_DUMMY_BYTE;
static qm_spi_tmode_t tmode[QM_SPI_NUM];
/* DMA (memory to SPI controller) callback information. */
static dma_context_t dma_context_tx[QM_SPI_NUM];
/* DMA (SPI controller to memory) callback information. */
static dma_context_t dma_context_rx[QM_SPI_NUM];
/* DMA core being used by each SPI controller. */
static qm_dma_t dma_core[QM_SPI_NUM];

static void read_frame(const qm_spi_t spi, uint8_t *const rx_buffer)
{
	const qm_spi_reg_t *const controller = QM_SPI[spi];
	const uint8_t frame_size = dfs[spi];

	if (frame_size == 1) {
		*(uint8_t *)rx_buffer = controller->dr[0];
	} else if (frame_size == 2) {
		*(uint16_t *)rx_buffer = controller->dr[0];
	} else {
		*(uint32_t *)rx_buffer = controller->dr[0];
	}
}

static void write_frame(const qm_spi_t spi, const uint8_t *const tx_buffer)
{
	qm_spi_reg_t *const controller = QM_SPI[spi];
	const uint8_t frame_size = dfs[spi];

	if (frame_size == 1) {
		controller->dr[0] = *(uint8_t *)tx_buffer;
	} else if (frame_size == 2) {
		controller->dr[0] = *(uint16_t *)tx_buffer;
	} else {
		controller->dr[0] = *(uint32_t *)tx_buffer;
	}
}

static void wait_for_controller(const qm_spi_reg_t *const controller)
{
	/* Page 42 of databook says you must poll TFE status waiting for 1
	 * before checking QM_SPI_SR_BUSY.
	 */
	while (!(controller->sr & QM_SPI_SR_TFE))
		;
	while (controller->sr & QM_SPI_SR_BUSY)
		;
}

/**
 * Service an RX FIFO Full interrupt
 *
 * @brief Interrupt based transfer on SPI.
 * @param [in] spi Which SPI to transfer from.
 */
static __inline__ void handle_rx_interrupt(const qm_spi_t spi)
{
	qm_spi_reg_t *const controller = QM_SPI[spi];
	const qm_spi_async_transfer_t *const transfer = spi_async_transfer[spi];

	/* Jump to the right position of RX buffer.
	 * If no bytes were received before, we start from the beginning,
	 * otherwise we jump to the next available frame position.
	 */
	uint8_t *rx_buffer = transfer->rx + (rx_counter[spi] * dfs[spi]);

	while (controller->rxflr) {
		read_frame(spi, rx_buffer);
		rx_buffer += dfs[spi];
		rx_counter[spi]++;

		/* Check that there's not more data in the FIFO than we had
		 * requested.
		 */
		if (transfer->rx_len == rx_counter[spi]) {
			controller->imr &=
			    ~(QM_SPI_IMR_RXUIM | QM_SPI_IMR_RXOIM |
			      QM_SPI_IMR_RXFIM);
			if (transfer->callback &&
			    tmode[spi] == QM_SPI_TMOD_RX) {
				transfer->callback(transfer->callback_data, 0,
						   QM_SPI_IDLE,
						   transfer->rx_len);
			}
			break;
		}
	}

	/* Check if enough data will arrive to trigger an interrupt and adjust
	 * rxftlr accordingly.
	 */
	const uint32_t frames_left = transfer->rx_len - rx_counter[spi];
	if (frames_left <= controller->rxftlr) {
		controller->rxftlr = frames_left - 1;
	}
}

/**
 * Service an Tx FIFO Empty interrupt
 *
 * @brief Interrupt based transfer on SPI.
 * @param [in] spi Which SPI to transfer to.
 */
static __inline__ void handle_tx_interrupt(const qm_spi_t spi)
{
	qm_spi_reg_t *const controller = QM_SPI[spi];
	const qm_spi_async_transfer_t *const transfer = spi_async_transfer[spi];

	/* Jump to the right position of TX buffer.
	 * If no bytes were transmitted before, we start from the beginning,
	 * otherwise we jump to the next frame to be sent.
	 */
	const uint8_t *tx_buffer = transfer->tx + (tx_counter[spi] * dfs[spi]);

	int frames =
	    SPI_FIFOS_DEPTH - controller->txflr - controller->rxflr - 1;

	while (frames > 0) {
		write_frame(spi, tx_buffer);
		tx_buffer += dfs[spi];
		tx_counter[spi]++;
		frames--;

		if (transfer->tx_len == tx_counter[spi]) {
			controller->txftlr = 0;
			break;
		}
	}
}

static void handle_spi_interrupt(const qm_spi_t spi)
{
	qm_spi_reg_t *const controller = QM_SPI[spi];
	const qm_spi_async_transfer_t *transfer = spi_async_transfer[spi];
	const uint32_t int_status = controller->isr;

	QM_ASSERT((int_status & (QM_SPI_ISR_TXOIS | QM_SPI_ISR_RXUIS)) == 0);
	if (int_status & QM_SPI_ISR_RXOIS) {
		if (transfer->callback) {
			transfer->callback(transfer->callback_data, -EIO,
					   QM_SPI_RX_OVERFLOW,
					   rx_counter[spi]);
		}

		controller->rxoicr;
		controller->imr = QM_SPI_IMR_MASK_ALL;
		controller->ssienr = 0;
		return;
	}

	if (int_status & QM_SPI_ISR_RXFIS) {
		handle_rx_interrupt(spi);
	}

	if (transfer->rx_len == rx_counter[spi] &&
	    transfer->tx_len == tx_counter[spi] &&
	    (controller->sr & QM_SPI_SR_TFE) &&
	    !(controller->sr & QM_SPI_SR_BUSY)) {
		controller->imr = QM_SPI_IMR_MASK_ALL;
		controller->ssienr = 0;

		if (transfer->callback && tmode[spi] != QM_SPI_TMOD_RX) {
			transfer->callback(transfer->callback_data, 0,
					   QM_SPI_IDLE, transfer->tx_len);
		}

		return;
	}

	if (int_status & QM_SPI_ISR_TXEIS &&
	    transfer->tx_len > tx_counter[spi]) {
		handle_tx_interrupt(spi);
	}
}

int qm_spi_set_config(const qm_spi_t spi, const qm_spi_config_t *cfg)
{
	QM_CHECK(spi < QM_SPI_NUM, -EINVAL);
	QM_CHECK(cfg, -EINVAL);

	QM_ASSERT(QM_SPI[spi]->ssienr == 0);

	qm_spi_reg_t *const controller = QM_SPI[spi];

	/* Apply the selected cfg options */
	controller->ctrlr0 = (cfg->frame_size << QM_SPI_CTRLR0_DFS_32_OFFSET) |
			     (cfg->transfer_mode << QM_SPI_CTRLR0_TMOD_OFFSET) |
			     (cfg->bus_mode << QM_SPI_CTRLR0_SCPOL_SCPH_OFFSET);

	controller->baudr = cfg->clk_divider;

	/* Keep the current data frame size in bytes, being:
	 * - 1 byte for DFS set from 4 to 8 bits;
	 * - 2 bytes for DFS set from 9 to 16 bits;
	 * - 3 bytes for DFS set from 17 to 24 bits;
	 * - 4 bytes for DFS set from 25 to 32 bits.
	 */
	dfs[spi] = (cfg->frame_size / 8) + 1;

	tmode[spi] = cfg->transfer_mode;

	return 0;
}

int qm_spi_slave_select(const qm_spi_t spi, const qm_spi_slave_select_t ss)
{
	QM_CHECK(spi < QM_SPI_NUM, -EINVAL);

	/* Check if the device reports as busy. */
	QM_ASSERT(!(QM_SPI[spi]->sr & QM_SPI_SR_BUSY));

	QM_SPI[spi]->ser = ss;

	return 0;
}

int qm_spi_get_status(const qm_spi_t spi, qm_spi_status_t *const status)
{
	QM_CHECK(spi < QM_SPI_NUM, -EINVAL);
	QM_CHECK(status, -EINVAL);

	qm_spi_reg_t *const controller = QM_SPI[spi];

	if (controller->sr & QM_SPI_SR_BUSY) {
		*status = QM_SPI_BUSY;
	} else {
		*status = QM_SPI_IDLE;
	}

	if (controller->risr & QM_SPI_RISR_RXOIR) {
		*status = QM_SPI_RX_OVERFLOW;
	}

	return 0;
}

int qm_spi_transfer(const qm_spi_t spi, const qm_spi_transfer_t *const xfer,
		    qm_spi_status_t *const status)
{
	QM_CHECK(spi < QM_SPI_NUM, -EINVAL);
	QM_CHECK(xfer, -EINVAL);
	QM_CHECK(tmode[spi] == QM_SPI_TMOD_TX_RX
		     ? (xfer->tx_len == xfer->rx_len)
		     : 1,
		 -EINVAL);
	QM_CHECK(tmode[spi] == QM_SPI_TMOD_TX ? (xfer->rx_len == 0) : 1,
		 -EINVAL);
	QM_CHECK(tmode[spi] == QM_SPI_TMOD_RX ? (xfer->tx_len == 0) : 1,
		 -EINVAL);
	QM_CHECK(tmode[spi] == QM_SPI_TMOD_EEPROM_READ
		     ? (xfer->tx_len && xfer->rx_len)
		     : 1,
		 -EINVAL);

	uint32_t i_tx = xfer->tx_len;
	uint32_t i_rx = xfer->rx_len;
	int rc = 0;

	qm_spi_reg_t *const controller = QM_SPI[spi];

	/* Wait for the SPI device to become available. */
	wait_for_controller(controller);

	/* Mask all interrupts, this is a blocking function. */
	controller->imr = QM_SPI_IMR_MASK_ALL;

	/* If we are in RX only or EEPROM Read mode, the ctrlr1 reg holds how
	 * many bytes the controller solicits, minus 1. */
	if (xfer->rx_len) {
		controller->ctrlr1 = xfer->rx_len - 1;
	}

	/* Enable SPI device */
	controller->ssienr = QM_SPI_SSIENR_SSIENR;

	/* Transfer is only complete when all the tx data is sent and all
	 * expected rx data has been received.
	 */
	uint8_t *rx_buffer = xfer->rx;
	const uint8_t *tx_buffer = xfer->tx;

	int frames;

	/* RX Only transfers need a dummy byte to be sent for starting.
	 * This is covered by the databook on page 42.
	 */
	if (tmode[spi] == QM_SPI_TMOD_RX) {
		tx_buffer = (uint8_t *)&tx_dummy_frame;
		i_tx = 1;
	}

	while (i_tx || i_rx) {
		if (controller->risr & QM_SPI_RISR_RXOIR) {
			rc = -EIO;
			if (status) {
				*status |= QM_SPI_RX_OVERFLOW;
			}
			controller->rxoicr;
			break;
		}

		while (i_rx && controller->rxflr) {
			read_frame(spi, rx_buffer);
			rx_buffer += dfs[spi];
			i_rx--;
		}

		frames =
		    SPI_FIFOS_DEPTH - controller->txflr - controller->rxflr - 1;
		while (i_tx && frames) {
			write_frame(spi, tx_buffer);
			tx_buffer += dfs[spi];
			i_tx--;
			frames--;
		}
		/* Databook page 43 says we always need to busy-wait until the
		 * controller is ready again after writing frames to the TX
		 * FIFO.
		 *
		 * That is only needed for TX or TX_RX transfer modes.
		 */
		if (tmode[spi] == QM_SPI_TMOD_TX_RX ||
		    tmode[spi] == QM_SPI_TMOD_TX) {
			wait_for_controller(controller);
		}
	}

	controller->ssienr = 0; /** Disable SPI Device */

	return rc;
}

int qm_spi_irq_transfer(const qm_spi_t spi,
			const qm_spi_async_transfer_t *const xfer)
{
	QM_CHECK(spi < QM_SPI_NUM, -EINVAL);
	QM_CHECK(xfer, -EINVAL);
	QM_CHECK(tmode[spi] == QM_SPI_TMOD_TX_RX
		     ? (xfer->tx_len == xfer->rx_len)
		     : 1,
		 -EINVAL);
	QM_CHECK(tmode[spi] == QM_SPI_TMOD_TX ? (xfer->rx_len == 0) : 1,
		 -EINVAL);
	QM_CHECK(tmode[spi] == QM_SPI_TMOD_RX ? (xfer->tx_len == 0) : 1,
		 -EINVAL);
	QM_CHECK(tmode[spi] == QM_SPI_TMOD_EEPROM_READ
		     ? (xfer->tx_len && xfer->rx_len)
		     : 1,
		 -EINVAL);

	qm_spi_reg_t *const controller = QM_SPI[spi];

	/* If we are in RX only or EEPROM Read mode, the ctrlr1 reg holds how
	 * many bytes the controller solicits, minus 1. We also set the same
	 * into rxftlr, so the controller only triggers a RX_FIFO_FULL
	 * interrupt when all frames are available at the FIFO for consumption.
	 */
	if (xfer->rx_len) {
		controller->ctrlr1 = xfer->rx_len - 1;
		controller->rxftlr = (xfer->rx_len < SPI_FIFOS_DEPTH)
					 ? xfer->rx_len - 1
					 : SPI_DEFAULT_RX_THRESHOLD;
	}
	controller->txftlr = SPI_DEFAULT_TX_THRESHOLD;

	spi_async_transfer[spi] = xfer;
	tx_counter[spi] = 0;
	rx_counter[spi] = 0;

	/* Unmask interrupts */
	if (tmode[spi] == QM_SPI_TMOD_TX) {
		controller->imr = QM_SPI_IMR_TXEIM | QM_SPI_IMR_TXOIM;
	} else if (tmode[spi] == QM_SPI_TMOD_RX) {
		controller->imr =
		    QM_SPI_IMR_RXUIM | QM_SPI_IMR_RXOIM | QM_SPI_IMR_RXFIM;
		controller->ssienr = QM_SPI_SSIENR_SSIENR;
		write_frame(spi, (uint8_t *)&tx_dummy_frame);
	} else {
		controller->imr = QM_SPI_IMR_TXEIM | QM_SPI_IMR_TXOIM |
				  QM_SPI_IMR_RXUIM | QM_SPI_IMR_RXOIM |
				  QM_SPI_IMR_RXFIM;
	}

	controller->ssienr = QM_SPI_SSIENR_SSIENR; /** Enable SPI Device */

	return 0;
}

QM_ISR_DECLARE(qm_spi_master_0_isr)
{
	handle_spi_interrupt(QM_SPI_MST_0);
	QM_ISR_EOI(QM_IRQ_SPI_MASTER_0_VECTOR);
}

#if (QUARK_SE)
QM_ISR_DECLARE(qm_spi_master_1_isr)
{
	handle_spi_interrupt(QM_SPI_MST_1);
	QM_ISR_EOI(QM_IRQ_SPI_MASTER_1_VECTOR);
}
#endif

int qm_spi_irq_transfer_terminate(const qm_spi_t spi)
{
	QM_CHECK(spi < QM_SPI_NUM, -EINVAL);

	qm_spi_reg_t *const controller = QM_SPI[spi];
	const qm_spi_async_transfer_t *const transfer = spi_async_transfer[spi];

	/* Mask the interrupts */
	controller->imr = QM_SPI_IMR_MASK_ALL;
	controller->ssienr = 0; /** Disable SPI device */

	if (transfer->callback) {
		uint16_t len = 0;
		if (tmode[spi] == QM_SPI_TMOD_TX ||
		    tmode[spi] == QM_SPI_TMOD_TX_RX) {
			len = tx_counter[spi];

		} else {
			len = rx_counter[spi];
		}
		/*
		 * NOTE: change this to return controller-specific code
		 * 'user aborted'.
		 */
		transfer->callback(transfer->callback_data, -ECANCELED,
				   QM_SPI_IDLE, len);
	}

	tx_counter[spi] = 0;
	rx_counter[spi] = 0;

	return 0;
}

/* DMA driver invoked callback. */
static void spi_dma_callback(void *callback_context, uint32_t len,
			     int error_code)
{
	QM_ASSERT(callback_context);

	int client_error = 0;
	uint32_t frames_expected;
	volatile bool *cb_pending_alternate_p;

	/* The DMA driver returns a pointer to a dma_context struct from which
	 * we find out the corresponding SPI device and transfer direction. */
	dma_context_t *const dma_context_p = callback_context;
	const qm_spi_t spi = dma_context_p->spi_id;
	QM_ASSERT(spi < QM_SPI_NUM);
	qm_spi_reg_t *const controller = QM_SPI[spi];
	const qm_spi_async_transfer_t *const transfer = spi_async_transfer[spi];
	QM_ASSERT(transfer);
	const uint8_t frame_size = dfs[spi];
	QM_ASSERT((frame_size == 1) || (frame_size == 2) || (frame_size == 4));

	/* DMA driver returns length in bytes but user expects number of frames.
	 */
	const uint32_t frames_transfered = len / frame_size;

	QM_ASSERT((dma_context_p == &dma_context_tx[spi]) ||
		  (dma_context_p == &dma_context_rx[spi]));

	if (dma_context_p == &dma_context_tx[spi]) {
		/* TX transfer. */
		frames_expected = transfer->tx_len;
		cb_pending_alternate_p = &dma_context_rx[spi].cb_pending;
	} else if (dma_context_p == &dma_context_rx[spi]) {
		/* RX tranfer. */
		frames_expected = transfer->rx_len;
		cb_pending_alternate_p = &dma_context_tx[spi].cb_pending;
	} else {
		return;
	}

	QM_ASSERT(cb_pending_alternate_p);
	QM_ASSERT(dma_context_p->cb_pending);
	dma_context_p->cb_pending = false;

	if (error_code) {
		/* Transfer failed, pass to client the error code returned by
		 * the DMA driver. */
		client_error = error_code;
	} else if (false == *cb_pending_alternate_p) {
		/* TX transfers invoke the callback before the TX data has been
		 * transmitted, we need to wait here. */
		wait_for_controller(controller);

		if (frames_transfered != frames_expected) {
			QM_ASSERT(frames_transfered < frames_expected);
			/* Callback triggered through a transfer terminate. */
			client_error = -ECANCELED;
		}
	} else {
		/* Controller busy due to alternate DMA channel active. */
		return;
	}

	/* Disable DMA setting and SPI controller. */
	controller->dmacr = 0;
	controller->ssienr = 0;

	if (transfer->callback) {
		transfer->callback(transfer->callback_data, client_error,
				   QM_SPI_IDLE, frames_transfered);
	}
}

int qm_spi_dma_channel_config(
    const qm_spi_t spi, const qm_dma_t dma_ctrl_id,
    const qm_dma_channel_id_t dma_channel_id,
    const qm_dma_channel_direction_t dma_channel_direction)
{
	QM_CHECK(spi < QM_SPI_NUM, -EINVAL);
	QM_CHECK(dma_ctrl_id < QM_DMA_NUM, -EINVAL);
	QM_CHECK(dma_channel_id < QM_DMA_CHANNEL_NUM, -EINVAL);

	int ret = -EINVAL;
	dma_context_t *dma_context_p = NULL;
	qm_dma_channel_config_t dma_chan_cfg = {0};
	dma_chan_cfg.handshake_polarity = QM_DMA_HANDSHAKE_POLARITY_HIGH;
	dma_chan_cfg.channel_direction = dma_channel_direction;
	dma_chan_cfg.client_callback = spi_dma_callback;

	/* Every data transfer performed by the DMA core corresponds to an SPI
	 * data frame, the SPI uses the number of bits determined by a previous
	 * qm_spi_set_config call where the frame size was specified. */
	switch (dfs[spi]) {
	case 1:
		dma_chan_cfg.source_transfer_width = QM_DMA_TRANS_WIDTH_8;
		break;

	case 2:
		dma_chan_cfg.source_transfer_width = QM_DMA_TRANS_WIDTH_16;
		break;

	case 4:
		dma_chan_cfg.source_transfer_width = QM_DMA_TRANS_WIDTH_32;
		break;

	default:
		/* The DMA core cannot handle 3 byte frame sizes. */
		return -EINVAL;
	}
	dma_chan_cfg.destination_transfer_width =
	    dma_chan_cfg.source_transfer_width;

	switch (dma_channel_direction) {
	case QM_DMA_MEMORY_TO_PERIPHERAL:
		switch (spi) {
		case QM_SPI_MST_0:
			dma_chan_cfg.handshake_interface =
			    DMA_HW_IF_SPI_MASTER_0_TX;
			break;
#if (QUARK_SE)
		case QM_SPI_MST_1:
			dma_chan_cfg.handshake_interface =
			    DMA_HW_IF_SPI_MASTER_1_TX;
			break;
#endif
		default:
			/* Slave SPI is not supported. */
			return -EINVAL;
		}

		/* The DMA burst length has to fit in the space remaining in the
		 * TX FIFO after the watermark level, DMATDLR. */
		dma_chan_cfg.source_burst_length = SPI_DMA_WRITE_BURST_LENGTH;
		dma_chan_cfg.destination_burst_length =
		    SPI_DMA_WRITE_BURST_LENGTH;

		dma_context_p = &dma_context_tx[spi];
		break;

	case QM_DMA_PERIPHERAL_TO_MEMORY:
		switch (spi) {
		case QM_SPI_MST_0:
			dma_chan_cfg.handshake_interface =
			    DMA_HW_IF_SPI_MASTER_0_RX;
			break;
#if (QUARK_SE)
		case QM_SPI_MST_1:
			dma_chan_cfg.handshake_interface =
			    DMA_HW_IF_SPI_MASTER_1_RX;
			break;
#endif
		default:
			/* Slave SPI is not supported. */
			return -EINVAL;
		}

		/* The DMA burst length has to match the value of the receive
		 * watermark level, DMARDLR + 1. */
		dma_chan_cfg.source_burst_length = SPI_DMA_READ_BURST_LENGTH;
		dma_chan_cfg.destination_burst_length =
		    SPI_DMA_READ_BURST_LENGTH;

		dma_context_p = &dma_context_rx[spi];
		break;

	default:
		/* Memory to memory not allowed on SPI transfers. */
		return -EINVAL;
	}

	/* The DMA driver needs a pointer to the client callback function so
	 * that later we can identify to which SPI controller the DMA callback
	 * corresponds to as well as whether we are dealing with a TX or RX
	 * dma_context struct. */
	QM_ASSERT(dma_context_p);
	dma_chan_cfg.callback_context = dma_context_p;

	ret = qm_dma_channel_set_config(dma_ctrl_id, dma_channel_id,
					&dma_chan_cfg);
	if (ret) {
		return ret;
	}

	/* To be used on received DMA callback. */
	dma_context_p->spi_id = spi;
	dma_context_p->dma_channel_id = dma_channel_id;

	/* To be used on transfer setup. */
	dma_core[spi] = dma_ctrl_id;

	return 0;
}

int qm_spi_dma_transfer(const qm_spi_t spi,
			const qm_spi_async_transfer_t *const xfer)
{
	QM_CHECK(spi < QM_SPI_NUM, -EINVAL);
	QM_CHECK(xfer, -EINVAL);
	QM_CHECK(xfer->tx_len
		     ? (xfer->tx &&
			dma_context_tx[spi].dma_channel_id < QM_DMA_CHANNEL_NUM)
		     : 1,
		 -EINVAL);
	QM_CHECK(xfer->rx_len
		     ? (xfer->rx &&
			dma_context_rx[spi].dma_channel_id < QM_DMA_CHANNEL_NUM)
		     : 1,
		 -EINVAL);
	QM_CHECK(tmode[spi] == QM_SPI_TMOD_TX_RX ? (xfer->tx && xfer->rx) : 1,
		 -EINVAL);
	QM_CHECK(tmode[spi] == QM_SPI_TMOD_TX_RX
		     ? (xfer->tx_len == xfer->rx_len)
		     : 1,
		 -EINVAL);
	QM_CHECK(tmode[spi] == QM_SPI_TMOD_TX ? (xfer->tx_len && !xfer->rx_len)
					      : 1,
		 -EINVAL);
	QM_CHECK(tmode[spi] == QM_SPI_TMOD_RX ? (xfer->rx_len && !xfer->tx_len)
					      : 1,
		 -EINVAL);
	QM_CHECK(tmode[spi] == QM_SPI_TMOD_EEPROM_READ
		     ? (xfer->tx_len && xfer->rx_len)
		     : 1,
		 -EINVAL);
	QM_CHECK(dma_core[spi] < QM_DMA_NUM, -EINVAL);

	int ret;
	qm_dma_transfer_t dma_trans = {0};
	qm_spi_reg_t *const controller = QM_SPI[spi];
	QM_ASSERT(0 == controller->ssienr);

	/* Mask interrupts. */
	controller->imr = QM_SPI_IMR_MASK_ALL;

	if (xfer->rx_len) {
		dma_trans.block_size = xfer->rx_len;
		dma_trans.source_address = (uint32_t *)&controller->dr[0];
		dma_trans.destination_address = (uint32_t *)xfer->rx;
		ret = qm_dma_transfer_set_config(
		    dma_core[spi], dma_context_rx[spi].dma_channel_id,
		    &dma_trans);
		if (ret) {
			return ret;
		}

		/* In RX-only or EEPROM mode, the ctrlr1 register holds how
		 * many data frames the controller solicits, minus 1. */
		controller->ctrlr1 = xfer->rx_len - 1;
	}

	if (xfer->tx_len) {
		dma_trans.block_size = xfer->tx_len;
		dma_trans.source_address = (uint32_t *)xfer->tx;
		dma_trans.destination_address = (uint32_t *)&controller->dr[0];
		ret = qm_dma_transfer_set_config(
		    dma_core[spi], dma_context_tx[spi].dma_channel_id,
		    &dma_trans);
		if (ret) {
			return ret;
		}
	}

	/* Transfer pointer kept to extract user callback address and transfer
	 * client id when DMA completes. */
	spi_async_transfer[spi] = xfer;

	/* Enable the SPI device. */
	controller->ssienr = QM_SPI_SSIENR_SSIENR;

	if (xfer->rx_len) {
		/* Enable receive DMA. */
		controller->dmacr |= QM_SPI_DMACR_RDMAE;

		/* Set the DMA receive threshold. */
		controller->dmardlr = SPI_DMARDLR_DMARDL;

		dma_context_rx[spi].cb_pending = true;

		ret = qm_dma_transfer_start(dma_core[spi],
					    dma_context_rx[spi].dma_channel_id);
		if (ret) {
			dma_context_rx[spi].cb_pending = false;

			/* Disable DMA setting and SPI controller. */
			controller->dmacr = 0;
			controller->ssienr = 0;
			return ret;
		}

		if (!xfer->tx_len) {
			/* In RX-only mode we need to transfer an initial dummy
			 * byte. */
			write_frame(spi, (uint8_t *)&tx_dummy_frame);
		}
	}

	if (xfer->tx_len) {
		/* Enable transmit DMA. */
		controller->dmacr |= QM_SPI_DMACR_TDMAE;

		/* Set the DMA transmit threshold. */
		controller->dmatdlr = SPI_DMATDLR_DMATDL;

		dma_context_tx[spi].cb_pending = true;

		ret = qm_dma_transfer_start(dma_core[spi],
					    dma_context_tx[spi].dma_channel_id);
		if (ret) {
			dma_context_tx[spi].cb_pending = false;
			if (xfer->rx_len) {
				/* If a RX transfer was previously started, we
				 * need to stop it - the SPI device will be
				 * disabled when handling the DMA callback. */
				qm_spi_dma_transfer_terminate(spi);
			} else {
				/* Disable DMA setting and SPI controller. */
				controller->dmacr = 0;
				controller->ssienr = 0;
			}
			return ret;
		}
	}

	return 0;
}

int qm_spi_dma_transfer_terminate(qm_spi_t spi)
{
	QM_CHECK(spi < QM_SPI_NUM, -EINVAL);
	QM_CHECK(dma_context_tx[spi].cb_pending
		     ? (dma_context_tx[spi].dma_channel_id < QM_DMA_CHANNEL_NUM)
		     : 1,
		 -EINVAL);
	QM_CHECK(dma_context_rx[spi].cb_pending
		     ? (dma_context_rx[spi].dma_channel_id < QM_DMA_CHANNEL_NUM)
		     : 1,
		 -EINVAL);

	int ret = 0;

	if (dma_context_tx[spi].cb_pending) {
		if (0 !=
		    qm_dma_transfer_terminate(
			dma_core[spi], dma_context_tx[spi].dma_channel_id)) {
			ret = -EIO;
		}
	}

	if (dma_context_rx[spi].cb_pending) {
		if (0 !=
		    qm_dma_transfer_terminate(
			dma_core[spi], dma_context_rx[spi].dma_channel_id)) {
			ret = -EIO;
		}
	}

	return ret;
}
