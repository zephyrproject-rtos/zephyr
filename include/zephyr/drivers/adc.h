/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup adc_interface
 * @brief Main header file for ADC (Analog-to-Digital Converter) driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ADC_H_
#define ZEPHYR_INCLUDE_DRIVERS_ADC_H_

#include <zephyr/device.h>
#include <zephyr/dt-bindings/adc/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/dsp/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Interfaces for Analog-to-Digital Converters (ADC).
 * @defgroup adc_interface ADC
 * @since 1.0
 * @version 1.0.0
 * @ingroup io_interfaces
 * @{
 */

/** @brief ADC channel gain factors. */
enum adc_gain {
	ADC_GAIN_1_6, /**< x 1/6. */
	ADC_GAIN_1_5, /**< x 1/5. */
	ADC_GAIN_1_4, /**< x 1/4. */
	ADC_GAIN_2_7, /**< x 2/7. */
	ADC_GAIN_1_3, /**< x 1/3. */
	ADC_GAIN_2_5, /**< x 2/5. */
	ADC_GAIN_1_2, /**< x 1/2. */
	ADC_GAIN_2_3, /**< x 2/3. */
	ADC_GAIN_4_5, /**< x 4/5. */
	ADC_GAIN_1,   /**< x 1. */
	ADC_GAIN_2,   /**< x 2. */
	ADC_GAIN_3,   /**< x 3. */
	ADC_GAIN_4,   /**< x 4. */
	ADC_GAIN_6,   /**< x 6. */
	ADC_GAIN_8,   /**< x 8. */
	ADC_GAIN_12,  /**< x 12. */
	ADC_GAIN_16,  /**< x 16. */
	ADC_GAIN_24,  /**< x 24. */
	ADC_GAIN_32,  /**< x 32. */
	ADC_GAIN_64,  /**< x 64. */
	ADC_GAIN_128, /**< x 128. */
};

/**
 * @brief Invert the application of gain to a measurement value.
 *
 * For example, if the gain passed in is ADC_GAIN_1_6 and the
 * referenced value is 10, the value after the function returns is 60.
 *
 * @param gain the gain used to amplify the input signal.
 *
 * @param value a pointer to a value that initially has the effect of
 * the applied gain but has that effect removed when this function
 * successfully returns.  If the gain cannot be reversed the value
 * remains unchanged.
 *
 * @retval 0 if the gain was successfully reversed
 * @retval -EINVAL if the gain could not be interpreted
 */
int adc_gain_invert(enum adc_gain gain, int32_t *value);

/**
 * @brief Invert the application of gain to a measurement value.
 *
 * For example, if the gain passed in is ADC_GAIN_1_6 and the
 * referenced value is 10, the value after the function returns is 60.
 *
 * @param gain the gain used to amplify the input signal.
 *
 * @param value a pointer to a value that initially has the effect of
 * the applied gain but has that effect removed when this function
 * successfully returns.  If the gain cannot be reversed the value
 * remains unchanged.
 *
 * @retval 0 if the gain was successfully reversed
 * @retval -EINVAL if the gain could not be interpreted
 */
int adc_gain_invert_64(enum adc_gain gain, int64_t *value);

/** @brief ADC references. */
enum adc_reference {
	ADC_REF_VDD_1,     /**< VDD. */
	ADC_REF_VDD_1_2,   /**< VDD/2. */
	ADC_REF_VDD_1_3,   /**< VDD/3. */
	ADC_REF_VDD_1_4,   /**< VDD/4. */
	ADC_REF_INTERNAL,  /**< Internal. */
	ADC_REF_EXTERNAL0, /**< External, input 0. */
	ADC_REF_EXTERNAL1, /**< External, input 1. */
};

/**
 * @brief Structure for specifying the configuration of an ADC channel.
 */
struct adc_channel_cfg {
	/** Gain selection. */
	enum adc_gain gain;

	/** Reference selection. */
	enum adc_reference reference;

	/**
	 * Acquisition time.
	 * Use the ADC_ACQ_TIME macro to compose the value for this field or
	 * pass ADC_ACQ_TIME_DEFAULT to use the default setting for a given
	 * hardware (e.g. when the hardware does not allow to configure the
	 * acquisition time).
	 * Particular drivers do not necessarily support all the possible units.
	 * Value range is 0-16383 for a given unit.
	 */
	uint16_t acquisition_time;

	/**
	 * Channel identifier.
	 * This value primarily identifies the channel within the ADC API - when
	 * a read request is done, the corresponding bit in the "channels" field
	 * of the "adc_sequence" structure must be set to include this channel
	 * in the sampling.
	 * For hardware that does not allow selection of analog inputs for given
	 * channels, but rather have dedicated ones, this value also selects the
	 * physical ADC input to be used in the sampling. Otherwise, when it is
	 * needed to explicitly select an analog input for the channel, or two
	 * inputs when the channel is a differential one, the selection is done
	 * in "input_positive" and "input_negative" fields.
	 * Particular drivers indicate which one of the above two cases they
	 * support by selecting or not a special hidden Kconfig option named
	 * ADC_CONFIGURABLE_INPUTS. If this option is not selected, the macro
	 * CONFIG_ADC_CONFIGURABLE_INPUTS is not defined and consequently the
	 * mentioned two fields are not present in this structure.
	 * While this API allows identifiers from range 0-31, particular drivers
	 * may support only a limited number of channel identifiers (dependent
	 * on the underlying hardware capabilities or configured via a dedicated
	 * Kconfig option).
	 */
	uint8_t channel_id   : 5;

	/** Channel type: single-ended or differential. */
	uint8_t differential : 1;

#ifdef CONFIG_ADC_CONFIGURABLE_INPUTS
	/**
	 * Positive ADC input.
	 * This is a driver dependent value that identifies an ADC input to be
	 * associated with the channel.
	 */
	uint8_t input_positive;

	/**
	 * Negative ADC input (used only for differential channels).
	 * This is a driver dependent value that identifies an ADC input to be
	 * associated with the channel.
	 */
	uint8_t input_negative;
#endif /* CONFIG_ADC_CONFIGURABLE_INPUTS */

#ifdef CONFIG_ADC_CONFIGURABLE_EXCITATION_CURRENT_SOURCE_PIN
	uint8_t current_source_pin_set : 1;
	/**
	 * Output pin for the current sources.
	 * This is only available if the driver enables this feature
	 * via the hidden configuration option ADC_CONFIGURABLE_EXCITATION_CURRENT_SOURCE_PIN.
	 * The meaning itself is then defined by the driver itself.
	 */
	uint8_t current_source_pin[2];
#endif /* CONFIG_ADC_CONFIGURABLE_EXCITATION_CURRENT_SOURCE_PIN */

#ifdef CONFIG_ADC_CONFIGURABLE_VBIAS_PIN
	/**
	 * Output pins for the bias voltage.
	 * This is only available if the driver enables this feature
	 * via the hidden configuration option ADC_CONFIGURABLE_VBIAS_PIN.
	 * The field is interpreted as a bitmask, where each bit represents
	 * one of the input pins. The actual mapping to the physical pins
	 * depends on the driver itself.
	 */
	uint32_t vbias_pins;
#endif /* CONFIG_ADC_CONFIGURABLE_VBIAS_PIN */
};

/**
 * @brief Get ADC channel configuration from a given devicetree node.
 *
 * This returns a static initializer for a <tt>struct adc_channel_cfg</tt>
 * filled with data from a given devicetree node.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 * &adc {
 *    #address-cells = <1>;
 *    #size-cells = <0>;
 *
 *    channel@0 {
 *        reg = <0>;
 *        zephyr,gain = "ADC_GAIN_1_6";
 *        zephyr,reference = "ADC_REF_INTERNAL";
 *        zephyr,acquisition-time = <ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 20)>;
 *        zephyr,input-positive = <NRF_SAADC_AIN6>;
 *        zephyr,input-negative = <NRF_SAADC_AIN7>;
 *    };
 *
 *    channel@1 {
 *        reg = <1>;
 *        zephyr,gain = "ADC_GAIN_1_6";
 *        zephyr,reference = "ADC_REF_INTERNAL";
 *        zephyr,acquisition-time = <ADC_ACQ_TIME_DEFAULT>;
 *        zephyr,input-positive = <NRF_SAADC_AIN0>;
 *    };
 * };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 * static const struct adc_channel_cfg ch0_cfg_dt =
 *     ADC_CHANNEL_CFG_DT(DT_CHILD(DT_NODELABEL(adc), channel_0));
 * static const struct adc_channel_cfg ch1_cfg_dt =
 *     ADC_CHANNEL_CFG_DT(DT_CHILD(DT_NODELABEL(adc), channel_1));
 *
 * // Initializes 'ch0_cfg_dt' to:
 * // {
 * //     .channel_id = 0,
 * //     .gain = ADC_GAIN_1_6,
 * //     .reference = ADC_REF_INTERNAL,
 * //     .acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 20),
 * //     .differential = true,
 * //     .input_positive = NRF_SAADC_AIN6,
 * //     .input-negative = NRF_SAADC_AIN7,
 * // }
 * // and 'ch1_cfg_dt' to:
 * // {
 * //     .channel_id = 1,
 * //     .gain = ADC_GAIN_1_6,
 * //     .reference = ADC_REF_INTERNAL,
 * //     .acquisition_time = ADC_ACQ_TIME_DEFAULT,
 * //     .input_positive = NRF_SAADC_AIN0,
 * // }
 * @endcode
 *
 * @param node_id Devicetree node identifier.
 *
 * @return Static initializer for an adc_channel_cfg structure.
 */
#define ADC_CHANNEL_CFG_DT(node_id) { \
	.gain             = DT_STRING_TOKEN(node_id, zephyr_gain), \
	.reference        = DT_STRING_TOKEN(node_id, zephyr_reference), \
	.acquisition_time = DT_PROP(node_id, zephyr_acquisition_time), \
	.channel_id       = DT_REG_ADDR(node_id), \
COND_CODE_1(UTIL_OR(DT_PROP(node_id, zephyr_differential), \
		   UTIL_AND(CONFIG_ADC_CONFIGURABLE_INPUTS, \
			    DT_NODE_HAS_PROP(node_id, zephyr_input_negative))), \
	(.differential    = true,), \
	(.differential    = false,)) \
IF_ENABLED(CONFIG_ADC_CONFIGURABLE_INPUTS, \
	(.input_positive  = DT_PROP_OR(node_id, zephyr_input_positive, 0), \
	 .input_negative  = DT_PROP_OR(node_id, zephyr_input_negative, 0),)) \
IF_ENABLED(CONFIG_ADC_CONFIGURABLE_EXCITATION_CURRENT_SOURCE_PIN, \
	(.current_source_pin_set = DT_NODE_HAS_PROP(node_id, zephyr_current_source_pin), \
	 .current_source_pin = DT_PROP_OR(node_id, zephyr_current_source_pin, {0}),)) \
IF_ENABLED(CONFIG_ADC_CONFIGURABLE_VBIAS_PIN, \
	(.vbias_pins = DT_PROP_OR(node_id, zephyr_vbias_pins, 0),)) \
}

