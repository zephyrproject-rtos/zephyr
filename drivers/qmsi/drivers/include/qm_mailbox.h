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

#ifndef __QM_MAILBOX_H__
#define __QM_MAILBOX_H__

#include "qm_common.h"
#include "qm_soc_regs.h"

#if (HAS_MAILBOX)
/**
 * Mailbox driver.
 *
 * @defgroup groupMailbox Mailbox
 * @{
 */

/**
 * Mailbox status. Those values are tied to HW bit setting and made up of
 * the bit 0 and bit 1 of the mailbox channel status register.
 */
typedef enum {
	/**< No interrupt pending nor any data to consume. */
	QM_MBOX_CH_IDLE = 0,
	QM_MBOX_CH_DATA = BIT(0), /**< Message has not been consumed. */
	QM_MBOX_CH_INT = BIT(1),  /**< Channel interrupt pending. */
	QM_MBOX_CH_STATUS_MASK = BIT(1) | BIT(0) /**< Status mask. */
} qm_mbox_ch_status_t;

/**
 * Mailbox channel.
 */
typedef enum {
	QM_MBOX_CH_0 = 0, /**< Channel 0. */
	QM_MBOX_CH_1,     /**< Channel 1. */
	QM_MBOX_CH_2,     /**< Channel 2. */
	QM_MBOX_CH_3,     /**< Channel 3. */
	QM_MBOX_CH_4,     /**< Channel 4. */
	QM_MBOX_CH_5,     /**< Channel 5. */
	QM_MBOX_CH_6,     /**< Channel 6. */
	QM_MBOX_CH_7,     /**< Channel 7. */
	QM_MBOX_CH_NUM    /**< Mailbox number of channels. */
} qm_mbox_ch_t;

/**
 * mailbox message pay-load index values.
 */
typedef enum {
	QM_MBOX_PAYLOAD_0 = 0, /**< Payload index value 0. */
	QM_MBOX_PAYLOAD_1,     /**< Payload index value 1. */
	QM_MBOX_PAYLOAD_2,     /**< Payload index value 2. */
	QM_MBOX_PAYLOAD_3,     /**< Payload index value 3. */
	QM_MBOX_PAYLOAD_NUM,   /**< Numbers of payloads. */
} qm_mbox_payload_t;

/**
 * Definition of the mailbox message.
 */
typedef struct {
	uint32_t ctrl;			    /**< Mailbox control element. */
	uint32_t data[QM_MBOX_PAYLOAD_NUM]; /**< Mailbox data buffer. */
} qm_mbox_msg_t;

/**
 * Definition of the mailbox callback function prototype.
 * @param[in] data The callback user data.
 */
typedef void (*qm_mbox_callback_t)(void *data);

/**
 * Set the mailbox channel configuration.
 *
 * Configure a IRQ callback, enables or disables IRQ for the chosen mailbox
 * channel.
 *
 * @param[in] mbox_ch Mailbox to enable.
 * @param[in] mpr_cb Callback function to call on read mailbox, NULL for a
 * write to Mailbox).
 * @param[in] cb_data Callback function data to return via the callback.
 * function. This must not be NULL.
 * @param[in] irq_en Flag to enable/disable IRQ for this channel.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_mbox_ch_set_config(const qm_mbox_ch_t mbox_ch, qm_mbox_callback_t mpr_cb,
		    void *cb_data, const bool irq_en);

/**
 * Write to a specified mailbox channel.
 *
 * @param[in] mbox_ch Mailbox channel identifier.
 * @param[in] msg Pointer to the data to write to the mailbox channel. This
 * must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_mbox_ch_write(const qm_mbox_ch_t mbox_ch,
		const qm_mbox_msg_t *const msg);

/**
 * Read specified mailbox channel.
 *
 * @param[in] mbox_ch mailbox channel identifier.
 * @param[out] data pointer to the data to read from the mailbox channel. This
 * must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
*/
int qm_mbox_ch_read(const qm_mbox_ch_t mbox_ch, qm_mbox_msg_t *const msg);

/**
 * Retrieve the specified mailbox channel status.
 *
 * @param[in] mbox_ch Mailbox identifier to retrieve the status from.
 * @param[out] status Mailbox status. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_mbox_ch_get_status(const qm_mbox_ch_t mbox_ch,
		qm_mbox_ch_status_t *const status);

/**
 * Acknowledge the data arrival.
 * @param[in] mbox_ch: Mailbox identifier to retrieve the status from.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_mbox_ch_data_ack(const qm_mbox_ch_t mbox_ch);

/**
 * @}
 */
#endif /* HAS_MAILBOX */
#endif /* __QM_MAILBOX_H__ */
