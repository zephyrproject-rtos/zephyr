/*
 * Copyright (c) 2022 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Physical Memory Protection (PMP) is RISC-V parlance for an MPU.
 *
 * The PMP is comprized of a number of entries or slots. This number depends
 * on the hardware design. For each slot there is an address register and
 * a configuration register. While each address register is matched to an
 * actual CSR register, configuration registers are small and therefore
 * several of them are bundled in a few additional CSR registers.
 *
 * PMP slot configurations are updated in memory to avoid read-modify-write
 * cycles on corresponding CSR registers. Relevant CSR registers are always
 * written in batch from their shadow copy in RAM for better efficiency.
 */

#include <kernel.h>
#include <kernel_internal.h>
#include <linker/linker-defs.h>
#include <pmp.h>
#include <sys/arch_interface.h>
#include <arch/riscv/csr.h>

#define LOG_LEVEL CONFIG_MPU_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(mpu);

#define PMP_DEBUG_DUMP 0

#ifdef CONFIG_64BIT
# define PR_ADDR "0x%016lx"
#else
# define PR_ADDR "0x%08lx"
#endif

#define PMPCFG_STRIDE sizeof(ulong_t)

#define PMP_ADDR(addr)			((addr) >> 2)
#define NAPOT_RANGE(size)		(((size) - 1) >> 1)
#define PMP_ADDR_NAPOT(addr, size)	PMP_ADDR(addr | NAPOT_RANGE(size))

#define PMP_NONE 0

static void print_pmp_entries(unsigned int start, unsigned int end,
			      ulong_t *pmp_addr, ulong_t *pmp_cfg,
			      const char *banner)
{
	uint8_t *pmp_n_cfg = (uint8_t *)pmp_cfg;
	unsigned int index;

	LOG_DBG("PMP %s:", banner);
	for (index = start; index < end; index++) {
		LOG_DBG("%3d: "PR_ADDR" 0x%02x", index,
			pmp_addr[index],
			pmp_n_cfg[index]);
	}
}

static void dump_pmp_regs(const char *banner)
{
	ulong_t pmp_addr[CONFIG_PMP_SLOT];
	ulong_t pmp_cfg[CONFIG_PMP_SLOT / PMPCFG_STRIDE];

#define PMPADDR_READ(x) pmp_addr[x] = csr_read(pmpaddr##x)

	FOR_EACH(PMPADDR_READ, (;), 0, 1, 2, 3, 4, 5, 6, 7);
#if CONFIG_PMP_SLOT > 8
	FOR_EACH(PMPADDR_READ, (;), 8, 9, 10, 11, 12, 13, 14, 15);
#endif

#undef PMPADDR_READ

#ifdef CONFIG_64BIT
	pmp_cfg[0] = csr_read(pmpcfg0);
#if CONFIG_PMP_SLOT > 8
	pmp_cfg[1] = csr_read(pmpcfg2);
#endif
#else
	pmp_cfg[0] = csr_read(pmpcfg0);
	pmp_cfg[1] = csr_read(pmpcfg1);
#if CONFIG_PMP_SLOT > 8
	pmp_cfg[2] = csr_read(pmpcfg2);
	pmp_cfg[3] = csr_read(pmpcfg3);
#endif
#endif

	print_pmp_entries(0, CONFIG_PMP_SLOT, pmp_addr, pmp_cfg, banner);
}

/**
 * @brief Set PMP shadow register values in memory
 *
 * Register content is built using this function which selects the most
 * appropriate address matching mode automatically. Note that the special
 * case start=0 size=0 is valid and means the whole address range.
 *
 * @param index_p Location of the current PMP slot index to use. This index
 *                will be updated according to the number of slots used.
 * @param perm PMP permission flags
 * @param start Start address of the memory area to cover
 * @param size Size of the memory area to cover
 * @param pmp_addr Array of pmpaddr values (starting at entry 0).
 * @param pmp_cfg Array of pmpcfg values (starting at entry 0).
 * @param index_limit Index value representing the size of the provided arrays.
 * @return true on success, false when out of free PMP slots.
 */
static bool set_pmp_entry(unsigned int *index_p, uint8_t perm,
			  uintptr_t start, size_t size,
			  ulong_t *pmp_addr, ulong_t *pmp_cfg,
			  unsigned int index_limit)
{
	uint8_t *pmp_n_cfg = (uint8_t *)pmp_cfg;
	unsigned int index = *index_p;
	bool ok = true;

	__ASSERT((start & 0x3) == 0, "misaligned start address");
	__ASSERT((size & 0x3) == 0, "misaligned size");

	if (index >= index_limit) {
		LOG_ERR("out of PMP slots");
		ok = false;
	} else if ((index == 0 && start == 0) ||
		   (index != 0 && pmp_addr[index - 1] == PMP_ADDR(start))) {
		/* We can use TOR using only one additional slot */
		pmp_addr[index] = PMP_ADDR(start + size);
		pmp_n_cfg[index] = perm | PMP_TOR;
		index += 1;
	} else if (((size  & (size - 1)) == 0) /* power of 2 */ &&
		   ((start & (size - 1)) == 0) /* naturally aligned */) {
		pmp_addr[index] = PMP_ADDR_NAPOT(start, size);
		pmp_n_cfg[index] = perm | (size == 4 ? PMP_NA4 : PMP_NAPOT);
		index += 1;
	} else if (index + 1 >= index_limit) {
		LOG_ERR("out of PMP slots");
		ok = false;
	} else {
		pmp_addr[index] = PMP_ADDR(start);
		pmp_n_cfg[index] = 0;
		index += 1;
		pmp_addr[index] = PMP_ADDR(start + size);
		pmp_n_cfg[index] = perm | PMP_TOR;
		index += 1;
	}

	*index_p = index;
	return ok;
}