/**
 * @brief Container for ADC channel information specified in devicetree.
 *
 * @see ADC_DT_SPEC_GET_BY_IDX
 * @see ADC_DT_SPEC_GET
 */
struct adc_dt_spec {
	/**
	 * Pointer to the device structure for the ADC driver instance
	 * used by this io-channel.
	 */
	const struct device *dev;

	/** ADC channel identifier used by this io-channel. */
	uint8_t channel_id;

	/**
	 * Flag indicating whether configuration of the associated ADC channel
	 * is provided as a child node of the corresponding ADC controller in
	 * devicetree.
	 */
	bool channel_cfg_dt_node_exists;

	/**
	 * Configuration of the associated ADC channel specified in devicetree.
	 * This field is valid only when @a channel_cfg_dt_node_exists is set
	 * to @a true.
	 */
	struct adc_channel_cfg channel_cfg;

	/**
	 * Voltage of the reference selected for the channel or 0 if this
	 * value is not provided in devicetree.
	 * This field is valid only when @a channel_cfg_dt_node_exists is set
	 * to @a true.
	 */
	uint16_t vref_mv;

	/**
	 * ADC resolution to be used for that channel.
	 * This field is valid only when @a channel_cfg_dt_node_exists is set
	 * to @a true.
	 */
	uint8_t resolution;

