/**
 * Copyright (c) 2026 Open Device Partnership and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Instantiates the shared bq4xz50 implementation for the bq41z50.
 */

#define DT_DRV_COMPAT ti_bq41z50

#include "bq4xz50.h"

DT_INST_FOREACH_STATUS_OKAY(BQ4XZ50_DEFINE)
