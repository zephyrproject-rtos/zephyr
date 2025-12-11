/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief File containing work specific definitions for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>

#include "work.h"

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF70_LOG_LEVEL);

K_THREAD_STACK_DEFINE(bh_wq_stack_area, CONFIG_NRF70_BH_WQ_STACK_SIZE);
struct k_work_q zep_wifi_bh_q;

K_THREAD_STACK_DEFINE(irq_wq_stack_area, CONFIG_NRF70_IRQ_WQ_STACK_SIZE);
struct k_work_q zep_wifi_intr_q;

#ifdef CONFIG_NRF70_TX_DONE_WQ_ENABLED
K_THREAD_STACK_DEFINE(tx_done_wq_stack_area, CONFIG_NRF70_TX_DONE_WQ_STACK_SIZE);
struct k_work_q zep_wifi_tx_done_q;
#endif /* CONFIG_NRF70_TX_DONE_WQ_ENABLED */

#ifdef CONFIG_NRF70_RX_WQ_ENABLED
K_THREAD_STACK_DEFINE(rx_wq_stack_area, CONFIG_NRF70_RX_WQ_STACK_SIZE);
struct k_work_q zep_wifi_rx_q;
#endif /* CONFIG_NRF70_RX_WQ_ENABLED */

struct zep_work_item zep_work_item[CONFIG_NRF70_WORKQ_MAX_ITEMS];

int get_free_work_item_index(void)
{
	int i;

	for (i = 0; i < CONFIG_NRF70_WORKQ_MAX_ITEMS; i++) {
		if (zep_work_item[i].in_use) {
			continue;
		}
		return i;
	}

	return -1;
}

void workqueue_callback(struct k_work *work)
{
	struct zep_work_item *item = CONTAINER_OF(work, struct zep_work_item, work);

	item->callback(item->data);
}

struct zep_work_item *work_alloc(enum zep_work_type type)
{
	int free_work_index = get_free_work_item_index();

	if (free_work_index < 0) {
		LOG_ERR("%s: Reached maximum work items", __func__);
		return NULL;
	}

	zep_work_item[free_work_index].in_use = true;
	zep_work_item[free_work_index].type = type;

	return &zep_work_item[free_work_index];
}

static int workqueue_init(void)
{
	k_work_queue_init(&zep_wifi_bh_q);

	k_work_queue_start(&zep_wifi_bh_q,
			   bh_wq_stack_area,
			   K_THREAD_STACK_SIZEOF(bh_wq_stack_area),
			   CONFIG_NRF70_BH_WQ_PRIORITY,
			   NULL);

	k_thread_name_set(&zep_wifi_bh_q.thread, "nrf70_bh_wq");

	k_work_queue_init(&zep_wifi_intr_q);

	k_work_queue_start(&zep_wifi_intr_q,
			   irq_wq_stack_area,
			   K_THREAD_STACK_SIZEOF(irq_wq_stack_area),
			   CONFIG_NRF70_IRQ_WQ_PRIORITY,
			   NULL);

	k_thread_name_set(&zep_wifi_intr_q.thread, "nrf70_intr_wq");
#ifdef CONFIG_NRF70_TX_DONE_WQ_ENABLED
	k_work_queue_init(&zep_wifi_tx_done_q);

	k_work_queue_start(&zep_wifi_tx_done_q,
			   tx_done_wq_stack_area,
			   K_THREAD_STACK_SIZEOF(tx_done_wq_stack_area),
			   CONFIG_NRF70_TX_DONE_WQ_PRIORITY,
			   NULL);

	k_thread_name_set(&zep_wifi_tx_done_q.thread, "nrf70_tx_done_wq");
#endif /* CONFIG_NRF70_TX_DONE_WQ_ENABLED */

#ifdef CONFIG_NRF70_RX_WQ_ENABLED
	k_work_queue_init(&zep_wifi_rx_q);

	k_work_queue_start(&zep_wifi_rx_q,
			   rx_wq_stack_area,
			   K_THREAD_STACK_SIZEOF(rx_wq_stack_area),
			   CONFIG_NRF70_RX_WQ_PRIORITY,
			   NULL);

	k_thread_name_set(&zep_wifi_rx_q.thread, "nrf70_rx_wq");
#endif /* CONFIG_NRF70_RX_WQ_ENABLED */

	return 0;
}

void work_init(struct zep_work_item *item, void (*callback)(unsigned long),
		  unsigned long data)
{
	item->callback = callback;
	item->data = data;

	k_work_init(&item->work, workqueue_callback);
}

void work_schedule(struct zep_work_item *item)
{
	if (item->type == ZEP_WORK_TYPE_IRQ) {
		k_work_submit_to_queue(&zep_wifi_intr_q, &item->work);
	} else if (item->type == ZEP_WORK_TYPE_BH) {
		k_work_submit_to_queue(&zep_wifi_bh_q, &item->work);
	}
#ifdef CONFIG_NRF70_TX_DONE_WQ_ENABLED
	else if (item->type == ZEP_WORK_TYPE_TX_DONE) {
		k_work_submit_to_queue(&zep_wifi_tx_done_q, &item->work);
	}
#endif /* CONFIG_NRF70_TX_DONE_WQ_ENABLED */
#ifdef CONFIG_NRF70_RX_WQ_ENABLED
	else if (item->type == ZEP_WORK_TYPE_RX) {
		k_work_submit_to_queue(&zep_wifi_rx_q, &item->work);
	}
#endif /* CONFIG_NRF70_RX_WQ_ENABLED */
}

void work_kill(struct zep_work_item *item)
{
	/* TODO: Based on context, use _sync version */
	k_work_cancel(&item->work);
}

void work_free(struct zep_work_item *item)
{
	item->in_use = 0;
}

SYS_INIT(workqueue_init, POST_KERNEL, 0);
