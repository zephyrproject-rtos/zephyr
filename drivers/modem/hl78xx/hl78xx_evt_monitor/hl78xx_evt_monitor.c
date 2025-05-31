/*
 * Copyright (c) 2025 Netfeasa
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/toolchain.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/modem/hl78xx_apis.h>

LOG_MODULE_REGISTER(hl78xx_evt_monitor, CONFIG_HL78XX_EVT_MONITOR_LOG_LEVEL);

struct evt_notif_fifo {
	void *fifo_reserved;
	struct hl78xx_evt data;
};

static void hl78xx_evt_monitor_task(struct k_work *work);

static K_FIFO_DEFINE(hl78xx_evt_monitor_fifo);
static K_HEAP_DEFINE(hl78xx_evt_monitor_heap, CONFIG_HL78XX_EVT_MONITOR_HEAP_SIZE);
static K_WORK_DEFINE(hl78xx_evt_monitor_work, hl78xx_evt_monitor_task);

static bool is_paused(const struct hl78xx_evt_monitor_entry *mon)
{
	return mon->flags.paused;
}

static bool is_direct(const struct hl78xx_evt_monitor_entry *mon)
{
	return mon->flags.direct;
}

/* Dispatch EVT notifications immediately, or schedules a workqueue task to do that.
 * Keep this function public so that it can be called by tests.
 * This function is called from an ISR.
 */
void hl78xx_evt_monitor_dispatch(struct hl78xx_evt *notif)
{
	bool monitored;
	struct evt_notif_fifo *evt_notif;
	size_t sz_needed;

	__ASSERT_NO_MSG(notif != NULL);

	monitored = false;
	STRUCT_SECTION_FOREACH(hl78xx_evt_monitor_entry, e) {
		if (!is_paused(e)) {
			if (is_direct(e)) {
				LOG_DBG("Dispatching to %p (ISR)", e->handler);
				e->handler(notif);
			} else {
				/* Copy and schedule work-queue task */
				monitored = true;
			}
		}
	}

	if (!monitored) {
		/* Only copy monitored notifications to save heap */
		return;
	}

	sz_needed = sizeof(struct evt_notif_fifo) + sizeof(notif);

	evt_notif = k_heap_alloc(&hl78xx_evt_monitor_heap, sz_needed, K_NO_WAIT);
	if (!evt_notif) {
		LOG_WRN("No heap space for incoming notification: %d", notif->type);
		__ASSERT(evt_notif, "No heap space for incoming notification: %d", notif->type);
		return;
	}

	evt_notif->data = *notif;

	k_fifo_put(&hl78xx_evt_monitor_fifo, evt_notif);
	k_work_submit(&hl78xx_evt_monitor_work);
}

static void hl78xx_evt_monitor_task(struct k_work *work)
{
	struct evt_notif_fifo *evt_notif;

	while ((evt_notif = k_fifo_get(&hl78xx_evt_monitor_fifo, K_NO_WAIT))) {
		/* Dispatch notification with all monitors */
		LOG_DBG("EVT notif: %d", evt_notif->data.type);
		STRUCT_SECTION_FOREACH(hl78xx_evt_monitor_entry, e) {
			if (!is_paused(e) && !is_direct(e)) {
				LOG_DBG("Dispatching to %p", e->handler);
				e->handler(&evt_notif->data);
			}
		}
		k_heap_free(&hl78xx_evt_monitor_heap, evt_notif);
	}
}

static int hl78xx_evt_monitor_sys_init(void)
{
	int err = 0;

	err = hl78xx_evt_notif_handler_set(hl78xx_evt_monitor_dispatch);
	if (err) {
		LOG_ERR("Failed to hook the dispatch function, err %d", err);
	}

	return 0;
}

/* Initialize during SYS_INIT */
SYS_INIT(hl78xx_evt_monitor_sys_init, APPLICATION, 0);
