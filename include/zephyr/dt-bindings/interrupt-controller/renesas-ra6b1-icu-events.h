/*
 * SPDX-FileCopyrightText: 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Renesas RA6B1 ICU event definitions for devicetree
 * @ingroup dt_ra6b1_icu_events
 */

#ifndef ZEPHYR_DT_BINDINGS_INTERRUPT_CONTROLLER_RENESAS_RA_ICU_EVENT_H_
#define ZEPHYR_DT_BINDINGS_INTERRUPT_CONTROLLER_RENESAS_RA_ICU_EVENT_H_

/**
 * @defgroup dt_ra6b1_icu_events Renesas RA6B1 ICU event identifiers
 * @brief ICU event numbers for the Renesas RA6B1 interrupt controller.
 * @ingroup devicetree-interrupt_controller
 *
 * These values describe the ICU event source identifiers used when configuring
 * RA6B1 interrupt routing in devicetree bindings.
 *
 * @{
 */

/** @cond INTERNAL_HIDDEN */

#define ICU_IELSR_NUMBER              (60)    /* It should be equal to CONFIG_NUM_IRQS */
#define ICU_EVENT_PRIO_LOWEST	      (14)
#define ICU_IELSR_UNSPECIFIED         (64)

#define ICU_EVENT_NONE                     (0)     /* Link disabled */
#define ICU_EVENT_WKUPCW_GPIO0_IRQ         (0x001) /* External pin interrupt 0 */
#define ICU_EVENT_WKUPCW_GPIO1_IRQ         (0x002) /* External pin interrupt 1 */
#define ICU_EVENT_WKUPCW_GPIO2_IRQ         (0x003) /* External pin interrupt 2 */
#define ICU_EVENT_WKUPCW_GPIO3_IRQ         (0x004) /* External pin interrupt 3 */
#define ICU_EVENT_WKUPCW_GPIO4_IRQ         (0x005) /* External pin interrupt 4 */
#define ICU_EVENT_WKUPCW_GPIO5_IRQ         (0x006) /* External pin interrupt 5 */
#define ICU_EVENT_WKUPCW_GPIO6_IRQ         (0x007) /* External pin interrupt 6 */
#define ICU_EVENT_WKUPCW_GPIO7_IRQ         (0x008) /* External pin interrupt 7 */
#define ICU_EVENT_DMACWB0_IRQ              (0x009) /* DMAC transfer end 0 */
#define ICU_EVENT_DMACWB1_IRQ              (0x00A) /* DMAC transfer end 1 */
#define ICU_EVENT_DMACWB2_IRQ              (0x00B) /* DMAC transfer end 2 */
#define ICU_EVENT_DMACWB3_IRQ              (0x00C) /* DMAC transfer end 3 */
#define ICU_EVENT_DMACWB4_IRQ              (0x00D) /* DMAC transfer end 4 */
#define ICU_EVENT_DMACWB5_IRQ              (0x00E) /* DMAC transfer end 5 */
#define ICU_EVENT_DMACWB6_IRQ              (0x00F) /* DMAC transfer end 6 */
#define ICU_EVENT_DMACWB7_IRQ              (0x010) /* DMAC transfer end 7 */
#define ICU_EVENT_DTCW_COMPLETE            (0x011) /* DTC transfer complete */
#define ICU_EVENT_DTCW_TRANSERR            (0x012) /* DTC transfer error */
#define ICU_EVENT_NVMCW_IRQ                (0x013) /* eNVM controller interrupt */
#define ICU_EVENT_XTAL32M_IRQ              (0x014) /* XTAL32M settled */
#define ICU_EVENT_PLLSYS_IRQ               (0x015) /* PLL locked */
#define ICU_EVENT_CMAC_CMAC2SYS_IRQ        (0x016) /* CMAC2SYS interrupt */
#define ICU_EVENT_CMAC_SLPTIM_IRQ          (0x017) /* CMAC sleep timer interrupt */
#define ICU_EVENT_SW_0_IRQ                 (0x018) /* Software interrupt 0 */
#define ICU_EVENT_SW_1_IRQ                 (0x019) /* Software interrupt 1 */
#define ICU_EVENT_SW_2_IRQ                 (0x01A) /* Software interrupt 2 */
#define ICU_EVENT_SW_3_IRQ                 (0x01B) /* Software interrupt 3 */
#define ICU_EVENT_CACW_MEASUREMENT_END_IRQ (0x01C) /* Clock calibration */
#define ICU_EVENT_PDC_SYSCPU_IRQ           (0x01E) /* PDC SYSCPU event */
#define ICU_EVENT_ELCW_GROUP1              (0x01F) /* ELC Group event 1 */
#define ICU_EVENT_ELCW_GROUP2              (0x020) /* ELC Group event 2 */
#define ICU_EVENT_ELCW_GROUP3              (0x021) /* ELC Group event 3 */
#define ICU_EVENT_ELCW_GROUP4              (0x022) /* ELC Group event 4 */
#define ICU_EVENT_ELCW_GROUP5              (0x023) /* ELC Group event 5 */
#define ICU_EVENT_ELCW_SWEVT0              (0x024) /* Software event 0 */
#define ICU_EVENT_ELCW_SWEVT1              (0x025) /* Software event 1 */
#define ICU_EVENT_ELCW_PDC                 (0x026) /* ELC PDC event */
#define ICU_EVENT_WKUPCW_P0_IRQ            (0x027) /* Wakeup controller GPIO P0 interrupt */
#define ICU_EVENT_WKUPCW_P1_IRQ            (0x028) /* Wakeup controller GPIO P1 interrupt */
#define ICU_EVENT_WKUPCW_P2_IRQ            (0x029) /* Wakeup controller GPIO P2 interrupt */
#define ICU_EVENT_WKUPCW_KEY_IRQ           (0x02A) /* Wakeup controller GPIO key interrupt */
#define ICU_EVENT_WDTSYSW_IRQ              (0x02B) /* WDT underflow */
#define ICU_EVENT_RTCWB_ALARM_IRQ          (0x02C) /* Alarm interrupt */
#define ICU_EVENT_RTCWB_MNTH_IRQ           (0x02D) /* Periodic month interrupt */
#define ICU_EVENT_RTCWB_DATE_IRQ           (0x02E) /* Periodic date interrupt */
#define ICU_EVENT_RTCWB_HOUR_IRQ           (0x02F) /* Periodic hour interrupt */
#define ICU_EVENT_RTCWB_MIN_IRQ            (0x030) /* Periodic minute interrupt */
#define ICU_EVENT_RTCWB_SEC_IRQ            (0x031) /* Periodic second interrupt */
#define ICU_EVENT_RTCWB_HOS_IRQ            (0x032) /* Periodic hundredths of second interrupt */
#define ICU_EVENT_RTCWB_RO_IRQ             (0x033) /* RTC rollover interrupt */
#define ICU_EVENT_RTCWB_CAP_IRQ            (0x034) /* RTC timer capture interrupt */
#define ICU_EVENT_TIMW1_IRQ                (0x035) /* Generic timer 1 IRQ */
#define ICU_EVENT_TIMW1_CCMA_IRQ           (0x036) /* Capture or Compare match channel A */
#define ICU_EVENT_TIMW1_CCMB_IRQ           (0x037) /* Capture or Compare match channel B */
#define ICU_EVENT_TIMW1_CCMC_IRQ           (0x038) /* Capture or Compare match channel C */
#define ICU_EVENT_TIMW1_CCMD_IRQ           (0x039) /* Capture or Compare match channel D */
#define ICU_EVENT_TIMW1_CCME_IRQ           (0x03A) /* Capture or Compare match channel E */
#define ICU_EVENT_TIMW1_CCMF_IRQ           (0x03B) /* Capture or Compare match channel F */
#define ICU_EVENT_TIMW1_CCMG_IRQ           (0x03C) /* Capture or Compare match channel G */
#define ICU_EVENT_TIMW1_CCMH_IRQ           (0x03D) /* Capture or Compare match channel H */
#define ICU_EVENT_TIMW1_OVF_IRQ            (0x03E) /* Overflow */
#define ICU_EVENT_TIMW1_UNF_IRQ            (0x03F) /* Underflow */
#define ICU_EVENT_TIMW2_IRQ                (0x040) /* Generic timer 2 IRQ */
#define ICU_EVENT_TIMW2_CCMA_IRQ           (0x041) /* Capture or Compare match channel A */
#define ICU_EVENT_TIMW2_CCMB_IRQ           (0x042) /* Capture or Compare match channel B */
#define ICU_EVENT_TIMW2_CCMC_IRQ           (0x043) /* Capture or Compare match channel C */
#define ICU_EVENT_TIMW2_CCMD_IRQ           (0x044) /* Capture or Compare match channel D */
#define ICU_EVENT_TIMW2_CCME_IRQ           (0x045) /* Capture or Compare match channel E */
#define ICU_EVENT_TIMW2_CCMF_IRQ           (0x046) /* Capture or Compare match channel F */
#define ICU_EVENT_TIMW2_CCMG_IRQ           (0x047) /* Capture or Compare match channel G */
#define ICU_EVENT_TIMW2_CCMH_IRQ           (0x048) /* Capture or Compare match channel H */
#define ICU_EVENT_TIMW2_OVF_IRQ            (0x049) /* Overflow */
#define ICU_EVENT_TIMW2_UNF_IRQ            (0x04A) /* Underflow */
#define ICU_EVENT_TIMW3_IRQ                (0x04B) /* Generic timer 3 IRQ */
#define ICU_EVENT_TIMW3_CCMA_IRQ           (0x04C) /* Capture or Compare match channel A */
#define ICU_EVENT_TIMW3_CCMB_IRQ           (0x04D) /* Capture or Compare match channel B */
#define ICU_EVENT_TIMW3_CCMC_IRQ           (0x04E) /* Capture or Compare match channel C */
#define ICU_EVENT_TIMW3_CCMD_IRQ           (0x04F) /* Capture or Compare match channel D */
#define ICU_EVENT_TIMW3_CCME_IRQ           (0x050) /* Capture or Compare match channel E */
#define ICU_EVENT_TIMW3_CCMF_IRQ           (0x051) /* Capture or Compare match channel F */
#define ICU_EVENT_TIMW3_CCMG_IRQ           (0x052) /* Capture or Compare match channel G */
#define ICU_EVENT_TIMW3_CCMH_IRQ           (0x053) /* Capture or Compare match channel H */
#define ICU_EVENT_TIMW3_OVF_IRQ            (0x054) /* Overflow */
#define ICU_EVENT_TIMW3_UNF_IRQ            (0x055) /* Underflow */
#define ICU_EVENT_TIMW4_IRQ                (0x056) /* Generic timer 4 IRQ */
#define ICU_EVENT_TIMW4_CCMA_IRQ           (0x057) /* Capture or Compare match channel A */
#define ICU_EVENT_TIMW4_CCMB_IRQ           (0x058) /* Capture or Compare match channel B */
#define ICU_EVENT_TIMW4_CCMC_IRQ           (0x059) /* Capture or Compare match channel C */
#define ICU_EVENT_TIMW4_CCMD_IRQ           (0x05A) /* Capture or Compare match channel D */
#define ICU_EVENT_TIMW4_CCME_IRQ           (0x05B) /* Capture or Compare match channel E */
#define ICU_EVENT_TIMW4_CCMF_IRQ           (0x05C) /* Capture or Compare match channel F */
#define ICU_EVENT_TIMW4_CCMG_IRQ           (0x05D) /* Capture or Compare match channel G */
#define ICU_EVENT_TIMW4_CCMH_IRQ           (0x05E) /* Capture or Compare match channel H */
#define ICU_EVENT_TIMW4_OVF_IRQ            (0x05F) /* Overflow */
#define ICU_EVENT_TIMW4_UNF_IRQ            (0x060) /* Underflow */
#define ICU_EVENT_TIMW5_IRQ                (0x061) /* Generic timer 5 IRQ */
#define ICU_EVENT_TIMW5_CCMA_IRQ           (0x062) /* Capture or Compare match channel A */
#define ICU_EVENT_TIMW5_CCMB_IRQ           (0x063) /* Capture or Compare match channel B */
#define ICU_EVENT_TIMW5_CCMC_IRQ           (0x064) /* Capture or Compare match channel C */
#define ICU_EVENT_TIMW5_CCMD_IRQ           (0x065) /* Capture or Compare match channel D */
#define ICU_EVENT_TIMW5_CCME_IRQ           (0x066) /* Capture or Compare match channel E */
#define ICU_EVENT_TIMW5_CCMF_IRQ           (0x067) /* Capture or Compare match channel F */
#define ICU_EVENT_TIMW5_CCMG_IRQ           (0x068) /* Capture or Compare match channel G */
#define ICU_EVENT_TIMW5_CCMH_IRQ           (0x069) /* Capture or Compare match channel H */
#define ICU_EVENT_TIMW5_OVF_IRQ            (0x06A) /* Overflow */
#define ICU_EVENT_TIMW5_UNF_IRQ            (0x06B) /* Underflow */
#define ICU_EVENT_TIMW6_IRQ                (0x06C) /* Generic timer 6 IRQ */
#define ICU_EVENT_TIMW6_CCMA_IRQ           (0x06D) /* Capture or Compare match channel A */
#define ICU_EVENT_TIMW6_CCMB_IRQ           (0x06E) /* Capture or Compare match channel B */
#define ICU_EVENT_TIMW6_CCMC_IRQ           (0x06F) /* Capture or Compare match channel C */
#define ICU_EVENT_TIMW6_CCMD_IRQ           (0x070) /* Capture or Compare match channel D */
#define ICU_EVENT_TIMW6_CCME_IRQ           (0x071) /* Capture or Compare match channel E */
#define ICU_EVENT_TIMW6_CCMF_IRQ           (0x072) /* Capture or Compare match channel F */
#define ICU_EVENT_TIMW6_CCMG_IRQ           (0x073) /* Capture or Compare match channel G */
#define ICU_EVENT_TIMW6_CCMH_IRQ           (0x074) /* Capture or Compare match channel H */
#define ICU_EVENT_TIMW6_OVF_IRQ            (0x075) /* Overflow */
#define ICU_EVENT_TIMW6_UNF_IRQ            (0x076) /* Underflow */
#define ICU_EVENT_TIMW7_IRQ                (0x077) /* Generic timer 7 IRQ */
#define ICU_EVENT_TIMW7_CCMA_IRQ           (0x078) /* Capture or Compare match channel A */
#define ICU_EVENT_TIMW7_CCMB_IRQ           (0x079) /* Capture or Compare match channel B */
#define ICU_EVENT_TIMW7_CCMC_IRQ           (0x07A) /* Capture or Compare match channel C */
#define ICU_EVENT_TIMW7_CCMD_IRQ           (0x07B) /* Capture or Compare match channel D */
#define ICU_EVENT_TIMW7_CCME_IRQ           (0x07C) /* Capture or Compare match channel E */
#define ICU_EVENT_TIMW7_CCMF_IRQ           (0x07D) /* Capture or Compare match channel F */
#define ICU_EVENT_TIMW7_CCMG_IRQ           (0x07E) /* Capture or Compare match channel G */
#define ICU_EVENT_TIMW7_CCMH_IRQ           (0x07F) /* Capture or Compare match channel H */
#define ICU_EVENT_TIMW7_OVF_IRQ            (0x080) /* Overflow */
#define ICU_EVENT_TIMW7_UNF_IRQ            (0x081) /* Underflow */
#define ICU_EVENT_TIMW8_IRQ                (0x082) /* Generic timer 8 IRQ */
#define ICU_EVENT_TIMW8_CCMA_IRQ           (0x083) /* Capture or Compare match channel A */
#define ICU_EVENT_TIMW8_CCMB_IRQ           (0x084) /* Capture or Compare match channel B */
#define ICU_EVENT_TIMW8_CCMC_IRQ           (0x085) /* Capture or Compare match channel C */
#define ICU_EVENT_TIMW8_CCMD_IRQ           (0x086) /* Capture or Compare match channel D */
#define ICU_EVENT_TIMW8_CCME_IRQ           (0x087) /* Capture or Compare match channel E */
#define ICU_EVENT_TIMW8_CCMF_IRQ           (0x088) /* Capture or Compare match channel F */
#define ICU_EVENT_TIMW8_CCMG_IRQ           (0x089) /* Capture or Compare match channel G */
#define ICU_EVENT_TIMW8_CCMH_IRQ           (0x08A) /* Capture or Compare match channel H */
#define ICU_EVENT_TIMW8_OVF_IRQ            (0x08B) /* Overflow */
#define ICU_EVENT_TIMW8_UNF_IRQ            (0x08C) /* Underflow */
#define ICU_EVENT_TIMW9_IRQ                (0x08D) /* Generic timer 9 IRQ */
#define ICU_EVENT_TIMW9_CCMA_IRQ           (0x08E) /* Capture or Compare match channel A */
#define ICU_EVENT_TIMW9_CCMB_IRQ           (0x08F) /* Capture or Compare match channel B */
#define ICU_EVENT_TIMW9_CCMC_IRQ           (0x090) /* Capture or Compare match channel C */
#define ICU_EVENT_TIMW9_CCMD_IRQ           (0x091) /* Capture or Compare match channel D */
#define ICU_EVENT_TIMW9_CCME_IRQ           (0x092) /* Capture or Compare match channel E */
#define ICU_EVENT_TIMW9_CCMF_IRQ           (0x093) /* Capture or Compare match channel F */
#define ICU_EVENT_TIMW9_CCMG_IRQ           (0x094) /* Capture or Compare match channel G */
#define ICU_EVENT_TIMW9_CCMH_IRQ           (0x095) /* Capture or Compare match channel H */
#define ICU_EVENT_TIMW9_OVF_IRQ            (0x096) /* Overflow */
#define ICU_EVENT_TIMW9_UNF_IRQ            (0x097) /* Underflow */
#define ICU_EVENT_TIMW10_IRQ               (0x098) /* Generic timer 10 IRQ */
#define ICU_EVENT_TIMW10_CCMA_IRQ          (0x099) /* Capture or Compare match channel A */
#define ICU_EVENT_TIMW10_CCMB_IRQ          (0x09A) /* Capture or Compare match channel B */
#define ICU_EVENT_TIMW10_CCMC_IRQ          (0x09B) /* Capture or Compare match channel C */
#define ICU_EVENT_TIMW10_CCMD_IRQ          (0x09C) /* Capture or Compare match channel D */
#define ICU_EVENT_TIMW10_CCME_IRQ          (0x09D) /* Capture or Compare match channel E */
#define ICU_EVENT_TIMW10_CCMF_IRQ          (0x09E) /* Capture or Compare match channel F */
#define ICU_EVENT_TIMW10_CCMG_IRQ          (0x09F) /* Capture or Compare match channel G */
#define ICU_EVENT_TIMW10_CCMH_IRQ          (0x0A0) /* Capture or Compare match channel H */
#define ICU_EVENT_TIMW10_OVF_IRQ           (0x0A1) /* Overflow */
#define ICU_EVENT_TIMW10_UNF_IRQ           (0x0A2) /* Underflow */
#define ICU_EVENT_UARTWB1_RX_IRQ           (0x0A3) /* Receive data full */
#define ICU_EVENT_UARTWB1_TXE_IRQ          (0x0A4) /* Transmit data empty */
#define ICU_EVENT_UARTWB1_TXR_IRQ          (0x0A5) /* Transmit end */
#define ICU_EVENT_UARTWB1_IRQ              (0x0A6) /* UART1 generic interrupt */
#define ICU_EVENT_UARTWB2_RX_IRQ           (0x0A7) /* Receive data full */
#define ICU_EVENT_UARTWB2_TXE_IRQ          (0x0A8) /* Transmit data empty */
#define ICU_EVENT_UARTWB2_TXR_IRQ          (0x0A9) /* Transmit end */
#define ICU_EVENT_UARTWB2_IRQ              (0x0AA) /* UART2 generic interrupt */
#define ICU_EVENT_UARTWB3_RX_IRQ           (0x0AB) /* Receive data full */
#define ICU_EVENT_UARTWB3_TXE_IRQ          (0x0AC) /* Transmit data empty */
#define ICU_EVENT_UARTWB3_TXR_IRQ          (0x0AD) /* Transmit end */
#define ICU_EVENT_UARTWB3_IRQ              (0x0AE) /* UART3 generic interrupt */
#define ICU_EVENT_UARTWB4_RX_IRQ           (0x0AF) /* Receive data full */
#define ICU_EVENT_UARTWB4_TXE_IRQ          (0x0B0) /* Transmit data empty */
#define ICU_EVENT_UARTWB4_TXR_IRQ          (0x0B1) /* Transmit end */
#define ICU_EVENT_UARTWB4_IRQ              (0x0B2) /* UART4 generic interrupt */
#define ICU_EVENT_SPIW1_RX_IRQ             (0x0B3) /* Receive buffer full */
#define ICU_EVENT_SPIW1_TX_IRQ             (0x0B4) /* Transmit buffer empty */
#define ICU_EVENT_SPIW1_EI_IRQ             (0x0B5) /* Error */
#define ICU_EVENT_SPIW1_II_IRQ             (0x0B6) /* Idle */
#define ICU_EVENT_SPIW1_IRQ                (0x0B7) /* SPI1 generic interrupt */
#define ICU_EVENT_SPIW2_RX_IRQ             (0x0B8) /* Receive buffer full */
#define ICU_EVENT_SPIW2_TX_IRQ             (0x0B9) /* Transmit buffer empty */
#define ICU_EVENT_SPIW2_EI_IRQ             (0x0BA) /* Error */
#define ICU_EVENT_SPIW2_II_IRQ             (0x0BB) /* Idle */
#define ICU_EVENT_SPIW2_IRQ                (0x0BC) /* SPI2 generic interrupt */
#define ICU_EVENT_SPIW3_RX_IRQ             (0x0BD) /* Receive buffer full */
#define ICU_EVENT_SPIW3_TX_IRQ             (0x0BE) /* Transmit buffer empty */
#define ICU_EVENT_SPIW3_EI_IRQ             (0x0BF) /* Error */
#define ICU_EVENT_SPIW3_II_IRQ             (0x0C0) /* Idle */
#define ICU_EVENT_SPIW3_IRQ                (0x0C1) /* SPI3 generic interrupt */
#define ICU_EVENT_I2CW1_RX_IRQ             (0x0C2) /* Receive buffer full */
#define ICU_EVENT_I2CW1_TXE_IRQ            (0x0C3) /* Transmit buffer empty */
#define ICU_EVENT_I2CW1_TXR_IRQ            (0x0C4) /* Transmit end */
#define ICU_EVENT_I2CW1_IRQ                (0x0C5) /* I2C1 generic interrupt */
#define ICU_EVENT_I2CW2_RX_IRQ             (0x0C6) /* Receive buffer full */
#define ICU_EVENT_I2CW2_TXE_IRQ            (0x0C7) /* Transmit buffer empty */
#define ICU_EVENT_I2CW2_TXR_IRQ            (0x0C8) /* Transmit end */
#define ICU_EVENT_I2CW2_IRQ                (0x0C9) /* I2C2 generic interrupt */
#define ICU_EVENT_I2CW3_RX_IRQ             (0x0CA) /* Receive buffer full */
#define ICU_EVENT_I2CW3_TXE_IRQ            (0x0CB) /* Transmit buffer empty */
#define ICU_EVENT_I2CW3_TXR_IRQ            (0x0CC) /* Transmit end */
#define ICU_EVENT_I2CW3_IRQ                (0x0CD) /* I2C3 generic interrupt */
#define ICU_EVENT_I3CW_RX_IRQ              (0x0CE) /* Receive buffer full */
#define ICU_EVENT_I3CW_TXE_IRQ             (0x0CF) /* Transmit buffer empty */
#define ICU_EVENT_I3CW_TXR_IRQ             (0x0D0) /* Transmit end */
#define ICU_EVENT_I3CW_IRQ                 (0x0D1) /* I3C generic interrupt */
#define ICU_EVENT_I3CW_IB_IRQ              (0x0D2) /* Inband interrupt received */
#define ICU_EVENT_IRGEN_IRQ                (0x0D3) /* IRGEN transmit completed */
#define ICU_EVENT_ADCWB_IRQ                (0x0D4) /* Conversion completed */
#define ICU_EVENT_ADCWB_CC0U_IRQ           (0x0D5) /* ADC Compare Channel 0 \
						    * Upper threshold exceed \
						    */
