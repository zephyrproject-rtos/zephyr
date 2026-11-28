/*
 * Copyright (c) 2026 Shreyas Acharya
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Register map for the Bosch Sensortec BMI260 6-axis IMU.
 *
 * The BMI260 belongs to the BMI2xy family and shares its host register
 * map with the BMI270; the chip ID and the on-chip feature set differ.
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BOSCH_BMI260_BMI260_REG_H_
#define ZEPHYR_DRIVERS_SENSOR_BOSCH_BMI260_BMI260_REG_H_

#include <zephyr/sys/util.h>

/* -------------------------------------------------------------------------
 * Device identification / bus
 * -------------------------------------------------------------------------
 */

#define BMI260_CHIP_ID            0x27

/* 7-bit I2C addresses (selected by the SDO pin) */
#define BMI260_I2C_ADDR_PRIMARY   0x68
#define BMI260_I2C_ADDR_SECONDARY 0x69

/* SPI reads return one dummy byte before the first data byte */
#define BMI260_SPI_DUMMY_BYTES    1
#define BMI260_SPI_READ_BIT       BIT(7)
#define BMI260_REG_MASK           GENMASK(6, 0)

/* -------------------------------------------------------------------------
 * Register addresses
 * -------------------------------------------------------------------------
 */

#define BMI260_REG_CHIP_ID          0x00 /* R   - chip identifier */
#define BMI260_REG_ERROR            0x02 /* R   - error flags */
#define BMI260_REG_STATUS           0x03 /* R   - sensor status flags */

/* Auxiliary (magnetometer) data, 8 bytes: X, Y, Z, R/HALL */
#define BMI260_REG_AUX_X_LSB        0x04 /* R */
#define BMI260_REG_AUX_X_MSB        0x05 /* R */
#define BMI260_REG_AUX_Y_LSB        0x06 /* R */
#define BMI260_REG_AUX_Y_MSB        0x07 /* R */
#define BMI260_REG_AUX_Z_LSB        0x08 /* R */
#define BMI260_REG_AUX_Z_MSB        0x09 /* R */
#define BMI260_REG_AUX_R_LSB        0x0A /* R */
#define BMI260_REG_AUX_R_MSB        0x0B /* R */

/* Accelerometer data, 6 bytes */
#define BMI260_REG_ACC_X_LSB        0x0C /* R */
#define BMI260_REG_ACC_X_MSB        0x0D /* R */
#define BMI260_REG_ACC_Y_LSB        0x0E /* R */
#define BMI260_REG_ACC_Y_MSB        0x0F /* R */
#define BMI260_REG_ACC_Z_LSB        0x10 /* R */
#define BMI260_REG_ACC_Z_MSB        0x11 /* R */

/* Gyroscope data, 6 bytes */
#define BMI260_REG_GYR_X_LSB        0x12 /* R */
#define BMI260_REG_GYR_X_MSB        0x13 /* R */
#define BMI260_REG_GYR_Y_LSB        0x14 /* R */
#define BMI260_REG_GYR_Y_MSB        0x15 /* R */
#define BMI260_REG_GYR_Z_LSB        0x16 /* R */
#define BMI260_REG_GYR_Z_MSB        0x17 /* R */

/* 24-bit sensor time, 39.0625 us per LSB */
#define BMI260_REG_SENSORTIME_0     0x18 /* R */
#define BMI260_REG_SENSORTIME_1     0x19 /* R */
#define BMI260_REG_SENSORTIME_2     0x1A /* R */

#define BMI260_REG_EVENT            0x1B /* R   - power-on / error event */
#define BMI260_REG_INT_STATUS_0     0x1C /* R   - feature interrupt status */
#define BMI260_REG_INT_STATUS_1     0x1D /* R   - data interrupt status */
#define BMI260_REG_SC_OUT_0         0x1E /* R   - step counter LSB */
#define BMI260_REG_SC_OUT_1         0x1F /* R   - step counter MSB */
#define BMI260_REG_WR_GEST_ACT      0x20 /* R   - gesture / activity output */
#define BMI260_REG_INTERNAL_STATUS  0x21 /* R   - config-load status */
#define BMI260_REG_TEMPERATURE_0    0x22 /* R   - temperature LSB */
#define BMI260_REG_TEMPERATURE_1    0x23 /* R   - temperature MSB */
#define BMI260_REG_FIFO_LENGTH_0    0x24 /* R   - FIFO fill level LSB */
#define BMI260_REG_FIFO_LENGTH_1    0x25 /* R   - FIFO fill level MSB */
#define BMI260_REG_FIFO_DATA        0x26 /* R   - FIFO read port */

