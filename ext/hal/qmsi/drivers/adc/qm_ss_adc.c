/*
 * Copyright (c) 2017, Intel Corporation
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

#include "qm_ss_adc.h"
#include <string.h>
#include "clk.h"

/* FIFO_INTERRUPT_THRESHOLD is used by qm_ss_adc_irq_convert to set the
 * threshold at which the FIFO will trigger an interrupt. It is also used the
 * ISR handler to determine the number of samples to read from the FIFO. */
#define FIFO_INTERRUPT_THRESHOLD (16)

#define QM_SS_ADC_CHAN_SEQ_MAX (32)
#define ADC_SAMPLE_SHIFT (11)

/* SS ADC commands. */
#define QM_SS_ADC_CMD_START_CAL (3)
#define QM_SS_ADC_CMD_LOAD_CAL (4)

/* Mode change delay is (clock speed * 5). */
#define CALCULATE_DELAY() (clk_sys_get_ticks_per_us() * 5)

static uint32_t adc_base[QM_SS_ADC_NUM] = {QM_SS_ADC_BASE};
static qm_ss_adc_xfer_t *irq_xfer[QM_SS_ADC_NUM];

static uint8_t sample_window[QM_SS_ADC_NUM];
static qm_ss_adc_resolution_t resolution[QM_SS_ADC_NUM];

static uint32_t count[QM_SS_ADC_NUM];

static void (*mode_callback[QM_SS_ADC_NUM])(void *data, int error,
					    qm_ss_adc_status_t status,
					    qm_ss_adc_cb_source_t source);
static void (*cal_callback[QM_SS_ADC_NUM])(void *data, int error,
					   qm_ss_adc_status_t status,
					   qm_ss_adc_cb_source_t source);
static void *mode_callback_data[QM_SS_ADC_NUM];
static void *cal_callback_data[QM_SS_ADC_NUM];

static void dummy_conversion(uint32_t controller);

/* As the mode change interrupt is always asserted when the requested mode
 * matches the current mode, an interrupt will be triggered whenever it is
 * unmasked, which may need to be suppressed. */
static volatile bool ignore_spurious_interrupt[QM_SS_ADC_NUM] = {true};
static qm_ss_adc_mode_t requested_mode[QM_SS_ADC_NUM];

static void enable_adc(void)
{
	QM_SS_REG_AUX_OR(QM_SS_ADC_BASE + QM_SS_ADC_CTRL,
			 QM_SS_ADC_CTRL_ADC_ENA);
}

static void disable_adc(void)
{
	QM_SS_REG_AUX_NAND(QM_SS_ADC_BASE + QM_SS_ADC_CTRL,
			   QM_SS_ADC_CTRL_ADC_ENA);
}

static void qm_ss_adc_isr_handler(const qm_ss_adc_t adc)
{
	uint32_t i, samples_to_read;
	uint32_t controller = adc_base[adc];

	/* Calculate the number of samples to read. */
	samples_to_read = FIFO_INTERRUPT_THRESHOLD;
	if (samples_to_read > (irq_xfer[adc]->samples_len - count[adc])) {
		samples_to_read = irq_xfer[adc]->samples_len - count[adc];
	}

	/* Read the samples into the array. */
	for (i = 0; i < samples_to_read; i++) {
		/* Pop one sample into the sample register. */
		QM_SS_REG_AUX_OR(controller + QM_SS_ADC_SET,
				 QM_SS_ADC_SET_POP_RX);
		/* Read the sample in the array. */
		irq_xfer[adc]->samples[count[adc]] =
		    (__builtin_arc_lr(controller + QM_SS_ADC_SAMPLE) >>
		     (ADC_SAMPLE_SHIFT - resolution[adc]));
		count[adc]++;
	}
	/* Clear the data available status register. */
	QM_SS_REG_AUX_OR(controller + QM_SS_ADC_CTRL,
			 QM_SS_ADC_CTRL_CLR_DATA_A);

	if (count[adc] == irq_xfer[adc]->samples_len) {
		/* Stop the sequencer. */
		QM_SS_REG_AUX_NAND(controller + QM_SS_ADC_CTRL,
				   QM_SS_ADC_CTRL_SEQ_START);

		/* Mask all interrupts. */
		QM_SS_REG_AUX_OR(controller + QM_SS_ADC_CTRL,
				 QM_SS_ADC_CTRL_MSK_ALL_INT);

		/* Call the user callback. */
		if (irq_xfer[adc]->callback) {
			irq_xfer[adc]->callback(irq_xfer[adc]->callback_data, 0,
						QM_SS_ADC_COMPLETE,
						QM_SS_ADC_TRANSFER);
		}

		/* Disable the ADC. */
		disable_adc();

		return;
	}
}

