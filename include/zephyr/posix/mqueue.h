/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Message queues.
 * @ingroup posix
 *
 * Provides the POSIX message queue API for inter-process (or inter-thread)
 * communication via named, persistent, prioritized message queues.
 *
 * @posix_header{mqueue.h}
 */

#ifndef ZEPHYR_INCLUDE_POSIX_MQUEUE_H_
#define ZEPHYR_INCLUDE_POSIX_MQUEUE_H_

#include <time.h>
#include <signal.h>

#include <zephyr/kernel.h>
#include <zephyr/posix/fcntl.h>
#include <zephyr/posix/sys/stat.h>
#include <zephyr/posix/posix_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque message queue descriptor returned by mq_open().
 */
typedef void *mqd_t;

/**
 * @brief Message queue attributes used with mq_getattr() and mq_setattr().
 */
struct mq_attr {
	/**
	 * @brief Message queue flags (@c O_NONBLOCK).
	 */
	long mq_flags;
	/**
	 * @brief Maximum number of messages.
	 */
	long mq_maxmsg;
	/**
	 * @brief Maximum message size in bytes.
	 */
	long mq_msgsize;
	/**
	 * @brief Number of messages currently queued.
	 */
	long mq_curmsgs;
};

/**
 * @brief Open or create a message queue.
 *
 * @param name   Queue name (must start with '/').
 * @param oflags Open flags (@c O_RDONLY, @c O_WRONLY, @c O_RDWR, @c O_CREAT, @c O_EXCL,
 *               @c O_NONBLOCK).
 * @param ...    If @c O_CREAT is set: mode_t mode, struct mq_attr *attr.
 *
 * @return Message queue descriptor on success, or @c (mqd_t)-1 on failure.
 *
 * @posix_func{mq_open}
 */
mqd_t mq_open(const char *name, int oflags, ...);

/**
 * @brief Close a message queue descriptor.
 *
 * @param mqdes Message queue descriptor.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{mq_close}
 */
int mq_close(mqd_t mqdes);

/**
 * @brief Remove a named message queue.
 *
 * @param name Queue name.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{mq_unlink}
 */
int mq_unlink(const char *name);

/**
 * @brief Get the attributes of a message queue.
 *
 * @param mqdes  Message queue descriptor.
 * @param mqstat Output: current attributes.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{mq_getattr}
 */
int mq_getattr(mqd_t mqdes, struct mq_attr *mqstat);

/**
 * @brief Receive the oldest highest-priority message from a queue.
 *
 * @param mqdes    Message queue descriptor (opened with @c O_RDONLY or @c O_RDWR).
 * @param msg_ptr  Buffer to receive the message.
 * @param msg_len  Size of @p msg_ptr (must be >= mq_msgsize).
 * @param msg_prio Output: priority of the received message, or NULL.
 *
 * @return Number of bytes in the received message on success, or -1 on failure.
 *
 * @posix_func{mq_receive}
 */
int mq_receive(mqd_t mqdes, char *msg_ptr, size_t msg_len,
		   unsigned int *msg_prio);

/**
 * @brief Add a message to a queue.
 *
 * @param mqdes    Message queue descriptor (opened with @c O_WRONLY or @c O_RDWR).
 * @param msg_ptr  Message to send.
 * @param msg_len  Size of the message in bytes.
 * @param msg_prio Priority of the message (higher value = higher priority).
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{mq_send}
 */
int mq_send(mqd_t mqdes, const char *msg_ptr, size_t msg_len,
	    unsigned int msg_prio);

/**
 * @brief Set the attributes of a message queue.
 *
 * @param mqdes   Message queue descriptor.
 * @param mqstat  New attributes (only mq_flags is used).
 * @param omqstat Output: previous attributes, or NULL.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{mq_setattr}
 */
int mq_setattr(mqd_t mqdes, const struct mq_attr *mqstat,
	       struct mq_attr *omqstat);

/**
 * @brief Receive a message from a queue with an absolute timeout.
 *
 * @param mqdes    Message queue descriptor.
 * @param msg_ptr  Buffer to receive the message.
 * @param msg_len  Size of @p msg_ptr.
 * @param msg_prio Output: message priority, or NULL.
 * @param abstime  Absolute timeout (@c CLOCK_REALTIME).
 *
 * @return Number of bytes in the message on success, or -1 on failure.
 *
 * @posix_func{mq_timedreceive}
 */
int mq_timedreceive(mqd_t mqdes, char *msg_ptr, size_t msg_len,
			unsigned int *msg_prio, const struct timespec *abstime);

/**
 * @brief Send a message to a queue with an absolute timeout.
 *
 * @param mqdes    Message queue descriptor.
 * @param msg_ptr  Message to send.
 * @param msg_len  Size of the message in bytes.
 * @param msg_prio Priority of the message.
 * @param abstime  Absolute timeout (@c CLOCK_REALTIME).
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{mq_timedsend}
 */
int mq_timedsend(mqd_t mqdes, const char *msg_ptr, size_t msg_len,
		 unsigned int msg_prio, const struct timespec *abstime);

/**
 * @brief Register for notification when a message arrives on an empty queue.
 *
 * @param mqdes        Message queue descriptor.
 * @param notification Notification specification, or NULL to cancel.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{mq_notify}
 */
int mq_notify(mqd_t mqdes, const struct sigevent *notification);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_MQUEUE_H_ */
