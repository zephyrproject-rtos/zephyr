/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file modem_cellular.h
 * @brief Backend helpers for implementing cellular modem drivers.
 * @ingroup modem_cellular_backend
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
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/atomic.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup modem_cellular_backend Cellular modem helpers
 * @brief Implement vendor-specific cellular modem drivers using common chat, CMUX, PPP, and
 * power-management support.
 * @in_driverbackendgroup{cellular_interface}
 * @{
 */

/** @cond INTERNAL_HIDDEN */

#define MODEM_CELLULAR_DATA_IMEI_LEN         (16)
#define MODEM_CELLULAR_DATA_MODEL_ID_LEN     (65)
#define MODEM_CELLULAR_DATA_IMSI_LEN         (23)
#define MODEM_CELLULAR_DATA_ICCID_LEN        (22)
#define MODEM_CELLULAR_DATA_MANUFACTURER_LEN (65)
#define MODEM_CELLULAR_DATA_FW_VERSION_LEN   (65)
#define MODEM_CELLULAR_DATA_APN_LEN          (32)
#define MODEM_CELLULAR_MAX_APN_CMDS          (2)
#define MODEM_CELLULAR_APN_BUF_SIZE          (64)

/* Zephyr networking interface states:
 *    NET_IF_LOWER_UP: Carrier is on in AWAIT_REGISTERED and REGISTERED
 *     NET_IF_DORMANT: Interface is dormant in every state except REGISTERED
 */
enum modem_cellular_state {
	MODEM_CELLULAR_STATE_IDLE = 0,
	MODEM_CELLULAR_STATE_RECOVERY,
	MODEM_CELLULAR_STATE_RESET_PULSE,
	MODEM_CELLULAR_STATE_AWAIT_RESET,
	MODEM_CELLULAR_STATE_POWER_ON_PULSE,
	MODEM_CELLULAR_STATE_AWAIT_POWER_ON,
	MODEM_CELLULAR_STATE_SET_BAUDRATE,
	MODEM_CELLULAR_STATE_RUN_INIT_SCRIPT,
	MODEM_CELLULAR_STATE_CONNECT_CMUX,
	MODEM_CELLULAR_STATE_OPEN_DLCI1,
	MODEM_CELLULAR_STATE_OPEN_DLCI2,
	MODEM_CELLULAR_STATE_RUN_BOARD_INIT_SCRIPT,
	MODEM_CELLULAR_STATE_WAIT_FOR_APN,
	MODEM_CELLULAR_STATE_RUN_APN_SCRIPT,
	MODEM_CELLULAR_STATE_RUN_NETWORK_SCRIPT,
	MODEM_CELLULAR_STATE_RUN_DIAL_SCRIPT,
	MODEM_CELLULAR_STATE_AWAIT_REGISTERED,
	MODEM_CELLULAR_STATE_REGISTERED,
	MODEM_CELLULAR_STATE_AWAIT_PPP_DEAD,
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
	MODEM_CELLULAR_EVENT_PERIODIC_KICK,
};

struct modem_cellular_event_cb {
	cellular_event_mask_t mask;
	cellular_event_cb_t fn;
	void *user_data;
};

/** @endcond */

/**
 * @brief Runtime data for a cellular modem driver instance.
 *
 * Define one zero-initialized object per device instance. The object and all referenced
 * configuration data must remain valid for the lifetime of the device. Set the documented
 * configuration members before passing the object to @ref MODEM_CELLULAR_DEFINE_INSTANCE().
 * The cellular modem implementation owns and updates all other members after device initialization.
 */
struct modem_cellular_data {
	/** @cond INTERNAL_HIDDEN */
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
	/* Points to dlci1_pipe or NULL. Used for shutdown script if not NULL */
	struct modem_pipe *cmd_pipe;
	uint8_t dlci1_receive_buf[MODEM_CMUX_WORK_BUFFER_SIZE];
	/* DLCI 2 is only used for chat scripts. */
	uint8_t dlci2_receive_buf[MODEM_CMUX_WORK_BUFFER_SIZE];

	/* Modem chat */
	struct modem_chat chat;
	uint8_t chat_receive_buf[CONFIG_MODEM_CELLULAR_CHAT_BUFFER_SIZE];
	/** @endcond */

	/**
	 * Command delimiter used by the modem, as a NULL-terminated string.
	 *
	 * Must not be NULL and must remain valid for the lifetime of the device.
	 */
	uint8_t *chat_delimiter;
	/**
	 * Characters filtered from modem responses, as a NULL-terminated string.
	 *
	 * May be NULL to disable filtering. A non-NULL string must remain valid for the lifetime of
	 * the device.
	 */
	uint8_t *chat_filter;

	/** @cond INTERNAL_HIDDEN */
	uint8_t *chat_argv[32];
	uint8_t script_failure_counter;
	uint8_t recovery_count;

	/* Status */
	enum cellular_registration_status registration_status_gsm;
	enum cellular_registration_status registration_status_gprs;
	enum cellular_registration_status registration_status_lte;
	enum cellular_access_technology access_tech;
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

	struct modem_chat_script board_init_script;

	/* PPP */
	/** @endcond */

	/** PPP context used for the modem data connection. Must not be NULL. */
	struct modem_ppp *ppp;

	/** @cond INTERNAL_HIDDEN */
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

	/** Set when the periodic chat script is paused. */
	atomic_t periodic_paused;
	/** Set when a TIMEOUT is swallowed while paused; cleared on KICK. */
	bool periodic_timeout_skipped;

#if defined(CONFIG_MODEM_CELLULAR_STATS)
	/** Operational statistics, exposed via cellular_get_stats(). */
	struct cellular_stats stats;
	/** k_uptime (ms) when registration was last lost; 0 while registered. */
	uint32_t stats_outage_start_ms;
#endif
	/** @endcond */
};

/** @cond INTERNAL_HIDDEN */

struct modem_cellular_user_pipe {
	struct modem_cmux_dlci dlci;
	uint8_t dlci_address;
	uint8_t *dlci_receive_buf;
	uint16_t dlci_receive_buf_size;
	struct modem_pipe *pipe;
	struct modem_pipelink *pipelink;
};

/** @endcond */

/**
 * @brief Chat scripts for cellular modem.
 *
 * Only the init and dial scripts are mandatory, other scripts are optional.
 *
 * If the network script is provided, it will be used to wait for network registration
 * before issuing the dial script.
 *
 * If the network script is not provided, the dial script is expected to
 * configure the network registration and the modem will wait for
 * registration after the dial script completes.
 *
 */
struct modem_cellular_config_scripts {
	/** Optional script that configures the modem's UART baud rate. */
	const struct modem_chat_script *set_baudrate;
	/** Script that initializes the modem and enables CMUX. Must not be NULL. */
	const struct modem_chat_script *init;
	/** Optional script that waits for network registration before dialing. */
	const struct modem_chat_script *network;
	/** Script that starts the PPP data connection. Must not be NULL. */
	const struct modem_chat_script *dial;
	/** Optional script that periodically polls modem state while registered. */
	const struct modem_chat_script *periodic;
	/** Optional script that prepares the modem for power-off. */
	const struct modem_chat_script *shutdown;
};

/**
 * @brief Vendor-specific cellular modem configuration.
 *
 * Define one constant object for each modem variant. The referenced scripts and unsolicited-match
 * array must remain valid for the lifetime of every device instance that uses this configuration.
 */
struct modem_cellular_vendor_config {
	/** Chat scripts implementing the modem lifecycle. */
	struct modem_cellular_config_scripts scripts;
	/** Unsolicited modem response matches. */
	struct {
		/** Array of unsolicited response matches, or NULL when @c size is zero. */
		const struct modem_chat_match *matches;
		/** Number of elements in @c matches. */
		uint16_t size;
	} unsol_matches;
	/** Duration of the modem power-key pulse, in milliseconds. */
	uint16_t power_pulse_duration_ms;
	/** Duration of the modem reset pulse, in milliseconds. */
	uint16_t reset_pulse_duration_ms;
	/** Maximum modem startup delay, in milliseconds. */
	uint16_t startup_time_ms;
	/** Maximum modem shutdown delay, in milliseconds. */
	uint16_t shutdown_time_ms;
	/** Force autostart regardless of the devicetree @c autostarts property. */
	bool force_autostart;
};

/** @cond INTERNAL_HIDDEN */

struct modem_cellular_config {
	const struct device *uart;
	const struct modem_cellular_vendor_config *vendor;
	struct gpio_dt_spec power_gpio;
	struct gpio_dt_spec reset_gpio;
	struct gpio_dt_spec wake_gpio;
	struct gpio_dt_spec ring_gpio;
	struct gpio_dt_spec dtr_gpio;
	bool autostarts;
	bool hold_reset_on_suspend;
	bool reset_on_resume;
	bool reset_on_recovery;
	bool cmux_enable_runtime_power_save;
	bool cmux_close_pipe_on_power_save;
	bool cmux_no_powersave_handshake;
	bool use_default_pdp_context;
	bool use_default_apn;
	k_timeout_t cmux_idle_timeout;
	struct modem_cellular_user_pipe *user_pipes;
	uint8_t user_pipes_size;
};

/** @brief Initialize the cellular modem
 * Called from the DEVICE_DT_INST_DEFINE macro for cellular modem drivers.
 */
int modem_cellular_init(const struct device *dev);

int modem_cellular_pm_action(const struct device *dev, enum pm_device_action action);

extern const struct cellular_driver_api modem_cellular_api;

void modem_cellular_emit_event(struct modem_cellular_data *data, enum cellular_event evt,
			       const void *payload);

void modem_cellular_chat_on_imei(struct modem_chat *chat, char **argv, uint16_t argc,
				 void *user_data);
void modem_cellular_chat_on_cgmm(struct modem_chat *chat, char **argv, uint16_t argc,
				 void *user_data);
void modem_cellular_chat_on_csq(struct modem_chat *chat, char **argv, uint16_t argc,
				void *user_data);
void modem_cellular_chat_on_cesq(struct modem_chat *chat, char **argv, uint16_t argc,
				 void *user_data);
void modem_cellular_chat_on_iccid(struct modem_chat *chat, char **argv, uint16_t argc,
				  void *user_data);
void modem_cellular_chat_on_imsi(struct modem_chat *chat, char **argv, uint16_t argc,
				 void *user_data);
void modem_cellular_chat_on_cgmi(struct modem_chat *chat, char **argv, uint16_t argc,
				 void *user_data);
void modem_cellular_chat_on_cgmr(struct modem_chat *chat, char **argv, uint16_t argc,
				 void *user_data);
void modem_cellular_chat_on_cxreg(struct modem_chat *chat, char **argv, uint16_t argc,
				  void *user_data);
void modem_cellular_chat_on_cgev(struct modem_chat *chat, char **argv, uint16_t argc,
				 void *user_data);
void modem_cellular_chat_on_modem_ready(struct modem_chat *chat, char **argv, uint16_t argc,
					void *user_data);

/** @endcond */

/**
 * @brief Handle completion of a modem chat script.
 *
 * Use this as the callback for lifecycle scripts executed by the cellular modem state machine. The
 * callback translates the chat result into the corresponding internal state-machine event.
 *
 * @param chat Chat instance that completed the script. Must not be NULL.
 * @param result Script completion result.
 * @param user_data Pointer to the associated @ref modem_cellular_data object. Must not be NULL.
 */
void modem_cellular_chat_callback_handler(struct modem_chat *chat,
					  enum modem_chat_script_result result, void *user_data);

/**
 * @defgroup modem_driver_macros Cellular modem driver definition macros
 * @brief Define device instances that use the cellular modem driver.
 * @ingroup modem_cellular_backend
 *
 * See the cellular modem documentation for a complete out-of-tree driver example.
 *
 * @{
 */

/**
 * @brief Generate a unique C identifier for a modem instance object.
 *
 * @param name Base object name.
 * @param inst Devicetree instance number.
 *
 * @return The generated C identifier.
 */
#define MODEM_CELLULAR_INST_NAME(name, inst) \
	CONCAT(name, _, DT_DRV_COMPAT, inst)

/** @cond INTERNAL_HIDDEN */

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

/** @endcond */

/**
 * @brief Define additional CMUX user pipes for a modem instance.
 *
 * Each variadic argument is a parenthesized @c (name, dlci_address) pair. The macro defines the
 * receive buffers, pipe links, and user-pipe array consumed by @ref MODEM_CELLULAR_DEFINE_INSTANCE.
 * Invoke it once for each modem instance before invoking @ref MODEM_CELLULAR_DEFINE_INSTANCE.
 *
 * @param inst Devicetree instance number.
 * @param ... One or more <tt>(name, dlci_address)</tt> pairs.
 */
#define MODEM_CELLULAR_DEFINE_AND_INIT_USER_PIPES(inst, ...)                                       \
	FOR_EACH_FIXED_ARG(MODEM_CELLULAR_DEFINE_USER_PIPE_DATA_HELPER,                            \
			   (;), inst, __VA_ARGS__);                                                \
	MODEM_CELLULAR_DEFINE_USER_PIPES(                                                          \
		inst,                                                                              \
		FOR_EACH_FIXED_ARG(MODEM_CELLULAR_INIT_USER_PIPE_HELPER,                           \
				   (,), inst, __VA_ARGS__)                                         \
	);

/**
 * @brief Define common chat matches used by cellular modem scripts.
 *
 * Invoke this macro once at file scope. It defines matches for successful commands, common command
 * failures, modem identity and signal queries, and PPP dial responses. The generated identifiers
 * are intended for use in the modem's @ref modem_cellular_config_scripts.
 */
#define MODEM_CELLULAR_COMMON_CHAT_MATCHES()							   \
	MODEM_CHAT_MATCH_DEFINE(ok_match, "OK", "", NULL);					   \
	MODEM_CHAT_MATCHES_DEFINE(__maybe_unused allow_match,					   \
				  MODEM_CHAT_MATCH("OK", "", NULL),				   \
				  MODEM_CHAT_MATCH("ERROR", "", NULL));				   \
	MODEM_CHAT_MATCH_DEFINE(imei_match __maybe_unused,					   \
				"", "", modem_cellular_chat_on_imei);				   \
	MODEM_CHAT_MATCH_DEFINE(cgmm_match __maybe_unused,					   \
				"", "", modem_cellular_chat_on_cgmm);				   \
	MODEM_CHAT_MATCH_DEFINE(csq_match __maybe_unused,					   \
				"+CSQ: ", ",", modem_cellular_chat_on_csq);			   \
	MODEM_CHAT_MATCH_DEFINE(cesq_match __maybe_unused,					   \
				"+CESQ: ", ",", modem_cellular_chat_on_cesq);			   \
	MODEM_CHAT_MATCH_DEFINE(qccid_match __maybe_unused,					   \
				"+QCCID: ", "", modem_cellular_chat_on_iccid);			   \
	MODEM_CHAT_MATCH_DEFINE(iccid_match __maybe_unused,					   \
				"+ICCID: ", "", modem_cellular_chat_on_iccid);			   \
	MODEM_CHAT_MATCH_DEFINE(ccid_match __maybe_unused,					   \
				"+CCID: ", "", modem_cellular_chat_on_iccid);			   \
	MODEM_CHAT_MATCH_DEFINE(cimi_match __maybe_unused,					   \
				"", "", modem_cellular_chat_on_imsi);				   \
	MODEM_CHAT_MATCH_DEFINE(cgmi_match __maybe_unused,					   \
				"", "", modem_cellular_chat_on_cgmi);				   \
	MODEM_CHAT_MATCH_DEFINE(cgmr_match __maybe_unused,					   \
				"", "", modem_cellular_chat_on_cgmr);				   \
	MODEM_CHAT_MATCH_DEFINE(connect_match __maybe_unused,					   \
				"CONNECT", "", NULL);						   \
	MODEM_CHAT_MATCHES_DEFINE(__maybe_unused abort_matches,					   \
				  MODEM_CHAT_MATCH("ERROR", "", NULL));				   \
	MODEM_CHAT_MATCHES_DEFINE(__maybe_unused dial_abort_matches,				   \
				  MODEM_CHAT_MATCH("ERROR", "", NULL),				   \
				  MODEM_CHAT_MATCH("BUSY", "", NULL),				   \
				  MODEM_CHAT_MATCH("NO ANSWER", "", NULL),			   \
				  MODEM_CHAT_MATCH("NO CARRIER", "", NULL),			   \
				  MODEM_CHAT_MATCH("NO DIALTONE", "", NULL))

/**
 * @brief Expand to the common unsolicited response matches.
 *
 * Use this macro in the entry list passed to MODEM_CHAT_MATCHES_DEFINE().
 * Vendor drivers may add additional entries after this expansion.
 */
#define MODEM_CELLULAR_COMMON_UNSOL_MATCHES							   \
	MODEM_CHAT_MATCH("+CREG: ", ",", modem_cellular_chat_on_cxreg),				   \
	MODEM_CHAT_MATCH("+CEREG: ", ",", modem_cellular_chat_on_cxreg),			   \
	MODEM_CHAT_MATCH("+CGREG: ", ",", modem_cellular_chat_on_cxreg),			   \
	MODEM_CHAT_MATCH("+CGEV: ", ",", modem_cellular_chat_on_cgev),				   \
	MODEM_CHAT_MATCH("APP RDY", "", modem_cellular_chat_on_modem_ready),			   \
	MODEM_CHAT_MATCH("Ready", "", modem_cellular_chat_on_modem_ready)

/**
 * @brief Define a cellular modem device instance.
 *
 * This macro creates the immutable device configuration, power-management object, and Zephyr device
 * for a devicetree instance. Before invoking it, define the instance's PPP object, initialize a
 * @ref modem_cellular_data object, and invoke @ref MODEM_CELLULAR_DEFINE_AND_INIT_USER_PIPES().
 *
 * @param inst Devicetree instance number.
 * @param vendor_config Pointer to a constant @ref modem_cellular_vendor_config object. Must not be
 *        NULL and must remain valid for the lifetime of the device.
 */
#define MODEM_CELLULAR_DEFINE_INSTANCE(inst, vendor_config)                                        \
	BUILD_ASSERT(vendor_config != NULL, "vendor_config must be non-NULL");                     \
	static const struct modem_cellular_config MODEM_CELLULAR_INST_NAME(config, inst) = {       \
		.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),                                          \
		.vendor = vendor_config,                                                           \
		.power_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_power_gpios, {}),                 \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_reset_gpios, {}),                 \
		.wake_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_wake_gpios, {}),                   \
		.ring_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_ring_gpios, {}),                   \
		.dtr_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_dtr_gpios, {}),                     \
		.autostarts = DT_INST_PROP(inst, autostarts),                                      \
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
		.cmux_no_powersave_handshake =                                                     \
			DT_INST_PROP_OR(inst, cmux_no_powersave_handshake, 0),                     \
		.use_default_pdp_context = DT_INST_PROP_OR(inst, zephyr_use_default_pdp_ctx, 0),   \
		.use_default_apn = DT_INST_PROP_OR(inst, zephyr_use_default_apn, 0),               \
		.cmux_idle_timeout = K_MSEC(DT_INST_PROP_OR(inst, cmux_idle_timeout_ms, 0)),       \
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

