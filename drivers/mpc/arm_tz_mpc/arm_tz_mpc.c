/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/mpc/mpc.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <stdint.h>

/* ARM TZ-MPC register offsets */
#define MPC_BLK_CFG 0x014U
#define MPC_BLK_IDX 0x018U
#define MPC_BLK_LUT 0x01CU

static uint32_t mpc_rd(uintptr_t base, uint32_t off)
{
	return *(volatile uint32_t *)(base + off);
}

static void mpc_wr(uintptr_t base, uint32_t off, uint32_t val)
{
	*(volatile uint32_t *)(base + off) = val;
}

static void mpc_configure_ns_range(uintptr_t base, uint32_t offset, uint32_t size)
{
	uint32_t blk_cfg = mpc_rd(base, MPC_BLK_CFG);
	uint32_t blk_size = 1U << (blk_cfg + 5U);
	uint32_t first_blk = offset / blk_size;
	uint32_t last_blk = (offset + size - 1U) / blk_size;
	uint32_t first_w = first_blk / 32U;
	uint32_t last_w = last_blk / 32U;

	for (uint32_t w = first_w; w <= last_w; w++) {
		mpc_wr(base, MPC_BLK_IDX, w);
		mpc_wr(base, MPC_BLK_LUT, 0xFFFFFFFFU);
	}
}

/*
 * For each child region node, program its (offset, size) range as NS in the
 * block-LUT of the parent MPC.  mpc_base is threaded via VARGS so we avoid
 * DT_PARENT inside the macro.
 */
#define MPC_CONFIGURE_REGION(region_id, mpc_base)                                                  \
	mpc_configure_ns_range((mpc_base), DT_REG_ADDR(region_id), DT_REG_SIZE(region_id));

#define MPC_CONFIGURE_NODE(node_id)                                                                \
	{                                                                                          \
		uintptr_t _base = (uintptr_t)DT_REG_ADDR(node_id);                                 \
		DT_FOREACH_CHILD_VARGS(node_id, MPC_CONFIGURE_REGION, _base)                       \
	}

void mpc_configure_all(void)
{
	DT_FOREACH_STATUS_OKAY(arm_tz_mpc, MPC_CONFIGURE_NODE)
}
