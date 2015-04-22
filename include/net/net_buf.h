/*! @file
 @brief Network buffer API

 Network data is passed between application and IP stack via
 a net_buf struct.
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
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

/* Data buffer API - used for all data to/from net */

#ifndef __NET_BUF_H
#define __NET_BUF_H

#include <stdint.h>
#include <net/core/ip/uip.h>

struct net_context;

/*! The default MTU is 1280 (minimum IPv6 packet size) + LL header
 */
#define NET_BUF_MAX_DATA (UIP_LINK_MTU + UIP_LLH_LEN)

struct net_buf {
	/*! @cond ignore */
	/* FIFO uses first 4 bytes itself, reserve space */
	int __unused;
	/* @endcond */

	/*! Network connection context */
	struct net_context *context;

	/*! Buffer data length */
	uint16_t len;
	/*! Buffer head pointer */
	uint8_t *data;
	/*! Actual network buffer storage */
	uint8_t buf[NET_BUF_MAX_DATA];
};

/*!
 * @brief Get buffer from the available buffers pool.
 *
 * @details Get network buffer from buffer pool. You must have
 * network context before able to use this function.
 *
 * @param context Network context that will be related to
 * this buffer.
 *
 * @return Network buffer if successful, NULL otherwise.
 */
struct net_buf *net_buf_get(struct net_context *context);

/*!
 * @brief Get buffer from pool but also reserve headroom for
 * potential headers.
 *
 * @details Normally this version is not useful for applications
 * but is mainly used by network fragmentation code.
 *
 * @param reserve How many bytes to reserve for headroom.
 *
 * @return Network buffer if successful, NULL otherwise.
 */
struct net_buf *net_buf_get_reserve(uint16_t reserve_head);

/*!
 * @brief Place buffer back into the available buffers pool.
 *
 * @details Releases the buffer to other use. This needs to be
 * called by application after it has finished with
 * the buffer.
 *
 * @param buf Network buffer to release.
 *
 */
void net_buf_put(struct net_buf *buf);

/*!
 * @brief Prepare data to be added at the end of the buffer.
 *
 * @details Move the tail pointer forward.
 *
 * @param buf Network buffer.
 * @param len Size of data to be added.
 *
 * @return Pointer to new tail.
 */
uint8_t *net_buf_add(struct net_buf *buf, uint16_t len);

/*!
 * @brief Push data to the beginning of the buffer.
 *
 * @details Move the data pointer backwards.
 *
 * @param buf Network buffer.
 * @param len Size of data to be added.
 *
 * @return Pointer to new head.
 */
uint8_t *net_buf_push(struct net_buf *buf, uint16_t len);

/*!
 * @brief Remove data from the beginning of the buffer.
 *
 * @details Move the data pointer forward.
 *
 * @param buf Network buffer.
 * @param len Size of data to be removed.
 *
 * @return Pointer to new head.
 */
uint8_t *net_buf_pull(struct net_buf *buf, uint16_t len);

/*! @def net_buf_tail
 *
 * @brief Return pointer to the end of the data in the buffer.
 *
 * @details This macro returns the tail of the buffer.
 *
 * @param buf Network buffer.
 *
 * @return Pointer to tail.
 */
#define net_buf_tail(buf) ((buf)->data + (buf)->len)

/*! @cond ignore */
void net_buf_init(void);
/* @endcond */

#endif /* __NET_BUF_H */
