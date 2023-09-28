/****************************************************************************
 * @file     tacho.h
 * @brief    Define TACHO's register and function
 *
 * @version  V1.0.0
 * @date     02. July 2021
****************************************************************************/

#ifndef TACHO_H
#define TACHO_H

/**
  \brief  Structure type to access TACHOTACHOTACHOx (TACHO).
 */
typedef volatile struct
{
  uint16_t TACHO0CFG;                                 /*Configuration Register */
  uint16_t Reserved0;                                 /*Reserved */
  uint8_t  TACHO0IE;                                  /*Interrupt Enable Register */
  uint8_t  Reserved1[3];                              /*Reserved */
  uint8_t  TACHO0PF;                                  /*Event Pending Flag Register */
  uint8_t  Reserved2[3];                              /*Reserved */
  uint16_t TACHO0CV;                                  /*TACHO0 Counter Value Register */
  uint16_t Reserved3;                                 /*Reserved */
  uint16_t TACHO1CFG;                                 /*Configuration Register */
  uint16_t Reserved4;                                 /*Reserved */
  uint8_t  TACHO1IE;                                  /*Interrupt Enable Register */
  uint8_t  Reserved5[3];                              /*Reserved */
  uint8_t  TACHO1PF;                                  /*Event Pending Flag Register */
  uint8_t  Reserved6[3];                              /*Reserved */
  uint16_t TACHO1CV;                                  /*TACHO1 Counter Value Register */
  uint16_t Reserved7;                                 /*Reserved */
  uint16_t TACHO2CFG;                                 /*Configuration Register */
  uint16_t Reserved8;                                 /*Reserved */
  uint8_t  TACHO2IE;                                  /*Interrupt Enable Register */
  uint8_t  Reserved9[3];                              /*Reserved */
  uint8_t  TACHO2PF;                                  /*Event Pending Flag Register */
  uint8_t  Reserved10[3];                             /*Reserved */
  uint16_t TACHO2CV;                                  /*TACHO2 Counter Value Register */
  uint16_t Reserved11;                                /*Reserved */
  uint16_t TACHO3CFG;                                 /*Configuration Register */
  uint16_t Reserved12;                                /*Reserved */
  uint8_t  TACHO3IE;                                  /*Interrupt Enable Register */
  uint8_t  Reserved13[3];                             /*Reserved */
  uint8_t  TACHO3PF;                                  /*Event Pending Flag Register */
  uint8_t  Reserved14[3];                             /*Reserved */
  uint16_t TACHO3CV;                                  /*TACHO3 Counter Value Register */
  uint16_t Reserved15;                                /*Reserved */
}  TACHO_T;

//-- Constant Define -------------------------------------------
#define TACHO_FANFB_NUM                     4

#define TACHO0_GPIO_Num                     0x48
#define TACHO1_GPIO_Num                     0x4E
#define TACHO2_GPIO_Num                     0x0E
#define TACHO3_GPIO_Num                     0x43

#define TACHO_CFG_OFFSET                    0
#define TACHO_IE_OFFSET                     1
#define TACHO_PF_OFFSET                     2
#define TACHO_CV_OFFSET                     3

#define TACHO_CNT_MAX_VALUE                 0x7FFF

#define TACHO_MONITOR_CLK_64us              0
#define TACHO_MONITOR_CLK_16us              1
#define TACHO_MONITOR_CLK_8us               2
#define TACHO_MONITOR_CLK_2us               3

#define TACHO_Ring_Edge_Sample              0
#define TACHO_Edge_Change_Sample            1

//-- Macro Define -----------------------------------------------
// FAN TACHOMETER
#define mTacho_GPIO_Enable(Tacho_Num)                   do { mGPIOIE_En(TACHO_GPIO_FANFB[Tacho_Num]);  } while (0)
#define mTacho_GPIO_Disable(Tacho_Num)                  do { mGPIOIE_Dis(TACHO_GPIO_FANFB[Tacho_Num]); } while (0)

// FAN Tachometer Function
#define mTacho_Function_Enable(Tacho_Num)               TACHO[TACHO_CFG_OFFSET+(Tacho_Num<<2)] |= 0x01
#define mTacho_Function_Disable(Tacho_Num)              TACHO[TACHO_CFG_OFFSET+(Tacho_Num<<2)] &= ~0x01

