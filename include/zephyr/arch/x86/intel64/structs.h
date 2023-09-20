/*
 * Copyright (c) 2023 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_STRUCTS_H_
#define ZEPHYR_INCLUDE_ARCH_X86_STRUCTS_H_

enum x86_cpu_type {
	CPU_TYPE_ATOM = 0x20,  /* CPU type Intel Atom */
	CPU_TYPE_CORE = 0x40,  /* CPU type Intel Core */
	CPU_TYPE_UNKNOWN = 0xFFFF, /* CPU type unsupported */
};

struct x86_cpu_info {
	uint8_t cpu_id;         /* ID for software to uniquely identify a CPU */
	uint32_t apic_id;       /* Local APIC ID for HW to uniquely identify a CPU */
	uint32_t family: 8;     /* Signature value for identify processor family */
	uint32_t model: 8;      /* Signature value for identify processor model */
	uint32_t stepping: 4;   /* Signature value for identify processor stepping */
	uint32_t hybrid: 1;     /* Set if SOC support hybrid core (have both Atom & Core) */
	uint32_t bsp: 1;        /* Set if core is the BSP (Boot Strap Processor) */
	enum x86_cpu_type type; /* Identify type of CPU (such as Atom or core) */
};

struct _cpu_arch {
	struct x86_cpu_info info;
};

#endif /* ZEPHYR_INCLUDE_ARCH_X86_STRUCTS_H_ */
