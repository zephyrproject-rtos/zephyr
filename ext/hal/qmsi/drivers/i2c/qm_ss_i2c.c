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
#include "qm_ss_i2c.h"
#include "clk.h"

#define TX_TL (2)
#define RX_TL (5)

/* number of retries before giving up on disabling the controller */
#define I2C_POLL_COUNT (1000000)
#define I2C_POLL_MICROSECOND (1)

static uint32_t i2c_base[QM_SS_I2C_NUM] = {QM_SS_I2C_0_BASE, QM_SS_I2C_1_BASE};
static volatile const qm_ss_i2c_transfer_t *i2c_transfer[QM_SS_I2C_NUM];
static volatile uint32_t i2c_write_pos[QM_SS_I2C_NUM],
    i2c_read_pos[QM_SS_I2C_NUM], i2c_read_cmd_send[QM_SS_I2C_NUM];

static void controller_enable(const qm_ss_i2c_t i2c);
static int controller_disable(const qm_ss_i2c_t i2c);

static uint32_t
i2c_fill_tx_fifo(const qm_i2c_t i2c,
		 const volatile qm_ss_i2c_transfer_t *const transfer,
		 uint32_t controller)
{
	uint32_t data_cmd = 0, count_tx = (QM_SS_I2C_FIFO_SIZE - TX_TL);
	uint32_t write_buffer_remaining = transfer->tx_len - i2c_write_pos[i2c];
	uint32_t read_buffer_remaining = transfer->rx_len - i2c_read_pos[i2c];

	while ((count_tx) && write_buffer_remaining) {
		count_tx--;
		write_buffer_remaining--;

		/* write command -IC_DATA_CMD[8] = 0 */
		/* fill IC_DATA_CMD[7:0] with the data */
		data_cmd = QM_SS_I2C_DATA_CMD_PUSH |
			   i2c_transfer[i2c]->tx[i2c_write_pos[i2c]];

		/* if transfer is a combined transfer, only
		 * send stop at
		 * end of the transfer sequence */
		if (i2c_transfer[i2c]->stop && (read_buffer_remaining == 0) &&
		    (write_buffer_remaining == 0)) {

			data_cmd |= QM_SS_I2C_DATA_CMD_STOP;
		}

		/* write data */
		QM_SS_I2C_WRITE_DATA_CMD(controller, data_cmd);
		i2c_write_pos[i2c]++;

		/* TX_EMPTY INTR is autocleared when the buffer
		 * levels goes above the threshold
		 */
	}

	return write_buffer_remaining;
}

static void handle_i2c_error_interrupt(const qm_ss_i2c_t i2c)
{
	uint32_t controller = i2c_base[i2c];
	qm_ss_i2c_status_t status = 0;
	int rc = -EIO;

	/* Check for TX_OVER error */
	if (QM_SS_I2C_READ_INTR_STAT(controller) &
	    QM_SS_I2C_INTR_STAT_TX_OVER) {

		status = QM_SS_I2C_TX_OVER;

		/* Clear interrupt */
		QM_SS_I2C_CLEAR_TX_OVER_INTR(controller);
	}

	/* Check for RX_UNDER error */
	if (QM_SS_I2C_READ_INTR_STAT(controller) &
	    QM_SS_I2C_INTR_STAT_RX_UNDER) {

		status = QM_SS_I2C_RX_UNDER;

		/* Clear interrupt */
		QM_SS_I2C_CLEAR_RX_UNDER_INTR(controller);
	}

	/* Check for RX_OVER error */
	if (QM_SS_I2C_READ_INTR_STAT(controller) &
	    QM_SS_I2C_INTR_STAT_RX_OVER) {

		status = QM_SS_I2C_RX_OVER;

		/* Clear interrupt */
		QM_SS_I2C_CLEAR_RX_OVER_INTR(controller);
	}

	/* Check for TX_ABRT error */
	if ((QM_SS_I2C_READ_INTR_STAT(controller) &
	     QM_SS_I2C_INTR_STAT_TX_ABRT)) {

		QM_ASSERT(!(QM_SS_I2C_READ_TX_ABRT_SOURCE(controller) &
			    QM_SS_I2C_TX_ABRT_SBYTE_NORSTRT));

		status = QM_SS_I2C_TX_ABORT;

		status |= (QM_SS_I2C_READ_TX_ABRT_SOURCE(controller) &
			   QM_SS_I2C_TX_ABRT_SOURCE_ALL_MASK);

		/* Clear interrupt */
		QM_SS_I2C_CLEAR_TX_ABRT_INTR(controller);

		rc = (status & QM_SS_I2C_TX_ABRT_USER_ABRT) ? -ECANCELED : -EIO;
	}

	/* Mask interrupts */
	QM_SS_I2C_MASK_ALL_INTERRUPTS(controller);

	controller_disable(i2c);
	if (i2c_transfer[i2c]->callback) {
		i2c_transfer[i2c]->callback(i2c_transfer[i2c]->callback_data,
					    rc, status, 0);
	}
}