#define mTacho_SampleClock_Get(Tacho_Num)               ((TACHO[TACHO_CFG_OFFSET+(Tacho_Num<<2)]&0x30)>>4)
#define mTacho_SampleClock_Clr(Tacho_Num)               TACHO[TACHO_CFG_OFFSET+(Tacho_Num<<2)] &= ~0x30
#define mTacho_SampleClock_Wr(Tacho_Num, bMoniClk)      TACHO[TACHO_CFG_OFFSET+(Tacho_Num<<2)] |= ((bMoniClk&0x03)<<4)
#define mTacho_SampleClock_Inti(Tacho_Num, bMoniClk)    do {mTacho_SampleClock_Clr(Tacho_Num); mTacho_SampleClock_Wr(Tacho_Num, bMoniClk);} while (0)

#define mTacho_TrgMode_Clr(Tacho_Num)                   TACHO[TACHO_CFG_OFFSET+(Tacho_Num<<2)] &= ~0x80
#define mTacho_TrgMode_Wr(Tacho_Num, TrgMode)           TACHO[TACHO_CFG_OFFSET+(Tacho_Num<<2)] |= ((TrgMode&0x01)<<7)
#define mTacho_TrgMode_Inti(Tacho_Num, TrgMode)         do {mTacho_TrgMode_Clr(Tacho_Num); mTacho_TrgMode_Wr(Tacho_Num, TrgMode); } while (0)

//FAN Tachometer Filter
//Filter Time = n*T  n:FILTER_TIMES(1~16)  T:Monitor_Clock(SampleClock)
#define mTacho_Filter_Enable(Tacho_Num)                 TACHO[TACHO_CFG_OFFSET+(Tacho_Num<<2)] |= 0x8000
#define mTacho_Filter_Disable(Tacho_Num)                TACHO[TACHO_CFG_OFFSET+(Tacho_Num<<2)] &= ~0x8000
#define mTacho_FilterTimes_Set(Tacho_Num, Times)        do {TACHO[TACHO_CFG_OFFSET+(Tacho_Num<<2)] &= ~0x1F00; TACHO[TACHO_CFG_OFFSET+(Tacho_Num<<2)] |= ((Times-1)&0x1F)<<8;} while (0)

//FAN Tachometer Pending Flag
#define mTacho_PF_Clr(Tacho_Num)                        TACHO[TACHO_PF_OFFSET+(Tacho_Num<<2)] = 0x03
#define mTacho_TimeoutPF_Clr(Tacho_Num)                 TACHO[TACHO_PF_OFFSET+(Tacho_Num<<2)] = 0x02
#define mTacho_UpdatePF_Clr(Tacho_Num)                  TACHO[TACHO_PF_OFFSET+(Tacho_Num<<2)] = 0x01
#define mTacho_TimeoutEvent(Tacho_Num)                  ((TACHO[TACHO_PF_OFFSET+(Tacho_Num<<2)]&0x02)>>1)
#define mTacho_UpdateEvent(Tacho_Num)                   (TACHO[TACHO_PF_OFFSET+(Tacho_Num<<2)]&0x01)

//FAN Tachometer Interrupt Enable
#define mTacho_Timeout_InterEn(Tacho_Num)               TACHO[TACHO_IE_OFFSET+(Tacho_Num<<2)] |= 0x02
#define mTacho_Timeout_InterDis(Tacho_Num)              TACHO[TACHO_IE_OFFSET+(Tacho_Num<<2)] &= ~0x02
#define mTacho_Update_InterEn(Tacho_Num)                TACHO[TACHO_IE_OFFSET+(Tacho_Num<<2)] |= 0x01
#define mTacho_Update_InterDis(Tacho_Num)               TACHO[TACHO_IE_OFFSET+(Tacho_Num<<2)] &= ~0x01

#define mTacho_SPEED_COUNTER(Tacho_Num)                 TACHO[TACHO_CV_OFFSET+(Tacho_Num<<2)]

#define mTacho_GetArg(CLK, ARG, DIV)                    (unsigned short)((60000000)/(unsigned long)(Tacho_clock_base[CLK]*ARG*DIV))
#define mTacho_GetRPM(CLK, CNT, DIV)                    mTacho_GetArg(CLK, CNT, DIV)

#endif //TACHO_H
