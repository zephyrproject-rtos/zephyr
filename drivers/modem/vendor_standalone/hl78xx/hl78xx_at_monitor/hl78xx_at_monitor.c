/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Netfeasa Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <zephyr/toolchain.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/modem/hl78xx_apis.h>

#include "hl78xx_at_monitor.h"

LOG_MODULE_REGISTER(hl78xx_at_monitor, CONFIG_HL78XX_AT_MONITOR_LOG_LEVEL);

struct at_notif_fifo {
	void *fifo_reserved;
	struct hl78xx_at_notification notif;
	char *argv[];
};

static struct hl78xx_at_monitor_entry *monitor_list_head;
static struct k_spinlock monitor_list_lock;
static size_t monitor_list_count;

static void hl78xx_at_monitor_task(struct k_work *work);

static K_FIFO_DEFINE(hl78xx_at_monitor_fifo);
static K_HEAP_DEFINE(hl78xx_at_monitor_heap, CONFIG_HL78XX_AT_MONITOR_HEAP_SIZE);
static K_WORK_DEFINE(hl78xx_at_monitor_work, hl78xx_at_monitor_task);

BUILD_ASSERT(CONFIG_HL78XX_AT_MONITOR_MAX_INSTANCE_MONITORS > 0,
	     "CONFIG_HL78XX_AT_MONITOR_MAX_INSTANCE_MONITORS must be greater than 0");

BUILD_ASSERT(CONFIG_HL78XX_AT_MONITOR_HEAP_SIZE >= sizeof(struct at_notif_fifo),
	     "CONFIG_HL78XX_AT_MONITOR_HEAP_SIZE is too small");

static bool is_paused(const struct hl78xx_at_monitor_entry *mon)
{
	return mon->flags.paused;
}

static bool is_direct(const struct hl78xx_at_monitor_entry *mon)
{
	return mon->flags.direct;
}

static bool filter_matches(const struct hl78xx_at_monitor_entry *mon,
			   const struct hl78xx_at_notification *notif)
{
	return (mon->filter == HL78XX_AT_MONITOR_ANY) || (strcmp(mon->filter, notif->pattern) == 0);
}

static bool should_dispatch_to_monitor(const struct hl78xx_at_monitor_entry *mon,
				       const struct hl78xx_at_notification *notif, bool direct)
{
	return !is_paused(mon) && (is_direct(mon) == direct) && filter_matches(mon, notif);
}

static bool has_static_monitor(const struct hl78xx_at_notification *notif, bool direct)
{
	STRUCT_SECTION_FOREACH(hl78xx_at_monitor_entry, mon) {
		if (should_dispatch_to_monitor(mon, notif, direct)) {
			return true;
		}
	}

	return false;
}

static bool has_instance_monitor(const struct hl78xx_at_notification *notif, bool direct)
{
	k_spinlock_key_t key = k_spin_lock(&monitor_list_lock);
	bool found = false;

	for (struct hl78xx_at_monitor_entry *mon = monitor_list_head; mon != NULL;
	     mon = mon->next) {
		if (should_dispatch_to_monitor(mon, notif, direct)) {
			found = true;
			break;
		}
	}

	k_spin_unlock(&monitor_list_lock, key);

	return found;
}

/*
 * Runtime monitor entries are protected by references. Runtime list membership
 * owns one reference and each dispatch snapshot owns one temporary reference.
 * Unregister removes the list reference but does not wait for in-flight
 * snapshots; final reclamation is deferred through the optional release
 * callback.
 */
static void hl78xx_at_monitor_get(struct hl78xx_at_monitor_entry *mon)
{
	atomic_inc(&mon->refcnt);
}

static void hl78xx_at_monitor_put(struct hl78xx_at_monitor_entry *mon)
{
	atomic_val_t old_refcnt = atomic_dec(&mon->refcnt);

	if ((old_refcnt == 1) && (mon->release != NULL)) {
		k_work_submit(&mon->release_work);
	}
}

static void release_monitor_snapshot(struct hl78xx_at_monitor_entry **snapshot, size_t count)
{
	for (size_t i = 0U; i < count; i++) {
		hl78xx_at_monitor_put(snapshot[i]);
	}
}

static bool monitor_is_registered_locked(const struct hl78xx_at_monitor_entry *mon)
{
	for (struct hl78xx_at_monitor_entry *entry = monitor_list_head; entry != NULL;
	     entry = entry->next) {
		if (entry == mon) {
			return true;
		}
	}

	return false;
}

static void hl78xx_at_monitor_release_work_handler(struct k_work *work)
{
	struct hl78xx_at_monitor_entry *mon =
		CONTAINER_OF(work, struct hl78xx_at_monitor_entry, release_work);

	if (mon->release != NULL) {
		mon->release(mon);
	}
}

static int snapshot_instance_monitors(const struct hl78xx_at_notification *notif, bool direct,
				      struct hl78xx_at_monitor_entry **snapshot, size_t capacity)
{
	size_t count = 0U;
	k_spinlock_key_t key;

	__ASSERT_NO_MSG(snapshot != NULL);

	key = k_spin_lock(&monitor_list_lock);

