/*
 * Copyright (c) 2022 ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>
#include <zephyr/drivers/syscon.h>

/*
 * cache area control: each bit controls 32KB cache area
 *	1: cacheable
 *	0: no-cache
 *
 *	bit[0]: 1st 32KB from 0x0000_0000 to 0x0000_7fff
 *	bit[1]: 2nd 32KB from 0x0000_8000 to 0x0000_ffff
 *	...
 *	bit[22]: 23th 32KB from 0x000a_8000 to 0x000a_ffff
 *	bit[23]: 24th 32KB from 0x000b_0000 to 0x000b_ffff
 */
#define CACHE_AREA_CTRL_REG	0xa50
#define CACHE_INVALID_REG	0xa54
#define CACHE_FUNC_CTRL_REG	0xa58

#define CACHED_SRAM_ADDR	CONFIG_SRAM_BASE_ADDRESS
#define CACHED_SRAM_SIZE	KB(CONFIG_SRAM_SIZE)
#define CACHED_SRAM_END		(CACHED_SRAM_ADDR + CACHED_SRAM_SIZE - 1)

#define CACHE_AREA_SIZE_LOG2	15
#define CACHE_AREA_SIZE		(1 << CACHE_AREA_SIZE_LOG2)

#define DCACHE_INVALID(addr)	(BIT(31) | ((addr & GENMASK(10, 0)) << 16))
#define ICACHE_INVALID(addr)	(BIT(15) | ((addr & GENMASK(10, 0)) << 0))

#define ICACHE_CLEAN		BIT(2)
#define DCACHE_CLEAN		BIT(1)
#define CACHE_EANABLE		BIT(0)

/* cache size = 32B * 128 = 4KB */
#define CACHE_LINE_SIZE_LOG2	5
#define CACHE_LINE_SIZE		(1 << CACHE_LINE_SIZE_LOG2)
#define N_CACHE_LINE		128
#define CACHE_ALIGNED_ADDR(addr) \
	((addr >> CACHE_LINE_SIZE_LOG2) << CACHE_LINE_SIZE_LOG2)

/* prefetch buffer */
#define PREFETCH_BUF_SIZE	CACHE_LINE_SIZE

static void aspeed_cache_init(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(syscon));
	uint32_t start_bit, end_bit, max_bit;

	/* set all cache areas to no-cache by default */
	syscon_write_reg(dev, CACHE_FUNC_CTRL_REG, 0);

	/* calculate how many areas need to be set */
	max_bit = 8 * sizeof(uint32_t) - 1;
	start_bit = MIN(max_bit, CACHED_SRAM_ADDR >> CACHE_AREA_SIZE_LOG2);
	end_bit = MIN(max_bit, CACHED_SRAM_END >> CACHE_AREA_SIZE_LOG2);
	syscon_write_reg(dev, CACHE_AREA_CTRL_REG, GENMASK(end_bit, start_bit));

	/* enable cache */
	syscon_write_reg(dev, CACHE_FUNC_CTRL_REG, CACHE_EANABLE);
}

/**
 * @brief get aligned address and the number of cachline to be invalied
 * @param [IN] addr - start address to be invalidated
 * @param [IN] size - size in byte
 * @param [OUT] p_aligned_addr - pointer to the cacheline aligned address variable
 * @return number of cacheline to be invalidated
 *
 *  * addr
 *   |--------size-------------|
 * |-----|-----|-----|-----|-----|
 *  \                             \
 *   head                          tail
 *
 * example 1:
 * addr = 0x100 (cacheline aligned), size = 64
 * then head = 0x100, number of cache line to be invalidated = 64 / 32 = 2
 * which means range [0x100, 0x140) will be invalidated
 *
 * example 2:
 * addr = 0x104 (cacheline unaligned), size = 64
 * then head = 0x100, number of cache line to be invalidated = 1 + 64 / 32 = 3
 * which means range [0x100, 0x160) will be invalidated
 */
static uint32_t get_n_cacheline(uint32_t addr, uint32_t size, uint32_t *p_head)
{
	uint32_t n = 0;
	uint32_t tail;

	/* head */
	*p_head = CACHE_ALIGNED_ADDR(addr);

	/* roundup the tail address */
	tail = addr + size + (CACHE_LINE_SIZE - 1);
	tail = CACHE_ALIGNED_ADDR(tail);

	n = (tail - *p_head) >> CACHE_LINE_SIZE_LOG2;

	return n;
}

void cache_data_enable(void)
{
	aspeed_cache_init();
}

void cache_data_disable(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(syscon));

	syscon_write_reg(dev, CACHE_FUNC_CTRL_REG, 0);
}

