/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LTC4286_LTC4286_H_
#define ZEPHYR_DRIVERS_SENSOR_LTC4286_LTC4286_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/ltc4286.h>

/* Device Configuration v */
#define LTC4286_CMD_PAGE          0x00
#define LTC4286_CMD_OPERATION     0x01
#define LTC4286_CMD_CLEAR_FAULTS  0x03
#define LTC4286_CMD_WRITE_PROTECT 0x10
#define LTC4286_CMD_CAPABILITY    0x19

/* Warning Threshold Command Codes */
#define LTC4286_CMD_VOUT_UPPER_THRES_WARN  0x42
#define LTC4286_CMD_VOUT_LOWER_THRES_WARN  0x43
#define LTC4286_CMD_IOUT_UPPER_THRES_WARN  0x4A
#define LTC4286_CMD_IOUT_UPPER_THRES_FAULT 0x4F
#define LTC4286_CMD_TEMP_UPPER_THRES_WARN  0x51
#define LTC4286_CMD_TEMP_LOWER_THRES_WARN  0x52
#define LTC4286_CMD_VIN_UPPER_THRES_WARN   0x57
#define LTC4286_CMD_VIN_LOWER_THRES_WARN   0x58
#define LTC4286_CMD_PIN_UPPER_THRES_WARN   0x6B

/* Fault Response Configuration Command Codes */
#define LTC4286_CMD_IOUT_OC_FAULT_RESPONSE 0x47
#define LTC4286_CMD_OT_FAULT_RESPONSE      0x50
#define LTC4286_CMD_VIN_OV_FAULT_RESPONSE  0x56
#define LTC4286_CMD_VIN_UV_FAULT_RESPONSE  0x5A
#define LTC4286_CMD_MFR_FET_FAULT_RESPONSE 0xD6
#define LTC4286_CMD_MFR_OP_FAULT_RESPONSE  0xD7

/* Status Command Codes */
#define LTC4286_CMD_STATUS_BYTE         0x78
#define LTC4286_CMD_STATUS_WORD         0x79
#define LTC4286_CMD_STATUS_VOUT         0x7A
#define LTC4286_CMD_STATUS_IOUT         0x7B
#define LTC4286_CMD_STATUS_INPUT        0x7C
#define LTC4286_CMD_STATUS_TEMPERATURE  0x7D
#define LTC4286_CMD_STATUS_CML          0x7E
#define LTC4286_CMD_STATUS_OTHER        0x7F
#define LTC4286_CMD_STATUS_MFR_SPECIFIC 0x80
#define LTC4286_CMD_MFR_SYSTEM_STATUS1  0xE0
#define LTC4286_CMD_MFR_SYSTEM_STATUS2  0xE1

/* Other Command Codes */
#define LTC4286_CMD_PMBUS_REVISION          0x98
#define LTC4286_CMD_MFR_ID                  0x99
#define LTC4286_CMD_MFR_MODEL               0x9A
#define LTC4286_CMD_MFR_REVISION            0x9B
#define LTC4286_CMD_IC_DEVICE_ID            0xAD
#define LTC4286_CMD_IC_DEVICE_REV           0xAE
#define LTC4286_CMD_USER_DATA_00            0xB0
#define LTC4286_CMD_USER_DATA_02            0xB2
#define LTC4286_CMD_USER_TIME               0xB9
#define LTC4286_CMD_MFR_FLT_CONFIG          0xD2
#define LTC4286_CMD_MFR_ADC_CONFIG          0xD8
#define LTC4286_CMD_MFR_AVG_SEL             0xD9
#define LTC4286_CMD_MFR_PMB_STAT            0xE2
#define LTC4286_CMD_MFR_PADS_LIVE_STATUS    0xE5
#define LTC4286_CMD_MFR_SPECIAL_ID          0xE7
#define LTC4286_CMD_MFR_COMMON              0xEF
#define LTC4286_CMD_MFR_SD_CAUSE            0xF1
#define LTC4286_CMD_MFR_CONFIG1             0xF2
#define LTC4286_CMD_MFR_CONFIG2             0xF3
#define LTC4286_CMD_MFR_GPIO_INV            0xF4
#define LTC4286_CMD_MFR_GPO_SEL41           0xF5
#define LTC4286_CMD_MFR_GPO_SEL85           0xF6
#define LTC4286_CMD_MFR_GPI_SEL             0xF7
#define LTC4286_CMD_MFR_GPI_DATA            0xF8
#define LTC4286_CMD_MFR_GPO_DATA            0xF9
#define LTC4286_CMD_MFR_REBOOT_CONTROL      0xFD
#define LTC4286_CMD_MFR_IOUT_UC_LIMIT       0xFE04
#define LTC4286_CMD_MFR_PIN_UP_LIMIT        0xFE0C
#define LTC4286_CMD_MFR_VDS_UV_LIMIT        0xFE24
#define LTC4286_CMD_MFR_VDS_OV_LIMIT        0xFE25
#define LTC4286_CMD_MFR_PIN_OP1_FAULT_LIMIT 0xFE58
#define LTC4286_CMD_MFR_PIN_OP2_FAULT_LIMIT 0xFE59