static void qm_ss_adc_isr_err_handler(const qm_ss_adc_t adc)
{
	uint32_t controller = adc_base[adc];
	uint32_t intstat = __builtin_arc_lr(controller + QM_SS_ADC_INTSTAT);

	/* Stop the sequencer. */
	QM_SS_REG_AUX_NAND(controller + QM_SS_ADC_CTRL,
			   QM_SS_ADC_CTRL_SEQ_START);

	/* Mask all interrupts. */
	QM_SS_REG_AUX_OR(controller + QM_SS_ADC_CTRL,
			 QM_SS_ADC_CTRL_MSK_ALL_INT);

	/* Call the user callback and pass it the status code. */
	if (intstat & QM_SS_ADC_INTSTAT_OVERFLOW) {
		if (irq_xfer[adc]->callback) {
			irq_xfer[adc]->callback(irq_xfer[adc]->callback_data,
						-EIO, QM_SS_ADC_OVERFLOW,
						QM_SS_ADC_TRANSFER);
		}
	}
	if (intstat & QM_SS_ADC_INTSTAT_UNDERFLOW) {
		if (irq_xfer[adc]->callback) {
			irq_xfer[adc]->callback(irq_xfer[adc]->callback_data,
						-EIO, QM_SS_ADC_UNDERFLOW,
						QM_SS_ADC_TRANSFER);
		}
	}
	if (intstat & QM_SS_ADC_INTSTAT_SEQERROR) {
		if (irq_xfer[adc]->callback) {
			irq_xfer[adc]->callback(irq_xfer[adc]->callback_data,
						-EIO, QM_SS_ADC_SEQERROR,
						QM_SS_ADC_TRANSFER);
		}
	}

	/* Clear all error interrupts. */
	QM_SS_REG_AUX_OR(controller + QM_SS_ADC_CTRL,
			 (QM_SS_ADC_CTRL_CLR_SEQERROR |
			  QM_SS_ADC_CTRL_CLR_OVERFLOW |
			  QM_SS_ADC_CTRL_CLR_UNDERFLOW));

	/* Disable the ADC. */
	disable_adc();
}

static void qm_ss_adc_isr_pwr_handler(const qm_ss_adc_t adc)
{
	uint32_t controller = adc_base[adc];

	/* The IRQ associated with the mode change fires an interrupt as soon
	 * as it is enabled so it is necessary to ignore it the first time the
	 * ISR runs. */
	if (ignore_spurious_interrupt[adc]) {
		ignore_spurious_interrupt[adc] = false;
		return;
	}

	/* Perform a dummy conversion if we are transitioning to Normal Mode. */
	if ((requested_mode[adc] >= QM_SS_ADC_MODE_NORM_CAL)) {
		dummy_conversion(controller);
	}

	/* Call the user callback if it is set. */
	if (mode_callback[adc]) {
		mode_callback[adc](mode_callback_data[adc], 0, QM_SS_ADC_IDLE,
				   QM_SS_ADC_MODE_CHANGED);
	}
}

static void qm_ss_adc_isr_cal_handler(const qm_ss_adc_t adc)
{
	/* Clear the calibration request reg. */
	QM_SS_REG_AUX_NAND(QM_SS_CREG_BASE + QM_SS_IO_CREG_MST0_CTRL,
			   QM_SS_IO_CREG_MST0_CTRL_ADC_CAL_REQ);

	/* Call the user callback if it is set. */
	if (cal_callback[adc]) {
		cal_callback[adc](cal_callback_data[adc], 0, QM_SS_ADC_IDLE,
				  QM_SS_ADC_CAL_COMPLETE);
	}

	/* Disable the ADC. */
	disable_adc();
}

/* ISR for SS ADC 0 Data available. */
QM_ISR_DECLARE(qm_ss_adc_0_isr)
{
	qm_ss_adc_isr_handler(QM_SS_ADC_0);
}

/* ISR for SS ADC 0 Error. */
QM_ISR_DECLARE(qm_ss_adc_0_error_isr)
{
	qm_ss_adc_isr_err_handler(QM_SS_ADC_0);
}

/* ISR for SS ADC 0 Mode change. */
QM_ISR_DECLARE(qm_ss_adc_0_pwr_isr)
{
	qm_ss_adc_isr_pwr_handler(QM_SS_ADC_0);
}