#define ICU_EVENT_ADCWB_CC0L_IRQ           (0x0D6) /* ADC Compare Channel 0 \
						    * Lower threshold exceed \
						    */
#define ICU_EVENT_ADCWB_CC0W_IRQ           (0x0D7) /* ADC Compare Channel 0 \
						    * within threshold limits \
						    */
#define ICU_EVENT_ADCWB_CC1U_IRQ           (0x0D8) /* ADC Compare Channel 1 \
						    * Upper threshold exceed \
						    */
#define ICU_EVENT_ADCWB_CC1L_IRQ           (0x0D9) /* ADC Compare Channel 1 \
						    * Lower threshold exceed \
						    */
#define ICU_EVENT_ADCWB_CC1W_IRQ           (0x0DA) /* ADC Compare Channel 1 \
						    * within threshold limits \
						    */
#define ICU_EVENT_ADCWB_CC2U_IRQ           (0x0DB) /* ADC Compare Channel 2 \
						    * Upper threshold exceed \
						    */
#define ICU_EVENT_ADCWB_CC2L_IRQ           (0x0DC) /* ADC Compare Channel 2 \
						    * Lower threshold exceed \
						    */
#define ICU_EVENT_ADCWB_CC2W_IRQ           (0x0DD) /* ADC Compare Channel 2 \
						    * within threshold limits \
						    */
