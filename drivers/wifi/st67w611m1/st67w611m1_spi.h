/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_WIFI_ST67W611M1_SPI_H_
#define ZEPHYR_DRIVERS_WIFI_ST67W611M1_SPI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/toolchain.h>
#include <zephyr/net/net_pkt.h>

#define ST67W611M1_SPI_MTU_BYTES   1520
#define ST67_SPI_BUFFER_SIZE_BYTES (ST67W611M1_SPI_MTU_BYTES + sizeof(struct st67_spi_header))

struct st67_spi_header {
	uint16_t sync_word;
	uint16_t data_length;
	uint8_t frame; /* version (0:1); rx_stall(2); flags(3:7) */
	uint8_t type;
	uint16_t reserved;
} __packed;

BUILD_ASSERT(sizeof(struct st67_spi_header) == 8, "st67_spi_header struct should be 8 bytes long!");

enum st67_spi_frame_type {
	AT_CMD = 0x00,
	STA_DATA = 0x01,
	SAP_DATA = 0x02,
	FRAME_TYPE_MAX,
};

struct st67_spi_pkt {
	void *fifo_reserved; /* Reserved for k_fifo. */
	struct net_pkt *net_pkt;
	enum st67_spi_frame_type type;
};

extern struct k_mem_slab st67_spi_pkt_mem_slab;

typedef void (*drv_rx_iface_cb_t)(const uint8_t *buf, size_t len);

int st67_spi_send(struct st67_spi_pkt *pkt);

int st67_spi_init(void);

void st67_spi_register_drv_rx_iface_cb(drv_rx_iface_cb_t cb, enum st67_spi_frame_type type);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_WIFI_ST67W611M1_SPI_H_ */
