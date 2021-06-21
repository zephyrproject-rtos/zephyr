/*
 * Copyright (c) 2020 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_internal.h>
#include <sys/__assert.h>
#include <sys/check.h>
#include "core_pmp.h"
#include <arch/riscv/csr.h>

#define LOG_LEVEL CONFIG_MPU_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(mpu);

#ifdef CONFIG_64BIT
# define PR_ADDR "0x%016lx"
#else
# define PR_ADDR "0x%08lx"
#endif

#ifdef CONFIG_64BIT
# define PMPCFG_NUM(index)	(((index) / 8) * 2)
# define PMPCFG_SHIFT(index)	(((index) % 8) * 8)
#else
# define PMPCFG_NUM(index)	((index) / 4)
# define PMPCFG_SHIFT(index)	(((index) % 4) * 8)
#endif

#if defined(CONFIG_PMP_POWER_OF_TWO_ALIGNMENT)
# define PMP_MODE_DEFAULT		PMP_MODE_NAPOT
#else
# define PMP_MODE_DEFAULT		PMP_MODE_TOR
#endif

enum pmp_region_mode {
	PMP_MODE_NA4,
	/* If NAPOT mode region size is 4, apply NA4 region to PMP CSR. */
	PMP_MODE_NAPOT,
	PMP_MODE_TOR,
};

/* Region definition data structure */
struct riscv_pmp_region {
	ulong_t start;
	ulong_t size;
	uint8_t perm;
	enum pmp_region_mode mode;
};

#ifdef CONFIG_USERSPACE
extern ulong_t is_user_mode;
#endif

#ifdef CONFIG_PMP_STACK_GUARD
static const struct riscv_pmp_region irq_stack_guard_region = {
	.start = (ulong_t) z_interrupt_stacks[0],
	.size = PMP_GUARD_ALIGN_AND_SIZE,
	.perm = 0,
	.mode = PMP_MODE_NAPOT,
};
#endif

enum {
	CSR_PMPCFG0,
	CSR_PMPCFG1,
	CSR_PMPCFG2,
	CSR_PMPCFG3,
	CSR_PMPADDR0,
	CSR_PMPADDR1,
	CSR_PMPADDR2,
	CSR_PMPADDR3,
	CSR_PMPADDR4,
	CSR_PMPADDR5,
	CSR_PMPADDR6,
	CSR_PMPADDR7,
	CSR_PMPADDR8,
	CSR_PMPADDR9,
	CSR_PMPADDR10,
	CSR_PMPADDR11,
	CSR_PMPADDR12,
	CSR_PMPADDR13,
	CSR_PMPADDR14,
	CSR_PMPADDR15
};

static __unused ulong_t csr_read_enum(int pmp_csr_enum)
{
	ulong_t res = -1;

	switch (pmp_csr_enum) {
	case CSR_PMPCFG0:
		res = csr_read(0x3A0); break;
	case CSR_PMPCFG1:
		res = csr_read(0x3A1); break;
	case CSR_PMPCFG2:
		res = csr_read(0x3A2); break;
	case CSR_PMPCFG3:
		res = csr_read(0x3A3); break;
	case CSR_PMPADDR0:
		res = csr_read(0x3B0); break;
	case CSR_PMPADDR1:
		res = csr_read(0x3B1); break;
	case CSR_PMPADDR2:
		res = csr_read(0x3B2); break;
	case CSR_PMPADDR3:
		res = csr_read(0x3B3); break;
	case CSR_PMPADDR4:
		res = csr_read(0x3B4); break;
	case CSR_PMPADDR5:
		res = csr_read(0x3B5); break;
	case CSR_PMPADDR6:
		res = csr_read(0x3B6); break;
	case CSR_PMPADDR7:
		res = csr_read(0x3B7); break;
	case CSR_PMPADDR8:
		res = csr_read(0x3B8); break;
	case CSR_PMPADDR9:
		res = csr_read(0x3B9); break;
	case CSR_PMPADDR10:
		res = csr_read(0x3BA); break;
	case CSR_PMPADDR11:
		res = csr_read(0x3BB); break;
	case CSR_PMPADDR12:
		res = csr_read(0x3BC); break;
	case CSR_PMPADDR13:
		res = csr_read(0x3BD); break;
	case CSR_PMPADDR14:
		res = csr_read(0x3BE); break;
	case CSR_PMPADDR15:
		res = csr_read(0x3BF); break;
	default:
		break;
	}
	return res;
}

