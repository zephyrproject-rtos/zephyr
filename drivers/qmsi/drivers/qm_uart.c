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

#include <errno.h>
#include "qm_uart.h"

#ifndef UNIT_TEST
qm_uart_reg_t *qm_uart[QM_UART_NUM] = {(qm_uart_reg_t *)QM_UART_0_BASE,
				       (qm_uart_reg_t *)QM_UART_1_BASE};
#endif

typedef void (*uart_client_callback_t)(void *data, int error,
				       qm_uart_status_t status, uint32_t len);

/**
 * DMA transfer information, relevant on callback invocations from the DMA
 * driver.
 */
typedef struct {
	qm_dma_channel_id_t dma_channel_id; /**< DMA channel. */
	const qm_uart_transfer_t *xfer;     /**< User transfer structure. */
} dma_context_t;

/* UART Callback pointers. */
static uart_client_callback_t write_callback[QM_UART_NUM];
static uart_client_callback_t read_callback[QM_UART_NUM];

/* User callback data. */
static void *write_data[QM_UART_NUM], *read_data[QM_UART_NUM];

/* Buffer pointers to store transmit / receive data for UART */
static uint8_t *write_buffer[QM_UART_NUM], *read_buffer[QM_UART_NUM];
static uint32_t write_pos[QM_UART_NUM], write_len[QM_UART_NUM];
static uint32_t read_pos[QM_UART_NUM], read_len[QM_UART_NUM];

/* DMA (memory to UART) callback information. */
static dma_context_t dma_context_tx[QM_UART_NUM];
/* DMA (UART to memory) callback information. */
static dma_context_t dma_context_rx[QM_UART_NUM];
/* DMA core being used by each UART. */
static qm_dma_t dma_core[QM_UART_NUM];

static bool is_read_xfer_complete(const qm_uart_t uart)
{
	return read_pos[uart] >= read_len[uart];
}

static bool is_write_xfer_complete(const qm_uart_t uart)
{
	return write_pos[uart] >= write_len[uart];
}

static void qm_uart_isr_handler(const qm_uart_t uart)
{
	qm_uart_reg_t *const regs = QM_UART[uart];
	uint8_t interrupt_id = regs->iir_fcr & QM_UART_IIR_IID_MASK;

	/*
	 * Interrupt ID priority levels (from highest to lowest):
	 * 1: QM_UART_IIR_RECV_LINE_STATUS
	 * 2: QM_UART_IIR_RECV_DATA_AVAIL and QM_UART_IIR_CHAR_TIMEOUT
	 * 3: QM_UART_IIR_THR_EMPTY
	 */
	switch (interrupt_id) {
	case QM_UART_IIR_THR_EMPTY:
		if (is_write_xfer_complete(uart)) {
			regs->ier_dlh &= ~QM_UART_IER_ETBEI;
			/*
			 * At this point the FIFOs are empty, but the shift
			 * register still is transmitting the last 8 bits. So if
			 * we were to read LSR, it would say the device is still
			 * busy. Use the SCR Bit 0 to indicate an irq tx is
			 * complete.
			 */
			regs->scr |= BIT(0);
			if (write_callback[uart]) {
				write_callback[uart](write_data[uart], 0,
						     QM_UART_IDLE,
						     write_pos[uart]);
			}
			return;
		}

		/*
		 * If we are starting the transfer then the TX FIFO is empty.
		 * In that case we set 'count' variable to QM_UART_FIFO_DEPTH
		 * in order to take advantage of the whole FIFO capacity.
		 */
		int count = (write_pos[uart] == 0) ? QM_UART_FIFO_DEPTH
						   : QM_UART_FIFO_HALF_DEPTH;
		while (count-- && !is_write_xfer_complete(uart)) {
			regs->rbr_thr_dll =
			    write_buffer[uart][write_pos[uart]++];
		}

		/*
		 * Change the threshold level to trigger an interrupt when the
		 * TX buffer is empty.
		 */
		if (is_write_xfer_complete(uart)) {
			regs->iir_fcr = QM_UART_FCR_TX_0_RX_1_2_THRESHOLD |
					QM_UART_FCR_FIFOE;
		}

		break;

	case QM_UART_IIR_CHAR_TIMEOUT:
	case QM_UART_IIR_RECV_DATA_AVAIL:
		/*
		 * Copy data from RX FIFO to xfer buffer as long as the xfer
		 * has not completed and we have data in the RX FIFO.
		 */
		while (!is_read_xfer_complete(uart)) {
			uint32_t lsr = regs->lsr;
			/*
			 * A break condition may cause a line status interrupt
			 * to follow very closely after a char timeout
			 * interrupt, but reading the lsr effectively clears the
			 * pending interrupts so we issue here the callback
			 * instead, otherwise we would miss it.
			 * NOTE: Returned len is 0 for now, this might change
			 * in the future.
			 */
			if (lsr & QM_UART_LSR_ERROR_BITS) {
				if (read_callback[uart]) {
					read_callback[uart](
					    read_data[uart], -EIO,
					    lsr & QM_UART_LSR_ERROR_BITS, 0);
				}
			}
			if (lsr & QM_UART_LSR_DR) {
				read_buffer[uart][read_pos[uart]++] =
				    regs->rbr_thr_dll;
			} else {
				/* No more data in the RX FIFO */
				break;
			}
		}

		if (is_read_xfer_complete(uart)) {
			/*
			 * Disable both 'Receiver Data Available' and
			 * 'Receiver Line Status' interrupts.
			 */
			regs->ier_dlh &=
			    ~(QM_UART_IER_ERBFI | QM_UART_IER_ELSI);
			if (read_callback[uart]) {
				read_callback[uart](read_data[uart], 0,
						    QM_UART_IDLE,
						    read_pos[uart]);
			}
		}

		break;

	case QM_UART_IIR_RECV_LINE_STATUS:
		if (read_callback[uart]) {
			/*
			 * NOTE: Returned len is 0 for now, this might change
			 * in the future.
			 */
			read_callback[uart](read_data[uart], -EIO,
					    regs->lsr & QM_UART_LSR_ERROR_BITS,
					    0);
		}
		break;
	}
}

