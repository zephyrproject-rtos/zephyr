/*
 * Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/misc/bflb_wo/bflb_wo.h>

#define STR		"HEWWO WORW\n"
#define DATA_BPC	10
#define DATA_LEN	DATA_BPC * sizeof(STR)

int main(void)
{
	int ret;
	// struct bflb_wo_config cfg = {
	// 	.pull_down = false,
	// 	.pull_up = true,
	// 	.total_cycles = bflb_wo_frequency_to_cycles(115200, false),
	// 	.set_cycles = bflb_wo_frequency_to_cycles(115200, false),
	// 	.unset_cycles = bflb_wo_frequency_to_cycles(115200, false),
	// 	.set_invert = true,
	// 	.unset_invert = false,
	// 	.park_high = true,
	// };
	// uint16_t data[DATA_LEN];
 //
	// if (cfg.total_cycles == 0) {
	// 	printf("Bad timing");
	// 	return -1;
	// }
 //
	// printf("Timing: %u cycles", cfg.total_cycles);
 //
	// memset(cfg.pins, BFLB_WO_PIN_NONE, 16);
	// cfg.pins[0] = 17;
 //
	// ret = bflb_wo_configure(&cfg);
 //
	// if (ret < 0) {
	// 	printf("Configuration bad...");
	// }
 //
	// for (int i =0; i < sizeof(STR); i++) {
	// 	data[i * DATA_BPC + 0] = 0;
	// 	for (int j = 0; j < 8; j++) {
	// 		data[i * DATA_BPC + j + 1] = (((STR[i] >>j) & 1)) << 1;
	// 	}
	// 	data[i * DATA_BPC + 9] = 1 << 1;
	// }

	// struct  uart_config ucf = {
	// 	.baudrate = 115200,
	// 	.parity = UART_CFG_PARITY_NONE,
	// 	.stop_bits = UART_CFG_STOP_BITS_2,
	// 	.data_bits = UART_CFG_DATA_BITS_8,
	// 	.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
	// };
 //
	// while (true) {
	// 	//bflb_wo_write(data, DATA_LEN);
	// 	ret = bflb_wo_uart(STR, 17, &ucf);
	// 	printf("Sent(%d): %s\n", ret, STR);
	// 	k_msleep(100);
	// }

	uint32_t color = 0xFF0000;
	while (true) {

		ret = bflb_wo_ws2812b(&color, 1, 17);
		if (color > 0x10000) {
			color -= 0x10000;
		} else if (color > 0x100) {
			color -= 0x100;
		} else {
			color -= 0x1;
		}
		if (color == 0)
			color = 0xFF0000;
		k_msleep(0);
	}

	return 0;
}
