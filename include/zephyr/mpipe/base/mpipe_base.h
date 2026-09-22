/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Base plugin.
 * @ingroup mpipe_base
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_BASE_MPIPE_BASE_H_
#define ZEPHYR_INCLUDE_MPIPE_BASE_MPIPE_BASE_H_

/**
 * @defgroup mpipe_base Base Elements
 * @ingroup mpipe_plugins
 * @brief Media-agnostic elements that shape a graph rather than its data.
 *
 * The base plugin holds the elements that belong to no media domain. They do
 * not look at what a buffer contains: they constrain what a link may carry,
 * split a graph into branches, split it across threads, or let the
 * application be one end of it. Every domain needs them, so they live
 * together rather than being duplicated per domain.
 *
 * - caps_filter constrains the capability of the link it sits on.
 * - tee pushes each buffer to every branch of a graph.
 * - queue decouples the elements downstream of it onto their own thread.
 * - app_src injects what the application pushes, as a source.
 * - app_sink hands what reaches it to the application, as a sink.
 */

#endif /* ZEPHYR_INCLUDE_MPIPE_BASE_MPIPE_BASE_H_ */