/* Telemetry Data Command Codes */
#define LTC4286_CMD_READ_VIN  0x88
#define LTC4286_CMD_READ_VOUT 0x8B
#define LTC4286_CMD_READ_IOUT 0x8C
#define LTC4286_CMD_READ_TEMP 0x8D
#define LTC4286_CMD_READ_PIN  0x97

/* VOUT Status (STATUS_VOUT) */
#define LTC4286_STATUS_VOUT_OV_WARN BIT(6)
#define LTC4286_STATUS_VOUT_UV_WARN BIT(5)

/* IOUT Status (STATUS_IOUT) */
#define LTC4286_STATUS_IOUT_OC_FAULT BIT(7)
#define LTC4286_STATUS_IOUT_OC_WARN  BIT(5)

/* Input Telemtry Statuses (STATUS_INPUT) */
#define LTC4286_STATUS_INPUT_VIN_OV_FAULT BIT(7)
#define LTC4286_STATUS_INPUT_VIN_OV_WARN  BIT(6)
#define LTC4286_STATUS_INPUT_VIN_UV_WARN  BIT(5)
#define LTC4286_STATUS_INPUT_VIN_UV_FAULT BIT(4)
#define LTC4286_STATUS_INPUT_PIN_OP_WARN  BIT(0)

/* Temperature Telemetry Status (STATUS_TEMP) */
#define LTC4286_STATUS_TEMP_OT_FAULT BIT(7)
#define LTC4286_STATUS_TEMP_OT_WARN  BIT(6)
#define LTC4286_STATUS_TEMP_UT_WARN  BIT(5)

/* Communication Line Status (STATUS_CML) */
#define LTC4286_STATUS_CML_BAD_CMD    BIT(7)
#define LTC4286_STATUS_CML_BAD_DATA   BIT(6)
#define LTC4286_STATUS_CML_PEC_FAIL   BIT(5)
#define LTC4286_STATUS_CML_MISC_FAULT BIT(1)

/* Other Device Statuses (STATUS_OTHER) */
#define LTC4286_STATUS_OTHER_FIRST_ALERT BIT(0)

/* MFR-Specific Statuses (STATUS_MFR_SPEC) */
#define LTC4286_STATUS_MFR_SPEC_EN_CHANGE     BIT(7)
#define LTC4286_STATUS_MFR_SPEC_TSD_FAULT     BIT(6)
#define LTC4286_STATUS_MFR_SPEC_VDD_UVLO      BIT(5)
#define LTC4286_STATUS_MFR_SPEC_PIN_OP2_FAULT BIT(4)
#define LTC4286_STATUS_MFR_SPEC_PIN_OP1_FAULT BIT(3)
#define LTC4286_STATUS_MFR_SPEC_FET_BAD_FAULT BIT(2)

/* Mask Command Codes for disabling status bits contributing to alerts */
#define LTC4286_CMD_ALERT_MASK          0xFE /* first byte of mask command */
#define LTC4286_CMD_ALERT_MASK_BYTE     0xD0
#define LTC4286_CMD_ALERT_MASK_VOUT     0xD2
#define LTC4286_CMD_ALERT_MASK_IOUT     0xD3
#define LTC4286_CMD_ALERT_MASK_INPUT    0xD4
#define LTC4286_CMD_ALERT_MASK_TEMP     0xD5
#define LTC4286_CMD_ALERT_MASK_COMMS    0xD6
#define LTC4286_CMD_ALERT_MASK_SPECIFIC 0xD8
#define LTC4286_CMD_ALERT_MASK_STAT1    0xDA
#define LTC4286_CMD_ALERT_MASK_STAT2    0xDC

#define LTC4286_CFG_WRITE_PROTECT_1 BIT(7)
#define LTC4286_CFG_WRITE_PROTECT_2 BIT(6) /* excludes operation and clear fault */

#define LTC4286_CFG_DISP_AVG    BIT(7)
#define LTC4286_CFG_ADC_AVG_SEL GENMASK(3, 0)

#define LTC4286_CFG_VDS_SEL  BIT(1)
#define LTC4286_CFG_VIN_VOUT BIT(0)

#define LTC4286_CFG_CONFIG1_RSVD  FIELD_PREP(GENMASK(12, 2), 0x5C)
#define LTC4286_CFG_CURRENT_LIMIT GENMASK(13, 10)
#define LTC4286_CFG_VRANGE_SEL    BIT(1)
#define LTC4286_CFG_VPWR_SEL      BIT(0)

