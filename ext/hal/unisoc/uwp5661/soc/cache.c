#include <zephyr.h>
#include <uwp_hal.h>
#include "cache.h"


static CACHE_BLOCK_OP_T C_ICACHE_DEFAULT_BLOCK_CFG[] = {
	{ BLOCK_0, 0x00000000, FALSE, FALSE },
	{ BLOCK_1, 0x00100000, TRUE, FALSE },
	{ BLOCK_2, 0x001EE000, FALSE, FALSE },
	{ BLOCK_3, 0x02000000, TRUE, FALSE },
	{ BLOCK_4, 0x02480000, FALSE, FALSE },
	{ BLOCK_5, 0x02890000, FALSE, FALSE },
	{ BLOCK_6, 0x028A0000, FALSE, FALSE },
	{ BLOCK_7, 0x028B0000, FALSE, FALSE },
};

static CACHE_BLOCK_OP_T C_DCACHE_DEFAULT_BLOCK_CFG[] = {
	{ BLOCK_0, 0x00000000, FALSE, FALSE },
	{ BLOCK_1, 0x00100000, TRUE, FALSE },
	{ BLOCK_2, 0x001EE000, FALSE, FALSE },
	{ BLOCK_3, 0x02000000, TRUE, FALSE },
	{ BLOCK_4, 0x02480000, FALSE, FALSE },
	{ BLOCK_5, 0x02890000, FALSE, FALSE },
	{ BLOCK_6, 0x028A0000, FALSE, FALSE },
	{ BLOCK_7, 0x028B0000, FALSE, FALSE },
};

static u32_t icache_bus_cfg_addr[] = {
	REG_ICACHE_BUS_CFG0,
	REG_ICACHE_BUS_CFG1,
	REG_ICACHE_BUS_CFG2,
	REG_ICACHE_BUS_CFG3,
	REG_ICACHE_BUS_CFG4,
	REG_ICACHE_BUS_CFG5,
	REG_ICACHE_BUS_CFG6,
	REG_ICACHE_BUS_CFG7,
};

static u32_t dcache_bus_cfg_addr[] = {
	REG_DCACHE_BUS_CFG0,
	REG_DCACHE_BUS_CFG1,
	REG_DCACHE_BUS_CFG2,
	REG_DCACHE_BUS_CFG3,
	REG_DCACHE_BUS_CFG4,
	REG_DCACHE_BUS_CFG5,
	REG_DCACHE_BUS_CFG6,
	REG_DCACHE_BUS_CFG7,
};

static u32_t icache_bus_remap_addr[] = {
	REG_ICACHE_BUS_REMAP0,
	REG_ICACHE_BUS_REMAP1,
	REG_ICACHE_BUS_REMAP2,
	REG_ICACHE_BUS_REMAP3,
	REG_ICACHE_BUS_REMAP4,
	REG_ICACHE_BUS_REMAP5,
	REG_ICACHE_BUS_REMAP6,
	REG_ICACHE_BUS_REMAP7,
};

static u32_t dcache_bus_remap_addr[] = {
	REG_DCACHE_BUS_REMAP0,
	REG_DCACHE_BUS_REMAP1,
	REG_DCACHE_BUS_REMAP2,
	REG_DCACHE_BUS_REMAP3,
	REG_DCACHE_BUS_REMAP4,
	REG_DCACHE_BUS_REMAP5,
	REG_DCACHE_BUS_REMAP6,
	REG_DCACHE_BUS_REMAP7,
};

u32_t *cache_bus_cfg_addr = &dcache_bus_cfg_addr[0];
u32_t *cache_bus_remap_addr = &dcache_bus_remap_addr[0];
CACHE_BLOCK_OP_T *block_cfg = NULL;
u32_t CACHE_BUS_STS0, CACHE_CFG0, CACHE_CMD_CFG0,
      CACHE_CMD_CFG1, CACHE_CMD_CFG2, CACHE_INT_CLR,
      CACHE_INT_RAW_STS, CACHE_INT_EN, CACHE_INT_MSK_STS,
      CACHE_WRITE_MISS_CNT, CACHE_WRITE_HIT_CNT, CACHE_READ_MISS_CNT,
      CACHE_READ_HIT_CNT;

