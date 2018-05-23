/*
 * Copyright (c) 2016 - 2017, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	freertos/irq.c
 * @brief	FreeRTOS libmetal irq definitions.
 */

#include <errno.h>
#include <metal/irq.h>
#include <metal/sys.h>
#include <metal/log.h>
#include <metal/mutex.h>
#include <metal/list.h>
#include <metal/utilities.h>
#include <metal/alloc.h>

/** IRQ handlers descriptor structure */
struct metal_irq_hddesc {
	metal_irq_handler hd;     /**< irq handler */
	void *drv_id;             /**< id to identify the driver
	                               of the irq handler */
	struct metal_device *dev; /**< device identifier */
	struct metal_list node;   /**< node on irq handlers list */
};

/** IRQ descriptor structure */
struct metal_irq_desc {
	int irq;                  /**< interrupt number */
	struct metal_list hdls;   /**< interrupt handlers */
	struct metal_list node;   /**< node on irqs list */
};

/** IRQ state structure */
struct metal_irqs_state {
	struct metal_list irqs;    /**< interrupt descriptors */
	metal_mutex_t irq_lock;    /**< access lock */
};

static struct metal_irqs_state _irqs = {
	.irqs = METAL_INIT_LIST(_irqs.irqs),
	.irq_lock = METAL_MUTEX_INIT(_irqs.irq_lock),
};

int metal_irq_register(int irq,
                       metal_irq_handler hd,
                       struct metal_device *dev,
                       void *drv_id)
{
	struct metal_irq_desc *irq_p = NULL;
	struct metal_irq_hddesc *hdl_p;
	struct metal_list *node;
	unsigned int irq_flags_save;

	if (irq < 0) {
		metal_log(METAL_LOG_ERROR,
			  "%s: irq %d need to be a positive number\n",
		          __func__, irq);
		return -EINVAL;
	}

	if ((drv_id == NULL) || (hd == NULL)) {
		metal_log(METAL_LOG_ERROR, "%s: irq %d need drv_id and hd.\n",
		          __func__, irq);
		return -EINVAL;
	}

	/* Search for irq in list */
	metal_mutex_acquire(&_irqs.irq_lock);
	metal_list_for_each(&_irqs.irqs, node) {
		irq_p = metal_container_of(node, struct metal_irq_desc, node);

		if (irq_p->irq == irq) {
			struct metal_list *h_node;

			/* Check if drv_id already exist */
			metal_list_for_each(&irq_p->hdls, h_node) {
				hdl_p = metal_container_of(h_node,
							   struct metal_irq_hddesc,
							   node);

				/* if drv_id already exist reject */
				if ((hdl_p->drv_id == drv_id) &&
					((dev == NULL) || (hdl_p->dev == dev))) {
					metal_log(METAL_LOG_ERROR,
						  "%s: irq %d already registered."
							"Will not register again.\n",
							__func__, irq);
					metal_mutex_release(&_irqs.irq_lock);
					return -EINVAL;
				}
			}
			/* irq found and drv_id not used, get out of metal_list_for_each */
			break;
		}
	}

	/* Either need to add handler to an existing list or to a new one */
	hdl_p = metal_allocate_memory(sizeof(struct metal_irq_hddesc));
	if (hdl_p == NULL) {
		metal_log(METAL_LOG_ERROR,
		          "%s: irq %d cannot allocate mem for drv_id %d.\n",
		          __func__, irq, drv_id);
		metal_mutex_release(&_irqs.irq_lock);
		return -ENOMEM;
	}
	hdl_p->hd = hd;
	hdl_p->drv_id = drv_id;
	hdl_p->dev = dev;

	/* interrupt already registered, add handler to existing list*/
	if ((irq_p != NULL) && (irq_p->irq == irq)) {
		irq_flags_save = metal_irq_save_disable();
		metal_list_add_tail(&irq_p->hdls, &hdl_p->node);
		metal_irq_restore_enable(irq_flags_save);

		metal_log(METAL_LOG_DEBUG, "%s: success, irq %d add drv_id %p \n",
		          __func__, irq, drv_id);
		metal_mutex_release(&_irqs.irq_lock);
		return 0;
	}

	/* interrupt was not already registered, add */
	irq_p = metal_allocate_memory(sizeof(struct metal_irq_desc));
	if (irq_p == NULL) {
		metal_log(METAL_LOG_ERROR, "%s: irq %d cannot allocate mem.\n",
		          __func__, irq);
		metal_mutex_release(&_irqs.irq_lock);
		return -ENOMEM;
	}
	irq_p->irq = irq;
	metal_list_init(&irq_p->hdls);
	metal_list_add_tail(&irq_p->hdls, &hdl_p->node);

	irq_flags_save = metal_irq_save_disable();
	metal_list_add_tail(&_irqs.irqs, &irq_p->node);
	metal_irq_restore_enable(irq_flags_save);

	metal_log(METAL_LOG_DEBUG, "%s: success, added irq %d\n", __func__, irq);
	metal_mutex_release(&_irqs.irq_lock);
	return 0;
}

