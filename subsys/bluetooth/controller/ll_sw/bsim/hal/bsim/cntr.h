/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>

/* The compare event shares its interrupt line with the ULL mayflies, so the
 * ISR has to ask whether the compare is what fired.
 */
bool cntr_cmp_evt_get_clear(void);
