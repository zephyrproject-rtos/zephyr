/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Render a pipeline's topology for debugging.
 * @ingroup mpipe_dump
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_UTILS_MPIPE_DUMP_H_
#define ZEPHYR_INCLUDE_MPIPE_UTILS_MPIPE_DUMP_H_

/**
 * @defgroup mpipe_dump Topology dump
 * @ingroup mpipe
 * @brief Render a bin's topology and the capability negotiated on each pad.
 *
 * Renders a bin as a Graphviz graph: every element with its state, its pads as
 * ports, the peer each pad is linked to and the capability that pad settled on.
 * Negotiation is what logs show least well, so this is the tool for "what did
 * each link agree on" and for "which pad is still unlinked".
 *
 * The target emits text and the host lays it out. A board with no filesystem
 * sends the text down its console and the graph is cut out of a serial capture:
 *
 * @code{.sh}
 * sed -n '/^digraph/,/^}/p' capture.log > pipe.dot
 * dot -Tpng pipe.dot -o pipe.png
 * @endcode
 *
 * An element whose pads are not all linked is drawn with a red border, and one
 * nothing reaches simply stands alone in the layout, so a broken graph shows
 * itself without a separate report.
 *
 * What is rendered is the graph as it is *now*, which is not always the graph
 * the application built: an element may legitimately take itself out once it
 * has nothing left to do, and a caps filter element does exactly that,
 * relinking its neighbors to each other after negotiation and re-inserting
 * itself on PAUSED to READY. A caps filter therefore shows up linked before a
 * run and detached during one, and both readings are correct.
 *
 * Nothing here allocates, and the walk holds no lock: a dump taken while the
 * pipeline is changing state is a snapshot that may catch a pad mid-update.
 * Dump from a settled state to read a settled answer.
 *
 * @{
 */

#include <zephyr/mpipe/mpipe_bin.h>
#include <zephyr/mpipe/mpipe_structure.h>

/**
 * @brief Output callback of a dump.
 *
 * The dump renders one line at a time and hands each to the callback, so it
 * needs no buffer of its own and can write wherever the caller wants: a shell
 * instance, the console, or a buffer a test then asserts on.
 *
 * @param ctx The context the dump was given, passed back untouched.
 * @param str One rendered line, NUL-terminated, with its newline if the
 *            rendering has one there.
 */
typedef void (*mpipe_dump_print_t)(void *ctx, const char *str);

/**
 * @brief Render a bin's topology and the capability on each of its pads.
 *
 * @param bin Bin to render. A pipeline is one, cast to @ref mpipe_bin.
 * @param print Where to write, or NULL to write to the console with printk().
 * @param ctx Passed to @p print untouched.
 *
 * @retval 0 Success.
 */
int mpipe_dump_bin(struct mpipe_bin *bin, mpipe_dump_print_t print, void *ctx);

/**
 * @brief Render one capability on a single line.
 *
 * Writes it as `<video, format=RGBP, width=640, height=480>`, a range as
 * `[min, max, step]`, a capability constraining nothing as `<any>` or
 * `<empty>`. Every capability is delimited the same way, so one can be picked
 * out of a line that carries other text after it. Ends without a newline, so a
 * caller can put it where it wants.
 *
 * @param caps Capability to render.
 * @param print Where to write, or NULL to write to the console with printk().
 * @param ctx Passed to @p print untouched.
 *
 * @retval 0 Success.
 */
int mpipe_dump_caps(const struct mpipe_structure *caps, mpipe_dump_print_t print, void *ctx);

/**
 * @brief Name an element state.
 *
 * @param state State to name, see @ref mpipe_state.
 * @return A short human-readable name, never NULL.
 */
const char *mpipe_dump_state_str(enum mpipe_state state);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_UTILS_MPIPE_DUMP_H_ */