void cache_waiting_idle(void)
{
	int32_t cnt = 0;

	while ((sci_read32(REG_ICACHE_CACHE_STS0) | sci_read32(REG_DCACHE_CACHE_STS0))) {
		if ((cnt++) > 1000) {
			SCI_ASSERT(0);
		}
	}

	return;
}

void icache_enable_block(CACHE_BLOCK_E block_num)
{
	set_bits((0x1 << block_num), REG_ICACHE_BASE);
}

void icache_disable_block(CACHE_BLOCK_E block_num)
{
	clr_bits((0x1 << block_num), REG_ICACHE_BASE);
}

void dcache_enable_block(CACHE_BLOCK_E block_num)
{
	u32_t t = 0;

	sci_write32(block_cfg[block_num].start, REG_DCACHE_CMD_CFG0);
	sci_write32(block_cfg[block_num + 1].start, REG_DCACHE_CMD_CFG1);
	set_bits(CMD_INVALID_RANGE, REG_DCACHE_CMD_CFG2);

	while (0 == (sci_read32(REG_DCACHE_INT_RAW_STS) & CACHE_CMD_IRQ_RAW)) {
		if (t++ > 100000) {
			break;
		}
	}
	set_bits(CACHE_CMD_IRQ_CLR, REG_DCACHE_INT_CLR);
	set_bits(0x1 << block_num, REG_DCACHE_BASE);
}

void dcache_disable_block(CACHE_BLOCK_E block_num)
{
	u32_t t = 0;

	sci_write32(C_DCACHE_DEFAULT_BLOCK_CFG[block_num].start, REG_DCACHE_CMD_CFG0);
	sci_write32(C_DCACHE_DEFAULT_BLOCK_CFG[block_num + 1].start,
		    REG_DCACHE_CMD_CFG1);

	set_bits(CMD_CLR_AND_INVALID_RANGE, REG_DCACHE_CMD_CFG2);
	while (0 == (sci_read32(REG_DCACHE_INT_RAW_STS) & CACHE_CMD_IRQ_RAW)) {
		if (t++ > 100000) {
			break;
		}
	}
	set_bits(CACHE_CMD_IRQ_CLR, REG_DCACHE_INT_CLR);

	clr_bits(0x1 << block_num, REG_DCACHE_BASE);

}

static inline void cache_cmd_entry(u32_t addr)
{
	sci_write32(addr & CACHE_STR_ADDR_MASK, CACHE_CMD_CFG0);
}

static inline void cache_cmd_range(u32_t start, u32_t end)
{
	sci_write32(start & CACHE_STR_ADDR_MASK, CACHE_CMD_CFG0);
	sci_write32(end & CACHE_END_ADDR_MASK, CACHE_CMD_CFG1);
}

u32_t cache_check(CACHE_CMD_T *cmd)
{
	u32_t ret = 0;
	u32_t size = BLOCK_MAX;
	u32_t area_masked = C_AREA_MASK & cmd->type;
	u32_t cache_op_masked = C_OP_MASK & cmd->type;

	switch (area_masked) {
	case C_ALLAREA:
		for (; size > 0; size--) {
			if (block_cfg[size - 1].en_cache == TRUE) {
				if (C_INVALID == cache_op_masked) {
					break;
				} else if (FALSE == block_cfg[size - 1].en_prot) {
					break;
				}
			}
		}

		if (size == 0) {
			ret = 1;
		}
		break;
	case C_RANGE:
		if (cmd->start >= cmd->end) {
			ret = 1;
			break;
		}
		for (; size > 0; size--) {
			if (block_cfg[size - 1].en_cache == TRUE) {
				if ((cmd->start >= block_cfg[size - 1].start)
				    && (cmd->end < (block_cfg[size - 1].start
						    + SIZE_PER_BLOCK))) {
					if (cache_op_masked == C_INVALID) {
						break;
					} else if (block_cfg[size - 1].en_prot ==
						   FALSE) {
						break;
					}
				}
			}
		}
		if (size == 0) {
			ret = 1;
		}
		break;
	case C_ENTRY:
		for (; size > 0; size--) {
			if (block_cfg[size - 1].en_cache == TRUE) {
				if ((cmd->start >= block_cfg[size - 1].start)
				    && (cmd->end < block_cfg[size - 1].start +
					SIZE_PER_BLOCK)) {
					if (cache_op_masked == C_INVALID) {
						break;
					} else if (block_cfg
						   [size - 1].en_prot == FALSE) {
						break;
					}
				}
			}
		}
		if (size == 0) {
			ret = 1;
		}
		break;
	default:
		ret = 1;
		break;
	}
	return ret;
}

