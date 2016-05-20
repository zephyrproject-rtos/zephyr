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

/* number of retries before giving up on disabling the controller */
#define MAX_T_POLL_COUNT (100)
#define TI2C_POLL_MICROSECOND (10000)

#ifndef UNIT_TEST
#if (QUARK_SE)
qm_i2c_reg_t *qm_i2c[QM_I2C_NUM] = {(qm_i2c_reg_t *)QM_I2C_0_BASE,
				    (qm_i2c_reg_t *)QM_I2C_1_BASE};
#elif(QUARK_D2000)
qm_i2c_reg_t *qm_i2c[QM_I2C_NUM] = {(qm_i2c_reg_t *)QM_I2C_0_BASE};
#endif
#endif

static qm_i2c_transfer_t i2c_transfer[QM_I2C_NUM];
static uint32_t i2c_write_pos[QM_I2C_NUM], i2c_read_pos[QM_I2C_NUM],
    i2c_read_buffer_remaining[QM_I2C_NUM];

/**
 * I2C DMA controller configuration descriptor.
 */
typedef struct {
	qm_i2c_t i2c; /* I2C controller */
	qm_dma_t dma_controller_id;
	qm_dma_channel_id_t dma_tx_channel_id;    /* TX channel ID */
	qm_dma_transfer_t dma_tx_transfer_config; /* Configuration for TX */
	volatile bool ongoing_dma_tx_operation;   /* Keep track of ongoing TX */
	qm_dma_channel_id_t dma_rx_channel_id;    /* RX channel ID */
	qm_dma_transfer_t dma_rx_transfer_config; /* Configuration for RX */
	/* Configuration for writing READ commands during an RX	operation */
	qm_dma_transfer_t dma_cmd_transfer_config;
	volatile bool ongoing_dma_rx_operation; /* Keep track of oingoing RX*/
} i2c_dma_context_t;

/* DMA context */
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

