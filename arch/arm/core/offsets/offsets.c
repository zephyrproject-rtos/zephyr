/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gen_offset.h>

#if defined(CONFIG_ARM64)
#include "offsets_aarch64.c"
#else
#include "offsets_aarch32.c"
#endif

GEN_ABS_SYM_END