u32_t cache_execmd(CACHE_CMD_T *cmd, u32_t force)
{
	u32_t value = CACHE_CMD_ISSUE_START;
	u32_t ret = 0;


	if (!force) {
		ret = cache_check(cmd);
	}
	set_bits(CACHE_CMD_IRQ_CLR, CACHE_INT_CLR);
	if (0 == ret) {
		switch (cmd->type) {
		case C_CLEAN_ENTRY:
			cache_cmd_entry(cmd->start);
			value |= (CACHE_CMD_CLEAN_ENTRY);
			break;
		case C_INVALID_ENTRY:
			cache_cmd_entry(cmd->start);
			value |= (CACHE_CMD_INVALID_ENTRY);
			break;
		case C_INVALID_RANGE:
			cache_cmd_range(cmd->start, cmd->end);
			value |= (CACHE_CMD_INVALID_RANGE);
			break;
		case C_CLEAN_RANGE:
			cache_cmd_range(cmd->start, cmd->end);
			value |= (CACHE_CMD_CLEAN_RANGE);
			break;
		case C_CLEAN_INVALID_ALL:
			value |= (CACHE_CMD_CLEAN_INVALID_ALL);
			break;
		case C_CLEAN_INVALID_RANGE:
			cache_cmd_range(cmd->start, cmd->end);
			value |= (CACHE_CMD_CLEAN_INVALID_RANGE);
			break;
		case C_CLEAN_INVALID_ENTRY:
			cache_cmd_entry(cmd->start);
			value |= (CACHE_CMD_CLEAN_INVALID_ENTRY);
			break;
		case C_CLEAN_ALL:
			value |= (CACHE_CMD_CLEAN_ALL);
			break;
		case C_INVALID_ALL:
			value |= (CACHE_CMD_INVALID_ALL);
			break;
		default:
			ret = 1;
			break;
		}
	}

#if 0
	if (ret == 0) {
		sci_write32(value & CACHE_CMD_CFG2_MASK, CACHE_CMD_CFG2);
		while (0 == (sci_read32(CACHE_INT_RAW_STS) & CACHE_CMD_IRQ_RAW)) {
			if (t++ > 100000) {
				printk("%s %d.\n", __func__, __LINE__);
				break;
			}
		}
		set_bits(CACHE_CMD_IRQ_CLR, CACHE_INT_CLR);
	}
#endif

	return ret;
}

u32_t cache_enableblock(CACHE_BLOCK_OP_T *pblock, cache_op op)
{
	u32_t enable_bit = pblock->id - (u32_t) (BLOCK_0);

	if (C_WRITE == op) {
		if (pblock->en_cache) {
			set_bits(0x1 << enable_bit, cache_bus_cfg_addr[0]);
			return TRUE;
		} else {
			CACHE_CMD_T c;
			c.type = C_CLEAN_INVALID_RANGE;
			c.start = pblock->start;
			if (pblock->id > BLOCK_MAX - 2) {
				c.end = CACHE_END_ADDR_MASK;
			} else {
				c.end = block_cfg[pblock->id + 1].start - 1;
			}
			cache_execmd(&c, TRUE);

			clr_bits(0x1 << enable_bit, cache_bus_cfg_addr[0]);
			return FALSE;
		}
	} else {
		if (sci_read32(cache_bus_cfg_addr[0]) & (0x1 << enable_bit)) {
			return TRUE;
		} else {
			return FALSE;
		}
	}
}

