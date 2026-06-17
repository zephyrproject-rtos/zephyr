/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADI_ADIS16607_H_
#define ZEPHYR_DRIVERS_SENSOR_ADI_ADIS16607_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

/* Register addresses */
#define ADIS16607_REG_DEV_ID         0x00
#define ADIS16607_REG_DIAG_STAT      0x05
#define ADIS16607_REG_USER_GPIO_CFG  0x2F
#define ADIS16607_REG_SPI_HALFDUPLEX 0x32
#define ADIS16607_REG_USER_DATA_CFG  0x34
#define ADIS16607_REG_SW_RES         0x36
#define ADIS16607_REG_SELF_TEST      0x39
#define ADIS16607_REG_DEC_RATE       0x3A
#define ADIS16607_REG_DIGITAL_STATUS 0x4E

#define ADIS16607_REG_SELF_TEST_DATA(x) ((x) + 0x23)

/* Bit masks and values */
#define ADIS16607_LOCK_SPI_HALFDUPLEX 0xB4B4

#define ADIS16607_DATA_CNTR_EN_MASK BIT(14)
#define ADIS16607_BURST32_MASK      BIT(15)

#define ADIS16607_GPIO_ACTIVE_DATA_RDY BIT(9)
#define ADIS16607_GPIO_ACTIVE_RESET    BIT(0)

#define ADIS16607_BOOTLOADER_BUSY_MASK BIT(0)

#define ADIS16607_DIAG_BOOT_MEM_FAIL BIT(9)
#define ADIS16607_DIAG_PWR_FAIL      BIT(11)
#define ADIS16607_DIAG_ACCEL_FAIL    BIT(12)
#define ADIS16607_DIAG_GYRO_FAIL     BIT(13)
#define ADIS16607_DIAG_ERROR_MASK                                                                  \
	(ADIS16607_DIAG_BOOT_MEM_FAIL | ADIS16607_DIAG_PWR_FAIL | ADIS16607_DIAG_ACCEL_FAIL |      \
	 ADIS16607_DIAG_GYRO_FAIL)

#define ADIS16607_SNSR_SELF_TEST_MASK BIT(6)
#define ADIS16607_ST_FORCE_MASK       BIT(7)

#define ADIS16607_ACCEL_XY_DELTA_MAX 260
#define ADIS16607_ACCEL_Z_DELTA_MAX  4000
#define ADIS16607_GYRO_DELTA_MAX     2600

/*
 * 32-bit burst format (DATA_CNTR enabled):
 *   cmd[0:3]   = {0x85, 0, 0, 0}          (4 bytes, sent by host)
 *   data[0:3]  = ACCEL_X  MSW+LSW (32-bit, 24-bit data in bits [31:8])
 *   data[4:7]  = ACCEL_Y
 *   data[8:11] = ACCEL_Z
 *   data[12:15]= GYRO_X
 *   data[16:19]= GYRO_Y
 *   data[20:23]= GYRO_Z
 *   data[24:27]= DELTVEL_X
 *   data[28:31]= DELTVEL_Y
 *   data[32:35]= DELTVEL_Z
 *   data[36:39]= DELTANG_X
 *   data[40:43]= DELTANG_Y
 *   data[44:47]= DELTANG_Z
 *   data[48:49]= TEMP (16-bit signed)
 *   data[50:51]= DATA_CNTR (16-bit)
 *   data[52:53]= CHECKSUM (16-bit)
 *
 * Each 32-bit entry: 24-bit = (MSW << 8) | (LSW >> 8), sign-extended.
 */
#define ADIS16607_BURST_CMD      0x85
#define ADIS16607_BURST_CMD_SIZE 4
#define ADIS16607_BURST_DATA_LEN 54
#define ADIS16607_BURST_TOTAL    (ADIS16607_BURST_CMD_SIZE + ADIS16607_BURST_DATA_LEN)

#define ADIS16607_BURST_OFF_ACCEL_X   0
#define ADIS16607_BURST_OFF_ACCEL_Y   4
#define ADIS16607_BURST_OFF_ACCEL_Z   8
#define ADIS16607_BURST_OFF_GYRO_X    12
#define ADIS16607_BURST_OFF_GYRO_Y    16
#define ADIS16607_BURST_OFF_GYRO_Z    20
#define ADIS16607_BURST_OFF_TEMP      48
#define ADIS16607_BURST_OFF_DATA_CNTR 50
#define ADIS16607_BURST_OFF_CHECKSUM  52

