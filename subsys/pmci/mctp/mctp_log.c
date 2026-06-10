/*
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <libmctp.h>

LOG_MODULE_REGISTER(libmctp, CONFIG_MCTP_LOG_LEVEL);

static int mctp_log_level;

static void mctp_zephyr_log(int level, const char *fmt, va_list ap)
{
	char buf[160];
	int rc;

	if (level > mctp_log_level) {
		return;
	}

	rc = vsnprintk(buf, sizeof(buf), fmt, ap);
	if (rc < 0) {
		return;
	}

	switch (level) {
	case MCTP_LOG_ERR:
		LOG_ERR("%s", buf);
		break;
	case MCTP_LOG_WARNING:
		LOG_WRN("%s", buf);
		break;
	case MCTP_LOG_NOTICE:
	case MCTP_LOG_INFO:
		LOG_INF("%s", buf);
		break;
	default:
		LOG_DBG("%s", buf);
		break;
	}
}

static int mctp_log_level_from_config(void)
{
	if (IS_ENABLED(CONFIG_MCTP_LOG_LEVEL_DBG)) {
		return MCTP_LOG_DEBUG;
	}

	if (IS_ENABLED(CONFIG_MCTP_LOG_LEVEL_INF)) {
		return MCTP_LOG_INFO;
	}

	if (IS_ENABLED(CONFIG_MCTP_LOG_LEVEL_WRN)) {
		return MCTP_LOG_WARNING;
	}

	if (IS_ENABLED(CONFIG_MCTP_LOG_LEVEL_ERR)) {
		return MCTP_LOG_ERR;
	}

	return -1;
}

int mctp_log_init(void)
{
	mctp_log_level = mctp_log_level_from_config();
	if (mctp_log_level >= 0) {
		mctp_set_log_custom(mctp_zephyr_log);
	}

	return 0;
}

SYS_INIT_NAMED(mctp_log, mctp_log_init, POST_KERNEL, 0);