u32_t cache_protblock(CACHE_BLOCK_OP_T *pblock, cache_op op)
{
	u32_t enable_bit = pblock->id - (u32_t) (BLOCK_0) +16;

	if (C_WRITE == op) {
		if (pblock->en_prot) {
			set_bits(0x1 << enable_bit, cache_bus_cfg_addr[0]);
			return TRUE;
		} else {
			clr_bits(0x1 << enable_bit, cache_bus_cfg_addr[0]);
			return FALSE;
		}
	} else {
		if (sci_read32(cache_bus_cfg_addr[0]) & (0x1 << enable_bit)) {
			return TRUE;
		} else {
			return FALSE;
		}
	}
}

void cache_configaddr(CACHE_BLOCK_OP_T *pblock)
{
	if (pblock->id < BLOCK_1 || pblock->id > BLOCK_7) {
		return;
	}
	pblock->start = pblock->start & BLOCK_1_START_ADDR_MASK;
	sci_write32(pblock->start,
		    cache_bus_cfg_addr[pblock->id - (u32_t) BLOCK_0]);
}

void cache_size_sel(CACHE_SIZE_SEL_E cache_size)
{
	clr_bits(CACHE_DEBUG_EN, CACHE_CFG0);
	set_bits(((cache_size) << BIT_CACHE_SZIE_OFFSET)
		 & CACHE_SIZE_SEL_MASK, CACHE_CFG0);
}

u32_t cache_execmd_invalid_clean_all(CACHE_CMD_T *cmd, u32_t force)
{
	u32_t value = CACHE_CMD_ISSUE_START;

	set_bits(CACHE_CMD_IRQ_CLR, CACHE_INT_CLR);
	if (C_CLEAN_ALL == cmd->type) {
		value |= (CACHE_CMD_CLEAN_ALL);
	} else if (C_INVALID_ALL == cmd->type) {
		value |= (CACHE_CMD_INVALID_ALL);
	} else {
		value |= (CACHE_CMD_CLEAN_INVALID_ALL);
	}

	sci_write32(value & CACHE_CMD_CFG2_MASK, CACHE_CMD_CFG2);

#if 0
	while (0 == (sci_read32(CACHE_INT_RAW_STS) & CACHE_CMD_IRQ_RAW)) {
		if (t++ > 100000) {
			break;
		}
	}
#endif

	set_bits(CACHE_CMD_IRQ_CLR, CACHE_INT_CLR);
	return 0;
}

void cache_set_thr_mode(void)
{
	clr_bits(CACHE_DEBUG_EN, CACHE_CFG0);
	clr_bits(CACHE_WRITE_MODE_MASK, CACHE_CFG0);
	set_bits(CACHE_WRITE_THROUGH, CACHE_CFG0);
}

void cache_set_wbk_allocate_mode(void)
{
	clr_bits(CACHE_DEBUG_EN, CACHE_CFG0);
	clr_bits(CACHE_WRITE_MODE_MASK, CACHE_CFG0);
	set_bits(CACHE_WRITE_BACK_ALLOCATE, CACHE_CFG0);
}

void icache_set_reg(void)
{
	CACHE_BUS_STS0 = REG_ICACHE_BUS_STS0;
	CACHE_CFG0 = REG_ICACHE_CFG0;
	CACHE_CMD_CFG0 = REG_ICACHE_CMD_CFG0;
	CACHE_CMD_CFG1 = REG_ICACHE_CMD_CFG1;
	CACHE_CMD_CFG2 = REG_ICACHE_CMD_CFG2;
	CACHE_INT_CLR = REG_ICACHE_INT_CLR;
	CACHE_INT_RAW_STS = REG_ICACHE_INT_RAW_STS;
	CACHE_INT_EN = REG_ICACHE_INT_EN;
	CACHE_INT_MSK_STS = REG_ICACHE_INT_MSK_STS;
	CACHE_WRITE_MISS_CNT = REG_ICACHE_WRITE_MISS_CNT;
	CACHE_WRITE_HIT_CNT = REG_ICACHE_WRITE_HIT_CNT;
	CACHE_READ_MISS_CNT = REG_ICACHE_READ_MISS_CNT;
	CACHE_READ_HIT_CNT = REG_ICACHE_READ_HIT_CNT;

	cache_bus_cfg_addr = &icache_bus_cfg_addr[0];
	cache_bus_remap_addr = &icache_bus_remap_addr[0];

	block_cfg = &C_ICACHE_DEFAULT_BLOCK_CFG[0];
}

