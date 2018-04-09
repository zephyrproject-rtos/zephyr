/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_PIPE_H
#define TEST_PIPE_H

#include <kernel.h>

#define PIPE_LEN 16

extern void test_pipe_thread2thread(void);
extern void test_pipe_put_fail(void);
extern void test_pipe_get_fail(void);
extern void test_pipe_block_put(void);
extern void test_pipe_block_put_sema(void);
extern void test_pipe_get_put(void);

/* k objects */


#endif /* TEST_PIPE_H */
