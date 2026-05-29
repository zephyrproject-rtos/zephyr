/*
 * Copyright (c) 2026 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Constants library C: depends on B.
 */

#include <gen_offset.h>
#include <zephyr/test_constants_b.h>

GEN_ABS_SYM_BEGIN(_TestConstantsC)

GEN_ABSOLUTE_SYM(TEST_CONST_C_SIZEOF, TEST_CONST_B_SIZEOF + 100);

GEN_ABS_SYM_END
