/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_MAXM86161_MAXM86161_H_
#define ZEPHYR_DRIVERS_SENSOR_MAXM86161_MAXM86161_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/dt-bindings/sensor/maxm86161.h>
#include <zephyr/drivers/sensor/maxm86161.h>

/*
 * Status Register
 *
 * The individual STATUS1/STATUS2 flag masks (MAXM86161_MSK_INT_STATUS1_* and
 * MAXM86161_MSK_INT_STATUS2_SHA_DONE) are part of the public API and are
 * defined in <zephyr/drivers/sensor/maxm86161.h>, which this header includes.
 */
#define MAXM86161_REG_INT_STATUS1	0x00
#define MAXM86161_MSK_INT_STATUS1	GENMASK(7, 0)

#define MAXM86161_REG_INT_STATUS2	0x01

#define MAXM86161_REG_INT_ENABLE1			0x02
#define MAXM86161_MSK_INT_ENABLE1_A_FULL_EN		BIT(7)
#define MAXM86161_MSK_INT_ENABLE1_DATA_RDY_EN		BIT(6)
#define MAXM86161_MSK_INT_ENABLE1_ALC_OVF_EN		BIT(5)
#define MAXM86161_MSK_INT_ENABLE1_PROX_INT_EN		BIT(4)
#define MAXM86161_MSK_INT_ENABLE1_LED_COMPB_EN		BIT(3)
#define MAXM86161_MSK_INT_ENABLE1_DIE_TEMP_RDY_EN	BIT(2)

#define MAXM86161_REG_INT_ENABLE2		0x03
#define MAXM86161_MSK_INT_ENABLE2_SHA_DONE_EN	BIT(0)

/* FIFO */
#define MAXM86161_REG_FIFO_WR_PTR	0x04
#define MAXM86161_MSK_FIFO_WR_PTR	GENMASK(6, 0)

#define MAXM86161_REG_FIFO_RD_PTR	0x05
#define MAXM86161_MSK_FIFO_RD_PTR	GENMASK(6, 0)

#define MAXM86161_REG_FIFO_OVF_COUNTER	0x06
#define MAXM86161_MSK_FIFO_OVF_COUNTER	GENMASK(6, 0)

#define MAXM86161_REG_FIFO_DATA_COUNTER	0x07
#define MAXM86161_MSK_FIFO_DATA_COUNT	GENMASK(7, 0)

#define MAXM86161_REG_FIFO_DATA_REGISTER	0x08
#define MAXM86161_MSK_FIFO_DATA_REGISTER	GENMASK(7, 0)

#define MAXM86161_REG_FIFO_CONFIG1	0x09
#define MAXM86161_MSK_FIFO_A_FULL	GENMASK(6, 0)

#define MAXM86161_REG_FIFO_CONFIG2	0x0A
#define MAXM86161_MSK_FIFO_FLUSH	BIT(4)
#define MAXM86161_MSK_FIFO_STAT_CLR	BIT(3)
#define MAXM86161_MSK_FIFO_A_FULL_TYPE	BIT(2)
#define MAXM86161_MSK_FIFO_ROLLOVER	BIT(1)

/* System Control */
#define MAXM86161_REG_SYSTEM_CONTROL		0x0D
#define MAXM86161_MSK_SYSTEM_CONTROL_RESET	BIT(0)
#define MAXM86161_MSK_SYSTEM_CONTROL_SHDN	BIT(1)
#define MAXM86161_MSK_SYSTEM_CONTROL_LP_MODE	BIT(2)
#define MAXM86161_MSK_SYSTEM_CONTROL_SINGLE_PPG	BIT(3)

/* PPG Configuration */
#define MAXM86161_REG_PPG_SYNC_CONTROL			0x10
#define MAXM86161_MSK_PPG_SYNC_CONTROL_SW_FORCE_SYNC	BIT(4)
#define MAXM86161_MSK_PPG_SYNC_CONTROL_DAC_CODE_CHG_TAG	BIT(6)
#define MAXM86161_MSK_PPG_SYNC_CONTROL_TIME_STAMP_EN	BIT(7)
#define MAXM86161_MSK_PPG_SYNC_CONTROL_GPIO_CTRL	GENMASK(3, 0)