#define BMI260_REG_FEAT_PAGE        0x2F /* R/W - feature page select */

/* Feature register window; contents depend on BMI260_REG_FEAT_PAGE */
#define BMI260_REG_FEATURES_0       0x30 /* R/W */
#define BMI260_REG_FEATURES_1       0x31 /* R/W */
#define BMI260_REG_FEATURES_2       0x32 /* R/W */
#define BMI260_REG_FEATURES_3       0x33 /* R/W */
#define BMI260_REG_FEATURES_4       0x34 /* R/W */
#define BMI260_REG_FEATURES_5       0x35 /* R/W */
#define BMI260_REG_FEATURES_6       0x36 /* R/W */
#define BMI260_REG_FEATURES_7       0x37 /* R/W */
#define BMI260_REG_FEATURES_8       0x38 /* R/W */
#define BMI260_REG_FEATURES_9       0x39 /* R/W */
#define BMI260_REG_FEATURES_10      0x3A /* R/W */
#define BMI260_REG_FEATURES_11      0x3B /* R/W */
#define BMI260_REG_FEATURES_12      0x3C /* R/W */
#define BMI260_REG_FEATURES_13      0x3D /* R/W */
#define BMI260_REG_FEATURES_14      0x3E /* R/W */
#define BMI260_REG_FEATURES_15      0x3F /* R/W */

#define BMI260_FEAT_PAGE_0          0x00
#define BMI260_FEAT_PAGE_1          0x01
#define BMI260_FEAT_PAGE_2          0x02
#define BMI260_FEAT_PAGE_3          0x03
#define BMI260_FEAT_PAGE_4          0x04
#define BMI260_FEAT_PAGE_5          0x05

#define BMI260_REG_ACC_CONF         0x40 /* R/W - acc ODR, bandwidth, filter */
#define BMI260_REG_ACC_RANGE        0x41 /* R/W - acc full-scale range */
#define BMI260_REG_GYR_CONF         0x42 /* R/W - gyr ODR, bandwidth, filter */
#define BMI260_REG_GYR_RANGE        0x43 /* R/W - gyr full-scale range */
#define BMI260_REG_AUX_CONF         0x44 /* R/W - aux ODR and offset */
#define BMI260_REG_FIFO_DOWNS       0x45 /* R/W - FIFO downsampling */
#define BMI260_REG_FIFO_WTM_0       0x46 /* R/W - FIFO watermark LSB */
#define BMI260_REG_FIFO_WTM_1       0x47 /* R/W - FIFO watermark MSB */
#define BMI260_REG_FIFO_CONFIG_0    0x48 /* R/W - FIFO stop-on-full, time */
#define BMI260_REG_FIFO_CONFIG_1    0x49 /* R/W - FIFO source enables */
#define BMI260_REG_SATURATION       0x4A /* R   - saturation flags */
#define BMI260_REG_AUX_DEV_ID       0x4B /* R/W - aux I2C device address */
#define BMI260_REG_AUX_IF_CONF      0x4C /* R/W - aux interface config */
#define BMI260_REG_AUX_RD_ADDR      0x4D /* R/W - aux read start address */
#define BMI260_REG_AUX_WR_ADDR      0x4E /* R/W - aux write address */
#define BMI260_REG_AUX_WR_DATA      0x4F /* R/W - aux write data */
#define BMI260_REG_ERR_REG_MSK      0x52 /* R/W - error interrupt mask */
#define BMI260_REG_INT1_IO_CTRL     0x53 /* R/W - INT1 pin electrical config */
#define BMI260_REG_INT2_IO_CTRL     0x54 /* R/W - INT2 pin electrical config */
#define BMI260_REG_INT_LATCH        0x55 /* R/W - interrupt latch mode */
#define BMI260_REG_INT1_MAP_FEAT    0x56 /* R/W - feature ints -> INT1 */
#define BMI260_REG_INT2_MAP_FEAT    0x57 /* R/W - feature ints -> INT2 */
#define BMI260_REG_INT_MAP_DATA     0x58 /* R/W - data ints -> INT1/INT2 */
#define BMI260_REG_INIT_CTRL        0x59 /* R/W - config load control */
#define BMI260_REG_INIT_ADDR_0      0x5B /* R/W - config load address [3:0] */
#define BMI260_REG_INIT_ADDR_1      0x5C /* R/W - config load address [11:4] */
#define BMI260_REG_INIT_DATA        0x5E /* W   - config file write port */
#define BMI260_REG_INTERNAL_ERROR   0x5F /* R   - internal error flags */
#define BMI260_REG_AUX_IF_TRIM      0x68 /* R/W - aux interface trim */
#define BMI260_REG_GYR_CRT_CONF     0x69 /* R/W - gyro CRT control */
#define BMI260_REG_NVM_CONF         0x6A /* R/W - NVM programming enable */
#define BMI260_REG_IF_CONF          0x6B /* R/W - serial interface settings */
#define BMI260_REG_DRV              0x6C /* R/W - output pad drive strength */
#define BMI260_REG_ACC_SELF_TEST    0x6D /* R/W - acc self-test control */
#define BMI260_REG_GYR_SELF_TEST    0x6E /* R   - gyro self-test result */
#define BMI260_REG_NV_CONF          0x70 /* R/W - NVM-backed configuration */

