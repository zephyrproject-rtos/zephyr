/* Kionix KX022 3-axis accelerometer driver
 *
 * Copyright (c) 2021 G-Technologies Sdn. Bhd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_KX022_KX022_H_
#define ZEPHYR_DRIVERS_SENSOR_KX022_KX022_H_

#include <zephyr/types.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <drivers/sensor/kx022.h>

#define KX022_RESET_DELAY 2000

#define KX022_REG_XHP_L 0x0
#define KX022_REG_XHP_H 0x1
#define KX022_REG_YHP_L 0x2
#define KX022_REG_YHP_H 0x3
#define KX022_REG_ZHP_L 0x4
#define KX022_REG_ZHP_H 0x5

#define KX022_REG_XOUT_L 0x06
#define KX022_REG_XOUT_H 0x07
#define KX022_REG_YOUT_L 0x08
#define KX022_REG_YOUT_H 0x09
#define KX022_REG_ZOUT_L 0x0A
#define KX022_REG_ZOUT_H 0x0B

#define KX022_REG_COTR 0x0C
#define KX022_REG_COTR_RESET 0x55

#define KX022_REG_WHO_AM_I 0x0F
#define KX022_VAL_WHO_AM_I 0x14

#define KX022_REG_TSCP 0x10
#define KX022_VAL_TSCP_RESET 0x20
#define KX022_MASK_TSCP_LE BIT(5)
#define KX022_MASK_TSCP_RI BIT(4)
#define KX022_MASK_TSCP_DO BIT(3)
#define KX022_MASK_TSCP_UP BIT(2)
#define KX022_MASK_TSCP_FD BIT(1)
#define KX022_MASK_TSCP_FU BIT(0)

#define KX022_REG_TSPP 0x11
#define KX022_VAL_TSPP_RESET 0x20
#define KX022_MASK_TSPP_LE BIT(5)
#define KX022_MASK_TSPP_RI BIT(4)
#define KX022_MASK_TSPP_DO BIT(3)
#define KX022_MASK_TSPP_UP BIT(2)
#define KX022_MASK_TSPP_FD BIT(1)
#define KX022_MASK_TSPP_FU BIT(0)

#define KX022_NEG_X_MASK 0x20

#define KX022_REG_INS1 0x12
#define KX022_MASK_INS1_TLE BIT(5)
#define KX022_MASK_INS1_TRI BIT(4)
#define KX022_MASK_INS1_TDO BIT(3)
#define KX022_MASK_INS1_TUP BIT(2)
#define KX022_MASK_INS1_TFD BIT(1)
#define KX022_MASK_INS1_TFU BIT(0)

#define KX022_REG_INS2 0x13
#define KX022_MASK_INS2_BFI BIT(6)
#define KX022_MASK_INS2_WMI BIT(5)
#define KX022_MASK_INS2_DRDY BIT(4)
#define KX022_MASK_INS2_TDTS (BIT(3) | BIT(2))
#define KX022_MASK_INS2_WUFS BIT(1)
#define KX022_MASK_INS2_TPS BIT(0)

#define KX022_REG_INS3 0x14
#define KX022_MASK_INS3_XNWU BIT(5)
#define KX022_MASK_INS3_XPWU BIT(4)
#define KX022_MASK_INS3_YNWU BIT(3)
#define KX022_MASK_INS3_YPWU BIT(2)
#define KX022_MASK_INS3_ZNWU BIT(1)
#define KX022_MASK_INS3_ZPWU BIT(0)

#define KX022_REG_STATUS_REG 0x15
#define KX022_MASK_STATUS_REG BIT(4)

#define KX022_REG_INT_REL 0x17

#define KX022_REG_CNTL1 0x18
#define KX022_VAL_CNTL1_RESET 0x0
#define KX022_MASK_CNTL1_PC1 BIT(7)
#define KX022_MASK_CNTL1_RES BIT(6)
#define KX022_MASK_CNTL1_DRDYE BIT(5)
#define KX022_MASK_CNTL1_GSEL (BIT(4) | BIT(3))
#define KX022_MASK_CNTL1_TDTE BIT(2)
#define KX022_MASK_CNTL1_WUFE BIT(1)
#define KX022_MASK_CNTL1_TPE BIT(0)
#define KX022_MASK_CNTL_POWER_MODE (BIT(7) | BIT(1))
#define KX022_MASK_CNTL_INT_TYPE_EN (BIT(2) | BIT(1) | BIT(0))

#define KX022_DEFAULT_CNTL1 0x40
#define KX022_CNTL1_DRDYE (0x01 << 5)
#define KX022_CNTL1_WUFE (0x01 << 1)
#define KX022_CNTL1_TPE_EN 1
#define KX022_STANDY_MODE 0
#define KX022_OPERATING_MODE 1
#define KX022_CNTL1_WUFE_RESET 0
#define KX022_CNTL1_TPE_RESET 0

#define KX022_CNTL1_TDTE_SHIFT 2
#define KX022_CNTL1_GSEL_SHIFT 3
#define KX022_CNTL1_DRDYE_SHIFT 5
#define KX022_CNTL1_PC1_SHIFT 7
#define KX022_CNTL1_RES_SHIFT 6

#define KX022_REG_CNTL2 0x19
#define KX022_VAL_CNTL2_RESET 0x3F
#define KX022_MASK_CNTL2_SRST BIT(7)
#define KX022_MASK_CNTL2_COTC BIT(6)
#define KX022_MASK_CNTL2_LEM BIT(5)
#define KX022_MASK_CNTL2_RIM BIT(4)
#define KX022_MASK_CNTL2_DOM BIT(3)
#define KX022_MASK_CNTL2_UPM BIT(2)
#define KX022_MASK_CNTL2_FDM BIT(1)
#define KX022_MASK_CNTL2_FUM BIT(0)
#define KX022_CNTL_TILT_ALL_EN 0x3F

#define KX022_REG_CNTL3 0x1A
#define KX022_DEFAULT_CNTL3_50HZ 0xD6
#define KX022_VAL_CNTL3_RESET 0x98
#define KX022_MASK_CNTL3_OTP (BIT(7) | BIT(6))
#define KX022_MASK_CNTL3_OTDT (BIT(5) | BIT(4) | BIT(3))
#define KX022_MASK_CNTL3_OWUF (BIT(2) | BIT(1) | BIT(0))
#define KX022_CNTL3_OTP_SHIFT 6

#define KX022_REG_ODCNTL 0x1B
#define KX022_VAL_ODCNTL_RESET 0x02
#define KX022_MASK_ODCNTL_IIR_BYPASS BIT(7)
#define KX022_MASK_ODCNTL_LPRO BIT(6)
#define KX022_MASK_ODCNTL_OSA (BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define KX022_ODCNTL_50HZ 2

#define KX022_REG_INC1 0x1C
#define KX022_VAL_INC1_RESET 0x10
#define KX022_MASK_INC1_IEN1 BIT(5)
#define KX022_MASK_INC1_IEA1 BIT(4)
#define KX022_MASK_INC1_IEL1 BIT(3)
#define KX022_MASK_INC1_STPOL BIT(1)
#define KX022_MASK_INC1_SPI3E BIT(0)
#define KX022_MASK_INC1_INT_EN KX022_MASK_INC1_IEN1 | KX022_MASK_INC1_IEA1 | KX022_MASK_INC1_IEL1
#define KX022_INT1_EN (6 << 3)
#define KX022_INC1_IEA1_SHIFT 4
#define KX022_INC1_IEL1_SHIFT 3

#define KX022_REG_INC2 0x1D
#define KX022_VAL_INC2_RESET 0x3F
#define KX022_MASK_INC2_XNWUE BIT(5)
#define KX022_MASK_INC2_XPWUE BIT(4)
#define KX022_MASK_INC2_YNWUE BIT(3)
#define KX022_MASK_INC2_YPWUE BIT(2)
#define KX022_MASK_INC2_ZNWUE BIT(1)
#define KX022_MASK_INC2_ZPWUE BIT(0)
#define KX022_DEFAULT_INC2 0x3F

#define KX022_REG_INC3 0x1E
#define KX022_VAL_INC3_RESET 0x3F
#define KX022_MASK_INC3_TLEM BIT(5)
#define KX022_MASK_INC3_TRIM BIT(4)
#define KX022_MASK_INC3_TDOM BIT(3)
#define KX022_MASK_INC3_TUPM BIT(2)
#define KX022_MASK_INC3_TFDM BIT(1)
#define KX022_MASK_INC3_TFUM BIT(0)

#define KX022_REG_INC4 0x1F
#define KX022_VAL_INC4_RESET 0x0
#define KX022_MASK_INC4_BFI1 BIT(6)
#define KX022_MASK_INC4_WMI1 BIT(5)
#define KX022_MASK_INC4_DRDYI1 BIT(4)
#define KX022_MASK_INC4_TDTI1 BIT(2)
#define KX022_MASK_INC4_WUFI1 BIT(1)
#define KX022_MASK_INC4_TPI1 BIT(0)
// Wake-Up (motion detect) interrupt reported on physical interrupt pin INT1
#define KX022_INC4_WUFI1_SET (0x01 << 1)
#define KX022_INC4_WUFI1_RESET (0x00)
// Data ready interrupt reported on physical interrupt pin INT1
#define KX022_INC4_DRDYI1 (0x01 << 4)
#define KX022_INC4_TPI1_SET (0x01)
#define KX022_INC4_TPI1_RESET (0x00)

#define KX022_REG_INC5 0x20
#define KX022_VAL_INC5_RESET 0x10
#define KX022_MASK_INC5_IEN2 BIT(5)
#define KX022_MASK_INC5_IEA2 BIT(4)
#define KX022_MASK_INC5_IEL2 BIT(3)
#define KX022_MASK_INT2_EN KX022_MASK_INC5_IEN2 | KX022_MASK_INC5_IEA2 | KX022_MASK_INC5_IEL2
#define KX022_INT2_EN (6 << 3)
#define KX022_INC5_IEA2_SHIFT 4
#define KX022_INC5_IEL2_SHIFT 3

#define KX022_REG_INC6 0x21
#define KX022_VAL_INC6_RESET 0x0
#define KX022_MASK_INC6_BFI2 BIT(6)
#define KX022_MASK_INC6_WMI2 BIT(5)
#define KX022_MASK_INC6_DRDYI2 BIT(4)
#define KX022_MASK_INC6_TDTI2 BIT(2)
#define KX022_MASK_INC6_WUFI2 BIT(1)
#define KX022_MASK_INC6_TPI2 BIT(0)
#define KX022_INC6_TPI2_SET (0x01)
#define KX022_INC6_TPI2_RESET (0x00)
#define KX022_INC6_WUFI2_SET (0x01 << 1)
#define KX022_INC6_WUFI2_RESET (0x00)
// DRDYI2  Data ready interrupt reported on physical interrupt pin INT2
#define KX022_INC6_DRDYI2 (0x01 << 4)

#define KX022_REG_TILT_TIMER 0x22
#define KX022_VAL_TILT_TIMER_RESET 0x0
#define KX022_MASK_TILT_TIMER_TSC 0xFF

#define KX022_REG_WUFC 0x23
#define KX022_VAL_WUFC_RESET 0x0
#define KX022_MASK_WUFC_TSC 0xFF

#define KX022_REG_TDTRC 0x24
#define KX022_VAL_TDTRC_RESET 0x03
#define KX022_MASK_TDTRC_DTRE BIT(1)
#define KX022_MASK_TDTRC_STRE BIT(0)

#define KX022_REG_TDTC 0x25
#define KX022_VAL_TDTC_RESET 0x78
#define KX022_MASK_TDTC_TDTC 0xFF

#define KX022_REG_TTH 0x26
#define KX022_VAL_TTH_RESET 0xCB
#define KX022_MASK_TTH_TTH 0xFF

#define KX022_REG_TTL 0x27
#define KX022_VAL_TTL_RESET 0x1A
#define KX022_MASK_TTL_TTL 0xFF

#define KX022_REG_FTD 0x28
#define KX022_VAL_FTD_RESET 0xA2
#define KX022_MASK_FTD_FTDH (BIT(7) | BIT(6) | BIT(5) | BIT(4) | BIT(3))
#define KX022_MASK_FTD_FTDL (BIT(2) | BIT(1) | BIT(0))

#define KX022_REG_STD 0x29
#define KX022_VAL_STD_RESET 0x24
#define KX022_MASK_STD_STD 0xFF

#define KX022_REG_TLT 0x2A
#define KX022_VAL_TLT_RESET 0x28
#define KX022_MASK_TLT_TLT 0xFF

#define KX022_REG_TWS 0x2B
#define KX022_VAL_TWS_RESET 0xA0
#define KX022_MASK_TWS_TWS 0xFF

#define KX022_REG_ATH 0x30
#define KX022_VAL_ATH_RESET 0x08
#define KX022_MASK_ATH_ATH 0xFF

#define KX022_REG_TILT_ANGLE_LL 0x32
#define KX022_VAL_LL_RESET 0x0C
#define KX022_MASK_LL_LL 0xFF

#define KX022_REG_TILT_ANGLE_HL 0x33
#define KX022_VAL_HL_RESET 0x2A
#define KX022_MASK_HL_HL 0xFF

#define KX022_REG_HYST_SET 0x34
#define KX022_VAL_HYST_SET_RESET 0x14
#define KX022_MASK_HYST_SET_RES (BIT(7) | BIT(6))
#define KX022_MASK_HYST_SET_HYST (BIT(5) | BIT(4) | BIT(3) | BIT(2) | BIT(1) | BIT(0))

#define KX022_REG_LP_CNTL 0x35
#define KX022_VAL_LP_CNTL_RESET 0x4B
#define KX022_MASK_LP_CNTL_AVC (BIT(6) | BIT(5) | BIT(4))

#define KX022_REG_BUF_CNTL1 0x3A
#define KX022_VAL_BUF_CNTL1_RESET 0x0
#define KX022_MASK_BUF_CNTL1_SMP_TH (BIT(6) | BIT(5) | BIT(4) | BIT(3) | BIT(2) | BIT(1) | BIT(0))

#define KX022_REG_BUF_CNTL2 0x3B
#define KX022_VAL_BUF_CNTL2_RESET 0x0
#define KX022_MASK_BUF_CNTL2_BUFE BIT(7)
#define KX022_MASK_BUF_CNTL2_BRES BIT(6)
#define KX022_MASK_BUF_CNTL2_BFIE BIT(5)
#define KX022_MASK_BUF_CNTL2_BUF_M (BIT(1) | BIT(0))

#define KX022_REG_BUF_STATUS_1 0x3C
#define KX022_MASK_BUF_STATUS_1_SMP_LEV 0xFF

#define KX022_REG_BUF_STATUS_2 0x3D
#define KX022_REG_BUF_STATUS_2_BUF_TRIG BIT(7)

#define KX022_REG_BUF_CLEAR 0x3E

#define KX022_REG_BUF_READ 0x3F

#define KX022_REG_SELF_TEST 0x60
#define KX022_REG_SELF_TEST_ENABLE 0xCA
#define KX022_REG_SELF_TEST_DISABLE 0x0

#define KX022_STNDBY_MODE_MOTION 0x42

#define KX022_FS_2G 0x0
#define KX022_FS_4G 0x1
#define KX022_FS_8G 0x2
#define KX022_ODR_RANGE_MAX 0x07
#define KX022_FS_RANGE_MAX 0x03
#define KX022_RES_RANGE_MAX 0x01
#define KX022_ATH_RANGE_MAX 0xFF
#define KX022_WUFC_RANGE_MAX 0xFF
#define KX022_TILT_ANGLE_LL_RANGE_MAX 0xFF
#define KX022_TILT_TIMER_RANGE_MAX 0xFF
#define KX022_MOTION_THS_RANGE_MAX 0xFF
#define KX022_ATH_RANGE_MIN 0
#define KX022_TILT_ANGLE_LL_MIN 0

#define BITWISE_SHIFT_7 7
#define BITWISE_SHIFT_6 6
#define BITWISE_SHIFT_5 5
#define BITWISE_SHIFT_4 4
#define BITWISE_SHIFT_3 3
#define BITWISE_SHIFT_2 2
#define BITWISE_SHIFT_1 1

/* Accel sensor sensitivity unit is 0.061 mg/LSB */
#define GAIN_XL (6103515625LL / 1000000000000.0)

