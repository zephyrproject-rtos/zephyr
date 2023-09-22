/**************************************************************************//**
 * @file     afan.h
 * @brief    Define AFAN's register and function
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef AFAN_H
#define AFAN_H

/**
  \brief  Structure type to access Auto FAN Controller (AFAN).
 */
typedef volatile struct
{
  uint32_t AFAN0CFG;                                  /*Configuration Register */
  uint8_t  AFAN0STS;                                  /*Status Register */
  uint8_t  Reserved0[3];                              /*Reserved */
  uint16_t AFAN0SET;                                  /*AFAN0 Speed Set Counter Register */
  uint16_t Reserved1;                                 /*Reserved */ 
  uint32_t Reserved2;                                 /*Reserved */                        
  uint32_t AFAN1CFG;                                  /*Configuration Register */
  uint8_t  AFAN1STS;                                  /*Status Register */
  uint8_t  Reserved3[3];                              /*Reserved */
  uint16_t AFAN1SET;                                  /*AFAN1 Speed Set Counter Register */
  uint16_t Reserved4;                                 /*Reserved */ 
  uint32_t Reserved5;                                 /*Reserved */   
}  AFAN_T;

#define afan                ((AFAN_T *) AFAN_BASE)             /* AFAN Struct       */

#define AFAN                ((volatile unsigned long  *) AFAN_BASE)     /* AFAN  array       */

//***************************************************************
// User Define                                                
//***************************************************************
#define AUTO_QFAN                           USR_AUTO_QFAN
    #define FAN0_AUTOFAN_SUPPORT            USR_FAN0_AUTOFAN_SUPPORT         
        #define FAN0_Start_Duty             USR_FAN0_Start_Duty         // Duty cycle
        #define FAN0_Start_FREQ             USR_FAN0_Start_FREQ         // Unit:Hz
        #define FAN0_AUTO_LOAD              USR_FAN0_AUTO_LOAD    
            #define FAN0_AUTO_LOAD_PERCENT  USR_FAN0_AUTO_LOAD_PERCENT    
            
    #define FAN1_AUTOFAN_SUPPORT            USR_FAN1_AUTOFAN_SUPPORT      
        #define FAN1_Start_Duty             USR_FAN1_Start_Duty         // Duty cycle
        #define FAN1_Start_FREQ             USR_FAN1_Start_FREQ         // Unit:Hz
        #define FAN1_AUTO_LOAD              USR_FAN1_AUTO_LOAD       
            #define FAN1_AUTO_LOAD_PERCENT  USR_FAN1_AUTO_LOAD_PERCENT
            
//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------
#define AFAN_NUM                            2

#define AFAN_CFG_OFFSET                     0
#define AFAN_STS_OFFSET                     1
#define AFAN_SET_OFFSET                     2


#define AFAN0                               0x00
#define AFAN1                               0x01
#define AFAN_NoMapping                      0xFE

#define AFAN_AUTO_LOAD_PERCENT_100          4/4
#define AFAN_AUTO_LOAD_PERCENT_75           3/4
#define AFAN_AUTO_LOAD_PERCENT_50           2/4
#define AFAN_AUTO_LOAD_PERCENT_25           1/4

//-- Macro Define -----------------------------------------------
#define mAFAN_Enable(FAN_Num)               AFAN[AFAN_CFG_OFFSET+(FAN_Num<<2)] |= 1UL
#define mAFAN_Disable(FAN_Num)              AFAN[AFAN_CFG_OFFSET+(FAN_Num<<2)] &= ~1UL

#define mAFAN_Set_Counter(FAN_Num,Cnt)      AFAN[AFAN_SET_OFFSET+(FAN_Num<<2)] = Cnt

#define mAFAN_AutoLoad_Enable(FAN_Num)      AFAN[AFAN_CFG_OFFSET+(FAN_Num<<2)] |= 0x80000000
#define mAFAN_Set_AutoLoad_HV(FAN_Num,val)  AFAN[AFAN_CFG_OFFSET+(FAN_Num<<2)] = (AFAN[AFAN_CFG_OFFSET+(FAN_Num<<2)]&(~0x3FFF0000))|((val&0x00003FFF)<<16)

#define mEnable_AutoFanGPIO(FAN_Num)        {FANPWM_GPIO_Enable(FAN_Num); mTacho_GPIO_Enable(FAN_Num); }
#define mDisable_AutoFanGPIO(FAN_Num)       {FANPWM_GPIO_Disable(FAN_Num); mTacho_GPIO_Disable(FAN_Num); }

#define mAFAN_Start_DUTY_Select(FAN_Num)    (((FAN_Num)==AFAN0)?FAN0_Start_Duty:FAN1_Start_Duty)
#define mAFAN_Start_FREQ_Select(FAN_Num)    (((FAN_Num)==AFAN0)?FAN0_Start_FREQ:FAN1_Start_FREQ)

//#define mAFAN_GetArg(CLK, ARG, DIV)         (unsigned short)((60000000>>(6-CLK))/(unsigned long)(ARG*DIV))
#define mAFAN_GetArg(CLK, ARG, DIV)         (unsigned short)((60000000)/(unsigned long)(Tacho_clock_base[CLK]*ARG*DIV))

#define mAFAN_GetCNT(CLK, RPM, DIV)         mAFAN_GetArg(CLK, RPM, DIV)

//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************

//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use -----------------------------------------------

//-- For OEM Use -----------------------------------------------
void AutoFan_Init(unsigned char bFAN_CH, unsigned short FAN_RPM, unsigned char OpenDrainEN);
void AutoFan_RPM_Set(unsigned char bFAN_Ch, unsigned short wRPM);
#endif //AFAN_H