#define MAXM86161_REG_PPG_CONFIG1		0x11
#define MAXM86161_MSK_PPG_CONFIG1_PPG_TINT	GENMASK(1, 0)
#define MAXM86161_MSK_PPG_CONFIG1_PPG1_ADC_RGE	GENMASK(3, 2)
#define MAXM86161_MSK_PPG_CONFIG1_ADD_OFFSET	BIT(6)
#define MAXM86161_MSK_PPG_CONFIG1_ALC_DISABLE	BIT(7)

#define MAXM86161_REG_PPG_CONFIG2		0x12
#define MAXM86161_MSK_PPG_CONFIG2_PPG_SR	GENMASK(7, 3)
#define MAXM86161_MSK_PPG_CONFIG2_SMP_AVE	GENMASK(2, 0)

#define MAXM86161_REG_PPG_CONFIG3		0x13
#define MAXM86161_MSK_PPG_CONFIG3_BURST_EN	BIT(0)
#define MAXM86161_MSK_PPG_CONFIG3_BURST_RATE	GENMASK(2, 1)
#define MAXM86161_MSK_PPG_CONFIG3_DIG_FILT_SEL	BIT(5)
#define MAXM86161_MSK_PPG_CONFIG3_LED_SETLNG	GENMASK(7, 6)

#define MAXM86161_REG_PPG_PROX_INT_THRESH	0x14
#define MAXM86161_MSK_PPG_PROX_INT_THRESH	GENMASK(7, 0)

#define MAXM86161_REG_PPG_PD_BIAS	0x15
#define MAXM86161_MSK_PPG_PD_BIAS	GENMASK(2, 0)

/* PPG Picket Fence Detect and Replace */
#define MAXM86161_REG_PPG_PICKET_FENCE			0x16
#define MAXM86161_MSK_PPG_PICKET_FENCE_THRESH_SIGMA	GENMASK(1, 0)
#define MAXM86161_MSK_PPG_PICKET_FENCE_IIR_INIT_VAL	GENMASK(3, 2)
#define MAXM86161_MSK_PPG_PICKET_FENCE_IIR_TC		GENMASK(5, 4)
#define MAXM86161_MSK_PPG_PICKET_FENCE_PF_ORDER		BIT(6)
#define MAXM86161_MSK_PPG_PICKET_FENCE_PF_ENABLE	BIT(7)

/* LED Sequence Control */
#define MAXM86161_REG_LED_SEQ_REG1	0x20
#define MAXM86161_REG_LED_SEQ_REG2	0x21
#define MAXM86161_REG_LED_SEQ_REG3	0x22
#define MAXM86161_MSK_LED_SEQ_ODD	GENMASK(3, 0)
#define MAXM86161_MSK_LED_SEQ_EVEN	GENMASK(7, 4)

/* LED Pulse Amplitude */
#define MAXM86161_REG_LED1_PA		0x23
#define MAXM86161_REG_LED2_PA		0x24
#define MAXM86161_REG_LED3_PA		0x25
#define MAXM86161_REG_LED_PILOT_PA	0x29
#define MAXM86161_MSK_LED_PA		GENMASK(7, 0)

#define MAXM86161_REG_LED_RANGE_1		0x2A
#define MAXM86161_MSK_LED_RANGE1_LED1_RGE	GENMASK(1, 0)
#define MAXM86161_MSK_LED_RANGE1_LED2_RGE	GENMASK(3, 2)
#define MAXM86161_MSK_LED_RANGE1_LED3_RGE	GENMASK(5, 4)

/* PPG1_HI_RES_DAC */
#define MAXM86161_REG_S1_HI_RES_DAC1		0x2C
#define MAXM86161_REG_S2_HI_RES_DAC1		0x2D
#define MAXM86161_REG_S3_HI_RES_DAC1		0x2E
#define MAXM86161_REG_S4_HI_RES_DAC1		0x2F
#define MAXM86161_REG_S5_HI_RES_DAC1		0x30
#define MAXM86161_REG_S6_HI_RES_DAC1		0x31
#define MAXM86161_MSK_SN_HRES_DAC1		GENMASK(5, 0)
#define MAXM86161_MSK_SN_HI_RES_DAC1_OVR	BIT(7)

/* Die Temperature */
#define MAXM86161_REG_DIE_TEMP_CONFIG		0x40
#define MAXM86161_MSK_DIE_TEMP_CONFIG_TEMP_EN	BIT(0)