	/**
	 * Oversampling setting to be used for that channel.
	 * This field is valid only when @a channel_cfg_dt_node_exists is set
	 * to @a true.
	 */
	uint8_t oversampling;
};

/** @cond INTERNAL_HIDDEN */

#define ADC_DT_SPEC_STRUCT(ctlr, input) { \
		.dev = DEVICE_DT_GET(ctlr), \
		.channel_id = input, \
		ADC_CHANNEL_CFG_FROM_DT_NODE(\
			ADC_CHANNEL_DT_NODE(ctlr, input)) \
	}

#define ADC_CHANNEL_DT_NODE(ctlr, input) \
	DT_FOREACH_CHILD_VARGS(ctlr, ADC_FOREACH_INPUT, input)

#define ADC_FOREACH_INPUT(node, input) \
	IF_ENABLED(IS_EQ(DT_REG_ADDR_RAW(node), input), (node))

#define ADC_CHANNEL_CFG_FROM_DT_NODE(node_id) \
	IF_ENABLED(DT_NODE_EXISTS(node_id), \
		(.channel_cfg_dt_node_exists = true, \
		 .channel_cfg  = ADC_CHANNEL_CFG_DT(node_id), \
		 .vref_mv      = DT_PROP_OR(node_id, zephyr_vref_mv, 0), \
		 .resolution   = DT_PROP_OR(node_id, zephyr_resolution, 0), \
		 .oversampling = DT_PROP_OR(node_id, zephyr_oversampling, 0),))

/** @endcond */

/**
 * @brief Get ADC io-channel information from devicetree by name.
 *
 * This returns a static initializer for an @p adc_dt_spec structure
 * given a devicetree node and a channel name. The node must have
 * the "io-channels" property defined.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 * / {
 *     zephyr,user {
 *         io-channels = <&adc0 1>, <&adc0 3>;
 *         io-channel-names = "A0", "A1";
 *     };
 * };
 *
 * &adc0 {
 *    #address-cells = <1>;
 *    #size-cells = <0>;
 *
 *    channel@3 {
 *        reg = <3>;
 *        zephyr,gain = "ADC_GAIN_1_5";
 *        zephyr,reference = "ADC_REF_VDD_1_4";
 *        zephyr,vref-mv = <750>;
 *        zephyr,acquisition-time = <ADC_ACQ_TIME_DEFAULT>;
 *        zephyr,resolution = <12>;
 *        zephyr,oversampling = <4>;
 *    };
 * };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 * static const struct adc_dt_spec adc_chan0 =
 *     ADC_DT_SPEC_GET_BY_NAME(DT_PATH(zephyr_user), a0);
 * static const struct adc_dt_spec adc_chan1 =
 *     ADC_DT_SPEC_GET_BY_NAME(DT_PATH(zephyr_user), a1);
 *
 * // Initializes 'adc_chan0' to:
 * // {
 * //     .dev = DEVICE_DT_GET(DT_NODELABEL(adc0)),
 * //     .channel_id = 1,
 * // }
 * // and 'adc_chan1' to:
 * // {
 * //     .dev = DEVICE_DT_GET(DT_NODELABEL(adc0)),
 * //     .channel_id = 3,
 * //     .channel_cfg_dt_node_exists = true,
 * //     .channel_cfg = {
 * //         .channel_id = 3,
 * //         .gain = ADC_GAIN_1_5,
 * //         .reference = ADC_REF_VDD_1_4,
 * //         .acquisition_time = ADC_ACQ_TIME_DEFAULT,
 * //     },
 * //     .vref_mv = 750,
 * //     .resolution = 12,
 * //     .oversampling = 4,
 * // }
 * @endcode
 *
 * @param node_id Devicetree node identifier.
 * @param name Channel name.
 *
 * @return Static initializer for an adc_dt_spec structure.
 */
#define ADC_DT_SPEC_GET_BY_NAME(node_id, name) \
	ADC_DT_SPEC_STRUCT(DT_IO_CHANNELS_CTLR_BY_NAME(node_id, name), \
			   DT_IO_CHANNELS_INPUT_BY_NAME(node_id, name))

/** @brief Get ADC io-channel information from a DT_DRV_COMPAT devicetree
 *         instance by name.
 *
 * @see ADC_DT_SPEC_GET_BY_NAME()
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param name Channel name.
 *
 * @return Static initializer for an adc_dt_spec structure.
 */
#define ADC_DT_SPEC_INST_GET_BY_NAME(inst, name) \
	ADC_DT_SPEC_GET_BY_NAME(DT_DRV_INST(inst), name)

/**
 * @brief Get ADC io-channel information from devicetree.
 *
 * This returns a static initializer for an @p adc_dt_spec structure
 * given a devicetree node and a channel index. The node must have
 * the "io-channels" property defined.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 * / {
 *     zephyr,user {
 *         io-channels = <&adc0 1>, <&adc0 3>;
 *     };
 * };
 *
 * &adc0 {
 *    #address-cells = <1>;
 *    #size-cells = <0>;
 *
 *    channel@3 {
 *        reg = <3>;
 *        zephyr,gain = "ADC_GAIN_1_5";
 *        zephyr,reference = "ADC_REF_VDD_1_4";
 *        zephyr,vref-mv = <750>;
 *        zephyr,acquisition-time = <ADC_ACQ_TIME_DEFAULT>;
 *        zephyr,resolution = <12>;
 *        zephyr,oversampling = <4>;
 *    };
 * };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 * static const struct adc_dt_spec adc_chan0 =
 *     ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);
 * static const struct adc_dt_spec adc_chan1 =
 *     ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 1);
 *
 * // Initializes 'adc_chan0' to:
 * // {
 * //     .dev = DEVICE_DT_GET(DT_NODELABEL(adc0)),
 * //     .channel_id = 1,
 * // }
 * // and 'adc_chan1' to:
 * // {
 * //     .dev = DEVICE_DT_GET(DT_NODELABEL(adc0)),
 * //     .channel_id = 3,
 * //     .channel_cfg_dt_node_exists = true,
 * //     .channel_cfg = {
 * //         .channel_id = 3,
 * //         .gain = ADC_GAIN_1_5,
 * //         .reference = ADC_REF_VDD_1_4,
 * //         .acquisition_time = ADC_ACQ_TIME_DEFAULT,
 * //     },
 * //     .vref_mv = 750,
 * //     .resolution = 12,
 * //     .oversampling = 4,
 * // }
 * @endcode
 *
 * @see ADC_DT_SPEC_GET()
 *
 * @param node_id Devicetree node identifier.
 * @param idx Channel index.
 *
 * @return Static initializer for an adc_dt_spec structure.
 */
