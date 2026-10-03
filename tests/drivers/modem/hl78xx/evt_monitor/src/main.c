/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Netfeasa Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The deferred notification path of the HL78xx event monitor: notifications
 * for system-workqueue monitors are copied into a fixed-depth queue. A burst
 * that fits the depth is delivered in order once the workqueue runs; a burst
 * that exceeds it drops the excess, counts every drop, and recovers as soon as
 * the queue drains. Direct monitors and paused monitors never use a slot.
 *
 * The queue depth of 16 is the shipping default. The three-event burst is the
 * one the driver emits on every entry to the registered state (RAT update,
 * registration update, AT_CMD_READY); a 256-byte heap used to lose the third
 * of those whenever the workqueue was late (customer log, 30-09-2026).
 */

#include <string.h>

#include <zephyr/drivers/modem/hl78xx_apis.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define QUEUE_DEPTH CONFIG_HL78XX_EVT_MONITOR_QUEUE_DEPTH
#define RECORD_MAX  (QUEUE_DEPTH + 8)

/* The driver entry point the monitor library hooks itself into at init. The
 * test drives hl78xx_evt_monitor_dispatch() directly instead of through it.
 */
static hl78xx_evt_monitor_dispatcher_t installed_dispatcher;

int hl78xx_evt_notif_handler_set(hl78xx_evt_monitor_dispatcher_t handler)
{
	installed_dispatcher = handler;
	return 0;
}

static enum hl78xx_evt_type deferred_seen[RECORD_MAX];
static atomic_t deferred_count;
static atomic_t direct_count;

/** @brief Deferred monitor callback: records the event type in arrival order. */
static void deferred_handler(struct hl78xx_evt *notif, struct hl78xx_evt_monitor_entry *mon)
{
	atomic_val_t idx = atomic_inc(&deferred_count);

	ARG_UNUSED(mon);

	if (idx < RECORD_MAX) {
		deferred_seen[idx] = notif->type;
	}
}

/** @brief Direct monitor callback: runs in the dispatcher's context. */
static void direct_handler(struct hl78xx_evt *notif, struct hl78xx_evt_monitor_entry *mon)
{
	ARG_UNUSED(notif);
	ARG_UNUSED(mon);

	atomic_inc(&direct_count);
}

static struct hl78xx_evt_monitor_entry deferred_mon = {
	.handler = deferred_handler,
	.flags = {.paused = 0, .direct = 0},
};

static struct hl78xx_evt_monitor_entry direct_mon = {
	.handler = direct_handler,
	.flags = {.paused = 0, .direct = 1},
};

/* Holding the system workqueue keeps the deferred dispatch task from running,
 * so every dispatched notification has to wait in the queue.
 */
static K_SEM_DEFINE(workq_hold_sem, 0, 1);

static void workq_hold_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	k_sem_take(&workq_hold_sem, K_FOREVER);
}

static K_WORK_DEFINE(workq_hold_work, workq_hold_fn);

/** @brief Block the system workqueue until workq_release(). */
static void workq_hold(void)
{
	k_sem_reset(&workq_hold_sem);
	k_work_submit(&workq_hold_work);
	/* Let the hold work start before the caller queues anything. */
	k_sleep(K_MSEC(10));
}

/** @brief Release the system workqueue and let it drain the monitor queue. */
static void workq_release(void)
{
	k_sem_give(&workq_hold_sem);
	k_sleep(K_MSEC(50));
}

/** @brief Dispatch one notification of the given type. */
static void dispatch(enum hl78xx_evt_type type)
{
	struct hl78xx_evt notif = {.type = type};

	hl78xx_evt_monitor_dispatch(&notif);
}

/** @brief Dispatch @p count notifications carrying a recognisable type. */
static void dispatch_many(uint32_t count)
{
	for (uint32_t i = 0U; i < count; i++) {
		dispatch(HL78XX_LTE_REGISTRATION_STAT_UPDATE);
	}
}

static void *suite_setup(void)
{
	zassert_not_null(installed_dispatcher, "monitor did not hook the driver at init");
	zassert_equal(installed_dispatcher, hl78xx_evt_monitor_dispatch,
		      "unexpected dispatcher installed");
	zassert_ok(hl78xx_evt_monitor_register(&deferred_mon));

	return NULL;
}

static void test_before(void *fixture)
{
	ARG_UNUSED(fixture);

	atomic_set(&deferred_count, 0);
	atomic_set(&direct_count, 0);
	memset(deferred_seen, 0, sizeof(deferred_seen));
	hl78xx_evt_monitor_resume(&deferred_mon);
}

