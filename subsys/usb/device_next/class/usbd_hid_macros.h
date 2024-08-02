/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The macros in this file are not for public use, but only for HID driver
 * instantiation.
 */

#include <zephyr/sys/util_macro.h>
#include <zephyr/usb/usb_ch9.h>

#ifndef ZEPHYR_USB_DEVICE_CLASS_HID_MACROS_H_
#define ZEPHYR_USB_DEVICE_CLASS_HID_MACROS_H_

/*
 * This long list of definitions is used in HID_MPS_LESS_65 macro to determine
 * whether an endpoint MPS is equal to or less than 64 bytes.
 */
#define HID_MPS_LESS_65_0 1
#define HID_MPS_LESS_65_1 1
#define HID_MPS_LESS_65_2 1
#define HID_MPS_LESS_65_3 1
#define HID_MPS_LESS_65_4 1
#define HID_MPS_LESS_65_5 1
#define HID_MPS_LESS_65_6 1
#define HID_MPS_LESS_65_7 1
#define HID_MPS_LESS_65_8 1
#define HID_MPS_LESS_65_9 1
#define HID_MPS_LESS_65_10 1
#define HID_MPS_LESS_65_11 1
#define HID_MPS_LESS_65_12 1
#define HID_MPS_LESS_65_13 1
#define HID_MPS_LESS_65_14 1
#define HID_MPS_LESS_65_15 1
#define HID_MPS_LESS_65_16 1
#define HID_MPS_LESS_65_17 1
#define HID_MPS_LESS_65_18 1
#define HID_MPS_LESS_65_19 1
#define HID_MPS_LESS_65_20 1
#define HID_MPS_LESS_65_21 1
#define HID_MPS_LESS_65_22 1
#define HID_MPS_LESS_65_23 1
#define HID_MPS_LESS_65_24 1
#define HID_MPS_LESS_65_25 1
#define HID_MPS_LESS_65_26 1
#define HID_MPS_LESS_65_27 1
#define HID_MPS_LESS_65_28 1
#define HID_MPS_LESS_65_29 1
#define HID_MPS_LESS_65_30 1
#define HID_MPS_LESS_65_31 1
#define HID_MPS_LESS_65_32 1
#define HID_MPS_LESS_65_33 1
#define HID_MPS_LESS_65_34 1
#define HID_MPS_LESS_65_35 1
#define HID_MPS_LESS_65_36 1
#define HID_MPS_LESS_65_37 1
#define HID_MPS_LESS_65_38 1
#define HID_MPS_LESS_65_39 1
#define HID_MPS_LESS_65_40 1
#define HID_MPS_LESS_65_41 1
#define HID_MPS_LESS_65_42 1
#define HID_MPS_LESS_65_43 1
#define HID_MPS_LESS_65_44 1
#define HID_MPS_LESS_65_45 1
#define HID_MPS_LESS_65_46 1
#define HID_MPS_LESS_65_47 1
#define HID_MPS_LESS_65_48 1
#define HID_MPS_LESS_65_49 1
#define HID_MPS_LESS_65_50 1
#define HID_MPS_LESS_65_51 1
#define HID_MPS_LESS_65_52 1
#define HID_MPS_LESS_65_53 1
#define HID_MPS_LESS_65_54 1
#define HID_MPS_LESS_65_55 1
#define HID_MPS_LESS_65_56 1
#define HID_MPS_LESS_65_57 1
#define HID_MPS_LESS_65_58 1
#define HID_MPS_LESS_65_59 1
#define HID_MPS_LESS_65_60 1
#define HID_MPS_LESS_65_61 1
#define HID_MPS_LESS_65_62 1
#define HID_MPS_LESS_65_63 1
#define HID_MPS_LESS_65_64 1
#define HID_MPS_LESS_65_65 0
#define HID_MPS_LESS_65_66 0
#define HID_MPS_LESS_65_67 0
#define HID_MPS_LESS_65_68 0
#define HID_MPS_LESS_65_69 0
#define HID_MPS_LESS_65_70 0
#define HID_MPS_LESS_65_71 0
#define HID_MPS_LESS_65_72 0
#define HID_MPS_LESS_65_73 0
#define HID_MPS_LESS_65_74 0
#define HID_MPS_LESS_65_75 0
#define HID_MPS_LESS_65_76 0
#define HID_MPS_LESS_65_77 0
#define HID_MPS_LESS_65_78 0
#define HID_MPS_LESS_65_79 0
#define HID_MPS_LESS_65_80 0
#define HID_MPS_LESS_65_81 0
#define HID_MPS_LESS_65_82 0
#define HID_MPS_LESS_65_83 0
#define HID_MPS_LESS_65_84 0
#define HID_MPS_LESS_65_85 0
#define HID_MPS_LESS_65_86 0
#define HID_MPS_LESS_65_87 0
#define HID_MPS_LESS_65_88 0
#define HID_MPS_LESS_65_89 0
#define HID_MPS_LESS_65_90 0
#define HID_MPS_LESS_65_91 0
#define HID_MPS_LESS_65_92 0
#define HID_MPS_LESS_65_93 0
#define HID_MPS_LESS_65_94 0
#define HID_MPS_LESS_65_95 0
#define HID_MPS_LESS_65_96 0
#define HID_MPS_LESS_65_97 0
#define HID_MPS_LESS_65_98 0
#define HID_MPS_LESS_65_99 0
#define HID_MPS_LESS_65_100 0
#define HID_MPS_LESS_65_101 0
#define HID_MPS_LESS_65_102 0
#define HID_MPS_LESS_65_103 0
#define HID_MPS_LESS_65_104 0
#define HID_MPS_LESS_65_105 0
#define HID_MPS_LESS_65_106 0
#define HID_MPS_LESS_65_107 0
#define HID_MPS_LESS_65_108 0
#define HID_MPS_LESS_65_109 0
#define HID_MPS_LESS_65_110 0
#define HID_MPS_LESS_65_111 0
#define HID_MPS_LESS_65_112 0
#define HID_MPS_LESS_65_113 0
#define HID_MPS_LESS_65_114 0
#define HID_MPS_LESS_65_115 0
#define HID_MPS_LESS_65_116 0
#define HID_MPS_LESS_65_117 0
#define HID_MPS_LESS_65_118 0
#define HID_MPS_LESS_65_119 0
#define HID_MPS_LESS_65_120 0
#define HID_MPS_LESS_65_121 0
#define HID_MPS_LESS_65_122 0
#define HID_MPS_LESS_65_123 0
#define HID_MPS_LESS_65_124 0
#define HID_MPS_LESS_65_125 0
#define HID_MPS_LESS_65_126 0
#define HID_MPS_LESS_65_127 0
#define HID_MPS_LESS_65_128 0
#define HID_MPS_LESS_65_129 0
#define HID_MPS_LESS_65_130 0
#define HID_MPS_LESS_65_131 0
#define HID_MPS_LESS_65_132 0
#define HID_MPS_LESS_65_133 0
#define HID_MPS_LESS_65_134 0
#define HID_MPS_LESS_65_135 0
#define HID_MPS_LESS_65_136 0
#define HID_MPS_LESS_65_137 0
#define HID_MPS_LESS_65_138 0
#define HID_MPS_LESS_65_139 0
#define HID_MPS_LESS_65_140 0
#define HID_MPS_LESS_65_141 0
#define HID_MPS_LESS_65_142 0
#define HID_MPS_LESS_65_143 0
#define HID_MPS_LESS_65_144 0
#define HID_MPS_LESS_65_145 0
#define HID_MPS_LESS_65_146 0
#define HID_MPS_LESS_65_147 0
#define HID_MPS_LESS_65_148 0
#define HID_MPS_LESS_65_149 0
#define HID_MPS_LESS_65_150 0
#define HID_MPS_LESS_65_151 0
#define HID_MPS_LESS_65_152 0
#define HID_MPS_LESS_65_153 0
#define HID_MPS_LESS_65_154 0
#define HID_MPS_LESS_65_155 0
#define HID_MPS_LESS_65_156 0
#define HID_MPS_LESS_65_157 0
#define HID_MPS_LESS_65_158 0
#define HID_MPS_LESS_65_159 0
#define HID_MPS_LESS_65_160 0
#define HID_MPS_LESS_65_161 0
#define HID_MPS_LESS_65_162 0
#define HID_MPS_LESS_65_163 0
#define HID_MPS_LESS_65_164 0
#define HID_MPS_LESS_65_165 0
#define HID_MPS_LESS_65_166 0
#define HID_MPS_LESS_65_167 0
#define HID_MPS_LESS_65_168 0
#define HID_MPS_LESS_65_169 0
#define HID_MPS_LESS_65_170 0
#define HID_MPS_LESS_65_171 0
#define HID_MPS_LESS_65_172 0
#define HID_MPS_LESS_65_173 0
#define HID_MPS_LESS_65_174 0
#define HID_MPS_LESS_65_175 0
#define HID_MPS_LESS_65_176 0
#define HID_MPS_LESS_65_177 0
#define HID_MPS_LESS_65_178 0
#define HID_MPS_LESS_65_179 0
#define HID_MPS_LESS_65_180 0
#define HID_MPS_LESS_65_181 0
#define HID_MPS_LESS_65_182 0
#define HID_MPS_LESS_65_183 0
#define HID_MPS_LESS_65_184 0
#define HID_MPS_LESS_65_185 0
#define HID_MPS_LESS_65_186 0
#define HID_MPS_LESS_65_187 0
#define HID_MPS_LESS_65_188 0
#define HID_MPS_LESS_65_189 0
#define HID_MPS_LESS_65_190 0
#define HID_MPS_LESS_65_191 0
#define HID_MPS_LESS_65_192 0
#define HID_MPS_LESS_65_193 0
#define HID_MPS_LESS_65_194 0
#define HID_MPS_LESS_65_195 0
#define HID_MPS_LESS_65_196 0
#define HID_MPS_LESS_65_197 0
#define HID_MPS_LESS_65_198 0
#define HID_MPS_LESS_65_199 0
#define HID_MPS_LESS_65_200 0
#define HID_MPS_LESS_65_201 0
#define HID_MPS_LESS_65_202 0
#define HID_MPS_LESS_65_203 0
#define HID_MPS_LESS_65_204 0
#define HID_MPS_LESS_65_205 0
#define HID_MPS_LESS_65_206 0
#define HID_MPS_LESS_65_207 0
#define HID_MPS_LESS_65_208 0
#define HID_MPS_LESS_65_209 0
#define HID_MPS_LESS_65_210 0
#define HID_MPS_LESS_65_211 0
#define HID_MPS_LESS_65_212 0
#define HID_MPS_LESS_65_213 0
#define HID_MPS_LESS_65_214 0
#define HID_MPS_LESS_65_215 0
#define HID_MPS_LESS_65_216 0
#define HID_MPS_LESS_65_217 0
#define HID_MPS_LESS_65_218 0
#define HID_MPS_LESS_65_219 0
#define HID_MPS_LESS_65_220 0
#define HID_MPS_LESS_65_221 0
#define HID_MPS_LESS_65_222 0
#define HID_MPS_LESS_65_223 0
#define HID_MPS_LESS_65_224 0
#define HID_MPS_LESS_65_225 0
#define HID_MPS_LESS_65_226 0
#define HID_MPS_LESS_65_227 0
#define HID_MPS_LESS_65_228 0
#define HID_MPS_LESS_65_229 0
#define HID_MPS_LESS_65_230 0
#define HID_MPS_LESS_65_231 0
#define HID_MPS_LESS_65_232 0
#define HID_MPS_LESS_65_233 0
#define HID_MPS_LESS_65_234 0
#define HID_MPS_LESS_65_235 0
#define HID_MPS_LESS_65_236 0
#define HID_MPS_LESS_65_237 0
#define HID_MPS_LESS_65_238 0
#define HID_MPS_LESS_65_239 0
#define HID_MPS_LESS_65_240 0
#define HID_MPS_LESS_65_241 0
#define HID_MPS_LESS_65_242 0
#define HID_MPS_LESS_65_243 0
#define HID_MPS_LESS_65_244 0
#define HID_MPS_LESS_65_245 0
#define HID_MPS_LESS_65_246 0
#define HID_MPS_LESS_65_247 0
#define HID_MPS_LESS_65_248 0
#define HID_MPS_LESS_65_249 0
#define HID_MPS_LESS_65_250 0
#define HID_MPS_LESS_65_251 0
#define HID_MPS_LESS_65_252 0
#define HID_MPS_LESS_65_253 0
#define HID_MPS_LESS_65_254 0
#define HID_MPS_LESS_65_255 0
#define HID_MPS_LESS_65_256 0
#define HID_MPS_LESS_65_257 0
#define HID_MPS_LESS_65_258 0
#define HID_MPS_LESS_65_259 0
#define HID_MPS_LESS_65_260 0
#define HID_MPS_LESS_65_261 0
#define HID_MPS_LESS_65_262 0
#define HID_MPS_LESS_65_263 0
#define HID_MPS_LESS_65_264 0
#define HID_MPS_LESS_65_265 0
#define HID_MPS_LESS_65_266 0
#define HID_MPS_LESS_65_267 0
#define HID_MPS_LESS_65_268 0
#define HID_MPS_LESS_65_269 0
#define HID_MPS_LESS_65_270 0
#define HID_MPS_LESS_65_271 0
#define HID_MPS_LESS_65_272 0
#define HID_MPS_LESS_65_273 0
#define HID_MPS_LESS_65_274 0
#define HID_MPS_LESS_65_275 0
#define HID_MPS_LESS_65_276 0
#define HID_MPS_LESS_65_277 0
#define HID_MPS_LESS_65_278 0
#define HID_MPS_LESS_65_279 0
#define HID_MPS_LESS_65_280 0
#define HID_MPS_LESS_65_281 0
#define HID_MPS_LESS_65_282 0
#define HID_MPS_LESS_65_283 0
#define HID_MPS_LESS_65_284 0
#define HID_MPS_LESS_65_285 0
#define HID_MPS_LESS_65_286 0
#define HID_MPS_LESS_65_287 0
#define HID_MPS_LESS_65_288 0
#define HID_MPS_LESS_65_289 0
#define HID_MPS_LESS_65_290 0
#define HID_MPS_LESS_65_291 0
#define HID_MPS_LESS_65_292 0
#define HID_MPS_LESS_65_293 0
#define HID_MPS_LESS_65_294 0
#define HID_MPS_LESS_65_295 0
#define HID_MPS_LESS_65_296 0
#define HID_MPS_LESS_65_297 0
#define HID_MPS_LESS_65_298 0
#define HID_MPS_LESS_65_299 0
#define HID_MPS_LESS_65_300 0
#define HID_MPS_LESS_65_301 0
#define HID_MPS_LESS_65_302 0
#define HID_MPS_LESS_65_303 0
#define HID_MPS_LESS_65_304 0
#define HID_MPS_LESS_65_305 0
#define HID_MPS_LESS_65_306 0
#define HID_MPS_LESS_65_307 0
#define HID_MPS_LESS_65_308 0
#define HID_MPS_LESS_65_309 0
#define HID_MPS_LESS_65_310 0
#define HID_MPS_LESS_65_311 0
#define HID_MPS_LESS_65_312 0
#define HID_MPS_LESS_65_313 0
#define HID_MPS_LESS_65_314 0
#define HID_MPS_LESS_65_315 0
#define HID_MPS_LESS_65_316 0
#define HID_MPS_LESS_65_317 0
#define HID_MPS_LESS_65_318 0
#define HID_MPS_LESS_65_319 0
#define HID_MPS_LESS_65_320 0
#define HID_MPS_LESS_65_321 0
#define HID_MPS_LESS_65_322 0
#define HID_MPS_LESS_65_323 0
#define HID_MPS_LESS_65_324 0
#define HID_MPS_LESS_65_325 0
#define HID_MPS_LESS_65_326 0
#define HID_MPS_LESS_65_327 0
#define HID_MPS_LESS_65_328 0
#define HID_MPS_LESS_65_329 0
#define HID_MPS_LESS_65_330 0
#define HID_MPS_LESS_65_331 0
#define HID_MPS_LESS_65_332 0
#define HID_MPS_LESS_65_333 0
#define HID_MPS_LESS_65_334 0
#define HID_MPS_LESS_65_335 0
#define HID_MPS_LESS_65_336 0
#define HID_MPS_LESS_65_337 0
#define HID_MPS_LESS_65_338 0
#define HID_MPS_LESS_65_339 0
#define HID_MPS_LESS_65_340 0
#define HID_MPS_LESS_65_341 0
#define HID_MPS_LESS_65_342 0
#define HID_MPS_LESS_65_343 0
#define HID_MPS_LESS_65_344 0
#define HID_MPS_LESS_65_345 0
#define HID_MPS_LESS_65_346 0
#define HID_MPS_LESS_65_347 0
#define HID_MPS_LESS_65_348 0
#define HID_MPS_LESS_65_349 0
#define HID_MPS_LESS_65_350 0
#define HID_MPS_LESS_65_351 0
#define HID_MPS_LESS_65_352 0
#define HID_MPS_LESS_65_353 0
#define HID_MPS_LESS_65_354 0
#define HID_MPS_LESS_65_355 0
#define HID_MPS_LESS_65_356 0
#define HID_MPS_LESS_65_357 0
#define HID_MPS_LESS_65_358 0
#define HID_MPS_LESS_65_359 0
#define HID_MPS_LESS_65_360 0
#define HID_MPS_LESS_65_361 0
#define HID_MPS_LESS_65_362 0
#define HID_MPS_LESS_65_363 0
#define HID_MPS_LESS_65_364 0
#define HID_MPS_LESS_65_365 0
#define HID_MPS_LESS_65_366 0
#define HID_MPS_LESS_65_367 0
#define HID_MPS_LESS_65_368 0
#define HID_MPS_LESS_65_369 0
#define HID_MPS_LESS_65_370 0
#define HID_MPS_LESS_65_371 0
#define HID_MPS_LESS_65_372 0
#define HID_MPS_LESS_65_373 0
#define HID_MPS_LESS_65_374 0
#define HID_MPS_LESS_65_375 0
#define HID_MPS_LESS_65_376 0
#define HID_MPS_LESS_65_377 0
#define HID_MPS_LESS_65_378 0
#define HID_MPS_LESS_65_379 0
#define HID_MPS_LESS_65_380 0
#define HID_MPS_LESS_65_381 0
#define HID_MPS_LESS_65_382 0
#define HID_MPS_LESS_65_383 0
#define HID_MPS_LESS_65_384 0
#define HID_MPS_LESS_65_385 0
#define HID_MPS_LESS_65_386 0
#define HID_MPS_LESS_65_387 0
#define HID_MPS_LESS_65_388 0
#define HID_MPS_LESS_65_389 0
#define HID_MPS_LESS_65_390 0
#define HID_MPS_LESS_65_391 0
#define HID_MPS_LESS_65_392 0
#define HID_MPS_LESS_65_393 0
#define HID_MPS_LESS_65_394 0
#define HID_MPS_LESS_65_395 0
#define HID_MPS_LESS_65_396 0
#define HID_MPS_LESS_65_397 0
#define HID_MPS_LESS_65_398 0
#define HID_MPS_LESS_65_399 0
#define HID_MPS_LESS_65_400 0
#define HID_MPS_LESS_65_401 0
#define HID_MPS_LESS_65_402 0
#define HID_MPS_LESS_65_403 0
#define HID_MPS_LESS_65_404 0
#define HID_MPS_LESS_65_405 0
#define HID_MPS_LESS_65_406 0
#define HID_MPS_LESS_65_407 0
#define HID_MPS_LESS_65_408 0
#define HID_MPS_LESS_65_409 0
#define HID_MPS_LESS_65_410 0
#define HID_MPS_LESS_65_411 0
#define HID_MPS_LESS_65_412 0
#define HID_MPS_LESS_65_413 0
#define HID_MPS_LESS_65_414 0
#define HID_MPS_LESS_65_415 0
#define HID_MPS_LESS_65_416 0
#define HID_MPS_LESS_65_417 0
#define HID_MPS_LESS_65_418 0
#define HID_MPS_LESS_65_419 0
#define HID_MPS_LESS_65_420 0
#define HID_MPS_LESS_65_421 0
#define HID_MPS_LESS_65_422 0
#define HID_MPS_LESS_65_423 0
#define HID_MPS_LESS_65_424 0
#define HID_MPS_LESS_65_425 0
#define HID_MPS_LESS_65_426 0
#define HID_MPS_LESS_65_427 0
#define HID_MPS_LESS_65_428 0
#define HID_MPS_LESS_65_429 0
#define HID_MPS_LESS_65_430 0
#define HID_MPS_LESS_65_431 0
#define HID_MPS_LESS_65_432 0
#define HID_MPS_LESS_65_433 0
#define HID_MPS_LESS_65_434 0
#define HID_MPS_LESS_65_435 0
#define HID_MPS_LESS_65_436 0
#define HID_MPS_LESS_65_437 0
#define HID_MPS_LESS_65_438 0
#define HID_MPS_LESS_65_439 0
#define HID_MPS_LESS_65_440 0
#define HID_MPS_LESS_65_441 0
#define HID_MPS_LESS_65_442 0
#define HID_MPS_LESS_65_443 0
#define HID_MPS_LESS_65_444 0
#define HID_MPS_LESS_65_445 0
#define HID_MPS_LESS_65_446 0
#define HID_MPS_LESS_65_447 0
#define HID_MPS_LESS_65_448 0
#define HID_MPS_LESS_65_449 0
#define HID_MPS_LESS_65_450 0
#define HID_MPS_LESS_65_451 0
#define HID_MPS_LESS_65_452 0
#define HID_MPS_LESS_65_453 0
#define HID_MPS_LESS_65_454 0
#define HID_MPS_LESS_65_455 0
#define HID_MPS_LESS_65_456 0
#define HID_MPS_LESS_65_457 0
#define HID_MPS_LESS_65_458 0
#define HID_MPS_LESS_65_459 0
#define HID_MPS_LESS_65_460 0
#define HID_MPS_LESS_65_461 0
#define HID_MPS_LESS_65_462 0
#define HID_MPS_LESS_65_463 0
#define HID_MPS_LESS_65_464 0
#define HID_MPS_LESS_65_465 0
#define HID_MPS_LESS_65_466 0
#define HID_MPS_LESS_65_467 0
#define HID_MPS_LESS_65_468 0
#define HID_MPS_LESS_65_469 0
#define HID_MPS_LESS_65_470 0
#define HID_MPS_LESS_65_471 0
#define HID_MPS_LESS_65_472 0
#define HID_MPS_LESS_65_473 0
#define HID_MPS_LESS_65_474 0
#define HID_MPS_LESS_65_475 0
#define HID_MPS_LESS_65_476 0
#define HID_MPS_LESS_65_477 0
#define HID_MPS_LESS_65_478 0
#define HID_MPS_LESS_65_479 0
#define HID_MPS_LESS_65_480 0
#define HID_MPS_LESS_65_481 0
#define HID_MPS_LESS_65_482 0
#define HID_MPS_LESS_65_483 0
#define HID_MPS_LESS_65_484 0
#define HID_MPS_LESS_65_485 0
#define HID_MPS_LESS_65_486 0
#define HID_MPS_LESS_65_487 0
#define HID_MPS_LESS_65_488 0
#define HID_MPS_LESS_65_489 0
#define HID_MPS_LESS_65_490 0
#define HID_MPS_LESS_65_491 0
#define HID_MPS_LESS_65_492 0
#define HID_MPS_LESS_65_493 0
#define HID_MPS_LESS_65_494 0
#define HID_MPS_LESS_65_495 0
#define HID_MPS_LESS_65_496 0
#define HID_MPS_LESS_65_497 0
#define HID_MPS_LESS_65_498 0
#define HID_MPS_LESS_65_499 0
#define HID_MPS_LESS_65_500 0
#define HID_MPS_LESS_65_501 0
#define HID_MPS_LESS_65_502 0
#define HID_MPS_LESS_65_503 0
#define HID_MPS_LESS_65_504 0
#define HID_MPS_LESS_65_505 0
#define HID_MPS_LESS_65_506 0
#define HID_MPS_LESS_65_507 0
#define HID_MPS_LESS_65_508 0
#define HID_MPS_LESS_65_509 0
#define HID_MPS_LESS_65_510 0
#define HID_MPS_LESS_65_511 0
#define HID_MPS_LESS_65_512 0
#define HID_MPS_LESS_65_513 0
#define HID_MPS_LESS_65_514 0
#define HID_MPS_LESS_65_515 0
#define HID_MPS_LESS_65_516 0
#define HID_MPS_LESS_65_517 0
#define HID_MPS_LESS_65_518 0
#define HID_MPS_LESS_65_519 0
#define HID_MPS_LESS_65_520 0
#define HID_MPS_LESS_65_521 0
#define HID_MPS_LESS_65_522 0
#define HID_MPS_LESS_65_523 0
#define HID_MPS_LESS_65_524 0
#define HID_MPS_LESS_65_525 0
#define HID_MPS_LESS_65_526 0
#define HID_MPS_LESS_65_527 0
#define HID_MPS_LESS_65_528 0
#define HID_MPS_LESS_65_529 0
#define HID_MPS_LESS_65_530 0
#define HID_MPS_LESS_65_531 0
#define HID_MPS_LESS_65_532 0
#define HID_MPS_LESS_65_533 0
#define HID_MPS_LESS_65_534 0
#define HID_MPS_LESS_65_535 0
#define HID_MPS_LESS_65_536 0
#define HID_MPS_LESS_65_537 0
#define HID_MPS_LESS_65_538 0
#define HID_MPS_LESS_65_539 0
#define HID_MPS_LESS_65_540 0
#define HID_MPS_LESS_65_541 0
#define HID_MPS_LESS_65_542 0
#define HID_MPS_LESS_65_543 0
#define HID_MPS_LESS_65_544 0
#define HID_MPS_LESS_65_545 0
#define HID_MPS_LESS_65_546 0
#define HID_MPS_LESS_65_547 0
#define HID_MPS_LESS_65_548 0
#define HID_MPS_LESS_65_549 0
#define HID_MPS_LESS_65_550 0
#define HID_MPS_LESS_65_551 0
#define HID_MPS_LESS_65_552 0
#define HID_MPS_LESS_65_553 0
#define HID_MPS_LESS_65_554 0
#define HID_MPS_LESS_65_555 0
#define HID_MPS_LESS_65_556 0
#define HID_MPS_LESS_65_557 0
#define HID_MPS_LESS_65_558 0
#define HID_MPS_LESS_65_559 0
#define HID_MPS_LESS_65_560 0
#define HID_MPS_LESS_65_561 0
#define HID_MPS_LESS_65_562 0
#define HID_MPS_LESS_65_563 0
#define HID_MPS_LESS_65_564 0
#define HID_MPS_LESS_65_565 0
#define HID_MPS_LESS_65_566 0
#define HID_MPS_LESS_65_567 0
#define HID_MPS_LESS_65_568 0
#define HID_MPS_LESS_65_569 0
#define HID_MPS_LESS_65_570 0
#define HID_MPS_LESS_65_571 0
#define HID_MPS_LESS_65_572 0
#define HID_MPS_LESS_65_573 0
#define HID_MPS_LESS_65_574 0
#define HID_MPS_LESS_65_575 0
#define HID_MPS_LESS_65_576 0
#define HID_MPS_LESS_65_577 0
#define HID_MPS_LESS_65_578 0
#define HID_MPS_LESS_65_579 0
#define HID_MPS_LESS_65_580 0
#define HID_MPS_LESS_65_581 0
#define HID_MPS_LESS_65_582 0
#define HID_MPS_LESS_65_583 0
#define HID_MPS_LESS_65_584 0
#define HID_MPS_LESS_65_585 0
#define HID_MPS_LESS_65_586 0
#define HID_MPS_LESS_65_587 0
#define HID_MPS_LESS_65_588 0
#define HID_MPS_LESS_65_589 0
#define HID_MPS_LESS_65_590 0
#define HID_MPS_LESS_65_591 0
#define HID_MPS_LESS_65_592 0
#define HID_MPS_LESS_65_593 0
#define HID_MPS_LESS_65_594 0
#define HID_MPS_LESS_65_595 0
#define HID_MPS_LESS_65_596 0
#define HID_MPS_LESS_65_597 0
#define HID_MPS_LESS_65_598 0
#define HID_MPS_LESS_65_599 0
#define HID_MPS_LESS_65_600 0
#define HID_MPS_LESS_65_601 0
#define HID_MPS_LESS_65_602 0
#define HID_MPS_LESS_65_603 0
#define HID_MPS_LESS_65_604 0
#define HID_MPS_LESS_65_605 0
#define HID_MPS_LESS_65_606 0
#define HID_MPS_LESS_65_607 0
#define HID_MPS_LESS_65_608 0
#define HID_MPS_LESS_65_609 0
#define HID_MPS_LESS_65_610 0
#define HID_MPS_LESS_65_611 0
#define HID_MPS_LESS_65_612 0
#define HID_MPS_LESS_65_613 0
#define HID_MPS_LESS_65_614 0
#define HID_MPS_LESS_65_615 0
#define HID_MPS_LESS_65_616 0
#define HID_MPS_LESS_65_617 0
#define HID_MPS_LESS_65_618 0
#define HID_MPS_LESS_65_619 0
#define HID_MPS_LESS_65_620 0
#define HID_MPS_LESS_65_621 0
#define HID_MPS_LESS_65_622 0
#define HID_MPS_LESS_65_623 0
#define HID_MPS_LESS_65_624 0
#define HID_MPS_LESS_65_625 0
#define HID_MPS_LESS_65_626 0
#define HID_MPS_LESS_65_627 0
#define HID_MPS_LESS_65_628 0
#define HID_MPS_LESS_65_629 0
#define HID_MPS_LESS_65_630 0
#define HID_MPS_LESS_65_631 0
#define HID_MPS_LESS_65_632 0
#define HID_MPS_LESS_65_633 0
#define HID_MPS_LESS_65_634 0
#define HID_MPS_LESS_65_635 0
#define HID_MPS_LESS_65_636 0
#define HID_MPS_LESS_65_637 0
#define HID_MPS_LESS_65_638 0
#define HID_MPS_LESS_65_639 0
#define HID_MPS_LESS_65_640 0
#define HID_MPS_LESS_65_641 0
#define HID_MPS_LESS_65_642 0
#define HID_MPS_LESS_65_643 0
#define HID_MPS_LESS_65_644 0
#define HID_MPS_LESS_65_645 0
#define HID_MPS_LESS_65_646 0
#define HID_MPS_LESS_65_647 0
#define HID_MPS_LESS_65_648 0
#define HID_MPS_LESS_65_649 0
#define HID_MPS_LESS_65_650 0
#define HID_MPS_LESS_65_651 0
#define HID_MPS_LESS_65_652 0
#define HID_MPS_LESS_65_653 0
#define HID_MPS_LESS_65_654 0
#define HID_MPS_LESS_65_655 0
#define HID_MPS_LESS_65_656 0
#define HID_MPS_LESS_65_657 0
#define HID_MPS_LESS_65_658 0
#define HID_MPS_LESS_65_659 0
#define HID_MPS_LESS_65_660 0
#define HID_MPS_LESS_65_661 0
#define HID_MPS_LESS_65_662 0
#define HID_MPS_LESS_65_663 0
#define HID_MPS_LESS_65_664 0
#define HID_MPS_LESS_65_665 0
#define HID_MPS_LESS_65_666 0
#define HID_MPS_LESS_65_667 0
#define HID_MPS_LESS_65_668 0
#define HID_MPS_LESS_65_669 0
#define HID_MPS_LESS_65_670 0
#define HID_MPS_LESS_65_671 0
#define HID_MPS_LESS_65_672 0
#define HID_MPS_LESS_65_673 0
#define HID_MPS_LESS_65_674 0
#define HID_MPS_LESS_65_675 0
#define HID_MPS_LESS_65_676 0
#define HID_MPS_LESS_65_677 0
#define HID_MPS_LESS_65_678 0
#define HID_MPS_LESS_65_679 0
#define HID_MPS_LESS_65_680 0
#define HID_MPS_LESS_65_681 0
#define HID_MPS_LESS_65_682 0
#define HID_MPS_LESS_65_683 0
#define HID_MPS_LESS_65_684 0
#define HID_MPS_LESS_65_685 0
#define HID_MPS_LESS_65_686 0
#define HID_MPS_LESS_65_687 0
#define HID_MPS_LESS_65_688 0
#define HID_MPS_LESS_65_689 0
#define HID_MPS_LESS_65_690 0
#define HID_MPS_LESS_65_691 0
#define HID_MPS_LESS_65_692 0
#define HID_MPS_LESS_65_693 0
#define HID_MPS_LESS_65_694 0
#define HID_MPS_LESS_65_695 0
#define HID_MPS_LESS_65_696 0
#define HID_MPS_LESS_65_697 0
#define HID_MPS_LESS_65_698 0
#define HID_MPS_LESS_65_699 0
#define HID_MPS_LESS_65_700 0
#define HID_MPS_LESS_65_701 0
#define HID_MPS_LESS_65_702 0
#define HID_MPS_LESS_65_703 0
#define HID_MPS_LESS_65_704 0
#define HID_MPS_LESS_65_705 0
#define HID_MPS_LESS_65_706 0
#define HID_MPS_LESS_65_707 0
#define HID_MPS_LESS_65_708 0
#define HID_MPS_LESS_65_709 0
#define HID_MPS_LESS_65_710 0
#define HID_MPS_LESS_65_711 0
#define HID_MPS_LESS_65_712 0
#define HID_MPS_LESS_65_713 0
#define HID_MPS_LESS_65_714 0
#define HID_MPS_LESS_65_715 0
#define HID_MPS_LESS_65_716 0
#define HID_MPS_LESS_65_717 0
#define HID_MPS_LESS_65_718 0
#define HID_MPS_LESS_65_719 0
#define HID_MPS_LESS_65_720 0
#define HID_MPS_LESS_65_721 0
#define HID_MPS_LESS_65_722 0
#define HID_MPS_LESS_65_723 0
#define HID_MPS_LESS_65_724 0
#define HID_MPS_LESS_65_725 0
#define HID_MPS_LESS_65_726 0
#define HID_MPS_LESS_65_727 0
#define HID_MPS_LESS_65_728 0
#define HID_MPS_LESS_65_729 0
#define HID_MPS_LESS_65_730 0
#define HID_MPS_LESS_65_731 0
#define HID_MPS_LESS_65_732 0
#define HID_MPS_LESS_65_733 0
#define HID_MPS_LESS_65_734 0
#define HID_MPS_LESS_65_735 0
#define HID_MPS_LESS_65_736 0
#define HID_MPS_LESS_65_737 0
#define HID_MPS_LESS_65_738 0
#define HID_MPS_LESS_65_739 0
#define HID_MPS_LESS_65_740 0
#define HID_MPS_LESS_65_741 0
#define HID_MPS_LESS_65_742 0
#define HID_MPS_LESS_65_743 0
#define HID_MPS_LESS_65_744 0
#define HID_MPS_LESS_65_745 0
#define HID_MPS_LESS_65_746 0
#define HID_MPS_LESS_65_747 0
#define HID_MPS_LESS_65_748 0
#define HID_MPS_LESS_65_749 0
#define HID_MPS_LESS_65_750 0
#define HID_MPS_LESS_65_751 0
#define HID_MPS_LESS_65_752 0
#define HID_MPS_LESS_65_753 0
#define HID_MPS_LESS_65_754 0
#define HID_MPS_LESS_65_755 0
#define HID_MPS_LESS_65_756 0
#define HID_MPS_LESS_65_757 0
#define HID_MPS_LESS_65_758 0
#define HID_MPS_LESS_65_759 0
#define HID_MPS_LESS_65_760 0
#define HID_MPS_LESS_65_761 0
#define HID_MPS_LESS_65_762 0
#define HID_MPS_LESS_65_763 0
#define HID_MPS_LESS_65_764 0
#define HID_MPS_LESS_65_765 0
#define HID_MPS_LESS_65_766 0
#define HID_MPS_LESS_65_767 0
#define HID_MPS_LESS_65_768 0
#define HID_MPS_LESS_65_769 0
#define HID_MPS_LESS_65_770 0
#define HID_MPS_LESS_65_771 0
#define HID_MPS_LESS_65_772 0
#define HID_MPS_LESS_65_773 0
#define HID_MPS_LESS_65_774 0
#define HID_MPS_LESS_65_775 0
#define HID_MPS_LESS_65_776 0
#define HID_MPS_LESS_65_777 0
#define HID_MPS_LESS_65_778 0
#define HID_MPS_LESS_65_779 0
#define HID_MPS_LESS_65_780 0
#define HID_MPS_LESS_65_781 0
#define HID_MPS_LESS_65_782 0
#define HID_MPS_LESS_65_783 0
#define HID_MPS_LESS_65_784 0
#define HID_MPS_LESS_65_785 0
#define HID_MPS_LESS_65_786 0
#define HID_MPS_LESS_65_787 0
#define HID_MPS_LESS_65_788 0
#define HID_MPS_LESS_65_789 0
#define HID_MPS_LESS_65_790 0
#define HID_MPS_LESS_65_791 0
#define HID_MPS_LESS_65_792 0
#define HID_MPS_LESS_65_793 0
#define HID_MPS_LESS_65_794 0
#define HID_MPS_LESS_65_795 0
#define HID_MPS_LESS_65_796 0
#define HID_MPS_LESS_65_797 0
#define HID_MPS_LESS_65_798 0
#define HID_MPS_LESS_65_799 0
#define HID_MPS_LESS_65_800 0
#define HID_MPS_LESS_65_801 0
#define HID_MPS_LESS_65_802 0
#define HID_MPS_LESS_65_803 0
#define HID_MPS_LESS_65_804 0
#define HID_MPS_LESS_65_805 0
#define HID_MPS_LESS_65_806 0
#define HID_MPS_LESS_65_807 0
#define HID_MPS_LESS_65_808 0
#define HID_MPS_LESS_65_809 0
#define HID_MPS_LESS_65_810 0
#define HID_MPS_LESS_65_811 0
#define HID_MPS_LESS_65_812 0
#define HID_MPS_LESS_65_813 0
#define HID_MPS_LESS_65_814 0
#define HID_MPS_LESS_65_815 0
#define HID_MPS_LESS_65_816 0
#define HID_MPS_LESS_65_817 0
#define HID_MPS_LESS_65_818 0
#define HID_MPS_LESS_65_819 0
#define HID_MPS_LESS_65_820 0
#define HID_MPS_LESS_65_821 0
#define HID_MPS_LESS_65_822 0
#define HID_MPS_LESS_65_823 0
#define HID_MPS_LESS_65_824 0
#define HID_MPS_LESS_65_825 0
#define HID_MPS_LESS_65_826 0
#define HID_MPS_LESS_65_827 0
#define HID_MPS_LESS_65_828 0
#define HID_MPS_LESS_65_829 0
#define HID_MPS_LESS_65_830 0
#define HID_MPS_LESS_65_831 0
#define HID_MPS_LESS_65_832 0
#define HID_MPS_LESS_65_833 0
#define HID_MPS_LESS_65_834 0
#define HID_MPS_LESS_65_835 0
#define HID_MPS_LESS_65_836 0
#define HID_MPS_LESS_65_837 0
#define HID_MPS_LESS_65_838 0
#define HID_MPS_LESS_65_839 0
#define HID_MPS_LESS_65_840 0
#define HID_MPS_LESS_65_841 0
#define HID_MPS_LESS_65_842 0
#define HID_MPS_LESS_65_843 0
#define HID_MPS_LESS_65_844 0
#define HID_MPS_LESS_65_845 0
#define HID_MPS_LESS_65_846 0
#define HID_MPS_LESS_65_847 0
#define HID_MPS_LESS_65_848 0
#define HID_MPS_LESS_65_849 0
#define HID_MPS_LESS_65_850 0
#define HID_MPS_LESS_65_851 0
#define HID_MPS_LESS_65_852 0
#define HID_MPS_LESS_65_853 0
#define HID_MPS_LESS_65_854 0
#define HID_MPS_LESS_65_855 0
#define HID_MPS_LESS_65_856 0
#define HID_MPS_LESS_65_857 0
#define HID_MPS_LESS_65_858 0
#define HID_MPS_LESS_65_859 0
#define HID_MPS_LESS_65_860 0
#define HID_MPS_LESS_65_861 0
#define HID_MPS_LESS_65_862 0
#define HID_MPS_LESS_65_863 0
#define HID_MPS_LESS_65_864 0
#define HID_MPS_LESS_65_865 0
#define HID_MPS_LESS_65_866 0
#define HID_MPS_LESS_65_867 0
#define HID_MPS_LESS_65_868 0
#define HID_MPS_LESS_65_869 0
#define HID_MPS_LESS_65_870 0
#define HID_MPS_LESS_65_871 0
#define HID_MPS_LESS_65_872 0
#define HID_MPS_LESS_65_873 0
#define HID_MPS_LESS_65_874 0
#define HID_MPS_LESS_65_875 0
#define HID_MPS_LESS_65_876 0
#define HID_MPS_LESS_65_877 0
#define HID_MPS_LESS_65_878 0
#define HID_MPS_LESS_65_879 0
#define HID_MPS_LESS_65_880 0
#define HID_MPS_LESS_65_881 0
#define HID_MPS_LESS_65_882 0
#define HID_MPS_LESS_65_883 0
#define HID_MPS_LESS_65_884 0
#define HID_MPS_LESS_65_885 0
#define HID_MPS_LESS_65_886 0
#define HID_MPS_LESS_65_887 0
#define HID_MPS_LESS_65_888 0
#define HID_MPS_LESS_65_889 0
#define HID_MPS_LESS_65_890 0
#define HID_MPS_LESS_65_891 0
#define HID_MPS_LESS_65_892 0
#define HID_MPS_LESS_65_893 0
#define HID_MPS_LESS_65_894 0
#define HID_MPS_LESS_65_895 0
#define HID_MPS_LESS_65_896 0
#define HID_MPS_LESS_65_897 0
#define HID_MPS_LESS_65_898 0
#define HID_MPS_LESS_65_899 0
#define HID_MPS_LESS_65_900 0
#define HID_MPS_LESS_65_901 0
#define HID_MPS_LESS_65_902 0
#define HID_MPS_LESS_65_903 0
#define HID_MPS_LESS_65_904 0
#define HID_MPS_LESS_65_905 0
#define HID_MPS_LESS_65_906 0
#define HID_MPS_LESS_65_907 0
#define HID_MPS_LESS_65_908 0
#define HID_MPS_LESS_65_909 0
#define HID_MPS_LESS_65_910 0
#define HID_MPS_LESS_65_911 0
#define HID_MPS_LESS_65_912 0
#define HID_MPS_LESS_65_913 0
#define HID_MPS_LESS_65_914 0
#define HID_MPS_LESS_65_915 0
#define HID_MPS_LESS_65_916 0
#define HID_MPS_LESS_65_917 0
#define HID_MPS_LESS_65_918 0
#define HID_MPS_LESS_65_919 0
#define HID_MPS_LESS_65_920 0
#define HID_MPS_LESS_65_921 0
#define HID_MPS_LESS_65_922 0
#define HID_MPS_LESS_65_923 0
#define HID_MPS_LESS_65_924 0
#define HID_MPS_LESS_65_925 0
#define HID_MPS_LESS_65_926 0
#define HID_MPS_LESS_65_927 0
#define HID_MPS_LESS_65_928 0
#define HID_MPS_LESS_65_929 0
#define HID_MPS_LESS_65_930 0
#define HID_MPS_LESS_65_931 0
#define HID_MPS_LESS_65_932 0
#define HID_MPS_LESS_65_933 0
#define HID_MPS_LESS_65_934 0
#define HID_MPS_LESS_65_935 0
#define HID_MPS_LESS_65_936 0
#define HID_MPS_LESS_65_937 0
#define HID_MPS_LESS_65_938 0
#define HID_MPS_LESS_65_939 0
#define HID_MPS_LESS_65_940 0
#define HID_MPS_LESS_65_941 0
#define HID_MPS_LESS_65_942 0
#define HID_MPS_LESS_65_943 0
#define HID_MPS_LESS_65_944 0
#define HID_MPS_LESS_65_945 0
#define HID_MPS_LESS_65_946 0
#define HID_MPS_LESS_65_947 0
#define HID_MPS_LESS_65_948 0
#define HID_MPS_LESS_65_949 0
#define HID_MPS_LESS_65_950 0
#define HID_MPS_LESS_65_951 0
#define HID_MPS_LESS_65_952 0
#define HID_MPS_LESS_65_953 0
#define HID_MPS_LESS_65_954 0
#define HID_MPS_LESS_65_955 0
#define HID_MPS_LESS_65_956 0
#define HID_MPS_LESS_65_957 0
#define HID_MPS_LESS_65_958 0
#define HID_MPS_LESS_65_959 0
#define HID_MPS_LESS_65_960 0
#define HID_MPS_LESS_65_961 0
#define HID_MPS_LESS_65_962 0
#define HID_MPS_LESS_65_963 0
#define HID_MPS_LESS_65_964 0
#define HID_MPS_LESS_65_965 0
#define HID_MPS_LESS_65_966 0
#define HID_MPS_LESS_65_967 0
#define HID_MPS_LESS_65_968 0
#define HID_MPS_LESS_65_969 0
#define HID_MPS_LESS_65_970 0
#define HID_MPS_LESS_65_971 0
#define HID_MPS_LESS_65_972 0
#define HID_MPS_LESS_65_973 0
#define HID_MPS_LESS_65_974 0
#define HID_MPS_LESS_65_975 0
#define HID_MPS_LESS_65_976 0
#define HID_MPS_LESS_65_977 0
#define HID_MPS_LESS_65_978 0
#define HID_MPS_LESS_65_979 0
#define HID_MPS_LESS_65_980 0
#define HID_MPS_LESS_65_981 0
#define HID_MPS_LESS_65_982 0
#define HID_MPS_LESS_65_983 0
#define HID_MPS_LESS_65_984 0
#define HID_MPS_LESS_65_985 0
#define HID_MPS_LESS_65_986 0
#define HID_MPS_LESS_65_987 0
#define HID_MPS_LESS_65_988 0
#define HID_MPS_LESS_65_989 0
#define HID_MPS_LESS_65_990 0
#define HID_MPS_LESS_65_991 0
#define HID_MPS_LESS_65_992 0
#define HID_MPS_LESS_65_993 0
#define HID_MPS_LESS_65_994 0
#define HID_MPS_LESS_65_995 0
#define HID_MPS_LESS_65_996 0
#define HID_MPS_LESS_65_997 0
#define HID_MPS_LESS_65_998 0
#define HID_MPS_LESS_65_999 0
#define HID_MPS_LESS_65_1000 0
#define HID_MPS_LESS_65_1001 0
#define HID_MPS_LESS_65_1002 0
#define HID_MPS_LESS_65_1003 0
#define HID_MPS_LESS_65_1004 0
#define HID_MPS_LESS_65_1005 0
#define HID_MPS_LESS_65_1006 0
#define HID_MPS_LESS_65_1007 0
#define HID_MPS_LESS_65_1008 0
#define HID_MPS_LESS_65_1009 0
#define HID_MPS_LESS_65_1010 0
#define HID_MPS_LESS_65_1011 0
#define HID_MPS_LESS_65_1012 0
#define HID_MPS_LESS_65_1013 0
#define HID_MPS_LESS_65_1014 0
#define HID_MPS_LESS_65_1015 0
#define HID_MPS_LESS_65_1016 0
#define HID_MPS_LESS_65_1017 0
#define HID_MPS_LESS_65_1018 0
#define HID_MPS_LESS_65_1019 0
#define HID_MPS_LESS_65_1020 0
#define HID_MPS_LESS_65_1021 0
#define HID_MPS_LESS_65_1022 0
#define HID_MPS_LESS_65_1023 0
#define HID_MPS_LESS_65_1024 0

