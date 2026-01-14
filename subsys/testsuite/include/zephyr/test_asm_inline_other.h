/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TEST_ASM_INLINE_OTHER_H_
#define ZEPHYR_TEST_ASM_INLINE_OTHER_H_

/* Stub for non-GNUC toolchains (e.g. IAR) used in benchmark tests.
 * These macros are only required by timestamp.h during kernel tests.
 * We provide a no-op implementation so that tests can build.
 */

static inline void timestamp_serialize(void) { }

#endif /* ZEPHYR_TEST_ASM_INLINE_OTHER_H_ */
