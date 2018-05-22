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
#include <metal/sys.h>
#include <metal/mutex.h>
#include <metal/list.h>
#include <metal/utilities.h>
#include <metal/alloc.h>
#include <sys/time.h>
#include <sys/eventfd.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>

#define MAX_IRQS           FD_SETSIZE  /**< maximum number of irqs */
#define METAL_IRQ_STOP     0xFFFFFFFF  /**< stop interrupts handling thread */

/** IRQ handler descriptor structure */
struct metal_irq_hddesc {
	metal_irq_handler hd;     /**< irq handler */
	struct metal_device *dev; /**< metal device */
	void *drv_id;             /**< id to identify the driver
	                               of the irq handler*/
	struct metal_list list;   /**< handler list container */
};

struct metal_irqs_state {
	struct metal_irq_hddesc hds[MAX_IRQS]; /**< irqs handlers descriptor */
	signed char irq_reg_stat[MAX_IRQS]; /**< irqs registration statistics.
	                                      It restore how many handlers have
	                                      been registered for each IRQ. */

	int   irq_reg_fd; /**< irqs registration notification file
	                       descriptor */

	metal_mutex_t irq_lock; /**< irq handling lock */

	unsigned int irq_state; /**< global irq handling state */

	pthread_t    irq_pthread; /**< irq handling thread id */
};

struct metal_irqs_state _irqs;

int metal_irq_register(int irq,
		       metal_irq_handler hd,
		       struct metal_device *dev,
		       void *drv_id)
{
	uint64_t val = 1;
	struct metal_irq_hddesc *hd_desc;
	struct metal_list *h_node;
	int ret;

	if ((irq < 0) || (irq >= MAX_IRQS)) {
		metal_log(METAL_LOG_ERROR,
			  "%s: irq %d is larger than the max supported %d.\n",
			  __func__, irq, MAX_IRQS - 1);
		return -EINVAL;
	}

	if ((drv_id == NULL) || (hd == NULL)) {
		metal_log(METAL_LOG_ERROR, "%s: irq %d need drv_id and hd.\n",
			__func__, irq);
		return -EINVAL;
	}

	metal_mutex_acquire(&_irqs.irq_lock);
	if (_irqs.irq_state == METAL_IRQ_STOP) {
		metal_log(METAL_LOG_ERROR,
			  "%s: failed. metal IRQ handling has stopped.\n",
			  __func__);
		metal_mutex_release(&_irqs.irq_lock);
		return -EINVAL;
	}

	metal_list_for_each(&_irqs.hds[irq].list, h_node) {
		hd_desc = metal_container_of(h_node, struct metal_irq_hddesc, list);

		/* if drv_id already exist reject */
		if ((hd_desc->drv_id == drv_id) &&
		    ((dev == NULL) || (hd_desc->dev == dev))) {
			metal_log(METAL_LOG_ERROR, "%s: irq %d already registered."
			          "Will not register again.\n",
			           __func__, irq);
			metal_mutex_release(&_irqs.irq_lock);
			return -EINVAL;
		}
		/* drv_id not used, get out of metal_list_for_each */
		break;
	}

	/* Add to the end */
	hd_desc = metal_allocate_memory(sizeof(struct metal_irq_hddesc));
	if (hd_desc == NULL) {
		metal_log(METAL_LOG_ERROR,
		          "%s: irq %d cannot allocate mem for drv_id %d.\n",
		          __func__, irq, drv_id);
		metal_mutex_release(&_irqs.irq_lock);
		return -ENOMEM;
	}
	hd_desc->hd = hd;
	hd_desc->drv_id = drv_id;
	hd_desc->dev = dev;
	metal_list_add_tail(&_irqs.hds[irq].list, &hd_desc->list);

	_irqs.irq_reg_stat[irq]++;
	metal_mutex_release(&_irqs.irq_lock);

	ret = write(_irqs.irq_reg_fd, &val, sizeof(val));
	if (ret < 0) {
		metal_log(METAL_LOG_DEBUG, "%s: write failed IRQ %d\n", __func__, irq);
	}

	metal_log(METAL_LOG_DEBUG, "%s: registered IRQ %d\n", __func__, irq);
	return 0;
}

int metal_irq_unregister(int irq,
			metal_irq_handler hd,
			struct metal_device *dev,
			void *drv_id)
{
	uint64_t val = 1;
	struct metal_irq_hddesc *hd_desc;
	struct metal_list *h_node;
	int ret;
	unsigned int delete_count = 0;

	if ((irq < 0) || (irq >= MAX_IRQS)) {
		metal_log(METAL_LOG_ERROR,
			  "%s: irq %d is larger than the max supported %d.\n",
			  __func__, irq, MAX_IRQS);
		return -EINVAL;
	}

	metal_mutex_acquire(&_irqs.irq_lock);
	if (_irqs.irq_state == METAL_IRQ_STOP) {
		metal_log(METAL_LOG_ERROR,
			  "%s: failed. metal IRQ handling has stopped.\n", __func__);
		metal_mutex_release(&_irqs.irq_lock);
		return -EINVAL;
	}

	if (!hd && !drv_id && !dev) {
		if (0 == _irqs.irq_reg_stat[irq])
			goto no_entry;

		_irqs.irq_reg_stat[irq] = 0;
		goto out;
	}

	/* Search through handlers */
	metal_list_for_each(&_irqs.hds[irq].list, h_node) {
		hd_desc = metal_container_of(h_node, struct metal_irq_hddesc, list);

		if (((hd == NULL) || (hd_desc->hd == hd)) &&
		    ((drv_id == NULL) || (hd_desc->drv_id == drv_id)) &&
		    ((dev == NULL) || (hd_desc->dev == dev))) {
			if (_irqs.irq_reg_stat[irq] > 0)
				_irqs.irq_reg_stat[irq]--;
			h_node = h_node->prev;
			metal_list_del(h_node->next);
			metal_free_memory(hd_desc);
			delete_count++;
		}
	}

	if (delete_count)
		goto out;

no_entry:
	metal_log(METAL_LOG_DEBUG, "%s: No matching entry.\n", __func__);
	metal_mutex_release(&_irqs.irq_lock);
	return -ENOENT;
out:
	metal_mutex_release(&_irqs.irq_lock);
	ret = write(_irqs.irq_reg_fd, &val, sizeof(val));
	if (ret < 0) {
		metal_log(METAL_LOG_DEBUG, "%s: write failed IRQ %d\n", __func__, irq);
	}
	metal_log(METAL_LOG_DEBUG, "%s: unregistered IRQ %d (%d)\n", __func__, irq, delete_count);
	return 0;
}