/** @brief The registered-state burst is delivered complete and in order. */
ZTEST(hl78xx_evt_monitor, test_registered_burst_is_delivered_in_order)
{
	uint32_t dropped_before = hl78xx_evt_monitor_dropped_count();

	workq_hold();
	dispatch(HL78XX_LTE_RAT_UPDATE);
	dispatch(HL78XX_LTE_REGISTRATION_STAT_UPDATE);
	dispatch(HL78XX_LTE_AT_CMD_READY);
	zassert_equal(atomic_get(&deferred_count), 0, "delivered while the workqueue was held");
	workq_release();

	zassert_equal(atomic_get(&deferred_count), 3);
	zassert_equal(deferred_seen[0], HL78XX_LTE_RAT_UPDATE);
	zassert_equal(deferred_seen[1], HL78XX_LTE_REGISTRATION_STAT_UPDATE);
	zassert_equal(deferred_seen[2], HL78XX_LTE_AT_CMD_READY);
	zassert_equal(hl78xx_evt_monitor_dropped_count(), dropped_before, "burst was dropped");
}

/** @brief A burst that exactly fills the queue loses nothing. */
ZTEST(hl78xx_evt_monitor, test_full_depth_is_delivered)
{
	uint32_t dropped_before = hl78xx_evt_monitor_dropped_count();

	workq_hold();
	dispatch_many(QUEUE_DEPTH);
	workq_release();

	zassert_equal(atomic_get(&deferred_count), QUEUE_DEPTH);
	zassert_equal(hl78xx_evt_monitor_dropped_count(), dropped_before);
}

/** @brief Overflow drops only the excess, counts every drop, and the queue recovers. */
ZTEST(hl78xx_evt_monitor, test_overflow_is_counted_and_queue_recovers)
{
	uint32_t dropped_before = hl78xx_evt_monitor_dropped_count();

	workq_hold();
	dispatch_many(QUEUE_DEPTH + 2U);
	/* The drop is decided synchronously, before the workqueue runs. */
	zassert_equal(hl78xx_evt_monitor_dropped_count(), dropped_before + 2U);
	workq_release();

	zassert_equal(atomic_get(&deferred_count), QUEUE_DEPTH, "queued events lost");

	/* Once drained, the next notification goes through again. */
	dispatch(HL78XX_LTE_AT_CMD_READY);
	k_sleep(K_MSEC(50));
	zassert_equal(atomic_get(&deferred_count), QUEUE_DEPTH + 1);
	zassert_equal(deferred_seen[QUEUE_DEPTH], HL78XX_LTE_AT_CMD_READY);
	zassert_equal(hl78xx_evt_monitor_dropped_count(), dropped_before + 2U);
}

/** @brief Direct monitors are called synchronously and never take a queue slot. */
ZTEST(hl78xx_evt_monitor, test_direct_monitor_bypasses_the_queue)
{
	uint32_t dropped_before = hl78xx_evt_monitor_dropped_count();

	zassert_ok(hl78xx_evt_monitor_register(&direct_mon));

	workq_hold();
	dispatch_many(QUEUE_DEPTH + 1U);
	zassert_equal(atomic_get(&direct_count), QUEUE_DEPTH + 1,
		      "direct monitor did not run synchronously");
	zassert_equal(hl78xx_evt_monitor_dropped_count(), dropped_before + 1U,
		      "deferred overflow not counted alongside direct delivery");
	workq_release();

	zassert_equal(atomic_get(&deferred_count), QUEUE_DEPTH);
	zassert_ok(hl78xx_evt_monitor_unregister(&direct_mon));
}

/** @brief With every deferred monitor paused, nothing is queued and nothing is dropped. */
ZTEST(hl78xx_evt_monitor, test_paused_monitors_use_no_queue_slot)
{
	uint32_t dropped_before = hl78xx_evt_monitor_dropped_count();

	hl78xx_evt_monitor_pause(&deferred_mon);

	workq_hold();
	dispatch_many(QUEUE_DEPTH + 3U);
	zassert_equal(hl78xx_evt_monitor_dropped_count(), dropped_before,
		      "unmonitored notifications consumed queue slots");
	workq_release();

	zassert_equal(atomic_get(&deferred_count), 0);
	hl78xx_evt_monitor_resume(&deferred_mon);
}

ZTEST_SUITE(hl78xx_evt_monitor, NULL, suite_setup, test_before, NULL, NULL);
