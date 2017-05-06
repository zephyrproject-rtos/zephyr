/* ---------------------------------------------------------------------------- */
/*                  Atmel Microcontroller Software Support                      */
/*                       SAM Software Package License                           */
/* ---------------------------------------------------------------------------- */
/* Copyright (c) %copyright_year%, Atmel Corporation                                        */
/*                                                                              */
/* All rights reserved.                                                         */
/*                                                                              */
/* Redistribution and use in source and binary forms, with or without           */
/* modification, are permitted provided that the following condition is met:    */
/*                                                                              */
/* - Redistributions of source code must retain the above copyright notice,     */
/* this list of conditions and the disclaimer below.                            */
/*                                                                              */
/* Atmel's name may not be used to endorse or promote products derived from     */
/* this software without specific prior written permission.                     */
/*                                                                              */
/* DISCLAIMER:  THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR   */
/* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE   */
/* DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,      */
/* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT */
/* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,  */
/* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF    */
/* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING         */
/* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, */
/* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                           */
/* ---------------------------------------------------------------------------- */

#ifndef _SAM4S_SUPC_COMPONENT_
#define _SAM4S_SUPC_COMPONENT_

/* ============================================================================= */
/**  SOFTWARE API DEFINITION FOR Supply Controller */
/* ============================================================================= */
/** \addtogroup SAM4S_SUPC Supply Controller */
/*@{*/

