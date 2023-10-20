/* master.h */

/*
 * Copyright (c) 1997-2010, 2014-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MASTER_H
#define _MASTER_H

#include <zephyr/kernel.h>

#include <stdio.h>

#include "receiver.h"

#include <zephyr/timestamp.h>

#include <string.h>

#include <zephyr/sys/util.h>

#include <zephyr/app_memory/app_memdomain.h>
#include <zephyr/timing/timing.h>

/* printf format defines. */
#define FORMAT "| %-65s|%10u|\n"

/* length of the output line */
#define SLINE_LEN 256

#define NR_OF_MSGQ_RUNS 500
#define NR_OF_SEMA_RUNS 500
#define NR_OF_MUTEX_RUNS 1000
#define NR_OF_MAP_RUNS 1000
#define NR_OF_MBOX_RUNS 128
#define NR_OF_PIPE_RUNS 256
#define SEMA_WAIT_TIME (5000)

#ifdef CONFIG_USERSPACE
#define BENCH_BMEM  K_APP_BMEM(bench_mem_partition)
#define BENCH_DMEM  K_APP_DMEM(bench_mem_partition)
#else
#define BENCH_BMEM
#define BENCH_DMEM
#endif

/* global data */

extern char msg[MAX_MSG];
extern char data_bench[MESSAGE_SIZE];
extern struct k_pipe *test_pipes[];
extern char sline[];

#define dashline \
	"|--------------------------------------" \
	"---------------------------------------|\n"

/*
 * To avoid divisions by 0 faults, wrap the divisor with this macro
 */
#define SAFE_DIVISOR(a) (((a) != 0) ? (a) : 1)


/* pipe amount of content to receive (0+, 1+, all) */
enum pipe_options {
	_0_TO_N = 0x0,
	_1_TO_N = 0x1,
	_ALL_N  = 0x2,
};

/* other external functions */

extern void recvtask(void *p1, void *p2, void *p3);

extern void mailbox_test(void);
extern void sema_test(void);
extern void message_queue_test(void);
extern void mutex_test(void);
extern void memorymap_test(void);
extern void pipe_test(void);

/* kernel objects needed for benchmarking */
extern struct k_mutex DEMO_MUTEX;

extern struct k_sem SEM0;
extern struct k_sem SEM1;
extern struct k_sem SEM2;
extern struct k_sem SEM3;
extern struct k_sem SEM4;
extern struct k_sem STARTRCV;

extern struct k_msgq DEMOQX1;
extern struct k_msgq DEMOQX4;
extern struct k_msgq DEMOQX192;
extern struct k_msgq MB_COMM;
extern struct k_msgq CH_COMM;

extern struct k_mbox MAILB1;

extern struct k_pipe PIPE_NOBUFF;
extern struct k_pipe PIPE_SMALLBUFF;
extern struct k_pipe PIPE_BIGBUFF;


extern struct k_mem_slab MAP1;

/* PRINT_STRING
 * Macro to print an ASCII NULL terminated string.
 */
#define PRINT_STRING(string)  printk("%s", string)

/* PRINT_F
 * Macro to print a formatted output string.
 * Assumed that sline character array of SLINE_LEN + 1 characters
 * is defined in the main file
 */

#define PRINT_F(fmt, ...)					        \
{									\
	snprintf(sline, SLINE_LEN, fmt, ##__VA_ARGS__);                 \
	PRINT_STRING(sline);					        \
}

__syscall void test_thread_priority_set(k_tid_t thread, int prio);
__syscall timing_t timing_timestamp_get(void);

#include <syscalls/master.h>

#endif /* _MASTER_H */
