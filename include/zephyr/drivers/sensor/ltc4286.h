/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of LTC4286 sensor
 * @ingroup ltc4286_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_LTC4286_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_LTC4286_H_

/**
 * @brief Analog Devices LTC4286 Hot Swap controller and power monitor
 * @defgroup ltc4286_interface LTC4286
 * @ingroup sensor_interface_ext
 * @{
 */

#include <zephyr/types.h>
#include <zephyr/drivers/sensor.h>

/**
 * @brief Custom sensor channels for LTC4286
 */
enum ltc4286_sensor_chan {
	SENSOR_CHAN_LTC4286_VIN = SENSOR_CHAN_PRIV_START, /**< Input voltage (VIN) channel */
	SENSOR_CHAN_LTC4286_ADC,                          /**< ADC configuration channel */
	SENSOR_CHAN_LTC4286_CONFIG,                       /**< Device configuration channel */
	SENSOR_CHAN_LTC4286_END,                          /**< End marker for sensor channels */
};

/**
 * @brief Trigger channels for LTC4286 interrupt sources
 */
enum ltc4286_trig_chan {
	SENSOR_CHAN_LTC4286_TRIG_POWER_GOOD =
		SENSOR_CHAN_LTC4286_END,     /**< Power good status channel */
	SENSOR_CHAN_LTC4286_TRIG_COMPARATOR, /**< Comparator output channel */
	SENSOR_CHAN_LTC4286_TRIG_ALERT,      /**< Alert status channel */
	SENSOR_CHAN_LTC4286_TRIG_END,        /**< End marker for trigger channels */
};

/**
 * @brief Status register channels for LTC4286
 */
enum ltc4286_status_chan {
	SENSOR_CHAN_LTC4286_STATUS =
		SENSOR_CHAN_LTC4286_TRIG_END,       /**< General status word channel */
	SENSOR_CHAN_LTC4286_STATUS_VOUT,            /**< Output voltage status channel */
	SENSOR_CHAN_LTC4286_STATUS_IOUT,            /**< Output current status channel */
	SENSOR_CHAN_LTC4286_STATUS_INPUT,           /**< Input voltage/power status channel */
	SENSOR_CHAN_LTC4286_STATUS_TEMP,            /**< Temperature status channel */
	SENSOR_CHAN_LTC4286_STATUS_CML,             /**< Communication/command status channel */
	SENSOR_CHAN_LTC4286_STATUS_OTHER,           /**< Other status conditions channel */
	SENSOR_CHAN_LTC4286_STATUS_MFR_SPEC,        /**< Manufacturer-specific status channel */
	SENSOR_CHAN_LTC4286_STATUS_MFR_SYS_STATUS1, /**< Manufacturer system status 1 channel */
	SENSOR_CHAN_LTC4286_STATUS_MFR_SYS_STATUS2, /**< Manufacturer system status 2 channel */
	SENSOR_CHAN_LTC4286_STATUS_END,             /**< End marker for status channels */
};

/**
 * @brief Manufacturer-specific configuration channels for LTC4286
 */
enum ltc4286_mfr_chan {
	/** Manufacturer configuration channel */
	SENSOR_CHAN_LTC4286_MFR_CONFIG = SENSOR_CHAN_LTC4286_STATUS_END,
	SENSOR_CHAN_LTC4286_MFR_FLT_CONFIG,       /**< Fault configuration channel */
	SENSOR_CHAN_LTC4286_MFR_PMB_STATUS,       /**< PMBus status channel */
	SENSOR_CHAN_LTC4286_MFR_PADS_LIVE_STATUS, /**< Pads live status channel */
	SENSOR_CHAN_LTC4286_MFR_COMMON,           /**< Common manufacturer settings channel */
	SENSOR_CHAN_LTC4286_MFR_REBOOT_CONTROL,   /**< Reboot control channel */
	SENSOR_CHAN_LTC4286_MFR_SHUTDOWN_CAUSE,   /**< Shutdown cause channel */
	SENSOR_CHAN_LTC4286_MFR_END,              /**< End marker for manufacturer channels */
};

/**
 * @brief Fault response configuration channels for LTC4286
 */
enum ltc4286_fault_response_chan {
	SENSOR_CHAN_LTC4286_FAULT_RESP_FET = SENSOR_CHAN_LTC4286_MFR_END, /**< FET fault response */
	SENSOR_CHAN_LTC4286_FAULT_RESP_END,                               /**< End marker */
};

