/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2024 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <errno.h>
#include <string.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/posix/mqueue.h>
#include <zephyr/posix/pthread.h>

#define SIGEV_MASK (SIGEV_NONE | SIGEV_SIGNAL | SIGEV_THREAD)

typedef struct mqueue_object {
	sys_snode_t snode;
	char *mem_buffer;
	char *mem_obj;
	struct k_msgq queue;
	atomic_t ref_count;
	char *name;
	struct sigevent not;
} mqueue_object;

typedef struct mqueue_desc {
	char *mem_desc;
	mqueue_object *mqueue;
	uint32_t  flags;
} mqueue_desc;

K_SEM_DEFINE(mq_sem, 1, 1);

/* Initialize the list */
sys_slist_t mq_list = SYS_SLIST_STATIC_INIT(&mq_list);

int64_t timespec_to_timeoutms(const struct timespec *abstime);
static mqueue_object *find_in_list(const char *name);
static int32_t send_message(mqueue_desc *mqd, const char *msg_ptr, size_t msg_len,
			  k_timeout_t timeout);
static int32_t receive_message(mqueue_desc *mqd, char *msg_ptr, size_t msg_len,
			   k_timeout_t timeout);
static void remove_notification(mqueue_object *msg_queue);
static void remove_mq(mqueue_object *msg_queue);
static void *mq_notify_thread(void *arg);

/**
 * @brief Open a message queue.
 *
 * Number of message queue and descriptor to message queue are limited by
 * heap size. increase the size through CONFIG_HEAP_MEM_POOL_SIZE.
 *
 * See IEEE 1003.1
 */
mqd_t mq_open(const char *name, int oflags, ...)
{
	va_list va;
	mode_t mode;
	struct mq_attr *attrs = NULL;
	long msg_size = 0U, max_msgs = 0U;
	mqueue_object *msg_queue;
	mqueue_desc *msg_queue_desc = NULL, *mqd = (mqueue_desc *)(-1);
	char *mq_desc_ptr, *mq_obj_ptr, *mq_buf_ptr, *mq_name_ptr;

	va_start(va, oflags);
	if ((oflags & O_CREAT) != 0) {
		BUILD_ASSERT(sizeof(mode_t) <= sizeof(int));
		mode = va_arg(va, unsigned int);
		attrs = va_arg(va, struct mq_attr*);
	}
	va_end(va);

	if (attrs != NULL) {
		msg_size = attrs->mq_msgsize;
		max_msgs = attrs->mq_maxmsg;
	}

	if ((name == NULL) || ((oflags & O_CREAT) != 0 && (msg_size <= 0 ||
						      max_msgs <= 0))) {
		errno = EINVAL;
		return (mqd_t)mqd;
	}

	if ((strlen(name) + 1)  > CONFIG_MQUEUE_NAMELEN_MAX) {
		errno = ENAMETOOLONG;
		return (mqd_t)mqd;
	}

	/* Check if queue already exists */
	k_sem_take(&mq_sem, K_FOREVER);
	msg_queue = find_in_list(name);
	k_sem_give(&mq_sem);

	if ((msg_queue != NULL) && (oflags & O_CREAT) != 0 &&
	    (oflags & O_EXCL) != 0) {
		/* Message queue has already been opened and O_EXCL is set */
		errno = EEXIST;
		return (mqd_t)mqd;
	}

	if ((msg_queue == NULL) && (oflags & O_CREAT) == 0) {
		errno = ENOENT;
		return (mqd_t)mqd;
	}

	mq_desc_ptr = k_malloc(sizeof(struct mqueue_desc));
	if (mq_desc_ptr != NULL) {
		(void)memset(mq_desc_ptr, 0, sizeof(struct mqueue_desc));
		msg_queue_desc = (struct mqueue_desc *)mq_desc_ptr;
		msg_queue_desc->mem_desc = mq_desc_ptr;
	} else {
		goto free_mq_desc;
	}


	/* Allocate mqueue object for new message queue */
	if (msg_queue == NULL) {

		/* Check for message quantity and size in message queue */
		if (attrs->mq_msgsize > CONFIG_MSG_SIZE_MAX &&
		    attrs->mq_maxmsg > CONFIG_MSG_COUNT_MAX) {
			goto free_mq_desc;
		}

		mq_obj_ptr = k_malloc(sizeof(mqueue_object));
		if (mq_obj_ptr != NULL) {
			(void)memset(mq_obj_ptr, 0, sizeof(mqueue_object));
			msg_queue = (mqueue_object *)mq_obj_ptr;
			msg_queue->mem_obj = mq_obj_ptr;

		} else {
			goto free_mq_object;
		}

		mq_name_ptr = k_malloc(strlen(name) + 1);
		if (mq_name_ptr != NULL) {
			(void)memset(mq_name_ptr, 0, strlen(name) + 1);
			msg_queue->name = mq_name_ptr;

		} else {
			goto free_mq_name;
		}

		strcpy(msg_queue->name, name);

		mq_buf_ptr = k_malloc(msg_size * max_msgs * sizeof(uint8_t));
		if (mq_buf_ptr != NULL) {
			(void)memset(mq_buf_ptr, 0,
				     msg_size * max_msgs * sizeof(uint8_t));
			msg_queue->mem_buffer = mq_buf_ptr;
		} else {
			goto free_mq_buffer;
		}

		(void)atomic_set(&msg_queue->ref_count, 1);
		/* initialize zephyr message queue */
		k_msgq_init(&msg_queue->queue, msg_queue->mem_buffer, msg_size,
			    max_msgs);
		k_sem_take(&mq_sem, K_FOREVER);
		sys_slist_append(&mq_list, (sys_snode_t *)&(msg_queue->snode));
		k_sem_give(&mq_sem);

	} else {
		atomic_inc(&msg_queue->ref_count);
	}

	msg_queue_desc->mqueue = msg_queue;
	msg_queue_desc->flags = (oflags & O_NONBLOCK) != 0 ? O_NONBLOCK : 0;
	return (mqd_t)msg_queue_desc;

free_mq_buffer:
	k_free(mq_name_ptr);
free_mq_name:
	k_free(mq_obj_ptr);
free_mq_object:
	k_free(mq_desc_ptr);
free_mq_desc:
	errno = ENOSPC;
	return (mqd_t)mqd;
}

