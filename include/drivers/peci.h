/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public Platform Environment Control Interface driver APIs
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PECI_H_
#define ZEPHYR_INCLUDE_DRIVERS_PECI_H_

/**
 * @brief PECI Interface 3.0
 * @defgroup peci_interface PECI Interface
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <device.h>
#include <dt-bindings/pwm/pwm.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief PECI error codes.
 */
enum peci_error_code {
	PECI_GENERAL_SENSOR_ERROR    = 0x8000,
	PECI_UNDERFLOW_SENSOR_ERROR  = 0x8002,
	PECI_OVERFLOW_SENSOR_ERROR   = 0x8003,
};

/**
 * @brief PECI commands.
 */
enum peci_command_code {
	PECI_CMD_PING                = 0x00,
	PECI_CMD_GET_TEMP0           = 0x01,
	PECI_CMD_GET_TEMP1           = 0x02,
	PECI_CMD_RD_PCI_CFG0         = 0x61,
	PECI_CMD_RD_PCI_CFG1         = 0x62,
	PECI_CMD_WR_PCI_CFG0         = 0x65,
	PECI_CMD_WR_PCI_CFG1         = 0x66,
	PECI_CMD_RD_PKG_CFG0         = 0xA1,
	PECI_CMD_RD_PKG_CFG1         = 0xA,
	PECI_CMD_WR_PKG_CFG0         = 0xA5,
	PECI_CMD_WR_PKG_CFG1         = 0xA6,
	PECI_CMD_RD_IAMSR0           = 0xB1,
	PECI_CMD_RD_IAMSR1           = 0xB2,
	PECI_CMD_WR_IAMSR0           = 0xB5,
	PECI_CMD_WR_IAMSR1           = 0xB6,
	PECI_CMD_RD_PCI_CFG_LOCAL0   = 0xE1,
	PECI_CMD_RD_PCI_CFG_LOCAL1   = 0xE2,
	PECI_CMD_WR_PCI_CFG_LOCAL0   = 0xE5,
	PECI_CMD_WR_PCI_CFG_LOCAL1   = 0xE6,
	PECI_CMD_GET_DIB             = 0xF7,
};

/** PECI read/write supported responses */
#define PECI_RW_PKG_CFG_RSP_PASS       (0x40U)
#define PECI_RW_PKG_CFG_RSP_TIMEOUT    (0x80U)
#define PECI_RW_PKG_CFG_RSP_ILLEGAL    (0x90U)

/** Ping command format. */
#define PECI_PING_WR_LEN               (0U)
#define PECI_PING_RD_LEN               (0U)
#define PECI_PING_LEN                  (3U)

/** GetDIB command format. */
#define PECI_GET_DIB_WR_LEN            (1U)
#define PECI_GET_DIB_RD_LEN            (8U)
#define PECI_GET_DIB_CMD_LEN           (4U)
#define PECI_GET_DIB_DEVINFO           (0U)
#define PECI_GET_DIB_REVNUM            (1U)
#define PECI_GET_DIB_DOMAIN_BIT_MASK   (0x4U)
#define PECI_GET_DIB_MAJOR_REV_MASK    0xF0
#define PECI_GET_DIB_MINOR_REV_MASK    0x0F

/** GetTemp command format. */
#define PECI_GET_TEMP_WR_LEN           (1U)
#define PECI_GET_TEMP_RD_LEN           (2U)
#define PECI_GET_TEMP_CMD_LEN          (4U)
#define PECI_GET_TEMP_LSB              (0U)
#define PECI_GET_TEMP_MSB              (1U)
#define PECI_GET_TEMP_ERR_MSB          (0x80U)
#define PECI_GET_TEMP_ERR_LSB_GENERAL  (0x0U)
#define PECI_GET_TEMP_ERR_LSB_RES      (0x1U)
#define PECI_GET_TEMP_ERR_LSB_TEMP_LO  (0x2U)
#define PECI_GET_TEMP_ERR_LSB_TEMP_HI  (0x3U)

/** RdPkgConfig command format. */
#define PECI_RD_PKG_WR_LEN             (5U)
#define PECI_RD_PKG_LEN_BYTE           (2U)
#define PECI_RD_PKG_LEN_WORD           (3U)
#define PECI_RD_PKG_LEN_DWORD          (5U)
#define PECI_RD_PKG_CMD_LEN            (8U)

/** WrPkgConfig command format */
#define PECI_WR_PKG_RD_LEN              (1U)
#define PECI_WR_PKG_LEN_BYTE            (7U)
#define PECI_WR_PKG_LEN_WORD            (8U)
#define PECI_WR_PKG_LEN_DWORD           (10U)
#define PECI_WR_PKG_CMD_LEN             (9U)

/** RdIAMSR command format */
#define PECI_RD_IAMSR_WR_LEN            (5U)
#define PECI_RD_IAMSR_LEN_BYTE          (2U)
#define PECI_RD_IAMSR_LEN_WORD          (3U)
#define PECI_RD_IAMSR_LEN_DWORD         (5U)
#define PECI_RD_IAMSR_LEN_QWORD         (9U)
#define PECI_RD_IAMSR_CMD_LEN           (8U)