QM_ISR_DECLARE(qm_uart_0_isr)
{
	qm_uart_isr_handler(QM_UART_0);
	QM_ISR_EOI(QM_IRQ_UART_0_VECTOR);
}

QM_ISR_DECLARE(qm_uart_1_isr)
{
	qm_uart_isr_handler(QM_UART_1);
	QM_ISR_EOI(QM_IRQ_UART_1_VECTOR);
}

int qm_uart_set_config(const qm_uart_t uart, const qm_uart_config_t *cfg)
{
	QM_CHECK(uart < QM_UART_NUM, -EINVAL);
	QM_CHECK(cfg != NULL, -EINVAL);

	qm_uart_reg_t *const regs = QM_UART[uart];

	/* Clear DLAB by unsetting line parameters */
	regs->lcr = 0;

	/* Set divisor latch registers (integer + fractional part) */
	regs->lcr = QM_UART_LCR_DLAB;
	regs->ier_dlh = QM_UART_CFG_BAUD_DLH_UNPACK(cfg->baud_divisor);
	regs->rbr_thr_dll = QM_UART_CFG_BAUD_DLL_UNPACK(cfg->baud_divisor);
	regs->dlf = QM_UART_CFG_BAUD_DLF_UNPACK(cfg->baud_divisor);

	/* Set line parameters. This also unsets the DLAB */
	regs->lcr = cfg->line_control;

	/* Hardware automatic flow control */
	regs->mcr = 0;
	if (true == cfg->hw_fc) {
		regs->mcr |= QM_UART_MCR_AFCE | QM_UART_MCR_RTS;
	}

	/* FIFO's enable and reset, set interrupt threshold */
	regs->iir_fcr =
	    (QM_UART_FCR_FIFOE | QM_UART_FCR_RFIFOR | QM_UART_FCR_XFIFOR |
	     QM_UART_FCR_DEFAULT_TX_RX_THRESHOLD);
	regs->ier_dlh |= QM_UART_IER_PTIME;

	return 0;
}

int qm_uart_get_status(const qm_uart_t uart, qm_uart_status_t *const status)
{
	QM_CHECK(uart < QM_UART_NUM, -EINVAL);
	QM_CHECK(status != NULL, -EINVAL);
	qm_uart_reg_t *const regs = QM_UART[uart];
	uint32_t lsr = regs->lsr;

	*status = (lsr & (QM_UART_LSR_OE | QM_UART_LSR_PE | QM_UART_LSR_FE |
			  QM_UART_LSR_BI));

	/*
	 * Check as an IRQ TX completed, if so, the Shift register may still be
	 * busy.
	 */
	if (regs->scr & BIT(0)) {
		regs->scr &= ~BIT(0);
	} else if (!(lsr & (QM_UART_LSR_TEMT))) {
		*status |= QM_UART_TX_BUSY;
	}

	if (lsr & QM_UART_LSR_DR) {
		*status |= QM_UART_RX_BUSY;
	}

	return 0;
}

