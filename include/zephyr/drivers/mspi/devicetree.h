/*
 * Copyright (c) 2024, Ambiq Micro Inc. <www.ambiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MSPI_DEVICETREE_H_
#define ZEPHYR_INCLUDE_DRIVERS_MSPI_DEVICETREE_H_

/**
 * @brief MSPI Devicetree related macros
 * @defgroup mspi_devicetree MSPI Devicetree related macros
 * @ingroup mspi_interface
 * @{
 */

#include <zephyr/drivers/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Structure initializer for <tt>struct mspi_dev_cfg</tt> from devicetree
 *
 * This helper macro expands to a static initializer for a <tt>struct
 * mspi_dev_cfg</tt> by reading the relevant data from the devicetree.
 *
 * @param mspi_dev Devicetree node identifier for the MSPI device whose
 *                 struct mspi_dev_cfg to create an initializer for
 */
#define MSPI_DEVICE_CONFIG_DT(mspi_dev)                                                           \
	{                                                                                         \
		.ce_num               = DT_PROP_OR(mspi_dev, mspi_hardware_ce_num, 0),            \
		.freq                 = DT_PROP(mspi_dev, mspi_max_frequency),                    \
		.io_mode              = DT_ENUM_IDX_OR(mspi_dev, mspi_io_mode,                    \
						MSPI_IO_MODE_SINGLE),                             \
		.data_rate            = DT_ENUM_IDX_OR(mspi_dev, mspi_data_rate,                  \
						MSPI_DATA_RATE_SINGLE),                           \
		.cpp                  = DT_ENUM_IDX_OR(mspi_dev, mspi_cpp_mode, MSPI_CPP_MODE_0), \
		.endian               = DT_ENUM_IDX_OR(mspi_dev, mspi_endian,                     \
						MSPI_XFER_LITTLE_ENDIAN),                         \
		.ce_polarity          = DT_ENUM_IDX_OR(mspi_dev, mspi_ce_polarity,                \
						MSPI_CE_ACTIVE_LOW),                              \
		.dqs_enable           = DT_PROP(mspi_dev, mspi_dqs_enable),                       \
		.rx_dummy             = DT_PROP_OR(mspi_dev, rx_dummy, 0),                        \
		.tx_dummy             = DT_PROP_OR(mspi_dev, tx_dummy, 0),                        \
		.read_cmd             = DT_PROP_OR(mspi_dev, read_command, 0),                    \
		.write_cmd            = DT_PROP_OR(mspi_dev, write_command, 0),                   \
		.cmd_length           = DT_ENUM_IDX_OR(mspi_dev, command_length, 0),              \
		.addr_length          = DT_ENUM_IDX_OR(mspi_dev, address_length, 0),              \
		.mem_boundary         = COND_CODE_1(DT_NODE_HAS_PROP(mspi_dev, ce_break_config),  \
						(DT_PROP_BY_IDX(mspi_dev, ce_break_config, 0)),   \
						(0)),                                             \
		.time_to_break        = COND_CODE_1(DT_NODE_HAS_PROP(mspi_dev, ce_break_config),  \
						(DT_PROP_BY_IDX(mspi_dev, ce_break_config, 1)),   \
						(0)),                                             \
	}

/**
 * @brief Structure initializer for <tt>struct mspi_dev_cfg</tt> from devicetree instance
 *
 * This is equivalent to
 * <tt>MSPI_DEVICE_CONFIG_DT(DT_DRV_INST(inst))</tt>.
 *
 * @param inst Devicetree instance number
 */
#define MSPI_DEVICE_CONFIG_DT_INST(inst) MSPI_DEVICE_CONFIG_DT(DT_DRV_INST(inst))

/**
 * @brief Structure initializer for <tt>struct mspi_xip_cfg</tt> from devicetree
 *
 * This helper macro expands to a static initializer for a <tt>struct
 * mspi_xip_cfg</tt> by reading the relevant data from the devicetree.
 *
 * @param mspi_dev Devicetree node identifier for the MSPI device whose
 *                 struct mspi_xip_cfg to create an initializer for
 */
#define MSPI_XIP_CONFIG_DT_NO_CHECK(mspi_dev)                                                     \
	{                                                                                         \
		.enable               = DT_PROP_BY_IDX(mspi_dev, xip_config, 0),                  \
		.address_offset       = DT_PROP_BY_IDX(mspi_dev, xip_config, 1),                  \
		.size                 = DT_PROP_BY_IDX(mspi_dev, xip_config, 2),                  \
		.permission           = DT_PROP_BY_IDX(mspi_dev, xip_config, 3),                  \
	}

/**
 * @brief Structure initializer for <tt>struct mspi_xip_cfg</tt> from devicetree
 *
 * This helper macro check whether <tt>xip_config</tt> binding exist first
 * before calling <tt>MSPI_XIP_CONFIG_DT_NO_CHECK</tt>.
 *
 * @param mspi_dev Devicetree node identifier for the MSPI device whose
 *                 struct mspi_xip_cfg to create an initializer for
 */
