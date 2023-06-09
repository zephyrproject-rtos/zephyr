/*
 * Copyright (c) 2021 Andes Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/linker/linker-defs.h>

#ifndef CONFIG_ASSERT
#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pma_init, LOG_LEVEL);
#endif

/* Programmable PMA mechanism is supported */
#define MMSC_CFG_PPMA		(1 << 30)

/*
 * PMA Configuration (PMACFG) bitfields
 */

/* ETYPE: Entry address matching mode */
#define PMACFG_ETYPE_MASK	3
#define PMACFG_ETYPE_OFF	0
#define PMACFG_ETYPE_TOR	1
#define PMACFG_ETYPE_NA4	2
#define PMACFG_ETYPE_NAPOT	3

/* MTYPE: Memory type attribute */
#define PMACFG_MTYPE_MASK			(0xF << 2)
/* non-cacheable attributes (bufferable or not) */
#define PMACFG_MTYPE_MEMORY_NOCACHE_BUFFERABLE	(3 << 2)
/* cacheable attributes (write-through/back, no/read/write/RW-allocate) */
#define PMACFG_MTYPE_MEMORY_WBCACHE_RWALLOC	(11 << 2)

/* pmaaddr is 4-byte granularity in each mode */
#define TO_PMA_ADDR(addr)		((addr) >> 2)

/* The base address is aligned to size */
#define NAPOT_BASE(start, size)		TO_PMA_ADDR((start) & ~((size) - 1))
/* The encoding of size is 0b01...1
 * (change the leading bit of bitmask to 0)
 */
#define NAPOT_SIZE(size)		TO_PMA_ADDR(((size) - 1) >> 1)

#define NA4_ENCODING(start)		TO_PMA_ADDR(start)
#define NAPOT_ENCODING(start, size)	(NAPOT_BASE(start, size) \
					 | NAPOT_SIZE(size))

#ifdef CONFIG_64BIT
/* In riscv64, CSR pmacfg number are even number (0, 2, ...) */
# define PMACFG_NUM(index)		((index / RV_REGSIZE) * 2)
#else
# define PMACFG_NUM(index)		(index / RV_REGSIZE)
#endif
#define PMACFG_SHIFT(index)		((index % RV_REGSIZE) * 8)

/* Wrappers of inline assembly */
#define read_csr(var, csr) \
	({ __asm__ volatile ("csrr %0, %1" : "=r" (var) : "i" (csr)); })
#define write_csr(csr, val) \
	({ __asm__ volatile ("csrw %0, %1" :: "i" (csr), "r" (val)); })

struct pma_region_attr {
	/* Attributes belonging to pmacfg{i} */
	uint8_t pmacfg;
};

struct pma_region {
	unsigned long start;
	unsigned long size;
	struct pma_region_attr attr;
};

/*
 * Write value to CSRs pmaaddr{i}
 */
static void write_pmaaddr_csr(const uint32_t index, unsigned long value)
{
	switch (index) {
	case 0:
		write_csr(NDS_PMAADDR0,  value); break;
	case 1:
		write_csr(NDS_PMAADDR1,  value); break;
	case 2:
		write_csr(NDS_PMAADDR2,  value); break;
	case 3:
		write_csr(NDS_PMAADDR3,  value); break;
	case 4:
		write_csr(NDS_PMAADDR4,  value); break;
	case 5:
		write_csr(NDS_PMAADDR5,  value); break;
	case 6:
		write_csr(NDS_PMAADDR6,  value); break;
	case 7:
		write_csr(NDS_PMAADDR7,  value); break;
	case 8:
		write_csr(NDS_PMAADDR8,  value); break;
	case 9:
		write_csr(NDS_PMAADDR9,  value); break;
	case 10:
		write_csr(NDS_PMAADDR10, value); break;
	case 11:
		write_csr(NDS_PMAADDR11, value); break;
	case 12:
		write_csr(NDS_PMAADDR12, value); break;
	case 13:
		write_csr(NDS_PMAADDR13, value); break;
	case 14:
		write_csr(NDS_PMAADDR14, value); break;
	case 15:
		write_csr(NDS_PMAADDR15, value); break;
	}
}

/*
 * Write value to pma{i}cfg entry which are packed into CSRs pmacfg{j}
 */
