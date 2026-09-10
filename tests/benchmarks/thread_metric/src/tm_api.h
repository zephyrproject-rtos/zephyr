/***************************************************************************
 * Copyright (c) 2024 Microsoft Corporation
 *
 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT.
 *
 * SPDX-License-Identifier: MIT
 **************************************************************************/

/**************************************************************************/
/**************************************************************************/
/**                                                                       */
/** Thread-Metric Component                                               */
/**                                                                       */
/** Application Interface (API)                                           */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

/**************************************************************************/
/*                                                                        */
/*  APPLICATION INTERFACE DEFINITION                       RELEASE        */
/*                                                                        */
/*    tm_api.h                                            PORTABLE C      */
/*                                                           6.1.7        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    William E. Lamie, Microsoft Corporation                             */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This file defines the basic Application Interface (API)             */
/*    implementation source code for the Thread-Metrics performance       */
/*    test suite. All service prototypes and data structure definitions   */
/*    are defined in this file.                                           */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  10-15-2021     William E. Lamie         Initial Version 6.1.7         */
/*                                                                        */
/**************************************************************************/

#ifndef TM_API_H
#define TM_API_H

#include "tm_porting_layer.h"

/*
 * Determine if a C++ compiler is being used.  If so, ensure that standard
 * C is used to process the API information.
 */

#ifdef __cplusplus

/* Yes, C++ compiler is present.  Use standard C.  */
extern "C" {

#endif

/* Define API constants.  */

#define TM_SUCCESS 0
#define TM_ERROR   1

/* Define the time interval in seconds. This can be changed with a -D compiler option.  */

#ifndef TM_TEST_DURATION
#define TM_TEST_DURATION 30
#endif

/*
 * Define RTOS Neutral APIs. RTOS vendors should fill in the guts of the following
 * API. Once this is done the Thread-Metric tests can be successfully run.
 */

void tm_initialize(void (*test_initialization_function)(void));
int tm_thread_create(int thread_id, int priority, void (*entry_function)(void *, void *, void *));
int tm_thread_resume(int thread_id);
int tm_thread_suspend(int thread_id);
void tm_thread_relinquish(void);
void tm_thread_sleep(int seconds);
int tm_queue_create(int queue_id);
int tm_queue_send(int queue_id, unsigned long *message_ptr);
int tm_queue_receive(int queue_id, unsigned long *message_ptr);
int tm_semaphore_create(int semaphore_id);
int tm_semaphore_get(int semaphore_id);
int tm_semaphore_put(int semaphore_id);
int tm_memory_pool_create(int pool_id);
int tm_memory_pool_allocate(int pool_id, unsigned char **memory_ptr);
int tm_memory_pool_deallocate(int pool_id, unsigned char *memory_ptr);

/*
 * Determine if a C++ compiler is being used.  If so, complete the standard
 * C conditional started above.
 */
#ifdef __cplusplus
}
#endif

#endif
