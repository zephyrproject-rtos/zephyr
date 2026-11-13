/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_RH850U2B_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_RH850U2B_PINCTRL_H_

#include "pinctrl-rh850-common.h"

/* Port definition */
#define PORT_00                                                                                    \
	(0 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 0 << RH850_PORT_GROUP_POS |          \
	 0) /* IO P Group 00 */
#define PORT_01                                                                                    \
	(1 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 1 << RH850_PORT_GROUP_POS |          \
	 0) /* IO P Group 01 */
#define PORT_02                                                                                    \
	(2 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 2 << RH850_PORT_GROUP_POS |          \
	 0) /* IO P Group 02 */

#define PORT_10                                                                                    \
	(3 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 10 << RH850_PORT_GROUP_POS |         \
	 0) /* IO P Group 10 */
#define PORT_11                                                                                    \
	(4 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 11 << RH850_PORT_GROUP_POS |         \
	 0) /* IO P Group 11 */
#define PORT_12                                                                                    \
	(5 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 12 << RH850_PORT_GROUP_POS |         \
	 0) /* IO P Group 12 */
#define PORT_13                                                                                    \
	(6 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 13 << RH850_PORT_GROUP_POS |         \
	 0) /* IO P Group 13 */
#define PORT_14                                                                                    \
	(7 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 14 << RH850_PORT_GROUP_POS |         \
	 0) /* IO P Group 14 */
#define PORT_15                                                                                    \
	(8 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 15 << RH850_PORT_GROUP_POS |         \
	 0) /* IO P Group 15 */

#define PORT_20                                                                                    \
	(9 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 20 << RH850_PORT_GROUP_POS |         \
	 0) /* IO P Group 20 */
#define PORT_21                                                                                    \
	(10 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 21 << RH850_PORT_GROUP_POS |        \
	 0) /* IO P Group 21 */
#define PORT_22                                                                                    \
	(11 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 22 << RH850_PORT_GROUP_POS |        \
	 0) /* IO P Group 22 */
#define PORT_23                                                                                    \
	(12 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 23 << RH850_PORT_GROUP_POS |        \
	 0) /* IO P Group 23 */
#define PORT_24                                                                                    \
	(13 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 24 << RH850_PORT_GROUP_POS |        \
	 0) /* IO P Group 24 */
#define PORT_25                                                                                    \
	(14 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 25 << RH850_PORT_GROUP_POS |        \
	 0) /* IO P Group 25 */
#define PORT_27                                                                                    \
	(16 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 27 << RH850_PORT_GROUP_POS |        \
	 0) /* IO P Group 27 */

#define PORT_30                                                                                    \
	(17 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 30 << RH850_PORT_GROUP_POS |        \
	 0) /* IO P Group 30 */
#define PORT_31                                                                                    \
	(18 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 31 << RH850_PORT_GROUP_POS |        \
	 0) /* IO P Group 31 */
#define PORT_32                                                                                    \
	(19 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 32 << RH850_PORT_GROUP_POS |        \
	 0) /* IO P Group 32 */
#define PORT_33                                                                                    \
	(20 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 33 << RH850_PORT_GROUP_POS |        \
	 0) /* IO P Group 33 */
#define PORT_34                                                                                    \
	(21 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 34 << RH850_PORT_GROUP_POS |        \
	 0) /* IO P Group 34 */
#define PORT_37                                                                                    \
	(255 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 37 << RH850_PORT_GROUP_POS |       \
	 0) /* IO P Group 37 */
#define PORT_38                                                                                    \
	(255 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 38 << RH850_PORT_GROUP_POS |       \
	 0) /* IO P Group 38 */
#define PORT_39                                                                                    \
	(255 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 39 << RH850_PORT_GROUP_POS |       \
	 0) /* IO P Group 39 */

#define PORT_40                                                                                    \
	(255 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 40 << RH850_PORT_GROUP_POS |       \
	 0) /* IO P Group 40 */
#define PORT_41                                                                                    \
	(255 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 41 << RH850_PORT_GROUP_POS |       \
	 0) /* IO P Group 41 */
#define PORT_42                                                                                    \
	(255 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 42 << RH850_PORT_GROUP_POS |       \
	 0) /* IO P Group 42 */
#define PORT_43                                                                                    \
	(255 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 43 << RH850_PORT_GROUP_POS |       \
	 0) /* IO P Group 43 */
#define PORT_44                                                                                    \
	(255 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 44 << RH850_PORT_GROUP_POS |       \
	 0) /* IO P Group 44 */
#define PORT_45                                                                                    \
	(255 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 45 << RH850_PORT_GROUP_POS |       \
	 0) /* IO P Group 45 */
#define PORT_46                                                                                    \
	(255 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 46 << RH850_PORT_GROUP_POS |       \
	 0) /* IO P Group 46 */
#define PORT_47                                                                                    \
	(255 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 47 << RH850_PORT_GROUP_POS |       \
	 0) /* IO P Group 47 */
#define PORT_48                                                                                    \
	(255 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 48 << RH850_PORT_GROUP_POS |       \
	 0) /* IO P Group 48 */
#define PORT_49                                                                                    \
	(255 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 49 << RH850_PORT_GROUP_POS |       \
	 0) /* IO P Group 49 */

#define PORT_50                                                                                    \
	(255 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 50 << RH850_PORT_GROUP_POS |       \
	 0) /* IO P Group 50 */
#define PORT_51                                                                                    \
	(255 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 51 << RH850_PORT_GROUP_POS |       \
	 0) /* IO P Group 51 */
#define PORT_52                                                                                    \
	(255 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 52 << RH850_PORT_GROUP_POS |       \
	 0) /* IO P Group 52 */
#define PORT_53                                                                                    \
	(255 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 53 << RH850_PORT_GROUP_POS |       \
	 0) /* IO P Group 53 */
#define PORT_54                                                                                    \
	(255 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 54 << RH850_PORT_GROUP_POS |       \
	 0) /* IO P Group 54 */
#define PORT_55                                                                                    \
	(255 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 55 << RH850_PORT_GROUP_POS |       \
	 0) /* IO P Group 55 */
#define PORT_56                                                                                    \
	(255 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 56 << RH850_PORT_GROUP_POS |       \
	 0) /* IO P Group 56 */
#define PORT_57                                                                                    \
	(255 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 57 << RH850_PORT_GROUP_POS |       \
	 0) /* IO P Group 57 */
#define PORT_58                                                                                    \
	(255 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 58 << RH850_PORT_GROUP_POS |       \
	 0) /* IO P Group 58 */
#define PORT_59                                                                                    \
	(255 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 59 << RH850_PORT_GROUP_POS |       \
	 0) /* IO P Group 59 */

#define PORT_60                                                                                    \
	(255 << RH850_PORT_PWE_POS | 0 << RH850_PORT_TYPE_POS | 60 << RH850_PORT_GROUP_POS |       \
	 0) /* IO P Group 60 */

#define PORT_JP00                                                                                  \
	(255 << RH850_PORT_PWE_POS | 1 << RH850_PORT_TYPE_POS | 0 << RH850_PORT_GROUP_POS |        \
	 0) /* IO port JP00 */
#define PORT_JP01                                                                                  \
	(255 << RH850_PORT_PWE_POS | 1 << RH850_PORT_TYPE_POS | 1 << RH850_PORT_GROUP_POS |        \
	 0) /* IO port JP01 */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_RH850U2B_PINCTRL_H_ */