static void write_pmacfg_entry(const uint32_t entry_index,
	uint8_t entry_value)
{
	/* 1-byte pma{i}cfg entries are packed into XLEN-byte CSRs pmacfg{j} */
	uint32_t index = PMACFG_NUM(entry_index);
	uint8_t shift = PMACFG_SHIFT(entry_index);
	unsigned long pmacfg = 0;

	switch (index) {
	case 0:
		read_csr(pmacfg, NDS_PMACFG0); break;
	case 1:
		read_csr(pmacfg, NDS_PMACFG1); break;
	case 2:
		read_csr(pmacfg, NDS_PMACFG2); break;
	case 3:
		read_csr(pmacfg, NDS_PMACFG3); break;
	}

	/* clear old value in pmacfg entry */
	pmacfg &= ~(0xFF << shift);
	/* set new value to pmacfg entry value */
	pmacfg |= entry_value << shift;

	switch (index) {
	case 0:
		write_csr(NDS_PMACFG0, pmacfg); break;
	case 1:
		write_csr(NDS_PMACFG1, pmacfg); break;
	case 2:
		write_csr(NDS_PMACFG2, pmacfg); break;
	case 3:
		write_csr(NDS_PMACFG3, pmacfg); break;
	}
}

/*
 * This internal function performs PMA region initialization.
 *
 * Note:
 *   The caller must provide a valid region index.
 */
static void region_init(const uint32_t index,
	const struct pma_region *region_conf)
{
	unsigned long pmaaddr;
	uint8_t pmacfg;

	if (region_conf->size == 4) {
		pmaaddr = NA4_ENCODING(region_conf->start);
		pmacfg = region_conf->attr.pmacfg | PMACFG_ETYPE_NA4;
	} else {
		pmaaddr = NAPOT_ENCODING(region_conf->start, region_conf->size);
		pmacfg = region_conf->attr.pmacfg | PMACFG_ETYPE_NAPOT;
	}

	write_pmaaddr_csr(index, pmaaddr);
	write_pmacfg_entry(index, pmacfg);
}

/*
 * This internal function performs run-time sanity check for
 * PMA region start address and size.
 */
static int pma_region_is_valid(const struct pma_region *region)
{
	/* Region size must be power-of-two,
	 * and greater or equal to the minimum
	 * PMA region size. Start address of the
	 * region must align with size.
	 */
	int region_is_valid =
		((region->size & (region->size - 1)) == 0U)
		&&
		(region->size >= CONFIG_SOC_ANDES_V5_PMA_REGION_MIN_ALIGN_AND_SIZE)
		&&
		((region->start & (region->size - 1)) == 0U);

	if (!region_is_valid) {
		return -EINVAL;
	}

	return 0;
}

#ifdef CONFIG_NOCACHE_MEMORY
static void configure_nocache_region(void)
{
	const struct pma_region nocache_region = {
		.start = (unsigned long)&_nocache_ram_start,
		.size = (unsigned long)&_nocache_ram_size,
		.attr = {PMACFG_MTYPE_MEMORY_NOCACHE_BUFFERABLE},
	};

	if (nocache_region.size != 0) {
		if (pma_region_is_valid(&nocache_region) == -EINVAL) {
			__ASSERT(0, "Configuring PMA region of nocache region failed\n");
		}

		/* Initialize nocache region at PMA region 0 */
		region_init(0, &nocache_region);
	}
}
#endif /* CONFIG_NOCACHE_MEMORY */

/*
 * @brief Init PMA CSRs of each CPU core
 *
 * In SMP, each CPU has it's own PMA CSR and PMA CSR only affect one CPU.
 * We should configure CSRs of all CPUs to make memory attribute
 * (e.g. uncacheable) affects all CPUs.
 */
void pma_init_per_core(void)
{
#ifdef CONFIG_NOCACHE_MEMORY
	configure_nocache_region();
#endif /* CONFIG_NOCACHE_MEMORY */
}

static int pma_init(void)
{
	unsigned long mmsc_cfg;

	__asm__ volatile ("csrr %0, %1" : "=r" (mmsc_cfg) : "i" (NDS_MMSC_CFG));

	if (!(mmsc_cfg & MMSC_CFG_PPMA)) {
		/* This CPU doesn't support PMA */

		__ASSERT(0, "CPU doesn't support PMA. "
			    "Please disable CONFIG_SOC_ANDES_V5_PMA\n");
#ifndef CONFIG_ASSERT
		LOG_ERR("CPU doesn't support PMA. Please disable CONFIG_SOC_ANDES_V5_PMA");
#endif
		return -ENODEV;
	}

	pma_init_per_core();

	return 0;
}

SYS_INIT(pma_init, PRE_KERNEL_2,
	CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
