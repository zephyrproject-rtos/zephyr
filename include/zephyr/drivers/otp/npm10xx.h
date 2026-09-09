/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_OTP_NPM10XX_H_
#define ZEPHYR_INCLUDE_DRIVERS_OTP_NPM10XX_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/otp.h>
#include <zephyr/sys/util.h>

/**
 * @file npm10xx.h
 * @brief Nordic's nPM10 Series PMIC OTP driver definitions
 * @defgroup otp_interface_npm10xx nPM10xx OTP interface
 * @ingroup otp_interface
 * @since 4.5
 * @version 0.1.0
 * @{
 */

/** Number of user-programmable UICR bits. */
#define OTP_NPM10XX_UICR_BITS 104U
/** Packed byte size that holds the bits. */
#define OTP_NPM10XX_UICR_SIZE DIV_ROUND_UP(OTP_NPM10XX_UICR_BITS, 8U)

/**
 * Lock bit index. The driver programs it last.
 * @important Ensure this bit is set to 0 in your data buffer if you do not wish to lock the UICR.
 */
#define OTP_NPM10XX_UICR_LOCK_BIT 96U

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_OTP_NPM10XX_H_ */
