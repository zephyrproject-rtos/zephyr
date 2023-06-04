/*
 * Copyright (c) 2011-2014, Wind River Systems, Inc.
 * Copyright (c) 2020, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Misc utilities
 *
 * Repetitive or obscure helper macros needed by sys/util.h.
 */

#ifndef ZEPHYR_INCLUDE_SYS_UTIL_INTERNAL_H_
#define ZEPHYR_INCLUDE_SYS_UTIL_INTERNAL_H_

#include "util_loops.h"

/* IS_ENABLED() helpers */

/* This is called from IS_ENABLED(), and sticks on a "_XXXX" prefix,
 * it will now be "_XXXX1" if config_macro is "1", or just "_XXXX" if it's
 * undefined.
 *   ENABLED:   Z_IS_ENABLED2(_XXXX1)
 *   DISABLED   Z_IS_ENABLED2(_XXXX)
 */
#define Z_IS_ENABLED1(config_macro) Z_IS_ENABLED2(_XXXX##config_macro)

/* Here's the core trick, we map "_XXXX1" to "_YYYY," (i.e. a string
 * with a trailing comma), so it has the effect of making this a
 * two-argument tuple to the preprocessor only in the case where the
 * value is defined to "1"
 *   ENABLED:    _YYYY,    <--- note comma!
 *   DISABLED:   _XXXX
 */
#define _XXXX1 _YYYY,

/* Then we append an extra argument to fool the gcc preprocessor into
 * accepting it as a varargs macro.
 *                         arg1   arg2  arg3
 *   ENABLED:   Z_IS_ENABLED3(_YYYY,    1,    0)
 *   DISABLED   Z_IS_ENABLED3(_XXXX 1,  0)
 */
#define Z_IS_ENABLED2(one_or_two_args) Z_IS_ENABLED3(one_or_two_args 1, 0)

/* And our second argument is thus now cooked to be 1 in the case
 * where the value is defined to 1, and 0 if not:
 */
#define Z_IS_ENABLED3(ignore_this, val, ...) val

/* Implementation of IS_EQ(). Returns 1 if _0 and _1 are the same integer from
 * 0 to 255, 0 otherwise.
 */
#define Z_IS_EQ(_0, _1) Z_HAS_COMMA(Z_CAT4(Z_IS_, _0, _EQ_, _1)())