#define ADC_DT_SPEC_GET_BY_IDX(node_id, idx) \
	ADC_DT_SPEC_STRUCT(DT_IO_CHANNELS_CTLR_BY_IDX(node_id, idx), \
			   DT_IO_CHANNELS_INPUT_BY_IDX(node_id, idx))

/** @brief Get ADC io-channel information from a DT_DRV_COMPAT devicetree
 *         instance.
 *
 * @see ADC_DT_SPEC_GET_BY_IDX()
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param idx Channel index.
 *
 * @return Static initializer for an adc_dt_spec structure.
 */
#define ADC_DT_SPEC_INST_GET_BY_IDX(inst, idx) \
	ADC_DT_SPEC_GET_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Equivalent to ADC_DT_SPEC_GET_BY_IDX(node_id, 0).
 *
 * @see ADC_DT_SPEC_GET_BY_IDX()
 *
 * @param node_id Devicetree node identifier.
 *
 * @return Static initializer for an adc_dt_spec structure.
 */
#define ADC_DT_SPEC_GET(node_id) ADC_DT_SPEC_GET_BY_IDX(node_id, 0)

/** @brief Equivalent to ADC_DT_SPEC_INST_GET_BY_IDX(inst, 0).
 *
 * @see ADC_DT_SPEC_GET()
 *
 * @param inst DT_DRV_COMPAT instance number
 *
 * @return Static initializer for an adc_dt_spec structure.
 */
#define ADC_DT_SPEC_INST_GET(inst) ADC_DT_SPEC_GET(DT_DRV_INST(inst))

/* Forward declaration of the adc_sequence structure. */
struct adc_sequence;

/**
 * @brief Action to be performed after a sampling is done.
 */
enum adc_action {
	/** The sequence should be continued normally. */
	ADC_ACTION_CONTINUE = 0,

	/**
	 * The sampling should be repeated. New samples or sample should be
	 * read from the ADC and written in the same place as the recent ones.
	 */
	ADC_ACTION_REPEAT,

	/** The sequence should be finished immediately. */
	ADC_ACTION_FINISH,
};

/**
 * @brief Type definition of the optional callback function to be called after
 *        a requested sampling is done.
 *
 * @param dev             Pointer to the device structure for the driver
 *                        instance.
 * @param sequence        Pointer to the sequence structure that triggered
 *                        the sampling. This parameter points to a copy of
 *                        the structure that was supplied to the call that
 *                        started the sampling sequence, thus it cannot be
 *                        used with the CONTAINER_OF() macro to retrieve
 *                        some other data associated with the sequence.
 *                        Instead, the adc_sequence_options::user_data field
 *                        should be used for such purpose.
 *
 * @param sampling_index  Index (0-65535) of the sampling done.
 *
 * @returns Action to be performed by the driver. See @ref adc_action.
 */
typedef enum adc_action (*adc_sequence_callback)(const struct device *dev,
						 const struct adc_sequence *sequence,
						 uint16_t sampling_index);

/**
 * @brief Structure defining additional options for an ADC sampling sequence.
 */
struct adc_sequence_options {
	/**
	 * Interval between consecutive samplings (in microseconds), 0 means
	 * sample as fast as possible, without involving any timer.
	 * The accuracy of this interval is dependent on the implementation of
	 * a given driver. The default routine that handles the intervals uses
	 * a kernel timer for this purpose, thus, it has the accuracy of the
	 * kernel's system clock. Particular drivers may use some dedicated
	 * hardware timers and achieve a better precision.
	 */
	uint32_t interval_us;

	/**
	 * Callback function to be called after each sampling is done.
	 * Optional - set to NULL if it is not needed.
	 */
	adc_sequence_callback callback;

	/**
	 * Pointer to user data. It can be used to associate the sequence
	 * with any other data that is needed in the callback function.
	 */
	void *user_data;

	/**
	 * Number of extra samplings to perform (the total number of samplings
	 * is 1 + extra_samplings).
	 */
	uint16_t extra_samplings;
};

/**
 * @brief Structure defining an ADC sampling sequence.
 */
struct adc_sequence {
	/**
	 * Pointer to a structure defining additional options for the sequence.
	 * If NULL, the sequence consists of a single sampling.
	 */
	const struct adc_sequence_options *options;

	/**
	 * Bit-mask indicating the channels to be included in each sampling
	 * of this sequence.
	 * All selected channels must be configured with adc_channel_setup()
	 * before they are used in a sequence.
	 * The least significant bit corresponds to channel 0.
	 */
	uint32_t channels;

	/**
	 * Pointer to a buffer where the samples are to be written. Samples
	 * from subsequent samplings are written sequentially in the buffer.
	 * The number of samples written for each sampling is determined by
	 * the number of channels selected in the "channels" field.
	 * The values written to the buffer represent a sample from each
	 * selected channel starting from the one with the lowest ID.
	 * The buffer must be of an appropriate size, taking into account
	 * the number of selected channels and the ADC resolution used,
	 * as well as the number of samplings contained in the sequence.
	 */
	void *buffer;

