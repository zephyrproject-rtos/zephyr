/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#include "messages.h"

#include <stdint.h>

#include <zephyr/fatal.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/zbus/zbus.h>

#if defined(CONFIG_ARCH_POSIX)
#include "native_rtc.h"
#define GET_ARCH_TIME_NS() (native_rtc_gettime_us(RTC_CLOCK_PSEUDOHOSTREALTIME) * NSEC_PER_USEC)
#else
#define GET_ARCH_TIME_NS() (k_cyc_to_ns_near32(sys_clock_cycle_get_32()))
#endif

LOG_MODULE_DECLARE(zbus, CONFIG_ZBUS_LOG_LEVEL);

#define CONSUMER_STACK_SIZE (CONFIG_IDLE_STACK_SIZE + CONFIG_BM_MESSAGE_SIZE)
#define PRODUCER_STACK_SIZE (CONFIG_MAIN_STACK_SIZE + CONFIG_BM_MESSAGE_SIZE)

ZBUS_CHAN_DEFINE(bm_channel,		   /* Name */
		 struct external_data_msg, /* Message type */

		 NULL, /* Validator */
		 NULL, /* User data */
		 ZBUS_OBSERVERS(s1

#if (CONFIG_BM_ONE_TO >= 2LLU)
				,
				s2
#if (CONFIG_BM_ONE_TO > 2LLU)
				,
				s3, s4
#if (CONFIG_BM_ONE_TO > 4LLU)
				,
				s5, s6, s7, s8
#if (CONFIG_BM_ONE_TO > 8LLU)
				,
				s9, s10, s11, s12, s13, s14, s15, s16
#endif
#endif
#endif
#endif
				), /* observers */
		 ZBUS_MSG_INIT(0)  /* Initial value {0} */
);

#define BYTES_TO_BE_SENT (256LLU * 1024LLU)
static atomic_t count;

#if (CONFIG_BM_ASYNC == 1)
ZBUS_SUBSCRIBER_DEFINE(s1, 4);
#if (CONFIG_BM_ONE_TO >= 2LLU)
ZBUS_SUBSCRIBER_DEFINE(s2, 4);
#if (CONFIG_BM_ONE_TO > 2LLU)
ZBUS_SUBSCRIBER_DEFINE(s3, 4);
ZBUS_SUBSCRIBER_DEFINE(s4, 4);
#if (CONFIG_BM_ONE_TO > 4LLU)
ZBUS_SUBSCRIBER_DEFINE(s5, 4);
ZBUS_SUBSCRIBER_DEFINE(s6, 4);
ZBUS_SUBSCRIBER_DEFINE(s7, 4);
ZBUS_SUBSCRIBER_DEFINE(s8, 4);
#if (CONFIG_BM_ONE_TO > 8LLU)
ZBUS_SUBSCRIBER_DEFINE(s9, 4);
ZBUS_SUBSCRIBER_DEFINE(s10, 4);
ZBUS_SUBSCRIBER_DEFINE(s11, 4);
ZBUS_SUBSCRIBER_DEFINE(s12, 4);
ZBUS_SUBSCRIBER_DEFINE(s13, 4);
ZBUS_SUBSCRIBER_DEFINE(s14, 4);
ZBUS_SUBSCRIBER_DEFINE(s15, 4);
ZBUS_SUBSCRIBER_DEFINE(s16, 4);
#endif
#endif
#endif
#endif

