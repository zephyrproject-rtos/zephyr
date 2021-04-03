/*
 * Copyright (c) 2020 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_internal.h>
#include <sys/__assert.h>
#include "core_pmp.h"
#include <arch/riscv/csr.h>
#include <stdio.h>

#define PMP_SLOT_NUMBER	CONFIG_PMP_SLOT

#ifdef CONFIG_USERSPACE
extern ulong_t is_user_mode;
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

ulong_t csr_read_enum(int pmp_csr_enum)
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

void csr_write_enum(int pmp_csr_enum, ulong_t value)
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

int z_riscv_pmp_set(unsigned int index, ulong_t cfg_val, ulong_t addr_val)
{
	ulong_t reg_val;
	ulong_t shift, mask;
	int pmpcfg_csr;
	int pmpaddr_csr;

	if (index >= PMP_SLOT_NUMBER) {
		return -1;
	}

	/* Calculate PMP config/addr register, shift and mask */
#ifdef CONFIG_64BIT
	pmpcfg_csr = CSR_PMPCFG0 + ((index >> 3) << 1);
	shift = (index & 0x7) << 3;
#else
	pmpcfg_csr = CSR_PMPCFG0 + (index >> 2);
	shift = (index & 0x3) << 3;
#endif /* CONFIG_64BIT */
	pmpaddr_csr = CSR_PMPADDR0 + index;

	/* Mask = 0x000000FF<<((index%4)*8) */
	mask = 0x000000FF << shift;

	cfg_val = cfg_val << shift;
	addr_val = TO_PMP_ADDR(addr_val);

	reg_val = csr_read_enum(pmpcfg_csr);
	reg_val = reg_val & ~mask;
	reg_val = reg_val | cfg_val;

	csr_write_enum(pmpaddr_csr, addr_val);
	csr_write_enum(pmpcfg_csr, reg_val);
	return 0;
}

int pmp_get(unsigned int index, ulong_t *cfg_val, ulong_t *addr_val)
{
	ulong_t shift;
	int pmpcfg_csr;
	int pmpaddr_csr;

	if (index >= PMP_SLOT_NUMBER) {
		return -1;
	}

	/* Calculate PMP config/addr register and shift */
#ifdef CONFIG_64BIT
	pmpcfg_csr = CSR_PMPCFG0 + (index >> 4);
	shift = (index & 0x0007) << 3;
#else
	pmpcfg_csr = CSR_PMPCFG0 + (index >> 2);
	shift = (index & 0x0003) << 3;
#endif /* CONFIG_64BIT */
	pmpaddr_csr = CSR_PMPADDR0 + index;

	*cfg_val = (csr_read_enum(pmpcfg_csr) >> shift) & 0xFF;
	*addr_val = FROM_PMP_ADDR(csr_read_enum(pmpaddr_csr));

	return 0;
}

void z_riscv_pmp_clear_config(void)
{
	for (unsigned int i = 0; i < RISCV_PMP_CFG_NUM; i++)
		csr_write_enum(CSR_PMPCFG0 + i, 0);
}

/* Function to help debug */
void z_riscv_pmp_print(unsigned int index)
{
	ulong_t cfg_val;
	ulong_t addr_val;

	if (pmp_get(index, &cfg_val, &addr_val)) {
		return;
	}
#ifdef CONFIG_64BIT
	printf("PMP[%d] :\t%02lX %16lX\n", index, cfg_val, addr_val);
#else
	printf("PMP[%d] :\t%02lX %08lX\n", index, cfg_val, addr_val);
#endif /* CONFIG_64BIT */
}

