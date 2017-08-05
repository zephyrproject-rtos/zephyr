/*
 * Copyright (c) 2017 Erwin Rol <erwin@erwinrol.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <stdio.h>

#include <can.h>
#include <can/frame_pool.h>

static void nmt_rx_cb(struct device *dev, struct can_frame* f);
static void sync_rx_cb(struct device *dev, struct can_frame* f);
static void emcy_rx_cb(struct device *dev, struct can_frame* f);
static void time_rx_cb(struct device *dev, struct can_frame* f);
static void rpdo1_rx_cb(struct device *dev, struct can_frame* f);
static void tpdo1_rx_cb(struct device *dev, struct can_frame* f);
static void rpdo2_rx_cb(struct device *dev, struct can_frame* f);
static void tpdo2_rx_cb(struct device *dev, struct can_frame* f);
static void rpdo3_rx_cb(struct device *dev, struct can_frame* f);
static void tpdo3_rx_cb(struct device *dev, struct can_frame* f);
static void rpdo4_rx_cb(struct device *dev, struct can_frame* f);
static void tpdo4_rx_cb(struct device *dev, struct can_frame* f);
static void tsdo_rx_cb(struct device *dev, struct can_frame* f);
static void rsdo_rx_cb(struct device *dev, struct can_frame* f);
static void nmt_ec_rx_cb(struct device *dev, struct can_frame* f);
static void tlss_rx_cb(struct device *dev, struct can_frame* f);
static void rlss_rx_cb(struct device *dev, struct can_frame* f);

static void can_2_rx_cb(struct device *dev, struct can_frame* f);

const struct can_recv_cb can_1_cbs[] = {
	{
		.cb = nmt_rx_cb,
		.fifo = 0,
		.filter = { // 0x000
			.id = 0x000,
			.mask = CAN_SFF_MASK | CAN_EFF_FLAG | CAN_RTR_FLAG,
		},
	},
	{
		.cb = sync_rx_cb,
		.fifo = 0,
		.filter = { // 0x080
			.id = 0x080,
			.mask = CAN_SFF_MASK | CAN_EFF_FLAG | CAN_RTR_FLAG,
		},
	},
	{
		.cb = emcy_rx_cb,
		.fifo = 0,
		.filter = { // 0x081-0x0FF
			.id = 0x080,
			.mask = 0x780 | CAN_EFF_FLAG | CAN_RTR_FLAG,
		},
	},
	{
		.cb = time_rx_cb,
		.fifo = 0,
		.filter = { // 0x100
			.id = 0x100,
			.mask = CAN_SFF_MASK | CAN_EFF_FLAG | CAN_RTR_FLAG,
		},
	},
	{
		.cb = tpdo1_rx_cb,
		.fifo = 1,
		.filter = { // 0x181-0x1FF
			.id = 0x180,
			.mask = 0x780 | CAN_EFF_FLAG | CAN_RTR_FLAG,
		},
	},
	{
		.cb = rpdo1_rx_cb,
		.fifo = 1,
		.filter = { // 0x200-0x27F
			.id = 0x200,
			.mask = 0x780 | CAN_EFF_FLAG | CAN_RTR_FLAG,
		},
	},
	{
		.cb = tpdo2_rx_cb,
		.fifo = 1,
		.filter = { // 0x280-0x2FF
			.id = 0x280,
			.mask = 0x780 | CAN_EFF_FLAG | CAN_RTR_FLAG,
		},
	},
	{
		.cb = rpdo2_rx_cb,
		.fifo = 1,
		.filter = { // 0x300-0x37F
			.id = 0x300,
			.mask = 0x780 | CAN_EFF_FLAG | CAN_RTR_FLAG,
		},
	},
	{
		.cb = tpdo3_rx_cb,
		.fifo = 1,
		.filter = { // 0x380-0x3FF
			.id = 0x380,
			.mask = 0x780 | CAN_EFF_FLAG | CAN_RTR_FLAG,
		},
	},
	{
		.cb = rpdo3_rx_cb,
		.fifo = 1,
		.filter = { // 0x400-0x47F
			.id = 0x400,
			.mask = 0x780 | CAN_EFF_FLAG | CAN_RTR_FLAG,
		},
	},
	{
		.cb = tpdo4_rx_cb,
		.fifo = 1,
		.filter = { // 0x480-0x4FF
			.id = 0x480,
			.mask = 0x780 | CAN_EFF_FLAG | CAN_RTR_FLAG,
		},
	},
	{
		.cb = rpdo4_rx_cb,
		.fifo = 1,
		.filter = { // 0x500-0x57F
			.id = 0x500,
			.mask = 0x780 | CAN_EFF_FLAG | CAN_RTR_FLAG,
		},
	},
	{
		.cb = tsdo_rx_cb,
		.fifo = 0,
		.filter = { // 0x580-0x5FF
			.id = 0x580,
			.mask = 0x780 | CAN_EFF_FLAG | CAN_RTR_FLAG,
		},
	},
	{
		.cb = rsdo_rx_cb,
		.fifo = 0,
		.filter = { // 0x600-0x67F
			.id = 0x600,
			.mask = 0x780 | CAN_EFF_FLAG | CAN_RTR_FLAG,
		},
	},
	{
		.cb = nmt_ec_rx_cb,
		.fifo = 0,
		.filter = { // 0x700-0x77F
			.id = 0x700,
			.mask = 0x780 | CAN_EFF_FLAG | CAN_RTR_FLAG,
		},
	},
	{
		.cb = rlss_rx_cb,
		.fifo = 0,
		.filter = { // 0x7e4
			.id = 0x7e4,
			.mask = CAN_SFF_MASK | CAN_EFF_FLAG | CAN_RTR_FLAG,
		},
	},
	{
		.cb = tlss_rx_cb,
		.fifo = 0,
		.filter = { // 0x7e5
			.id = 0x7e5,
			.mask = CAN_SFF_MASK | CAN_EFF_FLAG | CAN_RTR_FLAG,
		},
	},
};

const struct can_recv_cb can_2_cbs[] = {
	{
		.cb = can_2_rx_cb,
		.fifo = 0,
		.filter = {
			.id = 0,
			.mask = 0,
		},
	},
};


static int nmt_count = 0;
static void nmt_rx_cb(struct device *dev, struct can_frame* f)
{
	nmt_count++;
	can_frame_free(f);
}

static int sync_count = 0;
static void sync_rx_cb(struct device *dev, struct can_frame* f)
{
	sync_count++;
	can_frame_free(f);
}

static int emcy_count = 0;
static void emcy_rx_cb(struct device *dev, struct can_frame* f)
{
	emcy_count++;
	can_frame_free(f);
}

static int time_count = 0;
static void time_rx_cb(struct device *dev, struct can_frame* f)
{
	time_count++;
	can_frame_free(f);
}

static int rpdo1_count = 0;
static void rpdo1_rx_cb(struct device *dev, struct can_frame* f)
{
	rpdo1_count++;
	can_frame_free(f);
}

static int tpdo1_count = 0;
static int tpdo1_0_count = 0;
static int tpdo1_1_count = 0;
static int tpdo1_2_count = 0;
static int tpdo1_3_count = 0;
static void tpdo1_rx_cb(struct device *dev, struct can_frame* f)
{
	switch((f->id >> 5) & 0x03) {
	case 0x00:
		tpdo1_0_count++;
		break;
	case 0x01:
		tpdo1_1_count++;
		break;
	case 0x02:
		tpdo1_2_count++;
		break;
	case 0x03:
		tpdo1_3_count++;
		break;
	}

	tpdo1_count++;
	can_frame_free(f);
}

static int rpdo2_count = 0;
static void rpdo2_rx_cb(struct device *dev, struct can_frame* f)
{
	rpdo2_count++;
	can_frame_free(f);
}

static int tpdo2_count = 0;
static int tpdo2_0_count = 0;
static int tpdo2_1_count = 0;
static int tpdo2_2_count = 0;
static int tpdo2_3_count = 0;
static void tpdo2_rx_cb(struct device *dev, struct can_frame* f)
{
	switch((f->id >> 5) & 0x03) {
	case 0x00:
		tpdo2_0_count++;
		break;
	case 0x01:
		tpdo2_1_count++;
		break;
	case 0x02:
		tpdo2_2_count++;
		break;
	case 0x03:
		tpdo2_3_count++;
		break;
	}
	tpdo2_count++;
	can_frame_free(f);
}

static int rpdo3_count = 0;
static void rpdo3_rx_cb(struct device *dev, struct can_frame* f)
{
	rpdo3_count++;
	can_frame_free(f);
}

static int tpdo3_count = 0;
static int tpdo3_0_count = 0;
static int tpdo3_1_count = 0;
static int tpdo3_2_count = 0;
static int tpdo3_3_count = 0;
static void tpdo3_rx_cb(struct device *dev, struct can_frame* f)
{
	switch((f->id >> 5) & 0x03) {
	case 0x00:
		tpdo3_0_count++;
		break;
	case 0x01:
		tpdo3_1_count++;
		break;
	case 0x02:
		tpdo3_2_count++;
		break;
	case 0x03:
		tpdo3_3_count++;
		break;
	}
	tpdo3_count++;
	can_frame_free(f);
}

static int rpdo4_count = 0;
static void rpdo4_rx_cb(struct device *dev, struct can_frame* f)
{
	rpdo4_count++;
	can_frame_free(f);
}

static int tpdo4_count = 0;
static int tpdo4_0_count = 0;
static int tpdo4_1_count = 0;
static int tpdo4_2_count = 0;
static int tpdo4_3_count = 0;
static void tpdo4_rx_cb(struct device *dev, struct can_frame* f)
{
	switch((f->id >> 5) & 0x03) {
	case 0x00:
		tpdo4_0_count++;
		break;
	case 0x01:
		tpdo4_1_count++;
		break;
	case 0x02:
		tpdo4_2_count++;
		break;
	case 0x03:
		tpdo4_3_count++;
		break;
	}

	tpdo4_count++;
	can_frame_free(f);
}

static int tsdo_count = 0;
static void tsdo_rx_cb(struct device *dev, struct can_frame* f)
{
	tsdo_count++;
	can_frame_free(f);
}

static int rsdo_count = 0;
static void rsdo_rx_cb(struct device *dev, struct can_frame* f)
{
	rsdo_count++;
	can_frame_free(f);
}

static int nmt_ec_count = 0;
static void nmt_ec_rx_cb(struct device *dev, struct can_frame* f)
{
	nmt_ec_count++;
	can_frame_free(f);
}

static int tlss_count = 0;
static void tlss_rx_cb(struct device *dev, struct can_frame* f)
{
	tlss_count++;
	can_frame_free(f);
}

static int rlss_count = 0;
static void rlss_rx_cb(struct device *dev, struct can_frame* f)
{
	rlss_count++;
	can_frame_free(f);
}

static void can_2_rx_cb(struct device *dev, struct can_frame* f)
{
	printf("can2_rx_cb can_id=%08x\n", f->id);
	can_frame_free(f);
}

void main(void)
{
	struct device *can_1_dev;
	struct device *can_2_dev;

	printf("CAN TEST START\n");

	can_1_dev = device_get_binding(CONFIG_CAN_STM32_BXCAN_PORT_1_DEV_NAME);
	if (!can_1_dev) {
		printf("CAN_1 can driver was not found!\n");
		return;
	}

	can_2_dev = device_get_binding(CONFIG_CAN_STM32_BXCAN_PORT_2_DEV_NAME);
	if (!can_2_dev) {
		printf("CAN_2 can driver was not found!\n");
		return;
	}

	if (can_set_bitrate(can_1_dev, 500000) < 0) {
		printf("CAN1 failed to set bitrate\n");
	}
	if (can_set_bitrate(can_2_dev, 500000) < 0) {
		printf("CAN2 failed to set bitrate\n");
	}

	if (can_register_recv_callbacks(can_1_dev, can_1_cbs, sizeof(can_1_cbs)/sizeof(struct can_recv_cb)) < 0) {
		printf("CAN1 failed to set callbacks\n");
	}
	if (can_register_recv_callbacks(can_2_dev, can_2_cbs, sizeof(can_2_cbs)/sizeof(struct can_recv_cb)) < 0) {
		printf("CAN2 failed to set callbacks\n");
	}

	if (can_set_mode(can_1_dev, CAN_NORMAL_MODE) < 0)  {
		printf("CAN1 failed to set mode\n");
	}
	if (can_set_mode(can_2_dev, CAN_NORMAL_MODE) < 0) {
		printf("CAN2 failed to set mode\n");
	}

	while(1) {
		struct can_frame *f;
		if (can_frame_alloc(&f, K_FOREVER) < 0) {
			// OOM
		} else {
			f->id = (0x123 & CAN_EFF_MASK) | CAN_EFF_FLAG;
			f->dlc = 4;
			f->data[0] = 0x11;
			f->data[1] = 0x12;
			f->data[2] = 0x13;
			f->data[3] = 0x14;

			if (can_send(can_1_dev, f) < 0) {
				can_frame_free(f);
			}
		}

		k_sleep(1000);

		printf("--------------------------------------\n");
		printf("nmt: %d\n", nmt_count);
		printf("sync: %d\n", sync_count);
		printf("emcy: %d\n", emcy_count);
		printf("time: %d\n", time_count);
		printf("rpdo1: %d\n", rpdo1_count);
		printf("tpdo1: %d %d %d %d %d\n", tpdo1_count, tpdo1_0_count, tpdo1_1_count, tpdo1_2_count, tpdo1_3_count);
		printf("rpdo2: %d\n", rpdo2_count);
		printf("tpdo2: %d %d %d %d %d\n", tpdo2_count, tpdo2_0_count, tpdo2_1_count, tpdo2_2_count, tpdo2_3_count);
		printf("rpdo3: %d\n", rpdo3_count);
		printf("tpdo3: %d %d %d %d %d\n", tpdo3_count, tpdo3_0_count, tpdo3_1_count, tpdo3_2_count, tpdo3_3_count);
		printf("rpdo4: %d\n", rpdo4_count);
		printf("tpdo4: %d %d %d %d %d\n", tpdo4_count, tpdo4_0_count, tpdo4_1_count, tpdo4_2_count, tpdo4_3_count);
		printf("tsdo: %d\n", tsdo_count);
		printf("rsdo: %d\n", rsdo_count);
		printf("nmt_ec: %d\n", nmt_ec_count);
		printf("tlss: %d\n", tlss_count);
		printf("rlss: %d\n", rlss_count);
	}

	printf("CAN TEST END\n");
}