/**
 * @brief Configuration attributes for LTC4286
 */
enum ltc4286_config_attr {
	/** Device operation mode attribute */
	SENSOR_ATTR_LTC4286_CONFIG_OPERATION = SENSOR_ATTR_PRIV_START,
	SENSOR_ATTR_LTC4286_CONFIG_CLEAR_FAULTS,  /**< Clear faults command attribute */
	SENSOR_ATTR_LTC4286_CONFIG_WRITE_PROTECT, /**< Write protection control attribute */
	SENSOR_ATTR_LTC4286_CONFIG_END,           /**< End marker */
};

/**
 * @brief General status word attributes for LTC4286
 */
enum ltc4286_status_attr {
	/** Output voltage status attribute */
	SENSOR_ATTR_LTC4286_STATUS_VOUT = SENSOR_ATTR_LTC4286_CONFIG_END,
	SENSOR_ATTR_LTC4286_STATUS_IOUT,      /**< Output current status attribute */
	SENSOR_ATTR_LTC4286_STATUS_INPUT,     /**< Input voltage/power status attribute */
	SENSOR_ATTR_LTC4286_STATUS_MFRSPEC,   /**< Manufacturer-specific status attribute */
	SENSOR_ATTR_LTC4286_STATUS_PG,        /**< Power good status attribute */
	SENSOR_ATTR_LTC4286_STATUS_OTHER,     /**< Other status conditions attribute */
	SENSOR_ATTR_LTC4286_STATUS_UNKNOWN,   /**< Unknown fault/warning status attribute */
	SENSOR_ATTR_LTC4286_STATUS_BUSY,      /**< Device busy status attribute */
	SENSOR_ATTR_LTC4286_STATUS_OFF,       /**< Device off status attribute */
	SENSOR_ATTR_LTC4286_STATUS_IOUT_OC,   /**< Output current overcurrent fault status */
	SENSOR_ATTR_LTC4286_STATUS_VIN_UV,    /**< Input voltage undervoltage fault status */
	SENSOR_ATTR_LTC4286_STATUS_TEMP,      /**< Temperature status attribute */
	SENSOR_ATTR_LTC4286_STATUS_CML,       /**< Communication/command status attribute */
	SENSOR_ATTR_LTC4286_STATUS_NOT_CLEAR, /**< Status not cleared attribute */
	SENSOR_ATTR_LTC4286_STATUS_END,       /**< End marker */
};

/**
 * @brief Output voltage status attributes for LTC4286
 */
enum ltc4286_status_vout_attr {
	/** Output voltage overvoltage status */
	SENSOR_ATTR_LTC4286_STATUS_VOUT_OV = SENSOR_ATTR_LTC4286_STATUS_END,
	SENSOR_ATTR_LTC4286_STATUS_VOUT_UV,  /**< Output voltage undervoltage status */
	SENSOR_ATTR_LTC4286_STATUS_VOUT_END, /**< End marker */
};

/**
 * @brief Output current status attributes for LTC4286
 */
enum ltc4286_status_iout_attr {
	/** Output current overcurrent fault */
	SENSOR_ATTR_LTC4286_STATUS_IOUT_OC_FAULT = SENSOR_ATTR_LTC4286_STATUS_VOUT_END,
	SENSOR_ATTR_LTC4286_STATUS_IOUT_OC_WARN, /**< Output current overcurrent warning */
	SENSOR_ATTR_LTC4286_STATUS_IOUT_END,     /**< End marker */
};

/**
 * @brief Input voltage/power status attributes for LTC4286
 */
enum ltc4286_status_input_attr {
	/** Input voltage overvoltage fault */
	SENSOR_ATTR_LTC4286_STATUS_VIN_OV_FAULT = SENSOR_ATTR_LTC4286_STATUS_IOUT_END,
	SENSOR_ATTR_LTC4286_STATUS_VIN_OV_WARN,  /**< Input voltage overvoltage warning */
	SENSOR_ATTR_LTC4286_STATUS_VIN_UV_FAULT, /**< Input voltage undervoltage fault */
	SENSOR_ATTR_LTC4286_STATUS_VIN_UV_WARN,  /**< Input voltage undervoltage warning */
	SENSOR_ATTR_LTC4286_STATUS_PIN_OP_WARN,  /**< Output power warning */
	SENSOR_ATTR_LTC4286_STATUS_INPUT_END,    /**< End marker */
};