static void handle_i2c_rx_avail_interrupt(const qm_ss_i2c_t i2c)
{
	const volatile qm_ss_i2c_transfer_t *const transfer = i2c_transfer[i2c];
	uint32_t controller = i2c_base[i2c];
	uint32_t read_buffer_remaining = transfer->rx_len - i2c_read_pos[i2c];

	/* RX read from buffer */
	if ((QM_SS_I2C_READ_INTR_STAT(controller) &
	     QM_SS_I2C_INTR_STAT_RX_FULL)) {

		while (read_buffer_remaining &&
		       (QM_SS_I2C_READ_RXFLR(controller))) {
			QM_SS_I2C_READ_RX_FIFO(controller);
			/* IC_DATA_CMD[7:0] contains received data */
			i2c_transfer[i2c]->rx[i2c_read_pos[i2c]] =
			    QM_SS_I2C_READ_DATA_CMD(controller);
			read_buffer_remaining--;
			i2c_read_pos[i2c]++;

			if (read_buffer_remaining == 0) {
				/* mask rx full interrupt if transfer
				 * complete
				 */
				QM_SS_I2C_MASK_INTERRUPT(
				    controller,
				    QM_SS_I2C_INTR_MASK_RX_FULL |
					QM_SS_I2C_INTR_MASK_TX_EMPTY);

				if (i2c_transfer[i2c]->stop) {
					controller_disable(i2c);
				}

				if (i2c_transfer[i2c]->callback) {
					i2c_transfer[i2c]->callback(
					    i2c_transfer[i2c]->callback_data, 0,
					    QM_SS_I2C_IDLE, i2c_read_pos[i2c]);
				}
			}
		}
		if (read_buffer_remaining > 0 &&
		    read_buffer_remaining < (RX_TL + 1)) {
			/* Adjust the RX threshold so the next 'RX_FULL'
			 * interrupt is generated when all the remaining
			 * data are received.
			 */
			QM_SS_I2C_CLEAR_RX_TL(controller);
			QM_SS_I2C_WRITE_RX_TL(controller,
					      (read_buffer_remaining - 1));
		}

		/* RX_FULL INTR is autocleared when the buffer
		 * levels goes below the threshold
		 */
	}
}

