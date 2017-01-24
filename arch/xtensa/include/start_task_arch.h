/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief XTENSA nanokernel declarations to start a task
 *
 * XTENSA-specific parts of start_task().
 *
 * Currently empty, only here for abstraction.
 */

#ifndef _START_TASK_ARCH__H_
#define _START_TASK_ARCH__H_

#include <toolchain.h>
#include <sections.h>

#include <micro_private.h>
#include <kernel_structs.h>
#include <microkernel/task.h>

#ifdef __cplusplus
extern "C" {
#endif

#define _START_TASK_ARCH(task, opt_ptr) \
	do {/* nothing */              \
	} while ((0))

#ifdef __cplusplus
}
#endif

#endif /* _START_TASK_ARCH__H_ */
