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

#include "qm_adc.h"
#include "clk.h"
#include <string.h>

/* FIFO_INTERRUPT_THRESHOLD is used by qm_adc_irq_convert to set the threshold
 * at which the FIFO will trigger an interrupt. */
#define FIFO_INTERRUPT_THRESHOLD (16)

#define QM_ADC_CHAN_SEQ_MAX (32)

/* ADC commands. */
#define QM_ADC_CMD_START_SINGLE (0)
#define QM_ADC_CMD_START_CONT (1)
#define QM_ADC_CMD_RESET_CAL (2)
#define QM_ADC_CMD_START_CAL (3)
#define QM_ADC_CMD_LOAD_CAL (4)
#define QM_ADC_CMD_STOP_CONT (5)

static uint8_t sample_window[QM_ADC_NUM];
static qm_adc_resolution_t resolution[QM_ADC_NUM];

static qm_adc_xfer_t *irq_xfer[QM_ADC_NUM];
static uint32_t count[QM_ADC_NUM];
static bool dummy_conversion = false;

/* Callbacks for mode change and calibration. */
static void (*mode_callback[QM_ADC_NUM])(void *data, int error,
					 qm_adc_status_t status,
					 qm_adc_cb_source_t source);
static void (*cal_callback[QM_ADC_NUM])(void *data, int error,
					qm_adc_status_t status,
					qm_adc_cb_source_t source);
static void *mode_callback_data[QM_ADC_NUM];
static void *cal_callback_data[QM_ADC_NUM];

/* ISR handler for command/calibration complete. */
static void qm_adc_isr_handler(const qm_adc_t adc)
{
	uint32_t int_status = 0;
	uint32_t i, samples_to_read;

	int_status = QM_ADC[adc].adc_intr_status;

	/* FIFO overrun interrupt. */
	if (int_status & QM_ADC_INTR_STATUS_FO) {
		/* Stop the transfer. */
		QM_ADC[adc].adc_cmd = QM_ADC_CMD_STOP_CONT;
		/* Disable all interrupts. */
		QM_ADC[adc].adc_intr_enable = 0;
		/* Call the user callback. */
		if (irq_xfer[adc]->callback) {
			irq_xfer[adc]->callback(irq_xfer[adc]->callback_data,
						-EIO, QM_ADC_OVERFLOW,
						QM_ADC_TRANSFER);
		}
	}

	/* Continuous mode command complete interrupt. */
	if (int_status & QM_ADC_INTR_STATUS_CONT_CC) {
		/* Clear the interrupt. */
		QM_ADC[adc].adc_intr_status &= QM_ADC_INTR_STATUS_CONT_CC;

		/* Calculate the number of samples to read. */
		samples_to_read = QM_ADC[adc].adc_fifo_count;
		if (samples_to_read >
		    (irq_xfer[adc]->samples_len - count[adc])) {
			samples_to_read =
			    irq_xfer[adc]->samples_len - count[adc];
		}

		/* Copy data out of FIFO. The sample must be shifted right by
		 * 2, 4 or 6 bits for 10, 8 and 6 bit resolution respectively
		 * to get the correct value. */
		for (i = 0; i < samples_to_read; i++) {
			irq_xfer[adc]->samples[count[adc]] =
			    (QM_ADC[adc].adc_sample >>
			     (2 * (3 - resolution[adc])));
			count[adc]++;
		}

		/* Check if we have the requested number of samples, stop the
		 * conversion and call the user callback function. */
		if (count[adc] == irq_xfer[adc]->samples_len) {
			/* Stop the transfer. */
			QM_ADC[adc].adc_cmd = QM_ADC_CMD_STOP_CONT;
			/* Disable all interrupts. */
			QM_ADC[adc].adc_intr_enable = 0;
			/* Call the user callback. */
			if (irq_xfer[adc]->callback) {
				irq_xfer[adc]->callback(
				    irq_xfer[adc]->callback_data, 0,
				    QM_ADC_COMPLETE, QM_ADC_TRANSFER);
			}
		}
	}

	/* The Command Complete interrupt is currently used to notify of the
	 * completion of a calibration command or a dummy conversion. */
	if ((int_status & QM_ADC_INTR_STATUS_CC) && (!dummy_conversion)) {
		/* Disable and clear the Command Complete interrupt */
		QM_ADC[adc].adc_intr_enable &= ~QM_ADC_INTR_ENABLE_CC;
		QM_ADC[adc].adc_intr_status = QM_ADC_INTR_STATUS_CC;

		/* Call the user callback if it is set. */
		if (cal_callback[adc]) {
			cal_callback[adc](irq_xfer[adc]->callback_data, 0,
					  QM_ADC_IDLE, QM_ADC_CAL_COMPLETE);
		}
	}

	/* This dummy conversion is needed when switching to normal mode or
	 * normal mode with calibration. */
	if ((int_status & QM_ADC_INTR_STATUS_CC) && (dummy_conversion)) {
		/* Flush the FIFO to get rid of the dummy values. */
		QM_ADC[adc].adc_sample = QM_ADC_FIFO_CLEAR;
		/* Disable and clear the Command Complete interrupt. */
		QM_ADC[adc].adc_intr_enable &= ~QM_ADC_INTR_ENABLE_CC;
		QM_ADC[adc].adc_intr_status = QM_ADC_INTR_STATUS_CC;

		dummy_conversion = false;

		/* Call the user callback if it is set. */
		if (mode_callback[adc]) {
			mode_callback[adc](irq_xfer[adc]->callback_data, 0,
					   QM_ADC_IDLE, QM_ADC_MODE_CHANGED);
		}
	}
}

