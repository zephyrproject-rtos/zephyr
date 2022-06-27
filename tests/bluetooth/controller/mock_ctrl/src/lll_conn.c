/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>

#include <zephyr/toolchain.h>
#include <zephyr/types.h>
#include <zephyr/sys/util.h>

#include "hal/cpu.h"
#include "hal/ccm.h"
#include "hal/radio.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/mfifo.h"
#include "util/dbuf.h"

#include "pdu.h"

#include "lll.h"
#include "lll_df_types.h"
#include "lll_conn.h"

#include "lll_internal.h"
#include "lll_tim_internal.h"
#include "lll_prof_internal.h"

void lll_conn_flush(uint16_t handle, struct lll_conn *lll)
{
}