static void handle_i2c_tx_req_interrupt(const qm_ss_i2c_t i2c)
{
	const volatile qm_ss_i2c_transfer_t *const transfer = i2c_transfer[i2c];
	uint32_t controller = i2c_base[i2c];
	uint32_t count_tx;
	uint32_t read_buffer_remaining = transfer->rx_len - i2c_read_pos[i2c];
	uint32_t write_buffer_remaining = transfer->tx_len - i2c_write_pos[i2c];
	uint32_t missing_bytes;

	if ((QM_SS_I2C_READ_INTR_STAT(controller) &
	     QM_SS_I2C_INTR_STAT_TX_EMPTY)) {

		if ((QM_SS_I2C_READ_STATUS(controller) &
		     QM_SS_I2C_STATUS_TFE) &&
		    (i2c_transfer[i2c]->tx != NULL) &&
		    (write_buffer_remaining == 0) &&
		    (read_buffer_remaining == 0)) {

			QM_SS_I2C_MASK_INTERRUPT(controller,
						 QM_SS_I2C_INTR_MASK_TX_EMPTY);

			/* if this is not a combined
			 * transaction, disable the controller now
			 */
			if (i2c_transfer[i2c]->stop) {
				controller_disable(i2c);
			}

			/* callback */
			if (i2c_transfer[i2c]->callback) {
				i2c_transfer[i2c]->callback(
				    i2c_transfer[i2c]->callback_data, 0,
				    QM_SS_I2C_IDLE, i2c_write_pos[i2c]);
			}
		}

		write_buffer_remaining =
		    i2c_fill_tx_fifo(i2c, i2c_transfer[i2c], controller);

		/* If missing_bytes is not null, then that means we are already
		 * waiting for some bytes after sending read request on the
		 * previous interruption. We have to take into account this
		 * value in order to not send too much request so we won't fall
		 * into rx overflow */
		missing_bytes = read_buffer_remaining - i2c_read_cmd_send[i2c];

		/* Sanity check: The number of read data but not processed
		 * cannot be more than the number of expected bytes  */
		QM_ASSERT(QM_SS_I2C_READ_RXFLR(controller) <= missing_bytes);

		/* count_tx is the remaining size in the fifo */
		count_tx =
		    QM_SS_I2C_FIFO_SIZE - QM_SS_I2C_READ_TXFLR(controller);

		if (count_tx > missing_bytes) {
			count_tx -= missing_bytes;
		} else {
			count_tx = 0;
		}

		while (i2c_read_cmd_send[i2c] &&
		       (write_buffer_remaining == 0) && count_tx) {
			count_tx--;
			i2c_read_cmd_send[i2c]--;

			/* if transfer is a combined transfer, only
			 * send stop at
			 * end of
			 * the transfer sequence */
			if (i2c_transfer[i2c]->stop &&
			    (i2c_read_cmd_send[i2c] == 0)) {

				QM_SS_I2C_WRITE_DATA_CMD(
				    controller, (QM_SS_I2C_DATA_CMD_CMD |
						 QM_SS_I2C_DATA_CMD_PUSH |
						 QM_SS_I2C_DATA_CMD_STOP));

			} else {

				QM_SS_I2C_WRITE_DATA_CMD(
				    controller, (QM_SS_I2C_DATA_CMD_CMD |
						 QM_SS_I2C_DATA_CMD_PUSH));
			}
		}

		/* generate a tx_empty interrupt when tx fifo is fully
		 * empty */
		if ((write_buffer_remaining == 0) &&
		    (read_buffer_remaining == 0)) {
			QM_SS_I2C_CLEAR_TX_TL(controller);
		}
	}
}

static void handle_i2c_stop_det_interrupt(const qm_ss_i2c_t i2c)
{
	uint32_t controller = i2c_base[i2c];

	if ((QM_SS_I2C_READ_INTR_STAT(controller) & QM_SS_I2C_INTR_STAT_STOP)) {

		/* Clear interrupt */
		QM_SS_I2C_CLEAR_STOP_DET_INTR(controller);

		if (i2c_transfer[i2c]->callback) {
			i2c_transfer[i2c]->callback(
			    i2c_transfer[i2c]->callback_data, 0, QM_SS_I2C_IDLE,
			    0);
		}
	}
}

QM_ISR_DECLARE(qm_ss_i2c_0_error_isr)
{
	handle_i2c_error_interrupt(QM_SS_I2C_0);
}

QM_ISR_DECLARE(qm_ss_i2c_0_rx_avail_isr)
{
	handle_i2c_rx_avail_interrupt(QM_SS_I2C_0);
}

QM_ISR_DECLARE(qm_ss_i2c_0_tx_req_isr)
{
	handle_i2c_tx_req_interrupt(QM_SS_I2C_0);
}

QM_ISR_DECLARE(qm_ss_i2c_0_stop_det_isr)
{
	handle_i2c_stop_det_interrupt(QM_SS_I2C_0);
}

QM_ISR_DECLARE(qm_ss_i2c_1_error_isr)
{
	handle_i2c_error_interrupt(QM_SS_I2C_1);
}

QM_ISR_DECLARE(qm_ss_i2c_1_rx_avail_isr)
{
	handle_i2c_rx_avail_interrupt(QM_SS_I2C_1);
}

