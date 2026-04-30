/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/iterable_sections.h>
#include <string.h>

#include <zephyr/ebpf/ebpf_helpers.h>
#include <zephyr/ebpf/ebpf_map.h>
#include "ebpf_vm.h"

LOG_MODULE_REGISTER(ebpf_vm, CONFIG_EBPF_LOG_LEVEL);

/**
 * Initialize a fresh VM context for one program invocation.
 *
 * The VM state is ephemeral: registers start at zero, R1 receives the
 * event/context pointer, and R10 points to the top of the private stack.
 */
static inline void ebpf_vm_init(struct ebpf_vm_ctx *vm, void *ctx_data)
{
	memset(vm->regs, 0, sizeof(vm->regs));
	vm->regs[EBPF_REG_R1] = (uint64_t)(uintptr_t)ctx_data;
	vm->regs[EBPF_REG_R10] = (uint64_t)(uintptr_t)&vm->stack[CONFIG_EBPF_STACK_SIZE];
	vm->pc = 0;
}

#ifdef CONFIG_EBPF_RUNTIME_BOUNDS_CHECK
/**
 * Validate register indices before using them in the hot execution path.
 */
static inline int ebpf_vm_validate_regs(const struct ebpf_prog *prog,
					uint32_t pc, uint8_t dst, uint8_t src)
{
	if (dst >= EBPF_NUM_REGS || src >= EBPF_NUM_REGS) {
		LOG_ERR("Invalid register in '%s' at PC=%u", prog->name, pc);
		return -EINVAL;
	}

	return 0;
}

static bool ebpf_vm_region_contains(uintptr_t base, uint32_t region_size,
				    uintptr_t addr, uint32_t size)
{
	uintptr_t offset;

	if (region_size != 0U && (base + (uintptr_t)region_size) < base) {
		return false;
	}

	if (addr < base || size > region_size) {
		return false;
	}

	offset = addr - base;

	return offset <= (uintptr_t)(region_size - size);
}

/**
 * Check that a memory access falls within allowed regions:
 * (a) the VM stack, or
 * (b) the context buffer, or
 * (c) any registered eBPF map's data region.
 */
static bool ebpf_vm_mem_check(const struct ebpf_vm_ctx *vm,
			      uintptr_t addr, uint32_t size,
			      const void *ctx_data, uint32_t ctx_size)
{
	uintptr_t stack_base = (uintptr_t)vm->stack;

	/* Check stack region */
	if (ebpf_vm_region_contains(stack_base, CONFIG_EBPF_STACK_SIZE, addr, size)) {
		return true;
	}

	/* Check context region */
	if (ctx_data != NULL) {
		uintptr_t ctx_base = (uintptr_t)ctx_data;

		if (ebpf_vm_region_contains(ctx_base, ctx_size, addr, size)) {
			return true;
		}
	}

	STRUCT_SECTION_FOREACH(ebpf_map, map) {
		uintptr_t map_base;

		if (map->data == NULL || map->data_size == 0U) {
			continue;
		}

		map_base = (uintptr_t)map->data;

		if (ebpf_vm_region_contains(map_base, map->data_size, addr, size)) {
			return true;
		}
	}

	return false;
}

/**
 * Check one resolved memory access against the VM stack, context, and maps.
 */
static inline int ebpf_vm_check_access(const struct ebpf_prog *prog,
				       const struct ebpf_vm_ctx *vm,
				       uintptr_t addr, uint32_t size,
				       const void *ctx_data, uint32_t ctx_size,
				       bool is_write)
{
	if (!ebpf_vm_mem_check(vm, addr, size, ctx_data, ctx_size)) {
		LOG_ERR("OOB %s in '%s' at PC=%u", is_write ? "write" : "read",
			prog->name, vm->pc - 1);
		return -EFAULT;
	}

	return 0;
}

#endif /* CONFIG_EBPF_RUNTIME_BOUNDS_CHECK */

/**
 * Load a scalar value from an address already derived from VM registers.
 */
static inline int ebpf_vm_load_mem(const struct ebpf_prog *prog,
				   struct ebpf_vm_ctx *vm,
				   uintptr_t addr, uint32_t size,
				   const void *ctx_data, uint32_t ctx_size,
				   uint64_t *value)
{
#ifdef CONFIG_EBPF_RUNTIME_BOUNDS_CHECK
	int ret = ebpf_vm_check_access(prog, vm, addr, size, ctx_data, ctx_size, false);

	if (ret != 0) {
		return ret;
	}
#endif

	switch (size) {
	case 1:
		*value = *(uint8_t *)addr;
		return 0;
	case 2:
		*value = *(uint16_t *)addr;
		return 0;
	case 4:
		*value = *(uint32_t *)addr;
		return 0;
	case 8:
		*value = *(uint64_t *)addr;
		return 0;
	default:
		return -EINVAL;
	}
}

