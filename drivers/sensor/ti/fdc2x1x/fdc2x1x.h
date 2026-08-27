/*
 * Copyright (c) 2020 arithmetics.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_FDC2X1X_FDC2X1X_H_
#define ZEPHYR_DRIVERS_SENSOR_FDC2X1X_FDC2X1X_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>

#define PI              (3.14159265)

/*
 * FDC2X1X registers definition
 */
#define FDC2X1X_DATA_CH0                0x00
#define FDC2X1X_DATA_LSB_CH0            0x01
#define FDC2X1X_DATA_CH1                0x02
#define FDC2X1X_DATA_LSB_CH1            0x03
#define FDC2X1X_DATA_CH2                0x04
#define FDC2X1X_DATA_LSB_CH2            0x05
#define FDC2X1X_DATA_CH3                0x06
#define FDC2X1X_DATA_LSB_CH3            0x07
#define FDC2X1X_RCOUNT_CH0              0x08
#define FDC2X1X_RCOUNT_CH1              0x09
#define FDC2X1X_RCOUNT_CH2              0x0A
#define FDC2X1X_RCOUNT_CH3              0x0B
#define FDC2X1X_OFFSET_CH0              0x0C
#define FDC2X1X_OFFSET_CH1              0x0D
#define FDC2X1X_OFFSET_CH2              0x0E
#define FDC2X1X_OFFSET_CH3              0x0F
#define FDC2X1X_SETTLECOUNT_CH0         0x10
#define FDC2X1X_SETTLECOUNT_CH1         0x11
#define FDC2X1X_SETTLECOUNT_CH2         0x12
#define FDC2X1X_SETTLECOUNT_CH3         0x13
#define FDC2X1X_CLOCK_DIVIDERS_CH0      0x14
#define FDC2X1X_CLOCK_DIVIDERS_CH1      0x15
#define FDC2X1X_CLOCK_DIVIDERS_CH2      0x16
#define FDC2X1X_CLOCK_DIVIDERS_CH3      0x17
#define FDC2X1X_STATUS                  0x18
#define FDC2X1X_ERROR_CONFIG            0x19
#define FDC2X1X_CONFIG                  0x1A
#define FDC2X1X_MUX_CONFIG              0x1B
#define FDC2X1X_RESET_DEV               0x1C
#define FDC2X1X_DRIVE_CURRENT_CH0       0x1E
#define FDC2X1X_DRIVE_CURRENT_CH1       0x1F
#define FDC2X1X_DRIVE_CURRENT_CH2       0x20
#define FDC2X1X_DRIVE_CURRENT_CH3       0x21
#define FDC2X1X_MANUFACTURER_ID         0x7E
#define FDC2X1X_DEVICE_ID               0x7F

#define FDC2X1X_MANUFACTURER_ID_VAL     0x5449

#define FDC2X1X_DEVICE_ID_VAL_28BIT     0x3055
#define FDC2X1X_DEVICE_ID_VAL           0x3054

#define FDC2X1X_READ                    0x01u
#define FDC2X1X_REG_READ(x)             (((x & 0xFF) << 1) | FDC2X1X_READ)
#define FDC2X1X_REG_WRITE(x)            ((x & 0xFF) << 1)
#define FDC2X1X_TO_I2C_REG(x)           ((x) >> 1)

/* CLOCK_DIVIDERS_CHX Field Descriptions */
#define FDC2X1X_CLK_DIV_CHX_FIN_SEL_MSK             GENMASK(13, 12)
#define FDC2X1X_CLK_DIV_CHX_FIN_SEL_SET(x)          (((x) & 0x3) << 12)
#define FDC2X1X_CLK_DIV_CHX_FIN_SEL_GET(x)          (((x) >> 12) & 0x3)
#define FDC2X1X_CLK_DIV_CHX_FREF_DIV_MSK            GENMASK(9, 0)
#define FDC2X1X_CLK_DIV_CHX_FREF_DIV_SET(x)         ((x) & 0x1FF)
#define FDC2X1X_CLK_DIV_CHX_FREF_DIV_GET(x)         (((x) >> 0) & 0x1FF)

