/*
 * Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for Infineon AutAnalog PTCOMP.
 *
 * These constants match the PDL enum values and are intended for use in
 * devicetree properties that configure the AutAnalog programmable threshold
 * comparator and its post-processing block.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_COMPARATOR_INFINEON_AUTANALOG_PTCOMP_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_COMPARATOR_INFINEON_AUTANALOG_PTCOMP_H_

/**
 * @name Power modes (cy_en_autanalog_ptcomp_comp_pwr_t)
 * @{
 */
#define IFX_AUTANALOG_PTCOMP_PWR_OFF    0 /**< Comparator powered off */
#define IFX_AUTANALOG_PTCOMP_PWR_ULP    1 /**< Ultra low power mode */
#define IFX_AUTANALOG_PTCOMP_PWR_LP     2 /**< Low power mode */
#define IFX_AUTANALOG_PTCOMP_PWR_NORMAL 3 /**< Normal power mode */
/** @} */

/**
 * @name Interrupt and edge detection modes (cy_en_autanalog_ptcomp_comp_int_t)
 * @{
 */
#define IFX_AUTANALOG_PTCOMP_INT_DISABLED     0 /**< Interrupt disabled */
#define IFX_AUTANALOG_PTCOMP_INT_EDGE_RISING  1 /**< Rising edge detection */
#define IFX_AUTANALOG_PTCOMP_INT_EDGE_FALLING 2 /**< Falling edge detection */
#define IFX_AUTANALOG_PTCOMP_INT_EDGE_BOTH    3 /**< Both-edge detection */
/** @} */

/**
 * @name Input mux selectors (cy_en_autanalog_ptcomp_comp_mux_t)
 * @{
 */
#define IFX_AUTANALOG_PTCOMP_MUX_CTB0_PIN1    0  /**< CTB0 GPIO pin 1 */
#define IFX_AUTANALOG_PTCOMP_MUX_CTB0_PIN4    1  /**< CTB0 GPIO pin 4 */
#define IFX_AUTANALOG_PTCOMP_MUX_CTB0_PIN6    2  /**< CTB0 GPIO pin 6 */
#define IFX_AUTANALOG_PTCOMP_MUX_CTB0_PIN7    3  /**< CTB0 GPIO pin 7 */
#define IFX_AUTANALOG_PTCOMP_MUX_CTB1_PIN1    4  /**< CTB1 GPIO pin 1 */
#define IFX_AUTANALOG_PTCOMP_MUX_CTB1_PIN4    5  /**< CTB1 GPIO pin 4 */
#define IFX_AUTANALOG_PTCOMP_MUX_CTB1_PIN6    6  /**< CTB1 GPIO pin 6 */
#define IFX_AUTANALOG_PTCOMP_MUX_CTB1_PIN7    7  /**< CTB1 GPIO pin 7 */
#define IFX_AUTANALOG_PTCOMP_MUX_CTB0_OA0_OUT 8  /**< CTB0 OpAmp0 output */
#define IFX_AUTANALOG_PTCOMP_MUX_CTB0_OA1_OUT 9  /**< CTB0 OpAmp1 output */
#define IFX_AUTANALOG_PTCOMP_MUX_CTB1_OA0_OUT 10 /**< CTB1 OpAmp0 output */
#define IFX_AUTANALOG_PTCOMP_MUX_CTB1_OA1_OUT 11 /**< CTB1 OpAmp1 output */
#define IFX_AUTANALOG_PTCOMP_MUX_DAC0         12 /**< DAC0 output */
#define IFX_AUTANALOG_PTCOMP_MUX_DAC1         13 /**< DAC1 output */
#define IFX_AUTANALOG_PTCOMP_MUX_PRB_OUT0     14 /**< PRB output 0 */
#define IFX_AUTANALOG_PTCOMP_MUX_PRB_OUT1     15 /**< PRB output 1 */
#define IFX_AUTANALOG_PTCOMP_MUX_GPIO0        16 /**< GPIO 0 */
#define IFX_AUTANALOG_PTCOMP_MUX_GPIO1        17 /**< GPIO 1 */
#define IFX_AUTANALOG_PTCOMP_MUX_GPIO2        18 /**< GPIO 2 */
#define IFX_AUTANALOG_PTCOMP_MUX_GPIO3        19 /**< GPIO 3 */
#define IFX_AUTANALOG_PTCOMP_MUX_GPIO4        20 /**< GPIO 4 */
#define IFX_AUTANALOG_PTCOMP_MUX_GPIO5        21 /**< GPIO 5 */
#define IFX_AUTANALOG_PTCOMP_MUX_GPIO6        22 /**< GPIO 6 */
#define IFX_AUTANALOG_PTCOMP_MUX_GPIO7        23 /**< GPIO 7 */
/** @} */

/**
 * @name Post-processing input source (cy_en_autanalog_ptcomp_pp_input_src_t)
 * @{
 */