/**
 * Store a scalar value to an address already derived from VM registers.
 */
static inline int ebpf_vm_store_mem(const struct ebpf_prog *prog,
				    struct ebpf_vm_ctx *vm,
				    uintptr_t addr, uint32_t size, uint64_t value,
				    const void *ctx_data, uint32_t ctx_size)
{
#ifdef CONFIG_EBPF_RUNTIME_BOUNDS_CHECK
	int ret = ebpf_vm_check_access(prog, vm, addr, size, ctx_data, ctx_size, true);

	if (ret != 0) {
		return ret;
	}
#endif

	switch (size) {
	case 1:
		*(uint8_t *)addr = (uint8_t)value;
		return 0;
	case 2:
		*(uint16_t *)addr = (uint16_t)value;
		return 0;
	case 4:
		*(uint32_t *)addr = (uint32_t)value;
		return 0;
	case 8:
		*(uint64_t *)addr = value;
		return 0;
	default:
		return -EINVAL;
	}
}

/**
 * Execute one 64-bit ALU instruction.
 */
static inline int ebpf_vm_exec_alu64(struct ebpf_vm_ctx *vm,
				     const struct ebpf_insn *insn,
				     uint8_t opcode, uint8_t dst, uint8_t src)
{
	switch (opcode) {
	case EBPF_OP_ADD64_IMM:
		vm->regs[dst] += (int64_t)insn->imm;
		return 0;
	case EBPF_OP_ADD64_REG:
		vm->regs[dst] += vm->regs[src];
		return 0;
	case EBPF_OP_SUB64_IMM:
		vm->regs[dst] -= (int64_t)insn->imm;
		return 0;
	case EBPF_OP_SUB64_REG:
		vm->regs[dst] -= vm->regs[src];
		return 0;
	case EBPF_OP_MUL64_IMM:
		vm->regs[dst] *= (int64_t)insn->imm;
		return 0;
	case EBPF_OP_MUL64_REG:
		vm->regs[dst] *= vm->regs[src];
		return 0;
	case EBPF_OP_DIV64_IMM:
		if (insn->imm == 0) {
			vm->regs[dst] = 0;
		} else {
			vm->regs[dst] /= (uint64_t)(uint32_t)insn->imm;
		}
		return 0;
	case EBPF_OP_DIV64_REG:
		if (vm->regs[src] == 0) {
			vm->regs[dst] = 0;
		} else {
			vm->regs[dst] /= vm->regs[src];
		}
		return 0;
	case EBPF_OP_MOD64_IMM:
		if (insn->imm == 0) {
			vm->regs[dst] = 0;
		} else {
			vm->regs[dst] %= (uint64_t)(uint32_t)insn->imm;
		}
		return 0;
	case EBPF_OP_MOD64_REG:
		if (vm->regs[src] == 0) {
			vm->regs[dst] = 0;
		} else {
			vm->regs[dst] %= vm->regs[src];
		}
		return 0;
	case EBPF_OP_OR64_IMM:
		vm->regs[dst] |= (int64_t)insn->imm;
		return 0;
	case EBPF_OP_OR64_REG:
		vm->regs[dst] |= vm->regs[src];
		return 0;
	case EBPF_OP_AND64_IMM:
		vm->regs[dst] &= (int64_t)insn->imm;
		return 0;
	case EBPF_OP_AND64_REG:
		vm->regs[dst] &= vm->regs[src];
		return 0;
	case EBPF_OP_XOR64_IMM:
		vm->regs[dst] ^= (int64_t)insn->imm;
		return 0;
	case EBPF_OP_XOR64_REG:
		vm->regs[dst] ^= vm->regs[src];
		return 0;
	case EBPF_OP_LSH64_IMM:
		vm->regs[dst] <<= (uint32_t)insn->imm;
		return 0;
	case EBPF_OP_LSH64_REG:
		vm->regs[dst] <<= (uint32_t)vm->regs[src];
		return 0;
	case EBPF_OP_RSH64_IMM:
		vm->regs[dst] >>= (uint32_t)insn->imm;
		return 0;
	case EBPF_OP_RSH64_REG:
		vm->regs[dst] >>= (uint32_t)vm->regs[src];
		return 0;
	case EBPF_OP_NEG64:
		vm->regs[dst] = (uint64_t)(-(int64_t)vm->regs[dst]);
		return 0;
	case EBPF_OP_MOV64_IMM:
		vm->regs[dst] = (uint64_t)(int64_t)insn->imm;
		return 0;
	case EBPF_OP_MOV64_REG:
		vm->regs[dst] = vm->regs[src];
		return 0;
	default:
		return -EINVAL;
	}
}

