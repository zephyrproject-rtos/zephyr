/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2025 Netfeasa Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/toolchain.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <zephyr/drivers/modem/hl78xx_apis.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(hl78xx_evt_monitor, CONFIG_HL78XX_EVT_MONITOR_LOG_LEVEL);

struct evt_notif_fifo {
	void *fifo_reserved;
	struct hl78xx_evt data;
};

static struct hl78xx_evt_monitor_entry *monitor_list_head;
static struct k_spinlock monitor_list_lock;
static size_t monitor_list_count;

static void hl78xx_evt_monitor_task(struct k_work *work);

static K_FIFO_DEFINE(hl78xx_evt_monitor_fifo);
static K_HEAP_DEFINE(hl78xx_evt_monitor_heap, CONFIG_HL78XX_EVT_MONITOR_HEAP_SIZE);
static K_WORK_DEFINE(hl78xx_evt_monitor_work, hl78xx_evt_monitor_task);

BUILD_ASSERT(CONFIG_HL78XX_EVT_MONITOR_MAX_INSTANCE_MONITORS > 0,
	     "CONFIG_HL78XX_EVT_MONITOR_MAX_INSTANCE_MONITORS must be greater than 0");

BUILD_ASSERT(CONFIG_HL78XX_EVT_MONITOR_HEAP_SIZE >= sizeof(struct evt_notif_fifo),
	     "CONFIG_HL78XX_EVT_MONITOR_HEAP_SIZE is too small");

static bool is_paused(const struct hl78xx_evt_monitor_entry *mon)
{
	return mon->flags.paused;
}

static bool is_direct(const struct hl78xx_evt_monitor_entry *mon)
{
	return mon->flags.direct;
}

static bool should_dispatch_to_monitor(const struct hl78xx_evt_monitor_entry *mon, bool direct)
{
	return !is_paused(mon) && (is_direct(mon) == direct);
}

static bool has_static_monitor(bool direct)
{
	STRUCT_SECTION_FOREACH(hl78xx_evt_monitor_entry, mon) {
		if (should_dispatch_to_monitor(mon, direct)) {
			return true;
		}
	}

	return false;
}

static bool has_instance_monitor(bool direct)
{
	k_spinlock_key_t key = k_spin_lock(&monitor_list_lock);
	bool found = false;

	for (struct hl78xx_evt_monitor_entry *mon = monitor_list_head; mon != NULL;
	     mon = mon->next) {
		if (should_dispatch_to_monitor(mon, direct)) {
			found = true;
			break;
		}
	}

	k_spin_unlock(&monitor_list_lock, key);

	return found;
}

/**
 * Runtime monitor entries are protected by references. Runtime list membership
 * owns one reference and each dispatch snapshot owns one temporary reference.
 * Unregister removes the list reference but does not wait for in-flight
 * snapshots; final reclamation is deferred through the optional release
 * callback.
 */
static void hl78xx_evt_monitor_get(struct hl78xx_evt_monitor_entry *mon)
{
	atomic_inc(&mon->refcnt);
}

static void hl78xx_evt_monitor_release_work_handler(struct k_work *work)
{
	struct hl78xx_evt_monitor_entry *mon =
		CONTAINER_OF(work, struct hl78xx_evt_monitor_entry, release_work);

	if (mon->release != NULL) {
		mon->release(mon);
	}
}

static void hl78xx_evt_monitor_put(struct hl78xx_evt_monitor_entry *mon)
{
	atomic_val_t old_refcnt = atomic_dec(&mon->refcnt);

	if ((old_refcnt == 1) && (mon->release != NULL)) {
		k_work_submit(&mon->release_work);
	}
}

static void release_monitor_snapshot(struct hl78xx_evt_monitor_entry **snapshot, size_t count)
{
	for (size_t i = 0U; i < count; i++) {
		hl78xx_evt_monitor_put(snapshot[i]);
	}
}

static bool monitor_is_registered_locked(const struct hl78xx_evt_monitor_entry *mon)
{
	for (struct hl78xx_evt_monitor_entry *entry = monitor_list_head; entry != NULL;
	     entry = entry->next) {
		if (entry == mon) {
			return true;
		}
	}

	return false;
}

static int snapshot_instance_monitors(bool direct, struct hl78xx_evt_monitor_entry **snapshot,
				      size_t capacity)
{
	size_t count = 0U;
	k_spinlock_key_t key;

	__ASSERT_NO_MSG(snapshot != NULL);

	key = k_spin_lock(&monitor_list_lock);

	for (struct hl78xx_evt_monitor_entry *mon = monitor_list_head; mon != NULL;
	     mon = mon->next) {
		if (!should_dispatch_to_monitor(mon, direct)) {
			continue;
		}

		if (count >= capacity) {
			k_spin_unlock(&monitor_list_lock, key);
			release_monitor_snapshot(snapshot, count);
			return -ENOSPC;
		}

		hl78xx_evt_monitor_get(mon);
		snapshot[count++] = mon;
	}

	k_spin_unlock(&monitor_list_lock, key);

	return (int)count;
}

