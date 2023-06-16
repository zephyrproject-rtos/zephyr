/*
 * Copyright (c) 2023 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __OCPP_H
#define __OCPP_H

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>

#define CISTR50	50

typedef enum {
	AUTH_ACCEPTED,
	AUTH_BLOCKED,
	AUTH_EXPIRED,
	AUTH_INVALID,
	AUTH_CONCURRENT_TX
}auth_status_t;

typedef enum {
	/* user must fill the current reading */
	OCPP_USR_GET_METER_VALUE,

	/* start process as idtag received from local e.g authorize etc */
	OCPP_USR_START_CHARING,

	/* follow stop charging process */
	OCPP_USR_STOP_CHARING,

	/* unlock mechanical connector */
	OCPP_USR_UNLOCK_CONNECTOR,
}ocpp_notify_reason_t;

typedef enum {
	OMM_CURRENT_FROM_EV,		// in A
	OMM_CURRENT_TO_EV, 		// in A
	OMM_CURRENT_MAX_OFFERED_TO_EV,	// in A
	OMM_ACTIVE_ENERGY_FROM_EV,	// in Wh
	OMM_ACTIVE_ENERGY_TO_EV,	// in Wh
	OMM_REACTIVE_ENERGY_FROM_EV,	// in varh
	OMM_REACTIVE_ENERGY_TO_EV,	// in varh
	OMM_ACTIVE_POWER_FROM_EV,	// in W
	OMM_ACTIVE_POWER_TO_EV,		// in W
	OMM_REACTIVE_POWER_FROM_EV,	// in var
	OMM_REACTIVE_POWER_TO_EV,	// in var
	OMM_POWERLINE_FREQ,		//Hz
	OMM_POWER_FACTOR,
	OMM_POWER_MAX_OFFERED_TO_EV,	// in W
	OMM_FAN_SPEED,			// in rpm
	OMM_CHARGING_PERCENT,
	OMM_TEMPERATURE,		// in Celsius
	OMM_VOLTAGE_AC_RMS,		// in V

	OMM_END
}ocpp_meter_measurand_t;

typedef union {
	struct {
		int id_con; // requested connector_id or 0 - main meter
		ocpp_meter_measurand_t mes;
		char val[CISTR50]; // to be filled by user, value as string
	}meter_val;

	struct {
		char idtag[CISTR50]; // input to user
		int id_con; // input to user(optional). -1 means invalid
	}start_charge;

	struct {
		int id_con;
	}stop_charge;

	struct {
		int id_con;
	}unlock_con;
}ocpp_io_value_t;

typedef struct {
	char *model;
	char *vendor;

	int num_of_con; //number of connector supports

	/* optional */
	char *sl_no;
	char *box_sl_no;
	char *fw_ver;
	char *iccid;
	char *imsi;
	char *meter_sl_no; //main power
	char *meter_type; //main power
}ocpp_cp_info_t;

typedef struct {
	char *cs_ip; //central system ipaddr
	char *ws_url; //websocket url exclude ipaddr & port
	int port;
	sa_family_t sa_family;
}ocpp_cs_info_t;

typedef void *ocpp_session_handle_t;

/* io reffered corresponding to reason
 * adviced callback very short inorder to unblock the ocpp protocol stack/lib
 */
typedef int (*ocpp_user_notify_callback_t)(ocpp_notify_reason_t reason,
					   ocpp_io_value_t *io,
					   void *user_data);

/*
 * in: filled charge point informations
 * in: filled central system informations
 * in: user register callback
 * in: userdata passed on callback
 * result: 0 - on sucess, otherwise - fail
 */
int ocpp_init(ocpp_cp_info_t *cpi,
	      ocpp_cs_info_t *csi,
	      ocpp_user_notify_callback_t cb,
	      void *user_data);

/*
 * out: filled handl
 * result: 0 - on sucess, otherwise - fail
 * 
 * each connector should open unique session after ocpp_init and
 * prior to anyother ocpp_* request message api
 */
int ocpp_session_open(ocpp_session_handle_t *hndl);

/* in: handle recived from open session */
void ocpp_session_close(ocpp_session_handle_t hndl);

/*
 * in: session handle
 * in: idtag to authorize
 * in: timeout_ms - timeout in msec
 * out: auth status
 * result: 0 - success, otherwise - fail
 */
int ocpp_authorize(ocpp_session_handle_t hndl,
		   char *idtag,
		   auth_status_t *status,
		   uint32_t timeout_ms);

/*
 * in: session handle
 * in: Wh - Energy meter reading of connector
 * in: conn_id - connector id should be > 0 and sequential number
 * in: timeout_ms - timeout in msec
 * result: 0 - success, otherwise - fail and charging should be terminated
 */
int ocpp_start_transaction(ocpp_session_handle_t hndl,
			   int Wh,
			   uint8_t conn_id,
			   uint32_t timeout_ms);

/*
 * in: session handle
 * in: Wh - Energy meter reading of connector
 * in: timeout_ms - timeout in msec
 * result: 0 - success, otherwise - fail
 */
int ocpp_stop_transaction(ocpp_session_handle_t hndl,
			  int Wh,
			  uint32_t timeout_ms);

#endif /* __OCPP_H */
