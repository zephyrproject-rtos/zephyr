/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Application sink element.
 * @ingroup mpipe_app_sink
 *
 * The app_sink element hands the buffers reaching the end of a pipeline to
 * the application, either through a callback running in the pipeline's
 * streaming context or through a pull API the application calls at its own
 * pace.
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_BASE_MPIPE_APP_SINK_H_
#define ZEPHYR_INCLUDE_MPIPE_BASE_MPIPE_APP_SINK_H_

/**
 * @defgroup mpipe_app_sink Application sinks
 * @ingroup mpipe_base
 * @brief Element handing pipeline buffers to the application.
 *
 * The application boundary on the consuming side. Each buffer reaching the
 * sink is delivered once, fragment by fragment: to the callback when one is
 * registered, otherwise to a queue the application drains with
 * @ref mpipe_app_sink_pull. End of stream is reported on the pipeline bus,
 * as for every sink.
 *
 * @{
 */

#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>

#include <zephyr/mpipe/mpipe_element.h>
#include <zephyr/mpipe/mpipe_sink.h>
#include <zephyr/mpipe/mpipe_structure.h>

/**
 * @brief Application sink property identifiers
 */
enum {
	/**
	 * Capability this sink accepts, a const struct mpipe_structure pointer,
	 * copied. Until it is set the sink accepts anything.
	 */
	MPIPE_PROP_BASE_APP_SINK_CAPS = MPIPE_PROP_SINK_LAST,
	/**
	 * Buffer callback, a const struct mpipe_app_sink_cb pointer, copied.
	 * With a callback set, buffers are not queued for pulling.
	 */
	MPIPE_PROP_BASE_APP_SINK_CB,
};

/**
 * @brief Buffer callback
 *
 * Runs in the thread driving the sink: keep it short and copy out what is
 * needed. The buffer belongs to the element and is released on return.
 *
 * @param buf The buffer that reached the sink
 * @param user_data The pointer registered with the callback
 */
typedef void (*mpipe_app_sink_cb_t)(const struct net_buf *buf, void *user_data);

/**
 * @brief Callback registration for MPIPE_PROP_BASE_APP_SINK_CB
 */
struct mpipe_app_sink_cb {
	/** Function to call for every buffer */
	mpipe_app_sink_cb_t fn;
	/** Passed through to the callback */
	void *user_data;
};

/**
 * @brief Application sink element structure
 */
struct mpipe_app_sink {
	/** Base sink element (must be first) */
	struct mpipe_sink sink;
	/** Capability the sink accepts, offered at every negotiation */
	struct mpipe_structure caps;
	/** Optional buffer callback */
	struct mpipe_app_sink_cb cb;
	/** Queue of buffers awaiting a pull (when no callback is set) */
	struct k_msgq msgq;
	/** Backing storage for the queue */
	char msgq_buffer[CONFIG_MPIPE_BASE_APP_SINK_QUEUE_DEPTH * sizeof(void *)];
};

/**
 * @brief Initialize an application sink element
 *
 * @param app_sink Pointer to the element to initialize.
 * @param id Unique element identifier.
 *
 * @return 0 on success, negative errno otherwise.
 */
int mpipe_app_sink_init(struct mpipe_app_sink *app_sink, uint8_t id);

/**
 * @brief Pull the next buffer
 *
 * Only meaningful when no callback is set. The caller owns the returned
 * buffer and must release it with net_buf_unref(). A buffer arriving while
 * the queue is full is dropped, so a slow puller loses data rather than
 * stalling the pipeline.
 *
 * @param app_sink The application sink
 * @param buf Filled with the pulled buffer
 * @param timeout How long to wait for a buffer
 *
 * @retval 0 Success
 * @retval -EINVAL Bad arguments
 * @retval -EAGAIN No buffer arrived within @p timeout
 */
int mpipe_app_sink_pull(struct mpipe_app_sink *app_sink, struct net_buf **buf, k_timeout_t timeout);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_BASE_MPIPE_APP_SINK_H_ */
