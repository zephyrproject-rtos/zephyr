/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_INST_0_SHARED_IRQ_IRQ_0_SENSE	DT_INST_0_SHARED_IRQ_IRQ_0_FLAGS

#undef DT_INST_0_SHARED_IRQ_IRQ_0
#define DT_INST_0_SHARED_IRQ_IRQ_0	((DT_SHARED_IRQ_SHAREDIRQ0_IRQ_0 + 1) << 8)
