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

#include "qm_i2c.h"
#include "qm_scss.h"

#define TX_TL (2)
#define RX_TL (5)
#define SPK_LEN_SS (1)
#define SPK_LEN_FS_FSP (2)

static qm_i2c_speed_t i2c_speed_mode[QM_I2C_NUM];

static qm_i2c_transfer_t i2c_transfer[QM_I2C_NUM];
static uint32_t i2c_write_pos[QM_I2C_NUM], i2c_read_pos[QM_I2C_NUM],
    i2c_read_buffer_remaining[QM_I2C_NUM];

static qm_rc_t get_tx_abrt_status(const qm_i2c_t i2c);
static void controller_enable(const qm_i2c_t i2c);
static void controller_disable(const qm_i2c_t i2c);

static void qm_i2c_isr_handler(const qm_i2c_t i2c)
{
	uint32_t ic_data_cmd = 0, count_tx = (QM_I2C_FIFO_SIZE - TX_TL);
	qm_i2c_status_t status = 0;

	/* Check for errors */
	QM_ASSERT(!(QM_I2C[i2c].ic_intr_stat & QM_I2C_IC_INTR_STAT_TX_OVER));
	QM_ASSERT(!(QM_I2C[i2c].ic_intr_stat & QM_I2C_IC_INTR_STAT_RX_UNDER));
	QM_ASSERT(!(QM_I2C[i2c].ic_intr_stat & QM_I2C_IC_INTR_STAT_RX_OVER));

	if (QM_I2C[i2c].ic_intr_stat & QM_I2C_IC_INTR_STAT_TX_ABRT) {
		QM_ASSERT(!(QM_I2C[i2c].ic_tx_abrt_source &
			    QM_I2C_IC_TX_ABRT_SOURCE_ABRT_SBYTE_NORSTRT));
		status = (QM_I2C[i2c].ic_tx_abrt_source &
			  QM_I2C_IC_TX_ABRT_SOURCE_ALL_MASK);

		/* clear intr */
		QM_I2C[i2c].ic_clr_tx_abrt;

		/* mask interrupts */
		QM_I2C[i2c].ic_intr_mask = QM_I2C_IC_INTR_MASK_ALL;

		if (status) {
			i2c_transfer[i2c].err_callback(i2c_transfer[i2c].id,
						       status);
			controller_disable(i2c);
		}
	}

	/* RX read from buffer */
	if (QM_I2C[i2c].ic_intr_stat & QM_I2C_IC_INTR_STAT_RX_FULL) {

		while (i2c_read_buffer_remaining[i2c] && QM_I2C[i2c].ic_rxflr) {
			i2c_transfer[i2c].rx[i2c_read_pos[i2c]] =
			    QM_I2C[i2c].ic_data_cmd;
			i2c_read_buffer_remaining[i2c]--;
			i2c_read_pos[i2c]++;

			if (i2c_read_buffer_remaining[i2c] == 0) {
				/* mask rx full interrupt if transfer
				 * complete
				 */
				QM_I2C[i2c].ic_intr_mask &=
				    ~QM_I2C_IC_INTR_MASK_RX_FULL;

				if (i2c_transfer[i2c].stop) {
					controller_disable(i2c);
				}

				i2c_transfer[i2c].rx_callback(
				    i2c_transfer[i2c].id, i2c_read_pos[i2c]);
			}
		}
		if (i2c_read_buffer_remaining[i2c] < (RX_TL + 1)) {
			QM_I2C[i2c].ic_rx_tl = 0;
		}

		/* RX_FULL INTR is autocleared when the buffer
		 * levels goes below the threshold
		 */
	}

	if (QM_I2C[i2c].ic_intr_stat & QM_I2C_IC_INTR_STAT_TX_EMPTY) {

		if ((QM_I2C[i2c].ic_status & QM_I2C_IC_STATUS_TFE) &&
		    (i2c_transfer[i2c].tx != NULL) &&
		    (i2c_transfer[i2c].tx_len == 0) &&
		    (i2c_transfer[i2c].rx_len == 0)) {

			QM_I2C[i2c].ic_intr_mask &=
			    ~QM_I2C_IC_INTR_MASK_TX_EMPTY;

			/* if this is not a combined
			 * transaction, disable the controller now
			 */
			if ((i2c_read_buffer_remaining[i2c] == 0) &&
			    i2c_transfer[i2c].stop) {
				controller_disable(i2c);
			}

			/* write complete callback */
			i2c_transfer[i2c].tx_callback(i2c_transfer[i2c].id,
						      i2c_write_pos[i2c]);
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
			QM_I2C[i2c].ic_data_cmd = ic_data_cmd;
			i2c_write_pos[i2c]++;

			/* TX_EMPTY INTR is autocleared when the buffer
			 * levels goes above the threshold
			 */
		}

		/* TX read command */
		count_tx = QM_I2C_FIFO_SIZE -
			   (QM_I2C[i2c].ic_txflr + QM_I2C[i2c].ic_rxflr + 1);

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
				QM_I2C[i2c].ic_data_cmd =
				    QM_I2C_IC_DATA_CMD_READ |
				    QM_I2C_IC_DATA_CMD_STOP_BIT_CTRL;
			} else {
				QM_I2C[i2c].ic_data_cmd =
				    QM_I2C_IC_DATA_CMD_READ;
			}
		}

		/* generate a tx_empty interrupt when tx fifo is fully empty */
		if ((i2c_transfer[i2c].tx_len == 0) &&
		    (i2c_transfer[i2c].rx_len == 0)) {
			QM_I2C[i2c].ic_tx_tl = 0;
		}
	}
}

