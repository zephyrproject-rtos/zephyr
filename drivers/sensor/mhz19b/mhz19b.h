/*
 * Copyright (c) 2021 G-Technologies Sdn. Bhd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_MHZ19B_MHZ19B
#define ZEPHYR_DRIVERS_SENSOR_MHZ19B_MHZ19B

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

#define MHZ19B_BUF_LEN 9

#define MHZ19B_TX_CMD_IDX 2
#define MHZ19B_RX_CMD_IDX 1
#define MHZ19B_CHECKSUM_IDX 8

/* Arbitrary max duration to wait for the response */
#define MHZ19B_WAIT K_SECONDS(1)

enum mhz19b_cmd_idx {
	/* Command to poll for CO2 */
	MHZ19B_CMD_IDX_GET_CO2,
	/* Read range */
	MHZ19B_CMD_IDX_GET_RANGE,
	/* Get ABC status */
	MHZ19B_CMD_IDX_GET_ABC,
	/* Enable ABC */
	MHZ19B_CMD_IDX_SET_ABC_ON,
	/* Disable ABC */
	MHZ19B_CMD_IDX_SET_ABC_OFF,
	/* Set detection range to 2000 ppm */
	MHZ19B_CMD_IDX_SET_RANGE_2000,
	/* Set detection range to 5000 ppm */
	MHZ19B_CMD_IDX_SET_RANGE_5000,
	/* Set detection range to 10000 ppm */
	MHZ19B_CMD_IDX_SET_RANGE_10000,
	/* Number of supported commands */
	MHZ19B_CMD_IDX_MAX,
};

struct mhz19b_data {
	/* Max data length is 16 bits */
	uint16_t data;
	/* Command buf length is 9 */
	uint8_t xfer_bytes;
	bool has_rsp;

	uint8_t rd_data[MHZ19B_BUF_LEN];

	struct k_sem tx_sem;
	struct k_sem rx_sem;

	enum mhz19b_cmd_idx cmd_idx;
};

struct mhz19b_cfg {
	const struct device *uart_dev;
	uint16_t range;
	bool abc_on;
	uart_irq_callback_user_data_t cb;
};

#define MHZ19B_HEADER 0xff
#define MHZ19B_RESERVED 0x01
#define MHZ19B_NULL 0x00
#define MHZ19B_NULL_1 MHZ19B_NULL
#define MHZ19B_NULL_2 MHZ19B_NULL, MHZ19B_NULL_1
#define MHZ19B_NULL_3 MHZ19B_NULL, MHZ19B_NULL_2
#define MHZ19B_NULL_4 MHZ19B_NULL, MHZ19B_NULL_3
#define MHZ19B_NULL_5 MHZ19B_NULL, MHZ19B_NULL_4
#define MHZ19B_NULL_COUNT(c) MHZ19B_NULL_##c

#define MHZ19B_ABC_ON 0xA0
#define MHZ19B_ABC_OFF 0x00
#define MHZ19B_RANGE_2000 0x07, 0xD0
#define MHZ19B_RANGE_5000 0x13, 0x88
#define MHZ19B_RANGE_10000 0x27, 0x10

enum mhz19b_cmd {
	MHZ19B_CMD_SET_ABC = 0x79,
	MHZ19B_CMD_GET_ABC = 0x7D,
	MHZ19B_CMD_GET_CO2 = 0x86,
	MHZ19B_CMD_SET_RANGE = 0x99,
	MHZ19B_CMD_GET_RANGE = 0x9B,
};

#endif /* ZEPHYR_DRIVERS_SENSOR_MHZ19B_MHZ19B */
