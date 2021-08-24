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

/* PSM defines */
#define PM0 0
#define PM2 10
#define PM3 9
#define PSM_DISABLE 0
#define PSM_ENABLE 1
#define PSM_RESET 2

/* +NVSETPM */
int n310_psm_set_mode(int psm_mode);

/* +CSCLK */
int n310_psm_set_csclk(int setting);

/* +CPSMS */
int n310_psm_config(int mode, char *periodic_TAU, char *active_time);

/* reset modem */
int n310_modem_reset(void);

/* get modem info */
char *n310_get_model(void);
char *n310_get_iccid(void);
char *n310_get_imei(void);
char *n310_get_manufacturer(void);
char *n310_get_revision(void);
char *n310_get_ip(void);
int n310_get_network_state(void);

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