/**
 * @brief Close a message queue descriptor.
 *
 * See IEEE 1003.1
 */
int mq_close(mqd_t mqdes)
{
	mqueue_desc *mqd = (mqueue_desc *)mqdes;

	if (mqd == NULL) {
		errno = EBADF;
		return -1;
	}

	atomic_dec(&mqd->mqueue->ref_count);

	/* remove mq if marked for unlink */
	if (mqd->mqueue->name == NULL) {
		remove_mq(mqd->mqueue);
	}

	k_free(mqd->mem_desc);
	return 0;
}

/**
 * @brief Remove a message queue.
 *
 * See IEEE 1003.1
 */
int mq_unlink(const char *name)
{
	mqueue_object *msg_queue;

	k_sem_take(&mq_sem, K_FOREVER);
	msg_queue = find_in_list(name);

	if (msg_queue == NULL) {
		k_sem_give(&mq_sem);
		errno = EBADF;
		return -1;
	}

	k_free(msg_queue->name);
	msg_queue->name = NULL;
	k_sem_give(&mq_sem);
	remove_mq(msg_queue);
	return 0;
}

/**
 * @brief Send a message to a message queue.
 *
 * All messages in message queue are of equal priority.
 *
 * See IEEE 1003.1
 */
int mq_send(mqd_t mqdes, const char *msg_ptr, size_t msg_len,
	    unsigned int msg_prio)
{
	mqueue_desc *mqd = (mqueue_desc *)mqdes;

	return send_message(mqd, msg_ptr, msg_len, K_FOREVER);
}

/**
 * @brief Send message to a message queue within abstime time.
 *
 * All messages in message queue are of equal priority.
 *
 * See IEEE 1003.1
 */
