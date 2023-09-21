/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SEU_H_
#define ZEPHYR_INCLUDE_SEU_H_

#ifdef __cplusplus
extern "C" {
#endif

#define SEU_MAX_FRAME 2

/**
 * Structure to hold user input to inject safe SEU error frame data.
 */
struct inject_safe_seu_error_frame {
	/* Sector address */
	uint8_t sector_address;
	/* Inject type */
	uint8_t inject_type: 2;
	/* Number of injection */
	uint8_t number_of_injection: 4;
	/* CRAM selection 0 */
	uint8_t cram_sel_0: 4;
	/* CRAM selection 1 */
	uint8_t cram_sel_1: 4;
};

/**
 * Structure for SEU frame data
 */
struct seu_frame {
	/* Row frame data */
	uint16_t row_frame: 12;
	/* Error bit position */
	uint16_t bit_pos: 13;
};

/**
 * Union for SEU error frame data
 */
union seu_error_frame {
	struct seu_frame frame_data;
	uint32_t seu_frame_data;
};

/**
 * Enum for inject SEU error type.
 */
enum inject_seu_error {
	INJECT_SINGLE_BIT_SEU_ERROR = 0,
	INJECT_DOUBLE_BIT_SEU_ERROR
};

/**
 * Structure to hold user input to inject SEU error frame data.
 */
struct inject_seu_error_frame {
	/* Select error Type */
	enum inject_seu_error error_inject;
	/* Sector address */
	uint8_t sector_address;
	/* Frame data */
	union seu_error_frame frame[SEU_MAX_FRAME];
};

/**
 * Enum for inject ECC error type.
 */
enum inject_ecc_error {
	INJECT_SINGLE_BIT_ECC_ERROR = 1,
	INJECT_DOUBLE_BIT_ECC_ERROR
};

/**
 * Structure to hold user input to inject ECC error frame data.
 */
struct inject_ecc_error_frame {
	/* ECC error type */
	enum inject_ecc_error inject_type;
	/* Inject error to RAM ID */
	uint8_t ram_id: 5;
	/* Sector address */
	uint8_t sector_address;
};

/**
 * Structure to hold SEU error status data.
 */
struct seu_err_data {
	/* Sub-error Type */
	uint8_t sub_error_type: 3;
	/* Sector address */
	uint8_t sector_addr;
	/* Correction Status */
	uint8_t correction_status: 1;
	/* Row frame Index */
	uint16_t row_frame_index: 12;
	/* Error bit position */
	uint16_t bit_position: 13;
};

/**
 * Structure to hold ECC error status data.
 */
struct ecc_err_data {
	/* Sub-error Type */
	uint8_t sub_error_type: 3;
	/* Sector address */
	uint8_t sector_addr;
	/* Correction status */
	uint8_t correction_status: 1;
	/* RAM ID or Error code */
	uint16_t ram_id_error;
};

enum seu_reg_mode {
	SEU_ERROR_MODE = 0,
	ECC_ERROR_MODE,
	MISC_CNT_ERROR_MODE,
	PMF_ERROR_MODE,
	MISC_SDM_ERROR_MODE,
	MISC_EMIF_ERROR_MODE,
	MAX_SEU_ERROR_MODE
};

/**
 * Structure to hold misc error status data.
 */
struct misc_err_data {
	/* Sub-error type */
	uint8_t sub_error_type: 3;
	/* Sector address */
	uint8_t sector_addr;
	/* Correction status */
	uint8_t correction_status: 1;
	/* CNT type */
	uint8_t cnt_type: 4;
	/* Status error */
	uint16_t status_code: 12;
};

/**
 * Structure to hold PMF error status data.
 */
struct pmf_err_data {
	/* Sub-error type */
	uint8_t sub_error_type: 3;
	/* Sector address */
	uint8_t sector_addr;
	/* Correction status */
	uint8_t correction_status: 1;
	/* Status error */
	uint16_t status_code: 12;
};

/**
 * Structure to hold misc SDM error status data.
 */
struct misc_sdm_err_data {
	/* Sub-error type */
	uint8_t sub_error_type: 3;
	/* Sector address */
	uint8_t sector_addr;
	/* Correction status */
	uint8_t correction_status: 1;
	/* Watchdog error code */
	uint16_t wdt_code: 12;
};

/**
 * Structure to hold EMIF error status data.
 */
struct emif_err_data {
	/* Sector address */
	uint8_t sector_addr;
	/* EMIF ID */
	uint8_t emif_id;
	/* Source ID */
	uint8_t source_id: 7;
	/* EMIF error type */
	uint8_t emif_error_type: 4;
	/* MSB of DDR address */
	uint8_t ddr_addr_msb: 6;
	/* LSB of DDR address */
	uint32_t ddr_addr_lsb;
};

/**
 * Structure to hold Single Event Upset (SEU) status data.
 */
struct seu_statistics_data {
	/* SEU cycles */
	uint32_t t_seu_cycle;
	/* SEU detected */
	uint32_t t_seu_detect;
	/* SEU corrected */
	uint32_t t_seu_correct;
	/* Injected SEU detected*/
	uint32_t t_seu_inject_detect;
	/* Poll interval */
	uint32_t t_sdm_seu_poll_interval;
	/* Overhead due to pin toggling */
	uint32_t t_sdm_seu_pin_toggle_overhead;
};

/** @brief Single Event Upsets(SEU) callback function prototype for response after completion
 *
 * On success , response is returned via a callback to the user.
 *
 * @param err_data error data to user
 *
 */
typedef void (*seu_isr_callback_t)(void *err_data);

/**
 * @brief Callback API to check the response of seu_callback_function_register
 * See @a seu_callback_function_register() for argument description
 */
typedef int (*seu_callback_function_register_t)(const struct device *dev, seu_isr_callback_t func,
						enum seu_reg_mode mode, uint32_t *client);

/**
 * @brief Callback API to check the response of seu_callback_function_enable
 * See @a seu_callback_function_enable() for argument description
 */
typedef int (*seu_callback_function_enable_t)(const struct device *dev, uint32_t client);

/**
 * @brief Callback API to check the response of seu_callback_function_disable
 * See @a seu_callback_function_disable() for argument description
 */
typedef int (*seu_callback_function_disable_t)(const struct device *dev, uint32_t client);

/**
 * @brief Callback API to insert safe SEU error
 * See @a insert_safe_seu_error() for argument description
 */
typedef int (*insert_safe_seu_error_t)(const struct device *dev,
				       struct inject_safe_seu_error_frame *error_frame);

/**
 * @brief Callback API to insert SEU error
 * See @a insert_seu_error() for argument description
 */
typedef int (*insert_seu_error_t)(const struct device *dev,
				  struct inject_seu_error_frame *error_frame);

/**
 * @brief Callback API to insert ECC error
 * See @a insert_ecc_error() for argument description
 */
typedef int (*insert_ecc_error_t)(const struct device *dev,
				  struct inject_ecc_error_frame *ecc_error_frame);

/**
 * @brief Callback API to check the response of read SEU statistics
 * See @a read_seu_statistics() for argument description
 */
typedef int (*read_seu_statistics_t)(const struct device *dev, uint8_t sector,
				     struct seu_statistics_data *seu_statistics);

/**
 * @brief  API structure for SEU driver.
 *
 */
__subsystem struct seu_api {
	seu_callback_function_register_t seu_callback_function_register;
	seu_callback_function_enable_t seu_callback_function_enable;
	seu_callback_function_disable_t seu_callback_function_disable;
	insert_safe_seu_error_t insert_safe_seu_error;
	insert_seu_error_t insert_seu_error;
	insert_ecc_error_t insert_ecc_error;
	read_seu_statistics_t read_seu_statistics;
};

/** @brief Register a callback function to send Single Event Upsets (SEU) interrupts error to
 * user.
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param func    Pointer to the SEU error callback function.
 * @param mode    SEU error mode value, specifying the type of SEU error to trigger the callback.
 * @param client  Pointer to a client number to use enable and disable callback
 * function.
 *
 * @return        0 if the registration is successful, an error code otherwise.
 */
__syscall int seu_callback_function_register(const struct device *dev, seu_isr_callback_t func,
					     enum seu_reg_mode mode, uint32_t *client);
static inline int z_impl_seu_callback_function_register(const struct device *dev,
							seu_isr_callback_t func,
							enum seu_reg_mode mode, uint32_t *client)
{
	__ASSERT(dev, "Driver instance shouldn't be NULL");

	const struct seu_api *api = (struct seu_api *)dev->api;

	return api->seu_callback_function_register(dev, func, mode, client);
}

/**
 * Enable the Single Event Upsets (SEU) callback function for a specific client.
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param client  client number to enable callback function.
 *
 * @return        0 if the callback function is successfully enabled, an error code otherwise.
 */
__syscall int seu_callback_function_enable(const struct device *dev, uint32_t client);
static inline int z_impl_seu_callback_function_enable(const struct device *dev, uint32_t client)
{
	__ASSERT(dev, "Driver instance shouldn't be NULL");

	const struct seu_api *api = (struct seu_api *)dev->api;

	return api->seu_callback_function_enable(dev, client);
}

/**
 * Disable the Single Event Upsets (SEU) callback function for a specific client.
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param client  client number to disable callback function.
 *
 * @return        0 if the callback function is successfully enabled, an error code otherwise.
 */
__syscall int seu_callback_function_disable(const struct device *dev, uint32_t client);
static inline int z_impl_seu_callback_function_disable(const struct device *dev, uint32_t client)
{
	__ASSERT(dev, "Driver instance shouldn't be NULL");

	const struct seu_api *api = (struct seu_api *)dev->api;

	return api->seu_callback_function_disable(dev, client);
}

/**
 * Insert a safe Single Event Upset (SEU) error frame API into the system.
 *
 * @param dev          Pointer to the device structure for the driver instance.
 * @param error_frame  Pointer to the error frame structure containing SEU error information.
 *
 * @return             0 if the SEU error frame is successfully inserted, an error code otherwise.
 */
__syscall int insert_safe_seu_error(const struct device *dev,
				    struct inject_safe_seu_error_frame *error_frame);
static inline int z_impl_insert_safe_seu_error(const struct device *dev,
					       struct inject_safe_seu_error_frame *error_frame)
{
	__ASSERT(dev, "Driver instance shouldn't be NULL");

	const struct seu_api *api = (struct seu_api *)dev->api;

	__ASSERT(error_frame, "Error frame shouldn't be NULL");

	return api->insert_safe_seu_error(dev, error_frame);
}

/**
 * Insert a Traditional Single Event Upset (SEU) error frame API into the system.
 *
 * @param dev          Pointer to the device structure for the driver instance.
 * @param error_frame  Pointer to the error frame structure containing SEU error information.
 *
 * @return             0 if the SEU error frame is successfully inserted, an error code otherwise.
 */
__syscall int insert_seu_error(const struct device *dev,
			       struct inject_seu_error_frame *error_frame);
static inline int z_impl_insert_seu_error(const struct device *dev,
					  struct inject_seu_error_frame *error_frame)
{
	__ASSERT(dev, "Driver instance shouldn't be NULL");

	const struct seu_api *api = (struct seu_api *)dev->api;

	__ASSERT(error_frame, "Error frame shouldn't be NULL");

	return api->insert_seu_error(dev, error_frame);
}

/**
 * Insert a  ECC error frame API into the system.
 *
 * @param dev          Pointer to the device structure for the driver instance.
 * @param error_frame  Pointer to the error frame structure containing ECC error information.
 *
 * @return             0 if the SEU error frame is successfully inserted, an error code otherwise.
 */
__syscall int insert_ecc_error(const struct device *dev,
			       struct inject_ecc_error_frame *ecc_error_frame);
static inline int z_impl_insert_ecc_error(const struct device *dev,
					  struct inject_ecc_error_frame *ecc_error_frame)
{
	__ASSERT(dev, "Driver instance shouldn't be NULL");

	const struct seu_api *api = (struct seu_api *)dev->api;

	__ASSERT(ecc_error_frame, "ECC error frame shouldn't be NULL");

	return api->insert_ecc_error(dev, ecc_error_frame);
}

/**
 * Read the Single Event Upset (SEU) status for a specific sector.
 *
 * @param dev          Pointer to the device structure for the driver instance.
 * @param sector       The sector identifier for which SEU status is to be read.
 * @param seu_statistics   Pointer to the structure where SEU statistics data will be stored.
 *
 * @return             0 if SEU status is successfully read, an error code otherwise.
 */
__syscall int read_seu_statistics(const struct device *dev, uint8_t sector,
				  struct seu_statistics_data *seu_statistics);
static inline int z_impl_read_seu_statistics(const struct device *dev, uint8_t sector,
					     struct seu_statistics_data *seu_statistics)
{
	__ASSERT(dev, "Driver instance shouldn't be NULL");

	const struct seu_api *api = (struct seu_api *)dev->api;

	__ASSERT(seu_statistics, "SEU statistics shouldn't be NULL");

	return api->read_seu_statistics(dev, sector, seu_statistics);
}

#include <syscalls/seu.h>

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SEU_H_ */