	/**
	 * Specifies the actual size of the buffer pointed by the "buffer"
	 * field (in bytes). The driver must ensure that samples are not
	 * written beyond the limit and it must return an error if the buffer
	 * turns out to be not large enough to hold all the requested samples.
	 */
	size_t buffer_size;

	/**
	 * ADC resolution.
	 * For single-ended channels the sample values are from range:
	 *   0 .. 2^resolution - 1,
	 * for differential ones:
	 *   - 2^(resolution-1) .. 2^(resolution-1) - 1.
	 */
	uint8_t resolution;

	/**
	 * Oversampling setting.
	 * Each sample is averaged from 2^oversampling conversion results.
	 * This feature may be unsupported by a given ADC hardware, or in
	 * a specific mode (e.g. when sampling multiple channels).
	 */
	uint8_t oversampling;

	/**
	 * Perform calibration before the reading is taken if requested.
	 *
	 * The impact of channel configuration on the calibration
	 * process is specific to the underlying hardware.  ADC
	 * implementations that do not support calibration should
	 * ignore this flag.
	 */
	bool calibrate;
};

struct adc_data_header {
	/**
	 * The closest timestamp for when the first frame was generated as attained by
	 * :c:func:`k_uptime_ticks`.
	 */
	uint64_t base_timestamp_ns;
	/**
	 * The number of elements in the 'readings' array.
	 *
	 * This must be at least 1
	 */
	uint16_t reading_count;
};

/**
 * Data for the adc channel.
 */
struct adc_data {
	struct adc_data_header header;
	int8_t shift;
	struct adc_sample_data {
		uint32_t timestamp_delta;
		union {
			q31_t value;
		};
	} readings[1];
};

/**
 * @brief ADC trigger types.
 */
enum adc_trigger_type {
	/** Trigger fires whenever new data is ready. */
	ADC_TRIG_DATA_READY,

	/** Trigger fires when the FIFO watermark has been reached. */
	ADC_TRIG_FIFO_WATERMARK,

	/** Trigger fires when the FIFO becomes full. */
	ADC_TRIG_FIFO_FULL,

	/**
	 * Number of all common adc triggers.
	 */
	ADC_TRIG_COMMON_COUNT,

	/**
	 * This and higher values are adc specific.
	 * Refer to the adc header file.
	 */
	ADC_TRIG_PRIV_START = ADC_TRIG_COMMON_COUNT,

	/**
	 * Maximum value describing a adc trigger type.
	 */
	ADC_TRIG_MAX = INT16_MAX,
};

/**
 * @brief Options for what to do with the associated data when a trigger is consumed
 */
enum adc_stream_data_opt {
	/** @brief Include whatever data is associated with the trigger */
	ADC_STREAM_DATA_INCLUDE = 0,
	/** @brief Do nothing with the associated trigger data, it may be consumed later */
	ADC_STREAM_DATA_NOP = 1,
	/** @brief Flush/clear whatever data is associated with the trigger */
	ADC_STREAM_DATA_DROP = 2,
};

struct adc_stream_trigger {
	enum adc_trigger_type trigger;
	enum adc_stream_data_opt opt;
};

/**
 * @brief ADC Channel Specification
 *
 * A ADC channel specification is a unique identifier per ADC device describing
 * a measurement channel.
 *
 */
struct adc_chan_spec {
	uint8_t chan_idx; /**< A ADC channel index */
	uint8_t chan_resolution;  /**< A ADC channel resolution */
};

/*
 * Internal data structure used to store information about the IODevice for async reading and
 * streaming adc data.
 */
struct adc_read_config {
	const struct device *adc;
	const bool is_streaming;
	const struct adc_dt_spec *adc_spec;
	const struct adc_stream_trigger *triggers;
	struct adc_sequence *sequence;
	uint16_t fifo_watermark_lvl;
	uint16_t fifo_mode;
	size_t adc_spec_cnt;
	size_t trigger_cnt;
};

/**
 * @brief Decodes a single raw data buffer
 *
 */
struct adc_decoder_api {
	/**
	 * @brief Get the number of frames in the current buffer.
	 *
	 * @param[in]  buffer The buffer provided on the @ref rtio context.
	 * @param[in]  channel The channel to get the count for
	 * @param[out] frame_count The number of frames on the buffer (at least 1)
	 * @return 0 on success
	 * @return -ENOTSUP if the channel/channel_idx aren't found
	 */
	int (*get_frame_count)(const uint8_t *buffer, uint32_t channel,
			       uint16_t *frame_count);

	/**
	 * @brief Get the size required to decode a given channel
	 *
	 * When decoding a single frame, use @p base_size. For every additional frame, add another
	 * @p frame_size. As an example, to decode 3 frames use: 'base_size + 2 * frame_size'.
	 *
	 * @param[in] adc_spec ADC Specs
	 * @param[in]  channel The channel to query
	 * @param[out] base_size The size of decoding the first frame
	 * @param[out] frame_size The additional size of every additional frame
	 * @return 0 on success
	 * @return -ENOTSUP if the channel is not supported
	 */
	int (*get_size_info)(struct adc_dt_spec adc_spec, uint32_t channel, size_t *base_size,
			     size_t *frame_size);

	/**
	 * @brief Decode up to @p max_count samples from the buffer
	 *
	 * Decode samples of channel across multiple frames. If there exist
	 * multiple instances of the same channel, @p channel_index is used to differentiate them.
	 *
	 * @param[in]     buffer The buffer provided on the @ref rtio context
	 * @param[in]     channel The channel to decode
	 * @param[in,out] fit The current frame iterator
	 * @param[in]     max_count The maximum number of channels to decode.
	 * @param[out]    data_out The decoded data
	 * @return 0 no more samples to decode
	 * @return >0 the number of decoded frames
	 * @return <0 on error
	 */
	int (*decode)(const uint8_t *buffer, uint32_t channel, uint32_t *fit,
		      uint16_t max_count, void *data_out);

