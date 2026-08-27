/*
 * Copyright (c) 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

/**
 * @brief Setup web server
 *
 * This function starts a network monitoring thread
 * This thread will start automatically Web Server when network
 * interface is ready.
 *
 * @return 0 on success, negative errno on failure
 */
int web_server_setup(void);

/**
 * @brief Initialize and start the web server
 *
 * This function is called automatically when network is ready,
 * or can be called manually if needed.
 *
 * @return 0 on success, negative errno on failure
 */
int web_server_init(void);

#endif /* WEB_SERVER_H */
