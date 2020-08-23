/** @file
 * @brief Modem GSM PPP header file.
 *
 * Generic modem ppp initialization, restart
 */

/*
 * Copyright (c) 2020-2021
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_GSM_PPP_H_
#define ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_GSM_PPP_H_

#ifdef __cplusplus
extern "C" {
#endif

void gsm_ppp_restart(void);

#ifdef __cplusplus
}
#endif


#endif /* ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_GSM_PPP_H_ */