	/**
	 * @brief Check if the given trigger type is present
	 *
	 * @param[in] buffer The buffer provided on the @ref rtio context
	 * @param[in] trigger The trigger type in question
	 * @return Whether the trigger is present in the buffer
	 */
	bool (*has_trigger)(const uint8_t *buffer, enum adc_trigger_type trigger);
};

/**
 * @brief Type definition of ADC API function for configuring a channel.
 * See adc_channel_setup() for argument descriptions.
 */
typedef int (*adc_api_channel_setup)(const struct device *dev,
				     const struct adc_channel_cfg *channel_cfg);

/**
 * @brief Type definition of ADC API function for setting a read request.
 * See adc_read() for argument descriptions.
 */
typedef int (*adc_api_read)(const struct device *dev,
			    const struct adc_sequence *sequence);

/**
 * @brief Type definition of ADC API function for setting an submit
 *        stream request.
 */
typedef void (*adc_api_submit)(const struct device *dev,
				  struct rtio_iodev_sqe *sqe);

/**
 * @brief Get the decoder associate with the given device
 *
 * @see adc_get_decoder() for more details
 */
typedef int (*adc_api_get_decoder)(const struct device *dev,
				    const struct adc_decoder_api **api);


/**
 * @brief Type definition of ADC API function for setting an asynchronous
 *        read request.
 * See adc_read_async() for argument descriptions.
 */
typedef int (*adc_api_read_async)(const struct device *dev,
				  const struct adc_sequence *sequence,
				  struct k_poll_signal *async);

/**
 * @brief ADC driver API
 *
 * This is the mandatory API any ADC driver needs to expose.
 */
__subsystem struct adc_driver_api {
	adc_api_channel_setup channel_setup;
	adc_api_read          read;
#ifdef CONFIG_ADC_ASYNC
	adc_api_read_async    read_async;
#endif
#ifdef CONFIG_ADC_STREAM
	adc_api_submit    submit;
	adc_api_get_decoder get_decoder;
#endif
	uint16_t ref_internal;	/* mV */
};

/**
 * @brief Configure an ADC channel.
 *
 * It is required to call this function and configure each channel before it is
 * selected for a read request.
 *
 * @param dev          Pointer to the device structure for the driver instance.
 * @param channel_cfg  Channel configuration.
 *
 * @retval 0       On success.
 * @retval -EINVAL If a parameter with an invalid value has been provided.
 */
__syscall int adc_channel_setup(const struct device *dev,
				const struct adc_channel_cfg *channel_cfg);

static inline int z_impl_adc_channel_setup(const struct device *dev,
					   const struct adc_channel_cfg *channel_cfg)
{
	return DEVICE_API_GET(adc, dev)->channel_setup(dev, channel_cfg);
}

/**
 * @brief Configure an ADC channel from a struct adc_dt_spec.
 *
 * @param spec ADC specification from Devicetree.
 *
 * @return A value from adc_channel_setup() or -ENOTSUP if information from
 * Devicetree is not valid.
 * @see adc_channel_setup()
 */
static inline int adc_channel_setup_dt(const struct adc_dt_spec *spec)
{
	if (!spec->channel_cfg_dt_node_exists) {
		return -ENOTSUP;
	}

	return adc_channel_setup(spec->dev, &spec->channel_cfg);
}

/**
 * @brief Set a read request.
 *
 * @param dev       Pointer to the device structure for the driver instance.
 * @param sequence  Structure specifying requested sequence of samplings.
 *
 * If invoked from user mode, any sequence struct options for callback must
 * be NULL.
 *
 * @retval 0        On success.
 * @retval -EINVAL  If a parameter with an invalid value has been provided.
 * @retval -ENOMEM  If the provided buffer is to small to hold the results
 *                  of all requested samplings.
 * @retval -ENOTSUP If the requested mode of operation is not supported.
 * @retval -EBUSY   If another sampling was triggered while the previous one
 *                  was still in progress. This may occur only when samplings
 *                  are done with intervals, and it indicates that the selected
 *                  interval was too small. All requested samples are written
 *                  in the buffer, but at least some of them were taken with
 *                  an extra delay compared to what was scheduled.
 */
__syscall int adc_read(const struct device *dev,
		       const struct adc_sequence *sequence);

static inline int z_impl_adc_read(const struct device *dev,
				  const struct adc_sequence *sequence)
{
	return DEVICE_API_GET(adc, dev)->read(dev, sequence);
}

/**
 * @brief Set a read request from a struct adc_dt_spec.
 *
 * @param spec ADC specification from Devicetree.
 * @param sequence  Structure specifying requested sequence of samplings.
 *
 * @return A value from adc_read().
 * @see adc_read()
 */
static inline int adc_read_dt(const struct adc_dt_spec *spec,
			      const struct adc_sequence *sequence)
{
	return adc_read(spec->dev, sequence);
}

/**
 * @brief Set an asynchronous read request.
 *
 * @note This function is available only if @kconfig{CONFIG_ADC_ASYNC}
 * is selected.
 *
 * If invoked from user mode, any sequence struct options for callback must
 * be NULL.
 *
 * @param dev       Pointer to the device structure for the driver instance.
 * @param sequence  Structure specifying requested sequence of samplings.
 * @param async     Pointer to a valid and ready to be signaled struct
 *                  k_poll_signal. (Note: if NULL this function will not notify
 *                  the end of the transaction, and whether it went successfully
 *                  or not).
 *
 * @returns 0 on success, negative error code otherwise.
 *          See adc_read() for a list of possible error codes.
 *
 */
__syscall int adc_read_async(const struct device *dev,
			     const struct adc_sequence *sequence,
			     struct k_poll_signal *async);

#ifdef CONFIG_ADC_ASYNC
static inline int z_impl_adc_read_async(const struct device *dev,
					const struct adc_sequence *sequence,
					struct k_poll_signal *async)
{
	return DEVICE_API_GET(adc, dev)->read_async(dev, sequence, async);
}
#endif /* CONFIG_ADC_ASYNC */

