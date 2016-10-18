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

#include <string.h>
#include "qm_i2c.h"
#include "clk.h"

#define TX_TL (2)
#define RX_TL (5)
#define SPK_LEN_SS (1)
#define SPK_LEN_FS_FSP (2)

/* Number of retries before giving up on disabling the controller. */
#define I2C_POLL_COUNT (1000000)
#define I2C_POLL_MICROSECOND (1)

#ifndef UNIT_TEST
#if (QUARK_SE)
qm_i2c_reg_t *qm_i2c[QM_I2C_NUM] = {(qm_i2c_reg_t *)QM_I2C_0_BASE,
				    (qm_i2c_reg_t *)QM_I2C_1_BASE};
#elif(QUARK_D2000)
qm_i2c_reg_t *qm_i2c[QM_I2C_NUM] = {(qm_i2c_reg_t *)QM_I2C_0_BASE};
#endif
#endif

static volatile const qm_i2c_transfer_t *i2c_transfer[QM_I2C_NUM];
static volatile uint32_t i2c_write_pos[QM_I2C_NUM], i2c_read_pos[QM_I2C_NUM],
    i2c_read_cmd_send[QM_I2C_NUM];

/* True if user buffers have been updated. */
static volatile bool transfer_ongoing = false;

/*
 * Keep track of activity if addressed.
 * There is no register which keeps track of the internal state machine status,
 * whether it is addressed, transmitting or receiving.
 * The only way to keep track of this is to save the information that the driver
 * received one the following interrupts:
 * - General call interrupt
 * - Read request
 * - RX FIFO full.
 * Also, if no interrupt has been received during an RX transaction, the driver
 * can check the controller has been addressed if data has been effectively
 * received.
 */
static volatile bool is_addressed = false;

/*
 * I2C DMA controller configuration descriptor.
 */
typedef struct {
	qm_i2c_t i2c; /* I2C controller. */
	qm_dma_t dma_controller_id;
	qm_dma_channel_id_t dma_tx_channel_id;    /* TX channel ID. */
	qm_dma_transfer_t dma_tx_transfer_config; /* Configuration for TX. */
	qm_dma_channel_id_t dma_rx_channel_id;    /* RX channel ID. */
	qm_dma_transfer_t dma_rx_transfer_config; /* Configuration for RX. */
	/* Configuration for writing READ commands during an RX operation. */
	qm_dma_transfer_t dma_cmd_transfer_config;
	volatile bool ongoing_dma_tx_operation; /* Keep track of ongoing TX. */
	volatile bool ongoing_dma_rx_operation; /* Keep track of oingoing RX. */
	int tx_abort_status;
	int i2c_error_code;
} i2c_dma_context_t;

/* DMA context. */
i2c_dma_context_t i2c_dma_context[QM_I2C_NUM];

/* Define the DMA interfaces. */
#if (QUARK_SE)
qm_dma_handshake_interface_t i2c_dma_interfaces[QM_I2C_NUM][3] = {
    {-1, DMA_HW_IF_I2C_MASTER_0_TX, DMA_HW_IF_I2C_MASTER_0_RX},
    {-1, DMA_HW_IF_I2C_MASTER_1_TX, DMA_HW_IF_I2C_MASTER_1_RX}};
#endif
#if (QUARK_D2000)
qm_dma_handshake_interface_t i2c_dma_interfaces[QM_I2C_NUM][3] = {
    {-1, DMA_HW_IF_I2C_MASTER_0_TX, DMA_HW_IF_I2C_MASTER_0_RX}};
#endif

static void i2c_dma_transmit_callback(void *callback_context, uint32_t len,
				      int error_code);
static void i2c_dma_receive_callback(void *callback_context, uint32_t len,
				     int error_code);
static int i2c_start_dma_read(const qm_i2c_t i2c);
void *i2c_dma_callbacks[] = {NULL, i2c_dma_transmit_callback,
			     i2c_dma_receive_callback};

static void controller_enable(const qm_i2c_t i2c);
static int controller_disable(const qm_i2c_t i2c);

/*
 * Empty RX FIFO.
 * Try to empty FIFO to user buffer. If RX buffer is full, trigger callback.
 * If user does not update buffer when requested, empty FIFO without storing
 * received data.
 */
static void empty_rx_fifo(const qm_i2c_t i2c,
			  const volatile qm_i2c_transfer_t *const transfer,
			  qm_i2c_reg_t *const controller)
{
	while (controller->ic_status & QM_I2C_IC_STATUS_RFNE) {
		if (!transfer_ongoing) {
			/* Dummy read. */
			controller->ic_data_cmd;
		} else {
			if (transfer->rx_len > i2c_read_pos[i2c]) {
				transfer->rx[i2c_read_pos[i2c]++] =
				    controller->ic_data_cmd & 0xFF;
			}

			if (transfer->rx_len == i2c_read_pos[i2c]) {
				/*
				 * End user transfer if user does not update
				 * buffers.
				 */
				transfer_ongoing = false;

				if (transfer->callback) {
					transfer->callback(
					    transfer->callback_data, 0,
					    QM_I2C_RX_FULL, transfer->rx_len);
				}
			}
		}
	}
}

/*
 * Fill TX FIFO.
 * Try to fill the FIFO with user data. If TX buffer is empty, trigger callback.
 * If user does not update buffer when requested, fill the FIFO with dummy
 * data.
 */
static void fill_tx_fifo(const qm_i2c_t i2c,
			 const volatile qm_i2c_transfer_t *const transfer,
			 qm_i2c_reg_t *const controller)
{
	while ((controller->ic_status & QM_I2C_IC_STATUS_TNF) &&
	       (!(controller->ic_intr_stat & QM_I2C_IC_INTR_STAT_TX_ABRT))) {
		if (!transfer_ongoing) {
			/* Dummy write. */
			controller->ic_data_cmd = 0;

		} else {
			if (transfer->tx_len > i2c_write_pos[i2c]) {
				controller->ic_data_cmd =
				    transfer->tx[i2c_write_pos[i2c]++];
			}

			if (transfer->tx_len == i2c_write_pos[i2c]) {
				/*
				 * End user transfer if user does not update
				 * buffers.
				 */
				transfer_ongoing = false;

				if (transfer->callback) {
					transfer->callback(
					    transfer->callback_data, 0,
					    QM_I2C_TX_EMPTY, transfer->tx_len);
				}
			}
		}
	}
}

static __inline__ void
handle_tx_abrt(const qm_i2c_t i2c,
	       const volatile qm_i2c_transfer_t *const transfer,
	       qm_i2c_reg_t *const controller)
{
	qm_i2c_status_t status = 0;
	int rc = 0;

	QM_ASSERT(!(controller->ic_tx_abrt_source &
		    QM_I2C_IC_TX_ABRT_SOURCE_ABRT_SBYTE_NORSTRT));
	status =
	    (controller->ic_tx_abrt_source & QM_I2C_IC_TX_ABRT_SOURCE_ALL_MASK);

	/* Clear TX ABORT interrupt. */
	controller->ic_clr_tx_abrt;

	/* Mask interrupts. */
	controller->ic_intr_mask = QM_I2C_IC_INTR_MASK_ALL;

	rc = (status & QM_I2C_TX_ABRT_USER_ABRT) ? -ECANCELED : -EIO;

	if ((i2c_dma_context[i2c].ongoing_dma_rx_operation == true) ||
	    (i2c_dma_context[i2c].ongoing_dma_tx_operation == true)) {
		/* If in DMA mode, raise a flag and stop the channels. */
		i2c_dma_context[i2c].tx_abort_status = status;
		i2c_dma_context[i2c].i2c_error_code = rc;
		/*
		 * When terminating the DMA transfer, the DMA controller calls
		 * the TX or RX callback, which will trigger the error callback.
		 * This will disable the I2C controller.
		 */
		qm_i2c_dma_transfer_terminate(i2c);
	} else {
		controller_disable(i2c);
		if (transfer->callback) {
			transfer->callback(transfer->callback_data, rc, status,
					   i2c_write_pos[i2c]);
		}
	}
}

