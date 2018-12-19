/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _FSL_FLEXCOMM_H_
#define _FSL_FLEXCOMM_H_

#include "fsl_common.h"

/*!
 * @addtogroup flexcomm_driver
 * @{
 */

/*! @name Driver version */
/*@{*/
/*! @brief FlexCOMM driver version 2.0.0. */
#define FSL_FLEXCOMM_DRIVER_VERSION (MAKE_VERSION(2, 0, 0))
/*@}*/

/*! @brief FLEXCOMM peripheral modes. */
typedef enum
{
    FLEXCOMM_PERIPH_NONE,   /*!< No peripheral */
    FLEXCOMM_PERIPH_USART,  /*!< USART peripheral */
    FLEXCOMM_PERIPH_SPI,    /*!< SPI Peripheral */
    FLEXCOMM_PERIPH_I2C,    /*!< I2C Peripheral */
    FLEXCOMM_PERIPH_I2S_TX, /*!< I2S TX Peripheral */
    FLEXCOMM_PERIPH_I2S_RX, /*!< I2S RX Peripheral */
} FLEXCOMM_PERIPH_T;

/*! @brief Typedef for interrupt handler. */
typedef void (*flexcomm_irq_handler_t)(void *base, void *handle);

/*! @brief Array with IRQ number for each FLEXCOMM module. */
extern IRQn_Type const kFlexcommIrqs[];

/*! @brief Returns instance number for FLEXCOMM module with given base address. */
uint32_t FLEXCOMM_GetInstance(void *base);

/*! @brief Initializes FLEXCOMM and selects peripheral mode according to the second parameter. */
status_t FLEXCOMM_Init(void *base, FLEXCOMM_PERIPH_T periph);

/*! @brief Sets IRQ handler for given FLEXCOMM module. It is used by drivers register IRQ handler according to FLEXCOMM
 * mode */
void FLEXCOMM_SetIRQHandler(void *base, flexcomm_irq_handler_t handler, void *handle);

/*@}*/

#endif /* _FSL_FLEXCOMM_H_*/
