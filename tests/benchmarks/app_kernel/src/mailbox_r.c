/* mailbox_r.c */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "receiver.h"
#include "master.h"

#ifdef MAILBOX_BENCH

/*
 * Function prototypes.
 */
void mailbox_get(struct k_mbox *mailbox,
		int size,
		int count,
		unsigned int *time);

/*
 * Function declarations.
 */

/* mailbox transfer speed test */

/**
 *
 * @brief Receive task
 *
 */
void mailrecvtask(void)
{
	int getsize;
	unsigned int gettime;
	int getcount;
	struct getinfo getinfo;

	getcount = NR_OF_MBOX_RUNS;

	getsize = 0;
	mailbox_get(&MAILB1, getsize, getcount, &gettime);
	getinfo.time = gettime;
	getinfo.size = getsize;
	getinfo.count = getcount;
	/* acknowledge to master */
	k_msgq_put(&MB_COMM, &getinfo, K_FOREVER);

	for (getsize = 8; getsize <= MESSAGE_SIZE; getsize <<= 1) {
		mailbox_get(&MAILB1, getsize, getcount, &gettime);
		getinfo.time = gettime;
		getinfo.size = getsize;
		getinfo.count = getcount;
		/* acknowledge to master */
		k_msgq_put(&MB_COMM, &getinfo, K_FOREVER);
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
void mailbox_get(struct k_mbox *mailbox,
		 int size,
		 int count,
		 unsigned int *time)
{
	int i;
	unsigned int t;
	int32_t return_value = 0;
	struct k_mbox_msg Message;

	Message.rx_source_thread = K_ANY;
	Message.size = size;

	/* sync with the sender */
	k_sem_take(&SEM0, K_FOREVER);
	t = BENCH_START();
	for (i = 0; i < count; i++) {
		return_value |= k_mbox_get(mailbox,
					  &Message,
					  &data_recv,
					  K_FOREVER);
	}

	t = TIME_STAMP_DELTA_GET(t);
	*time = SYS_CLOCK_HW_CYCLES_TO_NS_AVG(t, count);
	if (bench_test_end() < 0) {
		PRINT_OVERFLOW_ERROR();
	}
	if (return_value != 0) {
		k_panic();
	}
}

#endif /* MAILBOX_BENCH */