#define HID_MPS_LESS_65(x) UTIL_PRIMITIVE_CAT(HID_MPS_LESS_65_, x)

/*
 * If all the endpoint MPS are less than 65 bytes, we do not need to define and
 * configure an alternate interface.
 */
#define HID_ALL_MPS_LESS_65(n)							\
	UTIL_AND(HID_MPS_LESS_65(DT_INST_PROP(n, out_report_size)),		\
		 HID_MPS_LESS_65(DT_INST_PROP(n, in_report_size)))

/* Get IN endpoint polling rate based on the desired speed. */
#define HID_IN_EP_INTERVAL(n, hs)							\
	COND_CODE_1(hs,									\
		    (USB_HS_INT_EP_INTERVAL(DT_INST_PROP(n, in_polling_period_us))),	\
		    (USB_FS_INT_EP_INTERVAL(DT_INST_PROP(n, in_polling_period_us))))

/* Get OUT endpoint polling rate based on the desired speed. */
#define HID_OUT_EP_INTERVAL(n, hs)							\
	COND_CODE_1(hs,									\
		    (USB_HS_INT_EP_INTERVAL(DT_INST_PROP(n, out_polling_period_us))),	\
		    (USB_FS_INT_EP_INTERVAL(DT_INST_PROP(n, out_polling_period_us))))

