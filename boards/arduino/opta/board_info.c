/*
 * Copyright (c) 2024 DNDG srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/flash/stm32_flash_api_extensions.h>
#include <soc.h>
#include <errno.h>
#include <stdint.h>
#include "board.h"

#define AT25SF128_READ_SECURITY_REGISTERS 0x48

static struct opta_board_info info;
static char serial_number[OPTA_SERIAL_NUMBER_SIZE + 1];

#if defined(CONFIG_FLASH_STM32_QSPI_GENERIC_READ)

const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(qspi_flash));

static int board_info(void)
{
	QSPI_CommandTypeDef cmd = {
		.Instruction = AT25SF128_READ_SECURITY_REGISTERS,
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

	int ret = flash_ex_op(dev, FLASH_STM32_QSPI_EX_OP_GENERIC_READ, (uintptr_t)&cmd, &info);

	if (ret != 0) {
		return -EIO;
	}

	return 0;
}

SYS_INIT(board_info, APPLICATION, 0);

#endif /* CONFIG_FLASH_STM32_QSPI_GENERIC_READ */

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
	if (info.magic == OPTA_OTP_MAGIC) {
		return &info;
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
