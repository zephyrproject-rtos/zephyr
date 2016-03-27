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

#include "qm_adc.h"
#include "qm_scss.h"
#include <string.h>

#if (QUARK_D2000)

#define SAMPLE_BLOCK_LEN (16)

#define QM_ADC_CHAN_SEQ_MAX (32)

/* ADC commands */
#define QM_ADC_CMD_START_SINGLE (0)
#define QM_ADC_CMD_START_CONT (1)
#define QM_ADC_CMD_START_CAL (3)
#define QM_ADC_CMD_STOP_CONT (5)

/* It is neceesary to store some of the fields within the ADC_CMD as writing
 * them will immediately kick off a conversion or calibration
 */
static uint32_t adc_cmd[QM_ADC_NUM];

static qm_adc_xfer_t irq_xfer[QM_ADC_NUM];
static uint32_t count[QM_ADC_NUM];

/* ISR handler */
static void qm_adc_isr_handler(const qm_adc_t adc)
{
	uint32_t int_status = 0;
	uint32_t i, samples_to_read;

	int_status = QM_ADC[adc].adc_intr_status;

	/* FIFO overrun interrupt */
	if (int_status & QM_ADC_INTR_STATUS_FO) {
		/* Stop the transfer */
		QM_ADC[adc].adc_cmd = QM_ADC_CMD_STOP_CONT;
		/* Disable all interrupts */
		QM_ADC[adc].adc_intr_enable = 0;
		/* Call the user callback */
		irq_xfer[adc].error_callback();
	}

	/* Continuous mode command complete interrupt */
	if (int_status & QM_ADC_INTR_STATUS_CONT_CC) {
		/* Clear the interrupt */
		QM_ADC[adc].adc_intr_status &= QM_ADC_INTR_STATUS_CONT_CC;

		/* Figure out how many samples to read */
		if ((count[adc] + SAMPLE_BLOCK_LEN) <= irq_xfer->samples_len) {
			samples_to_read = SAMPLE_BLOCK_LEN;
		} else {
			samples_to_read = irq_xfer->samples_len - count[adc];
		}

		/* Copy data out of FIFO */
		for (i = 0; i < samples_to_read; i++) {
			irq_xfer->samples[count[adc]] = QM_ADC[adc].adc_sample;
			count[adc]++;
		}
	}

	/* Check if we have the requested number of samples, stop the conversion
	 * and call the user callback function */
	if (count[adc] == irq_xfer->samples_len) {
		/* Stop the transfer */
		QM_ADC[adc].adc_cmd = QM_ADC_CMD_STOP_CONT;
		/* Disable all interrupts */
		QM_ADC[adc].adc_intr_enable = 0;
		/* Call the user callback */
		irq_xfer[adc].complete_callback();
	}

	/* We do not currently handle the Command Complete interrupt as it is
	 * not being used anywhere */
}

/* ISR for ADC 0 */
void qm_adc_0_isr(void)
{
	qm_adc_isr_handler(QM_ADC_0);

	QM_ISR_EOI(QM_IRQ_ADC_0_VECTOR);
}

static void setup_seq_table(const qm_adc_t adc, qm_adc_xfer_t *xfer)
{
	uint32_t i, offset = 0;
	volatile uint32_t *reg_pointer = NULL;

	/* Loop over all of the channels to be added */
	for (i = 0; i < xfer->ch_len; i++) {
		/* Get a pointer to the correct address */
		reg_pointer = &QM_ADC[adc].adc_seq0 + (i / 4);
		/* Get the offset within the register */
		offset = ((i % 4) * 8);
		/* Clear the Last bit from all entries we will use */
		*reg_pointer &= ~(1 << (offset + 7));
		/* Place the channel numnber into the sequence table */
		*reg_pointer |= (xfer->ch[i] << offset);
	}

	if (reg_pointer) {
		/* Set the correct Last bit */
		*reg_pointer |= (1 << (offset + 7));
	}
}

qm_rc_t qm_adc_calibrate(const qm_adc_t adc)
{
	QM_CHECK(adc < QM_ADC_NUM, QM_RC_EINVAL);

	/* Clear the command complete interrupt status field */
	QM_ADC[adc].adc_intr_status = QM_ADC_INTR_STATUS_CC;
	/* Start the calibration and wait for it to complete */
	QM_ADC[adc].adc_cmd = (QM_ADC_CMD_IE | QM_ADC_CMD_START_CAL);
	while (!(QM_ADC[adc].adc_intr_status & QM_ADC_INTR_STATUS_CC))
		;
	/* Clear the command complete interrupt status field again */
	QM_ADC[adc].adc_intr_status = QM_ADC_INTR_STATUS_CC;

	return QM_RC_OK;
}

qm_rc_t qm_adc_set_mode(const qm_adc_t adc, const qm_adc_mode_t mode)
{
	QM_CHECK(adc < QM_ADC_NUM, QM_RC_EINVAL);
	QM_CHECK(mode <= QM_ADC_MODE_NORM_NO_CAL, QM_RC_EINVAL);

	/* Issue mode change command and wait for it to complete */
	QM_ADC[adc].adc_op_mode = mode;
	while ((QM_ADC[adc].adc_op_mode & QM_ADC_OP_MODE_OM_MASK) != mode)
		;

	/* Perform a dummy conversion if we are transitioning to Normal Mode */
	if ((mode >= QM_ADC_MODE_NORM_CAL)) {
		/* Set the first sequence register back to its default (ch 0) */
		QM_ADC[adc].adc_seq0 = QM_ADC_CAL_SEQ_TABLE_DEFAULT;

		/* Clear the command complete interrupt status field */
		QM_ADC[adc].adc_intr_status = QM_ADC_INTR_STATUS_CC;
		/* Run a dummy convert and wait for it to complete */
		QM_ADC[adc].adc_cmd = (QM_ADC_CMD_IE | QM_ADC_CMD_START_SINGLE);
		while (!(QM_ADC[adc].adc_intr_status & QM_ADC_INTR_STATUS_CC))
			;

		/* Flush the FIFO to get rid of the dummy values */
		QM_ADC[adc].adc_sample = QM_ADC_FIFO_CLEAR;
		/* Clear the command complete interrupt status field */
		QM_ADC[adc].adc_intr_status = QM_ADC_INTR_STATUS_CC;
	}

	return QM_RC_OK;
}

