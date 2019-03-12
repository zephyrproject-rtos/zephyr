/*
 * Copyright (c) 2019 Ioannis <gon1332> Konstantelias
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ucam3.h"
#include <device.h>
#include <tty.h>
#include <misc/byteorder.h>
#include <string.h>
#include <stdbool.h>

#define CAM_UART_NAME DT_UART_STM32_USART_2_NAME

#define CONFIG_BUFFER_SER_UART_RX_SIZE 128
#define IMAGE_DATA_START 4
#define MAX_PACKAGE_SIZE 512

#define CMD_LEN 6

#define CMD_INITIAL_ID 0x01
#define CMD_GET_PICTURE_ID 0x04
#define CMD_SNAPSHOT_ID 0x05
#define CMD_SET_PACKAGE_SIZE_ID 0x06
#define CMD_DATA_ID 0x0A
#define CMD_SYNC_ID 0x0D
#define CMD_ACK_ID 0x0E

static struct {
	u16_t package_size;
	u8_t package_buf[MAX_PACKAGE_SIZE];
	u8_t resp[CMD_LEN];
} cam_data;

static struct tty_serial cam;

typedef enum {
	ACK_INITIAL,
	ACK_SNAPSHOT,
	ACK_GET_PICTURE,
	ACK_SET_PACKAGE_SIZE,
	ACK_SYNC,
} cmd_ack_t;

u8_t cmd_sync[CMD_LEN] = { 0xAA, CMD_SYNC_ID, 0x00, 0x00, 0x00, 0x00 };
u8_t cmd_sync_ack[CMD_LEN] = { 0xAA, CMD_ACK_ID, CMD_SYNC_ID, 0x00, 0x00, 0x00 };
u8_t cmd_initial[CMD_LEN] = { 0xAA, CMD_INITIAL_ID, 0x00, 0x00, 0x00, 0x00 };
u8_t cmd_initial_ack[CMD_LEN] = { 0xAA, CMD_ACK_ID, CMD_INITIAL_ID, 0x00, 0x00, 0x00 };
u8_t cmd_get_picture[CMD_LEN] = { 0xAA, CMD_GET_PICTURE_ID, 0x01, 0x00, 0x00, 0x00 };
u8_t cmd_data[CMD_LEN] = { 0xAA, CMD_DATA_ID, 0x01, 0x00, 0x00, 0x00 };
u8_t cmd_ack[CMD_LEN] = { 0xAA, CMD_ACK_ID, 0x00, 0x00, 0x00, 0x00 };

static struct tty_serial cam;
static u8_t cam_rxbuf[64];

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
	case ACK_SNAPSHOT:
		if (buf[2] != CMD_SNAPSHOT_ID) {
			return false;
		}
		break;
	case ACK_GET_PICTURE:
		if (buf[2] != CMD_GET_PICTURE_ID) {
			return false;
		}
		break;
	case ACK_SET_PACKAGE_SIZE:
		if (buf[2] != CMD_SET_PACKAGE_SIZE_ID) {
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
			ret = tty_read(&cam, cam_data.resp, CMD_LEN);
			if (ret == CMD_LEN && cam_check_ack(cam_data.resp, ACK_SYNC)) {
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
	ret = tty_read(&cam, cam_data.resp, CMD_LEN);
	if (ret != CMD_LEN) {
		return -1;
	}

	/* Check if camera responded with SYNC */
	if (memcmp(cam_data.resp, cmd_sync, CMD_LEN)) {
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
	ret = tty_read(&cam, cam_data.resp, CMD_LEN);
	if (ret == CMD_LEN && cam_check_ack(cam_data.resp, ACK_INITIAL)) {
		/* Camera responded with ACK */
		return 0;
	}

	return -1;
}

/**
 * @brief Construct SET_PACKAGE_SIZE command
 *
 * @param[out] cmd The SET_PACKAGE_SIZE command
 * @param size The package size
 *
 * @retval 0 on success, -1 otherwise
 */
