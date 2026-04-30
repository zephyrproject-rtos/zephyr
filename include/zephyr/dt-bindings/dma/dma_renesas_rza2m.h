/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_DMA_RENESAS_RZA2M_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_DMA_RENESAS_RZA2M_H_

/* DMA request sources for RZA2M */

/** Memory-to-memory transfer (software triggered) */
#define RZA2M_DMA_MEM_2_MEM 0

/** OSTM0 timer interrupt */
#define RZA2M_DMA_RS_OSTM0TINT 1
/** OSTM1 timer interrupt */
#define RZA2M_DMA_RS_OSTM1TINT 2
/** OSTM2 timer interrupt */
#define RZA2M_DMA_RS_OSTM2TINT 3

/** MTU channel 0, compare match/input capture A */
#define RZA2M_DMA_RS_TGIA_0 4
/** MTU channel 0, compare match/input capture B */
#define RZA2M_DMA_RS_TGIB_0 5
/** MTU channel 0, compare match/input capture C */
#define RZA2M_DMA_RS_TGIC_0 6
/** MTU channel 0, compare match/input capture D */
#define RZA2M_DMA_RS_TGID_0 7

/** MTU channel 1, compare match/input capture A */
#define RZA2M_DMA_RS_TGIA_1 8
/** MTU channel 1, compare match/input capture B */
#define RZA2M_DMA_RS_TGIB_1 9

/** MTU channel 2, compare match/input capture A */
#define RZA2M_DMA_RS_TGIA_2 10
/** MTU channel 2, compare match/input capture B */
#define RZA2M_DMA_RS_TGIB_2 11

/** MTU channel 3, compare match/input capture A */
#define RZA2M_DMA_RS_TGIA_3 12
/** MTU channel 3, compare match/input capture B */
#define RZA2M_DMA_RS_TGIB_3 13
/** MTU channel 3, compare match/input capture C */
#define RZA2M_DMA_RS_TGIC_3 14
/** MTU channel 3, compare match/input capture D */
#define RZA2M_DMA_RS_TGID_3 15

/** MTU channel 4, compare match/input capture A */
#define RZA2M_DMA_RS_TGIA_4 16
/** MTU channel 4, compare match/input capture B */
#define RZA2M_DMA_RS_TGIB_4 17
/** MTU channel 4, compare match/input capture C */
#define RZA2M_DMA_RS_TGIC_4 18
/** MTU channel 4, compare match/input capture D */
#define RZA2M_DMA_RS_TGID_4 19
/** MTU channel 4, overflow/underflow interrupt of TCNT */
#define RZA2M_DMA_RS_TCIV_4 20

/** MTU channel 5, compare match/input capture U */
#define RZA2M_DMA_RS_TGIU_5 21
/** MTU channel 5, compare match/input capture V */
#define RZA2M_DMA_RS_TGIV_5 22
/** MTU channel 5, compare match/input capture W */
#define RZA2M_DMA_RS_TGIW_5 23

/** MTU channel 6, compare match/input capture A */
#define RZA2M_DMA_RS_TGIA_6 24
/** MTU channel 6, compare match/input capture B */
#define RZA2M_DMA_RS_TGIB_6 25
/** MTU channel 6, compare match/input capture C */
#define RZA2M_DMA_RS_TGIC_6 26
/** MTU channel 6, compare match/input capture D */
#define RZA2M_DMA_RS_TGID_6 27

/** MTU channel 7, compare match/input capture A */
#define RZA2M_DMA_RS_TGIA_7 28
/** MTU channel 7, compare match/input capture B */
#define RZA2M_DMA_RS_TGIB_7 29
/** MTU channel 7, compare match/input capture C */
#define RZA2M_DMA_RS_TGIC_7 30
/** MTU channel 7, compare match/input capture D */
#define RZA2M_DMA_RS_TGID_7 31
/** MTU channel 7, overflow/underflow interrupt of TCNT */
#define RZA2M_DMA_RS_TCIV_7 32

