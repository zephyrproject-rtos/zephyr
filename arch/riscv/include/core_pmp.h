/*
 * Copyright (c) 2020 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CORE_PMP_H_
#define CORE_PMP_H_

/* Configuration register flags (cfg_val)*/
#define PMP_R		0x01	/* Allow read */
#define PMP_W		0x02	/* Allow write */
#define PMP_X		0x04	/* Allow execute */
#define PMP_A		0x18	/* Address-matching mode field */
#define PMP_L		0x80	/* PMP entry is locked */
#define PMP_OFF		0x00	/* Null region */
#define PMP_TOR		0x08	/* Top of range */
#define PMP_NA4		0x10	/* Naturally aligned four-byte region */
#define PMP_NAPOT	0x18	/* Naturally aligned power-of-two region */

#define PMP_SHIFT_ADDR	2
#define PMP_TYPE_MASK		0x18
#define TO_PMP_ADDR(addr)	((addr) >> PMP_SHIFT_ADDR)
#define FROM_PMP_ADDR(addr)	((addr) << PMP_SHIFT_ADDR)
#define TO_NAPOT_RANGE(size)	(((size) - 1) >> 1)
#define TO_PMP_NAPOT(addr, size)	TO_PMP_ADDR(addr | \
					TO_NAPOT_RANGE(size))

#ifdef CONFIG_RISCV_PMP

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
int z_riscv_pmp_set(unsigned int index, ulong_t cfg_val, ulong_t addr_val);

/*
 * @brief Reset to 0 all PMP setup registers
 */
void z_riscv_pmp_clear_config(void);

/*
 * @brief Print PMP setup register for info/debug
 */
void z_riscv_pmp_print(unsigned int index);
#endif /* CONFIG_RISCV_PMP */

#endif /* CORE_PMP_H_ */