int qm_uart_write(const qm_uart_t uart, const uint8_t data)
{
	QM_CHECK(uart < QM_UART_NUM, -EINVAL);

	qm_uart_reg_t *const regs = QM_UART[uart];

	while (regs->lsr & QM_UART_LSR_THRE) {
	}
	regs->rbr_thr_dll = data;
	/* Wait for transaction to complete. */
	while (!(regs->lsr & QM_UART_LSR_TEMT)) {
	}

	return 0;
}

int qm_uart_read(const qm_uart_t uart, uint8_t *const data,
		 qm_uart_status_t *status)
{
	QM_CHECK(uart < QM_UART_NUM, -EINVAL);
	QM_CHECK(data != NULL, -EINVAL);

	qm_uart_reg_t *const regs = QM_UART[uart];

	uint32_t lsr = regs->lsr;
	while (!(lsr & QM_UART_LSR_DR)) {
		lsr = regs->lsr;
	}
	/* Check if there are any errors on the line. */
	if (lsr & QM_UART_LSR_ERROR_BITS) {
		if (status) {
			*status = (lsr & QM_UART_LSR_ERROR_BITS);
		}
		return -EIO;
	}
	*data = regs->rbr_thr_dll;

	return 0;
}

int qm_uart_write_non_block(const qm_uart_t uart, const uint8_t data)
{
	QM_CHECK(uart < QM_UART_NUM, -EINVAL);

	qm_uart_reg_t *const regs = QM_UART[uart];

	regs->rbr_thr_dll = data;

	return 0;
}

int qm_uart_read_non_block(const qm_uart_t uart, uint8_t *const data)
{
	QM_CHECK(uart < QM_UART_NUM, -EINVAL);
	QM_CHECK(data != NULL, -EINVAL);
	qm_uart_reg_t *const regs = QM_UART[uart];

	*data = regs->rbr_thr_dll;

	return 0;
}

int qm_uart_write_buffer(const qm_uart_t uart, const uint8_t *const data,
			 uint32_t len)
{
	QM_CHECK(uart < QM_UART_NUM, -EINVAL);
	QM_CHECK(data != NULL, -EINVAL);

	qm_uart_reg_t *const regs = QM_UART[uart];

	uint8_t *d = (uint8_t *)data;

	while (len--) {
		/*
		 * Because FCR_FIFOE and IER_PTIME are enabled, LSR_THRE
		 * behaves as a TX FIFO full indicator.
		 */
		while (regs->lsr & QM_UART_LSR_THRE) {
		}
		regs->rbr_thr_dll = *d;
		d++;
	}
	/* Wait for transaction to complete. */
	while (!(regs->lsr & QM_UART_LSR_TEMT)) {
	}
	return 0;
}

int qm_uart_irq_write(const qm_uart_t uart,
		      const qm_uart_transfer_t *const xfer)
{
	QM_CHECK(uart < QM_UART_NUM, -EINVAL);
	QM_CHECK(xfer != NULL, -EINVAL);

	qm_uart_reg_t *const regs = QM_UART[uart];

	write_pos[uart] = 0;
	write_len[uart] = xfer->data_len;
	write_buffer[uart] = xfer->data;
	write_callback[uart] = xfer->callback;
	write_data[uart] = xfer->callback_data;

	/* Set threshold */
	regs->iir_fcr =
	    (QM_UART_FCR_FIFOE | QM_UART_FCR_DEFAULT_TX_RX_THRESHOLD);

	/* Enable TX holding reg empty interrupt. */
	regs->ier_dlh |= QM_UART_IER_ETBEI;

	return 0;
}

int qm_uart_irq_read(const qm_uart_t uart, const qm_uart_transfer_t *const xfer)
{
	QM_CHECK(uart < QM_UART_NUM, -EINVAL);
	QM_CHECK(xfer != NULL, -EINVAL);

	qm_uart_reg_t *const regs = QM_UART[uart];

	read_pos[uart] = 0;
	read_len[uart] = xfer->data_len;
	read_buffer[uart] = xfer->data;
	read_callback[uart] = xfer->callback;
	read_data[uart] = xfer->callback_data;

	/* Set threshold */
	regs->iir_fcr =
	    (QM_UART_FCR_FIFOE | QM_UART_FCR_DEFAULT_TX_RX_THRESHOLD);

	/*
	 * Enable both 'Receiver Data Available' and 'Receiver
	 * Line Status' interrupts.
	 */
	regs->ier_dlh |= QM_UART_IER_ERBFI | QM_UART_IER_ELSI;

	return 0;
}

