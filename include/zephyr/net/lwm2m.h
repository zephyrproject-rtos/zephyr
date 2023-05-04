/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2017-2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file lwm2m.h
 *
 * @defgroup lwm2m_api LwM2M high-level API
 * @ingroup networking
 * @{
 * @brief LwM2M high-level API
 *
 * @details
 * LwM2M high-level interface is defined in this header.
 *
 * @note The implementation assumes UDP module is enabled.
 *
 * @note For more information refer to Technical Specification
 * OMA-TS-LightweightM2M_Core-V1_1_1-20190617-A
 */

#ifndef ZEPHYR_INCLUDE_NET_LWM2M_H_
#define ZEPHYR_INCLUDE_NET_LWM2M_H_

#include <time.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/mutex.h>
#include <zephyr/net/coap.h>
#include <zephyr/net/lwm2m_path.h>

/**
 * @brief LwM2M Objects managed by OMA for LwM2M tech specification. Objects in this range have IDs
 * from 0 to 1023.
 */

/* clang-format off */
#define LWM2M_OBJECT_SECURITY_ID                0
#define LWM2M_OBJECT_SERVER_ID                  1
#define LWM2M_OBJECT_ACCESS_CONTROL_ID          2
#define LWM2M_OBJECT_DEVICE_ID                  3
#define LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID 4
#define LWM2M_OBJECT_FIRMWARE_ID                5
#define LWM2M_OBJECT_LOCATION_ID                6
#define LWM2M_OBJECT_CONNECTIVITY_STATISTICS_ID 7
#define LWM2M_OBJECT_SOFTWARE_MANAGEMENT_ID     9
#define LWM2M_OBJECT_PORTFOLIO_ID               16
#define LWM2M_OBJECT_BINARYAPPDATACONTAINER_ID	19
#define LWM2M_OBJECT_EVENT_LOG_ID               20
#define LWM2M_OBJECT_GATEWAY_ID                 25
/* clang-format on */

/**
 * @brief LwM2M Objects produced by 3rd party Standards Development
 * Organizations.  Objects in this range have IDs from 2048 to 10240
 * Refer to the OMA LightweightM2M (LwM2M) Object and Resource Registry:
 * http://www.openmobilealliance.org/wp/OMNA/LwM2M/LwM2MRegistry.html
 */

/* clang-format off */
#define IPSO_OBJECT_GENERIC_SENSOR_ID       3300
#define IPSO_OBJECT_TEMP_SENSOR_ID          3303
#define IPSO_OBJECT_HUMIDITY_SENSOR_ID      3304
#define IPSO_OBJECT_LIGHT_CONTROL_ID        3311
#define IPSO_OBJECT_ACCELEROMETER_ID        3313
#define IPSO_OBJECT_VOLTAGE_SENSOR_ID       3316
#define IPSO_OBJECT_CURRENT_SENSOR_ID       3317
#define IPSO_OBJECT_PRESSURE_ID             3323
#define IPSO_OBJECT_BUZZER_ID               3338
#define IPSO_OBJECT_TIMER_ID                3340
#define IPSO_OBJECT_ONOFF_SWITCH_ID         3342
#define IPSO_OBJECT_PUSH_BUTTON_ID          3347
#define UCIFI_OBJECT_BATTERY_ID             3411
#define IPSO_OBJECT_FILLING_LEVEL_SENSOR_ID 3435
/* clang-format on */

typedef void (*lwm2m_socket_fault_cb_t)(int error);

struct lwm2m_obj_path {
	uint16_t obj_id;
	uint16_t obj_inst_id;
	uint16_t res_id;
	uint16_t res_inst_id;
	uint8_t  level;  /* 0/1/2/3/4 (4 = resource instance) */
};

/**
 * @brief Observe callback events
 */
enum lwm2m_observe_event {
	LWM2M_OBSERVE_EVENT_OBSERVER_ADDED,
	LWM2M_OBSERVE_EVENT_OBSERVER_REMOVED,
	LWM2M_OBSERVE_EVENT_NOTIFY_ACK,
	LWM2M_OBSERVE_EVENT_NOTIFY_TIMEOUT,
};

/**
 * @brief Observe callback indicating observer adds and deletes, and
 *	  notification ACKs and timeouts
 *
 * @param[in] event Observer add/delete or notification ack/timeout
 * @param[in] path LwM2M path
 * @param[in] user_data Pointer to user_data buffer, as provided in
 *			send_traceable_notification(). Used to determine for which
 *			data the ACKed/timed out notification was.
 */
typedef void (*lwm2m_observe_cb_t)(enum lwm2m_observe_event event, struct lwm2m_obj_path *path,
				   void *user_data);


struct lwm2m_ctx;
enum lwm2m_rd_client_event;
/**
 * @brief Asynchronous RD client event callback
 *
 * @param[in] ctx LwM2M context generating the event
 * @param[in] event LwM2M RD client event code
 */
typedef void (*lwm2m_ctx_event_cb_t)(struct lwm2m_ctx *ctx,
				     enum lwm2m_rd_client_event event);


/**
 * @brief LwM2M context structure to maintain information for a single
 * LwM2M connection.
 */
struct lwm2m_ctx {
	/** Destination address storage */
	struct sockaddr remote_addr;

	/** Private CoAP and networking structures + 1 is for RD Client own message */
	struct coap_pending pendings[CONFIG_LWM2M_ENGINE_MAX_PENDING + 1];
	struct coap_reply replies[CONFIG_LWM2M_ENGINE_MAX_REPLIES + 1];
	sys_slist_t pending_sends;
#if defined(CONFIG_LWM2M_QUEUE_MODE_ENABLED)
	sys_slist_t queued_messages;
#endif
	sys_slist_t observer;

	/** A pointer to currently processed request, for internal LwM2M engine
	 *  use. The underlying type is ``struct lwm2m_message``, but since it's
	 *  declared in a private header and not exposed to the application,
	 *  it's stored as a void pointer.
	 */
	void *processed_req;

#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
	/** TLS tag is set by client as a reference used when the
	 *  LwM2M engine calls tls_credential_(add|delete)
	 */
	int tls_tag;

	/** When MBEDTLS SNI is enabled socket must be set with destination
	 *  hostname server.
	 */
	char *desthostname;
	uint16_t desthostnamelen;
	bool hostname_verify;

	/** Client can set load_credentials function as a way of overriding
	 *  the default behavior of load_tls_credential() in lwm2m_engine.c
	 */
	int (*load_credentials)(struct lwm2m_ctx *client_ctx);
#endif
	/** Flag to indicate if context should use DTLS.
	 *  Enabled via the use of coaps:// protocol prefix in connection
	 *  information.
	 *  NOTE: requires CONFIG_LWM2M_DTLS_SUPPORT=y
	 */
	bool use_dtls;

	/**
	 * Flag to indicate that the socket connection is suspended.
	 * With queue mode, this will tell if there is a need to reconnect.
	 */
	bool connection_suspended;

#if defined(CONFIG_LWM2M_QUEUE_MODE_ENABLED)
	/**
	 * Flag to indicate that the client is buffering Notifications and Send messages.
	 * True value buffer Notifications and Send messages.
	 */
	bool buffer_client_messages;
#endif
	/** Current index of Security Object used for server credentials */
	int sec_obj_inst;

	/** Current index of Server Object used in this context. */
	int srv_obj_inst;

	/** Flag to enable BOOTSTRAP interface. See Section "Bootstrap Interface"
	 *  of LwM2M Technical Specification for more information.
	 */
	bool bootstrap_mode;

	/** Socket File Descriptor */
	int sock_fd;

	/** Socket fault callback. LwM2M processing thread will call this
	 *  callback in case of socket errors on receive.
	 */
	lwm2m_socket_fault_cb_t fault_cb;

	/** Callback for new or cancelled observations, and acknowledged or timed
	 *  out notifications.
	 */
	lwm2m_observe_cb_t observe_cb;

	lwm2m_ctx_event_cb_t event_cb;

	/** Validation buffer. Used as a temporary buffer to decode the resource
	 *  value before validation. On successful validation, its content is
	 *  copied into the actual resource buffer.
	 */
	uint8_t validate_buf[CONFIG_LWM2M_ENGINE_VALIDATION_BUFFER_SIZE];
};

/**
 * LwM2M Time series data structure
 */
struct lwm2m_time_series_elem {
	/* Cached data Unix timestamp */
	time_t t;
	union {
		uint8_t u8;
		uint16_t u16;
		uint32_t u32;
		uint64_t u64;
		int8_t i8;
		int16_t i16;
		int32_t i32;
		int64_t i64;
		time_t time;
		double f;
		bool b;
	};
};

/**
 * @brief Asynchronous callback to get a resource buffer and length.
 *
 * Prior to accessing the data buffer of a resource, the engine can
 * use this callback to get the buffer pointer and length instead
 * of using the resource's data buffer.
 *
 * The client or LwM2M objects can register a function of this type via:
 * lwm2m_engine_register_read_callback()
 * lwm2m_engine_register_pre_write_callback()
 *
 * @param[in] obj_inst_id Object instance ID generating the callback.
 * @param[in] res_id Resource ID generating the callback.
 * @param[in] res_inst_id Resource instance ID generating the callback
 *                        (typically 0 for non-multi instance resources).
 * @param[out] data_len Length of the data buffer.
 *
 * @return Callback returns a pointer to the data buffer or NULL for failure.
 */
typedef void *(*lwm2m_engine_get_data_cb_t)(uint16_t obj_inst_id,
					    uint16_t res_id,
					    uint16_t res_inst_id,
					    size_t *data_len);

/**
 * @brief Asynchronous callback when data has been set to a resource buffer.
 *
 * After changing the data of a resource buffer, the LwM2M engine can
 * make use of this callback to pass the data back to the client or LwM2M
 * objects.
 *
 * A function of this type can be registered via:
 * lwm2m_engine_register_validate_callback()
 * lwm2m_engine_register_post_write_callback()
 *
 * @param[in] obj_inst_id Object instance ID generating the callback.
 * @param[in] res_id Resource ID generating the callback.
 * @param[in] res_inst_id Resource instance ID generating the callback
 *                        (typically 0 for non-multi instance resources).
 * @param[in] data Pointer to data.
 * @param[in] data_len Length of the data.
 * @param[in] last_block Flag used during block transfer to indicate the last
 *                       block of data. For non-block transfers this is always
 *                       false.
 * @param[in] total_size Expected total size of data for a block transfer.
 *                       For non-block transfers this is 0.
 *
 * @return Callback returns a negative error code (errno.h) indicating
 *         reason of failure or 0 for success.
 */
typedef int (*lwm2m_engine_set_data_cb_t)(uint16_t obj_inst_id,
					  uint16_t res_id, uint16_t res_inst_id,
					  uint8_t *data, uint16_t data_len,
					  bool last_block, size_t total_size);

/**
 * @brief Asynchronous event notification callback.
 *
 * Various object instance and resource-based events in the LwM2M engine
 * can trigger a callback of this function type: object instance create,
 * and object instance delete.
 *
 * Register a function of this type via:
 * lwm2m_engine_register_create_callback()
 * lwm2m_engine_register_delete_callback()
 *
 * @param[in] obj_inst_id Object instance ID generating the callback.
 *
 * @return Callback returns a negative error code (errno.h) indicating
 *         reason of failure or 0 for success.
 */
typedef int (*lwm2m_engine_user_cb_t)(uint16_t obj_inst_id);

/**
 * @brief Asynchronous execute notification callback.
 *
 * Resource executes trigger a callback of this type.
 *
 * Register a function of this type via:
 * lwm2m_engine_register_exec_callback()
 *
 * @param[in] obj_inst_id Object instance ID generating the callback.
 * @param[in] args Pointer to execute arguments payload. (This can be
 *            NULL if no arguments are provided)
 * @param[in] args_len Length of argument payload in bytes.
 *
 * @return Callback returns a negative error code (errno.h) indicating
 *         reason of failure or 0 for success.
 */
typedef int (*lwm2m_engine_execute_cb_t)(uint16_t obj_inst_id,
					 uint8_t *args, uint16_t args_len);

/**
 * @brief Power source types used for the "Available Power Sources" resource of
 * the LwM2M Device object.
 */
#define LWM2M_DEVICE_PWR_SRC_TYPE_DC_POWER	0
#define LWM2M_DEVICE_PWR_SRC_TYPE_BAT_INT	1
#define LWM2M_DEVICE_PWR_SRC_TYPE_BAT_EXT	2
#define LWM2M_DEVICE_PWR_SRC_TYPE_UNUSED	3
#define LWM2M_DEVICE_PWR_SRC_TYPE_PWR_OVER_ETH	4
#define LWM2M_DEVICE_PWR_SRC_TYPE_USB		5
#define LWM2M_DEVICE_PWR_SRC_TYPE_AC_POWER	6
#define LWM2M_DEVICE_PWR_SRC_TYPE_SOLAR		7
#define LWM2M_DEVICE_PWR_SRC_TYPE_MAX		8

/**
 * @brief Error codes used for the "Error Code" resource of the LwM2M Device
 * object.  An LwM2M client can register one of the following error codes via
 * the lwm2m_device_add_err() function.
 */
#define LWM2M_DEVICE_ERROR_NONE			0
#define LWM2M_DEVICE_ERROR_LOW_POWER		1
#define LWM2M_DEVICE_ERROR_EXT_POWER_SUPPLY_OFF	2
#define LWM2M_DEVICE_ERROR_GPS_FAILURE		3
#define LWM2M_DEVICE_ERROR_LOW_SIGNAL_STRENGTH	4
#define LWM2M_DEVICE_ERROR_OUT_OF_MEMORY	5
#define LWM2M_DEVICE_ERROR_SMS_FAILURE		6
#define LWM2M_DEVICE_ERROR_NETWORK_FAILURE	7
#define LWM2M_DEVICE_ERROR_PERIPHERAL_FAILURE	8

/**
 * @brief Battery status codes used for the "Battery Status" resource (3/0/20)
 *        of the LwM2M Device object.  As the battery status changes, an LwM2M
 *        client can set one of the following codes via:
 *        lwm2m_engine_set_u8("3/0/20", [battery status])
 */
#define LWM2M_DEVICE_BATTERY_STATUS_NORMAL	0
#define LWM2M_DEVICE_BATTERY_STATUS_CHARGING	1
#define LWM2M_DEVICE_BATTERY_STATUS_CHARGE_COMP	2
#define LWM2M_DEVICE_BATTERY_STATUS_DAMAGED	3
#define LWM2M_DEVICE_BATTERY_STATUS_LOW		4
#define LWM2M_DEVICE_BATTERY_STATUS_NOT_INST	5
#define LWM2M_DEVICE_BATTERY_STATUS_UNKNOWN	6

/**
 * @brief Register a new error code with LwM2M Device object.
 *
 * @param[in] error_code New error code.
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_device_add_err(uint8_t error_code);


/**
 * @brief LWM2M Firmware Update object states
 *
 * An LwM2M client or the LwM2M Firmware Update object use the following codes
 * to represent the LwM2M Firmware Update state (5/0/3).
 */
#define STATE_IDLE		0
#define STATE_DOWNLOADING	1
#define STATE_DOWNLOADED	2
#define STATE_UPDATING		3

/**
 * @brief LWM2M Firmware Update object result codes
 *
 * After processing a firmware update, the client sets the result via one of
 * the following codes via lwm2m_engine_set_u8("5/0/5", [result code])
 */
#define RESULT_DEFAULT		0
#define RESULT_SUCCESS		1
#define RESULT_NO_STORAGE	2
#define RESULT_OUT_OF_MEM	3
#define RESULT_CONNECTION_LOST	4
#define RESULT_INTEGRITY_FAILED	5
#define RESULT_UNSUP_FW		6
#define RESULT_INVALID_URI	7
#define RESULT_UPDATE_FAILED	8
#define RESULT_UNSUP_PROTO	9

#if defined(CONFIG_LWM2M_FIRMWARE_UPDATE_OBJ_SUPPORT)
/**
 * @brief Set data callback for firmware block transfer.
 *
 * LwM2M clients use this function to register a callback for receiving the
 * block transfer data when performing a firmware update.
 *
 * @param[in] cb A callback function to receive the block transfer data
 */
void lwm2m_firmware_set_write_cb(lwm2m_engine_set_data_cb_t cb);

/**
 * @brief Get the data callback for firmware block transfer writes.
 *
 * @return A registered callback function to receive the block transfer data
 */
lwm2m_engine_set_data_cb_t lwm2m_firmware_get_write_cb(void);

/**
 * @brief Set data callback for firmware block transfer.
 *
 * LwM2M clients use this function to register a callback for receiving the
 * block transfer data when performing a firmware update.
 *
 * @param[in] obj_inst_id Object instance ID
 * @param[in] cb A callback function to receive the block transfer data
 */
void lwm2m_firmware_set_write_cb_inst(uint16_t obj_inst_id, lwm2m_engine_set_data_cb_t cb);

/**
 * @brief Get the data callback for firmware block transfer writes.
 *
 * @param[in] obj_inst_id Object instance ID
 * @return A registered callback function to receive the block transfer data
 */
lwm2m_engine_set_data_cb_t lwm2m_firmware_get_write_cb_inst(uint16_t obj_inst_id);

/**
 * @brief Set callback for firmware update cancel.
 *
 * LwM2M clients use this function to register a callback to perform actions
 * on firmware update cancel.
 *
 * @param[in] cb A callback function perform actions on firmware update cancel.
 */
void lwm2m_firmware_set_cancel_cb(lwm2m_engine_user_cb_t cb);

/**
 * @brief Get a callback for firmware update cancel.
 *
 * @return A registered callback function perform actions on firmware update cancel.
 */
lwm2m_engine_user_cb_t lwm2m_firmware_get_cancel_cb(void);

/**
 * @brief Set data callback for firmware update cancel.
 *
 * LwM2M clients use this function to register a callback to perform actions
 * on firmware update cancel.
 *
 * @param[in] obj_inst_id Object instance ID
 * @param[in] cb A callback function perform actions on firmware update cancel.
 */
void lwm2m_firmware_set_cancel_cb_inst(uint16_t obj_inst_id, lwm2m_engine_user_cb_t cb);

/**
 * @brief Get the callback for firmware update cancel.
 *
 * @param[in] obj_inst_id Object instance ID
 * @return A registered callback function perform actions on firmware update cancel.
 */
lwm2m_engine_user_cb_t lwm2m_firmware_get_cancel_cb_inst(uint16_t obj_inst_id);

#if defined(CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_SUPPORT)
/**
 * @brief Set data callback to handle firmware update execute events.
 *
 * LwM2M clients use this function to register a callback for receiving the
 * update resource "execute" operation on the LwM2M Firmware Update object.
 *
 * @param[in] cb A callback function to receive the execute event.
 */
void lwm2m_firmware_set_update_cb(lwm2m_engine_execute_cb_t cb);

/**
 * @brief Get the event callback for firmware update execute events.
 *
 * @return A registered callback function to receive the execute event.
 */
lwm2m_engine_execute_cb_t lwm2m_firmware_get_update_cb(void);

/**
 * @brief Set data callback to handle firmware update execute events.
 *
 * LwM2M clients use this function to register a callback for receiving the
 * update resource "execute" operation on the LwM2M Firmware Update object.
 *
 * @param[in] obj_inst_id Object instance ID
 * @param[in] cb A callback function to receive the execute event.
 */
void lwm2m_firmware_set_update_cb_inst(uint16_t obj_inst_id, lwm2m_engine_execute_cb_t cb);

/**
 * @brief Get the event callback for firmware update execute events.
 *
 * @param[in] obj_inst_id Object instance ID
 * @return A registered callback function to receive the execute event.
 */
lwm2m_engine_execute_cb_t lwm2m_firmware_get_update_cb_inst(uint16_t obj_inst_id);
#endif
#endif


#if defined(CONFIG_LWM2M_SWMGMT_OBJ_SUPPORT)

/**
 * @brief Set callback to handle software activation requests
 *
 * The callback will be executed when the LWM2M execute operation gets called
 * on the corresponding object's Activate resource instance.
 *
 * @param[in] obj_inst_id The instance number to set the callback for.
 * @param[in] cb A callback function to receive the execute event.
 *
 * @return 0 on success, otherwise a negative integer.
 */
int lwm2m_swmgmt_set_activate_cb(uint16_t obj_inst_id, lwm2m_engine_execute_cb_t cb);

/**
 * @brief Set callback to handle software deactivation requests
 *
 * The callback will be executed when the LWM2M execute operation gets called
 * on the corresponding object's Deactivate resource instance.
 *
 * @param[in] obj_inst_id The instance number to set the callback for.
 * @param[in] cb A callback function to receive the execute event.
 *
 * @return 0 on success, otherwise a negative integer.
 */
int lwm2m_swmgmt_set_deactivate_cb(uint16_t obj_inst_id, lwm2m_engine_execute_cb_t cb);

/**
 * @brief Set callback to handle software install requests
 *
 * The callback will be executed when the LWM2M execute operation gets called
 * on the corresponding object's Install resource instance.
 *
 * @param[in] obj_inst_id The instance number to set the callback for.
 * @param[in] cb A callback function to receive the execute event.
 *
 * @return 0 on success, otherwise a negative integer.
 */
int lwm2m_swmgmt_set_install_package_cb(uint16_t obj_inst_id, lwm2m_engine_execute_cb_t cb);

/**
 * @brief Set callback to handle software uninstall requests
 *
 * The callback will be executed when the LWM2M execute operation gets called
 * on the corresponding object's Uninstall resource instance.
 *
 * @param[in] obj_inst_id The instance number to set the callback for.
 * @param[in] cb A callback function for handling the execute event.
 *
 * @return 0 on success, otherwise a negative integer.
 */
int lwm2m_swmgmt_set_delete_package_cb(uint16_t obj_inst_id, lwm2m_engine_execute_cb_t cb);

/**
 * @brief Set callback to read software package
 *
 * The callback will be executed when the LWM2M read operation gets called
 * on the corresponding object.
 *
 * @param[in] obj_inst_id The instance number to set the callback for.
 * @param[in] cb A callback function for handling the read event.
 *
 * @return 0 on success, otherwise a negative integer.
 */
int lwm2m_swmgmt_set_read_package_version_cb(uint16_t obj_inst_id, lwm2m_engine_get_data_cb_t cb);

/**
 * @brief Set data callback for software management block transfer.
 *
 * The callback will be executed when the LWM2M block write operation gets called
 * on the corresponding object's resource instance.
 *
 * @param[in] obj_inst_id The instance number to set the callback for.
 * @param[in] cb A callback function for handling the block write event.
 *
 * @return 0 on success, otherwise a negative integer.
 */
int lwm2m_swmgmt_set_write_package_cb(uint16_t obj_inst_id, lwm2m_engine_set_data_cb_t cb);

/**
 * Function to be called when a Software Management object instance
 * completed the Install operation.
 *
 * @param[in] obj_inst_id The Software Management object instance
 * @param[in] error_code The result code of the operation. Zero on success
 * otherwise it should be a negative integer.
 *
 * return 0 on success, otherwise a negative integer.
 */
int lwm2m_swmgmt_install_completed(uint16_t obj_inst_id, int error_code);

#endif

#if defined(CONFIG_LWM2M_EVENT_LOG_OBJ_SUPPORT)

/**
 * @brief Set callback to read log data
 *
 * The callback will be executed when the LWM2M read operation gets called
 * on the corresponding object.
 *
 * @param[in] cb A callback function for handling the read event.
 */
void lwm2m_event_log_set_read_log_data_cb(lwm2m_engine_get_data_cb_t cb);

#endif

/**
 * @brief Maximum value for ObjLnk resource fields
 */
#define LWM2M_OBJLNK_MAX_ID USHRT_MAX

/**
 * @brief LWM2M ObjLnk resource type structure
 */
struct lwm2m_objlnk {
	uint16_t obj_id;
	uint16_t obj_inst;
};

/**
 * @brief Change an observer's pmin value.
 *
 * @deprecated Use lwm2m_update_observer_min_period() instead.
 *
 * LwM2M clients use this function to modify the pmin attribute
 * for an observation being made.
 * Example to update the pmin of a temperature sensor value being observed:
 * lwm2m_engine_update_observer_min_period("client_ctx, 3303/0/5700", 5);
 *
 * @param[in] client_ctx LwM2M context
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res"
 * @param[in] period_s Value of pmin to be given (in seconds).
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_update_observer_min_period(struct lwm2m_ctx *client_ctx, const char *pathstr,
					    uint32_t period_s);

/**
 * @brief Change an observer's pmin value.
 *
 * LwM2M clients use this function to modify the pmin attribute
 * for an observation being made.
 * Example to update the pmin of a temperature sensor value being observed:
 * lwm2m_update_observer_min_period(client_ctx, &LWM2M_OBJ(3303, 0, 5700), 5);
 *
 * @param[in] client_ctx LwM2M context
 * @param[in] path LwM2M path as a struct
 * @param[in] period_s Value of pmin to be given (in seconds).
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_update_observer_min_period(struct lwm2m_ctx *client_ctx,
				     const struct lwm2m_obj_path *path, uint32_t period_s);

/**
 * @brief Change an observer's pmax value.
 *
 * @deprecated Use lwm2m_update_observer_max_period() instead.
 *
 * LwM2M clients use this function to modify the pmax attribute
 * for an observation being made.
 * Example to update the pmax of a temperature sensor value being observed:
 * lwm2m_engine_update_observer_max_period("client_ctx, 3303/0/5700", 5);
 *
 * @param[in] client_ctx LwM2M context
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res"
 * @param[in] period_s Value of pmax to be given (in seconds).
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_update_observer_max_period(struct lwm2m_ctx *client_ctx, const char *pathstr,
					    uint32_t period_s);

/**
 * @brief Change an observer's pmax value.
 *
 * LwM2M clients use this function to modify the pmax attribute
 * for an observation being made.
 * Example to update the pmax of a temperature sensor value being observed:
 * lwm2m__update_observer_max_period(client_ctx, &LWM2M_OBJ(3303, 0, 5700), 5);
 *
 * @param[in] client_ctx LwM2M context
 * @param[in] path LwM2M path as a struct
 * @param[in] period_s Value of pmax to be given (in seconds).
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_update_observer_max_period(struct lwm2m_ctx *client_ctx,
				     const struct lwm2m_obj_path *path, uint32_t period_s);

/**
 * @brief Create an LwM2M object instance.
 *
 * @deprecated Use lwm2m_create_obj_inst() instead.
 *
 * LwM2M clients use this function to create non-default LwM2M objects:
 * Example to create first temperature sensor object:
 * lwm2m_engine_create_obj_inst("3303/0");
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst"
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_create_obj_inst(const char *pathstr);

/**
 * @brief Create an LwM2M object instance.
 *
 * LwM2M clients use this function to create non-default LwM2M objects:
 * Example to create first temperature sensor object:
 * lwm2m_create_obj_inst(&LWM2M_OBJ(3303, 0));
 *
 * @param[in] path LwM2M path as a struct
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_create_object_inst(const struct lwm2m_obj_path *path);

/**
 * @brief Delete an LwM2M object instance.
 *
 * @deprecated Use lwm2m_delete_obj_inst() instead.
 *
 * LwM2M clients use this function to delete LwM2M objects.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst"
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_delete_obj_inst(const char *pathstr);

/**
 * @brief Delete an LwM2M object instance.
 *
 * LwM2M clients use this function to delete LwM2M objects.
 *
 * @param[in] path LwM2M path as a struct
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_delete_object_inst(const struct lwm2m_obj_path *path);

/**
 * @brief Locks the registry for this thread.
 *
 * Use this function before writing to multiple resources. This halts the
 * lwm2m main thread until all the write-operations are finished.
 *
 */
void lwm2m_registry_lock(void);

/**
 * @brief Unlocks the registry previously locked by lwm2m_registry_lock().
 *
 */
void lwm2m_registry_unlock(void);

/**
 * @brief Set resource (instance) value (opaque buffer)
 *
 * @deprecated Use lwm2m_set_opaque() instead.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[in] data_ptr Data buffer
 * @param[in] data_len Length of buffer
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_set_opaque(const char *pathstr, const char *data_ptr, uint16_t data_len);

/**
 * @brief Set resource (instance) value (opaque buffer)
 *
 * @param[in] path LwM2M path as a struct
 * @param[in] data_ptr Data buffer
 * @param[in] data_len Length of buffer
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_set_opaque(const struct lwm2m_obj_path *path, const char *data_ptr, uint16_t data_len);

/**
 * @brief Set resource (instance) value (string)
 *
 * @deprecated Use lwm2m_set_string() instead.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[in] data_ptr NULL terminated char buffer
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_set_string(const char *pathstr, const char *data_ptr);

/**
 * @brief Set resource (instance) value (string)
 *
 * @param[in] path LwM2M path as a struct
 * @param[in] data_ptr NULL terminated char buffer
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_set_string(const struct lwm2m_obj_path *path, const char *data_ptr);

/**
 * @brief Set resource (instance) value (u8)
 *
 * @deprecated Use lwm2m_set_u8() instead.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[in] value u8 value
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_set_u8(const char *pathstr, uint8_t value);

/**
 * @brief Set resource (instance) value (u8)
 *
 * @param[in] path LwM2M path as a struct
 * @param[in] value u8 value
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_set_u8(const struct lwm2m_obj_path *path, uint8_t value);

/**
 * @brief Set resource (instance) value (u16)
 *
 * @deprecated Use lwm2m_set_u16() instead.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[in] value u16 value
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_set_u16(const char *pathstr, uint16_t value);

/**
 * @brief Set resource (instance) value (u16)
 *
 * @param[in] path LwM2M path as a struct
 * @param[in] value u16 value
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_set_u16(const struct lwm2m_obj_path *path, uint16_t value);

/**
 * @brief Set resource (instance) value (u32)
 *
 * @deprecated Use lwm2m_set_u32() instead.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[in] value u32 value
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_set_u32(const char *pathstr, uint32_t value);

/**
 * @brief Set resource (instance) value (u32)
 *
 * @param[in] path LwM2M path as a struct
 * @param[in] value u32 value
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_set_u32(const struct lwm2m_obj_path *path, uint32_t value);

/**
 * @brief Set resource (instance) value (u64)
 *
 * @deprecated Use lwm2m_set_u64() instead.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[in] value u64 value
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_set_u64(const char *pathstr, uint64_t value);

/**
 * @brief Set resource (instance) value (u64)
 *
 * @param[in] path LwM2M path as a struct
 * @param[in] value u64 value
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_set_u64(const struct lwm2m_obj_path *path, uint64_t value);

/**
 * @brief Set resource (instance) value (s8)
 *
 * @deprecated Use lwm2m_set_s8() instead.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[in] value s8 value
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_set_s8(const char *pathstr, int8_t value);

/**
 * @brief Set resource (instance) value (s8)
 *
 * @param[in] path LwM2M path as a struct
 * @param[in] value s8 value
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_set_s8(const struct lwm2m_obj_path *path, int8_t value);

/**
 * @brief Set resource (instance) value (s16)
 *
 * @deprecated Use lwm2m_set_s16() instead.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[in] value s16 value
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_set_s16(const char *pathstr, int16_t value);

/**
 * @brief Set resource (instance) value (s16)
 *
 * @param[in] path LwM2M path as a struct
 * @param[in] value s16 value
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_set_s16(const struct lwm2m_obj_path *path, int16_t value);

/**
 * @brief Set resource (instance) value (s32)
 *
 * @deprecated Use lwm2m_set_s32() instead.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[in] value s32 value
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_set_s32(const char *pathstr, int32_t value);

/**
 * @brief Set resource (instance) value (s32)
 *
 * @param[in] path LwM2M path as a struct
 * @param[in] value s32 value
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_set_s32(const struct lwm2m_obj_path *path, int32_t value);

/**
 * @brief Set resource (instance) value (s64)
 *
 * @deprecated Use lwm2m_set_s64() instead.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[in] value s64 value
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_set_s64(const char *pathstr, int64_t value);

/**
 * @brief Set resource (instance) value (s64)
 *
 * @param[in] path LwM2M path as a struct
 * @param[in] value s64 value
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_set_s64(const struct lwm2m_obj_path *path, int64_t value);

/**
 * @brief Set resource (instance) value (bool)
 *
 * @deprecated Use lwm2m_set_bool() instead.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[in] value bool value
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_set_bool(const char *pathstr, bool value);

/**
 * @brief Set resource (instance) value (bool)
 *
 * @param[in] path LwM2M path as a struct
 * @param[in] value bool value
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_set_bool(const struct lwm2m_obj_path *path, bool value);

/**
 * @brief Set resource (instance) value (double)
 *
 * @deprecated Use lwm2m_set_f64() instead.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[in] value double value
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_set_float(const char *pathstr, const double *value);

/**
 * @brief Set resource (instance) value (double)
 *
 * @param[in] path LwM2M path as a struct
 * @param[in] value double value
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_set_f64(const struct lwm2m_obj_path *path, const double value);

/**
 * @brief Set resource (instance) value (ObjLnk)
 *
 * @deprecated Use lwm2m_set_objlnk() instead.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[in] value pointer to the lwm2m_objlnk structure
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_set_objlnk(const char *pathstr, const struct lwm2m_objlnk *value);

/**
 * @brief Set resource (instance) value (ObjLnk)
 *
 * @param[in] path LwM2M path as a struct
 * @param[in] value pointer to the lwm2m_objlnk structure
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_set_objlnk(const struct lwm2m_obj_path *path, const struct lwm2m_objlnk *value);

/**
 * @brief Set resource (instance) value (Time)
 *
 * @deprecated Use lwm2m_set_time() instead.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[in] value Epoch timestamp
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_set_time(const char *pathstr, time_t value);

/**
 * @brief Set resource (instance) value (Time)
 *
 * @param[in] path LwM2M path as a struct
 * @param[in] value Epoch timestamp
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_set_time(const struct lwm2m_obj_path *path, time_t value);

/**
 * @brief Get resource (instance) value (opaque buffer)
 *
 * @deprecated Use lwm2m_get_opaque() instead.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[out] buf Data buffer to copy data into
 * @param[in] buflen Length of buffer
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_get_opaque(const char *pathstr, void *buf, uint16_t buflen);

/**
 * @brief Get resource (instance) value (opaque buffer)
 *
 * @param[in] path LwM2M path as a struct
 * @param[out] buf Data buffer to copy data into
 * @param[in] buflen Length of buffer
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_get_opaque(const struct lwm2m_obj_path *path, void *buf, uint16_t buflen);

/**
 * @brief Get resource (instance) value (string)
 *
 * @deprecated Use lwm2m_get_string() instead.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[out] str String buffer to copy data into
 * @param[in] strlen Length of buffer
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_get_string(const char *pathstr, void *str, uint16_t strlen);

/**
 * @brief Get resource (instance) value (string)
 *
 * @param[in] path LwM2M path as a struct
 * @param[out] str String buffer to copy data into
 * @param[in] strlen Length of buffer
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_get_string(const struct lwm2m_obj_path *path, void *str, uint16_t strlen);

/**
 * @brief Get resource (instance) value (u8)
 *
 * @deprecated Use lwm2m_get_u8() instead.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[out] value u8 buffer to copy data into
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_get_u8(const char *pathstr, uint8_t *value);

/**
 * @brief Get resource (instance) value (u8)
 *
 * @param[in] path LwM2M path as a struct
 * @param[out] value u8 buffer to copy data into
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_get_u8(const struct lwm2m_obj_path *path, uint8_t *value);

/**
 * @brief Get resource (instance) value (u16)
 *
 * @deprecated Use lwm2m_get_u16() instead.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[out] value u16 buffer to copy data into
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_get_u16(const char *pathstr, uint16_t *value);

/**
 * @brief Get resource (instance) value (u16)
 *
 * @param[in] path LwM2M path as a struct
 * @param[out] value u16 buffer to copy data into
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_get_u16(const struct lwm2m_obj_path *path, uint16_t *value);

/**
 * @brief Get resource (instance) value (u32)
 *
 * @deprecated Use lwm2m_get_u32() instead.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[out] value u32 buffer to copy data into
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_get_u32(const char *pathstr, uint32_t *value);

/**
 * @brief Get resource (instance) value (u32)
 *
 * @param[in] path LwM2M path as a struct
 * @param[out] value u32 buffer to copy data into
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_get_u32(const struct lwm2m_obj_path *path, uint32_t *value);

/**
 * @brief Get resource (instance) value (u64)
 *
 * @deprecated Use lwm2m_get_u64() instead.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[out] value u64 buffer to copy data into
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_get_u64(const char *pathstr, uint64_t *value);

/**
 * @brief Get resource (instance) value (u64)
 *
 * @param[in] path LwM2M path as a struct
 * @param[out] value u64 buffer to copy data into
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_get_u64(const struct lwm2m_obj_path *path, uint64_t *value);

/**
 * @brief Get resource (instance) value (s8)
 *
 * @deprecated Use lwm2m_get_s8() instead.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[out] value s8 buffer to copy data into
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_get_s8(const char *pathstr, int8_t *value);

/**
 * @brief Get resource (instance) value (s8)
 *
 * @param[in] path LwM2M path as a struct
 * @param[out] value s8 buffer to copy data into
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_get_s8(const struct lwm2m_obj_path *path, int8_t *value);

/**
 * @brief Get resource (instance) value (s16)
 *
 * @deprecated Use lwm2m_get_s16() instead.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[out] value s16 buffer to copy data into
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_get_s16(const char *pathstr, int16_t *value);

/**
 * @brief Get resource (instance) value (s16)
 *
 * @param[in] path LwM2M path as a struct
 * @param[out] value s16 buffer to copy data into
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_get_s16(const struct lwm2m_obj_path *path, int16_t *value);

/**
 * @brief Get resource (instance) value (s32)
 *
 * @deprecated Use lwm2m_get_s32() instead.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[out] value s32 buffer to copy data into
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_get_s32(const char *pathstr, int32_t *value);

/**
 * @brief Get resource (instance) value (s32)
 *
 * @param[in] path LwM2M path as a struct
 * @param[out] value s32 buffer to copy data into
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_get_s32(const struct lwm2m_obj_path *path, int32_t *value);

/**
 * @brief Get resource (instance) value (s64)
 *
 * @deprecated Use lwm2m_get_s64() instead.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[out] value s64 buffer to copy data into
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_get_s64(const char *pathstr, int64_t *value);

/**
 * @brief Get resource (instance) value (s64)
 *
 * @param[in] path LwM2M path as a struct
 * @param[out] value s64 buffer to copy data into
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_get_s64(const struct lwm2m_obj_path *path, int64_t *value);

/**
 * @brief Get resource (instance) value (bool)
 *
 * @deprecated Use lwm2m_get_bool() instead.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[out] value bool buffer to copy data into
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_get_bool(const char *pathstr, bool *value);

/**
 * @brief Get resource (instance) value (bool)
 *
 * @param[in] path LwM2M path as a struct
 * @param[out] value bool buffer to copy data into
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_get_bool(const struct lwm2m_obj_path *path, bool *value);

/**
 * @brief Get resource (instance) value (double)
 *
 * @deprecated Use lwm2m_get_f64() instead.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[out] buf double buffer to copy data into
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_get_float(const char *pathstr, double *buf);

/**
 * @brief Get resource (instance) value (double)
 *
 * @param[in] path LwM2M path as a struct
 * @param[out] value double buffer to copy data into
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_get_f64(const struct lwm2m_obj_path *path, double *value);

/**
 * @brief Get resource (instance) value (ObjLnk)
 *
 * @deprecated Use lwm2m_get_objlnk() instead.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[out] buf lwm2m_objlnk buffer to copy data into
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_get_objlnk(const char *pathstr, struct lwm2m_objlnk *buf);

/**
 * @brief Get resource (instance) value (ObjLnk)
 *
 * @param[in] path LwM2M path as a struct
 * @param[out] buf lwm2m_objlnk buffer to copy data into
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_get_objlnk(const struct lwm2m_obj_path *path, struct lwm2m_objlnk *buf);

/**
 * @brief Get resource (instance) value (Time)
 *
 * @deprecated Use lwm2m_get_time() instead.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[out] buf time_t pointer to copy data
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_get_time(const char *pathstr, time_t *buf);

/**
 * @brief Get resource (instance) value (Time)
 *
 * @param[in] path LwM2M path as a struct
 * @param[out] buf time_t pointer to copy data
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_get_time(const struct lwm2m_obj_path *path, time_t *buf);

/**
 * @brief Set resource (instance) read callback
 *
 * @deprecated Use lwm2m_register_read_callback() instead.
 *
 * LwM2M clients can use this to set the callback function for resource reads when data
 * handling in the LwM2M engine needs to be bypassed.
 * For example reading back opaque binary data from external storage.
 *
 * This callback should not generally be used for any data that might be observed as
 * engine does not have any knowledge of data changes.
 *
 * When separate buffer for data should be used, use lwm2m_engine_set_res_buf() instead
 * to set the storage.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[in] cb Read resource callback
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_register_read_callback(const char *pathstr,
					lwm2m_engine_get_data_cb_t cb);

/**
 * @brief Set resource (instance) read callback
 *
 * LwM2M clients can use this to set the callback function for resource reads when data
 * handling in the LwM2M engine needs to be bypassed.
 * For example reading back opaque binary data from external storage.
 *
 * This callback should not generally be used for any data that might be observed as
 * engine does not have any knowledge of data changes.
 *
 * When separate buffer for data should be used, use lwm2m_engine_set_res_buf() instead
 * to set the storage.
 *
 * @param[in] path LwM2M path as a struct
 * @param[in] cb Read resource callback
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_register_read_callback(const struct lwm2m_obj_path *path, lwm2m_engine_get_data_cb_t cb);

/**
 * @brief Set resource (instance) pre-write callback
 *
 * @deprecated Use lwm2m_register_pre_write_callback() instead.
 *
 * This callback is triggered before setting the value of a resource.  It
 * can pass a special data buffer to the engine so that the actual resource
 * value can be calculated later, etc.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[in] cb Pre-write resource callback
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_register_pre_write_callback(const char *pathstr,
					     lwm2m_engine_get_data_cb_t cb);

/**
 * @brief Set resource (instance) pre-write callback
 *
 * This callback is triggered before setting the value of a resource.  It
 * can pass a special data buffer to the engine so that the actual resource
 * value can be calculated later, etc.
 *
 * @param[in] path LwM2M path as a struct
 * @param[in] cb Pre-write resource callback
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_register_pre_write_callback(const struct lwm2m_obj_path *path,
				      lwm2m_engine_get_data_cb_t cb);

/**
 * @brief Set resource (instance) validation callback
 *
 * @deprecated Use lwm2m_register_validate_callback() instead.
 *
 * This callback is triggered before setting the value of a resource to the
 * resource data buffer.
 *
 * The callback allows an LwM2M client or object to validate the data before
 * writing and notify an error if the data should be discarded for any reason
 * (by returning a negative error code).
 *
 * @note All resources that have a validation callback registered are initially
 *       decoded into a temporary validation buffer. Make sure that
 *       ``CONFIG_LWM2M_ENGINE_VALIDATION_BUFFER_SIZE`` is large enough to
 *       store each of the validated resources (individually).
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[in] cb Validate resource data callback
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_register_validate_callback(const char *pathstr,
					    lwm2m_engine_set_data_cb_t cb);

/**
 * @brief Set resource (instance) validation callback
 *
 * This callback is triggered before setting the value of a resource to the
 * resource data buffer.
 *
 * The callback allows an LwM2M client or object to validate the data before
 * writing and notify an error if the data should be discarded for any reason
 * (by returning a negative error code).
 *
 * @note All resources that have a validation callback registered are initially
 *       decoded into a temporary validation buffer. Make sure that
 *       ``CONFIG_LWM2M_ENGINE_VALIDATION_BUFFER_SIZE`` is large enough to
 *       store each of the validated resources (individually).
 *
 * @param[in] path LwM2M path as a struct
 * @param[in] cb Validate resource data callback
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_register_validate_callback(const struct lwm2m_obj_path *path,
				     lwm2m_engine_set_data_cb_t cb);

/**
 * @brief Set resource (instance) post-write callback
 *
 * @deprecated Use lwm2m_register_post_write_callback() instead.
 *
 * This callback is triggered after setting the value of a resource to the
 * resource data buffer.
 *
 * It allows an LwM2M client or object to post-process the value of a resource
 * or trigger other related resource calculations.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[in] cb Post-write resource callback
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_register_post_write_callback(const char *pathstr,
					      lwm2m_engine_set_data_cb_t cb);

/**
 * @brief Set resource (instance) post-write callback
 *
 * This callback is triggered after setting the value of a resource to the
 * resource data buffer.
 *
 * It allows an LwM2M client or object to post-process the value of a resource
 * or trigger other related resource calculations.
 *
 * @param[in] path LwM2M path as a struct
 * @param[in] cb Post-write resource callback
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_register_post_write_callback(const struct lwm2m_obj_path *path,
				       lwm2m_engine_set_data_cb_t cb);

/**
 * @brief Set resource execute event callback
 *
 * @deprecated Use lwm2m_register_exec_callback() instead.
 *
 * This event is triggered when the execute method of a resource is enabled.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res"
 * @param[in] cb Execute resource callback
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_register_exec_callback(const char *pathstr,
					lwm2m_engine_execute_cb_t cb);

/**
 * @brief Set resource execute event callback
 *
 * This event is triggered when the execute method of a resource is enabled.
 *
 * @param[in] path LwM2M path as a struct
 * @param[in] cb Execute resource callback
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_register_exec_callback(const struct lwm2m_obj_path *path, lwm2m_engine_execute_cb_t cb);

/**
 * @brief Set object instance create event callback
 *
 * @deprecated Use lwm2m_register_create_callback instead.
 *
 * This event is triggered when an object instance is created.
 *
 * @param[in] obj_id LwM2M object id
 * @param[in] cb Create object instance callback
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_register_create_callback(uint16_t obj_id,
					  lwm2m_engine_user_cb_t cb);

/**
 * @brief Set object instance create event callback
 *
 * This event is triggered when an object instance is created.
 *
 * @param[in] obj_id LwM2M object id
 * @param[in] cb Create object instance callback
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_register_create_callback(uint16_t obj_id,
				   lwm2m_engine_user_cb_t cb);

/**
 * @brief Set object instance delete event callback
 *
 * @deprecated Use lwm2m_register_delete_callback instead
 *
 * This event is triggered when an object instance is deleted.
 *
 * @param[in] obj_id LwM2M object id
 * @param[in] cb Delete object instance callback
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_register_delete_callback(uint16_t obj_id,
					  lwm2m_engine_user_cb_t cb);

/**
 * @brief Set object instance delete event callback
 *
 * This event is triggered when an object instance is deleted.
 *
 * @param[in] obj_id LwM2M object id
 * @param[in] cb Delete object instance callback
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_register_delete_callback(uint16_t obj_id,
					  lwm2m_engine_user_cb_t cb);

/**
 * @brief Resource read-only value bit
 */
#define LWM2M_RES_DATA_READ_ONLY	0

/**
 * @brief Resource read-only flag
 */
#define LWM2M_RES_DATA_FLAG_RO		BIT(LWM2M_RES_DATA_READ_ONLY)

/**
 * @brief Read resource flags helper macro
 */
#define LWM2M_HAS_RES_FLAG(res, f)	((res->data_flags & f) == f)

/**
 * @brief Set data buffer for a resource
 *
 * @deprecated Use lwm2m_set_res_buf() instead.
 *
 * Use this function to set the data buffer and flags for the specified LwM2M
 * resource.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[in] buffer_ptr Data buffer pointer
 * @param[in] buffer_len Length of buffer
 * @param[in] data_len Length of existing data in the buffer
 * @param[in] data_flags Data buffer flags (such as read-only, etc)
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_set_res_buf(const char *pathstr, void *buffer_ptr, uint16_t buffer_len,
			     uint16_t data_len, uint8_t data_flags);

/**
 * @brief Set data buffer for a resource
 *
 * Use this function to set the data buffer and flags for the specified LwM2M
 * resource.
 *
 * @param[in] path LwM2M path as a struct
 * @param[in] buffer_ptr Data buffer pointer
 * @param[in] buffer_len Length of buffer
 * @param[in] data_len Length of existing data in the buffer
 * @param[in] data_flags Data buffer flags (such as read-only, etc)
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_set_res_buf(const struct lwm2m_obj_path *path, void *buffer_ptr, uint16_t buffer_len,
		      uint16_t data_len, uint8_t data_flags);

/**
 * @brief Set data buffer for a resource
 *
 * Use this function to set the data buffer and flags for the specified LwM2M
 * resource.
 *
 * @deprecated Use lwm2m_set_res_buf() instead, so you can define buffer size and data size
 *             separately.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[in] data_ptr Data buffer pointer
 * @param[in] data_len Length of buffer
 * @param[in] data_flags Data buffer flags (such as read-only, etc)
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_set_res_data(const char *pathstr, void *data_ptr, uint16_t data_len,
			      uint8_t data_flags);

/**
 * @brief Update data size for a resource
 *
 * @deprecated Use lwm2m_set_res_data_len() instead.
 *
 * Use this function to set the new size of data in the buffer if you write
 * to a buffer received by lwm2m_engine_get_res_buf().
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[in] data_len Length of data
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_set_res_data_len(const char *pathstr, uint16_t data_len);

/**
 * @brief Update data size for a resource
 *
 * Use this function to set the new size of data in the buffer if you write
 * to a buffer received by lwm2m_engine_get_res_buf().
 *
 * @param[in] path LwM2M path as a struct
 * @param[in] data_len Length of data
 * @return 0 for success or negative in case of error.
 */
int lwm2m_set_res_data_len(const struct lwm2m_obj_path *path, uint16_t data_len);

/**
 * @brief Get data buffer for a resource
 *
 * @deprecated Use lwm2m_get_res_buf() instead.
 *
 * Use this function to get the data buffer information for the specified LwM2M
 * resource.
 *
 * If you directly write into the buffer, you must use lwm2m_engine_set_res_data_len()
 * function to update the new size of the written data.
 *
 * All parameters except pathstr can NULL if you don't want to read those values.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[out] buffer_ptr Data buffer pointer
 * @param[out] buffer_len Length of buffer
 * @param[out] data_len Length of existing data in the buffer
 * @param[out] data_flags Data buffer flags (such as read-only, etc)
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_get_res_buf(const char *pathstr, void **buffer_ptr, uint16_t *buffer_len,
			     uint16_t *data_len, uint8_t *data_flags);

/**
 * @brief Get data buffer for a resource
 *
 * Use this function to get the data buffer information for the specified LwM2M
 * resource.
 *
 * If you directly write into the buffer, you must use lwm2m_set_res_data_len()
 * function to update the new size of the written data.
 *
 * All parameters, except for the pathstr, can be NULL if you don't want to read those values.
 *
 * @param[in] path LwM2M path as a struct
 * @param[out] buffer_ptr Data buffer pointer
 * @param[out] buffer_len Length of buffer
 * @param[out] data_len Length of existing data in the buffer
 * @param[out] data_flags Data buffer flags (such as read-only, etc)
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_get_res_buf(const struct lwm2m_obj_path *path, void **buffer_ptr, uint16_t *buffer_len,
		      uint16_t *data_len, uint8_t *data_flags);

/**
 * @brief Get data buffer for a resource
 *
 * Use this function to get the data buffer information for the specified LwM2M
 * resource.
 *
 * @deprecated Use lwm2m_get_res_buf() as it can tell you the size of the buffer as well.
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res(/res-inst)"
 * @param[out] data_ptr Data buffer pointer
 * @param[out] data_len Length of existing data in the buffer
 * @param[out] data_flags Data buffer flags (such as read-only, etc)
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_get_res_data(const char *pathstr, void **data_ptr, uint16_t *data_len,
			      uint8_t *data_flags);

/**
 * @brief Create a resource instance
 *
 * @deprecated Use lwm2m_create_res_inst() instead.
 *
 * LwM2M clients use this function to create multi-resource instances:
 * Example to create 0 instance of device available power sources:
 * lwm2m_engine_create_res_inst("3/0/6/0");
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res/res-inst"
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_create_res_inst(const char *pathstr);

/**
 * @brief Create a resource instance
 *
 * LwM2M clients use this function to create multi-resource instances:
 * Example to create 0 instance of device available power sources:
 * lwm2m_create_res_inst(&LWM2M_OBJ(3, 0, 6, 0));
 *
 * @param[in] path LwM2M path as a struct
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_create_res_inst(const struct lwm2m_obj_path *path);

/**
 * @brief Delete a resource instance
 *
 * @deprecated Use lwm2m_delete_res_inst() instead.
 *
 * Use this function to remove an existing resource instance
 *
 * @param[in] pathstr LwM2M path string "obj/obj-inst/res/res-inst"
 *
 * @return 0 for success or negative in case of error.
 */
__deprecated
int lwm2m_engine_delete_res_inst(const char *pathstr);

/**
 * @brief Delete a resource instance
 *
 * Use this function to remove an existing resource instance
 *
 * @param[in] path LwM2M path as a struct
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_delete_res_inst(const struct lwm2m_obj_path *path);

/**
 * @brief Update the period of a given service.
 *
 * Allow the period modification on an existing service created with
 * lwm2m_engine_add_service().
 * Example to frequency at which a periodic_service changes it's values :
 * lwm2m_engine_update_service(device_periodic_service,5*MSEC_PER_SEC);
 *
 * @param[in] service Handler of the periodic_service
 * @param[in] period_ms New period for the periodic_service (in milliseconds)
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_engine_update_service_period(k_work_handler_t service, uint32_t period_ms);

/**
 * @brief Update the period of the device service.
 *
 * Change the duration of the periodic device service that notifies the
 * current time.
 *
 * @param[in] period_ms New period for the device service (in milliseconds)
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_update_device_service_period(uint32_t period_ms);

/**
 * @brief Check whether a path is observed
 *
 * @deprecated Use lwm2m_path_is_observed() instead.
 *
 * @param[in] pathstr LwM2M path string to check, e.g. "3/0/1"
 *
 * @return true when there exists an observation of the same level
 *         or lower as the given path, false if it doesn't or path is not a
 *         valid LwM2M-path.
 *         E.g. true if path refers to a resource and the parent object has an
 *         observation, false for the inverse.
 */
__deprecated
bool lwm2m_engine_path_is_observed(const char *pathstr);

/**
 * @brief Check whether a path is observed
 *
 * @param[in] path LwM2M path as a struct to check
 *
 * @return true when there exists an observation of the same level
 *         or lower as the given path, false if it doesn't or path is not a
 *         valid LwM2M-path.
 *         E.g. true if path refers to a resource and the parent object has an
 *         observation, false for the inverse.
 */
bool lwm2m_path_is_observed(const struct lwm2m_obj_path *path);

/**
 * @brief Stop the LwM2M engine
 *
 * LwM2M clients normally do not need to call this function as it is called
 * within lwm2m_rd_client. However, if the client does not use the RD
 * client implementation, it will need to be called manually.
 *
 * @param[in] client_ctx LwM2M context
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_engine_stop(struct lwm2m_ctx *client_ctx);

/**
 * @brief Start the LwM2M engine
 *
 * LwM2M clients normally do not need to call this function as it is called
 * by lwm2m_rd_client_start().  However, if the client does not use the RD
 * client implementation, it will need to be called manually.
 *
 * @param[in] client_ctx LwM2M context
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_engine_start(struct lwm2m_ctx *client_ctx);

/**
 * @brief Acknowledge the currently processed request with an empty ACK.
 *
 * LwM2M engine by default sends piggybacked responses for requests.
 * This function allows to send an empty ACK for a request earlier (from the
 * application callback). The LwM2M engine will then send the actual response
 * as a separate CON message after all callbacks are executed.
 *
 * @param[in] client_ctx LwM2M context
 *
 */
void lwm2m_acknowledge(struct lwm2m_ctx *client_ctx);

/**
 * @brief LwM2M RD client events
 *
 * LwM2M client events are passed back to the event_cb function in
 * lwm2m_rd_client_start()
 */
enum lwm2m_rd_client_event {
	LWM2M_RD_CLIENT_EVENT_NONE,
	LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_FAILURE,
	LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_COMPLETE,
	LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_TRANSFER_COMPLETE,
	LWM2M_RD_CLIENT_EVENT_REGISTRATION_FAILURE,
	LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE,
	LWM2M_RD_CLIENT_EVENT_REG_TIMEOUT,
	LWM2M_RD_CLIENT_EVENT_REG_UPDATE_COMPLETE,
	LWM2M_RD_CLIENT_EVENT_DEREGISTER_FAILURE,
	LWM2M_RD_CLIENT_EVENT_DISCONNECT,
	LWM2M_RD_CLIENT_EVENT_QUEUE_MODE_RX_OFF,
	LWM2M_RD_CLIENT_EVENT_ENGINE_SUSPENDED,
	LWM2M_RD_CLIENT_EVENT_NETWORK_ERROR,
	LWM2M_RD_CLIENT_EVENT_REG_UPDATE,
};

/**
 *	Define for old event name keeping backward compatibility.
 */
#define LWM2M_RD_CLIENT_EVENT_REG_UPDATE_FAILURE                                                   \
	LWM2M_RD_CLIENT_EVENT_REG_TIMEOUT __DEPRECATED_MACRO

/*
 * LwM2M RD client flags, used to configure LwM2M session.
 */

/**
 * @brief Run bootstrap procedure in current session.
 */
#define LWM2M_RD_CLIENT_FLAG_BOOTSTRAP BIT(0)

/**
 * @brief Start the LwM2M RD (Registration / Discovery) Client
 *
 * The RD client sits just above the LwM2M engine and performs the necessary
 * actions to implement the "Registration interface".
 * For more information see Section "Client Registration Interface" of
 * LwM2M Technical Specification.
 *
 * NOTE: lwm2m_engine_start() is called automatically by this function.
 *
 * @param[in] client_ctx LwM2M context
 * @param[in] ep_name Registered endpoint name
 * @param[in] flags Flags used to configure current LwM2M session.
 * @param[in] event_cb Client event callback function
 * @param[in] observe_cb Observe callback function called when an observer was
 *			 added or deleted, and when a notification was acked or
 *			 has timed out
 *
 * @return 0 for success, -EINPROGRESS when client is already running
 *         or negative error codes in case of failure.
 */
int lwm2m_rd_client_start(struct lwm2m_ctx *client_ctx, const char *ep_name,
			   uint32_t flags, lwm2m_ctx_event_cb_t event_cb,
			   lwm2m_observe_cb_t observe_cb);

/**
 * @brief Stop the LwM2M RD (De-register) Client
 *
 * The RD client sits just above the LwM2M engine and performs the necessary
 * actions to implement the "Registration interface".
 * For more information see Section "Client Registration Interface" of the
 * LwM2M Technical Specification.
 *
 * @param[in] client_ctx LwM2M context
 * @param[in] event_cb Client event callback function
 * @param[in] deregister True to deregister the client if registered.
 *                       False to force close the connection.
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_rd_client_stop(struct lwm2m_ctx *client_ctx,
			  lwm2m_ctx_event_cb_t event_cb, bool deregister);

/**
 * @brief Suspend the LwM2M engine Thread
 *
 * Suspend LwM2M engine. Use case could be when network connection is down.
 * LwM2M Engine indicate before it suspend by
 * LWM2M_RD_CLIENT_EVENT_ENGINE_SUSPENDED event.
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_engine_pause(void);

/**
 * @brief Resume the LwM2M engine thread
 *
 * Resume suspended LwM2M engine. After successful resume call engine will do
 * full registration or registration update based on suspended time.
 * Event's LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE or WM2M_RD_CLIENT_EVENT_REG_UPDATE_COMPLETE
 * indicate that client is connected to server.
 *
 * @return 0 for success or negative in case of error.
 */
int lwm2m_engine_resume(void);

/**
 * @brief Trigger a Registration Update of the LwM2M RD Client
 */
void lwm2m_rd_client_update(void);

/**
 * @brief LwM2M path maximum length
 */
#define LWM2M_MAX_PATH_STR_SIZE sizeof("/65535/65535/65535/65535")

/**
 * @brief Helper function to print path objects' contents to log
 *
 * @param[in] buf The buffer to use for formatting the string
 * @param[in] path The path to stringify
 *
 * @return Resulting formatted path string
 */
char *lwm2m_path_log_buf(char *buf, struct lwm2m_obj_path *path);

/**
 * @brief LwM2M send status
 *
 * LwM2M send status are generated back to the lwm2m_send_cb_t function in
 * lwm2m_send_cb()
 */
enum lwm2m_send_status {
	LWM2M_SEND_STATUS_SUCCESS,
	LWM2M_SEND_STATUS_FAILURE,
	LWM2M_SEND_STATUS_TIMEOUT,
};

/**
 * @typedef lwm2m_send_cb_t
 * @brief Callback returning send status
 */
typedef void (*lwm2m_send_cb_t)(enum lwm2m_send_status status);

/** 
 * @brief LwM2M SEND operation to given path list
 *
 * @deprecated Use lwm2m_send_cb() instead.
 *
 * @param ctx LwM2M context
 * @param path_list LwM2M Path string list
 * @param path_list_size Length of path list. Max size is CONFIG_LWM2M_COMPOSITE_PATH_LIST_SIZE
 * @param confirmation_request True request confirmation for operation.
 *
 * @return 0 for success or negative in case of error.
 *
 */
__deprecated
int lwm2m_engine_send(struct lwm2m_ctx *ctx, char const *path_list[], uint8_t path_list_size,
		      bool confirmation_request);

/** 
 * @brief LwM2M SEND operation to given path list
 *
 * @deprecated Use lwm2m_send_cb() instead.
 *
 * @param ctx LwM2M context
 * @param path_list LwM2M path struct list
 * @param path_list_size Length of path list. Max size is CONFIG_LWM2M_COMPOSITE_PATH_LIST_SIZE
 * @param confirmation_request True request confirmation for operation.
 *
 * @return 0 for success or negative in case of error.
 *
 */
__deprecated
int lwm2m_send(struct lwm2m_ctx *ctx, const struct lwm2m_obj_path path_list[],
	       uint8_t path_list_size, bool confirmation_request);

/** 
 * @brief LwM2M SEND operation to given path list asynchronously with confirmation callback
 *
 * @param ctx LwM2M context
 * @param path_list LwM2M path struct list
 * @param path_list_size Length of path list. Max size is CONFIG_LWM2M_COMPOSITE_PATH_LIST_SIZE
 * @param reply_cb Callback triggered with confirmation state or NULL if not used
 *
 * @return 0 for success or negative in case of error.
 *
 */
int lwm2m_send_cb(struct lwm2m_ctx *ctx, const struct lwm2m_obj_path path_list[],
			  uint8_t path_list_size, lwm2m_send_cb_t reply_cb);

/** 
 * @brief Returns LwM2M client context
 *
 * @return ctx LwM2M context
 *
 */
struct lwm2m_ctx *lwm2m_rd_client_ctx(void);

/** 
 * @brief Enable data cache for a resource.
 *
 * @deprecated Use lwm2m_enable_cache instead
 *
 * Application may enable caching of resource data by allocating buffer for LwM2M engine to use.
 * Buffer must be size of struct @ref lwm2m_time_series_elem times cache_len
 *
 * @param resource_path LwM2M resourcepath string "obj/obj-inst/res(/res-inst)"
 * @param data_cache Pointer to Data cache array
 * @param cache_len number of cached entries
 *
 * @return 0 for success or negative in case of error.
 *
 */
__deprecated
int lwm2m_engine_enable_cache(char const *resource_path, struct lwm2m_time_series_elem *data_cache,
			      size_t cache_len);

/** 
 * @brief Enable data cache for a resource.
 *
 * Application may enable caching of resource data by allocating buffer for LwM2M engine to use.
 * Buffer must be size of struct @ref lwm2m_time_series_elem times cache_len
 *
 * @param path LwM2M path to resource as a struct
 * @param data_cache Pointer to Data cache array
 * @param cache_len number of cached entries
 *
 * @return 0 for success or negative in case of error.
 *
 */
int lwm2m_enable_cache(const struct lwm2m_obj_path *path, struct lwm2m_time_series_elem *data_cache,
		       size_t cache_len);

#endif	/* ZEPHYR_INCLUDE_NET_LWM2M_H_ */
/**@}  */
