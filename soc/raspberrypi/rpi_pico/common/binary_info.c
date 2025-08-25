/*
 * Copyright (c) 2023 TOKITA Hiroshi <tokita.hiroshi@fujitsu.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>

#include <zephyr/dt-bindings/pinctrl/rpi-pico-pinctrl-common.h>

#include <soc.h>

#include <boot_stage2/config.h>
#include <pico/binary_info.h>

#include <version.h>
#ifdef HAS_APP_VERSION
#include <app_version.h>
#endif

/* Definition for simple calculations using macros */

#define EXPR_ADD_0_0 0
#define EXPR_ADD_0_1 1
#define EXPR_ADD_0_2 2
#define EXPR_ADD_0_3 3
#define EXPR_ADD_0_4 4
#define EXPR_ADD_0_5 5
#define EXPR_ADD_0_6 6
#define EXPR_ADD_0_7 7
#define EXPR_ADD_0_8 8
#define EXPR_ADD_0_9 9
#define EXPR_ADD_1_0 1
#define EXPR_ADD_1_1 2
#define EXPR_ADD_1_2 3
#define EXPR_ADD_1_3 4
#define EXPR_ADD_1_4 5
#define EXPR_ADD_1_5 6
#define EXPR_ADD_1_6 7
#define EXPR_ADD_1_7 8
#define EXPR_ADD_1_8 9
#define EXPR_ADD_1_9 10
#define EXPR_ADD_2_0 2
#define EXPR_ADD_2_1 3
#define EXPR_ADD_2_2 4
#define EXPR_ADD_2_3 5
#define EXPR_ADD_2_4 6
#define EXPR_ADD_2_5 7
#define EXPR_ADD_2_6 8
#define EXPR_ADD_2_7 9
#define EXPR_ADD_2_8 10
#define EXPR_ADD_2_9 11
#define EXPR_ADD_3_0 3
#define EXPR_ADD_3_1 4
#define EXPR_ADD_3_2 5
#define EXPR_ADD_3_3 6
#define EXPR_ADD_3_4 7
#define EXPR_ADD_3_5 8
#define EXPR_ADD_3_6 9
#define EXPR_ADD_3_7 10
#define EXPR_ADD_3_8 11
#define EXPR_ADD_3_9 12
#define EXPR_ADD_4_0 4
#define EXPR_ADD_4_1 5
#define EXPR_ADD_4_2 6
#define EXPR_ADD_4_3 7
#define EXPR_ADD_4_4 8
#define EXPR_ADD_4_5 9
#define EXPR_ADD_4_6 10
#define EXPR_ADD_4_7 11
#define EXPR_ADD_4_8 12
#define EXPR_ADD_4_9 13
#define EXPR_ADD_5_0 5
#define EXPR_ADD_5_1 6
#define EXPR_ADD_5_2 7
#define EXPR_ADD_5_3 8
#define EXPR_ADD_5_4 9
#define EXPR_ADD_5_5 10
#define EXPR_ADD_5_6 11
#define EXPR_ADD_5_7 12
#define EXPR_ADD_5_8 13
#define EXPR_ADD_5_9 14
#define EXPR_ADD_6_0 6
#define EXPR_ADD_6_1 7
#define EXPR_ADD_6_2 8
#define EXPR_ADD_6_3 9
#define EXPR_ADD_6_4 10
#define EXPR_ADD_6_5 11
#define EXPR_ADD_6_6 12
#define EXPR_ADD_6_7 13
#define EXPR_ADD_6_8 14
#define EXPR_ADD_6_9 15
#define EXPR_ADD_7_0 7
#define EXPR_ADD_7_1 8
#define EXPR_ADD_7_2 9
#define EXPR_ADD_7_3 10
#define EXPR_ADD_7_4 11
#define EXPR_ADD_7_5 12
#define EXPR_ADD_7_6 13
#define EXPR_ADD_7_7 14
#define EXPR_ADD_7_8 15
#define EXPR_ADD_7_9 16
#define EXPR_ADD_8_0 8
#define EXPR_ADD_8_1 9
#define EXPR_ADD_8_2 10
#define EXPR_ADD_8_3 11
#define EXPR_ADD_8_4 12
#define EXPR_ADD_8_5 13
#define EXPR_ADD_8_6 14
#define EXPR_ADD_8_7 15
#define EXPR_ADD_8_8 16
#define EXPR_ADD_8_9 17
#define EXPR_ADD_9_0 9
#define EXPR_ADD_9_1 10
#define EXPR_ADD_9_2 11
#define EXPR_ADD_9_3 12
#define EXPR_ADD_9_4 13
#define EXPR_ADD_9_5 14
#define EXPR_ADD_9_6 15
#define EXPR_ADD_9_7 16
#define EXPR_ADD_9_8 17
#define EXPR_ADD_9_9 18

