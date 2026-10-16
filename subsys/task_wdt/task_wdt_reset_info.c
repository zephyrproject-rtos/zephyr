/*
 * Copyright (c) 2026 Dhruv Menon <dhruvmenon1104@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/util.h>
#include <zephyr/task_wdt/task_wdt.h>

#ifdef CONFIG_TASK_WDT_RESET_INFO_RETENTION
#include <zephyr/device.h>
#include <zephyr/retention/retention.h>
#endif

#ifdef CONFIG_TASK_WDT_RESET_INFO_HW_FALLBACK
#include <zephyr/drivers/hwinfo.h>
#endif

#ifdef CONFIG_TASK_WDT_SAFE_MODE
#include <zephyr/retention/bootmode.h>
#endif

#include "task_wdt_reset_info.h"

#define RESET_RECORD_MAGIC   0x54574452U /* "TWDR" */
#define RESET_RECORD_VERSION 1U

/* Record kept across resets */
struct reset_record {
	uint32_t magic;
	uint16_t version;
	/* enum task_wdt_reset_reason */
	uint8_t reason;
	/* set right before a reset, cleared after the record was read at boot */
	uint8_t armed;
	int32_t channel_id;
	uint32_t fault_count;
	uint64_t uptime_ms;
	char thread_name[TASK_WDT_RESET_INFO_THREAD_NAME_SIZE];
	uint32_t crc;
};

#ifdef CONFIG_TASK_WDT_RESET_INFO_RETENTION
static const struct device *const retention_dev =
	DEVICE_DT_GET(DT_CHOSEN(zephyr_task_wdt_reset_info));

BUILD_ASSERT(DT_REG_SIZE(DT_CHOSEN(zephyr_task_wdt_reset_info)) >= sizeof(struct reset_record),
	     "zephyr,task-wdt-reset-info retention area too small");
#else
static struct reset_record stored_record __noinit;
#endif

static struct k_spinlock lock;
static bool loaded;
/* consecutive task watchdog resets up to and including the last one */
static uint32_t fault_count;
static bool last_info_valid;
static struct task_wdt_reset_info last_info;
#ifdef CONFIG_TASK_WDT_SAFE_MODE
/* fault count of the record stored by task_wdt_reset_info_save(), 0 if none */
static uint32_t armed_fault_count;
#endif

static uint32_t record_crc(const struct reset_record *rec)
{
	return crc32_ieee((const uint8_t *)rec, offsetof(struct reset_record, crc));
}

static bool record_read(struct reset_record *rec)
{
#ifdef CONFIG_TASK_WDT_RESET_INFO_RETENTION
	if (!device_is_ready(retention_dev) ||
	    retention_read(retention_dev, 0, (uint8_t *)rec, sizeof(*rec)) != 0) {
		return false;
	}
#else
	memcpy(rec, &stored_record, sizeof(*rec));
#endif

	return rec->magic == RESET_RECORD_MAGIC && rec->version == RESET_RECORD_VERSION &&
	       rec->crc == record_crc(rec);
}

static int record_write(struct reset_record *rec)
{
	rec->magic = RESET_RECORD_MAGIC;
	rec->version = RESET_RECORD_VERSION;
	rec->crc = record_crc(rec);

#ifdef CONFIG_TASK_WDT_RESET_INFO_RETENTION
	if (!device_is_ready(retention_dev)) {
		return -ENODEV;
	}

	return retention_write(retention_dev, 0, (const uint8_t *)rec, sizeof(*rec));
#else
	memcpy(&stored_record, rec, sizeof(*rec));

	return 0;
#endif
}

/* Write a record which is not armed, keeping only the fault count */
static int record_write_idle(void)
{
	struct reset_record rec = {
		.channel_id = -1,
		.fault_count = fault_count,
	};

	return record_write(&rec);
}

