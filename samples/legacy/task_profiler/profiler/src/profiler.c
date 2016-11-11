/* profiler.c - Profiler code for flushing data on UART */

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

#include <zephyr.h>
#include <misc/kernel_event_logger.h>
#include <uart.h>

#define PROF_LOG_HEADER "*** [PROF] "

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define PRINT(format, ...)           printf(PROF_LOG_HEADER format, ##__VA_ARGS__)
#else
#include <misc/printk.h>
#define PRINT(format, ...)           printk(PROF_LOG_HEADER format, ##__VA_ARGS__)
#endif

#define PROF_HEADER_SIZE_U8 4
#define PROF_MAX_EVENT_SIZE_U32 4
#define PROFILER_SLEEP_MS 5000
#define PROFILER_SLEEPTICKS (PROFILER_SLEEP_MS * sys_clock_ticks_per_sec / 1000)
/*
 * This flag can be defined to check UART consistency
 * In that case, "profterm" used on the host must be compiled
 * in TESTMODE
 */
#undef PROF_UART_TESTMODE

struct prof_platform_info {
	uint32_t hw_ticks_per_sec;
};

struct prof_platform_info prof_pltfm_info;
static struct device *uart_console_dev;

int prof_initialized;

#define out(c) uart_poll_out(uart_console_dev, (char)c)

#define DLE 16
#define PROFILER_INFO 255 /* Must not overlap with event ID */

void prof_send_platform_info(void)
{
	uint8_t data_length = SIZE32_OF(prof_pltfm_info);
	uint32_t *data = (uint32_t *)(&prof_pltfm_info);
	int key;
	int i;

	key = irq_lock();
	out(DLE);
	out(PROFILER_INFO); /* Event ID */
	out(data_length);
	for (i = 0; i < data_length; i++) {
		out((data[i] & 0x000000ff));
		out(((data[i] & 0x0000ff00) >> 8));
		out(((data[i] & 0x00ff0000) >> 16));
		out(((data[i] & 0xff000000) >> 24));
	}
	irq_unlock(key);
}

#if defined(CONFIG_CONSOLE_HANDLER_SHELL) && defined(CONFIG_KERNEL_EVENT_LOGGER_DYNAMIC)
/* Profiler shell is only enabled if kernel event logger dynamic
 * enable/disable and console handler shell are enabled
 * Warning: if kernel event logger dynamic is enabled but no shell is available,
 * kernel event logger won't log anything
 */
#define PROFILER_SHELL
#else
#undef PROFILER_SHELL
#endif

#ifdef PROFILER_SHELL

#include "profiler.h"
#include <stdlib.h>
#include <string.h>
#include <misc/shell.h>

/* kernel_event_logger flags. Bit N = enable event ID N, except for task monitor
 * by default all enabled events at build time are logged when profiler enabled
 */
int cfg1 = 0xFFFF;
/* task monitor flags. By default set to build time configuration */
#if defined(CONFIG_TASK_MONITOR_MASK)
int cfg2 = CONFIG_TASK_MONITOR_MASK;
#else
int cfg2;
#endif

/*TODO: could configure flush period as well ? */
#define PROF_STOPPED  0
#define PROF_STARTING 1
#define PROF_STARTED  2
int prof_state;

int shell_cmd_prof(int argc, char *argv[])
{
	/*
	 * prof [cmd]
	 *   start
	 *   stop
	 *   cfg id1 id2
	 */
	int len = strlen(argv[1]);

	if (!strncmp(argv[1], "start", len)) {
		PRINT("Start\n");
		sys_k_event_logger_set_mask(cfg1);
#ifdef CONFIG_TASK_MONITOR
		sys_k_event_logger_set_monitor_mask(cfg2);
#endif
		if ((sys_k_event_logger_get_mask() != 0)
#ifdef CONFIG_TASK_MONITOR
		    || (sys_k_event_logger_get_monitor_mask() != 0)
#endif
		   ) {
			prof_state = PROF_STARTING;
		}
	} else if (!strncmp(argv[1], "stop", len)) {
		PRINT("Stop\n");
		sys_k_event_logger_set_mask(0);
#ifdef CONFIG_TASK_MONITOR
		sys_k_event_logger_set_monitor_mask(0);
#endif
		/* Could flush buffer here... but flush may be on-going... */
	} else if (!strncmp(argv[1], "cfg", len)) {
		if (argc >= 4) {
			cfg1 = atoi(argv[2]);
			cfg2 = atoi(argv[3]);
			PRINT("Configure %d %d\n", cfg1, cfg2);
		} else {
#ifdef CONFIG_TASK_MONITOR
			PRINT("%d(%d) %d(%d)\n",
					sys_k_event_logger_get_mask(),
					cfg1,
					sys_k_event_logger_get_monitor_mask(),
					cfg2);
#else
			PRINT("%d(%d)\n",
					sys_k_event_logger_get_mask(),
					cfg1);
#endif
		}
	} else {
		return -EINVAL;
	}

	return 0;
}

#define MY_SHELL_ENTITY "profiler"

struct shell_cmd prof_commands[] = {
	PROF_CMD,
	{ NULL, NULL}
};
#endif /* PROFILER_SHELL */

#undef TIMESTAMP_MODE_RTC
#undef TIMESTAMP_MODE_COUNTER
#if defined(CONFIG_KERNEL_EVENT_LOGGER_CUSTOM_TIMESTAMP)
#if defined(CONFIG_RTC) && !defined(PROFILER_USE_COUNTER)
#define TIMESTAMP_MODE_RTC
#elif defined(CONFIG_COUNTER)
#define TIMESTAMP_MODE_COUNTER
#else
#error KERNEL_EVENT_LOGGER_CUSTOM_TIMESTAMP set but RTC/COUNTER not enabled
#endif

#if defined(TIMESTAMP_MODE_RTC)
#include <rtc.h>
struct device *rtc_dev;
#elif defined(TIMESTAMP_MODE_COUNTER)
#include <counter.h>
struct device *counter_dev;
#endif

#if defined(TIMESTAMP_MODE_RTC)
uint32_t prof_read_timer(void)
{
	return rtc_read(rtc_dev);
}
#elif defined(TIMESTAMP_MODE_COUNTER)
uint32_t prof_read_timer(void)
{
	return counter_read(counter_dev);
}
#endif

#endif /* CONFIG_KERNEL_EVENT_LOGGER_CUSTOM_TIMESTAMP */

void prof_init(void)
{
#if defined(TIMESTAMP_MODE_RTC)
	struct rtc_config config;

	rtc_dev = device_get_binding("RTC_0");
	config.init_val = 0;
	config.alarm_enable = 0;
	rtc_enable(rtc_dev);
	rtc_set_config(rtc_dev, &config);

	sys_k_event_logger_set_timer(prof_read_timer);
#elif defined(TIMESTAMP_MODE_COUNTER)
	counter_dev = device_get_binding("AON_COUNTER");
	counter_start(counter_dev);

	sys_k_event_logger_set_timer(prof_read_timer);
#endif

#ifdef PROFILER_SHELL
#ifndef PROFILER_NO_SHELL_REGISTER
	SHELL_REGISTER(MY_SHELL_ENTITY, prof_commands);
	shell_register_default_module(MY_SHELL_ENTITY);
#endif
#endif

#if defined(TIMESTAMP_MODE_RTC) || defined(TIMESTAMP_MODE_COUNTER)
	prof_pltfm_info.hw_ticks_per_sec = 32768;
#else
	prof_pltfm_info.hw_ticks_per_sec = (sys_clock_ticks_per_sec * sys_clock_hw_cycles_per_tick);
#endif

	uart_console_dev = device_get_binding(CONFIG_UART_CONSOLE_ON_DEV_NAME);

	prof_initialized = 1;
}


void prof_flush(void)
{
	int i = 0;

#ifdef PROF_UART_TESTMODE
	/* Send fixed sequence to UART to check integrity on host side */
	key = irq_lock();
	for (i = 0; i < 256; i++) {
		out(i);
	}
	irq_unlock(key);
#else
	int res;
	uint32_t data[PROF_MAX_EVENT_SIZE_U32];
	uint8_t data_length = SIZE32_OF(data);
	uint8_t dropped_count;
	uint16_t event_id;
	static int dropped;
	int key;

	if (!prof_initialized)
		prof_init();

#ifdef PROFILER_SHELL
	/* Faster exit when profiling is disabled
	 * Microkernel: profiler background task remains active and schedules periodically
	 * when profiler is disabled to make things simple
	 * Nanokernel: the background task can continue calling flush without taking care
	 * of profiler state
	 * Have to consider improving this (task suspend/resume in microkernel mode)
	 */
	if (prof_state == PROF_STOPPED) {
		return;
	} else if (prof_state == PROF_STARTING) {
		prof_send_platform_info();
		prof_state = PROF_STARTED;
	}
#else
	prof_send_platform_info();
#endif

	while (1) {
		event_id = 0;
		data_length = SIZE32_OF(data);
		dropped_count = 0;

		/* collect the data */
		res = sys_k_event_logger_get(&event_id, &dropped_count, data,
					    &data_length);

		dropped += dropped_count;

		if (res == 0) {
			if (dropped > 0) {
				PRINT("#Dropped events=%d\n", dropped);
				dropped = 0;
			}

#ifdef PROFILER_SHELL
			/* Profiler flush activity will be bypassed after buffer have been fully
			 * flushed. This is done atomically to avoid running conditions with shell
			 * command execution (e.g. profiler re-enabled in the mean time)
			 * Locking IRQs... since code must be nanokernel compliant
			 */
			key = irq_lock();
			if ((sys_k_event_logger_get_mask() == 0)
#ifdef CONFIG_TASK_MONITOR
			    && (sys_k_event_logger_get_monitor_mask() == 0)
#endif
			   ) {
				prof_state = PROF_STOPPED;
			}
			irq_unlock(key);
#endif

			return;
		} else if (res < 0) {
			PRINT("#Error: %d\n", res);
		} else {
			if (event_id != 0) {
				key = irq_lock();
				/*PRINT("Event %x(%x): %x %x\n", event_id, data_length,
				 *				 data[0], data[1]);
				 */
				out(DLE);
				/* NOTE: event ID is 16-bit but we only send 8-bits LSB */
				out(event_id & 0xff);
				out(data_length);
				for (i = 0; i < data_length; i++) {
					out((data[i] & 0x000000ff));
					out(((data[i] & 0x0000ff00) >> 8));
					out(((data[i] & 0x00ff0000) >> 16));
					out(((data[i] & 0xff000000) >> 24));
				}
				irq_unlock(key);
			}
		}
	}
#endif /* ! PROF_UART_TESTMODE */
}

#ifdef CONFIG_MICROKERNEL

void prof(void)
{
	prof_init();

	PRINT("Background task started\n");

	while (1) {
		prof_flush();
		task_sleep(PROFILER_SLEEPTICKS);
	}
}

#endif /* CONFIG_MICROKERNEL */
