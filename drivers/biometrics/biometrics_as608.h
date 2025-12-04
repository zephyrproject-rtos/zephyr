/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2025 Siratul Islam
 */

#ifndef ZEPHYR_DRIVERS_BIOMETRICS_BIOMETRICS_AS608_H_
#define ZEPHYR_DRIVERS_BIOMETRICS_BIOMETRICS_AS608_H_

#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/biometrics.h>

/* Protocol constants */
#define AS608_START_CODE       0xEF01
#define AS608_DEFAULT_ADDRESS  0xFFFFFFFF
#define AS608_DEFAULT_PASSWORD 0x00000000

/* Packet identifiers */
#define AS608_PID_COMMAND  0x01
#define AS608_PID_DATA     0x02
#define AS608_PID_ACK      0x07
#define AS608_PID_END_DATA 0x08

/* Command codes */
#define AS608_CMD_GET_IMAGE      0x01
#define AS608_CMD_IMG_2_TZ       0x02
#define AS608_CMD_MATCH          0x03
#define AS608_CMD_SEARCH         0x04
#define AS608_CMD_REG_MODEL      0x05
#define AS608_CMD_STORE          0x06
#define AS608_CMD_LOAD           0x07
#define AS608_CMD_DELETE         0x0C
#define AS608_CMD_EMPTY          0x0D
#define AS608_CMD_SET_PARAM      0x0E
#define AS608_CMD_READ_PARAM     0x0F
#define AS608_CMD_VERIFY_PWD     0x13
#define AS608_CMD_TEMPLATE_COUNT 0x1D
#define AS608_CMD_READ_INDEX     0x1F
#define AS608_CMD_LED_CONFIG     0x35

/* LED control parameters */
#define AS608_LED_CTRL_BREATHING 0x01
#define AS608_LED_CTRL_FLASHING  0x02
#define AS608_LED_CTRL_ON        0x03
#define AS608_LED_CTRL_OFF       0x04

#define AS608_LED_COLOR_RED    0x01
#define AS608_LED_COLOR_BLUE   0x02
#define AS608_LED_COLOR_PURPLE 0x03

#define AS608_LED_SPEED_SLOW   0x80
#define AS608_LED_SPEED_MEDIUM 0x50

/* Confirmation codes */
#define AS608_OK                0x00
#define AS608_ERR_RECV          0x01
#define AS608_ERR_NO_FINGER     0x02
#define AS608_ERR_ENROLL_FAIL   0x03
#define AS608_ERR_BAD_IMAGE     0x06
#define AS608_ERR_TOO_FEW       0x07
#define AS608_ERR_NO_MATCH      0x08
#define AS608_ERR_NOT_FOUND     0x09
#define AS608_ERR_MERGE_FAIL    0x0A
#define AS608_ERR_BAD_LOCATION  0x0B
#define AS608_ERR_FLASH_ERR     0x18
#define AS608_ERR_INVALID_REG   0x1A
#define AS608_ERR_INVALID_IMAGE 0x15

/* Buffer indices */
#define AS608_BUFFER_1 0x01
#define AS608_BUFFER_2 0x02

/* System parameters */
#define AS608_PARAM_BAUD     4
#define AS608_PARAM_SECURITY 5
#define AS608_PARAM_PKG_SIZE 6

/* Packet structure offsets */
#define AS608_HDR_SIZE      9
#define AS608_CHECKSUM_SIZE 2

/* Maximum packet size - use safe max of 256 bytes data + header + checksum */
#define AS608_MAX_PACKET_SIZE (AS608_HDR_SIZE + 256 + AS608_CHECKSUM_SIZE)

/* Polling interval */
#define AS608_FINGER_POLL_MS 100

/* UART packet timeout */
#define AS608_UART_TIMEOUT_MS 500

/* Maximum reasonable timeout to prevent overflow (1 hour) */
#define AS608_MAX_TIMEOUT_MS (3600 * 1000U)

/* Enrollment state */
enum as608_enroll_state {
	AS608_ENROLL_IDLE,
	AS608_ENROLL_WAIT_SAMPLE_1,
	AS608_ENROLL_WAIT_SAMPLE_2,
	AS608_ENROLL_READY,
};

/* RX error flags */
enum as608_rx_error {
	AS608_RX_OK = 0,
	AS608_RX_OVERFLOW,
	AS608_RX_INVALID_LEN,
};

/* Packet structure */
struct as608_packet {
	uint8_t buf[AS608_MAX_PACKET_SIZE];
	volatile uint16_t len;
	volatile uint16_t offset; /* Current TX position for ISR */
};

/* Driver data */
struct as608_data {
	const struct device *dev;

	struct k_mutex lock;
	struct k_spinlock irq_lock;

	struct k_sem uart_tx_sem;
	struct k_sem uart_rx_sem;

	struct as608_packet tx_pkt;
	struct as608_packet rx_pkt;
	volatile uint16_t rx_expected;
	volatile enum as608_rx_error rx_error;

	enum as608_enroll_state enroll_state;
	uint16_t enroll_id;

	uint16_t template_count;
	uint16_t max_templates;
	uint32_t comm_addr;
	uint16_t last_match_id;

	int32_t match_threshold;
	int32_t enroll_quality;
	int32_t security_level;
	uint32_t timeout_ms;
	int32_t image_quality;

	enum biometric_led_state led_state;
};

/* Driver configuration */
struct as608_config {
	const struct device *uart_dev;
	uint32_t comm_addr;
	uint32_t password;
};

#endif /* ZEPHYR_DRIVERS_BIOMETRICS_BIOMETRICS_AS608_H_ */