/* Accel (3 x 8-bit) and gyro (3 x 10-bit + control) offset compensation */
#define BMI260_REG_OFFSET_0         0x71 /* R/W - acc X offset */
#define BMI260_REG_OFFSET_1         0x72 /* R/W - acc Y offset */
#define BMI260_REG_OFFSET_2         0x73 /* R/W - acc Z offset */
#define BMI260_REG_OFFSET_3         0x74 /* R/W - gyr X offset [7:0] */
#define BMI260_REG_OFFSET_4         0x75 /* R/W - gyr Y offset [7:0] */
#define BMI260_REG_OFFSET_5         0x76 /* R/W - gyr Z offset [7:0] */
#define BMI260_REG_OFFSET_6         0x77 /* R/W - offset MSBs and enables */

#define BMI260_REG_PWR_CONF         0x7C /* R/W - power save configuration */
#define BMI260_REG_PWR_CTRL         0x7D /* R/W - sensor enables */
#define BMI260_REG_CMD              0x7E /* W   - command register */

/* -------------------------------------------------------------------------
 * ERROR (0x02)
 * -------------------------------------------------------------------------
 */

#define BMI260_ERR_FATAL_ERR        BIT(0)
#define BMI260_ERR_INTERNAL_ERR_MSK GENMASK(4, 1)
#define BMI260_ERR_INTERNAL_ERR_POS 1
#define BMI260_ERR_FIFO_ERR         BIT(6)
#define BMI260_ERR_AUX_ERR          BIT(7)

/* -------------------------------------------------------------------------
 * STATUS (0x03)
 * -------------------------------------------------------------------------
 */

#define BMI260_STATUS_AUX_BUSY      BIT(2)
#define BMI260_STATUS_CMD_RDY       BIT(4)
#define BMI260_STATUS_DRDY_AUX      BIT(5)
#define BMI260_STATUS_DRDY_GYR      BIT(6)
#define BMI260_STATUS_DRDY_ACC      BIT(7)

/* -------------------------------------------------------------------------
 * EVENT (0x1B)
 * -------------------------------------------------------------------------
 */

#define BMI260_EVENT_POR_DETECTED   BIT(0)
#define BMI260_EVENT_ERR_CODE_MSK   GENMASK(4, 2)
#define BMI260_EVENT_ERR_CODE_POS   2

/* -------------------------------------------------------------------------
 * INT_STATUS_1 (0x1D) - data interrupts
 * -------------------------------------------------------------------------
 */

#define BMI260_INT_STATUS_1_FFULL_INT    BIT(0)
#define BMI260_INT_STATUS_1_FWM_INT      BIT(1)
#define BMI260_INT_STATUS_1_ERR_INT      BIT(2)
#define BMI260_INT_STATUS_1_AUX_DRDY_INT BIT(5)
#define BMI260_INT_STATUS_1_GYR_DRDY_INT BIT(6)
#define BMI260_INT_STATUS_1_ACC_DRDY_INT BIT(7)

/* -------------------------------------------------------------------------
 * INTERNAL_STATUS (0x21)
 * -------------------------------------------------------------------------
 */

