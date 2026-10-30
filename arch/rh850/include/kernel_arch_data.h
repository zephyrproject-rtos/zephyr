/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_RH850_INCLUDE_KERNEL_ARCH_DATA_H_
#define ZEPHYR_ARCH_RH850_INCLUDE_KERNEL_ARCH_DATA_H_

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>

/* GCC prefixes C symbol names with "_" */
#define RH850_C_SYM(sym) _CONCAT(__USER_LABEL_PREFIX__, sym)

/* EIIC is 40H + vector for TRAP0, 1000H + channel for EIINT */
#define RH850_EIIC_TRAP0 0x40
#define RH850_EIIC_EIINT 0x1000

#define RH850_TRAP_IRQ_OFFLOAD 0
#define RH850_TRAP_SWITCH      1

#define RH850_EXC_LEVEL_EI 0
#define RH850_EXC_LEVEL_FE 1

/* UCPOP cause code of coprocessor 1 (FXU) */
#define RH850_FEIC_UCPOP_FXU 0x81

/* PSW of a new thread */
#if CONFIG_NUM_IRQ_PRIO_LEVELS > 16
#define RH850_PSW_THREAD (RH850_PSW_EBV | RH850_PSW_CU0 | RH850_PSW_EIMASK_ALL)
#else
#define RH850_PSW_THREAD (RH850_PSW_EBV | RH850_PSW_CU0)
#endif

#endif /* ZEPHYR_ARCH_RH850_INCLUDE_KERNEL_ARCH_DATA_H_ */
