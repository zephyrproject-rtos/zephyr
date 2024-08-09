/*
 * Copyright (c) 2024 Chaim Zax
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADXL34X_REG_H_
#define ZEPHYR_DRIVERS_SENSOR_ADXL34X_REG_H_

#include <zephyr/sys/util.h>

/* registers */
#define ADXL34X_REG_DEVID          0x00 /**< Device ID */
#define ADXL34X_REG_THRESH_TAP     0x1D /**< Tap threshold */
#define ADXL34X_REG_OFSX           0x1E /**< X-axis offset */
#define ADXL34X_REG_OFSY           0x1F /**< Y-axis offset */
#define ADXL34X_REG_OFSZ           0x20 /**< Z-axis offset */
#define ADXL34X_REG_DUR            0x21 /**< Tap duration */
#define ADXL34X_REG_LATENT         0x22 /**< Tap latency */
#define ADXL34X_REG_WINDOW         0x23 /**< Tap window */
#define ADXL34X_REG_THRESH_ACT     0x24 /**< Activity threshold */
#define ADXL34X_REG_THRESH_INACT   0x25 /**< Inactivity threshold */
#define ADXL34X_REG_TIME_INACT     0x26 /**< Inactivity time */
#define ADXL34X_REG_ACT_INACT_CTL  0x27 /**< Axis control for activity and inactivity detection */
#define ADXL34X_REG_THRESH_FF      0x28 /**< Free-fall threshold */
#define ADXL34X_REG_TIME_FF        0x29 /**< Free-fall time */
#define ADXL34X_REG_TAP_AXES       0x2A /**< Axis control for single tap/double tap */
#define ADXL34X_REG_ACT_TAP_STATUS 0x2B /**< Source of single tap/double tap */
#define ADXL34X_REG_BW_RATE        0x2C /**< Data rate and power mode control */
#define ADXL34X_REG_POWER_CTL      0x2D /**< Power-saving features control */
#define ADXL34X_REG_INT_ENABLE     0x2E /**< Interrupt enable control */
#define ADXL34X_REG_INT_MAP        0x2F /**< Interrupt mapping control */
#define ADXL34X_REG_INT_SOURCE     0x30 /**< Source of interrupts */
#define ADXL34X_REG_DATA_FORMAT    0x31 /**< Data format control */
#define ADXL34X_REG_DATA           0x32 /**< FIFO data */
#define ADXL34X_REG_DATAX0         0x32 /**< X-Axis Data 0 */
#define ADXL34X_REG_DATAX1         0x33 /**< X-Axis Data 1 */
#define ADXL34X_REG_DATAY0         0x34 /**< Y-Axis Data 0 */
#define ADXL34X_REG_DATAY1         0x35 /**< Y-Axis Data 1 */
#define ADXL34X_REG_DATAZ0         0x36 /**< Z-Axis Data 0 */
#define ADXL34X_REG_DATAZ1         0x37 /**< Z-Axis Data 1 */
#define ADXL34X_REG_FIFO_CTL       0x38 /**< FIFO control */
#define ADXL34X_REG_FIFO_STATUS    0x39 /**< FIFO status */
/* additional registers for the ADXL344 and ADXL346 */
#define ADXL34X_REG_TAP_SIGN       0x3A /**< Sign and source for single tap/double tap */
#define ADXL34X_REG_ORIENT_CONF    0x3B /**< Orientation configuration */
#define ADXL34X_REG_ORIENT         0x3C /**< Orientation status */
/* keep as last */
#define ADXL34X_REG_MAX            0x3C /**< The highest register address */