#define MSPI_XIP_CONFIG_DT(mspi_dev)                                                              \
		COND_CODE_1(DT_NODE_HAS_PROP(mspi_dev, xip_config),                               \
			(MSPI_XIP_CONFIG_DT_NO_CHECK(mspi_dev)),                                  \
			({}))

/**
 * @brief Structure initializer for <tt>struct mspi_xip_cfg</tt> from devicetree instance
 *
 * This is equivalent to
 * <tt>MSPI_XIP_CONFIG_DT(DT_DRV_INST(inst))</tt>.
 *
 * @param inst Devicetree instance number
 */
#define MSPI_XIP_CONFIG_DT_INST(inst) MSPI_XIP_CONFIG_DT(DT_DRV_INST(inst))

/**
 * @brief Structure initializer for <tt>struct mspi_scramble_cfg</tt> from devicetree
 *
 * This helper macro expands to a static initializer for a <tt>struct
 * mspi_scramble_cfg</tt> by reading the relevant data from the devicetree.
 *
 * @param mspi_dev Devicetree node identifier for the MSPI device whose
 *                 struct mspi_scramble_cfg to create an initializer for
 */
#define MSPI_SCRAMBLE_CONFIG_DT_NO_CHECK(mspi_dev)                                                \
	{                                                                                         \
		.enable               = DT_PROP_BY_IDX(mspi_dev, scramble_config, 0),             \
		.address_offset       = DT_PROP_BY_IDX(mspi_dev, scramble_config, 1),             \
		.size                 = DT_PROP_BY_IDX(mspi_dev, scramble_config, 2),             \
	}

/**
 * @brief Structure initializer for <tt>struct mspi_scramble_cfg</tt> from devicetree
 *
 * This helper macro check whether <tt>scramble_config</tt> binding exist first
 * before calling <tt>MSPI_SCRAMBLE_CONFIG_DT_NO_CHECK</tt>.
 *
 * @param mspi_dev Devicetree node identifier for the MSPI device whose
 *                 struct mspi_scramble_cfg to create an initializer for
 */
#define MSPI_SCRAMBLE_CONFIG_DT(mspi_dev)                                                         \
		COND_CODE_1(DT_NODE_HAS_PROP(mspi_dev, scramble_config),                          \
			(MSPI_SCRAMBLE_CONFIG_DT_NO_CHECK(mspi_dev)),                             \
			({}))

/**
 * @brief Structure initializer for <tt>struct mspi_scramble_cfg</tt> from devicetree instance
 *
 * This is equivalent to
 * <tt>MSPI_SCRAMBLE_CONFIG_DT(DT_DRV_INST(inst))</tt>.
 *
 * @param inst Devicetree instance number
 */
#define MSPI_SCRAMBLE_CONFIG_DT_INST(inst) MSPI_SCRAMBLE_CONFIG_DT(DT_DRV_INST(inst))

/**
 * @brief Structure initializer for <tt>struct mspi_dev_id</tt> from devicetree
 *
 * This helper macro expands to a static initializer for a <tt>struct
 * mspi_dev_id</tt> by reading the relevant data from the devicetree.
 *
 * @param mspi_dev Devicetree node identifier for the MSPI device whose
 *                 struct mspi_dev_id to create an initializer for
 */
#define MSPI_DEVICE_ID_DT(mspi_dev)                                                               \
	{                                                                                         \
		.ce                   = MSPI_DEV_CE_GPIOS_DT_SPEC_GET(mspi_dev),                  \
		.dev_idx              = DT_REG_ADDR(mspi_dev),                                    \
	}

/**
 * @brief Structure initializer for <tt>struct mspi_dev_id</tt> from devicetree instance
 *
 * This is equivalent to
 * <tt>MSPI_DEVICE_ID_DT(DT_DRV_INST(inst))</tt>.
 *
 * @param inst Devicetree instance number
 */
#define MSPI_DEVICE_ID_DT_INST(inst) MSPI_DEVICE_ID_DT(DT_DRV_INST(inst))


/**
 * @brief Get a <tt>struct gpio_dt_spec</tt> for a MSPI device's chip enable pin
 *
 * Example devicetree fragment:
 *
 * @code{.devicetree}
 *     gpio1: gpio@abcd0001 { ... };
 *
 *     gpio2: gpio@abcd0002 { ... };
 *
 *     mspi@abcd0003 {
 *             compatible = "ambiq,mspi";
 *             ce-gpios = <&gpio1 10 GPIO_ACTIVE_LOW>,
 *                        <&gpio2 20 GPIO_ACTIVE_LOW>;
 *
 *             a: mspi-dev-a@0 {
 *                     reg = <0>;
 *             };
 *
 *             b: mspi-dev-b@1 {
 *                     reg = <1>;
 *             };
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     MSPI_DEV_CE_GPIOS_DT_SPEC_GET(DT_NODELABEL(a)) \
 *           // { DEVICE_DT_GET(DT_NODELABEL(gpio1)), 10, GPIO_ACTIVE_LOW }
 *     MSPI_DEV_CE_GPIOS_DT_SPEC_GET(DT_NODELABEL(b)) \
 *           // { DEVICE_DT_GET(DT_NODELABEL(gpio2)), 20, GPIO_ACTIVE_LOW }
 * @endcode
 *
 * @param mspi_dev a MSPI device node identifier
 * @return #gpio_dt_spec struct corresponding with mspi_dev's chip enable
 */
