/*
 * Copyright (c) 2020 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup dac_interface
 * @brief Main header file for DAC (Digital-to-Analog Converter) driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DAC_H_
#define ZEPHYR_INCLUDE_DRIVERS_DAC_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Interfaces for Digital-to-Analog Converters.
 * @defgroup dac_interface DAC
 * @since 2.3
 * @version 1.1.0
 * @ingroup io_interfaces
 * @{
 *
 * @defgroup dac_interface_ext Device-specific DAC API extensions
 *
 * @{
 * @}
 */

/**
 * @brief Broadcast channel identifier for DACs that support it.
 * @note Only for use in dac_write_value().
 */
#define DAC_CHANNEL_BROADCAST	0xFF

/**
 * @brief Structure for specifying the configuration of a DAC channel.
 */
struct dac_channel_cfg {
	/** Channel identifier of the DAC that should be configured. */
	uint8_t channel_id;
	/** Desired resolution of the DAC (depends on device capabilities). */
	uint8_t resolution;
	/** Enable output buffer for this channel.
	 * This is relevant for instance if the output is directly connected to the load,
	 * without an amplifierin between. The actual details on this are hardware dependent.
	 */
	bool buffered: 1;
	/** Enable internal output path for this channel. This is relevant for channels that
	 * support directly connecting to on-chip peripherals via internal paths. The actual
	 * details on this are hardware dependent.
	 */
	bool internal: 1;
};

/**
 * @brief Get DAC channel configuration from a given devicetree node.
 *
 * This returns a static initializer for a <tt>struct dac_channel_cfg</tt>
 * filled with data from a given devicetree node.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 * &dac {
 *    #address-cells = <1>;
 *    #size-cells = <0>;
 *
 *    channel@0 {
 *        reg = <0>;
 *        zephyr,resolution = <12>;
 *    };
 *
 *    channel@1 {
 *        reg = <1>;
 *        zephyr,resolution = <12>;
 *        zephyr,buffered;
 *    };
 * };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 * static const struct dac_channel_cfg ch0_cfg_dt =
 *     DAC_CHANNEL_CFG_DT(DT_CHILD(DT_NODELABEL(dac), channel_0));
 * static const struct dac_channel_cfg ch1_cfg_dt =
 *     DAC_CHANNEL_CFG_DT(DT_CHILD(DT_NODELABEL(dac), channel_1));
 *
 * // Initializes 'ch0_cfg_dt' to:
 * // {
 * //     .channel_id = 0,
 * //     .resolution = 12,
 * // }
 * // and 'ch1_cfg_dt' to:
 * // {
 * //     .channel_id = 1,
 * //     .resolution = 12,
 * //     .buffered = true,
 * // }
 * @endcode
 *
 * @param node_id Devicetree node identifier.
 *
 * @return Static initializer for an dac_channel_cfg structure.
 */
#define DAC_CHANNEL_CFG_DT(node_id) { \
	.resolution       = DT_PROP_OR(node_id, zephyr_resolution, 0), \
	.buffered         = DT_PROP(node_id, zephyr_buffered),         \
	.internal         = DT_PROP(node_id, zephyr_internal),         \
	.channel_id       = DT_REG_ADDR(node_id),                      \
}

/**
 * @brief Container for DAC channel information specified in devicetree.
 *
 * @see DAC_DT_SPEC_GET_BY_IDX
 * @see DAC_DT_SPEC_GET
 */
struct dac_dt_spec {
	/**
	 * Pointer to the device structure for the DAC driver instance
	 * used by this io-channel.
	 */
	const struct device *dev;

	/** DAC channel identifier used by this io-channel. */
	uint8_t channel_id;

	/**
	 * Flag indicating whether configuration of the associated DAC channel
	 * is provided as a child node of the corresponding DAC controller in
	 * devicetree.
	 */
	bool channel_cfg_dt_node_exists;

	/**
	 * Configuration of the associated DAC channel specified in devicetree.
	 * This field is valid only when @a channel_cfg_dt_node_exists is set
	 * to @a true.
	 */
	struct dac_channel_cfg channel_cfg;

	/**
	 * Voltage of the reference selected for the channel or 0 if this
	 * value is not provided in devicetree.
	 * This field is valid only when @a channel_cfg_dt_node_exists is set
	 * to @a true.
	 */
	uint16_t vref_mv;
};

/** @cond INTERNAL_HIDDEN */

#define DAC_DT_SPEC_STRUCT(ctlr, output) { \
		.dev = DEVICE_DT_GET(ctlr), \
		.channel_id = output, \
		DAC_CHANNEL_CFG_FROM_DT_NODE(\
			DAC_CHANNEL_DT_NODE(ctlr, output)) \
	}

