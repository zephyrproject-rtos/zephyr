/*
 * Copyright (c) 2024 VCI Development - LPC54S018J4MET180E
 * Private Porting , by David Hor - Xtooltech 2025, david.hor@xtooltech.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>

/* Get the CAN device */
#if DT_NODE_HAS_STATUS(DT_ALIAS(can0), okay)
#define CAN_DEV_NODE DT_ALIAS(can0)
#elif DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_canbus), okay)
#define CAN_DEV_NODE DT_CHOSEN(zephyr_canbus)
#else
#error "No CAN device defined"
#endif

/* Define some test CAN IDs */
#define TEST_CAN_STD_ID     0x123
#define TEST_CAN_EXT_ID     0x1234567

/* CAN frame work buffer */
static struct can_frame rx_frame;
static struct k_sem rx_sem;

/* CAN filter for receiving */
static const struct can_filter rx_filter = {
	.flags = CAN_FILTER_DATA | CAN_FILTER_STD,
	.id = TEST_CAN_STD_ID,
	.mask = CAN_STD_ID_MASK
};

/* RX callback function */
static void can_rx_callback(const struct device *dev, struct can_frame *frame, void *user_data)
{
	memcpy(&rx_frame, frame, sizeof(rx_frame));
	k_sem_give(&rx_sem);
}

static int setup_can(const struct device *can_dev)
{
	int ret;

	/* Set CAN mode and timing */
	ret = can_set_mode(can_dev, CAN_MODE_NORMAL);
	if (ret != 0) {
		printk("ERR: Failed to set CAN mode: %d\n", ret);
		return ret;
	}

	/* Set CAN timing for 500 kbps */
	struct can_timing timing = {
		.sjw = 1,
		.prop_seg = 0,
		.phase_seg1 = 5,
		.phase_seg2 = 6,
		.prescaler = 8
	};

	ret = can_set_timing(can_dev, &timing);
	if (ret != 0) {
		printk("ERR: Failed to set CAN timing: %d\n", ret);
		return ret;
	}

	/* Add RX filter */
	ret = can_add_rx_filter(can_dev, can_rx_callback, NULL, &rx_filter);
	if (ret < 0) {
		printk("ERR: Failed to add RX filter: %d\n", ret);
		return ret;
	}

	/* Start CAN controller */
	ret = can_start(can_dev);
	if (ret != 0) {
		printk("ERR: Failed to start CAN: %d\n", ret);
		return ret;
	}

	return 0;
}

static void send_test_frame(const struct device *can_dev)
{
	struct can_frame tx_frame = {
		.flags = 0,
		.id = TEST_CAN_STD_ID,
		.dlc = 8
	};

	/* Fill test data */
	for (int i = 0; i < 8; i++) {
		tx_frame.data[i] = i;
	}

	int ret = can_send(can_dev, &tx_frame, K_MSEC(100), NULL, NULL);
	if (ret != 0) {
		printk("ERR: Failed to send CAN frame: %d\n", ret);
	} else {
		printk("Sent CAN frame ID: 0x%03X, DLC: %d\n", tx_frame.id, tx_frame.dlc);
	}
}

void main(void)
{
	const struct device *can_dev;

	printk("=== LPC54S018 CAN Test ===\n");
	
	/* Initialize semaphore */
	k_sem_init(&rx_sem, 0, 1);

	/* Get CAN device */
	can_dev = DEVICE_DT_GET(CAN_DEV_NODE);
	if (!device_is_ready(can_dev)) {
		printk("ERR: CAN device not ready\n");
		return;
	}

	printk("CAN device ready\n");

	/* Configure CAN */
	if (setup_can(can_dev) != 0) {
		printk("ERR: Failed to setup CAN\n");
		return;
	}

	printk("CAN configured for 500 kbps\n");

	/* Print CAN info */
	struct can_bus_err_cnt err_cnt;
	enum can_state state;
	
	state = can_get_state(can_dev, &err_cnt);
	printk("CAN State: %d, TX errors: %d, RX errors: %d\n", 
	       state, err_cnt.tx_err_cnt, err_cnt.rx_err_cnt);

	/* Main loop */
	printk("Starting CAN test loop...\n");
	
	while (1) {
		/* Send a test frame every 2 seconds */
		send_test_frame(can_dev);

		/* Wait for RX or timeout */
		if (k_sem_take(&rx_sem, K_SECONDS(2)) == 0) {
			printk("Received CAN frame ID: 0x%03X, DLC: %d, Data: ",
			       rx_frame.id, rx_frame.dlc);
			
			for (int i = 0; i < rx_frame.dlc; i++) {
				printk("%02X ", rx_frame.data[i]);
			}
			printk("\n");
		}

		/* Check CAN state */
		state = can_get_state(can_dev, &err_cnt);
		if (state != CAN_STATE_ERROR_ACTIVE) {
			printk("WRN: CAN State changed: %d\n", state);
		}

		k_sleep(K_SECONDS(2));
	}
}