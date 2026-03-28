/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Renesas RA4M1 Event Link Controller (ELC) definitions
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_MISC_RENESAS_RA_ELC_RA4M1_ELC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_MISC_RENESAS_RA_ELC_RA4M1_ELC_H_

/**
 * @name Event codes for Renesas RA4M1 Event Link Controller (ELC).
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
#define RA_ELC_EVENT_ICU_IRQ14              0x00F /**< External pin interrupt 14. */
#define RA_ELC_EVENT_ICU_IRQ15              0x010 /**< External pin interrupt 15. */
#define RA_ELC_EVENT_DMAC0_INT              0x011 /**< DMAC0 transfer end. */
#define RA_ELC_EVENT_DMAC1_INT              0x012 /**< DMAC1 transfer end. */
#define RA_ELC_EVENT_DMAC2_INT              0x013 /**< DMAC2 transfer end. */
#define RA_ELC_EVENT_DMAC3_INT              0x014 /**< DMAC3 transfer end. */
#define RA_ELC_EVENT_DTC_COMPLETE           0x015 /**< DTC transfer complete. */
#define RA_ELC_EVENT_DTC_END                0x016 /**< DTC transfer end. */
#define RA_ELC_EVENT_ICU_SNOOZE_CANCEL      0x017 /**< Canceling from Snooze mode. */
#define RA_ELC_EVENT_FCU_FRDYI              0x018 /**< Flash ready interrupt. */
#define RA_ELC_EVENT_LVD_LVD1               0x019 /**< Voltage monitor 1 interrupt. */
#define RA_ELC_EVENT_LVD_LVD2               0x01A /**< Voltage monitor 2 interrupt. */
#define RA_ELC_EVENT_LVD_VBATT              0x01B /**< VBATT low voltage detect. */
#define RA_ELC_EVENT_CGC_MOSC_STOP          0x01C /**< Main Clock oscillation stop. */
#define RA_ELC_EVENT_LPM_SNOOZE_REQUEST     0x01D /**< Snooze entry. */
#define RA_ELC_EVENT_AGT0_INT               0x01E /**< AGT interrupt. */
#define RA_ELC_EVENT_AGT0_COMPARE_A         0x01F /**< Compare match A. */
#define RA_ELC_EVENT_AGT0_COMPARE_B         0x020 /**< Compare match B. */
#define RA_ELC_EVENT_AGT1_INT               0x021 /**< AGT interrupt. */
#define RA_ELC_EVENT_AGT1_COMPARE_A         0x022 /**< Compare match A. */
#define RA_ELC_EVENT_AGT1_COMPARE_B         0x023 /**< Compare match B. */
#define RA_ELC_EVENT_IWDT_UNDERFLOW         0x024 /**< IWDT underflow. */
#define RA_ELC_EVENT_WDT_UNDERFLOW          0x025 /**< WDT underflow. */
#define RA_ELC_EVENT_RTC_ALARM              0x026 /**< Alarm interrupt. */
#define RA_ELC_EVENT_RTC_PERIOD             0x027 /**< Periodic interrupt. */
#define RA_ELC_EVENT_RTC_CARRY              0x028 /**< Carry interrupt. */
#define RA_ELC_EVENT_ADC0_SCAN_END          0x029 /**< End of A/D scanning operation. */
#define RA_ELC_EVENT_ADC0_SCAN_END_B        0x02A /**< A/D scan end interrupt for group B. */
#define RA_ELC_EVENT_ADC0_WINDOW_A          0x02B /**< Window A Compare match interrupt. */
#define RA_ELC_EVENT_ADC0_WINDOW_B          0x02C /**< Window B Compare match interrupt. */
#define RA_ELC_EVENT_ADC0_COMPARE_MATCH     0x02D /**< Compare match. */
#define RA_ELC_EVENT_ADC0_COMPARE_MISMATCH  0x02E /**< Compare mismatch. */
#define RA_ELC_EVENT_ACMPLP0_INT            0x02F /**< Low Power Comparator channel 0 interrupt. */
#define RA_ELC_EVENT_ACMPLP1_INT            0x030 /**< Low Power Comparator channel 1 interrupt. */
#define RA_ELC_EVENT_USBFS_FIFO_0           0x031 /**< DMA/DTC transfer request 0. */
#define RA_ELC_EVENT_USBFS_FIFO_1           0x032 /**< DMA/DTC transfer request 1. */
#define RA_ELC_EVENT_USBFS_INT              0x033 /**< USBFS interrupt. */
#define RA_ELC_EVENT_USBFS_RESUME           0x034 /**< USBFS resume interrupt. */
#define RA_ELC_EVENT_IIC0_RXI               0x035 /**< Receive data full. */
#define RA_ELC_EVENT_IIC0_TXI               0x036 /**< Transmit data empty. */
#define RA_ELC_EVENT_IIC0_TEI               0x037 /**< Transmit end. */
#define RA_ELC_EVENT_IIC0_ERI               0x038 /**< Transfer error. */
#define RA_ELC_EVENT_IIC0_WUI               0x039 /**< Wakeup interrupt. */
#define RA_ELC_EVENT_IIC1_RXI               0x03A /**< Receive data full. */
#define RA_ELC_EVENT_IIC1_TXI               0x03B /**< Transmit data empty. */
#define RA_ELC_EVENT_IIC1_TEI               0x03C /**< Transmit end. */
#define RA_ELC_EVENT_IIC1_ERI               0x03D /**< Transfer error. */
#define RA_ELC_EVENT_SSI0_TXI               0x03E /**< Transmit data empty. */
#define RA_ELC_EVENT_SSI0_RXI               0x03F /**< Receive data full. */
#define RA_ELC_EVENT_SSI0_INT               0x041 /**< Error interrupt. */
#define RA_ELC_EVENT_CTSU_WRITE             0x042 /**< Write request interrupt. */
#define RA_ELC_EVENT_CTSU_READ              0x043 /**< Measurement data request interrupt. */
#define RA_ELC_EVENT_CTSU_END               0x044 /**< Measurement end interrupt. */
#define RA_ELC_EVENT_KEY_INT                0x045 /**< Key interrupt. */
#define RA_ELC_EVENT_DOC_INT                0x046 /**< Data operation circuit interrupt. */
#define RA_ELC_EVENT_CAC_FREQUENCY_ERROR    0x047 /**< Frequency error interrupt. */
#define RA_ELC_EVENT_CAC_MEASUREMENT_END    0x048 /**< Measurement end interrupt. */
#define RA_ELC_EVENT_CAC_OVERFLOW           0x049 /**< Overflow interrupt. */
#define RA_ELC_EVENT_CAN0_ERROR             0x04A /**< Error interrupt. */
#define RA_ELC_EVENT_CAN0_FIFO_RX           0x04B /**< Receive FIFO interrupt. */
#define RA_ELC_EVENT_CAN0_FIFO_TX           0x04C /**< Transmit FIFO interrupt. */
#define RA_ELC_EVENT_CAN0_MAILBOX_RX        0x04D /**< Reception complete interrupt. */
#define RA_ELC_EVENT_CAN0_MAILBOX_TX        0x04E /**< Transmission complete interrupt. */
#define RA_ELC_EVENT_IOPORT_EVENT_1         0x04F /**< Port 1 event. */
#define RA_ELC_EVENT_IOPORT_EVENT_2         0x050 /**< Port 2 event. */
#define RA_ELC_EVENT_IOPORT_EVENT_3         0x051 /**< Port 3 event. */
#define RA_ELC_EVENT_IOPORT_EVENT_4         0x052 /**< Port 4 event. */
#define RA_ELC_EVENT_ELC_SOFTWARE_EVENT_0   0x053 /**< Software event 0. */
#define RA_ELC_EVENT_ELC_SOFTWARE_EVENT_1   0x054 /**< Software event 1. */
#define RA_ELC_EVENT_POEG0_EVENT            0x055 /**< Port Output disable 0 interrupt. */
#define RA_ELC_EVENT_POEG1_EVENT            0x056 /**< Port Output disable 1 interrupt. */
#define RA_ELC_EVENT_GPT0_CAPTURE_COMPARE_A 0x057 /**< Capture/Compare match A. */
#define RA_ELC_EVENT_GPT0_CAPTURE_COMPARE_B 0x058 /**< Capture/Compare match B. */
#define RA_ELC_EVENT_GPT0_COMPARE_C         0x059 /**< Compare match C. */
#define RA_ELC_EVENT_GPT0_COMPARE_D         0x05A /**< Compare match D. */
#define RA_ELC_EVENT_GPT0_COMPARE_E         0x05B /**< Compare match E. */
#define RA_ELC_EVENT_GPT0_COMPARE_F         0x05C /**< Compare match F. */
#define RA_ELC_EVENT_GPT0_COUNTER_OVERFLOW  0x05D /**< Overflow. */
#define RA_ELC_EVENT_GPT0_COUNTER_UNDERFLOW 0x05E /**< Underflow. */
#define RA_ELC_EVENT_GPT1_CAPTURE_COMPARE_A 0x05F /**< Capture/Compare match A. */
#define RA_ELC_EVENT_GPT1_CAPTURE_COMPARE_B 0x060 /**< Capture/Compare match B. */
#define RA_ELC_EVENT_GPT1_COMPARE_C         0x061 /**< Compare match C. */
#define RA_ELC_EVENT_GPT1_COMPARE_D         0x062 /**< Compare match D. */
#define RA_ELC_EVENT_GPT1_COMPARE_E         0x063 /**< Compare match E. */
#define RA_ELC_EVENT_GPT1_COMPARE_F         0x064 /**< Compare match F. */
#define RA_ELC_EVENT_GPT1_COUNTER_OVERFLOW  0x065 /**< Overflow. */
#define RA_ELC_EVENT_GPT1_COUNTER_UNDERFLOW 0x066 /**< Underflow. */
#define RA_ELC_EVENT_GPT2_CAPTURE_COMPARE_A 0x067 /**< Capture/Compare match A. */
#define RA_ELC_EVENT_GPT2_CAPTURE_COMPARE_B 0x068 /**< Capture/Compare match B. */
#define RA_ELC_EVENT_GPT2_COMPARE_C         0x069 /**< Compare match C. */
#define RA_ELC_EVENT_GPT2_COMPARE_D         0x06A /**< Compare match D. */
#define RA_ELC_EVENT_GPT2_COMPARE_E         0x06B /**< Compare match E. */
#define RA_ELC_EVENT_GPT2_COMPARE_F         0x06C /**< Compare match F. */
#define RA_ELC_EVENT_GPT2_COUNTER_OVERFLOW  0x06D /**< Overflow. */
#define RA_ELC_EVENT_GPT2_COUNTER_UNDERFLOW 0x06E /**< Underflow. */
#define RA_ELC_EVENT_GPT3_CAPTURE_COMPARE_A 0x06F /**< Capture/Compare match A. */
#define RA_ELC_EVENT_GPT3_CAPTURE_COMPARE_B 0x070 /**< Capture/Compare match B. */
#define RA_ELC_EVENT_GPT3_COMPARE_C         0x071 /**< Compare match C. */
#define RA_ELC_EVENT_GPT3_COMPARE_D         0x072 /**< Compare match D. */
#define RA_ELC_EVENT_GPT3_COMPARE_E         0x073 /**< Compare match E. */
#define RA_ELC_EVENT_GPT3_COMPARE_F         0x074 /**< Compare match F. */
#define RA_ELC_EVENT_GPT3_COUNTER_OVERFLOW  0x075 /**< Overflow. */
#define RA_ELC_EVENT_GPT3_COUNTER_UNDERFLOW 0x076 /**< Underflow. */
#define RA_ELC_EVENT_GPT4_CAPTURE_COMPARE_A 0x077 /**< Capture/Compare match A. */
#define RA_ELC_EVENT_GPT4_CAPTURE_COMPARE_B 0x078 /**< Capture/Compare match B. */
#define RA_ELC_EVENT_GPT4_COMPARE_C         0x079 /**< Compare match C. */
#define RA_ELC_EVENT_GPT4_COMPARE_D         0x07A /**< Compare match D. */
#define RA_ELC_EVENT_GPT4_COMPARE_E         0x07B /**< Compare match E. */
#define RA_ELC_EVENT_GPT4_COMPARE_F         0x07C /**< Compare match F. */
#define RA_ELC_EVENT_GPT4_COUNTER_OVERFLOW  0x07D /**< Overflow. */
#define RA_ELC_EVENT_GPT4_COUNTER_UNDERFLOW 0x07E /**< Underflow. */
#define RA_ELC_EVENT_GPT5_CAPTURE_COMPARE_A 0x07F /**< Capture/Compare match A. */
#define RA_ELC_EVENT_GPT5_CAPTURE_COMPARE_B 0x080 /**< Capture/Compare match B. */
#define RA_ELC_EVENT_GPT5_COMPARE_C         0x081 /**< Compare match C. */
#define RA_ELC_EVENT_GPT5_COMPARE_D         0x082 /**< Compare match D. */
#define RA_ELC_EVENT_GPT5_COMPARE_E         0x083 /**< Compare match E. */
#define RA_ELC_EVENT_GPT5_COMPARE_F         0x084 /**< Compare match F. */
#define RA_ELC_EVENT_GPT5_COUNTER_OVERFLOW  0x085 /**< Overflow. */
#define RA_ELC_EVENT_GPT5_COUNTER_UNDERFLOW 0x086 /**< Underflow. */
#define RA_ELC_EVENT_GPT6_CAPTURE_COMPARE_A 0x087 /**< Capture/Compare match A. */
#define RA_ELC_EVENT_GPT6_CAPTURE_COMPARE_B 0x088 /**< Capture/Compare match B. */
#define RA_ELC_EVENT_GPT6_COMPARE_C         0x089 /**< Compare match C. */
#define RA_ELC_EVENT_GPT6_COMPARE_D         0x08A /**< Compare match D. */
#define RA_ELC_EVENT_GPT6_COMPARE_E         0x08B /**< Compare match E. */
#define RA_ELC_EVENT_GPT6_COMPARE_F         0x08C /**< Compare match F. */
#define RA_ELC_EVENT_GPT6_COUNTER_OVERFLOW  0x08D /**< Overflow. */
#define RA_ELC_EVENT_GPT6_COUNTER_UNDERFLOW 0x08E /**< Underflow. */
#define RA_ELC_EVENT_GPT7_CAPTURE_COMPARE_A 0x08F /**< Capture/Compare match A. */
#define RA_ELC_EVENT_GPT7_CAPTURE_COMPARE_B 0x090 /**< Capture/Compare match B. */
#define RA_ELC_EVENT_GPT7_COMPARE_C         0x091 /**< Compare match C. */
#define RA_ELC_EVENT_GPT7_COMPARE_D         0x092 /**< Compare match D. */
#define RA_ELC_EVENT_GPT7_COMPARE_E         0x093 /**< Compare match E. */
#define RA_ELC_EVENT_GPT7_COMPARE_F         0x094 /**< Compare match F. */
#define RA_ELC_EVENT_GPT7_COUNTER_OVERFLOW  0x095 /**< Overflow. */
#define RA_ELC_EVENT_GPT7_COUNTER_UNDERFLOW 0x096 /**< Underflow. */
#define RA_ELC_EVENT_OPS_UVW_EDGE           0x097 /**< UVW edge event. */
#define RA_ELC_EVENT_SCI0_RXI               0x098 /**< Receive data full. */
#define RA_ELC_EVENT_SCI0_TXI               0x099 /**< Transmit data empty. */
#define RA_ELC_EVENT_SCI0_TEI               0x09A /**< Transmit end. */
#define RA_ELC_EVENT_SCI0_ERI               0x09B /**< Receive error. */
#define RA_ELC_EVENT_SCI0_AM                0x09C /**< Address match event. */
#define RA_ELC_EVENT_SCI0_RXI_OR_ERI        0x09D /**< Receive data full/Receive error. */
#define RA_ELC_EVENT_SCI1_RXI               0x09E /**< Receive data full. */
#define RA_ELC_EVENT_SCI1_TXI               0x09F /**< Transmit data empty. */
#define RA_ELC_EVENT_SCI1_TEI               0x0A0 /**< Transmit end. */
#define RA_ELC_EVENT_SCI1_ERI               0x0A1 /**< Receive error. */
#define RA_ELC_EVENT_SCI1_AM                0x0A2 /**< Address match event. */
#define RA_ELC_EVENT_SCI2_RXI               0x0A3 /**< Receive data full. */
#define RA_ELC_EVENT_SCI2_TXI               0x0A4 /**< Transmit data empty. */
#define RA_ELC_EVENT_SCI2_TEI               0x0A5 /**< Transmit end. */
#define RA_ELC_EVENT_SCI2_ERI               0x0A6 /**< Receive error. */
#define RA_ELC_EVENT_SCI2_AM                0x0A7 /**< Address match event. */
#define RA_ELC_EVENT_SCI9_RXI               0x0A8 /**< Receive data full. */
#define RA_ELC_EVENT_SCI9_TXI               0x0A9 /**< Transmit data empty. */
#define RA_ELC_EVENT_SCI9_TEI               0x0AA /**< Transmit end. */
#define RA_ELC_EVENT_SCI9_ERI               0x0AB /**< Receive error. */
#define RA_ELC_EVENT_SCI9_AM                0x0AC /**< Address match event. */
#define RA_ELC_EVENT_SPI0_RXI               0x0AD /**< Receive buffer full. */
#define RA_ELC_EVENT_SPI0_TXI               0x0AE /**< Transmit buffer empty. */
#define RA_ELC_EVENT_SPI0_IDLE              0x0AF /**< Idle. */
#define RA_ELC_EVENT_SPI0_ERI               0x0B0 /**< Error. */
#define RA_ELC_EVENT_SPI0_TEI               0x0B1 /**< Transmission complete event. */
#define RA_ELC_EVENT_SPI1_RXI               0x0B2 /**< Receive buffer full. */
#define RA_ELC_EVENT_SPI1_TXI               0x0B3 /**< Transmit buffer empty. */
#define RA_ELC_EVENT_SPI1_IDLE              0x0B4 /**< Idle. */
#define RA_ELC_EVENT_SPI1_ERI               0x0B5 /**< Error. */
#define RA_ELC_EVENT_SPI1_TEI               0x0B6 /**< Transmission complete event. */

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
#define RA_ELC_PERIPHERAL_CTSU    18 /**< CTSU */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_MISC_RENESAS_RA_ELC_RA4M1_ELC_H_ */
