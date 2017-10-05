/*
 * Copyright (c) 2017, Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/__assert.h>
#include <kernel/zephyr/dpl/dpl.h>
#include <ti/drivers/dpl/MutexP.h>

/*
 * Zephyr kernel object pools:
 *
 * This bit of code enables the simplelink host driver, which assumes dynamic
 * allocation of kernel objects (semaphores, mutexes, hwis), to be
 * more easily ported to Zephyr (which supports static allocation).
 *
 * It leverages the Zephyr memory slab, enabling us to define a mutex object
 * pool for use by the SimpleLink host driver.
 */

/* Define a Mutex pool: */
#define DPL_MAX_MUTEXES	 4  /* From simplelink driver code inspection */
K_MEM_SLAB_DEFINE(mutex_slab, sizeof(struct k_mutex), DPL_MAX_MUTEXES,\
		  MEM_ALIGN);

static struct k_mutex *dpl_mutex_pool_alloc()
{
	struct k_mutex *mutex_ptr = NULL;

	if (k_mem_slab_alloc(&mutex_slab, (void **)&mutex_ptr,
			     K_NO_WAIT) < 0) {
		/*
		 * We assert, as this is a logic error, due to a change in #
		 * of mutexes needed by the simplelink driver. In that case,
		 * the mutex pool must be increased programmatically to match.
		 */
		 __ASSERT(0, "Increase size of DPL mutex pool");
	}
	return mutex_ptr;
}

static MutexP_Status dpl_mutex_pool_free(struct k_mutex *mutex)
{
	k_mem_slab_free(&mutex_slab, (void **)&mutex);
	return MutexP_OK;
}

MutexP_Handle MutexP_create(MutexP_Params *params)
{
	struct k_mutex *mutex;

	ARG_UNUSED(params);

	mutex = dpl_mutex_pool_alloc();
	__ASSERT(mutex, "MutexP_create failed\r\n");

	if (mutex) {
		k_mutex_init(mutex);
	}
	return ((MutexP_Handle)mutex);
}

void MutexP_delete(MutexP_Handle handle)
{
	/* No way in Zephyr to "reset" the lock, so just re-init: */
	k_mutex_init((struct k_mutex *)handle);

	dpl_mutex_pool_free((struct k_mutex *)handle);
}

uintptr_t MutexP_lock(MutexP_Handle handle)
{
	unsigned int key = 0;
	int retval;

	retval = k_mutex_lock((struct k_mutex *)handle, K_FOREVER);
	__ASSERT(retval == 0,
		 "MutexP_lock: retval: %d\r\n", retval);

	return ((uintptr_t)key);
}

void MutexP_Params_init(MutexP_Params *params)
{
	params->callback = NULL;
}

void MutexP_unlock(MutexP_Handle handle, uintptr_t key)
{
	ARG_UNUSED(key);

	k_mutex_unlock((struct k_mutex *)handle);
}
