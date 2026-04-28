/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * Implementation based on: tests/subsys/portability/cmsis_rtos_v2/src/msgq.c
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <os_task.h>
#include <os_msg.h>
#include <os_sched.h>

struct sample_data {
	int data1;
	unsigned char data2;
	unsigned int data3;
};

#define MESSAGE1   512
#define MESSAGE2   123456
#define TIMEOUT_MS 500
#define Q_LEN      5
#define STACKSZ    512

void *msgq_id;

void send_msg_thread(void *argument)
{
	int i;
	bool status;
	uint32_t msg_num;
	struct sample_data sample;
	struct sample_data data[Q_LEN] = {{0}};

	/* Wait for message_recv to complete initial checks */
	os_delay(TIMEOUT_MS);

	/* Prepare and send the 1st message (a simple integer data) */
	sample.data1 = MESSAGE1;
	status = os_msg_send(msgq_id, &sample, 0xFFFFFFFF);
	zassert_true(status == true, "os_msg_send failure for Message1");

	status = os_msg_queue_peek(msgq_id, &msg_num);
	zassert_true(status == true, "Something's wrong with os_msg_queue_peek!");

	/* The Queue should be empty at this point */
	zassert_equal(msg_num, 0, "The msg_num is not 0 unexpectedly!");

	/* Fill the queue with blocks of messages */
	for (i = 0; i < Q_LEN; i++) {
		data[i].data1 = i * 3;
		data[i].data2 = i * 3 + 1;
		data[i].data3 = i * 3 + 2;
		status = os_msg_send(msgq_id, data + i, 0xFFFFFFFF);
		zassert_true(status == true, "os_msg_send failure for message!");
	}

	status = os_msg_queue_peek(msgq_id, &msg_num);
	zassert_true(status == true, "Something's wrong with os_msg_queue_peek!");

	/* The Queue should be full at this point */
	zassert_equal(msg_num, Q_LEN, "The msg_num is not FULL unexpectedly!");

	/* Try putting message to a full queue immediately
	 * before it is emptied out and assert failure
	 */
	sample.data1 = MESSAGE2;
	status = os_msg_send(msgq_id, &sample, 0);
	zassert_true(status == false, "Something's wrong with os_msg_send. Sending msg to a full "
				      "queue unexpectedly succeeded!");

	/* Try putting message to a full queue within a duration
	 * less than TIMEOUT_TICKS, before the queue is emptied out
	 */
	sample.data1 = MESSAGE2;
	status = os_msg_send(msgq_id, &sample, TIMEOUT_MS / 2);
	zassert_true(status == false, "Something's wrong with os_msg_send. Sending msg to a full "
				      "queue unexpectedly succeeded!");

	/* Send another message after the queue is emptied */
	sample.data1 = MESSAGE2;
	status = os_msg_send(msgq_id, &sample, TIMEOUT_MS * 2);
	zassert_true(status == true, "os_msg_send failure for message!");
}

void message_recv(void)
{
	int i;
	bool status;
	struct sample_data recv_data;

	/* Try getting message immediately before the queue is populated */
	status = os_msg_recv(msgq_id, (void *)&recv_data, 0);
	zassert_true(status == false, "Something's wrong with os_msg_recv!");

	/* Try receiving message within a duration of TIMEOUT */
	status = os_msg_recv(msgq_id, (void *)&recv_data, TIMEOUT_MS);
	zassert_true(status == false, "Something's wrong with os_msg_recv!");

	/* Receive 1st message */
	status = os_msg_recv(msgq_id, (void *)&recv_data, 0xFFFFFFFF);
	zassert_true(status == true, "os_msg_recv failure");
	zassert_equal(recv_data.data1, MESSAGE1);

	/* Wait for queue to get filled */
	os_delay(TIMEOUT_MS);

	/* Empty the queue */
	for (i = 0; i < Q_LEN; i++) {
		status = os_msg_recv(msgq_id, (void *)&recv_data, 0xFFFFFFFF);
		zassert_true(status == true, "os_msg_recv failure");

		zassert_equal(recv_data.data1, i * 3);
		zassert_equal(recv_data.data2, i * 3 + 1);
		zassert_equal(recv_data.data3, i * 3 + 2);
	}

	/* Receive the next message */
	status = os_msg_recv(msgq_id, (void *)&recv_data, 0xFFFFFFFF);
	zassert_true(status == true, "os_msg_recv failure");
	zassert_equal(recv_data.data1, MESSAGE2);
}

ZTEST(osif_msgq, test_messageq)
{
	bool status;
	struct sample_data sample;
	void *tid;
	uint32_t task_param;
	uint32_t msg_num;

	status = os_msg_queue_create(&msgq_id, "Test Msg Q", Q_LEN, sizeof(struct sample_data));
	zassert_true(status != false, "Message Queue creation failed");

	status = os_task_create(&tid, "send_thread", send_msg_thread, &task_param, STACKSZ, 3);
	zassert_true(status != false, "Failed creating thread3");

	message_recv();

	/* Wait for the send_msg_thread to terminate before this thread
	 * terminates.
	 */
	os_delay(TIMEOUT_MS / 10);

	/* Make sure msgq is empty */
	status = os_msg_queue_peek(msgq_id, &msg_num);
	/* The Queue should be empty at this point */
	zassert_equal(msg_num, 0, "The msg_num is not 0 unexpectedly!");

	sample.data1 = MESSAGE1;
	/* no recv waiting， No task waiting，so return success immediately. */
	status = os_msg_send(msgq_id, &sample, 0xFFFFFFFF);
	zassert_true(status == true, "os_msg_send failure for Message1");

	status = os_msg_queue_peek(msgq_id, &msg_num);
	/* The Queue should be empty at this point */
	zassert_equal(msg_num, 1, "The msg_num is not 1 unexpectedly!");

	status = os_msg_queue_delete(msgq_id);
	zassert_true(status == true, "os_msg_queue_delete failure");

	status = os_task_delete(tid);
	zassert_true(status == true, "Error deleting TestMsgQ task");
}
ZTEST_SUITE(osif_msgq, NULL, NULL, NULL, NULL, NULL);
