/**
 * Copyright (c) 2021 Golden Bits Software, Inc.
 *
 * @file auth_lib.h
 *
 * @brief  Authentication library functions
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_AUTH_LIB_H_
#define ZEPHYR_INCLUDE_AUTH_LIB_H_


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Authentication API
 * @defgroup zauth_api Authentication API
 * @ingroup zauth
 * @{
 */

#include <sys/util.h>

/*
 *  Determine number of auth instances.
 */
#if (CONFIG_NUM_AUTH_INSTANCES == 0)
#error Error at least one Authentication instance must be defined.
#endif

#if (CONFIG_NUM_AUTH_INSTANCES > 0)
#define AUTH_INSTANCE_1
#endif

#if (CONFIG_NUM_AUTH_INSTANCES > 1)
#define AUTH_INSTANCE_2
#endif


/**
 * Auth instance id.  Either AUTH_INST_1_ID or AUTH_INST_2_ID
 * Each instance performs authentication over a given hardware transport
 * such as Bluetooth or serial.  It is possible to configure the authentication
 *  library to authenticate over BLE and Serial.  Or if the device
 *  is a Bluetooth central, then two instances can be used to authenticate
 *  two different peripherals.
 */
enum auth_instance_id {

#if defined(AUTH_INSTANCE_1)
	/** Auth instance 1 */
	AUTH_INST_1_ID = 0,
#endif

#if defined(AUTH_INSTANCE_2)
	/** Auth instance 2 */
	AUTH_INST_2_ID = 1,
#endif

	AUTH_MAX_INSTANCES
};


#include <auth/auth_xport.h>

/** Authentication success */
#define AUTH_SUCCESS                        0
/** Auth error base value */
#define AUTH_ERROR_BASE                     (-200)
/** Invalid param passed to function */
#define AUTH_ERROR_INVALID_PARAM            (AUTH_ERROR_BASE - 1)
/** Out of memory */
#define AUTH_ERROR_NO_MEMORY                (AUTH_ERROR_BASE - 2)
/** Operation timeout */
#define AUTH_ERROR_TIMEOUT                  (AUTH_ERROR_BASE - 3)
/** No resource such as buffers or DTLS context buffer */
#define AUTH_ERROR_NO_RESOURCE              (AUTH_ERROR_BASE - 4)
/** DTLS initialization failed. */
#define AUTH_ERROR_DTLS_INIT_FAILED         (AUTH_ERROR_BASE - 5)
/** One of the Rx or Tx buffers are full */
#define AUTH_ERROR_IOBUFF_FULL              (AUTH_ERROR_BASE - 6)
/** An unexpected error, such as a Bluetooth handle unexpectedly missing. */
#define AUTH_ERROR_INTERNAL                 (AUTH_ERROR_BASE - 7)
/** The lower transport failed to send */
#define AUTH_ERROR_XPORT_SEND               (AUTH_ERROR_BASE - 8)
/** A framing error occurred when re-assembling a message. */
#define AUTH_ERROR_XPORT_FRAME              (AUTH_ERROR_BASE - 9)
/** An error occurred when performming a crypto operation */
#define AUTH_ERROR_CRYPTO                   (AUTH_ERROR_BASE - 10)
/** The authentication check failed. */
#define AUTH_ERROR_FAILED                   (AUTH_ERROR_BASE - 11)
/** The authentication was canceled*/
#define AUTH_ERROR_CANCELED                 (AUTH_ERROR_BASE - 12)


/*
 * Flags used when initializing authentication connection
 */
enum auth_flags {
	/** Server role */
	AUTH_CONN_SERVER                = BIT(0),
	/** Client role */
	AUTH_CONN_CLIENT                = BIT(1),
	/** Use DTLS for authentication */
	AUTH_CONN_DTLS_AUTH_METHOD      = BIT(2),
	/** Use Challenge-Response for authentication */
	AUTH_CONN_CHALLENGE_AUTH_METHOD = BIT(3)
};


/**
 *  Authentication status enums
 */
enum auth_status {
	/** Authentication started */
	AUTH_STATUS_STARTED,
	/** Authentication in process */
	AUTH_STATUS_IN_PROCESS,
	/** Authentication has been canceled */
	AUTH_STATUS_CANCELED,
	/** An internal failure of some type */
	AUTH_STATUS_FAILED,
	/** Authentication failed */
	AUTH_STATUS_AUTHENTICATION_FAILED,
	/** Authentication successful */
	AUTH_STATUS_SUCCESSFUL
};



/* Forward declaration */
struct authenticate_conn;

/**
 * Authentication function prototype
 */
typedef void (*auth_instance_func_t)(struct authenticate_conn *);

/**
 * Authentication callback status function
 */
typedef void (*auth_status_cb_t)(struct authenticate_conn *auth_conn, enum auth_instance_id instance,
				 enum auth_status status, void *context);