#define ICU_EVENT_ADCWB_CC3U_IRQ           (0x0DE) /* ADC Compare Channel 3 \
						    * Upper threshold exceed \
						    */
#define ICU_EVENT_ADCWB_CC3L_IRQ           (0x0DF) /* ADC Compare Channel 3 \
						    * Lower threshold exceed \
						    */
#define ICU_EVENT_ADCWB_CC3W_IRQ           (0x0E0) /* ADC Compare Channel 3 \
						    * within threshold limits \
						    */
#define ICU_EVENT_ADCWB_CC4U_IRQ           (0x0E1) /* ADC Compare Channel 4 \
						    * Upper threshold exceed \
						    */
#define ICU_EVENT_ADCWB_CC4L_IRQ           (0x0E2) /* ADC Compare Channel 4 \
						    * Lower threshold exceed \
						    */
#define ICU_EVENT_ADCWB_CC4W_IRQ           (0x0E3) /* ADC Compare Channel 4 \
						    * within threshold limits \
						    */
#define ICU_EVENT_ADCWB_CC5U_IRQ           (0x0E4) /* ADC Compare Channel 5 \
						    * Upper threshold exceed \
						    */
#define ICU_EVENT_ADCWB_CC5L_IRQ           (0x0E5) /* ADC Compare Channel 5 \
						    * Lower threshold exceed \
						    */
