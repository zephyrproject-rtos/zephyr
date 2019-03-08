/*
 * Copyright (c) 2019 Ioannis <gon1332> Konstantelias
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _UCAM3_H
#define _UCAM3_H

typedef enum {
	FMT_8BIT_GRAY_SCALE             = 0x03,
	FMT_16BIT_COLOUR_RAW_CRYCBY     = 0x08,
	FMT_16BIT_COLOUR_RAW_RGB        = 0x06,
	FMT_JPEG                        = 0x07,
} image_format_t;

typedef enum {
	RAW_80_60       = 0x01,
	RAW_160_120     = 0x03,
	RAW_128_128     = 0x09,
	RAW_128_96      = 0x0B,
	RAW_INVALID     = 0x07,
} image_raw_size_t;

typedef enum {
	JPEG_160_128    = 0x03,
	JPEG_320_240    = 0x05,
	JPEG_640_480    = 0x07,
	JPEG_INVALID    = 0x06,
} image_jpeg_size_t;

typedef struct {
	image_format_t format;
	union {
		image_raw_size_t raw;
		image_jpeg_size_t jpeg;
	} size;
} image_config_t;

/**
 * @brief Initialize the uCAM-III
 *
 * Initialize the serial interface with the camera module.
 *
 * @retval 0 on success, otherwise -1
 */
extern int ucam3_create(void);

/**
 * @brief Synchronize the uCAM-III
 *
 * In order to operate the camera, it should be synchronized. There is a
 * handshake like protocol in order to achieve synchronization.
 *
 * @note See section 8.1. Synchronising the uCAM-III
 *
 * @note After power applied to camera and before calling this function, wait
 * for 800 milliseconds. See section 13. Specifications and Ratings.
 *
 * @note After synchronising, allow up to 1-2 seconds before capturing the first
 * image. See section 13. Specifications and Ratings.
 *
 * @retval 0 on synchronization, otherwise -1
 */
extern int ucam3_sync(void);

/**
 * @brief Configure image size and format
 *
 * The host issues INITIAL command to configure the image size and format.
 * After receiving this command, the module will send out an ACK command to the
 * host if the configuration is successful. Otherwise, a NAK command will be
 * sent out.
 *
 * @param[in] image_config Image settings
 *
 * @note See section 7.1. INITIAL (AA01h) and 8.2. INITIAL, ... Commands
 *
 * @retval 0 on synchronization, otherwise -1
 */
extern int ucam3_initial(const image_config_t *image_config);

/**
 * @brief Reset the uCAM-III
 *
 * Initialize the serial interface with the camera module.
 *
 * @retval 0 on success, otherwise -1
 */
extern void ucam3_reset(void);

#endif /* _UCAM3_H */