void dcache_set_reg(void)
{
	CACHE_BUS_STS0 = REG_DCACHE_BUS_STS0;
	CACHE_CFG0 = REG_DCACHE_CFG0;
	CACHE_CMD_CFG0 = REG_DCACHE_CMD_CFG0;
	CACHE_CMD_CFG1 = REG_DCACHE_CMD_CFG1;
	CACHE_CMD_CFG2 = REG_DCACHE_CMD_CFG2;
	CACHE_INT_CLR = REG_DCACHE_INT_CLR;
	CACHE_INT_RAW_STS = REG_DCACHE_INT_RAW_STS;
	CACHE_INT_EN = REG_DCACHE_INT_EN;
	CACHE_INT_MSK_STS = REG_DCACHE_INT_MSK_STS;
	CACHE_WRITE_MISS_CNT = REG_DCACHE_WRITE_MISS_CNT;
	CACHE_WRITE_HIT_CNT = REG_DCACHE_WRITE_HIT_CNT;
	CACHE_READ_MISS_CNT = REG_DCACHE_READ_MISS_CNT;
	CACHE_READ_HIT_CNT = REG_DCACHE_READ_HIT_CNT;

	cache_bus_cfg_addr = &dcache_bus_cfg_addr[0];
	cache_bus_remap_addr = &dcache_bus_remap_addr[0];

	block_cfg = &C_DCACHE_DEFAULT_BLOCK_CFG[0];
}

void cache_execusecfg(CACHE_BLOCK_OP_T *pcfg, u32_t size)
{
	int32_t i;

	for (; size > 0; size--) {
		cache_configaddr(&pcfg[size - 1]);
		cache_protblock(&pcfg[size - 1], C_WRITE);
		for (i = 0; i < 20; ++i) ;
		cache_enableblock(&pcfg[size - 1], C_WRITE);
	}
}

void icache_phy_init(CACHE_SIZE_SEL_E icache_size)
{
	u32_t i;
	CACHE_CMD_T cmd;

	cmd.type = C_INVALID_ALL;

	icache_set_reg();
	cache_size_sel(icache_size);
	cache_execmd_invalid_clean_all(&cmd, TRUE);

	cache_set_thr_mode();
	cache_execusecfg(block_cfg, BLOCK_MAX);

	for (i = 0; i < 50; ++i) ;
}

void dcache_phy_init(CACHE_SIZE_SEL_E dcache_size)
{
	u32_t i;
	CACHE_CMD_T cmd;

	cmd.type = C_INVALID_ALL;
	dcache_set_reg();
	cache_size_sel(dcache_size);
	cache_execmd_invalid_clean_all(&cmd, TRUE);

	cache_set_wbk_allocate_mode();
	cache_execusecfg(block_cfg, BLOCK_MAX);

	for (i = 0; i < 50; ++i) ;
}

void icache_dcache_enable_block_hal(void)
{
	icache_enable_block(BLOCK_1);
	icache_enable_block(BLOCK_3);

	dcache_enable_block(BLOCK_1);
}

void icache_dcache_disable_block_hal(void)
{
	icache_disable_block(BLOCK_1);
	icache_disable_block(BLOCK_3);

	dcache_disable_block(BLOCK_1);
}

void uwp_cache_init(void)
{
	icache_phy_init(CACHE_32K);
	dcache_phy_init(CACHE_32K);
}
