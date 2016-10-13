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

#include "qm_ss_spi.h"

#define FIFO_SIZE (8)      /* Maximum size of RX or TX FIFO */
#define FIFO_RX_W_MARK (6) /* Interrupt mark to read RX FIFO */
#define FIFO_TX_W_MARK (3) /* Interrupt mark to write TX FIFO */

#define BYTES_PER_FRAME(reg_data)                                              \
	(((reg_data & QM_SS_SPI_CTRL_DFS_MASK) >> 3) + 1)

static uint32_t base[QM_SS_SPI_NUM] = {QM_SS_SPI_0_BASE, QM_SS_SPI_1_BASE};

static const qm_ss_spi_async_transfer_t *spi_async_transfer[QM_SS_SPI_NUM];
static uint32_t rx_c[QM_SS_SPI_NUM];
static uint32_t tx_c[QM_SS_SPI_NUM];

static const uint16_t dummy_frame = 0;

/* Private Functions */
static void spi_disable(const qm_ss_spi_t spi)
{
	/* Disable SPI device */
	QM_SS_REG_AUX_NAND(base[spi] + QM_SS_SPI_SPIEN, QM_SS_SPI_SPIEN_EN);
	/* MASK all interrupts. */
	__builtin_arc_sr(0, base[spi] + QM_SS_SPI_INTR_MASK);
	/* Clear all interrupts */
	__builtin_arc_sr(QM_SS_SPI_INTR_ALL, base[spi] + QM_SS_SPI_CLR_INTR);
}

static __inline__ void fifo_write(const qm_ss_spi_t spi, const void *data,
				  uint8_t size)
{
	uint32_t dr;

	if (size == 1) {
		dr = *(uint8_t *)data;
	} else {
		dr = *(uint16_t *)data;
	}
	dr |= QM_SS_SPI_DR_W_MASK;

	__builtin_arc_sr(dr, base[spi] + QM_SS_SPI_DR);
}

static __inline__ void fifo_read(const qm_ss_spi_t spi, void *data,
				 uint8_t size)
{
	__builtin_arc_sr(QM_SS_SPI_DR_R_MASK, base[spi] + QM_SS_SPI_DR);
	if (size == 1) {
		*(uint8_t *)data = __builtin_arc_lr(base[spi] + QM_SS_SPI_DR);
	} else {
		*(uint16_t *)data = __builtin_arc_lr(base[spi] + QM_SS_SPI_DR);
	}
}

/* Public Functions */
int qm_ss_spi_set_config(const qm_ss_spi_t spi,
			 const qm_ss_spi_config_t *const cfg)
{
	QM_CHECK(spi < QM_SS_SPI_NUM, -EINVAL);
	QM_CHECK(cfg, -EINVAL);
	/* Configuration can be changed only when SPI is disabled */
	if (0 != (__builtin_arc_lr(base[spi] + QM_SS_SPI_SPIEN) &
		  QM_SS_SPI_SPIEN_EN)) {
		return -EBUSY;
	}

	uint32_t ctrl = __builtin_arc_lr(QM_SS_SPI_0_BASE + QM_SS_SPI_CTRL);
	ctrl &= QM_SS_SPI_CTRL_CLK_ENA;
	ctrl |= cfg->frame_size << QM_SS_SPI_CTRL_DFS_OFFS;
	ctrl |= cfg->transfer_mode << QM_SS_SPI_CTRL_TMOD_OFFS;
	ctrl |= cfg->bus_mode << QM_SS_SPI_CTRL_BMOD_OFFS;
	__builtin_arc_sr(ctrl, base[spi] + QM_SS_SPI_CTRL);

	__builtin_arc_sr(cfg->clk_divider, base[spi] + QM_SS_SPI_TIMING);

	return 0;
}

