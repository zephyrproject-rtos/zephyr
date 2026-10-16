/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODULES_TFM_INFINEON_PSE84_CYCFG_CLOCKS_DT_H_
#define ZEPHYR_MODULES_TFM_INFINEON_PSE84_CYCFG_CLOCKS_DT_H_

/*
 * PSE84 clock tree model derived from the devicetree of the Zephyr
 * non-secure image: node access, clock sources and the frequencies of the
 * DPLLs, clock paths and HF clocks, all evaluated at build time.
 */

#include <stdint.h>

#include "ifx_zephyr_dt.h"

/* Devicetree access */

#define CLK_NODE(label) DT_NODELABEL(label)
#define CLK_OKAY(label) DT_NODE_HAS_STATUS_OKAY(CLK_NODE(label))
#define CLK_FREQ(label)                                                                            \
	COND_CODE_1(CLK_OKAY(label), ((uint64_t)DT_PROP(CLK_NODE(label), clock_frequency)), (0U))

/* Clock path input: source-path of path_muxN, IHO if the mux is disabled */
/* Two-level helpers expand n first; UTIL_CAT cannot be nested in CONCAT */
#define PATH_NODE_(n) DT_NODELABEL(path_mux##n)
#define PATH_NODE(n)  PATH_NODE_(n)
#define PATH_OKAY(n)  DT_NODE_HAS_STATUS_OKAY(PATH_NODE(n))
#define PATH_SRC(n)                                                                                \
	COND_CODE_1(PATH_OKAY(n), (DT_PROP(PATH_NODE(n), source_path)), (IFX_CAT1_CLKPATH_IN_IHO))

/* DPLL on each of the first three clock paths */
#define PATH_DPLL_0   dpll_lp0
#define PATH_DPLL_1   dpll_lp1
#define PATH_DPLL_2   dpll_hp
#define PATH_DPLL_(n) PATH_DPLL_##n
#define PATH_DPLL(n)  PATH_DPLL_(n)

/* clock-div holds the register encoding: 0 divides by 1 */
#define HF_NODE(n) DT_NODELABEL(clk_hf##n)
#define HF_SRC(n)  DT_PROP(HF_NODE(n), source_path)
#define HF_DIV(n)  DT_PROP(HF_NODE(n), clock_div)

#define CLK_LF_SRC  DT_PROP(CLK_NODE(clk_lf), source_path)
#define CLK_BAK_SRC DT_PROP(CLK_NODE(clk_bak), source_path)

/* Frequency model, in 64-bit arithmetic (largest intermediate below 2^59) */

#define SRC_FREQ(src)                                                                              \
	((src) == IFX_CAT1_CLKPATH_IN_IHO   ? CLK_FREQ(clk_iho)                                    \
	 : (src) == IFX_CAT1_CLKPATH_IN_EXT ? CLK_FREQ(clk_ext)                                    \
	 : (src) == IFX_CAT1_CLKPATH_IN_ECO ? CLK_FREQ(clk_eco)                                    \
					    : 0U)

#define PATH_IN_FREQ(n) SRC_FREQ(PATH_SRC(n))

/* Divider of an enabled DPLL; 1 for a disabled one, so the model never divides by zero */
#define DPLL_DIV(label, prop)                                                                      \
	COND_CODE_1(CLK_OKAY(label), ((uint64_t)DT_PROP(CLK_NODE(label), prop)), (1U))
#define DPLL_FRAC(label) ((uint64_t)DT_PROP_OR(CLK_NODE(label), fraction_div, 0))

/* DPLL_LP on path n: fout = fin * (feedback + frac / 2^24) / (reference * output) */
#define DPLL_LP_FRAC_BITS  24U
#define DPLL_LP_NUM(label) ((DPLL_DIV(label, feedback_div) << DPLL_LP_FRAC_BITS) + DPLL_FRAC(label))
#define DPLL_LP_DEN(label)                                                                         \
	((DPLL_DIV(label, reference_div) * DPLL_DIV(label, output_div)) << DPLL_LP_FRAC_BITS)
#define DPLL_LP_FREQ(n) (PATH_IN_FREQ(n) * DPLL_LP_NUM(PATH_DPLL(n)) / DPLL_LP_DEN(PATH_DPLL(n)))

/* DPLL_HP on path n: fout = fin * (N + 1 + frac / 2^21) / ((P + 1) * (K + 1)) */
#define DPLL_HP_FRAC_BITS  21U
#define DPLL_HP_NUM(label) (((DPLL_DIV(label, div_n) + 1U) << DPLL_HP_FRAC_BITS) + DPLL_FRAC(label))
#define DPLL_HP_DEN(label)                                                                         \
	(((DPLL_DIV(label, div_p) + 1U) * (DPLL_DIV(label, div_k) + 1U)) << DPLL_HP_FRAC_BITS)
#define DPLL_HP_FREQ(n) (PATH_IN_FREQ(n) * DPLL_HP_NUM(PATH_DPLL(n)) / DPLL_HP_DEN(PATH_DPLL(n)))

/* Clock path output: DPLL output if the path has an enabled DPLL, else its input */
#define DPLL_PATH_OUT_FREQ(n) (CLK_OKAY(PATH_DPLL(n)) ? CLK_FREQ(PATH_DPLL(n)) : PATH_IN_FREQ(n))

#define PATH_OUT_FREQ_0   DPLL_PATH_OUT_FREQ(0)
#define PATH_OUT_FREQ_1   DPLL_PATH_OUT_FREQ(1)
#define PATH_OUT_FREQ_2   DPLL_PATH_OUT_FREQ(2)
#define PATH_OUT_FREQ_3   PATH_IN_FREQ(3)
#define PATH_OUT_FREQ_4   PATH_IN_FREQ(4)
#define PATH_OUT_FREQ_5   PATH_IN_FREQ(5)
#define PATH_OUT_FREQ_(n) PATH_OUT_FREQ_##n
#define PATH_OUT_FREQ(n)  PATH_OUT_FREQ_(n)

#define HF_FREQ(n) (PATH_OUT_FREQ(HF_SRC(n)) / (HF_DIV(n) + 1U))

#endif /* ZEPHYR_MODULES_TFM_INFINEON_PSE84_CYCFG_CLOCKS_DT_H_ */
