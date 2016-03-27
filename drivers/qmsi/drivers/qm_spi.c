/*
 * Copyright (c) 2015, Intel Corporation
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

/* SPI Ctrlr0 register */
#define SPI_CTRLR0_DFS_32_MASK (0x001F0000)
#define SPI_CTRLR0_TMOD_MASK (0x00000300)
#define SPI_CTRLR0_SCPOL_SCPH_MASK (0x000000C0)
#define SPI_CTRLR0_FRF_MASK (0x00000030)
#define SPI_CTRLR0_DFS_32_OFFSET (16)
#define SPI_CTRLR0_TMOD_OFFSET (8)
#define SPI_CTRLR0_SCPOL_SCPH_OFFSET (6)
#define SPI_CTRLR0_FRF_OFFSET (4)

/* SPI Status register */
#define SPI_SR_BUSY BIT(0)
#define SPI_SR_TFNF BIT(1)
#define SPI_SR_TFE BIT(2)

/* SPI Interrupt Mask register */
#define SPI_IMR_TXEIM BIT(0)
#define SPI_IMR_TXOIM BIT(1)
#define SPI_IMR_RXUIM BIT(2)
#define SPI_IMR_RXOIM BIT(3)
#define SPI_IMR_RXFIM BIT(4)

/* SPI Interrupt Status register */
#define SPI_ISR_TXEIS BIT(0)
#define SPI_ISR_TXOIS BIT(1)
#define SPI_ISR_RXUIS BIT(2)
#define SPI_ISR_RXOIS BIT(3)
#define SPI_ISR_RXFIS BIT(4)

/* SPI Raw Interrupt Status register */
#define SPI_RISR_TXEIR BIT(0)
#define SPI_RISR_TXOIR BIT(1)
#define SPI_RISR_RXUIR BIT(2)
#define SPI_RISR_RXOIR BIT(3)
#define SPI_RISR_RXFIR BIT(4)

/* SPI FIFO size defaults */
#define SPI_DEFAULT_TX_THRESHOLD (0x05)
#define SPI_DEFAULT_RX_THRESHOLD (0x05)
#define SPI_FIFOS_DEPTH (8)

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

static qm_spi_async_transfer_t *spi_async_transfer[QM_SPI_NUM];
static volatile uint32_t tx_counter[QM_SPI_NUM], rx_counter[QM_SPI_NUM];
static uint8_t dfs[QM_SPI_NUM];
static uint32_t tx_dummy_frame;
static qm_spi_tmode_t tmode[QM_SPI_NUM];

static void read_frame(const qm_spi_t spi, uint8_t *const rx_buffer)
{
	const qm_spi_reg_t *const controller = QM_SPI[spi];
	uint8_t frame_size = dfs[spi];

	if (frame_size == 1) {
		*(uint8_t *)rx_buffer = controller->dr[0];
	} else if (frame_size == 2) {
		*(uint16_t *)rx_buffer = controller->dr[0];
	} else {
		*(uint32_t *)rx_buffer = controller->dr[0];
	}
}

static void write_frame(const qm_spi_t spi, uint8_t *const tx_buffer)
{
	qm_spi_reg_t *const controller = QM_SPI[spi];
	uint8_t frame_size = dfs[spi];

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
	 * before checking SPI_SR_BUSY.
	 */
	while (!(controller->sr & SPI_SR_TFE))
		;
	while (controller->sr & SPI_SR_BUSY)
		;
}

/**
 * Service an RX FIFO Full interrupt
 *
 * @brief Interrupt based transfer on SPI.
 * @param [in] spi Which SPI to transfer from.
 */
static __inline void handle_rx_interrupt(const qm_spi_t spi)
{
	qm_spi_reg_t *const controller = QM_SPI[spi];
	qm_spi_async_transfer_t *const transfer = spi_async_transfer[spi];

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
			    ~(SPI_IMR_RXUIM | SPI_IMR_RXOIM | SPI_IMR_RXFIM);
			transfer->rx_callback(transfer->id, transfer->rx_len);
			break;
		}
	}

	/* Check if enough data will arrive to trigger an interrupt and adjust
	 * rxftlr accordingly.
	 */
	uint32_t frames_left = transfer->rx_len - rx_counter[spi];
	if (!frames_left) {
		controller->rxftlr = SPI_DEFAULT_RX_THRESHOLD;
	} else if (frames_left <= controller->rxftlr) {
		controller->rxftlr = frames_left - 1;
	}
}

/**
 * Service an Tx FIFO Empty interrupt
 *
 * @brief Interrupt based transfer on SPI.
 * @param [in] spi Which SPI to transfer to.
 */