/** MTU channel 8, compare match/input capture A */
#define RZA2M_DMA_RS_TGIA_8 33
/** MTU channel 8, compare match/input capture B */
#define RZA2M_DMA_RS_TGIB_8 34
/** MTU channel 8, compare match/input capture C */
#define RZA2M_DMA_RS_TGIC_8 35
/** MTU channel 8, compare match/input capture D */
#define RZA2M_DMA_RS_TGID_8 36

/** GPT channel 0, compare match/input capture A */
#define RZA2M_DMA_RS_CCMPA_0  37
/** GPT channel 0, compare match/input capture B */
#define RZA2M_DMA_RS_CCMPB_0  38
/** GPT channel 0, compare match C */
#define RZA2M_DMA_RS_CMPC_0   39
/** GPT channel 0, compare match D */
#define RZA2M_DMA_RS_CMPD_0   40
/** GPT channel 0, compare match E */
#define RZA2M_DMA_RS_CMPE_0   41
/** GPT channel 0, compare match F */
#define RZA2M_DMA_RS_CMPF_0   42
/** GPT channel 0, A/D conversion start A */
#define RZA2M_DMA_RS_ADTRGA_0 43
/** GPT channel 0, A/D conversion start B */
#define RZA2M_DMA_RS_ADTRGB_0 44
/** GPT channel 0, overflow */
#define RZA2M_DMA_RS_OVF_0    45
/** GPT channel 0, underflow */
#define RZA2M_DMA_RS_UNF_0    46

/** GPT channel 1, compare match/input capture A */
#define RZA2M_DMA_RS_CCMPA_1  47
/** GPT channel 1, compare match/input capture B */
#define RZA2M_DMA_RS_CCMPB_1  48
/** GPT channel 1, compare match C */
#define RZA2M_DMA_RS_CMPC_1   49
/** GPT channel 1, compare match D */
#define RZA2M_DMA_RS_CMPD_1   50
/** GPT channel 1, compare match E */
#define RZA2M_DMA_RS_CMPE_1   51
/** GPT channel 1, compare match F */
#define RZA2M_DMA_RS_CMPF_1   52
/** GPT channel 1, A/D conversion start A */
#define RZA2M_DMA_RS_ADTRGA_1 53
/** GPT channel 1, A/D conversion start B */
#define RZA2M_DMA_RS_ADTRGB_1 54
/** GPT channel 1, overflow */
#define RZA2M_DMA_RS_OVF_1    55
/** GPT channel 1, underflow */
#define RZA2M_DMA_RS_UNF_1    56

/** GPT channel 2, compare match/input capture A */
#define RZA2M_DMA_RS_CCMPA_2  57
/** GPT channel 2, compare match/input capture B */
#define RZA2M_DMA_RS_CCMPB_2  58
/** GPT channel 2, compare match C */
#define RZA2M_DMA_RS_CMPC_2   59
/** GPT channel 2, compare match D */
#define RZA2M_DMA_RS_CMPD_2   60
/** GPT channel 2, compare match E */
#define RZA2M_DMA_RS_CMPE_2   61
/** GPT channel 2, compare match F */
#define RZA2M_DMA_RS_CMPF_2   62
/** GPT channel 2, A/D conversion start A */
#define RZA2M_DMA_RS_ADTRGA_2 63
/** GPT channel 2, A/D conversion start B */
#define RZA2M_DMA_RS_ADTRGB_2 64
/** GPT channel 2, overflow */
#define RZA2M_DMA_RS_OVF_2    65
/** GPT channel 2, underflow */
#define RZA2M_DMA_RS_UNF_2    66

/** GPT channel 3, compare match/input capture A */
#define RZA2M_DMA_RS_CCMPA_3  67
/** GPT channel 3, compare match/input capture B */
#define RZA2M_DMA_RS_CCMPB_3  68
/** GPT channel 3, compare match C */
#define RZA2M_DMA_RS_CMPC_3   69
/** GPT channel 3, compare match D */
#define RZA2M_DMA_RS_CMPD_3   70
/** GPT channel 3, compare match E */
#define RZA2M_DMA_RS_CMPE_3   71
/** GPT channel 3, compare match F */
#define RZA2M_DMA_RS_CMPF_3   72
/** GPT channel 3, A/D conversion start A */
#define RZA2M_DMA_RS_ADTRGA_3 73
/** GPT channel 3, A/D conversion start B */
#define RZA2M_DMA_RS_ADTRGB_3 74
/** GPT channel 3, overflow */
#define RZA2M_DMA_RS_OVF_3    75
/** GPT channel 3, underflow */
#define RZA2M_DMA_RS_UNF_3    76