/* Initialisation timeouts */
#define ADIS16607_RESET_PULSE_US     100
#define ADIS16607_RESET_DELAY_MS     130
#define ADIS16607_SW_RESET_DELAY_MS  50
#define ADIS16607_STARTUP_DELAY_MS   100
#define ADIS16607_SELF_TEST_DELAY_MS 15

/* Minimum stall time the device needs between successive SPI transactions. */
#define ADIS16607_STALL_TIME_US 16

/*
 * Internal sample clock: 8 kHz.
 * ODR = ADIS16607_BASE_CLK_HZ / (DEC_RATE + 1)
 * DEC_RATE range: 0 (8 kHz) .. 65535 (~122 Hz)
 */
#define ADIS16607_BASE_CLK_HZ 8000U

/*
 * Accel (all variants): +/-40 g full scale, 24-bit signed (2^23 LSB).
 *   1 LSB = 40 * 9.80665 / 2^23 m/s^2 ~= 46.77 um/s^2
 *   SCALE_MICRO = 40g in um/s^2 = 40 * 9,806,650 um/s^2
 *   micro_m_s2 = raw * SCALE_MICRO / DENOM
 */
#define ADIS16607_ACCEL_SCALE_MICRO 392266000 /* 40 * 9806650 um/s^2 */
#define ADIS16607_ACCEL_DENOM       8388608   /* 2^23 */

/*
 * Gyro scale in urad/s at full scale (2^23 LSB):
 *   +/-450 deg/s  = 450 * pi/180 * 1e6 urad/s ~= 7,853,981
 *   +/-2000 deg/s = 2000 * pi/180 * 1e6 urad/s ~= 34,906,585
 *   micro_rad_s = raw * gyro_scale / DENOM  (DENOM = 2^23)
 */
#define ADIS16607_GYRO_DENOM         8388608 /* 2^23 */
#define ADIS16607_2_GYRO_SCALE_MICRO 7853981
#define ADIS16607_3_GYRO_SCALE_MICRO 34906585

/*
 * Temperature: degC = (raw + OFFSET) * SCALE / 1000000
 *   raw = 0 at 25 degC; scale = 5 mdegC/LSB = 5000 udegC/LSB.
 */
#define ADIS16607_TEMP_OFFSET        5000
#define ADIS16607_TEMP_SCALE_MICRO_C 5000

/* Per-variant constant chip information. */
struct adis16607_chip_info {
	/* Gyro full-scale value in urad/s at 2^23 LSB. */
	int32_t gyro_scale_micro;
	/* Expected device ID read from REG_DEV_ID. */
	uint16_t dev_id;
};

/* Compile-time device configuration (populated from DT). */
struct adis16607_config {
	struct spi_dt_spec spi;
	const struct adis16607_chip_info *chip_info;
	uint16_t odr;
	/* Optional hardware reset GPIO (active-low output). */
	struct gpio_dt_spec reset_gpio;
#ifdef CONFIG_ADIS16607_TRIGGER
	/* Data-ready interrupt GPIO (active-high input). */
	struct gpio_dt_spec drdy_gpio;
#endif
};

/* Runtime device state. */
struct adis16607_data {
	/* Latest burst-read sensor values (24-bit, sign-extended to 32-bit) */
	int32_t accel_x;
	int32_t accel_y;
	int32_t accel_z;
	int32_t gyro_x;
	int32_t gyro_y;
	int32_t gyro_z;
	int16_t temp;
	uint16_t data_cntr;
	bool data_cntr_valid;

#ifdef CONFIG_ADIS16607_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;
	struct k_spinlock lock;
	sensor_trigger_handler_t drdy_handler;
	const struct sensor_trigger *drdy_trigger;

#if defined(CONFIG_ADIS16607_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ADIS16607_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_ADIS16607_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif /* CONFIG_ADIS16607_TRIGGER */
};

#ifdef CONFIG_ADIS16607_TRIGGER
int adis16607_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler);
int adis16607_init_interrupt(const struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_ADI_ADIS16607_H_ */
