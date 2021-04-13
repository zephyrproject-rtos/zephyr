/*
 * Copyright (c) 2021 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief tpidrro_el0 bits allocation
 *
 * Among other things, the tpidrro_el0 holds the address for the current
 * CPU's struct _cpu instance. But such a pointer is at least 8-bytes
 * aligned, and the address space is 48 bits max. That leaves plenty of
 * free bits for other purposes.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM64_TPIDRRO_EL0_H_
#define ZEPHYR_INCLUDE_ARCH_ARM64_TPIDRRO_EL0_H_

#define TPIDRROEL0_IN_EL0	0x0000000000000001

#define TPIDRROEL0_CURR_CPU	0x0000fffffffffff8

#define TPIDRROEL0_EXC_DEPTH	0xff00000000000000
#define TPIDRROEL0_EXC_UNIT	0x0100000000000000
#define TPIDRROEL0_EXC_SHIFT	56

#endif /* ZEPHYR_INCLUDE_ARCH_ARM64_TPIDRRO_EL0_H_ */
