/*
 * Copyright (c) 2023, Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef KERNEL_INCLUDE_OBJ_CORE_H
#define KERNEL_INCLUDE_OBJ_CORE_H


void init_obj_core(void);

int init_sem_obj_core_list(void);

int init_mutex_obj_core_list(void);

int init_condvar_obj_core_list(void);

int init_mem_slab_obj_core_list(void);

int init_pipe_obj_core_list(void);

int init_msgq_obj_core_list(void);

int init_mailbox_obj_core_list(void);

int init_stack_obj_core_list(void);

int init_thread_obj_core_list(void);

int init_fifo_obj_core_list(void);

int init_lifo_obj_core_list(void);

int init_timer_obj_core_list(void);

int init_event_obj_core_list(void);

int init_cpu_obj_core_list(void);

int init_kernel_obj_core_list(void);

#endif /* KERNEL_INCLUDE_OBJ_CORE_H */
