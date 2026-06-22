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

#include <cmsis_os2.h>

#ifndef phil_obj_abstract__h
#define phil_obj_abstract__h

#if FORKS == SEMAPHORES
	osSemaphoreAttr_t sema_attr[] = {
		{"sema0", 0, NULL, 0U},
		{"sema1", 0, NULL, 0U},
		{"sema2", 0, NULL, 0U},
		{"sema3", 0, NULL, 0U},
		{"sema4", 0, NULL, 0U},
		{"sema5", 0, NULL, 0U}
	};

	#define fork_init(x) osSemaphoreNew(1, 1, &sema_attr[x])
	#define take(x) osSemaphoreAcquire(x, osWaitForever)
	#define drop(x) osSemaphoreRelease(x)
	#define fork_type_str "semaphores"
	#define fork_t osSemaphoreId_t

#elif FORKS == MUTEXES
	osMutexAttr_t mutex_attr[x] = {
		{"Mutex0", osMutexRecursive | osMutexPrioInherit, NULL, 0U},
		{"Mutex1", osMutexRecursive | osMutexPrioInherit, NULL, 0U},
		{"Mutex2", osMutexRecursive | osMutexPrioInherit, NULL, 0U},
		{"Mutex3", osMutexRecursive | osMutexPrioInherit, NULL, 0U},
		{"Mutex4", osMutexRecursive | osMutexPrioInherit, NULL, 0U},
		{"Mutex5", osMutexRecursive | osMutexPrioInherit, NULL, 0U}
	};

	#define fork_init(x) osMutexNew(&mutex_attr[x])
	#define take(x) osMutexAcquire(x, osWaitForever)
	#define drop(x) osMutexRelease(x)
	#define fork_type_str "mutexes"
	#define fork_t osMutexAttr_t
#else
	#error unknown fork type
#endif
#endif /* phil_obj_abstract__h */