/** GPT channel 4, compare match/input capture A */
#define RZA2M_DMA_RS_CCMPA_4  77
/** GPT channel 4, compare match/input capture B */
#define RZA2M_DMA_RS_CCMPB_4  78
/** GPT channel 4, compare match C */
#define RZA2M_DMA_RS_CMPC_4   79
/** GPT channel 4, compare match D */
#define RZA2M_DMA_RS_CMPD_4   80
/** GPT channel 4, compare match E */
#define RZA2M_DMA_RS_CMPE_4   81
/** GPT channel 4, compare match F */
#define RZA2M_DMA_RS_CMPF_4   82
/** GPT channel 4, A/D conversion start A */
#define RZA2M_DMA_RS_ADTRGA_4 83
/** GPT channel 4, A/D conversion start B */
#define RZA2M_DMA_RS_ADTRGB_4 84
/** GPT channel 4, overflow */
#define RZA2M_DMA_RS_OVF_4    85
/** GPT channel 4, underflow */
#define RZA2M_DMA_RS_UNF_4    86

/** GPT channel 5, compare match/input capture A */
#define RZA2M_DMA_RS_CCMPA_5  87
/** GPT channel 5, compare match/input capture B */
#define RZA2M_DMA_RS_CCMPB_5  88
/** GPT channel 5, compare match C */
#define RZA2M_DMA_RS_CMPC_5   89
/** GPT channel 5, compare match D */
#define RZA2M_DMA_RS_CMPD_5   90
/** GPT channel 5, compare match E */
#define RZA2M_DMA_RS_CMPE_5   91
/** GPT channel 5, compare match F */
#define RZA2M_DMA_RS_CMPF_5   92
/** GPT channel 5, A/D conversion start A */
#define RZA2M_DMA_RS_ADTRGA_5 93
/** GPT channel 5, A/D conversion start B */
#define RZA2M_DMA_RS_ADTRGB_5 94
/** GPT channel 5, overflow */
#define RZA2M_DMA_RS_OVF_5    95
/** GPT channel 5, underflow */
#define RZA2M_DMA_RS_UNF_5    96

/** GPT channel 6, compare match/input capture A */
#define RZA2M_DMA_RS_CCMPA_6  97
/** GPT channel 6, compare match/input capture B */
#define RZA2M_DMA_RS_CCMPB_6  98
/** GPT channel 6, compare match C */
#define RZA2M_DMA_RS_CMPC_6   99
/** GPT channel 6, compare match D */
#define RZA2M_DMA_RS_CMPD_6   100
/** GPT channel 6, compare match E */
#define RZA2M_DMA_RS_CMPE_6   101
/** GPT channel 6, compare match F */
#define RZA2M_DMA_RS_CMPF_6   102
/** GPT channel 6, A/D conversion start A */
#define RZA2M_DMA_RS_ADTRGA_6 103
/** GPT channel 6, A/D conversion start B */
#define RZA2M_DMA_RS_ADTRGB_6 104
/** GPT channel 6, overflow */
#define RZA2M_DMA_RS_OVF_6    105
/** GPT channel 6, underflow */
#define RZA2M_DMA_RS_UNF_6    106

/** GPT channel 7, compare match/input capture A */
#define RZA2M_DMA_RS_CCMPA_7  107
/** GPT channel 7, compare match/input capture B */
#define RZA2M_DMA_RS_CCMPB_7  108
/** GPT channel 7, compare match C */
#define RZA2M_DMA_RS_CMPC_7   109
/** GPT channel 7, compare match D */
#define RZA2M_DMA_RS_CMPD_7   110
/** GPT channel 7, compare match E */
#define RZA2M_DMA_RS_CMPE_7   111
/** GPT channel 7, compare match F */
#define RZA2M_DMA_RS_CMPF_7   112
/** GPT channel 7, A/D conversion start A */
#define RZA2M_DMA_RS_ADTRGA_7 113
/** GPT channel 7, A/D conversion start B */
#define RZA2M_DMA_RS_ADTRGB_7 114
/** GPT channel 7, overflow */
#define RZA2M_DMA_RS_OVF_7    115
/** GPT channel 7, underflow */
#define RZA2M_DMA_RS_UNF_7    116

