/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <errno.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/cache.h>
#include <zephyr/kernel.h>

#include <microblaze/mb_interface.h>

#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_ARCH_CACHE)
#if defined(CONFIG_DCACHE)

#define DCACHE_BASE DT_PROP_OR(_CPU, d_cache_base, 0)
#define ICACHE_BASE DT_PROP_OR(_CPU, i_cache_base, 0)

#define DCACHE_SIZE DT_PROP_OR(_CPU, d_cache_size, 0)
#define ICACHE_SIZE DT_PROP_OR(_CPU, i_cache_size, 0)

#define DCACHE_USE_WRITEBACK DT_PROP_OR(_CPU, d_cache_use_writeback, 0)

/**
 * @brief Enable the d-cache
 *
 * Enable the data cache.
 */
void arch_dcache_enable(void)
{
	microblaze_enable_dcache();
}

/**
 * @brief Disable the d-cache
 *
 * Disable the data cache.
 * It might be a good idea to flush the cache before disabling.
 */
void arch_dcache_disable(void)
{
	microblaze_disable_dcache();
}

/**
 * @brief Flush the d-cache
 *
 * Flush the whole data cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_dcache_flush_all(void)
{
	return arch_dcache_flush_range(DCACHE_BASE, DCACHE_SIZE);
}

/**
 * @brief Invalidate the d-cache
 *
 * Invalidate the whole data cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_dcache_invd_all(void)
{
	return arch_dcache_invd_range(DCACHE_BASE, DCACHE_SIZE);
}

/**
 * @brief Flush and Invalidate the d-cache
 *
 * Flush and Invalidate the whole data cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_dcache_flush_and_invd_all(void)
{
	return arch_dcache_flush_and_invd_range(DCACHE_BASE, DCACHE_SIZE);
}

/**
 * @brief Flush an address range in the d-cache
 *
 * Flush the specified address range of the data cache.
 *
 * @param addr Starting address to flush.
 * @param size Range size.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_dcache_flush_range(void *addr, size_t size)
{
	/* if ! MICROBLAZE_DCACHE_USE_WRITEBACK then
	 * CPU doesn't support flushing without invalidating.
	 */
	if (0 == DCACHE_USE_WRITEBACK)
		return -ENOTSUP;

	const size_t incrementer = 4 * sys_cache_data_line_size_get();
	const intptr_t end_addr = (intptr_t)addr + size;
	/* Aligning start address */
	intptr_t address_iterator = (intptr_t)addr & (-incrementer);

	/* We only need to iterate up to the cache size */
	while (address_iterator < end_addr) {
		wdc_flush(address_iterator);
		address_iterator += incrementer;
	}

	return 0;
}

/**
 * @brief Invalidate an address range in the d-cache
 *
 * Invalidate the specified address range of the data cache.
 *
 * @param addr Starting address to invalidate.
 * @param size Range size.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_dcache_invd_range(void *addr, size_t size)
{
	const uint32_t incrementer = 4 * sys_cache_data_line_size_get();
	const size_t end_addr = (intptr_t)addr + size;

	/* Aligning start address */
	intptr_t address_iterator = (intptr_t)addr & (-incrementer);

	/* We only need to iterate up to the cache size */
	while (address_iterator < end_addr) {
		wic(address_iterator);
		address_iterator += incrementer;
	}

	return 0;
}

/**
 * @brief Flush and Invalidate an address range in the d-cache
 *
 * Flush and Invalidate the specified address range of the data cache.
 *
 * @param addr Starting address to flush and invalidate.
 * @param size Range size.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_dcache_flush_and_invd_range(void *addr, size_t size)
{
	const uint32_t incrementer = 4 * sys_cache_data_line_size_get();
	const size_t end_addr = (intptr_t)addr + size;

	/* Aligning start address */
	intptr_t address_iterator = (intptr_t)addr & (-incrementer);

	/* We only need to iterate up to the cache size */
	while (address_iterator < end_addr) {
		wdc_clear(address_iterator);
		address_iterator += incrementer;
	}

	return 0;
}

#endif /* CONFIG_DCACHE */

#if defined(CONFIG_ICACHE)
/**
 * @brief Enable the i-cache
 *
 * Enable the instruction cache.
 */
void arch_icache_enable(void)
{
	microblaze_enable_icache();
}

/**
 * @brief Disable the i-cache
 *
 * Disable the instruction cache.
 */
void arch_icache_disable(void)
{
	microblaze_disable_icache();
}

/**
 * @brief Flush the i-cache
 *
 * Flush the whole instruction cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_icache_flush_all(void)
{
	return -ENOTSUP;
}

/**
 * @brief Invalidate the i-cache
 *
 * Invalidate the whole instruction cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_icache_invd_all(void)
{
	return arch_icache_invd_range(ICACHE_BASE, ICACHE_SIZE);
}

/**
 * @brief Flush and Invalidate the i-cache
 *
 * Flush and Invalidate the whole instruction cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_icache_flush_and_invd_all(void)
{
	return -ENOTSUP;
}

/**
 * @brief Flush an address range in the i-cache
 *
 * Flush the specified address range of the instruction cache.
 *
 * @param addr Starting address to flush.
 * @param size Range size.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_icache_flush_range(void *addr, size_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return -ENOTSUP;
}

/**
 * @brief Invalidate an address range in the i-cache
 *
 * Invalidate the specified address range of the instruction cache.
 *
 * @param addr Starting address to invalidate.
 * @param size Range size.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_icache_invd_range(void *addr, size_t size)
{
	int key = irq_lock();

	arch_icache_disable();

	const uint32_t incrementer = 4 * sys_cache_instr_line_size_get();
	const size_t end_addr = (intptr_t)addr + size;

	/* Aligning start address */
	intptr_t address_iterator = (intptr_t)addr & (-incrementer);

	/* We only need to iterate up to the cache size */
	while (address_iterator < end_addr) {
		wic(address_iterator);
		address_iterator += incrementer;
	}

	arch_icache_enable();
	irq_unlock(key);

	return 0;
}

/**
 * @brief Flush and Invalidate an address range in the i-cache
 *
 * Flush and Invalidate the specified address range of the instruction cache.
 *
 * @param addr Starting address to flush and invalidate.
 * @param size Range size.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_icache_flush_and_invd_range(void *addr, size_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return -ENOTSUP;
}

#endif /* CONFIG_ICACHE */
#endif /* CONFIG_CACHE_MANAGEMENT && CONFIG_ARCH_CACHE */
