/*
 * Copyright (c) 2018, Pinecone Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	nuttx/condition.h
 * @brief	NuttX condition variable primitives for libmetal.
 */

#ifndef __METAL_CONDITION__H__
#error "Include metal/condition.h instead of metal/nuttx/condition.h"
#endif

#ifndef __METAL_NUTTX_CONDITION__H__
#define __METAL_NUTTX_CONDITION__H__

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

struct metal_condition {
	pthread_cond_t cond;
};

/** Static metal condition variable initialization. */
#define METAL_CONDITION_INIT		{PTHREAD_COND_INITIALIZER}

static inline void metal_condition_init(struct metal_condition *cv)
{
	pthread_cond_init(&cv->cond, NULL);
}

static inline int metal_condition_signal(struct metal_condition *cv)
{
	return -pthread_cond_signal(&cv->cond);
}

static inline int metal_condition_broadcast(struct metal_condition *cv)
{
	return -pthread_cond_broadcast(&cv->cond);
}

#ifdef __cplusplus
}
#endif

#endif /* __METAL_NUTTX_CONDITION__H__ */