#define ICU_EVENT_ADCWB_CC5W_IRQ           (0x0E6) /* ADC Compare Channel 5 \
						    * within threshold limits \
						    */
#define ICU_EVENT_ADCWB_CC6U_IRQ           (0x0E7) /* ADC Compare Channel 6 \
						    * Upper threshold exceed \
						    */
#define ICU_EVENT_ADCWB_CC6L_IRQ           (0x0E8) /* ADC Compare Channel 6 \
						    * Lower threshold exceed \
						    */
#define ICU_EVENT_ADCWB_CC6W_IRQ           (0x0E9) /* ADC Compare Channel 6 \
						    * within threshold limits \
						    */
#define ICU_EVENT_ADCWB_CC7U_IRQ           (0x0EA) /* ADC Compare Channel 7 \
						    * Upper threshold exceed \
						    */
#define ICU_EVENT_ADCWB_CC7L_IRQ           (0x0EB) /* ADC Compare Channel 7 \
						    * Lower threshold exceed \
						    */
#define ICU_EVENT_ADCWB_CC7W_IRQ           (0x0EC) /* ADC Compare Channel 7 \
						    * within threshold limits \
						    */
#define ICU_EVENT_ACOMPW_IRQ               (0x0ED) /* Analog comparator interrupt */
#define ICU_EVENT_TEMPSEN_END_IRQ          (0x0EE) /* End of temperature sensor measurement */
#define ICU_EVENT_TEMPSEN_THR_IRQ          (0x0EF) /* Threshold matched */
#define ICU_EVENT_VBATMON_THR_IRQ          (0x0F1) /* Threshold matched */
#define ICU_EVENT_KBSCNW_IRQ               (0x0F2) /* Keyboard scanner interrupt */
#define ICU_EVENT_QDECW1_IRQ               (0x0F3) /* Quadrature Decoder 1 interrupt */
#define ICU_EVENT_QDECW2_IRQ               (0x0F4) /* Quadrature Decoder 2 interrupt */
#define ICU_EVENT_MOA_IRQ                  (0x0F5) /* Operation end */
#define ICU_EVENT_USB_IRQ                  (0x0F7) /* USB interrupt */
#define ICU_EVENT_VUSB_IRQ                 (0x0F8) /* USB present */
#define ICU_EVENT_RSIP_IRQ                 (0x0F9) /* Renesas security IP interrupt */
#define ICU_EVENT_DAI_IRQ                  (0x0FA) /* Digital audio interface interrupt */
#define ICU_EVENT_SRC_IN_IRQ               (0x0FC) /* Sample rate converter input line interrupt */
#define ICU_EVENT_SRC_OUT_IRQ              (0x0FD) /* Sample rate converter output line interrupt */
#define ICU_EVENT_DOCW_DOPCI               (0x0FE) /* Data operation circuit interrupt */
#define ICU_EVENT_MRM_IRQ                  (0x0FF) /* Cache miss rate monitor interrupt */
#define ICU_EVENT_DBG_SYSCPU_CTI0_IRQ      (0x100) /* Debugger Cross Trigger \
						    * Interface 0 interrupt \
						    */
#define ICU_EVENT_DBG_SYSCPU_CTI1_IRQ      (0x101) /* Debugger Cross Trigger \
						    * Interface 1 interrupt \
						    */
#define ICU_EVENT_ARP2SYS_IRQ              (0x102) /* ARP to SYSCPU interrupt */
#define ICU_EVENT_CAN_RXF                  (0x103) /* CAN RX FIFO Receive */
#define ICU_EVENT_CAN_GLERR                (0x104) /* CAN Global Error */
#define ICU_EVENT_CAN_TX                   (0x105) /* CAN Transmit End */
#define ICU_EVENT_CAN_CHERR                (0x106) /* CAN Channel Error */
#define ICU_EVENT_CAN_COMFRX               (0x107) /* CAN Channel COM FIFO RX interrupt */
#define ICU_EVENT_CAN_RXMB                 (0x108) /* CAN RXMB Receive */
#define ICU_EVENT_CAN_MRAM_ERI             (0x109) /* CAN ECC Error or ECC Error Overflow */

/** @endcond */

/** @} */

#endif /* ZEPHYR_DT_BINDINGS_INTERRUPT_CONTROLLER_RENESAS_RA_ICU_EVENT_H_ */
