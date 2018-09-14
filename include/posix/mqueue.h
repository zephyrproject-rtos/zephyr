/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_POSIX_MQUEUE_H_
#define ZEPHYR_INCLUDE_POSIX_MQUEUE_H_

#include <kernel.h>
#include <posix/time.h>
#include "sys/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void *mqd_t;
typedef unsigned int mode_t;

typedef struct mq_attr {
	long mq_flags;
	long mq_maxmsg;
	long mq_msgsize;
	long mq_curmsgs;	/* Number of messages currently queued. */
} mq_attr;

/* FIXME: below should be defined into fcntl.h file.
 * This is temporarily put here.
 */

#ifndef _SYS_FCNTL_H_
#define O_CREAT_POS	9
#define O_CREAT         (1 << O_CREAT_POS)

#define O_EXCL_POS	11
#define O_EXCL          (1 << O_EXCL_POS)

#define O_NONBLOCK_POS	14
#define O_NONBLOCK      (1 << O_NONBLOCK_POS)

#define O_RDONLY        0
#define O_WRONLY        1
#define O_RDWR          2
#endif /* _SYS_FCNTL_H_ */

mqd_t mq_open(const char *name, int oflags, ...);
int mq_close(mqd_t mqdes);
int mq_unlink(const char *name);
int mq_getattr(mqd_t mqdes, struct mq_attr *mqstat);
int mq_receive(mqd_t mqdes, char *msg_ptr, size_t msg_len,
		   unsigned int *msg_prio);
int mq_send(mqd_t mqdes, const char *msg_ptr, size_t msg_len,
	    unsigned int msg_prio);
int mq_setattr(mqd_t mqdes, const struct mq_attr *mqstat,
	       struct mq_attr *omqstat);
int mq_timedreceive(mqd_t mqdes, char *msg_ptr, size_t msg_len,
			unsigned int *msg_prio, const struct timespec *abstime);
int mq_timedsend(mqd_t mqdes, const char *msg_ptr, size_t msg_len,
		 unsigned int msg_prio, const struct timespec *abstime);

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_INCLUDE_POSIX_MQUEUE_H_ */
