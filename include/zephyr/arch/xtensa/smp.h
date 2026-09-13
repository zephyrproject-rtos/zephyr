/*
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SoC hook interface consumed by the shared Xtensa SMP layer
 *        (arch/xtensa/core/smp.c).
 *
 * Every Xtensa SoC that implements CONFIG_SMP must provide these four
 * functions. They are the only hardware-specific primitives the shared
 * layer needs; everything else (boot sequencing, the active-core bitmap,
 * directed-IPI looping, the shared ISR) lives in arch/xtensa/core/smp.c
 * and is identical for every SoC.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_XTENSA_SMP_H_
#define ZEPHYR_INCLUDE_ARCH_XTENSA_SMP_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Physically power on / release / reset the target core so that it
 *        begins executing the SoC's own secondary-core entry trampoline.
 *
 * Called exactly once per core, from arch_cpu_start() running on the
 * *initiating* core (never on @p cpu_num itself). Must not return until
 * the target core has been told to start; it does not need to wait for
 * the target core to finish booting -- xtensa_smp_secondary_start() (see
 * below) is what runs the completion handshake.
 *
 * The SoC's own trampoline (whatever gets the core from reset into C
 * code -- e.g. a ROM boot vector, a fixed jump table entry) is entirely
 * SoC-specific and is not part of this hook interface. Once that
 * trampoline reaches C code running on @p cpu_num, it must call
 * xtensa_smp_secondary_start(cpu_num) to hand off into the shared layer.
 *
 * @param cpu_num Target core to start (never the calling core, never 0).
 */
void soc_mp_start_core(int cpu_num);

/**
 * @brief Final per-core hardware init, executed by a core on itself.
 *
 * Called for every core, including core 0, but from different places:
 * core 0 gets this call from arch_smp_init() early in boot; every other
 * core gets it from xtensa_smp_secondary_start(), which runs *on* that
 * core right after soc_mp_start_core() has released it.
 *
 * Must, at minimum, register and enable this core's own local incoming
 * IPI interrupt line (IRQ_CONNECT() + irq_enable(), using
 * xtensa_smp_ipi_isr() -- declared below -- as the handler). May also
 * perform any other per-core hardware bring-up the SoC needs (e.g.
 * interrupt controller state that is genuinely per-core register state
 * on Xtensa and therefore cannot be set up remotely by another core).
 *
 * @param cpu_num The core this function is executing on (== calling
 *                core's own ID).
 */
void soc_mp_startup_self(int cpu_num);

/**
 * @brief Signal one specific *other* core's incoming IPI line.
 *
 * Called from the shared arch_sched_directed_ipi()/
 * arch_sched_broadcast_ipi() loop, once per bitmap-set, active,
 * non-self core. Never called with @p cpu_num equal to the calling
 * core.
 *
 * @param cpu_num Target core to signal.
 */
void soc_ipi_trigger(int cpu_num);

/**
 * @brief Clear the pending IPI signal on the calling core.
 *
 * Called from xtensa_smp_ipi_isr() (the shared ISR), which runs on the
 * core that just received the signal -- i.e. this always clears the
 * *current* core's own pending state, never another core's.
 */
void soc_ipi_clear(void);

/**
 * @brief Shared secondary-core entry point.
 *
 * Not a hook -- implemented once in arch/xtensa/core/smp.c. Every SoC's
 * own secondary-core boot trampoline must call this, in C, running on
 * @p cpu_num, as its last step before Zephyr code is considered "up" on
 * that core. Sets up the ZSR_CPU pointer for _kernel.cpus[cpu_num], calls
 * soc_mp_startup_self(cpu_num), marks the core active, then calls the
 * fn/arg pair that was passed to arch_cpu_start() for this core. Never
 * returns.
 *
 * @param cpu_num The core this function is executing on.
 */
FUNC_NORETURN void xtensa_smp_secondary_start(int cpu_num);

/**
 * @brief Shared IPI ISR.
 *
 * Not a hook -- implemented once in arch/xtensa/core/smp.c. SoC code
 * passes this as the handler argument when registering this core's own
 * incoming IPI line inside its soc_mp_startup_self() implementation.
 * Calls soc_ipi_clear() then z_sched_ipi().
 *
 * Deliberately takes a plain (non-const) void * to match
 * intr_handler_t (see intc_esp32.h) -- the interrupt registration API
 * ESP32's dynamic per-core interrupt matrix uses, as opposed to
 * IRQ_CONNECT()'s static-table isr_handler_t. If a future SoC hook
 * implementation needs IRQ_CONNECT() instead, it can wrap this
 * function rather than changing its signature, since ESP32 is this
 * layer's only consumer today and must match intr_handler_t exactly.
 */
void xtensa_smp_ipi_isr(void *arg);

/**
 * @brief Stack top for a core that has been told to start but hasn't
 *        reached xtensa_smp_secondary_start() yet.
 *
 * Not a hook -- implemented once in arch/xtensa/core/smp.c. SoC boot
 * trampolines run in assembly before any C stack exists, so they need
 * this value to set up SP themselves prior to calling into
 * xtensa_smp_secondary_start().
 *
 * @param cpu_num The core whose stack top to look up.
 */
void *xtensa_smp_cpu_stack_top(int cpu_num);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_XTENSA_SMP_H_ */
