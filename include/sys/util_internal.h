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
#define Z_IS_ENABLED2(one_or_two_args) Z_IS_ENABLED3(one_or_two_args true, false)

/* And our second argument is thus now cooked to be 1 in the case
 * where the value is defined to 1, and 0 if not:
 */
#define Z_IS_ENABLED3(ignore_this, val, ...) val

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
#define Z_IS_EMPTY_(...) Z_IS_EMPTY__(__VA_ARGS__)
#define Z_IS_EMPTY__(a, ...) Z_IS_EMPTY___(_ZZ##a##ZZ0, __VA_ARGS__)
#define Z_IS_EMPTY___(...) GET_ARG_N(3, __VA_ARGS__)

/* Used by LIST_DROP_EMPTY() */
/* Adding ',' after each element would add empty element at the end of
 * list, which is hard to remove, so instead precede each element with ',',
 * this way first element is empty, and this one is easy to drop.
 */
#define Z_LIST_ADD_ELEM(e) EMPTY, e
#define Z_LIST_DROP_FIRST(...) GET_ARGS_LESS_N(1, __VA_ARGS__)
#define Z_LIST_NO_EMPTIES(e) \
	COND_CODE_1(IS_EMPTY(e), (), (Z_LIST_ADD_ELEM(e)))

/*
 * Macros for doing code-generation with the preprocessor as if we
 * could do recursive macro expansion.
 *
 * Generally it is better to generate code with the preprocessor than
 * to copy-paste code or to generate code with the build system /
 * python script's etc.
 *
 * http://stackoverflow.com/a/12540675
 */

#define UTIL_EMPTY(...)
#define UTIL_DEFER(...) __VA_ARGS__ UTIL_EMPTY()
#define UTIL_OBSTRUCT(...) __VA_ARGS__ UTIL_DEFER(UTIL_EMPTY)()
#define UTIL_EXPAND(...) __VA_ARGS__

#define UTIL_EVAL(...)  UTIL_EVAL1(UTIL_EVAL1(UTIL_EVAL1(__VA_ARGS__)))
#define UTIL_EVAL1(...) UTIL_EVAL2(UTIL_EVAL2(UTIL_EVAL2(__VA_ARGS__)))
#define UTIL_EVAL2(...) UTIL_EVAL3(UTIL_EVAL3(UTIL_EVAL3(__VA_ARGS__)))
#define UTIL_EVAL3(...) UTIL_EVAL4(UTIL_EVAL4(UTIL_EVAL4(__VA_ARGS__)))
#define UTIL_EVAL4(...) UTIL_EVAL5(UTIL_EVAL5(UTIL_EVAL5(__VA_ARGS__)))
#define UTIL_EVAL5(...) __VA_ARGS__

#define UTIL_CAT(a, ...) UTIL_PRIMITIVE_CAT(a, __VA_ARGS__)
#define UTIL_PRIMITIVE_CAT(a, ...) a##__VA_ARGS__

/*
 * UTIL_INC(x) for an integer literal x from 0 to 255 expands to an
 * integer literal whose value is x+1.
 *
 * Similarly, UTIL_DEC(x) is (x-1) as an integer literal.
 */
#define UTIL_INC(x) UTIL_PRIMITIVE_CAT(UTIL_INC_, x)
#define UTIL_DEC(x) UTIL_PRIMITIVE_CAT(UTIL_DEC_, x)

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
#define UTIL_INC_256 257

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
#define UTIL_DEC_256 255

#define UTIL_CHECK_N(x, n, ...) n
#define UTIL_CHECK(...) UTIL_CHECK_N(__VA_ARGS__, 0,)
#define UTIL_NOT(x) UTIL_CHECK(UTIL_PRIMITIVE_CAT(UTIL_NOT_, x))
#define UTIL_NOT_0 ~, 1,
#define UTIL_COMPL(b) UTIL_PRIMITIVE_CAT(UTIL_COMPL_, b)
#define UTIL_COMPL_0 1
#define UTIL_COMPL_1 0
#define UTIL_BOOL(x) UTIL_COMPL(UTIL_NOT(x))
#define UTIL_IIF(c) UTIL_PRIMITIVE_CAT(UTIL_IIF_, c)
#define UTIL_IIF_0(t, ...) __VA_ARGS__
#define UTIL_IIF_1(t, ...) t
#define UTIL_IF(c) UTIL_IIF(UTIL_BOOL(c))

#define UTIL_EAT(...)
#define UTIL_WHEN(c) UTIL_IF(c)(UTIL_EXPAND, UTIL_EAT)

