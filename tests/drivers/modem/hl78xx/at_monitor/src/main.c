/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Netfeasa Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The deferred path of the HL78xx parsed AT monitor: a notification for the
 * system-workqueue monitors is copied, arguments included, into a byte heap.
 * A burst that fits is delivered in order with its arguments intact after the
 * chat buffers have been reused; a burst that exhausts the heap drops the
 * excess, counts every drop, and recovers once the queue drains. Filters,
 * direct monitors and paused monitors behave as documented.
 */

#include <string.h>

#include <zephyr/drivers/modem/hl78xx_apis.h>
#include <zephyr/kernel.h>
#include <zephyr/modem/chat.h>
#include <zephyr/ztest.h>

#include "hl78xx_at_monitor.h"

#define RECORD_MAX 64
#define TEXT_MAX   24
#define FLOOD      64U

static const struct modem_chat_match cereg_match = {
	.match = (const uint8_t *)"+CEREG: ",
};
static const struct modem_chat_match kstatev_match = {
	.match = (const uint8_t *)"+KSTATEV: ",
};

struct seen_notification {
	char pattern[TEXT_MAX];
	char arg1[TEXT_MAX];
	uint16_t argc;
};

static struct seen_notification deferred_seen[RECORD_MAX];
static atomic_t deferred_count;
static atomic_t direct_count;
static atomic_t cereg_only_count;

/** @brief Deferred monitor for every pattern: records what arrived, in order. */
static void deferred_handler(const struct hl78xx_at_notification *notif,
			     struct hl78xx_at_monitor_entry *mon)
{
	atomic_val_t idx = atomic_inc(&deferred_count);

	ARG_UNUSED(mon);

	if (idx < RECORD_MAX) {
		strncpy(deferred_seen[idx].pattern, notif->pattern, TEXT_MAX - 1);
		if (notif->argc > 1) {
			strncpy(deferred_seen[idx].arg1, notif->argv[1], TEXT_MAX - 1);
		}
		deferred_seen[idx].argc = notif->argc;
	}
}

/** @brief Direct monitor: runs in the dispatcher's context. */
static void direct_handler(const struct hl78xx_at_notification *notif,
			   struct hl78xx_at_monitor_entry *mon)
{
	ARG_UNUSED(notif);
	ARG_UNUSED(mon);

	atomic_inc(&direct_count);
}

/** @brief Deferred monitor filtered to one pattern. */
static void cereg_only_handler(const struct hl78xx_at_notification *notif,
			       struct hl78xx_at_monitor_entry *mon)
{
	ARG_UNUSED(mon);

	zassert_str_equal(notif->pattern, "+CEREG: ", "filter let another pattern through");
	atomic_inc(&cereg_only_count);
}

static struct hl78xx_at_monitor_entry deferred_any = {
	.filter = HL78XX_AT_MONITOR_ANY,
	.handler = deferred_handler,
	.flags = {.paused = 0, .direct = 0},
};

static struct hl78xx_at_monitor_entry direct_any = {
	.filter = HL78XX_AT_MONITOR_ANY,
	.handler = direct_handler,
	.flags = {.paused = 0, .direct = 1},
};

static struct hl78xx_at_monitor_entry deferred_cereg = {
	.filter = "+CEREG: ",
	.handler = cereg_only_handler,
	.flags = {.paused = 0, .direct = 0},
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
	k_sleep(K_MSEC(10));
}

/** @brief Release the system workqueue and let it drain the monitor queue. */
static void workq_release(void)
{
	k_sem_give(&workq_hold_sem);
	k_sleep(K_MSEC(50));
}

/* The chat layer hands the monitor its parse match and the argv it parsed
 * into its own receive buffer, which it reuses for the next line. The
 * dispatch helper mimics that: it scribbles over the argument buffers as
 * soon as the call returns, so a deferred delivery can only be correct if
 * the monitor made its own copy.
 */
static struct modem_chat chat;

/** @brief Dispatch one notification with up to two arguments after the token. */
static void dispatch(const struct modem_chat_match *match, const char *token, const char *arg1,
		     const char *arg2)
{
	char buf[3][TEXT_MAX];
	char *argv[3];
	uint16_t argc = 0U;

	strncpy(buf[0], token, TEXT_MAX - 1);
	buf[0][TEXT_MAX - 1] = '\0';
	argv[argc++] = buf[0];
	if (arg1 != NULL) {
		strncpy(buf[1], arg1, TEXT_MAX - 1);
		buf[1][TEXT_MAX - 1] = '\0';
		argv[argc++] = buf[1];
	}
	if (arg2 != NULL) {
		strncpy(buf[2], arg2, TEXT_MAX - 1);
		buf[2][TEXT_MAX - 1] = '\0';
		argv[argc++] = buf[2];
	}

	chat.parse_match = match;
	hl78xx_at_monitor_dispatch(&chat, argv, argc);

	memset(buf, 'x', sizeof(buf));
}

/** @brief Dispatch @p count registration notifications. */
static void dispatch_many(uint32_t count)
{
	for (uint32_t i = 0U; i < count; i++) {
		dispatch(&cereg_match, "+CEREG: ", "2", NULL);
	}
}

static void *suite_setup(void)
{
	zassert_ok(hl78xx_at_monitor_register(&deferred_any));

	return NULL;
}