static __inline__ void
i2c_isr_slave_handler(const qm_i2c_t i2c,
		      const volatile qm_i2c_transfer_t *const transfer,
		      qm_i2c_reg_t *const controller)
{
	/* Save register to speed up process in interrupt. */
	uint32_t ic_intr_stat = controller->ic_intr_stat;

	/*
	 * Order of interrupt handling:
	 * - RX Status interrupts
	 * - TX Status interrupts (RD_REQ, RX_DONE, TX_EMPTY)
	 * - General call (will only appear after few SCL clock cycles after
	 *   start interrupt).
	 * - Stop (can appear very shortly after RX_DONE interrupt)
	 * - Start (can appear very shortly after a stop interrupt or RX_DONE
	 *   interrupt)
	 */

	/*
	 * Check RX status.
	 * Master write (TX), slave read (RX).
	 *
	 * Interrupts handled for RX status:
	 * - RX FIFO Overflow
	 * - RX FIFO Full (interrupt remains active until FIFO emptied)
	 *
	 * RX FIFO overflow must always be checked though, in case of an
	 * overflow happens during RX_FULL interrupt handling.
	 */
	/* RX FIFO Overflow. */
	if (ic_intr_stat & QM_I2C_IC_INTR_STAT_RX_OVER) {
		controller->ic_clr_rx_over;
		if (transfer->callback) {
			transfer->callback(transfer->callback_data, 0,
					   QM_I2C_RX_OVER, 0);
		}
	}

	/* RX FIFO FULL. */
	if (ic_intr_stat & QM_I2C_IC_INTR_STAT_RX_FULL) {
		/* Empty RX FIFO. */
		empty_rx_fifo(i2c, transfer, controller);

		/* Track activity of controller when addressed. */
		is_addressed = true;
	}

	/*
	 * Check TX status.
	 * Master read (RX), slave write (TX).
	 *
	 * Interrupts handled for TX status:
	 * - Read request
	 * - RX done (actually a TX state: RX done by master)
	 * - TX FIFO empty.
	 *
	 * TX FIFO empty interrupt must be handled after RX DONE interrupt: when
	 * RX DONE is triggered, TX FIFO is flushed (thus emptied) creating a
	 * TX_ABORT interrupt and a TX_EMPTY condition. TX_ABORT shall be
	 * cleared and TX_EMPTY interrupt disabled.
	 */
	else if (ic_intr_stat & QM_I2C_IC_INTR_STAT_RD_REQ) {
		/* Clear read request interrupt. */
		controller->ic_clr_rd_req;

		/* Track activity of controller when addressed. */
		is_addressed = true;

		fill_tx_fifo(i2c, transfer, controller);

		/* Enable TX EMPTY interrupts. */
		controller->ic_intr_mask |= QM_I2C_IC_INTR_MASK_TX_EMPTY;
	} else if (ic_intr_stat & QM_I2C_IC_INTR_STAT_RX_DONE) {
		controller->ic_clr_rx_done;
		/* Clear TX ABORT as it is triggered when FIFO is flushed. */
		controller->ic_clr_tx_abrt;

		/* Disable TX EMPTY interrupt. */
		controller->ic_intr_mask &= ~QM_I2C_IC_INTR_MASK_TX_EMPTY;

		/*
		 * Read again the interrupt status in case of a stop or a start
		 * interrupt has been triggered in the meantime.
		 */
		ic_intr_stat = controller->ic_intr_stat;

	} else if (ic_intr_stat & QM_I2C_IC_INTR_STAT_TX_EMPTY) {
		fill_tx_fifo(i2c, transfer, controller);
	}

	/* General call detected. */
	else if (ic_intr_stat & QM_I2C_IC_INTR_STAT_GEN_CALL_DETECTED) {
		if (transfer->callback) {
			transfer->callback(transfer->callback_data, 0,
					   QM_I2C_GEN_CALL_DETECTED, 0);
		}
#if (FIX_1)
		/*
		 * Workaround.
		 * The interrupt may not actually be cleared when register is
		 * read too early.
		 */
		while (controller->ic_intr_stat &
		       QM_I2C_IC_INTR_STAT_GEN_CALL_DETECTED) {
			/* Clear General call interrupt. */
			controller->ic_clr_gen_call;
		}
#else
		controller->ic_clr_gen_call;
#endif

		/* Track activity of controller when addressed. */
		is_addressed = true;
	}

	/* Stop condition detected. */
	if (ic_intr_stat & QM_I2C_IC_INTR_STAT_STOP_DETECTED) {
		/* Empty RX FIFO. */
		empty_rx_fifo(i2c, transfer, controller);

		/*
		 * Stop transfer if single transfer asked and controller has
		 * been addressed.
		 * Driver only knows it has been addressed if:
		 * - It already triggered an interrupt on TX_EMPTY or RX_FULL
		 * - Data was read from RX FIFO.
		 */
		if ((transfer->stop == true) &&
		    (is_addressed || (i2c_read_pos[i2c] != 0))) {
			controller_disable(i2c);
		}

		if (transfer->callback) {
			transfer->callback(
			    transfer->callback_data, 0, QM_I2C_STOP_DETECTED,
			    (transfer_ongoing) ? i2c_read_pos[i2c] : 0);
		}
		i2c_write_pos[i2c] = 0;
		i2c_read_pos[i2c] = 0;

		controller->ic_intr_mask &= ~QM_I2C_IC_INTR_MASK_TX_EMPTY;

		is_addressed = false;

		/* Clear stop interrupt. */
		controller->ic_clr_stop_det;

		/*
		 * Read again the interrupt status in case of a start interrupt
		 * has been triggered in the meantime.
		 */
		ic_intr_stat = controller->ic_intr_stat;
	}

	/*
	 * START or RESTART condition detected.
	 * The RESTART_DETECTED interrupt is not used as it is redundant with
	 * the START_DETECTED interrupt.
	 */
	if (ic_intr_stat & QM_I2C_IC_INTR_STAT_START_DETECTED) {
		empty_rx_fifo(i2c, transfer, controller);

		if (transfer->callback) {
			transfer->callback(
			    transfer->callback_data, 0, QM_I2C_START_DETECTED,
			    (transfer_ongoing) ? i2c_read_pos[i2c] : 0);
		}
		transfer_ongoing = true;
		i2c_write_pos[i2c] = 0;
		i2c_read_pos[i2c] = 0;

		/* Clear Start detected interrupt. */
		controller->ic_clr_start_det;
	}
}

