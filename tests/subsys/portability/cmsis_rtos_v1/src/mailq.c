/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
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

#define TIMEOUT		500
#define Q_LEN		5

osMailQDef(mail, Q_LEN, struct sample_data);
osMailQId  mail_id;

void send_thread(void const *argument)
{
	int i;
	struct sample_data *tx_ptr;
	struct sample_data zeroblock;
	osStatus status;

	/* This is used for comparison later in the function */
	memset(&zeroblock, 0, sizeof(struct sample_data));

	status = osMailPut(mail_id, NULL);
	zassert_true(status == osErrorValue,
	 "Something's wrong with osMailPut. It is passing for NULL mail!");

	/* Wait for mail_recv to complete initial checks */
	osDelay(TIMEOUT);

	/* Prepare and send 1st mail */
	tx_ptr = osMailAlloc(mail_id, osWaitForever);
	zassert_true(tx_ptr != NULL, "Mail1 alloc failed");
	tx_ptr->data1 = MAIL1_DATA1;
	tx_ptr->data2 = MAIL1_DATA2;
	tx_ptr->data3 = MAIL1_DATA3;
	status = osMailPut(mail_id, tx_ptr);
	zassert_true(status == osOK, "osMailPut failure for mail1");

	/* Fill the queue with blocks of mails */
	for (i = 0; i < Q_LEN; i++) {

		/* Alternately use osMailAlloc and osMailCAlloc to ensure
		 * both the APIs are tested.
		 */
		if (i & 1) {
			tx_ptr = osMailCAlloc(mail_id, osWaitForever);
		} else {
			tx_ptr = osMailAlloc(mail_id, osWaitForever);
		}
		zassert_true(tx_ptr != NULL, "Mail alloc failed");

		tx_ptr->data1 = i;
		tx_ptr->data2 = i+1;
		tx_ptr->data3 = i+2;

		status = osMailPut(mail_id, tx_ptr);
		zassert_true(status == osOK,
				"osMailPut failure for mail!");
	}

	/* Try allocating mail to a full queue immediately
	 * before it is emptied out and assert failure
	 */
	tx_ptr = osMailAlloc(mail_id, 0);
	zassert_true(tx_ptr == NULL, "MailAlloc passed. Something's wrong");
	tx_ptr = osMailCAlloc(mail_id, 0);
	zassert_true(tx_ptr == NULL, "MailCAlloc passed. Something's wrong");

	/* Try allocating mail to a full queue within a duration
	 * less than TIMEOUT, before the queue is emptied out
	 */
	tx_ptr = osMailAlloc(mail_id, TIMEOUT/3);
	zassert_true(tx_ptr == NULL, "MailAlloc passed. Something's wrong");
	tx_ptr = osMailCAlloc(mail_id, TIMEOUT/3);
	zassert_true(tx_ptr == NULL, "MailCAlloc passed. Something's wrong");

	/* Send another mail after the queue is emptied */
	tx_ptr = osMailCAlloc(mail_id, TIMEOUT*2);
	zassert_true(tx_ptr != NULL, "Mail alloc failed");
	zassert_equal(memcmp(tx_ptr, &zeroblock, sizeof(struct sample_data)), 0,
		"osMailCAlloc returned memory not initialized to 0");

	tx_ptr->data1 = MAIL2_DATA1;
	tx_ptr->data2 = MAIL2_DATA2;
	tx_ptr->data3 = MAIL2_DATA3;
	status = osMailPut(mail_id, tx_ptr);
	zassert_true(status == osOK, "osMailPut failure for mail");
}

void mail_recv(void)
{
	int i;
	struct sample_data  *rx_ptr;
	osEvent  evt;
	osStatus status;

	/* Try getting mail immediately before the queue is populated */
	evt = osMailGet(mail_id, 0);
	zassert_true(evt.status == osOK,
			"Something's wrong with osMailGet!");

	/* Try receiving mail within a duration of TIMEOUT */
	evt = osMailGet(mail_id, TIMEOUT);
	zassert_true(evt.status == osEventTimeout,
		"Something's wrong with osMailGet!");

	/* Receive 1st mail */
	evt = osMailGet(mail_id, osWaitForever);
	zassert_true(evt.status == osEventMail, "osMailGet failure");

	rx_ptr = evt.value.p;
	zassert_equal(rx_ptr->data1, MAIL1_DATA1);
	zassert_equal(rx_ptr->data2, MAIL1_DATA2);
	zassert_equal(rx_ptr->data3, MAIL1_DATA3);

	status = osMailFree(mail_id, rx_ptr);
	zassert_true(status == osOK, "osMailFree failure");

	/* Wait for queue to get filled */
	osDelay(TIMEOUT);

	/* Empty the queue */
	for (i = 0; i < Q_LEN; i++) {
		evt = osMailGet(mail_id, osWaitForever);
		zassert_true(evt.status == osEventMail, "osMailGet failure");

		rx_ptr = evt.value.p;
		zassert_equal(rx_ptr->data1, i);
		zassert_equal(rx_ptr->data2, i + 1);
		zassert_equal(rx_ptr->data3, i + 2);

		status = osMailFree(mail_id, rx_ptr);
		zassert_true(status == osOK, "osMailFree failure");
	}

	/* Receive the next mail */
	evt = osMailGet(mail_id, osWaitForever);
	zassert_true(evt.status == osEventMail, "osMailGet failure");

	rx_ptr = evt.value.p;
	zassert_equal(rx_ptr->data1, MAIL2_DATA1);
	zassert_equal(rx_ptr->data2, MAIL2_DATA2);
	zassert_equal(rx_ptr->data3, MAIL2_DATA3);

	status = osMailFree(mail_id, rx_ptr);
	zassert_true(status == osOK, "osMailFree failure");
}

osThreadDef(send_thread, osPriorityNormal, 1, 0);

ZTEST(cmsis_mailq, test_mailq)
{
	osThreadId tid;

	mail_id = osMailCreate(osMailQ(mail), NULL);
	zassert_true(mail_id != NULL, "Mail creation failed");

	tid = osThreadCreate(osThread(send_thread), NULL);
	zassert_true(tid != NULL, "Thread creation failed");

	mail_recv();
}
ZTEST_SUITE(cmsis_mailq, NULL, NULL, NULL, NULL, NULL);