/**
 * @brief Temperature status attributes for LTC4286
 */
enum ltc4286_status_temp_attr {
	/** Temperature overtemperature fault */
	SENSOR_ATTR_LTC4286_STATUS_TEMP_OT_FAULT = SENSOR_ATTR_LTC4286_STATUS_INPUT_END,
	SENSOR_ATTR_LTC4286_STATUS_TEMP_OT_WARN, /**< Temperature overtemperature warning */
	SENSOR_ATTR_LTC4286_STATUS_TEMP_UT_WARN, /**< Temperature undertemperature warning */
	SENSOR_ATTR_LTC4286_STATUS_TEMP_END,     /**< End marker */
};

/**
 * @brief Communication/command status attributes for LTC4286
 */
enum ltc4286_status_cml_attr {
	/** Invalid command status */
	SENSOR_ATTR_LTC4286_STATUS_CML_BAD_CMD = SENSOR_ATTR_LTC4286_STATUS_TEMP_END,
	SENSOR_ATTR_LTC4286_STATUS_CML_BAD_DATA,     /**< Invalid data status */
	SENSOR_ATTR_LTC4286_STATUS_CML_BAD_PEC_FAIL, /**< PEC (Packet Error Code) check failed */
	SENSOR_ATTR_LTC4286_STATUS_CML_MISC_FAULT,   /**< Miscellaneous communication fault */
	SENSOR_ATTR_LTC4286_STATUS_CML_END,          /**< End marker */
};

/**
 * @brief Manufacturer-specific status attributes for LTC4286
 */
enum ltc4286_status_mfr_spec_attr {
	/** Enable pin state changed */
	SENSOR_ATTR_LTC4286_STATUS_MFR_SPEC_EN_CHANGED = SENSOR_ATTR_LTC4286_STATUS_CML_END,
	SENSOR_ATTR_LTC4286_STATUS_MFR_SPEC_TSD_FAULT,     /**< Thermal shutdown fault */
	SENSOR_ATTR_LTC4286_STATUS_MFR_SPEC_VDD_UVLO,      /**< VDD undervoltage lockout */
	SENSOR_ATTR_LTC4286_STATUS_MFR_SPEC_PIN_OP2_FAULT, /**< Output power 2 fault */
	SENSOR_ATTR_LTC4286_STATUS_MFR_SPEC_PIN_OP1_FAULT, /**< Output power 1 fault */
	SENSOR_ATTR_LTC4286_STATUS_MFR_SPEC_FET_BAD_FAULT, /**< FET bad fault */
	SENSOR_ATTR_LTC4286_STATUS_MFR_SPEC_END,           /**< End marker */
};

/**
 * @brief Other status attributes for LTC4286
 */
enum ltc4286_status_other_attr {
	/** First alert occurrence */
	SENSOR_ATTR_LTC4286_STATUS_OTHER_FIRST_ALERT = SENSOR_ATTR_LTC4286_STATUS_MFR_SPEC_END,
	SENSOR_ATTR_LTC4286_STATUS_OTHER_END, /**< End marker */
};

/**
 * @brief Manufacturer system status attributes for LTC4286
 */
