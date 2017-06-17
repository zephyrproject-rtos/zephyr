/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Per-arch thread definition
 *
 * This file contains definitions for
 *
 *  struct _thread_arch
 *  struct _callee_saved
 *  struct _caller_saved
 *
 * necessary to instantiate instances of struct k_thread.
 */


#ifndef _kernel_arch_thread__h_
#define _kernel_arch_thread__h_

#ifndef _ASMLANGUAGE

struct _caller_saved {
};

typedef struct _caller_saved _caller_saved_t;

struct _callee_saved {
};

typedef struct _callee_saved _callee_saved_t;

struct _thread_arch {
};

typedef struct _thread_arch _thread_arch_t;

#endif /* _ASMLANGUAGE */

#endif /* _kernel_arch_thread__h_ */

