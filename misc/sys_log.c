/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License
 * .
 */

#include <misc/sys_log.h>

void syslog_hook_default(const char *fmt, ...)
{
	(void)(fmt);  /* Prevent warning about unused argument */
}

void (*syslog_hook)(const char *fmt, ...) = syslog_hook_default;

void syslog_hook_install(void (*hook)(const char *, ...))
{
	syslog_hook = hook;
}