#define MAXM86161_REG_DIE_TEMP_INTEGER		0x41
#define MAXM86161_MSK_DIE_TEMP_INTEGER_TEMP_INT	GENMASK(7, 0)

#define MAXM86161_REG_DIE_TEMP_FRACTION			0x42
#define MAXM86161_MSK_DIE_TEMP_FRACTION_TEMP_FRAC	GENMASK(3, 0)

/* DAC Calibration */
#define MAXM86161_REG_DAC_CALIBRATION		0x50
#define MAXM86161_MSK_DAC_START_CAL		BIT(2)
#define MAXM86161_MSK_DAC_CAL_DAC1_OOR		BIT(4)
#define MAXM86161_MSK_DAC_CAL_DAC_COMPLETE	BIT(6)

/* SHA256 */
#define MAXM86161_REG_SHA_COMMAND		0xF0
#define MAXM86161_MSK_SHA_COMMAND_SHA_CMD	GENMASK(7, 0)

#define MAXM86161_REG_SHA_CONFIG		0xF1
#define MAXM86161_MSK_SHA_CONFIG_SHA_START	BIT(0)
#define MAXM86161_MSK_SHA_CONFIG_SHA_EN		BIT(1)

/* Memory */
#define MAXM86161_REG_MEM_CONTROL		0xF2
#define MAXM86161_MSK_MEM_CONTROL_BANK_SEL	BIT(0)
#define MAXM86161_MSK_MEM_CONTROL_MEM_WR_EN	BIT(1)

#define MAXM86161_REG_MEM_INDEX		0xF3
#define MAXM86161_MSK_MEM_INDEX_MEM_IDX	GENMASK(7, 0)

#define MAXM86161_REG_MEM_DATA		0xF4
#define MAXM86161_MSK_MEM_DATA_MEM_DATA	GENMASK(7, 0)

/* Part ID */
#define MAXM86161_REG_REV_ID		0xFE
#define MAXM86161_REG_PART_ID		0xFF
#define MAXM86161_MSK_PART_ID_PART_ID	GENMASK(7, 0)

enum maxm86161_fifo_tag {
	/* Normal PPG data tags are 1-based in the FIFO tag field. */
	MAXM86161_FIFO_TAG_LEDC1 = 1,
	MAXM86161_FIFO_TAG_LEDC2,
	MAXM86161_FIFO_TAG_LEDC3,
	MAXM86161_FIFO_TAG_LEDC4,
	MAXM86161_FIFO_TAG_LEDC5,
	MAXM86161_FIFO_TAG_LEDC6,
	MAXM86161_FIFO_TAG_LEDC1_PF = 13,
	MAXM86161_FIFO_TAG_LEDC2_PF,
	MAXM86161_FIFO_TAG_LEDC3_PF,
	MAXM86161_FIFO_TAG_PROX = 25,
	MAXM86161_FIFO_TAG_SUB_DAC = 29,
	MAXM86161_FIFO_TAG_INVALID = 30,
	MAXM86161_FIFO_TAG_TIMESTAMP = 31,
};

enum maxm86161_exposure_tag {
	MAXM86161_EXPOSURE_NONE = MAXM86161_DT_EXPOSURE_NONE,
	MAXM86161_EXPOSURE_LED1_GREEN = MAXM86161_DT_EXPOSURE_GREEN,
	MAXM86161_EXPOSURE_LED2_IR = MAXM86161_DT_EXPOSURE_IR,
	MAXM86161_EXPOSURE_LED3_RED = MAXM86161_DT_EXPOSURE_RED,
	MAXM86161_EXPOSURE_PILOT_ON_GREEN = MAXM86161_DT_EXPOSURE_PILOT_ON_GREEN,
	MAXM86161_EXPOSURE_AMBIENT_LIGHT = MAXM86161_DT_EXPOSURE_DIRECT_AMB_LIGHT,
};

enum maxm86161_attr_flags {
	MAXM86161_ATTR_FLAG_WR_ONLY = BIT(0),
	MAXM86161_ATTR_FLAG_RD_ONLY = BIT(1),
};

#define MAXM86161_RESET_DELAY_US 100

/** Proximity pilot FIFO tag */
#define MAXM86161_PROX_PILOT_TAG 0x19U

