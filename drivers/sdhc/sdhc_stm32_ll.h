/*
 * Copyright (c) 2025 EXALT Technologies.
 * Copyright (c) 2017 STMicroelectronics.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * This file is derived from STM32Cube HAL (stm32XXxx_hal_sdio.h)
 * with refactoring to align with Zephyr coding style and architecture.
 */

#ifndef ZEPHYR_DRIVERS_SDHC_SDHC_STM32_LL_H_
#define ZEPHYR_DRIVERS_SDHC_SDHC_STM32_LL_H_

#include <stm32_bitops.h>
#include <zephyr/cache.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/logging/log.h>

/**
 * @brief SDMMC card state enumeration
 */
typedef uint32_t SDMMC_CardStateTypeDef;
#define SDMMC_CARD_READY          0x00000001U /* Card state is ready */
#define SDMMC_CARD_IDENTIFICATION 0x00000002U /* Card is in identification state */
#define SDMMC_CARD_STANDBY        0x00000003U /* Card is in standby state */
#define SDMMC_CARD_TRANSFER       0x00000004U /* Card is in transfer state */
#define SDMMC_CARD_SENDING        0x00000005U /* Card is sending an operation */
#define SDMMC_CARD_RECEIVING      0x00000006U /* Card is receiving operation information */
#define SDMMC_CARD_PROGRAMMING    0x00000007U /* Card is in programming state */
#define SDMMC_CARD_DISCONNECTED   0x00000008U /* Card is disconnected */
#define SDMMC_CARD_ERROR          0x000000FFU /* Card response error */

/**
 * @brief SDMMC context flags
 */
#define SDMMC_CONTEXT_NONE                 0x00000000U /* No context */
#define SDMMC_CONTEXT_READ_SINGLE_BLOCK    0x00000001U /* Read single block operation */
#define SDMMC_CONTEXT_READ_MULTIPLE_BLOCK  0x00000002U /* Read multiple blocks operation */
#define SDMMC_CONTEXT_WRITE_SINGLE_BLOCK   0x00000010U /* Write single block operation */
#define SDMMC_CONTEXT_WRITE_MULTIPLE_BLOCK 0x00000020U /* Write multiple blocks operation */
#define SDMMC_CONTEXT_DMA                  0x00000080U /* Process in DMA mode */

/*
 * @brief SDMMC Timing and block definitions
 */
#define SDMMC_INIT_FREQ 400000U /* Initialization phase: max 400 kHz */

/**
 * @brief SDIO Direct Command argument structure (for CMD52)
 */
typedef struct {
	uint32_t Reg_Addr;       /* Register address to read/write */
	uint32_t ReadAfterWrite; /* Read after write flag */
	uint32_t IOFunctionNbr;  /* IO function number */
} sdhc_stm32_sdio_direct_cmd_t;

/**
 * @brief SDIO Extended Command argument structure (for CMD53)
 */
typedef struct {
	uint32_t Reg_Addr;      /* Register address */
	uint32_t IOFunctionNbr; /* IO function number */
	uint32_t Block_Mode;    /* Block or byte mode */
	uint32_t OpCode;        /* Operation code (increment/fixed address) */
} sdhc_stm32_sdio_ext_cmd_t;

struct sdhc_stm32_data {
	uint32_t total_transfer_bytes; /* number of bytes transferred */
	uint32_t sdmmc_clk;            /* Specifies the clock*/
	uint32_t block_size;           /* Block size for SDIO data transfer */

	uint32_t error_code; /* SD Card Error codes */
	uint32_t Context;   /* SD transfer context */

	struct k_mutex bus_mutex;     /* Sync between commands */
	struct sdhc_io host_io;       /* Input/Output host configuration */
	struct sdhc_host_props props; /* current host properties */
	struct k_sem device_sync_sem; /* Sync between device communication messages */
	void *sdio_dma_buf;           /* DMA buffer for SDIO/SDMMC data transfer */
	SDMMC_InitTypeDef Init;       /* SD required parameters */
};

/**
 * @brief SD/MMC Functions Prototypes
 */
void sdhc_stm32_ll_irq_handler(SDMMC_TypeDef *Instance, struct sdhc_stm32_data *data);
int sdhc_stm32_ll_init(SDMMC_TypeDef *Instance, struct sdhc_stm32_data *data);
int sdhc_stm32_ll_deinit(SDMMC_TypeDef *Instance, struct sdhc_stm32_data *data);
/**
 * @brief SDIO Functions Prototypes
 */
int sdhc_stm32_ll_sdio_read_direct(SDMMC_TypeDef *Instance, sdhc_stm32_sdio_direct_cmd_t *Argument,
				   uint8_t *pData, struct sdhc_stm32_data *dev_data);
int sdhc_stm32_ll_sdio_write_direct(SDMMC_TypeDef *Instance, sdhc_stm32_sdio_direct_cmd_t *Argument,
				    uint8_t Data, struct sdhc_stm32_data *dev_data);
int sdhc_stm32_ll_sdio_read_extended(SDMMC_TypeDef *Instance, sdhc_stm32_sdio_ext_cmd_t *Argument,
				     uint8_t *pData, uint32_t Size_byte, uint32_t Timeout_Ms,
				     struct sdhc_stm32_data *dev_data);
int sdhc_stm32_ll_sdio_write_extended(SDMMC_TypeDef *Instance, sdhc_stm32_sdio_ext_cmd_t *Argument,
				      uint8_t *pData, uint32_t Size_byte, uint32_t Timeout_Ms,
				      struct sdhc_stm32_data *dev_data);
int sdhc_stm32_ll_sdio_read_extended_dma(SDMMC_TypeDef *Instance,
					 sdhc_stm32_sdio_ext_cmd_t *Argument,
					 struct sdhc_stm32_data *dev_data);
int sdhc_stm32_ll_sdio_write_extended_dma(SDMMC_TypeDef *Instance,
					  sdhc_stm32_sdio_ext_cmd_t *Argument,
					  struct sdhc_stm32_data *dev_data);

#endif /* ZEPHYR_DRIVERS_SDHC_SDHC_STM32_LL_H_ */