int qm_ss_spi_slave_select(const qm_ss_spi_t spi,
			   const qm_ss_spi_slave_select_t ss)
{
	QM_CHECK(spi < QM_SS_SPI_NUM, -EINVAL);

	/* Check if the device reports as busy. */
	if (__builtin_arc_lr(base[spi] + QM_SS_SPI_SR) & QM_SS_SPI_SR_BUSY) {
		return -EBUSY;
	}

	uint32_t spien = __builtin_arc_lr(base[spi] + QM_SS_SPI_SPIEN);
	spien &= ~QM_SS_SPI_SPIEN_SER_MASK;
	spien |= (ss << QM_SS_SPI_SPIEN_SER_OFFS);
	__builtin_arc_sr(spien, base[spi] + QM_SS_SPI_SPIEN);

	return 0;
}

int qm_ss_spi_get_status(const qm_ss_spi_t spi,
			 qm_ss_spi_status_t *const status)
{
	QM_CHECK(spi < QM_SS_SPI_NUM, -EINVAL);
	QM_CHECK(status, -EINVAL);

	if (__builtin_arc_lr(base[spi] + QM_SS_SPI_SR) & QM_SS_SPI_SR_BUSY) {
		*status = QM_SS_SPI_BUSY;
	} else {
		*status = QM_SS_SPI_IDLE;
	}

	return 0;
}

int qm_ss_spi_transfer(const qm_ss_spi_t spi,
		       const qm_ss_spi_transfer_t *const xfer,
		       qm_ss_spi_status_t *const status)
{
	QM_CHECK(spi < QM_SS_SPI_NUM, -EINVAL);
	QM_CHECK(xfer, -EINVAL);

	uint32_t ctrl = __builtin_arc_lr(base[spi] + QM_SS_SPI_CTRL);
	uint8_t tmode = (uint8_t)((ctrl & QM_SS_SPI_CTRL_TMOD_MASK) >>
				  QM_SS_SPI_CTRL_TMOD_OFFS);

	QM_CHECK(tmode == QM_SS_SPI_TMOD_TX_RX ? (xfer->tx_len == xfer->rx_len)
					       : 1,
		 -EINVAL);
	QM_CHECK(tmode == QM_SS_SPI_TMOD_TX ? (xfer->rx_len == 0) : 1, -EINVAL);
	QM_CHECK(tmode == QM_SS_SPI_TMOD_EEPROM_READ ? (xfer->rx_len > 0) : 1,
		 -EINVAL);
	QM_CHECK(tmode == QM_SS_SPI_TMOD_RX ? (xfer->rx_len > 0) : 1, -EINVAL);
	QM_CHECK(tmode == QM_SS_SPI_TMOD_RX ? (xfer->tx_len == 0) : 1, -EINVAL);

	uint32_t tx_cnt = xfer->tx_len;
	uint32_t rx_cnt = xfer->rx_len;
	uint8_t *rx_buffer = xfer->rx;
	uint8_t *tx_buffer = xfer->tx;
	int ret = 0;

	/* Disable all SPI interrupts */
	__builtin_arc_sr(0, base[spi] + QM_SS_SPI_INTR_MASK);

	/* Set NDF (Number of Data Frames) in RX or EEPROM Read mode. (-1) */
	if (tmode == QM_SS_SPI_TMOD_RX || tmode == QM_SS_SPI_TMOD_EEPROM_READ) {
		ctrl &= ~QM_SS_SPI_CTRL_NDF_MASK;
		ctrl |= ((xfer->rx_len - 1) << QM_SS_SPI_CTRL_NDF_OFFS) &
			QM_SS_SPI_CTRL_NDF_MASK;
		__builtin_arc_sr(ctrl, base[spi] + QM_SS_SPI_CTRL);
	}

	/* RX only transfers need a dummy frame to be sent. */
	if (tmode == QM_SS_SPI_TMOD_RX) {
		tx_buffer = (uint8_t *)&dummy_frame;
		tx_cnt = 1;
	}

	/* Calculate number of bytes per frame (1 or 2)*/
	uint8_t bytes = BYTES_PER_FRAME(ctrl);
	/* Enable SPI device */
	QM_SS_REG_AUX_OR(base[spi] + QM_SS_SPI_SPIEN, QM_SS_SPI_SPIEN_EN);

	while (tx_cnt || rx_cnt) {
		uint32_t sr = __builtin_arc_lr(base[spi] + QM_SS_SPI_SR);
		/* Break and report error if RX FIFO has overflown */
		if (__builtin_arc_lr(base[spi] + QM_SS_SPI_INTR_STAT) &
		    QM_SS_SPI_INTR_RXOI) {
			ret = -EIO;
			if (status) {
				*status |= QM_SS_SPI_RX_OVERFLOW;
			}
			break;
		}
		/* Copy data to buffer as long RX-FIFO is not empty */
		if (sr & QM_SS_SPI_SR_RFNE && rx_cnt) {
			fifo_read(spi, rx_buffer, bytes);
			rx_buffer += bytes;
			rx_cnt--;
		}
		/* Copy data from buffer as long TX-FIFO is not full. */
		if (sr & QM_SS_SPI_SR_TFNF && tx_cnt) {
			fifo_write(spi, tx_buffer, bytes);
			tx_buffer += bytes;
			tx_cnt--;
		}
	}
	/* Wait for last byte transferred */
	while (__builtin_arc_lr(base[spi] + QM_SS_SPI_SR) & QM_SS_SPI_SR_BUSY)
		;

	spi_disable(spi);
	return ret;
}

