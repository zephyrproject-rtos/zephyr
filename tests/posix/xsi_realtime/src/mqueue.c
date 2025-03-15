/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2024 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fcntl.h>
#include <mqueue.h>
#include <pthread.h>

#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define N_THR            2
#define MESSAGE_SIZE     16
#define MESG_COUNT_PERMQ 4

static char queue[16] = "server";

static char send_data[MESSAGE_SIZE] = "timed data send";

/*
 * For platforms that select CONFIG_KERNEL_COHERENCE, the receive buffer can
 * not be on the stack as the k_msgq that underlies the mq_timedsend() will
 * copy directly to the receiver's buffer when there is already a waiting
 * receiver.
 */

static char rec_data[MESSAGE_SIZE];

static void *sender_thread(void *p1)
{
	mqd_t mqd;
	struct timespec curtime;

	mqd = mq_open(queue, O_WRONLY);
	clock_gettime(CLOCK_MONOTONIC, &curtime);
	curtime.tv_sec += 1;
	zassert_false(mq_timedsend(mqd, send_data, MESSAGE_SIZE, 0, &curtime),
		      "Not able to send message in timer");
	usleep(USEC_PER_MSEC);
	zassert_false(mq_close(mqd), "unable to close message queue descriptor.");
	pthread_exit(p1);
	return NULL;
}

static void *receiver_thread(void *p1)
{
	mqd_t mqd;
	struct timespec curtime;

	mqd = mq_open(queue, O_RDONLY);
	clock_gettime(CLOCK_MONOTONIC, &curtime);
	curtime.tv_sec += 1;
	mq_timedreceive(mqd, rec_data, MESSAGE_SIZE, 0, &curtime);
	zassert_false(strcmp(rec_data, send_data), "Error in data reception. exp: %s act: %s",
		      send_data, rec_data);
	usleep(USEC_PER_MSEC);
	zassert_false(mq_close(mqd), "unable to close message queue descriptor.");
	pthread_exit(p1);
	return NULL;
}

ZTEST(xsi_realtime, test_mqueue)
{
	mqd_t mqd;
	struct mq_attr attrs;
	int32_t mode = 0777;
	int flags = O_RDWR | O_CREAT;
	void *retval;
	pthread_t newthread[N_THR];

	attrs.mq_msgsize = MESSAGE_SIZE;
	attrs.mq_maxmsg = MESG_COUNT_PERMQ;

	mqd = mq_open(queue, flags, mode, &attrs);

	for (int i = 0; i < N_THR; i++) {
		/* Creating threads */
		zassert_ok(pthread_create(&newthread[i], NULL,
					  (i % 2 == 0) ? receiver_thread : sender_thread, NULL));
	}

	usleep(USEC_PER_MSEC * 10U);

	for (int i = 0; i < N_THR; i++) {
		pthread_join(newthread[i], &retval);
	}

	zassert_false(mq_close(mqd), "unable to close message queue descriptor.");
	zassert_false(mq_unlink(queue), "Not able to unlink Queue");
}

static bool notification_executed;

void notify_function_basic(union sigval val)
{
	mqd_t mqd;
	bool *executed = (bool *)val.sival_ptr;

	mqd = mq_open(queue, O_RDONLY);

	mq_receive(mqd, rec_data, MESSAGE_SIZE, 0);
	zassert_ok(strcmp(rec_data, send_data), "Error in data reception. exp: %s act: %s",
		   send_data, rec_data);

	zassert_ok(mq_close(mqd), "Unable to close message queue descriptor.");

	*executed = true;
}

ZTEST(xsi_realtime, test_mqueue_notify_basic)
{
	mqd_t mqd;
	struct mq_attr attrs = {
		.mq_msgsize = MESSAGE_SIZE,
		.mq_maxmsg = MESG_COUNT_PERMQ,
	};
	struct sigevent not = {
		.sigev_notify = SIGEV_NONE,
		.sigev_value.sival_ptr = (void *)&notification_executed,
		.sigev_notify_function = notify_function_basic,
	};
	int32_t mode = 0777;
	int flags = O_RDWR | O_CREAT;

	notification_executed = false;
	memset(rec_data, 0, MESSAGE_SIZE);

	mqd = mq_open(queue, flags, mode, &attrs);

	zassert_ok(mq_notify(mqd, &not), "Unable to set notification.");

	zassert_ok(mq_send(mqd, send_data, MESSAGE_SIZE, 0), "Unable to send message");

	zassert_true(notification_executed, "Notification not triggered.");

	zassert_ok(mq_close(mqd), "Unable to close message queue descriptor.");
	zassert_ok(mq_unlink(queue), "Unable to unlink queue");
}

