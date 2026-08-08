/*
 * Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_RISCV_CORE_SMP_BOOT_H_
#define ZEPHYR_ARCH_RISCV_CORE_SMP_BOOT_H_

/*
 * Layout of one per-CPU boot handshake slot in riscv_cpu_boot_slots[], shared
 * between smp.c and reset.S; pinned by BUILD_ASSERT() in smp.c.
 */

#if defined(CONFIG_64BIT)
#define RISCV_CPU_BOOT_SLOT_REG_SIZE 8
#else
#define RISCV_CPU_BOOT_SLOT_REG_SIZE 4
#endif

#define RISCV_CPU_BOOT_SLOT_WAKE_FLAG_OFF 0
#define RISCV_CPU_BOOT_SLOT_HARTID_OFF    (1 * RISCV_CPU_BOOT_SLOT_REG_SIZE)
#define RISCV_CPU_BOOT_SLOT_SP_OFF        (2 * RISCV_CPU_BOOT_SLOT_REG_SIZE)
#define RISCV_CPU_BOOT_SLOT_FN_OFF        (3 * RISCV_CPU_BOOT_SLOT_REG_SIZE)
#define RISCV_CPU_BOOT_SLOT_ARG_OFF       (4 * RISCV_CPU_BOOT_SLOT_REG_SIZE)
#define RISCV_CPU_BOOT_SLOT_SIZE          (5 * RISCV_CPU_BOOT_SLOT_REG_SIZE)

#endif /* ZEPHYR_ARCH_RISCV_CORE_SMP_BOOT_H_ */