#define DAC_CHANNEL_DT_NODE(ctlr, output) \
	DT_CHILD_BY_UNIT_ADDR_INT(ctlr, output)

#define DAC_CHANNEL_CFG_FROM_DT_NODE(node_id) \
	IF_ENABLED(DT_NODE_EXISTS(node_id), \
		(.channel_cfg_dt_node_exists = true, \
		 .channel_cfg  = DAC_CHANNEL_CFG_DT(node_id), \
		 .vref_mv      = DT_PROP_OR(node_id, zephyr_vref_mv, 0),))

/** @endcond */

/**
 * @brief Get DAC io-channel information from devicetree by name.
 *
 * This returns a static initializer for an @p dac_dt_spec structure
 * given a devicetree node and a channel name. The node must have
 * the "io-channels" property defined.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 * / {
 *     zephyr,user {
 *         io-channels = <&dac0 1>, <&dac0 3>;
 *         io-channel-names = "A0", "A1";
 *     };
 * };
 *
 * &dac0 {
 *    #address-cells = <1>;
 *    #size-cells = <0>;
 *
 *    channel@3 {
 *        reg = <3>;
 *        zephyr,resolution = <12>;
 *    };
 * };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 * static const struct dac_dt_spec dac_chan0 =
 *     DAC_DT_SPEC_GET_BY_NAME(DT_PATH(zephyr_user), a0);
 * static const struct dac_dt_spec dac_chan1 =
 *     DAC_DT_SPEC_GET_BY_NAME(DT_PATH(zephyr_user), a1);
 *
 * // Initializes 'dac_chan0' to:
 * // {
 * //     .dev = DEVICE_DT_GET(DT_NODELABEL(dac0)),
 * //     .channel_id = 1,
 * // }
 * // and 'dac_chan1' to:
 * // {
 * //     .dev = DEVICE_DT_GET(DT_NODELABEL(dac0)),
 * //     .channel_id = 3,
 * //     .channel_cfg_dt_node_exists = true,
 * //     .channel_cfg = {
 * //         .channel_id = 3,
 * //         .resolution = 12,
 * //     },
 * // }
 * @endcode
 *
 * @param node_id Devicetree node identifier.
 * @param name Channel name.
 *
 * @return Static initializer for an dac_dt_spec structure.
 */
#define DAC_DT_SPEC_GET_BY_NAME(node_id, name) \
	DAC_DT_SPEC_STRUCT(DT_IO_CHANNELS_CTLR_BY_NAME(node_id, name), \
			   DT_IO_CHANNELS_OUTPUT_BY_NAME(node_id, name))

/**
 * @brief Like DAC_DT_SPEC_GET_BY_NAME(), with a fallback to a default value.
 *
 * @param node_id Devicetree node identifier.
 * @param name Channel name.
 * @param default_value Fallback value to expand to.
 *
 * @return Static initializer for a struct dac_dt_spec for the property,
 *         or @p default_value if the node or property do not exist.
 *
 * @see DAC_DT_SPEC_INST_GET_BY_NAME_OR
 */
#define DAC_DT_SPEC_GET_BY_NAME_OR(node_id, name, default_value) \
	COND_CODE_1(DT_PROP_HAS_NAME(node_id, io_channels, name), \
		    (DAC_DT_SPEC_GET_BY_NAME(node_id, name)), (default_value))

/** @brief Get DAC io-channel information from a DT_DRV_COMPAT devicetree
 *         instance by name.
 *
 * @see DAC_DT_SPEC_GET_BY_NAME()
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param name Channel name.
 *
 * @return Static initializer for an dac_dt_spec structure.
 */
#define DAC_DT_SPEC_INST_GET_BY_NAME(inst, name) \
	DAC_DT_SPEC_GET_BY_NAME(DT_DRV_INST(inst), name)


/**
 * @brief Like DAC_DT_SPEC_INST_GET_BY_NAME(), with a fallback to a default value.
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param name Channel name.
 * @param default_value Fallback value to expand to.
 *
 * @return Static initializer for a struct dac_dt_spec for the property,
 *         or @p default_value if the node or property do not exist.
 *
 * @see DAC_DT_SPEC_GET_BY_NAME_OR
 */
#define DAC_DT_SPEC_INST_GET_BY_NAME_OR(inst, name, default_value) \
	DAC_DT_SPEC_GET_BY_NAME_OR(DT_DRV_INST(inst), name, default_value)

