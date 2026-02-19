/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 /**
  * @file
  * @brief Internal macros and definitions for cellular modem drivers.
  */

 #ifndef ZEPHYR_INCLUDE_DRIVERS_CELLULAR_INTERNAL_H_
 #define ZEPHYR_INCLUDE_DRIVERS_CELLULAR_INTERNAL_H_

#include <zephyr/drivers/cellular.h>
#include <zephyr/modem/backend/uart.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>
#include <zephyr/modem/chat.h>
#include <zephyr/modem/cmux.h>
#include <zephyr/modem/pipe.h>
#include <zephyr/modem/pipelink.h>
#include <zephyr/modem/ppp.h>

/**
 * @cond INTERNAL_HIDDEN
 *
 * For internal driver use only, skip these in public documentation.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup modem_cellular Definitions for cellular modem drivers
 *
 * This group contains internal definitions and macros for modem_cellular.c
 * These are exported into the header only to allow device macros to be exported,
 * these should not be used by external code.
 *
 *  @{
 */

#define MODEM_CELLULAR_DATA_IMEI_LEN         (16)
#define MODEM_CELLULAR_DATA_MODEL_ID_LEN     (65)
#define MODEM_CELLULAR_DATA_IMSI_LEN         (23)
#define MODEM_CELLULAR_DATA_ICCID_LEN        (22)
#define MODEM_CELLULAR_DATA_MANUFACTURER_LEN (65)
#define MODEM_CELLULAR_DATA_FW_VERSION_LEN   (65)
#define MODEM_CELLULAR_DATA_APN_LEN          (32)
#define MODEM_CELLULAR_MAX_APN_CMDS          (2)
#define MODEM_CELLULAR_APN_BUF_SIZE          (64)

enum modem_cellular_state {
	MODEM_CELLULAR_STATE_IDLE = 0,
	MODEM_CELLULAR_STATE_RESET_PULSE,
	MODEM_CELLULAR_STATE_AWAIT_RESET,
	MODEM_CELLULAR_STATE_POWER_ON_PULSE,
	MODEM_CELLULAR_STATE_AWAIT_POWER_ON,
	MODEM_CELLULAR_STATE_SET_BAUDRATE,
	MODEM_CELLULAR_STATE_RUN_INIT_SCRIPT,
	MODEM_CELLULAR_STATE_CONNECT_CMUX,
	MODEM_CELLULAR_STATE_OPEN_DLCI1,
	MODEM_CELLULAR_STATE_OPEN_DLCI2,
	MODEM_CELLULAR_STATE_WAIT_FOR_APN,
	MODEM_CELLULAR_STATE_RUN_APN_SCRIPT,
	MODEM_CELLULAR_STATE_RUN_DIAL_SCRIPT,
	MODEM_CELLULAR_STATE_AWAIT_REGISTERED,
	MODEM_CELLULAR_STATE_CARRIER_ON,
	MODEM_CELLULAR_STATE_DORMANT,
	MODEM_CELLULAR_STATE_INIT_POWER_OFF,
	MODEM_CELLULAR_STATE_RUN_SHUTDOWN_SCRIPT,
	MODEM_CELLULAR_STATE_POWER_OFF_PULSE,
	MODEM_CELLULAR_STATE_AWAIT_POWER_OFF,
};

enum modem_cellular_event {
	MODEM_CELLULAR_EVENT_RESUME = 0,
	MODEM_CELLULAR_EVENT_SUSPEND,
	MODEM_CELLULAR_EVENT_SCRIPT_SUCCESS,
	MODEM_CELLULAR_EVENT_SCRIPT_FAILED,
	MODEM_CELLULAR_EVENT_CMUX_CONNECTED,
	MODEM_CELLULAR_EVENT_CMUX_DISCONNECTED,
	MODEM_CELLULAR_EVENT_DLCI1_OPENED,
	MODEM_CELLULAR_EVENT_DLCI2_OPENED,
	MODEM_CELLULAR_EVENT_TIMEOUT,
	MODEM_CELLULAR_EVENT_REGISTERED,
	MODEM_CELLULAR_EVENT_DEREGISTERED,
	MODEM_CELLULAR_EVENT_BUS_OPENED,
	MODEM_CELLULAR_EVENT_BUS_CLOSED,
	MODEM_CELLULAR_EVENT_PPP_DEAD,
	MODEM_CELLULAR_EVENT_MODEM_READY,
	MODEM_CELLULAR_EVENT_APN_SET,
	MODEM_CELLULAR_EVENT_RING,
};

