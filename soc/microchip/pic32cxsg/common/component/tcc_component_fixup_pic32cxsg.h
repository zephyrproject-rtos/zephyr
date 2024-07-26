/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_TCC_COMPONENT_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_TCC_COMPONENT_FIXUP_H_

/* -------- TCC_CTRLA : (TCC Offset: 0x00) (R/W 32) Control A -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SWRST:1;          /*!< bit:      0  Software Reset                     */
    uint32_t ENABLE:1;         /*!< bit:      1  Enable                             */
    uint32_t :3;               /*!< bit:  2.. 4  Reserved                           */
    uint32_t RESOLUTION:2;     /*!< bit:  5.. 6  Enhanced Resolution                */
    uint32_t :1;               /*!< bit:      7  Reserved                           */
    uint32_t PRESCALER:3;      /*!< bit:  8..10  Prescaler                          */
    uint32_t RUNSTDBY:1;       /*!< bit:     11  Run in Standby                     */
    uint32_t PRESCSYNC:2;      /*!< bit: 12..13  Prescaler and Counter Synchronization Selection */
    uint32_t ALOCK:1;          /*!< bit:     14  Auto Lock                          */
    uint32_t MSYNC:1;          /*!< bit:     15  Master Synchronization (only for TCC Slave Instance) */
    uint32_t :7;               /*!< bit: 16..22  Reserved                           */
    uint32_t DMAOS:1;          /*!< bit:     23  DMA One-shot Trigger Mode          */
    uint32_t CPTEN0:1;         /*!< bit:     24  Capture Channel 0 Enable           */
    uint32_t CPTEN1:1;         /*!< bit:     25  Capture Channel 1 Enable           */
    uint32_t CPTEN2:1;         /*!< bit:     26  Capture Channel 2 Enable           */
    uint32_t CPTEN3:1;         /*!< bit:     27  Capture Channel 3 Enable           */
    uint32_t CPTEN4:1;         /*!< bit:     28  Capture Channel 4 Enable           */
    uint32_t CPTEN5:1;         /*!< bit:     29  Capture Channel 5 Enable           */
    uint32_t :2;               /*!< bit: 30..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t :24;              /*!< bit:  0..23  Reserved                           */
    uint32_t CPTEN:6;          /*!< bit: 24..29  Capture Channel x Enable           */
    uint32_t :2;               /*!< bit: 30..31  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} TCC_CTRLA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TCC_CTRLBCLR : (TCC Offset: 0x04) (R/W 8) Control B Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  DIR:1;            /*!< bit:      0  Counter Direction                  */
    uint8_t  LUPD:1;           /*!< bit:      1  Lock Update                        */
    uint8_t  ONESHOT:1;        /*!< bit:      2  One-Shot                           */
    uint8_t  IDXCMD:2;         /*!< bit:  3.. 4  Ramp Index Command                 */
    uint8_t  CMD:3;            /*!< bit:  5.. 7  TCC Command                        */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} TCC_CTRLBCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TCC_CTRLBSET : (TCC Offset: 0x05) (R/W 8) Control B Set -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  DIR:1;            /*!< bit:      0  Counter Direction                  */
    uint8_t  LUPD:1;           /*!< bit:      1  Lock Update                        */
    uint8_t  ONESHOT:1;        /*!< bit:      2  One-Shot                           */
    uint8_t  IDXCMD:2;         /*!< bit:  3.. 4  Ramp Index Command                 */
    uint8_t  CMD:3;            /*!< bit:  5.. 7  TCC Command                        */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} TCC_CTRLBSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TCC_SYNCBUSY : (TCC Offset: 0x08) ( R/ 32) Synchronization Busy -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SWRST:1;          /*!< bit:      0  Swrst Busy                         */
    uint32_t ENABLE:1;         /*!< bit:      1  Enable Busy                        */
    uint32_t CTRLB:1;          /*!< bit:      2  Ctrlb Busy                         */
    uint32_t STATUS:1;         /*!< bit:      3  Status Busy                        */
    uint32_t COUNT:1;          /*!< bit:      4  Count Busy                         */
    uint32_t PATT:1;           /*!< bit:      5  Pattern Busy                       */
    uint32_t WAVE:1;           /*!< bit:      6  Wave Busy                          */
    uint32_t PER:1;            /*!< bit:      7  Period Busy                        */
    uint32_t CC0:1;            /*!< bit:      8  Compare Channel 0 Busy             */
    uint32_t CC1:1;            /*!< bit:      9  Compare Channel 1 Busy             */
    uint32_t CC2:1;            /*!< bit:     10  Compare Channel 2 Busy             */
    uint32_t CC3:1;            /*!< bit:     11  Compare Channel 3 Busy             */
    uint32_t CC4:1;            /*!< bit:     12  Compare Channel 4 Busy             */
    uint32_t CC5:1;            /*!< bit:     13  Compare Channel 5 Busy             */
    uint32_t :18;              /*!< bit: 14..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t :8;               /*!< bit:  0.. 7  Reserved                           */
    uint32_t CC:6;             /*!< bit:  8..13  Compare Channel x Busy             */
    uint32_t :18;              /*!< bit: 14..31  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} TCC_SYNCBUSY_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TCC_FCTRLA : (TCC Offset: 0x0C) (R/W 32) Recoverable Fault A Configuration -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SRC:2;            /*!< bit:  0.. 1  Fault A Source                     */
    uint32_t :1;               /*!< bit:      2  Reserved                           */
    uint32_t KEEP:1;           /*!< bit:      3  Fault A Keeper                     */
    uint32_t QUAL:1;           /*!< bit:      4  Fault A Qualification              */
    uint32_t BLANK:2;          /*!< bit:  5.. 6  Fault A Blanking Mode              */
    uint32_t RESTART:1;        /*!< bit:      7  Fault A Restart                    */
    uint32_t HALT:2;           /*!< bit:  8.. 9  Fault A Halt Mode                  */
    uint32_t CHSEL:2;          /*!< bit: 10..11  Fault A Capture Channel            */
    uint32_t CAPTURE:3;        /*!< bit: 12..14  Fault A Capture Action             */
    uint32_t BLANKPRESC:1;     /*!< bit:     15  Fault A Blanking Prescaler         */
    uint32_t BLANKVAL:8;       /*!< bit: 16..23  Fault A Blanking Time              */
    uint32_t FILTERVAL:4;      /*!< bit: 24..27  Fault A Filter Value               */
    uint32_t :4;               /*!< bit: 28..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} TCC_FCTRLA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TCC_FCTRLB : (TCC Offset: 0x10) (R/W 32) Recoverable Fault B Configuration -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SRC:2;            /*!< bit:  0.. 1  Fault B Source                     */
    uint32_t :1;               /*!< bit:      2  Reserved                           */
    uint32_t KEEP:1;           /*!< bit:      3  Fault B Keeper                     */
    uint32_t QUAL:1;           /*!< bit:      4  Fault B Qualification              */
    uint32_t BLANK:2;          /*!< bit:  5.. 6  Fault B Blanking Mode              */
    uint32_t RESTART:1;        /*!< bit:      7  Fault B Restart                    */
    uint32_t HALT:2;           /*!< bit:  8.. 9  Fault B Halt Mode                  */
    uint32_t CHSEL:2;          /*!< bit: 10..11  Fault B Capture Channel            */
    uint32_t CAPTURE:3;        /*!< bit: 12..14  Fault B Capture Action             */
    uint32_t BLANKPRESC:1;     /*!< bit:     15  Fault B Blanking Prescaler         */
    uint32_t BLANKVAL:8;       /*!< bit: 16..23  Fault B Blanking Time              */
    uint32_t FILTERVAL:4;      /*!< bit: 24..27  Fault B Filter Value               */
    uint32_t :4;               /*!< bit: 28..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} TCC_FCTRLB_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TCC_WEXCTRL : (TCC Offset: 0x14) (R/W 32) Waveform Extension Configuration -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t OTMX:2;           /*!< bit:  0.. 1  Output Matrix                      */
    uint32_t :6;               /*!< bit:  2.. 7  Reserved                           */
    uint32_t DTIEN0:1;         /*!< bit:      8  Dead-time Insertion Generator 0 Enable */
    uint32_t DTIEN1:1;         /*!< bit:      9  Dead-time Insertion Generator 1 Enable */
    uint32_t DTIEN2:1;         /*!< bit:     10  Dead-time Insertion Generator 2 Enable */
    uint32_t DTIEN3:1;         /*!< bit:     11  Dead-time Insertion Generator 3 Enable */
    uint32_t :4;               /*!< bit: 12..15  Reserved                           */
    uint32_t DTLS:8;           /*!< bit: 16..23  Dead-time Low Side Outputs Value   */
    uint32_t DTHS:8;           /*!< bit: 24..31  Dead-time High Side Outputs Value  */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t :8;               /*!< bit:  0.. 7  Reserved                           */
    uint32_t DTIEN:4;          /*!< bit:  8..11  Dead-time Insertion Generator x Enable */
    uint32_t :20;              /*!< bit: 12..31  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} TCC_WEXCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TCC_DRVCTRL : (TCC Offset: 0x18) (R/W 32) Driver Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t NRE0:1;           /*!< bit:      0  Non-Recoverable State 0 Output Enable */
    uint32_t NRE1:1;           /*!< bit:      1  Non-Recoverable State 1 Output Enable */
    uint32_t NRE2:1;           /*!< bit:      2  Non-Recoverable State 2 Output Enable */
    uint32_t NRE3:1;           /*!< bit:      3  Non-Recoverable State 3 Output Enable */
    uint32_t NRE4:1;           /*!< bit:      4  Non-Recoverable State 4 Output Enable */
    uint32_t NRE5:1;           /*!< bit:      5  Non-Recoverable State 5 Output Enable */
    uint32_t NRE6:1;           /*!< bit:      6  Non-Recoverable State 6 Output Enable */
    uint32_t NRE7:1;           /*!< bit:      7  Non-Recoverable State 7 Output Enable */
    uint32_t NRV0:1;           /*!< bit:      8  Non-Recoverable State 0 Output Value */
    uint32_t NRV1:1;           /*!< bit:      9  Non-Recoverable State 1 Output Value */
    uint32_t NRV2:1;           /*!< bit:     10  Non-Recoverable State 2 Output Value */
    uint32_t NRV3:1;           /*!< bit:     11  Non-Recoverable State 3 Output Value */
    uint32_t NRV4:1;           /*!< bit:     12  Non-Recoverable State 4 Output Value */
    uint32_t NRV5:1;           /*!< bit:     13  Non-Recoverable State 5 Output Value */
    uint32_t NRV6:1;           /*!< bit:     14  Non-Recoverable State 6 Output Value */
    uint32_t NRV7:1;           /*!< bit:     15  Non-Recoverable State 7 Output Value */
    uint32_t INVEN0:1;         /*!< bit:     16  Output Waveform 0 Inversion        */
    uint32_t INVEN1:1;         /*!< bit:     17  Output Waveform 1 Inversion        */
    uint32_t INVEN2:1;         /*!< bit:     18  Output Waveform 2 Inversion        */
    uint32_t INVEN3:1;         /*!< bit:     19  Output Waveform 3 Inversion        */
    uint32_t INVEN4:1;         /*!< bit:     20  Output Waveform 4 Inversion        */
    uint32_t INVEN5:1;         /*!< bit:     21  Output Waveform 5 Inversion        */
    uint32_t INVEN6:1;         /*!< bit:     22  Output Waveform 6 Inversion        */
    uint32_t INVEN7:1;         /*!< bit:     23  Output Waveform 7 Inversion        */
    uint32_t FILTERVAL0:4;     /*!< bit: 24..27  Non-Recoverable Fault Input 0 Filter Value */
    uint32_t FILTERVAL1:4;     /*!< bit: 28..31  Non-Recoverable Fault Input 1 Filter Value */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t NRE:8;            /*!< bit:  0.. 7  Non-Recoverable State x Output Enable */
    uint32_t NRV:8;            /*!< bit:  8..15  Non-Recoverable State x Output Value */
    uint32_t INVEN:8;          /*!< bit: 16..23  Output Waveform x Inversion        */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} TCC_DRVCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TCC_DBGCTRL : (TCC Offset: 0x1E) (R/W 8) Debug Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  DBGRUN:1;         /*!< bit:      0  Debug Running Mode                 */
    uint8_t  :1;               /*!< bit:      1  Reserved                           */
    uint8_t  FDDBD:1;          /*!< bit:      2  Fault Detection on Debug Break Detection */
    uint8_t  :5;               /*!< bit:  3.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} TCC_DBGCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TCC_EVCTRL : (TCC Offset: 0x20) (R/W 32) Event Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t EVACT0:3;         /*!< bit:  0.. 2  Timer/counter Input Event0 Action  */
    uint32_t EVACT1:3;         /*!< bit:  3.. 5  Timer/counter Input Event1 Action  */
    uint32_t CNTSEL:2;         /*!< bit:  6.. 7  Timer/counter Output Event Mode    */
    uint32_t OVFEO:1;          /*!< bit:      8  Overflow/Underflow Output Event Enable */
    uint32_t TRGEO:1;          /*!< bit:      9  Retrigger Output Event Enable      */
    uint32_t CNTEO:1;          /*!< bit:     10  Timer/counter Output Event Enable  */
    uint32_t :1;               /*!< bit:     11  Reserved                           */
    uint32_t TCINV0:1;         /*!< bit:     12  Inverted Event 0 Input Enable      */
    uint32_t TCINV1:1;         /*!< bit:     13  Inverted Event 1 Input Enable      */
    uint32_t TCEI0:1;          /*!< bit:     14  Timer/counter Event 0 Input Enable */
    uint32_t TCEI1:1;          /*!< bit:     15  Timer/counter Event 1 Input Enable */
    uint32_t MCEI0:1;          /*!< bit:     16  Match or Capture Channel 0 Event Input Enable */
    uint32_t MCEI1:1;          /*!< bit:     17  Match or Capture Channel 1 Event Input Enable */
    uint32_t MCEI2:1;          /*!< bit:     18  Match or Capture Channel 2 Event Input Enable */
    uint32_t MCEI3:1;          /*!< bit:     19  Match or Capture Channel 3 Event Input Enable */
    uint32_t MCEI4:1;          /*!< bit:     20  Match or Capture Channel 4 Event Input Enable */
    uint32_t MCEI5:1;          /*!< bit:     21  Match or Capture Channel 5 Event Input Enable */
    uint32_t :2;               /*!< bit: 22..23  Reserved                           */
    uint32_t MCEO0:1;          /*!< bit:     24  Match or Capture Channel 0 Event Output Enable */
    uint32_t MCEO1:1;          /*!< bit:     25  Match or Capture Channel 1 Event Output Enable */
    uint32_t MCEO2:1;          /*!< bit:     26  Match or Capture Channel 2 Event Output Enable */
    uint32_t MCEO3:1;          /*!< bit:     27  Match or Capture Channel 3 Event Output Enable */
    uint32_t MCEO4:1;          /*!< bit:     28  Match or Capture Channel 4 Event Output Enable */
    uint32_t MCEO5:1;          /*!< bit:     29  Match or Capture Channel 5 Event Output Enable */
    uint32_t :2;               /*!< bit: 30..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t :12;              /*!< bit:  0..11  Reserved                           */
    uint32_t TCINV:2;          /*!< bit: 12..13  Inverted Event x Input Enable      */
    uint32_t TCEI:2;           /*!< bit: 14..15  Timer/counter Event x Input Enable */
    uint32_t MCEI:6;           /*!< bit: 16..21  Match or Capture Channel x Event Input Enable */
    uint32_t :2;               /*!< bit: 22..23  Reserved                           */
    uint32_t MCEO:6;           /*!< bit: 24..29  Match or Capture Channel x Event Output Enable */
    uint32_t :2;               /*!< bit: 30..31  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} TCC_EVCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TCC_INTENCLR : (TCC Offset: 0x24) (R/W 32) Interrupt Enable Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t OVF:1;            /*!< bit:      0  Overflow Interrupt Enable          */
    uint32_t TRG:1;            /*!< bit:      1  Retrigger Interrupt Enable         */
    uint32_t CNT:1;            /*!< bit:      2  Counter Interrupt Enable           */
    uint32_t ERR:1;            /*!< bit:      3  Error Interrupt Enable             */
    uint32_t :6;               /*!< bit:  4.. 9  Reserved                           */
    uint32_t UFS:1;            /*!< bit:     10  Non-Recoverable Update Fault Interrupt Enable */
    uint32_t DFS:1;            /*!< bit:     11  Non-Recoverable Debug Fault Interrupt Enable */
    uint32_t FAULTA:1;         /*!< bit:     12  Recoverable Fault A Interrupt Enable */
    uint32_t FAULTB:1;         /*!< bit:     13  Recoverable Fault B Interrupt Enable */
    uint32_t FAULT0:1;         /*!< bit:     14  Non-Recoverable Fault 0 Interrupt Enable */
    uint32_t FAULT1:1;         /*!< bit:     15  Non-Recoverable Fault 1 Interrupt Enable */
    uint32_t MC0:1;            /*!< bit:     16  Match or Capture Channel 0 Interrupt Enable */
    uint32_t MC1:1;            /*!< bit:     17  Match or Capture Channel 1 Interrupt Enable */
    uint32_t MC2:1;            /*!< bit:     18  Match or Capture Channel 2 Interrupt Enable */
    uint32_t MC3:1;            /*!< bit:     19  Match or Capture Channel 3 Interrupt Enable */
    uint32_t MC4:1;            /*!< bit:     20  Match or Capture Channel 4 Interrupt Enable */
    uint32_t MC5:1;            /*!< bit:     21  Match or Capture Channel 5 Interrupt Enable */
    uint32_t :10;              /*!< bit: 22..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t :16;              /*!< bit:  0..15  Reserved                           */
    uint32_t MC:6;             /*!< bit: 16..21  Match or Capture Channel x Interrupt Enable */
    uint32_t :10;              /*!< bit: 22..31  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} TCC_INTENCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TCC_INTENSET : (TCC Offset: 0x28) (R/W 32) Interrupt Enable Set -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t OVF:1;            /*!< bit:      0  Overflow Interrupt Enable          */
    uint32_t TRG:1;            /*!< bit:      1  Retrigger Interrupt Enable         */
    uint32_t CNT:1;            /*!< bit:      2  Counter Interrupt Enable           */
    uint32_t ERR:1;            /*!< bit:      3  Error Interrupt Enable             */
    uint32_t :6;               /*!< bit:  4.. 9  Reserved                           */
    uint32_t UFS:1;            /*!< bit:     10  Non-Recoverable Update Fault Interrupt Enable */
    uint32_t DFS:1;            /*!< bit:     11  Non-Recoverable Debug Fault Interrupt Enable */
    uint32_t FAULTA:1;         /*!< bit:     12  Recoverable Fault A Interrupt Enable */
    uint32_t FAULTB:1;         /*!< bit:     13  Recoverable Fault B Interrupt Enable */
    uint32_t FAULT0:1;         /*!< bit:     14  Non-Recoverable Fault 0 Interrupt Enable */
    uint32_t FAULT1:1;         /*!< bit:     15  Non-Recoverable Fault 1 Interrupt Enable */
    uint32_t MC0:1;            /*!< bit:     16  Match or Capture Channel 0 Interrupt Enable */
    uint32_t MC1:1;            /*!< bit:     17  Match or Capture Channel 1 Interrupt Enable */
    uint32_t MC2:1;            /*!< bit:     18  Match or Capture Channel 2 Interrupt Enable */
    uint32_t MC3:1;            /*!< bit:     19  Match or Capture Channel 3 Interrupt Enable */
    uint32_t MC4:1;            /*!< bit:     20  Match or Capture Channel 4 Interrupt Enable */
    uint32_t MC5:1;            /*!< bit:     21  Match or Capture Channel 5 Interrupt Enable */
    uint32_t :10;              /*!< bit: 22..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t :16;              /*!< bit:  0..15  Reserved                           */
    uint32_t MC:6;             /*!< bit: 16..21  Match or Capture Channel x Interrupt Enable */
    uint32_t :10;              /*!< bit: 22..31  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} TCC_INTENSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TCC_INTFLAG : (TCC Offset: 0x2C) (R/W 32) Interrupt Flag Status and Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { // __I to avoid read-modify-write on write-to-clear register
  struct {
    __I uint32_t OVF:1;            /*!< bit:      0  Overflow                           */
    __I uint32_t TRG:1;            /*!< bit:      1  Retrigger                          */
    __I uint32_t CNT:1;            /*!< bit:      2  Counter                            */
    __I uint32_t ERR:1;            /*!< bit:      3  Error                              */
    __I uint32_t :6;               /*!< bit:  4.. 9  Reserved                           */
    __I uint32_t UFS:1;            /*!< bit:     10  Non-Recoverable Update Fault       */
    __I uint32_t DFS:1;            /*!< bit:     11  Non-Recoverable Debug Fault        */
    __I uint32_t FAULTA:1;         /*!< bit:     12  Recoverable Fault A                */
    __I uint32_t FAULTB:1;         /*!< bit:     13  Recoverable Fault B                */
    __I uint32_t FAULT0:1;         /*!< bit:     14  Non-Recoverable Fault 0            */
    __I uint32_t FAULT1:1;         /*!< bit:     15  Non-Recoverable Fault 1            */
    __I uint32_t MC0:1;            /*!< bit:     16  Match or Capture 0                 */
    __I uint32_t MC1:1;            /*!< bit:     17  Match or Capture 1                 */
    __I uint32_t MC2:1;            /*!< bit:     18  Match or Capture 2                 */
    __I uint32_t MC3:1;            /*!< bit:     19  Match or Capture 3                 */
    __I uint32_t MC4:1;            /*!< bit:     20  Match or Capture 4                 */
    __I uint32_t MC5:1;            /*!< bit:     21  Match or Capture 5                 */
    __I uint32_t :10;              /*!< bit: 22..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    __I uint32_t :16;              /*!< bit:  0..15  Reserved                           */
    __I uint32_t MC:6;             /*!< bit: 16..21  Match or Capture x                 */
    __I uint32_t :10;              /*!< bit: 22..31  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} TCC_INTFLAG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TCC_STATUS : (TCC Offset: 0x30) (R/W 32) Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t STOP:1;           /*!< bit:      0  Stop                               */
    uint32_t IDX:1;            /*!< bit:      1  Ramp                               */
    uint32_t UFS:1;            /*!< bit:      2  Non-recoverable Update Fault State */
    uint32_t DFS:1;            /*!< bit:      3  Non-Recoverable Debug Fault State  */
    uint32_t SLAVE:1;          /*!< bit:      4  Slave                              */
    uint32_t PATTBUFV:1;       /*!< bit:      5  Pattern Buffer Valid               */
    uint32_t :1;               /*!< bit:      6  Reserved                           */
    uint32_t PERBUFV:1;        /*!< bit:      7  Period Buffer Valid                */
    uint32_t FAULTAIN:1;       /*!< bit:      8  Recoverable Fault A Input          */
    uint32_t FAULTBIN:1;       /*!< bit:      9  Recoverable Fault B Input          */
    uint32_t FAULT0IN:1;       /*!< bit:     10  Non-Recoverable Fault0 Input       */
    uint32_t FAULT1IN:1;       /*!< bit:     11  Non-Recoverable Fault1 Input       */
    uint32_t FAULTA:1;         /*!< bit:     12  Recoverable Fault A State          */
    uint32_t FAULTB:1;         /*!< bit:     13  Recoverable Fault B State          */
    uint32_t FAULT0:1;         /*!< bit:     14  Non-Recoverable Fault 0 State      */
    uint32_t FAULT1:1;         /*!< bit:     15  Non-Recoverable Fault 1 State      */
    uint32_t CCBUFV0:1;        /*!< bit:     16  Compare Channel 0 Buffer Valid     */
    uint32_t CCBUFV1:1;        /*!< bit:     17  Compare Channel 1 Buffer Valid     */
    uint32_t CCBUFV2:1;        /*!< bit:     18  Compare Channel 2 Buffer Valid     */
    uint32_t CCBUFV3:1;        /*!< bit:     19  Compare Channel 3 Buffer Valid     */
    uint32_t CCBUFV4:1;        /*!< bit:     20  Compare Channel 4 Buffer Valid     */
    uint32_t CCBUFV5:1;        /*!< bit:     21  Compare Channel 5 Buffer Valid     */
    uint32_t :2;               /*!< bit: 22..23  Reserved                           */
    uint32_t CMP0:1;           /*!< bit:     24  Compare Channel 0 Value            */
    uint32_t CMP1:1;           /*!< bit:     25  Compare Channel 1 Value            */
    uint32_t CMP2:1;           /*!< bit:     26  Compare Channel 2 Value            */
    uint32_t CMP3:1;           /*!< bit:     27  Compare Channel 3 Value            */
    uint32_t CMP4:1;           /*!< bit:     28  Compare Channel 4 Value            */
    uint32_t CMP5:1;           /*!< bit:     29  Compare Channel 5 Value            */
    uint32_t :2;               /*!< bit: 30..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t :16;              /*!< bit:  0..15  Reserved                           */
    uint32_t CCBUFV:6;         /*!< bit: 16..21  Compare Channel x Buffer Valid     */
    uint32_t :2;               /*!< bit: 22..23  Reserved                           */
    uint32_t CMP:6;            /*!< bit: 24..29  Compare Channel x Value            */
    uint32_t :2;               /*!< bit: 30..31  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} TCC_STATUS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TCC_COUNT : (TCC Offset: 0x34) (R/W 32) Count -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct { // DITH4 mode
    uint32_t :4;               /*!< bit:  0.. 3  Reserved                           */
    uint32_t COUNT:20;         /*!< bit:  4..23  Counter Value                      */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } DITH4;                     /*!< Structure used for DITH4                        */
  struct { // DITH5 mode
    uint32_t :5;               /*!< bit:  0.. 4  Reserved                           */
    uint32_t COUNT:19;         /*!< bit:  5..23  Counter Value                      */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } DITH5;                     /*!< Structure used for DITH5                        */
  struct { // DITH6 mode
    uint32_t :6;               /*!< bit:  0.. 5  Reserved                           */
    uint32_t COUNT:18;         /*!< bit:  6..23  Counter Value                      */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } DITH6;                     /*!< Structure used for DITH6                        */
  struct {
    uint32_t COUNT:24;         /*!< bit:  0..23  Counter Value                      */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} TCC_COUNT_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TCC_PATT : (TCC Offset: 0x38) (R/W 16) Pattern -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t PGE0:1;           /*!< bit:      0  Pattern Generator 0 Output Enable  */
    uint16_t PGE1:1;           /*!< bit:      1  Pattern Generator 1 Output Enable  */
    uint16_t PGE2:1;           /*!< bit:      2  Pattern Generator 2 Output Enable  */
    uint16_t PGE3:1;           /*!< bit:      3  Pattern Generator 3 Output Enable  */
    uint16_t PGE4:1;           /*!< bit:      4  Pattern Generator 4 Output Enable  */
    uint16_t PGE5:1;           /*!< bit:      5  Pattern Generator 5 Output Enable  */
    uint16_t PGE6:1;           /*!< bit:      6  Pattern Generator 6 Output Enable  */
    uint16_t PGE7:1;           /*!< bit:      7  Pattern Generator 7 Output Enable  */
    uint16_t PGV0:1;           /*!< bit:      8  Pattern Generator 0 Output Value   */
    uint16_t PGV1:1;           /*!< bit:      9  Pattern Generator 1 Output Value   */
    uint16_t PGV2:1;           /*!< bit:     10  Pattern Generator 2 Output Value   */
    uint16_t PGV3:1;           /*!< bit:     11  Pattern Generator 3 Output Value   */
    uint16_t PGV4:1;           /*!< bit:     12  Pattern Generator 4 Output Value   */
    uint16_t PGV5:1;           /*!< bit:     13  Pattern Generator 5 Output Value   */
    uint16_t PGV6:1;           /*!< bit:     14  Pattern Generator 6 Output Value   */
    uint16_t PGV7:1;           /*!< bit:     15  Pattern Generator 7 Output Value   */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint16_t PGE:8;            /*!< bit:  0.. 7  Pattern Generator x Output Enable  */
    uint16_t PGV:8;            /*!< bit:  8..15  Pattern Generator x Output Value   */
  } vec;                       /*!< Structure used for vec  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} TCC_PATT_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TCC_WAVE : (TCC Offset: 0x3C) (R/W 32) Waveform Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t WAVEGEN:3;        /*!< bit:  0.. 2  Waveform Generation                */
    uint32_t :1;               /*!< bit:      3  Reserved                           */
    uint32_t RAMP:2;           /*!< bit:  4.. 5  Ramp Mode                          */
    uint32_t :1;               /*!< bit:      6  Reserved                           */
    uint32_t CIPEREN:1;        /*!< bit:      7  Circular period Enable             */
    uint32_t CICCEN0:1;        /*!< bit:      8  Circular Channel 0 Enable          */
    uint32_t CICCEN1:1;        /*!< bit:      9  Circular Channel 1 Enable          */
    uint32_t CICCEN2:1;        /*!< bit:     10  Circular Channel 2 Enable          */
    uint32_t CICCEN3:1;        /*!< bit:     11  Circular Channel 3 Enable          */
    uint32_t :4;               /*!< bit: 12..15  Reserved                           */
    uint32_t POL0:1;           /*!< bit:     16  Channel 0 Polarity                 */
    uint32_t POL1:1;           /*!< bit:     17  Channel 1 Polarity                 */
    uint32_t POL2:1;           /*!< bit:     18  Channel 2 Polarity                 */
    uint32_t POL3:1;           /*!< bit:     19  Channel 3 Polarity                 */
    uint32_t POL4:1;           /*!< bit:     20  Channel 4 Polarity                 */
    uint32_t POL5:1;           /*!< bit:     21  Channel 5 Polarity                 */
    uint32_t :2;               /*!< bit: 22..23  Reserved                           */
    uint32_t SWAP0:1;          /*!< bit:     24  Swap DTI Output Pair 0             */
    uint32_t SWAP1:1;          /*!< bit:     25  Swap DTI Output Pair 1             */
    uint32_t SWAP2:1;          /*!< bit:     26  Swap DTI Output Pair 2             */
    uint32_t SWAP3:1;          /*!< bit:     27  Swap DTI Output Pair 3             */
    uint32_t :4;               /*!< bit: 28..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint32_t :8;               /*!< bit:  0.. 7  Reserved                           */
    uint32_t CICCEN:4;         /*!< bit:  8..11  Circular Channel x Enable          */
    uint32_t :4;               /*!< bit: 12..15  Reserved                           */
    uint32_t POL:6;            /*!< bit: 16..21  Channel x Polarity                 */
    uint32_t :2;               /*!< bit: 22..23  Reserved                           */
    uint32_t SWAP:4;           /*!< bit: 24..27  Swap DTI Output Pair x             */
    uint32_t :4;               /*!< bit: 28..31  Reserved                           */
  } vec;                       /*!< Structure used for vec  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} TCC_WAVE_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TCC_PER : (TCC Offset: 0x40) (R/W 32) Period -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct { // DITH4 mode
    uint32_t DITHER:4;         /*!< bit:  0.. 3  Dithering Cycle Number             */
    uint32_t PER:20;           /*!< bit:  4..23  Period Value                       */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } DITH4;                     /*!< Structure used for DITH4                        */
  struct { // DITH5 mode
    uint32_t DITHER:5;         /*!< bit:  0.. 4  Dithering Cycle Number             */
    uint32_t PER:19;           /*!< bit:  5..23  Period Value                       */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } DITH5;                     /*!< Structure used for DITH5                        */
  struct { // DITH6 mode
    uint32_t DITHER:6;         /*!< bit:  0.. 5  Dithering Cycle Number             */
    uint32_t PER:18;           /*!< bit:  6..23  Period Value                       */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } DITH6;                     /*!< Structure used for DITH6                        */
  struct {
    uint32_t PER:24;           /*!< bit:  0..23  Period Value                       */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} TCC_PER_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TCC_CC : (TCC Offset: 0x44) (R/W 32) Compare and Capture -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct { // DITH4 mode
    uint32_t DITHER:4;         /*!< bit:  0.. 3  Dithering Cycle Number             */
    uint32_t CC:20;            /*!< bit:  4..23  Channel Compare/Capture Value      */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } DITH4;                     /*!< Structure used for DITH4                        */
  struct { // DITH5 mode
    uint32_t DITHER:5;         /*!< bit:  0.. 4  Dithering Cycle Number             */
    uint32_t CC:19;            /*!< bit:  5..23  Channel Compare/Capture Value      */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } DITH5;                     /*!< Structure used for DITH5                        */
  struct { // DITH6 mode
    uint32_t DITHER:6;         /*!< bit:  0.. 5  Dithering Cycle Number             */
    uint32_t CC:18;            /*!< bit:  6..23  Channel Compare/Capture Value      */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } DITH6;                     /*!< Structure used for DITH6                        */
  struct {
    uint32_t CC:24;            /*!< bit:  0..23  Channel Compare/Capture Value      */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} TCC_CC_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TCC_PATTBUF : (TCC Offset: 0x64) (R/W 16) Pattern Buffer -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t PGEB0:1;          /*!< bit:      0  Pattern Generator 0 Output Enable Buffer */
    uint16_t PGEB1:1;          /*!< bit:      1  Pattern Generator 1 Output Enable Buffer */
    uint16_t PGEB2:1;          /*!< bit:      2  Pattern Generator 2 Output Enable Buffer */
    uint16_t PGEB3:1;          /*!< bit:      3  Pattern Generator 3 Output Enable Buffer */
    uint16_t PGEB4:1;          /*!< bit:      4  Pattern Generator 4 Output Enable Buffer */
    uint16_t PGEB5:1;          /*!< bit:      5  Pattern Generator 5 Output Enable Buffer */
    uint16_t PGEB6:1;          /*!< bit:      6  Pattern Generator 6 Output Enable Buffer */
    uint16_t PGEB7:1;          /*!< bit:      7  Pattern Generator 7 Output Enable Buffer */
    uint16_t PGVB0:1;          /*!< bit:      8  Pattern Generator 0 Output Enable  */
    uint16_t PGVB1:1;          /*!< bit:      9  Pattern Generator 1 Output Enable  */
    uint16_t PGVB2:1;          /*!< bit:     10  Pattern Generator 2 Output Enable  */
    uint16_t PGVB3:1;          /*!< bit:     11  Pattern Generator 3 Output Enable  */
    uint16_t PGVB4:1;          /*!< bit:     12  Pattern Generator 4 Output Enable  */
    uint16_t PGVB5:1;          /*!< bit:     13  Pattern Generator 5 Output Enable  */
    uint16_t PGVB6:1;          /*!< bit:     14  Pattern Generator 6 Output Enable  */
    uint16_t PGVB7:1;          /*!< bit:     15  Pattern Generator 7 Output Enable  */
  } bit;                       /*!< Structure used for bit  access                  */
  struct {
    uint16_t PGEB:8;           /*!< bit:  0.. 7  Pattern Generator x Output Enable Buffer */
    uint16_t PGVB:8;           /*!< bit:  8..15  Pattern Generator x Output Enable  */
  } vec;                       /*!< Structure used for vec  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} TCC_PATTBUF_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TCC_PERBUF : (TCC Offset: 0x6C) (R/W 32) Period Buffer -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct { // DITH4 mode
    uint32_t DITHERBUF:4;      /*!< bit:  0.. 3  Dithering Buffer Cycle Number      */
    uint32_t PERBUF:20;        /*!< bit:  4..23  Period Buffer Value                */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } DITH4;                     /*!< Structure used for DITH4                        */
  struct { // DITH5 mode
    uint32_t DITHERBUF:5;      /*!< bit:  0.. 4  Dithering Buffer Cycle Number      */
    uint32_t PERBUF:19;        /*!< bit:  5..23  Period Buffer Value                */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } DITH5;                     /*!< Structure used for DITH5                        */
  struct { // DITH6 mode
    uint32_t DITHERBUF:6;      /*!< bit:  0.. 5  Dithering Buffer Cycle Number      */
    uint32_t PERBUF:18;        /*!< bit:  6..23  Period Buffer Value                */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } DITH6;                     /*!< Structure used for DITH6                        */
  struct {
    uint32_t PERBUF:24;        /*!< bit:  0..23  Period Buffer Value                */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} TCC_PERBUF_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- TCC_CCBUF : (TCC Offset: 0x70) (R/W 32) Compare and Capture Buffer -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct { // DITH4 mode
    uint32_t CCBUF:4;          /*!< bit:  0.. 3  Channel Compare/Capture Buffer Value */
    uint32_t DITHERBUF:20;     /*!< bit:  4..23  Dithering Buffer Cycle Number      */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } DITH4;                     /*!< Structure used for DITH4                        */
  struct { // DITH5 mode
    uint32_t DITHERBUF:5;      /*!< bit:  0.. 4  Dithering Buffer Cycle Number      */
    uint32_t CCBUF:19;         /*!< bit:  5..23  Channel Compare/Capture Buffer Value */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } DITH5;                     /*!< Structure used for DITH5                        */
  struct { // DITH6 mode
    uint32_t DITHERBUF:6;      /*!< bit:  0.. 5  Dithering Buffer Cycle Number      */
    uint32_t CCBUF:18;         /*!< bit:  6..23  Channel Compare/Capture Buffer Value */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } DITH6;                     /*!< Structure used for DITH6                        */
  struct {
    uint32_t CCBUF:24;         /*!< bit:  0..23  Channel Compare/Capture Buffer Value */
    uint32_t :8;               /*!< bit: 24..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} TCC_CCBUF_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief TCC hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO TCC_CTRLA_Type            CTRLA;       /**< \brief Offset: 0x00 (R/W 32) Control A */
  __IO TCC_CTRLBCLR_Type         CTRLBCLR;    /**< \brief Offset: 0x04 (R/W  8) Control B Clear */
  __IO TCC_CTRLBSET_Type         CTRLBSET;    /**< \brief Offset: 0x05 (R/W  8) Control B Set */
       RoReg8                    Reserved1[0x2];
  __I  TCC_SYNCBUSY_Type         SYNCBUSY;    /**< \brief Offset: 0x08 (R/  32) Synchronization Busy */
  __IO TCC_FCTRLA_Type           FCTRLA;      /**< \brief Offset: 0x0C (R/W 32) Recoverable Fault A Configuration */
  __IO TCC_FCTRLB_Type           FCTRLB;      /**< \brief Offset: 0x10 (R/W 32) Recoverable Fault B Configuration */
  __IO TCC_WEXCTRL_Type          WEXCTRL;     /**< \brief Offset: 0x14 (R/W 32) Waveform Extension Configuration */
  __IO TCC_DRVCTRL_Type          DRVCTRL;     /**< \brief Offset: 0x18 (R/W 32) Driver Control */
       RoReg8                    Reserved2[0x2];
  __IO TCC_DBGCTRL_Type          DBGCTRL;     /**< \brief Offset: 0x1E (R/W  8) Debug Control */
       RoReg8                    Reserved3[0x1];
  __IO TCC_EVCTRL_Type           EVCTRL;      /**< \brief Offset: 0x20 (R/W 32) Event Control */
  __IO TCC_INTENCLR_Type         INTENCLR;    /**< \brief Offset: 0x24 (R/W 32) Interrupt Enable Clear */
  __IO TCC_INTENSET_Type         INTENSET;    /**< \brief Offset: 0x28 (R/W 32) Interrupt Enable Set */
  __IO TCC_INTFLAG_Type          INTFLAG;     /**< \brief Offset: 0x2C (R/W 32) Interrupt Flag Status and Clear */
  __IO TCC_STATUS_Type           STATUS;      /**< \brief Offset: 0x30 (R/W 32) Status */
  __IO TCC_COUNT_Type            COUNT;       /**< \brief Offset: 0x34 (R/W 32) Count */
  __IO TCC_PATT_Type             PATT;        /**< \brief Offset: 0x38 (R/W 16) Pattern */
       RoReg8                    Reserved4[0x2];
  __IO TCC_WAVE_Type             WAVE;        /**< \brief Offset: 0x3C (R/W 32) Waveform Control */
  __IO TCC_PER_Type              PER;         /**< \brief Offset: 0x40 (R/W 32) Period */
  __IO TCC_CC_Type               CC[6];       /**< \brief Offset: 0x44 (R/W 32) Compare and Capture */
       RoReg8                    Reserved5[0x8];
  __IO TCC_PATTBUF_Type          PATTBUF;     /**< \brief Offset: 0x64 (R/W 16) Pattern Buffer */
       RoReg8                    Reserved6[0x6];
  __IO TCC_PERBUF_Type           PERBUF;      /**< \brief Offset: 0x6C (R/W 32) Period Buffer */
  __IO TCC_CCBUF_Type            CCBUF[6];    /**< \brief Offset: 0x70 (R/W 32) Compare and Capture Buffer */
} Tcc;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */


#endif /* _MICROCHIP_PIC32CXSG_TCC_COMPONENT_FIXUP_H_ */
