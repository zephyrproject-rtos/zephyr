/* master.h */

/*
 * Copyright (c) 1997-2010, 2014-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MASTER_H
#define _MASTER_H

#include <zephyr/zephyr.h>

#include <stdio.h>

#include "receiver.h"

#include <timestamp.h>

#include <string.h>

#include <zephyr/sys/util.h>


/* uncomment the define below to use floating point arithmetic */
/* #define FLOAT */

/* printf format defines. */
#define FORMAT "| %-65s|%10u|\n"

/* length of the output line */
#define SLINE_LEN 256

#define SLEEP_TIME ((CONFIG_SYS_CLOCK_TICKS_PER_SEC / 4) > 0 ?	\
		    CONFIG_SYS_CLOCK_TICKS_PER_SEC / 4 : 1)
#define WAIT_TIME ((CONFIG_SYS_CLOCK_TICKS_PER_SEC / 10) > 0 ?	\
		   CONFIG_SYS_CLOCK_TICKS_PER_SEC / 10 : 1)
#define NR_OF_NOP_RUNS 10000
#define NR_OF_FIFO_RUNS 500
#define NR_OF_SEMA_RUNS 500
#define NR_OF_MUTEX_RUNS 1000
#define NR_OF_POOL_RUNS 1000
#define NR_OF_MAP_RUNS 1000
#define NR_OF_EVENT_RUNS  1000
#define NR_OF_MBOX_RUNS 128
#define NR_OF_PIPE_RUNS 256
/* #define SEMA_WAIT_TIME (5 * CONFIG_SYS_CLOCK_TICKS_PER_SEC) */
#define SEMA_WAIT_TIME (5000)
/* global data */
extern char msg[MAX_MSG];
extern char data_bench[MESSAGE_SIZE];
extern struct k_pipe *test_pipes[];
extern FILE *output_file;
extern const char newline[];
extern char sline[];

#define dashline \
	"|--------------------------------------" \
	"---------------------------------------|\n"

/*
 * To avoid divisions by 0 faults, wrap the divisor with this macro
 */
#define SAFE_DIVISOR(a) (((a) != 0)?(a):1)


/* pipe amount of content to receive (0+, 1+, all) */
enum pipe_options {
	_0_TO_N = 0x0,
	_1_TO_N = 0x1,
	_ALL_N  = 0x2,
};

/* dummy_test is a function that is mapped when we */
/* do not want to test a specific Benchmark */
extern void dummy_test(void);

/* other external functions */

extern void bench_task(void *p1, void *p2, void *p3);
extern void recvtask(void *p1, void *p2, void *p3);

#ifdef MAILBOX_BENCH
extern void mailbox_test(void);
#else
#define mailbox_test dummy_test
#endif

#ifdef SEMA_BENCH
extern void sema_test(void);
#else
#define sema_test dummy_test
#endif

#ifdef FIFO_BENCH
extern void queue_test(void);
#else
#define queue_test dummy_test
#endif

#ifdef MUTEX_BENCH
extern void mutex_test(void);
#else
#define mutex_test dummy_test
#endif

#ifdef MEMMAP_BENCH
extern void memorymap_test(void);
#else
#define memorymap_test dummy_test
#endif

#ifdef MEMPOOL_BENCH
extern void mempool_test(void);
#else
#define mempool_test dummy_test
#endif

#ifdef PIPE_BENCH
extern void pipe_test(void);
#else
#define pipe_test dummy_test
#endif

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
extern struct k_msgq MB_COMM;
extern struct k_msgq CH_COMM;

extern struct k_mbox MAILB1;


extern struct k_pipe PIPE_NOBUFF;
extern struct k_pipe PIPE_SMALLBUFF;
extern struct k_pipe PIPE_BIGBUFF;


extern struct k_mem_slab MAP1;

extern struct k_mem_pool DEMOPOOL;



/* PRINT_STRING
 * Macro to print an ASCII NULL terminated string. fprintf is used
 * so output can go to console.
 */
#define PRINT_STRING(string, stream)  fputs(string, stream)

/* PRINT_F
 * Macro to print a formatted output string. fprintf is used when
 * Assumed that sline character array of SLINE_LEN + 1 characters
 * is defined in the main file
 */

#define PRINT_F(stream, fmt, ...)					\
{									\
	snprintf(sline, SLINE_LEN, fmt, ##__VA_ARGS__);	\
	PRINT_STRING(sline, stream);					\
}

#define PRINT_OVERFLOW_ERROR()						\
	PRINT_F(output_file, __FILE__":%d Error: tick occurred\n", __LINE__)

static inline uint32_t BENCH_START(void)
{
	uint32_t et;

	bench_test_start();
	et = TIME_STAMP_DELTA_GET(0);
	return et;
}

static inline void check_result(void)
{
	if (bench_test_end() < 0) {
		PRINT_OVERFLOW_ERROR();
		return; /* error */
	}
}


#endif /* _MASTER_H */