enum ltc4286_status_mfr_sys_status_attr {
	/** Alert pin asserted */
	SENSOR_ATTR_LTC4286_STATUS_MFR_SYS_STAT1_ALERT = SENSOR_ATTR_LTC4286_STATUS_OTHER_END,
	SENSOR_ATTR_LTC4286_STATUS_MFR_SYS_STAT1_L_ALERT, /**< Latched alert status */
	SENSOR_ATTR_LTC4286_STATUS_MFR_SYS_POWER_LOSS,    /**< Power loss detected */
	SENSOR_ATTR_LTC4286_STATUS_MFR_SYS_RESET_DONE,    /**< Reset completed */
	SENSOR_ATTR_LTC4286_STATUS_MFR_SYS_AVG_DONE,      /**< Averaging completed */
	SENSOR_ATTR_LTC4286_STATUS_MFR_SYS_ADC_CONV,      /**< ADC conversion completed */
	/** System status 2 register has bits set */
	SENSOR_ATTR_LTC4286_STATUS_MFR_SYS_STAT2_SET,
	SENSOR_ATTR_LTC4286_STATUS_MFR_SYS_POWER_FAILED_WARN, /**< Power failed warning */
	SENSOR_ATTR_LTC4286_STATUS_MFR_SYS_FET_SHORT_WARN,    /**< FET short circuit warning */
	SENSOR_ATTR_LTC4286_STATUS_MFR_SYS_VDS_UV_WARN,       /**< VDS undervoltage warning */
	SENSOR_ATTR_LTC4286_STATUS_MFR_SYS_VDS_OV_WARN,       /**< VDS overvoltage warning */
	SENSOR_ATTR_LTC4286_STATUS_MFR_SYS_IOUT_UC_WARN, /**< Output current undercurrent warning */
	SENSOR_ATTR_LTC4286_STATUS_MFR_SYS_PIN_UP_WARN,  /**< Output power warning */
	SENSOR_ATTR_LTC4286_STATUS_MFR_SYS_END,          /**< End marker */
};

/**
 * @brief ADC configuration attributes for LTC4286
 */
enum ltc4286_adc_attr {
	/** VDS measurement selection */
	SENSOR_ATTR_LTC4286_ADC_VDS_SELECT = SENSOR_ATTR_LTC4286_STATUS_MFR_SYS_END,
	SENSOR_ATTR_LTC4286_ADC_VIN_VOUT_SELECT, /**< VIN/VOUT measurement selection */
	SENSOR_ATTR_LTC4286_ADC_DISP_AVG,        /**< Display averaged values */
	SENSOR_ATTR_LTC4286_ADC_AVG_SELECT,      /**< Averaging mode selection */
	SENSOR_ATTR_LTC4286_ADC_END,             /**< End marker */
};

/**
 * @brief Fault response configuration attributes for LTC4286
 */
enum ltc4286_fault_response_attr {
	SENSOR_ATTR_LTC4286_FAULT_RESPONSE =
		SENSOR_ATTR_LTC4286_ADC_END,   /**< Fault response mode */
	SENSOR_ATTR_LTC4286_FAULT_RETRY,       /**< Fault retry configuration */
	SENSOR_ATTR_LTC4286_FAULT_OP_TIMER,    /**< Fault operation timer */
	SENSOR_ATTR_LTC4286_FAULT_OV_RESPONSE, /**< Overvoltage fault response (VIN channel) */
	SENSOR_ATTR_LTC4286_FAULT_OV_RETRY,    /**< Overvoltage fault retry (VIN channel) */
	SENSOR_ATTR_LTC4286_FAULT_UV_RESPONSE, /**< Undervoltage fault response (VIN channel) */
	SENSOR_ATTR_LTC4286_FAULT_UV_RETRY,    /**< Undervoltage fault retry (VIN channel) */
	SENSOR_ATTR_LTC4286_FAULT_END,         /**< End marker */
};

/**
 * @brief Manufacturer fault configuration attributes for LTC4286
 */
enum ltc4286_mfr_flt_attr {
	/** Overpower timeout to fault */
	SENSOR_ATTR_LTC4286_MFR_FLT_OP_TO_FAULT = SENSOR_ATTR_LTC4286_FAULT_END,
	SENSOR_ATTR_LTC4286_MFR_FLT_OT_TO_FAULT, /**< Overtemperature timeout to fault */
	SENSOR_ATTR_LTC4286_MFR_FLT_END,         /**< End marker */
};

/**
 * @brief Reboot control attributes for LTC4286
 */
enum ltc4286_reboot_control_attr {
	/** Initialize reboot sequence */
	SENSOR_ATTR_LTC4286_REBOOT_CONTROL_INIT = SENSOR_ATTR_LTC4286_MFR_FLT_END,
	SENSOR_ATTR_LTC4286_REBOOT_CONTROL_DELAY, /**< Reboot delay time */
	SENSOR_ATTR_LTC4286_REBOOT_CONTROL_END,   /**< End marker */
};

/**
 * @brief Manufacturer configuration attributes for LTC4286
 */
