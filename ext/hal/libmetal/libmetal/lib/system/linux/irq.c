/*
 * Copyright (c) 2016, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file	linux/irq.c
 * @brief	Linux libmetal irq operations
 */

#include <pthread.h>
#include <sched.h>
#include <metal/device.h>
#include <metal/irq.h>
#include <metal/irq_controller.h>
#include <metal/sys.h>
#include <metal/mutex.h>
#include <metal/list.h>
#include <metal/utilities.h>
#include <metal/alloc.h>
#include <sys/time.h>
#include <sys/eventfd.h>
#include <sched.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <poll.h>
#include <unistd.h>

#define MAX_IRQS	(FD_SETSIZE - 1)  /**< maximum number of irqs */

static struct metal_device *irqs_devs[MAX_IRQS]; /**< Linux devices for IRQs */
static int irq_notify_fd; /**< irq handling state change notification file
			       descriptor */
static metal_mutex_t irq_lock; /**< irq handling lock */

static bool irq_handling_stop; /**< stop interrupts handling */

static pthread_t irq_pthread; /**< irq handling thread id */

/**< Indicate which IRQ is enabled */
static unsigned long
irqs_enabled[metal_div_round_up(MAX_IRQS, METAL_BITS_PER_ULONG)];

static struct metal_irq irqs[MAX_IRQS]; /**< Linux IRQs array */

/* Static functions */
static void metal_linux_irq_set_enable(struct metal_irq_controller *irq_cntr,
				       int irq, unsigned int state);

/**< Linux IRQ controller */
static METAL_IRQ_CONTROLLER_DECLARE(linux_irq_cntr,
				    0, MAX_IRQS,
				    NULL,
				    metal_linux_irq_set_enable, NULL,
				    irqs)

unsigned int metal_irq_save_disable()
{
	/* This is to avoid deadlock if it is called in ISR */
	if (pthread_self() == irq_pthread)
		return 0;
	metal_mutex_acquire(&irq_lock);
	return 0;
}

void metal_irq_restore_enable(unsigned flags)
{
	(void)flags;
	if (pthread_self() != irq_pthread)
		metal_mutex_release(&irq_lock);
}

static int metal_linux_irq_notify()
{
	uint64_t val = 1;
	int ret;

	ret = write(irq_notify_fd, &val, sizeof(val));
	if (ret < 0) {
		metal_log(METAL_LOG_ERROR, "%s failed\n", __func__);
	}
	return ret;
}

static void metal_linux_irq_set_enable(struct metal_irq_controller *irq_cntr,
				       int irq, unsigned int state)
{
	int offset, ret;

	if (irq < irq_cntr->irq_base ||
	    irq >= irq_cntr->irq_base + irq_cntr->irq_num) {
		metal_log(METAL_LOG_ERROR, "%s: invalid irq %d\n",
			  __func__, irq);
		return;
	}
	offset = irq - linux_irq_cntr.irq_base;
	metal_mutex_acquire(&irq_lock);
	if (state == METAL_IRQ_ENABLE)
		metal_bitmap_set_bit(irqs_enabled, offset);
	else
		metal_bitmap_clear_bit(irqs_enabled, offset);
	metal_mutex_release(&irq_lock);
	/* Notify IRQ thread that IRQ state has changed */
	ret = metal_linux_irq_notify();
	if (ret < 0) {
		metal_log(METAL_LOG_ERROR, "%s: failed to notify set %d enable\n",
			  __func__, irq);
	}
}

/**
  * @brief       IRQ handler
  * @param[in]   args  not used. required for pthread.
  */