#define BMI260_INST_MESSAGE_MSK        GENMASK(3, 0)
#define BMI260_INST_MESSAGE_NOT_INIT   0x00
#define BMI260_INST_MESSAGE_INIT_OK    0x01
#define BMI260_INST_MESSAGE_INIT_ERR   0x02
#define BMI260_INST_MESSAGE_DRV_ERR    0x03
#define BMI260_INST_MESSAGE_SNS_STOP   0x04
#define BMI260_INST_MESSAGE_NVM_ERR    0x05
#define BMI260_INST_MESSAGE_STRTUP_ERR 0x06
#define BMI260_INST_MESSAGE_COMPAT_ERR 0x07

#define BMI260_INST_AXES_REMAP_ERROR   BIT(5)
#define BMI260_INST_ODR_50HZ_ERROR     BIT(6)

/* -------------------------------------------------------------------------
 * ACC_CONF (0x40)
 * -------------------------------------------------------------------------
 */

#define BMI260_ACC_ODR_MSK      GENMASK(3, 0)
#define BMI260_ACC_ODR_POS      0
#define BMI260_ACC_ODR_25D32_HZ 0x01 /* 0.78125 Hz */
#define BMI260_ACC_ODR_25D16_HZ 0x02 /* 1.5625 Hz */
#define BMI260_ACC_ODR_25D8_HZ  0x03 /* 3.125 Hz */
#define BMI260_ACC_ODR_25D4_HZ  0x04 /* 6.25 Hz */
#define BMI260_ACC_ODR_25D2_HZ  0x05 /* 12.5 Hz */
#define BMI260_ACC_ODR_25_HZ    0x06
#define BMI260_ACC_ODR_50_HZ    0x07
#define BMI260_ACC_ODR_100_HZ   0x08
#define BMI260_ACC_ODR_200_HZ   0x09
#define BMI260_ACC_ODR_400_HZ   0x0A
#define BMI260_ACC_ODR_800_HZ   0x0B
#define BMI260_ACC_ODR_1600_HZ  0x0C

/* acc_bwp: oversampling in performance mode / averaging in power mode */
#define BMI260_ACC_BWP_MSK        GENMASK(6, 4)
#define BMI260_ACC_BWP_POS        4
#define BMI260_ACC_BWP_OSR4_AVG1  0x00
#define BMI260_ACC_BWP_OSR2_AVG2  0x01
#define BMI260_ACC_BWP_NORM_AVG4  0x02
#define BMI260_ACC_BWP_CIC_AVG8   0x03
#define BMI260_ACC_BWP_RES_AVG16  0x04
#define BMI260_ACC_BWP_RES_AVG32  0x05
#define BMI260_ACC_BWP_RES_AVG64  0x06
#define BMI260_ACC_BWP_RES_AVG128 0x07

#define BMI260_ACC_FILT_MSK      BIT(7)
#define BMI260_ACC_FILT_POS      7
#define BMI260_ACC_FILT_PWR_OPT  0x00 /* power optimised (averaging) */
#define BMI260_ACC_FILT_PERF_OPT 0x01 /* performance optimised (filter) */

/* -------------------------------------------------------------------------
 * ACC_RANGE (0x41)
 * -------------------------------------------------------------------------
 */

#define BMI260_ACC_RANGE_MSK GENMASK(1, 0)
#define BMI260_ACC_RANGE_POS 0
#define BMI260_ACC_RANGE_2G  0x00
#define BMI260_ACC_RANGE_4G  0x01
#define BMI260_ACC_RANGE_8G  0x02
#define BMI260_ACC_RANGE_16G 0x03

/* -------------------------------------------------------------------------
 * GYR_CONF (0x42)
 * -------------------------------------------------------------------------
 */

#define BMI260_GYR_ODR_MSK     GENMASK(3, 0)
#define BMI260_GYR_ODR_POS     0
#define BMI260_GYR_ODR_25_HZ   0x06
#define BMI260_GYR_ODR_50_HZ   0x07
#define BMI260_GYR_ODR_100_HZ  0x08
#define BMI260_GYR_ODR_200_HZ  0x09
#define BMI260_GYR_ODR_400_HZ  0x0A
#define BMI260_GYR_ODR_800_HZ  0x0B
#define BMI260_GYR_ODR_1600_HZ 0x0C
#define BMI260_GYR_ODR_3200_HZ 0x0D

