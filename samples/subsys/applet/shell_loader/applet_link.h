/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APPLET_SHELL_LOADER_APPLET_LINK_H_
#define APPLET_SHELL_LOADER_APPLET_LINK_H_

#include <stdint.h>

/** The token is not owned by anyone. */
#define APPLET_LINK_IDLE    0u
/** The token belongs to the "pong" applet. */
#define APPLET_LINK_TO_PONG 1u
/** The token belongs to the "ping" applet. */
#define APPLET_LINK_TO_PING 2u

/**
 * @brief Hand-off point between two applets.
 *
 * The shell application places this structure in a memory partition of its own
 * and adds that partition to every applet it loads, so applets that are
 * otherwise isolated from each other can still exchange a value. Ownership of
 * @c token strictly alternates between the two sides, so a plain volatile word
 * is enough to keep them from writing at the same time.
 */
struct applet_link {
	/** One of the @c APPLET_LINK_* values, says who owns @c token. */
	volatile uint32_t state;
	/** Value being handed over. */
	volatile uint32_t token;
	/** Number of completed hand-offs, maintained by the "pong" applet. */
	volatile uint32_t exchanges;
};

/**
 * @brief Get the shared hand-off point.
 *
 * Exported to extensions by the shell application, so an applet does not need
 * to know where the shared partition ended up in memory.
 */
struct applet_link *applet_link_get(void);

#endif /* APPLET_SHELL_LOADER_APPLET_LINK_H_ */
