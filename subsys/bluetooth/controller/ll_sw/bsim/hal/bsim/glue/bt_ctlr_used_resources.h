/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* The BabbleSim LLL does not use any nRF PPI channels or groups. Provided
 * because nrfx_reserved_resources.h includes it when the split controller is
 * enabled on an nRF based board.
 */
#define BT_CTLR_USED_PPI_CHANNELS 0
#define BT_CTLR_USED_PPI_GROUPS   0
