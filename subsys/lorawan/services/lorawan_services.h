/*
 * Copyright (c) 2022 Martin Jäger <martin@libre.solar>
 * Copyright (c) 2022 tado GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_LORAWAN_SERVICES_LORAWAN_SERVICES_H_
#define ZEPHYR_SUBSYS_LORAWAN_SERVICES_LORAWAN_SERVICES_H_

#include <zephyr/kernel.h>
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
 * @param data       Data buffer to be sent.
 * @param len        Length of the data to be sent. Maximum length of the
 *                   buffer is 18 bytes.
 * @param timeout    Relative timeout in milliseconds when the uplink message
 *                   should be scheduled.
 *
 * @return 0 if message was successfully queued, negative errno otherwise.
 */
int lorawan_services_schedule_uplink(uint8_t port, uint8_t *data, uint8_t len, uint32_t timeout);

/**
 * @brief Reschedule a delayable work item to the LoRaWAN services work queue
 *
 * This work queue is used to schedule the uplink messages, but can be used by
 * any of the services for internal tasks.
 *
 * @param dwork pointer to the delayable work item.
 * @param delay the time to wait before submitting the work item.

 * @returns Result of call to k_work_reschedule_for_queue()
 */
int lorawan_services_reschedule_work(struct k_work_delayable *dwork, k_timeout_t delay);


/**
 * @brief Start a class C session
 *
 * If there is already an ongoing class C session, only the internal counter of
 * active sessions is incremented.
 *
 * @returns Number of active sessions if successful or negative errno otherwise.
 */
int lorawan_services_class_c_start(void);

/**
 * @brief Stop class C session and revert to class A
 *
 * If there is more than one class C session ongoing, only the internal counter
 * of active sessions is decremented.
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