int qm_uart_irq_write_terminate(const qm_uart_t uart)
{
	QM_CHECK(uart < QM_UART_NUM, -EINVAL);

	qm_uart_reg_t *const regs = QM_UART[uart];

	/* Disable TX holding reg empty interrupt. */
	regs->ier_dlh &= ~QM_UART_IER_ETBEI;
	if (write_callback[uart]) {
		write_callback[uart](write_data[uart], -ECANCELED, QM_UART_IDLE,
				     write_pos[uart]);
	}
	write_len[uart] = 0;

	return 0;
}

int qm_uart_irq_read_terminate(const qm_uart_t uart)
{
	QM_CHECK(uart < QM_UART_NUM, -EINVAL);

	qm_uart_reg_t *const regs = QM_UART[uart];

	/*
	 * Disable both 'Receiver Data Available' and 'Receiver Line Status'
	 * interrupts.
	 */
	regs->ier_dlh &= ~(QM_UART_IER_ERBFI | QM_UART_IER_ELSI);
	if (read_callback[uart]) {
		read_callback[uart](read_data[uart], -ECANCELED, QM_UART_IDLE,
				    read_pos[uart]);
	}
	read_len[uart] = 0;

	return 0;
}

/* DMA driver invoked callback. */
static void uart_dma_callback(void *callback_context, uint32_t len,
			      int error_code)
{
	QM_ASSERT(callback_context);
	const qm_uart_transfer_t *const xfer =
	    ((dma_context_t *)callback_context)->xfer;
	QM_ASSERT(xfer);
	const uart_client_callback_t client_callback = xfer->callback;
	void *const client_data = xfer->callback_data;
	const uint32_t client_expected_len = xfer->data_len;

	if (!client_callback) {
		return;
	}

	if (error_code) {
		/*
		 * Transfer failed, pass to client the error code returned by
		 * the DMA driver.
		 */
		client_callback(client_data, error_code, QM_UART_IDLE, 0);
	} else if (len == client_expected_len) {
		/* Transfer completed successfully. */
		client_callback(client_data, 0, QM_UART_IDLE, len);
	} else {
		QM_ASSERT(len < client_expected_len);
		/* Transfer cancelled. */
		client_callback(client_data, -ECANCELED, QM_UART_IDLE, len);
	}
}

int qm_uart_dma_channel_config(
    const qm_uart_t uart, const qm_dma_t dma_ctrl_id,
    const qm_dma_channel_id_t dma_channel_id,
    const qm_dma_channel_direction_t dma_channel_direction)
{
	QM_CHECK(uart < QM_UART_NUM, -EINVAL);
	QM_CHECK(dma_ctrl_id < QM_DMA_NUM, -EINVAL);
	QM_CHECK(dma_channel_id < QM_DMA_CHANNEL_NUM, -EINVAL);

	int ret = -EINVAL;
	qm_dma_channel_config_t dma_chan_cfg = {0};
	/* UART has inverted handshake polarity. */
	dma_chan_cfg.handshake_polarity = QM_DMA_HANDSHAKE_POLARITY_LOW;
	dma_chan_cfg.channel_direction = dma_channel_direction;
	dma_chan_cfg.source_transfer_width = QM_DMA_TRANS_WIDTH_8;
	dma_chan_cfg.destination_transfer_width = QM_DMA_TRANS_WIDTH_8;
	/* Default FIFO threshold is 1/2 full (8 bytes). */
	dma_chan_cfg.source_burst_length = QM_DMA_BURST_TRANS_LENGTH_8;
	dma_chan_cfg.destination_burst_length = QM_DMA_BURST_TRANS_LENGTH_8;
	dma_chan_cfg.client_callback = uart_dma_callback;

	switch (dma_channel_direction) {
	case QM_DMA_MEMORY_TO_PERIPHERAL:
		switch (uart) {
		case QM_UART_0:
			dma_chan_cfg.handshake_interface = DMA_HW_IF_UART_A_TX;
			break;
		case QM_UART_1:
			dma_chan_cfg.handshake_interface = DMA_HW_IF_UART_B_TX;
			break;
		default:
			return -EINVAL;
		}

		/*
		 * The DMA driver needs a pointer to the DMA context structure
		 * used on DMA callback invocation.
		 */
		dma_context_tx[uart].dma_channel_id = dma_channel_id;
		dma_chan_cfg.callback_context = &dma_context_tx[uart];
		break;

	case QM_DMA_PERIPHERAL_TO_MEMORY:
		switch (uart) {
		case QM_UART_0:
			dma_chan_cfg.handshake_interface = DMA_HW_IF_UART_A_RX;
			break;
		case QM_UART_1:
			dma_chan_cfg.handshake_interface = DMA_HW_IF_UART_B_RX;
			break;
		default:
			return -EINVAL;
		}

		/*
		 * The DMA driver needs a pointer to the DMA context structure
		 * used on DMA callback invocation.
		 */
		dma_context_rx[uart].dma_channel_id = dma_channel_id;
		dma_chan_cfg.callback_context = &dma_context_rx[uart];
		break;

	default:
		/* Direction not allowed on UART transfers. */
		return -EINVAL;
	}

	dma_core[uart] = dma_ctrl_id;

	ret = qm_dma_channel_set_config(dma_ctrl_id, dma_channel_id,
					&dma_chan_cfg);
	return ret;
}