struct modem_cellular_event_cb {
	cellular_event_mask_t mask;
	cellular_event_cb_t fn;
	void *user_data;
};

struct modem_cellular_data {
	/* UART backend */
	struct modem_pipe *uart_pipe;
	struct modem_backend_uart uart_backend;
	uint8_t uart_backend_receive_buf[CONFIG_MODEM_CELLULAR_UART_BUFFER_SIZES];
	uint8_t uart_backend_transmit_buf[CONFIG_MODEM_CELLULAR_UART_BUFFER_SIZES];
	uint32_t original_baudrate;

	/* CMUX */
	struct modem_cmux cmux;
	uint8_t cmux_receive_buf[MODEM_CMUX_WORK_BUFFER_SIZE];
	uint8_t cmux_transmit_buf[MODEM_CMUX_WORK_BUFFER_SIZE];

	struct modem_cmux_dlci dlci1;
	struct modem_cmux_dlci dlci2;
	struct modem_pipe *dlci1_pipe;
	struct modem_pipe *dlci2_pipe;
	/* Points to dlci2_pipe or NULL. Used for shutdown script if not NULL */
	struct modem_pipe *cmd_pipe;
	uint8_t dlci1_receive_buf[MODEM_CMUX_WORK_BUFFER_SIZE];
	/* DLCI 2 is only used for chat scripts. */
	uint8_t dlci2_receive_buf[MODEM_CMUX_WORK_BUFFER_SIZE];

	/* Modem chat */
	struct modem_chat chat;
	uint8_t chat_receive_buf[CONFIG_MODEM_CELLULAR_CHAT_BUFFER_SIZE];
	uint8_t *chat_delimiter;
	uint8_t *chat_filter;
	uint8_t *chat_argv[32];
	uint8_t script_failure_counter;

	/* Status */
	enum cellular_registration_status registration_status_gsm;
	enum cellular_registration_status registration_status_gprs;
	enum cellular_registration_status registration_status_lte;
	uint8_t rssi;
	uint8_t rsrp;
	uint8_t rsrq;
	uint8_t imei[MODEM_CELLULAR_DATA_IMEI_LEN];
	uint8_t model_id[MODEM_CELLULAR_DATA_MODEL_ID_LEN];
	uint8_t imsi[MODEM_CELLULAR_DATA_IMSI_LEN];
	uint8_t iccid[MODEM_CELLULAR_DATA_ICCID_LEN];
	uint8_t manufacturer[MODEM_CELLULAR_DATA_MANUFACTURER_LEN];
	uint8_t fw_version[MODEM_CELLULAR_DATA_FW_VERSION_LEN];
	uint8_t apn[MODEM_CELLULAR_DATA_APN_LEN];

	struct modem_chat_script_chat apn_chats[MODEM_CELLULAR_MAX_APN_CMDS];
	struct modem_chat_script apn_script;
	char apn_buf[MODEM_CELLULAR_MAX_APN_CMDS][MODEM_CELLULAR_APN_BUF_SIZE];

	/* PPP */
	struct modem_ppp *ppp;
	struct net_mgmt_event_callback net_mgmt_event_callback;

	enum modem_cellular_state state;
	const struct device *dev;
	struct k_work_delayable timeout_work;

	/* Power management */
	struct k_sem suspended_sem;

	/* Event dispatcher */
	struct k_work event_dispatch_work;
	uint8_t event_buf[8];
	struct k_pipe event_pipe;

	struct k_mutex api_lock;
	struct modem_cellular_event_cb cb;

	/* Ring interrupt */
	struct gpio_callback ring_gpio_cb;
};

struct modem_cellular_user_pipe {
	struct modem_cmux_dlci dlci;
	uint8_t dlci_address;
	uint8_t *dlci_receive_buf;
	uint16_t dlci_receive_buf_size;
	struct modem_pipe *pipe;
	struct modem_pipelink *pipelink;
};