/* ISR handler for mode change. */
static void qm_adc_pwr_0_isr_handler(const qm_adc_t adc)
{
	/* Clear the interrupt. Note that this operates differently to the
	 * QM_ADC_INTR_STATUS regiseter because you have to write to the
	 * QM_ADC_OP_MODE register, Interrupt Enable bit to clear. */
	QM_ADC[adc].adc_op_mode &= ~QM_ADC_OP_MODE_IE;

	/* Perform a dummy conversion if we are transitioning to Normal Mode or
	 * Normal Mode With Calibration */
	if ((QM_ADC[adc].adc_op_mode & QM_ADC_OP_MODE_OM_MASK) >=
	    QM_ADC_MODE_NORM_CAL) {

		/* Set the first sequence register back to its default (ch 0) */
		QM_ADC[adc].adc_seq0 = QM_ADC_CAL_SEQ_TABLE_DEFAULT;
		/* Clear the command complete interrupt status field */
		QM_ADC[adc].adc_intr_status = QM_ADC_INTR_STATUS_CC;

		dummy_conversion = true;

		/* Run a dummy conversion */
		QM_ADC[adc].adc_cmd = (QM_ADC_CMD_IE | QM_ADC_CMD_START_SINGLE);
	} else {
		/* Call the user callback function */
		if (mode_callback[adc]) {
			mode_callback[adc](irq_xfer[adc]->callback_data, 0,
					   QM_ADC_IDLE, QM_ADC_MODE_CHANGED);
		}
	}
}

/* ISR for ADC 0 Command/Calibration Complete. */
QM_ISR_DECLARE(qm_adc_0_cal_isr)
{
	qm_adc_isr_handler(QM_ADC_0);

	QM_ISR_EOI(QM_IRQ_ADC_0_CAL_INT_VECTOR);
}

/* ISR for ADC 0 Mode Change. */
QM_ISR_DECLARE(qm_adc_0_pwr_isr)
{
	qm_adc_pwr_0_isr_handler(QM_ADC_0);

	QM_ISR_EOI(QM_IRQ_ADC_0_PWR_0_VECTOR);
}

static void setup_seq_table(const qm_adc_t adc, qm_adc_xfer_t *xfer)
{
	uint32_t i, offset = 0;
	volatile uint32_t *reg_pointer = NULL;

	/* Loop over all of the channels to be added. */
	for (i = 0; i < xfer->ch_len; i++) {
		/* Get a pointer to the correct address. */
		reg_pointer = &QM_ADC[adc].adc_seq0 + (i / 4);
		/* Get the offset within the register */
		offset = ((i % 4) * 8);
		/* Clear the Last bit from all entries we will use. */
		*reg_pointer &= ~(1 << (offset + 7));
		/* Place the channel numnber into the sequence table. */
		*reg_pointer |= (xfer->ch[i] << offset);
	}

	if (reg_pointer) {
		/* Set the correct Last bit. */
		*reg_pointer |= (1 << (offset + 7));
	}
}

int qm_adc_calibrate(const qm_adc_t adc)
{
	QM_CHECK(adc < QM_ADC_NUM, -EINVAL);

	/* Clear the command complete interrupt status field. */
	QM_ADC[adc].adc_intr_status = QM_ADC_INTR_STATUS_CC;
	/* Start the calibration and wait for it to complete. */
	QM_ADC[adc].adc_cmd = (QM_ADC_CMD_IE | QM_ADC_CMD_START_CAL);
	while (!(QM_ADC[adc].adc_intr_status & QM_ADC_INTR_STATUS_CC))
		;
	/* Clear the command complete interrupt status field again. */
	QM_ADC[adc].adc_intr_status = QM_ADC_INTR_STATUS_CC;

	return 0;
}