#define MSPI_DEV_CE_GPIOS_DT_SPEC_GET(mspi_dev)                                                   \
		GPIO_DT_SPEC_GET_BY_IDX_OR(DT_BUS(mspi_dev), ce_gpios,                            \
					   DT_REG_ADDR_RAW(mspi_dev), {})

/**
 * @brief Get a <tt>struct gpio_dt_spec</tt> for a MSPI device's chip enable pin
 *
 * This is equivalent to
 * <tt>MSPI_DEV_CE_GPIOS_DT_SPEC_GET(DT_DRV_INST(inst))</tt>.
 *
 * @param inst Devicetree instance number
 * @return #gpio_dt_spec struct corresponding with mspi_dev's chip enable
 */
#define MSPI_DEV_CE_GPIOS_DT_SPEC_INST_GET(inst)                                                  \
		MSPI_DEV_CE_GPIOS_DT_SPEC_GET(DT_DRV_INST(inst))


/**
 * @brief Get an array of <tt>struct gpio_dt_spec</tt> from devicetree for a MSPI controller
 *
 * This helper macro check whether <tt>ce_gpios</tt> binding exist first
 * before calling <tt>GPIO_DT_SPEC_GET_BY_IDX</tt> and expand to an array of
 * gpio_dt_spec.
 *
 * @param node_id Devicetree node identifier for the MSPI controller
 * @return an array of gpio_dt_spec struct corresponding with mspi_dev's chip enables
 */
#define MSPI_CE_GPIOS_DT_SPEC_GET(node_id)                                                        \
{                                                                                                 \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, ce_gpios),                                          \
		(DT_FOREACH_PROP_ELEM_SEP(node_id, ce_gpios, GPIO_DT_SPEC_GET_BY_IDX, (,))),      \
		())                                                                               \
}

/**
 * @brief Get an array of <tt>struct gpio_dt_spec</tt> for a MSPI controller
 *
 * This is equivalent to
 * <tt>MSPI_CE_GPIOS_DT_SPEC_GET(DT_DRV_INST(inst))</tt>.
 *
 * @param inst Devicetree instance number
 * @return an array of gpio_dt_spec struct corresponding with mspi_dev's chip enables
 */
#define MSPI_CE_GPIOS_DT_SPEC_INST_GET(inst)                                                      \
		MSPI_CE_GPIOS_DT_SPEC_GET(DT_DRV_INST(inst))

/**
 * @brief Initialize and get a pointer to a @p mspi_ce_control from a
 *        devicetree node identifier
 *
 * This helper is useful for initializing a device on a MSPI bus. It
 * initializes a struct mspi_ce_control and returns a pointer to it.
 * Here, @p node_id is a node identifier for a MSPI device, not a MSPI
 * controller.
 *
 * Example devicetree fragment:
 *
 * @code{.devicetree}
 *     mspi@abcd0001 {
 *             ce-gpios = <&gpio0 1 GPIO_ACTIVE_LOW>;
 *             mspidev: mspi-device@0 { ... };
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     struct mspi_ce_control ctrl =
 *             MSPI_CE_CONTROL_INIT(DT_NODELABEL(mspidev), 2);
 * @endcode
 *
 * This example is equivalent to:
 *
 * @code{.c}
 *     struct mspi_ce_control ctrl = {
 *             .gpio = MSPI_DEV_CE_GPIOS_DT_SPEC_GET(DT_NODELABEL(mspidev)),
 *             .delay = 2,
 *     };
 * @endcode
 *
 * @param node_id Devicetree node identifier for a device on a MSPI bus
 * @param delay_ The @p delay field to set in the @p mspi_ce_control
 * @return a pointer to the @p mspi_ce_control structure
 */
#define MSPI_CE_CONTROL_INIT(node_id, delay_)                                                     \
	{                                                                                         \
		.gpio = MSPI_DEV_CE_GPIOS_DT_SPEC_GET(node_id), .delay = (delay_),                \
	}

/**
 * @brief Get a pointer to a @p mspi_ce_control from a devicetree node
 *
 * This is equivalent to
 * <tt>MSPI_CE_CONTROL_INIT(DT_DRV_INST(inst), delay)</tt>.
 *
 * Therefore, @p DT_DRV_COMPAT must already be defined before using
 * this macro.
 *
 * @param inst Devicetree node instance number
 * @param delay_ The @p delay field to set in the @p mspi_ce_control
 * @return a pointer to the @p mspi_ce_control structure
 */
#define MSPI_CE_CONTROL_INIT_INST(inst, delay_) MSPI_CE_CONTROL_INIT(DT_DRV_INST(inst), delay_)

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_MSPI_DEVICETREE_H_ */
