/*
 * Copyright (c) 2020 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_internal.h>
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

	if ((index >= PMP_SLOT_NUMBER) | (index < 0))
		return -1;

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

	if ((index >= PMP_SLOT_NUMBER) | (index < 0))
		return -1;

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

	if (pmp_get(index, &cfg_val, &addr_val))
		return;
#ifdef CONFIG_64BIT
	printf("PMP[%d] :\t%02lX %16lX\n", index, cfg_val, addr_val);
#else
	printf("PMP[%d] :\t%02lX %08lX\n", index, cfg_val, addr_val);
#endif /* CONFIG_64BIT */
}
