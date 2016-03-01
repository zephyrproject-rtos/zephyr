/* FIXME - dummy file to be replaced/removed at some point */

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
 * limitations under the License.
 */

#ifndef __RTIMER_ARCH_H__
#define __RTIMER_ARCH_H__

#include "contiki-conf.h"
#include "sys/clock.h"

#define RTIMER_ARCH_SECOND CLOCK_CONF_SECOND

static inline rtimer_clock_t rtimer_arch_now(void) { return clock_time(); }
int rtimer_arch_check(void);
int rtimer_arch_pending(void);
rtimer_clock_t rtimer_arch_next(void);
static inline void rtimer_arch_init(void) {}

#endif /* __RTIMER_ARCH_H__ */