void notify_function_thread(union sigval val)
{
	mqd_t mqd;
	pthread_t sender = (pthread_t)val.sival_int;

	zassert_not_equal(sender, pthread_self(),
			  "Notification function should be executed from different thread.");

	mqd = mq_open(queue, O_RDONLY);

	mq_receive(mqd, rec_data, MESSAGE_SIZE, 0);
	zassert_ok(strcmp(rec_data, send_data), "Error in data reception. exp: %s act: %s",
		   send_data, rec_data);

	zassert_ok(mq_close(mqd), "Unable to close message queue descriptor.");

	notification_executed = true;
}

ZTEST(xsi_realtime, test_mqueue_notify_thread)
{
	mqd_t mqd;
	struct mq_attr attrs = {
		.mq_msgsize = MESSAGE_SIZE,
		.mq_maxmsg = MESG_COUNT_PERMQ,
	};
	struct sigevent not = {
		.sigev_notify = SIGEV_THREAD,
		.sigev_value.sival_int = (int)pthread_self(),
		.sigev_notify_function = notify_function_thread,
	};
	int32_t mode = 0777;
	int flags = O_RDWR | O_CREAT;

	notification_executed = false;
	memset(rec_data, 0, MESSAGE_SIZE);

	mqd = mq_open(queue, flags, mode, &attrs);

	zassert_ok(mq_notify(mqd, &not), "Unable to set notification.");

	zassert_ok(mq_send(mqd, send_data, MESSAGE_SIZE, 0), "Unable to send message");

	usleep(USEC_PER_MSEC * 100U);

	zassert_true(notification_executed, "Notification not triggered.");

	zassert_ok(mq_close(mqd), "Unable to close message queue descriptor.");
	zassert_ok(mq_unlink(queue), "Unable to unlink queue");
}

ZTEST(xsi_realtime, test_mqueue_notify_non_empty_queue)
{
	mqd_t mqd;
	struct mq_attr attrs = {
		.mq_msgsize = MESSAGE_SIZE,
		.mq_maxmsg = MESG_COUNT_PERMQ,
	};
	struct sigevent not = {
		.sigev_notify = SIGEV_NONE,
		.sigev_value.sival_ptr = (void *)&notification_executed,
		.sigev_notify_function = notify_function_basic,
	};
	int32_t mode = 0777;
	int flags = O_RDWR | O_CREAT;

	notification_executed = false;
	memset(rec_data, 0, MESSAGE_SIZE);

	mqd = mq_open(queue, flags, mode, &attrs);

	zassert_ok(mq_send(mqd, send_data, MESSAGE_SIZE, 0), "Unable to send message");

	zassert_ok(mq_notify(mqd, &not), "Unable to set notification.");

	zassert_false(notification_executed, "Notification shouldn't be processed.");

	mq_receive(mqd, rec_data, MESSAGE_SIZE, 0);
	zassert_false(strcmp(rec_data, send_data), "Error in data reception. exp: %s act: %s",
		      send_data, rec_data);

	memset(rec_data, 0, MESSAGE_SIZE);

	zassert_ok(mq_send(mqd, send_data, MESSAGE_SIZE, 0), "Unable to send message");

	zassert_true(notification_executed, "Notification not triggered.");

	zassert_ok(mq_close(mqd), "Unable to close message queue descriptor.");
	zassert_ok(mq_unlink(queue), "Unable to unlink queue");
}

ZTEST(xsi_realtime, test_mqueue_notify_errors)
{
	mqd_t mqd;
	struct mq_attr attrs = {
		.mq_msgsize = MESSAGE_SIZE,
		.mq_maxmsg = MESG_COUNT_PERMQ,
	};
	struct sigevent not = {
		.sigev_notify = SIGEV_SIGNAL,
		.sigev_value.sival_ptr = (void *)&notification_executed,
		.sigev_notify_function = notify_function_basic,
	};
	int32_t mode = 0777;
	int flags = O_RDWR | O_CREAT;

	zassert_not_ok(mq_notify(NULL, NULL), "Should return -1 and set errno to EBADF.");
	zassert_equal(errno, EBADF);

	mqd = mq_open(queue, flags, mode, &attrs);

	zassert_not_ok(mq_notify(mqd, NULL), "Should return -1 and set errno to EINVAL.");
	zassert_equal(errno, EINVAL);

	zassert_not_ok(mq_notify(mqd, &not), "SIGEV_SIGNAL not supported should return -1.");
	zassert_equal(errno, ENOSYS);

	not.sigev_notify = SIGEV_NONE;

	zassert_ok(mq_notify(mqd, &not),
		   "Unexpected error while asigning notification to the queue.");

	zassert_not_ok(mq_notify(mqd, &not),
		       "Can't assign notification when there is another assigned.");
	zassert_equal(errno, EBUSY);

	zassert_ok(mq_notify(mqd, NULL), "Unable to remove notification from the message queue.");

	zassert_ok(mq_close(mqd), "Unable to close message queue descriptor.");
	zassert_ok(mq_unlink(queue), "Unable to unlink queue");
}