static void csr_write_enum(int pmp_csr_enum, ulong_t value)
{
	switch (pmp_csr_enum) {
	case CSR_PMPCFG0:
		csr_write(0x3A0, value); break;
	case CSR_PMPCFG1:
		csr_write(0x3A1, value); break;
	case CSR_PMPCFG2:
		csr_write(0x3A2, value); break;
	case CSR_PMPCFG3:
		csr_write(0x3A3, value); break;
	case CSR_PMPADDR0:
		csr_write(0x3B0, value); break;
	case CSR_PMPADDR1:
		csr_write(0x3B1, value); break;
	case CSR_PMPADDR2:
		csr_write(0x3B2, value); break;
	case CSR_PMPADDR3:
		csr_write(0x3B3, value); break;
	case CSR_PMPADDR4:
		csr_write(0x3B4, value); break;
	case CSR_PMPADDR5:
		csr_write(0x3B5, value); break;
	case CSR_PMPADDR6:
		csr_write(0x3B6, value); break;
	case CSR_PMPADDR7:
		csr_write(0x3B7, value); break;
	case CSR_PMPADDR8:
		csr_write(0x3B8, value); break;
	case CSR_PMPADDR9:
		csr_write(0x3B9, value); break;
	case CSR_PMPADDR10:
		csr_write(0x3BA, value); break;
	case CSR_PMPADDR11:
		csr_write(0x3BB, value); break;
	case CSR_PMPADDR12:
		csr_write(0x3BC, value); break;
	case CSR_PMPADDR13:
		csr_write(0x3BD, value); break;
	case CSR_PMPADDR14:
		csr_write(0x3BE, value); break;
	case CSR_PMPADDR15:
		csr_write(0x3BF, value); break;
	default:
		break;
	}
}

/*
 * @brief Set a Physical Memory Protection slot
 *
 * Configure a memory region to be secured by one of the 16 PMP entries.
 *
 * @param index		Number of the targeted PMP entrie (0 to 15 only).
 * @param cfg_val	Configuration value (cf datasheet or defined flags)
 * @param addr_val	Address register value
 *
 * This function shall only be called from Secure state.
 *
 * @return -1 if bad argument, 0 otherwise.
 */
static __unused int riscv_pmp_set(unsigned int index, ulong_t cfg_val, ulong_t addr_val)
{
	ulong_t reg_val;
	ulong_t shift, mask;
	int pmpcfg_csr;
	int pmpaddr_csr;

	if (index >= CONFIG_PMP_SLOT) {
		return -1;
	}

	/* Calculate PMP config/addr register, shift and mask */
	pmpcfg_csr = CSR_PMPCFG0 + PMPCFG_NUM(index);
	pmpaddr_csr = CSR_PMPADDR0 + index;
	shift = PMPCFG_SHIFT(index);
	mask = 0xFF << shift;

	reg_val = csr_read_enum(pmpcfg_csr);
	reg_val = reg_val & ~mask;
	reg_val = reg_val | (cfg_val << shift);

	csr_write_enum(pmpaddr_csr, addr_val);
	csr_write_enum(pmpcfg_csr, reg_val);
	return 0;
}