int mq_timedsend(mqd_t mqdes, const char *msg_ptr, size_t msg_len,
		 unsigned int msg_prio, const struct timespec *abstime)
{
	mqueue_desc *mqd = (mqueue_desc *)mqdes;
	int32_t timeout = (int32_t) timespec_to_timeoutms(abstime);

	return send_message(mqd, msg_ptr, msg_len, K_MSEC(timeout));
}

/**
 * @brief Receive a message from a message queue.
 *
 * All messages in message queue are of equal priority.
 *
 * See IEEE 1003.1
 */
int mq_receive(mqd_t mqdes, char *msg_ptr, size_t msg_len,
		   unsigned int *msg_prio)
{
	mqueue_desc *mqd = (mqueue_desc *)mqdes;

	return receive_message(mqd, msg_ptr, msg_len, K_FOREVER);
}

/**
 * @brief Receive message from a message queue within abstime time.
 *
 * All messages in message queue are of equal priority.
 *
 * See IEEE 1003.1
 */
int mq_timedreceive(mqd_t mqdes, char *msg_ptr, size_t msg_len,
			unsigned int *msg_prio, const struct timespec *abstime)
{
	mqueue_desc *mqd = (mqueue_desc *)mqdes;
	int32_t timeout = (int32_t) timespec_to_timeoutms(abstime);

	return receive_message(mqd, msg_ptr, msg_len, K_MSEC(timeout));
}

/**
 * @brief Get message queue attributes.
 *
 * See IEEE 1003.1
 */
int mq_getattr(mqd_t mqdes, struct mq_attr *mqstat)
{
	mqueue_desc *mqd = (mqueue_desc *)mqdes;
	struct k_msgq_attrs attrs;

	if (mqd == NULL) {
		errno = EBADF;
		return -1;
	}

	k_sem_take(&mq_sem, K_FOREVER);
	k_msgq_get_attrs(&mqd->mqueue->queue, &attrs);
	mqstat->mq_flags = mqd->flags;
	mqstat->mq_maxmsg = attrs.max_msgs;
	mqstat->mq_msgsize = attrs.msg_size;
	mqstat->mq_curmsgs = attrs.used_msgs;
	k_sem_give(&mq_sem);
	return 0;
}

/**
 * @brief Set message queue attributes.
 *
 * See IEEE 1003.1
 */
int mq_setattr(mqd_t mqdes, const struct mq_attr *mqstat,
	       struct mq_attr *omqstat)
{
	mqueue_desc *mqd = (mqueue_desc *)mqdes;

	if (mqd == NULL) {
		errno = EBADF;
		return -1;
	}

	if (mqstat->mq_flags != 0 && mqstat->mq_flags != O_NONBLOCK) {
		errno = EINVAL;
		return -1;
	}

	if (omqstat != NULL) {
		mq_getattr(mqdes, omqstat);
	}

	k_sem_take(&mq_sem, K_FOREVER);
	mqd->flags = mqstat->mq_flags;
	k_sem_give(&mq_sem);

	return 0;
}

/**
 * @brief Notify process that a message is available.
 *
 * See IEEE 1003.1
 */
int mq_notify(mqd_t mqdes, const struct sigevent *notification)
{
	mqueue_desc *mqd = (mqueue_desc *)mqdes;

	if (mqd == NULL) {
		errno = EBADF;
		return -1;
	}

	mqueue_object *msg_queue = mqd->mqueue;

	if (notification == NULL) {
		if ((msg_queue->not.sigev_notify & SIGEV_MASK) == 0) {
			errno = EINVAL;
			return -1;
		}
		remove_notification(msg_queue);
		return 0;
	}

	if ((msg_queue->not.sigev_notify & SIGEV_MASK) != 0) {
		errno = EBUSY;
		return -1;
	}
	if (notification->sigev_notify == SIGEV_SIGNAL) {
		errno = ENOSYS;
		return -1;
	}
	if (notification->sigev_notify_attributes != NULL) {
		int ret = pthread_attr_setdetachstate(notification->sigev_notify_attributes,
						      PTHREAD_CREATE_DETACHED);
		if (ret != 0) {
			errno = ret;
			return -1;
		}
	}

	k_sem_take(&mq_sem, K_FOREVER);
	memcpy(&msg_queue->not, notification, sizeof(struct sigevent));
	k_sem_give(&mq_sem);

	return 0;
}