/**
 * Execute one 32-bit ALU instruction and zero-extend the result to 64 bits.
 */
static inline int ebpf_vm_exec_alu32(struct ebpf_vm_ctx *vm,
				     const struct ebpf_insn *insn,
				     uint8_t opcode, uint8_t dst, uint8_t src)
{
	switch (opcode) {
	case EBPF_OP_ADD_IMM:
		vm->regs[dst] = (uint32_t)((uint32_t)vm->regs[dst] + (uint32_t)insn->imm);
		return 0;
	case EBPF_OP_ADD_REG:
		vm->regs[dst] = (uint32_t)((uint32_t)vm->regs[dst] + (uint32_t)vm->regs[src]);
		return 0;
	case EBPF_OP_SUB_IMM:
		vm->regs[dst] = (uint32_t)((uint32_t)vm->regs[dst] - (uint32_t)insn->imm);
		return 0;
	case EBPF_OP_SUB_REG:
		vm->regs[dst] = (uint32_t)((uint32_t)vm->regs[dst] - (uint32_t)vm->regs[src]);
		return 0;
	case EBPF_OP_MUL_IMM:
		vm->regs[dst] = (uint32_t)((uint32_t)vm->regs[dst] * (uint32_t)insn->imm);
		return 0;
	case EBPF_OP_MUL_REG:
		vm->regs[dst] = (uint32_t)((uint32_t)vm->regs[dst] * (uint32_t)vm->regs[src]);
		return 0;
	case EBPF_OP_DIV_IMM:
		if (insn->imm == 0) {
			vm->regs[dst] = 0;
		} else {
			vm->regs[dst] = (uint32_t)vm->regs[dst] / (uint32_t)insn->imm;
		}
		return 0;
	case EBPF_OP_DIV_REG:
		if ((uint32_t)vm->regs[src] == 0) {
			vm->regs[dst] = 0;
		} else {
			vm->regs[dst] = (uint32_t)vm->regs[dst] / (uint32_t)vm->regs[src];
		}
		return 0;
	case EBPF_OP_MOD_IMM:
		if (insn->imm == 0) {
			vm->regs[dst] = 0;
		} else {
			vm->regs[dst] = (uint32_t)vm->regs[dst] % (uint32_t)insn->imm;
		}
		return 0;
	case EBPF_OP_MOD_REG:
		if ((uint32_t)vm->regs[src] == 0) {
			vm->regs[dst] = 0;
		} else {
			vm->regs[dst] = (uint32_t)vm->regs[dst] % (uint32_t)vm->regs[src];
		}
		return 0;
	case EBPF_OP_OR_IMM:
		vm->regs[dst] = (uint32_t)((uint32_t)vm->regs[dst] | (uint32_t)insn->imm);
		return 0;
	case EBPF_OP_OR_REG:
		vm->regs[dst] = (uint32_t)((uint32_t)vm->regs[dst] | (uint32_t)vm->regs[src]);
		return 0;
	case EBPF_OP_AND_IMM:
		vm->regs[dst] = (uint32_t)((uint32_t)vm->regs[dst] & (uint32_t)insn->imm);
		return 0;
	case EBPF_OP_AND_REG:
		vm->regs[dst] = (uint32_t)((uint32_t)vm->regs[dst] & (uint32_t)vm->regs[src]);
		return 0;
	case EBPF_OP_XOR_IMM:
		vm->regs[dst] = (uint32_t)((uint32_t)vm->regs[dst] ^ (uint32_t)insn->imm);
		return 0;
	case EBPF_OP_XOR_REG:
		vm->regs[dst] = (uint32_t)((uint32_t)vm->regs[dst] ^ (uint32_t)vm->regs[src]);
		return 0;
	case EBPF_OP_LSH_IMM:
		vm->regs[dst] = (uint32_t)((uint32_t)vm->regs[dst] << (uint32_t)insn->imm);
		return 0;
	case EBPF_OP_LSH_REG:
		vm->regs[dst] = (uint32_t)((uint32_t)vm->regs[dst] << (uint32_t)vm->regs[src]);
		return 0;
	case EBPF_OP_RSH_IMM:
		vm->regs[dst] = (uint32_t)((uint32_t)vm->regs[dst] >> (uint32_t)insn->imm);
		return 0;
	case EBPF_OP_RSH_REG:
		vm->regs[dst] = (uint32_t)((uint32_t)vm->regs[dst] >> (uint32_t)vm->regs[src]);
		return 0;
	case EBPF_OP_NEG:
		vm->regs[dst] = (uint32_t)(-(int32_t)vm->regs[dst]);
		return 0;
	case EBPF_OP_MOV_IMM:
		vm->regs[dst] = (uint32_t)insn->imm;
		return 0;
	case EBPF_OP_MOV_REG:
		vm->regs[dst] = (uint32_t)vm->regs[src];
		return 0;
	default:
		return -EINVAL;
	}
}

