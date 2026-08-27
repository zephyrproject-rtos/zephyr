/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2025 Siratul Islam
 */

#ifndef ZEPHYR_DRIVERS_BIOMETRICS_BIOMETRICS_ZFM_X0_H_
#define ZEPHYR_DRIVERS_BIOMETRICS_BIOMETRICS_ZFM_X0_H_

#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/biometrics.h>

/* Protocol constants */
#define ZFM_X0_START_CODE       0xEF01
#define ZFM_X0_DEFAULT_ADDRESS  0xFFFFFFFF
#define ZFM_X0_DEFAULT_PASSWORD 0x00000000

/* Packet identifiers */
#define ZFM_X0_PID_COMMAND  0x01
#define ZFM_X0_PID_DATA     0x02
#define ZFM_X0_PID_ACK      0x07
#define ZFM_X0_PID_END_DATA 0x08

/* Command codes */
#define ZFM_X0_CMD_GET_IMAGE      0x01
#define ZFM_X0_CMD_IMG_2_TZ       0x02
#define ZFM_X0_CMD_MATCH          0x03
#define ZFM_X0_CMD_SEARCH         0x04
#define ZFM_X0_CMD_REG_MODEL      0x05
#define ZFM_X0_CMD_STORE          0x06
#define ZFM_X0_CMD_LOAD           0x07
#define ZFM_X0_CMD_UP_CHAR        0x08
#define ZFM_X0_CMD_DOWN_CHAR      0x09
#define ZFM_X0_CMD_DELETE         0x0C
#define ZFM_X0_CMD_EMPTY          0x0D
#define ZFM_X0_CMD_SET_PARAM      0x0E
#define ZFM_X0_CMD_READ_PARAM     0x0F
#define ZFM_X0_CMD_VERIFY_PWD     0x13
#define ZFM_X0_CMD_TEMPLATE_COUNT 0x1D
#define ZFM_X0_CMD_READ_INDEX     0x1F
#define ZFM_X0_CMD_LED_CONFIG     0x35

/* LED control parameters */
#define ZFM_X0_LED_CTRL_BREATHING 0x01
#define ZFM_X0_LED_CTRL_FLASHING  0x02
#define ZFM_X0_LED_CTRL_ON        0x03
#define ZFM_X0_LED_CTRL_OFF       0x04

#define ZFM_X0_LED_COLOR_RED    0x01
#define ZFM_X0_LED_COLOR_BLUE   0x02
#define ZFM_X0_LED_COLOR_PURPLE 0x03

#define ZFM_X0_LED_SPEED_SLOW   0x80
#define ZFM_X0_LED_SPEED_MEDIUM 0x50

/* Confirmation codes */
#define ZFM_X0_OK                0x00
#define ZFM_X0_ERR_RECV          0x01
#define ZFM_X0_ERR_NO_FINGER     0x02
#define ZFM_X0_ERR_ENROLL_FAIL   0x03
#define ZFM_X0_ERR_BAD_IMAGE     0x06
#define ZFM_X0_ERR_TOO_FEW       0x07
#define ZFM_X0_ERR_NO_MATCH      0x08
#define ZFM_X0_ERR_NOT_FOUND     0x09
#define ZFM_X0_ERR_MERGE_FAIL    0x0A
#define ZFM_X0_ERR_BAD_LOCATION  0x0B
#define ZFM_X0_ERR_FLASH_ERR     0x18
#define ZFM_X0_ERR_INVALID_REG   0x1A
#define ZFM_X0_ERR_INVALID_IMAGE 0x15

/* Buffer indices */
#define ZFM_X0_BUFFER_1 0x01
#define ZFM_X0_BUFFER_2 0x02

/* System parameters */
#define ZFM_X0_PARAM_BAUD     4
#define ZFM_X0_PARAM_SECURITY 5
#define ZFM_X0_PARAM_PKG_SIZE 6

/* Packet structure offsets */
#define ZFM_X0_HDR_SIZE      9
#define ZFM_X0_CHECKSUM_SIZE 2
#define ZFM_X0_MAX_DATA_LEN  256

/* Data sizes */
#define ZFM_X0_TEMPLATE_SIZE       512
#define ZFM_X0_DATA_PACKET_SIZE    128
#define ZFM_X0_SYS_PARAMS_SIZE     32
#define ZFM_X0_MATCH_RESPONSE_SIZE 16
#define ZFM_X0_INDEX_TABLE_SIZE    33

/* Maximum packet size - use safe max of data + header + checksum */
#define ZFM_X0_MAX_PACKET_SIZE (ZFM_X0_HDR_SIZE + ZFM_X0_MAX_DATA_LEN + ZFM_X0_CHECKSUM_SIZE)

/* Polling interval */
#define ZFM_X0_FINGER_POLL_MS 100

/* UART packet timeout */
#define ZFM_X0_UART_TIMEOUT_MS 500

/* Maximum reasonable timeout to prevent overflow (1 hour) */
#define ZFM_X0_MAX_TIMEOUT_MS (3600 * 1000U)

/* Enrollment state */
enum zfm_x0_enroll_state {
	ZFM_X0_ENROLL_IDLE,
	ZFM_X0_ENROLL_WAIT_SAMPLE_1,
	ZFM_X0_ENROLL_WAIT_SAMPLE_2,
	ZFM_X0_ENROLL_READY,
};

/* RX error flags */
enum zfm_x0_rx_error {
	ZFM_X0_RX_OK = 0,
	ZFM_X0_RX_OVERFLOW,
	ZFM_X0_RX_INVALID_LEN,
};

/* Packet structure */
struct zfm_x0_packet {
	uint8_t buf[ZFM_X0_MAX_PACKET_SIZE];
	volatile uint16_t len;
	volatile uint16_t offset; /* Current TX position for ISR */
};

/* Driver data */
struct zfm_x0_data {
	const struct device *dev;

	struct k_mutex lock;
	struct k_spinlock irq_lock;

	struct k_sem uart_tx_sem;
	struct k_sem uart_rx_sem;

	struct zfm_x0_packet tx_pkt;
	struct zfm_x0_packet rx_pkt;
	volatile uint16_t rx_expected;
	volatile enum zfm_x0_rx_error rx_error;

	enum zfm_x0_enroll_state enroll_state;
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
struct zfm_x0_config {
	const struct device *uart_dev;
	uint32_t comm_addr;
	uint32_t password;
};

#endif /* ZEPHYR_DRIVERS_BIOMETRICS_BIOMETRICS_ZFM_X0_H_ */
