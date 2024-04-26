/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Run the application as a CAP Initiator for unicast
 *
 * This will start scanning for and connecting to a CAP acceptor, and then attempt to setup
 * unicast streams.
 *
 * @return 0 if success, errno on failure.
 */
int cap_initiator_unicast(void);
