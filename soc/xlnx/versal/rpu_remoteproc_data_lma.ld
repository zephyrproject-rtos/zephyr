/*
 * Copyright (c) 2026 Space Cubics Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The Cortex-R XIP linker script uses ROMABLE_REGION as the load region for
 * initialized RAM sections. Keep runtime data in the board-selected RAM region
 * while making its load image visible to remoteproc in RPROC.
 */
#undef ROMABLE_REGION
#define ROMABLE_REGION RPROC