struct kx022_config {
	int (*bus_init)(const struct device *dev);
	struct i2c_dt_spec bus_cfg;
    uint8_t int_pin_1_polarity;
    uint8_t int_pin_1_response;
    uint8_t full_scale;
    uint8_t odr;
	uint8_t resolution;
    uint8_t motion_odr;
    uint8_t motion_threshold;
    uint8_t motion_detection_timer;
    uint8_t tilt_odr;
    uint8_t tilt_timer;
    uint8_t tilt_angle_ll;
    uint8_t tilt_angle_hl;
#ifdef CONFIG_KX022_TRIGGER
	const char *irq_port;
	gpio_pin_t irq_pin;
	gpio_dt_flags_t irq_flags;
#endif
};

struct kx022_data;

struct kx022_transfer_function {
	int (*read_data)(const struct device *dev, uint8_t reg_addr, uint8_t *value, uint8_t len);
	int (*write_data)(const struct device *dev, uint8_t reg_addr, uint8_t *value, uint8_t len);
	int (*read_reg)(const struct device *dev, uint8_t reg_addr, uint8_t *value);
	int (*write_reg)(const struct device *dev, uint8_t reg_addr, uint8_t value);
	int (*update_reg)(const struct device *dev, uint8_t reg_addr, uint8_t mask, uint8_t value);
};

struct kx022_data {
	int sample_x;
	int sample_y;
	int sample_z;
	uint8_t sample_tspp;
	uint8_t sample_tscp;
	uint8_t sample_motion_dir;
	float gain;
	const struct kx022_transfer_function *hw_tf;

#ifdef CONFIG_KX022_TRIGGER
	const struct device *gpio;
	struct gpio_callback gpio_cb;

	struct sensor_trigger motion_trigger;
	sensor_trigger_handler_t motion_handler;

	struct sensor_trigger tilt_trigger;
	sensor_trigger_handler_t tilt_handler;
	const struct device *dev;

#if defined(CONFIG_KX022_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_KX022_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem trig_sem;
#elif defined(CONFIG_KX022_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif

#endif /* CONFIG_KX022_TRIGGER */
};

int kx022_i2c_init(const struct device *dev);

#ifdef CONFIG_KX022_TRIGGER
int kx022_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
		      sensor_trigger_handler_t handler);

int kx022_trigger_init(const struct device *dev);
int kx022_mode(const struct device *dev, bool mode);

#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_KX022_KX022_H_ */