struct modem_cellular_config {
	const struct device *uart;
	struct gpio_dt_spec power_gpio;
	struct gpio_dt_spec reset_gpio;
	struct gpio_dt_spec wake_gpio;
	struct gpio_dt_spec ring_gpio;
	struct gpio_dt_spec dtr_gpio;
	uint16_t power_pulse_duration_ms;
	uint16_t reset_pulse_duration_ms;
	uint16_t startup_time_ms;
	uint16_t shutdown_time_ms;
	bool autostarts;
	bool hold_reset_on_suspend;
	bool reset_on_resume;
	bool reset_on_recovery;
	bool cmux_enable_runtime_power_save;
	bool cmux_close_pipe_on_power_save;
	bool use_default_pdp_context;
	bool use_default_apn;
	k_timeout_t cmux_idle_timeout;
	const struct modem_chat_script *init_chat_script;
	const struct modem_chat_script *dial_chat_script;
	const struct modem_chat_script *periodic_chat_script;
	const struct modem_chat_script *shutdown_chat_script;
	const struct modem_chat_script *set_baudrate_chat_script;
	struct modem_cellular_user_pipe *user_pipes;
	uint8_t user_pipes_size;
};

/** @brief Initialize the cellular modem
 * Called from the DEVICE_DT_INST_DEFINE macro for cellular modem drivers.
 */
int modem_cellular_init(const struct device *dev);

int modem_cellular_pm_action(const struct device *dev, enum pm_device_action action);

extern const struct cellular_driver_api modem_cellular_api;

void modem_cellular_chat_callback_handler(struct modem_chat *chat,
						 enum modem_chat_script_result result,
						 void *user_data);
/** @} */

/**
 * @defgroup modem_driver_macros Macros for defining cellular modem driver instances
 *
 * These macros are used for defining cellular modem driver instances.
 * See modem_cellular.c for usage examples.
 *
 * @{
 */

#define MODEM_CELLULAR_INST_NAME(name, inst) \
	CONCAT(name, _, DT_DRV_COMPAT, inst)

#define MODEM_CELLULAR_DEFINE_USER_PIPE_DATA(inst, name, size)                                     \
	MODEM_PIPELINK_DT_INST_DEFINE(inst, name);                                                 \
	static uint8_t MODEM_CELLULAR_INST_NAME(name, inst)[size]                                  \

#define MODEM_CELLULAR_INIT_USER_PIPE(_inst, _name, _dlci_address)                                 \
	{                                                                                          \
		.dlci_address = _dlci_address,                                                     \
		.dlci_receive_buf = MODEM_CELLULAR_INST_NAME(_name, _inst),                        \
		.dlci_receive_buf_size = sizeof(MODEM_CELLULAR_INST_NAME(_name, _inst)),           \
		.pipelink = MODEM_PIPELINK_DT_INST_GET(_inst, _name),                              \
	}

#define MODEM_CELLULAR_DEFINE_USER_PIPES(inst, ...)                                                \
	static struct modem_cellular_user_pipe MODEM_CELLULAR_INST_NAME(user_pipes, inst)[] = {    \
		__VA_ARGS__                                                                        \
	}

#define MODEM_CELLULAR_GET_USER_PIPES(inst) \
	MODEM_CELLULAR_INST_NAME(user_pipes, inst)

/* Extract the first argument (pipe name) from a pair */
#define MODEM_CELLULAR_GET_PIPE_NAME_ARG(arg1, ...) arg1

/* Extract the second argument (DLCI address) from a pair */
#define MODEM_CELLULAR_GET_DLCI_ADDRESS_ARG(arg1, arg2, ...) arg2

/* Define user pipe data using instance and extracted pipe name */
#define MODEM_CELLULAR_DEFINE_USER_PIPE_DATA_HELPER(_args, inst)                                   \
	MODEM_CELLULAR_DEFINE_USER_PIPE_DATA(inst,                                                 \
					     MODEM_CELLULAR_GET_PIPE_NAME_ARG _args,               \
					     CONFIG_MODEM_CELLULAR_USER_PIPE_BUFFER_SIZES)

/* Initialize user pipe using instance, extracted pipe name, and DLCI address */
#define MODEM_CELLULAR_INIT_USER_PIPE_HELPER(_args, inst)                                          \
	MODEM_CELLULAR_INIT_USER_PIPE(inst,                                                        \
				      MODEM_CELLULAR_GET_PIPE_NAME_ARG _args,                      \
				      MODEM_CELLULAR_GET_DLCI_ADDRESS_ARG _args)

