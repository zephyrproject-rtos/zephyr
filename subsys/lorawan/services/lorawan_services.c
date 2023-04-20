/*
 * Copyright (c) 2022 Martin Jäger <martin@libre.solar>
 * Copyright (c) 2022 tado GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lorawan_services.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(lorawan_services, CONFIG_LORAWAN_SERVICES_LOG_LEVEL);

struct service_uplink_msg {
	sys_snode_t node;
	/* absolute ticks when this message should be scheduled */
	int64_t ticks;
	/* sufficient space for up to 3 answers (max 6 bytes each) */
	uint8_t data[18];
	uint8_t len;
	uint8_t port;
	bool used;
};

K_THREAD_STACK_DEFINE(thread_stack_area, CONFIG_LORAWAN_SERVICES_THREAD_STACK_SIZE);

/*
 * The services need a dedicated work queue, as the LoRaWAN stack uses the system
 * work queue and gets blocked if other LoRaWAN messages are sent and processed from
 * the system work queue in parallel.
 */
static struct k_work_q services_workq;

static struct k_work_delayable uplink_work;

/* single-linked list (with pointers) and array for implementation of priority queue */
static struct service_uplink_msg messages[10];
static sys_slist_t msg_list;
static struct k_sem msg_sem;

static void uplink_handler(struct k_work *work)
{
	struct service_uplink_msg msg_copy;
	struct service_uplink_msg *first;
	sys_snode_t *node;
	int err;

	ARG_UNUSED(work);

	/* take semaphore and create a copy of the next message */
	k_sem_take(&msg_sem, K_FOREVER);

	node = sys_slist_get(&msg_list);
	if (node == NULL) {
		goto out;
	}

	first = CONTAINER_OF(node, struct service_uplink_msg, node);
	msg_copy = *first;
	first->used = false;
	sys_slist_remove(&msg_list, NULL, &first->node);

	/* semaphore must be given back before calling lorawan_send */
	k_sem_give(&msg_sem);

	err = lorawan_send(msg_copy.port, msg_copy.data, msg_copy.len, LORAWAN_MSG_UNCONFIRMED);
	if (!err) {
		LOG_DBG("Message sent to port %d", msg_copy.port);
	} else {
		LOG_ERR("Sending message to port %d failed: %d",
			msg_copy.port, err);
	}

	/* take the semaphore again to schedule next uplink */
	k_sem_take(&msg_sem, K_FOREVER);

	node = sys_slist_peek_head(&msg_list);
	if (node == NULL) {
		goto out;
	}
	first = CONTAINER_OF(node, struct service_uplink_msg, node);
	k_work_reschedule_for_queue(&services_workq, &uplink_work,
		K_TIMEOUT_ABS_TICKS(first->ticks));

out:
	k_sem_give(&msg_sem);
}

static inline void insert_uplink(struct service_uplink_msg *msg_new)
{
	struct service_uplink_msg *msg_prev;

	if (sys_slist_is_empty(&msg_list)) {
		sys_slist_append(&msg_list, &msg_new->node);
	} else {
		int count = 0;

		SYS_SLIST_FOR_EACH_CONTAINER(&msg_list, msg_prev, node) {
			count++;
			if (msg_prev->ticks <= msg_new->ticks) {
				break;
			}
		}
		if (msg_prev != NULL) {
			sys_slist_insert(&msg_list, &msg_prev->node, &msg_new->node);
		} else {
			sys_slist_append(&msg_list, &msg_new->node);
		}
	}
}

int lorawan_services_schedule_uplink(uint8_t port, uint8_t *data, uint8_t len, uint32_t timeout)
{
	struct service_uplink_msg *next;
	int64_t timeout_abs_ticks;

	if (len > sizeof(messages[0].data)) {
		LOG_ERR("Uplink payload for port %u too long: %u bytes", port, len);
		LOG_HEXDUMP_ERR(data, len, "Payload: ");
		return -EFBIG;
	}

	timeout_abs_ticks = k_uptime_ticks() + k_ms_to_ticks_ceil64(timeout);

	k_sem_take(&msg_sem, K_FOREVER);

	for (int i = 0; i < ARRAY_SIZE(messages); i++) {
		if (!messages[i].used) {
			memcpy(messages[i].data, data, len);
			messages[i].port = port;
			messages[i].len = len;
			messages[i].ticks = timeout_abs_ticks;
			messages[i].used = true;

			insert_uplink(&messages[i]);

			next = SYS_SLIST_PEEK_HEAD_CONTAINER(&msg_list, next, node);
			if (next != NULL) {
				k_work_reschedule_for_queue(&services_workq, &uplink_work,
					K_TIMEOUT_ABS_TICKS(next->ticks));
			}

			k_sem_give(&msg_sem);

			return 0;
		}
	}

	k_sem_give(&msg_sem);

	LOG_WRN("Message queue full, message for port %u dropped.", port);

	return -ENOSPC;
}

int lorawan_services_reschedule_work(struct k_work_delayable *dwork, k_timeout_t delay)
{
	return k_work_reschedule_for_queue(&services_workq, dwork, delay);
}

static int lorawan_services_init(void)
{

	sys_slist_init(&msg_list);
	k_sem_init(&msg_sem, 1, 1);

	k_work_queue_init(&services_workq);
	k_work_queue_start(&services_workq,
			   thread_stack_area, K_THREAD_STACK_SIZEOF(thread_stack_area),
			   CONFIG_LORAWAN_SERVICES_THREAD_PRIORITY, NULL);

	k_work_init_delayable(&uplink_work, uplink_handler);

	k_thread_name_set(&services_workq.thread, "lorawan_services");

	return 0;
}

SYS_INIT(lorawan_services_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
