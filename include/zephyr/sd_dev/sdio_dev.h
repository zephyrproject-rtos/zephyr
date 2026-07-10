/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SDIO Device-Side Driver APIs
 *
 * This file provides APIs for SDIO device-side drivers, including
 * device initialization, function management, and capability detection.
 */

#ifndef ZEPHYR_INCLUDE_SD_SDIO_DEV_H_
#define ZEPHYR_INCLUDE_SD_SDIO_DEV_H_

#include <zephyr/kernel.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/drivers/sd_dev.h>

/**
 * @brief SDIO Device APIs
 * @defgroup sdio_dev SDIO Device APIs
 * @ingroup sd_dev_interface
 * @{
 */

/**
 * @cond INTERNAL_HIDDEN
 */

/* Forward declarations */
struct sd_dev_card;
struct sdio_dev_cfg;

/**
 * @endcond
 */

/**
 * @name SDIO Function Management
 * @{
 */

/**
 * @brief SDIO function state
 *
 * Defines the operational state of an SDIO function.
 */
enum sdio_func_state {
	/** SDIO function is disabled */
	SDIO_FUNC_DISABLED = 0,
	/** SDIO function is enabled and ready to transfer */
	SDIO_FUNC_READY,
};

/**
 * @brief SDIO function context
 *
 * This structure represents a single SDIO function (1-7) and contains
 * its configuration, state, and runtime data.
 */
struct sdio_dev_func {
	/** Function number */
	enum sdio_func_num fn;
	/** Parent SDIO device */
	struct sdio_dev *sdio;
	/** Current function state */
	int state;
	/** Function Basic Register (FBR) address */
	uint32_t fbr_addr;
	/** Card Information Structure (CIS) address */
	uint32_t cis_addr;
	/** Function code (type identifier) */
	uint8_t func_code;
	/** Block size for block transfers */
	uint16_t block_size;
	/** FIFO for received packets */
	struct k_fifo rx_fifo;
	/** Function-specific private data */
	void *priv;
};

/** @} */

/**
 * @name SDIO Device Management
 * @{
 */

/**
 * @brief SDIO device context
 *
 * This structure represents the complete SDIO device including all
 * functions and device-level configuration.
 */
struct sdio_dev {
	/** Parent SD card context */
	struct sd_dev_card *card;
	/** Array of function pointers (functions 1-7) */
	struct sdio_dev_func *funcs[CONFIG_SDIO_DEV_MAX_FUNCS];
	/** Card Common Control Registers (CCCR) address */
	uint32_t cccr_addr;
	/** Number of functions (1-7) */
	uint8_t num_funcs;
	/** Vendor ID */
	uint16_t vendor_id;
	/** Device ID */
	uint16_t device_id;
	/** SDIO device config */
	struct sdio_dev_cfg *cfg;
	/** Device-specific private data */
	const void *priv;
};

/** @} */

/**
 * @name SDIO Timing Constants
 * @{
 */

/** Polling interval for I/O ready status in microseconds */
#define SDIO_IO_READY_POLL_US     100
/** Timeout for I/O ready status in microseconds (100 ms) */
#define SDIO_IO_READY_TIMEOUT_US (100 * 1000)

/** @} */

/**
 * @brief SDIO function code
 *
 * Standard function codes defined by the SDIO specification.
 * These codes identify the type of function (FBR[0x00]).
 */
enum sdio_fn_code {
	/** No specific function */
	SDIO_FUNC_CODE_NONE       = 0x00,
	/** UART function */
	SDIO_FUNC_CODE_UART       = 0x01,
	/** Bluetooth function */
	SDIO_FUNC_CODE_BT         = 0x02,
	/** GPS function */
	SDIO_FUNC_CODE_GPS        = 0x03,
	/** Camera function */
	SDIO_FUNC_CODE_CAMERA     = 0x04,
	/** PHS function */
	SDIO_FUNC_CODE_PHS        = 0x05,
	/** WLAN (WiFi) function */
	SDIO_FUNC_CODE_WLAN       = 0x0C,
	/** Vendor-specific function code minimum */
	SDIO_FUNC_CODE_VENDOR_MIN = 0x80,
	/** Vendor-specific function code maximum */
	SDIO_FUNC_CODE_VENDOR_MAX = 0xFF
};

/**
 * @name SDIO Device Initialization
 * @{
 */