static int riscv_pmp_region_translate(int index,
	const struct riscv_pmp_region *region, bool to_csr,
	ulong_t pmpcfg[], ulong_t pmpaddr[])
{
	int result, pmp_mode;

	if ((region->start == 0) && (region->size == 0)) {
		/* special case: set whole memory as single PMP region.
		 *   RV32: 0 ~ (2**32 - 1)
		 *   RV64: 0 ~ (2**64 - 1)
		 */
		if (index >= CONFIG_PMP_SLOT) {
			return -ENOSPC;
		}

		pmp_mode = PMP_NAPOT;

		uint8_t cfg_val = PMP_NAPOT | region->perm;
#ifdef CONFIG_64BIT
		ulong_t addr_val = 0x1FFFFFFFFFFFFFFF;
#else
		ulong_t addr_val = 0x1FFFFFFF;
#endif

		if (to_csr) {
			riscv_pmp_set(index, cfg_val, addr_val);
		} else {
			uint8_t *u8_pmpcfg = (uint8_t *)pmpcfg;

			u8_pmpcfg[index] = cfg_val;
			pmpaddr[index] = addr_val;
		}

		result = index+1;
	} else if (region->mode == PMP_MODE_TOR) {
		if ((index+1) >= CONFIG_PMP_SLOT) {
			return -ENOSPC;
		}

		pmp_mode = PMP_TOR;

		uint8_t cfg_val1 = PMP_NA4 | region->perm;
		ulong_t addr_val1 = TO_PMP_ADDR(region->start);
		uint8_t cfg_val2 = PMP_TOR | region->perm;
		ulong_t addr_val2 = TO_PMP_ADDR(region->start + region->size);

		if (to_csr) {
			riscv_pmp_set(index, cfg_val1, addr_val1);
			riscv_pmp_set(index+1, cfg_val2, addr_val2);
		} else {
			uint8_t *u8_pmpcfg = (uint8_t *)pmpcfg;

			u8_pmpcfg[index] = cfg_val1;
			pmpaddr[index] = addr_val1;
			u8_pmpcfg[index+1] = cfg_val2;
			pmpaddr[index+1] = addr_val2;
		}

		result = index+2;
	} else {
		if (index >= CONFIG_PMP_SLOT) {
			return -ENOSPC;
		}

		if ((region->mode == PMP_MODE_NA4) || (region->size == 4)) {
			pmp_mode = PMP_NA4;
		} else {
			pmp_mode = PMP_NAPOT;
		}

		uint8_t cfg_val = pmp_mode | region->perm;
		ulong_t addr_val = TO_PMP_NAPOT(region->start, region->size);

		if (to_csr) {
			riscv_pmp_set(index, cfg_val, addr_val);
		} else {
			uint8_t *u8_pmpcfg = (uint8_t *)pmpcfg;

			u8_pmpcfg[index] = cfg_val;
			pmpaddr[index] = addr_val;
		}

		result = index+1;
	}

	if (to_csr) {
		LOG_DBG("Set PMP region %d: (" PR_ADDR ", " PR_ADDR
			", %s%s%s, %s)", index, region->start, region->size,
			((region->perm & PMP_R) ? "R" : " "),
			((region->perm & PMP_W) ? "W" : " "),
			((region->perm & PMP_X) ? "X" : " "),
			((pmp_mode == PMP_TOR) ? "TOR" :
			(pmp_mode == PMP_NAPOT) ? "NAPOT" : "NA4"));
	} else {
		LOG_DBG("PMP context " PR_ADDR " add region %d: (" PR_ADDR
			", " PR_ADDR ", %s%s%s, %s)", (ulong_t) pmpcfg, index,
			region->start, region->size,
			((region->perm & PMP_R) ? "R" : " "),
			((region->perm & PMP_W) ? "W" : " "),
			((region->perm & PMP_X) ? "X" : " "),
			((pmp_mode == PMP_TOR) ? "TOR" :
			(pmp_mode == PMP_NAPOT) ? "NAPOT" : "NA4"));
	}

	if (pmp_mode == PMP_TOR) {
		LOG_DBG("TOR mode region also use entry %d", index+1);
	}

	return result;
}

#if defined(CONFIG_PMP_STACK_GUARD) || defined(CONFIG_USERSPACE)
static __unused int riscv_pmp_regions_translate(int start_reg_index,
	const struct riscv_pmp_region regions[], uint8_t regions_num,
	ulong_t pmpcfg[], ulong_t pmpaddr[])
{
	int reg_index = start_reg_index;

	for (int i = 0; i < regions_num; i++) {
		/*
		 * Empty region.
		 *
		 * Note: start = size = 0 is valid region (special case).
		 */
		if ((regions[i].size == 0U) && (regions[i].start != 0)) {
			continue;
		}

		/* Non-empty region. */
		reg_index = riscv_pmp_region_translate(reg_index, &regions[i],
			false, pmpcfg, pmpaddr);

		if (reg_index == -ENOSPC) {
			LOG_ERR("no free PMP entry");
			return -ENOSPC;
		}
	}

	return reg_index;
}
#endif /* defined(CONFIG_PMP_STACK_GUARD) || defined(CONFIG_USERSPACE) */

static int riscv_pmp_region_set(int index, const struct riscv_pmp_region *region)
{
	/* Check 4 bytes alignment */
	CHECKIF(!(((region->start & 0x3) == 0) &&
		((region->size & 0x3) == 0) &&
		region->size)) {
		LOG_ERR("PMP address/size are not 4 bytes aligned");
		return -EINVAL;
	}

	return riscv_pmp_region_translate(index, region, true, NULL, NULL);
}