void qm_i2c_0_isr(void)
{
	qm_i2c_isr_handler(QM_I2C_0);
	QM_ISR_EOI(QM_IRQ_I2C_0_VECTOR);
}

#if (QUARK_SE)
void qm_i2c_1_isr(void)
{
	qm_i2c_isr_handler(QM_I2C_1);
	QM_ISR_EOI(QM_IRQ_I2C_1_VECTOR);
}
#endif

static uint32_t get_lo_cnt(qm_i2c_t i2c, uint32_t lo_time_ns)
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
		 7 - QM_I2C[i2c].ic_fs_spklen) +
		1);
}

qm_rc_t qm_i2c_set_config(const qm_i2c_t i2c, const qm_i2c_config_t *const cfg)
{
	uint32_t lcnt = 0, hcnt = 0, min_lcnt = 0, lcnt_diff = 0, ic_con = 0;
	QM_CHECK(i2c < QM_I2C_NUM, QM_RC_EINVAL);
	QM_CHECK(cfg != NULL, QM_RC_EINVAL);

	/* mask all interrupts */
	QM_I2C[i2c].ic_intr_mask = QM_I2C_IC_INTR_MASK_ALL;

	/* disable controller */
	controller_disable(i2c);

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

			QM_I2C[i2c].ic_fs_spklen = SPK_LEN_SS;

			min_lcnt = get_lo_cnt(i2c, QM_I2C_MIN_SS_NS);
			lcnt = get_lo_cnt(i2c, QM_I2C_SS_50_DC_NS);
			hcnt = get_hi_cnt(i2c, QM_I2C_SS_50_DC_NS);
			break;

		case QM_I2C_SPEED_FAST:
			ic_con |= QM_I2C_IC_CON_SPEED_FS_FSP;

			QM_I2C[i2c].ic_fs_spklen = SPK_LEN_FS_FSP;

			min_lcnt = get_lo_cnt(i2c, QM_I2C_MIN_FS_NS);
			lcnt = get_lo_cnt(i2c, QM_I2C_FS_50_DC_NS);
			hcnt = get_hi_cnt(i2c, QM_I2C_FS_50_DC_NS);
			break;

		case QM_I2C_SPEED_FAST_PLUS:
			ic_con |= QM_I2C_IC_CON_SPEED_FS_FSP;

			QM_I2C[i2c].ic_fs_spklen = SPK_LEN_FS_FSP;

			min_lcnt = get_lo_cnt(i2c, QM_I2C_MIN_FSP_NS);
			lcnt = get_lo_cnt(i2c, QM_I2C_FSP_50_DC_NS);
			hcnt = get_hi_cnt(i2c, QM_I2C_FSP_50_DC_NS);
			break;
		}

		if (hcnt > QM_I2C_IC_HCNT_MAX || hcnt < QM_I2C_IC_HCNT_MIN) {
			return QM_RC_ERROR;
		}

		if (lcnt > QM_I2C_IC_LCNT_MAX || lcnt < QM_I2C_IC_LCNT_MIN) {
			return QM_RC_ERROR;
		}

		/* Increment minimum low count to account for rounding down */
		min_lcnt++;
		if (lcnt < min_lcnt) {
			lcnt_diff = (min_lcnt - lcnt);
			lcnt += (lcnt_diff);
			hcnt -= (lcnt_diff);
		}
		if (QM_I2C_SPEED_STD == cfg->speed) {
			QM_I2C[i2c].ic_ss_scl_lcnt = lcnt;
			QM_I2C[i2c].ic_ss_scl_hcnt = hcnt;
		} else {
			QM_I2C[i2c].ic_fs_scl_hcnt = hcnt;
			QM_I2C[i2c].ic_fs_scl_lcnt = lcnt;
		}

		i2c_speed_mode[i2c] = cfg->speed;
		break;

	case QM_I2C_SLAVE:
		/* QM_I2C_IC_CON_MASTER_MODE and
		 * QM_I2C_IC_CON_SLAVE_DISABLE are
		 * deasserted */

		/* set 7/10 bit address mode */
		ic_con = cfg->address_mode
			 << QM_I2C_IC_CON_10BITADDR_SLAVE_OFFSET;

		/* set slave address */
		QM_I2C[i2c].ic_sar = cfg->slave_addr;
		break;
	}

	QM_I2C[i2c].ic_con = ic_con;
	return QM_RC_OK;
}

