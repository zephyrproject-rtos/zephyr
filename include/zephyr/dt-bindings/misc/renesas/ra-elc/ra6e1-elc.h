/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Renesas RA6E1 Event Link Controller (ELC) definitions
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_MISC_RENESAS_RA_ELC_RA6E1_ELC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_MISC_RENESAS_RA_ELC_RA6E1_ELC_H_

/**
 * @name Event codes for Renesas RA6E1 Event Link Controller (ELC).
 * @{
 */
#define RA_ELC_EVENT_NONE                   0x0   /**< Link disabled. */
#define RA_ELC_EVENT_ICU_IRQ0               0x001 /**< External pin interrupt 0. */
#define RA_ELC_EVENT_ICU_IRQ1               0x002 /**< External pin interrupt 1. */
#define RA_ELC_EVENT_ICU_IRQ2               0x003 /**< External pin interrupt 2. */
#define RA_ELC_EVENT_ICU_IRQ3               0x004 /**< External pin interrupt 3. */
#define RA_ELC_EVENT_ICU_IRQ4               0x005 /**< External pin interrupt 4. */
#define RA_ELC_EVENT_ICU_IRQ5               0x006 /**< External pin interrupt 5. */
#define RA_ELC_EVENT_ICU_IRQ6               0x007 /**< External pin interrupt 6. */
#define RA_ELC_EVENT_ICU_IRQ7               0x008 /**< External pin interrupt 7. */
#define RA_ELC_EVENT_ICU_IRQ8               0x009 /**< External pin interrupt 8. */
#define RA_ELC_EVENT_ICU_IRQ9               0x00A /**< External pin interrupt 9. */
#define RA_ELC_EVENT_ICU_IRQ10              0x00B /**< External pin interrupt 10. */
#define RA_ELC_EVENT_ICU_IRQ11              0x00C /**< External pin interrupt 11. */
#define RA_ELC_EVENT_ICU_IRQ12              0x00D /**< External pin interrupt 12. */
#define RA_ELC_EVENT_ICU_IRQ13              0x00E /**< External pin interrupt 13. */
#define RA_ELC_EVENT_ICU_IRQ14              0x00F /**< External pin interrupt 14. */
#define RA_ELC_EVENT_ICU_IRQ15              0x010 /**< External pin interrupt 15. */
#define RA_ELC_EVENT_DMAC0_INT              0x020 /**< DMAC0 transfer end. */
#define RA_ELC_EVENT_DMAC1_INT              0x021 /**< DMAC1 transfer end. */
#define RA_ELC_EVENT_DMAC2_INT              0x022 /**< DMAC2 transfer end. */
#define RA_ELC_EVENT_DMAC3_INT              0x023 /**< DMAC3 transfer end. */
#define RA_ELC_EVENT_DMAC4_INT              0x024 /**< DMAC4 transfer end. */
#define RA_ELC_EVENT_DMAC5_INT              0x025 /**< DMAC5 transfer end. */
#define RA_ELC_EVENT_DMAC6_INT              0x026 /**< DMAC6 transfer end. */
#define RA_ELC_EVENT_DMAC7_INT              0x027 /**< DMAC7 transfer end. */
#define RA_ELC_EVENT_DTC_COMPLETE           0x029 /**< DTC transfer complete. */
#define RA_ELC_EVENT_DTC_END                0x02A /**< DTC transfer end. */
#define RA_ELC_EVENT_DMA_TRANSERR           0x02B /**< DMA/DTC transfer error. */
#define RA_ELC_EVENT_ICU_SNOOZE_CANCEL      0x02D /**< Canceling from Snooze mode. */
#define RA_ELC_EVENT_FCU_FIFERR             0x030 /**< Flash access error interrupt. */
#define RA_ELC_EVENT_FCU_FRDYI              0x031 /**< Flash ready interrupt. */
#define RA_ELC_EVENT_LVD_LVD1               0x038 /**< Voltage monitor 1 interrupt. */
#define RA_ELC_EVENT_LVD_LVD2               0x039 /**< Voltage monitor 2 interrupt. */
#define RA_ELC_EVENT_CGC_MOSC_STOP          0x03B /**< Main Clock oscillation stop. */
#define RA_ELC_EVENT_LPM_SNOOZE_REQUEST     0x03C /**< Snooze entry. */
#define RA_ELC_EVENT_AGT0_INT               0x040 /**< AGT interrupt. */
#define RA_ELC_EVENT_AGT0_COMPARE_A         0x041 /**< Compare match A. */
#define RA_ELC_EVENT_AGT0_COMPARE_B         0x042 /**< Compare match B. */
#define RA_ELC_EVENT_AGT1_INT               0x043 /**< AGT interrupt. */
#define RA_ELC_EVENT_AGT1_COMPARE_A         0x044 /**< Compare match A. */
#define RA_ELC_EVENT_AGT1_COMPARE_B         0x045 /**< Compare match B. */
#define RA_ELC_EVENT_AGT2_INT               0x046 /**< AGT interrupt. */
#define RA_ELC_EVENT_AGT2_COMPARE_A         0x047 /**< Compare match A. */
#define RA_ELC_EVENT_AGT2_COMPARE_B         0x048 /**< Compare match B. */
#define RA_ELC_EVENT_AGT3_INT               0x049 /**< AGT interrupt. */
#define RA_ELC_EVENT_AGT3_COMPARE_A         0x04A /**< Compare match A. */
#define RA_ELC_EVENT_AGT3_COMPARE_B         0x04B /**< Compare match B. */
#define RA_ELC_EVENT_AGT4_INT               0x04C /**< AGT interrupt. */
#define RA_ELC_EVENT_AGT4_COMPARE_A         0x04D /**< Compare match A. */
#define RA_ELC_EVENT_AGT4_COMPARE_B         0x04E /**< Compare match B. */
#define RA_ELC_EVENT_AGT5_INT               0x04F /**< AGT interrupt. */
#define RA_ELC_EVENT_AGT5_COMPARE_A         0x050 /**< Compare match A. */
#define RA_ELC_EVENT_AGT5_COMPARE_B         0x051 /**< Compare match B. */
#define RA_ELC_EVENT_IWDT_UNDERFLOW         0x052 /**< IWDT underflow. */
#define RA_ELC_EVENT_WDT_UNDERFLOW          0x053 /**< WDT underflow. */
#define RA_ELC_EVENT_RTC_ALARM              0x054 /**< Alarm interrupt. */
#define RA_ELC_EVENT_RTC_PERIOD             0x055 /**< Periodic interrupt. */
#define RA_ELC_EVENT_RTC_CARRY              0x056 /**< Carry interrupt. */
#define RA_ELC_EVENT_USBFS_FIFO_0           0x06B /**< DMA/DTC transfer request 0. */
#define RA_ELC_EVENT_USBFS_FIFO_1           0x06C /**< DMA/DTC transfer request 1. */
#define RA_ELC_EVENT_USBFS_INT              0x06D /**< USBFS interrupt. */
#define RA_ELC_EVENT_USBFS_RESUME           0x06E /**< USBFS resume interrupt. */
#define RA_ELC_EVENT_IIC0_RXI               0x073 /**< Receive data full. */
#define RA_ELC_EVENT_IIC0_TXI               0x074 /**< Transmit data empty. */
#define RA_ELC_EVENT_IIC0_TEI               0x075 /**< Transmit end. */
#define RA_ELC_EVENT_IIC0_ERI               0x076 /**< Transfer error. */
#define RA_ELC_EVENT_IIC0_WUI               0x077 /**< Wakeup interrupt. */
#define RA_ELC_EVENT_IIC1_RXI               0x078 /**< Receive data full. */
#define RA_ELC_EVENT_IIC1_TXI               0x079 /**< Transmit data empty. */
#define RA_ELC_EVENT_IIC1_TEI               0x07A /**< Transmit end. */
#define RA_ELC_EVENT_IIC1_ERI               0x07B /**< Transfer error. */
#define RA_ELC_EVENT_SDHIMMC0_ACCS          0x082 /**< Card access. */
#define RA_ELC_EVENT_SDHIMMC0_SDIO          0x083 /**< SDIO access. */
#define RA_ELC_EVENT_SDHIMMC0_CARD          0x084 /**< Card detect. */
#define RA_ELC_EVENT_SDHIMMC0_DMA_REQ       0x085 /**< DMA transfer request. */
#define RA_ELC_EVENT_SSI0_TXI               0x08A /**< Transmit data empty. */
#define RA_ELC_EVENT_SSI0_RXI               0x08B /**< Receive data full. */
#define RA_ELC_EVENT_SSI0_INT               0x08D /**< Error interrupt. */
#define RA_ELC_EVENT_CAC_FREQUENCY_ERROR    0x09E /**< Frequency error interrupt. */
#define RA_ELC_EVENT_CAC_MEASUREMENT_END    0x09F /**< Measurement end interrupt. */
#define RA_ELC_EVENT_CAC_OVERFLOW           0x0A0 /**< Overflow interrupt. */
#define RA_ELC_EVENT_CAN0_ERROR             0x0A1 /**< Error interrupt. */
#define RA_ELC_EVENT_CAN0_FIFO_RX           0x0A2 /**< Receive FIFO interrupt. */
#define RA_ELC_EVENT_CAN0_FIFO_TX           0x0A3 /**< Transmit FIFO interrupt. */
#define RA_ELC_EVENT_CAN0_MAILBOX_RX        0x0A4 /**< Reception complete interrupt. */
#define RA_ELC_EVENT_CAN0_MAILBOX_TX        0x0A5 /**< Transmission complete interrupt. */
#define RA_ELC_EVENT_IOPORT_EVENT_1         0x0B1 /**< Port 1 event. */
#define RA_ELC_EVENT_IOPORT_EVENT_2         0x0B2 /**< Port 2 event. */
#define RA_ELC_EVENT_IOPORT_EVENT_3         0x0B3 /**< Port 3 event. */
#define RA_ELC_EVENT_IOPORT_EVENT_4         0x0B4 /**< Port 4 event. */
#define RA_ELC_EVENT_ELC_SOFTWARE_EVENT_0   0x0B5 /**< Software event 0. */
#define RA_ELC_EVENT_ELC_SOFTWARE_EVENT_1   0x0B6 /**< Software event 1. */
#define RA_ELC_EVENT_POEG0_EVENT            0x0B7 /**< Port Output disable 0 interrupt. */
#define RA_ELC_EVENT_POEG1_EVENT            0x0B8 /**< Port Output disable 1 interrupt. */
#define RA_ELC_EVENT_POEG2_EVENT            0x0B9 /**< Port Output disable 2 interrupt. */
#define RA_ELC_EVENT_POEG3_EVENT            0x0BA /**< Port Output disable 3 interrupt. */
#define RA_ELC_EVENT_GPT1_CAPTURE_COMPARE_A 0x0C9 /**< Capture/Compare match A. */
#define RA_ELC_EVENT_GPT1_CAPTURE_COMPARE_B 0x0CA /**< Capture/Compare match B. */
#define RA_ELC_EVENT_GPT1_COMPARE_C         0x0CB /**< Compare match C. */
#define RA_ELC_EVENT_GPT1_COMPARE_D         0x0CC /**< Compare match D. */
#define RA_ELC_EVENT_GPT1_COMPARE_E         0x0CD /**< Compare match E. */
#define RA_ELC_EVENT_GPT1_COMPARE_F         0x0CE /**< Compare match F. */
#define RA_ELC_EVENT_GPT1_COUNTER_OVERFLOW  0x0CF /**< Overflow. */
#define RA_ELC_EVENT_GPT1_COUNTER_UNDERFLOW 0x0D0 /**< Underflow. */
#define RA_ELC_EVENT_GPT1_PC                0x0D1 /**< Period count function finish. */
#define RA_ELC_EVENT_GPT2_CAPTURE_COMPARE_A 0x0D2 /**< Capture/Compare match A. */
#define RA_ELC_EVENT_GPT2_CAPTURE_COMPARE_B 0x0D3 /**< Capture/Compare match B. */
#define RA_ELC_EVENT_GPT2_COMPARE_C         0x0D4 /**< Compare match C. */
#define RA_ELC_EVENT_GPT2_COMPARE_D         0x0D5 /**< Compare match D. */
#define RA_ELC_EVENT_GPT2_COMPARE_E         0x0D6 /**< Compare match E. */
#define RA_ELC_EVENT_GPT2_COMPARE_F         0x0D7 /**< Compare match F. */
#define RA_ELC_EVENT_GPT2_COUNTER_OVERFLOW  0x0D8 /**< Overflow. */
#define RA_ELC_EVENT_GPT2_COUNTER_UNDERFLOW 0x0D9 /**< Underflow. */
#define RA_ELC_EVENT_GPT4_CAPTURE_COMPARE_A 0x0E4 /**< Capture/Compare match A. */
#define RA_ELC_EVENT_GPT4_CAPTURE_COMPARE_B 0x0E5 /**< Capture/Compare match B. */
#define RA_ELC_EVENT_GPT4_COMPARE_C         0x0E6 /**< Compare match C. */
#define RA_ELC_EVENT_GPT4_COMPARE_D         0x0E7 /**< Compare match D. */
#define RA_ELC_EVENT_GPT4_COMPARE_E         0x0E8 /**< Compare match E. */
#define RA_ELC_EVENT_GPT4_COMPARE_F         0x0E9 /**< Compare match F. */
#define RA_ELC_EVENT_GPT4_COUNTER_OVERFLOW  0x0EA /**< Overflow. */
#define RA_ELC_EVENT_GPT4_COUNTER_UNDERFLOW 0x0EB /**< Underflow. */
#define RA_ELC_EVENT_GPT4_PC                0x0EC /**< Period count function finish. */
#define RA_ELC_EVENT_GPT5_CAPTURE_COMPARE_A 0x0ED /**< Capture/Compare match A. */
#define RA_ELC_EVENT_GPT5_CAPTURE_COMPARE_B 0x0EE /**< Capture/Compare match B. */
#define RA_ELC_EVENT_GPT5_COMPARE_C         0x0EF /**< Compare match C. */
#define RA_ELC_EVENT_GPT5_COMPARE_D         0x0F0 /**< Compare match D. */
#define RA_ELC_EVENT_GPT5_COMPARE_E         0x0F1 /**< Compare match E. */
#define RA_ELC_EVENT_GPT5_COMPARE_F         0x0F2 /**< Compare match F. */
#define RA_ELC_EVENT_GPT5_COUNTER_OVERFLOW  0x0F3 /**< Overflow. */
#define RA_ELC_EVENT_GPT5_COUNTER_UNDERFLOW 0x0F4 /**< Underflow. */
#define RA_ELC_EVENT_GPT5_PC                0x0F5 /**< Period count function finish. */
#define RA_ELC_EVENT_GPT6_CAPTURE_COMPARE_A 0x0F6 /**< Capture/Compare match A. */
#define RA_ELC_EVENT_GPT6_CAPTURE_COMPARE_B 0x0F7 /**< Capture/Compare match B. */
#define RA_ELC_EVENT_GPT6_COMPARE_C         0x0F8 /**< Compare match C. */
#define RA_ELC_EVENT_GPT6_COMPARE_D         0x0F9 /**< Compare match D. */
#define RA_ELC_EVENT_GPT6_COMPARE_E         0x0FA /**< Compare match E. */
#define RA_ELC_EVENT_GPT6_COMPARE_F         0x0FB /**< Compare match F. */
#define RA_ELC_EVENT_GPT6_COUNTER_OVERFLOW  0x0FC /**< Overflow. */
#define RA_ELC_EVENT_GPT6_COUNTER_UNDERFLOW 0x0FD /**< Underflow. */
#define RA_ELC_EVENT_GPT6_PC                0x0FE /**< Period count function finish. */
#define RA_ELC_EVENT_GPT7_CAPTURE_COMPARE_A 0x0FF /**< Capture/Compare match A. */
#define RA_ELC_EVENT_GPT7_CAPTURE_COMPARE_B 0x100 /**< Capture/Compare match B. */
#define RA_ELC_EVENT_GPT7_COMPARE_C         0x101 /**< Compare match C. */
#define RA_ELC_EVENT_GPT7_COMPARE_D         0x102 /**< Compare match D. */
#define RA_ELC_EVENT_GPT7_COMPARE_E         0x103 /**< Compare match E. */
#define RA_ELC_EVENT_GPT7_COMPARE_F         0x104 /**< Compare match F. */
#define RA_ELC_EVENT_GPT7_COUNTER_OVERFLOW  0x105 /**< Overflow. */
#define RA_ELC_EVENT_GPT7_COUNTER_UNDERFLOW 0x106 /**< Underflow. */
#define RA_ELC_EVENT_ADC0_SCAN_END          0x160 /**< End of A/D scanning operation. */
#define RA_ELC_EVENT_ADC0_SCAN_END_B        0x161 /**< A/D scan end interrupt for group B. */
#define RA_ELC_EVENT_ADC0_WINDOW_A          0x162 /**< Window A Compare match interrupt. */
#define RA_ELC_EVENT_ADC0_WINDOW_B          0x163 /**< Window B Compare match interrupt. */
#define RA_ELC_EVENT_ADC0_COMPARE_MATCH     0x164 /**< Compare match. */
#define RA_ELC_EVENT_ADC0_COMPARE_MISMATCH  0x165 /**< Compare mismatch. */
#define RA_ELC_EVENT_EDMAC0_EINT            0x16F /**< EDMAC 0 interrupt. */
#define RA_ELC_EVENT_SCI0_RXI               0x180 /**< Receive data full. */
#define RA_ELC_EVENT_SCI0_TXI               0x181 /**< Transmit data empty. */
#define RA_ELC_EVENT_SCI0_TEI               0x182 /**< Transmit end. */
#define RA_ELC_EVENT_SCI0_ERI               0x183 /**< Receive error. */
#define RA_ELC_EVENT_SCI0_AM                0x184 /**< Address match event. */
#define RA_ELC_EVENT_SCI0_RXI_OR_ERI        0x185 /**< Receive data full/Receive error. */
#define RA_ELC_EVENT_SCI1_RXI               0x186 /**< Receive data full. */
#define RA_ELC_EVENT_SCI1_TXI               0x187 /**< Transmit data empty. */
#define RA_ELC_EVENT_SCI1_TEI               0x188 /**< Transmit end. */
#define RA_ELC_EVENT_SCI1_ERI               0x189 /**< Receive error. */
#define RA_ELC_EVENT_SCI2_RXI               0x18C /**< Receive data full. */
#define RA_ELC_EVENT_SCI2_TXI               0x18D /**< Transmit data empty. */
#define RA_ELC_EVENT_SCI2_TEI               0x18E /**< Transmit end. */
#define RA_ELC_EVENT_SCI2_ERI               0x18F /**< Receive error. */
#define RA_ELC_EVENT_SCI3_RXI               0x192 /**< Receive data full. */
#define RA_ELC_EVENT_SCI3_TXI               0x193 /**< Transmit data empty. */
#define RA_ELC_EVENT_SCI3_TEI               0x194 /**< Transmit end. */
#define RA_ELC_EVENT_SCI3_ERI               0x195 /**< Receive error. */
#define RA_ELC_EVENT_SCI3_AM                0x196 /**< Address match event. */
#define RA_ELC_EVENT_SCI4_RXI               0x198 /**< Receive data full. */
#define RA_ELC_EVENT_SCI4_TXI               0x199 /**< Transmit data empty. */
#define RA_ELC_EVENT_SCI4_TEI               0x19A /**< Transmit end. */
#define RA_ELC_EVENT_SCI4_ERI               0x19B /**< Receive error. */
#define RA_ELC_EVENT_SCI4_AM                0x19C /**< Address match event. */
#define RA_ELC_EVENT_SCI9_RXI               0x1B6 /**< Receive data full. */
#define RA_ELC_EVENT_SCI9_TXI               0x1B7 /**< Transmit data empty. */
#define RA_ELC_EVENT_SCI9_TEI               0x1B8 /**< Transmit end. */
#define RA_ELC_EVENT_SCI9_ERI               0x1B9 /**< Receive error. */
#define RA_ELC_EVENT_SCI9_AM                0x1BA /**< Address match event. */
#define RA_ELC_EVENT_SCIX1_SCIX0            0x1C0 /**< SCI1 extended serial mode event 0. */
#define RA_ELC_EVENT_SCI2_SCIX0             0x1C0 /**< SCI1 extended serial mode event 0. */
#define RA_ELC_EVENT_SCIX1_SCIX1            0x1C1 /**< SCI1 extended serial mode event 1. */
#define RA_ELC_EVENT_SCI2_SCIX1             0x1C1 /**< SCI1 extended serial mode event 1. */
#define RA_ELC_EVENT_SCIX1_SCIX2            0x1C2 /**< SCI1 extended serial mode event 2. */
#define RA_ELC_EVENT_SCI2_SCIX2             0x1C2 /**< SCI1 extended serial mode event 2. */
#define RA_ELC_EVENT_SCIX1_SCIX3            0x1C3 /**< SCI1 extended serial mode event 3. */
#define RA_ELC_EVENT_SCI2_SCIX3             0x1C3 /**< SCI1 extended serial mode event 3. */
#define RA_ELC_EVENT_SPI0_RXI               0x1C4 /**< Receive buffer full. */
#define RA_ELC_EVENT_SPI0_TXI               0x1C5 /**< Transmit buffer empty. */
#define RA_ELC_EVENT_SPI0_IDLE              0x1C6 /**< Idle. */
#define RA_ELC_EVENT_SPI0_ERI               0x1C7 /**< Error. */
#define RA_ELC_EVENT_SPI0_TEI               0x1C8 /**< Transmission complete event. */
#define RA_ELC_EVENT_SPI1_RXI               0x1C9 /**< Receive buffer full. */
#define RA_ELC_EVENT_SPI1_TXI               0x1CA /**< Transmit buffer empty. */
#define RA_ELC_EVENT_SPI1_IDLE              0x1CB /**< Idle. */
#define RA_ELC_EVENT_SPI1_ERI               0x1CC /**< Error. */
#define RA_ELC_EVENT_SPI1_TEI               0x1CD /**< Transmission complete event. */
#define RA_ELC_EVENT_QSPI_INT               0x1DA /**< QSPI interrupt. */
#define RA_ELC_EVENT_DOC_INT                0x1DB /**< Data operation circuit interrupt. */

