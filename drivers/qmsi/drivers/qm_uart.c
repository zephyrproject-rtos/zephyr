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

#include "qm_uart.h"

/* 1/2 fifo full in RX, 1/2 fifo full in TX. */
#define QM_UART_DEFAULT_TX_RX_THRESHOLD (0xB0)
#define QM_UART_TX_0_RX_1_2_THRESHOLD (0x80)

/* UART Callback pointers. */
static void (*uart_write_callback[QM_UART_NUM])(uint32_t id, uint32_t len);
static void (*uart_read_callback[QM_UART_NUM])(uint32_t id, uint32_t len);
static void (*uart_write_err_callback[QM_UART_NUM])(uint32_t id,
						    qm_uart_status_t status);
static void (*uart_read_err_callback[QM_UART_NUM])(uint32_t id,
						   qm_uart_status_t status);

/* Txfer transaction ids. */
static uint32_t uart_write_id[QM_UART_NUM], uart_read_id[QM_UART_NUM];

/* Buffer pointers to store transmit / receive data for UART */
static uint8_t *uart_write_buffer[QM_UART_NUM], *uart_read_buffer[QM_UART_NUM];
static uint32_t uart_write_pos[QM_UART_NUM], uart_write_remaining[QM_UART_NUM];
static uint32_t uart_read_pos[QM_UART_NUM], uart_read_remaining[QM_UART_NUM];

static void qm_uart_isr_handler(const qm_uart_t uart)
{
	uint32_t lsr;
	uint8_t interrupt_id = QM_UART[uart].iir_fcr & QM_UART_IIR_IID_MASK;

	/* Is the transmit holding empty? */
	if (interrupt_id == QM_UART_IIR_THR_EMPTY) {
		if (!(uart_write_remaining[uart])) {
			QM_UART[uart].ier_dlh &= ~QM_UART_IER_ETBEI;
			/*
			 * At this point the FIFOs are empty, but the shift
			 * register still is transmitting the last 8 bits. So if
			 * we were to read LSR, it would say the device is still
			 * busy. Use the SCR Bit 0 to indicate an irq tx is
			 * complete.
			 */
			QM_UART[uart].scr |= BIT(0);
			uart_write_callback[uart](uart_write_id[uart],
						  uart_write_pos[uart]);
		}

		uint32_t i =
		    uart_write_remaining[uart] >= QM_UART_FIFO_HALF_DEPTH
			? QM_UART_FIFO_HALF_DEPTH
			: uart_write_remaining[uart];
		while (i--) {
			QM_UART[uart].rbr_thr_dll =
			    uart_write_buffer[uart][uart_write_pos[uart]++];
			uart_write_remaining[uart]--;
		}

		/*
		 * Change the threshold level to trigger an interrupt when the
		 * TX buffer is empty.
		 */
		if (!(uart_write_remaining[uart])) {
			QM_UART[uart].iir_fcr =
			    QM_UART_TX_0_RX_1_2_THRESHOLD | QM_UART_FCR_FIFOE;
		}
	}

	/* Read any bytes on the line. */
	lsr = QM_UART[uart].lsr &
	      (QM_UART_LSR_ERROR_BITS | QM_UART_LSR_DR | QM_UART_LSR_RFE);
	while (lsr) {
		/* If there's an error, tell the application. */
		if (lsr & QM_UART_LSR_ERROR_BITS) {
			uart_read_err_callback[uart](
			    uart_read_id[uart], lsr & QM_UART_LSR_ERROR_BITS);
		}

		if ((lsr & QM_UART_LSR_DR) && uart_read_remaining[uart]) {
			uart_read_buffer[uart][uart_read_pos[uart]++] =
			    QM_UART[uart].rbr_thr_dll;
			uart_read_remaining[uart]--;
			if (!(uart_read_remaining[uart])) {
				/* Disable receive interrupts */
				QM_UART[uart].ier_dlh &= ~QM_UART_IER_ERBFI;
				uart_read_callback[uart](uart_read_id[uart],
							 uart_read_pos[uart]);

				/* It might have more data available in the
				 * RX FIFO which belongs to a subsequent
				 * transfer. So, since this read transfer
				 * has completed, should stop reading the
				 * LSR otherwise we might loop forever.
				 */
				break;
			}
		}
		lsr = QM_UART[uart].lsr & (QM_UART_LSR_ERROR_BITS |
					   QM_UART_LSR_DR | QM_UART_LSR_RFE);
	}
}

void qm_uart_0_isr(void)
{
	qm_uart_isr_handler(QM_UART_0);
	QM_ISR_EOI(QM_IRQ_UART_0_VECTOR);
}

void qm_uart_1_isr(void)
{
	qm_uart_isr_handler(QM_UART_1);
	QM_ISR_EOI(QM_IRQ_UART_1_VECTOR);
}

