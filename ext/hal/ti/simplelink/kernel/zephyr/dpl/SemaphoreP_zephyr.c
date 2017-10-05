/*
 * Copyright (c) 2017, Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/__assert.h>
#include <kernel/zephyr/dpl/dpl.h>
#include <ti/drivers/dpl/SemaphoreP.h>

/*
 * Zephyr kernel object pools:
 *
 * This bit of code enables the simplelink host driver, which assumes dynamic
 * allocation of kernel objects (semaphores, mutexes, hwis), to be
 * more easily ported to Zephyr (which supports static allocation).
 *
 * It leverages the Zephyr memory slab, enabling us to define a semaphore
 * object pool for use by the SimpleLink host driver.
 */
#define DPL_MAX_SEMAPHORES 14  /* (user.h:MAX_CONCURRENT_ACTIONS+4) = 14 */
K_MEM_SLAB_DEFINE(sem_slab, sizeof(struct k_sem), DPL_MAX_SEMAPHORES,\
		  MEM_ALIGN);

static struct k_sem *dpl_sem_pool_alloc()
{
	struct k_sem *sem_ptr = NULL;

	if (k_mem_slab_alloc(&sem_slab, (void **)&sem_ptr, K_NO_WAIT) < 0) {
		/*
		 * We assert, as this is a logic error, due to a change in #
		 * of semaphores needed by the simplelink driver. In that case,
		 * the sem pool must be increased programmatically to match.
		 */
		 __ASSERT(0, "Increase size of DPL semaphore pool");
	}
	return sem_ptr;
}

static SemaphoreP_Status dpl_sem_pool_free(struct k_sem *sem)
{
	k_mem_slab_free(&sem_slab, (void **)&sem);

	return SemaphoreP_OK;
}

/* timeout comes in and out as milliSeconds: */
static int32_t dpl_convert_timeout(uint32_t timeout)
{
	int32_t zephyr_timeout;

	switch(timeout) {
	case SemaphoreP_NO_WAIT:
		zephyr_timeout = K_NO_WAIT;
		break;
	case SemaphoreP_WAIT_FOREVER:
		zephyr_timeout = K_FOREVER;
		break;
	default:
		zephyr_timeout = timeout;
	}
	return zephyr_timeout;
}


SemaphoreP_Handle SemaphoreP_create(unsigned int count,
				    SemaphoreP_Params *params)
{
	unsigned int limit = UINT_MAX;
	struct k_sem *sem;

	if (params) {
		limit = (params->mode == SemaphoreP_Mode_BINARY) ?
			1 : UINT_MAX;
	}

	sem = dpl_sem_pool_alloc();
	if (sem) {
		k_sem_init(sem, 0, limit);
	}

	return (SemaphoreP_Handle)sem;
}

SemaphoreP_Handle SemaphoreP_createBinary(unsigned int count)
{
	SemaphoreP_Params params;

	SemaphoreP_Params_init(&params);
	params.mode = SemaphoreP_Mode_BINARY;

	return (SemaphoreP_create(count, &params));
}


void SemaphoreP_delete(SemaphoreP_Handle handle)
{
	k_sem_reset((struct k_sem *)handle);

	(void)dpl_sem_pool_free((struct k_sem *)handle);
}

void SemaphoreP_Params_init(SemaphoreP_Params *params)
{
	params->mode = SemaphoreP_Mode_COUNTING;
	params->callback = NULL;
}

/*
 * The SimpleLink driver calls this function with a timeout of 0 to "clear"
 * the SyncObject, rather than calling dpl_SyncObjClear() directly.
 * See: <simplelinksdk>/source/ti/drivers/net/wifi/source/driver.h
 *  #define SL_DRV_SYNC_OBJ_CLEAR(pObj)
 *	    (void)sl_SyncObjWait(pObj,SL_OS_NO_WAIT);
 *
 * So, we claim (via simplelink driver code inspection), that SyncObjWait
 * will *only* be called with timeout == 0 if the intention is to clear the
 * semaphore: in that case, we just call k_sem_reset.
 */
SemaphoreP_Status SemaphoreP_pend(SemaphoreP_Handle handle, uint32_t timeout)
{
	int retval;

	if (0 == timeout) {
		k_sem_reset((struct k_sem *)handle);
		retval = SemaphoreP_OK;
	} else {
		retval = k_sem_take((struct k_sem *)handle,
				    dpl_convert_timeout(timeout));
		__ASSERT_NO_MSG(retval != -EBUSY);
		retval = (retval >= 0) ? SemaphoreP_OK : SemaphoreP_TIMEOUT;
	}
	return retval;
}

void SemaphoreP_post(SemaphoreP_Handle handle)
{
	k_sem_give((struct k_sem *)handle);
}