QM_ISR_DECLARE(qm_ss_i2c_1_tx_req_isr)
{
	handle_i2c_tx_req_interrupt(QM_SS_I2C_1);
}

QM_ISR_DECLARE(qm_ss_i2c_1_stop_det_isr)
{
	handle_i2c_stop_det_interrupt(QM_SS_I2C_1);
}

static uint32_t get_lo_cnt(uint32_t lo_time_ns)
{
	return (((clk_sys_get_ticks_per_us() * lo_time_ns) / 1000) - 1);
}

static uint32_t get_hi_cnt(qm_ss_i2c_t i2c, uint32_t hi_time_ns)
{
	uint32_t controller = i2c_base[i2c];

	return (((clk_sys_get_ticks_per_us() * hi_time_ns) / 1000) - 7 -
		(QM_SS_I2C_READ_SPKLEN(controller)));
}

int qm_ss_i2c_set_config(const qm_ss_i2c_t i2c,
			 const qm_ss_i2c_config_t *const cfg)
{
	QM_CHECK(i2c < QM_SS_I2C_NUM, -EINVAL);
	QM_CHECK(cfg != NULL, -EINVAL);

	uint32_t controller = i2c_base[i2c], lcnt = 0, hcnt = 0, min_lcnt = 0,
		 lcnt_diff = 0;

	QM_SS_I2C_WRITE_CLKEN(controller);

	/* mask all interrupts */
	QM_SS_I2C_MASK_ALL_INTERRUPTS(controller);

	/* disable controller */
	if (controller_disable(i2c)) {
		return -EBUSY;
	}

	/* Set mode */
	QM_SS_I2C_WRITE_RESTART_EN(controller);
	QM_SS_I2C_WRITE_ADDRESS_MODE(controller, cfg->address_mode);

	/*
	 * Timing generation algorithm:
	 * 1. compute hi/lo count so as to achieve the desired bus
	 *    speed at 50% duty cycle
	 * 2. adjust the hi/lo count to ensure that minimum hi/lo
	 *    timings are guaranteed as per spec.
	 */

	QM_SS_I2C_CLEAR_SPKLEN(controller);
	QM_SS_I2C_CLEAR_SPEED(controller);

	switch (cfg->speed) {
	case QM_SS_I2C_SPEED_STD:

		QM_SS_I2C_WRITE_SPEED(controller, QM_SS_I2C_CON_SPEED_SS);
		QM_SS_I2C_WRITE_SPKLEN(controller, QM_SS_I2C_SPK_LEN_SS);

		min_lcnt = get_lo_cnt(QM_I2C_MIN_SS_NS);
		lcnt = get_lo_cnt(QM_I2C_SS_50_DC_NS);
		hcnt = get_hi_cnt(i2c, QM_I2C_SS_50_DC_NS);
		break;

	case QM_SS_I2C_SPEED_FAST:

		QM_SS_I2C_WRITE_SPEED(controller, QM_SS_I2C_CON_SPEED_FS);
		QM_SS_I2C_WRITE_SPKLEN(controller, QM_SS_I2C_SPK_LEN_FS);

		min_lcnt = get_lo_cnt(QM_I2C_MIN_FS_NS);
		lcnt = get_lo_cnt(QM_I2C_FS_50_DC_NS);
		hcnt = get_hi_cnt(i2c, QM_I2C_FS_50_DC_NS);
		break;

#if HAS_SS_I2C_FAST_PLUS_SPEED
	case QM_SS_I2C_SPEED_FAST_PLUS:

		QM_SS_I2C_WRITE_SPEED(controller, QM_SS_I2C_CON_SPEED_FSP);
		QM_SS_I2C_WRITE_SPKLEN(controller, QM_SS_I2C_SPK_LEN_FSP);

		min_lcnt = get_lo_cnt(QM_I2C_MIN_FSP_NS);
		lcnt = get_lo_cnt(QM_I2C_FSP_50_DC_NS);
		hcnt = get_hi_cnt(i2c, QM_I2C_FSP_50_DC_NS);

		break;
#endif /* HAS_SS_I2C_FAST_PLUS_SPEED */
	}

	if (hcnt > QM_SS_I2C_IC_HCNT_MAX || hcnt < QM_SS_I2C_IC_HCNT_MIN) {
		return -EINVAL;
	}

	if (lcnt > QM_SS_I2C_IC_LCNT_MAX || lcnt < QM_SS_I2C_IC_LCNT_MIN) {
		return -EINVAL;
	}

	/* Increment minimum low count to account for rounding down */
	min_lcnt++;
	if (lcnt < min_lcnt) {
		lcnt_diff = (min_lcnt - lcnt);
		lcnt += (lcnt_diff);
		hcnt -= (lcnt_diff);
	}

	if (QM_SS_I2C_SPEED_STD == cfg->speed) {
		QM_SS_I2C_CLEAR_SS_SCL_HCNT(controller);
		QM_SS_I2C_CLEAR_SS_SCL_LCNT(controller);

		QM_SS_I2C_WRITE_SS_SCL_HCNT(controller, hcnt);
		QM_SS_I2C_WRITE_SS_SCL_LCNT(controller, lcnt);

	} else { /* Fast and fast plus modes */
		QM_SS_I2C_CLEAR_FS_SCL_HCNT(controller);
		QM_SS_I2C_CLEAR_FS_SCL_LCNT(controller);

		QM_SS_I2C_WRITE_FS_SCL_HCNT(controller, hcnt);
		QM_SS_I2C_WRITE_FS_SCL_LCNT(controller, lcnt);
	}

	return 0;
}

