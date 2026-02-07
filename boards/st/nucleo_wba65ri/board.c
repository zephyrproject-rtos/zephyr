/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <stm32_ll_rcc.h>

typedef __PACKED_STRUCT
{
	uint8_t additional_data[8];	/*!< 64 bits of data to fill OTP slot Ex: MB184510 */
	uint8_t bd_address[6];		/*!< Bluetooth Device Address*/
	uint8_t hsetune;		/*!< Load capacitance to be applied on HSE pad */
	uint8_t index;			/*!< Structure index */
} OTP_Data_s;

#define DEFAULT_OTP_IDX     0

static HAL_StatusTypeDef OTP_Read(uint8_t index, OTP_Data_s **otp_ptr);

void board_early_init_hook(void)
{
	OTP_Data_s *otp_ptr = NULL;

	/* Read HSE_Tuning from OTP */
	if (OTP_Read(DEFAULT_OTP_IDX, &otp_ptr) != HAL_OK) {
		/* OTP no present in flash, apply default gain */
		LL_RCC_HSE_SetClockTrimming(0x0C);
	} else {
		LL_RCC_HSE_SetClockTrimming(otp_ptr->hsetune);
	}
}

static HAL_StatusTypeDef OTP_Read(uint8_t index, OTP_Data_s **otp_ptr)
{
	*otp_ptr = (OTP_Data_s *) (FLASH_OTP_BASE + FLASH_OTP_SIZE - 16);

	while (((*otp_ptr)->index != index) && ((*otp_ptr) != (OTP_Data_s *) FLASH_OTP_BASE)) {
		(*otp_ptr) -= 1;
	}

	if ((*otp_ptr)->index != index) {
		return HAL_ERROR;
	}

	return HAL_OK;
}