/* ISR for SS ADC 0 Calibration. */
QM_ISR_DECLARE(qm_ss_adc_0_cal_isr)
{
	qm_ss_adc_isr_cal_handler(QM_SS_ADC_0);
}

static void setup_seq_table(const qm_ss_adc_t adc, qm_ss_adc_xfer_t *xfer,
			    bool single_run)
{
	uint32_t i, reg, ch_odd, ch_even, seq_entry = 0;
	uint32_t num_channels, controller = adc_base[adc];
	/* The sample window is the time in cycles between the start of one
	 * sample and the start of the next. Resolution is indexed from 0 so we
	 * need to add 1 and a further 2 for the time it takes to process. */
	int delay = sample_window[adc] - (resolution[adc] + 3);
	delay = delay < 0 ? 0 : delay;  /* clamp underflows */

	/* Reset the sequence table and sequence pointer. */
	QM_SS_REG_AUX_OR(controller + QM_SS_ADC_CTRL,
			 QM_SS_ADC_CTRL_SEQ_TABLE_RST);

	/* If a single run is requested and the number of channels in ch is less
	 * than the number of samples requested we need to insert multiple
	 * channels into the sequence table. */
	num_channels = single_run ? xfer->samples_len : xfer->ch_len;

	/* The sequence table has to be populated with pairs of entries so there
	 * are sample_len/2 pairs of entries. These entries are read from the
	 * ch array in pairs. The same delay is used between all entries. */
	for (i = 0; i < (num_channels - 1); i += 2) {
		ch_odd = xfer->ch[(i + 1) % xfer->ch_len];
		ch_even = xfer->ch[i % xfer->ch_len];
		seq_entry =
		    ((delay << QM_SS_ADC_SEQ_DELAYODD_OFFSET) |
		     (ch_odd << QM_SS_ADC_SEQ_MUXODD_OFFSET) |
		     (delay << QM_SS_ADC_SEQ_DELAYEVEN_OFFSET) | ch_even);
		__builtin_arc_sr(seq_entry, controller + QM_SS_ADC_SEQ);
	}
	/* If there is an uneven number of entries we need to create a final
	 * pair with a singly entry. */
	if (num_channels % 2) {
		ch_even = xfer->ch[i % xfer->ch_len];
		seq_entry =
		    ((delay << QM_SS_ADC_SEQ_DELAYEVEN_OFFSET) | (ch_even));
		__builtin_arc_sr(seq_entry, controller + QM_SS_ADC_SEQ);
	}

	/* Reset the sequence pointer back to 0. */
	QM_SS_REG_AUX_OR(controller + QM_SS_ADC_CTRL,
			 QM_SS_ADC_CTRL_SEQ_PTR_RST);

	/* Set the number of entries in the sequencer. */
	reg = __builtin_arc_lr(controller + QM_SS_ADC_SET);
	reg &= ~QM_SS_ADC_SET_SEQ_ENTRIES_MASK;
	reg |= ((num_channels - 1) << QM_SS_ADC_SET_SEQ_ENTRIES_OFFSET);
	__builtin_arc_sr(reg, controller + QM_SS_ADC_SET);
}