#define IFX_AUTANALOG_PTCOMP_PP_SRC_DISABLED 0 /**< Post-processing input disabled */
#define IFX_AUTANALOG_PTCOMP_PP_SRC_COMP0    1 /**< Comparator 0 output */
#define IFX_AUTANALOG_PTCOMP_PP_SRC_COMP1    2 /**< Comparator 1 output */
/** @} */

/**
 * @name Post-processing input type (cy_en_autanalog_ptcomp_pp_input_type_t)
 * @{
 */
#define IFX_AUTANALOG_PTCOMP_PP_TYPE_LEVEL        0 /**< Level-sensitive input */
#define IFX_AUTANALOG_PTCOMP_PP_TYPE_EDGE_RISING  1 /**< Rising edge input */
#define IFX_AUTANALOG_PTCOMP_PP_TYPE_EDGE_FALLING 2 /**< Falling edge input */
#define IFX_AUTANALOG_PTCOMP_PP_TYPE_EDGE_BOTH    3 /**< Both-edge input */
/** @} */

/**
 * @name Post-processing counter mode (cy_en_autanalog_ptcomp_pp_cnt_mode_t)
 * @{
 */
#define IFX_AUTANALOG_PTCOMP_PP_MODE_DIRECT 0 /**< Direct counter mode */
#define IFX_AUTANALOG_PTCOMP_PP_MODE_FRAME  1 /**< Frame counter mode */
#define IFX_AUTANALOG_PTCOMP_PP_MODE_WINDOW 2 /**< Window counter mode */
/** @} */

/**
 * @name Post-processing data function / LUT (cy_en_autanalog_ptcomp_pp_data_func_t)
 * @{
 */
#define IFX_AUTANALOG_PTCOMP_PP_FUNC_A           0  /**< Pass input A */
#define IFX_AUTANALOG_PTCOMP_PP_FUNC_A_BAR_AND_B 1  /**< Logical NOT A AND B */
#define IFX_AUTANALOG_PTCOMP_PP_FUNC_B           2  /**< Pass input B */
#define IFX_AUTANALOG_PTCOMP_PP_FUNC_A_AND_B     3  /**< Logical A AND B */
#define IFX_AUTANALOG_PTCOMP_PP_FUNC_A_AND_B_BAR 4  /**< Logical A AND NOT B */
#define IFX_AUTANALOG_PTCOMP_PP_FUNC_A_XOR_B     5  /**< Logical A XOR B */
#define IFX_AUTANALOG_PTCOMP_PP_FUNC_A_OR_B      6  /**< Logical A OR B */
#define IFX_AUTANALOG_PTCOMP_PP_FUNC_A_NOR_B     7  /**< Logical A NOR B */
#define IFX_AUTANALOG_PTCOMP_PP_FUNC_A_XNOR_B    8  /**< Logical A XNOR B */
#define IFX_AUTANALOG_PTCOMP_PP_FUNC_B_BAR       9  /**< Pass logical NOT B */
#define IFX_AUTANALOG_PTCOMP_PP_FUNC_A_OR_B_BAR  10 /**< Logical A OR NOT B */
#define IFX_AUTANALOG_PTCOMP_PP_FUNC_A_BAR       11 /**< Pass logical NOT A */
#define IFX_AUTANALOG_PTCOMP_PP_FUNC_A_BAR_OR_B  12 /**< Logical NOT A OR B */
#define IFX_AUTANALOG_PTCOMP_PP_FUNC_A_NAND_B    13 /**< Logical A NAND B */
/** @} */

/**
 * @name Post-processing window size (cy_en_autanalog_ptcomp_pp_window_size_t)
 * @{
 */
#define IFX_AUTANALOG_PTCOMP_PP_WIN_2   0 /**< Window size of 2 samples */
#define IFX_AUTANALOG_PTCOMP_PP_WIN_4   1 /**< Window size of 4 samples */
#define IFX_AUTANALOG_PTCOMP_PP_WIN_8   2 /**< Window size of 8 samples */
#define IFX_AUTANALOG_PTCOMP_PP_WIN_16  3 /**< Window size of 16 samples */
#define IFX_AUTANALOG_PTCOMP_PP_WIN_32  4 /**< Window size of 32 samples */
#define IFX_AUTANALOG_PTCOMP_PP_WIN_64  5 /**< Window size of 64 samples */
#define IFX_AUTANALOG_PTCOMP_PP_WIN_128 6 /**< Window size of 128 samples */
/** @} */

/**
 * @name Post-processing limit condition (cy_en_autanalog_ptcomp_pp_cond_t)
 * @{
 */
#define IFX_AUTANALOG_PTCOMP_PP_COND_BELOW   0 /**< Result is below the threshold window */
#define IFX_AUTANALOG_PTCOMP_PP_COND_INSIDE  1 /**< Result is inside the threshold window */
#define IFX_AUTANALOG_PTCOMP_PP_COND_ABOVE   2 /**< Result is above the threshold window */
#define IFX_AUTANALOG_PTCOMP_PP_COND_OUTSIDE 3 /**< Result is outside the threshold window */
/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_COMPARATOR_INFINEON_AUTANALOG_PTCOMP_H_ */