/**
 * @brief Initialize SDIO device
 *
 * This function initializes an SDIO device with the provided configuration.
 *
 * @param sdio Pointer to SDIO device structure
 * @param cfg Pointer to SDIO device configuration
 *
 * @retval 0 on success
 * @retval -EINVAL if sdio or cfg is NULL
 * @retval negative errno code on failure
 */
int sdio_dev_init(struct sdio_dev *sdio,
		  const struct sdio_dev_cfg *cfg);

/**
 * @brief Deinitialize SDIO device
 *
 * This function deinitializes the SDIO device by cleaning up all allocated
 * functions, releasing memory, and resetting device state to initial values.
 * It should be called when the SDIO device is no longer needed or before
 * reinitializing the device.
 *
 * The function will:
 * - Deinitialize all registered SDIO functions
 * - Free allocated memory for function structures
 * - Reset all SDIO device parameters to default values
 *
 * @param sdio Pointer to the SDIO device structure
 *
 * @retval 0 on success
 * @retval -EINVAL if sdio pointer is NULL
 */
int sdio_dev_deinit(struct sdio_dev *sdio);

/**
 * @brief Initialize SDIO function
 *
 * This function initializes a single SDIO function.
 *
 * @param func Pointer to SDIO function structure
 *
 * @retval 0 on success
 * @retval -EINVAL if func is NULL
 * @retval negative errno code on failure
 */
int sdio_dev_func_init(struct sdio_dev_func *func);

/** @} */

/**
 * @name SDIO Device Status
 * @{
 */

/**
 * @brief Check if SDIO device is enumerated
 *
 * @param sdio Pointer to SDIO device structure
 *
 * @retval 1 if device is enumerated
 * @retval 0 if device is not enumerated
 * @retval negative errno code on failure
 */
int sdio_dev_is_enumed(struct sdio_dev *sdio);

/**
 * @brief Configure CIS table for SDIO device
 *
 * This function configures the Card Information Structure (CIS) table
 * for the SDIO device.
 *
 * @param sdio Pointer to SDIO device structure
 *
 * @retval 0 on success
 * @retval negative errno code on failure
 */
int sdio_dev_cis_table_configurate(struct sdio_dev *sdio);

/**
 * @brief Get CCCR version
 *
 * This function reads the CCCR revision from the CCCR register.
 *
 * @param sdio Pointer to SDIO device structure
 *
 * @retval CCCR version number (0-15)
 * @retval -EINVAL if sdio is NULL or on read error
 */
int sdio_dev_get_cccr_version(struct sdio_dev *sdio);

/**
 * @brief Set CCCR version
 *
 * This function sets the CCCR revision in the CCCR register and verifies
 * the change.
 *
 * @param sdio Pointer to SDIO device structure
 * @param version CCCR version to set (0-15)
 *
 * @retval 0 on success
 * @retval -EINVAL if sdio is NULL or version is out of range
 * @retval -EACCES if verification fails
 * @retval negative errno code on other failures
 */
int sdio_dev_set_cccr_version(struct sdio_dev *sdio, uint8_t version);

/**
 * @brief Get SDIO version
 *
 * This function reads the SDIO specification version from the CCCR register.
 *
 * @param sdio Pointer to SDIO device structure
 *
 * @retval SDIO version number (0-15)
 * @retval -EINVAL if sdio is NULL or on read error
 */
int sdio_dev_get_sdio_version(struct sdio_dev *sdio);

/**
 * @brief Set SDIO version
 *
 * This function sets the SDIO specification version in the CCCR register
 * and verifies the change.
 *
 * @param sdio Pointer to SDIO device structure
 * @param version SDIO version to set (0-15)
 *
 * @retval 0 on success
 * @retval -EINVAL if sdio is NULL or version is out of range
 * @retval -EACCES if verification fails
 * @retval negative errno code on other failures
 */
int sdio_dev_set_sdio_version(struct sdio_dev *sdio, int version);

/**
 * @brief Set SD specification version
 *
 * This function sets the SD specification version in the CCCR SD register
 * and verifies the change.
 *
 * @param sdio Pointer to SDIO device structure
 * @param version SD specification version to set (0-15)
 *
 * @retval 0 on success
 * @retval -EINVAL if sdio is NULL or version is out of range
 * @retval -EACCES if verification fails
 * @retval negative errno code on other failures
 */
int sdio_dev_set_spec_version(struct sdio_dev *sdio, uint8_t version);