/**
 * Execute one memory instruction across LDX, STX, and ST families.
 */
static inline int ebpf_vm_exec_memory(const struct ebpf_prog *prog,
				      struct ebpf_vm_ctx *vm,
				      const struct ebpf_insn *insn,
				      uint8_t opcode, uint8_t dst, uint8_t src,
				      const void *ctx_data, uint32_t ctx_size)
{
	uintptr_t addr;
	uint32_t size;
	uint64_t value;

	switch (opcode) {
	case EBPF_OP_LDX_B: case EBPF_OP_STX_B: case EBPF_OP_ST_B:
		size = 1;
		break;
	case EBPF_OP_LDX_H: case EBPF_OP_STX_H: case EBPF_OP_ST_H:
		size = 2;
		break;
	case EBPF_OP_LDX_W: case EBPF_OP_STX_W: case EBPF_OP_ST_W:
		size = 4;
		break;
	case EBPF_OP_LDX_DW: case EBPF_OP_STX_DW: case EBPF_OP_ST_DW:
		size = 8;
		break;
	default:
		return -EINVAL;
	}

	if (EBPF_INSN_CLASS(opcode) == EBPF_CLS_LDX) {
		addr = (uintptr_t)(vm->regs[src] + insn->offset);
		if (ebpf_vm_load_mem(prog, vm, addr, size, ctx_data, ctx_size, &value) != 0) {
			return -EFAULT;
		}
		vm->regs[dst] = value;
		return 0;
	}

	addr = (uintptr_t)(vm->regs[dst] + insn->offset);
	value = (EBPF_INSN_CLASS(opcode) == EBPF_CLS_ST) ?
		(uint64_t)(int64_t)insn->imm : vm->regs[src];

	return ebpf_vm_store_mem(prog, vm, addr, size, value, ctx_data, ctx_size);
}

/**
 * Execute one conditional or unconditional jump instruction.
 */
static inline int ebpf_vm_exec_jump(struct ebpf_vm_ctx *vm,
				    const struct ebpf_insn *insn,
				    uint8_t opcode, uint8_t dst, uint8_t src)
{
	switch (opcode) {
	case EBPF_OP_JA:
		vm->pc += insn->offset;
		return 0;
	case EBPF_OP_JEQ_IMM:
		if (vm->regs[dst] == (uint64_t)(int64_t)insn->imm) {
			vm->pc += insn->offset;
		}
		return 0;
	case EBPF_OP_JEQ_REG:
		if (vm->regs[dst] == vm->regs[src]) {
			vm->pc += insn->offset;
		}
		return 0;
	case EBPF_OP_JGT_IMM:
		if (vm->regs[dst] > (uint64_t)(int64_t)insn->imm) {
			vm->pc += insn->offset;
		}
		return 0;
	case EBPF_OP_JGT_REG:
		if (vm->regs[dst] > vm->regs[src]) {
			vm->pc += insn->offset;
		}
		return 0;
	case EBPF_OP_JGE_IMM:
		if (vm->regs[dst] >= (uint64_t)(int64_t)insn->imm) {
			vm->pc += insn->offset;
		}
		return 0;
	case EBPF_OP_JGE_REG:
		if (vm->regs[dst] >= vm->regs[src]) {
			vm->pc += insn->offset;
		}
		return 0;
	case EBPF_OP_JSET_IMM:
		if (vm->regs[dst] & (uint64_t)(int64_t)insn->imm) {
			vm->pc += insn->offset;
		}
		return 0;
	case EBPF_OP_JSET_REG:
		if (vm->regs[dst] & vm->regs[src]) {
			vm->pc += insn->offset;
		}
		return 0;
	case EBPF_OP_JNE_IMM:
		if (vm->regs[dst] != (uint64_t)(int64_t)insn->imm) {
			vm->pc += insn->offset;
		}
		return 0;
	case EBPF_OP_JNE_REG:
		if (vm->regs[dst] != vm->regs[src]) {
			vm->pc += insn->offset;
		}
		return 0;
	case EBPF_OP_JLT_IMM:
		if (vm->regs[dst] < (uint64_t)(int64_t)insn->imm) {
			vm->pc += insn->offset;
		}
		return 0;
	case EBPF_OP_JLT_REG:
		if (vm->regs[dst] < vm->regs[src]) {
			vm->pc += insn->offset;
		}
		return 0;
	case EBPF_OP_JLE_IMM:
		if (vm->regs[dst] <= (uint64_t)(int64_t)insn->imm) {
			vm->pc += insn->offset;
		}
		return 0;
	case EBPF_OP_JLE_REG:
		if (vm->regs[dst] <= vm->regs[src]) {
			vm->pc += insn->offset;
		}
		return 0;
	default:
		return -EINVAL;
	}
}