#define EXPR_ADD(n, m) UTIL_CAT(UTIL_CAT(UTIL_CAT(EXPR_ADD_, n), _), m)
#define EXPR_INCR(n)   EXPR_ADD(n, 1)

#define EXPR_EQ_0_0 1
#define EXPR_EQ_0_1 0
#define EXPR_EQ_0_2 0
#define EXPR_EQ_0_3 0
#define EXPR_EQ_0_4 0
#define EXPR_EQ_0_5 0
#define EXPR_EQ_0_6 0
#define EXPR_EQ_0_7 0
#define EXPR_EQ_0_8 0
#define EXPR_EQ_0_9 0
#define EXPR_EQ_1_0 0
#define EXPR_EQ_1_1 1
#define EXPR_EQ_1_2 0
#define EXPR_EQ_1_3 0
#define EXPR_EQ_1_4 0
#define EXPR_EQ_1_5 0
#define EXPR_EQ_1_6 0
#define EXPR_EQ_1_7 0
#define EXPR_EQ_1_8 0
#define EXPR_EQ_1_9 0
#define EXPR_EQ_2_0 0
#define EXPR_EQ_2_1 0
#define EXPR_EQ_2_2 1
#define EXPR_EQ_2_3 0
#define EXPR_EQ_2_4 0
#define EXPR_EQ_2_5 0
#define EXPR_EQ_2_6 0
#define EXPR_EQ_2_7 0
#define EXPR_EQ_2_8 0
#define EXPR_EQ_2_9 0
#define EXPR_EQ_3_0 0
#define EXPR_EQ_3_1 0
#define EXPR_EQ_3_2 0
#define EXPR_EQ_3_3 1
#define EXPR_EQ_3_4 0
#define EXPR_EQ_3_5 0
#define EXPR_EQ_3_6 0
#define EXPR_EQ_3_7 0
#define EXPR_EQ_3_8 0
#define EXPR_EQ_3_9 0
#define EXPR_EQ_4_0 0
#define EXPR_EQ_4_1 0
#define EXPR_EQ_4_2 0
#define EXPR_EQ_4_3 0
#define EXPR_EQ_4_4 1
#define EXPR_EQ_4_5 0
#define EXPR_EQ_4_6 0
#define EXPR_EQ_4_7 0
#define EXPR_EQ_4_8 0
#define EXPR_EQ_4_9 0
#define EXPR_EQ_5_0 0
#define EXPR_EQ_5_1 0
#define EXPR_EQ_5_2 0
#define EXPR_EQ_5_3 0
#define EXPR_EQ_5_4 0
#define EXPR_EQ_5_5 1
#define EXPR_EQ_5_6 0
#define EXPR_EQ_5_7 0
#define EXPR_EQ_5_8 0
#define EXPR_EQ_5_9 0
#define EXPR_EQ_6_0 0
#define EXPR_EQ_6_1 0
#define EXPR_EQ_6_2 0
#define EXPR_EQ_6_3 0
#define EXPR_EQ_6_4 0
#define EXPR_EQ_6_5 0
#define EXPR_EQ_6_6 1
#define EXPR_EQ_6_7 0
#define EXPR_EQ_6_8 0
#define EXPR_EQ_6_9 0
#define EXPR_EQ_7_0 0
#define EXPR_EQ_7_1 0
#define EXPR_EQ_7_2 0
#define EXPR_EQ_7_3 0
#define EXPR_EQ_7_4 0
#define EXPR_EQ_7_5 0
#define EXPR_EQ_7_6 0
#define EXPR_EQ_7_7 1
#define EXPR_EQ_7_8 0
#define EXPR_EQ_7_9 0
#define EXPR_EQ_8_0 0
#define EXPR_EQ_8_1 0
#define EXPR_EQ_8_2 0
#define EXPR_EQ_8_3 0
#define EXPR_EQ_8_4 0
#define EXPR_EQ_8_5 0
#define EXPR_EQ_8_6 0
#define EXPR_EQ_8_7 0
#define EXPR_EQ_8_8 1
#define EXPR_EQ_8_9 0
#define EXPR_EQ_9_0 0
#define EXPR_EQ_9_1 0
#define EXPR_EQ_9_2 0
#define EXPR_EQ_9_3 0
#define EXPR_EQ_9_4 0
#define EXPR_EQ_9_5 0
#define EXPR_EQ_9_6 0
#define EXPR_EQ_9_7 0
#define EXPR_EQ_9_8 0
#define EXPR_EQ_9_9 1

