/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_socket_can_sample, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>

#include <zephyr/drivers/can.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socketcan.h>
#include <zephyr/net/socketcan_utils.h>

#define PRIORITY  k_thread_priority_get(k_current_get())
#define STACKSIZE 1024
#define SLEEP_PERIOD K_SECONDS(1)

static k_tid_t tx_tid;
static K_THREAD_STACK_DEFINE(tx_stack, STACKSIZE);
static struct k_thread tx_data;

/* For testing purposes, we create another RX receiver if configured so */
#if CONFIG_NET_SOCKETS_CAN_RECEIVERS == 2
static k_tid_t rx_tid;
static K_THREAD_STACK_DEFINE(rx_stack, STACKSIZE);
static struct k_thread rx_data;
#endif

#define CLOSE_PERIOD 15

static const struct can_filter zfilter = {
	.flags = CAN_FILTER_DATA,
	.id = 0x1,
	.mask = CAN_STD_ID_MASK
};

static struct socketcan_filter sfilter;

static void tx(int *can_fd)
{
	int fd = POINTER_TO_INT(can_fd);
	struct can_frame zframe = {0};
	struct socketcan_frame sframe = {0};
	int ret, i;

	zframe.id = 0x1;
	zframe.dlc = 8U;

	for (i = 0; i < zframe.dlc; i++) {
		zframe.data[i] = 0xF0 | i;
	}

	socketcan_from_can_frame(&zframe, &sframe);

	LOG_DBG("Sending CAN data...");

	while (1) {
		ret = send(fd, &sframe, sizeof(sframe), 0);
		if (ret < 0) {
			LOG_ERR("Cannot send CAN frame (%d)", -errno);
		}

		k_sleep(SLEEP_PERIOD);
	}
}

static int create_socket(const struct socketcan_filter *sfilter)
{
	struct sockaddr_can can_addr;
	int fd, ret;

	fd = socket(AF_CAN, SOCK_RAW, CAN_RAW);
	if (fd < 0) {
		LOG_ERR("Cannot create %s CAN socket (%d)", "2nd", fd);
		return fd;
	}

	can_addr.can_ifindex = net_if_get_by_iface(
		net_if_get_first_by_type(&NET_L2_GET_NAME(CANBUS_RAW)));
	can_addr.can_family = PF_CAN;

	ret = bind(fd, (struct sockaddr *)&can_addr, sizeof(can_addr));
	if (ret < 0) {
		LOG_ERR("Cannot bind %s CAN socket (%d)", "2nd", -errno);
		(void)close(fd);
		return ret;
	}

	(void)setsockopt(fd, SOL_CAN_RAW, CAN_RAW_FILTER, sfilter,
			 sizeof(*sfilter));

	return fd;
}

static void rx(int *can_fd, int *do_close_period,
	       const struct socketcan_filter *sfilter)
{
	int close_period = POINTER_TO_INT(do_close_period);
	int fd = POINTER_TO_INT(can_fd);
	struct sockaddr_can can_addr;
	socklen_t addr_len;
	struct can_frame zframe;
	struct socketcan_frame sframe;
	int ret;

	LOG_DBG("[%d] Waiting CAN data...", fd);

	while (1) {
		uint8_t *data;

		memset(&sframe, 0, sizeof(sframe));
		addr_len = sizeof(can_addr);

		ret = recvfrom(fd, &sframe, sizeof(struct socketcan_frame),
			       0, (struct sockaddr *)&can_addr, &addr_len);
		if (ret < 0) {
			LOG_ERR("[%d] Cannot receive CAN frame (%d)", fd,
				-errno);
			continue;
		}

		socketcan_to_can_frame(&sframe, &zframe);

		LOG_INF("[%d] CAN frame: IDE 0x%x RTR 0x%x ID 0x%x DLC 0x%x",
			fd,
			(zframe.flags & CAN_FRAME_IDE) != 0 ? 1 : 0,
			(zframe.flags & CAN_FRAME_RTR) != 0 ? 1 : 0,
			zframe.id, zframe.dlc);

		if ((zframe.flags & CAN_FRAME_RTR) != 0) {
			LOG_INF("[%d] EXT Remote frame received", fd);
		} else {
			if (zframe.dlc > 8) {
				data = (uint8_t *)zframe.data_32;
			} else {
				data = zframe.data;
			}

			LOG_HEXDUMP_INF(data, zframe.dlc, "Data");
		}

		if (POINTER_TO_INT(do_close_period) > 0) {
			close_period--;
			if (close_period <= 0) {
				(void)close(fd);

				k_sleep(K_SECONDS(1));

				fd = create_socket(sfilter);
				if (fd < 0) {
					LOG_ERR("Cannot get socket (%d)",
						-errno);
					return;
				}

				close_period = POINTER_TO_INT(do_close_period);
			}
		}
	}
}

