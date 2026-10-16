/*
 * Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Vendor extension for the Analog Devices AXI SPI Engine.
 *
 * In offload mode a fixed command program is loaded once into the core's
 * command memory and replayed autonomously on every external trigger (e.g. a
 * PWMGEN CNV pulse), streaming the captured SDI data to an AXI DMAC with no
 * per-sample CPU cost.
 *
 * Zephyr has no generic SPI-offload framework, so this is exposed as a
 * driver-specific extension alongside the standard SPI controller API rather
 * than through it. Register-mode transfers still use spi_transceive(); only
 * the offload program setup lives here. The device handle is the SPI
 * controller device itself.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SPI_SPI_ADI_AXI_SPI_ENGINE_H_
#define ZEPHYR_INCLUDE_DRIVERS_SPI_SPI_ADI_AXI_SPI_ENGINE_H_

#include <zephyr/device.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Build a raw SPI Engine command word.
 *
 * Bit layout: [14:12]=instruction, [10:8]=arg1, [7:0]=arg2.
 * @p _arg1 MUST be 3-bit-masked to avoid SDO_LANE_CONFIG aliasing to CLK_DIV.
 *
 * @param _inst Instruction field (3 bits).
 * @param _arg1 First argument field (3 bits).
 * @param _arg2 Second argument field (8 bits).
 */
#define SPI_ENGINE_CMD_BUILD(_inst, _arg1, _arg2)                                                  \
	((((_inst) & 0x07) << 12) | (((_arg1) & 0x07) << 8) | ((_arg2) & 0xFF))

/** @brief Build a CONFIG command that sets the clock divider to @p _div. */
#define SPI_ENGINE_CMD_BUILD_CONFIG_CLK_DIV(_div)    SPI_ENGINE_CMD_BUILD(0x02, 0x00, (_div))
/** @brief Build a CONFIG command that sets the SPI mode to @p _mode. */
#define SPI_ENGINE_CMD_BUILD_CONFIG_MODE(_mode)      SPI_ENGINE_CMD_BUILD(0x02, 0x01, (_mode))
/** @brief Build a CONFIG command that sets the transfer width to @p _bits. */
#define SPI_ENGINE_CMD_BUILD_CONFIG_XFER_BITS(_bits) SPI_ENGINE_CMD_BUILD(0x02, 0x02, (_bits))

/** @brief Build a TRANSFER command that reads @p _n words. */
#define SPI_ENGINE_CMD_BUILD_READ_N_WORDS(_n)       SPI_ENGINE_CMD_BUILD(0x00, 0x02, ((_n) - 1))
/** @brief Build a TRANSFER command that writes @p _n words. */
#define SPI_ENGINE_CMD_BUILD_WRITE_N_WORDS(_n)      SPI_ENGINE_CMD_BUILD(0x00, 0x01, ((_n) - 1))
/** @brief Build a TRANSFER command that writes and reads @p _n words. */
#define SPI_ENGINE_CMD_BUILD_WRITE_READ_N_WORDS(_n) SPI_ENGINE_CMD_BUILD(0x00, 0x03, ((_n) - 1))

/** @brief Build a CHIPSELECT command that deasserts all chip-select lines. */
#define SPI_ENGINE_CMD_BUILD_CS_LOW              SPI_ENGINE_CMD_BUILD(0x01, 0x03, 0x00)
/** @brief Build a CHIPSELECT command that asserts all chip-select lines. */
#define SPI_ENGINE_CMD_BUILD_CS_HIGH             SPI_ENGINE_CMD_BUILD(0x01, 0x03, 0xFF)
/** @brief Build a CHIPSELECT command that sets the CS lines to @p _cs_pattern. */
#define SPI_ENGINE_CMD_BUILD_ASSERT(_cs_pattern) SPI_ENGINE_CMD_BUILD(0x01, 0x03, (_cs_pattern))

/** @brief Build a SYNC command carrying identifier @p _id. */
#define SPI_ENGINE_CMD_BUILD_SYNC(_id)      SPI_ENGINE_CMD_BUILD(0x03, 0x00, (_id))
/** @brief Build a SLEEP command that idles for @p _cycles core clock cycles. */
#define SPI_ENGINE_CMD_BUILD_SLEEP(_cycles) SPI_ENGINE_CMD_BUILD(0x03, 0x01, (_cycles))

/**
 * @brief Offload program description.
 */
struct spi_engine_offload_msg {
	/** Array of SPI Engine command words (built with the
	 *  SPI_ENGINE_CMD_BUILD_* macros) replayed on each trigger.
	 */
	uint32_t *commands;
	/** Number of entries in @ref spi_engine_offload_msg::commands. */
	uint32_t num_commands;
	/** Optional SDO words preloaded into offload SDO memory, or NULL for
	 *  read-only programs.
	 */
	uint32_t *tx_data;
	/** Number of entries in @ref spi_engine_offload_msg::tx_data. */
	uint32_t tx_len;
	/** Optional consumer-side (DMAC) capture address hint. */
	uint32_t rx_addr;
	/** Optional producer-side address hint. */
	uint32_t tx_addr;
};

/**
 * @brief Load an offload command program into the SPI Engine.
 *
 * Resets the offload module and writes @p msg into the core's command
 * (and optional SDO) memory. Does not start execution; call
 * spi_engine_offload_enable() once the downstream DMAC is armed.
 *
 * @param dev SPI Engine controller device (compatible "adi,axi-spi-engine").
 * @param msg Offload program to load.
 *
 * @return 0 on success, negative errno otherwise.
 */
int spi_engine_offload_load(const struct device *dev, struct spi_engine_offload_msg *msg);

/**
 * @brief Enable or disable autonomous offload execution.
 *
 * When enabled, the loaded program is replayed on every external trigger.
 * Must be enabled before the first trigger arrives, otherwise the engine
 * and the ADC drift out of frame.
 *
 * @param dev    SPI Engine controller device.
 * @param enable true to start offload replay, false to stop.
 *
 * @return 0 on success, negative errno otherwise.
 */
int spi_engine_offload_enable(const struct device *dev, bool enable);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SPI_SPI_ADI_AXI_SPI_ENGINE_H_ */
