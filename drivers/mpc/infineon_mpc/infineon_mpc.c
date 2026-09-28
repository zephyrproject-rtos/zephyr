/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/mpc/mpc.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>

LOG_MODULE_REGISTER(infineon_mpc);

/*
 * Infineon Memory Protection Controller (MPC) driver.
 *
 * The MPC has two block lookup tables, indexed by block (block size read from
 * BLK_CFG):
 *   - ROT LUT: per protection context, 8 blocks/word, a 4-bit (NS,R,W) nibble
 *     per block; programmed from each region's pc-configs.
 *   - bus LUT: 32 blocks/word, one Non-Secure bit per block; marks a block
 *     Non-Secure for the bus-level security check.
 */
#define MPC_CFG         0x000U
#define MPC_CTRL        0x100U /* [8] AUTO_INC: BLK_IDX increments on BLK_LUT access */
#define MPC_BLK_CFG     0x108U /* [3:0] block size = 1 << (field + 5) */
#define MPC_BLK_IDX     0x10CU /* index of the 32-block group in BLK_LUT */
#define MPC_BLK_LUT     0x110U /* NS bit x 32 blocks (non-ROT bus LUT)   */
#define MPC_ROT_CTRL    0x200U /* [8] AUTO_INC: ROT_BLK_IDX increments on ROT_BLK_LUT access */
#define MPC_ROT_BLK_CFG 0x20CU /* [31] INIT_IN_PROGRESS */
#define MPC_ROT_BLK_IDX 0x210U /* index of the 8-block group in ROT_BLK_LUT */
#define MPC_ROT_BLK_PC  0x214U /* protection context of the 8-block group   */
#define MPC_ROT_BLK_LUT 0x218U /* (NS,R,W) nibble x 8 blocks for ROT_BLK_PC */

#define MPC_CFG_RESPONSE          BIT(4)
#define MPC_CTRL_AUTO_INC         BIT(8)
#define MPC_BLK_CFG_BLOCK_SIZE    0xFU
#define MPC_ROT_BLK_CFG_INIT_PROG BIT(31)

/* ROT_BLK_LUT per-block nibble: bit0 = NS, bit1 = R, bit2 = W. */
#define MPC_NIBBLE_NS BIT(0)
#define MPC_NIBBLE_R  BIT(1)
#define MPC_NIBBLE_W  BIT(2)

#define MPC_ROT_BLOCKS_PER_LUT 8U
#define MPC_BUS_BLOCKS_PER_LUT 32U

/* pc-configs access field: 0 = none, 1 = R, 2 = W, 3 = RW. */
static inline uint32_t infineon_mpc_nibble(uint32_t secure, uint32_t access)
{
	uint32_t nib = 0U;

	if ((access & 1U) != 0U) {
		nib |= MPC_NIBBLE_R;
	}
	if ((access & 2U) != 0U) {
		nib |= MPC_NIBBLE_W;
	}
	if (secure != 0U) {
		nib |= MPC_NIBBLE_NS; /* secure = 1 means Non-Secure */
	}

	return nib;
}

static inline uint32_t infineon_mpc_block_size(uintptr_t base)
{
	return 1U << ((sys_read32(base + MPC_BLK_CFG) & MPC_BLK_CFG_BLOCK_SIZE) + 5U);
}

/*
 * Assign the ROT_BLK_LUT nibble @p nib to every block of [offset, offset+size)
 * for protection context @p pc. ROT_BLK_LUT holds MPC_ROT_BLOCKS_PER_LUT
 * blocks selected by ROT_BLK_IDX; walk the range group-by-group with a
 * read-modify-write so untouched blocks in a shared group keep their bits.
 */
