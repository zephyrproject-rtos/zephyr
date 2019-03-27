/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_socket_can_sample, LOG_LEVEL_DBG);

#include <zephyr.h>

#include <net/socket.h>
#include <net/socket_can.h>

#define PRIORITY  7
#define STACKSIZE 750
#define SLEEP_PERIOD K_SECONDS(1)

static k_tid_t tx_tid;
static K_THREAD_STACK_DEFINE(tx_stack, STACKSIZE);
static struct k_thread tx_data;

static void tx(int *can_fd)
{
	int fd = POINTER_TO_INT(can_fd);
	struct zcan_frame msg = {0};
	struct can_frame frame = {0};
	int ret, i;

	msg.dlc = 8U;
	msg.id_type = CAN_STANDARD_IDENTIFIER;
	msg.std_id = 0x1;
	msg.rtr = CAN_DATAFRAME;

	for (i = 0; i < msg.dlc; i++) {
		msg.data[i] = 0xF0 | i;
	}

	can_copy_zframe_to_frame(&msg, &frame);

	LOG_DBG("Sending CAN data...");

	while (1) {
		ret = send(fd, &frame, sizeof(frame), K_FOREVER);
		if (ret < 0) {
			LOG_ERR("Cannot send CAN message (%d)", -errno);
		}

		k_sleep(SLEEP_PERIOD);
	}
}

static void rx(int fd)
{
	struct sockaddr_can can_addr;
	socklen_t addr_len;
	struct zcan_frame msg;
	struct can_frame frame;
	int ret;

	LOG_DBG("Waiting CAN data...");

	while (1) {
		u8_t *data;

		memset(&frame, 0, sizeof(frame));
		addr_len = sizeof(can_addr);

		ret = recvfrom(fd, &frame, sizeof(struct can_frame),
			       0, (struct sockaddr *)&can_addr, &addr_len);
		if (ret < 0) {
			LOG_ERR("Cannot receive CAN message (%d)", ret);
			continue;
		}

		can_copy_frame_to_zframe(&frame, &msg);

		LOG_INF("CAN msg: type 0x%x RTR 0x%x EID 0x%x DLC 0x%x",
			msg.id_type, msg.rtr, msg.std_id, msg.dlc);

		if (!msg.rtr) {
			if (msg.dlc > 8) {
				data = (u8_t *)msg.data_32;
			} else {
				data = msg.data;
			}

			LOG_HEXDUMP_INF(data, msg.dlc, "Data");
		} else {
			LOG_INF("EXT Remote message received");
		}
	}
}

static int setup_socket(void)
{
	const struct zcan_filter zfilter = {
		.id_type = CAN_STANDARD_IDENTIFIER,
		.rtr = CAN_DATAFRAME,
		.std_id = 0x1,
		.rtr_mask = 1,
		.std_id_mask = CAN_STD_ID_MASK
	};
	struct can_filter filter;
	struct sockaddr_can can_addr;
	struct net_if *iface;
	int fd;
	int ret;

	can_copy_zfilter_to_filter(&zfilter, &filter);

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(CANBUS));
	if (!iface) {
		LOG_ERR("No CANBUS network interface found!");
		return -ENOENT;
	}

	fd = socket(AF_CAN, SOCK_RAW, CAN_RAW);
	if (fd < 0) {
		ret = -errno;
		LOG_ERR("Cannot create CAN socket (%d)", ret);
		return ret;
	}

	can_addr.can_ifindex = net_if_get_by_iface(iface);
	can_addr.can_family = PF_CAN;

	ret = bind(fd, (struct sockaddr *)&can_addr, sizeof(can_addr));
	if (ret < 0) {
		ret = -errno;
		LOG_ERR("Cannot bind CAN socket (%d)", ret);
		goto cleanup;
	}

	ret = setsockopt(fd, SOL_CAN_RAW, CAN_RAW_FILTER, &filter,
			 sizeof(filter));
	if (ret < 0) {
		ret = -errno;
		LOG_ERR("Cannot set CAN sockopt (%d)", ret);
		goto cleanup;
	}

	/* Delay TX startup so that RX is ready to receive */
	tx_tid = k_thread_create(&tx_data, tx_stack,
				 K_THREAD_STACK_SIZEOF(tx_stack),
				 (k_thread_entry_t)tx, INT_TO_POINTER(fd),
				 NULL, NULL, PRIORITY, 0, K_SECONDS(1));
	if (!tx_tid) {
		ret = -ENOENT;
		errno = -ret;
		LOG_ERR("Cannot create TX thread!");
		goto cleanup;
	}

	LOG_DBG("Started socket CAN TX thread");

	return fd;

cleanup:
	(void)close(fd);
	return ret;
}

void main(void)
{
	int fd;

	fd = setup_socket();
	if (fd < 0) {
		LOG_ERR("Cannot start CAN application (%d)", fd);
		return;
	}

	rx(fd);
}
