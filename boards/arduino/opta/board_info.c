/*
 * Copyright (c) 2024 DNDG srl
 *
 * Part of this code has been copied from flash_stm32_qspi.c.
 *
 * Copyright (c) 2020 Piotr Mienkowski
 * Copyright (c) 2020 Linaro Limited
 * Copyright (c) 2022 Georgij Cernysiov
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <errno.h>
#include <stdint.h>
#include "board.h"

static struct opta_board_info opta_board_info = {0};
static char serial_number[OPTA_SERIAL_NUMBER_SIZE + 1] = {0};

#ifdef CONFIG_FLASH

const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(external_flash));

struct flash_stm32_qspi_data {
	QSPI_HandleTypeDef hqspi;
	struct k_sem sem;
	/* More fields here but we're not interested in them. */
};

static int board_info(void)
{
	struct flash_stm32_qspi_data *dev_data = dev->data;

	HAL_StatusTypeDef hal_ret;

	QSPI_CommandTypeDef cmd = {
		.Instruction = 0x48,
		.InstructionMode = QSPI_INSTRUCTION_1_LINE,
		.Address = (1 << 13),
		.AddressSize = QSPI_ADDRESS_24_BITS,
		.AddressMode = QSPI_ADDRESS_1_LINE,
		.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE,
		.DataMode = QSPI_DATA_1_LINE,
		.NbData = sizeof(struct opta_board_info),
		.DummyCycles = 8,
	};

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	hal_ret = HAL_QSPI_Command_IT(&dev_data->hqspi, &cmd);

	if (hal_ret != HAL_OK) {
		return -EIO;
	}

	hal_ret = HAL_QSPI_Receive(
		&dev_data->hqspi,
		(uint8_t *)&opta_board_info,
		HAL_QSPI_TIMEOUT_DEFAULT_VALUE);

	if (hal_ret != HAL_OK) {
		return -EIO;
	}

	return 0;
}

SYS_INIT(board_info, APPLICATION, 0);

#endif

static void uint32tohex(char *dst, uint32_t value)
{
	int v;

	for (int i = 0; i < 8; i++) {
		v = (value >> ((8 - i - 1) * 4)) & 0x0F;
		dst[i] = v <= 9 ? (0x30 + v) : (0x40 + v - 9);
	}
}

const struct opta_board_info *const opta_get_board_info(void)
{
	if (opta_board_info.magic == OPTA_OTP_MAGIC) {
		return &opta_board_info;
	}
	return NULL;
}

const char *const opta_get_serial_number(void)
{
	if (serial_number[0] == 0) {
		uint32tohex(&serial_number[0], HAL_GetUIDw0());
		uint32tohex(&serial_number[8], HAL_GetUIDw1());
		uint32tohex(&serial_number[16], HAL_GetUIDw2());
	}
	return serial_number;
}