/**
 * @brief Get SD specification version
 *
 * This function reads the SD specification version from the CCCR SD register.
 *
 * @param sdio Pointer to SDIO device structure
 *
 * @retval SD specification version (0-15)
 * @retval -EINVAL if sdio is NULL or on read error
 */
int sdio_dev_get_spec_version(struct sdio_dev *sdio);

/**
 * @brief Get bus width
 *
 * This function reads the current bus width configuration from the CCCR
 * bus interface register.
 *
 * @param sdio Pointer to SDIO device structure
 *
 * @retval SDIO_CCCR_BUS_IF_WIDTH_1_BIT for 1-bit bus width
 * @retval SDIO_CCCR_BUS_IF_WIDTH_4_BIT for 4-bit bus width
 * @retval SDIO_CCCR_BUS_IF_WIDTH_8_BIT for 8-bit bus width
 * @retval -1 on error
 */
int sdio_dev_get_bus_width(struct sdio_dev *sdio);

/**
 * @brief Check if master interrupt is enabled
 *
 * This function checks the master interrupt enable bit in the CCCR
 * interrupt enable register.
 *
 * @param sdio Pointer to SDIO device structure
 *
 * @retval true if master interrupt is enabled
 * @retval false if master interrupt is disabled or on error
 */
bool sdio_dev_master_intr_is_enable(const struct sdio_dev *sdio);

/**
 * @brief Set bus width
 *
 * This function sets the bus width configuration in the CCCR bus interface
 * register and verifies the change.
 *
 * @param sdio Pointer to SDIO device structure
 * @param width Desired bus width (1 or 4)
 *
 * @retval 0 on success
 * @retval -EINVAL if sdio is NULL or width is invalid
 * @retval -EACCES if verification fails
 * @retval negative errno code on other failures
 */
int sdio_dev_set_bus_width(struct sdio_dev *sdio, uint8_t width);

/**
 * @brief Set CSPI interrupt mode
 *
 * This function enables or disables the CSPI (Continuous SPI) interrupt
 * mode in the bus interface register.
 *
 * @param sdio Pointer to SDIO device structure
 * @param cspi_int 1 to enable CSPI interrupt, 0 to disable
 *
 * @retval 0 on success
 * @retval -EINVAL if sdio is NULL
 * @retval -EACCES if verification fails
 * @retval negative errno code on other failures
 */
int sdio_dev_set_cspi_int(struct sdio_dev *sdio, int cspi_int);

/**
 * @brief Get CSPI interrupt mode
 *
 * This function reads the CSPI interrupt mode from the bus interface register.
 *
 * @param sdio Pointer to SDIO device structure
 *
 * @retval 1 if CSPI interrupt is enabled
 * @retval 0 if CSPI interrupt is disabled
 * @retval -EINVAL if sdio is NULL or on read error
 */
int sdio_dev_get_cspi_int(struct sdio_dev *sdio);

/**
 * @brief Get multi-block support capability
 *
 * This function reads the multi-block support capability from the CCCR
 * capabilities register.
 *
 * @param sdio Pointer to SDIO device structure
 *
 * @retval 1 if multi-block is supported
 * @retval 0 if multi-block is not supported
 * @retval -EINVAL if sdio is NULL or on read error
 */
int sdio_dev_get_multi_block(struct sdio_dev *sdio);

/**
 * @brief Set multi-block support capability
 *
 * This function sets the multi-block support capability in the CCCR
 * capabilities register and verifies the change.
 *
 * @param sdio Pointer to SDIO device structure
 * @param multi_block 1 to enable multi-block, 0 to disable
 *
 * @retval 0 on success
 * @retval -EINVAL if sdio is NULL
 * @retval -EACCES if verification fails
 * @retval negative errno code on other failures
 */
int sdio_dev_set_multi_block(struct sdio_dev *sdio, int multi_block);

/**
 * @brief Get low-speed card capability
 *
 * This function reads the low-speed card capability from the CCCR
 * capabilities register.
 *
 * @param sdio Pointer to SDIO device structure
 *
 * @retval 1 if low-speed is supported
 * @retval 0 if low-speed is not supported
 * @retval -EINVAL if sdio is NULL or on read error
 */
int sdio_dev_get_low_speed(struct sdio_dev *sdio);

