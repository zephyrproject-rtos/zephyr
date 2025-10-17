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
#define VIDEO_CID_ARDUCAM_EV       (VIDEO_CID_PRIVATE_BASE + 1)
#define VIDEO_CID_ARDUCAM_RESET    (VIDEO_CID_PRIVATE_BASE + 5)
#define VIDEO_CID_ARDUCAM_LOWPOWER (VIDEO_CID_PRIVATE_BASE + 6)

/* Read only registers */
#define VIDEO_CID_ARDUCAM_SUPP_RES     (VIDEO_CID_PRIVATE_BASE + 7)
#define VIDEO_CID_ARDUCAM_SUPP_SP_EFF  (VIDEO_CID_PRIVATE_BASE + 8)

/* Info default settings */
#define SUPPORT_RESOLUTION_5M 7894
#define SUPPORT_SP_EFF_5M     63

#define SUPPORT_RESOLUTION_3M 7638
#define SUPPORT_SP_EFF_3M     319

#define EXPOSURE_MAX   30000
#define EXPOSURE_MIN   0
#define GAIN_MAX       1023
#define GAIN_MIN       0
#define DEVICE_ADDRESS 0x78

/**
 * @enum mega_contrast_level
 * @brief Configure camera contrast level
 */
enum mega_contrast_level {
	MEGA_CONTRAST_LEVEL_NEGATIVE_3 = 6, /**<Level -3 */
	MEGA_CONTRAST_LEVEL_NEGATIVE_2 = 4, /**<Level -2 */
	MEGA_CONTRAST_LEVEL_NEGATIVE_1 = 2, /**<Level -1 */
	MEGA_CONTRAST_LEVEL_DEFAULT = 0,    /**<Level Default*/
	MEGA_CONTRAST_LEVEL_1 = 1,          /**<Level +1 */
	MEGA_CONTRAST_LEVEL_2 = 3,          /**<Level +2 */
	MEGA_CONTRAST_LEVEL_3 = 5,          /**<Level +3 */
};

/**
 * @enum mega_ev_level
 * @brief Configure camera EV level
 */
enum mega_ev_level {
	MEGA_EV_LEVEL_NEGATIVE_3 = 6, /**<Level -3 */
	MEGA_EV_LEVEL_NEGATIVE_2 = 4, /**<Level -2 */
	MEGA_EV_LEVEL_NEGATIVE_1 = 2, /**<Level -1 */
	MEGA_EV_LEVEL_DEFAULT = 0,    /**<Level Default*/
	MEGA_EV_LEVEL_1 = 1,          /**<Level +1 */
	MEGA_EV_LEVEL_2 = 3,          /**<Level +2 */
	MEGA_EV_LEVEL_3 = 5,          /**<Level +3 */
};

/**
 * @enum mega_saturation_level
 * @brief Configure camera saturation  level
 */
enum mega_saturation_level {
	MEGA_SATURATION_LEVEL_NEGATIVE_3 = 6, /**<Level -3 */
	MEGA_SATURATION_LEVEL_NEGATIVE_2 = 4, /**<Level -2 */
	MEGA_SATURATION_LEVEL_NEGATIVE_1 = 2, /**<Level -1 */
	MEGA_SATURATION_LEVEL_DEFAULT = 0,    /**<Level Default*/
	MEGA_SATURATION_LEVEL_1 = 1,          /**<Level +1 */
	MEGA_SATURATION_LEVEL_2 = 3,          /**<Level +2 */
	MEGA_SATURATION_LEVEL_3 = 5,          /**<Level +3 */
};

/**
 * @enum mega_brightness_level
 * @brief Configure camera brightness level
 */
enum mega_brightness_level {
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
 * @enum mega_sharpness_level
 * @brief Configure camera Sharpness level
 */
enum mega_sharpness_level {
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
 * @enum mega_auto_focus_level
 * @brief Configure camera auto focus level
 */
enum mega_auto_focus_level {
	MEGA_AUTO_FOCUS_ON= 0,		/**<Auto focus On */
	MEGA_AUTO_FOCUS_SINGLE,		/**<Auto focus Single */
	MEGA_AUTO_FOCUS_CONT,		/**<Auto focus Continous */
	MEGA_AUTO_FOCUS_PAUSE,		/**<Auto focus Pause */
	MEGA_AUTO_FOCUS_OFF,		/**<Auto focus Off */
};

/**
 * @enum mega_color_fx
 * @brief Configure special effects
 */
enum mega_color_fx {
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
 * @enum mega_white_balance
 * @brief Configure white balance mode
 */
enum mega_white_balance {
	MEGA_WHITE_BALANCE_MODE_DEFAULT = 0, /**< Auto */
	MEGA_WHITE_BALANCE_MODE_SUNNY,       /**< Sunny */
	MEGA_WHITE_BALANCE_MODE_OFFICE,      /**< Office */
	MEGA_WHITE_BALANCE_MODE_CLOUDY,      /**< Cloudy*/
	MEGA_WHITE_BALANCE_MODE_HOME,        /**< Home */
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
 * @enum mega_resolution
 * @brief Configure camera resolution
 */
enum mega_resolution {
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
 * @enum mega_features
 * @brief Configure camera available features
 */
enum mega_features {
	MEGA_HAS_DEFAULT = 0,			/**< Default features enabled */
    MEGA_HAS_SHARPNESS = 1 << 0,	/**< Sharpness feature enabled */
    MEGA_HAS_FOCUS = 1 << 1,		/**< Focus feature enabled */
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CAMERA_ARDUCAM_MEGA_H_ */