static int dispatch_instance_monitors(struct hl78xx_evt *notif, bool direct)
{
	struct hl78xx_evt_monitor_entry *snapshot[CONFIG_HL78XX_EVT_MONITOR_MAX_INSTANCE_MONITORS];
	int snapshot_count;

	snapshot_count = snapshot_instance_monitors(direct, snapshot, ARRAY_SIZE(snapshot));
	if (snapshot_count <= 0) {
		return snapshot_count;
	}

	for (int i = 0; i < snapshot_count; i++) {
		snapshot[i]->handler(notif, snapshot[i]);
		hl78xx_evt_monitor_put(snapshot[i]);
	}

	return snapshot_count;
}

/* Register an event monitor */
int hl78xx_evt_monitor_register(struct hl78xx_evt_monitor_entry *mon)
{
	k_spinlock_key_t key;
	int ret = 0;

	if (mon == NULL) {
		return -EINVAL;
	}

	key = k_spin_lock(&monitor_list_lock);

	if (monitor_is_registered_locked(mon)) {
		ret = -EALREADY;
		goto out;
	}

	if (atomic_get(&mon->refcnt) != 0) {
		ret = -EBUSY;
		goto out;
	}

	if (monitor_list_count >= CONFIG_HL78XX_EVT_MONITOR_MAX_INSTANCE_MONITORS) {
		ret = -ENOMEM;
		goto out;
	}

	atomic_set(&mon->refcnt, 1);
	k_work_init(&mon->release_work, hl78xx_evt_monitor_release_work_handler);

	mon->next = monitor_list_head;
	monitor_list_head = mon;
	monitor_list_count++;

out:
	k_spin_unlock(&monitor_list_lock, key);

	return ret;
}

/* Unregister an event monitor */
int hl78xx_evt_monitor_unregister(struct hl78xx_evt_monitor_entry *mon)
{
	k_spinlock_key_t key;
	struct hl78xx_evt_monitor_entry **pp;

	if (mon == NULL) {
		return -EINVAL;
	}

	key = k_spin_lock(&monitor_list_lock);
	pp = &monitor_list_head;
	while (*pp != NULL) {
		if (*pp == mon) {
			*pp = mon->next;
			mon->next = NULL;
			monitor_list_count--;
			k_spin_unlock(&monitor_list_lock, key);

			hl78xx_evt_monitor_put(mon);

			return 0;
		}
		pp = &(*pp)->next;
	}
	k_spin_unlock(&monitor_list_lock, key);

	return -ENOENT;
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
	int ret;

	__ASSERT_NO_MSG(notif != NULL);

	monitored = false;
	/* Global monitors: SECTION_ITERABLE */
	STRUCT_SECTION_FOREACH(hl78xx_evt_monitor_entry, e) {
		if (should_dispatch_to_monitor(e, true)) {
			LOG_DBG("calling direct static handler %p", e->handler);
			e->handler(notif, e);
		} else if (should_dispatch_to_monitor(e, false)) {
			monitored = true;
		} else {
			LOG_DBG("skipping paused monitor %p", e->handler);
		}
	}

	ret = dispatch_instance_monitors(notif, true);
	if (ret < 0) {
		LOG_WRN("Failed to snapshot direct event monitors (evt=%d): %d", notif->type, ret);
	}

	monitored = monitored || has_static_monitor(false) || has_instance_monitor(false);

	if (!monitored) {
		/* Only copy monitored notifications to save heap */
		return;
	}

	sz_needed = sizeof(struct evt_notif_fifo);

	evt_notif = k_heap_alloc(&hl78xx_evt_monitor_heap, sz_needed, K_NO_WAIT);
	if (!evt_notif) {
		LOG_WRN("No heap space for incoming notification: %d", notif->type);
		return;
	}

	evt_notif->data = *notif;

	k_fifo_put(&hl78xx_evt_monitor_fifo, evt_notif);
	k_work_submit(&hl78xx_evt_monitor_work);
}

static void hl78xx_evt_monitor_task(struct k_work *work)
{
	struct evt_notif_fifo *evt_notif;
	int ret;

	ARG_UNUSED(work);

	while ((evt_notif = k_fifo_get(&hl78xx_evt_monitor_fifo, K_NO_WAIT))) {
		/* Dispatch notification with all monitors */
		LOG_DBG("EVT notif: %d", evt_notif->data.type);
		STRUCT_SECTION_FOREACH(hl78xx_evt_monitor_entry, e) {
			if (should_dispatch_to_monitor(e, false)) {
				LOG_DBG("Dispatching to %p", e->handler);
				e->handler(&evt_notif->data, e);
			}
		}

		ret = dispatch_instance_monitors(&evt_notif->data, false);
		if (ret < 0) {
			LOG_WRN("Failed to snapshot deferred event monitors (evt=%d): %d",
				evt_notif->data.type, ret);
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
SYS_INIT(hl78xx_evt_monitor_sys_init, APPLICATION, CONFIG_HL78XX_EVT_MONITOR_APP_INIT_PRIORITY);
