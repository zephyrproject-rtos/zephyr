/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_ADC_COMPONENT_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_ADC_COMPONENT_FIXUP_H_

/* -------- ADC_CTRLA : (ADC Offset: 0x00) (R/W 16) Control A -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t SWRST:1;          /*!< bit:      0  Software Reset                     */
    uint16_t ENABLE:1;         /*!< bit:      1  Enable                             */
    uint16_t :1;               /*!< bit:      2  Reserved                           */
    uint16_t DUALSEL:2;        /*!< bit:  3.. 4  Dual Mode Trigger Selection        */
    uint16_t SLAVEEN:1;        /*!< bit:      5  Slave Enable                       */
    uint16_t RUNSTDBY:1;       /*!< bit:      6  Run in Standby                     */
    uint16_t ONDEMAND:1;       /*!< bit:      7  On Demand Control                  */
    uint16_t PRESCALER:3;      /*!< bit:  8..10  Prescaler Configuration            */
    uint16_t :4;               /*!< bit: 11..14  Reserved                           */
    uint16_t R2R:1;            /*!< bit:     15  Rail to Rail Operation Enable      */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} ADC_CTRLA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- ADC_EVCTRL : (ADC Offset: 0x02) (R/W 8) Event Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  FLUSHEI:1;        /*!< bit:      0  Flush Event Input Enable           */
    uint8_t  STARTEI:1;        /*!< bit:      1  Start Conversion Event Input Enable */
    uint8_t  FLUSHINV:1;       /*!< bit:      2  Flush Event Invert Enable          */
    uint8_t  STARTINV:1;       /*!< bit:      3  Start Conversion Event Invert Enable */
    uint8_t  RESRDYEO:1;       /*!< bit:      4  Result Ready Event Out             */
    uint8_t  WINMONEO:1;       /*!< bit:      5  Window Monitor Event Out           */
    uint8_t  :2;               /*!< bit:  6.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} ADC_EVCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- ADC_DBGCTRL : (ADC Offset: 0x03) (R/W 8) Debug Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  DBGRUN:1;         /*!< bit:      0  Debug Run                          */
    uint8_t  :7;               /*!< bit:  1.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} ADC_DBGCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- ADC_INPUTCTRL : (ADC Offset: 0x04) (R/W 16) Input Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t MUXPOS:5;         /*!< bit:  0.. 4  Positive Mux Input Selection       */
    uint16_t :2;               /*!< bit:  5.. 6  Reserved                           */
    uint16_t DIFFMODE:1;       /*!< bit:      7  Differential Mode                  */
    uint16_t MUXNEG:5;         /*!< bit:  8..12  Negative Mux Input Selection       */
    uint16_t :2;               /*!< bit: 13..14  Reserved                           */
    uint16_t DSEQSTOP:1;       /*!< bit:     15  Stop DMA Sequencing                */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} ADC_INPUTCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- ADC_CTRLB : (ADC Offset: 0x06) (R/W 16) Control B -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t LEFTADJ:1;        /*!< bit:      0  Left-Adjusted Result               */
    uint16_t FREERUN:1;        /*!< bit:      1  Free Running Mode                  */
    uint16_t CORREN:1;         /*!< bit:      2  Digital Correction Logic Enable    */
    uint16_t RESSEL:2;         /*!< bit:  3.. 4  Conversion Result Resolution       */
    uint16_t :3;               /*!< bit:  5.. 7  Reserved                           */
    uint16_t WINMODE:3;        /*!< bit:  8..10  Window Monitor Mode                */
    uint16_t WINSS:1;          /*!< bit:     11  Window Single Sample               */
    uint16_t :4;               /*!< bit: 12..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} ADC_CTRLB_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- ADC_REFCTRL : (ADC Offset: 0x08) (R/W 8) Reference Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  REFSEL:4;         /*!< bit:  0.. 3  Reference Selection                */
    uint8_t  :3;               /*!< bit:  4.. 6  Reserved                           */
    uint8_t  REFCOMP:1;        /*!< bit:      7  Reference Buffer Offset Compensation Enable */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} ADC_REFCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- ADC_AVGCTRL : (ADC Offset: 0x0A) (R/W 8) Average Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  SAMPLENUM:4;      /*!< bit:  0.. 3  Number of Samples to be Collected  */
    uint8_t  ADJRES:3;         /*!< bit:  4.. 6  Adjusting Result / Division Coefficient */
    uint8_t  :1;               /*!< bit:      7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} ADC_AVGCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- ADC_SAMPCTRL : (ADC Offset: 0x0B) (R/W 8) Sample Time Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  SAMPLEN:6;        /*!< bit:  0.. 5  Sampling Time Length               */
    uint8_t  :1;               /*!< bit:      6  Reserved                           */
    uint8_t  OFFCOMP:1;        /*!< bit:      7  Comparator Offset Compensation Enable */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} ADC_SAMPCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- ADC_WINLT : (ADC Offset: 0x0C) (R/W 16) Window Monitor Lower Threshold -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t WINLT:16;         /*!< bit:  0..15  Window Lower Threshold             */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} ADC_WINLT_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- ADC_WINUT : (ADC Offset: 0x0E) (R/W 16) Window Monitor Upper Threshold -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t WINUT:16;         /*!< bit:  0..15  Window Upper Threshold             */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} ADC_WINUT_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- ADC_GAINCORR : (ADC Offset: 0x10) (R/W 16) Gain Correction -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t GAINCORR:12;      /*!< bit:  0..11  Gain Correction Value              */
    uint16_t :4;               /*!< bit: 12..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} ADC_GAINCORR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */
/* -------- ADC_OFFSETCORR : (ADC Offset: 0x12) (R/W 16) Offset Correction -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t OFFSETCORR:12;    /*!< bit:  0..11  Offset Correction Value            */
    uint16_t :4;               /*!< bit: 12..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} ADC_OFFSETCORR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- ADC_SWTRIG : (ADC Offset: 0x14) (R/W 8) Software Trigger -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  FLUSH:1;          /*!< bit:      0  ADC Conversion Flush               */
    uint8_t  START:1;          /*!< bit:      1  Start ADC Conversion               */
    uint8_t  :6;               /*!< bit:  2.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} ADC_SWTRIG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- ADC_INTENCLR : (ADC Offset: 0x2C) (R/W 8) Interrupt Enable Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  RESRDY:1;         /*!< bit:      0  Result Ready Interrupt Disable     */
    uint8_t  OVERRUN:1;        /*!< bit:      1  Overrun Interrupt Disable          */
    uint8_t  WINMON:1;         /*!< bit:      2  Window Monitor Interrupt Disable   */
    uint8_t  :5;               /*!< bit:  3.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} ADC_INTENCLR_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- ADC_INTENSET : (ADC Offset: 0x2D) (R/W 8) Interrupt Enable Set -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  RESRDY:1;         /*!< bit:      0  Result Ready Interrupt Enable      */
    uint8_t  OVERRUN:1;        /*!< bit:      1  Overrun Interrupt Enable           */
    uint8_t  WINMON:1;         /*!< bit:      2  Window Monitor Interrupt Enable    */
    uint8_t  :5;               /*!< bit:  3.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} ADC_INTENSET_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- ADC_INTFLAG : (ADC Offset: 0x2E) (R/W 8) Interrupt Flag Status and Clear -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union { // __I to avoid read-modify-write on write-to-clear register
  struct {
    __I uint8_t  RESRDY:1;         /*!< bit:      0  Result Ready Interrupt Flag        */
    __I uint8_t  OVERRUN:1;        /*!< bit:      1  Overrun Interrupt Flag             */
    __I uint8_t  WINMON:1;         /*!< bit:      2  Window Monitor Interrupt Flag      */
    __I uint8_t  :5;               /*!< bit:  3.. 7  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} ADC_INTFLAG_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- ADC_STATUS : (ADC Offset: 0x2F) ( R/ 8) Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint8_t  ADCBUSY:1;        /*!< bit:      0  ADC Busy Status                    */
    uint8_t  :1;               /*!< bit:      1  Reserved                           */
    uint8_t  WCC:6;            /*!< bit:  2.. 7  Window Comparator Counter          */
  } bit;                       /*!< Structure used for bit  access                  */
  uint8_t reg;                 /*!< Type      used for register access              */
} ADC_STATUS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- ADC_SYNCBUSY : (ADC Offset: 0x30) ( R/ 32) Synchronization Busy -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t SWRST:1;          /*!< bit:      0  SWRST Synchronization Busy         */
    uint32_t ENABLE:1;         /*!< bit:      1  ENABLE Synchronization Busy        */
    uint32_t INPUTCTRL:1;      /*!< bit:      2  Input Control Synchronization Busy */
    uint32_t CTRLB:1;          /*!< bit:      3  Control B Synchronization Busy     */
    uint32_t REFCTRL:1;        /*!< bit:      4  Reference Control Synchronization Busy */
    uint32_t AVGCTRL:1;        /*!< bit:      5  Average Control Synchronization Busy */
    uint32_t SAMPCTRL:1;       /*!< bit:      6  Sampling Time Control Synchronization Busy */
    uint32_t WINLT:1;          /*!< bit:      7  Window Monitor Lower Threshold Synchronization Busy */
    uint32_t WINUT:1;          /*!< bit:      8  Window Monitor Upper Threshold Synchronization Busy */
    uint32_t GAINCORR:1;       /*!< bit:      9  Gain Correction Synchronization Busy */
    uint32_t OFFSETCORR:1;     /*!< bit:     10  Offset Correction Synchronization Busy */
    uint32_t SWTRIG:1;         /*!< bit:     11  Software Trigger Synchronization Busy */
    uint32_t :20;              /*!< bit: 12..31  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} ADC_SYNCBUSY_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- ADC_DSEQDATA : (ADC Offset: 0x34) ( /W 32) DMA Sequencial Data -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t DATA:32;          /*!< bit:  0..31  DMA Sequential Data                */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} ADC_DSEQDATA_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- ADC_DSEQCTRL : (ADC Offset: 0x38) (R/W 32) DMA Sequential Control -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t INPUTCTRL:1;      /*!< bit:      0  Input Control                      */
    uint32_t CTRLB:1;          /*!< bit:      1  Control B                          */
    uint32_t REFCTRL:1;        /*!< bit:      2  Reference Control                  */
    uint32_t AVGCTRL:1;        /*!< bit:      3  Average Control                    */
    uint32_t SAMPCTRL:1;       /*!< bit:      4  Sampling Time Control              */
    uint32_t WINLT:1;          /*!< bit:      5  Window Monitor Lower Threshold     */
    uint32_t WINUT:1;          /*!< bit:      6  Window Monitor Upper Threshold     */
    uint32_t GAINCORR:1;       /*!< bit:      7  Gain Correction                    */
    uint32_t OFFSETCORR:1;     /*!< bit:      8  Offset Correction                  */
    uint32_t :22;              /*!< bit:  9..30  Reserved                           */
    uint32_t AUTOSTART:1;      /*!< bit:     31  ADC Auto-Start Conversion          */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} ADC_DSEQCTRL_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- ADC_DSEQSTAT : (ADC Offset: 0x3C) ( R/ 32) DMA Sequencial Status -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint32_t INPUTCTRL:1;      /*!< bit:      0  Input Control                      */
    uint32_t CTRLB:1;          /*!< bit:      1  Control B                          */
    uint32_t REFCTRL:1;        /*!< bit:      2  Reference Control                  */
    uint32_t AVGCTRL:1;        /*!< bit:      3  Average Control                    */
    uint32_t SAMPCTRL:1;       /*!< bit:      4  Sampling Time Control              */
    uint32_t WINLT:1;          /*!< bit:      5  Window Monitor Lower Threshold     */
    uint32_t WINUT:1;          /*!< bit:      6  Window Monitor Upper Threshold     */
    uint32_t GAINCORR:1;       /*!< bit:      7  Gain Correction                    */
    uint32_t OFFSETCORR:1;     /*!< bit:      8  Offset Correction                  */
    uint32_t :22;              /*!< bit:  9..30  Reserved                           */
    uint32_t BUSY:1;           /*!< bit:     31  DMA Sequencing Busy                */
  } bit;                       /*!< Structure used for bit  access                  */
  uint32_t reg;                /*!< Type      used for register access              */
} ADC_DSEQSTAT_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- ADC_RESULT : (ADC Offset: 0x40) ( R/ 16) Result Conversion Value -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t RESULT:16;        /*!< bit:  0..15  Result Conversion Value            */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} ADC_RESULT_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- ADC_RESS : (ADC Offset: 0x44) ( R/ 16) Last Sample Result -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t RESS:16;          /*!< bit:  0..15  Last ADC conversion result         */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} ADC_RESS_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* -------- ADC_CALIB : (ADC Offset: 0x48) (R/W 16) Calibration -------- */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef union {
  struct {
    uint16_t BIASCOMP:3;       /*!< bit:  0.. 2  Bias Comparator Scaling            */
    uint16_t :1;               /*!< bit:      3  Reserved                           */
    uint16_t BIASR2R:3;        /*!< bit:  4.. 6  Bias R2R Ampli scaling             */
    uint16_t :1;               /*!< bit:      7  Reserved                           */
    uint16_t BIASREFBUF:3;     /*!< bit:  8..10  Bias  Reference Buffer Scaling     */
    uint16_t :5;               /*!< bit: 11..15  Reserved                           */
  } bit;                       /*!< Structure used for bit  access                  */
  uint16_t reg;                /*!< Type      used for register access              */
} ADC_CALIB_Type;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/** \brief ADC hardware registers */
#if !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
typedef struct {
  __IO ADC_CTRLA_Type            CTRLA;       /**< \brief Offset: 0x00 (R/W 16) Control A */
  __IO ADC_EVCTRL_Type           EVCTRL;      /**< \brief Offset: 0x02 (R/W  8) Event Control */
  __IO ADC_DBGCTRL_Type          DBGCTRL;     /**< \brief Offset: 0x03 (R/W  8) Debug Control */
  __IO ADC_INPUTCTRL_Type        INPUTCTRL;   /**< \brief Offset: 0x04 (R/W 16) Input Control */
  __IO ADC_CTRLB_Type            CTRLB;       /**< \brief Offset: 0x06 (R/W 16) Control B */
  __IO ADC_REFCTRL_Type          REFCTRL;     /**< \brief Offset: 0x08 (R/W  8) Reference Control */
       RoReg8                    Reserved1[0x1];
  __IO ADC_AVGCTRL_Type          AVGCTRL;     /**< \brief Offset: 0x0A (R/W  8) Average Control */
  __IO ADC_SAMPCTRL_Type         SAMPCTRL;    /**< \brief Offset: 0x0B (R/W  8) Sample Time Control */
  __IO ADC_WINLT_Type            WINLT;       /**< \brief Offset: 0x0C (R/W 16) Window Monitor Lower Threshold */
  __IO ADC_WINUT_Type            WINUT;       /**< \brief Offset: 0x0E (R/W 16) Window Monitor Upper Threshold */
  __IO ADC_GAINCORR_Type         GAINCORR;    /**< \brief Offset: 0x10 (R/W 16) Gain Correction */
  __IO ADC_OFFSETCORR_Type       OFFSETCORR;  /**< \brief Offset: 0x12 (R/W 16) Offset Correction */
  __IO ADC_SWTRIG_Type           SWTRIG;      /**< \brief Offset: 0x14 (R/W  8) Software Trigger */
       RoReg8                    Reserved2[0x17];
  __IO ADC_INTENCLR_Type         INTENCLR;    /**< \brief Offset: 0x2C (R/W  8) Interrupt Enable Clear */
  __IO ADC_INTENSET_Type         INTENSET;    /**< \brief Offset: 0x2D (R/W  8) Interrupt Enable Set */
  __IO ADC_INTFLAG_Type          INTFLAG;     /**< \brief Offset: 0x2E (R/W  8) Interrupt Flag Status and Clear */
  __I  ADC_STATUS_Type           STATUS;      /**< \brief Offset: 0x2F (R/   8) Status */
  __I  ADC_SYNCBUSY_Type         SYNCBUSY;    /**< \brief Offset: 0x30 (R/  32) Synchronization Busy */
  __O  ADC_DSEQDATA_Type         DSEQDATA;    /**< \brief Offset: 0x34 ( /W 32) DMA Sequencial Data */
  __IO ADC_DSEQCTRL_Type         DSEQCTRL;    /**< \brief Offset: 0x38 (R/W 32) DMA Sequential Control */
  __I  ADC_DSEQSTAT_Type         DSEQSTAT;    /**< \brief Offset: 0x3C (R/  32) DMA Sequencial Status */
  __I  ADC_RESULT_Type           RESULT;      /**< \brief Offset: 0x40 (R/  16) Result Conversion Value */
       RoReg8                    Reserved3[0x2];
  __I  ADC_RESS_Type             RESS;        /**< \brief Offset: 0x44 (R/  16) Last Sample Result */
       RoReg8                    Reserved4[0x2];
  __IO ADC_CALIB_Type            CALIB;       /**< \brief Offset: 0x48 (R/W 16) Calibration */
} Adc;
#endif /* !(defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_ADC_COMPONENT_FIXUP_H_ */
