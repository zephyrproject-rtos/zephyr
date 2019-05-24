/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Synchronization demo using CMSIS RTOS V2 APIs.
 */

#include <zephyr.h>
#include <cmsis_os2.h>

/* specify delay between greetings (in ms); compute equivalent in ticks */
#define TIMER_TICKS  50
#define MSGLEN 12
#define Q_LEN 1
#define INITIAL_DATA_VALUE 5

const osTimerAttr_t timer_attr = {
	"myTimer", 0, NULL, 0U
};

osMessageQueueId_t message_id;

u32_t data;

static char __aligned(4) sample_mem[sizeof(data) * Q_LEN];

static const osMessageQueueAttr_t mem_attrs = {
	.name = "SampleMsgQ",
	.attr_bits = 0,
	.cb_mem = NULL,
	.cb_size = 0,
	.mq_mem = sample_mem,
	.mq_size = sizeof(data) * Q_LEN,
};

void read_msg_callback(void *arg)
{
	u32_t read_msg;
	osStatus_t status;

	status = osMessageQueueGet(message_id, (void *)&read_msg,
				   NULL, 0);
	if (status == osOK) {
		printk("Read from message queue: %d\n\n", read_msg);
	} else {
		printk("\n**Error reading message from message queue**\n");
	}
}

int send_msg_thread(void)
{
	osStatus_t status;

	status = osMessageQueuePut(message_id, &data, 0, osWaitForever);
	if (osMessageQueueGetCount(message_id) == 1 && status == osOK) {
		printk("Wrote to message queue: %d\n", data);
		return 0;
	}
	printk("\n**Error sending message to message queue**\n");
	return 1;
}

void main(void)
{
	osTimerId_t timer_id;
	osStatus_t status;
	u32_t counter = 10U;

	data = INITIAL_DATA_VALUE;

	message_id = osMessageQueueNew(Q_LEN, sizeof(data), &mem_attrs);

	status = osMessageQueuePut(message_id, &data, 0, osWaitForever);

	if (osMessageQueueGetCount(message_id) == 1 && status == osOK) {
		printk("Wrote to message queue: %d\n", data);
	} else {
		printk("\n**Error sending message to message queue**\n");
		goto exit;
	}

	timer_id = osTimerNew(read_msg_callback, osTimerPeriodic, NULL,
			      &timer_attr);
	osTimerStart(timer_id, TIMER_TICKS);

	while (--counter) {
		data++;
		if (send_msg_thread()) {
			/* Writing to message queue has failed */
			break;
		}
	}
	/* Let the last message be read */
	osDelay(TIMER_TICKS);

	osTimerStop(timer_id);
	osTimerDelete(timer_id);

exit:
	if (counter == 0U) {
		printk("Sample execution successful\n");
	} else {
		printk("Error in execution! \n");
	}
}