/* Interrupt related functions. */

int qm_ss_spi_irq_transfer(const qm_ss_spi_t spi,
			   const qm_ss_spi_async_transfer_t *const xfer)
{
	QM_CHECK(spi < QM_SS_SPI_NUM, -EINVAL);
	QM_CHECK(xfer, -EINVAL);

	/* Load and save initial control register */
	uint32_t ctrl = __builtin_arc_lr(base[spi] + QM_SS_SPI_CTRL);
	uint8_t tmode = (uint8_t)((ctrl & QM_SS_SPI_CTRL_TMOD_MASK) >>
				  QM_SS_SPI_CTRL_TMOD_OFFS);
	uint8_t bytes = BYTES_PER_FRAME(ctrl);

	QM_CHECK(tmode == QM_SS_SPI_TMOD_TX_RX ? (xfer->tx_len == xfer->rx_len)
					       : 1,
		 -EINVAL);

	spi_async_transfer[spi] = xfer;
	tx_c[spi] = xfer->tx_len;
	rx_c[spi] = xfer->rx_len;

	/* Set NDF (Number of Data Frames) in RX or EEPROM Read mode. (-1) */
	if (tmode == QM_SS_SPI_TMOD_RX || tmode == QM_SS_SPI_TMOD_EEPROM_READ) {
		ctrl &= ~QM_SS_SPI_CTRL_NDF_MASK;
		ctrl |= ((xfer->rx_len - 1) << QM_SS_SPI_CTRL_NDF_OFFS) &
			QM_SS_SPI_CTRL_NDF_MASK;
		__builtin_arc_sr(ctrl, base[spi] + QM_SS_SPI_CTRL);
	}

	uint32_t ftlr =
	    (((FIFO_RX_W_MARK < xfer->rx_len ? FIFO_RX_W_MARK : xfer->rx_len) -
	      1)
	     << QM_SS_SPI_FTLR_RFT_OFFS) &
	    QM_SS_SPI_FTLR_RFT_MASK;
	__builtin_arc_sr(ftlr, base[spi] + QM_SS_SPI_FTLR);

	/* Unmask all interrupts */
	__builtin_arc_sr(QM_SS_SPI_INTR_ALL, base[spi] + QM_SS_SPI_INTR_MASK);

	/* Enable SPI device */
	QM_SS_REG_AUX_OR(base[spi] + QM_SS_SPI_SPIEN, QM_SS_SPI_SPIEN_EN);

	/* RX only transfers need a dummy frame byte to be sent. */
	if (tmode == QM_SS_SPI_TMOD_RX) {
		fifo_write(spi, (uint8_t *)&dummy_frame, bytes);
	}

	return 0;
}

