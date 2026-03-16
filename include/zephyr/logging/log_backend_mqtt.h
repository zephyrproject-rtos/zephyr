/*
 * Copyright (c) 2024 Arif Balik <arifbalik@outlook.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for the MQTT log backend API
 * @ingroup log_backend_mqtt
 */

#ifndef ZEPHYR_INCLUDE_LOGGING_LOG_BACKEND_MQTT_H_
#define ZEPHYR_INCLUDE_LOGGING_LOG_BACKEND_MQTT_H_

#include <zephyr/net/mqtt.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MQTT log backend API
 * @defgroup log_backend_mqtt MQTT log backend API
 * @ingroup log_backend
 * @{
 */

/**
 * @brief Set the MQTT client instance to be able to publish application's log messages to broker.
 *
 * This function allows the application to provide its own initialized
 * MQTT client to the log backend. The backend will use this client
 * exclusively for publishing log messages via mqtt_publish().
 *
 * @param client Pointer to an initialized and connected MQTT client.
 *               The client must remain valid for the lifetime of the
 *               log backend usage. Pass NULL to disable MQTT logging.
 *
 * @return 0 on success, negative error code on failure.
 *
 * @note The MQTT client must be connected before calling this function.
 * @note The backend will not manage the client connection - this is the
 *       responsibility of the application.
 * @note The backend will only use mqtt_publish() and will not perform
 *       any other operations on the client.
 */
int log_backend_mqtt_client_set(struct mqtt_client *client);

/**
 * @brief Set the MQTT topic to which log messages will be published.
 *
 * Allows the application to specify the MQTT topic that the log backend
 * will use for publishing log messages to.
 *
 * @param topic Pointer to a null-terminated string containing the MQTT topic.
 *              The topic must remain valid for the lifetime of the log backend usage.
 *
 * @return 0 on success, negative error code on failure.
 *
 * @note The topic must be a valid UTF-8 string, null-terminated and should not exceed
 *       the maximum length supported by the MQTT broker.
 */
int log_backend_mqtt_topic_set(const char *topic);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_LOGGING_LOG_BACKEND_MQTT_H_ */
