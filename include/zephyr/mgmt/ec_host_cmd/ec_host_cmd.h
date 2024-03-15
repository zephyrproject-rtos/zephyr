/*
 * Copyright (c) 2020 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MGMT_EC_HOST_CMD_EC_HOST_CMD_H_
#define ZEPHYR_INCLUDE_MGMT_EC_HOST_CMD_EC_HOST_CMD_H_

/**
 * @brief EC Host Command Interface
 * @defgroup ec_host_cmd_interface EC Host Command Interface
 * @ingroup io_interfaces
 * @{
 */

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/mgmt/ec_host_cmd/backend.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/iterable_sections.h>

/**
 * @brief Host command response codes (16-bit).
 */
enum ec_host_cmd_status {
	/** Host command was successful. */
	EC_HOST_CMD_SUCCESS = 0,
	/** The specified command id is not recognized or supported. */
	EC_HOST_CMD_INVALID_COMMAND = 1,
	/** Generic Error. */
	EC_HOST_CMD_ERROR = 2,
	/** One of more of the input request parameters is invalid. */
	EC_HOST_CMD_INVALID_PARAM = 3,
	/** Host command is not permitted. */
	EC_HOST_CMD_ACCESS_DENIED = 4,
	/** Response was invalid (e.g. not version 3 of header). */
	EC_HOST_CMD_INVALID_RESPONSE = 5,
	/** Host command id version unsupported. */
	EC_HOST_CMD_INVALID_VERSION = 6,
	/** Checksum did not match. */
	EC_HOST_CMD_INVALID_CHECKSUM = 7,
	/** A host command is currently being processed. */
	EC_HOST_CMD_IN_PROGRESS = 8,
	/** Requested information is currently unavailable. */
	EC_HOST_CMD_UNAVAILABLE = 9,
	/** Timeout during processing. */
	EC_HOST_CMD_TIMEOUT = 10,
	/** Data or table overflow. */
	EC_HOST_CMD_OVERFLOW = 11,
	/** Header is invalid or unsupported (e.g. not version 3 of header). */
	EC_HOST_CMD_INVALID_HEADER = 12,
	/** Did not receive all expected request data. */
	EC_HOST_CMD_REQUEST_TRUNCATED = 13,
	/** Response was too big to send within one response packet. */
	EC_HOST_CMD_RESPONSE_TOO_BIG = 14,
	/** Error on underlying communication bus. */
	EC_HOST_CMD_BUS_ERROR = 15,
	/** System busy. Should retry later. */
	EC_HOST_CMD_BUSY = 16,
	/** Header version invalid. */
	EC_HOST_CMD_INVALID_HEADER_VERSION = 17,
	/** Header CRC invalid. */
	EC_HOST_CMD_INVALID_HEADER_CRC = 18,
	/** Data CRC invalid. */
	EC_HOST_CMD_INVALID_DATA_CRC = 19,
	/** Can't resend response. */
	EC_HOST_CMD_DUP_UNAVAILABLE = 20,

	EC_HOST_CMD_MAX = UINT16_MAX /* Force enum to be 16 bits. */
} __packed;

enum ec_host_cmd_log_level {
	EC_HOST_CMD_DEBUG_OFF, /* No Host Command debug output */
	EC_HOST_CMD_DEBUG_NORMAL, /* Normal output mode; skips repeated commands */
	EC_HOST_CMD_DEBUG_EVERY, /* Print every command */
	EC_HOST_CMD_DEBUG_PARAMS, /* ... and print params for request/response */
	EC_HOST_CMD_DEBUG_MODES /* Number of host command debug modes */
};

enum ec_host_cmd_state {
	EC_HOST_CMD_STATE_DISABLED = 0,
	EC_HOST_CMD_STATE_RECEIVING,
	EC_HOST_CMD_STATE_PROCESSING,
	EC_HOST_CMD_STATE_SENDING,
};