/* Get the number of endpoints, which can be either 1 or 2 */
#define HID_NUM_ENDPOINTS(n)							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, out_report_size), (2), (1))

/*
 * Either the device does not support a boot protocol, or it supports the
 * keyboard or mouse boot protocol.
 */
#define HID_INTERFACE_PROTOCOL(n) DT_INST_ENUM_IDX_OR(n, protocol_code, 0)

/* bInterfaceSubClass must be set to 1 if a boot device protocol is supported */
#define HID_INTERFACE_SUBCLASS(n)						\
	COND_CODE_0(HID_INTERFACE_PROTOCOL(n), (0), (1))

#define HID_INTERFACE_DEFINE(n, alt)						\
	{									\
		.bLength = sizeof(struct usb_if_descriptor),			\
		.bDescriptorType = USB_DESC_INTERFACE,				\
		.bInterfaceNumber = 0,						\
		.bAlternateSetting = alt,					\
		.bNumEndpoints = HID_NUM_ENDPOINTS(n),				\
		.bInterfaceClass = USB_BCC_HID,					\
		.bInterfaceSubClass = HID_INTERFACE_SUBCLASS(n),		\
		.bInterfaceProtocol = HID_INTERFACE_PROTOCOL(n),		\
		.iInterface = 0,						\
	}