static void test_before(void *fixture)
{
	ARG_UNUSED(fixture);

	atomic_set(&deferred_count, 0);
	atomic_set(&direct_count, 0);
	atomic_set(&cereg_only_count, 0);
	memset(deferred_seen, 0, sizeof(deferred_seen));
	hl78xx_at_monitor_resume(&deferred_any);
}

/** @brief A registration burst arrives in order, arguments copied, nothing dropped. */
ZTEST(hl78xx_at_monitor, test_burst_is_delivered_in_order_with_copied_arguments)
{
	uint32_t dropped_before = hl78xx_at_monitor_dropped_count();

	workq_hold();
	dispatch(&cereg_match, "+CEREG: ", "5", "\"4EA5\"");
	dispatch(&kstatev_match, "+KSTATEV: ", "2", "1");
	dispatch(&cereg_match, "+CEREG: ", "4", NULL);
	zassert_equal(atomic_get(&deferred_count), 0, "delivered while the workqueue was held");
	workq_release();

	zassert_equal(atomic_get(&deferred_count), 3);
	zassert_str_equal(deferred_seen[0].pattern, "+CEREG: ");
	zassert_str_equal(deferred_seen[0].arg1, "5");
	zassert_equal(deferred_seen[0].argc, 3);
	zassert_str_equal(deferred_seen[1].pattern, "+KSTATEV: ");
	zassert_str_equal(deferred_seen[1].arg1, "2");
	zassert_str_equal(deferred_seen[2].pattern, "+CEREG: ");
	zassert_str_equal(deferred_seen[2].arg1, "4");
	zassert_equal(deferred_seen[2].argc, 2);
	zassert_equal(hl78xx_at_monitor_dropped_count(), dropped_before, "burst was dropped");
}

/** @brief Exhausting the heap drops only the excess, counts it, and the queue recovers. */
ZTEST(hl78xx_at_monitor, test_heap_exhaustion_is_counted_and_queue_recovers)
{
	uint32_t dropped_before = hl78xx_at_monitor_dropped_count();
	uint32_t dropped;

	workq_hold();
	dispatch_many(FLOOD);
	/* The drop is decided synchronously, before the workqueue runs. */
	dropped = hl78xx_at_monitor_dropped_count() - dropped_before;
	zassert_true(dropped > 0U, "a %u-notification flood never exhausted the heap", FLOOD);
	workq_release();

	zassert_equal((uint32_t)atomic_get(&deferred_count) + dropped, FLOOD,
		      "delivered plus dropped does not add up to the flood");
	zassert_true(atomic_get(&deferred_count) >= 3, "heap holds less than a registration burst");

	/* Once drained, the next notification goes through again. */
	dispatch(&kstatev_match, "+KSTATEV: ", "5", "1");
	k_sleep(K_MSEC(50));
	zassert_equal((uint32_t)atomic_get(&deferred_count) + dropped, FLOOD + 1U);
	zassert_equal(hl78xx_at_monitor_dropped_count(), dropped_before + dropped);
}

/** @brief A filtered monitor receives only its pattern. */
ZTEST(hl78xx_at_monitor, test_filter_selects_matching_pattern_only)
{
	zassert_ok(hl78xx_at_monitor_register(&deferred_cereg));

	dispatch(&kstatev_match, "+KSTATEV: ", "0", "1");
	dispatch(&cereg_match, "+CEREG: ", "3", NULL);
	dispatch(&kstatev_match, "+KSTATEV: ", "1", "1");
	k_sleep(K_MSEC(50));

	zassert_equal(atomic_get(&cereg_only_count), 1);
	zassert_equal(atomic_get(&deferred_count), 3);
	zassert_ok(hl78xx_at_monitor_unregister(&deferred_cereg));
}

/** @brief Direct monitors are called synchronously and never touch the heap. */
ZTEST(hl78xx_at_monitor, test_direct_monitor_bypasses_the_heap)
{
	uint32_t dropped_before = hl78xx_at_monitor_dropped_count();

	zassert_ok(hl78xx_at_monitor_register(&direct_any));

	workq_hold();
	dispatch_many(FLOOD);
	zassert_equal(atomic_get(&direct_count), FLOOD, "direct monitor did not run synchronously");
	zassert_true(hl78xx_at_monitor_dropped_count() > dropped_before,
		     "deferred overflow not counted alongside direct delivery");
	workq_release();

	zassert_ok(hl78xx_at_monitor_unregister(&direct_any));
}

/** @brief With every deferred monitor paused, nothing is copied and nothing is dropped. */
ZTEST(hl78xx_at_monitor, test_paused_monitors_use_no_heap)
{
	uint32_t dropped_before = hl78xx_at_monitor_dropped_count();

	hl78xx_at_monitor_pause(&deferred_any);

	workq_hold();
	dispatch_many(FLOOD);
	zassert_equal(hl78xx_at_monitor_dropped_count(), dropped_before,
		      "unmonitored notifications consumed heap");
	workq_release();

	zassert_equal(atomic_get(&deferred_count), 0);
	hl78xx_at_monitor_resume(&deferred_any);
}

ZTEST_SUITE(hl78xx_at_monitor, NULL, suite_setup, test_before, NULL, NULL);