static __inline__ void
i2c_isr_master_handler(const qm_i2c_t i2c,
		       const volatile qm_i2c_transfer_t *const transfer,
		       qm_i2c_reg_t *const controller)
{
	uint32_t ic_data_cmd = 0, count_tx = (QM_I2C_FIFO_SIZE - TX_TL);
	uint32_t read_buffer_remaining = transfer->rx_len - i2c_read_pos[i2c];
	uint32_t write_buffer_remaining = transfer->tx_len - i2c_write_pos[i2c];
	uint32_t missing_bytes;

	/* RX read from buffer. */
	if (controller->ic_intr_stat & QM_I2C_IC_INTR_STAT_RX_FULL) {

		while (read_buffer_remaining && controller->ic_rxflr) {
			transfer->rx[i2c_read_pos[i2c]] =
			    controller->ic_data_cmd;
			read_buffer_remaining--;
			i2c_read_pos[i2c]++;

			if (read_buffer_remaining == 0) {
				/*
				 * Mask RX full interrupt if transfer
				 * complete.
				 */
				controller->ic_intr_mask &=
				    ~(QM_I2C_IC_INTR_MASK_RX_FULL |
				      QM_I2C_IC_INTR_MASK_TX_EMPTY);

				if (transfer->stop) {
					controller_disable(i2c);
				}

				if (transfer->callback) {
					transfer->callback(
					    transfer->callback_data, 0,
					    QM_I2C_IDLE, i2c_read_pos[i2c]);
				}
			}
		}

		if (read_buffer_remaining > 0 &&
		    read_buffer_remaining < (RX_TL + 1)) {
			/*
			 * Adjust the RX threshold so the next 'RX_FULL'
			 * interrupt is generated when all the remaining
			 * data are received.
			 */
			controller->ic_rx_tl = read_buffer_remaining - 1;
		}

		/*
		 * RX_FULL INTR is autocleared when the buffer levels goes below
		 * the threshold.
		 */
	}

	if (controller->ic_intr_stat & QM_I2C_IC_INTR_STAT_TX_EMPTY) {

		if ((controller->ic_status & QM_I2C_IC_STATUS_TFE) &&
		    (transfer->tx != NULL) && (write_buffer_remaining == 0) &&
		    (read_buffer_remaining == 0)) {

			controller->ic_intr_mask &=
			    ~QM_I2C_IC_INTR_MASK_TX_EMPTY;

			/*
			 * If this is not a combined transaction, disable the
			 * controller now.
			 */
			if (transfer->stop) {
				controller_disable(i2c);
			}

			/* Callback. */
			if (transfer->callback) {
				transfer->callback(transfer->callback_data, 0,
						   QM_I2C_IDLE,
						   i2c_write_pos[i2c]);
			}
		}

		while ((count_tx) && write_buffer_remaining) {
			count_tx--;
			write_buffer_remaining--;

			/*
			 * Write command -IC_DATA_CMD[8] = 0.
			 * Fill IC_DATA_CMD[7:0] with the data.
			 */
			ic_data_cmd = transfer->tx[i2c_write_pos[i2c]];

			/*
			 * If transfer is a combined transfer, only send stop at
			 * end of the transfer sequence.
			 */
			if (transfer->stop && (write_buffer_remaining == 0) &&
			    (read_buffer_remaining == 0)) {

				ic_data_cmd |= QM_I2C_IC_DATA_CMD_STOP_BIT_CTRL;
			}

			/* Write data. */
			controller->ic_data_cmd = ic_data_cmd;
			i2c_write_pos[i2c]++;

			/*
			 * TX_EMPTY INTR is autocleared when the buffer levels
			 * goes above the threshold.
			 */
		}

		/*
		 * If missing_bytes is not null, then that means we are already
		 * waiting for some bytes after sending read request on the
		 * previous interruption. We have to take into account this
		 * value in order to not send too much request so we won't fall
		 * into rx overflow.
		 */
		missing_bytes = read_buffer_remaining - i2c_read_cmd_send[i2c];

		/*
		 * Sanity check: The number of read data but not processed
		 * cannot be more than the number of expected bytes.
		 */
		QM_ASSERT(controller->ic_rxflr <= missing_bytes);

		/* Count_tx is the remaining size in the FIFO. */
		count_tx = QM_I2C_FIFO_SIZE - controller->ic_txflr;

		if (count_tx > missing_bytes) {
			count_tx -= missing_bytes;
		} else {
			count_tx = 0;
		}

		while (i2c_read_cmd_send[i2c] &&
		       (write_buffer_remaining == 0) && count_tx) {
			count_tx--;
			i2c_read_cmd_send[i2c]--;

			/*
			 * If transfer is a combined transfer, only send stop at
			 * end of the transfer sequence.
			 */
			if (transfer->stop && (i2c_read_cmd_send[i2c] == 0)) {
				controller->ic_data_cmd =
				    QM_I2C_IC_DATA_CMD_READ |
				    QM_I2C_IC_DATA_CMD_STOP_BIT_CTRL;
			} else {
				controller->ic_data_cmd =
				    QM_I2C_IC_DATA_CMD_READ;
			}
		}

		/* Generate a tx_empty interrupt when TX FIFO is fully empty. */
		if ((write_buffer_remaining == 0) &&
		    (read_buffer_remaining == 0)) {
			controller->ic_tx_tl = 0;
		}
	}
}

static void i2c_isr_handler(const qm_i2c_t i2c)
{
	const volatile qm_i2c_transfer_t *const transfer = i2c_transfer[i2c];
	qm_i2c_reg_t *const controller = QM_I2C[i2c];

	/* Check for errors. */
	QM_ASSERT(!(controller->ic_intr_stat & QM_I2C_IC_INTR_STAT_TX_OVER));
	QM_ASSERT(!(controller->ic_intr_stat & QM_I2C_IC_INTR_STAT_RX_UNDER));

	/*
	 * TX ABORT interrupt.
	 * Avoid spurious interrupts by checking RX DONE interrupt: RX_DONE
	 * interrupt also trigger a TX_ABORT interrupt when flushing FIFO.
	 */
	if ((controller->ic_intr_stat &
	     (QM_I2C_IC_INTR_STAT_TX_ABRT | QM_I2C_IC_INTR_STAT_RX_DONE)) ==
	    QM_I2C_IC_INTR_STAT_TX_ABRT) {
		handle_tx_abrt(i2c, transfer, controller);
	}

	/* Master mode. */
	if (controller->ic_con & QM_I2C_IC_CON_MASTER_MODE) {
		QM_ASSERT(
		    !(controller->ic_intr_stat & QM_I2C_IC_INTR_STAT_RX_OVER));
		i2c_isr_master_handler(i2c, transfer, controller);
	}
	/* Slave mode. */
	else {
		i2c_isr_slave_handler(i2c, transfer, controller);
	}
}

QM_ISR_DECLARE(qm_i2c_0_isr)
{
	i2c_isr_handler(QM_I2C_0);
	QM_ISR_EOI(QM_IRQ_I2C_0_INT_VECTOR);
}

#if (QUARK_SE)
QM_ISR_DECLARE(qm_i2c_1_isr)
{
	i2c_isr_handler(QM_I2C_1);
	QM_ISR_EOI(QM_IRQ_I2C_1_INT_VECTOR);
}
#endif

static uint32_t get_lo_cnt(uint32_t lo_time_ns)
{
	return ((((clk_sys_get_ticks_per_us() >>
		   ((QM_SCSS_CCU->ccu_periph_clk_div_ctl0 &
		     CLK_PERIPH_DIV_DEF_MASK) >>
		    QM_CCU_PERIPH_PCLK_DIV_OFFSET)) *
		  lo_time_ns) /
		 1000) -
		1);
}

