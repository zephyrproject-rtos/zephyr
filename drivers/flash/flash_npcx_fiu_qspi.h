/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FLASH_NPCX_FIU_QSPI_H_
#define ZEPHYR_DRIVERS_FLASH_NPCX_FIU_QSPI_H_

#include <zephyr/device.h>
#include "jesd216.h"

#ifdef __cplusplus
extern "C" {
#endif

/* UMA operation flags */
#define NPCX_UMA_ACCESS_WRITE BIT(0)
#define NPCX_UMA_ACCESS_READ  BIT(1)
#define NPCX_UMA_ACCESS_ADDR  BIT(2)

/* Valid value of Dn_NADDRB that sets the number of address bytes in a transaction */
#define NPCX_DEV_NUM_ADDR_1BYTE 1
#define NPCX_DEV_NUM_ADDR_2BYTE 2
#define NPCX_DEV_NUM_ADDR_3BYTE 3
#define NPCX_DEV_NUM_ADDR_4BYTE 4

#define NPCX_SPI_F_CS0 0
#define NPCX_SPI_F_CS1 1

enum NPCX_SPI_DEV_SIZE {
	NPCX_SPI_DEV_SIZE_1M,
	NPCX_SPI_DEV_SIZE_2M,
	NPCX_SPI_DEV_SIZE_4M,
	NPCX_SPI_DEV_SIZE_8M,
	NPCX_SPI_DEV_SIZE_16M,
	NPCX_SPI_DEV_SIZE_32M,
	NPCX_SPI_DEV_SIZE_64M,
	NPCX_SPI_DEV_SIZE_128M,
};

/* UMA operation configuration for a SPI device */
struct npcx_uma_cfg {
	uint8_t opcode;
	uint8_t *tx_buf;
	size_t  tx_count;
	uint8_t *rx_buf;
	size_t rx_count;
	union {
		uint32_t u32;
		uint8_t u8[4];
	} addr;
};

/* QSPI bus configuration for a SPI device */
struct npcx_qspi_cfg {
	/* Type of Quad Enable bit in status register */
	enum jesd216_dw15_qer_type qer_type;
	/* Pinctrl for QSPI bus */
	const struct pinctrl_dev_config *pcfg;
	/* Enter four bytes address mode value */
	uint8_t enter_4ba;
	/* SPI read access type of Direct Read Access mode */
	uint8_t rd_mode;
	bool is_logical_low_dev;
	uint8_t spi_dev_sz;
	/* Configurations for the Quad-SPI peripherals */
	int flags;
};

/**
 * @brief Execute UMA transactions on qspi bus
 *
 * @param dev Pointer to the device structure for qspi bus controller instance.
 * @param cfg Pointer to the configuration of UMA transactions.
 * @param flags Flags to be used during transactions.
 * @retval 0 on success, -EPERM if an UMA transaction is not permitted.
 */
int qspi_npcx_fiu_uma_transceive(const struct device *dev, struct npcx_uma_cfg *cfg,
				 uint32_t flags);

/**
 * @brief Lock the mutex of npcx qspi bus controller and apply its configuration
 *
 * @param dev Pointer to the device structure for qspi bus controller instance.
 * @param cfg Pointer to the configuration for the device on qspi bus.
 * @param operation Qspi bus operation for the device.
 */
void qspi_npcx_fiu_mutex_lock_configure(const struct device *dev,
					const struct npcx_qspi_cfg *cfg,
					const uint32_t operation);

/**
 * @brief Unlock the mutex of npcx qspi bus controller.
 *
 * @param dev Pointer to the device structure for qspi bus controller instance.
 */
void qspi_npcx_fiu_mutex_unlock(const struct device *dev);

/**
 * @brief Set the size of the address space allocated for SPI device.
 *
 * @param dev Pointer to the device structure for qspi bus controller instance.
 * @param cfg Pointer to the configuration for the device on qspi bus.
 */
void qspi_npcx_fiu_set_spi_size(const struct device *dev, const struct npcx_qspi_cfg *cfg);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_FLASH_NPCX_FIU_QSPI_H_ */