enum ltc4286_mfr_config_attr {
	/** Current limit configuration */
	SENSOR_ATTR_LTC4286_MFR_CONFIG_ILIM = SENSOR_ATTR_LTC4286_REBOOT_CONTROL_END,
	SENSOR_ATTR_LTC4286_MFR_CONFIG_VRANGE_SEL,          /**< Voltage range selection */
	SENSOR_ATTR_LTC4286_MFR_CONFIG_VPWR_SEL,            /**< Power voltage selection */
	SENSOR_ATTR_LTC4286_MFR_CONFIG_PMBUS_1MBIT,         /**< PMBus 1Mbit speed mode */
	SENSOR_ATTR_LTC4286_MFR_CONFIG_RESET_FAULT,         /**< Reset fault condition */
	SENSOR_ATTR_LTC4286_MFR_CONFIG_PWR_GOOD_RESET_CTRL, /**< Power good reset control */
	SENSOR_ATTR_LTC4286_MFR_CONFIG_MASS_WRITE,          /**< Mass write enable */
	SENSOR_ATTR_LTC4286_MFR_CONFIG_EXT_TEMP,            /**< External temperature sensor */
	SENSOR_ATTR_LTC4286_MFR_CONFIG_DEBOUNCE_TIMER,      /**< Input debounce timer */
	SENSOR_ATTR_LTC4286_MFR_CONFIG_END,                 /**< End marker */
};

/**
 * @brief Status word bit field definitions for LTC4286
 */
enum ltc4286_status_word_field {
	LTC4286_STATUS_WORD_VOUT_BIT = BIT(15),    /**< Output voltage fault or warning */
	LTC4286_STATUS_WORD_IOUT_BIT = BIT(14),    /**< Output current fault or warning */
	LTC4286_STATUS_WORD_INPUT_BIT = BIT(13),   /**< Input voltage fault or warning */
	LTC4286_STATUS_WORD_MFRSPEC_BIT = BIT(12), /**< Manufacturer-specific fault or warning */
	LTC4286_STATUS_WORD_FET_UV_THRES_BIT = BIT(11), /**< FET undervoltage threshold fault */
	LTC4286_STATUS_WORD_OTHER_BIT = BIT(9),         /**< Other fault or warning */
	LTC4286_STATUS_WORD_UNKNOWN_BIT = BIT(8),       /**< Unknown fault or warning */
	LTC4286_STATUS_WORD_BUSY_BIT = BIT(7),          /**< Device is busy processing command */
	LTC4286_STATUS_WORD_OFF_BIT = BIT(6),           /**< Device is in off state */
	LTC4286_STATUS_WORD_IOUT_OTHRES_BIT =
		BIT(4), /**< Output current overcurrent threshold fault */
	LTC4286_STATUS_WORD_VIN_UTHRES_BIT =
		BIT(3),                           /**< Input voltage undervoltage threshold fault */
	LTC4286_STATUS_WORD_TEMP_BIT = BIT(2),    /**< Temperature fault or warning */
	LTC4286_STATUS_WORD_COMLINE_BIT = BIT(1), /**< Communication fault */
	LTC4286_STATUS_WORD_NOT_CLEAR_BIT = BIT(0), /**< Status not cleared */
};

/**
 * @brief Output voltage status bit field definitions for LTC4286
 */
enum ltc4286_status_vout_field {
	LTC4286_VOUT_UV_WARNING_BIT = BIT(5), /**< Output voltage undervoltage warning */
	LTC4286_VOUT_OV_WARNING_BIT = BIT(6), /**< Output voltage overvoltage warning */
};

/**
 * @brief Output current status bit field definitions for LTC4286
 */
enum ltc4286_status_iout_field {
	LTC4286_IOUT_OC_WARNING_BIT = BIT(5), /**< Output current overcurrent warning */
	LTC4286_IOUT_OC_FAULT_BIT = BIT(7),   /**< Output current overcurrent fault */
};

/**
 * @brief Input voltage/power status bit field definitions for LTC4286
 */
enum ltc4286_status_input_field {
	LTC4286_PIN_OP_WARNING_BIT = BIT(0), /**< Output power warning */
	LTC4286_VIN_UV_FAULT_BIT = BIT(4),   /**< Input voltage undervoltage fault */
	LTC4286_VIN_UV_WARNING_BIT = BIT(5), /**< Input voltage undervoltage warning */
	LTC4286_VIN_OV_WARNING_BIT = BIT(6), /**< Input voltage overvoltage warning */
	LTC4286_VIN_OV_FAULT_BIT = BIT(7),   /**< Input voltage overvoltage fault */
};