/** Settling suppression window (ms) after prox-to-normal transitions */
#define MAXM86161_PROX_SETTLE_MS 200

#define MAXM86161_LED_STATE_POS_NONE	(-1)
#define MAXM86161_LED_STATE_POS_PROX	0

#define MAXM86161_FIFO_DEPTH		128
#define MAXM86161_FIFO_WMARK_MAX	127
#define MAXM86161_FIFO_SAMPLE_SIZE	3U
#define MAXM86161_FIFO_TAG_MASK		GENMASK(23, 19)
#define MAXM86161_FIFO_DATA_MASK	GENMASK(18, 0)
#define MAXM86161_MAX_NUM_CHANNELS	6
#define MAXM86161_MAX_NUM_SEQUENCES	6
#define MAXM86161_MAX_NUM_SAMPLES	(MAXM86161_MAX_NUM_CHANNELS * MAXM86161_FIFO_SAMPLE_SIZE)

/*
 * chan_pos[] is indexed directly by the exposure tag, so it must be large enough
 * to hold the highest tag value (ambient light).
 */
#define MAXM86161_CHAN_POS_COUNT	(MAXM86161_EXPOSURE_AMBIENT_LIGHT + 1)

#define MAXM86161_PART_ID_VAL	0x36

#define MAXM86161_TEMP_MEAS_WAIT_TRIES	10
#define MAXM86161_TEMP_MEAS_DELAY_MS	10
#define MAXM86161_TEMP_FRAC_SCALE	62500 /* 0.0625C */

#define MAXM86161_LED_SEQ_COUNT 6

static const uint16_t sample_rate[] = {
	[MAXM86161_DT_24p995SPS_1PPS] = 25,
	[MAXM86161_DT_50p027SPS_1PPS] = 50,
	[MAXM86161_DT_84p021SPS_1PPS] = 84,
	[MAXM86161_DT_99p902SPS_1PPS] = 100,
	[MAXM86161_DT_199p805SPS_1PPS] = 200,
	[MAXM86161_DT_399p610SPS_1PPS] = 400,
	[MAXM86161_DT_24p995SPS_2PPS] = 25,
	[MAXM86161_DT_50p027SPS_2PPS] = 50,
	[MAXM86161_DT_84p021SPS_2PPS] = 84,
	[MAXM86161_DT_99p902SPS_2PPS] = 100,
	[MAXM86161_DT_8SPS_1PPS] = 8,
	[MAXM86161_DT_16SPS_1PPS] = 16,
	[MAXM86161_DT_32SPS_1PPS] = 32,
	[MAXM86161_DT_64SPS_1PPS] = 64,
	[MAXM86161_DT_128SPS_1PPS] = 128,
	[MAXM86161_DT_256SPS_1PPS] = 256,
	[MAXM86161_DT_512SPS_1PPS] = 512,
	[MAXM86161_DT_1024SPS_1PPS] = 1024,
	[MAXM86161_DT_2048SPS_1PPS] = 2048,
	[MAXM86161_DT_4096SPS_1PPS] = 4096,
};

static const uint32_t sample_avg[] = {
	[MAXM86161_DT_SMP_AVG_1] = 1,
	[MAXM86161_DT_SMP_AVG_2] = 2,
	[MAXM86161_DT_SMP_AVG_4] = 4,
	[MAXM86161_DT_SMP_AVG_8] = 8,
	[MAXM86161_DT_SMP_AVG_16] = 16,
	[MAXM86161_DT_SMP_AVG_32] = 32,
	[MAXM86161_DT_SMP_AVG_64] = 64,
	[MAXM86161_DT_SMP_AVG_128] = 128,
};

static const uint16_t burst_rate[] = {
	[MAXM86161_DT_BURST_8HZ] = 8,
	[MAXM86161_DT_BURST_32HZ] = 32,
	[MAXM86161_DT_BURST_84HZ] = 84,
	[MAXM86161_DT_BURST_256HZ] = 256,
};

