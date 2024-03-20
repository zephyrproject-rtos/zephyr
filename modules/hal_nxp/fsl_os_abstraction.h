/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __FSL_OS_ABSTRACTION__
#define __FSL_OS_ABSTRACTION__

#include <zephyr/irq.h>
#include <zephyr/kernel.h>

typedef enum _osa_status {
	KOSA_StatusSuccess = 0, /*!< Success */
	KOSA_StatusError   = 1, /*!< Failed */
	KOSA_StatusTimeout = 2, /*!< Timeout occurs while waiting */
	KOSA_StatusIdle    = 3, /*!< Used for bare metal only, the wait object is not ready
				 * and timeout still not occur
				 */
} osa_status_t;

/* enter critical macros */
#define OSA_SR_ALLOC() int osa_current_sr
#define OSA_ENTER_CRITICAL() osa_current_sr = irq_lock()
#define OSA_EXIT_CRITICAL() irq_unlock(osa_current_sr)

typedef struct k_mutex *osa_mutex_handle_t;
#define OSA_MUTEX_HANDLE_SIZE (sizeof(struct k_mutex))
#define OSA_MutexCreate(p) k_mutex_init(p)
#define OSA_MutexDestroy(p) (p)
#define OSA_MutexLock(p, t) k_mutex_lock(p, K_FOREVER)
#define OSA_MutexUnlock(p) k_mutex_unlock(p)

typedef uint32_t osa_event_flags_t;
typedef struct k_event *osa_event_handle_t;
#define OSA_EVENT_HANDLE_SIZE (sizeof(struct k_event))
static inline osa_status_t OSA_EventCreate(osa_event_handle_t eventHandle, uint8_t autoClear)
{
	k_event_init(eventHandle);
	return KOSA_StatusSuccess;
}
#define OSA_EventDestroy(p) (p)
#define OSA_EventSet(p, e) k_event_post(p, e)
#define OSA_EventClear(p, e) k_event_clear(p, e)
static inline osa_status_t OSA_EventWait(osa_event_handle_t eventHandle,
						   osa_event_flags_t flagsToWait,
						   uint8_t waitAll,
						   uint32_t millisec,
						   osa_event_flags_t *pSetFlags)
{
	*pSetFlags = k_event_wait(eventHandle, flagsToWait, false, K_FOREVER);
	if (*pSetFlags != 0) {
		k_event_clear(eventHandle, *pSetFlags);
		return KOSA_StatusSuccess;
	}
	return KOSA_StatusError;
}

void *OSA_MemoryAllocate(uint32_t memLength);
void OSA_MemoryFree(void *p);

#endif /* __FSL_OS_ABSTRACTION__ */
