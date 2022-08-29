/*
 * Copyright (c) 2022 ITE Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_RISCV_ITE_IT8XXX2_COMMON_PINCTRL_KSCAN_SOC_H_
#define ZEPHYR_SOC_RISCV_ITE_IT8XXX2_COMMON_PINCTRL_KSCAN_SOC_H_

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

/**
 * @brief ITE IT8XXX2 kscan pin control type.
 */
struct pinctrl_kscan_soc_pin {
	/* Pinctrl group */
	const struct device *pinctrls;
	/* Pin configuration (pullup, push-pull/open drain) */
	uint32_t pincfg;
	/* KSI[7:0]/KSO[15:0] pin */
	uint8_t pin;
};

struct pinctrl_kscan_dev_config {
	/* Pin configurations */
	const struct pinctrl_kscan_soc_pin *pins;
	/* Pin count of pinctrl-kscan property */
	uint8_t pin_cnt;
};

#define IT8XXX2_KSI_KSO_PUSH_PULL      0x0U
#define IT8XXX2_KSI_KSO_OPEN_DRAIN     0x1U
#define IT8XXX2_KSI_KSO_NOT_PULL_UP    0x0U
#define IT8XXX2_KSI_KSO_PULL_UP        0x1U

/**
 * @brief PIN configuration bitfield.
 *
 * Pin bitfield configuration is defined as following:
 * Bit[0] Pin push-pull = 0b, open-drain = 1b
 * Bit[2] Pin not pull-up = 0b, pull-up = 1b
 */
/* Pin push-pull/open-drain mode */
#define IT8XXX2_KSI_KSO_PP_OD_SHIFT    0U
#define IT8XXX2_KSI_KSO_PP_OD_MASK     BIT_MASK(1)
/* Pin pull-up */
#define IT8XXX2_KSI_KSO_PULL_UP_SHIFT  2U
#define IT8XXX2_KSI_KSO_PULL_UP_MASK   BIT_MASK(1)

/**
 * @brief Utility macro to obtain configuration of push-pull/open-drain mode.
 */
#define IT8XXX2_PINCTRL_KSCAN_DT_PINCFG_PP_OD(__mode) \
	(((__mode) >> IT8XXX2_KSI_KSO_PP_OD_SHIFT) & IT8XXX2_KSI_KSO_PP_OD_MASK)

/**
 * @brief Utility macro to obtain configuration of pull-up or not.
 */
#define IT8XXX2_PINCTRL_KSCAN_DT_PINCFG_PULLUP(__mode) \
	(((__mode) >> IT8XXX2_KSI_KSO_PULL_UP_SHIFT) & IT8XXX2_KSI_KSO_PULL_UP_MASK)

/**
 * @brief Utility macro to initialize pinctrls member of #pinctrl_kscan_soc_pin.
 *
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_KSCAN_IT8XXX2_PINCTRL_INIT(node_id) \
	DEVICE_DT_GET(DT_PHANDLE(node_id, pinmuxs))

/**
 * @brief Utility macro to initialize pincfg member of #pinctrl_kscan_soc_pin.
 *
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_KSCAN_IT8XXX2_PINCFG_INIT(node_id)                            \
	(((IT8XXX2_KSI_KSO_OPEN_DRAIN * DT_PROP(node_id, drive_open_drain))     \
	   << IT8XXX2_KSI_KSO_PP_OD_SHIFT) |                                    \
	  ((IT8XXX2_KSI_KSO_PULL_UP * DT_PROP(node_id, bias_pull_up))           \
	   << IT8XXX2_KSI_KSO_PULL_UP_SHIFT))

/**
 * @brief Utility macro to initialize pin member of #pinctrl_kscan_soc_pin.
 *
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_KSCAN_IT8XXX2_PIN_INIT(node_id) \
	DT_PHA(node_id, pinmuxs, pin)

/**
 * @brief Utility macro to initialize pin_cnt member of #pinctrl_kscan_soc_pin.
 *
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_KSCAN_IT8XXX2_PIN_CNT_INIT(node_id) \
	DT_PROP_LEN(node_id, pinctrl_kscan)

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param prop Property name, ex. pinctrl-kscan.
 * @param idx Property elements index.
 */