#define BMI260_GYR_BWP_MSK  GENMASK(5, 4)
#define BMI260_GYR_BWP_POS  4
#define BMI260_GYR_BWP_OSR4 0x00
#define BMI260_GYR_BWP_OSR2 0x01
#define BMI260_GYR_BWP_NORM 0x02

/* gyr_noise_perf */
#define BMI260_GYR_FILT_NOISE_MSK  BIT(6)
#define BMI260_GYR_FILT_NOISE_POS  6
#define BMI260_GYR_FILT_NOISE_PWR  0x00
#define BMI260_GYR_FILT_NOISE_PERF 0x01

/* gyr_filter_perf */
#define BMI260_GYR_FILT_MSK      BIT(7)
#define BMI260_GYR_FILT_POS      7
#define BMI260_GYR_FILT_PWR_OPT  0x00
#define BMI260_GYR_FILT_PERF_OPT 0x01

/* -------------------------------------------------------------------------
 * GYR_RANGE (0x43)
 * -------------------------------------------------------------------------
 */

#define BMI260_GYR_RANGE_MSK     GENMASK(2, 0)
#define BMI260_GYR_RANGE_POS     0
#define BMI260_GYR_RANGE_2000DPS 0x00
#define BMI260_GYR_RANGE_1000DPS 0x01
#define BMI260_GYR_RANGE_500DPS  0x02
#define BMI260_GYR_RANGE_250DPS  0x03
#define BMI260_GYR_RANGE_125DPS  0x04

/* ois_range */
#define BMI260_GYR_OIS_RANGE_MSK     BIT(3)
#define BMI260_GYR_OIS_RANGE_POS     3
#define BMI260_GYR_OIS_RANGE_250DPS  0x00
#define BMI260_GYR_OIS_RANGE_2000DPS 0x01

/* -------------------------------------------------------------------------
 * FIFO_DOWNS (0x45)
 * -------------------------------------------------------------------------
 */

#define BMI260_FIFO_GYR_DOWNS_MSK   GENMASK(2, 0)
#define BMI260_FIFO_GYR_DOWNS_POS   0
#define BMI260_FIFO_GYR_FILT_EN_MSK BIT(3)
#define BMI260_FIFO_GYR_FILT_EN_POS 3
#define BMI260_FIFO_ACC_DOWNS_MSK   GENMASK(6, 4)
#define BMI260_FIFO_ACC_DOWNS_POS   4
#define BMI260_FIFO_ACC_FILT_EN_MSK BIT(7)
#define BMI260_FIFO_ACC_FILT_EN_POS 7

/* -------------------------------------------------------------------------
 * FIFO_CONFIG_0 (0x48) / FIFO_CONFIG_1 (0x49)
 * -------------------------------------------------------------------------
 */

#define BMI260_FIFO_CONFIG_0_STOP_ON_FULL_MSK BIT(0)
#define BMI260_FIFO_CONFIG_0_FIFO_TIME_EN_MSK BIT(1)

#define BMI260_FIFO_CONFIG_1_FIFO_TAG_INT1_EN_MSK BIT(2)
#define BMI260_FIFO_CONFIG_1_FIFO_TAG_INT2_EN_MSK BIT(3)
#define BMI260_FIFO_CONFIG_1_FIFO_HEADER_EN_MSK   BIT(4)
#define BMI260_FIFO_CONFIG_1_FIFO_AUX_EN_MSK      BIT(5)
#define BMI260_FIFO_CONFIG_1_FIFO_ACC_EN_MSK      BIT(6)
#define BMI260_FIFO_CONFIG_1_FIFO_GYR_EN_MSK      BIT(7)

/* FIFO geometry */
#define BMI260_FIFO_SIZE_BYTES                  2048
#define BMI260_FIFO_HEADER_BYTES                1
#define BMI260_FIFO_FRAME_PAYLOAD_ACC_BYTES     6
#define BMI260_FIFO_FRAME_PAYLOAD_GYR_BYTES     6
#define BMI260_FIFO_FRAME_PAYLOAD_AUX_BYTES     8
#define BMI260_FIFO_FRAME_PAYLOAD_ACC_GYR_BYTES 12
#define BMI260_FIFO_FRAME_ACC_GYR_BYTES                                                            \
	(BMI260_FIFO_HEADER_BYTES + BMI260_FIFO_FRAME_PAYLOAD_ACC_GYR_BYTES)