/**
 * Execute one helper call and place the helper return value in R0.
 */
static inline int ebpf_vm_exec_call(const struct ebpf_prog *prog,
				    struct ebpf_vm_ctx *vm,
				    const struct ebpf_insn *insn)
{
	ebpf_helper_fn fn = ebpf_get_helper(insn->imm);

	if (fn == NULL) {
		LOG_ERR("Unknown helper %d in '%s'", insn->imm, prog->name);
		return -EINVAL;
	}

	vm->regs[0] = fn(vm->regs[1], vm->regs[2], vm->regs[3], vm->regs[4], vm->regs[5]);
	return 0;
}

/**
 * Dispatch one decoded instruction to its handler based on instruction class.
 */
static inline int ebpf_vm_exec_insn(const struct ebpf_prog *prog,
				    struct ebpf_vm_ctx *vm,
				    const struct ebpf_insn *insn,
				    uint8_t opcode, uint8_t dst, uint8_t src,
				    const void *ctx_data, uint32_t ctx_size)
{
	int ret;

	switch (EBPF_INSN_CLASS(opcode)) {
	case EBPF_CLS_ALU64:
		ret = ebpf_vm_exec_alu64(vm, insn, opcode, dst, src);
		break;
	case EBPF_CLS_ALU:
		ret = ebpf_vm_exec_alu32(vm, insn, opcode, dst, src);
		break;
	case EBPF_CLS_LDX:
	case EBPF_CLS_STX:
	case EBPF_CLS_ST:
		ret = ebpf_vm_exec_memory(prog, vm, insn, opcode, dst, src, ctx_data, ctx_size);
		break;
	case EBPF_CLS_JMP:
		if (opcode == EBPF_OP_CALL) {
			return ebpf_vm_exec_call(prog, vm, insn);
		}
		if (opcode == EBPF_OP_EXIT) {
			return 1;
		}
		ret = ebpf_vm_exec_jump(vm, insn, opcode, dst, src);
		break;
	default:
		LOG_ERR("Unknown opcode 0x%02x in '%s' at PC=%u", opcode, prog->name, vm->pc - 1);
		return -EINVAL;
	}

	if (ret == -EINVAL) {
		LOG_ERR("Unknown opcode 0x%02x in '%s' at PC=%u", opcode, prog->name, vm->pc - 1);
	}

	return ret;
}

int64_t ebpf_vm_exec(const struct ebpf_prog *prog, void *ctx_data, uint32_t ctx_size)
{
	struct ebpf_vm_ctx vm;
	const struct ebpf_insn *insns = prog->insns;
	uint32_t insn_cnt = prog->insn_cnt;
	uint32_t insn_limit = CONFIG_EBPF_MAX_INSNS_PER_RUN;
	uint32_t insn_executed = 0;

	ebpf_vm_init(&vm, ctx_data);

	/* Main VM execution loop */
	while (vm.pc < insn_cnt) {
		int ret;
		uint32_t pc = vm.pc;
		const struct ebpf_insn *insn;
		uint8_t opcode, dst, src;

		if (insn_executed >= insn_limit) {
			LOG_WRN("eBPF program '%s' exceeded instruction limit (%u)",
				prog->name, insn_limit);
			return -ETIMEDOUT;
		}

		insn = &insns[pc];
		opcode = insn->opcode;
		dst = EBPF_INSN_DST(insn);
		src = EBPF_INSN_SRC(insn);

#ifdef CONFIG_EBPF_RUNTIME_BOUNDS_CHECK
		ret = ebpf_vm_validate_regs(prog, pc, dst, src);

		if (ret != 0) {
			return ret;
		}
#endif

		vm.pc++;
		insn_executed++;

		ret = ebpf_vm_exec_insn(prog, &vm, insn, opcode, dst, src, ctx_data, ctx_size);

		if (ret == 1) {
			return (int64_t)vm.regs[0];
		}

		if (ret != 0) {
			return ret;
		}
	}

	/* Fall through without EXIT */
	return -EINVAL;
}
