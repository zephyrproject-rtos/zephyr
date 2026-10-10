/*
 * Copyright (c) 2026 Carl Zeiss Meditec AG
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_SENSOR_TSL2522_TSL2522_H_
#define ZEPHYR_DRIVERS_SENSOR_TSL2522_TSL2522_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/sensor/tsl2522.h>

#define TSL2522_MAX_INIT_RETRY             5U
#define TSL2522_DEVICE_ID                  0x5C
#define TSL2522_RETRY_ACCESS_US            100U
#define TSL2522_SOFTRESET_WAIT_US          500U
#define TSL2522_NUMBER_OF_SAMPLES_MIN      1U
#define TSL2522_NUMBER_OF_SAMPLES_MAX      2048U
#define TSL2522_MEASUREMENT_TIME_STEPS_MIN 1U
#define TSL2522_MEASUREMENT_TIME_STEPS_MAX 2048U
#define TSL2522_DEFAULT_ALS_SCALE          4U

#define TSL2522_REG_ENABLE     0x80
#define TSL2522_ENABLE_DISABLE 0U
#define TSL2522_ENABLE_FDEN    BIT(6)
#define TSL2522_ENABLE_AEN     BIT(1)
#define TSL2522_ENABLE_PON     BIT(0)

#define TSL2522_REG_MEAS_MODE                        0x81
#define TSL2522_MEAS_MODE_STOP_AFTER_NTH_ITERATION   BIT(7)
#define TSL2522_MEAS_MODE_EN_AGC_ASAT_DBL_STEP_DOWN  BIT(6)
#define TSL2522_MEAS_MODE_MEAS_SEQ_SINGLE_SHOT_MODE  BIT(5)
#define TSL2522_MEAS_MODE_MOD_FIFO_ALS_STAT_WRITE_EN BIT(4)
#define TSL2522_MEAS_MODE_ALS_SCALE                  GENMASK(3, 0)

#define TSL2522_REG_MEAS_MODE1                        0x82
#define TSL2522_MEAS_MODE1_MOD_FIFO_FD_END_MARKER_WEN BIT(7)
#define TSL2522_MEAS_MODE1_MOD_FIFO_FD_CHECKSUM_WEN   BIT(6)
#define TSL2522_MEAS_MODE1_MOD_FIFO_FD_GAIN_WEN       BIT(5)
#define TSL2522_MEAS_MODE1_ALS_MSB_POSITION_MASK      GENMASK(4, 0)
#define TSL2522_MEAS_MODE1_ALS_MSB_POSITION_DEFAULT   0x08

#define TSL2522_REG_SAMPLE_TIME0 0x83

#define TSL2522_REG_ALS_NR_SAMPLES0 0x85

#define TSL2522_REG_WTIME 0x89

#define TSL2522_REG_DEVICE_ID 0x92

#define TSL2522_REG_STATUS  0x93
#define TSL2522_STATUS_MINT BIT(7)
#define TSL2522_STATUS_AINT BIT(3)
#define TSL2522_STATUS_FINT BIT(2)
#define TSL2522_STATUS_SINT BIT(0)

#define TSL2522_REG_ALS_STATUS            0x94
#define TSL2522_ALS_STATUS_MEAS_SEQR_STEP GENMASK(7, 6)
#define TSL2522_ALS_STATUS_DATA0_ANA_SAT  BIT(5)
#define TSL2522_ALS_STATUS_DATA1_ANA_SAT  BIT(4)
#define TSL2522_ALS_STATUS_DATA0_SCALED   BIT(2)
#define TSL2522_ALS_STATUS_DATA1_SCALED   BIT(1)

#define TSL2522_REG_ALS_DATA 0x95

#define TSL2522_REG_ALS_STATUS2        0x9B
#define TSL2522_ALS_STATUS2_DATA1_GAIN GENMASK(7, 4)
#define TSL2522_ALS_STATUS2_DATA0_GAIN GENMASK(3, 0)

#define TSL2522_REG_STATUS2            0x9D
#define TSL2522_STATUS2_ALS_DATA_VALID BIT(6)
#define TSL2522_STATUS2_ALS_DIG_SAT    BIT(4)
#define TSL2522_STATUS2_ALS_FD_DIG_SAT BIT(3)
#define TSL2522_STATUS2_MOD_ANA_SAT1   BIT(1)
#define TSL2522_STATUS2_MOD_ANA_SAT0   BIT(0)

#define TSL2522_REG_STATUS3                   0x9E
#define TSL2522_STATUS3_AINT_HYST_STATE_VALID BIT(7)
#define TSL2522_STATUS3_AINT_HYST_STATE_RD    BIT(6)
#define TSL2522_STATUS3_AINT_AIHT             BIT(5)
#define TSL2522_STATUS3_AINT_AILT             BIT(4)
#define TSL2522_STATUS3_VSYNC_LOST            BIT(3)
#define TSL2522_STATUS3_OSC_CALIB_SATURATION  BIT(1)
#define TSL2522_STATUS3_OSC_CALIB_FINISHED    BIT(0)

#define TSL2522_REG_STATUS4                      0x9F
#define TSL2522_STATUS4_MOD_SAMPLE_TRIGGER_ERROR BIT(3)
#define TSL2522_STATUS4_MOD_TRIGGER_ERROR        BIT(2)
#define TSL2522_STATUS4_SAI_ACTIVE               BIT(1)
#define TSL2522_STATUS4_INIT_BUSY                BIT(0)

#define TSL2522_REG_STATUS5                        0xA0
#define TSL2522_STATUS5_SINT_MEASUREMENT_SEQUENCER BIT(1)
#define TSL2522_STATUS5_SINT_VSYNC                 BIT(0)

#define TSL2522_REG_CFG2 0xA3

#define TSL2522_REG_CFG4                            0xA5
#define TSL2522_CFG4_MOD_CALIB_NTH_ITER_STEP_ENABLE BIT(6)
#define TSL2522_CFG4_MEAS_SEQ_AGC_PRED_TARGET_LVL   BIT(5)
#define TSL2522_CFG4_MEAS_SEQ_SINT_PER_STEP         BIT(4)
#define TSL2522_CFG4_OSC_TUNE_NO_RESET              BIT(3)
#define TSL2522_CFG4_MOD_ALS_FIFO_DATA_FORMAT       GENMASK(2, 0)

#define TSL2522_REG_TRIGGER_MODE      0xAE
#define TSL2522_TRIGGER_MODE_MASK     GENMASK(2, 0)
#define TSL2522_TRIGGER_MODE_OFF      0x00
#define TSL2522_TRIGGER_MODE_NORMAL   0x01
#define TSL2522_TRIGGER_MODE_LONG     0x02
#define TSL2522_TRIGGER_MODE_FAST     0x03
#define TSL2522_TRIGGER_MODE_FASTLONG 0x04
#define TSL2522_TRIGGER_MODE_VSYNC    0x05

#define TSL2522_REG_CONTROL              0xB1
#define TSL2522_CONTROL_SOFT_RESET       BIT(3)
#define TSL2522_CONTROL_FIFO_CLR         BIT(1)
#define TSL2522_CONTROL_CLEAR_SAI_ACTIVE BIT(0)

#define TSL2522_REG_INTENAB  0xBA
#define TSL2522_INTENAB_MIEN BIT(7)
#define TSL2522_INTENAB_AIEN BIT(3)
#define TSL2522_INTENAB_FIEN BIT(2)
#define TSL2522_INTENAB_SIEN BIT(0)

#define TSL2522_REG_SIEN                   0xBB
#define TSL2522_SIEN_MEASUREMENT_SEQUENCER BIT(1)
#define TSL2522_SIEN_VSYNC                 BIT(0)

#define TSL2522_REG_MEAS_SEQR_STEP0_MOD_GAINX_L 0xD4
#define TSL2522_MEAS_SEQR_STEP0_MOD_GAIN1       GENMASK(7, 4)
#define TSL2522_MEAS_SEQR_STEP0_MOD_GAIN0       GENMASK(3, 0)

#define TSL2522_REG_MEAS_SEQR_STEP0_MOD_PHDX_SMUX_L 0xDC
#define TSL2522_MEAS_SEQR_STEP0_MOD_PHD3            GENMASK(7, 6)
#define TSL2522_MEAS_SEQR_STEP0_MOD_PHD2            GENMASK(5, 4)
#define TSL2522_MEAS_SEQR_STEP0_MOD_PHD1            GENMASK(3, 2)
#define TSL2522_MEAS_SEQR_STEP0_MOD_PHD0            GENMASK(1, 0)

#define TSL2522_REG_MEAS_SEQR_STEP0_MOD_PHDX_SMUX_H 0xDD
#define TSL2522_MEAS_SEQR_STEP0_MOD_PHD5            GENMASK(3, 2)
#define TSL2522_MEAS_SEQR_STEP0_MOD_PHD4            GENMASK(1, 0)

#define TSL2522_MOD_SEL_NO_CONN 0U
#define TSL2522_MOD_SEL_MOD_0   1U
#define TSL2522_MOD_SEL_MOD_1   2U

struct tsl2522_status_regs {
	uint8_t status2;
	uint8_t status3;
	uint8_t status4;
	uint8_t status5;
};

struct tsl2522_dts_config {
	struct i2c_dt_spec i2c;
#ifdef CONFIG_TSL2522_TRIGGER
	const struct gpio_dt_spec int_gpio;
#endif
	uint32_t glass_attenuation;
	uint32_t glass_ir_attenuation;
	enum sensor_gain_tsl2522 gain_enum;
};

struct tsl2522_measurement {
	uint32_t photopic_channel;
	uint32_t ir_channel;
	double atime_ms;
	double gain;
	bool saturation;
};

struct tsl2522_data {
	struct k_mutex mutex;
	struct tsl2522_measurement measurement;
	uint16_t measurement_time_steps;
	uint16_t number_of_samples;
	enum sensor_gain_tsl2522 gain;
	uint8_t als_scale;
#ifdef CONFIG_TSL2522_TRIGGER
	uint8_t saved_status2;
	const struct device *dev;
	struct gpio_callback gpio_cb;
	const struct sensor_trigger *als_data_ready;
	sensor_trigger_handler_t als_data_ready_handler;
	const struct sensor_trigger *als_overflow;
	sensor_trigger_handler_t als_overflow_handler;
#endif
#ifdef CONFIG_TSL2522_TRIGGER_OWN_THREAD
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_TSL2522_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem trig_sem;
#endif
#ifdef CONFIG_TSL2522_TRIGGER_GLOBAL_THREAD
	struct k_work work;
#endif
};

#ifdef CONFIG_TSL2522_TRIGGER
int tsl2522_trigger_init(const struct device *dev);

int tsl2522_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_TSL2522_TSL2522_H_ */