static int get_set_package_size_cmd(uint8_t *cmd, u16_t size)
{
	if (!cmd) {
		return -1;
	}

	if (size < 64 || size > 512) {
		return -1;
	}

	cmd[0] = 0xAA;                                  /* Command */
	cmd[1] = CMD_SET_PACKAGE_SIZE_ID;               /* ID Number */
	cmd[2] = 0x08;                                  /* Parameter 1 */
	cmd[3] = (sys_cpu_to_be16(size) >> 8) & 0xFF;   /* Paramater2: Package Size (Low Byte) */
	cmd[4] = sys_cpu_to_be16(size) & 0xFF;          /* Paramater3: Package Size (High Byte) */
	cmd[5] = 0x00;                                  /* Parameter4 */

	return 0;
}

int ucam3_set_package_size(u16_t size)
{
	int ret;
	static u8_t buf[CMD_LEN];

	/* Construct SET_PACKAGE_SIZE command */
	ret = get_set_package_size_cmd(buf, size);
	if (ret) {
		return -1;
	}

	/* Send SET_PACKAGE_SIZE to the camera */
	ret = tty_write(&cam, buf, CMD_LEN);
	if (ret != CMD_LEN) {
		return -1;
	}

	/* Read response from the camera */
	ret = tty_read(&cam, cam_data.resp, CMD_LEN);
	if (ret == CMD_LEN && cam_check_ack(cam_data.resp, ACK_SET_PACKAGE_SIZE)) {
		/* Camera responded with ACK */
		cam_data.package_size = size;
		return 0;
	}

	return -1;
}

/**
 * @brief Construct SNAPSHOT command out of image_config_t
 *
 * @param[out] cmd The SNAPSHOT command
 * @param[in] image_config The image settings
 *
 * @retval 0 on success, -1 otherwise
 */
static int get_snapshot_cmd(uint8_t *cmd, const image_config_t *image_config)
{
	bool is_raw;

	if (!cmd || !image_config) {
		return -1;
	}

	cmd[0] = 0xAA;                  /* Command */
	cmd[1] = CMD_SNAPSHOT_ID;       /* ID Number */

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
		cmd[2] = 0x01;  /* Parameter1: RAW Snapshot */
	} else {
		cmd[2] = 0x00;  /* Parameter1: RAW Snapshot */
	}

	cmd[3] = 0x00;          /* Parameter2: Skip Frame (Low Byte) */
	cmd[4] = 0x00;          /* Parameter3: Skip Frame (High Byte) */
	cmd[5] = 0x00;          /* Parameter4 */

	return 0;
}

int ucam3_snapshot(const image_config_t *image_config)
{
	int ret;
	static u8_t buf[CMD_LEN];

	/* Construct SNAPSHOT command out of image_config */
	ret = get_snapshot_cmd(buf, image_config);
	if (ret) {
		return -1;
	}

	/* Send SNAPSHOT to the camera */
	ret = tty_write(&cam, buf, CMD_LEN);
	if (ret != CMD_LEN) {
		return -1;
	}

	/* Read response from the camera */
	ret = tty_read(&cam, cam_data.resp, CMD_LEN);
	if (ret == CMD_LEN && cam_check_ack(cam_data.resp, ACK_SNAPSHOT)) {
		/* Camera responded with ACK */
		return 0;
	}

	return -1;
}

/**
 * @brief Verify the integrity of the package
 *
 * Verify the package according to Section 7.4.1. Package Size
 *
 * @param[in] data Buffer containing the package
 * @param size The package size
 *
 * @retval true on success, false otherwise
 */
static inline bool verify_package(const u8_t *data, u16_t size)
{
	u16_t sum;
	u8_t verification_code;
	uint16_t i;

	sum = 0;
	for (i = 0; i < size - 2; i++) {
		sum += data[i];
	}

	verification_code = (sys_cpu_to_be16(sum) >> 8) & 0xFF;

	if (data[size - 1] != 0x00 &&
	    data[size - 2] != verification_code) {
		return false;
	}

	return true;
}