static uint32_t get_hi_cnt(qm_i2c_t i2c, uint32_t hi_time_ns)
{
	/*
	 * Generated SCL HIGH period is less than the expected SCL clock HIGH
	 * period in the Master receiver mode.
	 * Summary: workaround is +1 to hcnt.
	 */
	return (((((clk_sys_get_ticks_per_us() >>
		    ((QM_SCSS_CCU->ccu_periph_clk_div_ctl0 &
		      CLK_PERIPH_DIV_DEF_MASK) >>
		     QM_CCU_PERIPH_PCLK_DIV_OFFSET)) *
		   hi_time_ns) /
		  1000) -
		 7 - QM_I2C[i2c]->ic_fs_spklen) +
		1);
}

int qm_i2c_set_config(const qm_i2c_t i2c, const qm_i2c_config_t *const cfg)
{
	uint32_t lcnt = 0, hcnt = 0, min_lcnt = 0, lcnt_diff = 0, ic_con = 0;
	QM_CHECK(i2c < QM_I2C_NUM, -EINVAL);
	QM_CHECK(cfg != NULL, -EINVAL);

	qm_i2c_reg_t *const controller = QM_I2C[i2c];

	/* Mask all interrupts. */
	controller->ic_intr_mask = QM_I2C_IC_INTR_MASK_ALL;

	/* Disable controller. */
	if (controller_disable(i2c)) {
		return -EBUSY;
	}

	switch (cfg->mode) {
	case QM_I2C_MASTER:
		/* Set mode. */
		ic_con = QM_I2C_IC_CON_MASTER_MODE | QM_I2C_IC_CON_RESTART_EN |
			 QM_I2C_IC_CON_SLAVE_DISABLE |
			 /* Set 7/10 bit address mode. */
			 (cfg->address_mode
			  << QM_I2C_IC_CON_10BITADDR_MASTER_OFFSET);

		/*
		 * Timing generation algorithm:
		 * 1. compute hi/lo count so as to achieve the desired bus speed
		 *    at 50% duty cycle
		 * 2. adjust the hi/lo count to ensure that minimum hi/lo
		 *    timings are guaranteed as per spec.
		 */

		switch (cfg->speed) {
		case QM_I2C_SPEED_STD:

			ic_con |= QM_I2C_IC_CON_SPEED_SS;

			controller->ic_fs_spklen = SPK_LEN_SS;

			min_lcnt = get_lo_cnt(QM_I2C_MIN_SS_NS);
			lcnt = get_lo_cnt(QM_I2C_SS_50_DC_NS);
			hcnt = get_hi_cnt(i2c, QM_I2C_SS_50_DC_NS);
			break;

		case QM_I2C_SPEED_FAST:
			ic_con |= QM_I2C_IC_CON_SPEED_FS_FSP;

			controller->ic_fs_spklen = SPK_LEN_FS_FSP;

			min_lcnt = get_lo_cnt(QM_I2C_MIN_FS_NS);
			lcnt = get_lo_cnt(QM_I2C_FS_50_DC_NS);
			hcnt = get_hi_cnt(i2c, QM_I2C_FS_50_DC_NS);
			break;

		case QM_I2C_SPEED_FAST_PLUS:
			ic_con |= QM_I2C_IC_CON_SPEED_FS_FSP;

			controller->ic_fs_spklen = SPK_LEN_FS_FSP;

			min_lcnt = get_lo_cnt(QM_I2C_MIN_FSP_NS);
			lcnt = get_lo_cnt(QM_I2C_FSP_50_DC_NS);
			hcnt = get_hi_cnt(i2c, QM_I2C_FSP_50_DC_NS);
			break;
		}

		if (hcnt > QM_I2C_IC_HCNT_MAX || hcnt < QM_I2C_IC_HCNT_MIN) {
			return -EINVAL;
		}

		if (lcnt > QM_I2C_IC_LCNT_MAX || lcnt < QM_I2C_IC_LCNT_MIN) {
			return -EINVAL;
		}

		/* Increment minimum low count to account for rounding down. */
		min_lcnt++;
		if (lcnt < min_lcnt) {
			lcnt_diff = (min_lcnt - lcnt);
			lcnt += (lcnt_diff);
			hcnt -= (lcnt_diff);
		}
		if (QM_I2C_SPEED_STD == cfg->speed) {
			controller->ic_ss_scl_lcnt = lcnt;
			controller->ic_ss_scl_hcnt = hcnt;
		} else {
			controller->ic_fs_scl_hcnt = hcnt;
			controller->ic_fs_scl_lcnt = lcnt;
		}

		break;

	case QM_I2C_SLAVE:
		/*
		 * QM_I2C_IC_CON_MASTER_MODE and QM_I2C_IC_CON_SLAVE_DISABLE are
		 * deasserted.
		 */

		/* Set 7/10 bit address mode. */
		ic_con = cfg->address_mode
			 << QM_I2C_IC_CON_10BITADDR_SLAVE_OFFSET;

		if (cfg->stop_detect_behaviour ==
		    QM_I2C_SLAVE_INTERRUPT_WHEN_ADDRESSED) {
			/* Set stop interrupt only when addressed. */
			ic_con |= QM_I2C_IC_CON_STOP_DET_IFADDRESSED;
		}

		/* Set slave address. */
		controller->ic_sar = cfg->slave_addr;
		break;
	}

	controller->ic_con = ic_con;
	return 0;
}

int qm_i2c_set_speed(const qm_i2c_t i2c, const qm_i2c_speed_t speed,
		     const uint16_t lo_cnt, const uint16_t hi_cnt)
{
	QM_CHECK(i2c < QM_I2C_NUM, -EINVAL);
	QM_CHECK(hi_cnt < QM_I2C_IC_HCNT_MAX && lo_cnt > QM_I2C_IC_HCNT_MIN,
		 -EINVAL);
	QM_CHECK(lo_cnt < QM_I2C_IC_LCNT_MAX && lo_cnt > QM_I2C_IC_LCNT_MIN,
		 -EINVAL);

	qm_i2c_reg_t *const controller = QM_I2C[i2c];

	uint32_t ic_con = controller->ic_con;
	ic_con &= ~QM_I2C_IC_CON_SPEED_MASK;

	switch (speed) {
	case QM_I2C_SPEED_STD:
		ic_con |= QM_I2C_IC_CON_SPEED_SS;
		controller->ic_ss_scl_lcnt = lo_cnt;
		controller->ic_ss_scl_hcnt = hi_cnt;
		controller->ic_fs_spklen = SPK_LEN_SS;
		break;

	case QM_I2C_SPEED_FAST:
	case QM_I2C_SPEED_FAST_PLUS:
		ic_con |= QM_I2C_IC_CON_SPEED_FS_FSP;
		controller->ic_fs_scl_lcnt = lo_cnt;
		controller->ic_fs_scl_hcnt = hi_cnt;
		controller->ic_fs_spklen = SPK_LEN_FS_FSP;
		break;
	}

	controller->ic_con = ic_con;

	return 0;
}

int qm_i2c_get_status(const qm_i2c_t i2c, qm_i2c_status_t *const status)
{
	QM_CHECK(i2c < QM_I2C_NUM, -EINVAL);
	QM_CHECK(status != NULL, -EINVAL);

	qm_i2c_reg_t *const controller = QM_I2C[i2c];

	*status = 0;

	/* Check if slave or master are active. */
	if (controller->ic_status & QM_I2C_IC_STATUS_BUSY_MASK) {
		*status = QM_I2C_BUSY;
	}

	/* Check for abort status. */
	*status |=
	    (controller->ic_tx_abrt_source & QM_I2C_IC_TX_ABRT_SOURCE_ALL_MASK);

	return 0;
}

