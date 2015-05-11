/* mailbox_r.c */

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

#include "receiver.h"
#include "master.h"

#ifdef MAILBOX_BENCH

/*
 * Function prototypes.
 */
int mailbox_get (kmbox_t mailbox,int size,int count,unsigned int* time);

/*
 * Function declarations.
 */

/* mailbox transfer speed test */

/*******************************************************************************
 *
 * mailrecvtask - receive task
 *
 * RETURNS: N/A
 *
 * \NOMANUAL
 */

void mailrecvtask (void)
	{
	int getsize;
	unsigned int gettime;
	int getcount;
	GetInfo getinfo;

	getcount = NR_OF_MBOX_RUNS;

	getsize = 0;
	mailbox_get (MAILB1, getsize, getcount, &gettime);
	getinfo.time = gettime;
	getinfo.size = getsize;
	getinfo.count = getcount;
	task_fifo_put_wait (MB_COMM, &getinfo); /* acknowledge to master */

	for (getsize = 8; getsize <= MESSAGE_SIZE; getsize <<= 1) {
	mailbox_get (MAILB1, getsize, getcount, &gettime);
	getinfo.time = gettime;
	getinfo.size = getsize;
	getinfo.count = getcount;
	task_fifo_put_wait (MB_COMM, &getinfo); /* acknowledge to master */
	}
	}


/*******************************************************************************
 *
 * mailbox_get - receive data portions from the specified mailbox
 *
 * RETURNS: 0
 *
 * \NOMANUAL
 */

int mailbox_get (
	kmbox_t mailbox, /* the mailbox to read data from */
	int size, /* size of each data portion */
	int count, /* number of data portions */
	unsigned int* time /* resulting time */
	)
	{
	int i;
	unsigned int t;
	struct k_msg Message;

	Message.tx_task = ANYTASK;
	Message.rx_data = data_recv;
	Message.size = size;

    /* sync with the sender */
	task_sem_take_wait (SEM0);
	t = BENCH_START ();
	for (i = 0; i < count; i++) {
	task_mbox_get_wait (mailbox, &Message);
	}

	t = TIME_STAMP_DELTA_GET (t);
	*time = SYS_CLOCK_HW_CYCLES_TO_NS_AVG (t, count);
	if (bench_test_end () < 0)
	PRINT_OVERFLOW_ERROR ();
	return 0;
	}

#endif /* MAILBOX_BENCH */
