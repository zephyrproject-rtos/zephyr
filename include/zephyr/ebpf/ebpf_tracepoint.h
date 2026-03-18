/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief eBPF tracepoint IDs and context structures.
 * @ingroup ebpf
 */

#ifndef ZEPHYR_INCLUDE_EBPF_TRACEPOINT_H_
#define ZEPHYR_INCLUDE_EBPF_TRACEPOINT_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Tracepoint IDs for eBPF program attachment.
 */
enum ebpf_tracepoint_id {
	/* Scheduler / Thread events */
	EBPF_TP_THREAD_SWITCHED_IN = 0, /**< Thread switch-in event. */
	EBPF_TP_THREAD_SWITCHED_OUT,    /**< Thread switch-out event. */
	EBPF_TP_THREAD_CREATE,          /**< Thread creation event. */
	EBPF_TP_THREAD_ABORT,           /**< Thread abort event. */
	EBPF_TP_THREAD_SUSPEND,         /**< Thread suspend event. */
	EBPF_TP_THREAD_RESUME,          /**< Thread resume event. */
	EBPF_TP_THREAD_READY,           /**< Thread ready event. */
	EBPF_TP_THREAD_PEND,            /**< Thread pend event. */
	EBPF_TP_THREAD_YIELD,           /**< Thread yield event. */
	EBPF_TP_THREAD_SLEEP_ENTER,     /**< Thread sleep-entry event. */
	EBPF_TP_THREAD_SLEEP_EXIT,      /**< Thread wake-from-sleep event. */
	EBPF_TP_THREAD_WAKEUP,          /**< Thread wakeup event. */

	/* ISR events */
	EBPF_TP_ISR_ENTER,              /**< ISR entry event. */
	EBPF_TP_ISR_EXIT,               /**< ISR exit event. */

	/* Idle events */
	EBPF_TP_IDLE_ENTER,             /**< CPU idle entry event. */
	EBPF_TP_IDLE_EXIT,              /**< CPU idle exit event. */

	/* Semaphore events */
	EBPF_TP_SEM_INIT,               /**< Semaphore initialization event. */
	EBPF_TP_SEM_GIVE_ENTER,         /**< Semaphore give entry event. */
	EBPF_TP_SEM_TAKE_ENTER,         /**< Semaphore take entry event. */
	EBPF_TP_SEM_TAKE_EXIT,          /**< Semaphore take exit event. */

	/* Mutex events */
	EBPF_TP_MUTEX_INIT,             /**< Mutex initialization event. */
	EBPF_TP_MUTEX_LOCK_ENTER,       /**< Mutex lock entry event. */
	EBPF_TP_MUTEX_LOCK_EXIT,        /**< Mutex lock exit event. */
	EBPF_TP_MUTEX_UNLOCK_ENTER,     /**< Mutex unlock entry event. */
	EBPF_TP_MUTEX_UNLOCK_EXIT,      /**< Mutex unlock exit event. */

	/* Syscall events */
	EBPF_TP_SYSCALL_ENTER,          /**< System call entry event. */
	EBPF_TP_SYSCALL_EXIT,           /**< System call exit event. */

	/* Must be last */
	EBPF_TP_MAX                     /**< Number of supported tracepoints. */
};

/**
 * @brief Context passed to eBPF programs for thread events (via R1).
 */
struct ebpf_ctx_thread {
	uint32_t thread_id; /**< Thread identifier derived from the associated k_thread pointer. */
	int32_t  priority;  /**< Thread priority at the time of the event. */
	uint32_t flags;     /**< Event-specific thread state flags. */
};

/**
 * @brief Context for ISR events.
 */
struct ebpf_ctx_isr {
	uint32_t irq_num; /**< Interrupt line number. */
};

/**
 * @brief Context for syscall events.
 */
struct ebpf_ctx_syscall {
	uint32_t syscall_id; /**< System call identifier. */
	uint32_t arg0;       /**< First system call argument. */
	uint32_t arg1;       /**< Second system call argument. */
	uint32_t arg2;       /**< Third system call argument. */
};

/**
 * @brief Context for scheduler events (context switch).
 */
struct ebpf_ctx_sched {
	uint32_t prev_thread_id; /**< Identifier of the outgoing thread. */
	uint32_t next_thread_id; /**< Identifier of the incoming thread. */
	int32_t  prev_priority;  /**< Priority of the outgoing thread. */
	int32_t  next_priority;  /**< Priority of the incoming thread. */
};

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Check if any program is attached to a tracepoint.
 *
 * @param tp_id Tracepoint ID
 * @return true if active, false otherwise
 */
bool ebpf_tracepoint_is_active(enum ebpf_tracepoint_id tp_id);

/**
 * @brief Dispatch an eBPF tracepoint.
 *
 * Called from the tracing macro integration layer.
 *
 * @param tp_id    Tracepoint ID
 * @param ctx      Pointer to context data
 * @param ctx_size Size of context data
 */
void ebpf_tracepoint_dispatch(enum ebpf_tracepoint_id tp_id, void *ctx, uint32_t ctx_size);

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_EBPF_TRACEPOINT_H_ */