static void *metal_linux_irq_handling(void *args)
{
	struct sched_param param;
	uint64_t val;
	int ret;
	int i, j, pfds_total;
	struct pollfd *pfds;

	(void) args;

	pfds = (struct pollfd *)malloc(FD_SETSIZE * sizeof(struct pollfd));
	if (!pfds) {
		metal_log(METAL_LOG_ERROR, "%s: failed to allocate irq fds mem.\n",
			  __func__);
		return NULL;
	}

	param.sched_priority = sched_get_priority_max(SCHED_FIFO);
	/* Ignore the set scheduler error */
	ret = sched_setscheduler(0, SCHED_FIFO, &param);
	if (ret) {
		metal_log(METAL_LOG_WARNING, "%s: Failed to set scheduler: %s.\n",
			  __func__, strerror(ret));
	}

	while (1) {
		metal_mutex_acquire(&irq_lock);
		if (irq_handling_stop == true) {
			/* Killing this IRQ handling thread */
			metal_mutex_release(&irq_lock);
			break;
		}

		/* Get the fdset */
		memset(pfds, 0, MAX_IRQS * sizeof(struct pollfd));
		pfds[0].fd = irq_notify_fd;
		pfds[0].events = POLLIN;
		j = 1;
		metal_bitmap_for_each_set_bit(irqs_enabled, i,
					      linux_irq_cntr.irq_num) {
			pfds[j].fd = i;
			pfds[j].events = POLLIN;
			j++;
		}
		metal_mutex_release(&irq_lock);
		/* Wait for interrupt */
		ret = poll(pfds, j, -1);
		if (ret < 0) {
			metal_log(METAL_LOG_ERROR, "%s: poll() failed: %s.\n",
				  __func__, strerror(errno));
			break;
		}
		/* Waken up from interrupt */
		pfds_total = j;
		for (i = 0; i < pfds_total; i++) {
			if ( (pfds[i].fd == irq_notify_fd) &&
			     (pfds[i].revents & (POLLIN | POLLRDNORM))) {
				/* IRQ registration change notification */
				if (read(pfds[i].fd, (void*)&val, sizeof(uint64_t)) < 0)
					metal_log(METAL_LOG_ERROR,
						  "%s, read irq fd %d failed.\n",
						  __func__, pfds[i].fd);
			} else if ((pfds[i].revents & (POLLIN | POLLRDNORM))) {
				struct metal_device *dev = NULL;
				int irq_handled = 0;
				int fd;

				fd = pfds[i].fd;
				dev = irqs_devs[fd];
				metal_mutex_acquire(&irq_lock);
				if (metal_irq_handle(&irqs[fd], fd)
				    == METAL_IRQ_HANDLED)
					irq_handled = 1;
				if (irq_handled) {
					if (dev && dev->bus->ops.dev_irq_ack)
						dev->bus->ops.dev_irq_ack(dev->bus, dev, fd);
				}
				metal_mutex_release(&irq_lock);
			} else if (pfds[i].revents) {
				metal_log(METAL_LOG_DEBUG,
					  "%s: poll unexpected. fd %d: %d\n",
					  __func__, pfds[i].fd, pfds[i].revents);
			}
		}
	}
	free(pfds);
	return NULL;
}

/**
  * @brief irq handling initialization
  * @return 0 on sucess, non-zero on failure
  */
int metal_linux_irq_init()
{
	int ret;

	memset(&irqs, 0, sizeof(irqs));

	irq_notify_fd = eventfd(0,0);
	if (irq_notify_fd < 0) {
		metal_log(METAL_LOG_ERROR, "Failed to create eventfd for IRQ handling.\n");
		return  -EAGAIN;
	}

	metal_mutex_init(&irq_lock);
	irq_handling_stop = false;
	ret = metal_irq_register_controller(&linux_irq_cntr);
	if (ret < 0) {
		metal_log(METAL_LOG_ERROR,
			  "Linux IRQ controller failed to register.\n");
		return -EINVAL;
	}
	ret = pthread_create(&irq_pthread, NULL,
				metal_linux_irq_handling, NULL);
	if (ret != 0) {
		metal_log(METAL_LOG_ERROR, "Failed to create IRQ thread: %d.\n", ret);
		return -EAGAIN;
	}

	return 0;
}

/**
  * @brief irq handling shutdown
  */
void metal_linux_irq_shutdown()
{
	int ret;

	metal_log(METAL_LOG_DEBUG, "%s\n", __func__);
	irq_handling_stop = true;
	metal_linux_irq_notify();
	ret = pthread_join(irq_pthread, NULL);
	if (ret) {
		metal_log(METAL_LOG_ERROR, "Failed to join IRQ thread: %d.\n", ret);
	}
	close(irq_notify_fd);
	metal_mutex_deinit(&irq_lock);
}

void metal_linux_irq_register_dev(struct metal_device *dev, int irq)
{
	if (irq > MAX_IRQS) {
		metal_log(METAL_LOG_ERROR, "Failed to register device to irq %d\n",
			  irq);
		return;
	}
	irqs_devs[irq] = dev;
}