/**
 * @brief Get DAC io-channel information from devicetree.
 *
 * This returns a static initializer for an @p dac_dt_spec structure
 * given a devicetree node and a channel index. The node must have
 * the "io-channels" property defined.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 * / {
 *     zephyr,user {
 *         io-channels = <&dac0 1>, <&dac0 3>;
 *     };
 * };
 *
 * &dac0 {
 *    #address-cells = <1>;
 *    #size-cells = <0>;
 *
 *    channel@3 {
 *        reg = <3>;

 *        zephyr,resolution = <12>;
 *    };
 * };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 * static const struct dac_dt_spec dac_chan0 =
 *     DAC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);
 * static const struct dac_dt_spec dac_chan1 =
 *     DAC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 1);
 *
 * // Initializes 'dac_chan0' to:
 * // {
 * //     .dev = DEVICE_DT_GET(DT_NODELABEL(dac0)),
 * //     .channel_id = 1,
 * // }
 * // and 'dac_chan1' to:
 * // {
 * //     .dev = DEVICE_DT_GET(DT_NODELABEL(dac0)),
 * //     .channel_id = 3,
 * //     .channel_cfg_dt_node_exists = true,
 * //     .channel_cfg = {
 * //         .channel_id = 3,
 * //         .resolution = 12
 * //     },
 * // }
 * @endcode
 *
 * @see DAC_DT_SPEC_GET()
 *
 * @param node_id Devicetree node identifier.
 * @param idx Channel index.
 *
 * @return Static initializer for an dac_dt_spec structure.
 */
#define DAC_DT_SPEC_GET_BY_IDX(node_id, idx) \
	DAC_DT_SPEC_STRUCT(DT_IO_CHANNELS_CTLR_BY_IDX(node_id, idx), \
			   COND_CODE_1(DT_PHA_HAS_CELL_AT_IDX(node_id, io_channels, idx, output), \
				       (DT_IO_CHANNELS_OUTPUT_BY_IDX(node_id, idx)), \
				       (0)))

/**
 * @brief Like DAC_DT_SPEC_GET_BY_IDX(), with a fallback to a default value.
 *
 * @param node_id Devicetree node identifier.
 * @param idx Channel index.
 * @param default_value Fallback value to expand to.
 *
 * @return Static initializer for a struct dac_dt_spec for the property,
 *         or @p default_value if the node or property do not exist.
 *
 * @see DAC_DT_SPEC_INST_GET_BY_IDX_OR
 */
#define DAC_DT_SPEC_GET_BY_IDX_OR(node_id, idx, default_value) \
	COND_CODE_1(DT_PROP_HAS_IDX(node_id, io_channels, idx), \
		    (DAC_DT_SPEC_GET_BY_IDX(node_id, idx)), (default_value))

/** @brief Get DAC io-channel information from a DT_DRV_COMPAT devicetree
 *         instance.
 *
 * @see DAC_DT_SPEC_GET_BY_IDX()
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param idx Channel index.
 *
 * @return Static initializer for an dac_dt_spec structure.
 */
#define DAC_DT_SPEC_INST_GET_BY_IDX(inst, idx) \
	DAC_DT_SPEC_GET_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Like DAC_DT_SPEC_INST_GET_BY_IDX(), with a fallback to a default value.
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param idx Channel index.
 * @param default_value Fallback value to expand to.
 *
 * @return Static initializer for a struct dac_dt_spec for the property,
 *         or @p default_value if the node or property do not exist.
 *
 * @see DAC_DT_SPEC_GET_BY_IDX_OR
 */
#define DAC_DT_SPEC_INST_GET_BY_IDX_OR(inst, idx, default_value) \
	DAC_DT_SPEC_GET_BY_IDX_OR(DT_DRV_INST(inst), idx, default_value)

/**
 * @brief Equivalent to DAC_DT_SPEC_GET_BY_IDX(node_id, 0).
 *
 * @see DAC_DT_SPEC_GET_BY_IDX()
 *
 * @param node_id Devicetree node identifier.
 *
 * @return Static initializer for an dac_dt_spec structure.
 */
#define DAC_DT_SPEC_GET(node_id) DAC_DT_SPEC_GET_BY_IDX(node_id, 0)

/**
 * @brief Equivalent to DAC_DT_SPEC_GET_BY_IDX_OR(node_id, 0, default_value).
 *
 * @see DAC_DT_SPEC_GET_BY_IDX_OR()
 *
 * @param node_id Devicetree node identifier.
 * @param default_value Fallback value to expand to.
 *
 * @return Static initializer for a struct dac_dt_spec for the property,
 *         or @p default_value if the node or property do not exist.
 */
#define DAC_DT_SPEC_GET_OR(node_id, default_value) \
	DAC_DT_SPEC_GET_BY_IDX_OR(node_id, 0, default_value)