#if defined(CONFIG_USERSPACE)
#include <linker/linker-defs.h>
void z_riscv_init_user_accesses(struct k_thread *thread)
{
	unsigned char index;
	unsigned char *uchar_pmpcfg;
	ulong_t rom_start = (ulong_t) _image_rom_start;
#if defined(CONFIG_PMP_POWER_OF_TWO_ALIGNMENT)
	ulong_t rom_size = (ulong_t) _image_rom_size;
#else /* CONFIG_PMP_POWER_OF_TWO_ALIGNMENT */
	ulong_t rom_end = (ulong_t) _image_rom_end;
#endif /* CONFIG_PMP_POWER_OF_TWO_ALIGNMENT */
	index = 0U;
	uchar_pmpcfg = (unsigned char *) thread->arch.u_pmpcfg;

#ifdef CONFIG_PMP_STACK_GUARD
	index++;
#endif /* CONFIG_PMP_STACK_GUARD */

	/* MCU state */
	thread->arch.u_pmpaddr[index] = TO_PMP_ADDR((ulong_t) &is_user_mode);
	uchar_pmpcfg[index++] = PMP_NA4 | PMP_R;
#if defined(CONFIG_PMP_POWER_OF_TWO_ALIGNMENT)
	/* Program and RO data */
	thread->arch.u_pmpaddr[index] = TO_PMP_NAPOT(rom_start, rom_size);
	uchar_pmpcfg[index++] = PMP_NAPOT | PMP_R | PMP_X;

	/* RAM */
	thread->arch.u_pmpaddr[index] = TO_PMP_NAPOT(thread->stack_info.start,
					thread->stack_info.size);

	uchar_pmpcfg[index++] = PMP_NAPOT | PMP_R | PMP_W;
#else /* CONFIG_PMP_POWER_OF_TWO_ALIGNMENT */
	/* Program and RO data */
	thread->arch.u_pmpaddr[index] = TO_PMP_ADDR(rom_start);
	uchar_pmpcfg[index++] = PMP_NA4 | PMP_R | PMP_X;
	thread->arch.u_pmpaddr[index] = TO_PMP_ADDR(rom_end);
	uchar_pmpcfg[index++] = PMP_TOR | PMP_R | PMP_X;

	/* RAM */
	thread->arch.u_pmpaddr[index] = TO_PMP_ADDR(thread->stack_info.start);
	uchar_pmpcfg[index++] = PMP_NA4 | PMP_R | PMP_W;
	thread->arch.u_pmpaddr[index] = TO_PMP_ADDR(thread->stack_info.start +
		thread->stack_info.size);
	uchar_pmpcfg[index++] = PMP_TOR | PMP_R | PMP_W;
#endif /* CONFIG_PMP_POWER_OF_TWO_ALIGNMENT */
}

void z_riscv_configure_user_allowed_stack(struct k_thread *thread)
{
	unsigned int i;

	z_riscv_pmp_clear_config();

	for (i = 0U; i < CONFIG_PMP_SLOT; i++)
		csr_write_enum(CSR_PMPADDR0 + i, thread->arch.u_pmpaddr[i]);

	for (i = 0U; i < RISCV_PMP_CFG_NUM; i++)
		csr_write_enum(CSR_PMPCFG0 + i, thread->arch.u_pmpcfg[i]);
}

