/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <kernel_structs.h>
#include <string.h>
#include <stdlib.h>

/* Flag needed to figure out if the fault was expected or not. */
extern bool valid_fault;

/* For inherit.c */
#define INHERIT_STACK_SIZE CONFIG_MAIN_STACK_SIZE
#define SEMAPHORE_MAX_COUNT (10)
#define SEMAPHORE_INIT_COUNT (0)
#define MSG_Q_SIZE (10)
#define MSG_Q_MAX_NUM_MSGS (10)
#define MSG_Q_ALIGN (2)

/* For mem_domain.c  */
#define MEM_DOMAIN_STACK_SIZE CONFIG_MAIN_STACK_SIZE
#define MEM_PARTITION_INIT_NUM (1)

#if defined(CONFIG_ARM)

#ifdef CONFIG_BOARD_MPS2_AN385
#define MEM_REGION_ALLOC (1024)
#else
#define MEM_REGION_ALLOC (32)
#endif  /* CONFIG_BOARD_MPS2_AN385 */

#elif defined(CONFIG_X86)
#define MEM_REGION_ALLOC (4096)
#elif defined(CONFIG_ARC)
#define MEM_REGION_ALLOC (STACK_ALIGN)
#else
#error "Test suite not compatible for the given architecture"
#endif
#define MEM_DOMAIN_ALIGNMENT __aligned(MEM_REGION_ALLOC)

/* for kobject.c */
#ifdef CONFIG_BOARD_MPS2_AN385
#define KOBJECT_STACK_SIZE CONFIG_MAIN_STACK_SIZE
#else
#define KOBJECT_STACK_SIZE 512
#endif

/* Sync semaphore */
extern struct k_sem sync_sem;
#define SYNC_SEM_TIMEOUT (K_FOREVER)

/* For the data memory barrier */
extern struct k_sem barrier_sem;
#define USERSPACE_BARRIER k_sem_give(&barrier_sem)
