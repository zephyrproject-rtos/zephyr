/* mailbox_b.c */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
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

#include "master.h"

#ifdef MAILBOX_BENCH

static struct k_msg Message;

#ifdef FLOAT
#define PRINT_HEADER()                                                       \
    PRINT_STRING                                                             \
	   ("|   size(B) |       time/packet (usec)       |          MB/sec" \
	    "                |\n", output_file);
#define PRINT_ONE_RESULT()                                                   \
    PRINT_F (output_file, "|%11lu|%32.3f|%32f|\n", putsize, puttime / 1000.0, \
	     (1000.0 * putsize) / puttime)

#define PRINT_OVERHEAD()                                                     \
    PRINT_F (output_file,						\
	     "| message overhead:  %10.3f     usec/packet                   "\
	     "            |\n", EmptyMsgPutTime / 1000.0)

#define PRINT_XFER_RATE()                                                     \
    double NettoTransferRate;                                                 \
    NettoTransferRate = 1000.0 * (putsize >> 1) / (puttime - EmptyMsgPutTime);\
    PRINT_F (output_file,						\
	     "| raw transfer rate:     %10.3f MB/sec (without"		\
	     " overhead)                 |\n", NettoTransferRate)

#else
#define PRINT_HEADER()                                                       \
    PRINT_STRING                                                             \
	   ("|   size(B) |       time/packet (nsec)       |          KB/sec" \
	    "                |\n", output_file);

#define PRINT_ONE_RESULT()                                                   \
    PRINT_F (output_file, "|%11lu|%32lu|%32lu|\n", putsize, puttime,	\
	     (uint32_t)((1000000 * (uint64_t)putsize) / puttime))

#define PRINT_OVERHEAD()                                                     \
    PRINT_F (output_file,						\
	     "| message overhead:  %10u     nsec/packet                     "\
	     "          |\n", EmptyMsgPutTime)

#define PRINT_XFER_RATE()                                                    \
    PRINT_F (output_file, "| raw transfer rate:     %10lu KB/sec (without" \
	     " overhead)                 |\n",                               \
	     (uint32_t)(1000000 * (uint64_t)(putsize >> 1)                   \
	     / (puttime - EmptyMsgPutTime)))

#endif


/*
 * Function prototypes.
 */
void mailbox_put (uint32_t size, int count, uint32_t *time);

/*
 * Function declarations.
 */

/*******************************************************************************
 *
 * mailbox_test - mailbox transfer speed test
 *
 * RETURNS: N/A
 *
 * \NOMANUAL
 */

void mailbox_test (void)
    {
    uint32_t putsize;
    uint32_t puttime;
    int putcount;
    unsigned int EmptyMsgPutTime;
    GetInfo getinfo;

    PRINT_STRING (dashline, output_file);
    PRINT_STRING ("|                "
		  "M A I L B O X   M E A S U R E M E N T S"
		  "                      |\n", output_file);
    PRINT_STRING (dashline, output_file);
    PRINT_STRING ("| Send mailbox message to waiting high "
		  "priority task and wait                 |\n", output_file);
    PRINT_F (output_file, "| repeat for %4d times and take the "
	     "average                                  |\n",
	     NR_OF_MBOX_RUNS);
    PRINT_STRING (dashline, output_file);
    PRINT_HEADER ();
    PRINT_STRING (dashline, output_file);
    task_sem_reset (SEM0);
    task_sem_give (STARTRCV);

    putcount = NR_OF_MBOX_RUNS;

    putsize = 0;
    mailbox_put (putsize, putcount, &puttime);
    task_fifo_get_wait (MB_COMM, &getinfo); /* waiting for ack */
    PRINT_ONE_RESULT ();
    EmptyMsgPutTime = puttime;
    for (putsize = 8; putsize <= MESSAGE_SIZE; putsize <<= 1) {
	mailbox_put (putsize, putcount, &puttime);
	task_fifo_get_wait (MB_COMM, &getinfo); /* waiting for ack */
	PRINT_ONE_RESULT ();
	}
    PRINT_STRING (dashline, output_file);
    PRINT_OVERHEAD ();
    PRINT_XFER_RATE ();
    }


/*******************************************************************************
 *
 * mailbox_put - write the number of data chunks into the mailbox
 *
 * RETURNS: N/A
 *
 * \NOMANUAL
 */

void mailbox_put (
    uint32_t size, /* the size of the data chunk */
    int count, /* number of data chunks */
    uint32_t *time /* the total time */
    )
    {
    int i;
    unsigned int t;

    Message.rx_task = ANYTASK;
    Message.tx_data = data_bench;
    Message.size = size;

    /* first sync with the receiver */
    task_sem_give (SEM0);
    t = BENCH_START ();
    for (i = 0; i < count; i++) {
	task_mbox_put_wait (MAILB1, 1, &Message);
	}
    t = TIME_STAMP_DELTA_GET (t);
    *time = SYS_CLOCK_HW_CYCLES_TO_NS_AVG (t, count);
    check_result ();
    }

#endif /* MAILBOX_BENCH */
