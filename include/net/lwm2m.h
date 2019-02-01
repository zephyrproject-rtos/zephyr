/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2017-2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_LWM2M_H_
#define ZEPHYR_INCLUDE_NET_LWM2M_H_

#include <kernel.h>
#include <net/coap.h>

/**
 * @brief LwM2M high-level API
 * @defgroup lwm2m_api LwM2M high-level API
 * @{
 */

/* LWM2M Objects defined by OMA */

#define LWM2M_OBJECT_SECURITY_ID			0
#define LWM2M_OBJECT_SERVER_ID				1
#define LWM2M_OBJECT_ACCESS_CONTROL_ID			2
#define LWM2M_OBJECT_DEVICE_ID				3
#define LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID		4
#define LWM2M_OBJECT_FIRMWARE_ID			5
#define LWM2M_OBJECT_LOCATION_ID			6
#define LWM2M_OBJECT_CONNECTIVITY_STATISTICS_ID		7

/* IPSO Alliance Objects */

#define IPSO_OBJECT_TEMP_SENSOR_ID			3303
#define IPSO_OBJECT_LIGHT_CONTROL_ID			3311

/**
 * @brief LwM2M context structure
 *
 * @details Context structure for the LwM2M high-level API.
 *
 * @param remote_addr Stored remote IP address of the LwM2M client
 */
struct lwm2m_ctx {
	/** destination address storage */
	struct sockaddr remote_addr;

	/** Private CoAP and networking structures */
	struct coap_pending pendings[CONFIG_LWM2M_ENGINE_MAX_PENDING];
	struct coap_reply replies[CONFIG_LWM2M_ENGINE_MAX_REPLIES];
	struct k_delayed_work retransmit_work;

#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
	/** DTLS settings */
	int tls_tag;
#endif
	bool use_dtls;

	/** Current security object index */
	int sec_obj_inst;
	bool bootstrap_mode;

	/** Packet Flow Settings */
	bool handle_separate_response;

	/** Socket File Descriptor */
	int sock_fd;
};

typedef void *(*lwm2m_engine_get_data_cb_t)(u16_t obj_inst_id,
				       size_t *data_len);
/* callbacks return 0 on success and error code otherwise */
typedef int (*lwm2m_engine_set_data_cb_t)(u16_t obj_inst_id,
				       u8_t *data, u16_t data_len,
				       bool last_block, size_t total_size);
typedef int (*lwm2m_engine_user_cb_t)(u16_t obj_inst_id);


/* LWM2M Device Object */

#define LWM2M_DEVICE_PWR_SRC_TYPE_DC_POWER	0
#define LWM2M_DEVICE_PWR_SRC_TYPE_BAT_INT	1
#define LWM2M_DEVICE_PWR_SRC_TYPE_BAT_EXT	2
#define LWM2M_DEVICE_PWR_SRC_TYPE_UNUSED	3
#define LWM2M_DEVICE_PWR_SRC_TYPE_PWR_OVER_ETH	4
#define LWM2M_DEVICE_PWR_SRC_TYPE_USB		5
#define LWM2M_DEVICE_PWR_SRC_TYPE_AC_POWER	6
#define LWM2M_DEVICE_PWR_SRC_TYPE_SOLAR		7
#define LWM2M_DEVICE_PWR_SRC_TYPE_MAX		8

#define LWM2M_DEVICE_ERROR_NONE			0
#define LWM2M_DEVICE_ERROR_LOW_POWER		1
#define LWM2M_DEVICE_ERROR_EXT_POWER_SUPPLY_OFF	2
#define LWM2M_DEVICE_ERROR_GPS_FAILURE		3
#define LWM2M_DEVICE_ERROR_LOW_SIGNAL_STRENGTH	4
#define LWM2M_DEVICE_ERROR_OUT_OF_MEMORY	5
#define LWM2M_DEVICE_ERROR_SMS_FAILURE		6
#define LWM2M_DEVICE_ERROR_NETWORK_FAILURE	7
#define LWM2M_DEVICE_ERROR_PERIPHERAL_FAILURE	8

#define LWM2M_DEVICE_BATTERY_STATUS_NORMAL	0
#define LWM2M_DEVICE_BATTERY_STATUS_CHARGING	1
#define LWM2M_DEVICE_BATTERY_STATUS_CHARGE_COMP	2
#define LWM2M_DEVICE_BATTERY_STATUS_DAMAGED	3
#define LWM2M_DEVICE_BATTERY_STATUS_LOW		4
#define LWM2M_DEVICE_BATTERY_STATUS_NOT_INST	5
#define LWM2M_DEVICE_BATTERY_STATUS_UNKNOWN	6

int lwm2m_device_add_pwrsrc(u8_t pwr_src_type); /* returns index */
int lwm2m_device_remove_pwrsrc(int index);
int lwm2m_device_set_pwrsrc_voltage_mv(int index, int voltage_mv);
int lwm2m_device_set_pwrsrc_current_ma(int index, int current_ma);
int lwm2m_device_add_err(u8_t error_code);


/* LWM2M Firemware Update Object */

#define STATE_IDLE		0
#define STATE_DOWNLOADING	1
#define STATE_DOWNLOADED	2
#define STATE_UPDATING		3

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
void lwm2m_firmware_set_write_cb(lwm2m_engine_set_data_cb_t cb);
lwm2m_engine_set_data_cb_t lwm2m_firmware_get_write_cb(void);