#define UTIL_REPEAT(count, macro, ...)			    \
	UTIL_WHEN(count)				    \
	(						    \
		UTIL_OBSTRUCT(UTIL_REPEAT_INDIRECT) ()	    \
		(					    \
			UTIL_DEC(count), macro, __VA_ARGS__ \
		)					    \
		UTIL_OBSTRUCT(macro)			    \
		(					    \
			UTIL_DEC(count), __VA_ARGS__	    \
		)					    \
	)
#define UTIL_REPEAT_INDIRECT() UTIL_REPEAT

/* Internal macros used by FOR_EACH, FOR_EACH_IDX, etc. */

#define Z_FOR_EACH_IDX(count, n, macro, sep, fixed_arg0, fixed_arg1, ...)\
	UTIL_WHEN(count)						\
	(								\
		UTIL_OBSTRUCT(macro)					\
		(							\
			fixed_arg0, fixed_arg1, n, Z_GET_ARG1(__VA_ARGS__)\
		) COND_CODE_1(count, (), (__DEBRACKET sep))		\
		UTIL_OBSTRUCT(Z_FOR_EACH_IDX_INDIRECT) ()		\
		(							\
			UTIL_DEC(count), UTIL_INC(n), macro, sep,	\
			fixed_arg0, fixed_arg1,				\
			Z_GET_ARGS_LESS_1(__VA_ARGS__)			\
		)							\
	)

#define Z_GET_ARG1(arg1, ...) arg1
#define Z_GET_ARGS_LESS_1(arg1, ...) __VA_ARGS__

#define Z_FOR_EACH_IDX_INDIRECT() Z_FOR_EACH_IDX

#define Z_FOR_EACH_IDX2(count, iter, macro, sc, fixed_arg0, fixed_arg1, ...) \
	UTIL_EVAL(Z_FOR_EACH_IDX(count, iter, macro, sc,\
				 fixed_arg0, fixed_arg1, __VA_ARGS__))

#define Z_FOR_EACH_SWALLOW_NOTHING(F, fixed_arg, index, arg) \
	F(index, arg, fixed_arg)

#define Z_FOR_EACH_SWALLOW_FIXED_ARG(F, fixed_arg, index, arg) F(index, arg)

#define Z_FOR_EACH_SWALLOW_INDEX_FIXED_ARG(F, fixed_arg, index, arg) F(arg)
#define Z_FOR_EACH_SWALLOW_INDEX(F, fixed_arg, index, arg) F(arg, fixed_arg)

/* This is a workaround to enable mixing GET_ARG_N with FOR_EACH macros. If
 * same UTIL_EVAL macro is used then macro is incorrectly resolved.
 */
#define Z_GET_ARG_N_EVAL(...) \
	Z_GET_ARG_N_EVAL1(Z_GET_ARG_N_EVAL1(Z_GET_ARG_N_EVAL1(__VA_ARGS__)))
#define Z_GET_ARG_N_EVAL1(...) \
	Z_GET_ARG_N_EVAL2(Z_GET_ARG_N_EVAL2(Z_GET_ARG_N_EVAL2(__VA_ARGS__)))
#define Z_GET_ARG_N_EVAL2(...) \
	Z_GET_ARG_N_EVAL3(Z_GET_ARG_N_EVAL3(Z_GET_ARG_N_EVAL3(__VA_ARGS__)))
#define Z_GET_ARG_N_EVAL3(...) \
	Z_GET_ARG_N_EVAL4(Z_GET_ARG_N_EVAL4(Z_GET_ARG_N_EVAL4(__VA_ARGS__)))
#define Z_GET_ARG_N_EVAL4(...) \
	Z_GET_ARG_N_EVAL5(Z_GET_ARG_N_EVAL5(Z_GET_ARG_N_EVAL5(__VA_ARGS__)))
#define Z_GET_ARG_N_EVAL5(...) __VA_ARGS__

/* Set of internal macros used for GET_ARG_N of macros. */
#define Z_GET_ARG_N(count, single_arg, ...)				\
	UTIL_WHEN(count)						\
	(								\
		IF_ENABLED(count, (UTIL_OBSTRUCT(__DEBRACKET)		\
		(							\
			COND_CODE_1(single_arg,				\
				(Z_GET_ARG1(__VA_ARGS__)), (__VA_ARGS__))\
		)))							\
		UTIL_OBSTRUCT(Z_GET_ARG_N_INDIRECT) ()			\
		(							\
			UTIL_DEC(count), single_arg,			\
			Z_GET_ARGS_LESS_1(__VA_ARGS__)			\
		)							\
	)

#define Z_GET_ARG_N_INDIRECT() Z_GET_ARG_N

#define _Z_GET_ARG_N(N, single_arg, ...) \
	Z_GET_ARG_N_EVAL(Z_GET_ARG_N(N, single_arg, __VA_ARGS__))

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

#endif /* ZEPHYR_INCLUDE_SYS_UTIL_INTERNAL_H_ */