int qm_i2c_master_write(const qm_i2c_t i2c, const uint16_t slave_addr,
			const uint8_t *const data, uint32_t len,
			const bool stop, qm_i2c_status_t *const status)
{
	uint8_t *d = (uint8_t *)data;
	uint32_t ic_data_cmd = 0;
	int ret = 0;

	QM_CHECK(i2c < QM_I2C_NUM, -EINVAL);
	QM_CHECK(data != NULL, -EINVAL);
	QM_CHECK(len > 0, -EINVAL);

	qm_i2c_reg_t *const controller = QM_I2C[i2c];

	/* Write slave address to TAR. */
	controller->ic_tar = slave_addr;

	/* Enable controller. */
	controller_enable(i2c);

	while (len--) {

		/* Wait if FIFO is full. */
		while (!(controller->ic_status & QM_I2C_IC_STATUS_TNF))
			;

		/*
		 * Write command -IC_DATA_CMD[8] = 0.
		 * Fill IC_DATA_CMD[7:0] with the data.
		 */
		ic_data_cmd = *d;

		/* Send stop after last byte. */
		if (len == 0 && stop) {
			ic_data_cmd |= QM_I2C_IC_DATA_CMD_STOP_BIT_CTRL;
		}

		controller->ic_data_cmd = ic_data_cmd;
		d++;
	}

	/*
	 * This is a blocking call, wait until FIFO is empty or tx abrt error.
	 */
	while (!(controller->ic_status & QM_I2C_IC_STATUS_TFE))
		;

	if (controller->ic_tx_abrt_source & QM_I2C_IC_TX_ABRT_SOURCE_ALL_MASK) {
		ret = -EIO;
	}

	/*  Disable controller. */
	if (true == stop) {
		if (controller_disable(i2c)) {
			ret = -EBUSY;
		}
	}

	if (status != NULL) {
		qm_i2c_get_status(i2c, status);
	}

	/*
	 * Clear abort status.
	 * The controller flushes/resets/empties the TX FIFO whenever this bit
	 * is set. The TX FIFO remains in this flushed state until the
	 * register IC_CLR_TX_ABRT is read.
	 */
	controller->ic_clr_tx_abrt;

	return ret;
}

int qm_i2c_master_read(const qm_i2c_t i2c, const uint16_t slave_addr,
		       uint8_t *const data, uint32_t len, const bool stop,
		       qm_i2c_status_t *const status)
{
	uint8_t *d = (uint8_t *)data;
	int ret = 0;

	QM_CHECK(i2c < QM_I2C_NUM, -EINVAL);
	QM_CHECK(data != NULL, -EINVAL);
	QM_CHECK(len > 0, -EINVAL);

	qm_i2c_reg_t *const controller = QM_I2C[i2c];

	/* Write slave address to TAR. */
	controller->ic_tar = slave_addr;

	/* Enable controller. */
	controller_enable(i2c);

	while (len--) {
		if (len == 0 && stop) {
			controller->ic_data_cmd =
			    QM_I2C_IC_DATA_CMD_READ |
			    QM_I2C_IC_DATA_CMD_STOP_BIT_CTRL;
		}

		else {
			/*  Read command -IC_DATA_CMD[8] = 1. */
			controller->ic_data_cmd = QM_I2C_IC_DATA_CMD_READ;
		}

		/* Wait if RX FIFO is empty, break if TX empty and error. */
		while (!(controller->ic_status & QM_I2C_IC_STATUS_RFNE)) {

			if (controller->ic_raw_intr_stat &
			    QM_I2C_IC_RAW_INTR_STAT_TX_ABRT) {
				break;
			}
		}

		if (controller->ic_tx_abrt_source &
		    QM_I2C_IC_TX_ABRT_SOURCE_ALL_MASK) {
			ret = -EIO;
			break;
		}
		/* IC_DATA_CMD[7:0] contains received data. */
		*d = controller->ic_data_cmd;
		d++;
	}

	/*  Disable controller. */
	if (true == stop) {
		if (controller_disable(i2c)) {
			ret = -EBUSY;
		}
	}

	if (status != NULL) {
		qm_i2c_get_status(i2c, status);
	}

	/*
	 * Clear abort status.
	 * The controller flushes/resets/empties the TX FIFO whenever this bit
	 * is set. The TX FIFO remains in this flushed state until the
	 * register IC_CLR_TX_ABRT is read.
	 */
	controller->ic_clr_tx_abrt;

	return ret;
}

int qm_i2c_master_irq_transfer(const qm_i2c_t i2c,
			       const qm_i2c_transfer_t *const xfer,
			       const uint16_t slave_addr)
{
	QM_CHECK(i2c < QM_I2C_NUM, -EINVAL);
	QM_CHECK(NULL != xfer, -EINVAL);

	qm_i2c_reg_t *const controller = QM_I2C[i2c];

	/* Write slave address to TAR. */
	controller->ic_tar = slave_addr;

	i2c_write_pos[i2c] = 0;
	i2c_read_pos[i2c] = 0;
	i2c_read_cmd_send[i2c] = xfer->rx_len;
	i2c_transfer[i2c] = xfer;

	/* Set threshold. */
	controller->ic_tx_tl = TX_TL;
	if (xfer->rx_len > 0 && xfer->rx_len < (RX_TL + 1)) {
		/*
		 * If 'rx_len' is less than the default threshold, we have to
		 * change the threshold value so the 'RX FULL' interrupt is
		 * generated once all data from the transfer is received.
		 */
		controller->ic_rx_tl = xfer->rx_len - 1;
	} else {
		controller->ic_rx_tl = RX_TL;
	}

	/* Mask interrupts. */
	QM_I2C[i2c]->ic_intr_mask = QM_I2C_IC_INTR_MASK_ALL;

	/* Enable controller. */
	controller_enable(i2c);

	/* Unmask interrupts. */
	controller->ic_intr_mask |=
	    QM_I2C_IC_INTR_MASK_RX_UNDER | QM_I2C_IC_INTR_MASK_RX_OVER |
	    QM_I2C_IC_INTR_MASK_RX_FULL | QM_I2C_IC_INTR_MASK_TX_OVER |
	    QM_I2C_IC_INTR_MASK_TX_EMPTY | QM_I2C_IC_INTR_MASK_TX_ABORT;

	return 0;
}

int qm_i2c_slave_irq_transfer(const qm_i2c_t i2c,
			      volatile const qm_i2c_transfer_t *const xfer)
{
	QM_CHECK(i2c < QM_I2C_NUM, -EINVAL);
	QM_CHECK(xfer != NULL, -EINVAL);

	/* Assign common properties. */
	i2c_transfer[i2c] = xfer;
	i2c_write_pos[i2c] = 0;
	i2c_read_pos[i2c] = 0;

	transfer_ongoing = false;
	is_addressed = false;

	QM_I2C[i2c]->ic_intr_mask = QM_I2C_IC_INTR_MASK_ALL;

	controller_enable(i2c);

	/*
	 * Almost all interrupts must be active to handle everything from the
	 * driver, for the controller not to be stuck in a specific state.
	 * Only TX_EMPTY must be set when needed, otherwise it will be triggered
	 * everytime, even when it is not required to fill the TX FIFO.
	 */
	QM_I2C[i2c]->ic_intr_mask =
	    QM_I2C_IC_INTR_MASK_RX_UNDER | QM_I2C_IC_INTR_MASK_RX_OVER |
	    QM_I2C_IC_INTR_MASK_RX_FULL | QM_I2C_IC_INTR_MASK_TX_ABORT |
	    QM_I2C_IC_INTR_MASK_RX_DONE | QM_I2C_IC_INTR_MASK_STOP_DETECTED |
	    QM_I2C_IC_INTR_MASK_START_DETECTED | QM_I2C_IC_INTR_MASK_RD_REQ |
	    QM_I2C_IC_INTR_MASK_GEN_CALL_DETECTED;

	return 0;
}