static __inline void handle_tx_interrupt(const qm_spi_t spi)
{
	qm_spi_reg_t *const controller = QM_SPI[spi];
	qm_spi_async_transfer_t *const transfer = spi_async_transfer[spi];

	/* Jump to the right position of TX buffer.
	 * If no bytes were transmitted before, we start from the beginning,
	 * otherwise we jump to the next frame to be sent.
	 */
	uint8_t *tx_buffer = transfer->tx + (tx_counter[spi] * dfs[spi]);

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
	qm_spi_async_transfer_t *transfer = spi_async_transfer[spi];
	uint32_t int_status = controller->isr;

	QM_ASSERT((int_status & (SPI_ISR_TXOIS | SPI_ISR_RXUIS)) == 0);
	if (int_status & SPI_ISR_RXOIS) {
		transfer->err_callback(transfer->id, QM_RC_SPI_RX_OE);
		controller->rxoicr;
		controller->imr = 0;
		controller->ssienr = 0;
		return;
	}

	if (int_status & SPI_ISR_RXFIS) {
		handle_rx_interrupt(spi);
	}

	if (transfer->rx_len == rx_counter[spi] &&
	    transfer->tx_len == tx_counter[spi] &&
	    (controller->sr & SPI_SR_TFE) && !(controller->sr & SPI_SR_BUSY)) {
		controller->txftlr = SPI_DEFAULT_TX_THRESHOLD;
		controller->imr = 0;
		controller->ssienr = 0;

		if (tmode[spi] != QM_SPI_TMOD_RX) {
			transfer->tx_callback(transfer->id, transfer->tx_len);
		}

		return;
	}

	if (int_status & SPI_ISR_TXEIS && transfer->tx_len > tx_counter[spi]) {
		handle_tx_interrupt(spi);
	}
}

qm_rc_t qm_spi_set_config(const qm_spi_t spi, const qm_spi_config_t *cfg)
{
	QM_CHECK(spi < QM_SPI_NUM, QM_RC_EINVAL);
	QM_CHECK(cfg, QM_RC_EINVAL);

	QM_ASSERT(QM_SPI[spi]->ssienr == 0);

	qm_spi_reg_t *const controller = QM_SPI[spi];

	/* Apply the selected cfg options */
	controller->ctrlr0 = (cfg->frame_size << SPI_CTRLR0_DFS_32_OFFSET) |
			     (cfg->transfer_mode << SPI_CTRLR0_TMOD_OFFSET) |
			     (cfg->bus_mode << SPI_CTRLR0_SCPOL_SCPH_OFFSET);

	controller->txftlr = SPI_DEFAULT_TX_THRESHOLD;
	controller->rxftlr = SPI_DEFAULT_RX_THRESHOLD;

	controller->baudr = cfg->clk_divider;

	/* Keep the current data frame size in bytes, being:
	 * - 1 byte for DFS set from 4 to 8 bits;
	 * - 2 bytes for DFS set from 9 to 16 bits;
	 * - 3 bytes for DFS set from 17 to 24 bits;
	 * - 4 bytes for DFS set from 25 to 32 bits.
	 */
	dfs[spi] = (cfg->frame_size / 8) + 1;

	tmode[spi] = cfg->transfer_mode;

	return QM_RC_OK;
}

qm_rc_t qm_spi_get_config(const qm_spi_t spi, qm_spi_config_t *const cfg)
{
	QM_CHECK(spi < QM_SPI_NUM, QM_RC_EINVAL);
	QM_CHECK(cfg, QM_RC_EINVAL);

	qm_spi_reg_t *const controller = QM_SPI[spi];

	cfg->transfer_mode = (controller->ctrlr0 & SPI_CTRLR0_TMOD_MASK) >>
			     SPI_CTRLR0_TMOD_OFFSET;
	cfg->bus_mode = (controller->ctrlr0 & SPI_CTRLR0_SCPOL_SCPH_MASK) >>
			SPI_CTRLR0_SCPOL_SCPH_OFFSET;
	cfg->frame_size = (controller->ctrlr0 & SPI_CTRLR0_DFS_32_MASK) >>
			  SPI_CTRLR0_DFS_32_OFFSET;
	cfg->clk_divider = controller->baudr;

	return QM_RC_OK;
}

qm_rc_t qm_spi_slave_select(const qm_spi_t spi, const qm_spi_slave_select_t ss)
{
	QM_CHECK(spi < QM_SPI_NUM, QM_RC_EINVAL);

	/* Check if the device reports as busy. */
	QM_ASSERT(!(QM_SPI[spi]->sr & SPI_SR_BUSY));

	QM_SPI[spi]->ser = ss;

	return QM_RC_OK;
}

qm_spi_status_t qm_spi_get_status(const qm_spi_t spi)
{
	QM_CHECK(spi < QM_SPI_NUM, QM_SPI_EINVAL);

	if (QM_SPI[spi]->sr & SPI_SR_BUSY) {
		return QM_SPI_BUSY;
	} else {
		return QM_SPI_FREE;
	}
}