int qm_ss_spi_transfer_terminate(const qm_ss_spi_t spi)
{
	QM_CHECK(spi < QM_SS_SPI_NUM, -EINVAL);
	const qm_ss_spi_async_transfer_t *const transfer =
	    spi_async_transfer[spi];

	spi_disable(spi);

	if (transfer->callback) {
		uint32_t len = 0;
		uint32_t ctrl = __builtin_arc_lr(base[spi] + QM_SS_SPI_CTRL);
		uint8_t tmode = (uint8_t)((ctrl & QM_SS_SPI_CTRL_TMOD_MASK) >>
					  QM_SS_SPI_CTRL_TMOD_OFFS);
		if (tmode == QM_SS_SPI_TMOD_TX ||
		    tmode == QM_SS_SPI_TMOD_TX_RX) {
			len = transfer->tx_len - tx_c[spi];
		} else {
			len = transfer->rx_len - rx_c[spi];
		}

		/*
		 * NOTE: change this to return controller-specific code
		 * 'user aborted'.
		 */
		transfer->callback(transfer->callback_data, -ECANCELED,
				   QM_SS_SPI_IDLE, (uint16_t)len);
	}

	return 0;
}

static void handle_spi_err_interrupt(const qm_ss_spi_t spi)
{
	uint32_t intr_stat = __builtin_arc_lr(base[spi] + QM_SS_SPI_INTR_STAT);
	const qm_ss_spi_async_transfer_t *const transfer =
	    spi_async_transfer[spi];

	spi_disable(spi);
	QM_ASSERT((intr_stat & QM_SS_SPI_INTR_STAT_TXOI) == 0);
	QM_ASSERT((intr_stat & QM_SS_SPI_INTR_STAT_RXUI) == 0);

	if ((intr_stat & QM_SS_SPI_INTR_RXOI) && transfer->callback) {
		transfer->callback(transfer->callback_data, -EIO,
				   QM_SS_SPI_RX_OVERFLOW,
				   transfer->rx_len - rx_c[spi]);
	}
}

static void handle_spi_tx_interrupt(const qm_ss_spi_t spi)
{
	/* Clear Transmit Fifo Emtpy interrupt */
	__builtin_arc_sr(QM_SS_SPI_INTR_TXEI, base[spi] + QM_SS_SPI_CLR_INTR);

	uint32_t ctrl = __builtin_arc_lr(base[spi] + QM_SS_SPI_CTRL);
	/* Calculate number of bytes per frame (1 or 2)*/
	uint8_t bytes = BYTES_PER_FRAME(ctrl);
	uint8_t tmode = (uint8_t)((ctrl & QM_SS_SPI_CTRL_TMOD_MASK) >>
				  QM_SS_SPI_CTRL_TMOD_OFFS);
	const qm_ss_spi_async_transfer_t *const transfer =
	    spi_async_transfer[spi];

	/* Jump to the right position of TX buffer.
	 * If no bytes were transmitted before, we start from the beginning,
	 * otherwise we jump to the next frame to be sent.
	 */
	const uint8_t *tx_buffer =
	    transfer->tx + ((transfer->tx_len - tx_c[spi]) * bytes);

	if (tx_c[spi] == 0 &&
	    !(__builtin_arc_lr(base[spi] + QM_SS_SPI_SR) & QM_SS_SPI_SR_BUSY)) {
		if (tmode == QM_SS_SPI_TMOD_TX) {
			spi_disable(spi);
			if (transfer->callback) {
				transfer->callback(transfer->callback_data, 0,
						   QM_SS_SPI_IDLE,
						   transfer->tx_len);
			}
		} else {
			QM_SS_REG_AUX_NAND(base[spi] + QM_SS_SPI_INTR_MASK,
					   QM_SS_SPI_INTR_TXEI);
		}
		return;
	}
	/* Make sure RX fifo does not overflow */
	uint32_t rxflr = __builtin_arc_lr(base[spi] + QM_SS_SPI_RXFLR);
	uint32_t txflr = __builtin_arc_lr(base[spi] + QM_SS_SPI_TXFLR);
	int32_t cnt = FIFO_SIZE - rxflr - txflr - 1;
	while (tx_c[spi] && cnt > 0) {
		fifo_write(spi, tx_buffer, bytes);
		tx_buffer += bytes;
		tx_c[spi]--;
		cnt--;
	}
}