int qm_adc_irq_calibrate(const qm_adc_t adc,
			 void (*callback)(void *data, int error,
					  qm_adc_status_t status,
					  qm_adc_cb_source_t source),
			 void *callback_data)
{
	QM_CHECK(adc < QM_ADC_NUM, -EINVAL);

	/* Set the callback. */
	cal_callback[adc] = callback;
	cal_callback_data[adc] = callback_data;

	/* Clear and enable the command complete interrupt. */
	QM_ADC[adc].adc_intr_status = QM_ADC_INTR_STATUS_CC;
	QM_ADC[adc].adc_intr_enable |= QM_ADC_INTR_ENABLE_CC;

	/* Start the calibration */
	QM_ADC[adc].adc_cmd = (QM_ADC_CMD_IE | QM_ADC_CMD_START_CAL);

	return 0;
}

int qm_adc_set_calibration(const qm_adc_t adc, const qm_adc_calibration_t cal)
{
	QM_CHECK(adc < QM_ADC_NUM, -EINVAL);
	QM_CHECK(cal < 0x3F, -EINVAL);

	/* Clear the command complete interrupt status field. */
	QM_ADC[adc].adc_intr_status = QM_ADC_INTR_STATUS_CC;

	/* Set the calibration data and wait for it to complete. */
	QM_ADC[adc].adc_cmd = ((cal << QM_ADC_CMD_CAL_DATA_OFFSET) |
			       QM_ADC_CMD_IE | QM_ADC_CMD_LOAD_CAL);
	while (!(QM_ADC[adc].adc_intr_status & QM_ADC_INTR_STATUS_CC))
		;
	/* Clear the command complete interrupt status field again. */
	QM_ADC[adc].adc_intr_status = QM_ADC_INTR_STATUS_CC;

	return 0;
}

int qm_adc_get_calibration(const qm_adc_t adc, qm_adc_calibration_t *const cal)
{
	QM_CHECK(adc < QM_ADC_NUM, -EINVAL);
	QM_CHECK(NULL != cal, -EINVAL);

	*cal = QM_ADC[adc].adc_calibration;

	return 0;
}

int qm_adc_set_mode(const qm_adc_t adc, const qm_adc_mode_t mode)
{
	QM_CHECK(adc < QM_ADC_NUM, -EINVAL);
	QM_CHECK(mode <= QM_ADC_MODE_NORM_NO_CAL, -EINVAL);

	/* Issue mode change command and wait for it to complete. */
	QM_ADC[adc].adc_op_mode = mode;
	while ((QM_ADC[adc].adc_op_mode & QM_ADC_OP_MODE_OM_MASK) != mode)
		;

	/* Perform a dummy conversion if we are transitioning to Normal Mode. */
	if ((mode >= QM_ADC_MODE_NORM_CAL)) {
		/* Set the first sequence register back to its default. */
		QM_ADC[adc].adc_seq0 = QM_ADC_CAL_SEQ_TABLE_DEFAULT;

		/* Clear the command complete interrupt status field. */
		QM_ADC[adc].adc_intr_status = QM_ADC_INTR_STATUS_CC;
		/* Run a dummy convert and wait for it to complete. */
		QM_ADC[adc].adc_cmd = (QM_ADC_CMD_IE | QM_ADC_CMD_START_SINGLE);
		while (!(QM_ADC[adc].adc_intr_status & QM_ADC_INTR_STATUS_CC))
			;

		/* Flush the FIFO to get rid of the dummy values. */
		QM_ADC[adc].adc_sample = QM_ADC_FIFO_CLEAR;
		/* Clear the command complete interrupt status field. */
		QM_ADC[adc].adc_intr_status = QM_ADC_INTR_STATUS_CC;
	}

	return 0;
}

int qm_adc_irq_set_mode(const qm_adc_t adc, const qm_adc_mode_t mode,
			void (*callback)(void *data, int error,
					 qm_adc_status_t status,
					 qm_adc_cb_source_t source),
			void *callback_data)
{
	QM_CHECK(adc < QM_ADC_NUM, -EINVAL);
	QM_CHECK(mode <= QM_ADC_MODE_NORM_NO_CAL, -EINVAL);

	/* Set the callback. */
	mode_callback[adc] = callback;
	mode_callback_data[adc] = callback_data;

	/* When transitioning to Normal Mode or Normal Mode With Calibration,
	 * enable command complete interrupt to perform a dummy conversion. */
	if ((mode >= QM_ADC_MODE_NORM_CAL)) {
		QM_ADC[adc].adc_intr_enable |= QM_ADC_INTR_ENABLE_CC;
	}

	/* Issue mode change command. Completion if this command is notified via
	 * the ADC Power interrupt source, which is serviced separately to the
	 * Command/Calibration Complete interrupt. */
	QM_ADC[adc].adc_op_mode = (QM_ADC_OP_MODE_IE | mode);

	return 0;
}