qm_rc_t qm_i2c_get_config(const qm_i2c_t i2c, qm_i2c_config_t *const cfg)
{
	QM_CHECK(i2c < QM_I2C_NUM, QM_RC_EINVAL);
	QM_CHECK(cfg != NULL, QM_RC_EINVAL);

	cfg->mode = (QM_I2C[i2c].ic_con & QM_I2C_IC_CON_MASTER_MODE)
			? QM_I2C_MASTER
			: QM_I2C_SLAVE;

	switch (cfg->mode) {
	case QM_I2C_MASTER:
		cfg->address_mode =
		    (QM_I2C[i2c].ic_con & QM_I2C_IC_CON_10BITADDR_MASTER) >>
		    QM_I2C_IC_CON_10BITADDR_MASTER_OFFSET;
		cfg->speed = i2c_speed_mode[i2c];
		break;

	case QM_I2C_SLAVE:
		cfg->address_mode =
		    (QM_I2C[i2c].ic_con & QM_I2C_IC_CON_10BITADDR_SLAVE) >>
		    QM_I2C_IC_CON_10BITADDR_SLAVE_OFFSET;
		cfg->slave_addr = QM_I2C[i2c].ic_sar;
		break;
	}

	return QM_RC_OK;
}

qm_rc_t qm_i2c_set_speed(const qm_i2c_t i2c, qm_i2c_speed_t speed,
			 uint16_t lo_cnt, uint16_t hi_cnt)
{
	uint32_t ic_con = QM_I2C[i2c].ic_con;
	QM_CHECK(i2c < QM_I2C_NUM, QM_RC_EINVAL);
	QM_CHECK(hi_cnt < QM_I2C_IC_HCNT_MAX && lo_cnt > QM_I2C_IC_HCNT_MIN,
		 QM_RC_EINVAL);
	QM_CHECK(lo_cnt < QM_I2C_IC_LCNT_MAX && lo_cnt > QM_I2C_IC_LCNT_MIN,
		 QM_RC_EINVAL);

	ic_con &= ~QM_I2C_IC_CON_SPEED_MASK;

	switch (speed) {
	case QM_I2C_SPEED_STD:
		ic_con |= QM_I2C_IC_CON_SPEED_SS;
		QM_I2C[i2c].ic_ss_scl_lcnt = lo_cnt;
		QM_I2C[i2c].ic_ss_scl_hcnt = hi_cnt;
		break;

	case QM_I2C_SPEED_FAST:
	case QM_I2C_SPEED_FAST_PLUS:
		ic_con |= QM_I2C_IC_CON_SPEED_FS_FSP;
		QM_I2C[i2c].ic_fs_scl_lcnt = lo_cnt;
		QM_I2C[i2c].ic_fs_scl_hcnt = hi_cnt;
		break;
	}

	i2c_speed_mode[i2c] = speed;
	QM_I2C[i2c].ic_con = ic_con;

	return QM_RC_OK;
}

qm_i2c_status_t qm_i2c_get_status(const qm_i2c_t i2c)
{
	/* check if slave or master are active */
	if (QM_I2C[i2c].ic_status & QM_I2C_IC_STATUS_BUSY_MASK) {
		return QM_I2C_BUSY;
	} else {
		return QM_I2C_IDLE;
	}
}

