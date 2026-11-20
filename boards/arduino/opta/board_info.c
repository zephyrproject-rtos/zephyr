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

#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_mgmt.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(board, CONFIG_LOG_DEFAULT_LEVEL);

#define AT25SF128_READ_SECURITY_REGISTERS 0x48

static struct opta_board_info info;
static char serial_number[OPTA_SERIAL_NUMBER_SIZE + 1];

#if defined(CONFIG_FLASH_STM32_QSPI_GENERIC_READ) && defined(CONFIG_FLASH_EX_OP_ENABLED)

const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(qspi_flash));

void board_late_init_hook(void)
{
	int ret = 0;
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
		return;
	}

	ret = flash_ex_op(dev, FLASH_STM32_QSPI_EX_OP_GENERIC_READ, (uintptr_t)&cmd, &info);

	if (ret != 0) {
		LOG_ERR("failed reading secure registers: %d", ret);
		return;
	}

	if (info.magic != OPTA_OTP_MAGIC) {
		LOG_ERR("invalid board info magic number %x", info.magic);
		memset(&info, 0, sizeof(info));
		return;
	}

#if defined(CONFIG_BOARD_ARDUINO_OPTA_FACTORY_MAC)
	LOG_INF("setting opta mac from factory value");
	struct net_if *iface = net_if_get_first_ethernet();

	if (iface == NULL) {
		printk("no net iface defined\n");
		return;
	}

	/* Reject an all-zero or all-FF address rather than applying garbage */
	bool allZero = true, allFF = true;

	for (int i = 0; i < sizeof(info.mac_address); i++) {
		allZero &= (info.mac_address[i] == 0x00);
		allFF &= (info.mac_address[i] == 0xFF);
	}
	if (allZero || allFF) {
		LOG_ERR("invalid board info mac address %02x:%02x:%02x:%02x:%02x:%02x",
			info.mac_address[0], info.mac_address[1], info.mac_address[2],
			info.mac_address[3], info.mac_address[4], info.mac_address[5]);
		return;
	}

	struct ethernet_req_params params;

	memcpy(params.mac_address.addr, info.mac_address, sizeof(params.mac_address.addr));

	bool wasUp = net_if_is_admin_up(iface);

	if (wasUp) {
		net_if_down(iface);
	}
	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_MAC_ADDRESS, iface, &params, sizeof(params));
	if (ret != 0) {
		LOG_ERR("Setting MAC failed: %d", ret);
	}
	if (wasUp) {
		net_if_up(iface);
	}
#endif /* CONFIG_BOARD_ARDUINO_OPTA_FACTORY_MAC */
}

#endif /* CONFIG_FLASH_STM32_QSPI_GENERIC_READ && CONFIG_FLASH_EX_OP_ENABLED */

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