#define Z_PINCTRL_KSCAN_PIN_INIT(node_id, prop, idx)				\
	{									\
		.pinctrls = Z_PINCTRL_KSCAN_IT8XXX2_PINCTRL_INIT(		\
			    DT_PROP_BY_IDX(node_id, prop, idx)),		\
		.pincfg = Z_PINCTRL_KSCAN_IT8XXX2_PINCFG_INIT(			\
			  DT_PROP_BY_IDX(node_id, prop, idx)),			\
		.pin = Z_PINCTRL_KSCAN_IT8XXX2_PIN_INIT(			\
		       DT_PROP_BY_IDX(node_id, prop, idx)),			\
	},

/**
 * @brief Utility macro to initialize kscan pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop Property name, ex. pinctrl-kscan.
 */
#define Z_PINCTRL_KSCAN_PINS_INIT(node_id, prop) \
	{DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_KSCAN_PIN_INIT)};

/**
 * @brief Obtain the variable name storing kscan pinctrl pins for the given DT
 * node identifier.
 *
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_KSCAN_PINS_NAME(node_id) \
	_CONCAT(__pinctrl_kscan_pins, DEVICE_DT_NAME_GET(node_id))

/**
 * @brief Obtain the variable name storing kscan pinctrl config for the given DT
 * node identifier.
 *
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_KSCAN_DEV_CONFIG_NAME(node_id) \
	_CONCAT(__pinctrl_kscan_dev_config, DEVICE_DT_NAME_GET(node_id))

/**
 * @brief Helper macro to define pins for a given pin.
 *
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_KSCAN_PINS_DEFINE(node_id)					\
	static const struct pinctrl_kscan_soc_pin				\
	Z_PINCTRL_KSCAN_PINS_NAME(node_id)[] =					\
	Z_PINCTRL_KSCAN_PINS_INIT(node_id, pinctrl_kscan)

/**
 * @brief Helper macro to initialize kscan pin control config.
 *
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_KSCAN_DEV_CONFIG_INIT(node_id)				\
	{									\
		.pins = Z_PINCTRL_KSCAN_PINS_NAME(node_id),			\
		.pin_cnt = ARRAY_SIZE(Z_PINCTRL_KSCAN_PINS_NAME(node_id))	\
	}

/**
 * @brief Define all kscan pin control information for the given node identifier.
 *
 * @param node_id Node identifier.
 */
#define PINCTRL_KSCAN_DT_DEFINE(node_id)					\
	Z_PINCTRL_KSCAN_PINS_DEFINE(node_id)					\
	struct pinctrl_kscan_dev_config Z_PINCTRL_KSCAN_DEV_CONFIG_NAME(node_id) =	\
	Z_PINCTRL_KSCAN_DEV_CONFIG_INIT(node_id)

/**
 * @brief Define all kscan pin control information for the given compatible index.
 *
 * @param inst Instance number.
 */
#define PINCTRL_KSCAN_DT_INST_DEFINE(inst) \
	PINCTRL_KSCAN_DT_DEFINE(DT_DRV_INST(inst))

/**
 * @brief Obtain a reference to the kscan pin control configuration given a node
 * identifier.
 *
 * @param node_id Node identifier.
 */
#define PINCTRL_KSCAN_DT_DEV_CONFIG_GET(node_id) \
	&Z_PINCTRL_KSCAN_DEV_CONFIG_NAME(node_id)

/**
 * @brief Obtain a reference to the kscan pin control configuration given current
 * compatible instance number.
 *
 * @param inst Instance number.
 */
#define PINCTRL_KSCAN_DT_INST_DEV_CONFIG_GET(inst) \
	PINCTRL_KSCAN_DT_DEV_CONFIG_GET(DT_DRV_INST(inst))

#endif /* ZEPHYR_SOC_RISCV_ITE_IT8XXX2_COMMON_PINCTRL_KSCAN_SOC_H_ */