int qm_i2c_slave_irq_transfer_update(
    const qm_i2c_t i2c, volatile const qm_i2c_transfer_t *const xfer)
{
	QM_CHECK(i2c < QM_I2C_NUM, -EINVAL);
	QM_CHECK(xfer != NULL, -EINVAL);

	/* Assign common properties. */
	i2c_transfer[i2c] = xfer;
	i2c_write_pos[i2c] = 0;
	i2c_read_pos[i2c] = 0;

	/* Tell the ISR we still have data to transfer. */
	transfer_ongoing = true;

	return 0;
}

static void controller_enable(const qm_i2c_t i2c)
{
	qm_i2c_reg_t *const controller = QM_I2C[i2c];

	if (!(controller->ic_enable_status & QM_I2C_IC_ENABLE_STATUS_IC_EN)) {
		/* Enable controller. */
		controller->ic_enable |= QM_I2C_IC_ENABLE_CONTROLLER_EN;

		/* Wait until controller is enabled. */
		while (!(controller->ic_enable_status &
			 QM_I2C_IC_ENABLE_STATUS_IC_EN))
			;
	}

	/* Be sure that all interrupts flag are cleared. */
	controller->ic_clr_intr;
}

static int controller_disable(const qm_i2c_t i2c)
{
	qm_i2c_reg_t *const controller = QM_I2C[i2c];
	int poll_count = I2C_POLL_COUNT;

	/* Disable controller. */
	controller->ic_enable &= ~QM_I2C_IC_ENABLE_CONTROLLER_EN;

	/* Wait until controller is disabled. */
	while ((controller->ic_enable_status & QM_I2C_IC_ENABLE_STATUS_IC_EN) &&
	       poll_count--) {
		clk_sys_udelay(I2C_POLL_MICROSECOND);
	}

	/* Returns 0 if ok, meaning controller is disabled. */
	return (controller->ic_enable_status & QM_I2C_IC_ENABLE_STATUS_IC_EN);
}

int qm_i2c_irq_transfer_terminate(const qm_i2c_t i2c)
{
	QM_CHECK(i2c < QM_I2C_NUM, -EINVAL);

	/* Abort:
	 * In response to an ABORT, the controller issues a STOP and flushes the
	 * Tx FIFO after completing the current transfer, then sets the TX_ABORT
	 * interrupt after the abort operation. The ABORT bit is cleared
	 * automatically by hardware after the abort operation.
	 */
	QM_I2C[i2c]->ic_enable |= QM_I2C_IC_ENABLE_CONTROLLER_ABORT;

	return 0;
}

/*
 * Stops DMA channel and terminates I2C transfer.
 */
int qm_i2c_dma_transfer_terminate(const qm_i2c_t i2c)
{
	int rc = 0;

	QM_CHECK(i2c < QM_I2C_NUM, -EINVAL);

	if (i2c_dma_context[i2c].ongoing_dma_tx_operation) {
		/* First terminate the DMA transfer. */
		rc = qm_dma_transfer_terminate(
		    i2c_dma_context[i2c].dma_controller_id,
		    i2c_dma_context[i2c].dma_tx_channel_id);
	}

	if (i2c_dma_context[i2c].ongoing_dma_rx_operation) {
		/* First terminate the DMA transfer. */
		rc |= qm_dma_transfer_terminate(
		    i2c_dma_context[i2c].dma_controller_id,
		    i2c_dma_context[i2c].dma_rx_channel_id);
	}

	/* Check if any of the calls failed. */
	if (rc != 0) {
		rc = -EIO;
	}

	return rc;
}

/*
 * Disable TX and/or RX and call user error callback if provided.
 */
static void i2c_dma_transfer_error_callback(uint32_t i2c, int error_code,
					    uint32_t len)
{
	const volatile qm_i2c_transfer_t *const transfer = i2c_transfer[i2c];

	if (error_code != 0) {
		if (i2c_dma_context[i2c].ongoing_dma_tx_operation == true) {
			/* Disable DMA transmit. */
			QM_I2C[i2c]->ic_dma_cr &= ~QM_I2C_IC_DMA_CR_TX_ENABLE;
			i2c_dma_context[i2c].ongoing_dma_tx_operation = false;
		}

		if (i2c_dma_context[i2c].ongoing_dma_rx_operation == true) {
			/* Disable DMA receive. */
			QM_I2C[i2c]->ic_dma_cr &= ~QM_I2C_IC_DMA_CR_RX_ENABLE;
			i2c_dma_context[i2c].ongoing_dma_rx_operation = false;
		}

		/* Disable the controller. */
		controller_disable(i2c);

		/* If the user has provided a callback, let's call it. */
		if (transfer->callback != NULL) {
			transfer->callback(transfer->callback_data, error_code,
					   i2c_dma_context[i2c].tx_abort_status,
					   len);
		}
	}
}

/*
 * After a TX operation, a stop condition may need to be issued along the last
 * data byte, so that byte is handled here. DMA TX mode does also need to be
 * disabled.
 */
static void i2c_dma_transmit_callback(void *callback_context, uint32_t len,
				      int error_code)
{
	qm_i2c_status_t status;

	qm_i2c_t i2c = ((i2c_dma_context_t *)callback_context)->i2c;
	const volatile qm_i2c_transfer_t *const transfer = i2c_transfer[i2c];

	if ((error_code == 0) && (i2c_dma_context[i2c].i2c_error_code == 0)) {
		/* Disable DMA transmit. */
		QM_I2C[i2c]->ic_dma_cr &= ~QM_I2C_IC_DMA_CR_TX_ENABLE;
		i2c_dma_context[i2c].ongoing_dma_tx_operation = false;

		/*
		 * As the callback is used for both real TX and read command
		 * "TX" during an RX operation, we need to know which case we
		 * are in.
		 */
		if (i2c_dma_context[i2c].ongoing_dma_rx_operation == false) {
			/* Write last byte */
			uint32_t data_command =
			    QM_I2C_IC_DATA_CMD_LSB_MASK &
			    ((uint8_t *)i2c_dma_context[i2c]
				 .dma_tx_transfer_config.source_address)
				[i2c_dma_context[i2c]
				     .dma_tx_transfer_config.block_size];

			/*
			 * Check if we must issue a stop condition and it's not
			 * a combined transaction, or bytes transfered are less
			 * than expected.
			 */
			if (((transfer->stop == true) &&
			     (transfer->rx_len == 0)) ||
			    (len != transfer->tx_len - 1)) {
				data_command |=
				    QM_I2C_IC_DATA_CMD_STOP_BIT_CTRL;
			}

			/* Wait if FIFO is full */
			while (!(QM_I2C[i2c]->ic_status & QM_I2C_IC_STATUS_TNF))
				;
			/* Write last byte and increase len count */
			QM_I2C[i2c]->ic_data_cmd = data_command;
			len++;

			/*
			 * Check if there is a pending read operation, meaning
			 * this is a combined transaction, and transfered data
			 * length is the expected.
			 */
			if ((transfer->rx_len > 0) &&
			    (len == transfer->tx_len)) {
				i2c_start_dma_read(i2c);
			} else {
				/*
				 * Let's disable the I2C controller if we are
				 * done.
				 */
				if ((transfer->stop == true) ||
				    (len != transfer->tx_len)) {
					/*
					 * This callback is called when DMA is
					 * done, but I2C can still be
					 * transmitting, so let's wait until all
					 * data is sent.
					 */

					while (!(QM_I2C[i2c]->ic_status &
						 QM_I2C_IC_STATUS_TFE)) {
					}
					controller_disable(i2c);
				}
				/*
				 * If user provided a callback, it'll be called
				 * only if this is a TX only operation, not in a
				 * combined transaction.
				 */
				if (transfer->callback != NULL) {
					qm_i2c_get_status(i2c, &status);
					transfer->callback(
					    transfer->callback_data, error_code,
					    status, len);
				}
			}
		}
	} else {
		/*
		 * If error code is 0, a multimaster arbitration loss has
		 * happened, so use it as error code.
		 */
		if (error_code == 0) {
			error_code = i2c_dma_context[i2c].i2c_error_code;
		}

		i2c_dma_transfer_error_callback(i2c, error_code, len);
	}
}

