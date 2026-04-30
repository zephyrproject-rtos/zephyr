/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief eBPF program descriptor and management API.
 * @ingroup ebpf
 */

#ifndef ZEPHYR_INCLUDE_EBPF_PROG_H_
#define ZEPHYR_INCLUDE_EBPF_PROG_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/ebpf/ebpf_insn.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Program types determine which attachment points are valid
 *  and what context structure is passed to R1.
 */
enum ebpf_prog_type {
	EBPF_PROG_TYPE_TRACEPOINT = 0, /**< Program attached to an eBPF tracepoint. */
	EBPF_PROG_TYPE_SCHED      = 1, /**< Program intended for scheduler-related hooks. */
	EBPF_PROG_TYPE_SYSCALL    = 2, /**< Program intended for system call hooks. */
	EBPF_PROG_TYPE_ISR        = 3, /**< Program intended for interrupt-service hooks. */
	EBPF_PROG_TYPE_MAX             /**< Number of supported program types. */
};

/** @cond INTERNAL_HIDDEN */

/** Program state */
enum ebpf_prog_state {
	EBPF_PROG_STATE_DISABLED = 0,
	EBPF_PROG_STATE_VERIFIED = 1,
	EBPF_PROG_STATE_ENABLED  = 2,
};

/** @endcond */

/**
 * @brief eBPF program descriptor.
 *
 * Placed in a RAM iterable section so runtime state is mutable.
 * The bytecode pointer itself points to const ROM data.
 */
struct ebpf_prog {
	/** Human-readable name (for shell, diagnostics) */
	const char *name;

	/** Program type */
	enum ebpf_prog_type type;

	/** Pointer to the bytecode array (const, in ROM) */
	const struct ebpf_insn *insns;

	/** Number of instructions */
	uint32_t insn_cnt;

	/** @cond INTERNAL_HIDDEN */

	/** Runtime state (disabled / verified / enabled) */
	enum ebpf_prog_state state;

	/** Tracepoint ID this program is attached to (or -1 if detached) */
	int32_t attach_point;

	/** Execution statistics */
	uint32_t run_count;
	uint64_t run_time_ns;

	/** @endcond */
};

/**
 * @brief Define an eBPF program at compile time.
 *
 * The program starts in DISABLED state and must be attached and enabled
 * at runtime.
 *
 * @param _name     C identifier for this program
 * @param _type     Program type (enum ebpf_prog_type)
 * @param _insns    Pointer to const instruction array
 * @param _insn_cnt Number of instructions
 */
#define EBPF_PROG_DEFINE(_name, _type, _insns, _insn_cnt)	\
	STRUCT_SECTION_ITERABLE(ebpf_prog, _name) = {		\
		.name         = STRINGIFY(_name),		\
		.type         = (_type),			\
		.insns        = (_insns),			\
		.insn_cnt     = (_insn_cnt),			\
		.state        = EBPF_PROG_STATE_DISABLED,	\
		.attach_point = -1,				\
		.run_count    = 0,				\
		.run_time_ns  = 0,				\
	}

/**
 * @brief Attach a program to a tracepoint.
 *
 * @param prog  Program to attach
 * @param tp_id Tracepoint ID (from enum ebpf_tracepoint_id)
 * @return 0 on success, negative errno on failure
 */
int ebpf_prog_attach(struct ebpf_prog *prog, int32_t tp_id);

/**
 * @brief Detach a program from its tracepoint.
 *
 * @param prog Program to detach
 * @return 0 on success, negative errno on failure
 */
int ebpf_prog_detach(struct ebpf_prog *prog);

/**
 * @brief Enable a program (program must be attached and verified).
 *
 * If the program has not been verified yet, verification runs automatically.
 *
 * @param prog Program to enable
 * @return 0 on success, negative errno on failure
 */
int ebpf_prog_enable(struct ebpf_prog *prog);

/**
 * @brief Disable a program (stop it from running but keep attachment).
 *
 * @param prog Program to disable
 * @return 0 on success, negative errno on failure
 */
int ebpf_prog_disable(struct ebpf_prog *prog);

/**
 * @brief Find a program by name.
 *
 * @param name Name string
 * @return Pointer to program, or NULL if not found
 */
struct ebpf_prog *ebpf_prog_find(const char *name);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_EBPF_PROG_H_ */
