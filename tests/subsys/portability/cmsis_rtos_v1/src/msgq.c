/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <kernel.h>
#include <cmsis_os.h>

#define MESSAGE1	512
#define MESSAGE2	123456

#define TIMEOUT 500
#define Q_LEN 5

osMessageQDef(message, Q_LEN, uint32_t);
osMessageQId message_id;

void send_msg_thread(void const *argument)
{
	int i;
	osStatus status;
	uint32_t data = MESSAGE1;

	/* Wait for message_recv to complete initial checks */
	osDelay(TIMEOUT);

	/* Prepare and send the 1st message */
	status = osMessagePut(message_id, data, osWaitForever);
	zassert_true(status == osOK, "osMessagePut failure for Message1");

	/* Fill the queue with blocks of messages */
	for (i = 0; i < Q_LEN; i++) {
		data = i;
		status = osMessagePut(message_id, data, osWaitForever);
		zassert_true(status == osOK,
				"osMessagePut failure for message!");
	}

	/* Try putting message to a full queue immediately
	 * before it is emptied out and assert failure
	 */
	data = MESSAGE2;
	status = osMessagePut(message_id, data, 0);
	zassert_true(status == osErrorResource,
			"Something's wrong with osMessagePut!");

	/* Try putting message to a full queue within a duration
	 * less than TIMEOUT, before the queue is emptied out
	 */
	data = MESSAGE2;
	status = osMessagePut(message_id, data, TIMEOUT/2);
	zassert_true(status == osErrorTimeoutResource,
			"Something's wrong with osMessagePut!");

	/* Send another message after the queue is emptied */
	data = MESSAGE2;
	status = osMessagePut(message_id, data, TIMEOUT*2);
	zassert_true(status == osOK,
			"osMessagePut failure for message!");
}

void message_recv(void)
{
	int i;
	osEvent  evt;
	uint32_t data;

	/* Try getting message immediately before the queue is populated */
	evt = osMessageGet(message_id, 0);
	zassert_true(evt.status == osOK,
			"Something's wrong with osMessageGet!");

	/* Try receiving message within a duration of TIMEOUT */
	evt = osMessageGet(message_id, TIMEOUT);
	zassert_true(evt.status == osEventTimeout,
			"Something's wrong with osMessageGet!");

	/* Receive 1st message */
	evt = osMessageGet(message_id, osWaitForever);
	zassert_true(evt.status == osEventMessage, "osMessageGet failure");

	data = evt.value.v;
	zassert_equal(data, MESSAGE1, NULL);

	/* Wait for queue to get filled */
	osDelay(TIMEOUT);

	/* Empty the queue */
	for (i = 0; i < Q_LEN; i++) {
		evt = osMessageGet(message_id, osWaitForever);
		zassert_true(evt.status == osEventMessage,
				"osMessageGet failure");

		data = evt.value.v;
		zassert_equal(data, i, NULL);
	}

	/* Receive the next message */
	evt = osMessageGet(message_id, osWaitForever);
	zassert_true(evt.status == osEventMessage, "osMessageGet failure");

	data = evt.value.v;
	zassert_equal(data, MESSAGE2, NULL);
}

osThreadDef(send_msg_thread, osPriorityNormal, 1, 0);

void test_messageq(void)
{
	osThreadId tid;

	message_id = osMessageCreate(osMessageQ(message), NULL);
	zassert_true(message_id != NULL, "Message creation failed");

	tid = osThreadCreate(osThread(send_msg_thread), NULL);
	zassert_true(tid != NULL, "Thread creation failed");

	message_recv();
}