static void dummy_conversion(uint32_t controller)
{
	uint32_t reg;
	int res;

	/* Flush the FIFO. */
	QM_SS_REG_AUX_OR(controller + QM_SS_ADC_SET, QM_SS_ADC_SET_FLUSH_RX);

	/* Set up sequence table. */
	QM_SS_REG_AUX_OR(controller + QM_SS_ADC_CTRL,
			 QM_SS_ADC_CTRL_SEQ_TABLE_RST);

	/* Populate the seq table. */
	__builtin_arc_sr(QM_SS_ADC_SEQ_DUMMY, controller + QM_SS_ADC_SEQ);

	/* Reset the sequence pointer back to 0. */
	QM_SS_REG_AUX_OR(controller + QM_SS_ADC_CTRL,
			 QM_SS_ADC_CTRL_SEQ_PTR_RST);

	/* Set the number of entries in the sequencer. */
	reg = __builtin_arc_lr(controller + QM_SS_ADC_SET);
	reg &= ~QM_SS_ADC_SET_SEQ_ENTRIES_MASK;
	reg |= (0 << QM_SS_ADC_SET_SEQ_ENTRIES_OFFSET);
	__builtin_arc_sr(reg, controller + QM_SS_ADC_SET);

	/* Set the threshold. */
	reg = __builtin_arc_lr(controller + QM_SS_ADC_SET);
	reg &= ~QM_SS_ADC_SET_THRESHOLD_MASK;
	reg |= (0 << QM_SS_ADC_SET_THRESHOLD_OFFSET);
	__builtin_arc_sr(reg, controller + QM_SS_ADC_SET);

	/* Set the sequence mode to single run. */
	QM_SS_REG_AUX_NAND(controller + QM_SS_ADC_SET, QM_SS_ADC_SET_SEQ_MODE);

	/* Clear all interrupts. */
	QM_SS_REG_AUX_OR(controller + QM_SS_ADC_CTRL,
			 QM_SS_ADC_CTRL_CLR_ALL_INT);
	/* Mask all interrupts. */
	QM_SS_REG_AUX_OR(controller + QM_SS_ADC_CTRL,
			 QM_SS_ADC_CTRL_MSK_ALL_INT);

	/* Enable the ADC. */
	enable_adc();

	/* Start the sequencer. */
	QM_SS_REG_AUX_OR(controller + QM_SS_ADC_CTRL, QM_SS_ADC_CTRL_SEQ_START);

	/* Wait for the sequence to finish. */
	while (!(res = __builtin_arc_lr(controller + QM_SS_ADC_INTSTAT))) {
	}

	/* Flush the FIFO. */
	QM_SS_REG_AUX_OR(controller + QM_SS_ADC_SET, QM_SS_ADC_SET_FLUSH_RX);
	/* Clear the data available status register. */
	QM_SS_REG_AUX_OR(controller + QM_SS_ADC_CTRL,
			 QM_SS_ADC_CTRL_CLR_DATA_A);

	/* Unmask all interrupts. */
	QM_SS_REG_AUX_NAND(controller + QM_SS_ADC_CTRL,
			   QM_SS_ADC_CTRL_MSK_ALL_INT);
	/* Disable the ADC. */
	disable_adc();
}

int qm_ss_adc_set_config(const qm_ss_adc_t adc,
			 const qm_ss_adc_config_t *const cfg)
{
	uint32_t reg;
	uint32_t controller = adc_base[adc];

	QM_CHECK(adc < QM_SS_ADC_NUM, -EINVAL);
	QM_CHECK(NULL != cfg, -EINVAL);
	QM_CHECK(cfg->resolution <= QM_SS_ADC_RES_12_BITS, -EINVAL);
	/* The window must be 2 greater than the resolution but since this is
	 * indexed from 0 we need to add a further 1. */
	QM_CHECK(cfg->window >= (cfg->resolution + 3), -EINVAL);

	/* Set the sample window and resolution. */
	sample_window[adc] = cfg->window;
	resolution[adc] = cfg->resolution;

	/* Set the resolution. */
	reg = __builtin_arc_lr(controller + QM_SS_ADC_SET);
	reg &= ~QM_SS_ADC_SET_SAMPLE_WIDTH_MASK;
	reg |= resolution[adc];
	__builtin_arc_sr(reg, controller + QM_SS_ADC_SET);

	return 0;
}

int qm_ss_adc_set_mode(const qm_ss_adc_t adc, const qm_ss_adc_mode_t mode)
{
	uint32_t creg, delay, intstat;
	uint32_t controller = adc_base[adc];

	QM_CHECK(adc < QM_SS_ADC_NUM, -EINVAL);
	QM_CHECK(mode <= QM_SS_ADC_MODE_NORM_NO_CAL, -EINVAL);

	/* Save the state of the mode interrupt mask. */
	intstat = QM_IR_GET_MASK(QM_INTERRUPT_ROUTER->adc_0_pwr_int_mask);
	/* Mask the ADC mode change interrupt. */
	QM_IR_MASK_INTERRUPTS(QM_INTERRUPT_ROUTER->adc_0_pwr_int_mask);

	/* Calculate the delay. */
	delay = CALCULATE_DELAY();

	/* Issue mode change command and wait for it to complete. */
	creg = __builtin_arc_lr(QM_SS_CREG_BASE + QM_SS_IO_CREG_MST0_CTRL);
	creg &= ~((QM_SS_IO_CREG_MST0_CTRL_ADC_DELAY_MASK
		   << QM_SS_IO_CREG_MST0_CTRL_ADC_DELAY_OFFSET) |
		  QM_SS_IO_CREG_MST0_CTRL_ADC_PWR_MODE_MASK);
	creg |= ((delay << QM_SS_IO_CREG_MST0_CTRL_ADC_DELAY_OFFSET) | mode);
	__builtin_arc_sr(creg, QM_SS_CREG_BASE + QM_SS_IO_CREG_MST0_CTRL);

	/* Wait for the mode change to complete. */
	while (!(__builtin_arc_lr(QM_SS_CREG_BASE + QM_SS_IO_CREG_SLV0_OBSR) &
		 QM_SS_IO_CREG_SLV0_OBSR_ADC_PWR_MODE_STS)) {
	}

	/* Restore the state of the mode change interrupt mask if necessary. */
	if (!intstat) {
		ignore_spurious_interrupt[adc] = true;
		QM_IR_UNMASK_INTERRUPTS(
		    QM_INTERRUPT_ROUTER->adc_0_pwr_int_mask);
	}

	/* Perform a dummy conversion if transitioning to Normal Mode. */
	if ((mode >= QM_SS_ADC_MODE_NORM_CAL)) {
		dummy_conversion(controller);
	}

	return 0;
}