static void qm_i2c_isr_handler(const qm_i2c_t i2c)
{
	uint32_t ic_data_cmd = 0, count_tx = (QM_I2C_FIFO_SIZE - TX_TL);
	qm_i2c_status_t status = 0;
	int rc = 0;

	qm_i2c_reg_t *const controller = QM_I2C[i2c];

	/* Check for errors */
	QM_ASSERT(!(controller->ic_intr_stat & QM_I2C_IC_INTR_STAT_TX_OVER));
	QM_ASSERT(!(controller->ic_intr_stat & QM_I2C_IC_INTR_STAT_RX_UNDER));
	QM_ASSERT(!(controller->ic_intr_stat & QM_I2C_IC_INTR_STAT_RX_OVER));

	if (controller->ic_intr_stat & QM_I2C_IC_INTR_STAT_TX_ABRT) {
		QM_ASSERT(!(controller->ic_tx_abrt_source &
			    QM_I2C_IC_TX_ABRT_SOURCE_ABRT_SBYTE_NORSTRT));
		status = (controller->ic_tx_abrt_source &
			  QM_I2C_IC_TX_ABRT_SOURCE_ALL_MASK);

		/* clear intr */
		controller->ic_clr_tx_abrt;

		/* mask interrupts */
		controller->ic_intr_mask = QM_I2C_IC_INTR_MASK_ALL;

		rc = (status & QM_I2C_TX_ABRT_USER_ABRT) ? -ECANCELED : -EIO;
		if (i2c_transfer[i2c].callback) {
			/* NOTE: currently 0 is returned for length but we
			 * may revisit that soon
			 */
			i2c_transfer[i2c].callback(
			    i2c_transfer[i2c].callback_data, rc, status, 0);
			controller_disable(i2c);
		}
	}

	/* RX read from buffer */
	if (controller->ic_intr_stat & QM_I2C_IC_INTR_STAT_RX_FULL) {

		while (i2c_read_buffer_remaining[i2c] && controller->ic_rxflr) {
			i2c_transfer[i2c].rx[i2c_read_pos[i2c]] =
			    controller->ic_data_cmd;
			i2c_read_buffer_remaining[i2c]--;
			i2c_read_pos[i2c]++;

			if (i2c_read_buffer_remaining[i2c] == 0) {
				/* mask rx full interrupt if transfer
				 * complete
				 */
				controller->ic_intr_mask &=
				    ~QM_I2C_IC_INTR_MASK_RX_FULL;

				if (i2c_transfer[i2c].stop) {
					controller_disable(i2c);
				}

				if (i2c_transfer[i2c].callback) {
					i2c_transfer[i2c].callback(
					    i2c_transfer[i2c].callback_data, 0,
					    QM_I2C_IDLE, i2c_read_pos[i2c]);
				}
			}
		}

		if (i2c_read_buffer_remaining[i2c] > 0 &&
		    i2c_read_buffer_remaining[i2c] < (RX_TL + 1)) {
			/* Adjust the RX threshold so the next 'RX_FULL'
			 * interrupt is generated when all the remaining
			 * data are received.
			 */
			controller->ic_rx_tl =
			    i2c_read_buffer_remaining[i2c] - 1;
		}

		/* RX_FULL INTR is autocleared when the buffer
		 * levels goes below the threshold
		 */
	}

	if (controller->ic_intr_stat & QM_I2C_IC_INTR_STAT_TX_EMPTY) {

		if ((controller->ic_status & QM_I2C_IC_STATUS_TFE) &&
		    (i2c_transfer[i2c].tx != NULL) &&
		    (i2c_transfer[i2c].tx_len == 0) &&
		    (i2c_transfer[i2c].rx_len == 0)) {

			controller->ic_intr_mask &=
			    ~QM_I2C_IC_INTR_MASK_TX_EMPTY;

			/* if this is not a combined
			 * transaction, disable the controller now
			 */
			if ((i2c_read_buffer_remaining[i2c] == 0) &&
			    i2c_transfer[i2c].stop) {
				controller_disable(i2c);

				/* callback */
				if (i2c_transfer[i2c].callback) {
					i2c_transfer[i2c].callback(
					    i2c_transfer[i2c].callback_data, 0,
					    QM_I2C_IDLE, i2c_write_pos[i2c]);
				}
			}
		}

		while ((count_tx) && i2c_transfer[i2c].tx_len) {
			count_tx--;

			/* write command -IC_DATA_CMD[8] = 0 */
			/* fill IC_DATA_CMD[7:0] with the data */
			ic_data_cmd = i2c_transfer[i2c].tx[i2c_write_pos[i2c]];
			i2c_transfer[i2c].tx_len--;

			/* if transfer is a combined transfer, only
			 * send stop at
			 * end of the transfer sequence */
			if (i2c_transfer[i2c].stop &&
			    (i2c_transfer[i2c].tx_len == 0) &&
			    (i2c_transfer[i2c].rx_len == 0)) {

				ic_data_cmd |= QM_I2C_IC_DATA_CMD_STOP_BIT_CTRL;
			}

			/* write data */
			controller->ic_data_cmd = ic_data_cmd;
			i2c_write_pos[i2c]++;

			/* TX_EMPTY INTR is autocleared when the buffer
			 * levels goes above the threshold
			 */
		}

		/* TX read command */
		count_tx = QM_I2C_FIFO_SIZE -
			   (controller->ic_txflr + controller->ic_rxflr + 1);

		while (i2c_transfer[i2c].rx_len &&
		       (i2c_transfer[i2c].tx_len == 0) && count_tx) {
			count_tx--;
			i2c_transfer[i2c].rx_len--;

			/* if transfer is a combined transfer, only
			 * send stop at
			 * end of
			 * the transfer sequence */
			if (i2c_transfer[i2c].stop &&
			    (i2c_transfer[i2c].rx_len == 0) &&
			    (i2c_transfer[i2c].tx_len == 0)) {
				controller->ic_data_cmd =
				    QM_I2C_IC_DATA_CMD_READ |
				    QM_I2C_IC_DATA_CMD_STOP_BIT_CTRL;
			} else {
				controller->ic_data_cmd =
				    QM_I2C_IC_DATA_CMD_READ;
			}
		}

		/* generate a tx_empty interrupt when tx fifo is fully empty */
		if ((i2c_transfer[i2c].tx_len == 0) &&
		    (i2c_transfer[i2c].rx_len == 0)) {
			controller->ic_tx_tl = 0;
		}
	}
}