/* FIFO frame header: mode (bits 7:6) and parameter (bits 5:2) */
#define BMI260_FIFO_HEADER_MODE_MSK GENMASK(7, 6)
#define BMI260_FIFO_HEADER_MODE_POS 6
#define BMI260_FIFO_HEADER_PARM_MSK GENMASK(5, 2)
#define BMI260_FIFO_HEADER_PARM_POS 2

#define BMI260_FIFO_HEADER_CONTROL 0x01
#define BMI260_FIFO_HEADER_REGULAR 0x02

#define BMI260_FIFO_FHPARM_ACC BIT(0)
#define BMI260_FIFO_FHPARM_GYR BIT(1)
#define BMI260_FIFO_FHPARM_AUX BIT(2)

/* Control frame opcodes (fh_mode == BMI260_FIFO_HEADER_CONTROL) */
#define BMI260_FIFO_CTRL_SKIP_FRAME       0x01
#define BMI260_FIFO_CTRL_SENSORTIME_FRAME 0x02
#define BMI260_FIFO_CTRL_INPUT_CFG_FRAME  0x03

/* -------------------------------------------------------------------------
 * SATURATION (0x4A)
 * -------------------------------------------------------------------------
 */

#define BMI260_SATURATION_ACC_X BIT(0)
#define BMI260_SATURATION_ACC_Y BIT(1)
#define BMI260_SATURATION_ACC_Z BIT(2)
#define BMI260_SATURATION_GYR_X BIT(3)
#define BMI260_SATURATION_GYR_Y BIT(4)
#define BMI260_SATURATION_GYR_Z BIT(5)

/* -------------------------------------------------------------------------
 * AUX_IF_CONF (0x4C)
 * -------------------------------------------------------------------------
 */

#define BMI260_AUX_IF_CONF_RD_BURST_MSK    GENMASK(1, 0)
#define BMI260_AUX_IF_CONF_RD_BURST_POS    0
#define BMI260_AUX_IF_CONF_RD_BURST_1_BYTE 0x00
#define BMI260_AUX_IF_CONF_RD_BURST_2_BYTE 0x01
#define BMI260_AUX_IF_CONF_RD_BURST_6_BYTE 0x02
#define BMI260_AUX_IF_CONF_RD_BURST_8_BYTE 0x03

#define BMI260_AUX_IF_CONF_OFFSET_MSK  GENMASK(5, 4)
#define BMI260_AUX_IF_CONF_OFFSET_POS  4
#define BMI260_AUX_IF_CONF_FCU_WR_EN   BIT(6)
#define BMI260_AUX_IF_CONF_MAN_MODE_EN BIT(7)

/* -------------------------------------------------------------------------
 * INT1_IO_CTRL (0x53) / INT2_IO_CTRL (0x54)
 * -------------------------------------------------------------------------
 */

#define BMI260_INT_IO_CTRL_LVL       BIT(1) /* 0 = active low, 1 = active high */
#define BMI260_INT_IO_CTRL_OD        BIT(2) /* 0 = push-pull, 1 = open-drain */
#define BMI260_INT_IO_CTRL_OUTPUT_EN BIT(3)
#define BMI260_INT_IO_CTRL_INPUT_EN  BIT(4)

/* -------------------------------------------------------------------------
 * INT_LATCH (0x55)
 * -------------------------------------------------------------------------
 */

#define BMI260_INT_LATCH_MSK       BIT(0)
#define BMI260_INT_LATCH_NONE      0x00
#define BMI260_INT_LATCH_PERMANENT 0x01

/* -------------------------------------------------------------------------
 * INT_MAP_DATA (0x58)
 * -------------------------------------------------------------------------
 */

#define BMI260_INT_MAP_DATA_FFULL_INT1 BIT(0)
#define BMI260_INT_MAP_DATA_FWM_INT1   BIT(1)
#define BMI260_INT_MAP_DATA_DRDY_INT1  BIT(2)
#define BMI260_INT_MAP_DATA_ERR_INT1   BIT(3)
#define BMI260_INT_MAP_DATA_FFULL_INT2 BIT(4)
#define BMI260_INT_MAP_DATA_FWM_INT2   BIT(5)
#define BMI260_INT_MAP_DATA_DRDY_INT2  BIT(6)
#define BMI260_INT_MAP_DATA_ERR_INT2   BIT(7)

