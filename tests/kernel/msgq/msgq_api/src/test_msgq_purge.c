/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_msgq_api
 * @{
 * @defgroup t_msgq_purge test_msgq_purge
 * @brief TestPurpose: verify zephyr msgq purge under different scenario
 * @}
 */


#include "test_msgq.h"

static char __noinit __stack tstack[STACK_SIZE];
static char __aligned(4) tbuffer[MSG_SIZE * MSGQ_LEN];
static uint32_t data[MSGQ_LEN] = { MSG0, MSG1 };

static void tThread_entry(void *p1, void *p2, void *p3)
{
	int ret = k_msgq_put((struct k_msgq *)p1, (void *)&data[0], TIMEOUT);

	assert_equal(ret, -ENOMSG, NULL);
}

/*test cases*/
void test_msgq_purge_when_put(void)
{
	struct k_msgq msgq;
	int ret;

	k_msgq_init(&msgq, tbuffer, MSG_SIZE, MSGQ_LEN);

	/*fill the queue to full*/
	for (int i = 0; i < MSGQ_LEN; i++) {
		ret = k_msgq_put(&msgq, (void *)&data[i], K_NO_WAIT);
		assert_equal(ret, 0, NULL);
	}
	/*create another thread waiting to put msg*/
	k_tid_t tid = k_thread_spawn(tstack, STACK_SIZE,
				     tThread_entry, &msgq, NULL, NULL,
				     K_PRIO_PREEMPT(0), 0, 0);
	k_sleep(TIMEOUT >> 1);
	/**TESTPOINT: msgq purge while another thread waiting to put msg*/
	k_msgq_purge(&msgq);
	k_sleep(TIMEOUT >> 1);
	k_thread_abort(tid);

	/*verify msg put after purge*/
	for (int i = 0; i < MSGQ_LEN; i++) {
		ret = k_msgq_put(&msgq, (void *)&data[i], K_NO_WAIT);
		assert_equal(ret, 0, NULL);
	}
}
