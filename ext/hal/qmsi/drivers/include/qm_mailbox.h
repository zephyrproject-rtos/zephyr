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
 * Mailbox channel status return codes
 */
typedef enum {
	/** No interrupt pending nor any data to consume. */
	QM_MBOX_CH_IDLE = 0,
	/** Receiver has serviced the interrupt and data
	 * has not been consumed. */
	QM_MBOX_CH_INT_ACK_DATA_PEND,
	/** Receiver in polling mode and data has not been consumed. */
	QM_MBOX_CH_POLLING_DATA_PEND,
	/** Receiver hasn't serviced the interrupt and data
	 * has not been consumed.
	 */
	QM_MBOX_CH_INT_NACK_DATA_PEND,
} qm_mbox_ch_status_t;

/**
 * Mailbox channel identifiers
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
 * Mailbox message payload index values.
 */
typedef enum {
	QM_MBOX_PAYLOAD_0 = 0, /**< Payload index value 0. */
	QM_MBOX_PAYLOAD_1,     /**< Payload index value 1. */
	QM_MBOX_PAYLOAD_2,     /**< Payload index value 2. */
	QM_MBOX_PAYLOAD_3,     /**< Payload index value 3. */
	QM_MBOX_PAYLOAD_NUM,   /**< Number of payloads. */
} qm_mbox_payload_t;

/**
 * Definition of the mailbox direction of operation
 * The direction of communication for each channel is configurable by the user.
 * The list below describes the possible communication directions for each
 * channel.
 */
typedef enum {
	QM_MBOX_TO_LMT = 0, /**< Lakemont core as destination */
	QM_MBOX_TO_SS,      /**< Sensor Sub-System core as destination */
	QM_MBOX_UNUSED
} qm_mbox_destination_t;

/**
 * Definition of the mailbox mode of operation, interrupt mode or polling mode.
 */
typedef enum {
	/** Mailbox channel operates in interrupt mode. */
	QM_MBOX_INTERRUPT_MODE = 0,
	/** Mailbox channel operates in polling mode. */
	QM_MBOX_POLLING_MODE
} qm_mbox_mode_t;

/**
 * Definition of the mailbox message.
 */
typedef struct {
	/** Control word - bits 30 to 0 used as data/message id,
	 * bit 31 triggers channel interrupt when set by the driver.
	 */
	uint32_t ctrl;
	/** Mailbox data buffer. */
	uint32_t data[QM_MBOX_PAYLOAD_NUM];
} qm_mbox_msg_t;

/**
 * Definition of the mailbox callback function prototype.
 *
 * @param[in] data The callback user data.
 */
typedef void (*qm_mbox_callback_t)(void *data);

/**
 * Mailbox Configuration Structure
 */
typedef struct {
	/**< Mailbox Destination */
	qm_mbox_destination_t dest;
	/**< Mode of operation */
	qm_mbox_mode_t mode;

	/**<
	 * Message callback.
	 *
	 * Called after a message is received on the channel and the channel
	 * is configured in interrupt mode.
	 *
	 * @note NULL for a write Mailbox.
	 *
	 * @param[in] data User defined data.
	 */
	qm_mbox_callback_t callback;
	/**< Callback function data to return via the callback function. */
	void *callback_data;
} qm_mbox_config_t;

/**
 * Set the mailbox channel configuration.
 *
 * The function registers the interrupt vector to the mailbox ISR handler
 * when at least one mailbox channel is enabled, configured in interrupt mode
 * and the ISR is not handled by the application.
 *
 * @param[in] mbox_ch Mailbox channel identifier.
 * @param[in] config Mailbox configuration.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_mbox_ch_set_config(const qm_mbox_ch_t mbox_ch,
			  const qm_mbox_config_t *const config);

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
 * @param[in] mbox_ch Mailbox channel identifier.
 * @param[out] msg Pointer to the data to read from the mailbox channel. This
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
 * @}
 */

#endif /* HAS_MAILBOX */
#endif /* __QM_MAILBOX_H__ */
