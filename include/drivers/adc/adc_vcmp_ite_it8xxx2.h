#ifndef _ADC_VCMP_ITE_IT8XXX2_H_
#define _ADC_VCMP_ITE_IT8XXX2_H_

#include <device.h>

enum vcmp_scan_period {
	VCMP_SCAN_PERIOD_100US = 0x10,
	VCMP_SCAN_PERIOD_200US = 0x20,
	VCMP_SCAN_PERIOD_400US = 0x30,
	VCMP_SCAN_PERIOD_600US = 0x40,
	VCMP_SCAN_PERIOD_800US = 0x50,
	VCMP_SCAN_PERIOD_1MS   = 0x60,
	VCMP_SCAN_PERIOD_1_5MS = 0x70,
	VCMP_SCAN_PERIOD_2MS   = 0x80,
	VCMP_SCAN_PERIOD_2_5MS = 0x90,
	VCMP_SCAN_PERIOD_3MS   = 0xA0,
	VCMP_SCAN_PERIOD_4MS   = 0xB0,
	VCMP_SCAN_PERIOD_5MS   = 0xC0,
};

enum adc_vcmp_ite_it3xxx2_control_param {
	/* Selects ADC channel to be used for measurement */
	ADC_VCMP_ITE_IT8XXX2_PARAM_CSELL,
	/* Sets relation between measured value and assetion threshold value.*/
	ADC_VCMP_ITE_IT8XXX2_PARAM_TMOD,
	/* Sets the threshol value to which measured data is compared. */
	ADC_VCMP_ITE_IT8XXX2_PARAM_THRDAT,
	/* Sets worker queue thread to be notified */
	ADC_VCMP_ITE_IT8XXX2_PARAM_WORK,

	ADC_VCMP_ITE_IT8XXX2_PARAM_MAX,
};

enum adc_vcmp_ite_it3xxx2_trigger_mode {
	VCMP_TRIGGER_MODE_LESS_OR_EQUAL,
	VCMP_TRIGGER_MODE_GREATER
};

struct adc_vcmp_it8xxx2_vcmp_control_t {
	/* Voltage comparator control parameter */
	enum adc_vcmp_ite_it3xxx2_control_param param;
	/* Parameter value */
	uint32_t val;
};

/**
 * @brief Set ADC voltage comparator parameter.
 *
 * @note This function is available only if @kconfig{CONFIG}
 * is selected.
 *
 * @param dev       Pointer to the device structure for the driver instance.
 * @param th_sel    Threshold selected.
 * @param control   Pointer of control parameter structure.
 *                  See struct adc_npcx_threshold_control_t for supported
 *                  parameters.
 *
 * @returns 0 on success, negative error code otherwise.
 */
int adc_vcmp_it8xxx2_ctrl_set_param(const struct device *dev,
				    const uint8_t vcmp,
				    const struct adc_vcmp_it8xxx2_vcmp_control_t
				    *control);

/**
 * @brief Enables/Disables ADC threshold interruption.
 *
 * @note This function is available only if @kconfig{CONFIG_ADC_CMP_NPCX}
 * is selected.
 *
 * @param dev       Pointer to the device structure for the driver instance.
 * @param th_sel    Threshold selected.
 * @param enable    Enable or disables threshold interruption.
 *
 * @returns 0 on success, negative error code otherwise.
 *            all parameters must be configure prior enabling threshold
 *            interruption, otherwhise error will be returned.
 */
int adc_vcmp_it8xxx2_ctrl_enable(const struct device *dev, uint8_t vcmp,
				 const bool enable);


int adc_vcmp_it8xxx2_set_scan_period(const struct device *dev,
				     enum vcmp_scan_period scan_period);

#endif /* _ADC_VCMP_IT8XXX2_H_ */