#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
/** \brief Supc hardware registers */
typedef struct {
  __O  uint32_t SUPC_CR;   /**< \brief (Supc Offset: 0x00) Supply Controller Control Register */
  __IO uint32_t SUPC_SMMR; /**< \brief (Supc Offset: 0x04) Supply Controller Supply Monitor Mode Register */
  __IO uint32_t SUPC_MR;   /**< \brief (Supc Offset: 0x08) Supply Controller Mode Register */
  __IO uint32_t SUPC_WUMR; /**< \brief (Supc Offset: 0x0C) Supply Controller Wake-up Mode Register */
  __IO uint32_t SUPC_WUIR; /**< \brief (Supc Offset: 0x10) Supply Controller Wake-up Inputs Register */
  __I  uint32_t SUPC_SR;   /**< \brief (Supc Offset: 0x14) Supply Controller Status Register */
} Supc;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- SUPC_CR : (SUPC Offset: 0x00) Supply Controller Control Register -------- */
#define SUPC_CR_VROFF (0x1u << 2) /**< \brief (SUPC_CR) Voltage Regulator Off */
#define   SUPC_CR_VROFF_NO_EFFECT (0x0u << 2) /**< \brief (SUPC_CR) No effect. */
#define   SUPC_CR_VROFF_STOP_VREG (0x1u << 2) /**< \brief (SUPC_CR) If KEY is correct, VROFF asserts the vddcore_nreset and stops the voltage regulator. */
#define SUPC_CR_XTALSEL (0x1u << 3) /**< \brief (SUPC_CR) Crystal Oscillator Select */
#define   SUPC_CR_XTALSEL_NO_EFFECT (0x0u << 3) /**< \brief (SUPC_CR) No effect. */
#define   SUPC_CR_XTALSEL_CRYSTAL_SEL (0x1u << 3) /**< \brief (SUPC_CR) If KEY is correct, XTALSEL switches the slow clock on the crystal oscillator output. */
#define SUPC_CR_KEY_Pos 24
#define SUPC_CR_KEY_Msk (0xffu << SUPC_CR_KEY_Pos) /**< \brief (SUPC_CR) Password */
#define SUPC_CR_KEY(value) ((SUPC_CR_KEY_Msk & ((value) << SUPC_CR_KEY_Pos)))
#define   SUPC_CR_KEY_PASSWD (0xA5u << 24) /**< \brief (SUPC_CR) Writing any other value in this field aborts the write operation. */
/* -------- SUPC_SMMR : (SUPC Offset: 0x04) Supply Controller Supply Monitor Mode Register -------- */
#define SUPC_SMMR_SMTH_Pos 0
#define SUPC_SMMR_SMTH_Msk (0xfu << SUPC_SMMR_SMTH_Pos) /**< \brief (SUPC_SMMR) Supply Monitor Threshold */
#define SUPC_SMMR_SMTH(value) ((SUPC_SMMR_SMTH_Msk & ((value) << SUPC_SMMR_SMTH_Pos)))
#define SUPC_SMMR_SMSMPL_Pos 8
#define SUPC_SMMR_SMSMPL_Msk (0x7u << SUPC_SMMR_SMSMPL_Pos) /**< \brief (SUPC_SMMR) Supply Monitor Sampling Period */
#define SUPC_SMMR_SMSMPL(value) ((SUPC_SMMR_SMSMPL_Msk & ((value) << SUPC_SMMR_SMSMPL_Pos)))
#define   SUPC_SMMR_SMSMPL_SMD (0x0u << 8) /**< \brief (SUPC_SMMR) Supply Monitor disabled */
#define   SUPC_SMMR_SMSMPL_CSM (0x1u << 8) /**< \brief (SUPC_SMMR) Continuous Supply Monitor */
#define   SUPC_SMMR_SMSMPL_32SLCK (0x2u << 8) /**< \brief (SUPC_SMMR) Supply Monitor enabled one SLCK period every 32 SLCK periods */
#define   SUPC_SMMR_SMSMPL_256SLCK (0x3u << 8) /**< \brief (SUPC_SMMR) Supply Monitor enabled one SLCK period every 256 SLCK periods */
#define   SUPC_SMMR_SMSMPL_2048SLCK (0x4u << 8) /**< \brief (SUPC_SMMR) Supply Monitor enabled one SLCK period every 2,048 SLCK periods */
#define SUPC_SMMR_SMRSTEN (0x1u << 12) /**< \brief (SUPC_SMMR) Supply Monitor Reset Enable */
#define   SUPC_SMMR_SMRSTEN_NOT_ENABLE (0x0u << 12) /**< \brief (SUPC_SMMR) The core reset signal vddcore_nreset is not affected when a supply monitor detection occurs. */
#define   SUPC_SMMR_SMRSTEN_ENABLE (0x1u << 12) /**< \brief (SUPC_SMMR) The core reset signal, vddcore_nreset is asserted when a supply monitor detection occurs. */
#define SUPC_SMMR_SMIEN (0x1u << 13) /**< \brief (SUPC_SMMR) Supply Monitor Interrupt Enable */
#define   SUPC_SMMR_SMIEN_NOT_ENABLE (0x0u << 13) /**< \brief (SUPC_SMMR) The SUPC interrupt signal is not affected when a supply monitor detection occurs. */
#define   SUPC_SMMR_SMIEN_ENABLE (0x1u << 13) /**< \brief (SUPC_SMMR) The SUPC interrupt signal is asserted when a supply monitor detection occurs. */
/* -------- SUPC_MR : (SUPC Offset: 0x08) Supply Controller Mode Register -------- */
#define SUPC_MR_BODRSTEN (0x1u << 12) /**< \brief (SUPC_MR) Brownout Detector Reset Enable */
#define   SUPC_MR_BODRSTEN_NOT_ENABLE (0x0u << 12) /**< \brief (SUPC_MR) The core reset signal vddcore_nreset is not affected when a brownout detection occurs. */
#define   SUPC_MR_BODRSTEN_ENABLE (0x1u << 12) /**< \brief (SUPC_MR) The core reset signal, vddcore_nreset is asserted when a brownout detection occurs. */
#define SUPC_MR_BODDIS (0x1u << 13) /**< \brief (SUPC_MR) Brownout Detector Disable */
#define   SUPC_MR_BODDIS_ENABLE (0x0u << 13) /**< \brief (SUPC_MR) The core brownout detector is enabled. */
#define   SUPC_MR_BODDIS_DISABLE (0x1u << 13) /**< \brief (SUPC_MR) The core brownout detector is disabled. */
#define SUPC_MR_ONREG (0x1u << 14) /**< \brief (SUPC_MR) Voltage Regulator Enable */
#define   SUPC_MR_ONREG_ONREG_UNUSED (0x0u << 14) /**< \brief (SUPC_MR) Internal voltage regulator is not used (external power supply is used). */
#define   SUPC_MR_ONREG_ONREG_USED (0x1u << 14) /**< \brief (SUPC_MR) Internal voltage regulator is used. */
#define SUPC_MR_OSCBYPASS (0x1u << 20) /**< \brief (SUPC_MR) Oscillator Bypass */
#define   SUPC_MR_OSCBYPASS_NO_EFFECT (0x0u << 20) /**< \brief (SUPC_MR) No effect. Clock selection depends on the value of XTALSEL (SUPC_CR). */
#define   SUPC_MR_OSCBYPASS_BYPASS (0x1u << 20) /**< \brief (SUPC_MR) The 32 kHz crystal oscillator is bypassed if XTALSEL (SUPC_CR) is set. OSCBYPASS must be set prior to setting XTALSEL. */
#define SUPC_MR_KEY_Pos 24
#define SUPC_MR_KEY_Msk (0xffu << SUPC_MR_KEY_Pos) /**< \brief (SUPC_MR) Password Key */
#define SUPC_MR_KEY(value) ((SUPC_MR_KEY_Msk & ((value) << SUPC_MR_KEY_Pos)))
#define   SUPC_MR_KEY_PASSWD (0xA5u << 24) /**< \brief (SUPC_MR) Writing any other value in this field aborts the write operation. */
/* -------- SUPC_WUMR : (SUPC Offset: 0x0C) Supply Controller Wake-up Mode Register -------- */
#define SUPC_WUMR_SMEN (0x1u << 1) /**< \brief (SUPC_WUMR) Supply Monitor Wake-up Enable */
#define   SUPC_WUMR_SMEN_NOT_ENABLE (0x0u << 1) /**< \brief (SUPC_WUMR) The supply monitor detection has no wake-up effect. */
#define   SUPC_WUMR_SMEN_ENABLE (0x1u << 1) /**< \brief (SUPC_WUMR) The supply monitor detection forces the wake-up of the core power supply. */
#define SUPC_WUMR_RTTEN (0x1u << 2) /**< \brief (SUPC_WUMR) Real-time Timer Wake-up Enable */
#define   SUPC_WUMR_RTTEN_NOT_ENABLE (0x0u << 2) /**< \brief (SUPC_WUMR) The RTT alarm signal has no wake-up effect. */
#define   SUPC_WUMR_RTTEN_ENABLE (0x1u << 2) /**< \brief (SUPC_WUMR) The RTT alarm signal forces the wake-up of the core power supply. */
#define SUPC_WUMR_RTCEN (0x1u << 3) /**< \brief (SUPC_WUMR) Real-time Clock Wake-up Enable */
#define   SUPC_WUMR_RTCEN_NOT_ENABLE (0x0u << 3) /**< \brief (SUPC_WUMR) The RTC alarm signal has no wake-up effect. */
#define   SUPC_WUMR_RTCEN_ENABLE (0x1u << 3) /**< \brief (SUPC_WUMR) The RTC alarm signal forces the wake-up of the core power supply. */
#define SUPC_WUMR_LPDBCEN0 (0x1u << 5) /**< \brief (SUPC_WUMR) Low-power Debouncer Enable WKUP0 */
#define   SUPC_WUMR_LPDBCEN0_NOT_ENABLE (0x0u << 5) /**< \brief (SUPC_WUMR) The WKUP0 input pin is not connected to the low-power debouncer. */
#define   SUPC_WUMR_LPDBCEN0_ENABLE (0x1u << 5) /**< \brief (SUPC_WUMR) The WKUP0 input pin is connected to the low-power debouncer and forces a system wake-up. */
#define SUPC_WUMR_LPDBCEN1 (0x1u << 6) /**< \brief (SUPC_WUMR) Low-power Debouncer Enable WKUP1 */
#define   SUPC_WUMR_LPDBCEN1_NOT_ENABLE (0x0u << 6) /**< \brief (SUPC_WUMR) The WKUP1 input pin is not connected to the low-power debouncer. */
#define   SUPC_WUMR_LPDBCEN1_ENABLE (0x1u << 6) /**< \brief (SUPC_WUMR) The WKUP1 input pin is connected to the low-power debouncer and forces a system wake-up. */
#define SUPC_WUMR_LPDBCCLR (0x1u << 7) /**< \brief (SUPC_WUMR) Low-power Debouncer Clear */
#define   SUPC_WUMR_LPDBCCLR_NOT_ENABLE (0x0u << 7) /**< \brief (SUPC_WUMR) A low-power debounce event does not create an immediate clear on the first half of GPBR registers. */
#define   SUPC_WUMR_LPDBCCLR_ENABLE (0x1u << 7) /**< \brief (SUPC_WUMR) A low-power debounce event on WKUP0 or WKUP1 generates an immediate clear on the first half of GPBR registers. */
#define SUPC_WUMR_WKUPDBC_Pos 12
#define SUPC_WUMR_WKUPDBC_Msk (0x7u << SUPC_WUMR_WKUPDBC_Pos) /**< \brief (SUPC_WUMR) Wake-up Inputs Debouncer Period */
#define SUPC_WUMR_WKUPDBC(value) ((SUPC_WUMR_WKUPDBC_Msk & ((value) << SUPC_WUMR_WKUPDBC_Pos)))
#define   SUPC_WUMR_WKUPDBC_IMMEDIATE (0x0u << 12) /**< \brief (SUPC_WUMR) Immediate, no debouncing, detected active at least on one Slow Clock edge. */
#define   SUPC_WUMR_WKUPDBC_3_SCLK (0x1u << 12) /**< \brief (SUPC_WUMR) WKUPx shall be in its active state for at least 3 SLCK periods */
#define   SUPC_WUMR_WKUPDBC_32_SCLK (0x2u << 12) /**< \brief (SUPC_WUMR) WKUPx shall be in its active state for at least 32 SLCK periods */
#define   SUPC_WUMR_WKUPDBC_512_SCLK (0x3u << 12) /**< \brief (SUPC_WUMR) WKUPx shall be in its active state for at least 512 SLCK periods */
#define   SUPC_WUMR_WKUPDBC_4096_SCLK (0x4u << 12) /**< \brief (SUPC_WUMR) WKUPx shall be in its active state for at least 4,096 SLCK periods */
#define   SUPC_WUMR_WKUPDBC_32768_SCLK (0x5u << 12) /**< \brief (SUPC_WUMR) WKUPx shall be in its active state for at least 32,768 SLCK periods */
#define SUPC_WUMR_LPDBC_Pos 16
#define SUPC_WUMR_LPDBC_Msk (0x7u << SUPC_WUMR_LPDBC_Pos) /**< \brief (SUPC_WUMR) Low-power Debouncer Period */
#define SUPC_WUMR_LPDBC(value) ((SUPC_WUMR_LPDBC_Msk & ((value) << SUPC_WUMR_LPDBC_Pos)))
#define   SUPC_WUMR_LPDBC_DISABLE (0x0u << 16) /**< \brief (SUPC_WUMR) Disable the low-power debouncers. */
#define   SUPC_WUMR_LPDBC_2_RTCOUT0 (0x1u << 16) /**< \brief (SUPC_WUMR) WKUP0/1 in active state for at least 2 RTCOUTx clock periods */
#define   SUPC_WUMR_LPDBC_3_RTCOUT0 (0x2u << 16) /**< \brief (SUPC_WUMR) WKUP0/1 in active state for at least 3 RTCOUTx clock periods */
#define   SUPC_WUMR_LPDBC_4_RTCOUT0 (0x3u << 16) /**< \brief (SUPC_WUMR) WKUP0/1 in active state for at least 4 RTCOUTx clock periods */
#define   SUPC_WUMR_LPDBC_5_RTCOUT0 (0x4u << 16) /**< \brief (SUPC_WUMR) WKUP0/1 in active state for at least 5 RTCOUTx clock periods */
#define   SUPC_WUMR_LPDBC_6_RTCOUT0 (0x5u << 16) /**< \brief (SUPC_WUMR) WKUP0/1 in active state for at least 6 RTCOUTx clock periods */
#define   SUPC_WUMR_LPDBC_7_RTCOUT0 (0x6u << 16) /**< \brief (SUPC_WUMR) WKUP0/1 in active state for at least 7 RTCOUTx clock periods */
#define   SUPC_WUMR_LPDBC_8_RTCOUT0 (0x7u << 16) /**< \brief (SUPC_WUMR) WKUP0/1 in active state for at least 8 RTCOUTx clock periods */
/* -------- SUPC_WUIR : (SUPC Offset: 0x10) Supply Controller Wake-up Inputs Register -------- */
#define SUPC_WUIR_WKUPEN0 (0x1u << 0) /**< \brief (SUPC_WUIR) Wake-up Input Enable 0 to 0 */
#define   SUPC_WUIR_WKUPEN0_DISABLE (0x0u << 0) /**< \brief (SUPC_WUIR) The corresponding wake-up input has no wake-up effect. */
#define   SUPC_WUIR_WKUPEN0_ENABLE (0x1u << 0) /**< \brief (SUPC_WUIR) The corresponding wake-up input is enabled for a wake-up of the core power supply. */
#define SUPC_WUIR_WKUPEN1 (0x1u << 1) /**< \brief (SUPC_WUIR) Wake-up Input Enable 0 to 1 */
#define   SUPC_WUIR_WKUPEN1_DISABLE (0x0u << 1) /**< \brief (SUPC_WUIR) The corresponding wake-up input has no wake-up effect. */
#define   SUPC_WUIR_WKUPEN1_ENABLE (0x1u << 1) /**< \brief (SUPC_WUIR) The corresponding wake-up input is enabled for a wake-up of the core power supply. */
#define SUPC_WUIR_WKUPEN2 (0x1u << 2) /**< \brief (SUPC_WUIR) Wake-up Input Enable 0 to 2 */
#define   SUPC_WUIR_WKUPEN2_DISABLE (0x0u << 2) /**< \brief (SUPC_WUIR) The corresponding wake-up input has no wake-up effect. */
#define   SUPC_WUIR_WKUPEN2_ENABLE (0x1u << 2) /**< \brief (SUPC_WUIR) The corresponding wake-up input is enabled for a wake-up of the core power supply. */
#define SUPC_WUIR_WKUPEN3 (0x1u << 3) /**< \brief (SUPC_WUIR) Wake-up Input Enable 0 to 3 */
#define   SUPC_WUIR_WKUPEN3_DISABLE (0x0u << 3) /**< \brief (SUPC_WUIR) The corresponding wake-up input has no wake-up effect. */
#define   SUPC_WUIR_WKUPEN3_ENABLE (0x1u << 3) /**< \brief (SUPC_WUIR) The corresponding wake-up input is enabled for a wake-up of the core power supply. */
#define SUPC_WUIR_WKUPEN4 (0x1u << 4) /**< \brief (SUPC_WUIR) Wake-up Input Enable 0 to 4 */
#define   SUPC_WUIR_WKUPEN4_DISABLE (0x0u << 4) /**< \brief (SUPC_WUIR) The corresponding wake-up input has no wake-up effect. */
#define   SUPC_WUIR_WKUPEN4_ENABLE (0x1u << 4) /**< \brief (SUPC_WUIR) The corresponding wake-up input is enabled for a wake-up of the core power supply. */
#define SUPC_WUIR_WKUPEN5 (0x1u << 5) /**< \brief (SUPC_WUIR) Wake-up Input Enable 0 to 5 */
#define   SUPC_WUIR_WKUPEN5_DISABLE (0x0u << 5) /**< \brief (SUPC_WUIR) The corresponding wake-up input has no wake-up effect. */
#define   SUPC_WUIR_WKUPEN5_ENABLE (0x1u << 5) /**< \brief (SUPC_WUIR) The corresponding wake-up input is enabled for a wake-up of the core power supply. */
#define SUPC_WUIR_WKUPEN6 (0x1u << 6) /**< \brief (SUPC_WUIR) Wake-up Input Enable 0 to 6 */
#define   SUPC_WUIR_WKUPEN6_DISABLE (0x0u << 6) /**< \brief (SUPC_WUIR) The corresponding wake-up input has no wake-up effect. */
#define   SUPC_WUIR_WKUPEN6_ENABLE (0x1u << 6) /**< \brief (SUPC_WUIR) The corresponding wake-up input is enabled for a wake-up of the core power supply. */
#define SUPC_WUIR_WKUPEN7 (0x1u << 7) /**< \brief (SUPC_WUIR) Wake-up Input Enable 0 to 7 */
#define   SUPC_WUIR_WKUPEN7_DISABLE (0x0u << 7) /**< \brief (SUPC_WUIR) The corresponding wake-up input has no wake-up effect. */
#define   SUPC_WUIR_WKUPEN7_ENABLE (0x1u << 7) /**< \brief (SUPC_WUIR) The corresponding wake-up input is enabled for a wake-up of the core power supply. */
#define SUPC_WUIR_WKUPEN8 (0x1u << 8) /**< \brief (SUPC_WUIR) Wake-up Input Enable 0 to 8 */
#define   SUPC_WUIR_WKUPEN8_DISABLE (0x0u << 8) /**< \brief (SUPC_WUIR) The corresponding wake-up input has no wake-up effect. */
#define   SUPC_WUIR_WKUPEN8_ENABLE (0x1u << 8) /**< \brief (SUPC_WUIR) The corresponding wake-up input is enabled for a wake-up of the core power supply. */
#define SUPC_WUIR_WKUPEN9 (0x1u << 9) /**< \brief (SUPC_WUIR) Wake-up Input Enable 0 to 9 */
#define   SUPC_WUIR_WKUPEN9_DISABLE (0x0u << 9) /**< \brief (SUPC_WUIR) The corresponding wake-up input has no wake-up effect. */
#define   SUPC_WUIR_WKUPEN9_ENABLE (0x1u << 9) /**< \brief (SUPC_WUIR) The corresponding wake-up input is enabled for a wake-up of the core power supply. */
#define SUPC_WUIR_WKUPEN10 (0x1u << 10) /**< \brief (SUPC_WUIR) Wake-up Input Enable 0 to 10 */
#define   SUPC_WUIR_WKUPEN10_DISABLE (0x0u << 10) /**< \brief (SUPC_WUIR) The corresponding wake-up input has no wake-up effect. */
#define   SUPC_WUIR_WKUPEN10_ENABLE (0x1u << 10) /**< \brief (SUPC_WUIR) The corresponding wake-up input is enabled for a wake-up of the core power supply. */
#define SUPC_WUIR_WKUPEN11 (0x1u << 11) /**< \brief (SUPC_WUIR) Wake-up Input Enable 0 to 11 */
#define   SUPC_WUIR_WKUPEN11_DISABLE (0x0u << 11) /**< \brief (SUPC_WUIR) The corresponding wake-up input has no wake-up effect. */
#define   SUPC_WUIR_WKUPEN11_ENABLE (0x1u << 11) /**< \brief (SUPC_WUIR) The corresponding wake-up input is enabled for a wake-up of the core power supply. */
#define SUPC_WUIR_WKUPEN12 (0x1u << 12) /**< \brief (SUPC_WUIR) Wake-up Input Enable 0 to 12 */
#define   SUPC_WUIR_WKUPEN12_DISABLE (0x0u << 12) /**< \brief (SUPC_WUIR) The corresponding wake-up input has no wake-up effect. */
#define   SUPC_WUIR_WKUPEN12_ENABLE (0x1u << 12) /**< \brief (SUPC_WUIR) The corresponding wake-up input is enabled for a wake-up of the core power supply. */
#define SUPC_WUIR_WKUPEN13 (0x1u << 13) /**< \brief (SUPC_WUIR) Wake-up Input Enable 0 to 13 */
#define   SUPC_WUIR_WKUPEN13_DISABLE (0x0u << 13) /**< \brief (SUPC_WUIR) The corresponding wake-up input has no wake-up effect. */
#define   SUPC_WUIR_WKUPEN13_ENABLE (0x1u << 13) /**< \brief (SUPC_WUIR) The corresponding wake-up input is enabled for a wake-up of the core power supply. */
#define SUPC_WUIR_WKUPEN14 (0x1u << 14) /**< \brief (SUPC_WUIR) Wake-up Input Enable 0 to 14 */
#define   SUPC_WUIR_WKUPEN14_DISABLE (0x0u << 14) /**< \brief (SUPC_WUIR) The corresponding wake-up input has no wake-up effect. */
#define   SUPC_WUIR_WKUPEN14_ENABLE (0x1u << 14) /**< \brief (SUPC_WUIR) The corresponding wake-up input is enabled for a wake-up of the core power supply. */
#define SUPC_WUIR_WKUPEN15 (0x1u << 15) /**< \brief (SUPC_WUIR) Wake-up Input Enable 0 to 15 */
#define   SUPC_WUIR_WKUPEN15_DISABLE (0x0u << 15) /**< \brief (SUPC_WUIR) The corresponding wake-up input has no wake-up effect. */
#define   SUPC_WUIR_WKUPEN15_ENABLE (0x1u << 15) /**< \brief (SUPC_WUIR) The corresponding wake-up input is enabled for a wake-up of the core power supply. */
#define SUPC_WUIR_WKUPT0 (0x1u << 16) /**< \brief (SUPC_WUIR) Wake-up Input Type 0 to 0 */
#define   SUPC_WUIR_WKUPT0_LOW (0x0u << 16) /**< \brief (SUPC_WUIR) A falling edge followed by a low level for a period defined by WKUPDBC on the corre-sponding wake-up input forces the wake-up of the core power supply. */
#define   SUPC_WUIR_WKUPT0_HIGH (0x1u << 16) /**< \brief (SUPC_WUIR) A rising edge followed by a high level for a period defined by WKUPDBC on the cor-responding wake-up input forces the wake-up of the core power supply. */
#define SUPC_WUIR_WKUPT1 (0x1u << 17) /**< \brief (SUPC_WUIR) Wake-up Input Type 0 to 1 */
#define   SUPC_WUIR_WKUPT1_LOW (0x0u << 17) /**< \brief (SUPC_WUIR) A falling edge followed by a low level for a period defined by WKUPDBC on the corre-sponding wake-up input forces the wake-up of the core power supply. */
#define   SUPC_WUIR_WKUPT1_HIGH (0x1u << 17) /**< \brief (SUPC_WUIR) A rising edge followed by a high level for a period defined by WKUPDBC on the cor-responding wake-up input forces the wake-up of the core power supply. */
#define SUPC_WUIR_WKUPT2 (0x1u << 18) /**< \brief (SUPC_WUIR) Wake-up Input Type 0 to 2 */
#define   SUPC_WUIR_WKUPT2_LOW (0x0u << 18) /**< \brief (SUPC_WUIR) A falling edge followed by a low level for a period defined by WKUPDBC on the corre-sponding wake-up input forces the wake-up of the core power supply. */
#define   SUPC_WUIR_WKUPT2_HIGH (0x1u << 18) /**< \brief (SUPC_WUIR) A rising edge followed by a high level for a period defined by WKUPDBC on the cor-responding wake-up input forces the wake-up of the core power supply. */
#define SUPC_WUIR_WKUPT3 (0x1u << 19) /**< \brief (SUPC_WUIR) Wake-up Input Type 0 to 3 */
#define   SUPC_WUIR_WKUPT3_LOW (0x0u << 19) /**< \brief (SUPC_WUIR) A falling edge followed by a low level for a period defined by WKUPDBC on the corre-sponding wake-up input forces the wake-up of the core power supply. */
#define   SUPC_WUIR_WKUPT3_HIGH (0x1u << 19) /**< \brief (SUPC_WUIR) A rising edge followed by a high level for a period defined by WKUPDBC on the cor-responding wake-up input forces the wake-up of the core power supply. */
#define SUPC_WUIR_WKUPT4 (0x1u << 20) /**< \brief (SUPC_WUIR) Wake-up Input Type 0 to 4 */
#define   SUPC_WUIR_WKUPT4_LOW (0x0u << 20) /**< \brief (SUPC_WUIR) A falling edge followed by a low level for a period defined by WKUPDBC on the corre-sponding wake-up input forces the wake-up of the core power supply. */
#define   SUPC_WUIR_WKUPT4_HIGH (0x1u << 20) /**< \brief (SUPC_WUIR) A rising edge followed by a high level for a period defined by WKUPDBC on the cor-responding wake-up input forces the wake-up of the core power supply. */
#define SUPC_WUIR_WKUPT5 (0x1u << 21) /**< \brief (SUPC_WUIR) Wake-up Input Type 0 to 5 */
#define   SUPC_WUIR_WKUPT5_LOW (0x0u << 21) /**< \brief (SUPC_WUIR) A falling edge followed by a low level for a period defined by WKUPDBC on the corre-sponding wake-up input forces the wake-up of the core power supply. */
#define   SUPC_WUIR_WKUPT5_HIGH (0x1u << 21) /**< \brief (SUPC_WUIR) A rising edge followed by a high level for a period defined by WKUPDBC on the cor-responding wake-up input forces the wake-up of the core power supply. */
#define SUPC_WUIR_WKUPT6 (0x1u << 22) /**< \brief (SUPC_WUIR) Wake-up Input Type 0 to 6 */
#define   SUPC_WUIR_WKUPT6_LOW (0x0u << 22) /**< \brief (SUPC_WUIR) A falling edge followed by a low level for a period defined by WKUPDBC on the corre-sponding wake-up input forces the wake-up of the core power supply. */
#define   SUPC_WUIR_WKUPT6_HIGH (0x1u << 22) /**< \brief (SUPC_WUIR) A rising edge followed by a high level for a period defined by WKUPDBC on the cor-responding wake-up input forces the wake-up of the core power supply. */
#define SUPC_WUIR_WKUPT7 (0x1u << 23) /**< \brief (SUPC_WUIR) Wake-up Input Type 0 to 7 */
#define   SUPC_WUIR_WKUPT7_LOW (0x0u << 23) /**< \brief (SUPC_WUIR) A falling edge followed by a low level for a period defined by WKUPDBC on the corre-sponding wake-up input forces the wake-up of the core power supply. */
#define   SUPC_WUIR_WKUPT7_HIGH (0x1u << 23) /**< \brief (SUPC_WUIR) A rising edge followed by a high level for a period defined by WKUPDBC on the cor-responding wake-up input forces the wake-up of the core power supply. */
#define SUPC_WUIR_WKUPT8 (0x1u << 24) /**< \brief (SUPC_WUIR) Wake-up Input Type 0 to 8 */
#define   SUPC_WUIR_WKUPT8_LOW (0x0u << 24) /**< \brief (SUPC_WUIR) A falling edge followed by a low level for a period defined by WKUPDBC on the corre-sponding wake-up input forces the wake-up of the core power supply. */
#define   SUPC_WUIR_WKUPT8_HIGH (0x1u << 24) /**< \brief (SUPC_WUIR) A rising edge followed by a high level for a period defined by WKUPDBC on the cor-responding wake-up input forces the wake-up of the core power supply. */
#define SUPC_WUIR_WKUPT9 (0x1u << 25) /**< \brief (SUPC_WUIR) Wake-up Input Type 0 to 9 */
#define   SUPC_WUIR_WKUPT9_LOW (0x0u << 25) /**< \brief (SUPC_WUIR) A falling edge followed by a low level for a period defined by WKUPDBC on the corre-sponding wake-up input forces the wake-up of the core power supply. */
#define   SUPC_WUIR_WKUPT9_HIGH (0x1u << 25) /**< \brief (SUPC_WUIR) A rising edge followed by a high level for a period defined by WKUPDBC on the cor-responding wake-up input forces the wake-up of the core power supply. */
#define SUPC_WUIR_WKUPT10 (0x1u << 26) /**< \brief (SUPC_WUIR) Wake-up Input Type 0 to 10 */
#define   SUPC_WUIR_WKUPT10_LOW (0x0u << 26) /**< \brief (SUPC_WUIR) A falling edge followed by a low level for a period defined by WKUPDBC on the corre-sponding wake-up input forces the wake-up of the core power supply. */
#define   SUPC_WUIR_WKUPT10_HIGH (0x1u << 26) /**< \brief (SUPC_WUIR) A rising edge followed by a high level for a period defined by WKUPDBC on the cor-responding wake-up input forces the wake-up of the core power supply. */
#define SUPC_WUIR_WKUPT11 (0x1u << 27) /**< \brief (SUPC_WUIR) Wake-up Input Type 0 to 11 */
#define   SUPC_WUIR_WKUPT11_LOW (0x0u << 27) /**< \brief (SUPC_WUIR) A falling edge followed by a low level for a period defined by WKUPDBC on the corre-sponding wake-up input forces the wake-up of the core power supply. */
#define   SUPC_WUIR_WKUPT11_HIGH (0x1u << 27) /**< \brief (SUPC_WUIR) A rising edge followed by a high level for a period defined by WKUPDBC on the cor-responding wake-up input forces the wake-up of the core power supply. */
#define SUPC_WUIR_WKUPT12 (0x1u << 28) /**< \brief (SUPC_WUIR) Wake-up Input Type 0 to 12 */
#define   SUPC_WUIR_WKUPT12_LOW (0x0u << 28) /**< \brief (SUPC_WUIR) A falling edge followed by a low level for a period defined by WKUPDBC on the corre-sponding wake-up input forces the wake-up of the core power supply. */
#define   SUPC_WUIR_WKUPT12_HIGH (0x1u << 28) /**< \brief (SUPC_WUIR) A rising edge followed by a high level for a period defined by WKUPDBC on the cor-responding wake-up input forces the wake-up of the core power supply. */
#define SUPC_WUIR_WKUPT13 (0x1u << 29) /**< \brief (SUPC_WUIR) Wake-up Input Type 0 to 13 */
#define   SUPC_WUIR_WKUPT13_LOW (0x0u << 29) /**< \brief (SUPC_WUIR) A falling edge followed by a low level for a period defined by WKUPDBC on the corre-sponding wake-up input forces the wake-up of the core power supply. */
#define   SUPC_WUIR_WKUPT13_HIGH (0x1u << 29) /**< \brief (SUPC_WUIR) A rising edge followed by a high level for a period defined by WKUPDBC on the cor-responding wake-up input forces the wake-up of the core power supply. */
#define SUPC_WUIR_WKUPT14 (0x1u << 30) /**< \brief (SUPC_WUIR) Wake-up Input Type 0 to 14 */
#define   SUPC_WUIR_WKUPT14_LOW (0x0u << 30) /**< \brief (SUPC_WUIR) A falling edge followed by a low level for a period defined by WKUPDBC on the corre-sponding wake-up input forces the wake-up of the core power supply. */
#define   SUPC_WUIR_WKUPT14_HIGH (0x1u << 30) /**< \brief (SUPC_WUIR) A rising edge followed by a high level for a period defined by WKUPDBC on the cor-responding wake-up input forces the wake-up of the core power supply. */
#define SUPC_WUIR_WKUPT15 (0x1u << 31) /**< \brief (SUPC_WUIR) Wake-up Input Type 0 to 15 */
#define   SUPC_WUIR_WKUPT15_LOW (0x0u << 31) /**< \brief (SUPC_WUIR) A falling edge followed by a low level for a period defined by WKUPDBC on the corre-sponding wake-up input forces the wake-up of the core power supply. */
#define   SUPC_WUIR_WKUPT15_HIGH (0x1u << 31) /**< \brief (SUPC_WUIR) A rising edge followed by a high level for a period defined by WKUPDBC on the cor-responding wake-up input forces the wake-up of the core power supply. */
/* -------- SUPC_SR : (SUPC Offset: 0x14) Supply Controller Status Register -------- */
#define SUPC_SR_WKUPS (0x1u << 1) /**< \brief (SUPC_SR) WKUP Wake-up Status (cleared on read) */
#define   SUPC_SR_WKUPS_NO (0x0u << 1) /**< \brief (SUPC_SR) No wake-up due to the assertion of the WKUP pins has occurred since the last read of SUPC_SR. */
#define   SUPC_SR_WKUPS_PRESENT (0x1u << 1) /**< \brief (SUPC_SR) At least one wake-up due to the assertion of the WKUP pins has occurred since the last read of SUPC_SR. */
#define SUPC_SR_SMWS (0x1u << 2) /**< \brief (SUPC_SR) Supply Monitor Detection Wake-up Status (cleared on read) */
#define   SUPC_SR_SMWS_NO (0x0u << 2) /**< \brief (SUPC_SR) No wake-up due to a supply monitor detection has occurred since the last read of SUPC_SR. */
#define   SUPC_SR_SMWS_PRESENT (0x1u << 2) /**< \brief (SUPC_SR) At least one wake-up due to a supply monitor detection has occurred since the last read of SUPC_SR. */
#define SUPC_SR_BODRSTS (0x1u << 3) /**< \brief (SUPC_SR) Brownout Detector Reset Status (cleared on read) */
#define   SUPC_SR_BODRSTS_NO (0x0u << 3) /**< \brief (SUPC_SR) No core brownout rising edge event has been detected since the last read of the SUPC_SR. */
#define   SUPC_SR_BODRSTS_PRESENT (0x1u << 3) /**< \brief (SUPC_SR) At least one brownout output rising edge event has been detected since the last read of the SUPC_SR. */
#define SUPC_SR_SMRSTS (0x1u << 4) /**< \brief (SUPC_SR) Supply Monitor Reset Status (cleared on read) */
#define   SUPC_SR_SMRSTS_NO (0x0u << 4) /**< \brief (SUPC_SR) No supply monitor detection has generated a core reset since the last read of the SUPC_SR. */
#define   SUPC_SR_SMRSTS_PRESENT (0x1u << 4) /**< \brief (SUPC_SR) At least one supply monitor detection has generated a core reset since the last read of the SUPC_SR. */
#define SUPC_SR_SMS (0x1u << 5) /**< \brief (SUPC_SR) Supply Monitor Status (cleared on read) */
#define   SUPC_SR_SMS_NO (0x0u << 5) /**< \brief (SUPC_SR) No supply monitor detection since the last read of SUPC_SR. */
#define   SUPC_SR_SMS_PRESENT (0x1u << 5) /**< \brief (SUPC_SR) At least one supply monitor detection since the last read of SUPC_SR. */
#define SUPC_SR_SMOS (0x1u << 6) /**< \brief (SUPC_SR) Supply Monitor Output Status */
#define   SUPC_SR_SMOS_HIGH (0x0u << 6) /**< \brief (SUPC_SR) The supply monitor detected VDDIO higher than its threshold at its last measurement. */
#define   SUPC_SR_SMOS_LOW (0x1u << 6) /**< \brief (SUPC_SR) The supply monitor detected VDDIO lower than its threshold at its last measurement. */
#define SUPC_SR_OSCSEL (0x1u << 7) /**< \brief (SUPC_SR) 32-kHz Oscillator Selection Status */
#define   SUPC_SR_OSCSEL_RC (0x0u << 7) /**< \brief (SUPC_SR) The slow clock, SLCK, is generated by the embedded 32 kHz RC oscillator. */
#define   SUPC_SR_OSCSEL_CRYST (0x1u << 7) /**< \brief (SUPC_SR) The slow clock, SLCK, is generated by the 32 kHz crystal oscillator. */
#define SUPC_SR_LPDBCS0 (0x1u << 13) /**< \brief (SUPC_SR) Low-power Debouncer Wake-up Status on WKUP0 (cleared on read) */
#define   SUPC_SR_LPDBCS0_NO (0x0u << 13) /**< \brief (SUPC_SR) No wake-up due to the assertion of the WKUP0 pin has occurred since the last read of SUPC_SR. */
#define   SUPC_SR_LPDBCS0_PRESENT (0x1u << 13) /**< \brief (SUPC_SR) At least one wake-up due to the assertion of the WKUP0 pin has occurred since the last read of SUPC_SR. */
#define SUPC_SR_LPDBCS1 (0x1u << 14) /**< \brief (SUPC_SR) Low-power Debouncer Wake-up Status on WKUP1 (cleared on read) */
#define   SUPC_SR_LPDBCS1_NO (0x0u << 14) /**< \brief (SUPC_SR) No wake-up due to the assertion of the WKUP1 pin has occurred since the last read of SUPC_SR. */
#define   SUPC_SR_LPDBCS1_PRESENT (0x1u << 14) /**< \brief (SUPC_SR) At least one wake-up due to the assertion of the WKUP1 pin has occurred since the last read of SUPC_SR. */
#define SUPC_SR_WKUPIS0 (0x1u << 16) /**< \brief (SUPC_SR) WKUPx Input Status (cleared on read) */
#define   SUPC_SR_WKUPIS0_DIS (0x0u << 16) /**< \brief (SUPC_SR) The corresponding wake-up input is disabled, or was inactive at the time the debouncer triggered a wake-up event. */
#define   SUPC_SR_WKUPIS0_EN (0x1u << 16) /**< \brief (SUPC_SR) The corresponding wake-up input was active at the time the debouncer triggered a wake-up event since the last read of SUPC_SR. */
#define SUPC_SR_WKUPIS1 (0x1u << 17) /**< \brief (SUPC_SR) WKUPx Input Status (cleared on read) */
#define   SUPC_SR_WKUPIS1_DIS (0x0u << 17) /**< \brief (SUPC_SR) The corresponding wake-up input is disabled, or was inactive at the time the debouncer triggered a wake-up event. */
#define   SUPC_SR_WKUPIS1_EN (0x1u << 17) /**< \brief (SUPC_SR) The corresponding wake-up input was active at the time the debouncer triggered a wake-up event since the last read of SUPC_SR. */
#define SUPC_SR_WKUPIS2 (0x1u << 18) /**< \brief (SUPC_SR) WKUPx Input Status (cleared on read) */
#define   SUPC_SR_WKUPIS2_DIS (0x0u << 18) /**< \brief (SUPC_SR) The corresponding wake-up input is disabled, or was inactive at the time the debouncer triggered a wake-up event. */
#define   SUPC_SR_WKUPIS2_EN (0x1u << 18) /**< \brief (SUPC_SR) The corresponding wake-up input was active at the time the debouncer triggered a wake-up event since the last read of SUPC_SR. */
#define SUPC_SR_WKUPIS3 (0x1u << 19) /**< \brief (SUPC_SR) WKUPx Input Status (cleared on read) */
#define   SUPC_SR_WKUPIS3_DIS (0x0u << 19) /**< \brief (SUPC_SR) The corresponding wake-up input is disabled, or was inactive at the time the debouncer triggered a wake-up event. */
#define   SUPC_SR_WKUPIS3_EN (0x1u << 19) /**< \brief (SUPC_SR) The corresponding wake-up input was active at the time the debouncer triggered a wake-up event since the last read of SUPC_SR. */
#define SUPC_SR_WKUPIS4 (0x1u << 20) /**< \brief (SUPC_SR) WKUPx Input Status (cleared on read) */
#define   SUPC_SR_WKUPIS4_DIS (0x0u << 20) /**< \brief (SUPC_SR) The corresponding wake-up input is disabled, or was inactive at the time the debouncer triggered a wake-up event. */
#define   SUPC_SR_WKUPIS4_EN (0x1u << 20) /**< \brief (SUPC_SR) The corresponding wake-up input was active at the time the debouncer triggered a wake-up event since the last read of SUPC_SR. */
#define SUPC_SR_WKUPIS5 (0x1u << 21) /**< \brief (SUPC_SR) WKUPx Input Status (cleared on read) */
#define   SUPC_SR_WKUPIS5_DIS (0x0u << 21) /**< \brief (SUPC_SR) The corresponding wake-up input is disabled, or was inactive at the time the debouncer triggered a wake-up event. */
#define   SUPC_SR_WKUPIS5_EN (0x1u << 21) /**< \brief (SUPC_SR) The corresponding wake-up input was active at the time the debouncer triggered a wake-up event since the last read of SUPC_SR. */
#define SUPC_SR_WKUPIS6 (0x1u << 22) /**< \brief (SUPC_SR) WKUPx Input Status (cleared on read) */
#define   SUPC_SR_WKUPIS6_DIS (0x0u << 22) /**< \brief (SUPC_SR) The corresponding wake-up input is disabled, or was inactive at the time the debouncer triggered a wake-up event. */
#define   SUPC_SR_WKUPIS6_EN (0x1u << 22) /**< \brief (SUPC_SR) The corresponding wake-up input was active at the time the debouncer triggered a wake-up event since the last read of SUPC_SR. */
#define SUPC_SR_WKUPIS7 (0x1u << 23) /**< \brief (SUPC_SR) WKUPx Input Status (cleared on read) */
#define   SUPC_SR_WKUPIS7_DIS (0x0u << 23) /**< \brief (SUPC_SR) The corresponding wake-up input is disabled, or was inactive at the time the debouncer triggered a wake-up event. */
#define   SUPC_SR_WKUPIS7_EN (0x1u << 23) /**< \brief (SUPC_SR) The corresponding wake-up input was active at the time the debouncer triggered a wake-up event since the last read of SUPC_SR. */
#define SUPC_SR_WKUPIS8 (0x1u << 24) /**< \brief (SUPC_SR) WKUPx Input Status (cleared on read) */
#define   SUPC_SR_WKUPIS8_DIS (0x0u << 24) /**< \brief (SUPC_SR) The corresponding wake-up input is disabled, or was inactive at the time the debouncer triggered a wake-up event. */
#define   SUPC_SR_WKUPIS8_EN (0x1u << 24) /**< \brief (SUPC_SR) The corresponding wake-up input was active at the time the debouncer triggered a wake-up event since the last read of SUPC_SR. */
#define SUPC_SR_WKUPIS9 (0x1u << 25) /**< \brief (SUPC_SR) WKUPx Input Status (cleared on read) */
#define   SUPC_SR_WKUPIS9_DIS (0x0u << 25) /**< \brief (SUPC_SR) The corresponding wake-up input is disabled, or was inactive at the time the debouncer triggered a wake-up event. */
#define   SUPC_SR_WKUPIS9_EN (0x1u << 25) /**< \brief (SUPC_SR) The corresponding wake-up input was active at the time the debouncer triggered a wake-up event since the last read of SUPC_SR. */
#define SUPC_SR_WKUPIS10 (0x1u << 26) /**< \brief (SUPC_SR) WKUPx Input Status (cleared on read) */
#define   SUPC_SR_WKUPIS10_DIS (0x0u << 26) /**< \brief (SUPC_SR) The corresponding wake-up input is disabled, or was inactive at the time the debouncer triggered a wake-up event. */
#define   SUPC_SR_WKUPIS10_EN (0x1u << 26) /**< \brief (SUPC_SR) The corresponding wake-up input was active at the time the debouncer triggered a wake-up event since the last read of SUPC_SR. */
#define SUPC_SR_WKUPIS11 (0x1u << 27) /**< \brief (SUPC_SR) WKUPx Input Status (cleared on read) */
#define   SUPC_SR_WKUPIS11_DIS (0x0u << 27) /**< \brief (SUPC_SR) The corresponding wake-up input is disabled, or was inactive at the time the debouncer triggered a wake-up event. */
#define   SUPC_SR_WKUPIS11_EN (0x1u << 27) /**< \brief (SUPC_SR) The corresponding wake-up input was active at the time the debouncer triggered a wake-up event since the last read of SUPC_SR. */
#define SUPC_SR_WKUPIS12 (0x1u << 28) /**< \brief (SUPC_SR) WKUPx Input Status (cleared on read) */
#define   SUPC_SR_WKUPIS12_DIS (0x0u << 28) /**< \brief (SUPC_SR) The corresponding wake-up input is disabled, or was inactive at the time the debouncer triggered a wake-up event. */
#define   SUPC_SR_WKUPIS12_EN (0x1u << 28) /**< \brief (SUPC_SR) The corresponding wake-up input was active at the time the debouncer triggered a wake-up event since the last read of SUPC_SR. */
#define SUPC_SR_WKUPIS13 (0x1u << 29) /**< \brief (SUPC_SR) WKUPx Input Status (cleared on read) */
#define   SUPC_SR_WKUPIS13_DIS (0x0u << 29) /**< \brief (SUPC_SR) The corresponding wake-up input is disabled, or was inactive at the time the debouncer triggered a wake-up event. */
#define   SUPC_SR_WKUPIS13_EN (0x1u << 29) /**< \brief (SUPC_SR) The corresponding wake-up input was active at the time the debouncer triggered a wake-up event since the last read of SUPC_SR. */
#define SUPC_SR_WKUPIS14 (0x1u << 30) /**< \brief (SUPC_SR) WKUPx Input Status (cleared on read) */
#define   SUPC_SR_WKUPIS14_DIS (0x0u << 30) /**< \brief (SUPC_SR) The corresponding wake-up input is disabled, or was inactive at the time the debouncer triggered a wake-up event. */
#define   SUPC_SR_WKUPIS14_EN (0x1u << 30) /**< \brief (SUPC_SR) The corresponding wake-up input was active at the time the debouncer triggered a wake-up event since the last read of SUPC_SR. */
#define SUPC_SR_WKUPIS15 (0x1u << 31) /**< \brief (SUPC_SR) WKUPx Input Status (cleared on read) */
#define   SUPC_SR_WKUPIS15_DIS (0x0u << 31) /**< \brief (SUPC_SR) The corresponding wake-up input is disabled, or was inactive at the time the debouncer triggered a wake-up event. */
#define   SUPC_SR_WKUPIS15_EN (0x1u << 31) /**< \brief (SUPC_SR) The corresponding wake-up input was active at the time the debouncer triggered a wake-up event since the last read of SUPC_SR. */

/*@}*/


#endif /* _SAM4S_SUPC_COMPONENT_ */