/* defines limits that  */
static const int8_t sample_burst_count[][ARRAY_SIZE(burst_rate)] = {
	[MAXM86161_DT_24p995SPS_1PPS]	= {1, -1, -1, -1},
	[MAXM86161_DT_50p027SPS_1PPS]	= {2, 0, -1, -1},
	[MAXM86161_DT_84p021SPS_1PPS]	= {3, 1, -1, -1},
	[MAXM86161_DT_99p902SPS_1PPS]	= {3, 1, -1, -1},
	[MAXM86161_DT_199p805SPS_1PPS]	= {4, 2, 0, -1},
	[MAXM86161_DT_399p610SPS_1PPS]	= {5, 3, 1, -1},
	[MAXM86161_DT_24p995SPS_2PPS]	= {1, -1, -1, -1},
	[MAXM86161_DT_50p027SPS_2PPS]	= {2, 0, -1, -1},
	[MAXM86161_DT_84p021SPS_2PPS]	= {3, 1, -1, -1},
	[MAXM86161_DT_99p902SPS_2PPS]	= {3, 1, -1, -1},
	[MAXM86161_DT_8SPS_1PPS]	= {-1, -1, -1, -1},
	[MAXM86161_DT_16SPS_1PPS]	= {0, -1, -1, -1},
	[MAXM86161_DT_32SPS_1PPS]	= {1, -1, -1, -1},
	[MAXM86161_DT_64SPS_1PPS]	= {2, 0, -1, -1},
	[MAXM86161_DT_128SPS_1PPS]	= {3, 1, 0, -1},
	[MAXM86161_DT_256SPS_1PPS]	= {4, 2, 1, -1},
	[MAXM86161_DT_512SPS_1PPS]	= {5, 3, 2, -1},
	[MAXM86161_DT_1024SPS_1PPS]	= {6, 4, 3, 0},
	[MAXM86161_DT_2048SPS_1PPS]	= {7, 5, 4, 1},
	[MAXM86161_DT_4096SPS_1PPS]	= {7, 6, 5, 2},
};

struct maxm86161_prox_attributes {
	/**
	 * True while proximity mode is active at runtime, i.e. the proximity
	 * interrupt (PROX_INT) is enabled. Gates settle-suppression and
	 * picket-fence handling; set/cleared by maxm86161_trigger_set().
	 */
	bool enabled;
	bool object_detected;
	int64_t prox_transition_time;
};

#ifdef CONFIG_MAXM86161_TRIGGER
#define MAXM86161_CUSTOM_TRIGGER_COUNT 4
#define MAXM86161_COMMON_TRIGGER_COUNT 2

struct maxm86161_trigger {
	const struct sensor_trigger *trig;
	sensor_trigger_handler_t trig_handler;
};
#endif

#ifdef CONFIG_MAXM86161_STREAM
struct maxm86161_fifo_hdr {
	bool is_fifo: 1;
	uint8_t: 7;
	uint64_t timestamp;
	uint16_t fifo_byte_count;
	uint8_t fifo_samples;
	uint8_t int_status;
	uint8_t fifo_ovf_count;
	uint16_t odr;
} __packed;
#endif

struct maxm86161_led_state {
	/* Latest sample per FIFO position, indexed by the sample's FIFO slot. */
	uint32_t raw[MAXM86161_MAX_NUM_CHANNELS];
	/* Exposure tag configured for each LED sequence slot (MAXM86161_EXPOSURE_*). */
	uint8_t map[MAXM86161_MAX_NUM_SEQUENCES];
	/* Position in raw[] for each exposure tag, or MAXM86161_LED_STATE_POS_NONE
	 * when that exposure is not part of the configured sequence. Indexed by
	 * exposure tag (MAXM86161_EXPOSURE_*).
	 */
	int8_t chan_pos[MAXM86161_CHAN_POS_COUNT];
	/* Number of active (non-NONE) exposures in the sequence. */
	uint8_t num_active_channels;
};

struct maxm86161_data {
	struct maxm86161_led_state led_state;
	struct maxm86161_prox_attributes prox_attr;
	struct sensor_value temp_val;
	uint16_t odr;
#ifdef CONFIG_MAXM86161_TRIGGER
	struct gpio_callback gpio_cb;
	struct maxm86161_trigger custom_trigs[MAXM86161_CUSTOM_TRIGGER_COUNT];
	struct maxm86161_trigger common_trigs[MAXM86161_COMMON_TRIGGER_COUNT];
	struct maxm86161_trigger die_drdy_trigger;
	const struct device *dev;
	uint8_t status1_cache;
	bool status1_cache_ready;
	struct k_mutex trigger_mutex;
#if defined(CONFIG_MAXM86161_TRIGGER_OWN_THREAD)
	struct k_sem gpio_sem;
	struct k_thread thread;

	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_MAXM86161_THREAD_STACK_SIZE);

