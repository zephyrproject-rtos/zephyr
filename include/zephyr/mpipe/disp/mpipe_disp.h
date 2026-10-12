/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Display plugin.
 * @ingroup mpipe_disp
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_DISP_MPIPE_DISP_H_
#define ZEPHYR_INCLUDE_MPIPE_DISP_MPIPE_DISP_H_

/**
 * @defgroup mpipe_disp Display
 * @ingroup mpipe_plugins
 * @brief The element that puts frames on a screen.
 *
 * The display plugin sits on Zephyr's display API and terminates a video graph.
 * It is a sink in the strict sense: frames arrive and nothing leaves.
 *
 * What it accepts comes from the panel rather than from the element, so its
 * capabilities are enumerated from the display's own pixel-format bitmask and
 * its resolution, which is why a graph ending here usually needs a converter in
 * front of it unless the source can already produce what the panel takes.
 */

#endif /* ZEPHYR_INCLUDE_MPIPE_DISP_MPIPE_DISP_H_ */