static void handle_spi_rx_interrupt(const qm_ss_spi_t spi)
{
	/* Clear RX-FIFO FULL interrupt */
	__builtin_arc_sr(QM_SS_SPI_INTR_RXFI, base[spi] + QM_SS_SPI_CLR_INTR);

	uint32_t ctrl = __builtin_arc_lr(base[spi] + QM_SS_SPI_CTRL);
	/* Calculate number of bytes per frame (1 or 2)*/
	uint8_t bytes = BYTES_PER_FRAME(ctrl);
	const qm_ss_spi_async_transfer_t *const transfer =
	    spi_async_transfer[spi];

	/*
	 * Jump to the right position of RX buffer.
	 * If no bytes were received before, we start from the beginning,
	 * otherwise we jump to the next available frame position.
	 */
	uint8_t *rx_buffer =
	    transfer->rx + ((transfer->rx_len - rx_c[spi]) * bytes);

	while (__builtin_arc_lr(base[spi] + QM_SS_SPI_SR) & QM_SS_SPI_SR_RFNE &&
	       rx_c[spi]) {
		fifo_read(spi, rx_buffer, bytes);
		rx_buffer += bytes;
		rx_c[spi]--;
	}
	/* Set new FIFO threshold or complete transfer */
	uint32_t new_irq_level =
	    (FIFO_RX_W_MARK < rx_c[spi] ? FIFO_RX_W_MARK : rx_c[spi]);
	if (rx_c[spi]) {
		new_irq_level--;
		uint32_t ftlr = __builtin_arc_lr(base[spi] + QM_SS_SPI_FTLR);
		ftlr &= ~QM_SS_SPI_FTLR_RFT_MASK;
		ftlr |= (new_irq_level << QM_SS_SPI_FTLR_RFT_OFFS);
		__builtin_arc_sr(ftlr, base[spi] + QM_SS_SPI_FTLR);
	} else {
		spi_disable(spi);
		if (transfer->callback) {
			transfer->callback(transfer->callback_data, 0,
					   QM_SS_SPI_IDLE, transfer->rx_len);
		}
	}
}

QM_ISR_DECLARE(qm_ss_spi_0_err_isr)
{
	handle_spi_err_interrupt(QM_SS_SPI_0);
}
QM_ISR_DECLARE(qm_ss_spi_1_err_isr)
{
	handle_spi_err_interrupt(QM_SS_SPI_1);
}
QM_ISR_DECLARE(qm_ss_spi_0_rx_isr)
{
	handle_spi_rx_interrupt(QM_SS_SPI_0);
}
QM_ISR_DECLARE(qm_ss_spi_1_rx_isr)
{
	handle_spi_rx_interrupt(QM_SS_SPI_1);
}
QM_ISR_DECLARE(qm_ss_spi_0_tx_isr)
{
	handle_spi_tx_interrupt(QM_SS_SPI_0);
}
QM_ISR_DECLARE(qm_ss_spi_1_tx_isr)
{
	handle_spi_tx_interrupt(QM_SS_SPI_1);
}