int qm_ss_i2c_set_speed(const qm_ss_i2c_t i2c, const qm_ss_i2c_speed_t speed,
			const uint16_t lo_cnt, const uint16_t hi_cnt)
{
	QM_CHECK(i2c < QM_SS_I2C_NUM, -EINVAL);
	QM_CHECK(hi_cnt < QM_SS_I2C_IC_HCNT_MAX &&
		     hi_cnt > QM_SS_I2C_IC_HCNT_MIN,
		 -EINVAL);
	QM_CHECK(lo_cnt < QM_SS_I2C_IC_LCNT_MAX &&
		     lo_cnt > QM_SS_I2C_IC_LCNT_MIN,
		 -EINVAL);

	uint32_t controller = i2c_base[i2c];

	QM_SS_I2C_CLEAR_SPKLEN(controller);
	QM_SS_I2C_CLEAR_SPEED(controller);

	switch (speed) {
	case QM_SS_I2C_SPEED_STD:
		QM_SS_I2C_WRITE_SPKLEN(controller, QM_SS_I2C_SPK_LEN_SS);
		QM_SS_I2C_WRITE_SPEED(controller, QM_SS_I2C_CON_SPEED_SS);

		QM_SS_I2C_CLEAR_SS_SCL_HCNT(controller);
		QM_SS_I2C_CLEAR_SS_SCL_LCNT(controller);

		QM_SS_I2C_WRITE_SS_SCL_HCNT(controller, hi_cnt);
		QM_SS_I2C_WRITE_SS_SCL_LCNT(controller, lo_cnt);
		break;

	case QM_SS_I2C_SPEED_FAST:
		QM_SS_I2C_WRITE_SPKLEN(controller, QM_SS_I2C_SPK_LEN_FS);
		QM_SS_I2C_WRITE_SPEED(controller, QM_SS_I2C_CON_SPEED_FS);

		QM_SS_I2C_CLEAR_FS_SCL_HCNT(controller);
		QM_SS_I2C_CLEAR_FS_SCL_LCNT(controller);

		QM_SS_I2C_WRITE_FS_SCL_HCNT(controller, hi_cnt);
		QM_SS_I2C_WRITE_FS_SCL_LCNT(controller, lo_cnt);
		break;
#if HAS_SS_I2C_FAST_PLUS_SPEED
	case QM_SS_I2C_SPEED_FAST_PLUS:
		QM_SS_I2C_WRITE_SPKLEN(controller, QM_SS_I2C_SPK_LEN_FSP);
		QM_SS_I2C_WRITE_SPEED(controller, QM_SS_I2C_CON_SPEED_FSP);

		QM_SS_I2C_CLEAR_FS_SCL_HCNT(controller);
		QM_SS_I2C_CLEAR_FS_SCL_LCNT(controller);

		QM_SS_I2C_WRITE_FS_SCL_HCNT(controller, hi_cnt);
		QM_SS_I2C_WRITE_FS_SCL_LCNT(controller, lo_cnt);
		break;
#endif /* HAS_SS_I2C_FAST_PLUS_SPEED */
	}

	return 0;
}

