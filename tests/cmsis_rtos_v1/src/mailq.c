/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <kernel.h>
#include <cmsis_os.h>

struct sample_data {
	int		data1;
	unsigned char	data2;
	unsigned int	data3;
};

#define MAIL1_DATA1	75663
#define MAIL1_DATA2	156
#define MAIL1_DATA3	1000001

#define MAIL2_DATA1	93567
#define MAIL2_DATA2	255
#define MAIL2_DATA3	1234567

osMailQDef(mail, 16, struct sample_data);
osMailQId  mail_id;

void send_thread(void const *argument)
{
	struct sample_data *tx_ptr;
	osStatus status;

	/* Prepare and send 1st mail */
	tx_ptr = osMailAlloc(mail_id, osWaitForever);
	zassert_true(tx_ptr != NULL, "Mail1 alloc failed");
	tx_ptr->data1 = MAIL1_DATA1;
	tx_ptr->data2 = MAIL1_DATA2;
	tx_ptr->data3 = MAIL1_DATA3;
	status = osMailPut(mail_id, tx_ptr);
	zassert_true(status == osOK, "osMailPut failure for mail1");
	osDelay(100);

	/* Prepare and send 2nd mail */
	tx_ptr = osMailCAlloc(mail_id, osWaitForever);
	zassert_true(tx_ptr != NULL, "Mail2 alloc failed");
	tx_ptr->data1 = MAIL2_DATA1;
	tx_ptr->data2 = MAIL2_DATA2;
	tx_ptr->data3 = MAIL2_DATA3;
	status = osMailPut(mail_id, tx_ptr);
	zassert_true(status == osOK, "osMailPut failure for mail2");
}

void mail_recv(void)
{
	struct sample_data  *rx_ptr;
	osEvent  evt;
	osStatus status;

	/* Receive 1st mail */
	evt = osMailGet(mail_id, osWaitForever);
	zassert_true(evt.status == osEventMail, "osMailGet failure");

	rx_ptr = evt.value.p;
	zassert_equal(rx_ptr->data1, MAIL1_DATA1, NULL);
	zassert_equal(rx_ptr->data2, MAIL1_DATA2, NULL);
	zassert_equal(rx_ptr->data3, MAIL1_DATA3, NULL);

	status = osMailFree(mail_id, rx_ptr);
	zassert_true(status == osOK, "osMailFree failure");

	/* Receive 2nd mail */
	evt = osMailGet(mail_id, osWaitForever);
	zassert_true(evt.status == osEventMail, "osMailGet failure");

	rx_ptr = evt.value.p;
	zassert_equal(rx_ptr->data1, MAIL2_DATA1, NULL);
	zassert_equal(rx_ptr->data2, MAIL2_DATA2, NULL);
	zassert_equal(rx_ptr->data3, MAIL2_DATA3, NULL);

	status = osMailFree(mail_id, rx_ptr);
	zassert_true(status == osOK, "osMailFree failure");
}

osThreadDef(send_thread, osPriorityNormal, 1, 0);

void test_mailq(void)
{
	osThreadId tid;

	mail_id = osMailCreate(osMailQ(mail), NULL);
	zassert_true(mail_id != NULL, "Mail creation failed");

	tid = osThreadCreate(osThread(send_thread), NULL);
	zassert_true(tid != NULL, "Thread creation failed");

	mail_recv();
}