/* STATUS Field Descriptions */
#define FDC2X1X_STATUS_ERR_CHAN(x)                  (((x) >> 14) & 0x3)
#define FDC2X1X_STATUS_ERR_WD(x)                    (((x) >> 11) & 0x1)
#define FDC2X1X_STATUS_ERR_AHW(x)                   (((x) >> 10) & 0x1)
#define FDC2X1X_STATUS_ERR_ALW(x)                   (((x) >> 9) & 0x1)
#define FDC2X1X_STATUS_DRDY(x)                      (((x) >> 6) & 0x1)
#define FDC2X1X_STATUS_CH0_UNREADCONV_RDY(x)        (((x) >> 3) & 0x1)
#define FDC2X1X_STATUS_CH1_UNREADCONV_RDY(x)        (((x) >> 2) & 0x1)
#define FDC2X1X_STATUS_CH2_UNREADCONV_RDY(x)        (((x) >> 1) & 0x1)
#define FDC2X1X_STATUS_CH3_UNREADCONV_RDY(x)        (((x) >> 0) & 0x1)

/* ERROR_CONFIG */
#define FDC2X1X_ERROR_CONFIG_WD_ERR2OUT_MSK         BIT(13)
#define FDC2X1X_ERROR_CONFIG_WD_ERR2OUT_SET(x)      (((x) & 0x1) << 13)
#define FDC2X1X_ERROR_CONFIG_WD_ERR2OUT_GET(x)      (((x) >> 13) & 0x1)
#define FDC2X1X_ERROR_CONFIG_AH_WARN2OUT_MSK        BIT(12)
#define FDC2X1X_ERROR_CONFIG_AH_WARN2OUT_SET(x)     (((x) & 0x1) << 12)
#define FDC2X1X_ERROR_CONFIG_AH_WARN2OUT_GET(x)     (((x) >> 12) & 0x1)
#define FDC2X1X_ERROR_CONFIG_AL_WARN2OUT_MSK        BIT(11)
#define FDC2X1X_ERROR_CONFIG_AL_WARN2OUT_SET(x)     (((x) & 0x1) << 11)
#define FDC2X1X_ERROR_CONFIG_AL_WARN2OUT_GET(x)     (((x) >> 11) & 0x1)
#define FDC2X1X_ERROR_CONFIG_WD_ERR2INT_MSK         BIT(5)
#define FDC2X1X_ERROR_CONFIG_WD_ERR2INT_SET(x)      (((x) & 0x1) << 5)
#define FDC2X1X_ERROR_CONFIG_WD_ERR2INT_GET(x)      (((x) >> 5) & 0x1)
#define FDC2X1X_ERROR_CONFIG_DRDY_2INT_MSK          BIT(0)
#define FDC2X1X_ERROR_CONFIG_DRDY_2INT_SET(x)       (((x) & 0x1) << 0)
#define FDC2X1X_ERROR_CONFIG_DRDY_2INT_GET(x)       (((x) >> 0) & 0x1)

/* CONFIG Field Descriptions */
#define FDC2X1X_CFG_ACTIVE_CHAN_MSK                 GENMASK(15, 14)
#define FDC2X1X_CFG_ACTIVE_CHAN_SET(x)              (((x) & 0x3) << 14)
#define FDC2X1X_CFG_ACTIVE_CHAN_GET(x)              (((x) >> 14) & 0x3)
#define FDC2X1X_CFG_SLEEP_SET_EN_MSK                BIT(13)
#define FDC2X1X_CFG_SLEEP_SET_EN_SET(x)             (((x) & 0x1) << 13)
#define FDC2X1X_CFG_SLEEP_SET_EN_GET(x)             (((x) >> 13) & 0x1)
#define FDC2X1X_CFG_SENSOR_ACTIVATE_SEL_MSK         BIT(11)
#define FDC2X1X_CFG_SENSOR_ACTIVATE_SEL_SET(x)      (((x) & 0x1) << 11)
#define FDC2X1X_CFG_SENSOR_ACTIVATE_SEL_GET(x)      (((x) >> 11) & 0x1)
#define FDC2X1X_CFG_REF_CLK_SRC_MSK                 BIT(9)
#define FDC2X1X_CFG_REF_CLK_SRC_SET(x)              (((x) & 0x1) << 9)
#define FDC2X1X_CFG_REF_CLK_SRC_GET(x)              (((x) >> 9) & 0x1)
#define FDC2X1X_CFG_INTB_DIS_MSK                    BIT(7)
#define FDC2X1X_CFG_INTB_DIS_SET(x)                 (((x) & 0x1) << 7)
#define FDC2X1X_CFG_INTB_DIS_GET(x)                 (((x) >> 7) & 0x1)
#define FDC2X1X_CFG_HIGH_CURRENT_DRV_MSK            BIT(6)
#define FDC2X1X_CFG_HIGH_CURRENT_DRV_SET(x)         (((x) & 0x1) << 6)
#define FDC2X1X_CFG_HIGH_CURRENT_DRV_GET(x)         (((x) >> 6) & 0x1)