#define LTC4286_CFG_CONFIG2_RSVD   FIELD_PREP(GENMASK(3, 0), 0x9)
#define LTC4286_CFG_PMBUS_1MBPS    BIT(12)
#define LTC4286_CFG_RESET_FAULT_EN BIT(7)
#define LTC4286_CFG_PWRGD_RESET    BIT(6)
#define LTC4286_CFG_MASS_WR_EN     BIT(5)
#define LTC4286_CFG_EXT_TEMP_EN    BIT(2)
#define LTC4286_CFG_DEB_TMR_EN     BIT(1)

#define LTC4286_CFG_GPO_8_4     GENMASK(15, 12)
#define LTC4286_CFG_GPO_7_3     GENMASK(11, 8)
#define LTC4286_CFG_GPO_6_2     GENMASK(7, 4)
#define LTC4286_CFG_GPO_5_1     GENMASK(3, 0)
#define LTC4286_CFG_GPI_RBT_EN  BIT(7)
#define LTC4286_CFG_GPI_RBT_PIN GENMASK(6, 4)
#define LTC4286_CFG_GPI_CMP_SEL GENMASK(2, 0)

#define LTC4286_NUM_GPIOS 8

#define LTC4286_CFG_REBOOT_INV   BIT(8)
#define LTC4286_CFG_REBOOT_INIT  BIT(5)
#define LTC4286_CFG_REBOOT_DELAY GENMASK(2, 0)

#define LTC4286_FAULT_RESPONSE_MASK GENMASK(7, 6)
#define LTC4286_FAULT_RETRY_MASK    GENMASK(5, 3)

#define LTC4286_FAULT_RESP_TYP1 0b01
#define LTC4286_FAULT_RESP_TYP2 0b10
#define LTC4286_FAULT_RESP_TYP3 0b11

#define DT_FAULT_CFG_GET(node_id, prop)                                                            \
	{                                                                                          \
		.retries = DT_INST_PROP_OR(node_id, prop, idx, retries),                           \
		.no_ignore = DT_PHA_BY_IDX(node_id, prop, idx, response_ignore),                   \
	}

#define DT_FAULT_CFG_GET_OR(node_id, prop, default_value)                                          \
	COND_CODE_1(DT_PROP_HAS_IDX(node_id, prop, 0),				\
			(DT_FAULT_CFG_GET_BY_IDX(node_id, prop, 0)),		\
			(default_value))

/* Constant conversion factors for telemetry data */
#define LTC4286_ADC_VAL_TO_UVAL          1000000UL   /* 10^6 */
#define LTC4286_ADC_VIN_VOUT_100V_FACTOR 3125UL      /* (10^-1)/32 = .003125 */
#define LTC4286_ADC_VIN_VOUT_100V_SCALE  1000000UL   /* 10^6 */
#define LTC4286_ADC_VIN_VOUT_25V_FACTOR  78125UL     /* (10^-1)/128 = .00078125 */
#define LTC4286_ADC_VIN_VOUT_25V_SCALE   100000000UL /* 10^8 */
#define LTC4286_ADC_TEMP_K_TO_C          27315UL     /* 273.15 */
#define LTC4286_ADC_TEMP_SCALE           100UL       /* 10^2 */

/* Constant conversion factors to calculate with RSense (factor/rsense(uohm)) */
#define LTC4286_ADC_RSENSE_MILI_TO_MICRO_OHMS 1000UL     /* 10^3 */
#define LTC4286_ADC_IOUT_FACTOR               9765625UL  /* (Rsense scaling)/1024 = 0.9765625 */
#define LTC4286_ADC_IOUT_SCALE                10000000UL /* 10^7 */
#define LTC4286_ADC_PIN_100V_FACTOR           1UL        /* (Rsense scaling) * 10^-4 = 0.1 */
#define LTC4286_ADC_PIN_100V_SCALE            10UL       /* 10^1 */
#define LTC4286_ADC_PIN_25V_FACTOR            25UL       /* (Rsense scaling)/4 * 10^-4 = 0.025 */
#define LTC4286_ADC_PIN_25V_SCALE             1000UL     /* 10^3 */

#ifdef CONFIG_LTC4286_TRIGGER
enum ltc4286_trig_idx {
	LTC4286_TRIG_IDX_OP = 0,
	LTC4286_TRIG_IDX_OC,
	LTC4286_TRIG_IDX_COMP,
	LTC4286_TRIG_IDX_FET,
	LTC4286_TRIG_IDX_PWR_GD,
	LTC4286_TRIG_IDX_ALERT
};
#endif