/* -------------------------------------------------------------------------
 * INIT_CTRL (0x59) and config-file download
 * -------------------------------------------------------------------------
 */

#define BMI260_PREPARE_CONFIG_LOAD  0x00
#define BMI260_COMPLETE_CONFIG_LOAD 0x01

/* Size of the configuration image; BUILD_ASSERTed against the array itself */
#define BMI260_CONFIG_FILE_SIZE           8192

/* Maximum burst length used when streaming the config file to INIT_DATA */
#define BMI260_WR_LEN                     32
#define BMI260_CONFIG_FILE_RETRIES        15
#define BMI260_CONFIG_FILE_POLL_PERIOD_US 10000
#define BMI260_INTER_WRITE_DELAY_US       1000

/* -------------------------------------------------------------------------
 * INTERNAL_ERROR (0x5F)
 * -------------------------------------------------------------------------
 */

#define BMI260_INTERNAL_ERROR_INT_ERR_1         BIT(1)
#define BMI260_INTERNAL_ERROR_INT_ERR_2         BIT(2)
#define BMI260_INTERNAL_ERROR_FEAT_ENG_DISABLED BIT(4)

/* -------------------------------------------------------------------------
 * GYR_CRT_CONF (0x69)
 * -------------------------------------------------------------------------
 */

#define BMI260_GYR_CRT_CRT_RUNNING_MSK BIT(2)
#define BMI260_GYR_CRT_CRT_RUNNING_POS 2
#define BMI260_GYR_CRT_RDY_FOR_DL_MSK  BIT(3)
#define BMI260_GYR_CRT_RDY_FOR_DL_POS  3

/* -------------------------------------------------------------------------
 * NVM_CONF (0x6A)
 * -------------------------------------------------------------------------
 */

#define BMI260_NVM_CONF_PROG_EN BIT(1)

/* -------------------------------------------------------------------------
 * IF_CONF (0x6B)
 * -------------------------------------------------------------------------
 */

#define BMI260_IF_CONF_SPI3_MSK    BIT(0) /* 0 = 4-wire SPI, 1 = 3-wire SPI */
#define BMI260_IF_CONF_IF_MODE_MSK GENMASK(5, 4)
#define BMI260_IF_CONF_IF_MODE_POS 4
#define BMI260_IF_CONF_IF_MODE_PRI 0x00 /* primary interface only */
#define BMI260_IF_CONF_IF_MODE_AUX 0x01 /* aux I2C interface enabled */

/* -------------------------------------------------------------------------
 * ACC_SELF_TEST (0x6D) / GYR_SELF_TEST (0x6E)
 * -------------------------------------------------------------------------
 */

#define BMI260_ACC_SELF_TEST_EN_MSK BIT(0)
#define BMI260_ACC_SELF_TEST_SIGN   BIT(2) /* 0 = negative, 1 = positive */
#define BMI260_ACC_SELF_TEST_AMP    BIT(3)

#define BMI260_GYR_SELF_TEST_RDY       BIT(1)
#define BMI260_GYR_SELF_TEST_RATIO_OK  BIT(2)
#define BMI260_GYR_SELF_TEST_OFFSET_OK BIT(3)
#define BMI260_GYR_SELF_TEST_AXES_OK   BIT(4)

/* -------------------------------------------------------------------------
 * NV_CONF (0x70)
 * -------------------------------------------------------------------------
 */

#define BMI260_NV_CONF_SPI_EN      BIT(0)
#define BMI260_NV_CONF_I2C_WDT_SEL BIT(1) /* 0 = 1.25 ms, 1 = 40 ms */
#define BMI260_NV_CONF_I2C_WDT_EN  BIT(2)
#define BMI260_NV_CONF_ACC_OFF_EN  BIT(3)

/* -------------------------------------------------------------------------
 * OFFSET_6 (0x77)
 * -------------------------------------------------------------------------
 */

#define BMI260_OFFSET_6_GYR_X_MSB_MSK GENMASK(1, 0)
#define BMI260_OFFSET_6_GYR_X_MSB_POS 0
#define BMI260_OFFSET_6_GYR_Y_MSB_MSK GENMASK(3, 2)
#define BMI260_OFFSET_6_GYR_Y_MSB_POS 2
#define BMI260_OFFSET_6_GYR_Z_MSB_MSK GENMASK(5, 4)
#define BMI260_OFFSET_6_GYR_Z_MSB_POS 4
#define BMI260_OFFSET_6_GYR_GAIN_EN   BIT(6)
#define BMI260_OFFSET_6_GYR_OFF_EN    BIT(7)