void cache_instr_enable(void)
{
	aspeed_cache_init();
}

void cache_instr_disable(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(syscon));

	syscon_write_reg(dev, CACHE_FUNC_CTRL_REG, 0);
}

int cache_data_all(int op)
{
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(syscon));
	uint32_t ctrl;
	unsigned int key = 0;

	ARG_UNUSED(op);
	syscon_read_reg(dev, CACHE_FUNC_CTRL_REG, &ctrl);

	/* enter critical section */
	if (!k_is_in_isr()) {
		key = irq_lock();
	}

	ctrl &= ~DCACHE_CLEAN;
	syscon_write_reg(dev, CACHE_FUNC_CTRL_REG, ctrl);

	__DSB();
	ctrl |= DCACHE_CLEAN;
	syscon_write_reg(dev, CACHE_FUNC_CTRL_REG, ctrl);
	__DSB();

	/* exit critical section */
	if (!k_is_in_isr()) {
		irq_unlock(key);
	}

	return 0;
}

int cache_data_range(void *addr, size_t size, int op)
{
	uint32_t aligned_addr, i, n;
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(syscon));
	unsigned int key = 0;

	ARG_UNUSED(op);

	if (((uint32_t)addr < CACHED_SRAM_ADDR) ||
	    ((uint32_t)addr > CACHED_SRAM_END)) {
		return 0;
	}

	/* enter critical section */
	if (!k_is_in_isr()) {
		key = irq_lock();
	}

	n = get_n_cacheline((uint32_t)addr, size, &aligned_addr);

	for (i = 0; i < n; i++) {
		syscon_write_reg(dev, CACHE_INVALID_REG, 0);
		syscon_write_reg(dev, CACHE_INVALID_REG, DCACHE_INVALID(aligned_addr));
		aligned_addr += CACHE_LINE_SIZE;
	}
	__DSB();

	/* exit critical section */
	if (!k_is_in_isr()) {
		irq_unlock(key);
	}

	return 0;
}

int cache_instr_all(int op)
{
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(syscon));
	uint32_t ctrl;
	unsigned int key = 0;

	ARG_UNUSED(op);

	syscon_read_reg(dev, CACHE_FUNC_CTRL_REG, &ctrl);

	/* enter critical section */
	if (!k_is_in_isr()) {
		key = irq_lock();
	}

	ctrl &= ~ICACHE_CLEAN;
	syscon_write_reg(dev, CACHE_FUNC_CTRL_REG, ctrl);
	__ISB();
	ctrl |= ICACHE_CLEAN;
	syscon_write_reg(dev, CACHE_FUNC_CTRL_REG, ctrl);
	__ISB();

	/* exit critical section */
	if (!k_is_in_isr()) {
		irq_unlock(key);
	}

	return 0;
}

int cache_instr_range(void *addr, size_t size, int op)
{
	uint32_t aligned_addr, i, n;
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(syscon));
	unsigned int key = 0;

	ARG_UNUSED(op);

	if (((uint32_t)addr < CACHED_SRAM_ADDR) ||
	    ((uint32_t)addr > CACHED_SRAM_END)) {
		return 0;
	}

	n = get_n_cacheline((uint32_t)addr, size, &aligned_addr);

	/* enter critical section */
	if (!k_is_in_isr()) {
		key = irq_lock();
	}

	for (i = 0; i < n; i++) {
		syscon_write_reg(dev, CACHE_INVALID_REG, 0);
		syscon_write_reg(dev, CACHE_INVALID_REG, ICACHE_INVALID(aligned_addr));
		aligned_addr += CACHE_LINE_SIZE;
	}
	__DSB();

	/* exit critical section */
	if (!k_is_in_isr()) {
		irq_unlock(key);
	}

	return 0;
}

#ifdef CONFIG_DCACHE_LINE_SIZE_DETECT
size_t cache_data_line_size_get(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(syscon));
	uint32_t ctrl;

	syscon_read_reg(dev, CACHE_FUNC_CTRL_REG, &ctrl);

	return (ctrl & CACHE_EANABLE) ? CACHE_LINE_SIZE : 0;
}
#endif /* CONFIG_DCACHE_LINE_SIZE_DETECT */

#ifdef CONFIG_ICACHE_LINE_SIZE_DETECT
size_t cache_instr_line_size_get(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(syscon));
	uint32_t ctrl;

	syscon_read_reg(dev, CACHE_FUNC_CTRL_REG, &ctrl);

	return (ctrl & CACHE_EANABLE) ? CACHE_LINE_SIZE : 0;
}
#endif /* CONFIG_ICACHE_LINE_SIZE_DETECT */
