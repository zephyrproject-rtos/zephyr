/**
 * Copyright (c) 2023 Arducam Technology Co., Ltd. <www.arducam.com>
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_VIDEO_ARDUCAM_MEGA_H_
#define ZEPHYR_INCLUDE_DRIVERS_VIDEO_ARDUCAM_MEGA_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Arducam specific camera controls */
#define VIDEO_CID_ARDUCAM_EV			(VIDEO_CID_PRIVATE_BASE + 1)
#define VIDEO_CID_ARDUCAM_RESET			(VIDEO_CID_PRIVATE_BASE + 5)
#define VIDEO_CID_ARDUCAM_LOWPOWER		(VIDEO_CID_PRIVATE_BASE + 6)

/* Read only registers */
#define VIDEO_CID_ARDUCAM_SUPP_RES		(VIDEO_CID_PRIVATE_BASE + 7)
#define VIDEO_CID_ARDUCAM_SUPP_SP_EFF	(VIDEO_CID_PRIVATE_BASE + 8)

/* Info default settings */
#define SUPPORT_RESOLUTION_5M	7894
#define SUPPORT_SP_EFF_5M		63

#define SUPPORT_RESOLUTION_3M	7638
#define SUPPORT_SP_EFF_3M		319

#define DEVICE_ADDRESS 0x78

/**
 * @enum mega_contrast_level
 * @brief Configure camera contrast level
 */
enum mega_contrast_level {
	MEGA_CONTRAST_LEVEL_NEGATIVE_3 = 6,
	MEGA_CONTRAST_LEVEL_NEGATIVE_2 = 4,
	MEGA_CONTRAST_LEVEL_NEGATIVE_1 = 2,
	MEGA_CONTRAST_LEVEL_DEFAULT = 0,
	MEGA_CONTRAST_LEVEL_1 = 1,
	MEGA_CONTRAST_LEVEL_2 = 3,
	MEGA_CONTRAST_LEVEL_3 = 5,
};

/**
 * @enum mega_ev_level
 * @brief Configure camera EV level
 */
enum mega_ev_level {
	MEGA_EV_LEVEL_NEGATIVE_3 = 6,
	MEGA_EV_LEVEL_NEGATIVE_2 = 4,
	MEGA_EV_LEVEL_NEGATIVE_1 = 2,
	MEGA_EV_LEVEL_DEFAULT = 0,
	MEGA_EV_LEVEL_1 = 1,
	MEGA_EV_LEVEL_2 = 3,
	MEGA_EV_LEVEL_3 = 5,
};

/**
 * @enum mega_saturation_level
 * @brief Configure camera saturation  level
 */
enum mega_saturation_level {
	MEGA_SATURATION_LEVEL_NEGATIVE_3 = 6,
	MEGA_SATURATION_LEVEL_NEGATIVE_2 = 4,
	MEGA_SATURATION_LEVEL_NEGATIVE_1 = 2,
	MEGA_SATURATION_LEVEL_DEFAULT = 0,
	MEGA_SATURATION_LEVEL_1 = 1,
	MEGA_SATURATION_LEVEL_2 = 3,
	MEGA_SATURATION_LEVEL_3 = 5,
};

/**
 * @enum mega_brightness_level
 * @brief Configure camera brightness level
 */
enum mega_brightness_level {
	MEGA_BRIGHTNESS_LEVEL_NEGATIVE_4 = 8,
	MEGA_BRIGHTNESS_LEVEL_NEGATIVE_3 = 6,
	MEGA_BRIGHTNESS_LEVEL_NEGATIVE_2 = 4,
	MEGA_BRIGHTNESS_LEVEL_NEGATIVE_1 = 2,
	MEGA_BRIGHTNESS_LEVEL_DEFAULT = 0,
	MEGA_BRIGHTNESS_LEVEL_1 = 1,
	MEGA_BRIGHTNESS_LEVEL_2 = 3,
	MEGA_BRIGHTNESS_LEVEL_3 = 5,
	MEGA_BRIGHTNESS_LEVEL_4 = 7,
};

/**
 * @enum mega_sharpness_level
 * @brief Configure camera Sharpness level
 */
enum mega_sharpness_level {
	MEGA_SHARPNESS_LEVEL_AUTO = 0,
	MEGA_SHARPNESS_LEVEL_1,
	MEGA_SHARPNESS_LEVEL_2,
	MEGA_SHARPNESS_LEVEL_3,
	MEGA_SHARPNESS_LEVEL_4,
	MEGA_SHARPNESS_LEVEL_5,
	MEGA_SHARPNESS_LEVEL_6,
	MEGA_SHARPNESS_LEVEL_7,
	MEGA_SHARPNESS_LEVEL_8,
};

/**
 * @enum mega_auto_focus_level
 * @brief Configure camera auto focus level
 */
enum mega_auto_focus_level {
	MEGA_AUTO_FOCUS_ON = 0,
	MEGA_AUTO_FOCUS_SINGLE,
	MEGA_AUTO_FOCUS_CONT,
	MEGA_AUTO_FOCUS_PAUSE,
	MEGA_AUTO_FOCUS_OFF,
};

/**
 * @enum mega_color_fx
 * @brief Configure special effects
 */
enum mega_color_fx {
	MEGA_COLOR_FX_NONE = 0,
	MEGA_COLOR_FX_BLUEISH,
	MEGA_COLOR_FX_REDISH,
	MEGA_COLOR_FX_BW,
	MEGA_COLOR_FX_SEPIA,
	MEGA_COLOR_FX_NEGATIVE,
	MEGA_COLOR_FX_GRASS_GREEN,
	MEGA_COLOR_FX_OVER_EXPOSURE,
	MEGA_COLOR_FX_SOLARIZE,
};

/**
 * @enum mega_white_balance
 * @brief Configure white balance mode
 */
enum mega_white_balance {
	MEGA_WHITE_BALANCE_MODE_DEFAULT = 0,
	MEGA_WHITE_BALANCE_MODE_SUNNY,
	MEGA_WHITE_BALANCE_MODE_OFFICE,
	MEGA_WHITE_BALANCE_MODE_CLOUDY,
	MEGA_WHITE_BALANCE_MODE_HOME,
};

/**
 * @enum mega_image_quality
 * @brief Configure JPEG image quality
 */
enum mega_image_quality {
	HIGH_QUALITY = 0,
	DEFAULT_QUALITY = 1,
	LOW_QUALITY = 2,
};

enum {
	ARDUCAM_SENSOR_5MP_1 = 0x81,
	ARDUCAM_SENSOR_3MP_1 = 0x82,
	ARDUCAM_SENSOR_5MP_2 = 0x83, /* 2592x1936 */
	ARDUCAM_SENSOR_3MP_2 = 0x84,
};

/**
 * @enum mega_pixelformat
 * @brief Configure camera pixel format
 */
enum mega_pixelformat {
	MEGA_PIXELFORMAT_JPG = 0X01,
	MEGA_PIXELFORMAT_RGB565 = 0X02,
	MEGA_PIXELFORMAT_YUV = 0X03,
};

/**
 * @enum mega_features
 * @brief Configure camera available features
 */
enum mega_features {
	MEGA_HAS_DEFAULT = 0,
    MEGA_HAS_SHARPNESS = 1 << 0,
    MEGA_HAS_FOCUS = 1 << 1,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CAMERA_ARDUCAM_MEGA_H_ */