#elif defined(CONFIG_MAXM86161_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif /* CONFIG_MAXM86161_TRIGGER */
#ifdef CONFIG_MAXM86161_STREAM
	struct rtio *rtio_ctx;
	struct rtio_iodev *iodev;
	struct rtio_iodev_sqe *iodev_sqe;
	uint64_t timestamp;
	uint8_t fifo_watermark;
	atomic_t stream_mode;
	uint8_t fifo_count_regs[2];
	uint8_t fifo_config_cache;
#endif /* CONFIG_MAXM86161_STREAM */
};

struct maxm86161_fifo_config {
	uint8_t watermark;
	uint8_t rollover: 1;
	uint8_t a_full_type: 1;
	uint8_t: 6;
};

struct maxm86161_ppg_sync_control {
	uint8_t gpio_ctrl: 4;
	uint8_t dac_code_chg_tag: 1;
	uint8_t time_stamp_en: 1;
	uint8_t: 2;
};

struct maxm86161_ppg_config1 {
	uint8_t ppg_tint: 2;
	uint8_t ppg1_adc_rge: 2;
	uint8_t add_offset: 1;
	uint8_t alc_disable: 1;
	uint8_t: 2;
};

struct maxm86161_ppg_config2 {
	uint8_t smp_avg: 3;
	uint8_t ppg_sr: 5;
};

struct maxm86161_ppg_config3 {
	uint8_t burst_en: 1;
	uint8_t burst_rate: 2;
	uint8_t dig_filt_sel: 1;
	uint8_t led_setlng: 2;
	uint8_t: 2;
};

struct maxm86161_ppg_config {
	struct maxm86161_ppg_sync_control sync_ctrl;
	struct maxm86161_ppg_config1 ppg_cfg1;
	struct maxm86161_ppg_config2 ppg_cfg2;
	struct maxm86161_ppg_config3 ppg_cfg3;
	uint8_t prox_int_thresh;
	uint8_t pd_bias;
};

struct maxm86161_picket_fence_config {
	uint8_t thresh_sigma: 2;
	uint8_t iir_init_val: 2;
	uint8_t iir_tc: 2;
	uint8_t order: 1;
	uint8_t enable: 1;
};

struct maxm86161_led_range_config {
	uint8_t led1_rge: 2;
	uint8_t led2_rge: 2;
	uint8_t led3_rge: 2;
	uint8_t: 2;
};

struct maxm86161_led_pa_config {
	uint16_t led1_pa;
	uint16_t led2_pa;
	uint16_t led3_pa;
	uint16_t led_pilot_pa;
};

struct maxm86161_led_config {
	struct maxm86161_led_pa_config led_pa_cfg;
	uint8_t led_seq[MAXM86161_LED_SEQ_COUNT];
	struct maxm86161_led_range_config led_rge_cfg;
};

struct maxm86161_sys_config {
	uint8_t low_power_mode: 1;
	uint8_t single_ppg: 1;
	uint8_t: 6;
};

struct maxm86161_config {
	struct i2c_dt_spec i2c;
	struct maxm86161_led_config led_cfg;
	struct maxm86161_ppg_config ppg_cfg;
	struct maxm86161_sys_config sys_cfg;
	struct maxm86161_fifo_config fifo_cfg;
	struct maxm86161_picket_fence_config pf_cfg;
	bool temp_en;

#if defined(CONFIG_MAXM86161_TRIGGER)
	struct gpio_dt_spec interrupt_gpio;
#endif
};

struct maxm86161_attr_desc {
	enum sensor_attribute attr; /* sensor attribute to access */
	uint8_t reg;		    /* register address of target attribute */
	uint8_t mask;		    /* bitfield mask */
	uint8_t flags;		    /* Attribute flags (specified in maxm86161_attr_flags)*/
	/* Encode/Validate the input value before sending to device */
	int (*set_prep)(const struct device *dev, enum sensor_channel channel,
			enum sensor_attribute attr, struct sensor_value *val);
	/* Optional callback to post-process after a successful set */
	int (*set_callback)(const struct device *dev, enum sensor_channel channel,
			    enum sensor_attribute attr, const struct sensor_value *val);
};

