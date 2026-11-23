/*
 * Copyright (c) 2026 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Constants library A: depends on B.
 */

#include <gen_offset.h>
#include <zephyr/test_constants_b.h>

GEN_ABS_SYM_BEGIN(_TestConstantsA)

GEN_ABSOLUTE_SYM(TEST_CONST_A_SIZEOF, TEST_CONST_B_SIZEOF + 1);

GEN_ABS_SYM_END