/* reg ACT_INACT_CTL */
#define ADXL34X_REG_ACT_INACT_CTL_ACT_ACDC       BIT(7)
#define ADXL34X_REG_ACT_INACT_CTL_ACT_X_ENABLE   BIT(6)
#define ADXL34X_REG_ACT_INACT_CTL_ACT_Y_ENABLE   BIT(5)
#define ADXL34X_REG_ACT_INACT_CTL_ACT_Z_ENABLE   BIT(4)
#define ADXL34X_REG_ACT_INACT_CTL_INACT_ACDC     BIT(3)
#define ADXL34X_REG_ACT_INACT_CTL_INACT_X_ENABLE BIT(2)
#define ADXL34X_REG_ACT_INACT_CTL_INACT_Y_ENABLE BIT(1)
#define ADXL34X_REG_ACT_INACT_CTL_INACT_Z_ENABLE BIT(0)

/* reg TAP_AXES */
#define ADXL34X_REG_TAP_AXES_IMPROVED_TAB BIT(4)
#define ADXL34X_REG_TAP_AXES_SUPPRESS     BIT(3)
#define ADXL34X_REG_TAP_AXES_TAP_X_ENABLE BIT(2)
#define ADXL34X_REG_TAP_AXES_TAP_Y_ENABLE BIT(1)
#define ADXL34X_REG_TAP_AXES_TAP_Z_ENABLE BIT(0)

/* reg ACT_TAP_STATUS */
#define ADXL34X_REG_ACT_TAP_STATUS_ACT_X_SOURCE BIT(6)
#define ADXL34X_REG_ACT_TAP_STATUS_ACT_Y_SOURCE BIT(5)
#define ADXL34X_REG_ACT_TAP_STATUS_ACT_Z_SOURCE BIT(4)
#define ADXL34X_REG_ACT_TAP_STATUS_ASLEEP       BIT(3)
#define ADXL34X_REG_ACT_TAP_STATUS_TAP_X_SOURCE BIT(2)
#define ADXL34X_REG_ACT_TAP_STATUS_TAP_Y_SOURCE BIT(1)
#define ADXL34X_REG_ACT_TAP_STATUS_TAP_Z_SOURCE BIT(0)

/* reg BW_RATE */
#define ADXL34X_REG_BW_RATE_LOW_POWER BIT(4)
#define ADXL34X_REG_BW_RATE_RATE      GENMASK(3, 0)

/* reg POWER_CTL */
#define ADXL34X_REG_POWER_CTL_LINK       BIT(5)
#define ADXL34X_REG_POWER_CTL_AUTO_SLEEP BIT(4)
#define ADXL34X_REG_POWER_CTL_MEASURE    BIT(3)
#define ADXL34X_REG_POWER_CTL_SLEEP      BIT(2)
#define ADXL34X_REG_POWER_CTL_WAKEUP     GENMASK(1, 0)

/* reg INT_ENABLE */
#define ADXL34X_REG_INT_ENABLE_DATA_READY BIT(7)
#define ADXL34X_REG_INT_ENABLE_SINGLE_TAP BIT(6)
#define ADXL34X_REG_INT_ENABLE_DOUBLE_TAP BIT(5)
#define ADXL34X_REG_INT_ENABLE_ACTIVITY   BIT(4)
#define ADXL34X_REG_INT_ENABLE_INACTIVITY BIT(3)
#define ADXL34X_REG_INT_ENABLE_FREE_FALL  BIT(2)
#define ADXL34X_REG_INT_ENABLE_WATERMARK  BIT(1)
#define ADXL34X_REG_INT_ENABLE_OVERRUN    BIT(0)

/* reg INT_MAP */
#define ADXL34X_REG_INT_MAP_DATA_READY BIT(7)
#define ADXL34X_REG_INT_MAP_SINGLE_TAP BIT(6)
#define ADXL34X_REG_INT_MAP_DOUBLE_TAP BIT(5)
#define ADXL34X_REG_INT_MAP_ACTIVITY   BIT(4)
#define ADXL34X_REG_INT_MAP_INACTIVITY BIT(3)
#define ADXL34X_REG_INT_MAP_FREE_FALL  BIT(2)
#define ADXL34X_REG_INT_MAP_WATERMARK  BIT(2)
#define ADXL34X_REG_INT_MAP_OVERRUN    BIT(0)