qm_rc_t qm_uart_set_config(const qm_uart_t uart, const qm_uart_config_t *cfg)
{
	QM_CHECK(uart < QM_UART_NUM, QM_RC_EINVAL);
	QM_CHECK(cfg != NULL, QM_RC_EINVAL);

	/* Clear DLAB by unsetting line parameters */
	QM_UART[uart].lcr = 0;

	/* Set divisor latch registers (integer + fractional part) */
	QM_UART[uart].lcr = QM_UART_LCR_DLAB;
	QM_UART[uart].ier_dlh = QM_UART_CFG_BAUD_DLH_UNPACK(cfg->baud_divisor);
	QM_UART[uart].rbr_thr_dll =
	    QM_UART_CFG_BAUD_DLL_UNPACK(cfg->baud_divisor);
	QM_UART[uart].dlf = QM_UART_CFG_BAUD_DLF_UNPACK(cfg->baud_divisor);

	/* Set line parameters. This also unsets the DLAB */
	QM_UART[uart].lcr = cfg->line_control;

	/* Hardware automatic flow control */
	QM_UART[uart].mcr = 0;
	if (true == cfg->hw_fc) {
		QM_UART[uart].mcr |= QM_UART_MCR_AFCE | QM_UART_MCR_RTS;
	}

	/* FIFO's enable and reset, set interrupt threshold */
	QM_UART[uart].iir_fcr =
	    (QM_UART_FCR_FIFOE | QM_UART_FCR_RFIFOR | QM_UART_FCR_XFIFOR |
	     QM_UART_DEFAULT_TX_RX_THRESHOLD);
	QM_UART[uart].ier_dlh |= QM_UART_IER_PTIME;

	return QM_RC_OK;
}

qm_rc_t qm_uart_get_config(const qm_uart_t uart, qm_uart_config_t *cfg)
{
	QM_CHECK(uart < QM_UART_NUM, QM_RC_EINVAL);
	QM_CHECK(cfg != NULL, QM_RC_EINVAL);

	cfg->baud_divisor = 0;
	cfg->hw_fc = 0;

	QM_UART[uart].lcr |= QM_UART_LCR_DLAB;
	cfg->baud_divisor = QM_UART_CFG_BAUD_DL_PACK(QM_UART[uart].ier_dlh,
						     QM_UART[uart].rbr_thr_dll,
						     QM_UART[uart].dlf);
	QM_UART[uart].lcr &= ~QM_UART_LCR_DLAB;
	cfg->line_control = QM_UART[uart].lcr;
	cfg->hw_fc = QM_UART[uart].mcr & QM_UART_MCR_AFCE;

	return QM_RC_OK;
}

qm_uart_status_t qm_uart_get_status(const qm_uart_t uart)
{
	qm_uart_status_t ret = QM_UART_IDLE;
	uint32_t lsr = QM_UART[uart].lsr;

	ret |= (lsr & (QM_UART_LSR_OE | QM_UART_LSR_PE | QM_UART_LSR_FE |
		       QM_UART_LSR_BI));

	/*
	 * Check as an IRQ TX completed, if so, the Shift register may still be
	 * busy.
	 */
	if (QM_UART[uart].scr & BIT(0)) {
		QM_UART[uart].scr &= ~BIT(0);
	} else if (!(lsr & (QM_UART_LSR_TEMT))) {
		ret |= QM_UART_TX_BUSY;
	}

	if (lsr & QM_UART_LSR_DR) {
		ret |= QM_UART_RX_BUSY;
	}

	return ret;
}

qm_rc_t qm_uart_write(const qm_uart_t uart, const uint8_t data)
{
	QM_CHECK(uart < QM_UART_NUM, QM_RC_EINVAL);

	while (QM_UART[uart].lsr & QM_UART_LSR_THRE) {
	}
	QM_UART[uart].rbr_thr_dll = data;
	/* Wait for transaction to complete. */
	while (!(QM_UART[uart].lsr & QM_UART_LSR_TEMT)) {
	}

	return QM_RC_OK;
}

qm_uart_status_t qm_uart_read(const qm_uart_t uart, uint8_t *data)
{
	QM_CHECK(uart < QM_UART_NUM, QM_UART_EINVAL);
	QM_CHECK(data != NULL, QM_UART_EINVAL);

	uint32_t lsr = QM_UART[uart].lsr;
	while (!(lsr & QM_UART_LSR_DR)) {
		lsr = QM_UART[uart].lsr;
	}
	/* Check are there any errors on the line. */
	if (lsr & QM_UART_LSR_ERROR_BITS) {
		return (lsr & QM_UART_LSR_ERROR_BITS);
	}
	*data = QM_UART[uart].rbr_thr_dll;

	return QM_UART_OK;
}