void z_riscv_pmp_add_dynamic(struct k_thread *thread,
			ulong_t addr,
			ulong_t size,
			unsigned char flags)
{
	unsigned char index = 0U;
	unsigned char *uchar_pmpcfg;

	/* Check 4 bytes alignment */
	__ASSERT(((addr & 0x3) == 0) && ((size & 0x3) == 0) && size,
		 "address/size are not 4 bytes aligned\n");

	/* Get next free entry */
	uchar_pmpcfg = (unsigned char *) thread->arch.u_pmpcfg;

	index = PMP_REGION_NUM_FOR_U_THREAD;

	while ((index < CONFIG_PMP_SLOT) && uchar_pmpcfg[index]) {
		index++;
	}

	__ASSERT((index < CONFIG_PMP_SLOT), "no free PMP entry\n");

	/* Select the best type */
	if (size == 4) {
		thread->arch.u_pmpaddr[index] = TO_PMP_ADDR(addr);
		uchar_pmpcfg[index] = flags | PMP_NA4;
	}
#if !defined(CONFIG_PMP_POWER_OF_TWO_ALIGNMENT)
	else if ((addr & (size - 1)) || (size & (size - 1))) {
		__ASSERT(((index + 1) < CONFIG_PMP_SLOT),
			"not enough free PMP entries\n");
		thread->arch.u_pmpaddr[index] = TO_PMP_ADDR(addr);
		uchar_pmpcfg[index++] = flags | PMP_NA4;
		thread->arch.u_pmpaddr[index] = TO_PMP_ADDR(addr + size);
		uchar_pmpcfg[index++] = flags | PMP_TOR;
	}
#endif /* !CONFIG_PMP_POWER_OF_TWO_ALIGNMENT */
	else {
		thread->arch.u_pmpaddr[index] = TO_PMP_NAPOT(addr, size);
		uchar_pmpcfg[index] = flags | PMP_NAPOT;
	}
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

void arch_mem_domain_partition_remove(struct k_mem_domain *domain,
				       uint32_t  partition_id)
{
	sys_dnode_t *node, *next_node;
	uint32_t index, i, num;
	ulong_t pmp_type, pmp_addr;
	unsigned char *uchar_pmpcfg;
	struct k_thread *thread;
	ulong_t size = (ulong_t) domain->partitions[partition_id].size;
	ulong_t start = (ulong_t) domain->partitions[partition_id].start;

	if (size == 4) {
		pmp_type = PMP_NA4;
		pmp_addr = TO_PMP_ADDR(start);
		num = 1U;
	}
#if !defined(CONFIG_PMP_POWER_OF_TWO_ALIGNMENT) || defined(CONFIG_PMP_STACK_GUARD)
	else if ((start & (size - 1)) || (size & (size - 1))) {
		pmp_type = PMP_TOR;
		pmp_addr = TO_PMP_ADDR(start + size);
		num = 2U;
	}
#endif /* CONFIG_PMP_POWER_OF_TWO_ALIGNMENT || CONFIG_PMP_STACK_GUARD */
	else {
		pmp_type = PMP_NAPOT;
		pmp_addr = TO_PMP_NAPOT(start, size);
		num = 1U;
	}

	node = sys_dlist_peek_head(&domain->mem_domain_q);
	if (!node) {
		return;
	}

	thread = CONTAINER_OF(node, struct k_thread, mem_domain_info);

	uchar_pmpcfg = (unsigned char *) thread->arch.u_pmpcfg;
	for (index = PMP_REGION_NUM_FOR_U_THREAD;
		index < CONFIG_PMP_SLOT;
		index++) {
		if (((uchar_pmpcfg[index] & PMP_TYPE_MASK) == pmp_type) &&
			(pmp_addr == thread->arch.u_pmpaddr[index])) {
			break;
		}
	}

	__ASSERT((index < CONFIG_PMP_SLOT), "partition not found\n");

#if !defined(CONFIG_PMP_POWER_OF_TWO_ALIGNMENT) || defined(CONFIG_PMP_STACK_GUARD)
	if (pmp_type == PMP_TOR) {
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
}

void arch_mem_domain_thread_add(struct k_thread *thread)
{
	struct k_mem_partition *partition;

	for (int i = 0, pcount = 0;
		pcount < thread->mem_domain_info.mem_domain->num_partitions;
		i++) {
		partition = &thread->mem_domain_info.mem_domain->partitions[i];
		if (partition->size == 0) {
			continue;
		}
		pcount++;

		z_riscv_pmp_add_dynamic(thread, (ulong_t) partition->start,
			(ulong_t) partition->size, partition->attr.pmp_attr);
	}
}

void arch_mem_domain_partition_add(struct k_mem_domain *domain,
				    uint32_t partition_id)
{
	sys_dnode_t *node, *next_node;
	struct k_thread *thread;
	struct k_mem_partition *partition;

	partition = &domain->partitions[partition_id];

	SYS_DLIST_FOR_EACH_NODE_SAFE(&domain->mem_domain_q, node, next_node) {
		thread = CONTAINER_OF(node, struct k_thread, mem_domain_info);

		z_riscv_pmp_add_dynamic(thread, (ulong_t) partition->start,
			(ulong_t) partition->size, partition->attr.pmp_attr);
	}
}

void arch_mem_domain_thread_remove(struct k_thread *thread)
{
	uint32_t i;
	unsigned char *uchar_pmpcfg;

	uchar_pmpcfg = (unsigned char *) thread->arch.u_pmpcfg;

	for (i = PMP_REGION_NUM_FOR_U_THREAD; i < CONFIG_PMP_SLOT; i++) {
		uchar_pmpcfg[i] = 0U;
	}
}

#endif /* CONFIG_USERSPACE */

#ifdef CONFIG_PMP_STACK_GUARD

void z_riscv_init_stack_guard(struct k_thread *thread)
{
	unsigned char index = 0U;
	unsigned char *uchar_pmpcfg;
	ulong_t stack_guard_addr;

	uchar_pmpcfg = (unsigned char *) thread->arch.s_pmpcfg;

	uchar_pmpcfg++;

	/* stack guard: None */
	thread->arch.s_pmpaddr[index] = TO_PMP_ADDR(thread->stack_info.start);
	uchar_pmpcfg[index++] = PMP_NA4;
	thread->arch.s_pmpaddr[index] =
		TO_PMP_ADDR(thread->stack_info.start +
			PMP_GUARD_ALIGN_AND_SIZE);
	uchar_pmpcfg[index++] = PMP_TOR;

#ifdef CONFIG_USERSPACE
	if (thread->arch.priv_stack_start) {
#ifdef CONFIG_PMP_POWER_OF_TWO_ALIGNMENT
		stack_guard_addr = thread->arch.priv_stack_start;
#else
		stack_guard_addr = (ulong_t) thread->stack_obj;
#endif /* CONFIG_PMP_POWER_OF_TWO_ALIGNMENT */
		thread->arch.s_pmpaddr[index] =
			TO_PMP_ADDR(stack_guard_addr);
		uchar_pmpcfg[index++] = PMP_NA4;
		thread->arch.s_pmpaddr[index] =
			TO_PMP_ADDR(stack_guard_addr +
				PMP_GUARD_ALIGN_AND_SIZE);
		uchar_pmpcfg[index++] = PMP_TOR;
	}
#endif /* CONFIG_USERSPACE */

	/* RAM: RW */
	thread->arch.s_pmpaddr[index] = TO_PMP_ADDR(CONFIG_SRAM_BASE_ADDRESS |
				TO_NAPOT_RANGE(KB(CONFIG_SRAM_SIZE)));
	uchar_pmpcfg[index++] = (PMP_NAPOT | PMP_R | PMP_W);

	/* All other memory: RWX */
#ifdef CONFIG_64BIT
	thread->arch.s_pmpaddr[index] = 0x1FFFFFFFFFFFFFFF;
#else
	thread->arch.s_pmpaddr[index] = 0x1FFFFFFF;
#endif /* CONFIG_64BIT */
	uchar_pmpcfg[index] = PMP_NAPOT | PMP_R | PMP_W | PMP_X;
}

void z_riscv_configure_stack_guard(struct k_thread *thread)
{
	unsigned int i;

	/* Disable PMP for machine mode */
	csr_clear(mstatus, MSTATUS_MPRV);

	z_riscv_pmp_clear_config();

	for (i = 0U; i < PMP_REGION_NUM_FOR_STACK_GUARD; i++)
		csr_write_enum(CSR_PMPADDR1 + i, thread->arch.s_pmpaddr[i]);

	for (i = 0U; i < PMP_CFG_CSR_NUM_FOR_STACK_GUARD; i++)
		csr_write_enum(CSR_PMPCFG0 + i, thread->arch.s_pmpcfg[i]);

	/* Enable PMP for machine mode */
	csr_set(mstatus, MSTATUS_MPRV);
}

void z_riscv_configure_interrupt_stack_guard(void)
{
	if (PMP_GUARD_ALIGN_AND_SIZE > 4) {
		z_riscv_pmp_set(0, PMP_NAPOT | PMP_L,
			(ulong_t) z_interrupt_stacks[0] |
			TO_NAPOT_RANGE(PMP_GUARD_ALIGN_AND_SIZE));
	} else {
		z_riscv_pmp_set(0, PMP_NA4 | PMP_L,
			(ulong_t) z_interrupt_stacks[0]);
	}
}
#endif /* CONFIG_PMP_STACK_GUARD */

#if defined(CONFIG_PMP_STACK_GUARD) || defined(CONFIG_USERSPACE)

void z_riscv_pmp_init_thread(struct k_thread *thread)
{
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
