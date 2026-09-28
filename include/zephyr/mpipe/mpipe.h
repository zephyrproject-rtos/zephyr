/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Umbrella header.
 * @ingroup mpipe
 *
 * Applications include this header, and only this one, for the whole framework
 * API; each plugin element they instantiate adds that element's own header.
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_MPIPE_H_
#define ZEPHYR_INCLUDE_MPIPE_MPIPE_H_

/**
 * @defgroup mpipe Multimedia Pipeline
 * @ingroup os_services
 * @brief Build a media stream out of self-contained processing elements.
 * @since 4.5
 * @version 0.1.0
 */

/**
 * @defgroup mpipe_framework Framework
 * @ingroup mpipe
 * @brief The object model, the negotiation and the runtime.
 *
 * An application declares the elements it needs, links them into a graph, and
 * drives that graph through a state machine. The framework is what negotiates
 * the data format between neighbors, settles the buffer configuration, and
 * moves the buffers from one element to the next.
 *
 * @ref mpipe_object is the base every type embeds; @ref mpipe_element is what a
 * graph is made of and @ref mpipe_pad is where two elements meet;
 * @ref mpipe_structure describes what crosses a link and @ref mpipe_dispatch
 * carries the negotiation; @ref mpipe_pipeline runs the result.
 *
 * @section mpipe_no_alloc No dynamic allocation
 *
 * The framework allocates nothing. Elements and pipelines are objects the
 * application owns, buffers come from pools sized when the pipeline starts, and
 * every type on the negotiation path is fixed-size and held by value, so a
 * stream that runs for hours cannot fragment a heap it never touches. An
 * element needing scratch memory takes it from the application at init, the
 * way pools and stacks are taken. Negotiation runs on the thread that changes
 * state, which needs a stack sized for it.
 *
 * @section mpipe_null Pointer parameters
 *
 * A pointer parameter must not be NULL unless its own documentation says what
 * NULL means for it - "or NULL to reset to ANY", "or NULL if unused". Passing
 * NULL anywhere else is a programming error, not a runtime condition: the
 * caller is handing over an object it owns, so there is nothing to recover
 * from and nothing useful to report. Those are trapped by an assertion, which
 * costs nothing once @kconfig{CONFIG_ASSERT} is off.
 *
 * Values are different. Where an argument carries data rather than an object -
 * a capability that may be empty, a property whose value the application
 * chose - the API validates it and returns a negative errno, because a
 * pipeline can report that and carry on.
 */

/**
 * @defgroup mpipe_plugins Plugins
 * @ingroup mpipe
 * @brief The concrete elements, grouped by the domain they serve.
 *
 * Plugins are where the framework meets real hardware and real formats. A
 * plugin adds its own directory, Kconfig and headers without altering the
 * framework, so a vendor or a middleware provider can ship elements of its own,
 * and an application pays only for the domains it enables.
 */

#include <zephyr/mpipe/mpipe_bin.h>
#include <zephyr/mpipe/mpipe_buffer.h>
#include <zephyr/mpipe/mpipe_dispatch.h>
#include <zephyr/mpipe/mpipe_element.h>
#include <zephyr/mpipe/mpipe_message.h>
#include <zephyr/mpipe/mpipe_object.h>
#include <zephyr/mpipe/mpipe_pad.h>
#include <zephyr/mpipe/mpipe_parser.h>
#include <zephyr/mpipe/mpipe_pipeline.h>
#include <zephyr/mpipe/mpipe_sink.h>
#include <zephyr/mpipe/mpipe_src.h>
#include <zephyr/mpipe/mpipe_structure.h>
#include <zephyr/mpipe/mpipe_transform.h>
#include <zephyr/mpipe/mpipe_value.h>
#if CONFIG_MPIPE_RPC
#include <zephyr/mpipe/mpipe_transform_client.h>
#endif

#endif /* ZEPHYR_INCLUDE_MPIPE_MPIPE_H_ */