/** WrIAMSR command format */
#define PECI_WR_IAMSR_RD_LEN            (1U)
#define PECI_WR_IAMSR_LEN_BYTE          (7U)
#define PECI_WR_IAMSR_LEN_WORD          (8U)
#define PECI_WR_IAMSR_LEN_DWORD         (10U)
#define PECI_WR_IAMSR_LEN_QWORD         (14U)
#define PECI_WR_IAMSR_CMD_LEN           (9U)

/** RdPCIConfig command format */
#define PECI_RD_PCICFG_WR_LEN           (6U)
#define PECI_RD_PCICFG_LEN_BYTE         (2U)
#define PECI_RD_PCICFG_LEN_WORD         (3U)
#define PECI_RD_PCICFG_LEN_DWORD        (5U)
#define PECI_RD_PCICFG_CMD_LEN          (9U)

/** WrPCIConfig command format */
#define PECI_WR_PCICFG_RD_LEN           (1U)
#define PECI_WR_PCICFG_LEN_BYTE         (8U)
#define PECI_WR_PCICFG_LEN_WORD         (9U)
#define PECI_WR_PCICFG_LEN_DWORD        (11U)
#define PECI_WR_PCICFG_CMD_LEN          (10U)

/** RdPCIConfigLocal command format */
#define PECI_RD_PCICFGL_WR_LEN          (5U)
#define PECI_RD_PCICFGL_RD_LEN_BYTE     (2U)
#define PECI_RD_PCICFGL_RD_LEN_WORD     (3U)
#define PECI_RD_PCICFGL_RD_LEN_DWORD    (5U)
#define PECI_RD_PCICFGL_CMD_LEN         (8U)

/** WrPCIConfigLocal command format */
#define PECI_WR_PCICFGL_RD_LEN          (1U)
#define PECI_WR_PCICFGL_WR_LEN_BYTE     (7U)
#define PECI_WR_PCICFGL_WR_LEN_WORD     (8U)
#define PECI_WR_PCICFGL_WR_LEN_DWORD    (10U)
#define PECI_WR_PCICFGL_CMD_LEN         (9U)

/**
 * @brief PECI buffer structure
 *
 * @param buf is a valid pointer on a data buffer, or NULL otherwise.
 * @param len is the length of the data buffer expected to received without
 * considering the frame check sequence byte.
 *
 * Note: Frame check sequence byte is added into rx buffer, need to allocate
 * an additional byte for this in rx buffer.
 */
struct peci_buf {
	uint8_t *buf;
	size_t len;
};

/**
 * @brief PECI transaction packet format.
 */
struct peci_msg {
	/** Client address */
	uint8_t addr;
	/** Command code */
	enum peci_command_code cmd_code;
	/** Pointer to buffer of write data */
	struct peci_buf tx_buffer;
	/** Pointer to buffer of read data */
	struct peci_buf rx_buffer;
	/** PECI msg flags */
	uint8_t flags;
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * PECI driver API definition and system call entry points
 *
 * (Internal use only.)
 */
typedef int (*peci_config_t)(const struct device *dev, uint32_t bitrate);
typedef int (*peci_transfer_t)(const struct device *dev, struct peci_msg *msg);
typedef int (*peci_disable_t)(const struct device *dev);
typedef int (*peci_enable_t)(const struct device *dev);

struct peci_driver_api {
	peci_config_t config;
	peci_disable_t disable;
	peci_enable_t enable;
	peci_transfer_t transfer;
};

/**
 * @endcond
 */

/**
 * @brief Configures the PECI interface.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param bitrate the selected expressed in Kbps.
 * command or when an event needs to be sent to the client application.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int peci_config(const struct device *dev, uint32_t bitrate);

static inline int z_impl_peci_config(const struct device *dev,
				     uint32_t bitrate)
{
	struct peci_driver_api *api;

	api = (struct peci_driver_api *)dev->api;
	return api->config(dev, bitrate);
}

/**
 * @brief Enable PECI interface.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int peci_enable(const struct device *dev);

static inline int z_impl_peci_enable(const struct device *dev)
{
	struct peci_driver_api *api;

	api = (struct peci_driver_api *)dev->api;
	return api->enable(dev);
}

/**
 * @brief Disable PECI interface.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int peci_disable(const struct device *dev);

static inline int z_impl_peci_disable(const struct device *dev)
{
	struct peci_driver_api *api;

	api = (struct peci_driver_api *)dev->api;
	return api->disable(dev);
}

/**
 * @brief Performs a PECI transaction.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param msg Structure representing a PECI transaction.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */

__syscall int peci_transfer(const struct device *dev, struct peci_msg *msg);

static inline int z_impl_peci_transfer(const struct device *dev,
				       struct peci_msg *msg)
{
	struct peci_driver_api *api;

	api = (struct peci_driver_api *)dev->api;
	return api->transfer(dev, msg);
}


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/peci.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_PECI_H_ */