/**
 * @brief Construct DATA ACK command using package ID
 *
 * @param[out] cmd The DATA ACK command
 * @param package_id The package ID
 *
 * @retval 0 on success, -1 otherwise
 */
static int get_data_ack_cmd(u8_t *cmd, u16_t package_id)
{
	if (!cmd) {
		return -1;
	}

	cmd[0] = 0xAA;                                          /* Command */
	cmd[1] = CMD_ACK_ID;                                    /* ID Number */
	cmd[2] = 0x00;                                          /* Parameter1 */
	cmd[3] = 0x00;                                          /* Parameter2 */
	cmd[4] = (sys_cpu_to_be16(package_id) >> 8) & 0xFF;     /* Parameter3: Package ID Byte 0 */
	cmd[5] = sys_cpu_to_be16(package_id) & 0xFF;            /* Parameter4: Package ID Byte 1 */

	return 0;
}

int ucam3_get_picture(u8_t *data, u32_t *size)
{
	int ret;
	static u8_t buf[CMD_LEN];
	u32_t image_size;
	u32_t num_of_packages;
	u32_t i;
	u32_t offset;
	u16_t data_size;

	/* Send GET_PICTURE to the camera */
	ret = tty_write(&cam, cmd_get_picture, CMD_LEN);
	if (ret != CMD_LEN) {
		return -1;
	}

	/* Read response from the camera */
	ret = tty_read(&cam, cam_data.resp, CMD_LEN);
	if (ret != CMD_LEN || !cam_check_ack(cam_data.resp, ACK_GET_PICTURE)) {
		return -1;
	}

	/* 'Shutter' delay: Time after GET_PICTURE is sent to when image output
	 * begins. Section 13. Specifications and Ratings
	 */
	k_sleep(SHUTTER_DELAY);

	/* Camera responded with ACK */
	/* Wait for DATA command from the camera */
	ret = tty_read(&cam, cam_data.resp, CMD_LEN);
	if (ret != CMD_LEN || memcmp(cam_data.resp, cmd_data, 3)) {
		return -1;
	}

	/* Calculate the size of the image from the DATA packet */
	image_size = 0;
	image_size |= cam_data.resp[3];
	image_size |= cam_data.resp[4] << 8;
	image_size |= cam_data.resp[5] << 16;

	/* Get and construct the image from the packages */
	num_of_packages = image_size / (cam_data.package_size - 6);

	offset = 0;
	for (i = 0; i <= num_of_packages; i++) {

		/* Send ACK with Package ID to the camera */
		ret = get_data_ack_cmd(buf, i);
		if (ret) {
			return -1;
		}

		ret = tty_write(&cam, buf, CMD_LEN);
		if (ret != CMD_LEN) {
			return -1;
		}

		/* Wait for Image Data Package from the camera */
		ret = tty_read(&cam, cam_data.package_buf, cam_data.package_size);

		/* Verify package */
		if (!verify_package(cam_data.package_buf, cam_data.package_size)) {
			return -1;
		}

		/* Figure out data size in package and copy to data image */
		data_size = 0;
		data_size |= cam_data.package_buf[2];
		data_size |= cam_data.package_buf[3] << 8;
		memcpy(&data[offset], &cam_data.package_buf[IMAGE_DATA_START],
		       data_size);
		offset += data_size;
	}

	/* Send ACK after the last package to the camera */
	ret = get_data_ack_cmd(buf, i);
	if (ret) {
		return -1;
	}

	ret = tty_write(&cam, buf, CMD_LEN);
	if (ret != CMD_LEN) {
		return -1;
	}

	/* Check the size of the image */
	if (image_size != offset) {
		return -1;
	}

	/* Return the image size */
	*size = image_size;

	return 0;
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
	tty_set_rx_timeout(&cam, 200);

	return 0;
}
