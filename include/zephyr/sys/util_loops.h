/*
 * Copyright (c) 2021, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Internals for looping macros
 *
 * Repetitive or obscure helper macros needed by sys/util.h.
 */

#ifndef ZEPHYR_INCLUDE_SYS_UTIL_LOOPS_H_
#define ZEPHYR_INCLUDE_SYS_UTIL_LOOPS_H_

#define Z_FOR_LOOP_GET_ARG(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			   _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, \
			   _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
			   _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, \
			   _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
			   _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
			   _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, \
			   _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, \
			   _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			   _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, \
			   _101, _102, _103, _104, _105, _106, _107, _108, _109, _110, \
			   _111, _112, _113, _114, _115, _116, _117, _118, _119, _120, \
			   _121, _122, _123, _124, _125, _126, _127, _128, _129, _130, \
			   _131, _132, _133, _134, _135, _136, _137, _138, _139, _140, \
			   _141, _142, _143, _144, _145, _146, _147, _148, _149, _150, \
			   _151, _152, _153, _154, _155, _156, _157, _158, _159, _160, \
			   _161, _162, _163, _164, _165, _166, _167, _168, _169, _170, \
			   _171, _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			   _181, _182, _183, _184, _185, _186, _187, _188, _189, _190, \
			   _191, _192, _193, _194, _195, _196, _197, _198, _199, _200, \
			   _201, _202, _203, _204, _205, _206, _207, _208, _209, _210, \
			   _211, _212, _213, _214, _215, _216, _217, _218, _219, _220, \
			   _221, _222, _223, _224, _225, _226, _227, _228, _229, _230, \
			   _231, _232, _233, _234, _235, _236, _237, _238, _239, _240, \
			   _241, _242, _243, _244, _245, _246, _247, _248, _249, _250, \
                           _251, _252, _253, _254, _255, _256, N, ... ) N

#define Z_FOR_LOOP_0(z_call, sep, fixed_arg0, fixed_arg1, ...)

