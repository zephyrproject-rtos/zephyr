/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>

#define SLI_PSEC_OSAL_WAIT_FOREVER (K_TICKS_FOREVER)
#define SLI_PSEC_OSAL_NON_BLOCKING (0)

#if DT_IRQ_HAS_NAME(DT_NODELABEL(se), sembrx)
#define SE_MANAGER_USER_SEMBRX_IRQ_PRIORITY                                                        \
	(DT_IRQ_BY_NAME(DT_NODELABEL(se), sembrx, priority) + CONFIG_ZERO_LATENCY_LEVELS + 1)
#endif

typedef enum {
	osKernelInactive = 0,
	osKernelReady = 1,
	osKernelRunning = 2,
} osKernelState_t;

typedef struct k_sem sli_psec_osal_completion_t;
typedef struct k_mutex sli_psec_osal_lock_t;

#define SLI_PSEC_OSAL_KERNEL_RUNNING (!k_is_pre_kernel())

/* Lock kernel to enter critical section */
#define SLI_PSEC_OSAL_KERNEL_CRITICAL_SECTION_START                                                \
	int __key = 0;                                                                             \
	if (!k_is_pre_kernel()) {                                                                  \
		__key = irq_lock();                                                                \
	}

/* Resume kernel to exit critical section */
#define SLI_PSEC_OSAL_KERNEL_CRITICAL_SECTION_END                                                  \
	if (!k_is_pre_kernel()) {                                                                  \
		irq_unlock(__key);                                                                 \
	}

static inline sl_status_t sli_psec_osal_set_recursive_lock(sli_psec_osal_lock_t *mutex)
{
	/* Zephyr mutexes are recursive (reentrant) by default */
	return SL_STATUS_OK;
}

static inline sl_status_t sli_psec_osal_init_lock(sli_psec_osal_lock_t *mutex)
{
	int ret;

	__ASSERT_NO_MSG(mutex);

	ret = k_mutex_init(mutex);
	if (ret < 0) {
		return SL_STATUS_FAIL;
	}

	return SL_STATUS_OK;
}

static inline sl_status_t sli_psec_osal_free_lock(sli_psec_osal_lock_t *mutex)
{
	return SL_STATUS_OK;
}

static inline sl_status_t sli_psec_osal_take_lock_timeout(sli_psec_osal_lock_t *mutex,
							  uint32_t timeout)
{
	int ret;

	__ASSERT_NO_MSG(mutex);

	/* K_TICKS(timeout) will be one of K_NO_WAIT or K_FOREVER. */
	ret = k_mutex_lock(mutex, K_TICKS(timeout));
	if (ret < 0) {
		return SL_STATUS_FAIL;
	}

	return SL_STATUS_OK;
}

static inline sl_status_t sli_psec_osal_take_lock(sli_psec_osal_lock_t *mutex)
{
	return sli_psec_osal_take_lock_timeout(mutex, SLI_PSEC_OSAL_WAIT_FOREVER);
}

static inline sl_status_t sli_psec_osal_take_lock_non_blocking(sli_psec_osal_lock_t *mutex)
{
	return sli_psec_osal_take_lock_timeout(mutex, SLI_PSEC_OSAL_NON_BLOCKING);
}

static inline sl_status_t sli_psec_osal_give_lock(sli_psec_osal_lock_t *mutex)
{
	int ret;

	__ASSERT_NO_MSG(mutex);

	ret = k_mutex_unlock(mutex);
	if (ret < 0) {
		return SL_STATUS_FAIL;
	}

	return SL_STATUS_OK;
}

static inline sl_status_t sli_psec_osal_init_completion(sli_psec_osal_completion_t *sem)
{
	int ret;

	__ASSERT_NO_MSG(sem);

	ret = k_sem_init(sem, 0, 1);
	if (ret < 0) {
		return SL_STATUS_FAIL;
	}

	return SL_STATUS_OK;
}

static inline sl_status_t sli_psec_osal_free_completion(sli_psec_osal_completion_t *sem)
{
	/* Nothing to do */
	return SL_STATUS_OK;
}

static inline sl_status_t sli_psec_osal_wait_completion(sli_psec_osal_completion_t *sem, int ticks)
{
	int ret;

	__ASSERT_NO_MSG(sem);

	if (k_is_pre_kernel()) {
		return SL_STATUS_OK;
	}

	ret = k_sem_take(sem, K_TICKS(ticks));
	if (ret < 0) {
		return SL_STATUS_FAIL;
	}

	return SL_STATUS_OK;
}

static inline sl_status_t sli_psec_osal_complete(sli_psec_osal_completion_t *sem)
{
	__ASSERT_NO_MSG(sem);

	if (!k_is_pre_kernel()) {
		k_sem_give(sem);
	}

	return SL_STATUS_OK;
}

static inline int32_t sli_psec_osal_kernel_lock(void)
{
	if (k_is_in_isr()) {
		return -ENOTSUP;
	}

	k_sched_lock();
	return 0;
}

static inline int32_t sli_psec_osal_kernel_restore_lock(int32_t lock)
{
	if (k_is_in_isr()) {
		return -ENOTSUP;
	}
	if (lock < 0) {
		/* Lock state contains error code from the corresponding lock attempt.
		 * If it returned an error, do not attempt to unlock.
		 */
		return -ENOTSUP;
	}

	k_sched_unlock();
	return 0;
}

static inline osKernelState_t sli_psec_osal_kernel_get_state(void)
{
	return k_is_pre_kernel() ? osKernelInactive : osKernelRunning;
}
