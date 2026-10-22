/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <nsi_errno.h>
#include <nsi_hw_scheduler.h>
#include <nsi_hws_models_if.h>
#include <nsi_tracing.h>
#include <nsi_utils.h>

#include "irq_ctrl.h"
#include "i2c_native_linux_bottom.h"

_Static_assert(I2C_NATIVE_LINUX_MAX_MSGS <= I2C_RDWR_IOCTL_MAX_MSGS,
	       "I2C_NATIVE_LINUX_MAX_MSGS is bigger than one I2C_RDWR ioctl accepts");

#ifdef I2C_NATIVE_LINUX_TEST_HOOKS
/* Provided by the test in place of the host calls */
int i2c_native_linux_host_open(const char *path);
int i2c_native_linux_host_ioctl(int fd, unsigned long request, void *arg);
int i2c_native_linux_host_close(int fd);
#else
NSI_INLINE int i2c_native_linux_host_open(const char *path)
{
	return open(path, O_RDWR | O_CLOEXEC);
}

NSI_INLINE int i2c_native_linux_host_ioctl(int fd, unsigned long request, void *arg)
{
	return ioctl(fd, request, arg);
}

NSI_INLINE int i2c_native_linux_host_close(int fd)
{
	return close(fd);
}
#endif

struct i2c_native_linux_dev {
	struct i2c_native_linux_dev *next;
	int fd;
	pthread_t thread;
	sem_t kick;
	/* The transfer in progress, owned by the host thread while busy is set */
	const struct i2c_native_linux_msg *msgs;
	int num;
	int result;
	atomic_bool busy;
	/* Started and not yet reported by i2c_native_linux_transfer_done() */
	bool pending;
	bool stop;
};

/* Open adapters, only used from the simulation thread */
static struct i2c_native_linux_dev *i2c_native_linux_devs;
static uint64_t i2c_native_linux_poll_time = NSI_NEVER;
/* Shared by all instances, set from the Zephyr side at init */
static uint32_t i2c_native_linux_irq;
static uint32_t i2c_native_linux_poll_us;

static int i2c_native_linux_transfer(int fd, const struct i2c_native_linux_msg *msgs, int num)
{
	struct i2c_msg lmsgs[I2C_NATIVE_LINUX_MAX_MSGS];
	struct i2c_rdwr_ioctl_data rdwr;
	const struct i2c_native_linux_msg *msg;
	struct i2c_msg *lmsg;
	int i;

	if (num <= 0 || num > I2C_NATIVE_LINUX_MAX_MSGS) {
		return -nsi_errno_to_mid(EINVAL);
	}

	for (i = 0; i < num; i++) {
		msg = &msgs[i];
		lmsg = &lmsgs[i];

		lmsg->addr = msg->addr;
		lmsg->flags = 0U;
		lmsg->len = msg->len;
		lmsg->buf = msg->buf;

		if ((msg->flags & I2C_NATIVE_LINUX_M_RD) != 0U) {
			lmsg->flags |= I2C_M_RD;
		}

		if ((msg->flags & I2C_NATIVE_LINUX_M_TEN) != 0U) {
			lmsg->flags |= I2C_M_TEN;
		}
	}

	rdwr.msgs = lmsgs;
	rdwr.nmsgs = (__u32)num;

	/* Not logged: a NACK is normal, e.g. when scanning */
	if (i2c_native_linux_host_ioctl(fd, I2C_RDWR, &rdwr) < 0) {
		return -nsi_errno_to_mid(errno);
	}

	return 0;
}

/* Runs the transfers so that the blocking ioctl() does not stall the simulation */
static void *i2c_native_linux_thread(void *arg)
{
	struct i2c_native_linux_dev *dev = arg;

	while (true) {
		/* Retried when interrupted by a signal */
		while (sem_wait(&dev->kick) != 0) {
			if (errno != EINTR) {
				nsi_print_warning("i2c-native-linux: sem_wait() failed: %s\n",
						  strerror(errno));
				return NULL;
			}
		}

		if (dev->stop) {
			break;
		}

		dev->result = i2c_native_linux_transfer(dev->fd, dev->msgs, dev->num);
		/* Publishes the result to the simulation thread */
		atomic_store(&dev->busy, false);
	}

	return NULL;
}

