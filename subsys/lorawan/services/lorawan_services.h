/*
 * Copyright (c) 2022 Martin Jäger <martin@libre.solar>
 * Copyright (c) 2022 tado GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_LORAWAN_SERVICES_LORAWAN_SERVICES_H_
#define ZEPHYR_SUBSYS_LORAWAN_SERVICES_LORAWAN_SERVICES_H_

#include <zephyr/zephyr.h>
#include <zephyr/lorawan/lorawan.h>

/**
 * Unique package identifiers used for LoRaWAN services.
 */
enum lorawan_package_id {
	LORAWAN_PACKAGE_ID_COMPLIANCE = 0,
	LORAWAN_PACKAGE_ID_CLOCK_SYNC = 1,
	LORAWAN_PACKAGE_ID_REMOTE_MULTICAST_SETUP = 2,
	LORAWAN_PACKAGE_ID_FRAG_TRANSPORT_BLOCK = 3,
};

/**
 * Default ports used for LoRaWAN services.
 */
enum lorawan_services_port {
	LORAWAN_PORT_MULTICAST = 200,
	LORAWAN_PORT_FRAG_TRANSPORT = 201,
	LORAWAN_PORT_CLOCK_SYNC = 202,
};

/**
 * @brief Send unconfirmed LoRaWAN uplink message after the specified timeout
 *
 * @param port       Port to be used for sending data.
 * @param data       Data buffer to be sent
 * @param len        Length of the buffer to be sent. Maximum length of this
 *                   buffer is 255 bytes but the actual payload size varies with
 *                   region and datarate.
 * @param timeout    Timeout when the uplink message should be scheduled.
 *
 * @return 0 if message was successfully queued, negative errno otherwise.
 */
int lorawan_services_schedule_uplink(uint8_t port, uint8_t *data, uint8_t len, k_timeout_t timeout);

/**
 * @brief Get the work queue handle for LoRaWAN services
 *
 * This work queue is used to schedule the uplink messages, but can be used by
 * any of the services for internal tasks.
 *
 * @returns Services work queue handle
 */
struct k_work_q *lorawan_services_get_work_queue(void);

/**
 * @brief Start a class C session
 *
 * If there is already an ongoing class C session, only the internal counter of
 * active sessions is increased.
 *
 * @returns Number of active sessions if successful or negative errno otherwise.
 */
int lorawan_services_class_c_start(void);

/**
 * @brief Stop class C session and revert to class A
 *
 * If there is more than one class C session ongoing, only the internal counter
 * of active sessions is decreased.
 *
 * @returns Number of active sessions if successful or negative errno otherwise.
 */
int lorawan_services_class_c_stop(void);

/**
 * @brief Retrieve number of active sessions
 *
 * Can be used to determine if sessions are ongoing and avoid disturbing an
 * ongoing session by sending out unnecessary messages.
 *
 * @returns Number of active class C sessions.
 */
int lorawan_services_class_c_active(void);

#endif /* ZEPHYR_SUBSYS_LORAWAN_SERVICES_LORAWAN_SERVICES_H_ */
