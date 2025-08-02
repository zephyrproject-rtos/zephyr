/*
 * Copyright (c) 2025 Instituto Superior de Engenharia do Porto (ISEP).
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#define BUF_SIZE 10
#define INACTIVE -1
#define PRIORITY 5

K_MSGQ_DEFINE(my_msgq, sizeof(char), BUF_SIZE, 1);

void producer_function(void *rec, void *p2, void *p3)
{
	k_tid_t receiving_thread = (k_tid_t) rec;

	char normal_data = '0';
	char urgent_data = 'A';
	int total_sent = 0;

	/*
	 * sends messages every 100 msec, in repeating
	 * sequence: normal, normal, urgent, ...
	 */
	while (total_sent < (BUF_SIZE - 1)) {
		for (int i = 0; i < 2; i++) {
			printk("[producer] sending: %c\n", normal_data);
			k_msgq_put(&my_msgq, &normal_data, K_NO_WAIT);
			k_sleep(K_MSEC(100));
			normal_data++;
		}
		printk("[producer] sending: %c (urgent)\n", urgent_data);
		k_msgq_put_front(&my_msgq, &urgent_data);
		k_sleep(K_MSEC(100));
		urgent_data++;

		total_sent += 3;
	}

	/*
	 * finished sending messages, now start the receiving thread.
	 * keep in mind both threads can be running at the same time,
	 * but in this example we wish to see the queue accumulate some
	 * messages before the receiver thread starts reading them out.
	 */
	k_thread_start(receiving_thread);
}

void consumer_function(void *p1, void *p2, void *p3)
{
	char received[BUF_SIZE];

	for (int i = 0; i < (BUF_SIZE - 1); i++) {
		k_msgq_get(&my_msgq, &received[i], K_NO_WAIT);
	}

	received[BUF_SIZE - 1] = '\0';
	/* we expect to see CBA012345... */
	printk("[consumer] got sequence: %s\n", received);
}

K_THREAD_DEFINE(consumer_thread, 2048, consumer_function,
	NULL, NULL, NULL, PRIORITY, 0, INACTIVE);

K_THREAD_DEFINE(producer_thread, 2048, producer_function,
	((void *) consumer_thread), NULL, NULL, PRIORITY, 0, 0);