qm_rc_t qm_spi_transfer(const qm_spi_t spi, qm_spi_transfer_t *const xfer)
{
	QM_CHECK(spi < QM_SPI_NUM, QM_RC_EINVAL);
	QM_CHECK(xfer, QM_RC_EINVAL);
	QM_CHECK(tmode[spi] == QM_SPI_TMOD_TX_RX
		     ? (xfer->tx_len == xfer->rx_len)
		     : 1,
		 QM_RC_EINVAL);
	QM_CHECK(tmode[spi] == QM_SPI_TMOD_TX ? (xfer->rx_len == 0) : 1,
		 QM_RC_EINVAL);
	QM_CHECK(tmode[spi] == QM_SPI_TMOD_RX ? (xfer->tx_len == 0) : 1,
		 QM_RC_EINVAL);

	uint32_t i_tx = xfer->tx_len;
	uint32_t i_rx = xfer->rx_len;
	qm_rc_t rc = QM_RC_OK;

	qm_spi_reg_t *const controller = QM_SPI[spi];

	/* Wait for the SPI device to become available. */
	wait_for_controller(controller);

	/* Mask all interrupts, this is a blocking function. */
	controller->imr = 0;

	/* If we are in RX only or EEPROM Read mode, the ctrlr1 reg holds how
	 * many bytes the controller solicits, minus 1. */
	if (xfer->rx_len) {
		controller->ctrlr1 = xfer->rx_len - 1;
	}

	/* Enable SPI device */
	controller->ssienr = 1;

	/* Transfer is only complete when all the tx data is sent and all
	 * expected rx data has been received.
	 */
	uint8_t *rx_buffer = xfer->rx;
	uint8_t *tx_buffer = xfer->tx;

	int frames;

	/* RX Only transfers need a dummy byte to be sent for starting.
	 * This is covered by the databook on page 42.
	 */
	if (tmode[spi] == QM_SPI_TMOD_RX) {
		tx_buffer = (uint8_t*)&tx_dummy_frame;
		i_tx = 1;
	}

	while (i_tx || i_rx) {
		if (controller->risr & SPI_RISR_RXOIR) {
			rc = QM_RC_SPI_RX_OE;
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

qm_rc_t qm_spi_irq_transfer(const qm_spi_t spi,
			    qm_spi_async_transfer_t *const xfer)
{
	QM_CHECK(spi < QM_SPI_NUM, QM_RC_EINVAL);
	QM_CHECK(xfer, QM_RC_EINVAL);
	QM_CHECK(tmode[spi] == QM_SPI_TMOD_TX_RX
		     ? (xfer->tx_len == xfer->rx_len)
		     : 1,
		 QM_RC_EINVAL);
	QM_CHECK(tmode[spi] == QM_SPI_TMOD_TX_RX
		     ? (xfer->tx_callback && xfer->rx_callback)
		     : 1,
		 QM_RC_EINVAL);
	QM_CHECK(tmode[spi] == QM_SPI_TMOD_TX
		     ? (xfer->tx_callback && (xfer->rx_len == 0))
		     : 1,
		 QM_RC_EINVAL);
	QM_CHECK(tmode[spi] == QM_SPI_TMOD_RX
		     ? (xfer->rx_callback && (xfer->tx_len == 0))
		     : 1,
		 QM_RC_EINVAL);
	QM_CHECK(xfer->err_callback, QM_RC_EINVAL);

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

	spi_async_transfer[spi] = xfer;
	tx_counter[spi] = 0;
	rx_counter[spi] = 0;

	/* Unmask interrupts */
	if (tmode[spi] == QM_SPI_TMOD_TX) {
		controller->imr = SPI_IMR_TXEIM | SPI_IMR_TXOIM;
	} else if (tmode[spi] == QM_SPI_TMOD_RX) {
		controller->imr = SPI_IMR_RXUIM | SPI_IMR_RXOIM |
				  SPI_IMR_RXFIM;
		controller->ssienr = 1;
		write_frame(spi, (uint8_t*)&tx_dummy_frame);
	} else {
		controller->imr = SPI_IMR_TXEIM | SPI_IMR_TXOIM |
				  SPI_IMR_RXUIM | SPI_IMR_RXOIM |
				  SPI_IMR_RXFIM;
	}

	controller->ssienr = 1; /** Enable SPI Device */

	return QM_RC_OK;
}

void qm_spi_master_0_isr(void)
{
	handle_spi_interrupt(QM_SPI_MST_0);
	QM_ISR_EOI(QM_IRQ_SPI_MASTER_0_VECTOR);
}

#if (QUARK_SE)
void qm_spi_master_1_isr(void)
{
	handle_spi_interrupt(QM_SPI_MST_1);
	QM_ISR_EOI(QM_IRQ_SPI_MASTER_1_VECTOR);
}
#endif

qm_rc_t qm_spi_transfer_terminate(const qm_spi_t spi)
{
	QM_CHECK(spi < QM_SPI_NUM, QM_RC_EINVAL);

	qm_spi_reg_t *const controller = QM_SPI[spi];
	qm_spi_async_transfer_t *const transfer = spi_async_transfer[spi];

	/* Mask the interrupts */
	controller->imr = 0;
	controller->ssienr = 0; /** Disable SPI device */

	if (transfer->tx_callback != NULL) {
		transfer->tx_callback(transfer->id, tx_counter[spi]);
	}

	if (transfer->rx_callback != NULL) {
		transfer->rx_callback(transfer->id, rx_counter[spi]);
	}

	tx_counter[spi] = 0;
	rx_counter[spi] = 0;

	return QM_RC_OK;
}