/**
 * @brief Descriptor binding a board-supplied init script to a modem instance.
 *
 * Registered with MODEM_CELLULAR_BOARD_INIT_DEFINE.
 */
struct modem_cellular_board_init {
	const struct device *dev;
	const struct modem_chat_script *script;
};

/**
 * @brief Register a board-specific init script for a modem instance.
 *
 * The script runs over the AT control channel after CMUX is established and
 * before APN and network configuration. The driver supplies the completion
 * callback that advances the connect sequence, so the script's own callback
 * is unused.
 *
 * @param node_id Devicetree node identifier of the modem instance.
 * @param _script Pointer to a modem_chat_script defined with
 *                MODEM_CHAT_SCRIPT_DEFINE.
 */
#if defined(CONFIG_MODEM_CELLULAR)
#define MODEM_CELLULAR_BOARD_INIT_DEFINE(node_id, _script)                                         \
	static const STRUCT_SECTION_ITERABLE(                                                      \
		modem_cellular_board_init,                                                         \
		CONCAT(modem_cellular_board_init_, DT_DEP_ORD(node_id))) = {                       \
		.dev = DEVICE_DT_GET(node_id),                                                     \
		.script = (_script),                                                               \
	}
#else
#define MODEM_CELLULAR_BOARD_INIT_DEFINE(node_id, _script)
#endif

