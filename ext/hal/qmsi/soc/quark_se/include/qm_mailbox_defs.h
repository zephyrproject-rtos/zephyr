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

#ifndef __QM_MAILBOX_DEFS_H__
#define __QM_MAILBOX_DEFS_H__

/**
 * Mailbox definitions.
 *
 * @defgroup groupMBOX_DEFS Mailbox Definitions
 * @{
 */

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
} qm_mbox_ch_t;

/**
 * Definition of the mailbox direction of operation
 * The direction of communication for each channel is configurable by the user.
 * The list below describes the possible communication directions for each
 * channel.
 */
typedef enum {
	QM_MBOX_UNUSED = 0,
	QM_MBOX_TO_LMT, /**< Lakemont core as destination */
	QM_MBOX_TO_SS,  /**< Sensor Sub-System core as destination */
} qm_mbox_destination_t;

/**
 * @}
 */

#endif /* __QM_MAILBOX_DEFS_H__ */