/**
 * @brief Set low-speed card capability
 *
 * This function sets the low-speed card capability in the CCCR
 * capabilities register and verifies the change.
 *
 * @param sdio Pointer to SDIO device structure
 * @param low_speed 1 to enable low-speed, 0 to disable
 *
 * @retval 0 on success
 * @retval -EINVAL if sdio is NULL
 * @retval -EACCES if verification fails
 * @retval negative errno code on other failures
 */
int sdio_dev_set_low_speed(struct sdio_dev *sdio, int low_speed);

/**
 * @brief Check if high-speed is supported
 *
 * This function reads the high-speed support capability from the CCCR
 * speed register.
 *
 * @param sdio Pointer to SDIO device structure
 *
 * @retval 1 if high-speed is supported
 * @retval 0 if high-speed is not supported
 * @retval -EINVAL if sdio is NULL or on read error
 */
int sdio_dev_support_high_speed(struct sdio_dev *sdio);

/**
 * @brief Set high-speed support
 *
 * This function enables or disables high-speed support in the CCCR
 * speed register and verifies the change.
 *
 * @param sdio Pointer to SDIO device structure
 * @param high_speed High-speed mode to set (0-7)
 *
 * @retval 0 on success
 * @retval -EINVAL if sdio is NULL or high_speed is out of range
 * @retval -EACCES if verification fails
 * @retval negative errno code on other failures
 */
int sdio_dev_set_high_speed(struct sdio_dev *sdio, uint8_t high_speed);

/**
 * @brief Get UHS mode
 *
 * This function reads the UHS (Ultra High Speed) mode from the CCCR
 * UHS register.
 *
 * @param sdio Pointer to SDIO device structure
 *
 * @retval UHS mode value (0-7)
 * @retval -EINVAL if sdio is NULL or on read error
 */
int sdio_dev_get_uhs(struct sdio_dev *sdio);

/**
 * @brief Set UHS mode
 *
 * This function sets the UHS (Ultra High Speed) mode in the CCCR
 * UHS register and verifies the change.
 *
 * @param sdio Pointer to SDIO device structure
 * @param uhs_mode UHS mode to set (0-7)
 *
 * @retval 0 on success
 * @retval -EINVAL if sdio is NULL or uhs_mode is out of range
 * @retval -EACCES if verification fails
 * @retval negative errno code on other failures
 */
int sdio_dev_set_uhs(struct sdio_dev *sdio, uint8_t uhs_mode);

/**
 * @brief Get bus speed
 *
 * This function reads the current bus speed setting from the CCCR
 * speed register.
 *
 * @param sdio Pointer to SDIO device structure
 *
 * @retval Bus speed select value
 * @retval SDIO_CCCR_SPEED_SHS on error or if high-speed not supported
 */
int sdio_dev_get_bus_speed(struct sdio_dev *sdio);

/**
 * @brief Get drive strength
 *
 * This function reads the drive strength setting from the CCCR
 * drive strength register.
 *
 * @param sdio Pointer to SDIO device structure
 *
 * @retval Drive strength value (0-7)
 * @retval -EINVAL if sdio is NULL or on read error
 */
int sdio_dev_get_strength(struct sdio_dev *sdio);

/**
 * @brief Set drive strength
 *
 * This function sets the drive strength in the CCCR drive strength
 * register and verifies the change.
 *
 * @param sdio Pointer to SDIO device structure
 * @param strength Drive strength to set (0-7)
 *
 * @retval 0 on success
 * @retval -EINVAL if sdio is NULL or strength is out of range
 * @retval -EACCES if verification fails
 * @retval negative errno code on other failures
 */
int sdio_dev_set_strength(struct sdio_dev *sdio, uint8_t strength);

/**
 * @brief Set asynchronous interrupt mode
 *
 * This function enables or disables asynchronous interrupt support
 * in the CCCR interrupt extension register.
 *
 * @param sdio Pointer to SDIO device structure
 * @param aync_int 1 to enable asynchronous interrupt, 0 to disable
 *
 * @retval 0 on success
 * @retval -EINVAL if sdio is NULL
 * @retval -EACCES if verification fails
 * @retval negative errno code on other failures
 */
int sdio_dev_set_aync_intr(struct sdio_dev *sdio, int aync_int);

/**
 * @brief Get asynchronous interrupt status
 *
 * This function reads the asynchronous interrupt status from the CCCR
 * interrupt extension register.
 *
 * @param sdio Pointer to SDIO device structure
 *
 * @retval 1 if asynchronous interrupt is supported
 * @retval 0 if asynchronous interrupt is not supported
 * @retval -EINVAL if sdio is NULL or on read error
 */