QM_ISR_DECLARE(qm_i2c_0_isr)
{
	qm_i2c_isr_handler(QM_I2C_0);
	QM_ISR_EOI(QM_IRQ_I2C_0_VECTOR);
}

#if (QUARK_SE)
QM_ISR_DECLARE(qm_i2c_1_isr)
{
	qm_i2c_isr_handler(QM_I2C_1);
	QM_ISR_EOI(QM_IRQ_I2C_1_VECTOR);
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
	* See HAS for Known Limitation : Generated SCL HIGH
	* period is less than the expected SCL clock HIGH period in
	* the Master receiver mode.
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

	/* mask all interrupts */
	controller->ic_intr_mask = QM_I2C_IC_INTR_MASK_ALL;

	/* disable controller */
	if (controller_disable(i2c)) {
		return -EAGAIN;
	}

	switch (cfg->mode) {
	case QM_I2C_MASTER:
		/* set mode */
		ic_con = QM_I2C_IC_CON_MASTER_MODE | QM_I2C_IC_CON_RESTART_EN |
			 QM_I2C_IC_CON_SLAVE_DISABLE |
			 /* set 7/10 bit address mode */
			 (cfg->address_mode
			  << QM_I2C_IC_CON_10BITADDR_MASTER_OFFSET);

		/*
		 * Timing generation algorithm:
		 * 1. compute hi/lo count so as to achieve the desired bus
		 *    speed at 50% duty cycle
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

		/* Increment minimum low count to account for rounding down */
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
		/* QM_I2C_IC_CON_MASTER_MODE and
		 * QM_I2C_IC_CON_SLAVE_DISABLE are
		 * deasserted */

		/* set 7/10 bit address mode */
		ic_con = cfg->address_mode
			 << QM_I2C_IC_CON_10BITADDR_SLAVE_OFFSET;

		/* set slave address */
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
		break;

	case QM_I2C_SPEED_FAST:
	case QM_I2C_SPEED_FAST_PLUS:
		ic_con |= QM_I2C_IC_CON_SPEED_FS_FSP;
		controller->ic_fs_scl_lcnt = lo_cnt;
		controller->ic_fs_scl_hcnt = hi_cnt;
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

	/* check if slave or master are active */
	if (controller->ic_status & QM_I2C_IC_STATUS_BUSY_MASK) {
		*status = QM_I2C_BUSY;
	}

	/* Check for abort status */
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

	/* write slave address to TAR */
	controller->ic_tar = slave_addr;

	/* enable controller */
	controller_enable(i2c);

	while (len--) {

		/* wait if FIFO is full */
		while (!(controller->ic_status & QM_I2C_IC_STATUS_TNF))
			;

		/* write command -IC_DATA_CMD[8] = 0 */
		/* fill IC_DATA_CMD[7:0] with the data */
		ic_data_cmd = *d;

		/* send stop after last byte */
		if (len == 0 && stop) {
			ic_data_cmd |= QM_I2C_IC_DATA_CMD_STOP_BIT_CTRL;
		}

		controller->ic_data_cmd = ic_data_cmd;
		d++;
	}

	/* this is a blocking call, wait until FIFO is empty or tx abrt
	 * error */
	while (!(controller->ic_status & QM_I2C_IC_STATUS_TFE))
		;

	if (controller->ic_tx_abrt_source & QM_I2C_IC_TX_ABRT_SOURCE_ALL_MASK) {
		ret = -EIO;
	}

	/*  disable controller */
	if (true == stop) {
		if (controller_disable(i2c)) {
			ret = -EIO;
		}
	}

	if (status != NULL) {
		qm_i2c_get_status(i2c, status);
	}

	/* Clear abort status
	 * The controller flushes/resets/empties
	 * the TX FIFO whenever this bit is set. The TX
	 * FIFO remains in this flushed state until the
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

	/* write slave address to TAR */
	controller->ic_tar = slave_addr;

	/* enable controller */
	controller_enable(i2c);

	while (len--) {
		if (len == 0 && stop) {
			controller->ic_data_cmd =
			    QM_I2C_IC_DATA_CMD_READ |
			    QM_I2C_IC_DATA_CMD_STOP_BIT_CTRL;
		}

		else {
			/*  read command -IC_DATA_CMD[8] = 1 */
			controller->ic_data_cmd = QM_I2C_IC_DATA_CMD_READ;
		}

		/*  wait if rx fifo is empty, break if tx empty and
		 * error*/
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
		/* IC_DATA_CMD[7:0] contains received data */
		*d = controller->ic_data_cmd;
		d++;
	}

	/*  disable controller */
	if (true == stop) {
		if (controller_disable(i2c)) {
			ret = -EIO;
		}
	}

	if (status != NULL) {
		qm_i2c_get_status(i2c, status);
	}

	/* Clear abort status
	 * The controller flushes/resets/empties
	 * the TX FIFO whenever this bit is set. The TX
	 * FIFO remains in this flushed state until the
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

	/* write slave address to TAR */
	controller->ic_tar = slave_addr;

	i2c_write_pos[i2c] = 0;
	i2c_read_pos[i2c] = 0;
	i2c_read_buffer_remaining[i2c] = xfer->rx_len;
	memcpy(&i2c_transfer[i2c], xfer, sizeof(i2c_transfer[i2c]));

	/* set threshold */
	controller->ic_tx_tl = TX_TL;
	if (xfer->rx_len > 0 && xfer->rx_len < (RX_TL + 1)) {
		/* If 'rx_len' is less than the default threshold, we have to
		 * change the threshold value so the 'RX FULL' interrupt is
		 * generated once all data from the transfer is received.
		 */
		controller->ic_rx_tl = xfer->rx_len - 1;
	} else {
		controller->ic_rx_tl = RX_TL;
	}

	/* mask interrupts */
	QM_I2C[i2c]->ic_intr_mask = QM_I2C_IC_INTR_MASK_ALL;

	/* enable controller */
	controller_enable(i2c);

	/* unmask interrupts */
	controller->ic_intr_mask |=
	    QM_I2C_IC_INTR_MASK_RX_UNDER | QM_I2C_IC_INTR_MASK_RX_OVER |
	    QM_I2C_IC_INTR_MASK_RX_FULL | QM_I2C_IC_INTR_MASK_TX_OVER |
	    QM_I2C_IC_INTR_MASK_TX_EMPTY | QM_I2C_IC_INTR_MASK_TX_ABORT;

	return 0;
}

static void controller_enable(const qm_i2c_t i2c)
{
	qm_i2c_reg_t *const controller = QM_I2C[i2c];

	if (!(controller->ic_enable_status & QM_I2C_IC_ENABLE_STATUS_IC_EN)) {
		/* enable controller */
		controller->ic_enable |= QM_I2C_IC_ENABLE_CONTROLLER_EN;

		/* wait until controller is enabled */
		while (!(controller->ic_enable_status &
			 QM_I2C_IC_ENABLE_STATUS_IC_EN))
			;
	}
}

static int controller_disable(const qm_i2c_t i2c)
{
	qm_i2c_reg_t *const controller = QM_I2C[i2c];
	int poll_count = MAX_T_POLL_COUNT;

	/* disable controller */
	controller->ic_enable &= ~QM_I2C_IC_ENABLE_CONTROLLER_EN;

	/* wait until controller is disabled */
	while ((controller->ic_enable_status & QM_I2C_IC_ENABLE_STATUS_IC_EN) &&
	       poll_count--) {
		clk_sys_udelay(TI2C_POLL_MICROSECOND);
	}

	/* returns 0 if ok, meaning controller is disabled */
	return (controller->ic_enable_status & QM_I2C_IC_ENABLE_STATUS_IC_EN);
}

int qm_i2c_irq_transfer_terminate(const qm_i2c_t i2c)
{
	QM_CHECK(i2c < QM_I2C_NUM, -EINVAL);

	/* Abort:
	 * In response to an ABORT, the controller issues a STOP and
	 * flushes
	 * the Tx FIFO after completing the current transfer, then sets
	 * the
	 * TX_ABORT interrupt after the abort operation. The ABORT bit
	 * is
	 * cleared automatically by hardware after the abort operation.
	 */
	QM_I2C[i2c]->ic_enable |= QM_I2C_IC_ENABLE_CONTROLLER_ABORT;

	return 0;
}

/**
 * Stops DMA channel and terminates I2C transfer.
 */
int qm_i2c_dma_transfer_terminate(const qm_i2c_t i2c)
{
	int rc = 0;

	QM_CHECK(i2c < QM_I2C_NUM, -EINVAL);

	if (i2c_dma_context[i2c].ongoing_dma_tx_operation) {
		/* First terminate the DMA transfer */
		rc = qm_dma_transfer_terminate(
		    i2c_dma_context[i2c].dma_controller_id,
		    i2c_dma_context[i2c].dma_tx_channel_id);
		/* Then disable DMA transmit */
		QM_I2C[i2c]->ic_dma_cr &= ~QM_I2C_IC_DMA_CR_TX_ENABLE;
		i2c_dma_context[i2c].ongoing_dma_tx_operation = false;
	}

	if (rc == 0) {
		if (i2c_dma_context[i2c].ongoing_dma_rx_operation) {
			/* First terminate the DMA transfer */
			rc = qm_dma_transfer_terminate(
			    i2c_dma_context[i2c].dma_controller_id,
			    i2c_dma_context[i2c].dma_rx_channel_id);
			/* Then disable DMA receive */
			QM_I2C[i2c]->ic_dma_cr &= ~QM_I2C_IC_DMA_CR_RX_ENABLE;
			i2c_dma_context[i2c].ongoing_dma_rx_operation = false;
		}
	}

	/* And terminate the I2C transfer */
	if (rc == 0) {
		QM_I2C[i2c]->ic_enable |= QM_I2C_IC_ENABLE_CONTROLLER_ABORT;
	}

	return rc;
}

/**
 * Disable TX and/or RX and call user error callback if provided.
 */
static void i2c_dma_transfer_error_callback(uint32_t i2c, int error_code,
					    uint32_t len)
{
	qm_i2c_status_t status;

	if (error_code != 0) {
		if (i2c_dma_context[i2c].ongoing_dma_tx_operation == true) {
			/* Disable DMA transmit */
			QM_I2C[i2c]->ic_dma_cr &= ~QM_I2C_IC_DMA_CR_TX_ENABLE;
			i2c_dma_context[i2c].ongoing_dma_tx_operation = false;
		}

		if (i2c_dma_context[i2c].ongoing_dma_rx_operation == true) {
			/* Disable DMA receive */
			QM_I2C[i2c]->ic_dma_cr &= ~QM_I2C_IC_DMA_CR_RX_ENABLE;
			i2c_dma_context[i2c].ongoing_dma_rx_operation = false;
		}

		/* If the user has provided a callback, let's call it */
		if (i2c_transfer[i2c].callback != NULL) {
			qm_i2c_get_status(i2c, &status);
			i2c_transfer[i2c].callback(
			    i2c_transfer[i2c].callback_data, error_code, status,
			    len);
		}
	}
}

/**
 * After a TX operation, a stop condition may need to be issued along the last
 * data byte, so that byte is handled here.
 * DMA TX mode does also need to be disabled.
 */
static void i2c_dma_transmit_callback(void *callback_context, uint32_t len,
				      int error_code)
{
	qm_i2c_status_t status;

	qm_i2c_t i2c = ((i2c_dma_context_t *)callback_context)->i2c;

	if (error_code == 0) {
		/* Disable DMA transmit */
		QM_I2C[i2c]->ic_dma_cr &= ~QM_I2C_IC_DMA_CR_TX_ENABLE;
		i2c_dma_context[i2c].ongoing_dma_tx_operation = false;

		/* As the callback is used for both real TX and read command
		   "TX" during an RX operation, we need to know what case we are
		   in */
		if (i2c_dma_context[i2c].ongoing_dma_rx_operation == false) {
			/* Write last byte */
			uint32_t data_command =
			    QM_I2C_IC_DATA_CMD_LSB_MASK &
			    ((uint8_t *)i2c_dma_context[i2c]
				 .dma_tx_transfer_config.source_address)
				[i2c_dma_context[i2c]
				     .dma_tx_transfer_config.block_size];

			/* Check if we must issue a stop condition and it's not
			   a combined transaction */
			if ((i2c_transfer[i2c].stop == true) &&
			    (i2c_transfer[i2c].rx_len == 0)) {
				data_command |=
				    QM_I2C_IC_DATA_CMD_STOP_BIT_CTRL;
			}
			/* Write last byte and increase len count */
			QM_I2C[i2c]->ic_data_cmd = data_command;
			len++;

			/* Check if there is a pending read operation, meaning
			   this is a combined transaction */
			if (i2c_transfer[i2c].rx_len > 0) {
				i2c_start_dma_read(i2c);
			} else {
				/* Let's disable the I2C controller if we are
				   done */
				if (i2c_transfer[i2c].stop == true) {
					/* This callback is called when DMA is
					   done, but I2C can still be
					   transmitting, so let's wait
					   until all data is sent */
					while (!(QM_I2C[i2c]->ic_status &
						 QM_I2C_IC_STATUS_TFE)) {
					}
					controller_disable(i2c);
				}
				/* If user provided a callback, it'll be called
				   only if this is a TX only operation, not in a
				   combined transaction */
				if (i2c_transfer[i2c].callback != NULL) {
					qm_i2c_get_status(i2c, &status);
					i2c_transfer[i2c].callback(
					    i2c_transfer[i2c].callback_data,
					    error_code, status, len);
				}
			}
		}
	} else {
		i2c_dma_transfer_error_callback(i2c, error_code, len);
	}
}

/**
 * After an RX operation, we need to disable DMA RX mode.
 */
static void i2c_dma_receive_callback(void *callback_context, uint32_t len,
				     int error_code)
{
	qm_i2c_status_t status;

	qm_i2c_t i2c = ((i2c_dma_context_t *)callback_context)->i2c;

	if (error_code == 0) {
		/* Disable DMA receive */
		QM_I2C[i2c]->ic_dma_cr &= ~QM_I2C_IC_DMA_CR_RX_ENABLE;
		i2c_dma_context[i2c].ongoing_dma_rx_operation = false;

		/* Let's disable the I2C controller if we are done */
		if (i2c_transfer[i2c].stop == true) {
			controller_disable(i2c);
		}

		/* If the user has provided a callback, let's call it */
		if (i2c_transfer[i2c].callback != NULL) {
			qm_i2c_get_status(i2c, &status);
			i2c_transfer[i2c].callback(
			    i2c_transfer[i2c].callback_data, error_code, status,
			    len);
		}
	} else {
		i2c_dma_transfer_error_callback(i2c, error_code, len);
	}
}

/**
 * Effectively starts a previously configured read operation. For doing this, 2
 * DMA channels are needed, one for writting READ commands to the I2C controller
 * and the other to get the read data from the I2C controller to memory. Thus,
 * a TX operation is also needed in order to perform an RX.
 */
static int i2c_start_dma_read(const qm_i2c_t i2c)
{
	int rc = 0;

	/* Enable DMA transmit and receive */
	QM_I2C[i2c]->ic_dma_cr =
	    QM_I2C_IC_DMA_CR_RX_ENABLE | QM_I2C_IC_DMA_CR_TX_ENABLE;

	/* enable controller */
	controller_enable(i2c);

	/* A RX operation need to read and write */
	i2c_dma_context[i2c].ongoing_dma_rx_operation = true;
	i2c_dma_context[i2c].ongoing_dma_tx_operation = true;
	/* Configure DMA TX for writing READ commands */
	rc = qm_dma_transfer_set_config(
	    i2c_dma_context[i2c].dma_controller_id,
	    i2c_dma_context[i2c].dma_tx_channel_id,
	    &(i2c_dma_context[i2c].dma_cmd_transfer_config));
	if (rc == 0) {
		/* Configure DMA RX */
		rc = qm_dma_transfer_set_config(
		    i2c_dma_context[i2c].dma_controller_id,
		    i2c_dma_context[i2c].dma_rx_channel_id,
		    &(i2c_dma_context[i2c].dma_rx_transfer_config));
		if (rc == 0) {
			/* Start both transfers "at once" */
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

/**
 * Configures given DMA channel with the appropriate width and length and the
 * right handshaking interface and callback depending on the direction.
 */
int qm_i2c_dma_channel_config(const qm_i2c_t i2c,
			      const qm_dma_t dma_controller_id,
			      const qm_dma_channel_id_t channel_id,
			      const qm_dma_channel_direction_t direction)
{
	qm_dma_channel_config_t dma_channel_config = {0};
	int rc = 0;

	/* Test input values */
	QM_CHECK(i2c < QM_I2C_NUM, -EINVAL);
	QM_CHECK(channel_id < QM_DMA_CHANNEL_NUM, -EINVAL);
	QM_CHECK(dma_controller_id < QM_DMA_NUM, -EINVAL);
	QM_CHECK(direction <= QM_DMA_PERIPHERAL_TO_MEMORY, -EINVAL);
	QM_CHECK(direction >= QM_DMA_MEMORY_TO_PERIPHERAL, -EINVAL);

	/* Set DMA channel configuration */
	dma_channel_config.handshake_interface =
	    i2c_dma_interfaces[i2c][direction];
	dma_channel_config.handshake_polarity = QM_DMA_HANDSHAKE_POLARITY_HIGH;
	dma_channel_config.channel_direction = direction;
	dma_channel_config.source_transfer_width = QM_DMA_TRANS_WIDTH_8;
	dma_channel_config.destination_transfer_width = QM_DMA_TRANS_WIDTH_8;
	/* NOTE: This can be optimized for performance */
	dma_channel_config.source_burst_length = QM_DMA_BURST_TRANS_LENGTH_1;
	dma_channel_config.destination_burst_length =
	    QM_DMA_BURST_TRANS_LENGTH_1;
	dma_channel_config.client_callback = i2c_dma_callbacks[direction];

	/* Hold the channel IDs and controller ID in the DMA context */
	if (direction == QM_DMA_PERIPHERAL_TO_MEMORY) {
		i2c_dma_context[i2c].dma_rx_channel_id = channel_id;
	} else {
		i2c_dma_context[i2c].dma_tx_channel_id = channel_id;
	}
	i2c_dma_context[i2c].dma_controller_id = dma_controller_id;
	i2c_dma_context[i2c].i2c = i2c;
	dma_channel_config.callback_context = &i2c_dma_context[i2c];

	/* Configure DMA channel */
	rc = qm_dma_channel_set_config(dma_controller_id, channel_id,
				       &dma_channel_config);

	return rc;
}

/**
 * Setups and starts a DMA transaction, wether it's read, write or combined one.
 * In case of combined transaction, it sets up both operations and starts the
 * write one; the read operation will be started in the read operation callback.
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

	/* Disable all IRQs but the TX abort one */
	QM_I2C[i2c]->ic_intr_mask = QM_I2C_IC_INTR_STAT_TX_ABRT;

	/* write slave address to TAR */
	QM_I2C[i2c]->ic_tar = slave_addr;

	memcpy(&i2c_transfer[i2c], xfer, sizeof(i2c_transfer[i2c]));

	/* Set DMA TX and RX waterlevels to 0, to make sure no data is lost */
	/* NOTE: This can be optimized for performance */
	QM_I2C[i2c]->ic_dma_rdlr = 0;
	QM_I2C[i2c]->ic_dma_tdlr = 0;

	/* Setup RX if something to receive */
	if (xfer->rx_len > 0) {
		i2c_dma_context[i2c].dma_rx_transfer_config.block_size =
		    xfer->rx_len;
		i2c_dma_context[i2c].dma_rx_transfer_config.source_address =
		    (uint32_t *)&(QM_I2C[i2c]->ic_data_cmd);
		i2c_dma_context[i2c]
		    .dma_rx_transfer_config.destination_address =
		    (uint32_t *)(xfer->rx);

		/* For receiving, READ commands need to be written, a TX
		   transfer is needed for writting them */
		i2c_dma_context[i2c].dma_cmd_transfer_config.block_size =
		    xfer->rx_len;
		i2c_dma_context[i2c].dma_cmd_transfer_config.source_address =
		    (uint32_t *)(xfer->rx);
		/* RX buffer will be filled with READ commands and use it as
		   source for the READ command write operation. As READ commands
		   are written, data will be read overwriting the already
		   written READ commands */
		for (
		    i = 0;
		    i < i2c_dma_context[i2c].dma_cmd_transfer_config.block_size;
		    i++) {
			((uint8_t *)xfer->rx)[i] =
			    DATA_COMMAND_READ_COMMAND_BYTE;
		}
		/* The STOP condition will be issued on the last READ command */
		if (xfer->stop) {
			((uint8_t *)xfer
			     ->rx)[(i2c_dma_context[i2c]
					.dma_cmd_transfer_config.block_size -
				    1)] |= DATA_COMMAND_STOP_BIT_BYTE;
		}
		/* Only the second byte of IC_DATA_CMD register is written */
		i2c_dma_context[i2c]
		    .dma_cmd_transfer_config.destination_address =
		    (uint32_t *)(((uint32_t) & (QM_I2C[i2c]->ic_data_cmd)) + 1);

		/* Start the RX operation in case of RX transaction only. If
		   TX is specified, it's a combined transaction and RX will
		   start once TX is done. */
		if (xfer->tx_len == 0) {
			rc = i2c_start_dma_read(i2c);
		}
	}

	/* Setup TX if something to transmit */
	if (xfer->tx_len > 0) {
		/* Last byte is handled manually as it may need to be sent with
		   a STOP condition */
		i2c_dma_context[i2c].dma_tx_transfer_config.block_size =
		    xfer->tx_len - 1;
		i2c_dma_context[i2c].dma_tx_transfer_config.source_address =
		    (uint32_t *)xfer->tx;
		i2c_dma_context[i2c]
		    .dma_tx_transfer_config.destination_address =
		    (uint32_t *)&(QM_I2C[i2c]->ic_data_cmd);

		/* Enable DMA transmit */
		QM_I2C[i2c]->ic_dma_cr = QM_I2C_IC_DMA_CR_TX_ENABLE;

		/* enable controller */
		controller_enable(i2c);

		/* Setup the DMA transfer */
		rc = qm_dma_transfer_set_config(
		    i2c_dma_context[i2c].dma_controller_id,
		    i2c_dma_context[i2c].dma_tx_channel_id,
		    &(i2c_dma_context[i2c].dma_tx_transfer_config));
		if (rc == 0) {
			/* Mark the TX operation as ongoing */
			i2c_dma_context[i2c].ongoing_dma_rx_operation = false;
			i2c_dma_context[i2c].ongoing_dma_tx_operation = true;
			rc = qm_dma_transfer_start(
			    i2c_dma_context[i2c].dma_controller_id,
			    i2c_dma_context[i2c].dma_tx_channel_id);
		}
	}

	return rc;
}
