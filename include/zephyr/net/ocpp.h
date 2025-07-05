/*
 * Copyright (c) 2024 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file ocpp.h
 *
 * @defgroup ocpp_api OCPP library
 * @ingroup networking
 * @{
 * @brief OCPP Charge Point Implementation
 *
 * @note The implementation assumes Websocket module is enabled.
 *
 * @note By default the implementation uses OCPP version 1.6.
 */

#ifndef ZEPHYR_INCLUDE_NET_OCPP_H_
#define ZEPHYR_INCLUDE_NET_OCPP_H_

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Max length of string literals e.g idtag */
#define CISTR50	50

/**
 * @brief OCPP IdTag authorization status in result to ocpp request
 *        authorization
 */
enum ocpp_auth_status {
	OCPP_AUTH_INVALID,	/*< IdTag not valid */
	OCPP_AUTH_ACCEPTED,	/*< accepted, allowed to charge */
	OCPP_AUTH_BLOCKED,	/*< blocked to charge */
	OCPP_AUTH_EXPIRED,	/*< IdTag expired, not allowed to charge */
	OCPP_AUTH_CONCURRENT_TX /*< Parallel access of same IdTag */
};

enum ocpp_notify_reason {
	/** User must fill the current reading */
	OCPP_USR_GET_METER_VALUE,

	/** Process the start charging request as like idtag received from local
	 *  e.g authorize etc
	 */
	OCPP_USR_START_CHARGING,

	/** Process the stop charging sequence */
	OCPP_USR_STOP_CHARGING,

	/** Unlock mechanical connector of CP */
	OCPP_USR_UNLOCK_CONNECTOR,
};

/** @brief OCPP meter readings to be filled on user callback request from
 *         library
 */
enum ocpp_meter_measurand {
	OCPP_OMM_CURRENT_FROM_EV,		/*< current from EV, in A */
	OCPP_OMM_CURRENT_TO_EV,			/*< current to EV, in A */
	OCPP_OMM_CURRENT_MAX_OFFERED_TO_EV,	/*< maximum current offered to EV, in A */
	OCPP_OMM_ACTIVE_ENERGY_FROM_EV,		/*< active energy from EV, in Wh */
	OCPP_OMM_ACTIVE_ENERGY_TO_EV,		/*< active energy to EV, in Wh */
	OCPP_OMM_REACTIVE_ENERGY_FROM_EV,	/*< reactive energy from EV, in varh */
	OCPP_OMM_REACTIVE_ENERGY_TO_EV,		/*< reactive energy to EV, in varh */
	OCPP_OMM_ACTIVE_POWER_FROM_EV,		/*< active power from EV, in W */
	OCPP_OMM_ACTIVE_POWER_TO_EV,		/*< active power to EV, in W */
	OCPP_OMM_REACTIVE_POWER_FROM_EV,	/*< reactive power from EV, in var */
	OCPP_OMM_REACTIVE_POWER_TO_EV,		/*< reactive power to EV, in var */
	OCPP_OMM_POWERLINE_FREQ,		/*< powerline frequency, in Hz */
	OCPP_OMM_POWER_FACTOR,			/*< power factor of supply */
	OCPP_OMM_POWER_MAX_OFFERED_TO_EV,	/*< maximum power offered to EV, in W */
	OCPP_OMM_FAN_SPEED,			/*< fan speed, in rpm */
	OCPP_OMM_CHARGING_PERCENT,		/*< charging percentage */
	OCPP_OMM_TEMPERATURE,			/*< temperature inside charge point, in Celsius */
	OCPP_OMM_VOLTAGE_AC_RMS,		/*< AC RMS supply voltage, in V */

	OCPP_OMM_END
};

/** @brief OCPP user callback notification/request of input/output values
 *         union member should be accessed with enum value ocpp_notify_reason
 *         correspondingly.
 *         e.g. callback reason is OCPP_USR_GET_METER_VALUE, struct meter_val
 *              is valid
 */
union ocpp_io_value {

	struct {
		/** Input to user, requested connector_id or 0 - main meter */
		int id_con;

		/** Input to user, measurand */
		enum ocpp_meter_measurand mes;

		 /** To be filled by user, value as string */
		char val[CISTR50];
	} meter_val;

	struct {
		char idtag[CISTR50];	/**< Input to user */

		/** Input to user(optional). connector id -1 means invalid */
		int id_con;
	} start_charge;

