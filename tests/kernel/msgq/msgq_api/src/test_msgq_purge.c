/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_msgq.h"

K_THREAD_STACK_EXTERN(tstack);
extern struct k_thread tdata;
extern struct k_msgq msgq;
static ZTEST_BMEM char __aligned(4) tbuffer[MSG_SIZE * MSGQ_LEN];
static ZTEST_DMEM uint32_t data[MSGQ_LEN] = { MSG0, MSG1 };

static void tThread_entry(void *p1, void *p2, void *p3)
{
	int ret = k_msgq_put((struct k_msgq *)p1, (void *)&data[0], TIMEOUT);

	zassert_equal(ret, -ENOMSG, NULL);
}

static void purge_when_put(struct k_msgq *q)
{
	int ret;

	/*fill the queue to full*/
	for (int i = 0; i < MSGQ_LEN; i++) {
		ret = k_msgq_put(q, (void *)&data[i], K_NO_WAIT);
		zassert_equal(ret, 0, NULL);
	}
	/*create another thread waiting to put msg*/
	k_thread_create(&tdata, tstack, STACK_SIZE,
			tThread_entry, q, NULL, NULL,
			K_PRIO_PREEMPT(0), K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);
	k_msleep(TIMEOUT_MS >> 1);
	/**TESTPOINT: msgq purge while another thread waiting to put msg*/
	k_msgq_purge(q);

	/*verify msg put after purge*/
	for (int i = 0; i < MSGQ_LEN; i++) {
		ret = k_msgq_put(q, (void *)&data[i], K_NO_WAIT);
		zassert_equal(ret, 0, NULL);
	}

	k_thread_abort(&tdata);
}

/**
 * @addtogroup kernel_message_queue_tests
 * @{
 */

/**
 * @brief Test purge a message queue
 * @see k_msgq_init(), k_msgq_purge(), k_msgq_put()
 */
void test_msgq_purge_when_put(void)
{
	k_msgq_init(&msgq, tbuffer, MSG_SIZE, MSGQ_LEN);

	purge_when_put(&msgq);
}

#ifdef CONFIG_USERSPACE
/**
 * @brief Test purge a message queue
 * @see k_msgq_init(), k_msgq_purge(), k_msgq_put()
 */
void test_msgq_user_purge_when_put(void)
{
	struct k_msgq *q;

	q = k_object_alloc(K_OBJ_MSGQ);
	zassert_not_null(q, "couldn't alloc message queue");
	zassert_false(k_msgq_alloc_init(q, MSG_SIZE, MSGQ_LEN), NULL);

	purge_when_put(q);
}
#endif

/**
 * @}
 */