/** ADC scan end interrupt */
#define RZA2M_DMA_RS_S12ADI_0   117
/** ADC group B scan end interrupt */
#define RZA2M_DMA_RS_S12GBADI_0 118
/** ADC group C scan end interrupt */
#define RZA2M_DMA_RS_S12GCADI_0 119

/** SSIF channel 0, DMA receive */
#define RZA2M_DMA_RS_INT_SSIF_DMA_RX_0 120
/** SSIF channel 0, DMA transmit */
#define RZA2M_DMA_RS_INT_SSIF_DMA_TX_0 121
/** SSIF channel 1, DMA receive */
#define RZA2M_DMA_RS_INT_SSIF_DMA_RX_1 122
/** SSIF channel 1, DMA transmit */
#define RZA2M_DMA_RS_INT_SSIF_DMA_TX_1 123
/** SSIF channel 2, DMA receive */
#define RZA2M_DMA_RS_INT_SSIF_DMA_RX_2 124
/** SSIF channel 2, DMA transmit */
#define RZA2M_DMA_RS_INT_SSIF_DMA_TX_2 125
/** SSIF channel 3, DMA receive */
#define RZA2M_DMA_RS_INT_SSIF_DMA_RX_3 126
/** SSIF channel 3, DMA transmit */
#define RZA2M_DMA_RS_INT_SSIF_DMA_TX_3 127

/** SPDIF transmit interrupt */
#define RZA2M_DMA_RS_SPDIFTXI 128
/** SPDIF receive interrupt */
#define RZA2M_DMA_RS_SPDIFRXI 129

/** RIIC channel 0, receive interrupt */
#define RZA2M_DMA_RS_INTRIIC_RI0 130
/** RIIC channel 0, transmit interrupt */
#define RZA2M_DMA_RS_INTRIIC_TI0 131
/** RIIC channel 1, receive interrupt */
#define RZA2M_DMA_RS_INTRIIC_RI1 132
/** RIIC channel 1, transmit interrupt */
#define RZA2M_DMA_RS_INTRIIC_TI1 133
/** RIIC channel 2, receive interrupt */
#define RZA2M_DMA_RS_INTRIIC_RI2 134
/** RIIC channel 2, transmit interrupt */
#define RZA2M_DMA_RS_INTRIIC_TI2 135
/** RIIC channel 3, receive interrupt */
#define RZA2M_DMA_RS_INTRIIC_RI3 136
/** RIIC channel 3, transmit interrupt */
#define RZA2M_DMA_RS_INTRIIC_TI3 137

/** SCIF channel 0, receive interrupt */
#define RZA2M_DMA_RS_RXI0 138
/** SCIF channel 0, transmit interrupt */
#define RZA2M_DMA_RS_TXI0 139
/** SCIF channel 1, receive interrupt */
#define RZA2M_DMA_RS_RXI1 140
/** SCIF channel 1, transmit interrupt */
#define RZA2M_DMA_RS_TXI1 141
/** SCIF channel 2, receive interrupt */
#define RZA2M_DMA_RS_RXI2 142
/** SCIF channel 2, transmit interrupt */
#define RZA2M_DMA_RS_TXI2 143
/** SCIF channel 3, receive interrupt */
#define RZA2M_DMA_RS_RXI3 144
/** SCIF channel 3, transmit interrupt */
#define RZA2M_DMA_RS_TXI3 145
/** SCIF channel 4, receive interrupt */
#define RZA2M_DMA_RS_RXI4 146
/** SCIF channel 4, transmit interrupt */
#define RZA2M_DMA_RS_TXI4 147

