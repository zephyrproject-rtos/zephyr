/*
 * Copyright (c) 2019 Ioannis <gon1332> Konstantelias
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ucam3.h"
#include <device.h>
#include <tty.h>
#include <string.h>
#include <stdbool.h>

#define CAM_UART_NAME DT_UART_STM32_USART_2_NAME

#define CONFIG_BUFFER_SER_UART_RX_SIZE 128

static struct tty_serial cam;

typedef enum {
	ACK_INITIAL,
	ACK_SYNC,
} cmd_ack_t;

#define CMD_LEN 6

#define CMD_INITIAL_ID 0x01
#define CMD_SYNC_ID 0x0D
#define CMD_ACK_ID 0x0E

u8_t cmd_sync[CMD_LEN] = { 0xAA, CMD_SYNC_ID, 0x00, 0x00, 0x00, 0x00 };
u8_t cmd_sync_ack[CMD_LEN] = { 0xAA, CMD_ACK_ID, CMD_SYNC_ID, 0x00, 0x00, 0x00 };
u8_t cmd_initial[CMD_LEN] = { 0xAA, CMD_INITIAL_ID, 0x00, 0x00, 0x00, 0x00 };
u8_t cmd_initial_ack[CMD_LEN] = { 0xAA, CMD_ACK_ID, CMD_INITIAL_ID, 0x00, 0x00, 0x00 };

static struct tty_serial cam;
static u8_t cam_rxbuf[64];

static u8_t resp[CMD_LEN];

static bool cam_check_ack(const uint8_t *buf, cmd_ack_t ack_type)
{
	/* Check the Command and ID Number */
	if (buf[0] != 0xAA || buf[1] != CMD_ACK_ID) {
		return false;
	}

	/* Check the Command ID */
	switch (ack_type) {
	case ACK_SYNC:
		if (buf[2] != CMD_SYNC_ID) {
			return false;
		}
		break;
	case ACK_INITIAL:
		if (buf[2] != CMD_INITIAL_ID) {
			return false;
		}
		break;
	default:
		break;

	}

	/* The 2 last fields should be 0x00 */
	return buf[4] == 0x00 && buf[5] == 0x00;
}

static void print_cmd(uint8_t *cmd, int size)
{
	for (int i = 0; i < size; i++) {
		printk("0x%02x ", cmd[i]);
	}
	printk("\n");
}

void ucam3_reset(void)
{
	uint8_t b;

	while (tty_read(&cam, &b, 1) > 0) {
		continue;
	}
}

int ucam3_sync(void)
{
	u8_t i;
	u8_t ret;
	s32_t delay = 5;

	for (i = 1; i <= 60; i++) {
		/* Send SYNC to the camera */
		ret = tty_write(&cam, cmd_sync, CMD_LEN);
		if (ret == CMD_LEN) {
			/* Read response from the camera */
			ret = tty_read(&cam, resp, CMD_LEN);
			if (ret == CMD_LEN && cam_check_ack(resp, ACK_SYNC)) {
				/* Camera responded with ACK */
				break;
			}
		}

		/* Wait for some time before retrying
		 * pg.13, Note 1
		 */
		k_sleep(delay++);
	}

	if (i > 60) {
		return -1;
	}

	/* Wait for the SYNC from the camera */
	ret = tty_read(&cam, resp, CMD_LEN);
	if (ret != CMD_LEN) {
		return -1;
	}

	/* Check if camera responded with SYNC */
	if (memcmp(resp, cmd_sync, CMD_LEN)) {
		return -1;
	}

	/* Send back ACK */
	ret = tty_write(&cam, cmd_sync_ack, CMD_LEN);
	if (ret != CMD_LEN) {
		return -1;
	}

	return 0;
}

/**
 * @brief Construct INITIAL command out of image_config_t
 *
 * @param[out] cmd The INITIAL command
 * @param[in] image_config The image settings
 *
 * @retval 0 on success, -1 otherwise
 */
static int get_initial_cmd(uint8_t *cmd, const image_config_t *image_config)
{
	bool is_raw;

	if (!cmd || !image_config) {
		return -1;
	}

	cmd[0] = 0xAA;                  /* Command */
	cmd[1] = CMD_INITIAL_ID;        /* ID Number */
	cmd[2] = 0x00;                  /* Parameter1*/
	cmd[3] = image_config->format;  /* Parameter2: Image Format */

	switch (image_config->format) {
	case FMT_8BIT_GRAY_SCALE:
	case FMT_16BIT_COLOUR_RAW_CRYCBY:
	case FMT_16BIT_COLOUR_RAW_RGB:
		is_raw = true;
		break;
	case FMT_JPEG:
		is_raw = false;
		break;
	default:
		return -1;
		break;
	}

	if (is_raw) {
		cmd[4] = image_config->size.raw;        /* Paramater3: RAW Resolution */
		cmd[5] = JPEG_INVALID;                  /* Paramater4: JPEG Resolution */
	} else {
		cmd[4] = RAW_INVALID;                   /* Paramater3: RAW Resolution */
		cmd[5] = image_config->size.jpeg;       /* Paramater4: JPEG Resolution */
	}

	return 0;
}

int ucam3_initial(const image_config_t *image_config)
{
	int ret;
	static u8_t buf[CMD_LEN];

	/* Construct INITIAL command out of image_config */
	ret = get_initial_cmd(buf, image_config);
	if (ret) {
		return -1;
	}

	/* Send INITIAL to the camera */
	ret = tty_write(&cam, buf, CMD_LEN);
	if (ret != CMD_LEN) {
		return -1;
	}

	/* Read response from the camera */
	ret = tty_read(&cam, resp, CMD_LEN);
	if (ret == CMD_LEN && cam_check_ack(resp, ACK_INITIAL)) {
		/* Camera responded with ACK */
		return 0;
	}

	return -1;
}

int ucam3_create(void)
{
	struct device *uart;

	uart = device_get_binding(CAM_UART_NAME);
	if (!uart) {
		return -1;
	}

	tty_init(&cam, uart);
	tty_set_rx_buf(&cam, cam_rxbuf, sizeof(cam_rxbuf));
	tty_set_rx_timeout(&cam, 50);

	return 0;
}