qm_rc_t qm_i2c_master_write(const qm_i2c_t i2c, const uint16_t slave_addr,
			    const uint8_t *const data, uint32_t len, bool stop)
{
	uint8_t *d = (uint8_t *)data;
	uint32_t ic_data_cmd = 0;
	qm_rc_t ret = QM_RC_OK;

	QM_CHECK(i2c < QM_I2C_NUM, QM_RC_EINVAL);
	QM_CHECK(data != NULL, QM_RC_EINVAL);
	QM_CHECK(len > 0, QM_RC_EINVAL);

	/* write slave address to TAR */
	QM_I2C[i2c].ic_tar = slave_addr;

	/* enable controller */
	controller_enable(i2c);

	while (len--) {

		/* wait if FIFO is full */
		while (!(QM_I2C[i2c].ic_status & QM_I2C_IC_STATUS_TNF))
			;

		/* write command -IC_DATA_CMD[8] = 0 */
		/* fill IC_DATA_CMD[7:0] with the data */
		ic_data_cmd = *d;

		/* send stop after last byte */
		if (len == 0 && stop) {
			ic_data_cmd |= QM_I2C_IC_DATA_CMD_STOP_BIT_CTRL;
		}

		QM_I2C[i2c].ic_data_cmd = ic_data_cmd;
		d++;
	}

	/* this is a blocking call, wait until FIFO is empty or tx abrt
	 * error */
	while (!(QM_I2C[i2c].ic_status & QM_I2C_IC_STATUS_TFE))
		;

	ret = get_tx_abrt_status(i2c);

	/*  disable controller */
	if (true == stop) {
		controller_disable(i2c);
	}

	return ret;
}

qm_rc_t qm_i2c_master_read(const qm_i2c_t i2c, const uint16_t slave_addr,
			   uint8_t *const data, uint32_t len, bool stop)
{
	uint8_t *d = (uint8_t *)data;
	qm_rc_t ret = QM_RC_OK;

	QM_CHECK(i2c < QM_I2C_NUM, QM_RC_EINVAL);
	QM_CHECK(data != NULL, QM_RC_EINVAL);
	QM_CHECK(len > 0, QM_RC_EINVAL);

	/* write slave address to TAR */
	QM_I2C[i2c].ic_tar = slave_addr;

	/* enable controller */
	controller_enable(i2c);

	while (len--) {
		if (len == 0 && stop) {
			QM_I2C[i2c].ic_data_cmd =
			    QM_I2C_IC_DATA_CMD_READ |
			    QM_I2C_IC_DATA_CMD_STOP_BIT_CTRL;
		}

		else {
			/*  read command -IC_DATA_CMD[8] = 1 */
			QM_I2C[i2c].ic_data_cmd = QM_I2C_IC_DATA_CMD_READ;
		}

		/*  wait if rx fifo is empty, break if tx empty and
		 * error*/
		while (!(QM_I2C[i2c].ic_status & QM_I2C_IC_STATUS_RFNE)) {

			if (QM_I2C[i2c].ic_raw_intr_stat &
			    QM_I2C_IC_RAW_INTR_STAT_TX_ABRT) {
				break;
			}
		}

		ret = get_tx_abrt_status(i2c);

		if (ret != QM_RC_OK) {
			break;
		}

		/* IC_DATA_CMD[7:0] contains received data */
		*d = QM_I2C[i2c].ic_data_cmd;
		d++;
	}

	/*  disable controller */
	if (true == stop) {
		controller_disable(i2c);
	}

	return ret;
}

static qm_rc_t get_tx_abrt_status(const qm_i2c_t i2c)
{
	qm_rc_t ret = QM_RC_OK;

	/* Check for errors:
	 * read raw_intr_status tx_abrt - The controller
	 * flushes/resets/empties
	 * the TX FIFO whenever this bit is set. The TX
	 * FIFO remains in this flushed state until the
	 * register IC_CLR_TX_ABRT is read.
	 * */
	if ((QM_I2C[i2c].ic_raw_intr_stat & QM_I2C_IC_RAW_INTR_STAT_TX_ABRT)) {
		/* read abort status */
		if (QM_I2C[i2c].ic_tx_abrt_source &
		    QM_I2C_IC_TX_ABRT_SOURCE_NAK_MASK) {

			ret = QM_RC_I2C_NAK;
		}

		else if (QM_I2C[i2c].ic_tx_abrt_source &
			 QM_I2C_IC_TX_ABRT_SOURCE_ARB_LOST) {

			ret = QM_RC_I2C_ARB_LOST;
		}

		else {
			ret = QM_RC_ERROR;
		}

		/* clear abort */
		QM_I2C[i2c].ic_clr_tx_abrt;
	}

	return ret;
}

