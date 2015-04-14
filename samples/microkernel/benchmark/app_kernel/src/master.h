/* master.h */

/*
 * Copyright (c) 1997-2010, 2014-2015 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _MASTER_H
#define _MASTER_H

#include <microkernel.h>

#include <stdio.h>

#include "receiver.h"

#include <timestamp.h>

#include <string.h>

/* uncomment the define below to use floating point arithmetic */
/* #define FLOAT */

/* printf format defines. */
#define FORMAT "| %-65s|%10lu|\n"

/* global defines */
/* number of nsec per usec */
#define NSEC_PER_USEC 1000

/* length of the output line */
#define SLINE_LEN 256

#define SLEEP_TIME ((sys_clock_ticks_per_sec / 4) > 0? sys_clock_ticks_per_sec / 4: 1)
#define WAIT_TIME ((sys_clock_ticks_per_sec / 10) > 0? sys_clock_ticks_per_sec / 10: 1)
#define NR_OF_NOP_RUNS 10000
#define NR_OF_FIFO_RUNS 500
#define NR_OF_SEMA_RUNS 500
#define NR_OF_MUTEX_RUNS 1000
#define NR_OF_POOL_RUNS 1000
#define NR_OF_MAP_RUNS 1000
#define NR_OF_EVENT_RUNS  1000
#define NR_OF_MBOX_RUNS 128
#define NR_OF_PIPE_RUNS 256
#define SEMA_WAIT_TIME (5 * sys_clock_ticks_per_sec)
/* global data */
extern char Msg[MAX_MSG];
extern char data_bench[OCTET_TO_SIZEOFUNIT (MESSAGE_SIZE)];
extern kpipe_t TestPipes[];
extern FILE * output_file;
extern const char dashline[];
extern const char newline[];
extern char sline[];

/* dummy_test is a function that is mapped when we */
/* do not want to test a specific Benchmark */
extern void dummy_test (void);

/* other external functions */
#ifdef KSCALL_BENCH
extern void call_test (void);
#else
#define call_test dummy_test
#endif

#ifdef MAILBOX_BENCH
extern void mailbox_test (void);
#else
#define mailbox_test dummy_test
#endif

#ifdef SEMA_BENCH
extern void sema_test (void);
#else
#define sema_test dummy_test
#endif

#ifdef FIFO_BENCH
extern void queue_test (void);
#else
#define queue_test dummy_test
#endif

#ifdef MUTEX_BENCH
extern void mutex_test (void);
#else
#define mutex_test dummy_test
#endif

#ifdef MEMMAP_BENCH
extern void memorymap_test (void);
#else
#define memorymap_test dummy_test
#endif

#ifdef MEMPOOL_BENCH
extern void mempool_test (void);
#else
#define mempool_test dummy_test
#endif

#ifdef PIPE_BENCH
extern void pipe_test (void);
#else
#define pipe_test dummy_test
#endif

#ifdef EVENT_BENCH
extern void event_test (void);
#else
#define event_test dummy_test
#endif

/* PRINT_STRING
 * Macro to print an ASCII NULL terminated string. fprintf is used
 * so output can go to console.
 */
#define PRINT_STRING(string, stream)  fprintf (stream, string)

/* PRINT_F
 * Macro to print a formatted output string. fprintf is used when
 * Assumed that sline character array of SLINE_LEN + 1 characters
 * is defined in the main file
 */

#define PRINT_F(stream, fmt, ...)			\
    {							\
    snprintf (sline, SLINE_LEN, fmt, ##__VA_ARGS__);	\
    PRINT_STRING (sline, stream);			\
    }

#define PRINT_OVERFLOW_ERROR()						\
    PRINT_F (output_file,						\
	     __FILE__":%d Error: tick occured\n", __LINE__)

static inline uint32_t BENCH_START (void)
    {
    uint32_t et;

    bench_test_start ();
    et = TIME_STAMP_DELTA_GET (0);
    return et;
    }

#define check_result()				\
    {						\
    if (bench_test_end () < 0)			\
	{					\
	PRINT_OVERFLOW_ERROR ();		\
	return; /* error */			\
	}					\
    }


#endif /* _MASTER_H */
