/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * Object type abstraction.
 *
 * Each object type that can be used as a "fork" provides:
 *
 * - a definition for fork_t (a reference to a fork) and fork_obj_t (an
 *   instance of the fork object)
 * - a 'fork_init' function that initializes the object
 * - a 'take' function that simulates taking the fork (eg. k_sem_take)
 * - a 'drop' function that simulates dropping the fork (eg. k_mutex_unlock)
 * - a 'fork_type_str' string defining the object type
 *
 * When using dynamic objects, the instances of the fork objects are placed
 * automatically in the fork_objs[] array . References to these are in turn
 * placed automatically in the forks[] array, the array of references to the
 * forks.
 *
 * When using static objects, references to each object must be put by hand in
 * the forks[] array.
 */

#ifndef phil_obj_abstract__h
#define phil_obj_abstract__h

#if FORKS == SEMAPHORES
	osSemaphoreDef(0);
	osSemaphoreDef(1);
	osSemaphoreDef(2);
	osSemaphoreDef(3);
	osSemaphoreDef(4);
	osSemaphoreDef(5);

	#define fork_init(x)  osSemaphoreCreate(osSemaphore(##x), 1)
	#define take(x) osSemaphoreWait(x, osWaitForever)
	#define drop(x) osSemaphoreRelease(x)
	#define fork_type_str "semaphores"
	#define fork_t osSemaphoreId

#elif FORKS == MUTEXES
	osMutexDef(0);
	osMutexDef(1);
	osMutexDef(2);
	osMutexDef(3);
	osMutexDef(4);
	osMutexDef(5);

	#define fork_init(x) osMutexCreate(osMutex(##x));
	#define take(x)  osMutexWait(x, 0)
	#define drop(x) osMutexRelease(x)
	#define fork_type_str "mutexes"
	#define fork_t osMutexId
#else
	    #error unknown fork type
#endif
#endif /* phil_obj_abstract__h */