/** CAN, receive FIFO DMA channel 0 */
#define RZA2M_DMA_RS_RXF_DMA0 148
/** CAN, receive FIFO DMA channel 1 */
#define RZA2M_DMA_RS_RXF_DMA1 149
/** CAN, receive FIFO DMA channel 2 */
#define RZA2M_DMA_RS_RXF_DMA2 150
/** CAN, receive FIFO DMA channel 3 */
#define RZA2M_DMA_RS_RXF_DMA3 151
/** CAN, receive FIFO DMA channel 4 */
#define RZA2M_DMA_RS_RXF_DMA4 152
/** CAN, receive FIFO DMA channel 5 */
#define RZA2M_DMA_RS_RXF_DMA5 153
/** CAN, receive FIFO DMA channel 6 */
#define RZA2M_DMA_RS_RXF_DMA6 154
/** CAN, receive FIFO DMA channel 7 */
#define RZA2M_DMA_RS_RXF_DMA7 155
/** CAN, common FIFO DMA channel 0 */
#define RZA2M_DMA_RS_COM_DMA0 156
/** CAN, common FIFO DMA channel 1 */
#define RZA2M_DMA_RS_COM_DMA1 157

/** RSPI channel 0, receive interrupt */
#define RZA2M_DMA_RS_SPRI0 158
/** RSPI channel 0, transmit interrupt */
#define RZA2M_DMA_RS_SPTI0 159
/** RSPI channel 1, receive interrupt */
#define RZA2M_DMA_RS_SPRI1 160
/** RSPI channel 1, transmit interrupt */
#define RZA2M_DMA_RS_SPTI1 161
/** RSPI channel 2, receive interrupt */
#define RZA2M_DMA_RS_SPRI2 162
/** RSPI channel 2, transmit interrupt */
#define RZA2M_DMA_RS_SPTI2 163

/** SCI channel 0, receive interrupt */
#define RZA2M_DMA_RS_SCI_RXI0 164
/** SCI channel 0, transmit interrupt */
#define RZA2M_DMA_RS_SCI_TXI0 165
/** SCI channel 1, receive interrupt */
#define RZA2M_DMA_RS_SCI_RXI1 166
/** SCI channel 1, transmit interrupt */
#define RZA2M_DMA_RS_SCI_TXI1 167

/** Ethernet MAC controller IPLS */
#define RZA2M_DMA_RS_IPLS 168

/** DRP Tile 0 Programmable Almost Full */
#define RZA2M_DMA_RS_TILE_0_PAFI 169
/** DRP Tile 0 Programmable Almost Empty */
#define RZA2M_DMA_RS_TILE_0_PAEI 170
/** DRP Tile 1 Programmable Almost Full */
#define RZA2M_DMA_RS_TILE_1_PAFI 171
/** DRP Tile 1 Programmable Almost Empty */
#define RZA2M_DMA_RS_TILE_1_PAEI 172
/** DRP Tile 2 Programmable Almost Full */
#define RZA2M_DMA_RS_TILE_2_PAFI 173
/** DRP Tile 2 Programmable Almost Empty */
#define RZA2M_DMA_RS_TILE_2_PAEI 174
/** DRP Tile 3 Programmable Almost Full */
#define RZA2M_DMA_RS_TILE_3_PAFI 175
/** DRP Tile 3 Programmable Almost Empty */
#define RZA2M_DMA_RS_TILE_3_PAEI 176
/** DRP Tile 4 Programmable Almost Full */
#define RZA2M_DMA_RS_TILE_4_PAFI 177
/** DRP Tile 4 Programmable Almost Empty */
#define RZA2M_DMA_RS_TILE_4_PAEI 178
/** DRP Tile 5 Programmable Almost Full */
#define RZA2M_DMA_RS_TILE_5_PAFI 179
/** DRP Tile 5 Programmable Almost Empty */
#define RZA2M_DMA_RS_TILE_5_PAEI 180

/** External DMA request */
#define RZA2M_DMA_DREQ0 181

/* Marker indicating the last valid DMA resource entry */
#define RZA2M_DMA_LAST_RESOURCE_MARKER 182

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_DMA_RENESAS_RZA2M_H_ */