int sdio_dev_get_aync_intr(struct sdio_dev *sdio);

/**
 * @brief Check if asynchronous interrupt is enabled
 *
 * This function checks whether asynchronous interrupt mode is currently
 * enabled in the CCCR interrupt extension register.
 *
 * @param sdio Pointer to SDIO device structure
 *
 * @retval true if asynchronous interrupt is enabled
 * @retval false if asynchronous interrupt is disabled or on error
 */
bool sdio_dev_aync_intr_is_enabled(struct sdio_dev *sdio);

/**
 * @brief Check if SDIO function is enabled
 *
 * This function checks the I/O Enable register in CCCR to determine
 * if the specified function is enabled.
 *
 * @param func Pointer to SDIO function structure
 *
 * @retval true if function is enabled
 * @retval false if function is disabled or on error
 */
bool sdio_dev_func_is_enabled(const struct sdio_dev_func *func);

/**
 * @brief Check if SDIO function is ready
 *
 * This function checks the I/O Ready register in CCCR to determine
 * if the specified function is ready for I/O operations.
 *
 * @param func Pointer to SDIO function structure
 *
 * @retval true if function is ready
 * @retval false if function is not ready or on error
 */
bool sdio_dev_func_is_ready(const struct sdio_dev_func *func);

/**
 * @brief Check if function interrupt is enabled
 *
 * This function checks the interrupt enable register in CCCR to determine
 * if interrupts are enabled for the specified function.
 *
 * @param func Pointer to SDIO function structure
 *
 * @retval 1 if function interrupt is enabled
 * @retval 0 if function interrupt is disabled
 * @retval -EINVAL if func is NULL or on read error
 */
int sdio_dev_func_intr_is_enable(struct sdio_dev_func *func);

/**
 * @brief Check if interrupt is pending for function
 *
 * This function checks the interrupt pending register in CCCR to determine
 * if there is a pending interrupt for the specified function.
 *
 * @param func Pointer to SDIO function structure
 *
 * @retval 1 if interrupt is pending
 * @retval 0 if no interrupt is pending
 * @retval -EINVAL if func is NULL or on read error
 */
int sdio_dev_func_intr_is_pending(struct sdio_dev_func *func);

/**
 * @brief Get function block size
 *
 * This function reads the block size for the specified function from
 * the FBR (Function Basic Register).
 *
 * @param func Pointer to SDIO function structure
 *
 * @retval Block size in bytes on success
 * @retval -EINVAL if func is NULL
 * @retval negative errno code on other failures
 */
int sdio_dev_get_block_size(struct sdio_dev_func *func);

/**
 * @brief Set function block size
 *
 * This function sets the block size for the specified function in
 * the FBR (Function Basic Register) and verifies the change.
 *
 * @param func Pointer to SDIO function structure
 * @param block_size Block size to set (in bytes)
 *
 * @retval 0 on success
 * @retval -EINVAL if func is NULL or dev is invalid
 * @retval -EACCES if verification fails
 * @retval negative errno code on other failures
 */
int sdio_dev_set_block_size(struct sdio_dev_func *func, uint16_t block_size);

/**
 * @brief Set function code
 *
 * This function writes the function code (type identifier) to the
 * FBR register. Function 0 cannot have its code set.
 *
 * @param func Pointer to SDIO function structure
 * @param fn_code Function code to set
 *
 * @retval 0 on success
 * @retval -EINVAL if func->fn is 0
 * @retval negative errno code on write failure
 */
int sdio_dev_set_fn_code(struct sdio_dev_func *func, uint8_t fn_code);

/**
 * @brief Read function code
 *
 * This function reads the function code (type identifier) from the
 * FBR register.
 *
 * @param func Pointer to SDIO function structure
 *
 * @retval Function code value on success
 * @retval 0 on failure
 */
uint8_t sdio_dev_get_fn_code(const struct sdio_dev_func *func);

/**
 * @brief Read CIS address
 *
 * This function reads the 24-bit Card Information Structure (CIS)
 * pointer from the FBR registers.
 *
 * @param func Pointer to SDIO function structure
 *
 * @retval CIS address on success
 * @retval Error code on failure (returned as uint32_t)
 */
uint32_t sdio_dev_read_cis_addr(struct sdio_dev_func *func);

/** @} */

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_SD_SDIO_DEV_H_ */
