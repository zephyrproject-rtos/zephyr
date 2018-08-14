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

osMessageQDef(message, 5, u32_t);
osMessageQId message_id;

void send_msg_thread(void const *argument)
{
	osStatus status;
	u32_t data = MESSAGE1;

	/* Prepare and send 1st Message */
	status = osMessagePut(message_id, data, osWaitForever);
	zassert_true(status == osOK, "osMessagePut failure for Message1");
	osDelay(100);

	/* Prepare and send 2nd Message */
	data = MESSAGE2;
	status = osMessagePut(message_id, data, osWaitForever);
	zassert_true(status == osOK, "osMessagePut failure for Message2");
}

void message_recv(void)
{
	osEvent  evt;
	u32_t data;

	/* Receive 1st Message */
	evt = osMessageGet(message_id, osWaitForever);
	zassert_true(evt.status == osEventMessage, "osMessageGet failure");

	data = evt.value.v;
	zassert_equal(data, MESSAGE1, NULL);

	/* Receive 2nd Message */
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
