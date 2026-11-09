/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup wuc_interface
 * @brief NXP RW6xx PMU wakeup source identifiers.
 *
 * Values match IRQn_Type in the RW61x SDK device headers, without the
 * Cortex-M exception offset of 16. Use numeric macros so devicetree
 * preprocessing can expand them.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_WUC_NXP_RW_PMU_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_WUC_NXP_RW_PMU_H_

/** Watchdog timer 0. */
#define NXP_RW_PMU_WDT0                 0
/** DMA controller 0. */
#define NXP_RW_PMU_DMA0                 1
/** GPIO interrupt A. */
#define NXP_RW_PMU_GPIO_INTA            2
/** GPIO interrupt B. */
#define NXP_RW_PMU_GPIO_INTB            3
/** Pin interrupt 0. */
#define NXP_RW_PMU_PIN_INT0             4
/** Pin interrupt 1. */
#define NXP_RW_PMU_PIN_INT1             5
/** Pin interrupt 2. */
#define NXP_RW_PMU_PIN_INT2             6
/** Pin interrupt 3. */
#define NXP_RW_PMU_PIN_INT3             7
/** Microtick timer. */
#define NXP_RW_PMU_UTICK                8
/** Multirate timer. */
#define NXP_RW_PMU_MRT                  9
/** Counter timer 0. */
#define NXP_RW_PMU_CTIMER0              10
/** Counter timer 1. */
#define NXP_RW_PMU_CTIMER1              11
/** State configurable timer 0. */
#define NXP_RW_PMU_SCT0                 12
/** Counter timer 3. */
#define NXP_RW_PMU_CTIMER3              13
/** Flexcomm interface 0. */
#define NXP_RW_PMU_FLEXCOMM0            14
/** Flexcomm interface 1. */
#define NXP_RW_PMU_FLEXCOMM1            15
/** Flexcomm interface 2. */
#define NXP_RW_PMU_FLEXCOMM2            16
/** Flexcomm interface 3. */
#define NXP_RW_PMU_FLEXCOMM3            17
/** Flexcomm interface 14. */
#define NXP_RW_PMU_FLEXCOMM14           20
/** Free-running multirate timer. */
#define NXP_RW_PMU_GFMRT                23
/** Digital microphone. */
#define NXP_RW_PMU_DMIC                 25
/** Deep-sleep wakeup interrupt. */
#define NXP_RW_PMU_WKDEEPSLEEP          26
/** Hypervisor interrupt. */
#define NXP_RW_PMU_HYPERVISOR           27
/** Security violation interrupt. */
#define NXP_RW_PMU_SECUREVIOLATION      28
/** Hardware voice activity detector. */
#define NXP_RW_PMU_HWVAD                29
/** RTC alarm and wakeup timer. */
#define NXP_RW_PMU_RTC                  32
/** Pin interrupt 4. */
#define NXP_RW_PMU_PIN_INT4             35
/** Pin interrupt 5. */
#define NXP_RW_PMU_PIN_INT5             36
/** Pin interrupt 6. */
#define NXP_RW_PMU_PIN_INT6             37
/** Pin interrupt 7. */
#define NXP_RW_PMU_PIN_INT7             38
/** Counter timer 2. */
#define NXP_RW_PMU_CTIMER2              39
/** Counter timer 4. */
#define NXP_RW_PMU_CTIMER4              40
/** OS event timer. */
#define NXP_RW_PMU_OS_EVENT_TIMER       41
/** FlexSPI controller. */
#define NXP_RW_PMU_FLEXSPI              42
/** SDIO controller. */
#define NXP_RW_PMU_SDIO                 46
/** Secure GPIO interrupt A. */
#define NXP_RW_PMU_SGPIO_INTA           47
/** Secure GPIO interrupt B. */
#define NXP_RW_PMU_SGPIO_INTB           48
/** USB controller. */
#define NXP_RW_PMU_USB                  50
/** DMA controller 1. */
#define NXP_RW_PMU_DMA1                 54
/** Physically unclonable function. */
#define NXP_RW_PMU_PUF                  55
/** PowerQuad accelerator. */
#define NXP_RW_PMU_POWERQUAD            56
/** Capture timer. */
#define NXP_RW_PMU_CAPTIMER             62
/** WLAN wakeup signal 0. */
#define NXP_RW_PMU_WLAN_WAKEUP0         72
/** WLAN wakeup signal 1. */
#define NXP_RW_PMU_WLAN_WAKEUP1         73
/** BLE/802.15.4 wakeup signal 0. */
#define NXP_RW_PMU_BLE_WAKEUP0          90
/** BLE/802.15.4 wakeup signal 1. */
#define NXP_RW_PMU_BLE_WAKEUP1          91
/** AON wakeup pin 0. */
#define NXP_RW_PMU_PIN0                 100
/** AON wakeup pin 1. */
#define NXP_RW_PMU_PIN1                 101
/** Analog unit DAC. */
#define NXP_RW_PMU_GAU_DAC              108
/** Analog comparator wakeup. */
#define NXP_RW_PMU_GAU_ACOMP_WKUP       109
/** Analog comparator interrupt. */
#define NXP_RW_PMU_GAU_ACOMP            110
/** Analog unit ADC1. */
#define NXP_RW_PMU_GAU_ADC1             111
/** Analog unit ADC0. */
#define NXP_RW_PMU_GAU_ADC0             112
/** USIM interface. */
#define NXP_RW_PMU_USIM                 113
/** OTP controller. */
#define NXP_RW_PMU_OTP                  114
/** Ethernet controller. */
#define NXP_RW_PMU_ENET                 115
/** Ethernet timer. */
#define NXP_RW_PMU_ENET_TIMER           116
/** Power management interrupt. */
#define NXP_RW_PMU_PMIP                 117
/** Power management state change. */
#define NXP_RW_PMU_PMIP_CHANGE          118
/** Intrusion and tamper response controller. */
#define NXP_RW_PMU_ITRC                 119

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_WUC_NXP_RW_PMU_H_ */
