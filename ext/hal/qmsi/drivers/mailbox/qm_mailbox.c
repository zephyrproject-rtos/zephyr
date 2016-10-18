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

/**
 * The Active core can be either Lakemont or the Sensor Sub-System.
 * The active core depends on a compilation flag indicating which
 * core is being used.
 *
 * Core specific mailbox #defines are grouped here to prevent
 * duplication below.
 */
#if (HAS_MAILBOX_LAKEMONT_DEST)
#ifdef QM_LAKEMONT
#define ACTIVE_CORE_DEST QM_MBOX_TO_LMT
#define MBOX_INT_MASK QM_MBOX_LMT_INT_MASK
#define MBOX_INT_LOCK_MASK(N) QM_MBOX_LMT_INT_LOCK_MASK(N)
#define MBOX_INT_LOCK_HALT_MASK(N) QM_MBOX_LMT_INT_LOCK_HALT_MASK(N)
#define MBOX_ENABLE_INT_MASK(N) QM_MBOX_ENABLE_LMT_INT_MASK(N)
#define MBOX_DISABLE_INT_MASK(N) QM_MBOX_DISABLE_LMT_INT_MASK(N)
#endif /* QM_LAKEMONT */
#endif /* HAS_MAILBOX_LAKEMONT_DEST */

#if (HAS_MAILBOX_SENSOR_SUB_SYSTEM_DEST)
#ifdef QM_SENSOR
#define ACTIVE_CORE_DEST QM_MBOX_TO_SS
#define MBOX_INT_MASK QM_MBOX_SS_INT_MASK
#define MBOX_INT_LOCK_MASK(N) QM_MBOX_SS_INT_LOCK_HALT_MASK(N)
#define MBOX_INT_LOCK_HALT_MASK(N) QM_MBOX_SS_INT_LOCK_MASK(N)
#define MBOX_ENABLE_INT_MASK(N) QM_MBOX_ENABLE_SS_INT_MASK(N)
#define MBOX_DISABLE_INT_MASK(N) QM_MBOX_DISABLE_SS_INT_MASK(N)
#endif /* QM_SENSOR */
#endif /* HAS_MAILBOX_SENSOR_SUB_SYSTEM_DEST */

/**
 * Private data structure maintained by the driver
 */
typedef struct {
	/** Destination of the mailbox channel. */
	qm_mbox_destination_t dest;
	/** Defines if the mailbox channel operates in interrupt
	 * mode or polling mode. */
	qm_mbox_mode_t mode;
	/** Callback function registered with the application. */
	qm_mbox_callback_t callback;
	/** Callback function data return via the callback function. */
	void *callback_data;
} qm_mailbox_info_t;

/* Mailbox channels private data structures */
static qm_mailbox_info_t mailbox_devs[QM_MBOX_CH_NUM];

QM_ISR_DECLARE(qm_mailbox_0_isr)
{
	qm_mailbox_t *const mbox_reg = (qm_mailbox_t *)QM_MAILBOX;
	uint8_t i = 0;
	uint8_t mask;
	uint16_t chall_sts = QM_MAILBOX->mbox_chall_sts;

	mask = MBOX_INT_MASK;

	for (i = 0; chall_sts; i++, chall_sts >>= 2) {
		if ((chall_sts & QM_MBOX_CH_STS_CTRL_INT) == 0) {
			continue;
		}
		if (mask & BIT(i)) {
			continue;
		}
		if (mbox_reg[i].ch_sts & QM_MBOX_CH_STS_CTRL_INT) {
			if (NULL != mailbox_devs[i].callback) {
				/* Callback */
				mailbox_devs[i].callback(
				    mailbox_devs[i].callback_data);
			}
			/* Clear the interrupt */
			mbox_reg[i].ch_sts = QM_MBOX_CH_STS_CTRL_INT;
		}
	}

	QM_ISR_EOI(QM_IRQ_MAILBOX_0_INT_VECTOR);
}

int qm_mbox_ch_set_config(const qm_mbox_ch_t mbox_ch,
			  const qm_mbox_config_t *const config)
{

	QM_CHECK((QM_MBOX_CH_0 <= mbox_ch) && (mbox_ch < QM_MBOX_CH_NUM),
		 -EINVAL);
	qm_mailbox_info_t *device = &mailbox_devs[mbox_ch];

	/* Block interrupts while configuring MBOX */
	qm_irq_mask(QM_IRQ_MAILBOX_0_INT);

	/* Store the device destination */
	device->dest = config->dest;

	/* Check if we are enabling or disabling the channel. */
	if (QM_MBOX_UNUSED != config->dest) {

		if (QM_MBOX_INTERRUPT_MODE == config->mode) {
			QM_CHECK(NULL != config->callback, -EINVAL);

			/* Register callback function */
			device->callback = config->callback;
			/* Register callback function data */
			device->callback_data = config->callback_data;
			/* Update the mode of operation for the mailbox channel.
			 */
			device->mode = QM_MBOX_INTERRUPT_MODE;

			/* Enable the mailbox interrupt if the lock is not set.
			 */
			if (!(MBOX_INT_LOCK_MASK(mbox_ch))) {
				/* Note: Routing is done now, cannot be done in
				 * irq_request! */
				MBOX_ENABLE_INT_MASK(mbox_ch);
			}
		} else {
			device->mode = QM_MBOX_POLLING_MODE;
			/* Disable the mailbox interrupt if the lock is not set.
			 */
			if (!(MBOX_INT_LOCK_MASK(mbox_ch))) {
				/* Note: Routing is done now, cannot be done in
				 * irq_request! */
				MBOX_DISABLE_INT_MASK(mbox_ch);
			}
			device->callback = NULL;
			device->callback_data = 0;
		}
	} else {
		/* Disable the mailbox interrupt if the lock is not set. */
		if (!(MBOX_INT_LOCK_MASK(mbox_ch))) {
			/* Note: Routing is done now, cannot be done in
			 * irq_request! */
			MBOX_DISABLE_INT_MASK(mbox_ch);
		}

		/* Set the mailbox channel to its default configuration. */
		device->mode = QM_MBOX_INTERRUPT_MODE;
		device->callback = NULL;
		device->callback_data = 0;
	}

	/* UnBlock MBOX interrupts. */
	qm_irq_unmask(QM_IRQ_MAILBOX_0_INT);
	return 0;
}