/*
 * Define and initialize user pipes dynamically
 * Takes an instance and pairs of (pipe name, DLCI address)
 */
#define MODEM_CELLULAR_DEFINE_AND_INIT_USER_PIPES(inst, ...)                                       \
	FOR_EACH_FIXED_ARG(MODEM_CELLULAR_DEFINE_USER_PIPE_DATA_HELPER,                            \
			   (;), inst, __VA_ARGS__);                                                \
	MODEM_CELLULAR_DEFINE_USER_PIPES(                                                          \
		inst,                                                                              \
		FOR_EACH_FIXED_ARG(MODEM_CELLULAR_INIT_USER_PIPE_HELPER,                           \
				   (,), inst, __VA_ARGS__)                                         \
	);

/* Helper to define modem instance */
#define MODEM_CELLULAR_DEFINE_INSTANCE(inst, power_ms, reset_ms, startup_ms, shutdown_ms, start,   \
				       set_baudrate_script, init_script, dial_script,              \
				       periodic_script, shutdown_script)                           \
	static const struct modem_cellular_config MODEM_CELLULAR_INST_NAME(config, inst) = {       \
		.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),                                          \
		.power_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_power_gpios, {}),                 \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_reset_gpios, {}),                 \
		.wake_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_wake_gpios, {}),                   \
		.ring_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_ring_gpios, {}),                   \
		.dtr_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_dtr_gpios, {}),                     \
		.power_pulse_duration_ms = (power_ms),                                             \
		.reset_pulse_duration_ms = (reset_ms),                                             \
		.startup_time_ms = (startup_ms),                                                   \
		.shutdown_time_ms = (shutdown_ms),                                                 \
		.autostarts = DT_INST_PROP_OR(inst, autostarts, (start)),                          \
		.hold_reset_on_suspend =                                                           \
			DT_INST_ENUM_HAS_VALUE(inst, zephyr_mdm_reset_behavior, hold_on_suspend),  \
		.reset_on_resume = DT_INST_ENUM_HAS_VALUE(inst, zephyr_mdm_reset_behavior,         \
							    toggle_on_resume),                     \
		.reset_on_recovery = DT_INST_ENUM_HAS_VALUE(inst, zephyr_mdm_reset_behavior,       \
							    toggle_on_recovery),                   \
		.cmux_enable_runtime_power_save =                                                  \
			DT_INST_PROP_OR(inst, cmux_enable_runtime_power_save, 0),                  \
		.cmux_close_pipe_on_power_save =                                                   \
			DT_INST_PROP_OR(inst, cmux_close_pipe_on_power_save, 0),                   \
		.use_default_pdp_context = DT_INST_PROP_OR(inst, zephyr_use_default_pdp_ctx, 0),   \
		.use_default_apn = DT_INST_PROP_OR(inst, zephyr_use_default_apn, 0),               \
		.cmux_idle_timeout = K_MSEC(DT_INST_PROP_OR(inst, cmux_idle_timeout_ms, 0)),       \
		.set_baudrate_chat_script = (set_baudrate_script),                                 \
		.init_chat_script = (init_script),                                                 \
		.dial_chat_script = (dial_script),                                                 \
		.periodic_chat_script = (periodic_script),                                         \
		.shutdown_chat_script = (shutdown_script),                                         \
		.user_pipes = MODEM_CELLULAR_GET_USER_PIPES(inst),                                 \
		.user_pipes_size = ARRAY_SIZE(MODEM_CELLULAR_GET_USER_PIPES(inst)),                \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, modem_cellular_pm_action);                                  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, modem_cellular_init, PM_DEVICE_DT_INST_GET(inst),              \
			      &MODEM_CELLULAR_INST_NAME(data, inst),                               \
			      &MODEM_CELLULAR_INST_NAME(config, inst), POST_KERNEL,                \
			      CONFIG_MODEM_CELLULAR_INIT_PRIORITY, &modem_cellular_api);

/** @} */

#ifdef __cplusplus
}
#endif

/** @endcond */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CELLULAR_INTERNAL_H_ */
