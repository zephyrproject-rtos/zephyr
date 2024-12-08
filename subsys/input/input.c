/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/iterable_sections.h>

LOG_MODULE_REGISTER(input, CONFIG_INPUT_LOG_LEVEL);

#ifdef CONFIG_INPUT_MODE_THREAD

K_MSGQ_DEFINE(input_msgq, sizeof(struct input_event),
	      CONFIG_INPUT_QUEUE_MAX_MSGS, 4);

#endif

static void input_process(struct input_event *evt)
{
	STRUCT_SECTION_FOREACH(input_callback, callback) {
		if (callback->dev == NULL || callback->dev == evt->dev) {
			callback->callback(evt, callback->user_data);
		}
	}
}

bool input_queue_empty(void)
{
#ifdef CONFIG_INPUT_MODE_THREAD
	if (k_msgq_num_used_get(&input_msgq) > 0) {
		return false;
	}
#endif
	return true;
}

int input_report(const struct device *dev,
		 uint8_t type, uint16_t code, int32_t value, bool sync,
		 k_timeout_t timeout)
{
	struct input_event evt = {
		.dev = dev,
		.sync = sync,
		.type = type,
		.code = code,
		.value = value,
	};

#ifdef CONFIG_INPUT_MODE_THREAD
	int ret;

	if (!K_TIMEOUT_EQ(timeout, K_NO_WAIT) &&
	    k_current_get() == k_work_queue_thread_get(&k_sys_work_q)) {
		LOG_DBG("Timeout discarded. No blocking in syswq.");
		timeout = K_NO_WAIT;
	}

	ret = k_msgq_put(&input_msgq, &evt, timeout);
	if (ret < 0) {
		LOG_WRN("Event dropped, queue full, not blocking in syswq.");
		return ret;
	}

	return 0;
#else
	input_process(&evt);
	return 0;
#endif
}

#ifdef CONFIG_INPUT_MODE_THREAD

static void input_thread(void)
{
	struct input_event evt;
	int ret;

	while (true) {
		ret = k_msgq_get(&input_msgq, &evt, K_FOREVER);
		if (ret) {
			LOG_ERR("k_msgq_get error: %d", ret);
			continue;
		}

		input_process(&evt);
	}
}

#define INPUT_THREAD_PRIORITY \
	COND_CODE_1(CONFIG_INPUT_THREAD_PRIORITY_OVERRIDE, \
		    (CONFIG_INPUT_THREAD_PRIORITY), (K_LOWEST_APPLICATION_THREAD_PRIO))

K_THREAD_DEFINE(input,
		CONFIG_INPUT_THREAD_STACK_SIZE,
		input_thread,
		NULL, NULL, NULL,
		INPUT_THREAD_PRIORITY, 0, 0);

#endif /* CONFIG_INPUT_MODE_THREAD */