/* helper function for metal_irq_unregister() */
static void metal_irq_delete_node(struct metal_list *node, void *p_to_free)
{
	unsigned int irq_flags_save;

	irq_flags_save=metal_irq_save_disable();
	metal_list_del(node);
	metal_irq_restore_enable(irq_flags_save);
	metal_free_memory(p_to_free);
}

int metal_irq_unregister(int irq,
                         metal_irq_handler hd,
                         struct metal_device *dev,
                         void *drv_id)
{
	struct metal_irq_desc *irq_p;
	struct metal_list *node;

	if (irq < 0) {
		metal_log(METAL_LOG_ERROR, "%s: irq %d need to be a positive number\n",
		          __func__, irq);
		return -EINVAL;
	}

	/* Search for irq in list */
	metal_mutex_acquire(&_irqs.irq_lock);
	metal_list_for_each(&_irqs.irqs, node) {

		irq_p = metal_container_of(node, struct metal_irq_desc, node);

		if (irq_p->irq == irq) {
			struct metal_list *h_node, *h_prenode;
			struct metal_irq_hddesc *hdl_p;
			unsigned int delete_count = 0;

			metal_log(METAL_LOG_DEBUG, "%s: found irq %d\n",
				  __func__, irq);

			/* Search through handlers */
			metal_list_for_each(&irq_p->hdls, h_node) {
				hdl_p = metal_container_of(h_node,
							   struct metal_irq_hddesc,
							   node);

				if (((hd == NULL) || (hdl_p->hd == hd)) &&
				    ((drv_id == NULL) || (hdl_p->drv_id == drv_id)) &&
				    ((dev    == NULL) || (hdl_p->dev == dev))) {
					metal_log(METAL_LOG_DEBUG,
					          "%s: unregister hd=%p drv_id=%p dev=%p\n",
						  __func__, hdl_p->hd, hdl_p->drv_id, hdl_p->dev);
					h_prenode = h_node->prev;
					metal_irq_delete_node(h_node, hdl_p);
					delete_count++;
					h_node = h_prenode;
				}
			}

			/* we did not find any handler to delete */
			if (!delete_count) {
				metal_log(METAL_LOG_DEBUG, "%s: No matching entry\n",
				          __func__);
				metal_mutex_release(&_irqs.irq_lock);
				return -ENOENT;

			}

			/* if interrupt handlers list is empty, unregister interrupt */
			if (metal_list_is_empty(&irq_p->hdls)) {
				metal_log(METAL_LOG_DEBUG,
				          "%s: handlers list empty, unregister interrupt\n",
					  __func__);
				metal_irq_delete_node(node, irq_p);
			}

			metal_log(METAL_LOG_DEBUG, "%s: success\n", __func__);

			metal_mutex_release(&_irqs.irq_lock);
			return 0;
		}
	}

	metal_log(METAL_LOG_DEBUG, "%s: No matching IRQ entry\n", __func__);

	metal_mutex_release(&_irqs.irq_lock);
	return -ENOENT;
}

unsigned int metal_irq_save_disable(void)
{
	sys_irq_save_disable();
	return 0;
}

void metal_irq_restore_enable(unsigned int flags)
{
	(void)flags;

	sys_irq_restore_enable();
}

void metal_irq_enable(unsigned int vector)
{
	sys_irq_enable(vector);
}

void metal_irq_disable(unsigned int vector)
{
	sys_irq_disable(vector);
}

/**
 * @brief default handler
 */
void metal_irq_isr(unsigned int vector)
{
	struct metal_list *node;
	struct metal_irq_desc *irq_p;

	metal_list_for_each(&_irqs.irqs, node) {
		irq_p = metal_container_of(node, struct metal_irq_desc, node);

		if ((unsigned int)irq_p->irq == vector) {
			struct metal_list *h_node;
			struct metal_irq_hddesc *hdl_p;

			metal_list_for_each(&irq_p->hdls, h_node) {
				hdl_p = metal_container_of(h_node,
							   struct metal_irq_hddesc,
							   node);

				(hdl_p->hd)(vector, hdl_p->drv_id);
			}
		}
	}
}