static void infineon_mpc_config_rot(uintptr_t base, uint32_t offset, uint32_t size, uint32_t pc,
				    uint32_t nib)
{
	uint32_t block = infineon_mpc_block_size(base);
	uint32_t blk = offset / block;
	uint32_t last = (offset + size) / block; /* exclusive */

	/* Clear AUTO_INC so the read-modify-writes below keep ROT_BLK_IDX put. */
	sys_write32(sys_read32(base + MPC_ROT_CTRL) & ~MPC_CTRL_AUTO_INC, base + MPC_ROT_CTRL);

	while ((sys_read32(base + MPC_ROT_BLK_CFG) & MPC_ROT_BLK_CFG_INIT_PROG) != 0U) {
	}

	sys_write32(pc, base + MPC_ROT_BLK_PC);

	while (blk < last) {
		uint32_t group = blk / MPC_ROT_BLOCKS_PER_LUT;
		uint32_t word_mask = 0U;
		uint32_t word_val = 0U;

		while (blk < last && (blk / MPC_ROT_BLOCKS_PER_LUT) == group) {
			uint32_t shift = 4U * (blk % MPC_ROT_BLOCKS_PER_LUT);

			word_mask |= 0xFU << shift;
			word_val |= nib << shift;
			blk++;
		}

		sys_write32(group, base + MPC_ROT_BLK_IDX);
		sys_write32((sys_read32(base + MPC_ROT_BLK_LUT) & ~word_mask) | word_val,
			    base + MPC_ROT_BLK_LUT);
	}
}

/* Mark [offset, offset+size) Non-Secure in the non-ROT bus LUT (1 bit/block). */
static __maybe_unused void infineon_mpc_config_bus_ns(uintptr_t base, uint32_t offset,
						      uint32_t size)
{
	uint32_t block = infineon_mpc_block_size(base);
	uint32_t blk = offset / block;
	uint32_t last = (offset + size) / block; /* exclusive */

	/* Clear AUTO_INC so the read-modify-writes below keep BLK_IDX put. */
	sys_write32(sys_read32(base + MPC_CTRL) & ~MPC_CTRL_AUTO_INC, base + MPC_CTRL);

	while (blk < last) {
		uint32_t group = blk / MPC_BUS_BLOCKS_PER_LUT;
		uint32_t word = 0U;

		while (blk < last && (blk / MPC_BUS_BLOCKS_PER_LUT) == group) {
			word |= BIT(blk % MPC_BUS_BLOCKS_PER_LUT);
			blk++;
		}

		sys_write32(group, base + MPC_BLK_IDX);
		sys_write32(sys_read32(base + MPC_BLK_LUT) | word, base + MPC_BLK_LUT);
	}
}

/*
 * clang-format off: the node/region config macros use COND_CODE_0()/IF_ENABLED()
 * and expand without statement separators under DT_FOREACH_*, so they are brace
 * blocks (concatenable) rather than do/while; keep the hand alignment.
 */
/* clang-format off */

/*
 * Configure one region node: apply its (pc, secure, access) triples to the ROT
 * LUT, and, if bus-nonsecure, mark the range NS in the bus LUT. pc-configs is
 * a flat <pc secure access> list threaded via DT_FOREACH_CHILD_VARGS so we
 * avoid DT_PARENT in the macro.
 */
#define MPC_CONFIGURE_REGION(region_id, mpc_base)                             \
	{                                                                      \
		static const uint32_t _pcs[] = DT_PROP(region_id, pc_configs); \
		BUILD_ASSERT(ARRAY_SIZE(_pcs) % 3U == 0U,                     \
			"pc-configs must be <pc secure access> triples");     \
		uint32_t _off = DT_REG_ADDR(region_id);                        \
		uint32_t _sz = DT_REG_SIZE(region_id);                         \
		for (size_t _i = 0; _i < ARRAY_SIZE(_pcs); _i += 3) {          \
			infineon_mpc_config_rot((mpc_base), _off, _sz, _pcs[_i],  \
					     infineon_mpc_nibble(_pcs[_i + 1],    \
							      _pcs[_i + 2]));  \
		}                                                              \
		IF_ENABLED(DT_PROP(region_id, bus_nonsecure),                 \
			   (infineon_mpc_config_bus_ns((mpc_base), _off, _sz);))  \
	}

#define MPC_CONFIGURE_NODE(mpc_id)                                             \
	{                                                                      \
		uintptr_t _base = (uintptr_t)DT_REG_ADDR(mpc_id);              \
		if (DT_ENUM_IDX(mpc_id, violation_response) == 1) {            \
			sys_write32(sys_read32(_base + MPC_CFG) |              \
					    MPC_CFG_RESPONSE, _base + MPC_CFG);\
		}                                                              \
		COND_CODE_0(DT_PROP(mpc_id, skip_auto_config), (              \
			DT_FOREACH_CHILD_VARGS(mpc_id, MPC_CONFIGURE_REGION,   \
					       _base)), ())                   \
	}

/* clang-format on */

void mpc_configure_all(void)
{
	DT_FOREACH_STATUS_OKAY(infineon_mpc, MPC_CONFIGURE_NODE)
}
