/*
 * Copyright (c) 2025 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

#include "hal/ccm.h"

#include "util/mem.h"
#include "util/memq.h"

#include "isoal.h"
#include "ull_iso_types.h"
#include "lll.h"
#include "lll_conn_iso.h"
#include "ull_conn_iso_types.h"

bool ull_central_iso_all_cises_terminated(struct ll_conn_iso_group *cig)
{
	return true;
}