unsigned int metal_irq_save_disable()
{
	metal_mutex_acquire(&_irqs.irq_lock);
	return 0;
}

void metal_irq_restore_enable(unsigned flags)
{
	(void)flags;
	metal_mutex_release(&_irqs.irq_lock);
}

void metal_irq_enable(unsigned int vector)
{
	(void)vector;
}

void metal_irq_disable(unsigned int vector)
{
	(void)vector;
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

	pfds = (struct pollfd *)malloc(MAX_IRQS * sizeof(struct pollfd));
	if (!pfds) {
		metal_log(METAL_LOG_ERROR, "%s: failed to allocate irq fds mem.\n",
			  __func__);
		return NULL;
	}

	param.sched_priority = sched_get_priority_max(SCHED_FIFO);
	/* Ignore the set scheduler error */
	ret = sched_setscheduler(0, SCHED_FIFO, &param);
	if (ret) {
		metal_log(METAL_LOG_WARNING, "%s: Failed to set scheduler: %d.\n",
			  __func__, ret);
	}

	while (1) {
		metal_mutex_acquire(&_irqs.irq_lock);
		if (_irqs.irq_state == METAL_IRQ_STOP) {
			/* Killing this IRQ handling thread */
			metal_mutex_release(&_irqs.irq_lock);
			break;
		}

		/* Get the fdset */
		memset(pfds, 0, MAX_IRQS * sizeof(struct pollfd));
		pfds[0].fd = _irqs.irq_reg_fd;
		pfds[0].events = POLLIN;
		for(i = 0, j = 1; i < MAX_IRQS && j < MAX_IRQS; i++) {
			if (_irqs.irq_reg_stat[i] > 0) {
				pfds[j].fd = i;
				pfds[j].events = POLLIN;
				j++;
			}
		}
		metal_mutex_release(&_irqs.irq_lock);
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
			if ( (pfds[i].fd == _irqs.irq_reg_fd) &&
			     (pfds[i].revents & (POLLIN | POLLRDNORM))) {
				/* IRQ registration change notification */
				if (read(pfds[i].fd, (void*)&val, sizeof(uint64_t)) < 0)
					metal_log(METAL_LOG_ERROR,
					"%s, read irq fd %d failed.\n",
					__func__, pfds[i].fd);
			} else if ((pfds[i].revents & (POLLIN | POLLRDNORM))) {
				struct metal_irq_hddesc *hd_desc; /**< irq handler descriptor */
				struct metal_device *dev = NULL; /**< metal device IRQ belongs to */
				int irq_handled = 0; /**< flag to indicate if irq is handled */
				struct metal_list *h_node;

				metal_list_for_each(&_irqs.hds[pfds[i].fd].list, h_node) {
					hd_desc = metal_container_of(h_node, struct metal_irq_hddesc, list);

					metal_mutex_acquire(&_irqs.irq_lock);
					if (!dev)
						dev = hd_desc->dev;
					metal_mutex_release(&_irqs.irq_lock);

					if ((hd_desc->hd)(pfds[i].fd, hd_desc->drv_id) == METAL_IRQ_HANDLED)
						irq_handled = 1;
				}
				if (irq_handled) {
					if (dev && dev->bus->ops.dev_irq_ack)
					    dev->bus->ops.dev_irq_ack(dev->bus, dev, i);
				}
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
	int ret, irq;

	memset(&_irqs, 0, sizeof(_irqs));

	/* init handlers list for each interrupt in table */
	for (irq=0; irq < MAX_IRQS; irq++) {
		metal_list_init(&_irqs.hds[irq].list);
	}

	_irqs.irq_reg_fd = eventfd(0,0);
	if (_irqs.irq_reg_fd < 0) {
		metal_log(METAL_LOG_ERROR, "Failed to create eventfd for IRQ handling.\n");
		return  -EAGAIN;
	}

	metal_mutex_init(&_irqs.irq_lock);
	ret = pthread_create(&_irqs.irq_pthread, NULL,
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
	uint64_t val = 1;

	metal_log(METAL_LOG_DEBUG, "%s\n", __func__);
	metal_mutex_acquire(&_irqs.irq_lock);
	_irqs.irq_state = METAL_IRQ_STOP;
	metal_mutex_release(&_irqs.irq_lock);
	ret = write (_irqs.irq_reg_fd, &val, sizeof(val));
	if (ret < 0) {
		metal_log(METAL_LOG_ERROR, "Failed to write.\n");
	}
	ret = pthread_join(_irqs.irq_pthread, NULL);
	if (ret) {
		metal_log(METAL_LOG_ERROR, "Failed to join IRQ thread: %d.\n", ret);
	}
	close(_irqs.irq_reg_fd);
	metal_mutex_deinit(&_irqs.irq_lock);
}
