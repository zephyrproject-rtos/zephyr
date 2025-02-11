/*
 * Copyright (c) 2024, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#undef LOG_ERR
#include <zephyr/posix/syslog.h>
#include <zephyr/sys/printk.h>

static struct k_spinlock syslog_lock;
static uint8_t syslog_mask;

static int syslog_priority_to_zephyr_log_level(int priority)
{
	switch (priority) {
	case LOG_EMERG:
	case LOG_ALERT:
	case LOG_CRIT:
	case LOG_ERR:
		return LOG_LEVEL_ERR;
	case LOG_WARNING:
		return LOG_LEVEL_WRN;
	case LOG_NOTICE:
	case LOG_INFO:
		return LOG_LEVEL_INF;
	case LOG_DEBUG:
		return LOG_LEVEL_DBG;
	default:
		return -EINVAL;
	}
}

void closelog(void)
{
}

void openlog(const char *ident, int option, int facility)
{
	ARG_UNUSED(ident);
	ARG_UNUSED(option);
	ARG_UNUSED(facility);
}

void syslog(int priority, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	vsyslog(priority, format, ap);
	va_end(ap);
}

int setlogmask(int maskpri)
{
	int oldpri = -1;

	K_SPINLOCK(&syslog_lock) {
		oldpri = syslog_mask;
		syslog_mask = maskpri;
	}

	return oldpri;
}

void vsyslog(int priority, const char *format, va_list ap)
{
	uint8_t mask = 0;
	int level = syslog_priority_to_zephyr_log_level(priority);

	if (level < 0) {
		/* invalid priority */
		return;
	}

	K_SPINLOCK(&syslog_lock) {
		mask = syslog_mask;
	}

	if ((BIT(level) & mask) == 0) {
		/* masked */
		return;
	}

#if !defined(CONFIG_LOG) || defined(CONFIG_LOG_MODE_MINIMAL)
	vprintk(format, ap);
#else
	log_generic(level, format, ap);
#endif
}
