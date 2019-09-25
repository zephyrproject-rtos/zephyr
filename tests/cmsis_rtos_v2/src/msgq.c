/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <kernel.h>
#include <cmsis_os2.h>

struct sample_data {
	int data1;
	unsigned char data2;
	unsigned int data3;
};

#define MESSAGE1        512
#define MESSAGE2        123456
#define TIMEOUT_TICKS   50
#define Q_LEN           5
#define STACKSZ         CONFIG_CMSIS_V2_THREAD_MAX_STACK_SIZE

osMessageQueueId_t message_id;

void send_msg_thread(void *argument)
{
	int i;
	osStatus_t status;
	struct sample_data sample;
	struct sample_data data[Q_LEN] = { 0 };

	/* Wait for message_recv to complete initial checks */
	osDelay(TIMEOUT_TICKS);

	/* Prepare and send the 1st message (a simple integer data) */
	sample.data1 = MESSAGE1;
	status = osMessageQueuePut(message_id, &sample, 0, osWaitForever);
	zassert_true(status == osOK, "osMessageQueuePut failure for Message1");

	/* The Queue should be empty at this point */
	zassert_equal(osMessageQueueGetCount(message_id), 0,
		      "Something's wrong with osMessageQueueGetCount!");
	zassert_equal(osMessageQueueGetSpace(message_id), Q_LEN,
		      "Something's wrong with osMessageQueueGetSpace!");

	/* Fill the queue with blocks of messages */
	for (i = 0; i < Q_LEN; i++) {
		data[i].data1 = i * 3;
		data[i].data2 = i * 3 + 1;
		data[i].data3 = i * 3 + 2;
		status = osMessageQueuePut(message_id, data + i,
					   0, osWaitForever);
		zassert_true(status == osOK,
			     "osMessageQueuePut failure for message!");
	}

	/* The Queue should be full at this point */
	zassert_equal(osMessageQueueGetCount(message_id), Q_LEN,
		      "Something's wrong with osMessageQueueGetCount!");
	zassert_equal(osMessageQueueGetSpace(message_id), 0,
		      "Something's wrong with osMessageQueueGetSpace!");

	/* Try putting message to a full queue immediately
	 * before it is emptied out and assert failure
	 */
	sample.data1 = MESSAGE2;
	status = osMessageQueuePut(message_id, &sample, 0, 0);
	zassert_true(status == osErrorResource,
		     "Something's wrong with osMessageQueuePut!");

	/* Try putting message to a full queue within a duration
	 * less than TIMEOUT_TICKS, before the queue is emptied out
	 */
	sample.data1 = MESSAGE2;
	status = osMessageQueuePut(message_id, &sample, 0, TIMEOUT_TICKS / 2);
	zassert_true(status == osErrorTimeout,
		     "Something's wrong with osMessageQueuePut!");

	/* Send another message after the queue is emptied */
	sample.data1 = MESSAGE2;
	status = osMessageQueuePut(message_id, &sample, 0, TIMEOUT_TICKS * 2);
	zassert_true(status == osOK,
		     "osMessageQueuePut failure for message!");
}

void message_recv(void)
{
	int i;
	osStatus_t status;
	struct sample_data recv_data;

	/* Try getting message immediately before the queue is populated */
	status = osMessageQueueGet(message_id, (void *)&recv_data, NULL, 0);
	zassert_true(status == osErrorResource,
		     "Something's wrong with osMessageQueueGet!");

	/* Try receiving message within a duration of TIMEOUT */
	status = osMessageQueueGet(message_id, (void *)&recv_data,
				   NULL, TIMEOUT_TICKS);
	zassert_true(status == osErrorTimeout,
		     "Something's wrong with osMessageQueueGet!");

	zassert_equal(osMessageQueueGetCapacity(message_id), Q_LEN,
		      "Something's wrong with osMessageQueueGetCapacity!");

	zassert_equal(osMessageQueueGetMsgSize(message_id),
		      sizeof(struct sample_data),
		      "Something's wrong with osMessageQueueGetMsgSize!");

	/* Receive 1st message */
	status = osMessageQueueGet(message_id, (void *)&recv_data,
				   NULL, osWaitForever);
	zassert_true(status == osOK, "osMessageQueueGet failure");
	zassert_equal(recv_data.data1, MESSAGE1, NULL);

	/* Wait for queue to get filled */
	osDelay(TIMEOUT_TICKS);

	/* Empty the queue */
	for (i = 0; i < Q_LEN; i++) {
		status = osMessageQueueGet(message_id, (void *)&recv_data, NULL,
					   osWaitForever);
		zassert_true(status == osOK, "osMessageQueueGet failure");

		zassert_equal(recv_data.data1, i * 3, NULL);
		zassert_equal(recv_data.data2, i * 3 + 1, NULL);
		zassert_equal(recv_data.data3, i * 3 + 2, NULL);
	}

	/* Receive the next message */
	status = osMessageQueueGet(message_id, (void *)&recv_data,
				   NULL, osWaitForever);
	zassert_true(status == osOK, "osMessageQueueGet failure");
	zassert_equal(recv_data.data1, MESSAGE2, NULL);
}

static K_THREAD_STACK_DEFINE(test_stack, STACKSZ);
osThreadAttr_t thread_attr = {
	.name = "send_thread",
	.stack_mem = &test_stack,
	.stack_size = STACKSZ,
	.priority = osPriorityNormal,
};

static char __aligned(4) sample_mem[sizeof(struct sample_data) * Q_LEN];
static const osMessageQueueAttr_t init_mem_attrs = {
	.name = "TestMsgQ",
	.attr_bits = 0,
	.cb_mem = NULL,
	.cb_size = 0,
	.mq_mem = sample_mem,
	.mq_size = sizeof(struct sample_data) * Q_LEN,
};

void test_messageq(void)
{
	osStatus_t status;
	struct sample_data sample;
	osThreadId_t tid;

	message_id = osMessageQueueNew(Q_LEN, sizeof(struct sample_data),
				       &init_mem_attrs);
	zassert_true(message_id != NULL, "Message creation failed");

	tid = osThreadNew(send_msg_thread, NULL, &thread_attr);
	zassert_true(tid != NULL, "Thread creation failed");

	message_recv();

	/* Wait for the send_msg_thread to terminate before this thread
	 * terminates.
	 */
	osDelay(TIMEOUT_TICKS / 10);

	/* Make sure msgq is empty */
	zassert_equal(osMessageQueueGetCount(message_id), 0,
		      "Something's wrong with osMessageQueueGetCount!");

	sample.data1 = MESSAGE1;
	status = osMessageQueuePut(message_id, &sample, 0, osWaitForever);
	zassert_true(status == osOK, "osMessageQueuePut failure for Message1");

	zassert_equal(osMessageQueueGetCount(message_id), 1,
		      "Something's wrong with osMessageQueueGetCount!");

	status = osMessageQueueReset(message_id);
	zassert_true(status == osOK, "osMessageQueueReset failure");

	/* After reset msgq must be empty */
	zassert_equal(osMessageQueueGetCount(message_id), 0,
		      "Something's wrong with osMessageQueueGetCount!");

	status = osMessageQueueDelete(message_id);
	zassert_true(status == osOK, "osMessageQueueDelete failure");
}
