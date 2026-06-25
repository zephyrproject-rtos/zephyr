/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_msgq.h"

K_THREAD_STACK_DECLARE(tstack, STACK_SIZE);
extern struct k_thread tdata;
extern k_tid_t tids[2];
extern struct k_msgq msgq;
static ZTEST_BMEM char __aligned(4) tbuffer[MSG_SIZE * MSGQ_LEN];
static ZTEST_DMEM uint32_t data[MSGQ_LEN] = { MSG0, MSG1 };

static void tThread_entry(void *p1, void *p2, void *p3)
{
	int ret = k_msgq_put((struct k_msgq *)p1, (void *)&data[0], TIMEOUT);

	zassert_equal(ret, -ENOMSG);
}

static void purge_when_put(struct k_msgq *q)
{
	int ret;

	/*fill the queue to full*/
	for (int i = 0; i < MSGQ_LEN; i++) {
		ret = k_msgq_put(q, (void *)&data[i], K_NO_WAIT);
		zassert_equal(ret, 0);
	}
	/*create another thread waiting to put msg*/
	tids[0] = k_thread_create(&tdata, tstack, STACK_SIZE,
				  tThread_entry, q, NULL, NULL,
				  K_PRIO_PREEMPT(0), K_USER | K_INHERIT_PERMS,
				  K_NO_WAIT);
	k_msleep(TIMEOUT_MS >> 1);
	/**TESTPOINT: msgq purge while another thread waiting to put msg*/
	k_msgq_purge(q);

	/*verify msg put after purge*/
	for (int i = 0; i < MSGQ_LEN; i++) {
		ret = k_msgq_put(q, (void *)&data[i], K_NO_WAIT);
		zassert_equal(ret, 0);
	}

	k_thread_abort(tids[0]);
}

/**
 * @addtogroup tests_kernel_msgq
 * @{
 */

/**
 * @brief Verify k_msgq_purge() drops messages and wakes a pending writer.
 *
 * @details
 * Purging a full queue must discard all queued messages and unblock any thread
 * waiting to put, returning -ENOMSG to that writer. After the purge the queue
 * must be usable again for new puts.
 *
 * Test steps:
 * - Fill the queue, then start a writer that blocks on a put with a timeout.
 * - Purge the queue while the writer is pending.
 * - Verify the writer's put returns -ENOMSG and the queue accepts new messages.
 *
 * Expected result:
 * - Purge empties the queue, the pending writer gets -ENOMSG, and the queue is
 *   usable afterwards.
 *
 * @see k_msgq_purge()
 * @see k_msgq_put()
 * @verifies ZEP-SRS-31-14
 */
ZTEST(msgq_api_1cpu, test_msgq_purge_when_put)
{
	k_msgq_init(&msgq, tbuffer, MSG_SIZE, MSGQ_LEN);

	purge_when_put(&msgq);
}

#ifdef CONFIG_USERSPACE
/**
 * @brief Verify k_msgq_purge() with a pending writer from user mode.
 *
 * @details
 * User-mode counterpart of test_msgq_purge_when_put() on a queue created with
 * k_msgq_alloc_init(): purging while a writer pends must discard messages, wake
 * the writer with -ENOMSG, and leave the queue usable.
 *
 * Test steps:
 * - Allocate and alloc-init a queue as a user thread and fill it.
 * - Start a pending writer, purge, and verify recovery.
 *
 * Expected result:
 * - Behavior matches the supervisor case from user mode.
 *
 * @see k_msgq_purge()
 * @see k_msgq_alloc_init()
 */
ZTEST_USER(msgq_api, test_msgq_user_purge_when_put)
{
	struct k_msgq *q;

	q = k_object_alloc(K_OBJ_MSGQ);
	zassert_not_null(q, "couldn't alloc message queue");
	zassert_false(k_msgq_alloc_init(q, MSG_SIZE, MSGQ_LEN));

	purge_when_put(q);
}
#endif

/**
 * @}
 */
