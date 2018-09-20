/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CACHE_H__
#define __CACHE_H__

#define set_bits(value, addr)	\
	(*(volatile unsigned long *)(addr)) |= (value)
#define clr_bits(value, addr)	\
	(*(volatile unsigned long *)(addr)) &= ~(value)

#define SIZE_PER_BLOCK			(0x10000000)
#define CMD_INVALID_RANGE		0x800000005
#define CMD_CLR_AND_INVALID_RANGE	0x800000009
#define BIT_CACHE_SZIE_OFFSET		(28)
/* register base addresses */

#define REG_ICACHE_BASE		(0x401C0000)
#define REG_DCACHE_BASE		(0x401E0000)

/*
 * cache bus configuration : enable/disable
 * protect enable/disable.
 */
#define REG_ICACHE_BUS_CFG0			(REG_ICACHE_BASE + 0x0000)
#define REG_DCACHE_BUS_CFG0			(REG_DCACHE_BASE + 0x0000)
#define BLOCK_0_CACHE_ENABLE_BIT	BIT(0)
#define BLOCK_1_CACHE_ENABLE_BIT	BIT(1)
#define BLOCK_2_CACHE_ENABLE_BIT	BIT(2)
#define BLOCK_3_CACHE_ENABLE_BIT	BIT(3)
#define BLOCK_4_CACHE_ENABLE_BIT	BIT(4)
#define BLOCK_5_CACHE_ENABLE_BIT	BIT(5)
#define BLOCK_6_CACHE_ENABLE_BIT	BIT(6)
#define BLOCK_7_CACHE_ENABLE_BIT	BIT(7)
/* reserved */
#define BLOCK_0_PROTECT_ENABLE_BIT	BIT(16)
#define BLOCK_1_PROTECT_ENABLE_BIT	BIT(17)
#define BLOCK_2_PROTECT_ENABLE_BIT	BIT(18)
#define BLOCK_3_PROTECT_ENABLE_BIT	BIT(19)
#define BLOCK_4_PROTECT_ENABLE_BIT	BIT(20)
#define BLOCK_5_PROTECT_ENABLE_BIT	BIT(21)
#define BLOCK_6_PROTECT_ENABLE_BIT	BIT(22)
#define BLOCK_7_PROTECT_ENABLE_BIT	BIT(23)

/* cache bus configuration : block 1 start addr. */
#define REG_ICACHE_BUS_CFG1		(REG_ICACHE_BASE + 0x0004)
#define REG_DCACHE_BUS_CFG1		(REG_DCACHE_BASE + 0x0004)
/* reserved 0-11, 28-31 */
#define BLOCK_1_START_ADDR_MASK		(0x0FFFF000)

// cache bus configuration : block 2 start addr.
#define REG_ICACHE_BUS_CFG2		(REG_ICACHE_BASE + 0x0008)
#define REG_DCACHE_BUS_CFG2		(REG_DCACHE_BASE + 0x0008)
/* reserved 0-11, 28-31 */
#define BLOCK_2_START_ADDR_MASK		(0x0FFFF000)

// cache bus configuration : block 3 start addr.
#define REG_ICACHE_BUS_CFG3		(REG_ICACHE_BASE + 0x000C)
#define REG_DCACHE_BUS_CFG3		(REG_DCACHE_BASE + 0x000C)
/* reserved 0-11, 28-31 */
#define BLOCK_3_START_ADDR_MASK		(0x0FFFF000)

// cache bus configuration : block 4 start addr.
#define REG_ICACHE_BUS_CFG4		(REG_ICACHE_BASE + 0x0010)
#define REG_DCACHE_BUS_CFG4		(REG_DCACHE_BASE + 0x0010)
/* reserved 0-11, 28-31 */
#define BLOCK_4_START_ADDR_MASK		(0x0FFFF000)

