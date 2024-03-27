/*
 * Copyright 2023 Andestech
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/riscv/csr.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/barrier.h>

static struct k_spinlock cache_lock;

#ifdef CONFIG_DCACHE
static ALWAYS_INLINE size_t arch_dcache_line_size_get(void)
{
	size_t dcache_line_size = 0;

#if (CONFIG_DCACHE_LINE_SIZE != 0)
	dcache_line_size = CONFIG_DCACHE_LINE_SIZE;
#elif DT_NODE_HAS_PROP(DT_PATH(cpus, cpu_0), d_cache_line_size)
	dcache_line_size =
		DT_PROP(DT_PATH(cpus, cpu_0), "d_cache_line_size");
#else
	dcache_line_size = 0;
#endif

	return dcache_line_size;
}

static ALWAYS_INLINE void arch_dcache_enable(void)
{
	/* nothing */
}

static ALWAYS_INLINE void arch_dcache_disable(void)
{
	/* nothing */
}

static ALWAYS_INLINE int arch_dcache_flush_all(void)
{
	return -ENOTSUP;
}

static ALWAYS_INLINE int arch_dcache_invd_all(void)
{
	return -ENOTSUP;
}

static ALWAYS_INLINE int arch_dcache_flush_and_invd_all(void)
{
	return -ENOTSUP;
}

#ifndef CONFIG_CBOM_DCACHE_SUPPORTED
static ALWAYS_INLINE int arch_dcache_flush_range(void *start_addr, size_t size)
{
	return -ENOTSUP;
}

static ALWAYS_INLINE int arch_dcache_invd_range(void *start_addr, size_t size)
{
	return -ENOTSUP;
}

static ALWAYS_INLINE int arch_dcache_flush_and_invd_range(void *start_addr, size_t size)
{
	return -ENOTSUP;
}

#else
static ALWAYS_INLINE int arch_dcache_flush_range(void *start_addr, size_t size)
{
	unsigned long last_byte, align_addr;
	size_t line_size;
	k_spinlock_key_t key;

	line_size = arch_dcache_line_size_get();

	if (line_size == 0) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&cache_lock);
	last_byte = (unsigned long)start_addr + size - 1;

	for (align_addr = ROUND_DOWN(start_addr, line_size); align_addr <= last_byte;
			align_addr += line_size) {
		cbo_clean(align_addr);
	}

	k_spin_unlock(&cache_lock, key);

	return 0;
}

static ALWAYS_INLINE int arch_dcache_invd_range(void *start_addr, size_t size)
{
	unsigned long last_byte, align_addr;
	size_t line_size;
	k_spinlock_key_t key;

	line_size = arch_dcache_line_size_get();

	if (line_size == 0) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&cache_lock);
	last_byte = (unsigned long)start_addr + size - 1;

	for (align_addr = ROUND_DOWN(start_addr, line_size); align_addr <= last_byte;
		align_addr += line_size) {

		cbo_inval(align_addr);
	}

	k_spin_unlock(&cache_lock, key);

	return 0;
}

static ALWAYS_INLINE int arch_dcache_flush_and_invd_range(void *start_addr, size_t size)
{
	unsigned long last_byte, align_addr;
	size_t line_size;
	k_spinlock_key_t key;

	line_size = arch_dcache_line_size_get();

	if (line_size == 0) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&cache_lock);
	last_byte = (unsigned long)start_addr + size - 1;

	for (align_addr = ROUND_DOWN(start_addr, line_size); align_addr <= last_byte;
		align_addr += line_size) {

		cbo_flush(align_addr);
	}

	k_spin_unlock(&cache_lock, key);

	return 0;
}
#endif
#endif

