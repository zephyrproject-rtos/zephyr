/*
 * Copyright (c) 2020 Synopsys, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pktqueue.h"

/* Amount of parallel processed sender/receiver queues of packet headers */
#define QUEUE_NUM 2

/* Amount of execution threads per pair of queues*/
#define THREADS_NUM (CONFIG_MP_MAX_NUM_CPUS+1)

/* Amount of packet headers in a queue */
#define SIZE_OF_QUEUE 5000

/* Size of packet header (in bytes) */
#define SIZE_OF_HEADER 24

/* CRC16 polynomial */
#define POLYNOMIAL 0x8005

/* CRC bytes in the packet */
#define CRC_BYTE_1 10
#define CRC_BYTE_2 11

#define STACK_SIZE	2048

static struct k_thread tthread[THREADS_NUM*QUEUE_NUM];
static struct k_thread qthread[QUEUE_NUM];

/* Each queue has its own mutex */
struct k_mutex sender_queue_mtx[QUEUE_NUM];
struct k_mutex receiver_queue_mtx[QUEUE_NUM];

/* Variable which indicates the amount of processed queues */
int queues_remain = QUEUE_NUM;
/* Variable to define current queue in thread */
int current_queue;

/* Array of packet header descriptors */
struct phdr_desc descriptors[QUEUE_NUM][SIZE_OF_QUEUE];

/* Arrays of receiver and sender queues */
struct phdr_desc_queue sender[QUEUE_NUM], receiver[QUEUE_NUM];

/* Array of packet headers */
uint8_t headers[QUEUE_NUM][SIZE_OF_QUEUE][SIZE_OF_HEADER];

static K_THREAD_STACK_ARRAY_DEFINE(tstack, THREADS_NUM*QUEUE_NUM, STACK_SIZE);
static K_THREAD_STACK_ARRAY_DEFINE(qstack, QUEUE_NUM, STACK_SIZE);

K_MUTEX_DEFINE(fetch_queue_mtx);

/* Function for initializing "sender" packet header queue */
void init_datagram_queue(struct phdr_desc_queue *queue, int queue_num)
{
	queue->head = descriptors[queue_num];

	for (int i = 0; i < SIZE_OF_QUEUE; i++) {
		queue->tail = &descriptors[queue_num][i];
		descriptors[queue_num][i].ptr = (uint8_t *)&headers[queue_num][i];
		/* Fill packet header with random values */
		for (int j = 0; j < SIZE_OF_HEADER; j++) {
			/* leave crc field zeroed */
			if (j < CRC_BYTE_1 || j > CRC_BYTE_2) {
				descriptors[queue_num][i].ptr[j] = sys_rand8_get();
			} else {
				descriptors[queue_num][i].ptr[j] = 0;
			}
		}
		/* Compute crc for further comparison */
		uint16_t crc;

		crc = crc16(POLYNOMIAL, 0x0000,
			    descriptors[queue_num][i].ptr, SIZE_OF_HEADER);

		/* Save crc value in header[CRC_BYTE_1-CRC_BYTE_2] field */
		descriptors[queue_num][i].ptr[CRC_BYTE_1] = (uint8_t)(crc >> 8);
		descriptors[queue_num][i].ptr[CRC_BYTE_2] = (uint8_t)(crc);
		queue->count++;
		descriptors[queue_num][i].next = &descriptors[queue_num][i+1];
	}
}

/* Thread takes packet from "sender" queue and puts it to "receiver" queue.
 * Each queue can be accessed only by one thread in a time. */
void test_thread(void *arg1, void *arg2, void *arg3)
{
	struct phdr_desc_queue *sender_queue = (struct phdr_desc_queue *)arg1;
	struct phdr_desc_queue *receiver_queue = (struct phdr_desc_queue *)arg2;
	struct phdr_desc *qin_ptr = NULL;
	int queue_num = *(int *)arg3;

	/* Fetching one queue */
	uint16_t crc, crc_orig;

	qin_ptr = phdr_desc_dequeue(sender_queue, &sender_queue_mtx[queue_num]);
	while (qin_ptr != NULL) {
		/* Store original crc value from header */
		crc_orig  =  qin_ptr->ptr[CRC_BYTE_1] << 8;
		crc_orig |=   qin_ptr->ptr[11];

		/* Crc field should be zero before crc calculation */
		qin_ptr->ptr[CRC_BYTE_1] = 0;
		qin_ptr->ptr[CRC_BYTE_2] = 0;
		crc = crc16(POLYNOMIAL, 0x0000, qin_ptr->ptr, SIZE_OF_HEADER);

		/* Compare computed crc with crc from phdr_desc->crc */
		if (crc == crc_orig) {
			phdr_desc_enqueue(receiver_queue, qin_ptr,
						 &receiver_queue_mtx[queue_num]);
		}
		/* Take next element from "sender queue" */
		qin_ptr = phdr_desc_dequeue(sender_queue,
						&sender_queue_mtx[queue_num]);
	}
}