/** @} */

/**
 * @name Renesas RA ELC possible peripherals to be linked to event signals
 * @{
 */
#define RA_ELC_PERIPHERAL_GPT_A   0  /**< General PWM Timer A */
#define RA_ELC_PERIPHERAL_GPT_B   1  /**< General PWM Timer B */
#define RA_ELC_PERIPHERAL_GPT_C   2  /**< General PWM Timer C */
#define RA_ELC_PERIPHERAL_GPT_D   3  /**< General PWM Timer D */
#define RA_ELC_PERIPHERAL_GPT_E   4  /**< General PWM Timer E */
#define RA_ELC_PERIPHERAL_GPT_F   5  /**< General PWM Timer F */
#define RA_ELC_PERIPHERAL_GPT_G   6  /**< General PWM Timer G */
#define RA_ELC_PERIPHERAL_GPT_H   7  /**< General PWM Timer H */
#define RA_ELC_PERIPHERAL_ADC0    8  /**< ADC0 */
#define RA_ELC_PERIPHERAL_ADC0_B  9  /**< ADC0 Group B */
#define RA_ELC_PERIPHERAL_DAC0    12 /**< DAC0 */
#define RA_ELC_PERIPHERAL_IOPORT1 14 /**< IOPORT1 */
#define RA_ELC_PERIPHERAL_IOPORT2 15 /**< IOPORT2 */
#define RA_ELC_PERIPHERAL_IOPORT3 16 /**< IOPORT3 */
#define RA_ELC_PERIPHERAL_IOPORT4 17 /**< IOPORT4 */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_MISC_RENESAS_RA_ELC_RA6E1_ELC_H_ */