#define Z_FOR_LOOP_1(z_call, sep, fixed_arg0, fixed_arg1, x) \
	z_call(0, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_2(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_1(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(1, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_3(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_2(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(2, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_4(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_3(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(3, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_5(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_4(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(4, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_6(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_5(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(5, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_7(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_6(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(6, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_8(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_7(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(7, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_9(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_8(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(8, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_10(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_9(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(9, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_11(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_10(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(10, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_12(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_11(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(11, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_13(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_12(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(12, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_14(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_13(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(13, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_15(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_14(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(14, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_16(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_15(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(15, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_17(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_16(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(16, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_18(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_17(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(17, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_19(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_18(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(18, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_20(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_19(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(19, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_21(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_20(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(20, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_22(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_21(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(21, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_23(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_22(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(22, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_24(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_23(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(23, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_25(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_24(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(24, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_26(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_25(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(25, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_27(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_26(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(26, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_28(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_27(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(27, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_29(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_28(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(28, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_30(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_29(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(29, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_31(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_30(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(30, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_32(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_31(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(31, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_33(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_32(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(32, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_34(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_33(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(33, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_35(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_34(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(34, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_36(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_35(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(35, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_37(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_36(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(36, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_38(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_37(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(37, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_39(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_38(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(38, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_40(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_39(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(39, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_41(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_40(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(40, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_42(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_41(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(41, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_43(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_42(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(42, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_44(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_43(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(43, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_45(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_44(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(44, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_46(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_45(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(45, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_47(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_46(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(46, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_48(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_47(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(47, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_49(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_48(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(48, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_50(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_49(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(49, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_51(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_50(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(50, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_52(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_51(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(51, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_53(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_52(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(52, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_54(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_53(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(53, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_55(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_54(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(54, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_56(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_55(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(55, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_57(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_56(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(56, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_58(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_57(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(57, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_59(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_58(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(58, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_60(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_59(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(59, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_61(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_60(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(60, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_62(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_61(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(61, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_63(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_62(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(62, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_64(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_63(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(63, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_65(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_64(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(64, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_66(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_65(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(65, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_67(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_66(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(66, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_68(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_67(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(67, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_69(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_68(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(68, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_70(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_69(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(69, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_71(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_70(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(70, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_72(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_71(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(71, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_73(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_72(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(72, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_74(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_73(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(73, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_75(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_74(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(74, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_76(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_75(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(75, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_77(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_76(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(76, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_78(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_77(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(77, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_79(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_78(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(78, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_80(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_79(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(79, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_81(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_80(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(80, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_82(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_81(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(81, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_83(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_82(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(82, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_84(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_83(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(83, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_85(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_84(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(84, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_86(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_85(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(85, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_87(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_86(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(86, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_88(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_87(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(87, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_89(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_88(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(88, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_90(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_89(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(89, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_91(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_90(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(90, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_92(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_91(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(91, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_93(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_92(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(92, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_94(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_93(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(93, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_95(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_94(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(94, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_96(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_95(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(95, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_97(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_96(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(96, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_98(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_97(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(97, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_99(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_98(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(98, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_100(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_99(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(99, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_101(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_100(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(100, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_102(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_101(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(101, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_103(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_102(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(102, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_104(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_103(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(103, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_105(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_104(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(104, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_106(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_105(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(105, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_107(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_106(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(106, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_108(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_107(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(107, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_109(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_108(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(108, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_110(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_109(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(109, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_111(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_110(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(110, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_112(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_111(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(111, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_113(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_112(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(112, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_114(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_113(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(113, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_115(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_114(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(114, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_116(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_115(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(115, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_117(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_116(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(116, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_118(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_117(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(117, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_119(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_118(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(118, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_120(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_119(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(119, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_121(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_120(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(120, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_122(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_121(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(121, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_123(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_122(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(122, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_124(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_123(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(123, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_125(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_124(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(124, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_126(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_125(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(125, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_127(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_126(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(126, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_128(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_127(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(127, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_129(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_128(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(128, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_130(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_129(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(129, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_131(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_130(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(130, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_132(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_131(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(131, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_133(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_132(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(132, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_134(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_133(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(133, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_135(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_134(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(134, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_136(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_135(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(135, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_137(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_136(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(136, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_138(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_137(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(137, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_139(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_138(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(138, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_140(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_139(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(139, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_141(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_140(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(140, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_142(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_141(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(141, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_143(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_142(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(142, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_144(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_143(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(143, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_145(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_144(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(144, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_146(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_145(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(145, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_147(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_146(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(146, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_148(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_147(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(147, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_149(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_148(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(148, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_150(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_149(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(149, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_151(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_150(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(150, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_152(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_151(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(151, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_153(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_152(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(152, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_154(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_153(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(153, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_155(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_154(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(154, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_156(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_155(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(155, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_157(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_156(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(156, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_158(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_157(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(157, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_159(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_158(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(158, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_160(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_159(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(159, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_161(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_160(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(160, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_162(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_161(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(161, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_163(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_162(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(162, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_164(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_163(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(163, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_165(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_164(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(164, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_166(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_165(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(165, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_167(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_166(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(166, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_168(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_167(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(167, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_169(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_168(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(168, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_170(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_169(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(169, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_171(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_170(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(170, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_172(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_171(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(171, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_173(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_172(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(172, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_174(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_173(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(173, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_175(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_174(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(174, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_176(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_175(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(175, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_177(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_176(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(176, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_178(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_177(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(177, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_179(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_178(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(178, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_180(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_179(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(179, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_181(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_180(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(180, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_182(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_181(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(181, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_183(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_182(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(182, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_184(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_183(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(183, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_185(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_184(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(184, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_186(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_185(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(185, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_187(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_186(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(186, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_188(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_187(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(187, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_189(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_188(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(188, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_190(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_189(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(189, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_191(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_190(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(190, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_192(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_191(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(191, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_193(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_192(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(192, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_194(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_193(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(193, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_195(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_194(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(194, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_196(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_195(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(195, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_197(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_196(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(196, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_198(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_197(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(197, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_199(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_198(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(198, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_200(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_199(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(199, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_201(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_200(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(200, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_202(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_201(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(201, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_203(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_202(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(202, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_204(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_203(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(203, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_205(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_204(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(204, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_206(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_205(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(205, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_207(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_206(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(206, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_208(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_207(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(207, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_209(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_208(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(208, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_210(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_209(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(209, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_211(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_210(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(210, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_212(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_211(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(211, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_213(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_212(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(212, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_214(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_213(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(213, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_215(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_214(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(214, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_216(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_215(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(215, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_217(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_216(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(216, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_218(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_217(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(217, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_219(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_218(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(218, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_220(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_219(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(219, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_221(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_220(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(220, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_222(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_221(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(221, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_223(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_222(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(222, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_224(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_223(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(223, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_225(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_224(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(224, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_226(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_225(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(225, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_227(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_226(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(226, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_228(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_227(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(227, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_229(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_228(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(228, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_230(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_229(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(229, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_231(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_230(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(230, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_232(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_231(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(231, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_233(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_232(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(232, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_234(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_233(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(233, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_235(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_234(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(234, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_236(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_235(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(235, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_237(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_236(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(236, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_238(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_237(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(237, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_239(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_238(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(238, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_240(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_239(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(239, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_241(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_240(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(240, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_242(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_241(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(241, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_243(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_242(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(242, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_244(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_243(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(243, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_245(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_244(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(244, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_246(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_245(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(245, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_247(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_246(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(246, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_248(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_247(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(247, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_249(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_248(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(248, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_250(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_249(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(249, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_251(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_250(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(250, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_252(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_251(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(251, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_253(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_252(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(252, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_254(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_253(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(253, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_255(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_254(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(254, x, fixed_arg0, fixed_arg1)

#define Z_FOR_LOOP_256(z_call, sep, fixed_arg0, fixed_arg1, x, ...) \
	Z_FOR_LOOP_255(z_call, sep, fixed_arg0, fixed_arg1, ##__VA_ARGS__) \
	__DEBRACKET sep \
	z_call(255, x, fixed_arg0, fixed_arg1)

#define Z_FOR_EACH_ENGINE(x, sep, fixed_arg0, fixed_arg1, ...) \
	Z_FOR_LOOP_GET_ARG(__VA_ARGS__, \
		Z_FOR_LOOP_256, \
		Z_FOR_LOOP_255, \
		Z_FOR_LOOP_254, \
		Z_FOR_LOOP_253, \
		Z_FOR_LOOP_252, \
		Z_FOR_LOOP_251, \
		Z_FOR_LOOP_250, \
		Z_FOR_LOOP_249, \
		Z_FOR_LOOP_248, \
		Z_FOR_LOOP_247, \
		Z_FOR_LOOP_246, \
		Z_FOR_LOOP_245, \
		Z_FOR_LOOP_244, \
		Z_FOR_LOOP_243, \
		Z_FOR_LOOP_242, \
		Z_FOR_LOOP_241, \
		Z_FOR_LOOP_240, \
		Z_FOR_LOOP_239, \
		Z_FOR_LOOP_238, \
		Z_FOR_LOOP_237, \
		Z_FOR_LOOP_236, \
		Z_FOR_LOOP_235, \
		Z_FOR_LOOP_234, \
		Z_FOR_LOOP_233, \
		Z_FOR_LOOP_232, \
		Z_FOR_LOOP_231, \
		Z_FOR_LOOP_230, \
		Z_FOR_LOOP_229, \
		Z_FOR_LOOP_228, \
		Z_FOR_LOOP_227, \
		Z_FOR_LOOP_226, \
		Z_FOR_LOOP_225, \
		Z_FOR_LOOP_224, \
		Z_FOR_LOOP_223, \
		Z_FOR_LOOP_222, \
		Z_FOR_LOOP_221, \
		Z_FOR_LOOP_220, \
		Z_FOR_LOOP_219, \
		Z_FOR_LOOP_218, \
		Z_FOR_LOOP_217, \
		Z_FOR_LOOP_216, \
		Z_FOR_LOOP_215, \
		Z_FOR_LOOP_214, \
		Z_FOR_LOOP_213, \
		Z_FOR_LOOP_212, \
		Z_FOR_LOOP_211, \
		Z_FOR_LOOP_210, \
		Z_FOR_LOOP_209, \
		Z_FOR_LOOP_208, \
		Z_FOR_LOOP_207, \
		Z_FOR_LOOP_206, \
		Z_FOR_LOOP_205, \
		Z_FOR_LOOP_204, \
		Z_FOR_LOOP_203, \
		Z_FOR_LOOP_202, \
		Z_FOR_LOOP_201, \
		Z_FOR_LOOP_200, \
		Z_FOR_LOOP_199, \
		Z_FOR_LOOP_198, \
		Z_FOR_LOOP_197, \
		Z_FOR_LOOP_196, \
		Z_FOR_LOOP_195, \
		Z_FOR_LOOP_194, \
		Z_FOR_LOOP_193, \
		Z_FOR_LOOP_192, \
		Z_FOR_LOOP_191, \
		Z_FOR_LOOP_190, \
		Z_FOR_LOOP_189, \
		Z_FOR_LOOP_188, \
		Z_FOR_LOOP_187, \
		Z_FOR_LOOP_186, \
		Z_FOR_LOOP_185, \
		Z_FOR_LOOP_184, \
		Z_FOR_LOOP_183, \
		Z_FOR_LOOP_182, \
		Z_FOR_LOOP_181, \
		Z_FOR_LOOP_180, \
		Z_FOR_LOOP_179, \
		Z_FOR_LOOP_178, \
		Z_FOR_LOOP_177, \
		Z_FOR_LOOP_176, \
		Z_FOR_LOOP_175, \
		Z_FOR_LOOP_174, \
		Z_FOR_LOOP_173, \
		Z_FOR_LOOP_172, \
		Z_FOR_LOOP_171, \
		Z_FOR_LOOP_170, \
		Z_FOR_LOOP_169, \
		Z_FOR_LOOP_168, \
		Z_FOR_LOOP_167, \
		Z_FOR_LOOP_166, \
		Z_FOR_LOOP_165, \
		Z_FOR_LOOP_164, \
		Z_FOR_LOOP_163, \
		Z_FOR_LOOP_162, \
		Z_FOR_LOOP_161, \
		Z_FOR_LOOP_160, \
		Z_FOR_LOOP_159, \
		Z_FOR_LOOP_158, \
		Z_FOR_LOOP_157, \
		Z_FOR_LOOP_156, \
		Z_FOR_LOOP_155, \
		Z_FOR_LOOP_154, \
		Z_FOR_LOOP_153, \
		Z_FOR_LOOP_152, \
		Z_FOR_LOOP_151, \
		Z_FOR_LOOP_150, \
		Z_FOR_LOOP_149, \
		Z_FOR_LOOP_148, \
		Z_FOR_LOOP_147, \
		Z_FOR_LOOP_146, \
		Z_FOR_LOOP_145, \
		Z_FOR_LOOP_144, \
		Z_FOR_LOOP_143, \
		Z_FOR_LOOP_142, \
		Z_FOR_LOOP_141, \
		Z_FOR_LOOP_140, \
		Z_FOR_LOOP_139, \
		Z_FOR_LOOP_138, \
		Z_FOR_LOOP_137, \
		Z_FOR_LOOP_136, \
		Z_FOR_LOOP_135, \
		Z_FOR_LOOP_134, \
		Z_FOR_LOOP_133, \
		Z_FOR_LOOP_132, \
		Z_FOR_LOOP_131, \
		Z_FOR_LOOP_130, \
		Z_FOR_LOOP_129, \
		Z_FOR_LOOP_128, \
		Z_FOR_LOOP_127, \
		Z_FOR_LOOP_126, \
		Z_FOR_LOOP_125, \
		Z_FOR_LOOP_124, \
		Z_FOR_LOOP_123, \
		Z_FOR_LOOP_122, \
		Z_FOR_LOOP_121, \
		Z_FOR_LOOP_120, \
		Z_FOR_LOOP_119, \
		Z_FOR_LOOP_118, \
		Z_FOR_LOOP_117, \
		Z_FOR_LOOP_116, \
		Z_FOR_LOOP_115, \
		Z_FOR_LOOP_114, \
		Z_FOR_LOOP_113, \
		Z_FOR_LOOP_112, \
		Z_FOR_LOOP_111, \
		Z_FOR_LOOP_110, \
		Z_FOR_LOOP_109, \
		Z_FOR_LOOP_108, \
		Z_FOR_LOOP_107, \
		Z_FOR_LOOP_106, \
		Z_FOR_LOOP_105, \
		Z_FOR_LOOP_104, \
		Z_FOR_LOOP_103, \
		Z_FOR_LOOP_102, \
		Z_FOR_LOOP_101, \
		Z_FOR_LOOP_100, \
		Z_FOR_LOOP_99, \
		Z_FOR_LOOP_98, \
		Z_FOR_LOOP_97, \
		Z_FOR_LOOP_96, \
		Z_FOR_LOOP_95, \
		Z_FOR_LOOP_94, \
		Z_FOR_LOOP_93, \
		Z_FOR_LOOP_92, \
		Z_FOR_LOOP_91, \
		Z_FOR_LOOP_90, \
		Z_FOR_LOOP_89, \
		Z_FOR_LOOP_88, \
		Z_FOR_LOOP_87, \
		Z_FOR_LOOP_86, \
		Z_FOR_LOOP_85, \
		Z_FOR_LOOP_84, \
		Z_FOR_LOOP_83, \
		Z_FOR_LOOP_82, \
		Z_FOR_LOOP_81, \
		Z_FOR_LOOP_80, \
		Z_FOR_LOOP_79, \
		Z_FOR_LOOP_78, \
		Z_FOR_LOOP_77, \
		Z_FOR_LOOP_76, \
		Z_FOR_LOOP_75, \
		Z_FOR_LOOP_74, \
		Z_FOR_LOOP_73, \
		Z_FOR_LOOP_72, \
		Z_FOR_LOOP_71, \
		Z_FOR_LOOP_70, \
		Z_FOR_LOOP_69, \
		Z_FOR_LOOP_68, \
		Z_FOR_LOOP_67, \
		Z_FOR_LOOP_66, \
		Z_FOR_LOOP_65, \
		Z_FOR_LOOP_64, \
		Z_FOR_LOOP_63, \
		Z_FOR_LOOP_62, \
		Z_FOR_LOOP_61, \
		Z_FOR_LOOP_60, \
		Z_FOR_LOOP_59, \
		Z_FOR_LOOP_58, \
		Z_FOR_LOOP_57, \
		Z_FOR_LOOP_56, \
		Z_FOR_LOOP_55, \
		Z_FOR_LOOP_54, \
		Z_FOR_LOOP_53, \
		Z_FOR_LOOP_52, \
		Z_FOR_LOOP_51, \
		Z_FOR_LOOP_50, \
		Z_FOR_LOOP_49, \
		Z_FOR_LOOP_48, \
		Z_FOR_LOOP_47, \
		Z_FOR_LOOP_46, \
		Z_FOR_LOOP_45, \
		Z_FOR_LOOP_44, \
		Z_FOR_LOOP_43, \
		Z_FOR_LOOP_42, \
		Z_FOR_LOOP_41, \
		Z_FOR_LOOP_40, \
		Z_FOR_LOOP_39, \
		Z_FOR_LOOP_38, \
		Z_FOR_LOOP_37, \
		Z_FOR_LOOP_36, \
		Z_FOR_LOOP_35, \
		Z_FOR_LOOP_34, \
		Z_FOR_LOOP_33, \
		Z_FOR_LOOP_32, \
		Z_FOR_LOOP_31, \
		Z_FOR_LOOP_30, \
		Z_FOR_LOOP_29, \
		Z_FOR_LOOP_28, \
		Z_FOR_LOOP_27, \
		Z_FOR_LOOP_26, \
		Z_FOR_LOOP_25, \
		Z_FOR_LOOP_24, \
		Z_FOR_LOOP_23, \
		Z_FOR_LOOP_22, \
		Z_FOR_LOOP_21, \
		Z_FOR_LOOP_20, \
		Z_FOR_LOOP_19, \
		Z_FOR_LOOP_18, \
		Z_FOR_LOOP_17, \
		Z_FOR_LOOP_16, \
		Z_FOR_LOOP_15, \
		Z_FOR_LOOP_14, \
		Z_FOR_LOOP_13, \
		Z_FOR_LOOP_12, \
		Z_FOR_LOOP_11, \
		Z_FOR_LOOP_10, \
		Z_FOR_LOOP_9, \
		Z_FOR_LOOP_8, \
		Z_FOR_LOOP_7, \
		Z_FOR_LOOP_6, \
		Z_FOR_LOOP_5, \
		Z_FOR_LOOP_4, \
		Z_FOR_LOOP_3, \
		Z_FOR_LOOP_2, \
		Z_FOR_LOOP_1, \
		Z_FOR_LOOP_0)(x, sep, fixed_arg0, fixed_arg1, __VA_ARGS__)

#define Z_GET_ARG_1(_0, ...) _0

#define Z_GET_ARG_2(_0, _1, ...) _1

#define Z_GET_ARG_3(_0, _1, _2, ...) _2

#define Z_GET_ARG_4(_0, _1, _2, _3, ...) _3

#define Z_GET_ARG_5(_0, _1, _2, _3, _4, ...) _4

#define Z_GET_ARG_6(_0, _1, _2, _3, _4, _5, ...) _5

#define Z_GET_ARG_7(_0, _1, _2, _3, _4, _5, _6, ...) _6

#define Z_GET_ARG_8(_0, _1, _2, _3, _4, _5, _6, _7, ...) _7

#define Z_GET_ARG_9(_0, _1, _2, _3, _4, _5, _6, _7, _8, ...) _8

#define Z_GET_ARG_10(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, ...) _9

#define Z_GET_ARG_11(_0, _1, _2, _3, _4, _5, \
			  _6, _7, _8, _9, _10, ...) _10

#define Z_GET_ARG_12(_0, _1, _2, _3, _4, _5, _6,\
			  _7, _8, _9, _10, _11, ...) _11

#define Z_GET_ARG_13(_0, _1, _2, _3, _4, _5, _6, \
			  _7, _8, _9, _10, _11, _12, ...) _12

#define Z_GET_ARG_14(_0, _1, _2, _3, _4, _5, _6, \
			  _7, _8, _9, _10, _11, _12, _13, ...) _13

#define Z_GET_ARG_15(_0, _1, _2, _3, _4, _5, _6, _7, \
			  _8, _9, _10, _11, _12, _13, _14, ...) _14

#define Z_GET_ARG_16(_0, _1, _2, _3, _4, _5, _6, _7, \
			  _8, _9, _10, _11, _12, _13, _14, _15, ...) _15

#define Z_GET_ARG_17(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, ...) _16

#define Z_GET_ARG_18(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, ...) _17

#define Z_GET_ARG_19(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, ...) _18

#define Z_GET_ARG_20(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  ...) _19

#define Z_GET_ARG_21(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, ...) _20

#define Z_GET_ARG_22(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, ...) _21

#define Z_GET_ARG_23(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, ...) _22

#define Z_GET_ARG_24(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, ...) _23

#define Z_GET_ARG_25(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, ...) _24

#define Z_GET_ARG_26(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, ...) _25

#define Z_GET_ARG_27(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, ...) _26

#define Z_GET_ARG_28(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, _27, ...) _27

#define Z_GET_ARG_29(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, _27, _28, \
			  ...) _28

#define Z_GET_ARG_30(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, _27, _28, \
			  _29, ...) _29

#define Z_GET_ARG_31(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, _27, _28, \
			  _29, _30, ...) _30

#define Z_GET_ARG_32(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, _27, _28, \
			  _29, _30, _31, ...) _31

#define Z_GET_ARG_33(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, _27, _28, \
			  _29, _30, _31, _32, ...) _32

#define Z_GET_ARG_34(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, _27, _28, \
			  _29, _30, _31, _32, _33, ...) _33

#define Z_GET_ARG_35(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, _27, _28, \
			  _29, _30, _31, _32, _33, _34, ...) _34

#define Z_GET_ARG_36(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, _27, _28, \
			  _29, _30, _31, _32, _33, _34, _35, ...) _35

#define Z_GET_ARG_37(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, _27, _28, \
			  _29, _30, _31, _32, _33, _34, _35, _36, ...) _36

#define Z_GET_ARG_38(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, _27, _28, \
			  _29, _30, _31, _32, _33, _34, _35, _36, _37, ...) _37

#define Z_GET_ARG_39(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, _27, _28, \
			  _29, _30, _31, _32, _33, _34, _35, _36, _37, \
			  _38, ...) _38

#define Z_GET_ARG_40(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, _27, _28, \
			  _29, _30, _31, _32, _33, _34, _35, _36, _37, \
			  _38, _39, ...) _39

#define Z_GET_ARG_41(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, _27, _28, \
			  _29, _30, _31, _32, _33, _34, _35, _36, _37, \
			  _38, _39, _40, ...) _40

#define Z_GET_ARG_42(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, _27, _28, \
			  _29, _30, _31, _32, _33, _34, _35, _36, _37, \
			  _38, _39, _40, _41, ...) _41

#define Z_GET_ARG_43(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, _27, _28, \
			  _29, _30, _31, _32, _33, _34, _35, _36, _37, \
			  _38, _39, _40, _41, _42, ...) _42

#define Z_GET_ARG_44(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, _27, _28, \
			  _29, _30, _31, _32, _33, _34, _35, _36, _37, \
			  _38, _39, _40, _41, _42, _43, ...) _43

#define Z_GET_ARG_45(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, _27, _28, \
			  _29, _30, _31, _32, _33, _34, _35, _36, _37, \
			  _38, _39, _40, _41, _42, _43, _44, ...) _44

#define Z_GET_ARG_46(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, _27, _28, \
			  _29, _30, _31, _32, _33, _34, _35, _36, _37, \
			  _38, _39, _40, _41, _42, _43, _44, _45, ...) _45

#define Z_GET_ARG_47(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, _27, _28, \
			  _29, _30, _31, _32, _33, _34, _35, _36, _37, \
			  _38, _39, _40, _41, _42, _43, _44, _45, _46, ...) _46

#define Z_GET_ARG_48(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, _27, _28, \
			  _29, _30, _31, _32, _33, _34, _35, _36, _37, \
			  _38, _39, _40, _41, _42, _43, _44, _45, _46, \
			  _47, ...) _47

#define Z_GET_ARG_49(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, _27, _28, \
			  _29, _30, _31, _32, _33, _34, _35, _36, _37, \
			  _38, _39, _40, _41, _42, _43, _44, _45, _46, \
			  _47, _48, ...) _48

#define Z_GET_ARG_50(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, _27, _28, \
			  _29, _30, _31, _32, _33, _34, _35, _36, _37, \
			  _38, _39, _40, _41, _42, _43, _44, _45, _46, \
			  _47, _48, _49, ...) _49

#define Z_GET_ARG_51(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, _27, _28, \
			  _29, _30, _31, _32, _33, _34, _35, _36, _37, \
			  _38, _39, _40, _41, _42, _43, _44, _45, _46, \
			  _47, _48, _49, _50, ...) _50

#define Z_GET_ARG_52(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, _27, _28, \
			  _29, _30, _31, _32, _33, _34, _35, _36, _37, \
			  _38, _39, _40, _41, _42, _43, _44, _45, _46, \
			  _47, _48, _49, _50, _51, ...) _51

#define Z_GET_ARG_53(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, _27, _28, \
			  _29, _30, _31, _32, _33, _34, _35, _36, _37, \
			  _38, _39, _40, _41, _42, _43, _44, _45, _46, \
			  _47, _48, _49, _50, _51, _52, ...) _52

#define Z_GET_ARG_54(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, _27, _28, \
			  _29, _30, _31, _32, _33, _34, _35, _36, _37, \
			  _38, _39, _40, _41, _42, _43, _44, _45, _46, \
			  _47, _48, _49, _50, _51, _52, _53, ...) _53

#define Z_GET_ARG_55(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, _27, _28, \
			  _29, _30, _31, _32, _33, _34, _35, _36, _37, \
			  _38, _39, _40, _41, _42, _43, _44, _45, _46, \
			  _47, _48, _49, _50, _51, _52, _53, _54, ...) _54

#define Z_GET_ARG_56(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, _27, _28, \
			  _29, _30, _31, _32, _33, _34, _35, _36, _37, \
			  _38, _39, _40, _41, _42, _43, _44, _45, _46, \
			  _47, _48, _49, _50, _51, _52, _53, _54, _55, ...) _55

#define Z_GET_ARG_57(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, _27, _28, \
			  _29, _30, _31, _32, _33, _34, _35, _36, _37, \
			  _38, _39, _40, _41, _42, _43, _44, _45, _46, \
			  _47, _48, _49, _50, _51, _52, _53, _54, _55, \
			  _56, ...) _56

#define Z_GET_ARG_58(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, _27, _28, \
			  _29, _30, _31, _32, _33, _34, _35, _36, _37, \
			  _38, _39, _40, _41, _42, _43, _44, _45, _46, \
			  _47, _48, _49, _50, _51, _52, _53, _54, _55, \
			  _56, _57, ...) _57

#define Z_GET_ARG_59(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, _27, _28, \
			  _29, _30, _31, _32, _33, _34, _35, _36, _37, \
			  _38, _39, _40, _41, _42, _43, _44, _45, _46, \
			  _47, _48, _49, _50, _51, _52, _53, _54, _55, \
			  _56, _57, _58, ...) _58

#define Z_GET_ARG_60(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, _27, _28, \
			  _29, _30, _31, _32, _33, _34, _35, _36, _37, \
			  _38, _39, _40, _41, _42, _43, _44, _45, _46, \
			  _47, _48, _49, _50, _51, _52, _53, _54, _55, \
			  _56, _57, _58, _59, ...) _59

#define Z_GET_ARG_61(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, _27, _28, \
			  _29, _30, _31, _32, _33, _34, _35, _36, _37, \
			  _38, _39, _40, _41, _42, _43, _44, _45, _46, \
			  _47, _48, _49, _50, _51, _52, _53, _54, _55, \
			  _56, _57, _58, _59, _60, ...) _60

#define Z_GET_ARG_62(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
			  _11, _12, _13, _14, _15, _16, _17, _18, _19, \
			  _20, _21, _22, _23, _24, _25, _26, _27, _28, \
			  _29, _30, _31, _32, _33, _34, _35, _36, _37, \
			  _38, _39, _40, _41, _42, _43, _44, _45, _46, \
			  _47, _48, _49, _50, _51, _52, _53, _54, _55, \
			  _56, _57, _58, _59, _60, _61, ...) _61

#define Z_GET_ARG_63(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, ...) _62

#define Z_GET_ARG_64(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, ...) _63

#define Z_GET_ARG_65(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, ...) _64

#define Z_GET_ARG_66(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, ...) _65

#define Z_GET_ARG_67(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, ...) _66

#define Z_GET_ARG_68(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, ...) _67

#define Z_GET_ARG_69(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, ...) _68

#define Z_GET_ARG_70(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, ...) _69

#define Z_GET_ARG_71(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, ...) _70

#define Z_GET_ARG_72(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, ...) _71

#define Z_GET_ARG_73(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, ...) _72

#define Z_GET_ARG_74(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, ...) _73

#define Z_GET_ARG_75(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, ...) _74

#define Z_GET_ARG_76(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, ...) _75

#define Z_GET_ARG_77(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, ...) _76

#define Z_GET_ARG_78(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, ...) _77

#define Z_GET_ARG_79(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, ...) _78

#define Z_GET_ARG_80(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, ...) _79

#define Z_GET_ARG_81(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, ...) _80

#define Z_GET_ARG_82(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, ...) _81

#define Z_GET_ARG_83(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, ...) _82

#define Z_GET_ARG_84(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, ...) _83

#define Z_GET_ARG_85(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, ...) _84

#define Z_GET_ARG_86(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, ...) _85

#define Z_GET_ARG_87(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, ...) _86

#define Z_GET_ARG_88(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, ...) _87

#define Z_GET_ARG_89(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, ...) _88

#define Z_GET_ARG_90(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, ...) _89

#define Z_GET_ARG_91(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, ...) _90

#define Z_GET_ARG_92(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, ...) _91

#define Z_GET_ARG_93(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, ...) _92

#define Z_GET_ARG_94(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, ...) _93

#define Z_GET_ARG_95(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, ...) _94

#define Z_GET_ARG_96(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, ...) _95

#define Z_GET_ARG_97(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, ...) _96

#define Z_GET_ARG_98(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, ...) _97

#define Z_GET_ARG_99(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, ...) _98

#define Z_GET_ARG_100(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, ...) _99

#define Z_GET_ARG_101(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, ...) _100

#define Z_GET_ARG_102(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, ...) _101

#define Z_GET_ARG_103(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, ...) _102

#define Z_GET_ARG_104(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, ...) _103

#define Z_GET_ARG_105(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, ...) _104

#define Z_GET_ARG_106(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, ...) _105

#define Z_GET_ARG_107(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, ...) _106

#define Z_GET_ARG_108(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, ...) _107

#define Z_GET_ARG_109(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, ...) _108

#define Z_GET_ARG_110(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, ...) _109

#define Z_GET_ARG_111(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, ...) _110

#define Z_GET_ARG_112(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, ...) _111

#define Z_GET_ARG_113(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, ...) _112

#define Z_GET_ARG_114(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, ...) _113

#define Z_GET_ARG_115(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, ...) _114

#define Z_GET_ARG_116(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, ...) _115

#define Z_GET_ARG_117(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, ...) _116

#define Z_GET_ARG_118(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, ...) _117

#define Z_GET_ARG_119(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, ...) _118

#define Z_GET_ARG_120(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, ...) _119

#define Z_GET_ARG_121(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, ...) _120

#define Z_GET_ARG_122(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, ...) _121

#define Z_GET_ARG_123(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, ...) _122

#define Z_GET_ARG_124(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, ...) _123

#define Z_GET_ARG_125(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, ...) _124

#define Z_GET_ARG_126(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, ...) _125

#define Z_GET_ARG_127(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, ...) _126

#define Z_GET_ARG_128(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, ...) _127

#define Z_GET_ARG_129(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, ...) _128

#define Z_GET_ARG_130(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, ...) _129

#define Z_GET_ARG_131(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, ...) _130

#define Z_GET_ARG_132(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, ...) _131

#define Z_GET_ARG_133(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, ...) _132

#define Z_GET_ARG_134(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, ...) _133

#define Z_GET_ARG_135(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, ...) _134

#define Z_GET_ARG_136(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, ...) _135

#define Z_GET_ARG_137(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, ...) _136

#define Z_GET_ARG_138(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, ...) _137

#define Z_GET_ARG_139(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, ...) _138

#define Z_GET_ARG_140(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, ...) _139

#define Z_GET_ARG_141(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, ...) _140

#define Z_GET_ARG_142(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, ...) _141

#define Z_GET_ARG_143(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, ...) _142

#define Z_GET_ARG_144(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, ...) _143

#define Z_GET_ARG_145(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, ...) _144

#define Z_GET_ARG_146(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, ...) _145

#define Z_GET_ARG_147(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, ...) _146

#define Z_GET_ARG_148(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, ...) _147

#define Z_GET_ARG_149(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, ...) _148

#define Z_GET_ARG_150(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, ...) _149

#define Z_GET_ARG_151(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, ...) _150

#define Z_GET_ARG_152(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, ...) _151

#define Z_GET_ARG_153(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, ...) _152

#define Z_GET_ARG_154(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, ...) _153

#define Z_GET_ARG_155(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, ...) _154

#define Z_GET_ARG_156(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, ...) _155

#define Z_GET_ARG_157(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, ...) _156

#define Z_GET_ARG_158(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, ...) _157

#define Z_GET_ARG_159(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, ...) _158

#define Z_GET_ARG_160(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, ...) _159

#define Z_GET_ARG_161(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, ...) _160

#define Z_GET_ARG_162(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, ...) _161

#define Z_GET_ARG_163(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, ...) _162

#define Z_GET_ARG_164(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, ...) _163

#define Z_GET_ARG_165(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, ...) _164

#define Z_GET_ARG_166(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, ...) _165

#define Z_GET_ARG_167(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, ...) _166

#define Z_GET_ARG_168(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, ...) _167

#define Z_GET_ARG_169(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, ...) _168

#define Z_GET_ARG_170(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, ...) _169

#define Z_GET_ARG_171(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, ...) _170

#define Z_GET_ARG_172(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, ...) _171

#define Z_GET_ARG_173(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, ...) _172

#define Z_GET_ARG_174(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, ...) _173

#define Z_GET_ARG_175(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, ...) _174

#define Z_GET_ARG_176(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, ...) _175

#define Z_GET_ARG_177(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, ...) _176

#define Z_GET_ARG_178(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, ...) _177

#define Z_GET_ARG_179(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, ...) _178

#define Z_GET_ARG_180(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, ...) _179

#define Z_GET_ARG_181(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, ...) _180

#define Z_GET_ARG_182(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, ...) _181

#define Z_GET_ARG_183(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, ...) _182

#define Z_GET_ARG_184(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, ...) _183

#define Z_GET_ARG_185(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, ...) _184

#define Z_GET_ARG_186(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, ...) _185

#define Z_GET_ARG_187(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, ...) _186

#define Z_GET_ARG_188(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, ...) _187

#define Z_GET_ARG_189(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, ...) _188

#define Z_GET_ARG_190(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, ...) _189

#define Z_GET_ARG_191(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, ...) _190

#define Z_GET_ARG_192(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, ...) _191

#define Z_GET_ARG_193(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, ...) _192

#define Z_GET_ARG_194(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, ...) _193

#define Z_GET_ARG_195(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, ...) _194

#define Z_GET_ARG_196(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, ...) _195

#define Z_GET_ARG_197(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, ...) _196

#define Z_GET_ARG_198(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, ...) _197

#define Z_GET_ARG_199(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, ...) _198

#define Z_GET_ARG_200(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, ...) _199

#define Z_GET_ARG_201(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, ...) _200

#define Z_GET_ARG_202(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, ...) _201

#define Z_GET_ARG_203(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, ...) _202

#define Z_GET_ARG_204(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, ...) _203

#define Z_GET_ARG_205(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, ...) _204

#define Z_GET_ARG_206(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, ...) _205

#define Z_GET_ARG_207(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, ...) _206

#define Z_GET_ARG_208(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, ...) _207

#define Z_GET_ARG_209(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, ...) _208

#define Z_GET_ARG_210(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, ...) _209

#define Z_GET_ARG_211(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, ...) _210

#define Z_GET_ARG_212(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, ...) _211

#define Z_GET_ARG_213(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, ...) _212

#define Z_GET_ARG_214(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, ...) _213

#define Z_GET_ARG_215(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, ...) _214

#define Z_GET_ARG_216(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, ...) _215

#define Z_GET_ARG_217(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, ...) _216

#define Z_GET_ARG_218(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, ...) _217

#define Z_GET_ARG_219(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, ...) _218

#define Z_GET_ARG_220(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, ...) _219

#define Z_GET_ARG_221(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, ...) _220

#define Z_GET_ARG_222(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, ...) _221

#define Z_GET_ARG_223(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, ...) _222

#define Z_GET_ARG_224(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, ...) _223

#define Z_GET_ARG_225(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, ...) _224

#define Z_GET_ARG_226(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, ...) _225

#define Z_GET_ARG_227(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, ...) _226

#define Z_GET_ARG_228(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, ...) _227

#define Z_GET_ARG_229(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, ...) _228

#define Z_GET_ARG_230(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, ...) _229

#define Z_GET_ARG_231(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, ...) _230

#define Z_GET_ARG_232(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, ...) _231

#define Z_GET_ARG_233(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, ...) _232

#define Z_GET_ARG_234(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, ...) _233

#define Z_GET_ARG_235(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, ...) _234

#define Z_GET_ARG_236(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, ...) _235

#define Z_GET_ARG_237(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, ...) _236

#define Z_GET_ARG_238(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, ...) _237

#define Z_GET_ARG_239(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, ...) _238

#define Z_GET_ARG_240(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, _240, ...) _239

#define Z_GET_ARG_241(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, _240, _241, ...) _240

#define Z_GET_ARG_242(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, _240, _241, _242, ...) _241

#define Z_GET_ARG_243(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, _240, _241, _242, _243, ...) _242

#define Z_GET_ARG_244(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, _240, _241, _242, _243, \
			  _244, ...) _243

#define Z_GET_ARG_245(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, _240, _241, _242, _243, \
			  _244, _245, ...) _244

#define Z_GET_ARG_246(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, _240, _241, _242, _243, \
			  _244, _245, _246, ...) _245

#define Z_GET_ARG_247(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, _240, _241, _242, _243, \
			  _244, _245, _246, _247, ...) _246

#define Z_GET_ARG_248(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, _240, _241, _242, _243, \
			  _244, _245, _246, _247, _248, ...) _247

#define Z_GET_ARG_249(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, _240, _241, _242, _243, \
			  _244, _245, _246, _247, _248, _249, ...) _248

#define Z_GET_ARG_250(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, _240, _241, _242, _243, \
			  _244, _245, _246, _247, _248, _249, _250, ...) _249

#define Z_GET_ARG_251(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, _240, _241, _242, _243, \
			  _244, _245, _246, _247, _248, _249, _250, _251, ...) _250

#define Z_GET_ARG_252(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, _240, _241, _242, _243, \
			  _244, _245, _246, _247, _248, _249, _250, _251, _252, ...) _251

#define Z_GET_ARG_253(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, _240, _241, _242, _243, \
			  _244, _245, _246, _247, _248, _249, _250, _251, _252, \
			  _253, ...) _252

#define Z_GET_ARG_254(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, _240, _241, _242, _243, \
			  _244, _245, _246, _247, _248, _249, _250, _251, _252, \
			  _253, _254, ...) _253

#define Z_GET_ARG_255(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, _240, _241, _242, _243, \
			  _244, _245, _246, _247, _248, _249, _250, _251, _252, \
			  _253, _254, _255, ...) _254

#define Z_GET_ARG_256(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, _240, _241, _242, _243, \
			  _244, _245, _246, _247, _248, _249, _250, _251, _252, \
			  _253, _254, _255, _256, ...) _255

#define Z_GET_ARGS_LESS_0(...) __VA_ARGS__

#define Z_GET_ARGS_LESS_1(_0, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_2(_0, _1, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_3(_0, _1, _2, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_4(_0, _1, _2, _3, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_5(_0, _1, _2, _3, _4, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_6(_0, _1, _2, _3, _4, _5, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_7(_0, _1, _2, _3, _4, _5, _6, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_8(_0, _1, _2, _3, _4, _5, \
				_6, _7, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_9(_0, _1, _2, _3, _4, _5, \
				_6, _7, _8, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_10(_0, _1, _2, _3, _4, _5, \
				_6, _7, _8, _9, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_11(_0, _1, _2, _3, _4, _5, \
				_6, _7, _8, _9, _10, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_12(_0, _1, _2, _3, _4, _5, _6,\
				_7, _8, _9, _10, _11, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_13(_0, _1, _2, _3, _4, _5, _6, \
				_7, _8, _9, _10, _11, _12, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_14(_0, _1, _2, _3, _4, _5, _6, \
				_7, _8, _9, _10, _11, _12, _13, \
				...) __VA_ARGS__

#define Z_GET_ARGS_LESS_15(_0, _1, _2, _3, _4, _5, _6, _7, \
				_8, _9, _10, _11, _12, _13, _14, \
				...) __VA_ARGS__

#define Z_GET_ARGS_LESS_16(_0, _1, _2, _3, _4, _5, _6, _7, \
				_8, _9, _10, _11, _12, _13, _14, _15, ...) \
				__VA_ARGS__

#define Z_GET_ARGS_LESS_17(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_18(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, ...) \
				__VA_ARGS__

#define Z_GET_ARGS_LESS_19(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, ...) \
				__VA_ARGS__

#define Z_GET_ARGS_LESS_20(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				...) __VA_ARGS__

#define Z_GET_ARGS_LESS_21(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_22(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_23(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_24(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_25(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_26(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_27(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, ...) \
				__VA_ARGS__

#define Z_GET_ARGS_LESS_28(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, _27, \
				...) __VA_ARGS__

#define Z_GET_ARGS_LESS_29(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, _27, _28, \
				...) __VA_ARGS__

#define Z_GET_ARGS_LESS_30(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, _27, _28, \
				_29, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_31(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, _27, _28, \
				_29, _30, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_32(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, _27, _28, \
				_29, _30, _31, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_33(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, _27, _28, \
				_29, _30, _31, _32, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_34(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, _27, _28, \
				_29, _30, _31, _32, _33, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_35(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, _27, _28, \
				_29, _30, _31, _32, _33, _34, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_36(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, _27, _28, \
				_29, _30, _31, _32, _33, _34, _35, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_37(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, _27, _28, \
				_29, _30, _31, _32, _33, _34, _35, _36, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_38(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, _27, _28, \
				_29, _30, _31, _32, _33, _34, _35, _36, _37, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_39(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, _27, _28, \
				_29, _30, _31, _32, _33, _34, _35, _36, _37, \
				_38, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_40(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, _27, _28, \
				_29, _30, _31, _32, _33, _34, _35, _36, _37, \
				_38, _39, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_41(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, _27, _28, \
				_29, _30, _31, _32, _33, _34, _35, _36, _37, \
				_38, _39, _40, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_42(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, _27, _28, \
				_29, _30, _31, _32, _33, _34, _35, _36, _37, \
				_38, _39, _40, _41, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_43(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, _27, _28, \
				_29, _30, _31, _32, _33, _34, _35, _36, _37, \
				_38, _39, _40, _41, _42, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_44(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, _27, _28, \
				_29, _30, _31, _32, _33, _34, _35, _36, _37, \
				_38, _39, _40, _41, _42, _43, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_45(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, _27, _28, \
				_29, _30, _31, _32, _33, _34, _35, _36, _37, \
				_38, _39, _40, _41, _42, _43, _44, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_46(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, _27, _28, \
				_29, _30, _31, _32, _33, _34, _35, _36, _37, \
				_38, _39, _40, _41, _42, _43, _44, _45, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_47(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, _27, _28, \
				_29, _30, _31, _32, _33, _34, _35, _36, _37, \
				_38, _39, _40, _41, _42, _43, _44, _45, _46, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_48(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, _27, _28, \
				_29, _30, _31, _32, _33, _34, _35, _36, _37, \
				_38, _39, _40, _41, _42, _43, _44, _45, _46, \
				_47, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_49(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, _27, _28, \
				_29, _30, _31, _32, _33, _34, _35, _36, _37, \
				_38, _39, _40, _41, _42, _43, _44, _45, _46, \
				_47, _48, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_50(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, _27, _28, \
				_29, _30, _31, _32, _33, _34, _35, _36, _37, \
				_38, _39, _40, _41, _42, _43, _44, _45, _46, \
				_47, _48, _49, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_51(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, _27, _28, \
				_29, _30, _31, _32, _33, _34, _35, _36, _37, \
				_38, _39, _40, _41, _42, _43, _44, _45, _46, \
				_47, _48, _49, _50, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_52(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, _27, _28, \
				_29, _30, _31, _32, _33, _34, _35, _36, _37, \
				_38, _39, _40, _41, _42, _43, _44, _45, _46, \
				_47, _48, _49, _50, _51, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_53(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, _27, _28, \
				_29, _30, _31, _32, _33, _34, _35, _36, _37, \
				_38, _39, _40, _41, _42, _43, _44, _45, _46, \
				_47, _48, _49, _50, _51, _52, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_54(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, _27, _28, \
				_29, _30, _31, _32, _33, _34, _35, _36, _37, \
				_38, _39, _40, _41, _42, _43, _44, _45, _46, \
				_47, _48, _49, _50, _51, _52, _53, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_55(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, _27, _28, \
				_29, _30, _31, _32, _33, _34, _35, _36, _37, \
				_38, _39, _40, _41, _42, _43, _44, _45, _46, \
				_47, _48, _49, _50, _51, _52, _53, _54, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_56(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, _27, _28, \
				_29, _30, _31, _32, _33, _34, _35, _36, _37, \
				_38, _39, _40, _41, _42, _43, _44, _45, _46, \
				_47, _48, _49, _50, _51, _52, _53, _54, _55, \
				...) __VA_ARGS__

#define Z_GET_ARGS_LESS_57(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, _27, _28, \
				_29, _30, _31, _32, _33, _34, _35, _36, _37, \
				_38, _39, _40, _41, _42, _43, _44, _45, _46, \
				_47, _48, _49, _50, _51, _52, _53, _54, _55, \
				_56, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_58(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, _27, _28, \
				_29, _30, _31, _32, _33, _34, _35, _36, _37, \
				_38, _39, _40, _41, _42, _43, _44, _45, _46, \
				_47, _48, _49, _50, _51, _52, _53, _54, _55, \
				_56, _57, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_59(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, _27, _28, \
				_29, _30, _31, _32, _33, _34, _35, _36, _37, \
				_38, _39, _40, _41, _42, _43, _44, _45, _46, \
				_47, _48, _49, _50, _51, _52, _53, _54, _55, \
				_56, _57, _58, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_60(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, _27, _28, \
				_29, _30, _31, _32, _33, _34, _35, _36, _37, \
				_38, _39, _40, _41, _42, _43, _44, _45, _46, \
				_47, _48, _49, _50, _51, _52, _53, _54, _55, \
				_56, _57, _58, _59, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_61(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, _27, _28, \
				_29, _30, _31, _32, _33, _34, _35, _36, _37, \
				_38, _39, _40, _41, _42, _43, _44, _45, _46, \
				_47, _48, _49, _50, _51, _52, _53, _54, _55, \
				_56, _57, _58, _59, _60, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_62(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, \
				_20, _21, _22, _23, _24, _25, _26, _27, _28, \
				_29, _30, _31, _32, _33, _34, _35, _36, _37, \
				_38, _39, _40, _41, _42, _43, _44, _45, _46, \
				_47, _48, _49, _50, _51, _52, _53, _54, _55, \
				_56, _57, _58, _59, _60, _61, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_63(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_64(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_65(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_66(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_67(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_68(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_69(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_70(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_71(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_72(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_73(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_74(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_75(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_76(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_77(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_78(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_79(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_80(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_81(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_82(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_83(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_84(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_85(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_86(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_87(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_88(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_89(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_90(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_91(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_92(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_93(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_94(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_95(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_96(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_97(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_98(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_99(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_100(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_101(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_102(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_103(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_104(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_105(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_106(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_107(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_108(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_109(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_110(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_111(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_112(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_113(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_114(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_115(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_116(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_117(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_118(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_119(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_120(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_121(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_122(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_123(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_124(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_125(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_126(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_127(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_128(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_129(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_130(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_131(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_132(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_133(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_134(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_135(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_136(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_137(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_138(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_139(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_140(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_141(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_142(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_143(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_144(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_145(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_146(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_147(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_148(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_149(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_150(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_151(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_152(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_153(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_154(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_155(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_156(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_157(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_158(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_159(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_160(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_161(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_162(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_163(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_164(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_165(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_166(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_167(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_168(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_169(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_170(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_171(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_172(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_173(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_174(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_175(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_176(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_177(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_178(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_179(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_180(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_181(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_182(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_183(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_184(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_185(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_186(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_187(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_188(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_189(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_190(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_191(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_192(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_193(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_194(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_195(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_196(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_197(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_198(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_199(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_200(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_201(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_202(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_203(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_204(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_205(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_206(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_207(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_208(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_209(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_210(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_211(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_212(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_213(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_214(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_215(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_216(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_217(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_218(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_219(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_220(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_221(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_222(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_223(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_224(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_225(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_226(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_227(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_228(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_229(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_230(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_231(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_232(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_233(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_234(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_235(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_236(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_237(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_238(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_239(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_240(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, _240, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_241(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, _240, _241, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_242(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, _240, _241, _242, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_243(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, _240, _241, _242, _243, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_244(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, _240, _241, _242, _243, \
			  _244, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_245(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, _240, _241, _242, _243, \
			  _244, _245, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_246(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, _240, _241, _242, _243, \
			  _244, _245, _246, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_247(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, _240, _241, _242, _243, \
			  _244, _245, _246, _247, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_248(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, _240, _241, _242, _243, \
			  _244, _245, _246, _247, _248, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_249(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, _240, _241, _242, _243, \
			  _244, _245, _246, _247, _248, _249, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_250(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, _240, _241, _242, _243, \
			  _244, _245, _246, _247, _248, _249, _250, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_251(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, _240, _241, _242, _243, \
			  _244, _245, _246, _247, _248, _249, _250, _251, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_252(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, _240, _241, _242, _243, \
			  _244, _245, _246, _247, _248, _249, _250, _251, _252, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_253(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, _240, _241, _242, _243, \
			  _244, _245, _246, _247, _248, _249, _250, _251, _252, \
			  _253, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_254(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, _240, _241, _242, _243, \
			  _244, _245, _246, _247, _248, _249, _250, _251, _252, \
			  _253, _254, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_255(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, _240, _241, _242, _243, \
			  _244, _245, _246, _247, _248, _249, _250, _251, _252, \
			  _253, _254, _255, ...) __VA_ARGS__

#define Z_GET_ARGS_LESS_256(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, \
			  _10, _11, _12, _13, _14, _15, _16, _17, _18, \
			  _19, _20, _21, _22, _23, _24, _25, _26, _27, \
			  _28, _29, _30, _31, _32, _33, _34, _35, _36, \
			  _37, _38, _39, _40, _41, _42, _43, _44, _45, \
			  _46, _47, _48, _49, _50, _51, _52, _53, _54, \
			  _55, _56, _57, _58, _59, _60, _61, _62, _63, \
			  _64, _65, _66, _67, _68, _69, _70, _71, _72, \
			  _73, _74, _75, _76, _77, _78, _79, _80, _81, \
			  _82, _83, _84, _85, _86, _87, _88, _89, _90, \
			  _91, _92, _93, _94, _95, _96, _97, _98, _99, \
			  _100, _101, _102, _103, _104, _105, _106, _107, _108, \
			  _109, _110, _111, _112, _113, _114, _115, _116, _117, \
			  _118, _119, _120, _121, _122, _123, _124, _125, _126, \
			  _127, _128, _129, _130, _131, _132, _133, _134, _135, \
			  _136, _137, _138, _139, _140, _141, _142, _143, _144, \
			  _145, _146, _147, _148, _149, _150, _151, _152, _153, \
			  _154, _155, _156, _157, _158, _159, _160, _161, _162, \
			  _163, _164, _165, _166, _167, _168, _169, _170, _171, \
			  _172, _173, _174, _175, _176, _177, _178, _179, _180, \
			  _181, _182, _183, _184, _185, _186, _187, _188, _189, \
			  _190, _191, _192, _193, _194, _195, _196, _197, _198, \
			  _199, _200, _201, _202, _203, _204, _205, _206, _207, \
			  _208, _209, _210, _211, _212, _213, _214, _215, _216, \
			  _217, _218, _219, _220, _221, _222, _223, _224, _225, \
			  _226, _227, _228, _229, _230, _231, _232, _233, _234, \
			  _235, _236, _237, _238, _239, _240, _241, _242, _243, \
			  _244, _245, _246, _247, _248, _249, _250, _251, _252, \
			  _253, _254, _255, _256, ...) __VA_ARGS__

#define Z_FOR_EACH_IDX_FIXED_ARG_EXEC(idx, x, fixed_arg0, fixed_arg1) \
	fixed_arg0(idx, x, fixed_arg1)

#define Z_FOR_EACH_IDX_FIXED_ARG(F, sep, fixed_arg, ...) \
	Z_FOR_EACH_ENGINE(Z_FOR_EACH_IDX_FIXED_ARG_EXEC, sep, \
			     F, fixed_arg, __VA_ARGS__)

#define Z_FOR_EACH_FIXED_ARG_EXEC(idx, x, fixed_arg0, fixed_arg1) \
	fixed_arg0(x, fixed_arg1)

#define Z_FOR_EACH_FIXED_ARG(F, sep, fixed_arg, ...) \
	Z_FOR_EACH_ENGINE(Z_FOR_EACH_FIXED_ARG_EXEC, sep, \
			     F, fixed_arg, __VA_ARGS__)

#define Z_FOR_EACH_IDX_EXEC(idx, x, fixed_arg0, fixed_arg1) \
	fixed_arg0(idx, x)

#define Z_FOR_EACH_IDX(F, sep, ...) \
	Z_FOR_EACH_ENGINE(Z_FOR_EACH_IDX_EXEC, sep, F, _, __VA_ARGS__)

#define Z_FOR_EACH_EXEC(idx, x, fixed_arg0, fixed_arg1) \
	fixed_arg0(x)

#define Z_FOR_EACH(F, sep, ...) \
	Z_FOR_EACH_ENGINE(Z_FOR_EACH_EXEC, sep, F, _, __VA_ARGS__)

#define Z_BYPASS(x) x

/* Set of UTIL_LISTIFY particles */
#include "util_listify.h"

#endif /* ZEPHYR_INCLUDE_SYS_UTIL_LOOPS_H_ */
