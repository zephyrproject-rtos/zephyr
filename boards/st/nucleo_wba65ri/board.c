/*
 * Copyright (c) 2025-2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/toolchain.h>
#include <stm32_ll_rcc.h>
#include <stddef.h>

struct __packed stm32_board_otp_data {
	uint8_t additional_data[8];	/* 64 bits of data to fill OTP slot Ex: MB184510 */
	uint8_t bd_address[6];		/* Bluetooth Device Address */
	uint8_t hsetune;		/* Load capacitance to be applied on HSE pad */
	uint8_t index;			/* Structure index */
};

#define DEFAULT_OTP_IDX     0

static struct stm32_board_otp_data *get_otp_ptr(uint8_t index)
{
	struct stm32_board_otp_data *otp_ptr =
		(struct stm32_board_otp_data *)(FLASH_OTP_BASE + FLASH_OTP_SIZE);

	while (--otp_ptr >= (struct stm32_board_otp_data *)FLASH_OTP_BASE) {
		if (otp_ptr->index == index) {
			return otp_ptr;
		}
	}

	return NULL;
}

void board_late_init_hook(void)
{
	struct stm32_board_otp_data *otp_ptr = get_otp_ptr(DEFAULT_OTP_IDX);

	if (otp_ptr != NULL) {
		LL_RCC_HSE_SetClockTrimming(otp_ptr->hsetune);
	} else {
		/* OTP no present in flash, apply default gain */
		LL_RCC_HSE_SetClockTrimming(0x0C);
	}
}