static int setup_socket(void)
{
	struct sockaddr_can can_addr;
	struct net_if *iface;
	int fd, rx_fd;
	int ret;

	socketcan_from_can_filter(&zfilter, &sfilter);

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(CANBUS_RAW));
	if (!iface) {
		LOG_ERR("No CANBUS network interface found!");
		return -ENOENT;
	}

	fd = socket(AF_CAN, SOCK_RAW, CAN_RAW);
	if (fd < 0) {
		ret = -errno;
		LOG_ERR("Cannot create %s CAN socket (%d)", "1st", ret);
		return ret;
	}

	can_addr.can_ifindex = net_if_get_by_iface(iface);
	can_addr.can_family = PF_CAN;

	ret = bind(fd, (struct sockaddr *)&can_addr, sizeof(can_addr));
	if (ret < 0) {
		ret = -errno;
		LOG_ERR("Cannot bind %s CAN socket (%d)", "1st", ret);
		goto cleanup;
	}

	ret = setsockopt(fd, SOL_CAN_RAW, CAN_RAW_FILTER, &sfilter,
			 sizeof(sfilter));
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

	LOG_INF("1st RX fd %d", fd);

	rx_fd = fd;

#if CONFIG_NET_SOCKETS_CAN_RECEIVERS == 2
	fd = create_socket(&sfilter);
	if (fd >= 0) {
		rx_tid = k_thread_create(&rx_data, rx_stack,
					 K_THREAD_STACK_SIZEOF(rx_stack),
					 (k_thread_entry_t)rx,
					 INT_TO_POINTER(fd),
					 INT_TO_POINTER(CLOSE_PERIOD),
					 &sfilter, PRIORITY, 0, K_NO_WAIT);
		if (!rx_tid) {
			ret = -ENOENT;
			errno = -ret;
			LOG_ERR("Cannot create 2nd RX thread!");
			goto cleanup2;
		}

		LOG_INF("2nd RX fd %d", fd);
	} else {
		LOG_ERR("2nd RX not created (%d)", fd);
	}
#endif

	return rx_fd;

#if CONFIG_NET_SOCKETS_CAN_RECEIVERS == 2
cleanup2:
	(void)close(rx_fd);
#endif

cleanup:
	(void)close(fd);
	return ret;
}

int main(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
	int ret;
	int fd;

#ifdef CONFIG_SAMPLE_SOCKETCAN_LOOPBACK_MODE
	ret = can_set_mode(dev, CAN_MODE_LOOPBACK);
	if (ret != 0) {
		LOG_ERR("Cannot set CAN loopback mode (%d)", ret);
		return 0;
	}
#endif

	ret = can_start(dev);
	if (ret != 0) {
		LOG_ERR("Cannot start CAN controller (%d)", ret);
		return 0;
	}

	/* Let the device start before doing anything */
	k_sleep(K_SECONDS(2));

	fd = setup_socket();
	if (fd < 0) {
		LOG_ERR("Cannot start CAN application (%d)", fd);
		return 0;
	}

	rx(INT_TO_POINTER(fd), NULL, NULL);
	return 0;
}