int qm_adc_set_config(const qm_adc_t adc, const qm_adc_config_t *const cfg)
{
	QM_CHECK(adc < QM_ADC_NUM, -EINVAL);
	QM_CHECK(NULL != cfg, -EINVAL);
	QM_CHECK(cfg->resolution <= QM_ADC_RES_12_BITS, -EINVAL);
	/* Convert cfg->resolution to actual resolution (2x+6) and add 2 to get
	 * minimum value for window size. */
	QM_CHECK(cfg->window >= ((cfg->resolution * 2) + 8), -EINVAL);

	/* Set the sample window and resolution. */
	sample_window[adc] = cfg->window;
	resolution[adc] = cfg->resolution;

	return 0;
}

int qm_adc_convert(const qm_adc_t adc, qm_adc_xfer_t *xfer,
		   qm_adc_status_t *const status)
{
	uint32_t i;

	QM_CHECK(adc < QM_ADC_NUM, -EINVAL);
	QM_CHECK(NULL != xfer, -EINVAL);
	QM_CHECK(NULL != xfer->ch, -EINVAL);
	QM_CHECK(NULL != xfer->samples, -EINVAL);
	QM_CHECK(xfer->ch_len > 0, -EINVAL);
	QM_CHECK(xfer->ch_len <= QM_ADC_CHAN_SEQ_MAX, -EINVAL);
	QM_CHECK(xfer->samples_len > 0, -EINVAL);
	QM_CHECK(xfer->samples_len <= QM_ADC_FIFO_LEN, -EINVAL);

	/* Flush the FIFO. */
	QM_ADC[adc].adc_sample = QM_ADC_FIFO_CLEAR;

	/* Populate the sample sequence table. */
	setup_seq_table(adc, xfer);

	/* Issue cmd: window & resolution, number of samples, command. */
	QM_ADC[adc].adc_cmd =
	    (sample_window[adc] << QM_ADC_CMD_SW_OFFSET |
	     resolution[adc] << QM_ADC_CMD_RESOLUTION_OFFSET |
	     ((xfer->samples_len - 1) << QM_ADC_CMD_NS_OFFSET) |
	     QM_ADC_CMD_START_SINGLE);

	/* Wait for fifo count to reach number of samples. */
	while (QM_ADC[adc].adc_fifo_count != xfer->samples_len)
		;

	if (status) {
		*status = QM_ADC_COMPLETE;
	}

	/* Read the value into the data structure. The sample must be shifted
	 * right by 2, 4 or 6 bits for 10, 8 and 6 bit resolution respectively
	 * to get the correct value. */
	for (i = 0; i < xfer->samples_len; i++) {
		xfer->samples[i] =
		    (QM_ADC[adc].adc_sample >> (2 * (3 - resolution[adc])));
	}

	return 0;
}

int qm_adc_irq_convert(const qm_adc_t adc, qm_adc_xfer_t *xfer)
{
	QM_CHECK(adc < QM_ADC_NUM, -EINVAL);
	QM_CHECK(NULL != xfer, -EINVAL);
	QM_CHECK(NULL != xfer->ch, -EINVAL);
	QM_CHECK(NULL != xfer->samples, -EINVAL);
	QM_CHECK(xfer->ch_len > 0, -EINVAL);
	QM_CHECK(xfer->ch_len <= QM_ADC_CHAN_SEQ_MAX, -EINVAL);
	QM_CHECK(xfer->samples_len > 0, -EINVAL);

	/* Reset the count and flush the FIFO. */
	count[adc] = 0;
	QM_ADC[adc].adc_sample = QM_ADC_FIFO_CLEAR;

	/* Populate the sample sequence table. */
	setup_seq_table(adc, xfer);

	/* Copy the xfer struct so we can get access from the ISR. */
	irq_xfer[adc] = xfer;

	/* Clear all pending interrupts. */
	QM_ADC[adc].adc_intr_status = QM_ADC_INTR_STATUS_CC |
				      QM_ADC_INTR_STATUS_FO |
				      QM_ADC_INTR_STATUS_CONT_CC;
	/* Enable the continuous command and fifo overrun interupts. */
	QM_ADC[adc].adc_intr_enable =
	    QM_ADC_INTR_ENABLE_FO | QM_ADC_INTR_ENABLE_CONT_CC;

	/* Issue cmd: window & resolution, number of samples, interrupt enable
	 * and start continuous coversion command. If xfer->samples_len is less
	 * than FIFO_INTERRUPT_THRESHOLD extra samples will be discarded in the
	 * ISR. */
	QM_ADC[adc].adc_cmd =
	    (sample_window[adc] << QM_ADC_CMD_SW_OFFSET |
	     resolution[adc] << QM_ADC_CMD_RESOLUTION_OFFSET |
	     ((FIFO_INTERRUPT_THRESHOLD - 1) << QM_ADC_CMD_NS_OFFSET) |
	     QM_ADC_CMD_IE | QM_ADC_CMD_START_CONT);

	return 0;
}