#define HID_DESCRIPTOR_DEFINE(n)						\
	{									\
		.bLength = sizeof(struct hid_descriptor),			\
		.bDescriptorType = USB_DESC_HID,				\
		.bcdHID = sys_cpu_to_le16(USB_HID_VERSION),			\
		.bCountryCode = 0,						\
		.bNumDescriptors = HID_SUBORDINATE_DESC_NUM,			\
		.sub[0] = {							\
			.bDescriptorType = USB_DESC_HID_REPORT,			\
			.wDescriptorLength = 0,					\
		},								\
	}									\

/*
 * OUT endpoint MPS for either default or alternate interface.
 * MPS for the default interface is always limited to 64 bytes.
 */
#define HID_OUT_EP_MPS(n, alt)							\
	COND_CODE_1(alt,							\
	(sys_cpu_to_le16(DT_INST_PROP(n, out_report_size))),			\
	(sys_cpu_to_le16(MIN(DT_INST_PROP(n, out_report_size), 64U))))

/*
 * IN endpoint MPS for either default or alternate interface.
 * MPS for the default interface is always limited to 64 bytes.
 */
#define HID_IN_EP_MPS(n, alt)							\
	COND_CODE_1(alt,							\
	(sys_cpu_to_le16(DT_INST_PROP(n, in_report_size))),			\
	(sys_cpu_to_le16(MIN(DT_INST_PROP(n, in_report_size), 64U))))

