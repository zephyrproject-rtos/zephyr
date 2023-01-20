/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/types.h>
#include <zephyr/sys/byteorder.h>
#include <soc.h>

#include "hal/cpu.h"
#include "hal/ticker.h"

#include "util/memq.h"
#include "util/mfifo.h"

#include "pdu_df.h"
#include "pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll_conn_iso.h"
#include "lll_peripheral_iso.h"

#include "hal/debug.h"

int lll_central_iso_init(void)
{
	return 0;
}

int lll_central_iso_reset(void)
{
	return 0;
}

void lll_central_iso_prepare(void *param)
{
	ARG_UNUSED(param);
}