#define EXPR_EQ(n, m) UTIL_CAT(UTIL_CAT(UTIL_CAT(EXPR_EQ_, n), _), m)

/* utils for pin encoding calcluation */

#ifdef CONFIG_SOC_RP2040
#define PIN_ENCODE(n, idx, offset) ((uint32_t)PIN_NUM(n, idx) << (2 + (idx + offset + 1) * 5))
#define MAX_PIN_ENTRY              4
#define BI_ENCODE_PINS_WITH_FUNC   __bi_encoded_pins_with_func
#else
#define PIN_ENCODE(n, idx, offset) ((uint64_t)PIN_NUM(n, idx) << ((idx + offset + 1) * 8))
#define MAX_PIN_ENTRY              6
#define BI_ENCODE_PINS_WITH_FUNC   __bi_encoded_pins_64_with_func
#endif

#define PIN_NUM(n, idx)  ((DT_PROP_BY_IDX(n, pinmux, idx) >> RP2_PIN_NUM_POS) & RP2_PIN_NUM_MASK)
#define PIN_FUNC(n, idx) (DT_PROP_BY_IDX(n, pinmux, idx) & RP2_ALT_FUNC_MASK)

/* Recurrence formula for calculating pin offsets for each group */

#define OFFSET_TO_GROUP_IDX_N(n, i, x)                                                             \
	EXPR_ADD(COND_CODE_1(DT_NODE_HAS_PROP(DT_CHILD_BY_IDX(n, i), pinmux),                      \
			     (DT_PROP_LEN(DT_CHILD_BY_IDX(n, i), pinmux)), (0)), x)

