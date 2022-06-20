/**
 * @brief SARA N310 modem public API header file.
 *
 * Allows an application to control the SARA N310.
 *
 * Copyright (c) 2022 Abel Sensors
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

/* Signal quality callback data */
struct n310_signal_quality {
  int rssi;
  int quality;
};

enum n310_network_state {
	N310_NOT_REGISTERED = 0,
	N310_REGISTERED_HOME_NETWORK,
	N310_SEARCHING,
	N310_REGISTRATION_DENIED,
	N310_UNKNOWN,
	N310_REGISTERED_ROAMING,
};

enum n310_jamming_state {
	N310_JAMMING_NOT_DETECTED = 0,
	N310_JAMMING_DETECTED,
	N310_JAMMING_UNKNOWN,
};

enum n310_antenna_load {
	N310_OPEN_CIRCUIT = -1,
	N310_SHORT_CIRCUIT = 0,
};

/*
 * +NVSETPM: Power mode setting
 * Defines how the module switches between different power modes
 * from PM0 to PM3.
 */
int n310_psm_set_mode(int psm_mode);

/*
 * +CSCLK: Low clock mode setting
 * Configures and reads the low clock mode.
 */
int n310_psm_set_csclk(int setting);

/*
 * +CPSMS: Power Saving Mode Setting
 * Controls the setting of the UEs Power Saving Mode (PSM) parameters.
 */
int n310_psm_config(int mode, char *periodic_TAU, char *active_time);

/*
 * +ULGASP: Last gasp configuration
 * Enables/disables and configures the last gasp feature.
 */
int n310_set_last_gasp(const char *text, const char *ip_addr,
					   int tx_cnt, int shutdown);

/*
 * Triggers the last gasp.
 */
int n310_send_last_gasp(void);

/* Reset modem */
int n310_modem_reset(void);

/* Get modem info */
char *n310_get_model(void);
char *n310_get_iccid(void);
char *n310_get_imei(void);
char *n310_get_manufacturer(void);
char *n310_get_revision(void);
char *n310_get_ip(void);
int n310_get_network_state(void);
int n310_get_jamming_state(void);
int n310_get_antenna_load(void);
int n310_get_signal_quality(struct n310_signal_quality *buffer);

#endif /* UBLOX_SARA_N310_H_ */