/* Find out whether the last reset was caused by the task watchdog, once per boot */
static void load_locked(void)
{
	struct reset_record rec;
	bool valid;

	if (loaded) {
		return;
	}

	loaded = true;
	valid = record_read(&rec);

	if (valid && rec.armed) {
		fault_count = rec.fault_count;
		last_info.reason = rec.reason;
		last_info.channel_id = rec.channel_id;
		last_info.fault_count = rec.fault_count;
		last_info.uptime_ms = rec.uptime_ms;
		memcpy(last_info.thread_name, rec.thread_name, sizeof(last_info.thread_name));
		last_info.thread_name[sizeof(last_info.thread_name) - 1] = '\0';
		last_info_valid = true;
	} else {
#ifdef CONFIG_TASK_WDT_RESET_INFO_HW_FALLBACK
		uint32_t cause;

		/* the task watchdog had no chance to store a record */
		if (hwinfo_get_reset_cause(&cause) == 0 && (cause & RESET_WATCHDOG) != 0) {
			fault_count = (valid ? rec.fault_count : 0U) + 1U;
			last_info.reason = TASK_WDT_RESET_HW_FALLBACK;
			last_info.channel_id = -1;
			last_info.fault_count = fault_count;
			last_info.uptime_ms = 0U;
			last_info.thread_name[0] = '\0';
			last_info_valid = true;
		}
#endif
		if (!last_info_valid && valid && rec.fault_count == 0U) {
			/* nothing to update */
			return;
		}
	}

	(void)record_write_idle();
}

static int task_wdt_reset_info_init(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	load_locked();

	k_spin_unlock(&lock, key);

	return 0;
}

SYS_INIT(task_wdt_reset_info_init, APPLICATION, 0);

static uint8_t reset_reason(enum task_wdt_fault_reason reason)
{
	switch (reason) {
	case TASK_WDT_FAULT_TOO_EARLY:
		return TASK_WDT_RESET_TOO_EARLY;
	case TASK_WDT_FAULT_DEADLINE_MIN:
		return TASK_WDT_RESET_DEADLINE_MIN;
	case TASK_WDT_FAULT_DEADLINE_MAX:
		return TASK_WDT_RESET_DEADLINE_MAX;
	case TASK_WDT_FAULT_SEQUENCE:
		return TASK_WDT_RESET_SEQUENCE;
	default:
		return TASK_WDT_RESET_TIMEOUT;
	}
}

void task_wdt_reset_info_save(const struct task_wdt_fault *fault)
{
	struct reset_record rec = {
		.reason = reset_reason(fault->reason),
		.armed = 1U,
		.channel_id = fault->channel_id,
	};
	k_spinlock_key_t key;

#ifdef CONFIG_THREAD_NAME
	if (fault->thread != NULL) {
		const char *name = k_thread_name_get(fault->thread);

		if (name != NULL) {
			strncpy(rec.thread_name, name, sizeof(rec.thread_name) - 1);
		}
	}
#endif

	rec.uptime_ms = (uint64_t)k_uptime_get();

	key = k_spin_lock(&lock);

	load_locked();
	rec.fault_count = fault_count < UINT32_MAX ? fault_count + 1U : UINT32_MAX;
	(void)record_write(&rec);
#ifdef CONFIG_TASK_WDT_SAFE_MODE
	armed_fault_count = rec.fault_count;
#endif

	k_spin_unlock(&lock, key);
}

void task_wdt_reset_info_cancel(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	(void)record_write_idle();
#ifdef CONFIG_TASK_WDT_SAFE_MODE
	armed_fault_count = 0U;
#endif

	k_spin_unlock(&lock, key);
}

#ifdef CONFIG_TASK_WDT_SAFE_MODE
void task_wdt_reset_info_safe_mode(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	if (armed_fault_count >= CONFIG_TASK_WDT_SAFE_MODE_RESETS) {
		(void)bootmode_set(CONFIG_TASK_WDT_SAFE_MODE_BOOT_MODE);
	}

	k_spin_unlock(&lock, key);
}
#endif

int task_wdt_reset_info_get(struct task_wdt_reset_info *info)
{
	k_spinlock_key_t key;
	int ret = 0;

	if (info == NULL) {
		return -EINVAL;
	}

	key = k_spin_lock(&lock);

	load_locked();

	if (last_info_valid) {
		*info = last_info;
	} else {
		ret = -ENOENT;
	}

	k_spin_unlock(&lock, key);

	return ret;
}

int task_wdt_reset_info_clear(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	int ret;

	load_locked();

	last_info_valid = false;
	fault_count = 0U;
	ret = record_write_idle();

	k_spin_unlock(&lock, key);

	return ret;
}
