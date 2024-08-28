/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FLASH_NPCM_QSPI_H_
#define ZEPHYR_DRIVERS_FLASH_NPCM_QSPI_H_

#include <zephyr/device.h>
#include "jesd216.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Transceive operation flags */
#define NPCM_TRANSCEIVE_ACCESS_WRITE BIT(0)
#define NPCM_TRANSCEIVE_ACCESS_READ  BIT(1)
#define NPCM_TRANSCEIVE_ACCESS_ADDR  BIT(2)

/* Valid value of Dn_NADDRB that sets the number of address bytes in a transaction */
#define NPCM_DEV_NUM_ADDR_1BYTE 1
#define NPCM_DEV_NUM_ADDR_2BYTE 2
#define NPCM_DEV_NUM_ADDR_3BYTE 3
#define NPCM_DEV_NUM_ADDR_4BYTE 4

/* Transceive operation configuration for a SPI device */
struct npcm_transceive_cfg {
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
struct npcm_qspi_cfg {
	/* Type of Quad Enable bit in status register */
	enum jesd216_dw15_qer_type qer_type;
	/* Pinctrl for QSPI bus */
	const struct pinctrl_dev_config *pcfg;
	/* Enter four bytes address mode value */
	uint8_t enter_4ba;
	/* SPI read access type of Direct Read Access mode */
	uint8_t rd_mode;
	/* Configurations for the Quad-SPI peripherals */
	int flags;
};

/**
 * @brief Execute transactions on qspi bus
 *
 * @param dev Pointer to the device structure for qspi bus controller instance.
 * @param cfg Pointer to the configuration of transactions.
 * @param flags Flags to be used during transactions.
 * @retval 0 on success, -EPERM if an transaction is not permitted.
 */
typedef int (*qspi_npcm_transceive)(const struct device *dev,
				struct npcm_transceive_cfg *cfg, uint32_t flags);

/**
 * @brief Lock the mutex of npcm qspi bus controller and apply its configuration
 *
 * @param dev Pointer to the device structure for qspi bus controller instance.
 * @param cfg Pointer to the configuration for the device on qspi bus.
 * @param operation Qspi bus operation for the device.
 */
typedef void (*qspi_npcm_mutex_lock_configure)(const struct device *dev,
					const struct npcm_qspi_cfg *cfg,
					const uint32_t operation);

/**
 * @brief Unlock the mutex of npcm qspi bus controller.
 *
 * @param dev Pointer to the device structure for qspi bus controller instance.
 */
typedef void (*qspi_npcm_mutex_unlock)(const struct device *dev);

struct npcm_qspi_ops {
	qspi_npcm_mutex_lock_configure lock_configure;
	qspi_npcm_mutex_unlock unlock;
	qspi_npcm_transceive transceive;
};

/* Device data */
struct npcm_qspi_data {
	/* mutex of qspi bus controller */
	struct k_sem lock_sem;
	/* qspi operation interface */
	struct npcm_qspi_ops *qspi_ops;
	/* Current device configuration on QSPI bus */
	const struct npcm_qspi_cfg *cur_cfg;
	/* Current Software controlled Chip-Select number */
	int sw_cs;
	/* Current QSPI bus operation */
	uint32_t operation;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_FLASH_NPCM_QSPI_H_ */