int qm_ss_i2c_get_status(const qm_ss_i2c_t i2c,
			 qm_ss_i2c_status_t *const status)
{
	QM_CHECK(status != NULL, -EINVAL);

	uint32_t controller = i2c_base[i2c];

	*status = QM_SS_I2C_IDLE;

	/* check if slave or master are active */
	if (QM_SS_I2C_READ_STATUS(controller) & QM_SS_I2C_STATUS_BUSY_MASK) {
		*status |= QM_SS_I2C_BUSY;
	}

	/* check for abort status */
	*status |= (QM_SS_I2C_READ_TX_ABRT_SOURCE(controller) &
		    QM_SS_I2C_TX_ABRT_SOURCE_ALL_MASK);

	return 0;
}

int qm_ss_i2c_master_write(const qm_ss_i2c_t i2c, const uint16_t slave_addr,
			   const uint8_t *const data, uint32_t len,
			   const bool stop, qm_ss_i2c_status_t *const status)
{
	QM_CHECK(i2c < QM_SS_I2C_NUM, -EINVAL);
	QM_CHECK(data != NULL, -EINVAL);
	QM_CHECK(len > 0, -EINVAL);

	uint8_t *d = (uint8_t *)data;
	uint32_t controller = i2c_base[i2c], data_cmd = 0;
	int ret = 0;

	/* write slave address to TAR */
	QM_SS_I2C_CLEAR_TAR(controller);
	QM_SS_I2C_WRITE_TAR(controller, slave_addr);

	/* enable controller */
	controller_enable(i2c);

	while (len--) {

		/* wait if FIFO is full */
		while (!(QM_SS_I2C_READ_STATUS(controller) &
			 QM_SS_I2C_STATUS_TFNF))
			;

		/* write command -IC_DATA_CMD[8] = 0 */
		/* fill IC_DATA_CMD[7:0] with the data */
		data_cmd = *d;
		data_cmd |= QM_SS_I2C_DATA_CMD_PUSH;

		/* send stop after last byte */
		if (len == 0 && stop) {
			data_cmd |= QM_SS_I2C_DATA_CMD_STOP;
		}

		QM_SS_I2C_WRITE_DATA_CMD(controller, data_cmd);
		d++;
	}

	/* this is a blocking call, wait until FIFO is empty or tx abrt
	 * error */
	while (!(QM_SS_I2C_READ_STATUS(controller) & QM_SS_I2C_STATUS_TFE))
		;

	if ((QM_SS_I2C_READ_INTR_STAT(controller) &
	     QM_SS_I2C_INTR_STAT_TX_ABRT)) {
		ret = -EIO;
	}

	/*  disable controller */
	if (true == stop) {
		if (controller_disable(i2c)) {
			ret = -EBUSY;
		}
	}

	if (status != NULL) {
		qm_ss_i2c_get_status(i2c, status);
	}

	/* Clear abort status
	 * The controller flushes/resets/empties
	 * the TX FIFO whenever this bit is set. The TX
	 * FIFO remains in this flushed state until the
	 * register IC_CLR_TX_ABRT is read.
	 */
	QM_SS_I2C_CLEAR_TX_ABRT_INTR(controller);

	return ret;
}

