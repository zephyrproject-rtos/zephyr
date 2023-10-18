/* mailbox_b.c */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "master.h"

static struct k_mbox_msg message;

#define PRINT_HEADER()                                                       \
	(PRINT_STRING                                                        \
	   ("|   size(B) |       time/packet (nsec)       |          KB/sec" \
	    "                |\n"))

#define PRINT_ONE_RESULT()                                                   \
	PRINT_F("|%11u|%32u|%32u|\n", putsize, puttime,	                     \
		(uint32_t)                                                   \
		(((uint64_t)putsize * 1000000U) / SAFE_DIVISOR(puttime)))

#define PRINT_OVERHEAD()                                                     \
	PRINT_F("| message overhead:  %10u     nsec/packet                 " \
	     "              |\n", empty_msg_put_time)

#define PRINT_XFER_RATE()                                                    \
	PRINT_F("| raw transfer rate:     %10u KB/sec (without"              \
		" overhead)                 |\n",                            \
		(uint32_t)((uint64_t)(putsize >> 1) * 1000000U /             \
			   SAFE_DIVISOR(puttime - empty_msg_put_time)))

/*
 * Function prototypes.
 */
void mailbox_put(uint32_t size, int count, uint32_t *time);

/*
 * Function declarations.
 */

/**
 * @brief Mailbox transfer speed test
 */
void mailbox_test(void)
{
	uint32_t putsize;
	uint32_t puttime;
	int putcount;
	unsigned int empty_msg_put_time;
	struct getinfo getinfo;

	PRINT_STRING(dashline);
	PRINT_STRING("|                "
		     "M A I L B O X   M E A S U R E M E N T S"
		     "                      |\n");
	PRINT_STRING(dashline);
	PRINT_STRING("| Send mailbox message to waiting high "
		     "priority task and wait                 |\n");
	PRINT_F("| repeat for %4d times and take the "
		"average                                  |\n",
		NR_OF_MBOX_RUNS);
	PRINT_STRING(dashline);
	PRINT_HEADER();
	PRINT_STRING(dashline);
	k_sem_reset(&SEM0);
	k_sem_give(&STARTRCV);

	putcount = NR_OF_MBOX_RUNS;

	putsize = 0U;
	mailbox_put(putsize, putcount, &puttime);
	/* waiting for ack */
	k_msgq_get(&MB_COMM, &getinfo, K_FOREVER);
	PRINT_ONE_RESULT();
	empty_msg_put_time = puttime;
	for (putsize = 8U; putsize <= MESSAGE_SIZE; putsize <<= 1) {
		mailbox_put(putsize, putcount, &puttime);
		/* waiting for ack */
		k_msgq_get(&MB_COMM, &getinfo, K_FOREVER);
		PRINT_ONE_RESULT();
	}
	PRINT_STRING(dashline);
	PRINT_OVERHEAD();
	PRINT_XFER_RATE();
}


/**
 * @brief Write the number of data chunks into the mailbox
 *
 * @param size    The size of the data chunk.
 * @param count   Number of data chunks.
 * @param time    The total time.
 */
void mailbox_put(uint32_t size, int count, uint32_t *time)
{
	int i;
	unsigned int t;
	timing_t  start;
	timing_t  end;

	message.rx_source_thread = K_ANY;
	message.tx_target_thread = K_ANY;

	/* first sync with the receiver */
	k_sem_give(&SEM0);
	start = timing_timestamp_get();
	for (i = 0; i < count; i++) {
		k_mbox_put(&MAILB1, &message, K_FOREVER);
	}
	end = timing_timestamp_get();
	t = (unsigned int)timing_cycles_get(&start, &end);
	*time = SYS_CLOCK_HW_CYCLES_TO_NS_AVG(t, count);
}
