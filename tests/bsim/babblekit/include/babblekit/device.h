/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file provides utilities related to the device and cli arguments.
 */


/*
 * @brief Get the device's simulation number
 *
 * This returns the device's number in the BabbleSim simulation.
 * Ie will return 1 if the device was started with the "-d 1" argument.
 *
 * @return Device number in simulation
 */
int bk_device_get_number(void);