int qm_ss_i2c_master_read(const qm_ss_i2c_t i2c, const uint16_t slave_addr,
			  uint8_t *const data, uint32_t len, const bool stop,
			  qm_ss_i2c_status_t *const status)
{
	QM_CHECK(i2c < QM_SS_I2C_NUM, -EINVAL);
	QM_CHECK(data != NULL, -EINVAL);
	QM_CHECK(len > 0, -EINVAL);

	uint32_t controller = i2c_base[i2c],
		 data_cmd = QM_SS_I2C_DATA_CMD_CMD | QM_SS_I2C_DATA_CMD_PUSH;
	uint8_t *d = (uint8_t *)data;
	int ret = 0;

	/* write slave address to TAR */
	QM_SS_I2C_CLEAR_TAR(controller);
	QM_SS_I2C_WRITE_TAR(controller, slave_addr);

	/* enable controller */
	controller_enable(i2c);

	while (len--) {
		if (len == 0 && stop) {
			data_cmd |= QM_SS_I2C_DATA_CMD_STOP;
		}

		QM_SS_I2C_WRITE_DATA_CMD(controller, data_cmd);

		/*  wait if rx fifo is empty, break if tx empty and
		 * error*/
		while (!(QM_SS_I2C_READ_STATUS(controller) &
			 QM_SS_I2C_STATUS_RFNE)) {
			if (QM_SS_I2C_READ_INTR_STAT(controller) &
			    QM_SS_I2C_INTR_STAT_TX_ABRT) {
				break;
			}
		}

		if ((QM_SS_I2C_READ_INTR_STAT(controller) &
		     QM_SS_I2C_INTR_STAT_TX_ABRT)) {
			ret = -EIO;
			break;
		}

		QM_SS_I2C_READ_RX_FIFO(controller);

		/* wait until rx fifo is empty, indicating pop is complete*/
		while (
		    (QM_SS_I2C_READ_STATUS(controller) & QM_SS_I2C_STATUS_RFNE))
			;

		/* IC_DATA_CMD[7:0] contains received data */
		*d = QM_SS_I2C_READ_DATA_CMD(controller);
		d++;
	}

	/*  disable controller */
	if (true == stop) {
		if (controller_disable(i2c)) {
			ret = -EBUSY;
		}
	}

	if (status != NULL) {
		qm_ss_i2c_get_status(i2c, status);
	}

	/* Clear abort status
	 * The controller flushes/resets/empties
	 * the TX FIFO whenever this bit is set. The TX
	 * FIFO remains in this flushed state until the
	 * register IC_CLR_TX_ABRT is read.
	 */
	QM_SS_I2C_CLEAR_TX_ABRT_INTR(controller);

	return ret;
}

int qm_ss_i2c_master_irq_transfer(const qm_ss_i2c_t i2c,
				  const qm_ss_i2c_transfer_t *const xfer,
				  const uint16_t slave_addr)
{
	QM_CHECK(i2c < QM_SS_I2C_NUM, -EINVAL);
	QM_CHECK(NULL != xfer, -EINVAL);

	uint32_t controller = i2c_base[i2c];

	/* write slave address to TAR */
	QM_SS_I2C_CLEAR_TAR(controller);
	QM_SS_I2C_WRITE_TAR(controller, slave_addr);

	i2c_write_pos[i2c] = 0;
	i2c_read_pos[i2c] = 0;
	i2c_read_cmd_send[i2c] = xfer->rx_len;
	i2c_transfer[i2c] = xfer;

	/* set threshold */
	if (xfer->rx_len > 0 && xfer->rx_len < (RX_TL + 1)) {
		/* If 'rx_len' is less than the default threshold, we have to
		 * change the threshold value so the 'RX FULL' interrupt is
		 * generated once all data from the transfer is received.
		 */
		QM_SS_I2C_CLEAR_RX_TL(controller);
		QM_SS_I2C_CLEAR_TX_TL(controller);

		QM_SS_I2C_WRITE_RX_TL(controller, (xfer->rx_len - 1));
		QM_SS_I2C_WRITE_TX_TL(controller, TX_TL);
	} else {
		QM_SS_I2C_CLEAR_RX_TL(controller);
		QM_SS_I2C_CLEAR_TX_TL(controller);

		QM_SS_I2C_WRITE_RX_TL(controller, RX_TL);
		QM_SS_I2C_WRITE_TX_TL(controller, TX_TL);
	}

	/* enable controller */
	controller_enable(i2c);

	/* Start filling tx fifo. */
	i2c_fill_tx_fifo(i2c, xfer, controller);

	/* unmask interrupts */
	QM_SS_I2C_UNMASK_INTERRUPTS(controller);

	return 0;
}