/** @} */

/** @} */

/**
 * @addtogroup cellular_interface
 * @{
 */

/**
 * @brief Pause the cellular_modem driver's periodic chat script.
 *
 * Scheduled periodic-script runs are suppressed until
 * cellular_modem_resume_periodic_script() is called. An in-flight script
 * invocation at the time of this call is allowed to complete; suppression
 * takes effect from the next scheduled run.
 *
 * @param dev Cellular device created with @ref MODEM_CELLULAR_DEFINE_INSTANCE(). Must not be NULL.
 *
 * @retval 0 Success.
 * @retval -ENOTSUP Device has no periodic chat script configured.
 * @retval -EINVAL Periodic script is already paused.
 *
 * @see cellular_modem_resume_periodic_script
 */
int cellular_modem_pause_periodic_script(const struct device *dev);

/**
 * @brief Resume the cellular_modem driver's periodic chat script.
 *
 * Re-enables the periodic script. If at least one scheduled run was
 * skipped while paused, the script fires immediately; otherwise the
 * periodic timer restarts at the configured interval.
 *
 * @param dev Cellular device created with @ref MODEM_CELLULAR_DEFINE_INSTANCE(). Must not be NULL.
 *
 * @retval 0 Success.
 * @retval -ENOTSUP Device has no periodic chat script configured.
 * @retval -EINVAL Periodic script is not currently paused.
 *
 * @see cellular_modem_pause_periodic_script
 */
int cellular_modem_resume_periodic_script(const struct device *dev);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CELLULAR_INTERNAL_H_ */
