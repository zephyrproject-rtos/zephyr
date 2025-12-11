/*
 * Copyright (c) 2020 ITE Corporation. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ITE_INTC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ITE_INTC_H_

#define IRQ_TYPE_NONE           0
#define IRQ_TYPE_EDGE_RISING    1
#define IRQ_TYPE_EDGE_FALLING   2
#define IRQ_TYPE_EDGE_BOTH      (IRQ_TYPE_EDGE_FALLING | IRQ_TYPE_EDGE_RISING)
#define IRQ_TYPE_LEVEL_HIGH     4
#define IRQ_TYPE_LEVEL_LOW      8

/* IRQ numbers of WUC */
/* Group 0 of INTC */
#define IT8XXX2_IRQ_WU20        1
#define IT8XXX2_IRQ_KBC_OBE     2
#define IT8XXX2_IRQ_SMB_D       4
#define IT8XXX2_IRQ_WKINTD      5
#define IT8XXX2_IRQ_WU23        6
/* Group 1 */
#define IT8XXX2_IRQ_SMB_A       9
#define IT8XXX2_IRQ_SMB_B       10
#define IT8XXX2_IRQ_WU26        12
#define IT8XXX2_IRQ_WKINTC      13
#define IT8XXX2_IRQ_WU25        14
/* Group 2 */
#define IT8XXX2_IRQ_SMB_C       16
#define IT8XXX2_IRQ_WU24        17
#define IT8XXX2_IRQ_WU22        21
#define IT8XXX2_IRQ_USB         23
/* Group 3 */
#define IT8XXX2_IRQ_KBC_IBF     24
#define IT8XXX2_IRQ_PMC1_IBF    25
#define IT8XXX2_IRQ_PMC2_IBF    27
#define IT8XXX2_IRQ_TIMER1      30
#define IT8XXX2_IRQ_WU21        31
/* Group 5 */
#define IT8XXX2_IRQ_WU50        40
#define IT8XXX2_IRQ_WU51        41
#define IT8XXX2_IRQ_WU52        42
#define IT8XXX2_IRQ_WU53        43
#define IT8XXX2_IRQ_WU54        44
#define IT8XXX2_IRQ_WU55        45
#define IT8XXX2_IRQ_WU56        46
#define IT8XXX2_IRQ_WU57        47
/* Group 6 */
#define IT8XXX2_IRQ_WU60        48
#define IT8XXX2_IRQ_WU61        49
#define IT8XXX2_IRQ_WU62        50
#define IT8XXX2_IRQ_WU63        51
#define IT8XXX2_IRQ_WU64        52
#define IT8XXX2_IRQ_WU65        53
#define IT8XXX2_IRQ_WU66        54
#define IT8XXX2_IRQ_WU67        55
/* Group 7 */
#define IT8XXX2_IRQ_TIMER2      58
/* Group 8 */
#define IT8XXX2_IRQ_PMC3_IBF    67
#define IT8XXX2_IRQ_PMC4_IBF    69
/* Group 9 */
#define IT8XXX2_IRQ_WU70        72
#define IT8XXX2_IRQ_WU71        73
#define IT8XXX2_IRQ_WU72        74
#define IT8XXX2_IRQ_WU73        75
#define IT8XXX2_IRQ_WU74        76
#define IT8XXX2_IRQ_WU75        77
#define IT8XXX2_IRQ_WU76        78
#define IT8XXX2_IRQ_WU77        79
/* Group 10 */
#define IT8XXX2_IRQ_TIMER8      80
#define IT8XXX2_IRQ_WU88        85
#define IT8XXX2_IRQ_WU89        86
#define IT8XXX2_IRQ_WU90        87
/* Group 11 */
#define IT8XXX2_IRQ_WU80        88
#define IT8XXX2_IRQ_WU81        89
#define IT8XXX2_IRQ_WU82        90
#define IT8XXX2_IRQ_WU83        91
#define IT8XXX2_IRQ_WU84        92
#define IT8XXX2_IRQ_WU85        93
#define IT8XXX2_IRQ_WU86        94
#define IT8XXX2_IRQ_WU87        95
/* Group 12 */
#define IT8XXX2_IRQ_WU91        96
#define IT8XXX2_IRQ_WU92        97
#define IT8XXX2_IRQ_WU93        98
#define IT8XXX2_IRQ_WU94        99
#define IT8XXX2_IRQ_WU95        100
#define IT8XXX2_IRQ_WU96        101
#define IT8XXX2_IRQ_WU97        102
#define IT8XXX2_IRQ_WU98        103
/* Group 13 */
#define IT8XXX2_IRQ_WU99        104
#define IT8XXX2_IRQ_WU100       105
#define IT8XXX2_IRQ_WU101       106
#define IT8XXX2_IRQ_WU102       107
#define IT8XXX2_IRQ_WU103       108
#define IT8XXX2_IRQ_WU104       109
#define IT8XXX2_IRQ_WU105       110
#define IT8XXX2_IRQ_WU106       111
/* Group 14 */
#define IT8XXX2_IRQ_WU107       112
#define IT8XXX2_IRQ_WU108       113
#define IT8XXX2_IRQ_WU109       114
#define IT8XXX2_IRQ_WU110       115
#define IT8XXX2_IRQ_WU111       116
#define IT8XXX2_IRQ_WU112       117
#define IT8XXX2_IRQ_WU113       118
#define IT8XXX2_IRQ_WU114       119
/* Group 15 */
#define IT8XXX2_IRQ_WU115       120
#define IT8XXX2_IRQ_WU116       121
#define IT8XXX2_IRQ_WU117       122
#define IT8XXX2_IRQ_WU118       123
#define IT8XXX2_IRQ_WU119       124
#define IT8XXX2_IRQ_WU120       125
#define IT8XXX2_IRQ_WU121       126
#define IT8XXX2_IRQ_WU122       127
/* Group 16 */
#define IT8XXX2_IRQ_WU128       128
#define IT8XXX2_IRQ_WU129       129
#define IT8XXX2_IRQ_WU130       130
#define IT8XXX2_IRQ_WU131       131
#define IT8XXX2_IRQ_WU132       132
#define IT8XXX2_IRQ_WU133       133
#define IT8XXX2_IRQ_WU134       134
#define IT8XXX2_IRQ_WU135       135
/* Group 17 */
#define IT8XXX2_IRQ_WU136       136
#define IT8XXX2_IRQ_WU137       137
#define IT8XXX2_IRQ_WU138       138
#define IT8XXX2_IRQ_WU139       139
#define IT8XXX2_IRQ_WU140       140
#define IT8XXX2_IRQ_WU141       141
#define IT8XXX2_IRQ_WU142       142
#define IT8XXX2_IRQ_WU143       143
/* Group 18 */
#define IT8XXX2_IRQ_WU123       144
#define IT8XXX2_IRQ_WU124       145
#define IT8XXX2_IRQ_WU125       146
#define IT8XXX2_IRQ_WU126       147
#define IT8XXX2_IRQ_PMC5_IBF    150
#define IT8XXX2_IRQ_V_CMP       151
/* Group 19 */
#define IT8XXX2_IRQ_SMB_E       152
#define IT8XXX2_IRQ_SMB_F       153
#define IT8XXX2_IRQ_TIMER3      155
#define IT8XXX2_IRQ_TIMER4      156
#define IT8XXX2_IRQ_TIMER5      157
#define IT8XXX2_IRQ_TIMER6      158
#define IT8XXX2_IRQ_TIMER7      159
/* Group 20 */
#define IT8XXX2_IRQ_ESPI        162
#define IT8XXX2_IRQ_ESPI_VW     163
#define IT8XXX2_IRQ_PCH_P80     164
#define IT8XXX2_IRQ_USBPD0      165
#define IT8XXX2_IRQ_USBPD1      166
/* Group 21 */
#define IT8XXX2_IRQ_USBPD2      174
/* Group 22 */
#define IT8XXX2_IRQ_WU40        176
#define IT8XXX2_IRQ_WU45        177
#define IT8XXX2_IRQ_WU46        178
#define IT8XXX2_IRQ_WU144       179
#define IT8XXX2_IRQ_WU145       180
#define IT8XXX2_IRQ_WU146       181
#define IT8XXX2_IRQ_WU147       182
#define IT8XXX2_IRQ_WU148       183
/* Group 23 */
#define IT8XXX2_IRQ_WU149       184
#define IT8XXX2_IRQ_WU150       185
/* Group 27 */
#define IT8XXX2_IRQ_WU184       216
#define IT8XXX2_IRQ_WU185       217
#define IT8XXX2_IRQ_WU186       218
#define IT8XXX2_IRQ_WU188       220

#define IT8XXX2_IRQ_COUNT       (CONFIG_NUM_IRQS + 1)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ITE_INTC_H_ */