	struct {
		int id_con;	/**< Input to user, to stop charging connector */
	} stop_charge;

	struct {
		int id_con;	/**< Input to user, to unlock connector id. */
	} unlock_con;
};

/** @brief Parameters for ocpp_init information about Charge Point (CP)
 *         all are string literal except num_of_con
 */
struct ocpp_cp_info {
	char *model;		/**< Charge Point (CP) model */
	char *vendor;		/**< CP vendor */

	int num_of_con;		/**< Max. number of connector supports */

	/** optional fields */
	char *sl_no;		/**< CP serial number */
	char *box_sl_no;	/**< Box serial number */
	char *fw_ver;		/**< Firmware version */
	char *iccid;		/**< ICC ID */
	char *imsi;		/**< IMSI */
	char *meter_sl_no;	/**< Main power meter serial number */
	char *meter_type;	/**< Main power meter type */
};

/** @brief Parameters for ocpp_init information about central system (CS) */
struct ocpp_cs_info {
	char *cs_ip;	/**< Central system IP address */
	char *ws_url;	/**< Websocket url exclude ipaddr & port */
	int port;	/**< Central system port number */
	sa_family_t sa_family; /** IP protocol family type 4/6 */
};

/** @brief Parameters opaque session handle for ocpp_* API */
typedef void *ocpp_session_handle_t;

/**
 * @brief Asynchronous event notification callback registered by the
 *        application. advised callback should not be hold for longer time
 *        to unblock the ocpp protocol stack/lib.
 *
 * @param[in] reason for callback invoked.
 * @param[in] io reffered corresponding to reason.
 * @param[in] user_data passed on ocpp_init.
 *
 * @return 0 or a negative error code (errno.h)
 */
typedef int (*ocpp_user_notify_callback_t)(enum ocpp_notify_reason reason,
					   union ocpp_io_value *io,
					   void *user_data);

/**
 * @brief OCPP library init.
 *
 * @param[in] cpi Charge Point information
 * @param[in] csi Central System information
 * @param[in] cb user register callback
 * @param[in] user_data same reference will be passed on callback
 *
 * @return 0 on success or a negative error code (errno.h) indicating reason of failure
 *
 * @note Must be called before any other ocpp API
 */
int ocpp_init(struct ocpp_cp_info *cpi,
	      struct ocpp_cs_info *csi,
	      ocpp_user_notify_callback_t cb,
	      void *user_data);

/**
 * @brief API to request a new Session
 *
 * @param[out] hndl a valid opaque handle
 *
 * @return 0 on success or a negative error code (errno.h) indicating reason of failure
 *
 * @note Each connector should open unique session after ocpp_init and
 * prior to anyother ocpp_* request message api
 */
int ocpp_session_open(ocpp_session_handle_t *hndl);

/**
 * @brief API to close a Session
 *
 * @param[in] hndl a handle received from session open
 */
void ocpp_session_close(ocpp_session_handle_t hndl);

/**
 * @brief Authorize request call to CS to get validity of idtag
 *
 * @param[in] hndl session handle
 * @param[in] idtag (string literal) to get authorize validity
 * @param[out] status authorization status
 * @param[in] timeout_ms in msec
 *
 * @return 0 on success or a negative error code (errno.h) indicating reason of failure
 */
int ocpp_authorize(ocpp_session_handle_t hndl,
		   char *idtag,
		   enum ocpp_auth_status *status,
		   uint32_t timeout_ms);

/**
 * @brief Notify transaction start to CS
 *
 * @param[in] hndl session handle
 * @param[in] Wh energy meter reading of this connector
 * @param[in] conn_id connector id should be > 0 and sequential number
 * @param[in] timeout_ms timeout in msec
 *
 * @return: 0 on success
 *	    EACCES - not authorized, should follow the stop charging process
 *	    a negative error code (errno.h) indicating reason of failure
 */
int ocpp_start_transaction(ocpp_session_handle_t hndl,
			   int Wh,
			   uint8_t conn_id,
			   uint32_t timeout_ms);

/**
 * @brief Notify transaction stopped to CS
 *
 * @param[in] hndl session handle
 * @param[in] Wh energy meter reading of this connector
 * @param[in] timeout_ms timeout in msec
 *
 * @return 0 on success or a negative error code (errno.h) indicating reason of failure
 */
int ocpp_stop_transaction(ocpp_session_handle_t hndl,
			  int Wh,
			  uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_OCPP_H_ */

/**@}  */
