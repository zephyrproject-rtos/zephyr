/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <tc_util.h>
#include <kernel_structs.h>
#include <irq_offload.h>

#define SEM_ARRAY_SIZE	16

static __kernel struct k_sem semarray[SEM_ARRAY_SIZE];
K_SEM_DEFINE(sem1, 0, 1);
static __kernel struct k_sem sem2;
static __kernel char bad_sem[sizeof(struct k_sem)];
#ifdef CONFIG_APPLICATION_MEMORY
static struct k_sem sem3;
#endif


static int test_bad_object(struct k_sem *sem)
{
	return !_k_object_validate(sem, K_OBJ_SEM, 0);
}


void main(void) {
	struct k_sem stack_sem;
	int i, rv;

	TC_START("obj_validation");

	rv = TC_PASS;

	/* None of these should be even in the table */
	rv |= test_bad_object(&stack_sem);
	rv |= test_bad_object((struct k_sem *)&bad_sem);
	rv |= test_bad_object((struct k_sem *)0xFFFFFFFF);
#ifdef CONFIG_APPLICATION_MEMORY
	rv |= test_bad_object(&sem3);
#endif

	/* sem1 should already be ready to go */
	rv |= !test_bad_object(&sem1);

	/* sem2 has to be initialized at runtime */
	rv |= test_bad_object(&sem2);
	k_sem_init(&sem2, 0, 1);
	rv |= !test_bad_object(&sem2);

	for (i = 0; i < SEM_ARRAY_SIZE; i++) {
		rv |= test_bad_object(&semarray[i]);
		k_sem_init(&semarray[i], 0, 1);
		rv |= !test_bad_object(&semarray[i]);
	}

	TC_END_RESULT(rv);
	TC_END_REPORT(rv);
}