typedef void (*ec_host_cmd_user_cb_t)(const struct ec_host_cmd_rx_ctx *rx_ctx, void *user_data);
typedef enum ec_host_cmd_status (*ec_host_cmd_in_progress_cb_t)(void *user_data);

struct ec_host_cmd {
	struct ec_host_cmd_rx_ctx rx_ctx;
	struct ec_host_cmd_tx_buf tx;
	struct ec_host_cmd_backend *backend;
	/**
	 * The backend gives rx_ready (by calling the ec_host_cmd_send_receive function),
	 * when data in rx_ctx are ready. The handler takes rx_ready to read data in rx_ctx.
	 */
	struct k_sem rx_ready;
	/** Status of the rx data checked in the ec_host_cmd_send_received function. */
	enum ec_host_cmd_status rx_status;
	/**
	 * User callback after receiving a command. It is called by the ec_host_cmd_send_received
	 * function.
	 */
	ec_host_cmd_user_cb_t user_cb;
	void *user_data;
	enum ec_host_cmd_state state;
#ifdef CONFIG_EC_HOST_CMD_DEDICATED_THREAD
	struct k_thread thread;
#endif /* CONFIG_EC_HOST_CMD_DEDICATED_THREAD */
};

/**
 * @brief Arguments passed into every installed host command handler
 */
struct ec_host_cmd_handler_args {
	/** Reserved for compatibility. */
	void *reserved;
	/** Command identifier. */
	uint16_t command;
	/**
	 * The version of the host command that is being requested. This will
	 * be a value that has been static registered as valid for the handler.
	 */
	uint8_t version;
	/** The incoming data that can be cast to the handlers request type. */
	const void *input_buf;
	/** The number of valid bytes that can be read from @a input_buf. */
	uint16_t input_buf_size;
	/** The data written to this buffer will be send to the host. */
	void *output_buf;
	/** Maximum number of bytes that can be written to the @a output_buf. */
	uint16_t output_buf_max;
	/** Number of bytes of @a output_buf to send to the host. */
	uint16_t output_buf_size;
};

typedef enum ec_host_cmd_status (*ec_host_cmd_handler_cb)(struct ec_host_cmd_handler_args *args);
/**
 * @brief Structure use for statically registering host command handlers
 */
struct ec_host_cmd_handler {
	/** Callback routine to process commands that match @a id. */
	ec_host_cmd_handler_cb handler;
	/** The numerical command id used as the lookup for commands. */
	uint16_t id;
	/**
	 * The bitfield of all versions that the @a handler supports, where
	 * each bit value represents that the @a handler supports that version.
	 * E.g. BIT(0) corresponds to version 0.
	 */
	uint16_t version_mask;
	/**
	 * The minimum @a input_buf_size enforced by the framework before
	 * passing to the handler.
	 */
	uint16_t min_rqt_size;
	/**
	 * The minimum @a output_buf_size enforced by the framework before
	 * passing to the handler.
	 */
	uint16_t min_rsp_size;
};

/**
 * @brief Statically define and register a host command handler.
 *
 * Helper macro to statically define and register a host command handler that
 * has a compile-time-fixed sizes for its both request and response structures.
 *
 * @param _id Id of host command to handle request for.
 * @param _function Name of handler function.
 * @param _version_mask The bitfield of all versions that the @a _function
 *        supports. E.g. BIT(0) corresponds to version 0.
 * @param _request_type The datatype of the request parameters for @a _function.
 * @param _response_type The datatype of the response parameters for
 *        @a _function.
 */
#define EC_HOST_CMD_HANDLER(_id, _function, _version_mask, _request_type, _response_type)          \
	const STRUCT_SECTION_ITERABLE(ec_host_cmd_handler, __cmd##_id) = {                         \
		.handler = _function,                                                              \
		.id = _id,                                                                         \
		.version_mask = _version_mask,                                                     \
		.min_rqt_size = sizeof(_request_type),                                             \
		.min_rsp_size = sizeof(_response_type),                                            \
	}