int qm_ss_adc_irq_set_mode(const qm_ss_adc_t adc, const qm_ss_adc_mode_t mode,
			   void (*callback)(void *data, int error,
					    qm_ss_adc_status_t status,
					    qm_ss_adc_cb_source_t source),
			   void *callback_data)
{
	uint32_t creg, delay;
	QM_CHECK(adc < QM_SS_ADC_NUM, -EINVAL);
	QM_CHECK(mode <= QM_SS_ADC_MODE_NORM_NO_CAL, -EINVAL);

	mode_callback[adc] = callback;
	mode_callback_data[adc] = callback_data;
	requested_mode[adc] = mode;

	/* Calculate the delay. */
	delay = CALCULATE_DELAY();

	/* Issue mode change command and wait for it to complete. */
	creg = __builtin_arc_lr(QM_SS_CREG_BASE + QM_SS_IO_CREG_MST0_CTRL);
	creg &= ~((QM_SS_IO_CREG_MST0_CTRL_ADC_DELAY_MASK
		   << QM_SS_IO_CREG_MST0_CTRL_ADC_DELAY_OFFSET) |
		  QM_SS_IO_CREG_MST0_CTRL_ADC_PWR_MODE_MASK);
	creg |= ((delay << QM_SS_IO_CREG_MST0_CTRL_ADC_DELAY_OFFSET) | mode);
	__builtin_arc_sr(creg, QM_SS_CREG_BASE + QM_SS_IO_CREG_MST0_CTRL);

	return 0;
}

int qm_ss_adc_calibrate(const qm_ss_adc_t adc __attribute__((unused)))
{
	uint32_t creg, intstat;
	QM_CHECK(adc < QM_SS_ADC_NUM, -EINVAL);

	/* Save the state of the calibration interrupt mask. */
	intstat = QM_IR_GET_MASK(QM_INTERRUPT_ROUTER->adc_0_cal_int_mask);
	/* Mask the ADC calibration interrupt. */
	QM_IR_MASK_INTERRUPTS(QM_INTERRUPT_ROUTER->adc_0_cal_int_mask);

	/* Enable the ADC. */
	enable_adc();

	/* Issue the start calibrate command. */
	creg = __builtin_arc_lr(QM_SS_CREG_BASE + QM_SS_IO_CREG_MST0_CTRL);
	creg &= ~(QM_SS_IO_CREG_MST0_CTRL_ADC_CAL_CMD_MASK |
		  QM_SS_IO_CREG_MST0_CTRL_ADC_CAL_REQ);
	creg |= ((QM_SS_ADC_CMD_START_CAL
		  << QM_SS_IO_CREG_MST0_CTRL_ADC_CAL_CMD_OFFSET) |
		 QM_SS_IO_CREG_MST0_CTRL_ADC_CAL_REQ);
	__builtin_arc_sr(creg, QM_SS_CREG_BASE + QM_SS_IO_CREG_MST0_CTRL);

	/* Wait for the calibrate command to complete. */
	while (!(__builtin_arc_lr(QM_SS_CREG_BASE + QM_SS_IO_CREG_SLV0_OBSR) &
		 QM_SS_IO_CREG_SLV0_OBSR_ADC_CAL_ACK)) {
	}

	/* Clear the calibration request reg and wait for it to complete. */
	QM_SS_REG_AUX_NAND(QM_SS_CREG_BASE + QM_SS_IO_CREG_MST0_CTRL,
			   QM_SS_IO_CREG_MST0_CTRL_ADC_CAL_REQ);
	while (__builtin_arc_lr(QM_SS_CREG_BASE + QM_SS_IO_CREG_SLV0_OBSR) &
	       QM_SS_IO_CREG_SLV0_OBSR_ADC_CAL_ACK) {
	}

	/* Disable the ADC. */
	disable_adc();

	/* Restore the state of the calibration interrupt mask if necessary. */
	if (!intstat) {
		QM_IR_UNMASK_INTERRUPTS(
		    QM_INTERRUPT_ROUTER->adc_0_cal_int_mask);
	}

	return 0;
}

