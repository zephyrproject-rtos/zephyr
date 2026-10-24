/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * LRF_Config assembly from ti,cc23x0-radio-config nodes.
 *
 */

#ifndef DRIVERS_BASIC_RADIO_BACKENDS_TI_CC23X0_BASIC_RADIO_CC23X0_RADIO_CONFIG_H_
#define DRIVERS_BASIC_RADIO_BACKENDS_TI_CC23X0_BASIC_RADIO_CC23X0_RADIO_CONFIG_H_

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <zephyr/dt-bindings/basic_radio/ti-cc23x0-radio-config.h>
#include <ti/drivers/rcl/LRF.h>

/* Unique symbol per DT node: basic_radio_lrf_<dep_ord>_<name> */
#define LRF_SYM(node_id, name) UTIL_CAT(UTIL_CAT(basic_radio_lrf_, DT_DEP_ORD(node_id)), UTIL_CAT(_, name))

/* Microcode image symbol: LRF_<CORE>_binary_<DT_STRING_TOKEN> */
#define LRF_IMAGE_SYM(cfg, core, prop) UTIL_CAT(UTIL_CAT(LRF_, core), UTIL_CAT(_binary_, DT_STRING_TOKEN(cfg, prop)))

/* TX shape N, defined only if ti,tx-shape-N is present */
#define LRF_TX_SHAPE_DEFINE(cfg, n)                                                \
    COND_CODE_1(DT_NODE_HAS_PROP(cfg, ti_tx_shape_##n),                            \
                (static const LRF_TxShape LRF_SYM(cfg, tx_shape_##n) = {           \
                     .scale    = DT_PROP(cfg, ti_tx_shape_##n##_scale),            \
                     .numCoeff = DT_PROP_LEN(cfg, ti_tx_shape_##n),                \
                     .coeff    = DT_PROP(cfg, ti_tx_shape_##n),                    \
                 };),                                                              \
                ())

/* txShape cell of a sw-config: shape index, or TI_LRF_NO_TX_SHAPE */
#define LRF_TX_SHAPE_REF_0(cfg)   (&LRF_SYM(cfg, tx_shape_0))
#define LRF_TX_SHAPE_REF_1(cfg)   (&LRF_SYM(cfg, tx_shape_1))
#define LRF_TX_SHAPE_REF_2(cfg)   (&LRF_SYM(cfg, tx_shape_2))
#define LRF_TX_SHAPE_REF_3(cfg)   (&LRF_SYM(cfg, tx_shape_3))
#define LRF_TX_SHAPE_REF_255(cfg) (NULL)
#define LRF_TX_SHAPE_REF(cfg, idx) UTIL_CAT(LRF_TX_SHAPE_REF_, idx)(cfg)

/* Software config N, defined only if ti,sw-config-N is present */
#define LRF_SW_CONFIG_DEFINE(cfg, n)                                                              \
    COND_CODE_1(DT_NODE_HAS_PROP(cfg, ti_sw_config_##n),                                          \
                (static const LRF_SwConfig LRF_SYM(cfg, sw_config_##n) = {                        \
                     .rxIntFrequency        = (int32_t)DT_PROP_BY_IDX(cfg, ti_sw_config_##n, 0),  \
                     .rxFrequencyOffset     = (int32_t)DT_PROP_BY_IDX(cfg, ti_sw_config_##n, 1),  \
                     .txFrequencyOffset     = (int32_t)DT_PROP_BY_IDX(cfg, ti_sw_config_##n, 2),  \
                     .modFrequencyDeviation = DT_PROP_BY_IDX(cfg, ti_sw_config_##n, 3),           \
                     .txShape = LRF_TX_SHAPE_REF(cfg, DT_PROP_BY_IDX(cfg, ti_sw_config_##n, 4)),  \
                     .bwIndex               = DT_PROP_BY_IDX(cfg, ti_sw_config_##n, 5),           \
                     .bwIndexDither         = DT_PROP_BY_IDX(cfg, ti_sw_config_##n, 6),           \
                 };),                                                                             \
                ())

#define LRF_SW_CONFIG_ADDR(cfg, n)                                         \
    COND_CODE_1(DT_NODE_HAS_PROP(cfg, ti_sw_config_##n),                   \
                ((uint32_t)&LRF_SYM(cfg, sw_config_##n)), (0U))

/*
 * One register word. TI_LRF_PTR_* tokens become the address of the
 * matching object; GCC folds this to a relocation, so segments stay const.
 */
#define LRF_WORD(cfg, v)                                                     \
    (((v) == TI_LRF_PTR_TX_POWER_TABLE) ? (uint32_t)&LRF_SYM(cfg, tx_power) : \
     ((v) == TI_LRF_PTR_FCFG_TRIMS)     ? (uint32_t)&(fcfg->appTrims) :       \
     ((v) == TI_LRF_PTR_SW_CONFIG_0)    ? LRF_SW_CONFIG_ADDR(cfg, 0) :        \
     ((v) == TI_LRF_PTR_SW_CONFIG_1)    ? LRF_SW_CONFIG_ADDR(cfg, 1) :        \
     ((v) == TI_LRF_PTR_SW_CONFIG_2)    ? LRF_SW_CONFIG_ADDR(cfg, 2) :        \
     ((v) == TI_LRF_PTR_SW_CONFIG_3)    ? LRF_SW_CONFIG_ADDR(cfg, 3) :        \
                                          (uint32_t)(v))

/* Segment node -> reg-config container -> ti,cc23x0-radio-config */
#define LRF_SEG_CFG(seg) DT_PARENT(DT_PARENT(seg))

#define LRF_SEG_WORD(seg, prop, idx) LRF_WORD(LRF_SEG_CFG(seg), DT_PROP_BY_IDX(seg, prop, idx))

#define LRF_SEG_DEFINE(seg) \
    static const uint32_t LRF_SYM(seg, words)[] = {DT_FOREACH_PROP_ELEM_SEP(seg, ti_words, LRF_SEG_WORD, (,))};

#define LRF_SEG_REF(seg) ((LRF_ConfigWord *)LRF_SYM(seg, words))

/* Unpack one TI_LRF_TX_POWER() cell */
#define LRF_TX_POWER_ENTRY(cfg, prop, idx)                                                    \
    {                                                                                         \
        .power     = {.fraction = 0, .dBm = (int8_t)(DT_PROP_BY_IDX(cfg, prop, idx) & 0xFF)}, \
        .tempCoeff = (DT_PROP_BY_IDX(cfg, prop, idx) >> 8) & 0xFF,                            \
        .value     = {.ib   = (DT_PROP_BY_IDX(cfg, prop, idx) >> 16) & 0x3F,                  \
                      .gain = (DT_PROP_BY_IDX(cfg, prop, idx) >> 22) & 0x7,                   \
                      .mode = (DT_PROP_BY_IDX(cfg, prop, idx) >> 25) & 0x1},                  \
    },

#define LRF_REG_CONFIG(cfg) DT_CHILD(cfg, reg_config)

#define LRF_CONFIG_DEFINE(cfg)                                                                \
    extern const uint32_t LRF_IMAGE_SYM(cfg, PBE, ti_pbe_image)[];                            \
    extern const uint32_t LRF_IMAGE_SYM(cfg, MCE, ti_mce_image)[];                            \
    extern const uint32_t LRF_IMAGE_SYM(cfg, RFE, ti_rfe_image)[];                            \
    static const LRF_TxPowerTable LRF_SYM(cfg, tx_power) = {                                  \
        .numEntries = DT_PROP_LEN(cfg, ti_tx_power_table),                                    \
        .powerTable = {DT_FOREACH_PROP_ELEM(cfg, ti_tx_power_table, LRF_TX_POWER_ENTRY)},     \
    };                                                                                        \
    LRF_TX_SHAPE_DEFINE(cfg, 0)                                                               \
    LRF_TX_SHAPE_DEFINE(cfg, 1)                                                               \
    LRF_TX_SHAPE_DEFINE(cfg, 2)                                                               \
    LRF_TX_SHAPE_DEFINE(cfg, 3)                                                               \
    LRF_SW_CONFIG_DEFINE(cfg, 0)                                                              \
    LRF_SW_CONFIG_DEFINE(cfg, 1)                                                              \
    LRF_SW_CONFIG_DEFINE(cfg, 2)                                                              \
    LRF_SW_CONFIG_DEFINE(cfg, 3)                                                              \
    DT_FOREACH_CHILD(LRF_REG_CONFIG(cfg), LRF_SEG_DEFINE)                                     \
    static const LRF_RegConfigList LRF_SYM(cfg, reg_config_list) = {                          \
        .numEntries = DT_CHILD_NUM(LRF_REG_CONFIG(cfg)),                                      \
        .entries    = {DT_FOREACH_CHILD_SEP(LRF_REG_CONFIG(cfg), LRF_SEG_REF, (,))},          \
    };                                                                                        \
    static const LRF_Config LRF_SYM(cfg, config) __maybe_unused = {                           \
        .pbeImage      = (const LRF_TOPsmImage *)LRF_IMAGE_SYM(cfg, PBE, ti_pbe_image),       \
        .mceImage      = (const LRF_TOPsmImage *)LRF_IMAGE_SYM(cfg, MCE, ti_mce_image),       \
        .rfeImage      = (const LRF_TOPsmImage *)LRF_IMAGE_SYM(cfg, RFE, ti_rfe_image),       \
        .regConfigList = &LRF_SYM(cfg, reg_config_list),                                      \
    };

/* Expands for every ti,cc23x0-radio-config with status = "okay" */
DT_FOREACH_STATUS_OKAY(ti_cc23x0_radio_config, LRF_CONFIG_DEFINE)

/*
 * TODO: check again if needed when hal_ti RCL driver is updated in upstream.
 * RCL_AdcNoise (used by the entropy driver) opens this config. The HAL
 * normally provides it with its pre-generated settings, which basic_radio
 * disables (CONFIG_RCL_RF_SETTING_PREGEN=n). It is not a PHY.

extern const uint32_t LRF_RFE_binary_ble5[];

static const LRF_RegConfigList adc_noise_reg_config_list = {
    .numEntries = 0,
};

const LRF_Config LRF_configAdcNoise = {
    .pbeImage      = NULL,
    .mceImage      = NULL,
    .rfeImage      = (const LRF_TOPsmImage *)LRF_RFE_binary_ble5,
    .regConfigList = &adc_noise_reg_config_list,
};
*/

#endif /* DRIVERS_BASIC_RADIO_BACKENDS_TI_CC23X0_BASIC_RADIO_CC23X0_RADIO_CONFIG_H_ */