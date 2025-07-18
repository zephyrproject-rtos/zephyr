/*
 * Copyright (c) 2024 Arif Balik <arifbalik@outlook.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(log_backend_mqtt, CONFIG_LOG_DEFAULT_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_backend_std.h>
#include <zephyr/logging/log_backend_mqtt.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/sys/util.h>
#include <zephyr/random/random.h>

#include <string.h>
#include <errno.h>

static bool panic;
static const char *mqtt_topic = CONFIG_LOG_BACKEND_MQTT_TOPIC_DEFAULT;
static uint8_t log_buf[CONFIG_LOG_BACKEND_MQTT_MAX_MSG_SIZE];
static uint32_t log_format_current = CONFIG_LOG_BACKEND_MQTT_OUTPUT_DEFAULT;

static int log_output_func(uint8_t *data, size_t length, void *output_ctx)
{
	__ASSERT(output_ctx != NULL, "Output context must not be NULL");

	struct mqtt_client *client = (struct mqtt_client *)output_ctx;
	struct mqtt_publish_param param = {0};

	param.message.topic.topic.utf8 = (const uint8_t *)mqtt_topic;
	param.message.topic.topic.size = strlen(mqtt_topic);
	param.message.topic.qos = CONFIG_LOG_BACKEND_MQTT_QOS;
	param.message.payload.data = data;
	param.message.payload.len = length;
	param.retain_flag = IS_ENABLED(CONFIG_LOG_BACKEND_MQTT_RETAIN);

#if (CONFIG_LOG_BACKEND_MQTT_QOS > MQTT_QOS_0_AT_MOST_ONCE)
	param.message_id = sys_rand32_get();
#endif

	int ret = mqtt_publish(client, &param);

	if (ret != 0) {
		return ret;
	}

	return length;
}

LOG_OUTPUT_DEFINE(log_output_mqtt, log_output_func, log_buf, sizeof(log_buf));

static void mqtt_backend_process(const struct log_backend *const backend,
				 union log_msg_generic *msg)
{
	if (panic) {
		return;
	}

	uint32_t flags = log_backend_std_get_flags();

	log_format_func_t log_output_func = log_format_func_t_get(log_format_current);

	log_output_ctx_set(&log_output_mqtt, backend->cb->ctx);

	log_output_func(&log_output_mqtt, &msg->log, flags);
}

static int mqtt_backend_format_set(const struct log_backend *const backend, uint32_t log_type)
{
	ARG_UNUSED(backend);
	log_format_current = log_type;
	return 0;
}

static void mqtt_backend_panic(const struct log_backend *const backend)
{
	ARG_UNUSED(backend);
	panic = true;
}

const struct log_backend_api log_backend_mqtt_api = {
	.process = mqtt_backend_process,
	.format_set = mqtt_backend_format_set,
	.panic = mqtt_backend_panic,
};

LOG_BACKEND_DEFINE(log_backend_mqtt, log_backend_mqtt_api, false);

int log_backend_mqtt_client_set(struct mqtt_client *client)
{
	log_backend_disable(&log_backend_mqtt);

	if (client != NULL) {
		log_backend_enable(&log_backend_mqtt, (void *)client, CONFIG_LOG_MAX_LEVEL);
	}

	return 0;
}

int log_backend_mqtt_topic_set(const char *topic)
{
	if (topic == NULL || strlen(topic) == 0) {
		return -EINVAL;
	}

	mqtt_topic = topic;

	return 0;
}