/**
 * @brief Statically define and register a host command handler without sizes.
 *
 * Helper macro to statically define and register a host command handler whose
 * request or response structure size is not known as compile time.
 *
 * @param _id Id of host command to handle request for.
 * @param _function Name of handler function.
 * @param _version_mask The bitfield of all versions that the @a _function
 *        supports. E.g. BIT(0) corresponds to version 0.
 */
#define EC_HOST_CMD_HANDLER_UNBOUND(_id, _function, _version_mask)                                 \
	const STRUCT_SECTION_ITERABLE(ec_host_cmd_handler, __cmd##_id) = {                         \
		.handler = _function,                                                              \
		.id = _id,                                                                         \
		.version_mask = _version_mask,                                                     \
		.min_rqt_size = 0,                                                                 \
		.min_rsp_size = 0,                                                                 \
	}

/**
 * @brief Header for requests from host to embedded controller
 *
 * Represent the over-the-wire header in LE format for host command requests.
 * This represent version 3 of the host command header. The requests are always
 * sent from host to embedded controller.
 */
struct ec_host_cmd_request_header {
	/**
	 * Should be 3. The EC will return EC_HOST_CMD_INVALID_HEADER if it
	 * receives a header with a version it doesn't know how to parse.
	 */
	uint8_t prtcl_ver;
	/**
	 * Checksum of response and data; sum of all bytes including checksum.
	 * Should total to 0.
	 */
	uint8_t checksum;
	/** Id of command that is being sent. */
	uint16_t cmd_id;
	/**
	 * Version of the specific @a cmd_id being requested. Valid
	 * versions start at 0.
	 */
	uint8_t cmd_ver;
	/** Unused byte in current protocol version; set to 0. */
	uint8_t reserved;
	/** Length of data which follows this header. */
	uint16_t data_len;
} __packed;

/**
 * @brief Header for responses from embedded controller to host
 *
 * Represent the over-the-wire header in LE format for host command responses.
 * This represent version 3 of the host command header. Responses are always
 * sent from embedded controller to host.
 */
struct ec_host_cmd_response_header {
	/** Should be 3. */
	uint8_t prtcl_ver;
	/**
	 * Checksum of response and data; sum of all bytes including checksum.
	 * Should total to 0.
	 */
	uint8_t checksum;
	/** A @a ec_host_cmd_status response code for specific command. */
	uint16_t result;
	/** Length of data which follows this header. */
	uint16_t data_len;
	/** Unused bytes in current protocol version; set to 0. */
	uint16_t reserved;
} __packed;

/**
 * @brief Initialize the host command subsystem
 *
 * This routine initializes the host command subsystem. It includes initialization
 * of a backend and the handler.
 * When the application configures the zephyr,host-cmd-espi-backend/zephyr,host-cmd-shi-backend/
 * zephyr,host-cmd-uart-backend chosen node and @kconfig{CONFIG_EC_HOST_CMD_INITIALIZE_AT_BOOT} is
 * set, the chosen backend automatically calls this routine at
 * @kconfig{CONFIG_EC_HOST_CMD_INIT_PRIORITY}. Applications that require a run-time selection of the
 * backend must set @kconfig{CONFIG_EC_HOST_CMD_INITIALIZE_AT_BOOT} to n and must explicitly call
 * this routine.
 *
 * @param[in] backend        Pointer to the backend structure to initialize.
 *
 * @retval 0 if successful
 */
int ec_host_cmd_init(struct ec_host_cmd_backend *backend);

/**
 * @brief Send the host command response
 *
 * This routine sends the host command response. It should be used to send IN_PROGRESS status or
 * if the host command handler doesn't return e.g. reboot command.
 *
 * @param[in] status        Host command status to be sent.
 * @param[in] args          Pointer of a structure passed to the handler.
 *
 * @retval 0 if successful.
 */
int ec_host_cmd_send_response(enum ec_host_cmd_status status,
			      const struct ec_host_cmd_handler_args *args);