/* reg INT_SOURCE */
#define ADXL34X_REG_INT_SOURCE_DATA_READY BIT(7)
#define ADXL34X_REG_INT_SOURCE_SINGLE_TAP BIT(6)
#define ADXL34X_REG_INT_SOURCE_DOUBLE_TAP BIT(5)
#define ADXL34X_REG_INT_SOURCE_ACTIVITY   BIT(4)
#define ADXL34X_REG_INT_SOURCE_INACTIVITY BIT(3)
#define ADXL34X_REG_INT_SOURCE_FREE_FALL  BIT(2)
#define ADXL34X_REG_INT_SOURCE_WATERMARK  BIT(1)
#define ADXL34X_REG_INT_SOURCE_OVERRUN    BIT(0)

/* reg DATA_FORMAT */
#define ADXL34X_REG_DATA_FORMAT_SELF_TEST  BIT(7)
#define ADXL34X_REG_DATA_FORMAT_SPI        BIT(6)
#define ADXL34X_REG_DATA_FORMAT_INT_INVERT BIT(5)
#define ADXL34X_REG_DATA_FORMAT_FULL_RES   BIT(3)
#define ADXL34X_REG_DATA_FORMAT_JUSTIFY    BIT(2)
#define ADXL34X_REG_DATA_FORMAT_RANGE      GENMASK(1, 0)

/* reg FIFO_CTL */
#define ADXL34X_REG_FIFO_CTL_FIFO_MODE GENMASK(7, 6)
#define ADXL34X_REG_FIFO_CTL_TRIGGER   BIT(5)
#define ADXL34X_REG_FIFO_CTL_SAMPLES   GENMASK(4, 0)

/* reg FIFO_STATUS */
#define ADXL34X_REG_FIFO_STATUS_FIFO_TRIG BIT(7)
#define ADXL34X_REG_FIFO_STATUS_ENTRIES   GENMASK(5, 0)

/* reg TAP_SIGN */
#define ADXL34X_REG_TAP_SIGN_XSIGN BIT(6)
#define ADXL34X_REG_TAP_SIGN_YSIGN BIT(5)
#define ADXL34X_REG_TAP_SIGN_ZSIGN BIT(4)
#define ADXL34X_REG_TAP_SIGN_XTAP  BIT(2)
#define ADXL34X_REG_TAP_SIGN_YTAP  BIT(1)
#define ADXL34X_REG_TAP_SIGN_ZTAP  BIT(0)

/* reg ORIENT_CONF */
#define ADXL34X_REG_ORIENT_CONF_INT_ORIENT BIT(7)
#define ADXL34X_REG_ORIENT_CONF_DEAD_ZONE  GENMASK(6, 4)
#define ADXL34X_REG_ORIENT_CONF_INT_3D     BIT(3)
#define ADXL34X_REG_ORIENT_CONF_DIVISOR    GENMASK(2, 0)

/* reg ORIENT */
#define ADXL34X_REG_ORIENT_V2        BIT(6)
#define ADXL34X_REG_ORIENT_2D_ORIENT GENMASK(5, 4)
#define ADXL34X_REG_ORIENT_V3        BIT(3)
#define ADXL34X_REG_ORIENT_3D_ORIENT GENMASK(2, 0)

/* Various */
#define ADXL343_DEVID     0xE5 /**< Device ID of the ADXL343 */
#define ADXL344_DEVID     0xE6 /**< Device ID of the ADXL344 */
#define ADXL345_DEVID     0xE5 /**< Device ID of the ADXL345 */
#define ADXL346_DEVID     0xE6 /**< Device ID of the ADXL346 */
#define ADXL34X_FIFO_SIZE 32   /**< Maximum number of x, y, z values in the FIFO */

#define ADXL34X_SPI_MSG_READ   BIT(7)
#define ADXL34X_SPI_MULTI_BYTE BIT(6)

#endif /* ZEPHYR_DRIVERS_SENSOR_ADXL34X_REG_H_ */