/* -------------------------------------------------------------------------
 * PWR_CONF (0x7C)
 * -------------------------------------------------------------------------
 */

#define BMI260_PWR_CONF_ADV_PWR_SAVE_MSK BIT(0)
#define BMI260_PWR_CONF_ADV_PWR_SAVE_POS 0
#define BMI260_PWR_CONF_ADV_PWR_SAVE_EN  0x01
#define BMI260_PWR_CONF_ADV_PWR_SAVE_DIS 0x00

#define BMI260_PWR_CONF_FIFO_SELF_WKUP_MSK BIT(1)
#define BMI260_PWR_CONF_FIFO_SELF_WKUP_POS 1
#define BMI260_PWR_CONF_FIFO_SELF_WKUP_EN  0x01
#define BMI260_PWR_CONF_FIFO_SELF_WKUP_DIS 0x00

#define BMI260_PWR_CONF_FUP_EN_MSK BIT(2)
#define BMI260_PWR_CONF_FUP_EN_POS 2
#define BMI260_PWR_CONF_FUP_EN     0x01
#define BMI260_PWR_CONF_FUP_DIS    0x00

/* -------------------------------------------------------------------------
 * PWR_CTRL (0x7D)
 * -------------------------------------------------------------------------
 */

#define BMI260_PWR_CTRL_MSK     GENMASK(3, 0)
#define BMI260_PWR_CTRL_AUX_EN  BIT(0)
#define BMI260_PWR_CTRL_GYR_EN  BIT(1)
#define BMI260_PWR_CTRL_ACC_EN  BIT(2)
#define BMI260_PWR_CTRL_TEMP_EN BIT(3)

/* -------------------------------------------------------------------------
 * CMD (0x7E)
 * -------------------------------------------------------------------------
 */

#define BMI260_CMD_G_TRIGGER  0x02 /* run gyro CRT / offset trigger */
#define BMI260_CMD_USR_GAIN   0x03 /* apply user gain update */
#define BMI260_CMD_NVM_PROG   0xA0 /* write NVM-backed registers */
#define BMI260_CMD_FIFO_FLUSH 0xB0 /* clear FIFO contents */
#define BMI260_CMD_SOFT_RESET 0xB6 /* reset to power-on state */

/* -------------------------------------------------------------------------
 * Timing (microseconds)
 * -------------------------------------------------------------------------
 */

#define BMI260_POWER_ON_TIME                500
#define BMI260_SOFT_RESET_TIME              2000
#define BMI260_ACC_SUS_TO_NOR_START_UP_TIME 2000
#define BMI260_GYR_SUS_TO_NOR_START_UP_TIME 45000
#define BMI260_GYR_FAST_START_UP_TIME       2000
#define BMI260_TRANSC_DELAY_SUSPEND         450
#define BMI260_TRANSC_DELAY_NORMAL          2

/* -------------------------------------------------------------------------
 * Temperature sensor (TEMPERATURE_0/1, 0x22/0x23)
 *
 * 16-bit signed value, 0x8000 marks an invalid reading, 512 LSB per
 * degree Celsius with 0 corresponding to 23 degC.
 * -------------------------------------------------------------------------
 */

#define BMI260_TEMP_INVALID       0x8000
#define BMI260_TEMP_CENTER_OFFSET 23000000LL
#define BMI260_TEMP_LSB_PER_DEG   512LL

/* -------------------------------------------------------------------------
 * Helpers
 * -------------------------------------------------------------------------
 */

#define BMI260_SET_BITS(reg_data, bitname, data)                                                   \
	(((reg_data) & ~(bitname##_MSK)) | (((data) << bitname##_POS) & (bitname##_MSK)))

#define BMI260_SET_BITS_POS_0(reg_data, bitname, data)                                             \
	(((reg_data) & ~(bitname##_MSK)) | ((data) & (bitname##_MSK)))

#define BMI260_GET_BITS(reg_data, bitname)                                                         \
	(((reg_data) & (bitname##_MSK)) >> (bitname##_POS))

#endif /* ZEPHYR_DRIVERS_SENSOR_BOSCH_BMI260_BMI260_REG_H_ */