// cache bus configuration : block 5 start addr.
#define REG_ICACHE_BUS_CFG5		(REG_ICACHE_BASE + 0x0014)
#define REG_DCACHE_BUS_CFG5		(REG_DCACHE_BASE + 0x0014)
/* reserved 0-11, 28-31 */
#define BLOCK_5_START_ADDR_MASK		(0x0FFFF000)

// cache bus configuration : block 6 start addr.
#define REG_ICACHE_BUS_CFG6		(REG_ICACHE_BASE + 0x0018)
#define REG_DCACHE_BUS_CFG6		(REG_DCACHE_BASE + 0x0018)
/* reserved 0-11, 28-31 */
#define BLOCK_6_START_ADDR_MASK		(0x0FFFF000)

// cache bus configuration : block 7 start addr.
#define REG_ICACHE_BUS_CFG7		(REG_ICACHE_BASE + 0x001C)
#define REG_DCACHE_BUS_CFG7		(REG_DCACHE_BASE + 0x001C)
/* reserved 0-11, 28-31 */
#define BLOCK_7_START_ADDR_MASK		(0x0FFFF000)

// cache bus remap : block 0 start addr.
#define REG_ICACHE_BUS_REMAP0		(REG_ICACHE_BASE + 0x0020)
#define REG_DCACHE_BUS_REMAP0		(REG_DCACHE_BASE + 0x0020)
/* reserved 0-11 */
#define BLOCK_0_REMAP_OFFSET_MASK	(0xFFFFF000)

// cache bus remap : block 1 start addr.
#define REG_ICACHE_BUS_REMAP1		(REG_ICACHE_BASE + 0x0024)
#define REG_DCACHE_BUS_REMAP1		(REG_DCACHE_BASE + 0x0024)
/* reserved 0-11 */
#define BLOCK_1_REMAP_OFFSET_MASK	(0xFFFFF000)

// cache bus remap : block 2 start addr.
#define REG_ICACHE_BUS_REMAP2		(REG_ICACHE_BASE + 0x0028)
#define REG_DCACHE_BUS_REMAP2		(REG_DCACHE_BASE + 0x0028)
/* reserved 0-11 */
#define BLOCK_2_REMAP_OFFSET_MASK	(0xFFFFF000)

// cache bus remap : block 3 start addr.
#define REG_ICACHE_BUS_REMAP3		(REG_ICACHE_BASE + 0x002C)
#define REG_DCACHE_BUS_REMAP3		(REG_DCACHE_BASE + 0x002C)
/* reserved 0-11 */
#define BLOCK_3_REMAP_OFFSET_MASK	(0xFFFFF000)

// cache bus remap : block 4 start addr.
#define REG_ICACHE_BUS_REMAP4		(REG_ICACHE_BASE + 0x0030)
#define REG_DCACHE_BUS_REMAP4		(REG_DCACHE_BASE + 0x0030)
/* reserved 0-11 */
#define BLOCK_4_REMAP_OFFSET_MASK	(0xFFFFF000)

// cache bus remap : block 5 start addr.
#define REG_ICACHE_BUS_REMAP5		(REG_ICACHE_BASE + 0x0034)
#define REG_DCACHE_BUS_REMAP5		(REG_DCACHE_BASE + 0x0034)
/* reserved 0-11 */
#define BLOCK_5_REMAP_OFFSET_MASK	(0xFFFF0000)

// cache bus remap : block 6 start addr.
#define REG_ICACHE_BUS_REMAP6		(REG_ICACHE_BASE + 0x0038)
#define REG_DCACHE_BUS_REMAP6		(REG_DCACHE_BASE + 0x0038)
/* reserved 0-11 */
#define BLOCK_6_REMAP_OFFSET_MASK	(0xFFFFF000)

// cache bus remap : block 7 start addr.
#define REG_ICACHE_BUS_REMAP7		(REG_ICACHE_BASE + 0x003C)
#define REG_DCACHE_BUS_REMAP7		(REG_DCACHE_BASE + 0x003C)
/* reserved 0-11 */
#define BLOCK_7_REMAP_OFFSET_MASK	(0xFFFFF000)

