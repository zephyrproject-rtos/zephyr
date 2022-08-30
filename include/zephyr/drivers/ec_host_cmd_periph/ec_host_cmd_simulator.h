/*
 * Copyright (c) 2020 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_EC_HOST_CMD_SIMULATOR_H_
#define ZEPHYR_INCLUDE_DRIVERS_EC_HOST_CMD_SIMULATOR_H_

/**
 * @file
 * @brief Header for commands to interact with the simulator outside of normal
 *        device interface.
 */

/* For ec_host_cmd_periph_api_send function pointer type */
#include <zephyr/drivers/ec_host_cmd_periph.h>

/**
 * @brief Install callback for when this device would sends data to host
 *
 * When this host command simulator device should send data to the host, it
 * will call the the callback parameter provided by this function. Note that
 * only one callback may be installed at a time. Calling this a second time
 * will override the first callback installation.
 *
 * @param cb Callback that is called when device would send data to host.
 */
void ec_host_cmd_periph_sim_install_send_cb(ec_host_cmd_periph_api_send cb);

/**
 * @brief Simulate receiving data from host as passed in to this function
 *
 * Calling this function simulates that data was sent from the host to the DUT.
 *
 * @param buffer The buffer that contains the data to receive.
 * @param len The number of bytes that are received from the above buffer.
 *
 * @retval 0 if successful
 * @retval -ENOMEM if len is greater than the RX buffer size.
 * @retval -EBUSY if the host command framework is busy with another request.
 */
int ec_host_cmd_periph_sim_data_received(const uint8_t *buffer, size_t len);

#endif /* ZEPHYR_INCLUDE_DRIVERS_EC_HOST_CMD_SIMULATOR_H_ */
