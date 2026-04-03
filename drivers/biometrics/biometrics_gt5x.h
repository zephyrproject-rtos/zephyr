/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2025 Siratul Islam
 */

#ifndef ZEPHYR_DRIVERS_BIOMETRICS_BIOMETRICS_GT5X_H_
#define ZEPHYR_DRIVERS_BIOMETRICS_BIOMETRICS_GT5X_H_

#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/biometrics.h>

/* Protocol constants */
#define GT5X_CMD_START_CODE1  0x55
#define GT5X_CMD_START_CODE2  0xAA
#define GT5X_DATA_START_CODE1 0x5A
#define GT5X_DATA_START_CODE2 0xA5
#define GT5X_DEVICE_ID        0x0001

/* Packet layout and sizes */
#define GT5X_CMD_CHECKSUM_OFFSET 10
#define GT5X_CMD_PACKET_SIZE     12
#define GT5X_RESP_PACKET_SIZE    12
#define GT5X_DATA_HDR_SIZE       4
#define GT5X_CHECKSUM_SIZE       2

/* Response codes */
#define GT5X_ACK  0x30
#define GT5X_NACK 0x31

/* Command codes */
#define GT5X_CMD_OPEN               0x01
#define GT5X_CMD_CLOSE              0x02
#define GT5X_CMD_USB_INTERNAL_CHECK 0x03
#define GT5X_CMD_CHANGE_BAUDRATE    0x04
#define GT5X_CMD_SET_IAP_MODE       0x05
#define GT5X_CMD_CMOS_LED           0x12
#define GT5X_CMD_GET_ENROLL_COUNT   0x20
#define GT5X_CMD_CHECK_ENROLLED     0x21
#define GT5X_CMD_ENROLL_START       0x22
#define GT5X_CMD_ENROLL_1           0x23
#define GT5X_CMD_ENROLL_2           0x24
#define GT5X_CMD_ENROLL_3           0x25
#define GT5X_CMD_IS_PRESS_FINGER    0x26
#define GT5X_CMD_DELETE_ID          0x40
#define GT5X_CMD_DELETE_ALL         0x41
#define GT5X_CMD_VERIFY             0x50
#define GT5X_CMD_IDENTIFY           0x51
#define GT5X_CMD_VERIFY_TEMPLATE    0x52
#define GT5X_CMD_IDENTIFY_TEMPLATE  0x53
#define GT5X_CMD_CAPTURE_FINGER     0x60
#define GT5X_CMD_MAKE_TEMPLATE      0x61
#define GT5X_CMD_GET_IMAGE          0x62
#define GT5X_CMD_GET_RAW_IMAGE      0x63
#define GT5X_CMD_GET_TEMPLATE       0x70
#define GT5X_CMD_SET_TEMPLATE       0x71
#define GT5X_CMD_GET_DATABASE_START 0x72
#define GT5X_CMD_GET_DATABASE_END   0x73
#define GT5X_CMD_UPGRADE_FIRMWARE   0x80
#define GT5X_CMD_UPGRADE_ISO_IMAGE  0x81

/* NACK error codes */
#define GT5X_NACK_TIMEOUT               0x1001
#define GT5X_NACK_INVALID_BAUDRATE      0x1002
#define GT5X_NACK_INVALID_POS           0x1003
#define GT5X_NACK_IS_NOT_USED           0x1004
#define GT5X_NACK_IS_ALREADY_USED       0x1005
#define GT5X_NACK_COMM_ERR              0x1006
#define GT5X_NACK_VERIFY_FAILED         0x1007
#define GT5X_NACK_IDENTIFY_FAILED       0x1008
#define GT5X_NACK_DB_IS_FULL            0x1009
#define GT5X_NACK_DB_IS_EMPTY           0x100A
#define GT5X_NACK_TURN_ERR              0x100B
#define GT5X_NACK_BAD_FINGER            0x100C
#define GT5X_NACK_ENROLL_FAILED         0x100D
#define GT5X_NACK_IS_NOT_SUPPORTED      0x100E
#define GT5X_NACK_DEV_ERR               0x100F
#define GT5X_NACK_CAPTURE_CANCELED      0x1010
#define GT5X_NACK_INVALID_PARAM         0x1011
#define GT5X_NACK_FINGER_IS_NOT_PRESSED 0x1012

/* Polling interval for finger detection */
#define GT5X_FINGER_POLL_MS 100

/* UART packet timeout */
#define GT5X_UART_TIMEOUT_MS 1000

/* Maximum reasonable timeout to prevent overflow */
#define GT5X_MAX_TIMEOUT_MS (3600 * 1000U)

/* Device info structure (24 bytes from Open command) */
struct gt5x_device_info {
	uint32_t firmware_version;
	uint32_t iso_area_max_size;
	uint8_t serial_number[16];
} __packed;

/* Enrollment state machine */
enum gt5x_enroll_state {
	GT5X_ENROLL_IDLE,
	GT5X_ENROLL_WAIT_SAMPLE_1,
	GT5X_ENROLL_WAIT_SAMPLE_2,
	GT5X_ENROLL_WAIT_SAMPLE_3,
	GT5X_ENROLL_READY,
};

/* RX error flags */
enum gt5x_rx_error {
	GT5X_RX_OK = 0,
	GT5X_RX_OVERFLOW,
	GT5X_RX_INVALID,
};

/* Packet structure for fixed-size command/response */
struct gt5x_packet {
	uint8_t buf[GT5X_CMD_PACKET_SIZE];
	volatile uint16_t len;
	volatile uint16_t offset;
};

/* Driver configuration from device tree */
struct gt5x_config {
	const struct device *uart_dev;
	uint16_t max_templates;
	uint16_t template_size;
};

/* Driver runtime data */
struct gt5x_data {
	const struct device *dev;

	struct k_mutex lock;
	struct k_spinlock irq_lock;

	struct k_sem uart_tx_sem;
	struct k_sem uart_rx_sem;

	struct gt5x_packet tx_pkt;
	struct gt5x_packet rx_pkt;
	volatile uint16_t rx_expected;
	volatile enum gt5x_rx_error rx_error;

	enum gt5x_enroll_state enroll_state;
	uint16_t enroll_id;

	struct gt5x_device_info devinfo;
	uint16_t enrolled_count;

	uint8_t *template_buf;

	bool led_on;
	uint16_t last_match_id;

	int32_t match_threshold;
	int32_t enroll_quality;
	int32_t security_level;
	int32_t timeout_ms;
};

#endif /* ZEPHYR_DRIVERS_BIOMETRICS_BIOMETRICS_GT5X_H_ */