static void *mq_notify_thread(void *arg)
{
	mqueue_object *mqueue = (mqueue_object *)arg;
	struct sigevent *sevp = &mqueue->not;

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	if (sevp->sigev_notify_attributes == NULL) {
		pthread_detach(pthread_self());
	}

	sevp->sigev_notify_function(sevp->sigev_value);

	remove_notification(mqueue);

	pthread_exit(NULL);
	return NULL;
}

/* Internal functions */
static mqueue_object *find_in_list(const char *name)
{
	sys_snode_t *mq;
	mqueue_object *msg_queue;

	mq = mq_list.head;

	while (mq != NULL) {
		msg_queue = (mqueue_object *)mq;
		if (strcmp(msg_queue->name, name) == 0) {
			return msg_queue;
		}

		mq = mq->next;
	}

	return NULL;
}

static int32_t send_message(mqueue_desc *mqd, const char *msg_ptr, size_t msg_len,
			  k_timeout_t timeout)
{
	int32_t ret = -1;

	if (mqd == NULL) {
		errno = EBADF;
		return ret;
	}

	if ((mqd->flags & O_NONBLOCK) != 0U) {
		timeout = K_NO_WAIT;
	}

	if (msg_len >  mqd->mqueue->queue.msg_size) {
		errno = EMSGSIZE;
		return ret;
	}

	uint32_t msgq_num = k_msgq_num_used_get(&mqd->mqueue->queue);

	if (k_msgq_put(&mqd->mqueue->queue, (void *)msg_ptr, timeout) != 0) {
		errno = K_TIMEOUT_EQ(timeout, K_NO_WAIT) ? EAGAIN : ETIMEDOUT;
		return ret;
	}

	if (k_msgq_num_used_get(&mqd->mqueue->queue) - msgq_num > 0) {
		struct sigevent *sevp = &mqd->mqueue->not;

		if (sevp->sigev_notify == SIGEV_NONE) {
			sevp->sigev_notify_function(sevp->sigev_value);
		} else if (sevp->sigev_notify == SIGEV_THREAD) {
			pthread_t th;

			ret = pthread_create(&th,
					     sevp->sigev_notify_attributes,
					     mq_notify_thread,
					     mqd->mqueue);
		}
	}

	return 0;
}

static int32_t receive_message(mqueue_desc *mqd, char *msg_ptr, size_t msg_len,
			     k_timeout_t timeout)
{
	int ret = -1;

	if (mqd == NULL) {
		errno = EBADF;
		return ret;
	}

	if (msg_len < mqd->mqueue->queue.msg_size) {
		errno = EMSGSIZE;
		return ret;
	}

	if ((mqd->flags & O_NONBLOCK) != 0U) {
		timeout = K_NO_WAIT;
	}

	if (k_msgq_get(&mqd->mqueue->queue, (void *)msg_ptr, timeout) != 0) {
		errno = K_TIMEOUT_EQ(timeout, K_NO_WAIT) ? EAGAIN : ETIMEDOUT;
	} else {
		ret = mqd->mqueue->queue.msg_size;
	}

	return ret;
}

static void remove_mq(mqueue_object *msg_queue)
{
	if (atomic_cas(&msg_queue->ref_count, 0, 0)) {
		k_sem_take(&mq_sem, K_FOREVER);
		sys_slist_find_and_remove(&mq_list, (sys_snode_t *) msg_queue);
		k_sem_give(&mq_sem);

		/* Free mq buffer and pbject */
		k_free(msg_queue->mem_buffer);
		k_free(msg_queue->mem_obj);
	}
}

static void remove_notification(mqueue_object *msg_queue)
{
	k_sem_take(&mq_sem, K_FOREVER);
	memset(&msg_queue->not, 0, sizeof(struct sigevent));
	k_sem_give(&mq_sem);
}