qm_rc_t qm_i2c_master_irq_transfer(const qm_i2c_t i2c,
				   const qm_i2c_transfer_t *const xfer,
				   const uint16_t slave_addr)
{
	QM_CHECK(i2c < QM_I2C_NUM, QM_RC_EINVAL);
	QM_CHECK(NULL != xfer, QM_RC_EINVAL);
	QM_CHECK(NULL != xfer->err_callback, QM_RC_EINVAL);
	QM_CHECK(NULL != xfer->tx ? NULL != xfer->tx_callback : 1,
		 QM_RC_EINVAL);
	QM_CHECK(NULL != xfer->rx ? NULL != xfer->rx_callback : 1,
		 QM_RC_EINVAL);

	/* write slave address to TAR */
	QM_I2C[i2c].ic_tar = slave_addr;

	i2c_write_pos[i2c] = 0;
	i2c_read_pos[i2c] = 0;
	i2c_read_buffer_remaining[i2c] = xfer->rx_len;
	i2c_transfer[i2c].tx_len = xfer->tx_len;
	i2c_transfer[i2c].rx_len = xfer->rx_len;
	i2c_transfer[i2c].tx = xfer->tx;
	i2c_transfer[i2c].rx = xfer->rx;
	i2c_transfer[i2c].tx_callback = xfer->tx_callback;
	i2c_transfer[i2c].rx_callback = xfer->rx_callback;
	i2c_transfer[i2c].err_callback = xfer->err_callback;
	i2c_transfer[i2c].id = xfer->id;
	i2c_transfer[i2c].stop = xfer->stop;

	/* set threshold */
	QM_I2C[i2c].ic_tx_tl = TX_TL;
	if (xfer->rx_len > 0 && xfer->rx_len < (RX_TL + 1)) {
		/* If 'rx_len' is less than the default threshold, we have to
		 * change the threshold value so the 'RX FULL' interrupt is
		 * generated once all data from the transfer is received.
		 */
		QM_I2C[i2c].ic_rx_tl = xfer->rx_len - 1;
	} else {
		QM_I2C[i2c].ic_rx_tl = RX_TL;
	}

	/* mask interrupts */
	QM_I2C[i2c].ic_intr_mask = QM_I2C_IC_INTR_MASK_ALL;

	/* enable controller */
	controller_enable(i2c);

	/* unmask interrupts */
	QM_I2C[i2c].ic_intr_mask |=
	    QM_I2C_IC_INTR_MASK_RX_UNDER | QM_I2C_IC_INTR_MASK_RX_OVER |
	    QM_I2C_IC_INTR_MASK_RX_FULL | QM_I2C_IC_INTR_MASK_TX_OVER |
	    QM_I2C_IC_INTR_MASK_TX_EMPTY | QM_I2C_IC_INTR_MASK_TX_ABORT;

	return QM_RC_OK;
}

static void controller_enable(const qm_i2c_t i2c)
{
	if (!(QM_I2C[i2c].ic_enable_status & QM_I2C_IC_ENABLE_STATUS_IC_EN)) {
		/* enable controller */
		QM_I2C[i2c].ic_enable |= QM_I2C_IC_ENABLE_CONTROLLER_EN;

		/* wait until controller is enabled */
		while (!(QM_I2C[i2c].ic_enable_status &
			 QM_I2C_IC_ENABLE_STATUS_IC_EN))
			;
	}
}

static void controller_disable(const qm_i2c_t i2c)
{
	if (QM_I2C[i2c].ic_enable_status & QM_I2C_IC_ENABLE_STATUS_IC_EN) {
		/* disable controller */
		QM_I2C[i2c].ic_enable &= ~QM_I2C_IC_ENABLE_CONTROLLER_EN;

		/* wait until controller is disabled */
		while ((QM_I2C[i2c].ic_enable_status &
			QM_I2C_IC_ENABLE_STATUS_IC_EN))
			;
	}
}

qm_rc_t qm_i2c_transfer_terminate(const qm_i2c_t i2c)
{
	QM_CHECK(i2c < QM_I2C_NUM, QM_RC_EINVAL);

	/* Abort:
	 * In response to an ABORT, the controller issues a STOP and
	 * flushes
	 * the Tx FIFO after completing the current transfer, then sets
	 * the
	 * TX_ABORT interrupt after the abort operation. The ABORT bit
	 * is
	 * cleared automatically by hardware after the abort operation.
	 */
	QM_I2C[i2c].ic_enable |= QM_I2C_IC_ENABLE_CONTROLLER_ABORT;

	return QM_RC_OK;
}