#ifdef CONFIG_ADC_STREAM
/**
 * @brief Get decoder APIs for that device.
 *
 * @note This function is available only if @kconfig{CONFIG_ADC_STREAM}
 * is selected.
 *
 * @param dev	Pointer to the device structure for the driver instance.
 * @param api	Pointer to the decoder which will be set upon success.
 *
 * @returns 0 on success, negative error code otherwise.
 *
 *
 */
__syscall int adc_get_decoder(const struct device *dev,
				const struct adc_decoder_api **api);
/*
 * Generic data structure used for encoding the sample timestamp and number of channels sampled.
 */
struct __attribute__((__packed__)) adc_data_generic_header {
	/* The timestamp at which the data was collected from the adc */
	uint64_t timestamp_ns;

	/*
	 * The number of channels present in the frame.
	 */
	uint8_t num_channels;

	/* Shift value for all samples in the frame */
	int8_t shift;

	/* This padding is needed to make sure that the 'channels' field is aligned */
	int16_t _padding;

	/* Channels present in the frame */
	struct adc_chan_spec channels[0];
};

static inline int adc_stream(struct rtio_iodev *iodev, struct rtio *ctx, void *userdata,
				struct rtio_sqe **handle)
{
	if (IS_ENABLED(CONFIG_USERSPACE)) {
		struct rtio_sqe sqe;

		rtio_sqe_prep_read_multishot(&sqe, iodev, RTIO_PRIO_NORM, userdata);
		rtio_sqe_copy_in_get_handles(ctx, &sqe, handle, 1);
	} else {
		struct rtio_sqe *sqe = rtio_sqe_acquire(ctx);

		if (sqe == NULL) {
			return -ENOMEM;
		}
		if (handle != NULL) {
			*handle = sqe;
		}
		rtio_sqe_prep_read_multishot(sqe, iodev, RTIO_PRIO_NORM, userdata);
	}
	rtio_submit(ctx, 0);
	return 0;
}

static inline int z_impl_adc_get_decoder(const struct device *dev,
					    const struct adc_decoder_api **decoder)
{
	const struct adc_driver_api *api = DEVICE_API_GET(adc, dev);

	__ASSERT_NO_MSG(api != NULL);

	if (api->get_decoder == NULL) {
		*decoder = NULL;
		return -1;
	}

	return api->get_decoder(dev, decoder);
}
#endif /* CONFIG_ADC_STREAM */

/**
 * @brief Get the internal reference voltage.
 *
 * Returns the voltage corresponding to @ref ADC_REF_INTERNAL,
 * measured in millivolts.
 *
 * @return a positive value is the reference voltage value.  Returns
 * zero if reference voltage information is not available.
 */
static inline uint16_t adc_ref_internal(const struct device *dev)
{
	return DEVICE_API_GET(adc, dev)->ref_internal;
}

/**
 * @brief Conversion from raw ADC units to a specific output unit
 *
 * This function performs the necessary conversion to transform a raw
 * ADC measurement to a physical voltage.
 *
 * @param ref_mv the reference voltage used for the measurement, in
 * millivolts.  This may be from adc_ref_internal() or a known
 * external reference.
 *
 * @param gain the ADC gain configuration used to sample the input
 *
 * @param resolution the number of bits in the absolute value of the
 * sample.  For differential sampling this needs to be one less than the
 * resolution in struct adc_sequence.
 *
 * @param valp pointer to the raw measurement value on input, and the
 * corresponding output value on successful conversion.  If
 * conversion fails the stored value is left unchanged.
 *
 * @retval 0 on successful conversion
 * @retval -EINVAL if the gain is not reversible
 */
typedef int (*adc_raw_to_x_fn)(int32_t ref_mv, enum adc_gain gain, uint8_t resolution,
			       int32_t *valp);

/**
 * @brief Convert a raw ADC value to millivolts.
 *
 * @see adc_raw_to_x_fn
 */
static inline int adc_raw_to_millivolts(int32_t ref_mv, enum adc_gain gain, uint8_t resolution,
					int32_t *valp)
{
	int32_t adc_mv = *valp * ref_mv;
	int ret = adc_gain_invert(gain, &adc_mv);

	if (ret == 0) {
		*valp = (adc_mv >> resolution);
	}

	return ret;
}

/**
 * @brief Convert a raw ADC value to microvolts.
 *
 * @see adc_raw_to_x_fn
 */
static inline int adc_raw_to_microvolts(int32_t ref_mv, enum adc_gain gain, uint8_t resolution,
					int32_t *valp)
{
	int64_t adc_uv = (int64_t)*valp * ref_mv * 1000;
	int ret = adc_gain_invert_64(gain, &adc_uv);

	if (ret == 0) {
		*valp = (int32_t)(adc_uv >> resolution);
	}

	return ret;
}

/**
 * @brief Convert a raw ADC value to an arbitrary output unit
 *
 * @param[in] conv_func Function that converts to the final output unit.
 * @param[in] spec ADC specification from Devicetree.
 * @param[in] channel_cfg Channel configuration used for sampling. This can be
 * either the configuration from @a spec, or an alternate sampling configuration
 * based on @a spec, for example a different gain value.
 * @param[in,out] valp Pointer to the raw measurement value on input, and the
 * corresponding output value on successful conversion. If conversion fails
 * the stored value is left unchanged.
 *
 * @return A value from adc_raw_to_x_fn or -ENOTSUP if information from
 * Devicetree is not valid.
 * @see adc_raw_to_x_fn
 */
static inline int adc_raw_to_x_dt_chan(adc_raw_to_x_fn conv_func,
					    const struct adc_dt_spec *spec,
					    const struct adc_channel_cfg *channel_cfg,
					    int32_t *valp)
{
	int32_t vref_mv;
	uint8_t resolution;

	if (!spec->channel_cfg_dt_node_exists) {
		return -ENOTSUP;
	}

	if (channel_cfg->reference == ADC_REF_INTERNAL) {
		vref_mv = (int32_t)adc_ref_internal(spec->dev);
	} else {
		vref_mv = spec->vref_mv;
	}

	resolution = spec->resolution;

	/*
	 * For differential channels, one bit less needs to be specified
	 * for resolution to achieve correct conversion.
	 */
	if (channel_cfg->differential) {
		resolution -= 1U;
	}

	return conv_func(vref_mv, channel_cfg->gain, resolution, valp);
}