enum ltc4286_gpo_mode {
	LTC4286_GPO_TRISTATE = 0,
	LTC4286_GPO_MFR_DATA,
	LTC4286_GPO_CMPOUT,
	LTC4286_GPO_PWR_GOOD,
	LTC4286_GPO_FAULT,
	LTC4286_GPO_I_OUT_OC,
	LTC4286_GPO_P_IN_OP = 9,
	LTC4286_GPO_ALERT,
	LTC4286_GPO_L_ALERT,
	LTC4286_GPO_TEMP = 15, /* Temp sensor for use of GPIO3 only */
};

struct ltc4286_fault_response_spec {
	uint8_t retries;
	bool no_ignore;
};

struct ltc4286_fault_response_specs {
	struct ltc4286_fault_response_spec overcurr_config;
	struct ltc4286_fault_response_spec overvolt_config;
	struct ltc4286_fault_response_spec undervolt_config;
	struct ltc4286_fault_response_spec overtemp_config;
	struct ltc4286_fault_response_spec overpower_config;
	struct ltc4286_fault_response_spec fet_config;
};

struct ltc4286_gpio_specs {
	struct gpio_dt_spec reboot_gpio;
	struct gpio_dt_spec overpower_gpio;
	struct gpio_dt_spec overcurrent_gpio;
	struct gpio_dt_spec comp_out_gpio;
	struct gpio_dt_spec fault_out_gpio;
	struct gpio_dt_spec power_good_gpio;
	struct gpio_dt_spec alert_gpio;
};

struct ltc4286_gpio_config {
	const struct gpio_dt_spec *gpio_spec;
	uint8_t dev_gpio_pin;
	gpio_flags_t flags;
};

#ifdef CONFIG_LTC4286_TRIGGER
struct ltc4286_trig {
	sensor_trigger_handler_t handler;
	struct sensor_trigger trigger;
};

struct ltc4286_gpio_trig {
	struct ltc4286_trig trig;
	struct gpio_callback gpio_cb;
	const struct gpio_dt_spec *gpio;
	enum ltc4286_trig_idx trig_idx;
	bool is_enabled;
};
#endif /* CONFIG_LTC4286_TRIGGER */

struct ltc4286_data {
	uint32_t vin_sample;
	uint32_t vout_sample;
	uint32_t iout_sample;
	uint32_t pin_sample;
	uint32_t temp_sample;
	uint32_t status_flags_sample;
	bool is_enabled;

#ifdef CONFIG_LTC4286_TRIGGER
	struct ltc4286_gpio_trig overpower_trig;
	struct ltc4286_gpio_trig overcurrent_trig;
	struct ltc4286_gpio_trig comp_out_trig;
	struct ltc4286_gpio_trig fault_out_trig;
	struct ltc4286_gpio_trig power_good_trig;
	struct ltc4286_gpio_trig alert_trig;

	struct ltc4286_trig overvolt_trig;
	struct ltc4286_trig undervolt_trig;
	struct ltc4286_trig overtemp_trig;

	const struct device *dev;
	atomic_t trig_flags;

#if defined(CONFIG_LTC4286_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_LTC4286_THREAD_STACK_SIZE);
	struct k_sem gpio_sem;
	struct k_thread thread;
#elif defined(CONFIG_LTC4286_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif /* CONFIG_LTC4286_TRIGGER */
};

struct ltc4286_dev_config {
	struct i2c_dt_spec i2c_dev;
	struct ltc4286_gpio_specs gpios;
	struct ltc4286_fault_response_specs fault_responses;
	enum ltc4286_gpo_mode gpo_pin_output_select[LTC4286_NUM_GPIOS];
	uint32_t r_sense_uohms;
	uint16_t op_tmr;
	uint8_t avg_samples_cfg;
	uint8_t current_limit_cfg;
	uint8_t reboot_delay_cfg;
	uint8_t dev_gpio_reboot_pin;
	uint8_t dev_gpio_cmp_in_pin;
	uint8_t dev_gpio_alert_pin;
	bool reboot_init_fet_only;
	bool write_protect_en;
	bool vds_aux_en;
	bool vin_vout_en;
	bool avg_read_en;
	bool vrange_set;
	bool vpower_set;
	bool pmbus_1mbps_en;
	bool reset_fault_en;
	bool pg_latch_rst_ctrl;
	bool mass_write_en;
	bool ext_temp_en;
	bool deb_tmr_en;
	bool dev_gpio_reboot_en;
	bool dev_gpio_reboot_inv;
};

int ltc4286_cmd_read(const struct device *dev, uint8_t *code, uint8_t code_size,
		     uint8_t *val, uint8_t val_size);

int ltc4286_cmd_write(const struct device *dev, uint8_t code, uint8_t *val, uint8_t val_size);

#ifdef CONFIG_LTC4286_TRIGGER
int ltc4286_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int ltc4286_init_interrupts(const struct device *dev);
#endif /* CONFIG_LTC4286_TRIGGER */

#endif /* ZEPHYR_DRIVERS_SENSOR_LTC4286_LTC4286_H_ */
