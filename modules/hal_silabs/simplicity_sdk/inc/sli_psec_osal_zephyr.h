/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SLI_PSEC_OSAL_ZEPHYR_H
#define SLI_PSEC_OSAL_ZEPHYR_H

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>

#include <sl_status.h>

#define SLI_PSEC_OSAL_WAIT_FOREVER (-1)
#define SLI_PSEC_OSAL_NON_BLOCKING (0)

#if DT_IRQ_HAS_NAME(DT_NODELABEL(se), sembrx)
/* The SE driver programs the SEMBRX priority with NVIC_SetPriority(), which takes a hardware
 * priority, while devicetree holds a Zephyr priority. Apply the offset IRQ_CONNECT() would have
 * applied, to make the priority have the expected configured value.
 */
#define SE_MANAGER_USER_SEMBRX_IRQ_PRIORITY                                                        \
	(DT_IRQ_BY_NAME(DT_NODELABEL(se), sembrx, priority) +                                      \
	 COND_CODE_1(CONFIG_ZERO_LATENCY_IRQS, (CONFIG_ZERO_LATENCY_LEVELS), (0)) + 1)
#endif

typedef enum {
	osKernelInactive = 0,
	osKernelReady = 1,
	osKernelRunning = 2,
} osKernelState_t;

typedef struct k_sem sli_psec_osal_completion_t;
typedef struct k_mutex sli_psec_osal_lock_t;

#define SLI_PSEC_OSAL_KERNEL_RUNNING (!k_is_pre_kernel())

#define SLI_PSEC_OSAL_KERNEL_CRITICAL_SECTION_START                                                \
	bool _psec_osal_sched_lock = !k_is_pre_kernel();                                           \
	if (_psec_osal_sched_lock) {                                                               \
		k_sched_lock();                                                                    \
	}

#define SLI_PSEC_OSAL_KERNEL_CRITICAL_SECTION_END                                                  \
	if (_psec_osal_sched_lock) {                                                               \
		k_sched_unlock();                                                                  \
	}

static inline sl_status_t sli_psec_osal_set_recursive_lock(sli_psec_osal_lock_t *mutex)
{
	ARG_UNUSED(mutex);

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
	ARG_UNUSED(mutex);

	return SL_STATUS_OK;
}

static inline sl_status_t sli_psec_osal_take_lock_timeout(sli_psec_osal_lock_t *mutex,
							  k_timeout_t timeout)
{
	int ret;

	__ASSERT_NO_MSG(mutex);

	ret = k_mutex_lock(mutex, timeout);
	if (ret < 0) {
		return SL_STATUS_FAIL;
	}

	return SL_STATUS_OK;
}

static inline sl_status_t sli_psec_osal_take_lock(sli_psec_osal_lock_t *mutex)
{
	return sli_psec_osal_take_lock_timeout(mutex, K_FOREVER);
}

static inline sl_status_t sli_psec_osal_take_lock_non_blocking(sli_psec_osal_lock_t *mutex)
{
	return sli_psec_osal_take_lock_timeout(mutex, K_NO_WAIT);
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
	ARG_UNUSED(sem);

	return SL_STATUS_OK;
}

static inline sl_status_t sli_psec_osal_wait_completion(sli_psec_osal_completion_t *sem, int ticks)
{
	int ret;

	__ASSERT_NO_MSG(sem);

	if (k_is_pre_kernel()) {
		return SL_STATUS_FAIL;
	}

	ret = k_sem_take(sem, ticks == SLI_PSEC_OSAL_WAIT_FOREVER ? K_FOREVER : K_TICKS(ticks));
	if (ret < 0) {
		return SL_STATUS_FAIL;
	}

	return SL_STATUS_OK;
}

static inline sl_status_t sli_psec_osal_complete(sli_psec_osal_completion_t *sem)
{
	__ASSERT_NO_MSG(sem);

	k_sem_give(sem);

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

#endif /* SLI_PSEC_OSAL_ZEPHYR_H */