/**
 * @brief Write internal register.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg_addr Address of the internal register being updated.
 * @param value Value to write to internal register.
 * @return 0 If successful.
 * @return Negative errno code if failure.
 */
int maxm86161_i2c_write_byte(const struct device *dev, uint8_t reg_addr, uint8_t value);

/**
 * @brief Update internal register.
 * @note This function preserves the current value of bits not being updated.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg_addr Address of the internal register being updated.
 * @param mask Bitmask for updating internal register.
 * @param value Value for updating internal register.
 * @return 0 If successful.
 * @return Negative errno code if failure.
 */
int maxm86161_i2c_update_byte(const struct device *dev, uint8_t reg_addr, uint8_t mask,
			      uint8_t value);

/**
 * @brief Read internal register.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg_addr Address of the internal register being read.
 * @param rd_buf Pointer to where the read value will be stored.
 * @return 0 If successful.
 * @return Negative errno code if failure.
 */
int maxm86161_i2c_read_byte(const struct device *dev, uint8_t reg_addr, uint8_t *rd_buf);

/**
 * @brief Burst write to multiple consecutive internal registers.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param start_addr Starting address of the internal registers.
 * @param wr_buf Pointer to the data buffer to write.
 * @param num_bytes Number of bytes to write.
 * @return 0 If successful.
 * @return Negative errno code if failure.
 */
int maxm86161_i2c_burst_write(const struct device *dev, uint8_t start_addr, uint8_t *wr_buf,
			      uint8_t num_bytes);

/**
 * @brief Burst read from multiple consecutive internal registers.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param start_addr Starting address of the internal registers.
 * @param rd_buf Pointer to the data buffer to store read values.
 * @param num_bytes Number of bytes to read.
 * @return 0 If successful.
 * @return Negative errno code if failure.
 */
int maxm86161_i2c_burst_read(const struct device *dev, uint8_t start_addr, uint8_t *rd_buf,
			     uint8_t num_bytes);

#ifdef CONFIG_MAXM86161_TRIGGER

/**
 * @brief Set sensor trigger handler.
 * @note Implements the Zephyr sensor API trigger_set callback.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param trig Pointer to the sensor trigger configuration.
 * @param handler Trigger handler function, or NULL to disable.
 * @return 0 If successful.
 * @return -ENOTSUP if trigger type is not supported.
 * @return Negative errno code if failure.
 */
int maxm86161_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler);

/**
 * @brief Initialize GPIO interrupt and trigger handling thread.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @return 0 If successful.
 * @return Negative errno code if failure.
 */
int maxm86161_init_interrupt(const struct device *dev);
#endif /* CONFIG_MAXM86161_TRIGGER */

#ifdef CONFIG_MAXM86161_STREAM
/**
 * @brief Submit an async sensor read request.
 * @note Implements the Zephyr sensor API submit callback.
 *       Only streaming mode is currently supported.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param iodev_sqe RTIO submission queue entry for the read request.
 */
void maxm86161_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);

/**
 * @brief Get the sensor decoder API.
 * @note Copies the current channel-to-position map and active channel count
 *       into module-level state for use by the decoder functions.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param decoder Pointer to store the decoder API reference.
 * @return 0 Always successful.
 */
int maxm86161_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);
#endif /* CONFIG_MAXM86161_STREAM */

#ifdef CONFIG_MAXM86161_STREAM
/**
 * @brief Submit a streaming read request via RTIO.
 * @note Configures FIFO watermark interrupt and stores the SQE for later completion.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param iodev_sqe RTIO submission queue entry for the streaming request.
 */
void maxm86161_submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);

/**
 * @brief Handle GPIO interrupt for streaming mode.
 * @note Timestamps the event and submits an RTIO read of the interrupt
 *       status register to begin the streaming pipeline.
 *
 * @param dev Pointer to the device structure for the driver instance.
 */
void maxm86161_stream_irq_handler(const struct device *dev);
#endif /* CONFIG_MAXM86161_STREAM */

#endif /* ZEPHYR_DRIVERS_SENSOR_MAXM86161_MAXM86161_H_ */
