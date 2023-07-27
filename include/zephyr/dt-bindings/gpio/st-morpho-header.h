/*
 * Copyright (c) 2023 Teslabs Engineering S.L.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef INCLUDE_ZEPHYR_DT_BINDINGS_GPIO_ST_MORPHO_HEADER_H_
#define INCLUDE_ZEPHYR_DT_BINDINGS_GPIO_ST_MORPHO_HEADER_H_

/** ST Morpho pin mask (0...143). */
#define ST_MORPHO_PIN_MASK 0xFF

/**
 * @name ST Morpho pin identifiers
 * @{
 */
#define ST_MORPHO_L_1	0
#define ST_MORPHO_L_2	1
#define ST_MORPHO_L_3	2
#define ST_MORPHO_L_4	3
#define ST_MORPHO_L_5	4
#define ST_MORPHO_L_6	5
#define ST_MORPHO_L_7	6
#define ST_MORPHO_L_8	7
#define ST_MORPHO_L_9	8
#define ST_MORPHO_L_10	9
#define ST_MORPHO_L_11	10
#define ST_MORPHO_L_12	11
#define ST_MORPHO_L_13	12
#define ST_MORPHO_L_14	13
#define ST_MORPHO_L_15	14
#define ST_MORPHO_L_16	15
#define ST_MORPHO_L_17	16
#define ST_MORPHO_L_18	17
#define ST_MORPHO_L_19	18
#define ST_MORPHO_L_20	19
#define ST_MORPHO_L_21	20
#define ST_MORPHO_L_22	21
#define ST_MORPHO_L_23	22
#define ST_MORPHO_L_24	23
#define ST_MORPHO_L_25	24
#define ST_MORPHO_L_26	25
#define ST_MORPHO_L_27	26
#define ST_MORPHO_L_28	27
#define ST_MORPHO_L_29	28
#define ST_MORPHO_L_30	29
#define ST_MORPHO_L_31	30
#define ST_MORPHO_L_32	31
#define ST_MORPHO_L_33	32
#define ST_MORPHO_L_34	33
#define ST_MORPHO_L_35	34
#define ST_MORPHO_L_36	35
#define ST_MORPHO_L_37	36
#define ST_MORPHO_L_38	37
#define ST_MORPHO_L_39	38
#define ST_MORPHO_L_40	39
#define ST_MORPHO_L_41	40
#define ST_MORPHO_L_42	41
#define ST_MORPHO_L_43	42
#define ST_MORPHO_L_44	43
#define ST_MORPHO_L_45	44
#define ST_MORPHO_L_46	45
#define ST_MORPHO_L_47	46
#define ST_MORPHO_L_48	47
#define ST_MORPHO_L_49	48
#define ST_MORPHO_L_50	49
#define ST_MORPHO_L_51	50
#define ST_MORPHO_L_52	51
#define ST_MORPHO_L_53	52
#define ST_MORPHO_L_54	53
#define ST_MORPHO_L_55	54
#define ST_MORPHO_L_56	55
#define ST_MORPHO_L_57	56
#define ST_MORPHO_L_58	57
#define ST_MORPHO_L_59	58
#define ST_MORPHO_L_60	59
#define ST_MORPHO_L_61	60
#define ST_MORPHO_L_62	61
#define ST_MORPHO_L_63	62
#define ST_MORPHO_L_64	63
#define ST_MORPHO_L_65	64
#define ST_MORPHO_L_66	65
#define ST_MORPHO_L_67	66
#define ST_MORPHO_L_68	67
#define ST_MORPHO_L_69	68
#define ST_MORPHO_L_70	69
#define ST_MORPHO_L_71	70
#define ST_MORPHO_L_72	71

#define ST_MORPHO_R_1	72
#define ST_MORPHO_R_2	73
#define ST_MORPHO_R_3	74
#define ST_MORPHO_R_4	75
#define ST_MORPHO_R_5	76
#define ST_MORPHO_R_6	77
#define ST_MORPHO_R_7	78
#define ST_MORPHO_R_8	79
#define ST_MORPHO_R_9	80
#define ST_MORPHO_R_10	81
#define ST_MORPHO_R_11	82
#define ST_MORPHO_R_12	83
#define ST_MORPHO_R_13	84
#define ST_MORPHO_R_14	85
#define ST_MORPHO_R_15	86
#define ST_MORPHO_R_16	87
#define ST_MORPHO_R_17	88
#define ST_MORPHO_R_18	89
#define ST_MORPHO_R_19	90
#define ST_MORPHO_R_20	91
#define ST_MORPHO_R_21	92
#define ST_MORPHO_R_22	93
#define ST_MORPHO_R_23	94
#define ST_MORPHO_R_24	95
#define ST_MORPHO_R_25	96
#define ST_MORPHO_R_26	97
#define ST_MORPHO_R_27	98
#define ST_MORPHO_R_28	99
#define ST_MORPHO_R_29	100
#define ST_MORPHO_R_30	101
#define ST_MORPHO_R_31	102
#define ST_MORPHO_R_32	103
#define ST_MORPHO_R_33	104
#define ST_MORPHO_R_34	105
#define ST_MORPHO_R_35	106
#define ST_MORPHO_R_36	107
#define ST_MORPHO_R_37	108
#define ST_MORPHO_R_38	109
#define ST_MORPHO_R_39	110
#define ST_MORPHO_R_40	111
#define ST_MORPHO_R_41	112
#define ST_MORPHO_R_42	113
#define ST_MORPHO_R_43	114
#define ST_MORPHO_R_44	115
#define ST_MORPHO_R_45	116
#define ST_MORPHO_R_46	117
#define ST_MORPHO_R_47	118
#define ST_MORPHO_R_48	119
#define ST_MORPHO_R_49	120
#define ST_MORPHO_R_50	121
#define ST_MORPHO_R_51	122
#define ST_MORPHO_R_52	123
#define ST_MORPHO_R_53	124
#define ST_MORPHO_R_54	125
#define ST_MORPHO_R_55	126
#define ST_MORPHO_R_56	127
#define ST_MORPHO_R_57	128
#define ST_MORPHO_R_58	129
#define ST_MORPHO_R_59	130
#define ST_MORPHO_R_60	131
#define ST_MORPHO_R_61	132
#define ST_MORPHO_R_62	133
#define ST_MORPHO_R_63	134
#define ST_MORPHO_R_64	135
#define ST_MORPHO_R_65	136
#define ST_MORPHO_R_66	137
#define ST_MORPHO_R_67	138
#define ST_MORPHO_R_68	139
#define ST_MORPHO_R_69	140
#define ST_MORPHO_R_70	141
#define ST_MORPHO_R_71	142
#define ST_MORPHO_R_72	143

/** @} */

#endif /* INCLUDE_ZEPHYR_DT_BINDINGS_GPIO_ST_MORPHO_HEADER_H_ */