/**
 * @brief Temperature status bit field definitions for LTC4286
 */
enum ltc4286_status_temp_field {
	LTC4286_TEMP_UT_WARNING_BIT = BIT(5), /**< Temperature undertemperature warning */
	LTC4286_TEMP_OT_WARNING_BIT = BIT(6), /**< Temperature overtemperature warning */
	LTC4286_TEMP_OT_FAULT_BIT = BIT(7),   /**< Temperature overtemperature fault */
};

/**
 * @brief Communication/command status bit field definitions for LTC4286
 */
enum ltc4286_status_cml_field {
	LTC4286_MISC_FAULT_BIT = BIT(1), /**< Miscellaneous communication fault */
	LTC4286_PEC_FAILED_BIT = BIT(5), /**< PEC (Packet Error Code) check failed */
	LTC4286_BAD_DATA_BIT = BIT(6),   /**< Invalid data received */
	LTC4286_BAD_CMD_BIT = BIT(7),    /**< Invalid command received */
};

/**
 * @brief Other status bit field definitions for LTC4286
 */
enum ltc4286_status_other_field {
	LTC4286_OTHER_FIRST_ALERT_BIT = BIT(0), /**< First alert occurrence */
};

/**
 * @brief Manufacturer-specific status bit field definitions for LTC4286
 */
enum ltc4286_status_mfr_spec_field {
	LTC4286_MFR_SPEC_FET_BAD_FAULT_BIT = BIT(2), /**< FET bad fault */
	LTC4286_MFR_SPEC_PIN_OP1_FAULT_BIT = BIT(3), /**< Output power 1 fault */
	LTC4286_MFR_SPEC_PIN_OP2_FAULT_BIT = BIT(4), /**< Output power 2 fault */
	LTC4286_MFR_SPEC_VDD_UVLO_BIT = BIT(5),      /**< VDD undervoltage lockout */
	LTC4286_MFR_SPEC_TSD_FAULT_BIT = BIT(6),     /**< Thermal shutdown fault */
	LTC4286_MFR_SPEC_EN_CHANGED_BIT = BIT(7),    /**< Enable pin state changed */
};

/**
 * @brief Manufacturer system status 1 bit field definitions for LTC4286
 */
enum ltc4286_mfr_sys_status1_field {
	LTC4286_MFR_SYS_STATUS1_ADC_CONV_BIT = BIT(6),     /**< ADC conversion completed */
	LTC4286_MFR_SYS_STATUS1_AVERAGE_DONE_BIT = BIT(7), /**< Averaging completed */
	LTC4286_MFR_SYS_STATUS1_RESET_DONE_BIT = BIT(10),  /**< Reset completed */
	LTC4286_MFR_SYS_STATUS1_POWER_LOSS_BIT = BIT(11),  /**< Power loss detected */
	LTC4286_MFR_SYS_STATUS1_L_ALERT_BIT = BIT(14),     /**< Latched alert status */
	LTC4286_MFR_SYS_STATUS1_ALERT_BIT = BIT(15),       /**< Alert pin asserted */
};

/**
 * @brief Manufacturer system status 2 bit field definitions for LTC4286
 */
enum ltc4286_mfr_sys_status2_field {
	LTC4286_MFR_SYS_STATUS2_PIN_UP_WARNING_BIT = BIT(0), /**< Output power warning */
	/** Output current undercurrent warning */
	LTC4286_MFR_SYS_STATUS2_IOUT_UC_WARNING_BIT = BIT(1),
	LTC4286_MFR_SYS_STATUS2_VDS_UV_WARNING_BIT = BIT(2),     /**< VDS undervoltage warning */
	LTC4286_MFR_SYS_STATUS2_VDS_OV_WARNING_BIT = BIT(3),     /**< VDS overvoltage warning */
	LTC4286_MFR_SYS_STATUS2_FET_SHORT_WARNING_BIT = BIT(14), /**< FET short circuit warning */
	LTC4286_MFR_SYS_STATUS2_POWER_FAILED_WARNING_BIT = BIT(15), /**< Power failed warning */
};

/**
 * @brief Alert mask types for triggering alerts
 */