void z_riscv_pmp_clear_config(void)
{
	LOG_DBG("Clear all dynamic PMP regions");
	for (unsigned int i = 0; i < RISCV_PMP_CFG_NUM; i++)
		csr_write_enum(CSR_PMPCFG0 + i, 0);
}

#if defined(CONFIG_USERSPACE)
#include <linker/linker-defs.h>
void z_riscv_init_user_accesses(struct k_thread *thread)
{
	unsigned char index = 0U;
#ifdef CONFIG_PMP_STACK_GUARD
	index++;
#endif /* CONFIG_PMP_STACK_GUARD */

	struct riscv_pmp_region dynamic_regions[] = {
		{
			/* MCU state */
			.start = (ulong_t) &is_user_mode,
			.size = 4,
			.perm = PMP_R,
			.mode = PMP_MODE_NA4,
		},
		{
			/* Program and RO data */
			.start = (ulong_t) __rom_region_start,
			.size = (ulong_t) __rom_region_size,
			.perm = PMP_R | PMP_X,
			.mode = PMP_MODE_DEFAULT,
		},
		{
			/* User-mode thread stack */
			.start = thread->stack_info.start,
			.size = thread->stack_info.size,
			.perm = PMP_R | PMP_W,
			.mode = PMP_MODE_DEFAULT,
		},
	};

	riscv_pmp_regions_translate(index, dynamic_regions,
		ARRAY_SIZE(dynamic_regions), thread->arch.u_pmpcfg,
		thread->arch.u_pmpaddr);
}

void z_riscv_configure_user_allowed_stack(struct k_thread *thread)
{
	unsigned int i;

	z_riscv_pmp_clear_config();

	for (i = 0U; i < CONFIG_PMP_SLOT; i++)
		csr_write_enum(CSR_PMPADDR0 + i, thread->arch.u_pmpaddr[i]);

	for (i = 0U; i < RISCV_PMP_CFG_NUM; i++)
		csr_write_enum(CSR_PMPCFG0 + i, thread->arch.u_pmpcfg[i]);

	LOG_DBG("Apply user PMP context " PR_ADDR " to dynamic PMP regions",
		(ulong_t) thread->arch.u_pmpcfg);
}

int z_riscv_pmp_add_dynamic(struct k_thread *thread,
			ulong_t addr,
			ulong_t size,
			unsigned char flags)
{
	unsigned char index = 0U;
	unsigned char *uchar_pmpcfg;
	int ret = 0;

	/* Check 4 bytes alignment */
	CHECKIF(!(((addr & 0x3) == 0) && ((size & 0x3) == 0) && size)) {
		LOG_ERR("address/size are not 4 bytes aligned\n");
		ret = -EINVAL;
		goto out;
	}

	struct riscv_pmp_region pmp_region = {
		.start = addr,
		.size = size,
		.perm = flags,
	};

	/* Get next free entry */
	uchar_pmpcfg = (unsigned char *) thread->arch.u_pmpcfg;

	index = PMP_REGION_NUM_FOR_U_THREAD;

	while ((index < CONFIG_PMP_SLOT) && uchar_pmpcfg[index]) {
		index++;
	}

	/* Select the best mode */
	if (size == 4) {
		pmp_region.mode = PMP_MODE_NA4;
	} else {
		pmp_region.mode = PMP_MODE_DEFAULT;
	}

	index = riscv_pmp_region_translate(index, &pmp_region, false,
		thread->arch.u_pmpcfg, thread->arch.u_pmpaddr);

	if (index == -ENOSPC) {
		ret = -ENOSPC;
	}

out:
	return ret;
}