int qm_ss_adc_irq_calibrate(const qm_ss_adc_t adc,
			    void (*callback)(void *data, int error,
					     qm_ss_adc_status_t status,
					     qm_ss_adc_cb_source_t source),
			    void *callback_data)
{
	uint32_t creg;
	QM_CHECK(adc < QM_SS_ADC_NUM, -EINVAL);

	cal_callback[adc] = callback;
	cal_callback_data[adc] = callback_data;

	/* Enable the ADC. */
	enable_adc();

	/* Issue the start calibrate command. */
	creg = __builtin_arc_lr(QM_SS_CREG_BASE + QM_SS_IO_CREG_MST0_CTRL);
	creg &= ~(QM_SS_IO_CREG_MST0_CTRL_ADC_CAL_CMD_MASK |
		  QM_SS_IO_CREG_MST0_CTRL_ADC_CAL_REQ);
	creg |= ((QM_SS_ADC_CMD_START_CAL
		  << QM_SS_IO_CREG_MST0_CTRL_ADC_CAL_CMD_OFFSET) |
		 QM_SS_IO_CREG_MST0_CTRL_ADC_CAL_REQ);
	__builtin_arc_sr(creg, QM_SS_CREG_BASE + QM_SS_IO_CREG_MST0_CTRL);

	return 0;
}

int qm_ss_adc_set_calibration(const qm_ss_adc_t adc __attribute__((unused)),
			      const qm_ss_adc_calibration_t cal_data)
{
	uint32_t creg, intstat;

	QM_CHECK(adc < QM_SS_ADC_NUM, -EINVAL);
	QM_CHECK(cal_data <= QM_SS_IO_CREG_MST0_CTRL_ADC_CAL_VAL_MAX, -EINVAL);

	/* Save the state of the calibration interrupt mask. */
	intstat = QM_IR_GET_MASK(QM_INTERRUPT_ROUTER->adc_0_cal_int_mask);
	/* Mask the ADC calibration interrupt. */
	QM_IR_MASK_INTERRUPTS(QM_INTERRUPT_ROUTER->adc_0_cal_int_mask);

	/* Issue the load calibrate command. */
	creg = __builtin_arc_lr(QM_SS_CREG_BASE + QM_SS_IO_CREG_MST0_CTRL);
	creg &= ~(QM_SS_IO_CREG_MST0_CTRL_ADC_CAL_VAL_MASK |
		  QM_SS_IO_CREG_MST0_CTRL_ADC_CAL_CMD_MASK |
		  QM_SS_IO_CREG_MST0_CTRL_ADC_CAL_REQ);
	creg |= ((cal_data << QM_SS_IO_CREG_MST0_CTRL_ADC_CAL_VAL_OFFSET) |
		 (QM_SS_ADC_CMD_LOAD_CAL
		  << QM_SS_IO_CREG_MST0_CTRL_ADC_CAL_CMD_OFFSET) |
		 QM_SS_IO_CREG_MST0_CTRL_ADC_CAL_REQ);
	__builtin_arc_sr(creg, QM_SS_CREG_BASE + QM_SS_IO_CREG_MST0_CTRL);

	/* Wait for the calibrate command to complete. */
	while (!(__builtin_arc_lr(QM_SS_CREG_BASE + QM_SS_IO_CREG_SLV0_OBSR) &
		 QM_SS_IO_CREG_SLV0_OBSR_ADC_CAL_ACK)) {
	}

	/* Clear the calibration request reg and wait for it to complete. */
	QM_SS_REG_AUX_NAND(QM_SS_CREG_BASE + QM_SS_IO_CREG_MST0_CTRL,
			   QM_SS_IO_CREG_MST0_CTRL_ADC_CAL_REQ);
	while (__builtin_arc_lr(QM_SS_CREG_BASE + QM_SS_IO_CREG_SLV0_OBSR) &
	       QM_SS_IO_CREG_SLV0_OBSR_ADC_CAL_ACK) {
	}

	/* Restore the state of the calibration interrupt mask if necessary. */
	if (!intstat) {
		QM_IR_UNMASK_INTERRUPTS(
		    QM_INTERRUPT_ROUTER->adc_0_cal_int_mask);
	}

	return 0;
}

