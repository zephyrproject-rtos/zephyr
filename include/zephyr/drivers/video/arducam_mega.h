/**
 * Copyright (c) 2023 Arducam Technology Co., Ltd. <www.arducam.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CAMERA_ARDUCAM_MEGA_H_
#define ZEPHYR_INCLUDE_DRIVERS_CAMERA_ARDUCAM_MEGA_H_

#ifdef __cplusplus
extern "C" {
#endif

#define VIDEO_CID_ARDUCAM_EV        (VIDEO_CTRL_CLASS_VENDOR + 1)
#define VIDEO_CID_ARDUCAM_INFO      (VIDEO_CTRL_CLASS_VENDOR + 2)
#define VIDEO_CID_ARDUCAM_SHARPNESS (VIDEO_CTRL_CLASS_VENDOR + 3)
#define VIDEO_CID_ARDUCAM_COLOR_FX  (VIDEO_CTRL_CLASS_VENDOR + 4)
#define VIDEO_CID_ARDUCAM_RESET     (VIDEO_CTRL_CLASS_VENDOR + 5)
#define VIDEO_CID_ARDUCAM_LOWPOWER  (VIDEO_CTRL_CLASS_VENDOR + 6)

/**
 * @enum MEGA_CONTRAST_LEVEL
 * @brief Configure camera contrast level
 */
enum MEGA_CONTRAST_LEVEL {
	MEGA_CONTRAST_LEVEL_NEGATIVE_3 = 6, /**<Level -3 */
	MEGA_CONTRAST_LEVEL_NEGATIVE_2 = 4, /**<Level -2 */
	MEGA_CONTRAST_LEVEL_NEGATIVE_1 = 2, /**<Level -1 */
	MEGA_CONTRAST_LEVEL_DEFAULT = 0,    /**<Level Default*/
	MEGA_CONTRAST_LEVEL_1 = 1,          /**<Level +1 */
	MEGA_CONTRAST_LEVEL_2 = 3,          /**<Level +2 */
	MEGA_CONTRAST_LEVEL_3 = 5,          /**<Level +3 */
};

/**
 * @enum MEGA_EV_LEVEL
 * @brief Configure camera EV level
 */
enum MEGA_EV_LEVEL {
	MEGA_EV_LEVEL_NEGATIVE_3 = 6, /**<Level -3 */
	MEGA_EV_LEVEL_NEGATIVE_2 = 4, /**<Level -2 */
	MEGA_EV_LEVEL_NEGATIVE_1 = 2, /**<Level -1 */
	MEGA_EV_LEVEL_DEFAULT = 0,    /**<Level Default*/
	MEGA_EV_LEVEL_1 = 1,          /**<Level +1 */
	MEGA_EV_LEVEL_2 = 3,          /**<Level +2 */
	MEGA_EV_LEVEL_3 = 5,          /**<Level +3 */
};

/**
 * @enum MEGA_SATURATION_LEVEL
 * @brief Configure camera saturation  level
 */
enum MEGA_SATURATION_LEVEL {
	MEGA_SATURATION_LEVEL_NEGATIVE_3 = 6, /**<Level -3 */
	MEGA_SATURATION_LEVEL_NEGATIVE_2 = 4, /**<Level -2 */
	MEGA_SATURATION_LEVEL_NEGATIVE_1 = 2, /**<Level -1 */
	MEGA_SATURATION_LEVEL_DEFAULT = 0,    /**<Level Default*/
	MEGA_SATURATION_LEVEL_1 = 1,          /**<Level +1 */
	MEGA_SATURATION_LEVEL_2 = 3,          /**<Level +2 */
	MEGA_SATURATION_LEVEL_3 = 5,          /**<Level +3 */
};

/**
 * @enum MEGA_BRIGHTNESS_LEVEL
 * @brief Configure camera brightness level
 */
enum MEGA_BRIGHTNESS_LEVEL {
	MEGA_BRIGHTNESS_LEVEL_NEGATIVE_4 = 8, /**<Level -4 */
	MEGA_BRIGHTNESS_LEVEL_NEGATIVE_3 = 6, /**<Level -3 */
	MEGA_BRIGHTNESS_LEVEL_NEGATIVE_2 = 4, /**<Level -2 */
	MEGA_BRIGHTNESS_LEVEL_NEGATIVE_1 = 2, /**<Level -1 */
	MEGA_BRIGHTNESS_LEVEL_DEFAULT = 0,    /**<Level Default*/
	MEGA_BRIGHTNESS_LEVEL_1 = 1,          /**<Level +1 */
	MEGA_BRIGHTNESS_LEVEL_2 = 3,          /**<Level +2 */
	MEGA_BRIGHTNESS_LEVEL_3 = 5,          /**<Level +3 */
	MEGA_BRIGHTNESS_LEVEL_4 = 7,          /**<Level +4 */
};

/**
 * @enum MEGA_SHARPNESS_LEVEL
 * @brief Configure camera Sharpness level
 */