/*
 * After an RX operation, we need to disable DMA RX mode.
 */
static void i2c_dma_receive_callback(void *callback_context, uint32_t len,
				     int error_code)
{
	qm_i2c_status_t status;

	qm_i2c_t i2c = ((i2c_dma_context_t *)callback_context)->i2c;
	const volatile qm_i2c_transfer_t *const transfer = i2c_transfer[i2c];

	if ((error_code == 0) && (i2c_dma_context[i2c].i2c_error_code == 0)) {
		/* Disable DMA receive */
		QM_I2C[i2c]->ic_dma_cr &= ~QM_I2C_IC_DMA_CR_RX_ENABLE;
		i2c_dma_context[i2c].ongoing_dma_rx_operation = false;

		/* Let's disable the I2C controller if we are done. */
		if (transfer->stop == true) {
			controller_disable(i2c);
		}

		/* If the user has provided a callback, let's call it. */
		if (transfer->callback != NULL) {
			qm_i2c_get_status(i2c, &status);
			transfer->callback(transfer->callback_data, error_code,
					   status, len);
		}
	} else {
		/*
		 * Only call the error callback on RX error.
		 * Arbitration loss errors are handled on the TX callback.
		 */
		if (error_code != 0) {
			i2c_dma_transfer_error_callback(i2c, error_code, len);
		}
	}
}

/*
 * Effectively starts a previously configured read operation.
 * For doing this, 2 DMA channels are needed, one for writting READ commands to
 * the I2C controller and the other to get the read data from the I2C controller
 * to memory. Thus, a TX operation is also needed in order to perform an RX.
 */
static int i2c_start_dma_read(const qm_i2c_t i2c)
{
	int rc = 0;

	/* Enable DMA transmit and receive. */
	QM_I2C[i2c]->ic_dma_cr =
	    QM_I2C_IC_DMA_CR_RX_ENABLE | QM_I2C_IC_DMA_CR_TX_ENABLE;

	/* Enable controller. */
	controller_enable(i2c);

	/* A RX operation need to read and write. */
	i2c_dma_context[i2c].ongoing_dma_rx_operation = true;
	i2c_dma_context[i2c].ongoing_dma_tx_operation = true;
	/* Configure DMA TX for writing READ commands. */
	rc = qm_dma_transfer_set_config(
	    i2c_dma_context[i2c].dma_controller_id,
	    i2c_dma_context[i2c].dma_tx_channel_id,
	    &(i2c_dma_context[i2c].dma_cmd_transfer_config));
	if (rc == 0) {
		/* Configure DMA RX. */
		rc = qm_dma_transfer_set_config(
		    i2c_dma_context[i2c].dma_controller_id,
		    i2c_dma_context[i2c].dma_rx_channel_id,
		    &(i2c_dma_context[i2c].dma_rx_transfer_config));
		if (rc == 0) {
			/* Start both transfers "at once". */
			rc = qm_dma_transfer_start(
			    i2c_dma_context[i2c].dma_controller_id,
			    i2c_dma_context[i2c].dma_tx_channel_id);
			if (rc == 0) {
				rc = qm_dma_transfer_start(
				    i2c_dma_context[i2c].dma_controller_id,
				    i2c_dma_context[i2c].dma_rx_channel_id);
			}
		}
	}

	return rc;
}

/*
 * Configures given DMA channel with the appropriate width and
 * length and the right handshaking interface and callback depending on the
 * direction.
 */
int qm_i2c_dma_channel_config(const qm_i2c_t i2c,
			      const qm_dma_t dma_controller_id,
			      const qm_dma_channel_id_t channel_id,
			      const qm_dma_channel_direction_t direction)
{
	qm_dma_channel_config_t dma_channel_config = {0};
	int rc = 0;

	/* Test input values. */
	QM_CHECK(i2c < QM_I2C_NUM, -EINVAL);
	QM_CHECK(channel_id < QM_DMA_CHANNEL_NUM, -EINVAL);
	QM_CHECK(dma_controller_id < QM_DMA_NUM, -EINVAL);
	QM_CHECK(direction <= QM_DMA_PERIPHERAL_TO_MEMORY, -EINVAL);
	QM_CHECK(direction >= QM_DMA_MEMORY_TO_PERIPHERAL, -EINVAL);

	/* Set DMA channel configuration. */
	dma_channel_config.handshake_interface =
	    i2c_dma_interfaces[i2c][direction];
	dma_channel_config.handshake_polarity = QM_DMA_HANDSHAKE_POLARITY_HIGH;
	dma_channel_config.channel_direction = direction;
	dma_channel_config.source_transfer_width = QM_DMA_TRANS_WIDTH_8;
	dma_channel_config.destination_transfer_width = QM_DMA_TRANS_WIDTH_8;
	/* Burst length is set to half the FIFO for performance */
	dma_channel_config.source_burst_length = QM_DMA_BURST_TRANS_LENGTH_8;
	dma_channel_config.destination_burst_length =
	    QM_DMA_BURST_TRANS_LENGTH_8;
	dma_channel_config.client_callback = i2c_dma_callbacks[direction];
	dma_channel_config.transfer_type = QM_DMA_TYPE_SINGLE;

	/* Hold the channel IDs and controller ID in the DMA context. */
	if (direction == QM_DMA_PERIPHERAL_TO_MEMORY) {
		i2c_dma_context[i2c].dma_rx_channel_id = channel_id;
	} else {
		i2c_dma_context[i2c].dma_tx_channel_id = channel_id;
	}
	i2c_dma_context[i2c].dma_controller_id = dma_controller_id;
	i2c_dma_context[i2c].i2c = i2c;
	dma_channel_config.callback_context = &i2c_dma_context[i2c];

	/* Configure DMA channel. */
	rc = qm_dma_channel_set_config(dma_controller_id, channel_id,
				       &dma_channel_config);

	return rc;
}

