/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/sys_log.h>

void syslog_hook_default(const char *fmt, ...)
{
	(void)(fmt);  /* Prevent warning about unused argument */
}

void (*syslog_hook)(const char *fmt, ...) = syslog_hook_default;

void syslog_hook_install(void (*hook)(const char *, ...))
{
	syslog_hook = hook;
}