#ifdef CONFIG_ICACHE
static ALWAYS_INLINE size_t arch_icache_line_size_get(void)
{
	size_t icache_line_size = 0;

#if (CONFIG_ICACHE_LINE_SIZE != 0)
	icache_line_size = CONFIG_ICACHE_LINE_SIZE;
#elif DT_NODE_HAS_PROP(DT_PATH(cpus, cpu_0), i_cache_line_size)
	icache_line_size =
		DT_PROP(DT_PATH(cpus, cpu_0), "i_cache_line_size");
#else
	icache_line_size = 0;
#endif

	return icache_line_size;
}

static ALWAYS_INLINE void arch_icache_enable(void)
{
	/* nothing */
}


static ALWAYS_INLINE void arch_icache_disable(void)
{
	/* nothing */
}

static ALWAYS_INLINE int arch_icache_flush_all(void)
{
	return -ENOTSUP;
}

static ALWAYS_INLINE int arch_icache_invd_all(void)
{
	return -ENOTSUP;
}

static ALWAYS_INLINE int arch_icache_flush_and_invd_all(void)
{
	return -ENOTSUP;
}

#ifndef CONFIG_CBOM_ICACHE_SUPPORTED
static ALWAYS_INLINE int arch_icache_flush_range(void *start_addr, size_t size)
{
	return -ENOTSUP;
}

static ALWAYS_INLINE int arch_icache_invd_range(void *start_addr, size_t size)
{
	return -ENOTSUP;
}

static ALWAYS_INLINE int arch_icache_flush_and_invd_range(void *start_addr, size_t size)
{
	return -ENOTSUP;
}

#else
static ALWAYS_INLINE int arch_icache_flush_range(void *start_addr, size_t size)
{
	unsigned long last_byte, align_addr;
	size_t line_size;
	k_spinlock_key_t key;

	line_size = arch_icache_line_size_get();

	if (line_size == 0) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&cache_lock);
	last_byte = (unsigned long)start_addr + size - 1;

	for (align_addr = ROUND_DOWN(start_addr, line_size); align_addr <= last_byte;
		align_addr += line_size) {

		cbo_clean(align_addr);
	}

	k_spin_unlock(&cache_lock, key);

	return 0;
}

static ALWAYS_INLINE int arch_icache_invd_range(void *start_addr, size_t size)
{
	unsigned long last_byte, align_addr;
	size_t line_size;
	k_spinlock_key_t key;

	line_size = arch_icache_line_size_get();

	if (line_size == 0) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&cache_lock);
	last_byte = (unsigned long)start_addr + size - 1;

	for (align_addr = ROUND_DOWN(start_addr, line_size); align_addr <= last_byte;
		align_addr += line_size) {

		cbo_inval(align_addr);
	}

	k_spin_unlock(&cache_lock, key);

	return 0;
}

static ALWAYS_INLINE int arch_icache_flush_and_invd_range(void *start_addr, size_t size)
{
	unsigned long last_byte, align_addr;
	size_t line_size;
	k_spinlock_key_t key;

	line_size = arch_icache_line_size_get();

	if (line_size == 0) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&cache_lock);
	last_byte = (unsigned long)start_addr + size - 1;

	for (align_addr = ROUND_DOWN(start_addr, line_size); align_addr <= last_byte;
		align_addr += line_size) {

		cbo_flush(align_addr);
	}

	k_spin_unlock(&cache_lock, key);

	return 0;
}
#endif /* CONFIG_CBOM_ICACHE_SUPPORTED */
#endif /* CONFIG_ICACHE */

extern void __soc_cache_init(void);
void __weak __soc_cache_init(void) {}

static ALWAYS_INLINE int z_riscv_cache_init(void)
{
	unsigned long reg_val = 0;

	__soc_cache_init();

#ifdef CONFIG_RISCV_ISA_EXT_ZICBOM
	reg_val = reg_val | (GENMASK(6, 4));
#endif
#ifdef CONFIG_RISCV_ISA_EXT_ZICBOZ
	reg_val = reg_val | BIT(7);
#endif
	csr_set(menvcfg, reg_val);

	return 0;
}
