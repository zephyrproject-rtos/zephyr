/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * Copyright (c) 2015 Xilinx, Inc.
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 * Copyright 2016 NXP
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
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _RPMSG_QUEUE_H
#define _RPMSG_QUEUE_H

#include "rpmsg_lite.h"

//! @addtogroup rpmsg_queue
//! @{

/*! \typedef rpmsg_queue_handle
    \brief Rpmsg queue handle type.
*/
typedef void *rpmsg_queue_handle;

/* RL_API_HAS_ZEROCOPY has to be enabled for RPMsg Queue to work */
#if defined(RL_API_HAS_ZEROCOPY) && (RL_API_HAS_ZEROCOPY == 1)

/*******************************************************************************
 * API
 ******************************************************************************/

/* Exported API functions */

#if defined(__cplusplus)
extern "C" {
#endif

/*!
* @brief
* This callback needs to be registered with an endpoint
*
* @param payload Pointer to the buffer containing received data
* @param payload_len Size of data received, in bytes
* @param src Pointer to address of the endpoint from which data is received
* @param priv Private data provided during endpoint creation
*
* @return RL_HOLD or RL_RELEASE to release or hold the buffer in payload
*/
int rpmsg_queue_rx_cb(void *payload, int payload_len, unsigned long src, void *priv);

/*!
* @brief
* Create a RPMsg queue which can be used
* for blocking reception.
*
* @param rpmsg_lite_dev    RPMsg Lite instance
*
* @return RPMsg queue handle or RL_NULL
*
*/
rpmsg_queue_handle rpmsg_queue_create(struct rpmsg_lite_instance *rpmsg_lite_dev);

/*!
* @brief
* Destroy a queue and clean up.
* Do not destroy a queue which is registered with an active endpoint!
*
* @param rpmsg_lite_dev    RPMsg-Lite instance
* @param[in] q             RPMsg queue handle to destroy
*
* @return Status of function execution
*
*/
int rpmsg_queue_destroy(struct rpmsg_lite_instance *rpmsg_lite_dev, rpmsg_queue_handle q);

/*!
 * @brief
 * blocking receive function - blocking version of the received function that can be called from an RTOS task.
 * The data is copied from the receive buffer into the user supplied buffer.
 *
 * This is the "receive with copy" version of the RPMsg receive function. This version is simple
 * to use but it requires copying data from shared memory into the user space buffer.
 * The user has no obligation or burden to manage the shared memory buffers.
 *
 * @param rpmsg_lite_dev    RPMsg-Lite instance
 * @param[in] q             RPMsg queue handle to listen on
 * @param[in] data          Pointer to the user buffer the received data are copied to
 * @param[out] len          Pointer to an int variable that will contain the number of bytes actually copied into the
 * buffer
 * @param[in] maxlen        Maximum number of bytes to copy (received buffer size)
 * @param[out] src          Pointer to address of the endpoint from which data is received
 * @param[in] timeout       Timeout, in milliseconds, to wait for a message. A value of 0 means don't wait (non-blocking
 * call).
 *                          A value of 0xffffffff means wait forever (blocking call).
 *
 * @return Status of function execution
 *
 * @see rpmsg_queue_recv_nocopy
 */
int rpmsg_queue_recv(struct rpmsg_lite_instance *rpmsg_lite_dev,
                     rpmsg_queue_handle q,
                     unsigned long *src,
                     char *data,
                     int maxlen,
                     int *len,
                     unsigned long timeout);

/*!
 * @brief
 * blocking receive function - blocking version of the received function that can be called from an RTOS task.
 * The data is NOT copied into the user-app. buffer.
 *
 * This is the "zero-copy receive" version of the RPMsg receive function. No data is copied.
 * Only the pointer to the data is returned. This version is fast, but it requires the user to manage
 * buffer allocation. Specifically, the user must decide when a buffer is no longer in use and
 * make the appropriate API call to free it, see rpmsg_queue_nocopy_free().
 *
 * @param rpmsg_lite_dev    RPMsg Lite instance
 * @param[in] q             RPMsg queue handle to listen on
 * @param[out] data         Pointer to the RPMsg buffer of the shared memory where the received data is stored
 * @param[out] len          Pointer to an int variable that that will contain the number of valid bytes in the RPMsg
 * buffer
 * @param[out] src          Pointer to address of the endpoint from which data is received
 * @param[in] timeout       Timeout, in milliseconds, to wait for a message. A value of 0 means don't wait (non-blocking
 * call).
 *                          A value of 0xffffffff means wait forever (blocking call).
 *
 * @return Status of function execution.
 *
 * @see rpmsg_queue_nocopy_free
 * @see rpmsg_queue_recv
 */
int rpmsg_queue_recv_nocopy(struct rpmsg_lite_instance *rpmsg_lite_dev,
                            rpmsg_queue_handle q,
                            unsigned long *src,
                            char **data,
                            int *len,
                            unsigned long timeout);

/*!
 * @brief This function frees a buffer previously returned by rpmsg_queue_recv_nocopy().
 *
 * Once the zero-copy mechanism of receiving data is used, this function
 * has to be called to free a buffer and to make it available for the next data
 * transfer.
 *
 * @param rpmsg_lite_dev    RPMsg-Lite instance
 * @param[in] data          Pointer to the RPMsg buffer of the shared memory that has to be freed
 *
 * @return Status of function execution.
 *
 * @see rpmsg_queue_recv_nocopy
 */
int rpmsg_queue_nocopy_free(struct rpmsg_lite_instance *rpmsg_lite_dev, void *data);

/*!
 * @brief This function returns the number of pending messages in the queue.
 *
 * @param[in] q             RPMsg queue handle
 *
 * @return Number of pending messages in the queue.
 */
int rpmsg_queue_get_current_size(rpmsg_queue_handle q);

//! @}

#if defined(__cplusplus)
}
#endif

#endif /* RL_API_HAS_ZEROCOPY */

#endif /* _RPMSG_QUEUE_H */