#define S_TASK(name)                                                                               \
	void name##_task(void)                                                                     \
	{                                                                                          \
		struct external_data_msg *actual_message_data;                                     \
		const struct zbus_channel *chan;                                                   \
		struct bm_msg msg_received;                                                        \
                                                                                                   \
		while (!zbus_sub_wait(&name, &chan, K_FOREVER)) {                                  \
			zbus_chan_claim(chan, K_NO_WAIT);                                          \
                                                                                                   \
			actual_message_data = zbus_chan_msg(chan);                                 \
			__ASSERT_NO_MSG(actual_message_data->reference != NULL);                   \
                                                                                                   \
			memcpy(&msg_received, actual_message_data->reference,                      \
			       sizeof(struct bm_msg));                                             \
                                                                                                   \
			zbus_chan_finish(chan);                                                    \
                                                                                                   \
			atomic_add(&count, CONFIG_BM_MESSAGE_SIZE);                                \
		}                                                                                  \
	}                                                                                          \
                                                                                                   \
	K_THREAD_DEFINE(name##_id, CONSUMER_STACK_SIZE, name##_task, NULL, NULL, NULL, 3, 0, 0);

S_TASK(s1)
#if (CONFIG_BM_ONE_TO >= 2LLU)
S_TASK(s2)
#if (CONFIG_BM_ONE_TO > 2LLU)
S_TASK(s3)
S_TASK(s4)
#if (CONFIG_BM_ONE_TO > 4LLU)
S_TASK(s5)
S_TASK(s6)
S_TASK(s7)
S_TASK(s8)
#if (CONFIG_BM_ONE_TO > 8LLU)
S_TASK(s9)
S_TASK(s10)
S_TASK(s11)
S_TASK(s12)
S_TASK(s13)
S_TASK(s14)
S_TASK(s15)
S_TASK(s16)
#endif
#endif
#endif
#endif

#else /* SYNC */

static void s_cb(const struct zbus_channel *chan);

ZBUS_LISTENER_DEFINE(s1, s_cb);

#if (CONFIG_BM_ONE_TO >= 2LLU)
ZBUS_LISTENER_DEFINE(s2, s_cb);
#if (CONFIG_BM_ONE_TO > 2LLU)
ZBUS_LISTENER_DEFINE(s3, s_cb);
ZBUS_LISTENER_DEFINE(s4, s_cb);
#if (CONFIG_BM_ONE_TO > 4LLU)
ZBUS_LISTENER_DEFINE(s5, s_cb);
ZBUS_LISTENER_DEFINE(s6, s_cb);
ZBUS_LISTENER_DEFINE(s7, s_cb);
ZBUS_LISTENER_DEFINE(s8, s_cb);
#if (CONFIG_BM_ONE_TO > 8LLU)
ZBUS_LISTENER_DEFINE(s9, s_cb);
ZBUS_LISTENER_DEFINE(s10, s_cb);
ZBUS_LISTENER_DEFINE(s11, s_cb);
ZBUS_LISTENER_DEFINE(s12, s_cb);
ZBUS_LISTENER_DEFINE(s13, s_cb);
ZBUS_LISTENER_DEFINE(s14, s_cb);
ZBUS_LISTENER_DEFINE(s15, s_cb);
ZBUS_LISTENER_DEFINE(s16, s_cb);
#endif
#endif
#endif
#endif

static void s_cb(const struct zbus_channel *chan)
{
	struct bm_msg msg_received;
	const struct external_data_msg *actual_message_data = zbus_chan_const_msg(chan);

	memcpy(&msg_received, actual_message_data->reference, sizeof(struct bm_msg));

	count += CONFIG_BM_MESSAGE_SIZE;
}

#endif /* CONFIG_BM_ASYNC */

static void producer_thread(void)
{
	LOG_INF("Benchmark 1 to %d: Dynamic memory, %sSYNC transmission and message size %u",
		CONFIG_BM_ONE_TO, IS_ENABLED(CONFIG_BM_ASYNC) ? "A" : "", CONFIG_BM_MESSAGE_SIZE);

	struct bm_msg msg;
	struct external_data_msg *actual_message_data;

	for (uint64_t i = (CONFIG_BM_MESSAGE_SIZE - 1); i > 0; --i) {
		msg.bytes[i] = i;
	}

	zbus_chan_claim(&bm_channel, K_NO_WAIT);

	actual_message_data = zbus_chan_msg(&bm_channel);
	actual_message_data->reference = k_malloc(sizeof(struct bm_msg));
	__ASSERT_NO_MSG(actual_message_data->reference != NULL);
	actual_message_data->size = sizeof(struct bm_msg);
	__ASSERT_NO_MSG(actual_message_data->size > 0);

	zbus_chan_finish(&bm_channel);

	uint32_t start_ns = GET_ARCH_TIME_NS();

	for (uint64_t internal_count = BYTES_TO_BE_SENT / CONFIG_BM_ONE_TO; internal_count > 0;
	     internal_count -= CONFIG_BM_MESSAGE_SIZE) {
		zbus_chan_claim(&bm_channel, K_NO_WAIT);

		actual_message_data = zbus_chan_msg(&bm_channel);

		memcpy(actual_message_data->reference, &msg, CONFIG_BM_MESSAGE_SIZE);

		zbus_chan_finish(&bm_channel);

		zbus_chan_notify(&bm_channel, K_MSEC(200));
	}

	uint32_t end_ns = GET_ARCH_TIME_NS();

	uint32_t duration = end_ns - start_ns;

	if (duration == 0) {
		LOG_ERR("Something wrong. Duration is zero!\n");
		k_oops();
	}
	uint64_t i = ((BYTES_TO_BE_SENT * NSEC_PER_SEC) / MB(1)) / duration;
	uint64_t f = ((BYTES_TO_BE_SENT * NSEC_PER_SEC * 100) / MB(1) / duration) % 100;

	LOG_INF("Bytes sent = %lld, received = %lu", BYTES_TO_BE_SENT, atomic_get(&count));
	LOG_INF("Average data rate: %llu.%lluMB/s", i, f);
	LOG_INF("Duration: %u.%uus", duration / NSEC_PER_USEC, duration % NSEC_PER_USEC);

	printk("\n@%u\n", duration);
}

K_THREAD_DEFINE(producer_thread_id, PRODUCER_STACK_SIZE, producer_thread,
		NULL, NULL, NULL, 5, 0, 0);