int qm_uart_dma_write(const qm_uart_t uart,
		      const qm_uart_transfer_t *const xfer)
{
	QM_CHECK(uart < QM_UART_NUM, -EINVAL);
	QM_CHECK(xfer, -EINVAL);
	QM_CHECK(xfer->data, -EINVAL);
	QM_CHECK(xfer->data_len, -EINVAL);

	int ret = -EINVAL;
	qm_uart_reg_t *regs = QM_UART[uart];
	qm_dma_transfer_t dma_trans = {0};
	dma_trans.block_size = xfer->data_len;
	dma_trans.source_address = (uint32_t *)xfer->data;
	dma_trans.destination_address = (uint32_t *)&regs->rbr_thr_dll;

	ret = qm_dma_transfer_set_config(
	    dma_core[uart], dma_context_tx[uart].dma_channel_id, &dma_trans);
	if (ret) {
		return ret;
	}

	/* Store the user transfer pointer that we will need on DMA callback. */
	dma_context_tx[uart].xfer = xfer;

	/* Set the FCR register FIFO thresholds. */
	regs->iir_fcr =
	    (QM_UART_FCR_FIFOE | QM_UART_FCR_DEFAULT_TX_RX_THRESHOLD);

	ret = qm_dma_transfer_start(dma_core[uart],
				    dma_context_tx[uart].dma_channel_id);

	return ret;
}

int qm_uart_dma_read(const qm_uart_t uart, const qm_uart_transfer_t *const xfer)
{
	QM_CHECK(uart < QM_UART_NUM, -EINVAL);
	QM_CHECK(xfer, -EINVAL);
	QM_CHECK(xfer->data, -EINVAL);
	QM_CHECK(xfer->data_len, -EINVAL);

	int ret = -EINVAL;
	qm_uart_reg_t *regs = QM_UART[uart];
	qm_dma_transfer_t dma_trans = {0};
	dma_trans.block_size = xfer->data_len;
	dma_trans.source_address = (uint32_t *)&regs->rbr_thr_dll;
	dma_trans.destination_address = (uint32_t *)xfer->data;

	ret = qm_dma_transfer_set_config(
	    dma_core[uart], dma_context_rx[uart].dma_channel_id, &dma_trans);
	if (ret) {
		return ret;
	}

	/* Store the user transfer pointer that we will need on DMA callback. */
	dma_context_rx[uart].xfer = xfer;

	/* Set the FCR register FIFO thresholds. */
	regs->iir_fcr =
	    (QM_UART_FCR_FIFOE | QM_UART_FCR_DEFAULT_TX_RX_THRESHOLD);

	ret = qm_dma_transfer_start(dma_core[uart],
				    dma_context_rx[uart].dma_channel_id);

	return ret;
}

int qm_uart_dma_write_terminate(const qm_uart_t uart)
{
	QM_CHECK(uart < QM_UART_NUM, -EINVAL);

	int ret = qm_dma_transfer_terminate(
	    dma_core[uart], dma_context_tx[uart].dma_channel_id);

	return ret;
}

int qm_uart_dma_read_terminate(const qm_uart_t uart)
{
	QM_CHECK(uart < QM_UART_NUM, -EINVAL);

	int ret = qm_dma_transfer_terminate(
	    dma_core[uart], dma_context_rx[uart].dma_channel_id);

	return ret;
}