/**
 * @brief Write a range of PMP entries to corresponding PMP registers
 *
 * PMP registers are accessed with the csr instruction which only takes an
 * immediate value as the actual register. This is performed more efficiently
 * in assembly code (pmp.S) than what is possible with C code.
 *
 * Requirement: start < end && end <= CONFIG_PMP_SLOT
 *
 * @param start Start of the PMP range to be written
 * @param end End (exclusive) of the PMP range to be written
 * @param clear_trailing_entries True if trailing entries must be turned off
 * @param pmp_addr Array of pmpaddr values (starting at entry 0).
 * @param pmp_cfg Array of pmpcfg values (starting at entry 0).
 */
extern void z_riscv_write_pmp_entries(unsigned int start, unsigned int end,
				      bool clear_trailing_entries,
				      ulong_t *pmp_addr, ulong_t *pmp_cfg);

/**
 * @brief Write a range of PMP entries to corresponding PMP registers
 *
 * This performs some sanity checks before calling z_riscv_write_pmp_entries().
 *
 * @param start Start of the PMP range to be written
 * @param end End (exclusive) of the PMP range to be written
 * @param clear_trailing_entries True if trailing entries must be turned off
 * @param pmp_addr Array of pmpaddr values (starting at entry 0).
 * @param pmp_cfg Array of pmpcfg values (starting at entry 0).
 * @param index_limit Index value representing the size of the provided arrays.
 */
static void write_pmp_entries(unsigned int start, unsigned int end,
			      bool clear_trailing_entries,
			      ulong_t *pmp_addr, ulong_t *pmp_cfg,
			      unsigned int index_limit)
{
	__ASSERT(start < end && end <= index_limit &&
		 index_limit <= CONFIG_PMP_SLOT,
		 "bad PMP range (start=%u end=%u)", start, end);

	/* Be extra paranoid in case assertions are disabled */
	if (start >= end || end > index_limit) {
		k_panic();
	}

	if (clear_trailing_entries) {
		/*
		 * There are many config entries per pmpcfg register.
		 * Make sure to clear trailing garbage in the last
		 * register to be written if any. Remaining registers
		 * will be cleared in z_riscv_write_pmp_entries().
		 */
		uint8_t *pmp_n_cfg = (uint8_t *)pmp_cfg;
		unsigned int index;

		for (index = end; index % PMPCFG_STRIDE != 0; index++) {
			pmp_n_cfg[index] = 0;
		}
	}

	print_pmp_entries(start, end, pmp_addr, pmp_cfg, "register write");

	z_riscv_write_pmp_entries(start, end, clear_trailing_entries,
				  pmp_addr, pmp_cfg);
}

/*
 * This is used to seed thread PMP copies with global m-mode cfg entries
 * sharing the same cfg register. Locked entries aren't modifiable but
 * we could have non-locked entries here too.
 */
static ulong_t global_pmp_cfg[1];

/* End of global PMP entry range */
static unsigned int global_pmp_end_index;

/**
 * @Brief Initialize the PMP with global entries on each CPU
 */
void z_riscv_pmp_init(void)
{
	ulong_t pmp_addr[4];
	ulong_t pmp_cfg[1];
	unsigned int index = 0;

	/* The read-only area is always there for every mode */
	set_pmp_entry(&index, PMP_R | PMP_X | PMP_L,
		      (uintptr_t)__rom_region_start,
		      (size_t)__rom_region_size,
		      pmp_addr, pmp_cfg, ARRAY_SIZE(pmp_addr));

	write_pmp_entries(0, index, true, pmp_addr, pmp_cfg, ARRAY_SIZE(pmp_addr));

#ifdef CONFIG_SMP
	/* Make sure secondary CPUs produced the same values */
	if (global_pmp_end_index != 0) {
		__ASSERT(global_pmp_end_index == index, "");
		__ASSERT(global_pmp_cfg[0] == pmp_cfg[0], "");
	}
#endif

	global_pmp_cfg[0] = pmp_cfg[0];
	global_pmp_end_index = index;

	if (PMP_DEBUG_DUMP) {
		dump_pmp_regs("initial register dump");
	}
}