int qm_ss_adc_get_calibration(const qm_ss_adc_t adc __attribute__((unused)),
			      qm_ss_adc_calibration_t *const cal)
{
	QM_CHECK(adc < QM_SS_ADC_NUM, -EINVAL);
	QM_CHECK(NULL != cal, -EINVAL);

	*cal = ((__builtin_arc_lr(QM_SS_CREG_BASE + QM_SS_IO_CREG_SLV0_OBSR) &
		 QM_SS_IO_CREG_SLV0_OBSR_ADC_CAL_VAL_MASK) >>
		QM_SS_IO_CREG_SLV0_OBSR_ADC_CAL_VAL_OFFSET);

	return 0;
}

int qm_ss_adc_convert(const qm_ss_adc_t adc, qm_ss_adc_xfer_t *xfer,
		      qm_ss_adc_status_t *const status)
{
	uint32_t reg, i;
	uint32_t controller = adc_base[adc];
	int res;

	QM_CHECK(adc < QM_SS_ADC_NUM, -EINVAL);
	QM_CHECK(NULL != xfer, -EINVAL);
	QM_CHECK(NULL != xfer->ch, -EINVAL);
	QM_CHECK(NULL != xfer->samples, -EINVAL);
	QM_CHECK(xfer->ch_len > 0, -EINVAL);
	QM_CHECK(xfer->ch_len <= QM_SS_ADC_CHAN_SEQ_MAX, -EINVAL);
	QM_CHECK(xfer->samples_len > 0, -EINVAL);
	QM_CHECK(xfer->samples_len <= QM_SS_ADC_FIFO_LEN, -EINVAL);

	/* Flush the FIFO. */
	QM_SS_REG_AUX_OR(controller + QM_SS_ADC_SET, QM_SS_ADC_SET_FLUSH_RX);

	/* Populate the sequence table. */
	setup_seq_table(adc, xfer, true);

	/* Set the threshold. */
	reg = __builtin_arc_lr(controller + QM_SS_ADC_SET);
	reg &= ~QM_SS_ADC_SET_THRESHOLD_MASK;
	reg |= ((xfer->samples_len - 1) << QM_SS_ADC_SET_THRESHOLD_OFFSET);
	__builtin_arc_sr(reg, controller + QM_SS_ADC_SET);

	/* Set the sequence mode to single run. */
	QM_SS_REG_AUX_NAND(controller + QM_SS_ADC_SET, QM_SS_ADC_SET_SEQ_MODE);

	/* Mask all interrupts. */
	QM_SS_REG_AUX_OR(controller + QM_SS_ADC_CTRL,
			 QM_SS_ADC_CTRL_MSK_ALL_INT);

	/* Enable the ADC. */
	enable_adc();

	/* Start the sequencer. */
	QM_SS_REG_AUX_OR(controller + QM_SS_ADC_CTRL, QM_SS_ADC_CTRL_SEQ_START);

	/* Wait for the sequence to finish. */
	while (!(res = __builtin_arc_lr(controller + QM_SS_ADC_INTSTAT))) {
	}

	/* Return if we get an error (UNDERFLOW, OVERFLOW, SEQ_ERROR). */
	if (res > 1) {
		if (status) {
			*status = res;
		}
		return -EIO;
	}

	/* Read the samples into the array. */
	for (i = 0; i < xfer->samples_len; i++) {
		/* Pop one sample into the sample register. */
		QM_SS_REG_AUX_OR(controller + QM_SS_ADC_SET,
				 QM_SS_ADC_SET_POP_RX);
		/* Read the sample in the array. */
		xfer->samples[i] =
		    (__builtin_arc_lr(controller + QM_SS_ADC_SAMPLE) >>
		     (ADC_SAMPLE_SHIFT - resolution[adc]));
	}

	/* Clear the data available status register. */
	QM_SS_REG_AUX_OR(controller + QM_SS_ADC_CTRL,
			 QM_SS_ADC_CTRL_CLR_DATA_A);

	/* Disable the ADC. */
	disable_adc();

	return 0;
}