#if defined(CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_SUPPORT)
void lwm2m_firmware_set_update_cb(lwm2m_engine_user_cb_t cb);
lwm2m_engine_user_cb_t lwm2m_firmware_get_update_cb(void);
#endif
#endif

/* LWM2M Engine */

/*
 * float type below use the following logic:
 * val1 is the whole number portion of decimal
 * val2 is the decimal portion *1000000 for 32bit, *1000000000 for 64bit
 * Example: 123.456 == val1: 123, val2:456000
 * Example: 123.000456 = val1: 123, val2:456
 */
typedef struct float32_value {
	s32_t val1;
	s32_t val2;
} float32_value_t;

typedef struct float64_value {
	s64_t val1;
	s64_t val2;
} float64_value_t;

int lwm2m_engine_create_obj_inst(char *pathstr);

int lwm2m_engine_set_opaque(char *pathstr, char *data_ptr, u16_t data_len);
int lwm2m_engine_set_string(char *path, char *data_ptr);
int lwm2m_engine_set_u8(char *path, u8_t value);
int lwm2m_engine_set_u16(char *path, u16_t value);
int lwm2m_engine_set_u32(char *path, u32_t value);
int lwm2m_engine_set_u64(char *path, u64_t value);
int lwm2m_engine_set_s8(char *path, s8_t value);
int lwm2m_engine_set_s16(char *path, s16_t value);
int lwm2m_engine_set_s32(char *path, s32_t value);
int lwm2m_engine_set_s64(char *path, s64_t value);
int lwm2m_engine_set_bool(char *path, bool value);
int lwm2m_engine_set_float32(char *pathstr, float32_value_t *value);
int lwm2m_engine_set_float64(char *pathstr, float64_value_t *value);

int lwm2m_engine_get_opaque(char *pathstr, void *buf, u16_t buflen);
int lwm2m_engine_get_string(char *path, void *str, u16_t strlen);
int lwm2m_engine_get_u8(char *path, u8_t *value);
int lwm2m_engine_get_u16(char *path, u16_t *value);
int lwm2m_engine_get_u32(char *path, u32_t *value);
int lwm2m_engine_get_u64(char *path, u64_t *value);
int lwm2m_engine_get_s8(char *path, s8_t *value);
int lwm2m_engine_get_s16(char *path, s16_t *value);
int lwm2m_engine_get_s32(char *path, s32_t *value);
int lwm2m_engine_get_s64(char *path, s64_t *value);
int lwm2m_engine_get_bool(char *path, bool *value);
int lwm2m_engine_get_float32(char *pathstr, float32_value_t *buf);
int lwm2m_engine_get_float64(char *pathstr, float64_value_t *buf);

int lwm2m_engine_register_read_callback(char *path,
					lwm2m_engine_get_data_cb_t cb);
int lwm2m_engine_register_pre_write_callback(char *path,
					     lwm2m_engine_get_data_cb_t cb);
int lwm2m_engine_register_post_write_callback(char *path,
					      lwm2m_engine_set_data_cb_t cb);
int lwm2m_engine_register_exec_callback(char *path,
					lwm2m_engine_user_cb_t cb);
int lwm2m_engine_register_create_callback(u16_t obj_id,
					  lwm2m_engine_user_cb_t cb);
int lwm2m_engine_register_delete_callback(u16_t obj_id,
					  lwm2m_engine_user_cb_t cb);

/* resource data bit values */
#define LWM2M_RES_DATA_READ_ONLY	0

/* resource data flags */
#define LWM2M_RES_DATA_FLAG_RO		BIT(LWM2M_RES_DATA_READ_ONLY)

/* helper macro to read resource flag */
#define LWM2M_HAS_RES_FLAG(res, f)	((res->data_flags & f) == f)

int lwm2m_engine_set_res_data(char *pathstr, void *data_ptr, u16_t data_len,
			      u8_t data_flags);
int lwm2m_engine_get_res_data(char *pathstr, void **data_ptr, u16_t *data_len,
			      u8_t *data_flags);

int lwm2m_engine_start(struct lwm2m_ctx *client_ctx);

/* LWM2M RD Client */

/* Client events */
enum lwm2m_rd_client_event {
	LWM2M_RD_CLIENT_EVENT_NONE,
	LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_FAILURE,
	LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_COMPLETE,
	LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_TRANSFER_COMPLETE,
	LWM2M_RD_CLIENT_EVENT_REGISTRATION_FAILURE,
	LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE,
	LWM2M_RD_CLIENT_EVENT_REG_UPDATE_FAILURE,
	LWM2M_RD_CLIENT_EVENT_REG_UPDATE_COMPLETE,
	LWM2M_RD_CLIENT_EVENT_DEREGISTER_FAILURE,
	LWM2M_RD_CLIENT_EVENT_DISCONNECT
};

/* Event callback */
typedef void (*lwm2m_ctx_event_cb_t)(struct lwm2m_ctx *ctx,
				     enum lwm2m_rd_client_event event);

void lwm2m_rd_client_start(struct lwm2m_ctx *client_ctx, const char *ep_name,
			   lwm2m_ctx_event_cb_t event_cb);

/**
 * @}
 */
#endif	/* ZEPHYR_INCLUDE_NET_LWM2M_H_ */