int arch_buffer_validate(void *addr, size_t size, int write)
{
	uint32_t index, i;
	ulong_t pmp_type, pmp_addr_start, pmp_addr_stop;
	unsigned char *uchar_pmpcfg;
	struct k_thread *thread = _current;
	ulong_t start = (ulong_t) addr;
	ulong_t access_type = PMP_R;
	ulong_t napot_mask;
#ifdef CONFIG_64BIT
	ulong_t max_bit = 64;
#else
	ulong_t max_bit = 32;
#endif /* CONFIG_64BIT */

	if (write) {
		access_type |= PMP_W;
	}

	uchar_pmpcfg = (unsigned char *) thread->arch.u_pmpcfg;

#ifdef CONFIG_PMP_STACK_GUARD
	index = 1U;
#else
	index = 0U;
#endif /* CONFIG_PMP_STACK_GUARD */

#if !defined(CONFIG_PMP_POWER_OF_TWO_ALIGNMENT) || defined(CONFIG_PMP_STACK_GUARD)
__ASSERT((uchar_pmpcfg[index] & PMP_TYPE_MASK) != PMP_TOR,
	"The 1st PMP entry shouldn't configured as TOR");
#endif /* CONFIG_PMP_POWER_OF_TWO_ALIGNMENT || CONFIG_PMP_STACK_GUARD */

	for (; (index < CONFIG_PMP_SLOT) && uchar_pmpcfg[index]; index++) {
		if ((uchar_pmpcfg[index] & access_type) != access_type) {
			continue;
		}

		pmp_type = uchar_pmpcfg[index] & PMP_TYPE_MASK;

#if !defined(CONFIG_PMP_POWER_OF_TWO_ALIGNMENT) || defined(CONFIG_PMP_STACK_GUARD)
		if (pmp_type == PMP_TOR) {
			continue;
		}
#endif /* CONFIG_PMP_POWER_OF_TWO_ALIGNMENT || CONFIG_PMP_STACK_GUARD */

		if (pmp_type == PMP_NA4) {
			pmp_addr_start =
				FROM_PMP_ADDR(thread->arch.u_pmpaddr[index]);

			if ((index == CONFIG_PMP_SLOT - 1)  ||
				((uchar_pmpcfg[index + 1U] & PMP_TYPE_MASK)
					!= PMP_TOR)) {
				pmp_addr_stop = pmp_addr_start + 4;
			} else {
				pmp_addr_stop = FROM_PMP_ADDR(
					thread->arch.u_pmpaddr[index + 1U]);
				index++;
			}
		} else { /* pmp_type == PMP_NAPOT */
			for (i = 0U; i < max_bit; i++) {
				if (!(thread->arch.u_pmpaddr[index] & (1 << i))) {
					break;
				}
			}

			napot_mask = (1 << i) - 1;
			pmp_addr_start = FROM_PMP_ADDR(
				thread->arch.u_pmpaddr[index] & ~napot_mask);
			pmp_addr_stop = pmp_addr_start + (1 << (i + 3));
		}

		if ((start >= pmp_addr_start) && ((start + size - 1) <
			pmp_addr_stop)) {
			return 0;
		}
	}

	return 1;
}

int arch_mem_domain_max_partitions_get(void)
{
	return PMP_MAX_DYNAMIC_REGION;
}

