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

#include "qm_common.h"
#include "qm_mailbox.h"
#include "qm_interrupt.h"

/* Register offsets from a base register for a particular mailbox channel. */
#define QM_MBOX_CTRL_OFFSET (0x00)
#define QM_MBOX_DATA0_OFFSET (0x04)
#define QM_MBOX_DATA1_OFFSET (0x08)
#define QM_MBOX_DATA2_OFFSET (0x0C)
#define QM_MBOX_DATA3_OFFSET (0x10)
#define QM_MBOX_STATUS_OFFSET (0x14)

#define QM_MBOX_SS_MASK_OFFSET (0x8)

/* Private data structure maintained by the driver */
typedef struct qm_mailbox_info_t {
	/*!< Callback function registered with the application. */
	qm_mbox_callback_t mpr_cb;
	/*!< Callback function data return via the callback function. */
	void *cb_data;
} qm_mailbox_info_t;

/* Mailbox channels private data structures */
static qm_mailbox_info_t mailbox_devs[QM_MBOX_CH_NUM];

QM_ISR_DECLARE(qm_mbox_isr)
{
	qm_mailbox_t *const mbox_reg = (qm_mailbox_t *)QM_SCSS_MAILBOX;
	uint8_t i = 0;
	uint8_t mask;
	uint16_t chall_sts = QM_SCSS_MAILBOX->mbox_chall_sts;

/*
 * Interrupt masking register is has a bit assigned per MBOX channel.
 * QM_SCSS_INT 0-7 bits are MBOX interrupt gates to APIC, 8-15 are
 * gating interrupts to Sensors PIC.
 */
#if (QM_SENSOR)
	mask = 0xff & (QM_SCSS_INT->int_mailbox_mask >> QM_MBOX_SS_MASK_OFFSET);
#else
	mask = 0xff & QM_SCSS_INT->int_mailbox_mask;
#endif

	for (i = 0; chall_sts; i++, chall_sts >>= 2) {
		if ((chall_sts & QM_MBOX_CH_INT) == 0) {
			continue;
		}
		if (mask & BIT(i)) {
			continue;
		}
		if (mbox_reg[i].ch_sts & QM_MBOX_CH_INT) {
			if (NULL != mailbox_devs[i].mpr_cb) {
				/* Callback */
				mailbox_devs[i].mpr_cb(mailbox_devs[i].cb_data);
			}
			/* Clear the interrupt */
			mbox_reg[i].ch_sts = QM_MBOX_CH_INT;
		}
	}

	QM_ISR_EOI(QM_IRQ_MBOX_VECTOR);
}

int qm_mbox_ch_set_config(const qm_mbox_ch_t mbox_ch, qm_mbox_callback_t mpr_cb,
			  void *cb_data, const bool irq_en)
{
	uint32_t mask;

	QM_CHECK(mbox_ch < QM_MBOX_CH_NUM, -EINVAL);

	/* Block interrupts while configuring MBOX */
	qm_irq_mask(QM_IRQ_MBOX);

#if (QM_SENSOR)
	/* MBOX Interrupt Routing gate to SS core. */
	mask = BIT((mbox_ch + QM_MBOX_SS_MASK_OFFSET));
#else
	/* MBOX Interrupt Routing gate to LMT core. */
	mask = BIT(mbox_ch);
#endif

	/* Register callback function */
	mailbox_devs[mbox_ch].mpr_cb = mpr_cb;
	mailbox_devs[mbox_ch].cb_data = cb_data;

	if (irq_en == true) {
		/* Note: Routing is done now, cannot be done in irq_request! */
		QM_SCSS_INT->int_mailbox_mask &= ~mask;
		/* Clear the interrupt */
		((qm_mailbox_t *)QM_SCSS_MAILBOX + mbox_ch)->ch_sts =
		    QM_MBOX_CH_INT;
	} else {
		/* Note: Routing is done now, cannot be done in irq_request! */
		QM_SCSS_INT->int_mailbox_mask |= mask;
	}

	/* UnBlock MBOX interrupts. */
	qm_irq_unmask(QM_IRQ_MBOX);
	return 0;
}

int qm_mbox_ch_write(const qm_mbox_ch_t mbox_ch,
		     const qm_mbox_msg_t *const data)
{
	qm_mailbox_t *const mbox_reg =
	    (qm_mailbox_t *)QM_SCSS_MAILBOX + mbox_ch;

	/* Check if the previous message has been consumed. */
	if (!(mbox_reg->ch_ctrl & QM_MBOX_TRIGGER_CH_INT)) {
		/* Write the payload data to the mailbox channel. */
		mbox_reg->ch_data[0] = data->data[QM_MBOX_PAYLOAD_0];
		mbox_reg->ch_data[1] = data->data[QM_MBOX_PAYLOAD_1];
		mbox_reg->ch_data[2] = data->data[QM_MBOX_PAYLOAD_2];
		mbox_reg->ch_data[3] = data->data[QM_MBOX_PAYLOAD_3];
		/* Write the control word and trigger the channel interrupt. */
		mbox_reg->ch_ctrl = data->ctrl | QM_MBOX_TRIGGER_CH_INT;
		return 0;
	}

	/* Previous message has not been consumed. */
	return -EIO;
}

int qm_mbox_ch_read(const qm_mbox_ch_t mbox_ch, qm_mbox_msg_t *const data)
{
	qm_mailbox_t *const mbox_reg =
	    (qm_mailbox_t *)QM_SCSS_MAILBOX + mbox_ch;

	/* Read data from the mailbox channel and clear bit 31 of the
	 * control word. */
	data->ctrl = mbox_reg->ch_ctrl & ~QM_MBOX_TRIGGER_CH_INT;
	data->data[QM_MBOX_PAYLOAD_0] = mbox_reg->ch_data[0];
	data->data[QM_MBOX_PAYLOAD_1] = mbox_reg->ch_data[1];
	data->data[QM_MBOX_PAYLOAD_2] = mbox_reg->ch_data[2];
	data->data[QM_MBOX_PAYLOAD_3] = mbox_reg->ch_data[3];

	/* Check if the message has arrived. */
	if (mbox_reg->ch_sts & QM_MBOX_CH_DATA) {
		/* Clear data status bit */
		mbox_reg->ch_sts = QM_MBOX_CH_DATA;
		return 0;
	}
	/* there is no new data in mailbox */
	return -EIO;
}

int qm_mbox_ch_get_status(const qm_mbox_ch_t mbox_ch,
			  qm_mbox_ch_status_t *const status)
{
	QM_CHECK(mbox_ch < QM_MBOX_CH_NUM, -EINVAL);
	qm_mailbox_t *const mbox_reg =
	    (qm_mailbox_t *)QM_SCSS_MAILBOX + mbox_ch;
	*status = mbox_reg->ch_sts & QM_MBOX_CH_STATUS_MASK;
	return 0;
}

int qm_mbox_ch_data_ack(const qm_mbox_ch_t mbox_ch)
{
	QM_CHECK(mbox_ch < QM_MBOX_CH_NUM, -EINVAL);
	((qm_mailbox_t *)QM_SCSS_MAILBOX + mbox_ch)->ch_sts = QM_MBOX_CH_DATA;
	return 0;
}