// cache configuration.
#define REG_ICACHE_CFG0			(REG_ICACHE_BASE + 0x0040)
#define REG_DCACHE_CFG0			(REG_DCACHE_BASE + 0x0040)
#define CACHE_SIZE_SEL_MASK		(0x30000000)
#define CACHE_WRITE_MODE_MASK		(0x3)
#define CACHE_WRITE_THROUGH		(0x0)
#define CACHE_WRITE_BACK_NO_ALLOCATE	(0x1)
#define CACHE_WRITE_BACK_ALLOCATE	(0x2)
#define CACHE_WOKE_MODE_MASK		(0x80000000)
#define CACHE_DEBUG_EN			(0x80000000)

// cache bus status.
#define REG_ICACHE_BUS_STS0		(REG_ICACHE_BASE + 0x0044)
#define	REG_DCACHE_BUS_STS0		(REG_DCACHE_BASE + 0x0044)
#define	CACHE_BUS_STS0_MASK		(0x0FFFFFFF)

#define REG_ICACHE_CACHE_STS0		(REG_ICACHE_BASE + 0x0048)
#define REG_DCACHE_CACHE_STS0		(REG_DCACHE_BASE + 0x0048)

// cache software command control 0.
#define REG_ICACHE_CMD_CFG0		(REG_ICACHE_BASE + 0x0050)
#define REG_DCACHE_CMD_CFG0		(REG_DCACHE_BASE + 0x0050)
#define CACHE_STR_ADDR_MASK		(0x03FFFFFF)

// cache software command control 1.
#define REG_ICACHE_CMD_CFG1		(REG_ICACHE_BASE + 0x0054)
#define REG_DCACHE_CMD_CFG1		(REG_DCACHE_BASE + 0x0054)
#define CACHE_END_ADDR_MASK		(0x03FFFFFF)

// cache sofrware command control 2.
#define REG_ICACHE_CMD_CFG2		(REG_ICACHE_BASE + 0x0058)
#define REG_DCACHE_CMD_CFG2		(REG_DCACHE_BASE + 0x0058)
#define CACHE_CMD_CFG2_MASK		(0x8000003F)
#define CACHE_CMD_CLEAN_ALL		(0x0)
#define CACHE_CMD_CLEAN_RANGE		(0x01)
#define CACHE_CMD_CLEAN_ENTRY		(0x02)
#define CACHE_CMD_INVALID_ALL		(0x04)
#define CACHE_CMD_INVALID_RANGE		(0x05)
#define CACHE_CMD_INVALID_ENTRY		(0x06)
#define CACHE_CMD_CLEAN_INVALID_ALL	(0x08)
#define CACHE_CMD_CLEAN_INVALID_RANGE	(0x09)
#define CACHE_CMD_CLEAN_INVALID_ENTRY	(0x0A)
#define CACHE_CMD_ISSUE_START		BIT(31)

// cache interrupt enable
#define REG_ICACHE_INT_EN		(REG_ICACHE_BASE + 0x0060)
#define REG_DCACHE_INT_EN		(REG_DCACHE_BASE + 0x0060)
#define CACHE_CMD_IRQ_EN		BIT(0)
#define CACHE_PROT_IRQ_EN		BIT(1)

// cache interrupt raw status.
#define REG_ICACHE_INT_RAW_STS		(REG_ICACHE_BASE + 0x0064)
#define REG_DCACHE_INT_RAW_STS		(REG_DCACHE_BASE + 0x0064)
#define CACHE_CMD_IRQ_RAW		BIT(0)
#define CACHE_PROT_IRQ_RAW		BIT(1)

// cache interrupt masked status.
#define REG_ICACHE_INT_MSK_STS		(REG_ICACHE_BASE + 0x0068)
#define REG_DCACHE_INT_MSK_STS		(REG_DCACHE_BASE + 0x0068)
#define CACHE_CMD_IRQ_MASK		BIT(0)
#define CACHE_PROT_IRQ_MASK		BIT(1)