#define HID_OUT_EP_DEFINE(n, hs, alt)						\
	{									\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x01,					\
		.bmAttributes = USB_EP_TYPE_INTERRUPT,				\
		.wMaxPacketSize = HID_OUT_EP_MPS(n, alt),			\
		.bInterval = HID_OUT_EP_INTERVAL(n, hs),			\
	}

#define HID_IN_EP_DEFINE(n, hs, alt)						\
	{									\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x81,					\
		.bmAttributes = USB_EP_TYPE_INTERRUPT,				\
		.wMaxPacketSize = HID_IN_EP_MPS(n, alt),			\
		.bInterval = HID_IN_EP_INTERVAL(n, hs),				\
	}

/*
 * Both the optional OUT endpoint and the associated pool are only defined if
 * there is an out-report-size property.
 */
#define HID_OUT_EP_DEFINE_OR_ZERO(n, hs, alt)					\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, out_report_size),			\
		    (HID_OUT_EP_DEFINE(n, hs, alt)),				\
		    ({0}))

#define HID_OUT_POOL_DEFINE(n)							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, out_report_size),			\
		    (UDC_BUF_POOL_DEFINE(hid_buf_pool_out_##n,			\
					 CONFIG_USBD_HID_OUT_BUF_COUNT,		\
					 DT_INST_PROP(n, out_report_size),	\
					 sizeof(struct udc_buf_info), NULL)),	\
		    ())

#define HID_OUT_POOL_ADDR(n)							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, out_report_size),			\
		    (&hid_buf_pool_out_##n), (NULL))

#endif /* ZEPHYR_USB_DEVICE_CLASS_HID_MACROS_H_ */