#define OFFSET_TO_GROUP_IDX_0(n) 0
#define OFFSET_TO_GROUP_IDX_1(n) OFFSET_TO_GROUP_IDX_N(n, 0, OFFSET_TO_GROUP_IDX_0(n))
#define OFFSET_TO_GROUP_IDX_2(n) OFFSET_TO_GROUP_IDX_N(n, 1, OFFSET_TO_GROUP_IDX_1(n))
#define OFFSET_TO_GROUP_IDX_3(n) OFFSET_TO_GROUP_IDX_N(n, 2, OFFSET_TO_GROUP_IDX_2(n))
#define OFFSET_TO_GROUP_IDX_4(n) OFFSET_TO_GROUP_IDX_N(n, 3, OFFSET_TO_GROUP_IDX_3(n))
#define OFFSET_TO_GROUP_IDX_5(n) OFFSET_TO_GROUP_IDX_N(n, 5, OFFSET_TO_GROUP_IDX_4(n))
#define OFFSET_TO_GROUP_IDX_6(n) OFFSET_TO_GROUP_IDX_N(n, 6, OFFSET_TO_GROUP_IDX_5(n))
#define OFFSET_TO_GROUP_IDX_7(n) OFFSET_TO_GROUP_IDX_N(n, 7, OFFSET_TO_GROUP_IDX_6(n))
#define OFFSET_TO_GROUP_IDX_8(n) OFFSET_TO_GROUP_IDX_N(n, 8, OFFSET_TO_GROUP_IDX_7(n))

#define OFFSET_TO_GROUP_IDX(n, i) UTIL_CAT(OFFSET_TO_GROUP_IDX_, i)(n)
#define COUNT_ALL_PINS(n)         OFFSET_TO_GROUP_IDX_8(n)

/* Iterate groups and subgroups */

#define EACH_PINCTRL_SUBGROUP(n, fn, sep, ...)                                                     \
	DT_FOREACH_PROP_ELEM_SEP_VARGS(n, pinmux, fn, sep, __VA_ARGS__)
#define EACH_PINCTRL_GROUP(n, sep, fn, ...)                                                        \
	DT_FOREACH_CHILD_SEP_VARGS(n, EACH_PINCTRL_SUBGROUP, sep, fn, sep, __VA_ARGS__)

/* Encode the pins in a group */

#define IS_LAST_PIN(end, idx, off) EXPR_EQ(end, EXPR_ADD(1, EXPR_ADD(idx, off)))
#define PIN_TERMINATE(n, p, i, offset, end)                                                        \
	COND_CODE_1(IS_LAST_PIN(end, i, offset), (PIN_ENCODE(n, i, EXPR_INCR(offset))), (0))
#define PIN_ENTRY(n, p, i, off) COND_CODE_1(DT_PROP_HAS_IDX(n, p, i), (PIN_ENCODE(n, i, off)), (0))
#define ENCODE_EACH_PIN(n, p, i, end)                                                              \
	PIN_ENTRY(n, p, i, OFFSET_TO_GROUP_IDX(DT_PARENT(n), DT_NODE_CHILD_IDX(n))) |              \
		PIN_TERMINATE(n, p, i, OFFSET_TO_GROUP_IDX(DT_PARENT(n), DT_NODE_CHILD_IDX(n)),    \
			      end)
#define ENCODE_GROUP_PINS(n) (EACH_PINCTRL_GROUP(n, (|), ENCODE_EACH_PIN, COUNT_ALL_PINS(n)))

/* Get group-wide pin functions */

#define EACH_PIN_FUNC(n, p, i, _) PIN_FUNC(n, i)
#define GROUP_PIN_FUNC(n)         (EACH_PINCTRL_GROUP(n, (|), EACH_PIN_FUNC))

/* Check if pin functions are all equal within a group */

#define EACH_PIN_FUNC_IS(n, p, i, func) (PIN_FUNC(n, i) == func)
#define ALL_PIN_FUNC_IS(n, pinfunc)     (EACH_PINCTRL_GROUP(n, (&&), EACH_PIN_FUNC_IS, pinfunc))

#define BI_PINS_FROM_PINCTRL_GROUP_(n)                                                             \
	COND_CODE_1(EXPR_EQ(0, COUNT_ALL_PINS(n)), (),                                             \
		    (BUILD_ASSERT(COUNT_ALL_PINS(n) <= MAX_PIN_ENTRY,                              \
				  "Too many pin in group");                                        \
		     BUILD_ASSERT(ALL_PIN_FUNC_IS(n, GROUP_PIN_FUNC(n)),                           \
				  "Group must contain only single function type");                 \
		     bi_decl(BI_ENCODE_PINS_WITH_FUNC(BI_PINS_ENCODING_MULTI |                     \
			    (GROUP_PIN_FUNC(n) << 3) |  ENCODE_GROUP_PINS(n)))))