static void controller_enable(const qm_ss_i2c_t i2c)
{
	uint32_t controller = i2c_base[i2c];
	if (!(QM_SS_I2C_READ_ENABLE_STATUS(controller) &
	      QM_SS_I2C_ENABLE_STATUS_IC_EN)) {
		/* enable controller */
		QM_SS_I2C_ENABLE(controller);
		/* wait until controller is enabled */
		while (!(QM_SS_I2C_READ_ENABLE_STATUS(controller) &
			 QM_SS_I2C_ENABLE_STATUS_IC_EN))
			;
	}

	/* Clear all interruption flags */
	QM_SS_I2C_CLEAR_ALL_INTR(controller);
}

static int controller_disable(const qm_ss_i2c_t i2c)
{
	uint32_t controller = i2c_base[i2c];
	int poll_count = I2C_POLL_COUNT;

	/* disable controller */
	QM_SS_I2C_DISABLE(controller);

	/* wait until controller is disabled */
	while ((QM_SS_I2C_READ_ENABLE_STATUS(controller) &
		QM_SS_I2C_ENABLE_STATUS_IC_EN) &&
	       poll_count--) {
		clk_sys_udelay(I2C_POLL_MICROSECOND);
	}

	/* returns 0 if ok, meaning controller is disabled */
	return (QM_SS_I2C_READ_ENABLE_STATUS(controller) &
		QM_SS_I2C_ENABLE_STATUS_IC_EN);
}

int qm_ss_i2c_irq_transfer_terminate(const qm_ss_i2c_t i2c)
{
	QM_CHECK(i2c < QM_SS_I2C_NUM, -EINVAL);

	uint32_t controller = i2c_base[i2c];

	/* Abort:
	 * In response to an ABORT, the controller issues a STOP and
	 * flushes
	 * the Tx FIFO after completing the current transfer, then sets
	 * the
	 * TX_ABORT interrupt after the abort operation. The ABORT bit
	 * is
	 * cleared automatically by hardware after the abort operation.
	 */
	QM_SS_I2C_ABORT(controller);

	return 0;
}

#if (ENABLE_RESTORE_CONTEXT)
int qm_ss_i2c_save_context(const qm_ss_i2c_t i2c,
			   qm_ss_i2c_context_t *const ctx)
{
	uint32_t controller = i2c_base[i2c];

	QM_CHECK(i2c < QM_SS_I2C_NUM, -EINVAL);
	QM_CHECK(ctx != NULL, -EINVAL);

	ctx->i2c_fs_scl_cnt =
	    __builtin_arc_lr(controller + QM_SS_I2C_FS_SCL_CNT);
	ctx->i2c_ss_scl_cnt =
	    __builtin_arc_lr(controller + QM_SS_I2C_SS_SCL_CNT);
	ctx->i2c_con = __builtin_arc_lr(controller + QM_SS_I2C_CON);

	return 0;
}

int qm_ss_i2c_restore_context(const qm_ss_i2c_t i2c,
			      const qm_ss_i2c_context_t *const ctx)
{
	uint32_t controller = i2c_base[i2c];

	QM_CHECK(i2c < QM_SS_I2C_NUM, -EINVAL);
	QM_CHECK(ctx != NULL, -EINVAL);

	__builtin_arc_sr(ctx->i2c_fs_scl_cnt,
			 controller + QM_SS_I2C_FS_SCL_CNT);
	__builtin_arc_sr(ctx->i2c_ss_scl_cnt,
			 controller + QM_SS_I2C_SS_SCL_CNT);
	__builtin_arc_sr(ctx->i2c_con, controller + QM_SS_I2C_CON);

	return 0;
}
#else
int qm_ss_i2c_save_context(const qm_ss_i2c_t i2c,
			   qm_ss_i2c_context_t *const ctx)
{
	(void)i2c;
	(void)ctx;

	return 0;
}

int qm_ss_i2c_restore_context(const qm_ss_i2c_t i2c,
			      const qm_ss_i2c_context_t *const ctx)
{
	(void)i2c;
	(void)ctx;

	return 0;
}
#endif /* ENABLE_RESTORE_CONTEXT */
