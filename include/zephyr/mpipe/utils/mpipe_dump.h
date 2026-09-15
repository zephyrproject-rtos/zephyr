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
 * has nothing left to do, and @ref mpipe_caps_filter does exactly that, relinking
 * its neighbors to each other after negotiation and re-inserting itself on
 * PAUSED to READY. A caps_filter therefore shows up linked before a run and
 * detached during one, and both readings are correct.
 *
 * Nothing here allocates, and the walk holds no lock: a dump taken while the
 * pipeline is changing state is a snapshot that may catch a pad mid-update.
 * Dump from a settled state to read a settled answer.
 *
 * @{
 */

#include <stdarg.h>

#include <zephyr/mpipe/mpipe_bin.h>
#include <zephyr/mpipe/mpipe_structure.h>

/**
 * @brief Where a dump writes its output.
 *
 * The dump formats one piece at a time and hands each to @ref vprint, so it
 * needs no buffer of its own and can write wherever the caller wants: a shell
 * instance, the console, or a buffer a test then asserts on.
 */
struct mpipe_dump_sink {
	/** Called with each formatted piece of the dump */
	void (*vprint)(void *ctx, const char *fmt, va_list ap);
	/** Passed back to @ref vprint untouched */
	void *ctx;
};

/**
 * @brief Render a bin's topology and the capability on each of its pads.
 *
 * @param bin Bin to render. A pipeline is one, cast to @ref mpipe_bin.
 * @param sink Where to write, or NULL to write to the console with printk().
 *
 * @retval 0 Success.
 */
int mpipe_dump_bin(struct mpipe_bin *bin, const struct mpipe_dump_sink *sink);

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
 * @param sink Where to write, or NULL to write to the console with printk().
 *
 * @retval 0 Success.
 */
int mpipe_dump_caps(const struct mpipe_structure *caps, const struct mpipe_dump_sink *sink);

/**
 * @brief Name an element state.
 *
 * @param state State to name, see @ref mpipe_state.
 * @return A short human-readable name, never NULL.
 */
const char *mpipe_dump_state_str(enum mpipe_state state);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_UTILS_MPIPE_DUMP_H_ */