/* Thread that processes one pair of sender/receiver queue */
void queue_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	int queue_num;

	/* Fetching one queue */
	k_mutex_lock(&fetch_queue_mtx, K_FOREVER);
	queue_num = current_queue;
	current_queue++;
	k_mutex_unlock(&fetch_queue_mtx);

	for (int i = 0; i < THREADS_NUM; i++)
		k_thread_create(&tthread[i+THREADS_NUM*queue_num],
			tstack[i+THREADS_NUM*queue_num], STACK_SIZE,
			test_thread,
			(void *)&sender[queue_num],
			(void *)&receiver[queue_num], (void *)&queue_num,
			K_PRIO_PREEMPT(10), 0, K_NO_WAIT);

	/* Wait until sender queue is not empty */
	while (sender[queue_num].count != 0) {
		k_sleep(K_MSEC(1));
	}

	/* Decrementing queue counter */
	k_mutex_lock(&fetch_queue_mtx, K_FOREVER);
	queues_remain--;
	k_mutex_unlock(&fetch_queue_mtx);
}

int main(void)
{
	uint32_t start_time, stop_time, cycles_spent, nanoseconds_spent;

	current_queue = 0;
	printk("Simulating IP header validation on multiple cores.\n");
	printk("Each of %d parallel queues is processed by %d threads"
		" on %d cores and contain %d packet headers.\n",
		QUEUE_NUM, THREADS_NUM, arch_num_cpus(), SIZE_OF_QUEUE);
	printk("Bytes in packet header: %d\n\n", SIZE_OF_HEADER);

	/* initializing "sender" queue */
	for (int i = 0; i < QUEUE_NUM; i++) {
		init_datagram_queue(&sender[i], i);
		k_mutex_init(&sender_queue_mtx[i]);
		k_mutex_init(&receiver_queue_mtx[i]);
	}

	/* Capture initial time stamp */
	start_time = k_cycle_get_32();

	for (int i = 0; i < QUEUE_NUM; i++)
		k_thread_create(&qthread[i], qstack[i], STACK_SIZE,
				queue_thread,
				(void *)&sender[i], (void *)&receiver[i],
				(void *)&i, K_PRIO_PREEMPT(11), 0, K_NO_WAIT);

	/* Wait until all queues are not processed */
	while (queues_remain > 0) {
		k_sleep(K_MSEC(1));
	}

	/* Capture final time stamp */
	stop_time = k_cycle_get_32();
	cycles_spent = stop_time - start_time;
	nanoseconds_spent = (uint32_t)k_cyc_to_ns_floor64(cycles_spent);

	/* Verify result of packet transmission
	 * The counter of correct receiver queues */
	int correct = 0;
	struct phdr_desc *tmp;
	/* Iterate and count amount of packages in receiver queues */
	for (int i = 0; i < QUEUE_NUM; i++) {
		int count = 0;

		tmp = receiver[i].head;
		while (tmp != NULL) {
			tmp = tmp->next;
			count++;
		}
		if (receiver[i].count == SIZE_OF_QUEUE && count == SIZE_OF_QUEUE) {
			correct++;
		}
	}
	if (correct == QUEUE_NUM) {
		printk("RESULT: OK\n"
			"Application ran successfully.\n"
			"All %d headers were processed in %d msec\n",
			SIZE_OF_QUEUE*QUEUE_NUM,
			nanoseconds_spent / 1000 / 1000);
	} else {
		printk("RESULT: FAIL\n"
			"Application failed.\n"
			"The amount of packets in receiver queue "
			"is less than expected.\n");
	}

	k_sleep(K_MSEC(10));
	return 0;
}