enum MEGA_SHARPNESS_LEVEL {
	MEGA_SHARPNESS_LEVEL_AUTO = 0, /**<Sharpness Auto */
	MEGA_SHARPNESS_LEVEL_1,        /**<Sharpness Level 1 */
	MEGA_SHARPNESS_LEVEL_2,        /**<Sharpness Level 2 */
	MEGA_SHARPNESS_LEVEL_3,        /**<Sharpness Level 3 */
	MEGA_SHARPNESS_LEVEL_4,        /**<Sharpness Level 4 */
	MEGA_SHARPNESS_LEVEL_5,        /**<Sharpness Level 5 */
	MEGA_SHARPNESS_LEVEL_6,        /**<Sharpness Level 6 */
	MEGA_SHARPNESS_LEVEL_7,        /**<Sharpness Level 7 */
	MEGA_SHARPNESS_LEVEL_8,        /**<Sharpness Level 8 */
};

/**
 * @enum MEGA_COLOR_FX
 * @brief Configure special effects
 */
enum MEGA_COLOR_FX {
	MEGA_COLOR_FX_NONE = 0,      /**< no effect   */
	MEGA_COLOR_FX_BLUEISH,       /**< cool light   */
	MEGA_COLOR_FX_REDISH,        /**< warm   */
	MEGA_COLOR_FX_BW,            /**< Black/white   */
	MEGA_COLOR_FX_SEPIA,         /**<Sepia   */
	MEGA_COLOR_FX_NEGATIVE,      /**<positive/negative inversion  */
	MEGA_COLOR_FX_GRASS_GREEN,   /**<Grass green */
	MEGA_COLOR_FX_OVER_EXPOSURE, /**<Over exposure*/
	MEGA_COLOR_FX_SOLARIZE,      /**< Solarize   */
};

/**
 * @enum MEGA_WHITE_BALANCE
 * @brief Configure white balance mode
 */
enum MEGA_WHITE_BALANCE {
	MEGA_WHITE_BALANCE_MODE_DEFAULT = 0, /**< Auto */
	MEGA_WHITE_BALANCE_MODE_SUNNY,       /**< Sunny */
	MEGA_WHITE_BALANCE_MODE_OFFICE,      /**< Office */
	MEGA_WHITE_BALANCE_MODE_CLOUDY,      /**< Cloudy*/
	MEGA_WHITE_BALANCE_MODE_HOME,        /**< Home */
};

/**
 * @enum MEGA_IMAGE_QUALITY
 * @brief Configure JPEG image quality
 */
enum MEGA_IMAGE_QUALITY {
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
 * @enum MEGA_PIXELFORMAT
 * @brief Configure camera pixel format
 */
enum MEGA_PIXELFORMAT {
	MEGA_PIXELFORMAT_JPG = 0X01,
	MEGA_PIXELFORMAT_RGB565 = 0X02,
	MEGA_PIXELFORMAT_YUV = 0X03,
};

/**
 * @enum MEGA_RESOLUTION
 * @brief Configure camera resolution
 */
enum MEGA_RESOLUTION {
	MEGA_RESOLUTION_QQVGA = 0x00,   /**<160x120 */
	MEGA_RESOLUTION_QVGA = 0x01,    /**<320x240*/
	MEGA_RESOLUTION_VGA = 0x02,     /**<640x480*/
	MEGA_RESOLUTION_SVGA = 0x03,    /**<800x600*/
	MEGA_RESOLUTION_HD = 0x04,      /**<1280x720*/
	MEGA_RESOLUTION_SXGAM = 0x05,   /**<1280x960*/
	MEGA_RESOLUTION_UXGA = 0x06,    /**<1600x1200*/
	MEGA_RESOLUTION_FHD = 0x07,     /**<1920x1080*/
	MEGA_RESOLUTION_QXGA = 0x08,    /**<2048x1536*/
	MEGA_RESOLUTION_WQXGA2 = 0x09,  /**<2592x1944*/
	MEGA_RESOLUTION_96X96 = 0x0a,   /**<96x96*/
	MEGA_RESOLUTION_128X128 = 0x0b, /**<128x128*/
	MEGA_RESOLUTION_320X320 = 0x0c, /**<320x320*/
	MEGA_RESOLUTION_12 = 0x0d,      /**<Reserve*/
	MEGA_RESOLUTION_13 = 0x0e,      /**<Reserve*/
	MEGA_RESOLUTION_14 = 0x0f,      /**<Reserve*/
	MEGA_RESOLUTION_15 = 0x10,      /**<Reserve*/
	MEGA_RESOLUTION_NONE,
};

/**
 * @struct arducam_mega_info
 * @brief Some information about mega camera
 */
struct arducam_mega_info {
	int support_resolution;
	int support_special_effects;
	unsigned long exposure_value_max;
	unsigned int exposure_value_min;
	unsigned int gain_value_max;
	unsigned int gain_value_min;
	unsigned char enable_focus;
	unsigned char enable_sharpness;
	unsigned char device_address;
	unsigned char camera_id;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CAMERA_ARDUCAM_MEGA_H_ */