qm_rc_t qm_adc_set_config(const qm_adc_t adc, const qm_adc_config_t *const cfg)
{
	QM_CHECK(adc < QM_ADC_NUM, QM_RC_EINVAL);
	QM_CHECK(NULL != cfg, QM_RC_EINVAL);
	/* QM_CHECK(cfg->window > 255, QM_RC_EINVAL); unnecessary - uint8_t */
	QM_CHECK(cfg->resolution <= QM_ADC_RES_12_BITS, QM_RC_EINVAL);
	QM_CHECK(cfg->window >= cfg->resolution + 2, QM_RC_EINVAL);

	/* Set the sample window and resolution */
	adc_cmd[adc] = ((cfg->window << QM_ADC_CMD_SW_OFFSET) |
			(cfg->resolution << QM_ADC_CMD_RESOLUTION_OFFSET));

	return QM_RC_OK;
}

qm_rc_t qm_adc_get_config(const qm_adc_t adc, qm_adc_config_t *const cfg)
{
	QM_CHECK(adc < QM_ADC_NUM, QM_RC_EINVAL);

	/* Get the sample window and resolution */
	cfg->window =
	    ((adc_cmd[adc] & QM_ADC_CMD_SW_MASK) >> QM_ADC_CMD_SW_OFFSET);
	cfg->resolution = ((adc_cmd[adc] & QM_ADC_CMD_RESOLUTION_MASK) >>
			   QM_ADC_CMD_RESOLUTION_OFFSET);

	return QM_RC_OK;
}

qm_rc_t qm_adc_convert(const qm_adc_t adc, qm_adc_xfer_t *xfer)
{
	uint32_t i;

	/* Check the stuff that was passed in */
	QM_CHECK(adc < QM_ADC_NUM, QM_RC_EINVAL);
	QM_CHECK(NULL != xfer, QM_RC_EINVAL);
	QM_CHECK(xfer->ch_len < QM_ADC_CHAN_SEQ_MAX, QM_RC_EINVAL);
	QM_CHECK(xfer->samples_len > 0, QM_RC_EINVAL);
	QM_CHECK(xfer->samples_len < QM_ADC_FIFO_LEN, QM_RC_EINVAL);

	/* Flush the FIFO */
	QM_ADC[adc].adc_sample = QM_ADC_FIFO_CLEAR;

	/* Populate the sample sequence table */
	setup_seq_table(adc, xfer);

	/* Issue cmd: window & resolution, number of samples, command */
	QM_ADC[adc].adc_cmd =
	    (adc_cmd[adc] | ((xfer->samples_len - 1) << QM_ADC_CMD_NS_OFFSET) |
	     QM_ADC_CMD_START_SINGLE);

	/* Wait for fifo count to reach number of samples */
	while (QM_ADC[adc].adc_fifo_count != xfer->samples_len)
		;

	/* Read the value into the data structure */
	for (i = 0; i < xfer->samples_len; i++) {
		xfer->samples[i] = QM_ADC[adc].adc_sample;
	}

	return QM_RC_OK;
}

qm_rc_t qm_adc_irq_convert(const qm_adc_t adc, qm_adc_xfer_t *xfer)
{
	QM_CHECK(adc < QM_ADC_NUM, QM_RC_EINVAL);
	QM_CHECK(NULL != xfer, QM_RC_EINVAL);
	QM_CHECK(xfer->ch_len < QM_ADC_CHAN_SEQ_MAX, QM_RC_EINVAL);
	QM_CHECK(xfer->samples_len > 0, QM_RC_EINVAL);

	/* Reset the count and flush the FIFO */
	count[adc] = 0;
	QM_ADC[adc].adc_sample = QM_ADC_FIFO_CLEAR;

	/* Populate the sample sequence table */
	setup_seq_table(adc, xfer);

	/* Copy the xfer struct so we can get access from the ISR */
	memcpy(&irq_xfer[adc], xfer, sizeof(qm_adc_xfer_t));

	/* Clear and enable continuous command and fifo overrun interupts */
	QM_ADC[adc].adc_intr_status =
	    QM_ADC_INTR_STATUS_FO | QM_ADC_INTR_STATUS_CONT_CC;
	QM_ADC[adc].adc_intr_enable =
	    QM_ADC_INTR_ENABLE_FO | QM_ADC_INTR_ENABLE_CONT_CC;

	/* Issue cmd: window & resolution, number of samples, interrupt enable
	 * and start continuous coversion command. If xfer->samples_len is less
	 * than SAMPLE_BLOCK_LEN extra samples will be discarded in the ISR */
	QM_ADC[adc].adc_cmd =
	    (adc_cmd[adc] | ((SAMPLE_BLOCK_LEN - 1) << QM_ADC_CMD_NS_OFFSET) |
	     QM_ADC_CMD_IE | QM_ADC_CMD_START_CONT);

	return QM_RC_OK;
}

#endif /* QUARK_D2000 */