int arch_mem_domain_partition_remove(struct k_mem_domain *domain,
				     uint32_t partition_id)
{
	sys_dnode_t *node, *next_node;
	uint32_t index, i, num;
	ulong_t pmp_mode, pmp_addr;
	unsigned char *uchar_pmpcfg;
	struct k_thread *thread;
	ulong_t size = (ulong_t) domain->partitions[partition_id].size;
	ulong_t start = (ulong_t) domain->partitions[partition_id].start;
	int ret = 0;

	node = sys_dlist_peek_head(&domain->mem_domain_q);
	if (!node) {
		/*
		 * No thread use this memory domain currently, so there isn't
		 * any user PMP region translated from this memory partition.
		 *
		 * We could do nothing and just successfully return.
		 */

		return 0;
	}

	if (size == 4) {
		pmp_mode = PMP_NA4;
		pmp_addr = TO_PMP_ADDR(start);
		num = 1U;
	}
#if !defined(CONFIG_PMP_POWER_OF_TWO_ALIGNMENT) || defined(CONFIG_PMP_STACK_GUARD)
	else if ((start & (size - 1)) || (size & (size - 1))) {
		pmp_mode = PMP_TOR;
		pmp_addr = TO_PMP_ADDR(start + size);
		num = 2U;
	}
#endif /* CONFIG_PMP_POWER_OF_TWO_ALIGNMENT || CONFIG_PMP_STACK_GUARD */
	else {
		pmp_mode = PMP_NAPOT;
		pmp_addr = TO_PMP_NAPOT(start, size);
		num = 1U;
	}

	/*
	 * Find the user PMP region translated from removed
	 * memory partition.
	 */
	thread = CONTAINER_OF(node, struct k_thread, mem_domain_info);
	uchar_pmpcfg = (unsigned char *) thread->arch.u_pmpcfg;
	for (index = PMP_REGION_NUM_FOR_U_THREAD;
		index < CONFIG_PMP_SLOT;
		index++) {
		if (((uchar_pmpcfg[index] & PMP_TYPE_MASK) == pmp_mode) &&
			(pmp_addr == thread->arch.u_pmpaddr[index])) {
			break;
		}
	}

	CHECKIF(!(index < CONFIG_PMP_SLOT)) {
		LOG_DBG("%s: partition not found\n", __func__);
		ret = -ENOENT;
		goto out;
	}

	/*
	 * Remove the user PMP region translated from removed
	 * memory partition. The removal affects all threads
	 * using this memory domain.
	 */
#if !defined(CONFIG_PMP_POWER_OF_TWO_ALIGNMENT) || defined(CONFIG_PMP_STACK_GUARD)
	if (pmp_mode == PMP_TOR) {
		index--;
	}
#endif /* CONFIG_PMP_POWER_OF_TWO_ALIGNMENT || CONFIG_PMP_STACK_GUARD */

	SYS_DLIST_FOR_EACH_NODE_SAFE(&domain->mem_domain_q, node, next_node) {
		thread = CONTAINER_OF(node, struct k_thread, mem_domain_info);

		uchar_pmpcfg = (unsigned char *) thread->arch.u_pmpcfg;

		for (i = index + num; i < CONFIG_PMP_SLOT; i++) {
			uchar_pmpcfg[i - num] = uchar_pmpcfg[i];
			thread->arch.u_pmpaddr[i - num] =
				thread->arch.u_pmpaddr[i];
		}

		uchar_pmpcfg[CONFIG_PMP_SLOT - 1] = 0U;
		if (num == 2U) {
			uchar_pmpcfg[CONFIG_PMP_SLOT - 2] = 0U;
		}
	}

out:
	return ret;
}

int arch_mem_domain_thread_add(struct k_thread *thread)
{
	struct k_mem_partition *partition;
	int ret = 0, ret2;

	for (int i = 0, pcount = 0;
		pcount < thread->mem_domain_info.mem_domain->num_partitions;
		i++) {
		partition = &thread->mem_domain_info.mem_domain->partitions[i];
		if (partition->size == 0) {
			continue;
		}
		pcount++;

		ret2 = z_riscv_pmp_add_dynamic(thread,
			(ulong_t) partition->start,
			(ulong_t) partition->size, partition->attr.pmp_attr);
		ARG_UNUSED(ret2);
		CHECKIF(ret2 != 0) {
			ret = ret2;
		}
	}

	return ret;
}

int arch_mem_domain_partition_add(struct k_mem_domain *domain,
				  uint32_t partition_id)
{
	sys_dnode_t *node, *next_node;
	struct k_thread *thread;
	struct k_mem_partition *partition;
	int ret = 0, ret2;

	partition = &domain->partitions[partition_id];

	SYS_DLIST_FOR_EACH_NODE_SAFE(&domain->mem_domain_q, node, next_node) {
		thread = CONTAINER_OF(node, struct k_thread, mem_domain_info);

		ret2 = z_riscv_pmp_add_dynamic(thread,
			(ulong_t) partition->start,
			(ulong_t) partition->size, partition->attr.pmp_attr);
		ARG_UNUSED(ret2);
		CHECKIF(ret2 != 0) {
			ret = ret2;
		}
	}

	return ret;
}

int arch_mem_domain_thread_remove(struct k_thread *thread)
{
	uint32_t i;
	unsigned char *uchar_pmpcfg;

	uchar_pmpcfg = (unsigned char *) thread->arch.u_pmpcfg;

	for (i = PMP_REGION_NUM_FOR_U_THREAD; i < CONFIG_PMP_SLOT; i++) {
		uchar_pmpcfg[i] = 0U;
	}

	return 0;
}

#endif /* CONFIG_USERSPACE */