/* MUX_CONFIG Field Descriptions */
#define FDC2X1X_MUX_CFG_AUTOSCAN_EN_MSK             BIT(15)
#define FDC2X1X_MUX_CFG_AUTOSCAN_EN_SET(x)          (((x) & 0x1) << 15)
#define FDC2X1X_MUX_CFG_AUTOSCAN_EN_GET(x)          (((x) >> 15) & 0x1)
#define FDC2X1X_MUX_CFG_RR_SEQUENCE_MSK             GENMASK(14, 13)
#define FDC2X1X_MUX_CFG_RR_SEQUENCE_SET(x)          (((x) & 0x3) << 13)
#define FDC2X1X_MUX_CFG_RR_SEQUENCE_GET(x)          (((x) >> 13) & 0x3)
#define FDC2X1X_MUX_CFG_DEGLITCH_MSK                GENMASK(2, 0)
#define FDC2X1X_MUX_CFG_DEGLITCH_SET(x)             ((x) & 0x7)
#define FDC2X1X_MUX_CFG_DEGLITCH_GET(x)             (((x) >> 0) & 0x7)

/* RESET_DEV Field Descriptions */
#define FDC2X1X_RESET_DEV_MSK                       BIT(15)
#define FDC2X1X_RESET_DEV_SET(x)                    (((x) & 0x1) << 15)
#define FDC2X1X_RESET_DEV_OUTPUT_GAIN_MSK           GENMASK(10, 9)
#define FDC2X1X_RESET_DEV_OUTPUT_GAIN_SET(x)        (((x) & 0x3) << 9)
#define FDC2X1X_RESET_DEV_OUTPUT_GAIN_GET(x)        (((x) >> 9) & 0x3)

/* DRIVE_CURRENT_CHX Field Descriptions */
#define FDC2X1X_DRV_CURRENT_CHX_IDRIVE_MSK          GENMASK(15, 11)
#define FDC2X1X_DRV_CURRENT_CHX_IDRIVE_SET(x)       (((x) & 0x1F) << 11)
#define FDC2X1X_DRV_CURRENT_CHX_IDRIVE_GET(x)       (((x) >> 11) & 0x1F)

enum fdc2x1x_op_mode {
	FDC2X1X_ACTIVE_MODE,
	FDC2X1X_SLEEP_MODE
};

struct fdc2x1x_data {
	bool fdc221x;

#ifdef CONFIG_FDC2X1X_TRIGGER
	struct gpio_callback gpio_cb;
	uint16_t int_config;

	struct k_mutex trigger_mutex;
	sensor_trigger_handler_t drdy_handler;
	const struct sensor_trigger *drdy_trigger;
	const struct device *dev;

#ifdef CONFIG_FDC2X1X_TRIGGER_OWN_THREAD
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_FDC2X1X_THREAD_STACK_SIZE);
	struct k_sem gpio_sem;
	struct k_thread thread;
#elif CONFIG_FDC2X1X_TRIGGER_GLOBAL_THREAD
	struct k_work work;
#endif
#endif /* CONFIG_FDC2X1X_TRIGGER */

	uint32_t *channel_buf;
};

struct fdc2x1x_chx_config {
	uint16_t rcount;
	uint16_t offset;
	uint16_t settle_count;
	uint16_t fref_divider;
	uint8_t idrive;
	uint8_t fin_sel;
	uint8_t inductance;
};

struct fdc2x1x_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec sd_gpio;

#ifdef CONFIG_FDC2X1X_TRIGGER
	struct gpio_dt_spec intb_gpio;
#endif

	bool fdc2x14;
	uint8_t num_channels;

	/* Device Settings */
	bool autoscan_en;
	uint8_t rr_sequence;
	uint8_t active_channel;
	uint8_t output_gain;
	uint8_t deglitch;
	uint8_t sensor_activate_sel;
	uint8_t clk_src;
	uint8_t current_drv;
	uint16_t fref;

	/* Channel Settings */
	const struct fdc2x1x_chx_config *ch_cfg;
};

int fdc2x1x_set_interrupt_pin(const struct device *dev, bool enable);
int fdc2x1x_get_status(const struct device *dev, uint16_t *status);
int fdc2x1x_reg_write_mask(const struct device *dev, uint8_t reg_addr,
			   uint16_t mask, uint16_t data);

#ifdef CONFIG_FDC2X1X_TRIGGER

int fdc2x1x_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int fdc2x1x_init_interrupt(const struct device *dev);
#endif  /* CONFIG_FDC2X1X_TRIGGER */

#endif  /* ZEPHYR_DRIVERS_SENSOR_FDC2X1X_FDC2X1X_H_ */
