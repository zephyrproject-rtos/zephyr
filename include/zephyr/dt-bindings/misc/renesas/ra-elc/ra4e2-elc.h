/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_MISC_RENESAS_RA_ELC_RA4E2_ELC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_MISC_RENESAS_RA_ELC_RA4E2_ELC_H_

/* Sources of event signals to be linked to other peripherals or the CPU */
#define RA_ELC_EVENT_NONE                   0x0
#define RA_ELC_EVENT_ICU_IRQ0               0x001
#define RA_ELC_EVENT_ICU_IRQ1               0x002
#define RA_ELC_EVENT_ICU_IRQ2               0x003
#define RA_ELC_EVENT_ICU_IRQ3               0x004
#define RA_ELC_EVENT_ICU_IRQ4               0x005
#define RA_ELC_EVENT_ICU_IRQ5               0x006
#define RA_ELC_EVENT_ICU_IRQ6               0x007
#define RA_ELC_EVENT_ICU_IRQ7               0x008
#define RA_ELC_EVENT_ICU_IRQ8               0x009
#define RA_ELC_EVENT_ICU_IRQ9               0x00A
#define RA_ELC_EVENT_ICU_IRQ10              0x00B
#define RA_ELC_EVENT_ICU_IRQ11              0x00C
#define RA_ELC_EVENT_ICU_IRQ12              0x00D
#define RA_ELC_EVENT_ICU_IRQ13              0x00E
#define RA_ELC_EVENT_ICU_IRQ14              0x00F
#define RA_ELC_EVENT_DMAC0_INT              0x020
#define RA_ELC_EVENT_DMAC1_INT              0x021
#define RA_ELC_EVENT_DMAC2_INT              0x022
#define RA_ELC_EVENT_DMAC3_INT              0x023
#define RA_ELC_EVENT_DMAC4_INT              0x024
#define RA_ELC_EVENT_DMAC5_INT              0x025
#define RA_ELC_EVENT_DMAC6_INT              0x026
#define RA_ELC_EVENT_DMAC7_INT              0x027
#define RA_ELC_EVENT_DTC_COMPLETE           0x029
#define RA_ELC_EVENT_DMA_TRANSERR           0x02B
#define RA_ELC_EVENT_ICU_SNOOZE_CANCEL      0x02D
#define RA_ELC_EVENT_FCU_FIFERR             0x030
#define RA_ELC_EVENT_FCU_FRDYI              0x031
#define RA_ELC_EVENT_LVD_LVD1               0x038
#define RA_ELC_EVENT_LVD_LVD2               0x039
#define RA_ELC_EVENT_CGC_MOSC_STOP          0x03B
#define RA_ELC_EVENT_LPM_SNOOZE_REQUEST     0x03C
#define RA_ELC_EVENT_AGT0_INT               0x040
#define RA_ELC_EVENT_AGT0_COMPARE_A         0x041
#define RA_ELC_EVENT_AGT0_COMPARE_B         0x042
#define RA_ELC_EVENT_AGT1_INT               0x043
#define RA_ELC_EVENT_AGT1_COMPARE_A         0x044
#define RA_ELC_EVENT_AGT1_COMPARE_B         0x045
#define RA_ELC_EVENT_IWDT_UNDERFLOW         0x052
#define RA_ELC_EVENT_WDT_UNDERFLOW          0x053
#define RA_ELC_EVENT_RTC_ALARM              0x054
#define RA_ELC_EVENT_RTC_PERIOD             0x055
#define RA_ELC_EVENT_RTC_CARRY              0x056
#define RA_ELC_EVENT_CAN_RXF                0x059
#define RA_ELC_EVENT_CAN_GLERR              0x05A
#define RA_ELC_EVENT_CAN_DMAREQ0            0x05B
#define RA_ELC_EVENT_CAN_DMAREQ1            0x05C
#define RA_ELC_EVENT_CAN0_TX                0x063
#define RA_ELC_EVENT_CAN0_CHERR             0x064
#define RA_ELC_EVENT_CAN0_COMFRX            0x065
#define RA_ELC_EVENT_CAN0_CF_DMAREQ         0x066
#define RA_ELC_EVENT_CAN0_RXMB              0x067
#define RA_ELC_EVENT_USBFS_INT              0x06D
#define RA_ELC_EVENT_USBFS_RESUME           0x06E
#define RA_ELC_EVENT_SSI0_TXI               0x08A
#define RA_ELC_EVENT_SSI0_RXI               0x08B
#define RA_ELC_EVENT_SSI0_INT               0x08D
#define RA_ELC_EVENT_CAC_FREQUENCY_ERROR    0x09E
#define RA_ELC_EVENT_CAC_MEASUREMENT_END    0x09F
#define RA_ELC_EVENT_CAC_OVERFLOW           0x0A0
#define RA_ELC_EVENT_CEC_INTDA              0x0AB
#define RA_ELC_EVENT_CEC_INTCE              0x0AC
#define RA_ELC_EVENT_CEC_INTERR             0x0AD
#define RA_ELC_EVENT_IOPORT_EVENT_1         0x0B1
#define RA_ELC_EVENT_IOPORT_EVENT_2         0x0B2
#define RA_ELC_EVENT_IOPORT_EVENT_3         0x0B3
#define RA_ELC_EVENT_IOPORT_EVENT_4         0x0B4
#define RA_ELC_EVENT_ELC_SOFTWARE_EVENT_0   0x0B5
#define RA_ELC_EVENT_ELC_SOFTWARE_EVENT_1   0x0B6
#define RA_ELC_EVENT_POEG0_EVENT            0x0B7
#define RA_ELC_EVENT_POEG1_EVENT            0x0B8
#define RA_ELC_EVENT_POEG2_EVENT            0x0B9
#define RA_ELC_EVENT_POEG3_EVENT            0x0BA
#define RA_ELC_EVENT_GPT0_CAPTURE_COMPARE_A 0x0C0
#define RA_ELC_EVENT_GPT0_CAPTURE_COMPARE_B 0x0C1
#define RA_ELC_EVENT_GPT0_COMPARE_C         0x0C2
#define RA_ELC_EVENT_GPT0_COMPARE_D         0x0C3
#define RA_ELC_EVENT_GPT0_COMPARE_E         0x0C4
#define RA_ELC_EVENT_GPT0_COMPARE_F         0x0C5
#define RA_ELC_EVENT_GPT0_COUNTER_OVERFLOW  0x0C6
#define RA_ELC_EVENT_GPT0_COUNTER_UNDERFLOW 0x0C7
#define RA_ELC_EVENT_GPT0_PC                0x0C8
#define RA_ELC_EVENT_GPT0_AD_TRIG_A         0x0C9
#define RA_ELC_EVENT_GPT0_AD_TRIG_B         0x0CA
#define RA_ELC_EVENT_GPT1_CAPTURE_COMPARE_A 0x0CB
#define RA_ELC_EVENT_GPT1_CAPTURE_COMPARE_B 0x0CC
#define RA_ELC_EVENT_GPT1_COMPARE_C         0x0CD
#define RA_ELC_EVENT_GPT1_COMPARE_D         0x0CE
#define RA_ELC_EVENT_GPT1_COMPARE_E         0x0CF
#define RA_ELC_EVENT_GPT1_COMPARE_F         0x0D0
#define RA_ELC_EVENT_GPT1_COUNTER_OVERFLOW  0x0D1
#define RA_ELC_EVENT_GPT1_COUNTER_UNDERFLOW 0x0D2
#define RA_ELC_EVENT_GPT1_PC                0x0D3
#define RA_ELC_EVENT_GPT1_AD_TRIG_A         0x0D4
#define RA_ELC_EVENT_GPT1_AD_TRIG_B         0x0D5
#define RA_ELC_EVENT_GPT4_CAPTURE_COMPARE_A 0x0EC
#define RA_ELC_EVENT_GPT4_CAPTURE_COMPARE_B 0x0ED
#define RA_ELC_EVENT_GPT4_COMPARE_C         0x0EE
#define RA_ELC_EVENT_GPT4_COMPARE_D         0x0EF
#define RA_ELC_EVENT_GPT4_COMPARE_E         0x0F0
#define RA_ELC_EVENT_GPT4_COMPARE_F         0x0F1
#define RA_ELC_EVENT_GPT4_COUNTER_OVERFLOW  0x0F2
#define RA_ELC_EVENT_GPT4_COUNTER_UNDERFLOW 0x0F3
#define RA_ELC_EVENT_GPT4_PC                0x0F4
#define RA_ELC_EVENT_GPT4_AD_TRIG_A         0x0F5
#define RA_ELC_EVENT_GPT4_AD_TRIG_B         0x0F6
#define RA_ELC_EVENT_GPT5_CAPTURE_COMPARE_A 0x0F7
#define RA_ELC_EVENT_GPT5_CAPTURE_COMPARE_B 0x0F8
#define RA_ELC_EVENT_GPT5_COMPARE_C         0x0F9
#define RA_ELC_EVENT_GPT5_COMPARE_D         0x0FA
#define RA_ELC_EVENT_GPT5_COMPARE_E         0x0FB
#define RA_ELC_EVENT_GPT5_COMPARE_F         0x0FC
#define RA_ELC_EVENT_GPT5_COUNTER_OVERFLOW  0x0FD
#define RA_ELC_EVENT_GPT5_COUNTER_UNDERFLOW 0x0FE
#define RA_ELC_EVENT_GPT5_PC                0x0FF
#define RA_ELC_EVENT_GPT5_AD_TRIG_A         0x100
#define RA_ELC_EVENT_GPT5_AD_TRIG_B         0x101
#define RA_ELC_EVENT_OPS_UVW_EDGE           0x15C
#define RA_ELC_EVENT_ADC0_SCAN_END          0x160
#define RA_ELC_EVENT_ADC0_SCAN_END_B        0x161
#define RA_ELC_EVENT_ADC0_WINDOW_A          0x162
#define RA_ELC_EVENT_ADC0_WINDOW_B          0x163
#define RA_ELC_EVENT_ADC0_COMPARE_MATCH     0x164
#define RA_ELC_EVENT_ADC0_COMPARE_MISMATCH  0x165
#define RA_ELC_EVENT_SCI0_RXI               0x180
#define RA_ELC_EVENT_SCI0_TXI               0x181
#define RA_ELC_EVENT_SCI0_TEI               0x182
#define RA_ELC_EVENT_SCI0_ERI               0x183
#define RA_ELC_EVENT_SCI0_AM                0x184
#define RA_ELC_EVENT_SCI0_RXI_OR_ERI        0x185
#define RA_ELC_EVENT_SCI9_RXI               0x1B6
#define RA_ELC_EVENT_SCI9_TXI               0x1B7
#define RA_ELC_EVENT_SCI9_TEI               0x1B8
#define RA_ELC_EVENT_SCI9_ERI               0x1B9
#define RA_ELC_EVENT_SCI9_AM                0x1BA
#define RA_ELC_EVENT_SPI0_RXI               0x1C4
#define RA_ELC_EVENT_SPI0_TXI               0x1C5
#define RA_ELC_EVENT_SPI0_IDLE              0x1C6
#define RA_ELC_EVENT_SPI0_ERI               0x1C7
#define RA_ELC_EVENT_SPI0_TEI               0x1C8
#define RA_ELC_EVENT_SPI1_RXI               0x1C9
#define RA_ELC_EVENT_SPI1_TXI               0x1CA
#define RA_ELC_EVENT_SPI1_IDLE              0x1CB
#define RA_ELC_EVENT_SPI1_ERI               0x1CC
#define RA_ELC_EVENT_SPI1_TEI               0x1CD
#define RA_ELC_EVENT_CAN0_MRAM_ERI          0x1D0
#define RA_ELC_EVENT_DOC_INT                0x1DB
#define RA_ELC_EVENT_I3C0_RESPONSE          0x1DC
#define RA_ELC_EVENT_I3C0_COMMAND           0x1DD
#define RA_ELC_EVENT_I3C0_IBI               0x1DE
#define RA_ELC_EVENT_I3C0_RX                0x1DF
#define RA_ELC_EVENT_IICB0_RXI              0x1DF
#define RA_ELC_EVENT_I3C0_TX                0x1E0
#define RA_ELC_EVENT_IICB0_TXI              0x1E0
#define RA_ELC_EVENT_I3C0_RCV_STATUS        0x1E1
#define RA_ELC_EVENT_I3C0_HRESP             0x1E2
#define RA_ELC_EVENT_I3C0_HCMD              0x1E3
#define RA_ELC_EVENT_I3C0_HRX               0x1E4
#define RA_ELC_EVENT_I3C0_HTX               0x1E5
#define RA_ELC_EVENT_I3C0_TEND              0x1E6
#define RA_ELC_EVENT_IICB0_TEI              0x1E6
#define RA_ELC_EVENT_I3C0_EEI               0x1E7
#define RA_ELC_EVENT_IICB0_ERI              0x1E7
#define RA_ELC_EVENT_I3C0_STEV              0x1E8
#define RA_ELC_EVENT_I3C0_MREFOVF           0x1E9
#define RA_ELC_EVENT_I3C0_MREFCPT           0x1EA
#define RA_ELC_EVENT_I3C0_AMEV              0x1EB
#define RA_ELC_EVENT_I3C0_WU                0x1EC
#define RA_ELC_EVENT_TRNG_RDREQ             0x1F3

/* Possible peripherals to be linked to event signals */
#define RA_ELC_PERIPHERAL_GPT_A   0
#define RA_ELC_PERIPHERAL_GPT_B   1
#define RA_ELC_PERIPHERAL_GPT_C   2
#define RA_ELC_PERIPHERAL_GPT_D   3
#define RA_ELC_PERIPHERAL_GPT_E   4
#define RA_ELC_PERIPHERAL_GPT_F   5
#define RA_ELC_PERIPHERAL_GPT_G   6
#define RA_ELC_PERIPHERAL_GPT_H   7
#define RA_ELC_PERIPHERAL_ADC0    8
#define RA_ELC_PERIPHERAL_ADC0_B  9
#define RA_ELC_PERIPHERAL_DAC0    12
#define RA_ELC_PERIPHERAL_IOPORT1 14
#define RA_ELC_PERIPHERAL_IOPORT2 15
#define RA_ELC_PERIPHERAL_IOPORT3 16
#define RA_ELC_PERIPHERAL_IOPORT4 17
#define RA_ELC_PERIPHERAL_I3C     23

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_MISC_RENESAS_RA_ELC_RA4E2_ELC_H_ */