/* Used internally by COND_CODE_1 and COND_CODE_0. */
#define Z_COND_CODE_1(_flag, _if_1_code, _else_code) \
	__COND_CODE(_XXXX##_flag, _if_1_code, _else_code)
#define Z_COND_CODE_0(_flag, _if_0_code, _else_code) \
	__COND_CODE(_ZZZZ##_flag, _if_0_code, _else_code)
#define _ZZZZ0 _YYYY,
#define __COND_CODE(one_or_two_args, _if_code, _else_code) \
	__GET_ARG2_DEBRACKET(one_or_two_args _if_code, _else_code)

/* Gets second argument and removes brackets around that argument. It
 * is expected that the parameter is provided in brackets/parentheses.
 */
#define __GET_ARG2_DEBRACKET(ignore_this, val, ...) __DEBRACKET val

/* Used to remove brackets from around a single argument. */
#define __DEBRACKET(...) __VA_ARGS__

/* Used by IS_EMPTY() */
/* reference: https://gustedt.wordpress.com/2010/06/08/detect-empty-macro-arguments/ */
#define Z_HAS_COMMA(...) \
	NUM_VA_ARGS_LESS_1_IMPL(__VA_ARGS__, 1, 1, 1, 1, 1, 1, 1, 1, \
	 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
	 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
	 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0)
#define Z_TRIGGER_PARENTHESIS_(...) ,
#define Z_IS_EMPTY_(...) \
	Z_IS_EMPTY__( \
		Z_HAS_COMMA(__VA_ARGS__), \
		Z_HAS_COMMA(Z_TRIGGER_PARENTHESIS_ __VA_ARGS__), \
		Z_HAS_COMMA(__VA_ARGS__ (/*empty*/)), \
		Z_HAS_COMMA(Z_TRIGGER_PARENTHESIS_ __VA_ARGS__ (/*empty*/)))
#define Z_CAT4(_0, _1, _2, _3) _0 ## _1 ## _2 ## _3
#define Z_CAT5(_0, _1, _2, _3, _4) _0 ## _1 ## _2 ## _3 ## _4
#define Z_IS_EMPTY__(_0, _1, _2, _3) \
	Z_HAS_COMMA(Z_CAT5(Z_IS_EMPTY_CASE_, _0, _1, _2, _3))
#define Z_IS_EMPTY_CASE_0001 ,

/* Used by LIST_DROP_EMPTY() */
/* Adding ',' after each element would add empty element at the end of
 * list, which is hard to remove, so instead precede each element with ',',
 * this way first element is empty, and this one is easy to drop.
 */
#define Z_LIST_ADD_ELEM(e) EMPTY, e
#define Z_LIST_DROP_FIRST(...) GET_ARGS_LESS_N(1, __VA_ARGS__)
#define Z_LIST_NO_EMPTIES(e) \
	COND_CODE_1(IS_EMPTY(e), (), (Z_LIST_ADD_ELEM(e)))

#define UTIL_CAT(a, ...) UTIL_PRIMITIVE_CAT(a, __VA_ARGS__)
#define UTIL_PRIMITIVE_CAT(a, ...) a##__VA_ARGS__
#define UTIL_CHECK_N(x, n, ...) n
#define UTIL_CHECK(...) UTIL_CHECK_N(__VA_ARGS__, 0,)
#define UTIL_NOT(x) UTIL_CHECK(UTIL_PRIMITIVE_CAT(UTIL_NOT_, x))
#define UTIL_NOT_0 ~, 1,
#define UTIL_COMPL(b) UTIL_PRIMITIVE_CAT(UTIL_COMPL_, b)
#define UTIL_COMPL_0 1
#define UTIL_COMPL_1 0
#define UTIL_BOOL(x) UTIL_COMPL(UTIL_NOT(x))

#define UTIL_EVAL(...) __VA_ARGS__
#define UTIL_EXPAND(...) __VA_ARGS__
#define UTIL_REPEAT(...) UTIL_LISTIFY(__VA_ARGS__)

/* Implementation details for NUM_VA_ARGS_LESS_1 */
#define NUM_VA_ARGS_LESS_1_IMPL(				\
	_ignored,						\
	_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,		\
	_11, _12, _13, _14, _15, _16, _17, _18, _19, _20,	\
	_21, _22, _23, _24, _25, _26, _27, _28, _29, _30,	\
	_31, _32, _33, _34, _35, _36, _37, _38, _39, _40,	\
	_41, _42, _43, _44, _45, _46, _47, _48, _49, _50,	\
	_51, _52, _53, _54, _55, _56, _57, _58, _59, _60,	\
	_61, _62, N, ...) N

/* Used by MACRO_MAP_CAT */
#define MACRO_MAP_CAT_(...)						\
	/* To make sure it works also for 2 arguments in total */	\
	MACRO_MAP_CAT_N(NUM_VA_ARGS_LESS_1(__VA_ARGS__), __VA_ARGS__)
#define MACRO_MAP_CAT_N_(N, ...) UTIL_CAT(MACRO_MC_, N)(__VA_ARGS__,)
#define MACRO_MC_0(...)
#define MACRO_MC_1(m, a, ...)  m(a)
#define MACRO_MC_2(m, a, ...)  UTIL_CAT(m(a), MACRO_MC_1(m, __VA_ARGS__,))
#define MACRO_MC_3(m, a, ...)  UTIL_CAT(m(a), MACRO_MC_2(m, __VA_ARGS__,))
#define MACRO_MC_4(m, a, ...)  UTIL_CAT(m(a), MACRO_MC_3(m, __VA_ARGS__,))
#define MACRO_MC_5(m, a, ...)  UTIL_CAT(m(a), MACRO_MC_4(m, __VA_ARGS__,))
#define MACRO_MC_6(m, a, ...)  UTIL_CAT(m(a), MACRO_MC_5(m, __VA_ARGS__,))
#define MACRO_MC_7(m, a, ...)  UTIL_CAT(m(a), MACRO_MC_6(m, __VA_ARGS__,))
#define MACRO_MC_8(m, a, ...)  UTIL_CAT(m(a), MACRO_MC_7(m, __VA_ARGS__,))
#define MACRO_MC_9(m, a, ...)  UTIL_CAT(m(a), MACRO_MC_8(m, __VA_ARGS__,))
#define MACRO_MC_10(m, a, ...) UTIL_CAT(m(a), MACRO_MC_9(m, __VA_ARGS__,))
#define MACRO_MC_11(m, a, ...) UTIL_CAT(m(a), MACRO_MC_10(m, __VA_ARGS__,))
#define MACRO_MC_12(m, a, ...) UTIL_CAT(m(a), MACRO_MC_11(m, __VA_ARGS__,))
#define MACRO_MC_13(m, a, ...) UTIL_CAT(m(a), MACRO_MC_12(m, __VA_ARGS__,))
#define MACRO_MC_14(m, a, ...) UTIL_CAT(m(a), MACRO_MC_13(m, __VA_ARGS__,))
#define MACRO_MC_15(m, a, ...) UTIL_CAT(m(a), MACRO_MC_14(m, __VA_ARGS__,))

/* Used by Z_IS_EQ */
#define Z_IS_0_EQ_0(...) \,
#define Z_IS_1_EQ_1(...) \,
#define Z_IS_2_EQ_2(...) \,
#define Z_IS_3_EQ_3(...) \,
#define Z_IS_4_EQ_4(...) \,
#define Z_IS_5_EQ_5(...) \,
#define Z_IS_6_EQ_6(...) \,
#define Z_IS_7_EQ_7(...) \,
#define Z_IS_8_EQ_8(...) \,
#define Z_IS_9_EQ_9(...) \,
#define Z_IS_10_EQ_10(...) \,
#define Z_IS_11_EQ_11(...) \,
#define Z_IS_12_EQ_12(...) \,
#define Z_IS_13_EQ_13(...) \,
#define Z_IS_14_EQ_14(...) \,
#define Z_IS_15_EQ_15(...) \,
#define Z_IS_16_EQ_16(...) \,
#define Z_IS_17_EQ_17(...) \,
#define Z_IS_18_EQ_18(...) \,
#define Z_IS_19_EQ_19(...) \,
#define Z_IS_20_EQ_20(...) \,
#define Z_IS_21_EQ_21(...) \,
#define Z_IS_22_EQ_22(...) \,
#define Z_IS_23_EQ_23(...) \,
#define Z_IS_24_EQ_24(...) \,
#define Z_IS_25_EQ_25(...) \,
#define Z_IS_26_EQ_26(...) \,
#define Z_IS_27_EQ_27(...) \,
#define Z_IS_28_EQ_28(...) \,
#define Z_IS_29_EQ_29(...) \,
#define Z_IS_30_EQ_30(...) \,
#define Z_IS_31_EQ_31(...) \,
#define Z_IS_32_EQ_32(...) \,
#define Z_IS_33_EQ_33(...) \,
#define Z_IS_34_EQ_34(...) \,
#define Z_IS_35_EQ_35(...) \,
#define Z_IS_36_EQ_36(...) \,
#define Z_IS_37_EQ_37(...) \,
#define Z_IS_38_EQ_38(...) \,
#define Z_IS_39_EQ_39(...) \,
#define Z_IS_40_EQ_40(...) \,
#define Z_IS_41_EQ_41(...) \,
#define Z_IS_42_EQ_42(...) \,
#define Z_IS_43_EQ_43(...) \,
#define Z_IS_44_EQ_44(...) \,
#define Z_IS_45_EQ_45(...) \,
#define Z_IS_46_EQ_46(...) \,
#define Z_IS_47_EQ_47(...) \,
#define Z_IS_48_EQ_48(...) \,
#define Z_IS_49_EQ_49(...) \,
#define Z_IS_50_EQ_50(...) \,
#define Z_IS_51_EQ_51(...) \,
#define Z_IS_52_EQ_52(...) \,
#define Z_IS_53_EQ_53(...) \,
#define Z_IS_54_EQ_54(...) \,
#define Z_IS_55_EQ_55(...) \,
#define Z_IS_56_EQ_56(...) \,
#define Z_IS_57_EQ_57(...) \,
#define Z_IS_58_EQ_58(...) \,
#define Z_IS_59_EQ_59(...) \,
#define Z_IS_60_EQ_60(...) \,
#define Z_IS_61_EQ_61(...) \,
#define Z_IS_62_EQ_62(...) \,
#define Z_IS_63_EQ_63(...) \,
#define Z_IS_64_EQ_64(...) \,
#define Z_IS_65_EQ_65(...) \,
#define Z_IS_66_EQ_66(...) \,
#define Z_IS_67_EQ_67(...) \,
#define Z_IS_68_EQ_68(...) \,
#define Z_IS_69_EQ_69(...) \,
#define Z_IS_70_EQ_70(...) \,
#define Z_IS_71_EQ_71(...) \,
#define Z_IS_72_EQ_72(...) \,
#define Z_IS_73_EQ_73(...) \,
#define Z_IS_74_EQ_74(...) \,
#define Z_IS_75_EQ_75(...) \,
#define Z_IS_76_EQ_76(...) \,
#define Z_IS_77_EQ_77(...) \,
#define Z_IS_78_EQ_78(...) \,
#define Z_IS_79_EQ_79(...) \,
#define Z_IS_80_EQ_80(...) \,
#define Z_IS_81_EQ_81(...) \,
#define Z_IS_82_EQ_82(...) \,
#define Z_IS_83_EQ_83(...) \,
#define Z_IS_84_EQ_84(...) \,
#define Z_IS_85_EQ_85(...) \,
#define Z_IS_86_EQ_86(...) \,
#define Z_IS_87_EQ_87(...) \,
#define Z_IS_88_EQ_88(...) \,
#define Z_IS_89_EQ_89(...) \,
#define Z_IS_90_EQ_90(...) \,
#define Z_IS_91_EQ_91(...) \,
#define Z_IS_92_EQ_92(...) \,
#define Z_IS_93_EQ_93(...) \,
#define Z_IS_94_EQ_94(...) \,
#define Z_IS_95_EQ_95(...) \,
#define Z_IS_96_EQ_96(...) \,
#define Z_IS_97_EQ_97(...) \,
#define Z_IS_98_EQ_98(...) \,
#define Z_IS_99_EQ_99(...) \,
#define Z_IS_100_EQ_100(...) \,
#define Z_IS_101_EQ_101(...) \,
#define Z_IS_102_EQ_102(...) \,
#define Z_IS_103_EQ_103(...) \,
#define Z_IS_104_EQ_104(...) \,
#define Z_IS_105_EQ_105(...) \,
#define Z_IS_106_EQ_106(...) \,
#define Z_IS_107_EQ_107(...) \,
#define Z_IS_108_EQ_108(...) \,
#define Z_IS_109_EQ_109(...) \,
#define Z_IS_110_EQ_110(...) \,
#define Z_IS_111_EQ_111(...) \,
#define Z_IS_112_EQ_112(...) \,
#define Z_IS_113_EQ_113(...) \,
#define Z_IS_114_EQ_114(...) \,
#define Z_IS_115_EQ_115(...) \,
#define Z_IS_116_EQ_116(...) \,
#define Z_IS_117_EQ_117(...) \,
#define Z_IS_118_EQ_118(...) \,
#define Z_IS_119_EQ_119(...) \,
#define Z_IS_120_EQ_120(...) \,
#define Z_IS_121_EQ_121(...) \,
#define Z_IS_122_EQ_122(...) \,
#define Z_IS_123_EQ_123(...) \,
#define Z_IS_124_EQ_124(...) \,
#define Z_IS_125_EQ_125(...) \,
#define Z_IS_126_EQ_126(...) \,
#define Z_IS_127_EQ_127(...) \,
#define Z_IS_128_EQ_128(...) \,
#define Z_IS_129_EQ_129(...) \,
#define Z_IS_130_EQ_130(...) \,
#define Z_IS_131_EQ_131(...) \,
#define Z_IS_132_EQ_132(...) \,
#define Z_IS_133_EQ_133(...) \,
#define Z_IS_134_EQ_134(...) \,
#define Z_IS_135_EQ_135(...) \,
#define Z_IS_136_EQ_136(...) \,
#define Z_IS_137_EQ_137(...) \,
#define Z_IS_138_EQ_138(...) \,
#define Z_IS_139_EQ_139(...) \,
#define Z_IS_140_EQ_140(...) \,
#define Z_IS_141_EQ_141(...) \,
#define Z_IS_142_EQ_142(...) \,
#define Z_IS_143_EQ_143(...) \,
#define Z_IS_144_EQ_144(...) \,
#define Z_IS_145_EQ_145(...) \,
#define Z_IS_146_EQ_146(...) \,
#define Z_IS_147_EQ_147(...) \,
#define Z_IS_148_EQ_148(...) \,
#define Z_IS_149_EQ_149(...) \,
#define Z_IS_150_EQ_150(...) \,
#define Z_IS_151_EQ_151(...) \,
#define Z_IS_152_EQ_152(...) \,
#define Z_IS_153_EQ_153(...) \,
#define Z_IS_154_EQ_154(...) \,
#define Z_IS_155_EQ_155(...) \,
#define Z_IS_156_EQ_156(...) \,
#define Z_IS_157_EQ_157(...) \,
#define Z_IS_158_EQ_158(...) \,
#define Z_IS_159_EQ_159(...) \,
#define Z_IS_160_EQ_160(...) \,
#define Z_IS_161_EQ_161(...) \,
#define Z_IS_162_EQ_162(...) \,
#define Z_IS_163_EQ_163(...) \,
#define Z_IS_164_EQ_164(...) \,
#define Z_IS_165_EQ_165(...) \,
#define Z_IS_166_EQ_166(...) \,
#define Z_IS_167_EQ_167(...) \,
#define Z_IS_168_EQ_168(...) \,
#define Z_IS_169_EQ_169(...) \,
#define Z_IS_170_EQ_170(...) \,
#define Z_IS_171_EQ_171(...) \,
#define Z_IS_172_EQ_172(...) \,
#define Z_IS_173_EQ_173(...) \,
#define Z_IS_174_EQ_174(...) \,
#define Z_IS_175_EQ_175(...) \,
#define Z_IS_176_EQ_176(...) \,
#define Z_IS_177_EQ_177(...) \,
#define Z_IS_178_EQ_178(...) \,
#define Z_IS_179_EQ_179(...) \,
#define Z_IS_180_EQ_180(...) \,
#define Z_IS_181_EQ_181(...) \,
#define Z_IS_182_EQ_182(...) \,
#define Z_IS_183_EQ_183(...) \,
#define Z_IS_184_EQ_184(...) \,
#define Z_IS_185_EQ_185(...) \,
#define Z_IS_186_EQ_186(...) \,
#define Z_IS_187_EQ_187(...) \,
#define Z_IS_188_EQ_188(...) \,
#define Z_IS_189_EQ_189(...) \,
#define Z_IS_190_EQ_190(...) \,
#define Z_IS_191_EQ_191(...) \,
#define Z_IS_192_EQ_192(...) \,
#define Z_IS_193_EQ_193(...) \,
#define Z_IS_194_EQ_194(...) \,
#define Z_IS_195_EQ_195(...) \,
#define Z_IS_196_EQ_196(...) \,
#define Z_IS_197_EQ_197(...) \,
#define Z_IS_198_EQ_198(...) \,
#define Z_IS_199_EQ_199(...) \,
#define Z_IS_200_EQ_200(...) \,
#define Z_IS_201_EQ_201(...) \,
#define Z_IS_202_EQ_202(...) \,
#define Z_IS_203_EQ_203(...) \,
#define Z_IS_204_EQ_204(...) \,
#define Z_IS_205_EQ_205(...) \,
#define Z_IS_206_EQ_206(...) \,
#define Z_IS_207_EQ_207(...) \,
#define Z_IS_208_EQ_208(...) \,
#define Z_IS_209_EQ_209(...) \,
#define Z_IS_210_EQ_210(...) \,
#define Z_IS_211_EQ_211(...) \,
#define Z_IS_212_EQ_212(...) \,
#define Z_IS_213_EQ_213(...) \,
#define Z_IS_214_EQ_214(...) \,
#define Z_IS_215_EQ_215(...) \,
#define Z_IS_216_EQ_216(...) \,
#define Z_IS_217_EQ_217(...) \,
#define Z_IS_218_EQ_218(...) \,
#define Z_IS_219_EQ_219(...) \,
#define Z_IS_220_EQ_220(...) \,
#define Z_IS_221_EQ_221(...) \,
#define Z_IS_222_EQ_222(...) \,
#define Z_IS_223_EQ_223(...) \,
#define Z_IS_224_EQ_224(...) \,
#define Z_IS_225_EQ_225(...) \,
#define Z_IS_226_EQ_226(...) \,
#define Z_IS_227_EQ_227(...) \,
#define Z_IS_228_EQ_228(...) \,
#define Z_IS_229_EQ_229(...) \,
#define Z_IS_230_EQ_230(...) \,
#define Z_IS_231_EQ_231(...) \,
#define Z_IS_232_EQ_232(...) \,
#define Z_IS_233_EQ_233(...) \,
#define Z_IS_234_EQ_234(...) \,
#define Z_IS_235_EQ_235(...) \,
#define Z_IS_236_EQ_236(...) \,
#define Z_IS_237_EQ_237(...) \,
#define Z_IS_238_EQ_238(...) \,
#define Z_IS_239_EQ_239(...) \,
#define Z_IS_240_EQ_240(...) \,
#define Z_IS_241_EQ_241(...) \,
#define Z_IS_242_EQ_242(...) \,
#define Z_IS_243_EQ_243(...) \,
#define Z_IS_244_EQ_244(...) \,
#define Z_IS_245_EQ_245(...) \,
#define Z_IS_246_EQ_246(...) \,
#define Z_IS_247_EQ_247(...) \,
#define Z_IS_248_EQ_248(...) \,
#define Z_IS_249_EQ_249(...) \,
#define Z_IS_250_EQ_250(...) \,
#define Z_IS_251_EQ_251(...) \,
#define Z_IS_252_EQ_252(...) \,
#define Z_IS_253_EQ_253(...) \,
#define Z_IS_254_EQ_254(...) \,
#define Z_IS_255_EQ_255(...) \,

/*
 * Generic sparse list of odd numbers (check the implementation of
 * GPIO_DT_RESERVED_RANGES_NGPIOS as a usage example)
 */
#define Z_SPARSE_LIST_ODD_NUMBERS		\
	EMPTY,  1, EMPTY,  3, EMPTY,  5, EMPTY,  7, \
	EMPTY,  9, EMPTY, 11, EMPTY, 13, EMPTY, 15, \
	EMPTY, 17, EMPTY, 19, EMPTY, 21, EMPTY, 23, \
	EMPTY, 25, EMPTY, 27, EMPTY, 29, EMPTY, 31, \
	EMPTY, 33, EMPTY, 35, EMPTY, 37, EMPTY, 39, \
	EMPTY, 41, EMPTY, 43, EMPTY, 45, EMPTY, 47, \
	EMPTY, 49, EMPTY, 51, EMPTY, 53, EMPTY, 55, \
	EMPTY, 57, EMPTY, 59, EMPTY, 61, EMPTY, 63

/*
 * Generic sparse list of even numbers (check the implementation of
 * GPIO_DT_RESERVED_RANGES_NGPIOS as a usage example)
 */
#define Z_SPARSE_LIST_EVEN_NUMBERS		\
	 0, EMPTY,  2, EMPTY,  4, EMPTY,  6, EMPTY, \
	 8, EMPTY, 10, EMPTY, 12, EMPTY, 14, EMPTY, \
	16, EMPTY, 18, EMPTY, 20, EMPTY, 22, EMPTY, \
	24, EMPTY, 26, EMPTY, 28, EMPTY, 30, EMPTY, \
	32, EMPTY, 34, EMPTY, 36, EMPTY, 38, EMPTY, \
	40, EMPTY, 42, EMPTY, 44, EMPTY, 46, EMPTY, \
	48, EMPTY, 50, EMPTY, 52, EMPTY, 54, EMPTY, \
	56, EMPTY, 58, EMPTY, 60, EMPTY, 62, EMPTY

#define UTIL_INC_0 1
#define UTIL_INC_1 2
#define UTIL_INC_2 3
#define UTIL_INC_3 4
#define UTIL_INC_4 5
#define UTIL_INC_5 6
#define UTIL_INC_6 7
#define UTIL_INC_7 8
#define UTIL_INC_8 9
#define UTIL_INC_9 10
#define UTIL_INC_10 11
#define UTIL_INC_11 12
#define UTIL_INC_12 13
#define UTIL_INC_13 14
#define UTIL_INC_14 15
#define UTIL_INC_15 16
#define UTIL_INC_16 17
#define UTIL_INC_17 18
#define UTIL_INC_18 19
#define UTIL_INC_19 20
#define UTIL_INC_20 21
#define UTIL_INC_21 22
#define UTIL_INC_22 23
#define UTIL_INC_23 24
#define UTIL_INC_24 25
#define UTIL_INC_25 26
#define UTIL_INC_26 27
#define UTIL_INC_27 28
#define UTIL_INC_28 29
#define UTIL_INC_29 30
#define UTIL_INC_30 31
#define UTIL_INC_31 32
#define UTIL_INC_32 33
#define UTIL_INC_33 34
#define UTIL_INC_34 35
#define UTIL_INC_35 36
#define UTIL_INC_36 37
#define UTIL_INC_37 38
#define UTIL_INC_38 39
#define UTIL_INC_39 40
#define UTIL_INC_40 41
#define UTIL_INC_41 42
#define UTIL_INC_42 43
#define UTIL_INC_43 44
#define UTIL_INC_44 45
#define UTIL_INC_45 46
#define UTIL_INC_46 47
#define UTIL_INC_47 48
#define UTIL_INC_48 49
#define UTIL_INC_49 50
#define UTIL_INC_50 51
#define UTIL_INC_51 52
#define UTIL_INC_52 53
#define UTIL_INC_53 54
#define UTIL_INC_54 55
#define UTIL_INC_55 56
#define UTIL_INC_56 57
#define UTIL_INC_57 58
#define UTIL_INC_58 59
#define UTIL_INC_59 60
#define UTIL_INC_60 61
#define UTIL_INC_61 62
#define UTIL_INC_62 63
#define UTIL_INC_63 64
#define UTIL_INC_64 65
#define UTIL_INC_65 66
#define UTIL_INC_66 67
#define UTIL_INC_67 68
#define UTIL_INC_68 69
#define UTIL_INC_69 70
#define UTIL_INC_70 71
#define UTIL_INC_71 72
#define UTIL_INC_72 73
#define UTIL_INC_73 74
#define UTIL_INC_74 75
#define UTIL_INC_75 76
#define UTIL_INC_76 77
#define UTIL_INC_77 78
#define UTIL_INC_78 79
#define UTIL_INC_79 80
#define UTIL_INC_80 81
#define UTIL_INC_81 82
#define UTIL_INC_82 83
#define UTIL_INC_83 84
#define UTIL_INC_84 85
#define UTIL_INC_85 86
#define UTIL_INC_86 87
#define UTIL_INC_87 88
#define UTIL_INC_88 89
#define UTIL_INC_89 90
#define UTIL_INC_90 91
#define UTIL_INC_91 92
#define UTIL_INC_92 93
#define UTIL_INC_93 94
#define UTIL_INC_94 95
#define UTIL_INC_95 96
#define UTIL_INC_96 97
#define UTIL_INC_97 98
#define UTIL_INC_98 99
#define UTIL_INC_99 100
#define UTIL_INC_100 101
#define UTIL_INC_101 102
#define UTIL_INC_102 103
#define UTIL_INC_103 104
#define UTIL_INC_104 105
#define UTIL_INC_105 106
#define UTIL_INC_106 107
#define UTIL_INC_107 108
#define UTIL_INC_108 109
#define UTIL_INC_109 110
#define UTIL_INC_110 111
#define UTIL_INC_111 112
#define UTIL_INC_112 113
#define UTIL_INC_113 114
#define UTIL_INC_114 115
#define UTIL_INC_115 116
#define UTIL_INC_116 117
#define UTIL_INC_117 118
#define UTIL_INC_118 119
#define UTIL_INC_119 120
#define UTIL_INC_120 121
#define UTIL_INC_121 122
#define UTIL_INC_122 123
#define UTIL_INC_123 124
#define UTIL_INC_124 125
#define UTIL_INC_125 126
#define UTIL_INC_126 127
#define UTIL_INC_127 128
#define UTIL_INC_128 129
#define UTIL_INC_129 130
#define UTIL_INC_130 131
#define UTIL_INC_131 132
#define UTIL_INC_132 133
#define UTIL_INC_133 134
#define UTIL_INC_134 135
#define UTIL_INC_135 136
#define UTIL_INC_136 137
#define UTIL_INC_137 138
#define UTIL_INC_138 139
#define UTIL_INC_139 140
#define UTIL_INC_140 141
#define UTIL_INC_141 142
#define UTIL_INC_142 143
#define UTIL_INC_143 144
#define UTIL_INC_144 145
#define UTIL_INC_145 146
#define UTIL_INC_146 147
#define UTIL_INC_147 148
#define UTIL_INC_148 149
#define UTIL_INC_149 150
#define UTIL_INC_150 151
#define UTIL_INC_151 152
#define UTIL_INC_152 153
#define UTIL_INC_153 154
#define UTIL_INC_154 155
#define UTIL_INC_155 156
#define UTIL_INC_156 157
#define UTIL_INC_157 158
#define UTIL_INC_158 159
#define UTIL_INC_159 160
#define UTIL_INC_160 161
#define UTIL_INC_161 162
#define UTIL_INC_162 163
#define UTIL_INC_163 164
#define UTIL_INC_164 165
#define UTIL_INC_165 166
#define UTIL_INC_166 167
#define UTIL_INC_167 168
#define UTIL_INC_168 169
#define UTIL_INC_169 170
#define UTIL_INC_170 171
#define UTIL_INC_171 172
#define UTIL_INC_172 173
#define UTIL_INC_173 174
#define UTIL_INC_174 175
#define UTIL_INC_175 176
#define UTIL_INC_176 177
#define UTIL_INC_177 178
#define UTIL_INC_178 179
#define UTIL_INC_179 180
#define UTIL_INC_180 181
#define UTIL_INC_181 182
#define UTIL_INC_182 183
#define UTIL_INC_183 184
#define UTIL_INC_184 185
#define UTIL_INC_185 186
#define UTIL_INC_186 187
#define UTIL_INC_187 188
#define UTIL_INC_188 189
#define UTIL_INC_189 190
#define UTIL_INC_190 191
#define UTIL_INC_191 192
#define UTIL_INC_192 193
#define UTIL_INC_193 194
#define UTIL_INC_194 195
#define UTIL_INC_195 196
#define UTIL_INC_196 197
#define UTIL_INC_197 198
#define UTIL_INC_198 199
#define UTIL_INC_199 200
#define UTIL_INC_200 201
#define UTIL_INC_201 202
#define UTIL_INC_202 203
#define UTIL_INC_203 204
#define UTIL_INC_204 205
#define UTIL_INC_205 206
#define UTIL_INC_206 207
#define UTIL_INC_207 208
#define UTIL_INC_208 209
#define UTIL_INC_209 210
#define UTIL_INC_210 211
#define UTIL_INC_211 212
#define UTIL_INC_212 213
#define UTIL_INC_213 214
#define UTIL_INC_214 215
#define UTIL_INC_215 216
#define UTIL_INC_216 217
#define UTIL_INC_217 218
#define UTIL_INC_218 219
#define UTIL_INC_219 220
#define UTIL_INC_220 221
#define UTIL_INC_221 222
#define UTIL_INC_222 223
#define UTIL_INC_223 224
#define UTIL_INC_224 225
#define UTIL_INC_225 226
#define UTIL_INC_226 227
#define UTIL_INC_227 228
#define UTIL_INC_228 229
#define UTIL_INC_229 230
#define UTIL_INC_230 231
#define UTIL_INC_231 232
#define UTIL_INC_232 233
#define UTIL_INC_233 234
#define UTIL_INC_234 235
#define UTIL_INC_235 236
#define UTIL_INC_236 237
#define UTIL_INC_237 238
#define UTIL_INC_238 239
#define UTIL_INC_239 240
#define UTIL_INC_240 241
#define UTIL_INC_241 242
#define UTIL_INC_242 243
#define UTIL_INC_243 244
#define UTIL_INC_244 245
#define UTIL_INC_245 246
#define UTIL_INC_246 247
#define UTIL_INC_247 248
#define UTIL_INC_248 249
#define UTIL_INC_249 250
#define UTIL_INC_250 251
#define UTIL_INC_251 252
#define UTIL_INC_252 253
#define UTIL_INC_253 254
#define UTIL_INC_254 255
#define UTIL_INC_255 256

#define UTIL_DEC_0 0
#define UTIL_DEC_1 0
#define UTIL_DEC_2 1
#define UTIL_DEC_3 2
#define UTIL_DEC_4 3
#define UTIL_DEC_5 4
#define UTIL_DEC_6 5
#define UTIL_DEC_7 6
#define UTIL_DEC_8 7
#define UTIL_DEC_9 8
#define UTIL_DEC_10 9
#define UTIL_DEC_11 10
#define UTIL_DEC_12 11
#define UTIL_DEC_13 12
#define UTIL_DEC_14 13
#define UTIL_DEC_15 14
#define UTIL_DEC_16 15
#define UTIL_DEC_17 16
#define UTIL_DEC_18 17
#define UTIL_DEC_19 18
#define UTIL_DEC_20 19
#define UTIL_DEC_21 20
#define UTIL_DEC_22 21
#define UTIL_DEC_23 22
#define UTIL_DEC_24 23
#define UTIL_DEC_25 24
#define UTIL_DEC_26 25
#define UTIL_DEC_27 26
#define UTIL_DEC_28 27
#define UTIL_DEC_29 28
#define UTIL_DEC_30 29
#define UTIL_DEC_31 30
#define UTIL_DEC_32 31
#define UTIL_DEC_33 32
#define UTIL_DEC_34 33
#define UTIL_DEC_35 34
#define UTIL_DEC_36 35
#define UTIL_DEC_37 36
#define UTIL_DEC_38 37
#define UTIL_DEC_39 38
#define UTIL_DEC_40 39
#define UTIL_DEC_41 40
#define UTIL_DEC_42 41
#define UTIL_DEC_43 42
#define UTIL_DEC_44 43
#define UTIL_DEC_45 44
#define UTIL_DEC_46 45
#define UTIL_DEC_47 46
#define UTIL_DEC_48 47
#define UTIL_DEC_49 48
#define UTIL_DEC_50 49
#define UTIL_DEC_51 50
#define UTIL_DEC_52 51
#define UTIL_DEC_53 52
#define UTIL_DEC_54 53
#define UTIL_DEC_55 54
#define UTIL_DEC_56 55
#define UTIL_DEC_57 56
#define UTIL_DEC_58 57
#define UTIL_DEC_59 58
#define UTIL_DEC_60 59
#define UTIL_DEC_61 60
#define UTIL_DEC_62 61
#define UTIL_DEC_63 62
#define UTIL_DEC_64 63
#define UTIL_DEC_65 64
#define UTIL_DEC_66 65
#define UTIL_DEC_67 66
#define UTIL_DEC_68 67
#define UTIL_DEC_69 68
#define UTIL_DEC_70 69
#define UTIL_DEC_71 70
#define UTIL_DEC_72 71
#define UTIL_DEC_73 72
#define UTIL_DEC_74 73
#define UTIL_DEC_75 74
#define UTIL_DEC_76 75
#define UTIL_DEC_77 76
#define UTIL_DEC_78 77
#define UTIL_DEC_79 78
#define UTIL_DEC_80 79
#define UTIL_DEC_81 80
#define UTIL_DEC_82 81
#define UTIL_DEC_83 82
#define UTIL_DEC_84 83
#define UTIL_DEC_85 84
#define UTIL_DEC_86 85
#define UTIL_DEC_87 86
#define UTIL_DEC_88 87
#define UTIL_DEC_89 88
#define UTIL_DEC_90 89
#define UTIL_DEC_91 90
#define UTIL_DEC_92 91
#define UTIL_DEC_93 92
#define UTIL_DEC_94 93
#define UTIL_DEC_95 94
#define UTIL_DEC_96 95
#define UTIL_DEC_97 96
#define UTIL_DEC_98 97
#define UTIL_DEC_99 98
#define UTIL_DEC_100 99
#define UTIL_DEC_101 100
#define UTIL_DEC_102 101
#define UTIL_DEC_103 102
#define UTIL_DEC_104 103
#define UTIL_DEC_105 104
#define UTIL_DEC_106 105
#define UTIL_DEC_107 106
#define UTIL_DEC_108 107
#define UTIL_DEC_109 108
#define UTIL_DEC_110 109
#define UTIL_DEC_111 110
#define UTIL_DEC_112 111
#define UTIL_DEC_113 112
#define UTIL_DEC_114 113
#define UTIL_DEC_115 114
#define UTIL_DEC_116 115
#define UTIL_DEC_117 116
#define UTIL_DEC_118 117
#define UTIL_DEC_119 118
#define UTIL_DEC_120 119
#define UTIL_DEC_121 120
#define UTIL_DEC_122 121
#define UTIL_DEC_123 122
#define UTIL_DEC_124 123
#define UTIL_DEC_125 124
#define UTIL_DEC_126 125
#define UTIL_DEC_127 126
#define UTIL_DEC_128 127
#define UTIL_DEC_129 128
#define UTIL_DEC_130 129
#define UTIL_DEC_131 130
#define UTIL_DEC_132 131
#define UTIL_DEC_133 132
#define UTIL_DEC_134 133
#define UTIL_DEC_135 134
#define UTIL_DEC_136 135
#define UTIL_DEC_137 136
#define UTIL_DEC_138 137
#define UTIL_DEC_139 138
#define UTIL_DEC_140 139
#define UTIL_DEC_141 140
#define UTIL_DEC_142 141
#define UTIL_DEC_143 142
#define UTIL_DEC_144 143
#define UTIL_DEC_145 144
#define UTIL_DEC_146 145
#define UTIL_DEC_147 146
#define UTIL_DEC_148 147
#define UTIL_DEC_149 148
#define UTIL_DEC_150 149
#define UTIL_DEC_151 150
#define UTIL_DEC_152 151
#define UTIL_DEC_153 152
#define UTIL_DEC_154 153
#define UTIL_DEC_155 154
#define UTIL_DEC_156 155
#define UTIL_DEC_157 156
#define UTIL_DEC_158 157
#define UTIL_DEC_159 158
#define UTIL_DEC_160 159
#define UTIL_DEC_161 160
#define UTIL_DEC_162 161
#define UTIL_DEC_163 162
#define UTIL_DEC_164 163
#define UTIL_DEC_165 164
#define UTIL_DEC_166 165
#define UTIL_DEC_167 166
#define UTIL_DEC_168 167
#define UTIL_DEC_169 168
#define UTIL_DEC_170 169
#define UTIL_DEC_171 170
#define UTIL_DEC_172 171
#define UTIL_DEC_173 172
#define UTIL_DEC_174 173
#define UTIL_DEC_175 174
#define UTIL_DEC_176 175
#define UTIL_DEC_177 176
#define UTIL_DEC_178 177
#define UTIL_DEC_179 178
#define UTIL_DEC_180 179
#define UTIL_DEC_181 180
#define UTIL_DEC_182 181
#define UTIL_DEC_183 182
#define UTIL_DEC_184 183
#define UTIL_DEC_185 184
#define UTIL_DEC_186 185
#define UTIL_DEC_187 186
#define UTIL_DEC_188 187
#define UTIL_DEC_189 188
#define UTIL_DEC_190 189
#define UTIL_DEC_191 190
#define UTIL_DEC_192 191
#define UTIL_DEC_193 192
#define UTIL_DEC_194 193
#define UTIL_DEC_195 194
#define UTIL_DEC_196 195
#define UTIL_DEC_197 196
#define UTIL_DEC_198 197
#define UTIL_DEC_199 198
#define UTIL_DEC_200 199
#define UTIL_DEC_201 200
#define UTIL_DEC_202 201
#define UTIL_DEC_203 202
#define UTIL_DEC_204 203
#define UTIL_DEC_205 204
#define UTIL_DEC_206 205
#define UTIL_DEC_207 206
#define UTIL_DEC_208 207
#define UTIL_DEC_209 208
#define UTIL_DEC_210 209
#define UTIL_DEC_211 210
#define UTIL_DEC_212 211
#define UTIL_DEC_213 212
#define UTIL_DEC_214 213
#define UTIL_DEC_215 214
#define UTIL_DEC_216 215
#define UTIL_DEC_217 216
#define UTIL_DEC_218 217
#define UTIL_DEC_219 218
#define UTIL_DEC_220 219
#define UTIL_DEC_221 220
#define UTIL_DEC_222 221
#define UTIL_DEC_223 222
#define UTIL_DEC_224 223
#define UTIL_DEC_225 224
#define UTIL_DEC_226 225
#define UTIL_DEC_227 226
#define UTIL_DEC_228 227
#define UTIL_DEC_229 228
#define UTIL_DEC_230 229
#define UTIL_DEC_231 230
#define UTIL_DEC_232 231
#define UTIL_DEC_233 232
#define UTIL_DEC_234 233
#define UTIL_DEC_235 234
#define UTIL_DEC_236 235
#define UTIL_DEC_237 236
#define UTIL_DEC_238 237
#define UTIL_DEC_239 238
#define UTIL_DEC_240 239
#define UTIL_DEC_241 240
#define UTIL_DEC_242 241
#define UTIL_DEC_243 242
#define UTIL_DEC_244 243
#define UTIL_DEC_245 244
#define UTIL_DEC_246 245
#define UTIL_DEC_247 246
#define UTIL_DEC_248 247
#define UTIL_DEC_249 248
#define UTIL_DEC_250 249
#define UTIL_DEC_251 250
#define UTIL_DEC_252 251
#define UTIL_DEC_253 252
#define UTIL_DEC_254 253
#define UTIL_DEC_255 254

#define UTIL_X2_0 0
#define UTIL_X2_1 2
#define UTIL_X2_2 4
#define UTIL_X2_3 6
#define UTIL_X2_4 8
#define UTIL_X2_5 10
#define UTIL_X2_6 12
#define UTIL_X2_7 14
#define UTIL_X2_8 16
#define UTIL_X2_9 18
#define UTIL_X2_10 20
#define UTIL_X2_11 22
#define UTIL_X2_12 24
#define UTIL_X2_13 26
#define UTIL_X2_14 28
#define UTIL_X2_15 30
#define UTIL_X2_16 32
#define UTIL_X2_17 34
#define UTIL_X2_18 36
#define UTIL_X2_19 38
#define UTIL_X2_20 40
#define UTIL_X2_21 42
#define UTIL_X2_22 44
#define UTIL_X2_23 46
#define UTIL_X2_24 48
#define UTIL_X2_25 50
#define UTIL_X2_26 52
#define UTIL_X2_27 54
#define UTIL_X2_28 56
#define UTIL_X2_29 58
#define UTIL_X2_30 60
#define UTIL_X2_31 62
#define UTIL_X2_32 64
#define UTIL_X2_33 66
#define UTIL_X2_34 68
#define UTIL_X2_35 70
#define UTIL_X2_36 72
#define UTIL_X2_37 74
#define UTIL_X2_38 76
#define UTIL_X2_39 78
#define UTIL_X2_40 80
#define UTIL_X2_41 82
#define UTIL_X2_42 84
#define UTIL_X2_43 86
#define UTIL_X2_44 88
#define UTIL_X2_45 90
#define UTIL_X2_46 92
#define UTIL_X2_47 94
#define UTIL_X2_48 96
#define UTIL_X2_49 98
#define UTIL_X2_50 100
#define UTIL_X2_51 102
#define UTIL_X2_52 104
#define UTIL_X2_53 106
#define UTIL_X2_54 108
#define UTIL_X2_55 110
#define UTIL_X2_56 112
#define UTIL_X2_57 114
#define UTIL_X2_58 116
#define UTIL_X2_59 118
#define UTIL_X2_60 120
#define UTIL_X2_61 122
#define UTIL_X2_62 124
#define UTIL_X2_63 126
#define UTIL_X2_64 128
#define UTIL_X2_65 130
#define UTIL_X2_66 132
#define UTIL_X2_67 134
#define UTIL_X2_68 136
#define UTIL_X2_69 138
#define UTIL_X2_70 140
#define UTIL_X2_71 142
#define UTIL_X2_72 144
#define UTIL_X2_73 146
#define UTIL_X2_74 148
#define UTIL_X2_75 150
#define UTIL_X2_76 152
#define UTIL_X2_77 154
#define UTIL_X2_78 156
#define UTIL_X2_79 158
#define UTIL_X2_80 160
#define UTIL_X2_81 162
#define UTIL_X2_82 164
#define UTIL_X2_83 166
#define UTIL_X2_84 168
#define UTIL_X2_85 170
#define UTIL_X2_86 172
#define UTIL_X2_87 174
#define UTIL_X2_88 176
#define UTIL_X2_89 178
#define UTIL_X2_90 180
#define UTIL_X2_91 182
#define UTIL_X2_92 184
#define UTIL_X2_93 186
#define UTIL_X2_94 188
#define UTIL_X2_95 190
#define UTIL_X2_96 192
#define UTIL_X2_97 194
#define UTIL_X2_98 196
#define UTIL_X2_99 198
#define UTIL_X2_100 200
#define UTIL_X2_101 202
#define UTIL_X2_102 204
#define UTIL_X2_103 206
#define UTIL_X2_104 208
#define UTIL_X2_105 210
#define UTIL_X2_106 212
#define UTIL_X2_107 214
#define UTIL_X2_108 216
#define UTIL_X2_109 218
#define UTIL_X2_110 220
#define UTIL_X2_111 222
#define UTIL_X2_112 224
#define UTIL_X2_113 226
#define UTIL_X2_114 228
#define UTIL_X2_115 230
#define UTIL_X2_116 232
#define UTIL_X2_117 234
#define UTIL_X2_118 236
#define UTIL_X2_119 238
#define UTIL_X2_120 240
#define UTIL_X2_121 242
#define UTIL_X2_122 244
#define UTIL_X2_123 246
#define UTIL_X2_124 248
#define UTIL_X2_125 250
#define UTIL_X2_126 252
#define UTIL_X2_127 254
#define UTIL_X2_128 256
#define UTIL_X2_129 258
#define UTIL_X2_130 260
#define UTIL_X2_131 262
#define UTIL_X2_132 264
#define UTIL_X2_133 266
#define UTIL_X2_134 268
#define UTIL_X2_135 270
#define UTIL_X2_136 272
#define UTIL_X2_137 274
#define UTIL_X2_138 276
#define UTIL_X2_139 278
#define UTIL_X2_140 280
#define UTIL_X2_141 282
#define UTIL_X2_142 284
#define UTIL_X2_143 286
#define UTIL_X2_144 288
#define UTIL_X2_145 290
#define UTIL_X2_146 292
#define UTIL_X2_147 294
#define UTIL_X2_148 296
#define UTIL_X2_149 298
#define UTIL_X2_150 300
#define UTIL_X2_151 302
#define UTIL_X2_152 304
#define UTIL_X2_153 306
#define UTIL_X2_154 308
#define UTIL_X2_155 310
#define UTIL_X2_156 312
#define UTIL_X2_157 314
#define UTIL_X2_158 316
#define UTIL_X2_159 318
#define UTIL_X2_160 320
#define UTIL_X2_161 322
#define UTIL_X2_162 324
#define UTIL_X2_163 326
#define UTIL_X2_164 328
#define UTIL_X2_165 330
#define UTIL_X2_166 332
#define UTIL_X2_167 334
#define UTIL_X2_168 336
#define UTIL_X2_169 338
#define UTIL_X2_170 340
#define UTIL_X2_171 342
#define UTIL_X2_172 344
#define UTIL_X2_173 346
#define UTIL_X2_174 348
#define UTIL_X2_175 350
#define UTIL_X2_176 352
#define UTIL_X2_177 354
#define UTIL_X2_178 356
#define UTIL_X2_179 358
#define UTIL_X2_180 360
#define UTIL_X2_181 362
#define UTIL_X2_182 364
#define UTIL_X2_183 366
#define UTIL_X2_184 368
#define UTIL_X2_185 370
#define UTIL_X2_186 372
#define UTIL_X2_187 374
#define UTIL_X2_188 376
#define UTIL_X2_189 378
#define UTIL_X2_190 380
#define UTIL_X2_191 382
#define UTIL_X2_192 384
#define UTIL_X2_193 386
#define UTIL_X2_194 388
#define UTIL_X2_195 390
#define UTIL_X2_196 392
#define UTIL_X2_197 394
#define UTIL_X2_198 396
#define UTIL_X2_199 398
#define UTIL_X2_200 400
#define UTIL_X2_201 402
#define UTIL_X2_202 404
#define UTIL_X2_203 406
#define UTIL_X2_204 408
#define UTIL_X2_205 410
#define UTIL_X2_206 412
#define UTIL_X2_207 414
#define UTIL_X2_208 416
#define UTIL_X2_209 418
#define UTIL_X2_210 420
#define UTIL_X2_211 422
#define UTIL_X2_212 424
#define UTIL_X2_213 426
#define UTIL_X2_214 428
#define UTIL_X2_215 430
#define UTIL_X2_216 432
#define UTIL_X2_217 434
#define UTIL_X2_218 436
#define UTIL_X2_219 438
#define UTIL_X2_220 440
#define UTIL_X2_221 442
#define UTIL_X2_222 444
#define UTIL_X2_223 446
#define UTIL_X2_224 448
#define UTIL_X2_225 450
#define UTIL_X2_226 452
#define UTIL_X2_227 454
#define UTIL_X2_228 456
#define UTIL_X2_229 458
#define UTIL_X2_230 460
#define UTIL_X2_231 462
#define UTIL_X2_232 464
#define UTIL_X2_233 466
#define UTIL_X2_234 468
#define UTIL_X2_235 470
#define UTIL_X2_236 472
#define UTIL_X2_237 474
#define UTIL_X2_238 476
#define UTIL_X2_239 478
#define UTIL_X2_240 480
#define UTIL_X2_241 482
#define UTIL_X2_242 484
#define UTIL_X2_243 486
#define UTIL_X2_244 488
#define UTIL_X2_245 490
#define UTIL_X2_246 492
#define UTIL_X2_247 494
#define UTIL_X2_248 496
#define UTIL_X2_249 498
#define UTIL_X2_250 500
#define UTIL_X2_251 502
#define UTIL_X2_252 504
#define UTIL_X2_253 506
#define UTIL_X2_254 508
#define UTIL_X2_255 510

#endif /* ZEPHYR_INCLUDE_SYS_UTIL_INTERNAL_H_ */
