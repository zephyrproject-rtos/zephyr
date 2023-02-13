/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

#include <zephyr/toolchain.h>
#include <zephyr/types.h>
#include <zephyr/sys/util.h>

#include "hal/ccm.h"

#include "util/memq.h"
#include "util/mem.h"

#include "pdu_df.h"
#include "pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll_conn_iso.h"

#include <soc.h>
#include "hal/debug.h"


int lll_conn_iso_init(void)
{
	return 0;
}

int lll_conn_iso_reset(void)
{
	return 0;
}

void lll_conn_iso_flush(uint16_t handle, struct lll_conn_iso_stream *lll)
{
	ARG_UNUSED(handle);
	ARG_UNUSED(lll);
}
