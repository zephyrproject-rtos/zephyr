/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_TI_LDC1X1X_LDC1X1X_H_
#define ZEPHYR_DRIVERS_SENSOR_TI_LDC1X1X_LDC1X1X_H_

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>

#define LDC1X1X_DATA0           0x00
#define LDC1X1X_DATA1           0x02
#define LDC1X1X_DATA2           0x04
#define LDC1X1X_DATA3           0x06
#define LDC1X1X_RCOUNT0         0x08
#define LDC1X1X_RCOUNT1         0x09
#define LDC1X1X_RCOUNT2         0x0A
#define LDC1X1X_RCOUNT3         0x0B
#define LDC1X1X_OFFSET0         0x0C
#define LDC1X1X_OFFSET1         0x0D
#define LDC1X1X_OFFSET2         0x0E
#define LDC1X1X_OFFSET3         0x0F
#define LDC1X1X_SETTLECOUNT0    0x10
#define LDC1X1X_SETTLECOUNT1    0x11
#define LDC1X1X_SETTLECOUNT2    0x12
#define LDC1X1X_SETTLECOUNT3    0x13
#define LDC1X1X_CLOCK_DIVIDERS0 0x14
#define LDC1X1X_CLOCK_DIVIDERS1 0x15
#define LDC1X1X_CLOCK_DIVIDERS2 0x16
#define LDC1X1X_CLOCK_DIVIDERS3 0x17
#define LDC1X1X_STATUS          0x18
#define LDC1X1X_ERROR_CONFIG    0x19
#define LDC1X1X_CONFIG          0x1A
#define LDC1X1X_MUX_CONFIG      0x1B
#define LDC1X1X_RESET_DEV       0x1C
#define LDC1X1X_DRIVE_CURRENT0  0x1E
#define LDC1X1X_DRIVE_CURRENT1  0x1F
#define LDC1X1X_DRIVE_CURRENT2  0x20
#define LDC1X1X_DRIVE_CURRENT3  0x21
#define LDC1X1X_MANUFACTURER_ID 0x7E
#define LDC1X1X_DEVICE_ID       0x7F

#define LDC1X1X_MANUFACTURER_ID_VAL 0x5449
#define LDC1X1X_DEVICE_ID_LDC131X   0x3054
#define LDC1X1X_DEVICE_ID_LDC161X   0x3055

#define LDC1X1X_MAX_CHANNELS 4

#define LDC1X1X_DATA_ERR_UR BIT(15)
#define LDC1X1X_DATA_ERR_OR BIT(14)
#define LDC1X1X_DATA_ERR_WD BIT(13)
#define LDC1X1X_DATA_ERR_AE BIT(12)
#define LDC1X1X_DATA_ERR_MASK                                                                      \
	(LDC1X1X_DATA_ERR_UR | LDC1X1X_DATA_ERR_OR | LDC1X1X_DATA_ERR_WD | LDC1X1X_DATA_ERR_AE)

/* A conversion that runs to the end of its counter rather than tracking the
 * sensor, which is what an open or dead LC tank looks like.
 */
#define LDC1X1X_DATA_FULL_SCALE_161X GENMASK(27, 0)
#define LDC1X1X_DATA_FULL_SCALE_131X GENMASK(11, 0)

#define LDC1X1X_CLK_DIV_FIN_DIVIDER_MASK    GENMASK(15, 12)
#define LDC1X1X_CLK_DIV_FIN_DIVIDER_SET(x)  (((x) & 0xF) << 12)
#define LDC1X1X_CLK_DIV_FREF_DIVIDER_MASK   GENMASK(9, 0)
#define LDC1X1X_CLK_DIV_FREF_DIVIDER_SET(x) ((x) & 0x3FF)

#define LDC1X1X_STATUS_DRDY BIT(6)
/* Channel 0 is reported in bit 3 and channel 3 in bit 0. */
#define LDC1X1X_STATUS_UNREADCONV(ch) BIT(3U - (ch))

/*
 * Under-range, over-range, watchdog and amplitude high are reported into
 * DATAx_MSB. Amplitude low shares the same ERR_AE flag as amplitude high, and
 * is left out: a high amplitude means the ESD clamp has shifted the sensor
 * frequency and the result is meaningless, while a low one only costs signal
 * to noise ratio and still converts.
 */
#define LDC1X1X_ERROR_CONFIG_ERR2OUT_MASK     GENMASK(15, 11)
#define LDC1X1X_ERROR_CONFIG_REPORTED         GENMASK(15, 12)
#define LDC1X1X_ERROR_CONFIG_DRDY_2INT_MASK   BIT(0)
#define LDC1X1X_ERROR_CONFIG_DRDY_2INT_SET(x) ((x) & 0x1)