// cache interrupt clear
#define REG_ICACHE_INT_CLR		(REG_ICACHE_BASE + 0x006C)
#define REG_DCACHE_INT_CLR		(REG_DCACHE_BASE + 0x006C)
#define CACHE_CMD_IRQ_CLR		BIT(0)
#define CACHE_PROT_IRQ_CLR		BIT(1)

// cache HIT/MISS COUNT
#define REG_ICACHE_WRITE_HIT_CNT	(REG_ICACHE_BASE + 0x00070)
#define REG_DCACHE_WRITE_HIT_CNT	(REG_DCACHE_BASE + 0x00070)

#define REG_ICACHE_WRITE_MISS_CNT	(REG_ICACHE_BASE + 0x00074)
#define REG_DCACHE_WRITE_MISS_CNT	(REG_DCACHE_BASE + 0x00074)

#define REG_ICACHE_READ_HIT_CNT		(REG_ICACHE_BASE + 0x00078)
#define REG_DCACHE_READ_HIT_CNT		(REG_DCACHE_BASE + 0x00078)

#define REG_ICACHE_READ_MISS_CNT	(REG_ICACHE_BASE + 0x0007C)
#define REG_DCACHE_READ_MISS_CNT	(REG_DCACHE_BASE + 0x0007C)

/************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif
	typedef unsigned char BOOLEAN;

	typedef enum {
		BLOCK_0,
		BLOCK_1,
		BLOCK_2,
		BLOCK_3,
		BLOCK_4,
		BLOCK_5,
		BLOCK_6,
		BLOCK_7,
		BLOCK_MAX
	} CACHE_BLOCK_E;

	typedef struct {
		CACHE_BLOCK_E id;
		uint32_t start;
		BOOLEAN en_cache;
		BOOLEAN en_prot;
	} CACHE_BLOCK_OP_T;

	typedef enum {
		C_ALLAREA = 0X0,
		C_RANGE = 0X1,
		C_ENTRY = 0X2,
		C_AREA_MASK = (C_ALLAREA | C_RANGE | C_ENTRY),

		C_CLEAN = 0X0,
		C_INVALID = 0X4,
		C_CINVALID = 0X8,
		C_OP_MASK = (C_CLEAN | C_INVALID | C_CINVALID)
	} CACHE_CMD_AUX;

	typedef enum {
		C_CLEAN_ALL = (C_CLEAN | C_ALLAREA),
		C_CLEAN_RANGE = (C_CLEAN | C_RANGE),
		C_CLEAN_ENTRY = (C_CLEAN | C_ENTRY),
		C_INVALID_ALL = (C_INVALID | C_ALLAREA),
		C_INVALID_RANGE = (C_INVALID | C_RANGE),
		C_INVALID_ENTRY = (C_INVALID | C_ENTRY),
		C_CLEAN_INVALID_ALL = (C_CINVALID | C_ALLAREA),
		C_CLEAN_INVALID_RANGE = (C_CINVALID | C_RANGE),
		C_CLEAN_INVALID_ENTRY = (C_CINVALID | C_ENTRY)
	} CMD_TYPE_E;

	typedef struct {
		CMD_TYPE_E type;
		uint32_t start;
		uint32_t end;
	} CACHE_CMD_T;

	typedef enum {
		C_READ,
		C_WRITE
	} cache_op;

	typedef enum {
		CACHE_4K = 0x0,
		CACHE_8K = 0x1,
		CACHE_16K = 0x2,
		CACHE_32K = 0x3,
	} CACHE_SIZE_SEL_E;

	void uwp_cache_init(void);
	void icache_dcache_enable_block_hal(void);
	void icache_dcache_disable_block_hal(void);

#ifdef __cplusplus
}				/* end extern "C" */
#endif

#endif				/* __CACHE_H__ */