int qm_ss_adc_irq_convert(const qm_ss_adc_t adc, qm_ss_adc_xfer_t *xfer)
{
	uint32_t reg;
	uint32_t controller = adc_base[adc];

	QM_CHECK(adc < QM_SS_ADC_NUM, -EINVAL);
	QM_CHECK(NULL != xfer, -EINVAL);
	QM_CHECK(NULL != xfer->ch, -EINVAL);
	QM_CHECK(NULL != xfer->samples, -EINVAL);
	QM_CHECK(xfer->ch_len > 0, -EINVAL);
	QM_CHECK(xfer->samples_len > 0, -EINVAL);
	QM_CHECK(xfer->ch_len <= QM_SS_ADC_CHAN_SEQ_MAX, -EINVAL);

	/* Flush the FIFO. */
	QM_SS_REG_AUX_OR(controller + QM_SS_ADC_SET, QM_SS_ADC_SET_FLUSH_RX);

	/* Populate the sequence table. */
	setup_seq_table(adc, xfer, false);

	/* Copy the xfer struct so we can get access from the ISR. */
	irq_xfer[adc] = xfer;

	/* Set count back to 0. */
	count[adc] = 0;

	/* Set the threshold. */
	reg = __builtin_arc_lr(controller + QM_SS_ADC_SET);
	reg &= ~QM_SS_ADC_SET_THRESHOLD_MASK;
	reg |= (FIFO_INTERRUPT_THRESHOLD - 1) << QM_SS_ADC_SET_THRESHOLD_OFFSET;
	__builtin_arc_sr(reg, controller + QM_SS_ADC_SET);

	/* Set the sequence mode to repetitive. */
	QM_SS_REG_AUX_OR(controller + QM_SS_ADC_SET, QM_SS_ADC_SET_SEQ_MODE);

	/* Enable all interrupts. */
	QM_SS_REG_AUX_NAND(controller + QM_SS_ADC_CTRL, 0x1F00);

	/* Enable the ADC. */
	enable_adc();

	/* Start the sequencer. */
	QM_SS_REG_AUX_OR(controller + QM_SS_ADC_CTRL, QM_SS_ADC_CTRL_SEQ_START);

	return 0;
}

#if (ENABLE_RESTORE_CONTEXT)
int qm_ss_adc_save_context(const qm_ss_adc_t adc,
			   qm_ss_adc_context_t *const ctx)
{
	const uint32_t controller = adc_base[adc];

	QM_CHECK(adc < QM_SS_ADC_NUM, -EINVAL);
	QM_CHECK(NULL != ctx, -EINVAL);

	ctx->adc_set = __builtin_arc_lr(controller + QM_SS_ADC_SET);
	ctx->adc_divseqstat =
	    __builtin_arc_lr(controller + QM_SS_ADC_DIVSEQSTAT);
	ctx->adc_seq = __builtin_arc_lr(controller + QM_SS_ADC_SEQ);
	/* Restore control register with ADC enable bit cleared. */
	ctx->adc_ctrl = __builtin_arc_lr(controller + QM_SS_ADC_CTRL) &
			~QM_SS_ADC_CTRL_ADC_ENA;

	/*
	 * The IRQ associated with the mode change fires an interrupt as soon
	 * as it is enabled so it is necessary to ignore it the first time the
	 * ISR runs. This is set in the save function in case interrupts are
	 * enabled before the restore function runs.
	 */
	ignore_spurious_interrupt[adc] = true;

	return 0;
}

int qm_ss_adc_restore_context(const qm_ss_adc_t adc,
			      const qm_ss_adc_context_t *const ctx)
{
	const uint32_t controller = adc_base[adc];

	QM_CHECK(adc < QM_SS_ADC_NUM, -EINVAL);
	QM_CHECK(NULL != ctx, -EINVAL);

	__builtin_arc_sr(ctx->adc_set, controller + QM_SS_ADC_SET);
	__builtin_arc_sr(ctx->adc_divseqstat,
			 controller + QM_SS_ADC_DIVSEQSTAT);
	__builtin_arc_sr(ctx->adc_seq, controller + QM_SS_ADC_SEQ);
	__builtin_arc_sr(ctx->adc_ctrl, controller + QM_SS_ADC_CTRL);

	return 0;
}
#else
int qm_ss_adc_save_context(const qm_ss_adc_t adc,
			   qm_ss_adc_context_t *const ctx)
{
	(void)adc;
	(void)ctx;

	return 0;
}

int qm_ss_adc_restore_context(const qm_ss_adc_t adc,
			      const qm_ss_adc_context_t *const ctx)
{
	(void)adc;
	(void)ctx;

	return 0;
}
#endif /* ENABLE_RESTORE_CONTEXT */
