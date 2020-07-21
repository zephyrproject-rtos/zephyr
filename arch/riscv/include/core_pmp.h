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

#ifdef CONFIG_PMP_STACK_GUARD

#define PMP_GUARD_ALIGN_AND_SIZE CONFIG_PMP_STACK_GUARD_MIN_SIZE

#else

#define PMP_GUARD_ALIGN_AND_SIZE 0

#endif /* CONFIG_PMP_STACK_GUARD */

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

#if defined(CONFIG_USERSPACE)

/*
 * @brief Configure RISCV user thread access to the stack
 *
 * Determine and save allow access setup in thread structure.
 *
 * @param thread Thread info data pointer.
 */
void z_riscv_init_user_accesses(struct k_thread *thread);

/*
 * @brief Apply RISCV user thread access to the stack
 *
 * Write user access setup saved in this thread structure.
 *
 * @param thread Thread info data pointer.
 */
void z_riscv_configure_user_allowed_stack(struct k_thread *thread);

/*
 * @brief Add a new RISCV stack access
 *
 * Add a new memory permission area in the existing
 * pmp setup of the thread.
 *
 * @param thread Thread info data pointer.
 * @param addr   Start address of the memory area.
 * @param size   Size of the memory area.
 * @param flags  Pemissions: PMP_R, PMP_W, PMP_X, PMP_L
 */
void z_riscv_pmp_add_dynamic(struct k_thread *thread,
			ulong_t addr,
			ulong_t size,
			unsigned char flags);
#endif /* CONFIG_USERSPACE */

#ifdef CONFIG_PMP_STACK_GUARD

/*
 * @brief Configure RISCV stack guard for interrupt stack
 *
 * Write PMP registers to prevent RWX access from all privilege mode.
 */
void z_riscv_configure_interrupt_stack_guard(void);

/*
 * @brief Configure RISCV stack guard
 *
 * Determine and save stack guard setup in thread structure.
 *
 * @param thread Thread info data pointer.
 */
void z_riscv_init_stack_guard(struct k_thread *thread);

/*
 * @brief Apply RISCV stack guard
 *
 * Write stack guard setup saved in this thread structure.
 *
 * @param thread Thread info data pointer.
 */
void z_riscv_configure_stack_guard(struct k_thread *thread);
#endif /* CONFIG_PMP_STACK_GUARD */

#if defined(CONFIG_PMP_STACK_GUARD) || defined(CONFIG_USERSPACE)

/*
 * @brief Initialize thread PMP setup value to 0
 *
 * @param thread Thread info data pointer.
 */
void z_riscv_pmp_init_thread(struct k_thread *thread);
#endif /* CONFIG_PMP_STACK_GUARD || CONFIG_USERSPACE */

#endif /* CORE_PMP_H_ */