/** @brief Equivalent to DAC_DT_SPEC_INST_GET_BY_IDX(inst, 0).
 *
 * @see DAC_DT_SPEC_GET()
 *
 * @param inst DT_DRV_COMPAT instance number
 *
 * @return Static initializer for an dac_dt_spec structure.
 */
#define DAC_DT_SPEC_INST_GET(inst) DAC_DT_SPEC_GET(DT_DRV_INST(inst))

/**
 * @brief Equivalent to DAC_DT_SPEC_INST_GET_BY_IDX_OR(inst, 0, default).
 *
 * @see DAC_DT_SPEC_GET_OR()
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param default_value Fallback value to expand to.
 *
 * @return Static initializer for a struct dac_dt_spec for the property,
 *         or @p default_value if the node or property do not exist.
 */
#define DAC_DT_SPEC_INST_GET_OR(inst, default_value) \
	DAC_DT_SPEC_GET_OR(DT_DRV_INST(inst), default_value)

/**
 * @cond INTERNAL_HIDDEN
 *
 * For internal use only, skip these in public documentation.
 */

/*
 * Type definition of DAC API function for configuring a channel.
 * See dac_channel_setup() for argument descriptions.
 */
typedef int (*dac_api_channel_setup)(const struct device *dev,
				     const struct dac_channel_cfg *channel_cfg);

/*
 * Type definition of DAC API function for setting a write request.
 * See dac_write_value() for argument descriptions.
 */
typedef int (*dac_api_write_value)(const struct device *dev,
				    uint8_t channel, uint32_t value);

/*
 * DAC driver API
 *
 * This is the mandatory API any DAC driver needs to expose.
 */
__subsystem struct dac_driver_api {
	dac_api_channel_setup channel_setup;
	dac_api_write_value   write_value;
};

/**
 * @endcond
 */

/**
 * @brief Configure a DAC channel.
 *
 * It is required to call this function and configure each channel before it is
 * selected for a write request.
 *
 * @param dev          Pointer to the device structure for the driver instance.
 * @param channel_cfg  Channel configuration.
 *
 * @retval 0         On success.
 * @retval -EINVAL   If a parameter with an invalid value has been provided.
 * @retval -ENOTSUP  If the requested resolution is not supported.
 */
__syscall int dac_channel_setup(const struct device *dev,
				const struct dac_channel_cfg *channel_cfg);

static inline int z_impl_dac_channel_setup(const struct device *dev,
					   const struct dac_channel_cfg *channel_cfg)
{
	const struct dac_driver_api *api =
				(const struct dac_driver_api *)dev->api;

	return api->channel_setup(dev, channel_cfg);
}

/**
 * @brief Configure an DAC channel from a struct dac_dt_spec.
 *
 * @param spec DAC specification from Devicetree.
 *
 * @return A value from dac_channel_setup() or -ENOTSUP if information from
 * Devicetree is not valid.
 * @see dac_channel_setup()
 */
static inline int dac_channel_setup_dt(const struct dac_dt_spec *spec)
{
	if (!spec->channel_cfg_dt_node_exists) {
		return -ENOTSUP;
	}

	return dac_channel_setup(spec->dev, &spec->channel_cfg);
}

/**
 * @brief Write a single value to a DAC channel
 *
 * @param dev         Pointer to the device structure for the driver instance.
 * @param channel     Number of the channel to be used.
 * @param value       Data to be written to DAC output registers.
 *
 * @retval 0        On success.
 * @retval -EINVAL  If a parameter with an invalid value has been provided.
 */
__syscall int dac_write_value(const struct device *dev, uint8_t channel,
			      uint32_t value);

static inline int z_impl_dac_write_value(const struct device *dev,
						uint8_t channel, uint32_t value)
{
	const struct dac_driver_api *api =
				(const struct dac_driver_api *)dev->api;

	return api->write_value(dev, channel, value);
}

/**
 * @brief Write a single value to a DAC channel from a struct dac_dt_spec.
 *
 * @param spec   DAC specification from Devicetree.
 * @param value  Data to be written to DAC output registers.
 *
 * @return A value from dac_write_value() or -ENOTSUP if information from
 * Devicetree is not valid.
 * @see dac_write_value()
 */
static inline int dac_write_value_dt(const struct dac_dt_spec *spec,
				     uint32_t value)
{
	if (!spec->channel_cfg_dt_node_exists) {
		return -ENOTSUP;
	}

	return dac_write_value(spec->dev, spec->channel_id, value);
}