	for (struct hl78xx_at_monitor_entry *mon = monitor_list_head; mon != NULL;
	     mon = mon->next) {
		if (!should_dispatch_to_monitor(mon, notif, direct)) {
			continue;
		}

		if (count >= capacity) {
			k_spin_unlock(&monitor_list_lock, key);
			release_monitor_snapshot(snapshot, count);
			return -ENOSPC;
		}

		hl78xx_at_monitor_get(mon);
		snapshot[count++] = mon;
	}

	k_spin_unlock(&monitor_list_lock, key);

	return (int)count;
}

static int dispatch_instance_monitors(const struct hl78xx_at_notification *notif, bool direct)
{
	struct hl78xx_at_monitor_entry *snapshot[CONFIG_HL78XX_AT_MONITOR_MAX_INSTANCE_MONITORS];
	int snapshot_count;

	snapshot_count = snapshot_instance_monitors(notif, direct, snapshot, ARRAY_SIZE(snapshot));
	if (snapshot_count <= 0) {
		return snapshot_count;
	}

	for (int i = 0; i < snapshot_count; i++) {
		snapshot[i]->handler(notif, snapshot[i]);
		hl78xx_at_monitor_put(snapshot[i]);
	}

	return snapshot_count;
}

static size_t copied_notification_size(const struct hl78xx_at_notification *notif)
{
	size_t size = sizeof(struct at_notif_fifo) + (notif->argc * sizeof(char *));

	for (uint16_t i = 0; i < notif->argc; i++) {
		size += strlen(notif->argv[i]) + 1U;
	}

	return size;
}

static struct at_notif_fifo *copy_notification(const struct hl78xx_at_notification *notif)
{
	struct at_notif_fifo *copy;
	char *cursor;
	size_t size;

	size = copied_notification_size(notif);
	copy = k_heap_alloc(&hl78xx_at_monitor_heap, size, K_NO_WAIT);
	if (copy == NULL) {
		return NULL;
	}

	copy->notif.pattern = notif->pattern;
	copy->notif.argc = notif->argc;
	copy->notif.argv = (const char *const *)copy->argv;
	cursor = (char *)&copy->argv[notif->argc];

	for (uint16_t i = 0; i < notif->argc; i++) {
		size_t len = strlen(notif->argv[i]) + 1U;

		copy->argv[i] = cursor;
		memcpy(cursor, notif->argv[i], len);
		cursor += len;
	}

	return copy;
}

int hl78xx_at_monitor_register(struct hl78xx_at_monitor_entry *mon)
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

	if (monitor_list_count >= CONFIG_HL78XX_AT_MONITOR_MAX_INSTANCE_MONITORS) {
		ret = -ENOMEM;
		goto out;
	}

	atomic_set(&mon->refcnt, 1);
	k_work_init(&mon->release_work, hl78xx_at_monitor_release_work_handler);

	mon->next = monitor_list_head;
	monitor_list_head = mon;
	monitor_list_count++;

out:
	k_spin_unlock(&monitor_list_lock, key);

	return ret;
}

int hl78xx_at_monitor_unregister(struct hl78xx_at_monitor_entry *mon)
{
	k_spinlock_key_t key;
	struct hl78xx_at_monitor_entry **pp;

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

			hl78xx_at_monitor_put(mon);

			return 0;
		}

		pp = &(*pp)->next;
	}

	k_spin_unlock(&monitor_list_lock, key);

	return -ENOENT;
}

void hl78xx_at_monitor_dispatch(struct modem_chat *chat, char **argv, uint16_t argc)
{
	struct hl78xx_at_notification notif = {
		.pattern = (const char *)chat->parse_match->match,
		.argv = (const char *const *)argv,
		.argc = argc,
	};
	struct at_notif_fifo *copy;
	int ret;

	STRUCT_SECTION_FOREACH(hl78xx_at_monitor_entry, mon) {
		if (!should_dispatch_to_monitor(mon, &notif, true)) {
			continue;
		}

		mon->handler(&notif, mon);
	}

	ret = dispatch_instance_monitors(&notif, true);
	if (ret < 0) {
		LOG_WRN("Failed to snapshot direct AT monitors: %s: %d", notif.pattern, ret);
	}

	if (!has_static_monitor(&notif, false) && !has_instance_monitor(&notif, false)) {
		return;
	}

	copy = copy_notification(&notif);
	if (copy == NULL) {
		LOG_WRN("No heap space for AT notification: %s", notif.pattern);
		return;
	}

	k_fifo_put(&hl78xx_at_monitor_fifo, copy);
	k_work_submit(&hl78xx_at_monitor_work);
}

static void hl78xx_at_monitor_task(struct k_work *work)
{
	struct at_notif_fifo *notif;
	int ret;

	ARG_UNUSED(work);

	while ((notif = k_fifo_get(&hl78xx_at_monitor_fifo, K_NO_WAIT)) != NULL) {
		STRUCT_SECTION_FOREACH(hl78xx_at_monitor_entry, mon) {
			if (!should_dispatch_to_monitor(mon, &notif->notif, false)) {
				continue;
			}

			mon->handler(&notif->notif, mon);
		}

		ret = dispatch_instance_monitors(&notif->notif, false);
		if (ret < 0) {
			LOG_WRN("Failed to snapshot deferred AT monitors: %s: %d",
				notif->notif.pattern, ret);
		}

		k_heap_free(&hl78xx_at_monitor_heap, notif);
	}
}
