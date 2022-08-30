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

#endif /* ZEPHYR_INCLUDE_SYS_UTIL_INTERNAL_H_ */
