/* mailbox_r.c */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "receiver.h"
#include "master.h"

#ifdef MAILBOX_BENCH

/*
 * Function prototypes.
 */
int mailbox_get(kmbox_t mailbox,int size,int count,unsigned int* time);

/*
 * Function declarations.
 */

/* mailbox transfer speed test */

/**
 *
 * @brief Receive task
 *
 * @return N/A
 */
void mailrecvtask(void)
{
	int getsize;
	unsigned int gettime;
	int getcount;
	GetInfo getinfo;

	getcount = NR_OF_MBOX_RUNS;

	getsize = 0;
	mailbox_get(MAILB1, getsize, getcount, &gettime);
	getinfo.time = gettime;
	getinfo.size = getsize;
	getinfo.count = getcount;
	/* acknowledge to master */
	task_fifo_put(MB_COMM, &getinfo, TICKS_UNLIMITED);

	for (getsize = 8; getsize <= MESSAGE_SIZE; getsize <<= 1) {
		mailbox_get(MAILB1, getsize, getcount, &gettime);
		getinfo.time = gettime;
		getinfo.size = getsize;
		getinfo.count = getcount;
		/* acknowledge to master */
		task_fifo_put(MB_COMM, &getinfo, TICKS_UNLIMITED);
	}
}


/**
 *
 * @brief Receive data portions from the specified mailbox
 *
 * @return 0
 *
 * @param mailbox   The mailbox to read data from.
 * @param size      Size of each data portion.
 * @param count     Number of data portions.
 * @param time      Resulting time.
 */
int mailbox_get(kmbox_t mailbox, int size, int count, unsigned int* time)
{
	int i;
	unsigned int t;
	struct k_msg Message;

	Message.tx_task = ANYTASK;
	Message.rx_data = data_recv;
	Message.size = size;

	/* sync with the sender */
	task_sem_take(SEM0, TICKS_UNLIMITED);
	t = BENCH_START();
	for (i = 0; i < count; i++) {
		task_mbox_get(mailbox, &Message, TICKS_UNLIMITED);
	}

	t = TIME_STAMP_DELTA_GET(t);
	*time = SYS_CLOCK_HW_CYCLES_TO_NS_AVG(t, count);
	if (bench_test_end() < 0) {
		PRINT_OVERFLOW_ERROR();
	}
	return 0;
}

#endif /* MAILBOX_BENCH */