#define BI_PINS_FROM_PINCTRL_GROUP(n, idx) BI_PINS_FROM_PINCTRL_GROUP_(DT_CHILD_BY_IDX(n, idx))

#ifdef CONFIG_RPI_PICO_BINARY_INFO_PROGRAM_NAME
#define BI_PROGRAM_NAME CONFIG_RPI_PICO_BINARY_INFO_PROGRAM_NAME
#endif

#ifdef CONFIG_RPI_PICO_BINARY_INFO_PROGRAM_DESCRIPTION
#define BI_PROGRAM_DESCRIPTION CONFIG_RPI_PICO_BINARY_INFO_PROGRAM_DESCRIPTION
#endif

#ifdef CONFIG_RPI_PICO_BINARY_INFO_PROGRAM_URL
#define BI_PROGRAM_URL CONFIG_RPI_PICO_BINARY_INFO_PROGRAM_URL
#endif

#ifdef CONFIG_RPI_PICO_BINARY_INFO_PROGRAM_BUILD_DATE
#define BI_PROGRAM_BUILD_DATE CONFIG_RPI_PICO_BINARY_INFO_PROGRAM_BUILD_DATE
#else
#define BI_PROGRAM_BUILD_DATE __DATE__
#endif

extern uint32_t __rom_region_end;
bi_decl(bi_binary_end((intptr_t)&__rom_region_end));

#ifdef CONFIG_RPI_PICO_BINARY_INFO_PROGRAM_NAME_ENABLE
bi_decl(bi_program_name((uint32_t)BI_PROGRAM_NAME));
#endif

#ifdef CONFIG_RPI_PICO_BINARY_INFO_PICO_BOARD_ENABLE
bi_decl(bi_string(BINARY_INFO_TAG_RASPBERRY_PI, BINARY_INFO_ID_RP_PICO_BOARD,
		  (uint32_t)CONFIG_BOARD));
#endif

#ifdef CONFIG_RPI_PICO_BINARY_INFO_SDK_VERSION_STRING_ENABLE
bi_decl(bi_string(BINARY_INFO_TAG_RASPBERRY_PI, BINARY_INFO_ID_RP_SDK_VERSION,
		  (uint32_t)"zephyr-" STRINGIFY(BUILD_VERSION)));
#endif

#if defined(CONFIG_RPI_PICO_BINARY_INFO_PROGRAM_VERSION_STRING_ENABLE) && defined(HAS_APP_VERSION)
bi_decl(bi_program_version_string((uint32_t)APP_VERSION_EXTENDED_STRING));
#endif

#ifdef CONFIG_RPI_PICO_BINARY_INFO_PROGRAM_DESCRIPTION_ENABLE
bi_decl(bi_program_description((uint32_t)BI_PROGRAM_DESCRIPTION));
#endif

#ifdef CONFIG_RPI_PICO_BINARY_INFO_PROGRAM_URL_ENABLE
bi_decl(bi_program_url((uint32_t)BI_PROGRAM_URL));
#endif

#ifdef CONFIG_RPI_PICO_BINARY_INFO_PROGRAM_BUILD_DATE_ENABLE
bi_decl(bi_program_build_date_string((uint32_t)BI_PROGRAM_BUILD_DATE));
#endif

#ifdef CONFIG_RPI_PICO_BINARY_INFO_BOOT_STAGE2_NAME_ENABLE
bi_decl(bi_string(BINARY_INFO_TAG_RASPBERRY_PI, BINARY_INFO_ID_RP_BOOT2_NAME,
		  (uint32_t)PICO_BOOT_STAGE2_NAME));
#endif