int qm_mbox_ch_write(const qm_mbox_ch_t mbox_ch, const qm_mbox_msg_t *const msg)
{
	QM_CHECK((QM_MBOX_CH_0 <= mbox_ch) && (mbox_ch < QM_MBOX_CH_NUM),
		 -EINVAL);
	QM_CHECK(NULL != msg, -EINVAL);
	qm_mailbox_t *const mbox_reg = (qm_mailbox_t *)QM_MAILBOX + mbox_ch;

	uint32_t status = 0;

	status = QM_MAILBOX->mbox[mbox_ch].ch_sts;

	/* Check if the previous message has been consumed. */
	if (false == (status & (QM_MBOX_CH_STS_CTRL_INT | QM_MBOX_CH_STS))) {
		/* Write the payload data to the mailbox channel. */
		mbox_reg->ch_data[0] = msg->data[QM_MBOX_PAYLOAD_0];
		mbox_reg->ch_data[1] = msg->data[QM_MBOX_PAYLOAD_1];
		mbox_reg->ch_data[2] = msg->data[QM_MBOX_PAYLOAD_2];
		mbox_reg->ch_data[3] = msg->data[QM_MBOX_PAYLOAD_3];
		/* Write the control word and trigger the channel interrupt. */
		mbox_reg->ch_ctrl = msg->ctrl | QM_MBOX_CH_CTRL_INT;
		return 0;
	}

	/* Previous message has not been consumed. */
	return -EIO;
}

int qm_mbox_ch_read(const qm_mbox_ch_t mbox_ch, qm_mbox_msg_t *const msg)
{
	QM_CHECK((QM_MBOX_CH_0 <= mbox_ch) && (mbox_ch < QM_MBOX_CH_NUM),
		 -EINVAL);
	QM_CHECK(NULL != msg, -EINVAL);

	int rc = 0;
	uint32_t status = 0;

	qm_mailbox_t *mbox_reg = &QM_MAILBOX->mbox[mbox_ch];
	qm_mailbox_info_t *device = &mailbox_devs[mbox_ch];

	if (ACTIVE_CORE_DEST == device->dest) {
		status = mbox_reg->ch_sts;

		/* If there is data pending consume it */
		if (status & QM_MBOX_CH_STS) {
			/* Read data from the mailbox channel and clear bit 31
			 * of the control word. */
			msg->ctrl = mbox_reg->ch_ctrl & (~QM_MBOX_CH_CTRL_INT);
			msg->data[0] = mbox_reg->ch_data[0];
			msg->data[1] = mbox_reg->ch_data[1];
			msg->data[2] = mbox_reg->ch_data[2];
			msg->data[3] = mbox_reg->ch_data[3];

			if (QM_MBOX_POLLING_MODE == device->mode) {
				/* In polling mode the interrupt status still
				 * needs to be cleared since we are not using
				 * the ISR. Note we write 1 to clear the bit.
				 */
				mbox_reg->ch_sts = QM_MBOX_CH_STS_CTRL_INT;
			}

			/* Clear data status bit. This indicates to others that
			 * the mailbox data has been consumed and a new message
			 * can be sent on the channel */
			mbox_reg->ch_sts = QM_MBOX_CH_STS;
		} else {
			/* there is no pending data in the mailbox */
			rc = -EIO;
		}
	} else {
		/* Active destination has not been configured to consume data
		 * from this channel */
		rc = -EINVAL;
	}

	return rc;
}

int qm_mbox_ch_get_status(const qm_mbox_ch_t mbox_ch,
			  qm_mbox_ch_status_t *const status)
{
	QM_CHECK((QM_MBOX_CH_0 <= mbox_ch) && (mbox_ch < QM_MBOX_CH_NUM),
		 -EINVAL);
	QM_CHECK(NULL != status, -EINVAL);

	uint32_t mbox_status = QM_MAILBOX->mbox[mbox_ch].ch_sts;
	qm_mbox_ch_status_t rc = QM_MBOX_CH_IDLE;

	/* Check if the mailbox is in polling mode or not */
	if (QM_MBOX_POLLING_MODE == mailbox_devs[mbox_ch].mode) {
		/* Polling Mode
		 * Check if there is pending data to be consumed
		 */
		if (QM_MBOX_CH_STS & mbox_status) {
			rc = QM_MBOX_CH_POLLING_DATA_PEND;
		}
	} else {
		/* Interrupt Mode
		 * Check if there is a pending interrupt & data,
		 * otherwise check if there is pending data to be consumed
		 */
		if (QM_MBOX_STATUS_MASK == mbox_status) {
			rc = QM_MBOX_CH_INT_NACK_DATA_PEND;
		} else if (QM_MBOX_CH_STS == mbox_status) {
			rc = QM_MBOX_CH_INT_ACK_DATA_PEND;
		}
	}

	*status = rc;
	return 0;
}