enum ltc4286_trig_alert_mask {
	LTC4286_ALERT_STATUS_WORD_MASK = 0xD0, /**< Status byte low mask (0xD0) */
	LTC4286_ALERT_STATUS_VOUT_MASK = 0xD2, /**< Output voltage status mask (0xD2) */
	LTC4286_ALERT_STATUS_IOUT_MASK,        /**< Output current status mask */
	LTC4286_ALERT_STATUS_INPUT_MASK,       /**< Input voltage/power status mask */
	LTC4286_ALERT_STATUS_TEMP_MASK,        /**< Temperature status mask */
	LTC4286_ALERT_STATUS_COMMS_MASK,       /**< Communication status mask */
	LTC4286_ALERT_STATUS_SPEC_MASK = 0xD8, /**< Manufacturer-specific status mask (0xD8) */
	LTC4286_ALERT_STATUS_SYS1_MASK = 0xDA, /**< System status 1 mask (0xDA) */
	LTC4286_ALERT_STATUS_SYS2_MASK = 0xDC, /**< System status 2 mask (0xDC) */
};

/**
 * @brief Status word alert bit field definitions for LTC4286
 */
enum ltc4286_status_alert {
	LTC4286_ALERT_STATUS_BUSY_ALERT_BIT = BIT(7), /**< Device busy alert enable */
};

/**
 * @brief Output voltage alert bit field definitions for LTC4286
 */
enum ltc4286_vout_alert {
	/** Output voltage undervoltage warning alert enable */
	LTC4286_ALERT_VOUT_UV_WARNING_ALERT_BIT = BIT(5),
	/** Output voltage overvoltage warning alert enable */
	LTC4286_ALERT_VOUT_OV_WARNING_ALERT_BIT = BIT(6),
};

/**
 * @brief Output current alert bit field definitions for LTC4286
 */
enum ltc4286_iout_alert {
	/** Output current overcurrent warning alert enable */
	LTC4286_ALERT_IOUT_OC_WARNING_ALERT_BIT = BIT(5),
	/** Output current overcurrent fault alert enable */
	LTC4286_ALERT_IOUT_OC_FAULT_ALERT_BIT = BIT(7),
};

/**
 * @brief Input voltage/power alert bit field definitions for LTC4286
 */
enum ltc4286_input_alert {
	/** Output power warning alert enable */
	LTC4286_ALERT_PIN_OP_WARNING_ALERT_BIT = BIT(0),
	/** Input voltage undervoltage fault alert enable */
	LTC4286_ALERT_VIN_UV_FAULT_ALERT_BIT = BIT(4),
	/** Input voltage undervoltage warning alert enable */
	LTC4286_ALERT_VIN_UV_WARNING_ALERT_BIT = BIT(5),
	/** Input voltage overvoltage warning alert enable */
	LTC4286_ALERT_VIN_OV_WARNING_ALERT_BIT = BIT(6),
	/** Input voltage overvoltage fault alert enable */
	LTC4286_ALERT_VIN_OV_FAULT_ALERT_BIT = BIT(7),
};

/**
 * @brief Temperature alert bit field definitions for LTC4286
 */
enum ltc4286_temp_alert {
	/** Temperature undertemperature warning alert enable */
	LTC4286_ALERT_TEMP_UT_WARNING_ALERT_BIT = BIT(5),
	/** Temperature overtemperature warning alert enable */
	LTC4286_ALERT_TEMP_OT_WARNING_ALERT_BIT = BIT(6),
	/** Temperature overtemperature fault alert enable */
	LTC4286_ALERT_TEMP_OT_FAULT_ALERT_BIT = BIT(7),
};

/**
 * @brief Communication/command alert bit field definitions for LTC4286
 */
enum ltc4286_cml_alert {
	/** Miscellaneous communication fault alert enable */
	LTC4286_ALERT_MISC_FAULT_ALERT_BIT = BIT(1),
	/** PEC check failed alert enable */
	LTC4286_ALERT_PEC_FAILED_ALERT_BIT = BIT(5),
	/** Invalid data alert enable */
	LTC4286_ALERT_BAD_DATA_ALERT_BIT = BIT(6),
	/** Invalid command alert enable */
	LTC4286_ALERT_BAD_CMD_ALERT_BIT = BIT(7),
};

/**
 * @brief Manufacturer-specific alert bit field definitions for LTC4286
 */