/*
 * Setups and starts a DMA transaction, wether it's read, write or combined one.
 * In case of combined transaction, it sets up both operations and starts the
 * write one; the read operation will be started in the read operation
 * callback.
 */
int qm_i2c_master_dma_transfer(const qm_i2c_t i2c,
			       qm_i2c_transfer_t *const xfer,
			       const uint16_t slave_addr)
{
	int rc = 0;
	uint32_t i;

	QM_CHECK(i2c < QM_I2C_NUM, -EINVAL);
	QM_CHECK(NULL != xfer, -EINVAL);
	QM_CHECK(0 < xfer->tx_len ? xfer->tx != NULL : 1, -EINVAL);
	QM_CHECK(0 < xfer->rx_len ? xfer->rx != NULL : 1, -EINVAL);
	QM_CHECK(0 == xfer->rx_len ? xfer->tx_len != 0 : 1, -EINVAL);

	/* Disable all IRQs but the TX abort one. */
	QM_I2C[i2c]->ic_intr_mask = QM_I2C_IC_INTR_MASK_TX_ABORT;

	/* Write slave address to TAR. */
	QM_I2C[i2c]->ic_tar = slave_addr;

	i2c_read_cmd_send[i2c] = xfer->rx_len;
	i2c_transfer[i2c] = xfer;

	/* Set DMA TX and RX waterlevels to half the FIFO depth for performance
	   reasons */
	QM_I2C[i2c]->ic_dma_tdlr = (QM_I2C_FIFO_SIZE / 2);
	/* RDLR value is desired watermark-1, according to I2C datasheet section
	   3.17.7 */
	QM_I2C[i2c]->ic_dma_rdlr = (QM_I2C_FIFO_SIZE / 2) - 1;

	i2c_dma_context[i2c].i2c_error_code = 0;

	/* Setup RX if something to receive. */
	if (xfer->rx_len > 0) {
		i2c_dma_context[i2c].dma_rx_transfer_config.block_size =
		    xfer->rx_len;
		i2c_dma_context[i2c].dma_rx_transfer_config.source_address =
		    (uint32_t *)&(QM_I2C[i2c]->ic_data_cmd);
		i2c_dma_context[i2c]
		    .dma_rx_transfer_config.destination_address =
		    (uint32_t *)(xfer->rx);

		/*
		 * For receiving, READ commands need to be written, a TX
		 * transfer is needed for writting them.
		 */
		i2c_dma_context[i2c].dma_cmd_transfer_config.block_size =
		    xfer->rx_len;
		i2c_dma_context[i2c].dma_cmd_transfer_config.source_address =
		    (uint32_t *)(xfer->rx);
		/*
		 * RX buffer will be filled with READ commands and use it as
		 * source for the READ command write operation. As READ commands
		 * are written, data will be read overwriting the already
		 * written READ commands.
		 */
		for (
		    i = 0;
		    i < i2c_dma_context[i2c].dma_cmd_transfer_config.block_size;
		    i++) {
			((uint8_t *)xfer->rx)[i] =
			    DATA_COMMAND_READ_COMMAND_BYTE;
		}
		/*
		 * The STOP condition will be issued on the last READ command.
		 */
		if (xfer->stop) {
			((uint8_t *)xfer
			     ->rx)[(i2c_dma_context[i2c]
					.dma_cmd_transfer_config.block_size -
				    1)] |= DATA_COMMAND_STOP_BIT_BYTE;
		}
		/* Only the second byte of IC_DATA_CMD register is written. */
		i2c_dma_context[i2c]
		    .dma_cmd_transfer_config.destination_address =
		    (uint32_t *)(((uint32_t) & (QM_I2C[i2c]->ic_data_cmd)) + 1);

		/*
		 * Start the RX operation in case of RX transaction only. If TX
		 * is specified, it's a combined transaction and RX will start
		 * once TX is done.
		 */
		if (xfer->tx_len == 0) {
			rc = i2c_start_dma_read(i2c);
		}
	}

	/* Setup TX if something to transmit. */
	if (xfer->tx_len > 0) {
		/*
		 * Last byte is handled manually as it may need to be sent with
		 * a STOP condition.
		 */
		i2c_dma_context[i2c].dma_tx_transfer_config.block_size =
		    xfer->tx_len - 1;
		i2c_dma_context[i2c].dma_tx_transfer_config.source_address =
		    (uint32_t *)xfer->tx;
		i2c_dma_context[i2c]
		    .dma_tx_transfer_config.destination_address =
		    (uint32_t *)&(QM_I2C[i2c]->ic_data_cmd);

		/* Enable DMA transmit. */
		QM_I2C[i2c]->ic_dma_cr = QM_I2C_IC_DMA_CR_TX_ENABLE;

		/* Enable controller. */
		controller_enable(i2c);

		/* Setup the DMA transfer. */
		rc = qm_dma_transfer_set_config(
		    i2c_dma_context[i2c].dma_controller_id,
		    i2c_dma_context[i2c].dma_tx_channel_id,
		    &(i2c_dma_context[i2c].dma_tx_transfer_config));
		if (rc == 0) {
			/* Mark the TX operation as ongoing. */
			i2c_dma_context[i2c].ongoing_dma_rx_operation = false;
			i2c_dma_context[i2c].ongoing_dma_tx_operation = true;
			rc = qm_dma_transfer_start(
			    i2c_dma_context[i2c].dma_controller_id,
			    i2c_dma_context[i2c].dma_tx_channel_id);
		}
	}

	return rc;
}

#if (ENABLE_RESTORE_CONTEXT)
int qm_i2c_save_context(const qm_i2c_t i2c, qm_i2c_context_t *const ctx)
{
	QM_CHECK(i2c < QM_I2C_NUM, -EINVAL);
	QM_CHECK(ctx != NULL, -EINVAL);

	qm_i2c_reg_t *const regs = QM_I2C[i2c];

	ctx->con = regs->ic_con;
	ctx->sar = regs->ic_sar;
	ctx->ss_scl_hcnt = regs->ic_ss_scl_hcnt;
	ctx->ss_scl_lcnt = regs->ic_ss_scl_lcnt;
	ctx->fs_scl_hcnt = regs->ic_fs_scl_hcnt;
	ctx->fs_scl_lcnt = regs->ic_fs_scl_lcnt;
	ctx->fs_spklen = regs->ic_fs_spklen;
	ctx->ic_intr_mask = regs->ic_intr_mask;
	ctx->enable = regs->ic_enable;

	return 0;
}

int qm_i2c_restore_context(const qm_i2c_t i2c,
			   const qm_i2c_context_t *const ctx)
{
	QM_CHECK(i2c < QM_I2C_NUM, -EINVAL);
	QM_CHECK(ctx != NULL, -EINVAL);

	qm_i2c_reg_t *const regs = QM_I2C[i2c];

	regs->ic_con = ctx->con;
	regs->ic_sar = ctx->sar;
	regs->ic_ss_scl_hcnt = ctx->ss_scl_hcnt;
	regs->ic_ss_scl_lcnt = ctx->ss_scl_lcnt;
	regs->ic_fs_scl_hcnt = ctx->fs_scl_hcnt;
	regs->ic_fs_scl_lcnt = ctx->fs_scl_lcnt;
	regs->ic_fs_spklen = ctx->fs_spklen;
	regs->ic_intr_mask = ctx->ic_intr_mask;
	regs->ic_enable = ctx->enable;

	return 0;
}
#endif /* ENABLE_RESTORE_CONTEXT */