#ifdef CONFIG_RPI_PICO_BINARY_INFO_ATTRIBUTE_BUILD_TYPE_ENABLE
#ifdef CONFIG_DEBUG
bi_decl(bi_program_build_attribute((uint32_t)"Debug"));
#else
bi_decl(bi_program_build_attribute((uint32_t)"Release"));
#endif
#endif

#ifdef CONFIG_RPI_PICO_BINARY_INFO_PINS_WITH_FUNC_ENABLE
BI_PINS_FROM_PINCTRL_GROUP(DT_NODELABEL(pinctrl), 0);
BI_PINS_FROM_PINCTRL_GROUP(DT_NODELABEL(pinctrl), 1);
BI_PINS_FROM_PINCTRL_GROUP(DT_NODELABEL(pinctrl), 2);
BI_PINS_FROM_PINCTRL_GROUP(DT_NODELABEL(pinctrl), 3);
BI_PINS_FROM_PINCTRL_GROUP(DT_NODELABEL(pinctrl), 4);
BI_PINS_FROM_PINCTRL_GROUP(DT_NODELABEL(pinctrl), 5);
BI_PINS_FROM_PINCTRL_GROUP(DT_NODELABEL(pinctrl), 6);
BI_PINS_FROM_PINCTRL_GROUP(DT_NODELABEL(pinctrl), 7);
BI_PINS_FROM_PINCTRL_GROUP(DT_NODELABEL(pinctrl), 8);
BI_PINS_FROM_PINCTRL_GROUP(DT_NODELABEL(pinctrl), 9);
BI_PINS_FROM_PINCTRL_GROUP(DT_NODELABEL(pinctrl), 10);
BI_PINS_FROM_PINCTRL_GROUP(DT_NODELABEL(pinctrl), 11);
BI_PINS_FROM_PINCTRL_GROUP(DT_NODELABEL(pinctrl), 12);
BI_PINS_FROM_PINCTRL_GROUP(DT_NODELABEL(pinctrl), 13);
BI_PINS_FROM_PINCTRL_GROUP(DT_NODELABEL(pinctrl), 14);
BI_PINS_FROM_PINCTRL_GROUP(DT_NODELABEL(pinctrl), 15);
BI_PINS_FROM_PINCTRL_GROUP(DT_NODELABEL(pinctrl), 16);
BI_PINS_FROM_PINCTRL_GROUP(DT_NODELABEL(pinctrl), 17);
BI_PINS_FROM_PINCTRL_GROUP(DT_NODELABEL(pinctrl), 18);
BI_PINS_FROM_PINCTRL_GROUP(DT_NODELABEL(pinctrl), 19);
BI_PINS_FROM_PINCTRL_GROUP(DT_NODELABEL(pinctrl), 20);
BI_PINS_FROM_PINCTRL_GROUP(DT_NODELABEL(pinctrl), 21);
BI_PINS_FROM_PINCTRL_GROUP(DT_NODELABEL(pinctrl), 22);
BI_PINS_FROM_PINCTRL_GROUP(DT_NODELABEL(pinctrl), 23);
BI_PINS_FROM_PINCTRL_GROUP(DT_NODELABEL(pinctrl), 24);
BI_PINS_FROM_PINCTRL_GROUP(DT_NODELABEL(pinctrl), 25);
BI_PINS_FROM_PINCTRL_GROUP(DT_NODELABEL(pinctrl), 26);
BI_PINS_FROM_PINCTRL_GROUP(DT_NODELABEL(pinctrl), 27);
BI_PINS_FROM_PINCTRL_GROUP(DT_NODELABEL(pinctrl), 28);
BI_PINS_FROM_PINCTRL_GROUP(DT_NODELABEL(pinctrl), 29);
BI_PINS_FROM_PINCTRL_GROUP(DT_NODELABEL(pinctrl), 30);
BI_PINS_FROM_PINCTRL_GROUP(DT_NODELABEL(pinctrl), 31);
#endif