enum ltc4286_mfr_spec_alert {
	/** FET bad fault alert enable */
	LTC4286_ALERT_MFR_SPEC_FET_BAD_FAULT_ALERT_BIT = BIT(2),
	/** Output power 1 fault alert enable */
	LTC4286_ALERT_MFR_SPEC_PIN_OP1_FAULT_ALERT_BIT = BIT(3),
	/** Output power 2 fault alert enable */
	LTC4286_ALERT_MFR_SPEC_PIN_OP2_FAULT_ALERT_BIT = BIT(4),
	/** VDD undervoltage lockout alert enable */
	LTC4286_ALERT_MFR_SPEC_VDD_UVLO_ALERT_BIT = BIT(5),
	/** Thermal shutdown fault alert enable */
	LTC4286_ALERT_MFR_SPEC_TSD_FAULT_ALERT_BIT = BIT(6),
	/** Enable pin state changed alert enable */
	LTC4286_ALERT_MFR_SPEC_EN_CHANGED_ALERT_BIT = BIT(7),
};

/**
 * @brief Manufacturer system status 1 alert bit field definitions for LTC4286
 */
enum ltc4286_mfr_sys_status1_alert {
	/** ADC conversion completed alert enable */
	LTC4286_ALERT_MFR_SYS_STATUS1_ADC_CONV_ALERT_BIT = BIT(6),
	/** Averaging completed alert enable */
	LTC4286_ALERT_MFR_SYS_STATUS1_AVERAGE_DONE_ALERT_BIT = BIT(7),
	/** Reset completed alert enable */
	LTC4286_ALERT_MFR_SYS_STATUS1_RESET_DONE_ALERT_BIT = BIT(10),
	/** Power loss detected alert enable */
	LTC4286_ALERT_MFR_SYS_STATUS1_POWER_LOSS_ALERT_BIT = BIT(11),
};

/**
 * @brief Manufacturer system status 2 alert bit field definitions for LTC4286
 */
enum ltc4286_mfr_sys_status2_alert {
	/** Output power warning alert enable */
	LTC4286_ALERT_MFR_SYS_STATUS2_PIN_UP_WARNING_ALERT_BIT = BIT(0),
	/** Output current undercurrent warning alert enable */
	LTC4286_ALERT_MFR_SYS_STATUS2_IOUT_UC_WARNING_ALERT_BIT = BIT(1),
	/** VDS undervoltage warning alert enable */
	LTC4286_ALERT_MFR_SYS_STATUS2_VDS_UV_WARNING_ALERT_BIT = BIT(2),
	/** VDS overvoltage warning alert enable */
	LTC4286_ALERT_MFR_SYS_STATUS2_VDS_OV_WARNING_ALERT_BIT = BIT(3),
	/** FET short circuit warning alert enable */
	LTC4286_ALERT_MFR_SYS_STATUS2_FET_SHORT_WARNING_ALERT_BIT = BIT(14),
	/** Power failed warning alert enable */
	LTC4286_ALERT_MFR_SYS_STATUS2_POWER_FAILED_WARNING_ALERT_BIT = BIT(15),
};

/**
 * @brief Enable alert mask for LTC4286
 *
 * Enables the specified alert mask to trigger alerts when the corresponding
 * status condition occurs.
 *
 * @param dev Pointer to the LTC4286 device structure
 * @param alert_reg Alert mask type to enable
 * @param alert_bit Alert bit to enable
 *
 * @retval 0 on success
 * @retval -errno negative errno code on failure
 */
int ltc4286_enable_alert_mask(const struct device *dev, enum ltc4286_trig_alert_mask alert_reg,
			      uint16_t alert_bit);

/**
 * @brief Disable alert mask for LTC4286
 *
 * Disables the specified alert mask to prevent alerts from being triggered
 * when the corresponding status condition occurs.
 *
 * @param dev Pointer to the LTC4286 device structure
 * @param alert_reg Alert mask type to enable
 * @param alert_bit Alert bit to enable
 *
 * @retval 0 on success
 * @retval -errno negative errno code on failure
 */
int ltc4286_disable_alert_mask(const struct device *dev, enum ltc4286_trig_alert_mask alert_reg,
			       uint16_t alert_bit);

/**
 * @}
 */

#endif /* ZEPHYR_DRIVERS_SENSOR_LTC4286_LTC4286_H_ */
