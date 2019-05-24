/**
 * Copyright (c) 2015-2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file
 *
 * @brief Designware ADC header file
 */

#ifndef ZEPHYR_DRIVERS_ADC_ADC_INTEL_QUARK_SE_C1000_SS_H_
#define ZEPHYR_DRIVERS_ADC_ADC_INTEL_QUARK_SE_C1000_SS_H_

#include <zephyr/types.h>
#include <adc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ADC driver name.
 *
 * Name for the singleton instance of the ADC driver.
 *
 */
#define ADC_DRV_NAME "adc"

/**
 * Number of buffers.
 *
 * Number of reception buffers to be supported by the driver.
 */
#define BUFS_NUM 32

/* EAI ADC device registers */
#define     ADC_SET         (0x00)
#define     ADC_DIVSEQSTAT  (0x01)
#define     ADC_SEQ         (0x02)
#define     ADC_CTRL        (0x03)
#define     ADC_INTSTAT     (0x04)
#define     ADC_SAMPLE      (0x05)

/*  Sensor Subsystem Interrupt Routing Mask */
#define INT_SS_ADC_ERR_MASK             (0x400)
#define INT_SS_ADC_IRQ_MASK             (0x404)

/* ADC_DIVSEQSTAT register */
#define ADC_DIVSEQSTAT_CLK_RATIO_MASK	(0x1fffff)

/* ADC_SET register */
#define ADC_SET_POP_RX			BIT(31)
#define ADC_SET_FLUSH_RX		BIT(30)
#define ADC_SET_SEQ_MODE_MASK		BIT(13)
#define ADC_SET_INPUT_MODE_MASK		BIT(5)
#define ADC_SET_THRESHOLD_MASK		(0x3F000000)
#define ADC_SET_THRESHOLD_POS		24
#define ADC_SET_SEQ_ENTRIES_MASK	(0x003F0000)
#define ADC_SET_SEQ_ENTRIES_POS		16

/* ADC_CTRL register */
#define ADC_CTRL_CLR_DATA_A		BIT(16)
#define ADC_CTRL_SEQ_TABLE_RST		BIT(6)
#define ADC_CTRL_SEQ_PTR_RST		BIT(5)
#define ADC_CTRL_SEQ_START		BIT(4)
#define ADC_CTRL_CLK_ENABLE		BIT(2)
#define ADC_CTRL_INT_CLR_ALL		(0x000F0000)
#define ADC_CTRL_INT_MASK_ALL		(0x00000F00)

#define ADC_CTRL_ENABLE			BIT(1)
#define ADC_CTRL_DISABLE		(0x0)

/* ADC_INTSTAT register */
#define ADC_INTSTAT_SEQERROR		BIT(3)
#define ADC_INTSTAT_UNDERFLOW		BIT(2)
#define ADC_INTSTAT_OVERFLOW		BIT(1)
#define ADC_INTSTAT_DATA_A		BIT(0)

#define ADC_STATE_CLOSED        0
#define ADC_STATE_DISABLED      1
#define ADC_STATE_IDLE          2
#define ADC_STATE_SAMPLING      3
#define ADC_STATE_ERROR		4

#define ADC_CMD_RESET_CALIBRATION 2
#define ADC_CMD_START_CALIBRATION 3
#define ADC_CMD_LOAD_CALIBRATION 4

#define IO_ADC_SET_CLK_DIVIDER          (0x20)
#define IO_ADC_SET_CONFIG               (0x21)
#define IO_ADC_SET_SEQ_TABLE            (0x22)
#define IO_ADC_SET_SEQ_MODE             (0x23)
#define IO_ADC_SET_SEQ_STOP             (0x24)
#define IO_ADC_SET_RX_THRESHOLD         (0x25)

#define IO_ADC_INPUT_SINGLE_END       0
#define IO_ADC_INPUT_DIFF             1
#define IO_ADC_OUTPUT_PARAL           0
#define IO_ADC_OUTPUT_SERIAL          1
#define IO_ADC_CAPTURE_RISING_EDGE    0
#define IO_ADC_CAPTURE_FALLING_EDGE   1

#define IO_ADC_SEQ_MODE_SINGLESHOT    0
#define IO_ADC_SEQ_MODE_REPETITIVE    1

#define ENABLE_SSS_INTERRUPTS           ~(0x01 << 8)

#define ENABLE_ADC \
	( \
	 ADC_CTRL_CLK_ENABLE \
	 | ADC_CTRL_SEQ_TABLE_RST \
	 | ADC_CTRL_SEQ_PTR_RST \
	)

#define START_ADC_SEQ \
	( \
	   ADC_CTRL_SEQ_START \
	 | ADC_CTRL_ENABLE \
	 | ADC_CTRL_CLK_ENABLE \
	)

#define DW_CHANNEL_COUNT	19

/** mV = 3.3V*/
#define ADC_VREF 3300

/**
 *
 * @brief Converts ADC raw data into mV
 *
 * The ADC raw data readings are converted into mV:
 * result = (data * ADC_VREF) / (2^resolution).
 *
 * @param _data_ Raw data to be converted.
 * @param _resolution_ Resolution used during the data sampling.
 *
 * @return data read in mVolts.
 */
#define ss_adc_data_to_mv(_data_, _resolution_) \
	((_data_ * ADC_VREF) / (1 << _resolution_))

typedef void (*adc_intel_quark_se_c1000_ss_config_t)(void);
/** @brief ADC configuration
 * This structure defines the ADC configuration values
 * that define the ADC hardware instance and configuration.
 */
struct adc_config {
	/**Register base address for hardware registers.*/
	u32_t reg_base;
	/**IIO address for the IRQ mask register.*/
	u32_t reg_irq_mask;
	/**IIO address for the error mask register.*/
	u32_t reg_err_mask;
	/**Output mode*/
	u8_t  out_mode;
	/**Capture mode*/
	u8_t  capture_mode;
	/**Sequence mode*/
	u8_t  seq_mode;
	/**Serial delay*/
	u8_t  serial_dly;
	/**Sample width*/
	u8_t  sample_width;
	u8_t  padding[3];
	/**Clock ratio*/
	u32_t clock_ratio;
	/**Config handler*/
	adc_intel_quark_se_c1000_ss_config_t config_func;
};

/**@brief ADC information and data.
 *
 * This structure defines the data that will be used
 * during driver execution.
 */
struct adc_info {
	struct device *dev;
	struct adc_context ctx;
	u16_t *buffer;
	u32_t active_channels;
	u32_t channels;
	u32_t channel_id;

	/**Sequence entries' array*/
	const struct adc_sequence *entries;
	/**State of execution of the driver*/
	u8_t  state;
	/**Sequence size*/
	u8_t seq_size;
#ifdef CONFIG_ADC_INTEL_QUARK_SE_C1000_SS_CALIBRATION
	/**Calibration value*/
	u8_t calibration_value;
#endif

};

#ifdef __cplusplus
}
#endif

#endif  /*  ZEPHYR_DRIVERS_ADC_ADC_INTEL_QUARK_SE_C1000_SS_H_ */
