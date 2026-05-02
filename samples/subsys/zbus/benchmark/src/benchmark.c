/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#include "messages.h"

#include <zephyr/fatal.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/zbus/zbus.h>

#if defined(CONFIG_BOARD_NATIVE_SIM)
#include "native_rtc.h"
#define GET_ARCH_TIME_NS() (native_rtc_gettime_us(RTC_CLOCK_PSEUDOHOSTREALTIME) * NSEC_PER_USEC)
#elif defined(CONFIG_ARCH_POSIX)
#error "This sample cannot be built for other POSIX arch boards than native_sim"
#else
#define GET_ARCH_TIME_NS() (k_cyc_to_ns_near64(sys_clock_cycle_get_32()))
#endif

LOG_MODULE_DECLARE(zbus, CONFIG_ZBUS_LOG_LEVEL);

#define CONSUMER_STACK_SIZE (CONFIG_IDLE_STACK_SIZE + CONFIG_BM_MESSAGE_SIZE)
#define PRODUCER_STACK_SIZE (CONFIG_MAIN_STACK_SIZE + CONFIG_BM_MESSAGE_SIZE)

ZBUS_CHAN_DEFINE(bm_channel,    /* Name */
		 struct bm_msg, /* Message type */

		 NULL,                 /* Validator */
		 NULL,                 /* User data */
		 ZBUS_OBSERVERS_EMPTY, /* observers */
		 ZBUS_MSG_INIT(0)      /* Initial value {0} */
);

#define BYTES_TO_BE_SENT (256LLU * 1024LLU)
atomic_t count;

static void producer_thread(void)
{
	LOG_INF("Benchmark 1 to %d using %s to transmit with message size: %u bytes",
		CONFIG_BM_ONE_TO,
		IS_ENABLED(CONFIG_BM_LISTENERS)
			? "LISTENERS"
			: (IS_ENABLED(CONFIG_BM_SUBSCRIBERS) ? "SUBSCRIBERS" : "MSG_SUBSCRIBERS"),
		CONFIG_BM_MESSAGE_SIZE);

	struct bm_msg msg = {{0}};

	uint16_t message_size = CONFIG_BM_MESSAGE_SIZE;

	memcpy(msg.bytes, &message_size, sizeof(message_size));

	uint64_t start_ns = GET_ARCH_TIME_NS();

	for (uint64_t internal_count = BYTES_TO_BE_SENT / CONFIG_BM_ONE_TO; internal_count > 0;
	     internal_count -= CONFIG_BM_MESSAGE_SIZE) {
		zbus_chan_pub(&bm_channel, &msg, K_FOREVER);
	}

	uint64_t end_ns = GET_ARCH_TIME_NS();

	uint64_t duration_ns = end_ns - start_ns;

	if (duration_ns == 0) {
		LOG_ERR("Something wrong. Duration is zero!\n");
		k_oops();
	}
	uint64_t i = ((BYTES_TO_BE_SENT * NSEC_PER_SEC) / MB(1)) / duration_ns;
	uint64_t f = ((BYTES_TO_BE_SENT * NSEC_PER_SEC * 100) / MB(1) / duration_ns) % 100;

	LOG_INF("Bytes sent = %llu, received = %lu", BYTES_TO_BE_SENT, atomic_get(&count));
	LOG_INF("Average data rate: %llu.%lluMB/s", i, f);
	LOG_INF("Duration: %llu.%09llus", duration_ns / NSEC_PER_SEC, duration_ns % NSEC_PER_SEC);

	printk("\n@%llu\n", duration_ns / 1000);
}

K_THREAD_DEFINE(producer_thread_id, PRODUCER_STACK_SIZE * 2, producer_thread, NULL, NULL, NULL, 5,
		0, 0);