/* Raise the interrupt once a transfer is done, keep polling while one is pending */
static void i2c_native_linux_poll(void)
{
	struct i2c_native_linux_dev *dev;
	bool pending = false;

	for (dev = i2c_native_linux_devs; dev != NULL; dev = dev->next) {
		if (!dev->pending) {
			continue;
		}

		if (!atomic_load(&dev->busy)) {
			hw_irq_ctrl_set_irq(i2c_native_linux_irq);
		} else {
			pending = true;
		}
	}

	if (pending) {
		i2c_native_linux_poll_time = nsi_hws_get_time() + i2c_native_linux_poll_us;
	} else {
		i2c_native_linux_poll_time = NSI_NEVER;
	}
}

NSI_HW_EVENT(i2c_native_linux_poll_time, i2c_native_linux_poll, 500);

void *i2c_native_linux_open(const char *path, uint32_t irq, uint32_t poll_us, int *err)
{
	struct i2c_native_linux_dev *dev;
	unsigned long funcs;
	int ret;
	int fd;

	i2c_native_linux_irq = irq;
	i2c_native_linux_poll_us = poll_us;

	fd = i2c_native_linux_host_open(path);
	if (fd < 0) {
		*err = -nsi_errno_to_mid(errno);
		nsi_print_warning("Failed to open %s: %s\n", path, strerror(errno));
		return NULL;
	}

	if (i2c_native_linux_host_ioctl(fd, I2C_FUNCS, &funcs) < 0) {
		*err = -nsi_errno_to_mid(errno);
		nsi_print_warning("%s is not an i2c-dev node: %s\n", path, strerror(errno));
		i2c_native_linux_host_close(fd);
		return NULL;
	}

	/* I2C_RDWR needs a plain I2C adapter */
	if ((funcs & I2C_FUNC_I2C) == 0U) {
		*err = -nsi_errno_to_mid(EOPNOTSUPP);
		nsi_print_warning("%s does not support plain I2C transfers\n", path);
		i2c_native_linux_host_close(fd);
		return NULL;
	}

	dev = calloc(1, sizeof(*dev));
	if (dev == NULL) {
		*err = -nsi_errno_to_mid(ENOMEM);
		i2c_native_linux_host_close(fd);
		return NULL;
	}

	dev->fd = fd;
	sem_init(&dev->kick, 0, 0);

	ret = pthread_create(&dev->thread, NULL, i2c_native_linux_thread, dev);
	if (ret != 0) {
		*err = -nsi_errno_to_mid(ret);
		nsi_print_warning("Failed to start the transfer thread of %s: %s\n", path,
				  strerror(ret));
		sem_destroy(&dev->kick);
		i2c_native_linux_host_close(fd);
		free(dev);
		return NULL;
	}

	dev->next = i2c_native_linux_devs;
	i2c_native_linux_devs = dev;

	return dev;
}

void i2c_native_linux_transfer_start(void *handle, const struct i2c_native_linux_msg *msgs,
				     int num)
{
	struct i2c_native_linux_dev *dev = handle;

	dev->msgs = msgs;
	dev->num = num;
	dev->pending = true;
	atomic_store(&dev->busy, true);
	sem_post(&dev->kick);

	/* An ongoing poll of another instance is not delayed */
	if (i2c_native_linux_poll_time == NSI_NEVER) {
		i2c_native_linux_poll_time = nsi_hws_get_time() + i2c_native_linux_poll_us;
		nsi_hws_find_next_event();
	}
}

bool i2c_native_linux_transfer_done(void *handle, int *result)
{
	struct i2c_native_linux_dev *dev = handle;

	if (!dev->pending || atomic_load(&dev->busy)) {
		return false;
	}

	dev->pending = false;
	*result = dev->result;

	return true;
}

void i2c_native_linux_close_all(void)
{
	struct i2c_native_linux_dev *dev;
	struct i2c_native_linux_dev *next;

	for (dev = i2c_native_linux_devs; dev != NULL; dev = next) {
		next = dev->next;

		dev->stop = true;
		sem_post(&dev->kick);
		pthread_join(dev->thread, NULL);

		sem_destroy(&dev->kick);
		i2c_native_linux_host_close(dev->fd);
		free(dev);
	}

	i2c_native_linux_devs = NULL;
}
