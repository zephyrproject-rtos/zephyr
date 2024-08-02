/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/drivers/mspi.h>

#define MSPI_BUS                  DT_BUS(DT_ALIAS(dev0))
#define MSPI_TARGET               DT_ALIAS(dev0)

#define BUF_SIZE 1024

#if CONFIG_MEMC_MSPI_APS6404L
#define DEVICE_MEM_WRITE_INSTR    0x38
#define DEVICE_MEM_READ_INSTR     0xEB
#endif

uint8_t memc_write_buffer[BUF_SIZE];
uint8_t memc_read_buffer[BUF_SIZE];

struct user_context {
	uint32_t status;
	uint32_t total_packets;
};

void async_cb(struct mspi_callback_context *mspi_cb_ctx, uint32_t status)
{
	volatile struct user_context *usr_ctx = mspi_cb_ctx->ctx;

	mspi_cb_ctx->mspi_evt.evt_data.status = status;
	if (mspi_cb_ctx->mspi_evt.evt_data.packet_idx == usr_ctx->total_packets - 1) {
		usr_ctx->status = 0;
	}
}

struct mspi_xfer_packet packet1[] = {
	{
		.dir                = MSPI_TX,
		.cmd                = DEVICE_MEM_WRITE_INSTR,
		.address            = 0,
		.num_bytes          = 256,
		.data_buf           = memc_write_buffer,
		.cb_mask            = MSPI_BUS_NO_CB,
	},
	{
		.dir                = MSPI_TX,
		.cmd                = DEVICE_MEM_WRITE_INSTR,
		.address            = 256,
		.num_bytes          = 256,
		.data_buf           = memc_write_buffer + 256,
		.cb_mask            = MSPI_BUS_NO_CB,
	},
	{
		.dir                = MSPI_TX,
		.cmd                = DEVICE_MEM_WRITE_INSTR,
		.address            = 512,
		.num_bytes          = 256,
		.data_buf           = memc_write_buffer + 512,
		.cb_mask            = MSPI_BUS_NO_CB,
	},
	{
		.dir                = MSPI_TX,
		.cmd                = DEVICE_MEM_WRITE_INSTR,
		.address            = 512 + 256,
		.num_bytes          = 256,
		.data_buf           = memc_write_buffer + 512 + 256,
		.cb_mask            = MSPI_BUS_XFER_COMPLETE_CB,
	},
};

struct mspi_xfer_packet packet2[] = {
	{
		.dir                = MSPI_RX,
		.cmd                = DEVICE_MEM_READ_INSTR,
		.address            = 0,
		.num_bytes          = 256,
		.data_buf           = memc_read_buffer,
		.cb_mask            = MSPI_BUS_NO_CB,
	},
	{
		.dir                = MSPI_RX,
		.cmd                = DEVICE_MEM_READ_INSTR,
		.address            = 256,
		.num_bytes          = 256,
		.data_buf           = memc_read_buffer + 256,
		.cb_mask            = MSPI_BUS_NO_CB,
	},
	{
		.dir                = MSPI_RX,
		.cmd                = DEVICE_MEM_READ_INSTR,
		.address            = 512,
		.num_bytes          = 256,
		.data_buf           = memc_read_buffer + 512,
		.cb_mask            = MSPI_BUS_NO_CB,
	},
	{
		.dir                = MSPI_RX,
		.cmd                = DEVICE_MEM_READ_INSTR,
		.address            = 512 + 256,
		.num_bytes          = 256,
		.data_buf           = memc_read_buffer + 512 + 256,
		.cb_mask            = MSPI_BUS_XFER_COMPLETE_CB,
	},
};

struct mspi_xfer xfer1 = {
	.async                      = true,
	.xfer_mode                  = MSPI_DMA,
	.tx_dummy                   = 0,
	.cmd_length                 = 1,
	.addr_length                = 3,
	.priority                   = 1,
	.packets                    = (struct mspi_xfer_packet *)&packet1,
	.num_packet                 = sizeof(packet1) / sizeof(struct mspi_xfer_packet),
};

struct mspi_xfer xfer2 = {
	.async                      = true,
	.xfer_mode                  = MSPI_DMA,
	.rx_dummy                   = 6,
	.cmd_length                 = 1,
	.addr_length                = 3,
	.priority                   = 1,
	.packets                    = (struct mspi_xfer_packet *)&packet2,
	.num_packet                 = sizeof(packet2) / sizeof(struct mspi_xfer_packet),
};

int main(void)
{
	const struct device *controller = DEVICE_DT_GET(MSPI_BUS);
	struct mspi_dev_id dev_id = MSPI_DEVICE_ID_DT(MSPI_TARGET);
	struct mspi_callback_context cb_ctx1, cb_ctx2;
	volatile struct user_context write_ctx, read_ctx;
	int i, j;
	int ret;

	/* Initialize write buffer */
	for (i = 0; i < BUF_SIZE; i++) {
		memc_write_buffer[i] = (uint8_t)i;
	}

	ret = mspi_dev_config(controller, &dev_id, MSPI_DEVICE_CONFIG_NONE, NULL);
	if (ret) {
		printk("Failed to get controller access\n");
		return 1;
	}

	write_ctx.total_packets = xfer1.num_packet;
	write_ctx.status        = ~0;
	cb_ctx1.ctx             = (void *)&write_ctx;
	ret = mspi_register_callback(controller, &dev_id, MSPI_BUS_XFER_COMPLETE,
					(mspi_callback_handler_t)async_cb, &cb_ctx1);
	if (ret) {
		printk("Failed to register callback\n");
		return 1;
	}

	ret = mspi_transceive(controller, &dev_id, &xfer1);
	if (ret) {
		printk("Failed to send transceive\n");
		return 1;
	}

	read_ctx.total_packets  = xfer2.num_packet;
	read_ctx.status         = ~0;
	cb_ctx2.ctx             = (void *)&read_ctx;
	ret = mspi_register_callback(controller, &dev_id, MSPI_BUS_XFER_COMPLETE,
					(mspi_callback_handler_t)async_cb, &cb_ctx2);
	if (ret) {
		printk("Failed to register callback\n");
		return 1;
	}

	ret = mspi_transceive(controller, &dev_id, &xfer2);
	if (ret) {
		printk("Failed to send transceive\n");
		return 1;
	}

	while (write_ctx.status != 0 || read_ctx.status != 0) {
		printk("Waiting for complete..., write completed:%d, read completed:%d\n",
			cb_ctx1.mspi_evt.evt_data.packet_idx,
			cb_ctx2.mspi_evt.evt_data.packet_idx);
		k_busy_wait(100000);
	}

	for (j = 0; j < BUF_SIZE; j++) {
		if (memc_write_buffer[j] != memc_read_buffer[j]) {
			printk("Error: data differs at offset %d\n", j);
			break;
		}
	}
	if (j == BUF_SIZE) {
		printk("Read data matches written data\n");
	}

	return 0;
}
