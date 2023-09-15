/*
 * Copyright 2022-2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_SPI_SPI_NXP_S32_H_
#define ZEPHYR_DRIVERS_SPI_SPI_NXP_S32_H_

#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
LOG_MODULE_REGISTER(spi_nxp_s32);

#include "spi_context.h"

#include <Spi_Ip.h>

#define SPI_NXP_S32_NUM_PRESCALER 4U
#define SPI_NXP_S32_NUM_SCALER 16U

/* Modified SPI transfer format is not supported,
 * the maximum baudrate is 25Mhz.
 */
#define SPI_NXP_S32_MIN_FREQ 100000U
#define SPI_NXP_S32_MAX_FREQ 25000000U

#define SPI_NXP_S32_BYTE_PER_FRAME(frame_size)			\
	(frame_size <= 8U) ? 1U : ((frame_size <= 16U) ? 2U : 4U)

#define SPI_NXP_S32_MAX_BYTES_PER_PACKAGE(bytes_per_frame)	\
		((UINT16_MAX / bytes_per_frame) * bytes_per_frame)

struct spi_nxp_s32_baudrate_param {
	uint8_t scaler;
	uint8_t prescaler;
	uint32_t frequency;
};

struct spi_nxp_s32_data {
	uint8_t bytes_per_frame;
	uint32_t transfer_len;
	struct spi_context ctx;

	Spi_Ip_ExternalDeviceType transfer_cfg;
	Spi_Ip_DeviceParamsType	transfer_params;
};

struct spi_nxp_s32_config {
	uint8_t instance;
	uint8_t num_cs;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	uint32_t sck_cs_delay;
	uint32_t cs_sck_delay;
	uint32_t cs_cs_delay;

	Spi_Ip_ConfigType *spi_hw_cfg;
	const struct pinctrl_dev_config *pincfg;

#ifdef CONFIG_NXP_S32_SPI_INTERRUPT
	Spi_Ip_CallbackType cb;
	void (*irq_config_func)(const struct device *dev);
#endif /* CONFIG_NXP_S32_SPI_INTERRUPT */
};

#endif /* ZEPHYR_DRIVERS_SPI_SPI_NXP_S32_H_ */
