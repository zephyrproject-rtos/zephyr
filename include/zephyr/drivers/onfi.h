/*
 * Copyright (c) 2022 Macronix
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for ONFI drivers and applications
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ONFI_H_
#define ZEPHYR_INCLUDE_DRIVERS_ONFI_H_

/**
 * @brief ONFI Interface
 * @defgroup onfi_interface ONFI Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/*! @brief ONFI NAND device timing configuration structure. */
typedef struct onfi_nand_timing_config
{
	uint8_t tCeSetup_Ns;        /*!< CE setup time: tCS. */
	uint8_t tCeHold_Ns;         /*!< CE hold time: tCH. */
	uint8_t tCeInterval_Ns;     /*!< CE interval time:tCEITV. */
	uint8_t tWeLow_Ns;          /*!< WE low time: tWP. */
	uint8_t tWeHigh_Ns;         /*!< WE high time: tWH. */
	uint8_t tReLow_Ns;          /*!< RE low time: tRP. */
	uint8_t tReHigh_Ns;         /*!< RE high time: tREH. */
	uint8_t tTurnAround_Ns;     /*!< Turnaround time for async mode: tTA. */
	uint8_t tWehigh2Relow_Ns;   /*!< WE# high to RE# wait time: tWHR. */
	uint8_t tRehigh2Welow_Ns;   /*!< RE# high to WE# low wait time: tRHW. */
	uint8_t tAle2WriteStart_Ns; /*!< ALE to write start wait time: tADL. */
	uint8_t tReady2Relow_Ns;    /*!< Ready to RE# low min wait time: tRR. */
	uint8_t tWehigh2Busy_Ns;    /*!< WE# high to busy wait time: tWB. */
} onfi_nand_timing_config_t;

/*! @brief ONFI NAND configuration structure. */
typedef struct onfi_nand_config
{
	uint8_t addressCycle;                        /*!< Address option. */
	bool edoModeEnabled;                         /*!< EDO mode enabled. */
	onfi_nand_timing_config_t *timingConfig;     /*!< ONFI nand timing configuration. */
} onfi_nand_config_t;

/**
 * @typedef onfi_api_is_nand_ready
 * @brief Callback API for checking R/B pin level.
 * See onfi_is_nand_ready() for argument descriptions
 */
typedef bool (*onfi_api_is_nand_ready)(const struct device *dev);

/**
 * @typedef onfi_api_send_command
 * @brief Callback API for sending IP command.
 * See onfi_api_send_command() for argument descriptions
 */
typedef int (*onfi_api_send_command)(const struct device *dev, uint32_t address, uint32_t command, uint32_t write, uint32_t *read);

/**
 * @typedef onfi_api_write
 * @brief Callback API for writing data.
 * See onfi_api_write() for argument descriptions
 */
typedef int (*onfi_api_write)(const struct device *dev, uint32_t address, uint32_t *buffer, uint32_t size_bytes);

/**
 * @typedef onfi_api_read
 * @brief Callback API for reading data.
 * See onfi_api_read() for argument descriptions
 */
typedef int (*onfi_api_read)(const struct device *dev, uint32_t address, uint8_t *buffer, uint32_t size_bytes);

/**
 * @typedef onfi_api_configure_nand
 * @brief Callback API for configuring nand device.
 * See onfi_api_configure_nand() for argument descriptions
 */
typedef int (*onfi_api_configure_nand)(const struct device *dev, onfi_nand_config_t *config);

__subsystem struct onfi_api {
	onfi_api_is_nand_ready is_nand_ready;
	onfi_api_send_command send_command;
	onfi_api_write write;
	onfi_api_read read;
	onfi_api_configure_nand configure_nand;
};

/*!
 * @brief Check if the NAND device is ready.
 *
 * @return  True NAND is ready, false NAND is not ready.
 */
__syscall bool onfi_is_nand_ready(const struct device *dev);

static inline bool z_impl_onfi_is_nand_ready(const struct device *dev)
{
	const struct onfi_api *api =
		(const struct onfi_api *)dev->api;
	bool rc;

	rc = api->is_nand_ready(dev);

	return rc;
}

/*!
 * brief IP command access.
 *
 * param address  NAND device address.
 * param command  IP command.
 * param write  Data for write access.
 * param read   Data pointer for read data out.
 */
__syscall int onfi_send_command(const struct device *dev, uint32_t address, uint32_t command, uint32_t write, uint32_t *read);

static inline int z_impl_onfi_send_command(const struct device *dev, uint32_t address, uint32_t command, uint32_t write, uint32_t *read)
{
	const struct onfi_api *api =
		(const struct onfi_api *)dev->api;
	int rc;

	rc = api->send_command(dev, address, command, write, read);

	return rc;
}

/*!
 * brief NAND device memory write through IP command.
 *
 * param address  NAND device address.
 * param data  Data for write access.
 * param size_bytes   Data length.
 */
__syscall int onfi_write(const struct device *dev, uint32_t address, uint32_t *buffer, uint32_t size_bytes);

static inline int z_impl_onfi_write(const struct device *dev, uint32_t address, uint32_t *buffer, uint32_t size_bytes)
{
	const struct onfi_api *api =
		(const struct onfi_api *)dev->api;
	int rc;

	rc = api->write(dev, address, buffer, size_bytes);

	return rc;
}

/*!
 * brief NAND device memory read through IP command.
 *
 * param address  NAND device address.
 * param data  Data pointer for data read out.
 * param size_bytes   Data length.
 */
__syscall int onfi_read(const struct device *dev, uint32_t address, uint8_t *buffer, uint32_t size_bytes);

static inline int z_impl_onfi_read(const struct device *dev, uint32_t address, uint8_t *buffer, uint32_t size_bytes)
{
	const struct onfi_api *api =
		(const struct onfi_api *)dev->api;
	int rc;

	rc = api->read(dev, address, buffer, size_bytes);

	return rc;
}

/*!
 * brief Configures NAND controller.
 *
 * param config The nand configuration.
 */
__syscall int onfi_configure_nand(const struct device *dev, onfi_nand_config_t *config);

static inline int z_impl_onfi_configure_nand(const struct device *dev, onfi_nand_config_t *config)
{
	const struct onfi_api *api =
		(const struct onfi_api *)dev->api;
	int rc;

	rc = api->configure_nand(dev, config);

	return rc;
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/onfi.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_ONFI_H_ */

