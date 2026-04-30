/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Internal eBPF VM interfaces.
 */

#ifndef ZEPHYR_SUBSYS_EBPF_VM_H_
#define ZEPHYR_SUBSYS_EBPF_VM_H_

#include <zephyr/ebpf/ebpf_insn.h>
#include <zephyr/ebpf/ebpf_prog.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ebpf_vm_ctx {
	uint64_t regs[EBPF_NUM_REGS];
	uint32_t pc;
	uint8_t  stack[CONFIG_EBPF_STACK_SIZE];
};

int64_t ebpf_vm_exec(const struct ebpf_prog *prog, void *ctx_data, uint32_t ctx_size);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_EBPF_VM_H_ */
