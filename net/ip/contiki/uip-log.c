/* uip-log.c - Logging support */

/*
 * Copyright (c) 2015 Intel Corporation
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
 * limitations under the License.
 */

#include <misc/printk.h>

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define PRINT           printf
#else
#include <misc/printk.h>
#define PRINT           printk
#endif

#include "ip/uipopt.h"
#include "sys/log.h"

#if UIP_LOGGING
void uip_log(char *message)
{
	PRINT(message);
	PRINT("\n");
}
#endif /* UIP_LOGGING */

#if LOG_CONF_ENABLED
void log_message(const char *part1, const char *part2)
{
	PRINT(part1);
	PRINT(part2);
	PRINT("\n");
}
#endif /* LOG_CONF_ENABLED */