#define LDC1X1X_CONFIG_ACTIVE_CHAN_MASK           GENMASK(15, 14)
#define LDC1X1X_CONFIG_ACTIVE_CHAN_SET(x)         (((x) & 0x3) << 14)
#define LDC1X1X_CONFIG_SLEEP_SET_EN_MASK          BIT(13)
#define LDC1X1X_CONFIG_SLEEP_SET_EN_SET(x)        (((x) & 0x1) << 13)
#define LDC1X1X_CONFIG_RP_OVERRIDE_EN_MASK        BIT(12)
#define LDC1X1X_CONFIG_RP_OVERRIDE_EN_SET(x)      (((x) & 0x1) << 12)
#define LDC1X1X_CONFIG_SENSOR_ACTIVATE_SEL_MASK   BIT(11)
#define LDC1X1X_CONFIG_SENSOR_ACTIVATE_SEL_SET(x) (((x) & 0x1) << 11)
#define LDC1X1X_CONFIG_AUTO_AMP_DIS_MASK          BIT(10)
#define LDC1X1X_CONFIG_AUTO_AMP_DIS_SET(x)        (((x) & 0x1) << 10)
#define LDC1X1X_CONFIG_REF_CLK_SRC_MASK           BIT(9)
#define LDC1X1X_CONFIG_REF_CLK_SRC_SET(x)         (((x) & 0x1) << 9)
#define LDC1X1X_CONFIG_INTB_DIS_MASK              BIT(7)
#define LDC1X1X_CONFIG_INTB_DIS_SET(x)            (((x) & 0x1) << 7)
#define LDC1X1X_CONFIG_HIGH_CURRENT_DRV_MASK      BIT(6)
#define LDC1X1X_CONFIG_HIGH_CURRENT_DRV_SET(x)    (((x) & 0x1) << 6)

#define LDC1X1X_MUX_CONFIG_AUTOSCAN_EN_MASK   BIT(15)
#define LDC1X1X_MUX_CONFIG_AUTOSCAN_EN_SET(x) (((x) & 0x1) << 15)
#define LDC1X1X_MUX_CONFIG_RR_SEQUENCE_MASK   GENMASK(14, 13)
#define LDC1X1X_MUX_CONFIG_RR_SEQUENCE_SET(x) (((x) & 0x3) << 13)
#define LDC1X1X_MUX_CONFIG_DEGLITCH_MASK      GENMASK(2, 0)
#define LDC1X1X_MUX_CONFIG_DEGLITCH_SET(x)    ((x) & 0x7)

#define LDC1X1X_RESET_DEV_MASK               BIT(15)
#define LDC1X1X_RESET_DEV_SET(x)             (((x) & 0x1) << 15)
#define LDC1X1X_RESET_DEV_OUTPUT_GAIN_MASK   GENMASK(10, 9)
#define LDC1X1X_RESET_DEV_OUTPUT_GAIN_SET(x) (((x) & 0x3) << 9)

#define LDC1X1X_DRIVE_CURRENT_IDRIVE_MASK   GENMASK(15, 11)
#define LDC1X1X_DRIVE_CURRENT_IDRIVE_SET(x) (((x) & 0x1F) << 11)

enum ldc1x1x_op_mode {
	LDC1X1X_ACTIVE_MODE,
	LDC1X1X_SLEEP_MODE,
};

struct ldc1x1x_data {
	bool ldc161x;
	uint32_t *sample_buf;
	uint8_t ch_err;

#ifdef CONFIG_LDC1X1X_TRIGGER
	struct gpio_callback gpio_cb;
	struct k_mutex trigger_mutex;
	sensor_trigger_handler_t drdy_handler;
	struct sensor_trigger drdy_trigger;
	const struct device *dev;
	uint16_t int_config;

#ifdef CONFIG_LDC1X1X_TRIGGER_OWN_THREAD
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_LDC1X1X_THREAD_STACK_SIZE);
	struct k_sem gpio_sem;
	struct k_thread thread;
#elif defined(CONFIG_LDC1X1X_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif
};

struct ldc1x1x_channel_config {
	uint16_t rcount;
	uint16_t offset;
	uint16_t settle_count;
	uint16_t fref_divider;
	uint8_t idrive;
	uint8_t fin_divider;
};

struct ldc1x1x_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec sd_gpio;

#ifdef CONFIG_LDC1X1X_TRIGGER
	struct gpio_dt_spec intb_gpio;
#endif

	bool ldc1x14;
	bool autoscan;
	uint8_t rr_sequence;
	uint8_t active_channel;
	uint8_t output_gain;
	uint8_t deglitch;
	bool rp_override;
	bool auto_amplitude_dis;
	uint8_t sensor_activate_sel;
	uint8_t ref_clk_src;
	uint8_t current_drive;
	/* Bit N is set when channel N has a devicetree node. */
	uint8_t channel_mask;
	uint16_t fref;
	const struct ldc1x1x_channel_config *ch_cfg;
};

int ldc1x1x_get_status(const struct device *dev, uint16_t *status);
int ldc1x1x_reg_write_mask(const struct device *dev, uint8_t reg_addr, uint16_t mask,
			   uint16_t data);
int ldc1x1x_set_interrupt_pin(const struct device *dev, bool enable);

#ifdef CONFIG_LDC1X1X_TRIGGER
int ldc1x1x_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);
int ldc1x1x_init_interrupt(const struct device *dev);
#endif

#endif
