/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/devicetree.h>
#include <zephyr/toolchain.h>

#include <ironside_client/layout.h>

BUILD_ASSERT(DT_REG_ADDR(DT_NODELABEL(sram_ns)) + DT_REG_SIZE(DT_NODELABEL(sram_ns)) ==
		     IRONSIDE_RAM_NS_DATA_END,
	     "sram_ns must end where the APP-XRAM window starts");
