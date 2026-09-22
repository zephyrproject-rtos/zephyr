/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Application source element.
 * @ingroup mpipe_app_src
 *
 * The app_src element lets the application inject data into a pipeline: the
 * application pushes payloads at its own pace and declares end of stream
 * explicitly. The pipeline thread consumes them as any other source's buffers.
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_BASE_MPIPE_APP_SRC_H_
#define ZEPHYR_INCLUDE_MPIPE_BASE_MPIPE_APP_SRC_H_

/**
 * @defgroup mpipe_app_src Application sources
 * @ingroup mpipe_base
 * @brief Element injecting application data into a pipeline.
 *
 * The application boundary on the producing side. Payloads either travel by
 * copy, with @ref mpipe_app_src_push, or in place: @ref mpipe_app_src_alloc
 * hands out a buffer of the element's pool for the application to fill, and
 * @ref mpipe_app_src_push_buf queues it.
 *
 * Push once the pipeline is at least PAUSED: whatever was queued while READY
 * is discarded when the pipeline starts. End of stream is declared with
 * @ref mpipe_app_src_eos; alternatively MPIPE_PROP_SRC_NUM_BUFS ends the
 * stream after a number of buffers.
 *
 * @{
 */

#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>

#include <zephyr/mpipe/mpipe_buffer.h>
#include <zephyr/mpipe/mpipe_element.h>
#include <zephyr/mpipe/mpipe_src.h>
#include <zephyr/mpipe/mpipe_structure.h>

/**
 * @brief Application source property identifiers
 */
enum {
	/**
	 * Capability of the produced stream, a const struct mpipe_structure
	 * pointer, copied. It is what negotiation offers downstream; what the
	 * application pushes must match it, the element cannot check. Until it
	 * is set the element offers a capability constraining nothing.
	 */
	MPIPE_PROP_BASE_APP_SRC_CAPS = MPIPE_PROP_SRC_LAST,
};

/**
 * @brief Application source element structure
 */
struct mpipe_app_src {
	/** Base source element (must be first) */
	struct mpipe_src src;
	/** Capability of the produced stream, offered at every negotiation */
	struct mpipe_structure caps;
	/** Buffer pool the payloads travel in */
	struct mpipe_buffer_pool pool;
	/** Queue of pushed buffers, consumed by the pipeline thread */
	struct k_msgq msgq;
	/** Backing storage: the configured depth plus one slot for a sentinel */
	char msgq_buffer[(CONFIG_MPIPE_BASE_APP_SRC_QUEUE_DEPTH + 1) * sizeof(void *)];
};

/**
 * @brief Initialize an application source element
 *
 * @param app_src Pointer to the element to initialize.
 * @param id Unique element identifier.
 *
 * @return 0 on success, negative errno otherwise.
 */
int mpipe_app_src_init(struct mpipe_app_src *app_src, uint8_t id);

/**
 * @brief Take a buffer of the element's pool to fill in place
 *
 * The buffer holds up to CONFIG_MPIPE_BASE_APP_SRC_BUF_SZ bytes. The
 * application writes its payload into the buffer data and hands the buffer
 * to @ref mpipe_app_src_push_buf, or releases it with net_buf_unref() if it
 * changes its mind.
 *
 * @param app_src The application source
 * @param size Number of payload bytes the buffer must hold
 * @param timeout How long to wait for a free buffer
 * @param buf Filled with the buffer
 *
 * @retval 0 Success
 * @retval -EINVAL Bad arguments, or @p size above the buffer size
 * @retval -ENOBUFS No buffer became free within @p timeout
 */
int mpipe_app_src_alloc(struct mpipe_app_src *app_src, uint32_t size, k_timeout_t timeout,
			struct net_buf **buf);

/**
 * @brief Queue a filled buffer into the pipeline
 *
 * The buffer comes from @ref mpipe_app_src_alloc. On success the pipeline
 * owns it; on failure it stays the caller's, to retry or release.
 *
 * @param app_src The application source
 * @param buf The buffer to queue
 * @param size Number of valid payload bytes in the buffer
 * @param timeout How long to wait for a queue slot
 *
 * @retval 0 Success
 * @retval -EINVAL Bad arguments, or @p size above what the buffer holds
 * @retval -EAGAIN The queue stayed full within @p timeout
 */
int mpipe_app_src_push_buf(struct mpipe_app_src *app_src, struct net_buf *buf, uint32_t size,
			   k_timeout_t timeout);

/**
 * @brief Push one payload into the pipeline, by copy
 *
 * Takes a buffer, copies @p size bytes into it and queues it: the two waits
 * may each last up to @p timeout.
 *
 * @param app_src The application source
 * @param data Payload bytes
 * @param size Number of bytes, bounded by CONFIG_MPIPE_BASE_APP_SRC_BUF_SZ
 * @param timeout How long to wait for a free buffer, and for a queue slot
 *
 * @retval 0 Success
 * @retval -EINVAL Bad arguments
 * @retval -ENOBUFS No buffer became free within @p timeout
 * @retval -EAGAIN The queue stayed full within @p timeout
 */
int mpipe_app_src_push(struct mpipe_app_src *app_src, const void *data, uint32_t size,
		       k_timeout_t timeout);

/**
 * @brief Declare end of stream
 *
 * Buffers pushed before this call are still delivered; the pipeline then
 * receives EOS.
 *
 * @param app_src The application source
 * @param timeout How long to wait for a queue slot
 *
 * @retval 0 Success
 * @retval -EINVAL @p app_src is NULL
 * @retval -EAGAIN The queue stayed full within @p timeout
 */
int mpipe_app_src_eos(struct mpipe_app_src *app_src, k_timeout_t timeout);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_BASE_MPIPE_APP_SRC_H_ */