/**
 * @brief Convert a raw ADC value to millivolts using information stored
 * in a struct adc_dt_spec.
 *
 * @param[in] spec ADC specification from Devicetree.
 * @param[in,out] valp Pointer to the raw measurement value on input, and the
 * corresponding millivolt value on successful conversion. If conversion fails
 * the stored value is left unchanged.
 *
 * @return A value from adc_raw_to_millivolts() or -ENOTSUP if information from
 * Devicetree is not valid.
 * @see adc_raw_to_millivolts()
 */
static inline int adc_raw_to_millivolts_dt(const struct adc_dt_spec *spec, int32_t *valp)
{
	return adc_raw_to_x_dt_chan(adc_raw_to_millivolts, spec, &spec->channel_cfg, valp);
}

/**
 * @brief Convert a raw ADC value to microvolts using information stored
 * in a struct adc_dt_spec.
 *
 * @param[in] spec ADC specification from Devicetree.
 * @param[in,out] valp Pointer to the raw measurement value on input, and the
 * corresponding microvolt value on successful conversion. If conversion fails
 * the stored value is left unchanged.
 *
 * @return A value from adc_raw_to_microvolts() or -ENOTSUP if information from
 * Devicetree is not valid.
 * @see adc_raw_to_microvolts()
 */
static inline int adc_raw_to_microvolts_dt(const struct adc_dt_spec *spec, int32_t *valp)
{
	return adc_raw_to_x_dt_chan(adc_raw_to_microvolts, spec, &spec->channel_cfg, valp);
}

/**
 * @brief Initialize a struct adc_sequence from information stored in
 * struct adc_dt_spec.
 *
 * Note that this function only initializes the following fields:
 *
 * - @ref adc_sequence.channels
 * - @ref adc_sequence.resolution
 * - @ref adc_sequence.oversampling
 *
 * Other fields should be initialized by the caller.
 *
 * @param[in] spec ADC specification from Devicetree.
 * @param[out] seq Sequence to initialize.
 *
 * @retval 0 On success
 * @retval -ENOTSUP If @p spec does not have valid channel configuration
 */
static inline int adc_sequence_init_dt(const struct adc_dt_spec *spec,
				       struct adc_sequence *seq)
{
	if (!spec->channel_cfg_dt_node_exists) {
		return -ENOTSUP;
	}

	seq->channels = BIT(spec->channel_id);
	seq->resolution = spec->resolution;
	seq->oversampling = spec->oversampling;

	return 0;
}

/**
 * @brief Validate that the ADC device is ready.
 *
 * @param spec ADC specification from devicetree
 *
 * @retval true if the ADC device is ready for use and false otherwise.
 */
static inline bool adc_is_ready_dt(const struct adc_dt_spec *spec)
{
	return device_is_ready(spec->dev);
}

/**
 * @}
 */

/**
 * @brief Get the decoder name for the current driver
 *
 * This function depends on `DT_DRV_COMPAT` being defined.
 */
#define ADC_DECODER_NAME() UTIL_CAT(DT_DRV_COMPAT, __adc_decoder_api)

/**
 * @brief Statically get the decoder for a given node
 *
 * @code{.c}
 * static const adc_decoder_api *decoder = ADC_DECODER_DT_GET(DT_ALIAS(adc));
 * @endcode
 */
#define ADC_DECODER_DT_GET(node_id)                                                             \
	&UTIL_CAT(DT_STRING_TOKEN_BY_IDX(node_id, compatible, 0), __adc_decoder_api)

/**
 * @brief Define a decoder API
 *
 * This macro should be created once per compatible string of a adc and will create a statically
 * referenceable decoder API.
 *
 * @code{.c}
 * ADC_DECODER_API_DT_DEFINE() = {
 *   .get_frame_count = my_driver_get_frame_count,
 *   .get_timestamp = my_driver_get_timestamp,
 *   .get_shift = my_driver_get_shift,
 *   .decode = my_driver_decode,
 * };
 * @endcode
 */
#define ADC_DECODER_API_DT_DEFINE()								\
	COND_CODE_1(DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT), (), (static))			\
	const STRUCT_SECTION_ITERABLE(adc_decoder_api, ADC_DECODER_NAME())

#define Z_MAYBE_ADC_DECODER_DECLARE_INTERNAL_IDX(node_id, prop, idx)				\
	extern const struct adc_decoder_api UTIL_CAT(						\
		DT_STRING_TOKEN_BY_IDX(node_id, prop, idx), __adc_decoder_api);

#define Z_MAYBE_ADC_DECODER_DECLARE_INTERNAL(node_id)						\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, compatible),					\
			(DT_FOREACH_PROP_ELEM(node_id, compatible,				\
					  Z_MAYBE_ADC_DECODER_DECLARE_INTERNAL_IDX)),		\
						())

DT_FOREACH_STATUS_OKAY_NODE(Z_MAYBE_ADC_DECODER_DECLARE_INTERNAL)

/* The default adc iodev API */
extern const struct rtio_iodev_api __adc_iodev_api;

#define ADC_DT_STREAM_IODEV(name, dt_node, adc_dt_spec, ...)					\
	static struct adc_stream_trigger _CONCAT(__trigger_array_, name)[] = {__VA_ARGS__};	\
	static struct adc_read_config _CONCAT(__adc_read_config_, name) = {			\
		.adc = DEVICE_DT_GET(dt_node),							\
		.is_streaming = true,								\
		.adc_spec = adc_dt_spec,							\
		.triggers = _CONCAT(__trigger_array_, name),					\
		.adc_spec_cnt = ARRAY_SIZE(adc_dt_spec),					\
		.trigger_cnt = ARRAY_SIZE(_CONCAT(__trigger_array_, name)),			\
	};											\
	RTIO_IODEV_DEFINE(name, &__adc_iodev_api, &_CONCAT(__adc_read_config_, name))

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/adc.h>

#endif  /* ZEPHYR_INCLUDE_DRIVERS_ADC_H_ */
