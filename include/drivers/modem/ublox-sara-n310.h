/**
 * @brief SARA N310 modem public API header file.
 *
 * Allows an application to control the SARA N310.
 *
 * Copyright (c) 2021 Abel Sensors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UBLOX_SARA_N310_H_
#define UBLOX_SARA_N310_H_

/* NSOSTF specific flags */
#define RELEASE_AFTER_UPLINK 1
#define RELEASE_AFTER_FIRST_DOWNLINK 2

/* reset modem */
int n310_modem_reset(void);

/* get modem info */
char *n310_get_model(void);
char *n310_get_iccid(void);
char *n310_get_imei(void);
char *n310_get_manufacturer(void);
char *n310_get_revision(void);
char *n310_get_ip(void);
int n310_get_state(void);

/* network state */
enum n310_network_state {
	N310_NOT_REGISTERED = 0,
	N310_REGISTERED_HOME_NETWORK,
	N310_SEARCHING,
	N310_REGISTRATION_DENIED,
	N310_UNKNOWN,
	N310_REGISTERED_ROAMING,
};

#endif /* UBLOX_SARA_N310_H_ */
