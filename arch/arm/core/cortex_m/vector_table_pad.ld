/*
 * Copyright (c) 2019 - 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Padding inserted after the (first-stage) vector table, so that the
 * Zephyr image does not attempt to use the area which we reserve to
 * detect null pointer dereferencing (0x0 - <size>). If the end of the
 * vector table section is higher than the upper end of the reserved
 * area, we add no padding.
 *
 * Note that even if the following linker script snippet is included
 * multiple times, the padding will only be added at most once, to the
 * first stage vector table.
 */

#if defined(CONFIG_CORTEX_M_NULL_POINTER_EXCEPTION)
. = MAX(ABSOLUTE(.), CONFIG_CORTEX_M_NULL_POINTER_EXCEPTION_PAGE_SIZE);
#endif /* CONFIG_CORTEX_M_NULL_POINTER_EXCEPTION */
