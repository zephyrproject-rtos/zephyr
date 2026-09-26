/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver API for the NXP SEMC memory controller.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MEMC_MCUX_SEMC_H_
#define ZEPHYR_INCLUDE_DRIVERS_MEMC_MCUX_SEMC_H_

#include <zephyr/device.h>
#include <fsl_semc.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The SEMC controller can drive several memory types (SDRAM, SRAM/PSRAM,
 * NOR and NAND) on independent chip-selects. The memc driver only owns the
 * controller: memc_mcux_semc_reconfigure()/memc_mcux_semc_soft_reset()
 * initialize the SEMC core block itself, and the memc_mcux_semc_configure_*()
 * calls program the controller interface for one memory type per chip-select.
 *
 * After SEMC_Init()/reconfigure() no external device is configured yet; a
 * configure_*() call is required before the corresponding memory region can
 * be accessed.
 *
 * NOR/NAND configuration below only sets up the controller side of the
 * interface. Actual flash operations (read/write/erase) are meant to be
 * implemented by flash drivers on top of the SEMC IP-command path, e.g. via
 * SEMC_SendIPCommand()/SEMC_IPCommandNorRead()/SEMC_IPCommandNorWrite() and
 * the NAND equivalents.
 *
 * All functions here are synchronous and block until the operation completes
 * (they serialize access to the controller with a mutex).
 */

/**
 * @brief Populate a SEMC configuration with MCUX defaults.
 *
 * @param config Pointer to destination configuration.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p config is NULL.
 */
int memc_mcux_semc_get_default_config(semc_config_t *config);

/**
 * @brief Reconfigure the SEMC core block.
 *
 * This updates common SEMC options (timeouts, queue weight, DQS mode).
 * It software-resets the controller and leaves it without any configured
 * external device; memory interface timing and base address are programmed
 * separately with memc_mcux_semc_configure_*().
 *
 * @param dev SEMC controller device.
 * @param config Common SEMC configuration.
 *
 * @retval 0 on success.
 * @retval -EINVAL if arguments are invalid.
 * @retval -ENODEV if device is not ready.
 */
int memc_mcux_semc_reconfigure(const struct device *dev, const semc_config_t *config);

/**
 * @brief Trigger SEMC software reset.
 *
 * @param dev SEMC controller device.
 *
 * @retval 0 on success.
 * @retval -EINVAL if arguments are invalid.
 * @retval -ENODEV if device is not ready.
 * @retval -ETIMEDOUT if reset did not complete in time.
 */
int memc_mcux_semc_soft_reset(const struct device *dev);

/**
 * @brief Configure an SDRAM target on SEMC.
 */
int memc_mcux_semc_configure_sdram(const struct device *dev, semc_sdram_cs_t cs,
				   semc_sdram_config_t *config, uint32_t clk_src_hz);

/**
 * @brief Configure an SRAM target on SEMC.
 */
int memc_mcux_semc_configure_sram(const struct device *dev, semc_sram_cs_t cs,
				  semc_sram_config_t *config, uint32_t clk_src_hz);

/**
 * @brief Configure a NOR target on SEMC.
 */
int memc_mcux_semc_configure_nor(const struct device *dev, semc_nor_config_t *config,
				 uint32_t clk_src_hz);

/**
 * @brief Configure a NAND target on SEMC.
 */
int memc_mcux_semc_configure_nand(const struct device *dev, semc_nand_config_t *config,
				  uint32_t clk_src_hz);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MEMC_MCUX_SEMC_H_ */