/**
 * @brief Conversion from specified input units to raw DAC units
 *
 * This function performs the necessary conversion to transform a
 * physical voltage to a raw DAC value.
 *
 * @param ref_mv the reference voltage used for the value, in
 * millivolts.
 *
 * @param resolution the number of bits in the absolute value of the
 * output.
 *
 * @param valp pointer to the physical voltage value on input, and the
 * corresponding raw output value on successful conversion.  If
 * conversion fails the stored value is left unchanged.
 *
 * @retval 0 on successful conversion
 */
typedef int (*dac_x_to_raw_fn)(uint32_t ref_mv, uint8_t resolution, uint32_t *valp);

/**
 * @brief Convert a millivolts value to a raw DAC value.
 *
 * @see dac_x_to_raw_fn
 */
static inline int dac_millivolts_to_raw(uint32_t ref_mv, uint8_t resolution, uint32_t *valp)
{
	uint64_t dac_mv = (((uint64_t)*valp) << resolution) / (uint64_t)ref_mv;

	if (dac_mv > (1UL << resolution)) {
		__ASSERT_MSG_INFO("conversion result is out of range");
		return -ERANGE;
	}

	*valp = (uint32_t)dac_mv;

	return 0;
}

/**
 * @brief Convert a raw DAC value to microvolts.
 *
 * @see dac_x_to_raw_fn
 */
static inline int dac_microvolts_to_raw(uint32_t ref_mv, uint8_t resolution, uint32_t *valp)
{
	uint64_t dac_uv = (((uint64_t)*valp) << resolution) / (uint64_t)ref_mv / (uint64_t)1000;

	if (dac_uv > (1UL << resolution)) {
		__ASSERT_MSG_INFO("conversion result is out of range");
		return -ERANGE;
	}

	*valp = (uint32_t)dac_uv;

	return 0;
}

/**
 * @brief Convert a raw DAC value to an arbitrary output unit
 *
 * @param[in] conv_func Function that converts to the final output unit.
 * @param[in] spec DAC specification from Devicetree.
 * @param[in,out] valp pointer to the physical voltage value on input, and the
 * corresponding raw output value on successful conversion.  If
 * conversion fails the stored value is left unchanged.
 *
 * @return A value from dac_x_to_raw_dt_chan or -ENOTSUP if information from
 * Devicetree is not valid.
 * @see dac_x_to_raw_fn
 */
static inline int dac_x_to_raw_dt_chan(dac_x_to_raw_fn conv_func,
					    const struct dac_dt_spec *spec,
					    uint32_t *valp)
{
	if (!spec->channel_cfg_dt_node_exists) {
		return -ENOTSUP;
	}

	return conv_func(spec->vref_mv, spec->channel_cfg.resolution, valp);
}

/**
 * @brief Convert a millivolts value to raw DAC using information stored
 * in a struct dac_dt_spec.
 *
 * @param[in] spec DAC specification from Devicetree.
 * @param[in,out] valp Pointer to the raw measurement value on input, and the
 * corresponding millivolt value on successful conversion. If conversion fails
 * the stored value is left unchanged.
 *
 * @return A value from dac_millivolts_to_raw() or -ENOTSUP if information from
 * Devicetree is not valid.
 * @see dac_millivolts_to_raw()
 */
static inline int dac_millivolts_to_raw_dt(const struct dac_dt_spec *spec, uint32_t *valp)
{
	return dac_x_to_raw_dt_chan(dac_millivolts_to_raw, spec, valp);
}

/**
 * @brief Convert a microvolts value to raw DAC value using information stored
 * in a struct dac_dt_spec.
 *
 * @param[in] spec DAC specification from Devicetree.
 * @param[in,out] valp Pointer to the raw measurement value on input, and the
 * corresponding microvolt value on successful conversion. If conversion fails
 * the stored value is left unchanged.
 *
 * @return A value from dac_microvolts_to_raw() or -ENOTSUP if information from
 * Devicetree is not valid.
 * @see dac_microvolts_to_raw()
 */
static inline int dac_microvolts_to_raw_dt(const struct dac_dt_spec *spec, uint32_t *valp)
{
	return dac_x_to_raw_dt_chan(dac_microvolts_to_raw, spec, valp);
}

/**
 * @brief Validate that the DAC device is ready.
 *
 * @param spec DAC specification from devicetree
 *
 * @return true if the DAC device is ready for use and false otherwise.
 */
static inline bool dac_is_ready_dt(const struct dac_dt_spec *spec)
{
	return spec->channel_cfg_dt_node_exists && device_is_ready(spec->dev);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/dac.h>

#endif  /* ZEPHYR_INCLUDE_DRIVERS_DAC_H_ */
