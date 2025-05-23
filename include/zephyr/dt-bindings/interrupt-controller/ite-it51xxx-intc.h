/*
 * Copyright (c) 2025 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ITE_INTC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ITE_INTC_H_

#define IRQ_TYPE_NONE         0
#define IRQ_TYPE_EDGE_RISING  1
#define IRQ_TYPE_EDGE_FALLING 2
#define IRQ_TYPE_EDGE_BOTH    (IRQ_TYPE_EDGE_FALLING | IRQ_TYPE_EDGE_RISING)
#define IRQ_TYPE_LEVEL_HIGH   4
#define IRQ_TYPE_LEVEL_LOW    8

/* IRQ numbers of WUC */
/* Group 0 of INTC */
#define IT51XXX_IRQ_WU20       1
#define IT51XXX_IRQ_KBC_OBE    2
#define IT51XXX_IRQ_SMB_D      4
#define IT51XXX_IRQ_WKINTD     5
#define IT51XXX_IRQ_WU23       6
/* Group 1 */
#define IT51XXX_IRQ_SMB_A      9
#define IT51XXX_IRQ_SMB_B      10
#define IT51XXX_IRQ_WU26       12
#define IT51XXX_IRQ_WKINTC     13
#define IT51XXX_IRQ_WU25       14
/* Group 2 */
#define IT51XXX_IRQ_SMB_C      16
#define IT51XXX_IRQ_WU24       17
#define IT51XXX_IRQ_WU22       21
/* Group 3 */
#define IT51XXX_IRQ_KBC_IBF    24
#define IT51XXX_IRQ_PMC1_IBF   25
#define IT51XXX_IRQ_PMC2_IBF   27
#define IT51XXX_IRQ_TIMER1     30
#define IT51XXX_IRQ_WU21       31
/* Group 5 */
#define IT51XXX_IRQ_WU50       40
#define IT51XXX_IRQ_WU51       41
#define IT51XXX_IRQ_WU52       42
#define IT51XXX_IRQ_WU53       43
#define IT51XXX_IRQ_WU54       44
#define IT51XXX_IRQ_WU55       45
#define IT51XXX_IRQ_WU56       46
#define IT51XXX_IRQ_WU57       47
/* Group 6 */
#define IT51XXX_IRQ_WU60       48
#define IT51XXX_IRQ_WU61       49
#define IT51XXX_IRQ_WU62       50
#define IT51XXX_IRQ_WU63       51
#define IT51XXX_IRQ_WU64       52
#define IT51XXX_IRQ_WU65       53
#define IT51XXX_IRQ_WU66       54
#define IT51XXX_IRQ_WU67       55
/* Group 7 */
#define IT51XXX_IRQ_TIMER2     58
/* Group 9 */
#define IT51XXX_IRQ_WU70       72
#define IT51XXX_IRQ_WU71       73
#define IT51XXX_IRQ_WU72       74
#define IT51XXX_IRQ_WU73       75
#define IT51XXX_IRQ_WU74       76
#define IT51XXX_IRQ_WU75       77
#define IT51XXX_IRQ_WU76       78
#define IT51XXX_IRQ_WU77       79
/* Group 10 */
#define IT51XXX_IRQ_WU88       85
#define IT51XXX_IRQ_WU89       86
#define IT51XXX_IRQ_WU90       87
/* Group 11 */
#define IT51XXX_IRQ_WU80       88
#define IT51XXX_IRQ_WU81       89
#define IT51XXX_IRQ_WU82       90
#define IT51XXX_IRQ_WU83       91
#define IT51XXX_IRQ_WU84       92
#define IT51XXX_IRQ_WU85       93
#define IT51XXX_IRQ_WU86       94
#define IT51XXX_IRQ_WU87       95
/* Group 12 */
#define IT51XXX_IRQ_WU91       96
#define IT51XXX_IRQ_WU92       97
#define IT51XXX_IRQ_WU93       98
#define IT51XXX_IRQ_WU95       100
#define IT51XXX_IRQ_WU96       101
#define IT51XXX_IRQ_WU97       102
#define IT51XXX_IRQ_WU98       103
/* Group 13 */
#define IT51XXX_IRQ_WU99       104
#define IT51XXX_IRQ_WU100      105
#define IT51XXX_IRQ_WU101      106
#define IT51XXX_IRQ_WU102      107
#define IT51XXX_IRQ_WU103      108
#define IT51XXX_IRQ_WU104      109
#define IT51XXX_IRQ_WU105      110
#define IT51XXX_IRQ_WU106      111
/* Group 14 */
#define IT51XXX_IRQ_WU107      112
#define IT51XXX_IRQ_WU108      113
#define IT51XXX_IRQ_WU109      114
#define IT51XXX_IRQ_WU110      115
#define IT51XXX_IRQ_WU111      116
#define IT51XXX_IRQ_WU112      117
#define IT51XXX_IRQ_WU113      118
#define IT51XXX_IRQ_WU114      119
/* Group 15 */
#define IT51XXX_IRQ_WU115      120
#define IT51XXX_IRQ_WU116      121
#define IT51XXX_IRQ_WU117      122
#define IT51XXX_IRQ_WU118      123
#define IT51XXX_IRQ_WU119      124
#define IT51XXX_IRQ_WU120      125
#define IT51XXX_IRQ_WU121      126
#define IT51XXX_IRQ_WU122      127
/* Group 16 */
#define IT51XXX_IRQ_WU128      128
#define IT51XXX_IRQ_WU129      129
#define IT51XXX_IRQ_WU131      131
#define IT51XXX_IRQ_WU132      132
#define IT51XXX_IRQ_WU133      133
#define IT51XXX_IRQ_WU134      134
#define IT51XXX_IRQ_WU135      135
/* Group 17 */
#define IT51XXX_IRQ_WU136      136
#define IT51XXX_IRQ_WU137      137
#define IT51XXX_IRQ_WU138      138
#define IT51XXX_IRQ_WU139      139
#define IT51XXX_IRQ_WU140      140
#define IT51XXX_IRQ_WU141      141
#define IT51XXX_IRQ_WU142      142
/* Group 18 */
#define IT51XXX_IRQ_WU127      148
#define IT51XXX_IRQ_V_CMP      151
/* Group 19 */
#define IT51XXX_IRQ_PECI       152
#define IT51XXX_IRQ_ESPI       153
#define IT51XXX_IRQ_ESPI_VW    154
#define IT51XXX_IRQ_PCH_P80    155
#define IT51XXX_IRQ_TIMER3     157
#define IT51XXX_IRQ_PLL_CHANGE 159
/* Group 20 */
#define IT51XXX_IRQ_SMB_E      160
#define IT51XXX_IRQ_SMB_F      161
#define IT51XXX_IRQ_WU40       163
#define IT51XXX_IRQ_WU45       166
/* Group 21 */
#define IT51XXX_IRQ_WU46       168
#define IT51XXX_IRQ_WU144      170
#define IT51XXX_IRQ_WU145      171
#define IT51XXX_IRQ_WU146      172
#define IT51XXX_IRQ_WU147      173
#define IT51XXX_IRQ_TIMER4     175
/* Group 22 */
#define IT51XXX_IRQ_WU148      176
#define IT51XXX_IRQ_WU149      177
#define IT51XXX_IRQ_WU150      178
#define IT51XXX_IRQ_WU151      179
#define IT51XXX_IRQ_I3C_M0     180
#define IT51XXX_IRQ_I3C_M1     181
#define IT51XXX_IRQ_I3C_S0     182
#define IT51XXX_IRQ_I3C_S1     183
/* Group 25 */
#define IT51XXX_IRQ_SMB_SC     203
#define IT51XXX_IRQ_SMB_SB     204
#define IT51XXX_IRQ_SMB_SA     205
#define IT51XXX_IRQ_TIMER1_DW  207
/* Group 26 */
#define IT51XXX_IRQ_TIMER2_DW  208
#define IT51XXX_IRQ_TIMER3_DW  209
#define IT51XXX_IRQ_TIMER4_DW  210
#define IT51XXX_IRQ_TIMER5_DW  211
#define IT51XXX_IRQ_TIMER6_DW  212
#define IT51XXX_IRQ_TIMER7_DW  213
#define IT51XXX_IRQ_TIMER8_DW  214
/* Group 27 */
#define IT51XXX_IRQ_PWM_TACH0  219
#define IT51XXX_IRQ_PWM_TACH1  220
#define IT51XXX_IRQ_PWM_TACH2  221
#define IT51XXX_IRQ_SMB_G      222
#define IT51XXX_IRQ_SMB_H      223
/* Group 28 */
#define IT51XXX_IRQ_SMB_I      224

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ITE_INTC_H_ */
