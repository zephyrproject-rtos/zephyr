/** @file
 * @brief Modem socket header file.
 *
 * Generic modem driver heeader file
 */

/*
 * Copyright (c) 2020-2021
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_UBLOX_SARA_r4_H_
#define ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_UBLOX_SARA_r4_H_

#ifdef __cplusplus
extern "C" {
#endif
/* This function is called from the main application in order to
 * make the modem ready for application data when the modem is powered
 * off.
 */
void restart_modem_offload(void);

#ifdef __cplusplus
}
#endif


#endif /* ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_SOCKET_H_ */