/**
 * @brief  Used to manage one authentication instance with a peer.  It is possible
 *         to have multiple concurrent authentication instances. For example if
 *         a device is acting as a Central and Peripheral concurrently.
 */
struct authenticate_conn {
	bool is_client; /* True if client */

	/* lower transport opaque handle */
	auth_xport_hdl_t xport_hdl;

	/* current status of the authentication process */
	enum auth_status curr_status;

	/* The auth instance ID for this connection */
	enum auth_instance_id instance;

	/* status callback func */
	auth_status_cb_t status_cb;
	void *callback_context;

	/* Work queue used to return status. Important if authentication
	 * status changes/fails in an ISR context */
	struct k_work auth_status_work;


	/* authentication function, performs the actual authentication */
	auth_instance_func_t auth_func;

	/* cancel the authentication  */
	volatile bool cancel_auth;

	/* Pointer to internal details, do not touch!!! */
	void *internal_obj;
};


/**
 * Optional authentication parameters specific to a particular authentication method.
 * This is a "catch-all" for parameters such as certificates or device salts when
 * using an authentication method.  For example, a user password or PIN code would
 * be passed to a password based authentication method such as J-PAKE.
 */

/*
 * Optional parameter id
 */
enum auth_opt_param_id {
	/** Optional DTLS param */
	AUTH_DTLS_PARAM = 1,
	/** Optional Challenge-Response param */
	AUTH_CHALRESP_PARAM,
	/** Placeholder for future use */
	AUTH_PLACHOLDER_PARAM
};


/**
 * Structs specific to DTLS authentication
 */
struct auth_certificate_info {
	const uint8_t *cert;    /* Cert or cert chain in PEM format. */
	size_t cert_size;       /* Size of cert or chain */

	/* Optional private key for this cert, set to NULL if
	 * not used. */
	const uint8_t *priv_key;
	size_t priv_key_size;
};

/**
 * DTLS cert chain plus key info
 */
struct auth_dtls_certs {
	struct auth_certificate_info server_ca_chain_pem;
	struct auth_certificate_info device_cert_pem;
};

/**
 * Optional shared key vs. hard-coded key.  This shared key could come
 * from a secure element such as a Microchip ATECC608A or NXP SE050 device.
 */
struct auth_challenge_resp {
	const uint8_t *shared_key; /* 32 byte random nonce */
};

/**
 * Placeholder for future params
 */
struct auth_placeholder_opt {
	void *placeholder;
};


/**
 * Optional parameters specific to the authentication method used.
 */
struct auth_optional_param {
	enum auth_opt_param_id param_id;
	union opt_params {
		struct auth_dtls_certs dtls_certs;
		struct auth_challenge_resp chal_resp;

		/* For future use, additional optional params are added
		 * added here. */
		struct auth_placeholder_opt placeholder_opt;
	} param_body;
};


/**
 *  Initializes authentication library
 *
 * @param auth_conn    Authentication connection struct, initialized by this call.
 * @param instance     The instance ID.
 * @param status_func  Status function callback.
 * @param context      Optional context used in status callback. NULL if not used.
 * @param opt_params   Optional params specific to the authentication method selected.  NULL if not used.
 * @param auth_flags   Authentication flags.
 *
 * @return AUTH_SUCCESS on success else one of AUTH_ERROR_* values.
 */
int auth_lib_init(struct authenticate_conn *auth_conn, enum auth_instance_id instance,
		  auth_status_cb_t status_func, void *context, struct auth_optional_param *opt_params,
		  enum auth_flags auth_flags);


/**
 * Frees up any previously allocated resources.
 *
 * @param auth_conn  Pointer to Authentication connection struct.
 *
 * @return  AUTH_SUCCESS on success else one of AUTH_ERROR_* values.
 */
int auth_lib_deinit(struct authenticate_conn *auth_conn);


/**
 * Starts the authentication process
 *
 * @param auth_conn  Authentication connection struct.
 *
 * @return  AUTH_SUCCESS on success else one of AUTH_ERROR_* values.
 */
int auth_lib_start(struct authenticate_conn *auth_conn);

/**
 * Returns the current status of the authentication process.
 *
 * @param auth_conn  Authentication connection struct.
 *
 * @return One of AUTH_STATUS_*  values
 */
enum auth_status auth_lib_get_status(struct authenticate_conn *auth_conn);

/**
 * Helper routine to return string corresponding to status
 *
 * @param   status  Authentication status value.
 *
 * @return  Pointer to string representing the status.
 */
const char *auth_lib_getstatus_str(enum auth_status status);


/**
 * Cancels the authentication process.  Must wait until the AUTH_STATUS_CANCELED
 * status is returned.
 *
 * @param auth_conn  Authentication connection struct.
 *
 * @return One of AUTH_STATUS_* values
 */
int auth_lib_cancel(struct authenticate_conn *auth_conn);



/**
 * @}
 */


#ifdef __cplusplus
}
#endif


#endif /* ZEPHYR_INCLUDE_AUTH_LIB_H_ */