#ifdef CONFIG_PMP_STACK_GUARD
void z_riscv_init_stack_guard(struct k_thread *thread)
{
	ulong_t stack_guard_addr;
	struct riscv_pmp_region dynamic_regions[4]; /* Maximum region_num is 4 */
	uint8_t region_num = 0U;

	/* stack guard: None */
	dynamic_regions[region_num].start = thread->stack_info.start;
	dynamic_regions[region_num].size = PMP_GUARD_ALIGN_AND_SIZE;
	dynamic_regions[region_num].perm = 0;
	dynamic_regions[region_num].mode = PMP_MODE_TOR;
	region_num++;

#ifdef CONFIG_USERSPACE
	if (thread->arch.priv_stack_start) {
#ifdef CONFIG_PMP_POWER_OF_TWO_ALIGNMENT
		stack_guard_addr = thread->arch.priv_stack_start;
#else
		stack_guard_addr = (ulong_t) thread->stack_obj;
#endif /* CONFIG_PMP_POWER_OF_TWO_ALIGNMENT */
		dynamic_regions[region_num].start = stack_guard_addr;
		dynamic_regions[region_num].size = PMP_GUARD_ALIGN_AND_SIZE;
		dynamic_regions[region_num].perm = 0;
		dynamic_regions[region_num].mode = PMP_MODE_TOR;
		region_num++;
	}
#endif /* CONFIG_USERSPACE */

	/* RAM: RW */
	dynamic_regions[region_num].start = CONFIG_SRAM_BASE_ADDRESS;
	dynamic_regions[region_num].size = KB(CONFIG_SRAM_SIZE);
	dynamic_regions[region_num].perm = PMP_R | PMP_W;
	dynamic_regions[region_num].mode = PMP_MODE_NAPOT;
	region_num++;

	/* All other memory: RWX */
	/* special case: start = size = 0 means whole memory. */
	dynamic_regions[region_num].start = 0;
	dynamic_regions[region_num].size = 0;
	dynamic_regions[region_num].perm = PMP_R | PMP_W | PMP_X;
	dynamic_regions[region_num].mode = PMP_MODE_NAPOT;
	region_num++;

	/* Reserve index 0 for PMP stack guard */
	unsigned char index = 1;

	riscv_pmp_regions_translate(index, dynamic_regions, region_num,
		thread->arch.s_pmpcfg, thread->arch.s_pmpaddr);
}

void z_riscv_configure_stack_guard(struct k_thread *thread)
{
	unsigned int i;

	/* Disable PMP for machine mode */
	csr_clear(mstatus, MSTATUS_MPRV);

	z_riscv_pmp_clear_config();

	for (i = 1U; i < PMP_REGION_NUM_FOR_STACK_GUARD; i++)
		csr_write_enum(CSR_PMPADDR0 + i, thread->arch.s_pmpaddr[i]);

	for (i = 0U; i < PMP_CFG_CSR_NUM_FOR_STACK_GUARD; i++)
		csr_write_enum(CSR_PMPCFG0 + i, thread->arch.s_pmpcfg[i]);

	/* Enable PMP for machine mode */
	csr_set(mstatus, MSTATUS_MPRV);
}

void z_riscv_configure_interrupt_stack_guard(void)
{
	int index = 0, ret;

	LOG_DBG("Set static PMP region %d for IRQ stack guard", index);
	ret = riscv_pmp_region_set(index, &irq_stack_guard_region);
	if (ret == -EINVAL) {
		LOG_ERR("Configure static PMP region of IRQ stack guard "
			"failed");
	}
}
#endif /* CONFIG_PMP_STACK_GUARD */

#if defined(CONFIG_PMP_STACK_GUARD) || defined(CONFIG_USERSPACE)

void z_riscv_pmp_init_thread(struct k_thread *thread)
{
	/* Clear [u|s]_pmp[cfg|addr] field of k_thread */
	unsigned char i;
	ulong_t *pmpcfg;

#if defined(CONFIG_PMP_STACK_GUARD)
	pmpcfg = thread->arch.s_pmpcfg;
	for (i = 0U; i < PMP_CFG_CSR_NUM_FOR_STACK_GUARD; i++)
		pmpcfg[i] = 0;
#endif /* CONFIG_PMP_STACK_GUARD */

#if defined(CONFIG_USERSPACE)
	pmpcfg = thread->arch.u_pmpcfg;
	for (i = 0U; i < RISCV_PMP_CFG_NUM; i++)
		pmpcfg[i] = 0;
#endif /* CONFIG_USERSPACE */
}
#endif /* CONFIG_PMP_STACK_GUARD || CONFIG_USERSPACE */