qm_rc_t qm_uart_write_non_block(const qm_uart_t uart, const uint8_t data)
{
	QM_CHECK(uart < QM_UART_NUM, QM_RC_EINVAL);

	QM_UART[uart].rbr_thr_dll = data;

	return QM_RC_OK;
}

uint8_t qm_uart_read_non_block(const qm_uart_t uart)
{
	return QM_UART[uart].rbr_thr_dll;
}

qm_rc_t qm_uart_write_buffer(const qm_uart_t uart, const uint8_t *const data,
			     uint32_t len)
{
	QM_CHECK(uart < QM_UART_NUM, QM_RC_EINVAL);
	QM_CHECK(data != NULL, QM_RC_EINVAL);

	uint8_t *d = (uint8_t *)data;

	while (len--) {
		/*
		 * Because FCR_FIFOE and IER_PTIME are enabled, LSR_THRE
		 * behaves as a TX FIFO full indicator.
		 */
		while (QM_UART[uart].lsr & QM_UART_LSR_THRE) {
		}
		QM_UART[uart].rbr_thr_dll = *d;
		d++;
	}
	/* Wait for transaction to complete. */
	while (!(QM_UART[uart].lsr & QM_UART_LSR_TEMT)) {
	}
	return QM_RC_OK;
}

qm_uart_status_t qm_uart_irq_write(const qm_uart_t uart,
				   const qm_uart_transfer_t *const xfer)
{
	QM_CHECK(uart < QM_UART_NUM, QM_UART_EINVAL);
	QM_CHECK(xfer != NULL, QM_UART_EINVAL);
	QM_CHECK(xfer->fin_callback, QM_UART_EINVAL);
	QM_CHECK(xfer->err_callback, QM_UART_EINVAL);

	qm_uart_status_t ret = QM_UART_TX_BUSY;

	if (!(QM_UART_TX_BUSY & qm_uart_get_status(uart))) {
		ret = QM_UART_OK;

		uart_write_pos[uart] = 0;
		uart_write_remaining[uart] = xfer->data_len;
		uart_write_buffer[uart] = xfer->data;
		uart_write_callback[uart] = xfer->fin_callback;
		uart_write_err_callback[uart] = xfer->err_callback;
		uart_write_id[uart] = xfer->id;

		/* Set threshold */
		QM_UART[uart].iir_fcr =
		    (QM_UART_FCR_FIFOE | QM_UART_DEFAULT_TX_RX_THRESHOLD);

		/* Enable TX holding reg empty interrupt. */
		QM_UART[uart].ier_dlh |= QM_UART_IER_ETBEI;
	}

	return ret;
}

qm_uart_status_t qm_uart_irq_read(const qm_uart_t uart,
				  const qm_uart_transfer_t *const xfer)
{
	QM_CHECK(uart < QM_UART_NUM, QM_UART_EINVAL);
	QM_CHECK(xfer != NULL, QM_UART_EINVAL);
	QM_CHECK(xfer->fin_callback, QM_UART_EINVAL);
	QM_CHECK(xfer->err_callback, QM_UART_EINVAL);

	qm_uart_status_t ret = QM_UART_RX_BUSY;

	if (0 == uart_read_remaining[uart]) {
		ret = QM_UART_OK;

		uart_read_pos[uart] = 0;
		uart_read_remaining[uart] = xfer->data_len;
		uart_read_buffer[uart] = xfer->data;
		uart_read_callback[uart] = xfer->fin_callback;
		uart_read_err_callback[uart] = xfer->err_callback;
		uart_read_id[uart] = xfer->id;

		/* Set threshold */
		QM_UART[uart].iir_fcr =
		    (QM_UART_FCR_FIFOE | QM_UART_DEFAULT_TX_RX_THRESHOLD);

		/* Enable RX interrupt. */
		QM_UART[uart].ier_dlh |= QM_UART_IER_ERBFI;
	}
	return ret;
}

qm_rc_t qm_uart_write_terminate(const qm_uart_t uart)
{
	QM_CHECK(uart < QM_UART_NUM, QM_RC_EINVAL);

	/* Disable TX holding reg empty interrupt. */
	QM_UART[uart].ier_dlh &= ~QM_UART_IER_ETBEI;
	uart_write_callback[uart](uart_write_id[uart], uart_write_pos[uart]);
	uart_write_remaining[uart] = 0;

	return QM_RC_OK;
}

qm_rc_t qm_uart_read_terminate(const qm_uart_t uart)
{
	QM_CHECK(uart < QM_UART_NUM, QM_RC_EINVAL);

	/* Disable receive interrupts */
	QM_UART[uart].ier_dlh &= ~QM_UART_IER_ERBFI;
	uart_read_callback[uart](uart_read_id[uart], uart_read_pos[uart]);
	uart_read_remaining[uart] = 0;

	return QM_RC_OK;
}