/**
 * @brief Signal a new host command
 *
 * Signal that a new host command has been received. The function should be called by a backend
 * after copying data to the rx buffer and setting the length.
 */
void ec_host_cmd_rx_notify(void);

/**
 * @brief Install a user callback for receiving a host command
 *
 * It allows installing a custom procedure needed by a user after receiving a command.
 *
 * @param[in] cb          A callback to be installed.
 * @param[in] user_data   User data to be passed to the callback.
 */
void ec_host_cmd_set_user_cb(ec_host_cmd_user_cb_t cb, void *user_data);

/**
 * @brief Get the main ec host command structure
 *
 * This routine returns a pointer to the main host command structure.
 * It allows the application code to get inside information for any reason e.g.
 * the host command thread id.
 *
 * @retval A pointer to the main host command structure
 */
const struct ec_host_cmd *ec_host_cmd_get_hc(void);

#ifndef CONFIG_EC_HOST_CMD_DEDICATED_THREAD
/**
 * @brief The thread function for Host Command subsystem
 *
 * This routine calls the Host Command thread entry function. If
 * @kconfig{CONFIG_EC_HOST_CMD_DEDICATED_THREAD} is not defined, a new thread is not created,
 * and this function has to be called by application code. It doesn't return.
 */
FUNC_NORETURN void ec_host_cmd_task(void);
#endif

#ifdef CONFIG_EC_HOST_CMD_IN_PROGRESS_STATUS
/**
 * @brief Check if a Host Command that sent EC_HOST_CMD_IN_PROGRESS status has ended.
 *
 * A Host Command that sends EC_HOST_CMD_IN_PROGRESS status doesn't send a final result.
 * The final result can be get with the ec_host_cmd_send_in_progress_status function.
 *
 * @retval true if the Host Command endded
 */
bool ec_host_cmd_send_in_progress_ended(void);

/**
 * @brief Get final result of a last Host Command that has sent EC_HOST_CMD_IN_PROGRESS status.
 *
 * A Host Command that sends EC_HOST_CMD_IN_PROGRESS status doesn't send a final result.
 * Get the saved status with this function. The status can be get only once. Futher calls return
 * EC_HOST_CMD_UNAVAILABLE.
 *
 * Saving status of Host Commands that send response data is not supported.
 *
 * @retval The final status or EC_HOST_CMD_UNAVAILABLE if not available.
 */
enum ec_host_cmd_status ec_host_cmd_send_in_progress_status(void);

/**
 * @brief Continue processing a handler in callback after returning EC_HOST_CMD_IN_PROGRESS.
 *
 * A Host Command handler may return the EC_HOST_CMD_IN_PROGRESS, but needs to continue work.
 * This function should be called before returning EC_HOST_CMD_IN_PROGRESS with a callback that
 * will be executed. The return status of the callback will be stored and can be get with the
 * ec_host_cmd_send_in_progress_status function. The ec_host_cmd_send_in_progress_ended function
 * can be used to check if the callback has ended.
 *
 * @param[in] cb          A callback to be called after returning from a command handler.
 * @param[in] user_data   User data to be passed to the callback.
 *
 * @retval EC_HOST_CMD_BUSY if any command is already in progress, EC_HOST_CMD_SUCCESS otherwise
 */
enum ec_host_cmd_status ec_host_cmd_send_in_progress_continue(ec_host_cmd_in_progress_cb_t cb,
							      void *user_data);
#endif /* CONFIG_EC_HOST_CMD_IN_PROGRESS_STATUS */

/**
 * @brief Add a suppressed command.
 *
 * Suppressed commands are not logged. Add a command to be suppressed.
 *
 * @param[in] cmd_id        A command id to be suppressed.
 *
 * @retval 0 if successful, -EIO if exceeded max number of suppressed commands.
 */
int ec_host_cmd_add_suppressed(uint16_t cmd_id);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_MGMT_EC_HOST_CMD_EC_HOST_CMD_H_ */
