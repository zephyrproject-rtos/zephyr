/**************************************************************************//**
 * @file     vc.h
 * @brief    Define VC's register and function
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef VC_H
#define VC_H

/**
  \brief  Structure type to access Voltage Comparator (VC).
 */
typedef volatile struct
{
  uint8_t  VC0CFGC;                                   /*VC0 Configuration Register */
  uint8_t  VC0CFGDAC;                                 /*VC0 DAC Register */
  uint16_t Reserved0;                                 /*Reserved */ 
  uint8_t  VC0STS;                                    /*VC0 Status Register */
  uint8_t  Reserved1[3];                              /*Reserved */
  uint32_t Reserved2[2];                              /*Reserved */        
  uint8_t  VC1CFGC;                                   /*VC1 Configuration Register */
  uint8_t  VC1CFGDAC;                                 /*VC1 DAC Register */
  uint16_t Reserved3;                                 /*Reserved */ 
  uint8_t  VC1STS;                                    /*VC1 Status Register */
  uint8_t  Reserved4[3];                              /*Reserved */
//  uint8_t  VC1SRCCFGPF;                               /*VC1 Source Power Fail Register */
//  uint8_t  VC1SRCCFGAD0;                              /*VC1 Source AD0 Register */
//  uint8_t  VC1SRCCFGAD1;                              /*VC1 Source AD1 Fail Register */
//  uint8_t  VC1SRCCFGAI;                               /*VC1 Source AC IN Register */                
}  VC_T;

#define vc                ((VC_T *) VC_BASE)             /* VC Struct       */

#define VC                ((volatile unsigned long  *) VC_BASE)     /* VC  array       */

//***************************************************************
// User Define                                                
//***************************************************************
// VC0 Setting 
#define VC0_SUPPORT                         USR_VC0_SUPPORT                    
    #define VC0_VCOUT0_TYPE_SEL             USR_VC0_VCOUT0_TYPE_SEL        
    #define VC0_VCOUT0_ACTIVE_POLARITY      USR_VC0_VCOUT0_ACTIVE_POLARITY 
    #define VC0_VCIN0_COMP_SRC_SELECT       USR_VC0_VCIN0_COMP_SRC_SELECT  
    #define VC0_VCIN0_COMP_SRC_DAC_INIT     USR_VC0_VCIN0_COMP_SRC_DAC_INIT
    #define VC0_VCIN0_COMP_POLAR            USR_VC0_VCIN0_COMP_POLAR       
    #define VC0_VCIN0_DEBOUNCETIME          USR_VC0_VCIN0_DEBOUNCETIME     
    #define VC0_VCIN0_ACTIVE_BY_STATUS      USR_VC0_VCIN0_ACTIVE_BY_STATUS 
    
// VC1 Setting  
#define VC1_SUPPORT                         USR_VC1_SUPPORT                    
    #define VC1_VCOUT1_TYPE_SEL             USR_VC1_VCOUT1_TYPE_SEL        
    #define VC1_VCOUT1_ACTIVE_POLARITY      USR_VC1_VCOUT1_ACTIVE_POLARITY 

// VCIN1 Setting
    #define VC1_VCIN1_SUPPORT               USR_VC1_VCIN1_SUPPORT
    #define VC1_VCIN1_COMP_SRC_SELECT       USR_VC1_VCIN1_COMP_SRC_SELECT  
    #define VC1_VCIN1_COMP_SRC_DAC_INIT     USR_VC1_VCIN1_COMP_SRC_DAC_INIT
    #define VC1_VCIN1_COMP_POLAR            USR_VC1_VCIN1_COMP_POLAR       
    #define VC1_VCIN1_DEBOUNCETIME          USR_VC1_VCIN1_DEBOUNCETIME     
    #define VC1_VCIN1_ACTIVE_BY_STATUS      USR_VC1_VCIN1_ACTIVE_BY_STATUS 
    
//// VC1 AD0 Setting  
//    #define VC1_AD0_SUPPORT                 USR_VC1_AD0_SUPPORT                
//        #define VC1_AD0_COMP_SRC_DAC_INIT   USR_VC1_AD0_COMP_SRC_DAC_INIT
//        #define VC1_AD0_COMP_POLAR          USR_VC1_AD0_COMP_POLAR         
//        #define VC1_AD0_DEBOUNCETIME        USR_VC1_AD0_DEBOUNCETIME   
//            
//// VC1 AD1 Setting 
//    #define VC1_AD1_SUPPORT                 USR_VC1_AD1_SUPPORT                
//        #define VC1_AD1_COMP_POLAR          USR_VC1_AD1_COMP_POLAR         
//        #define VC1_AD1_DEBOUNCETIME        USR_VC1_AD1_DEBOUNCETIME  
//             
//// VC1 AC IN Setting  
//    #define VC1_AC_IN_SUPPORT               USR_VC1_AC_IN_SUPPORT              
//        #define VC1_AC_IN_DEBOUNCETIME      USR_VC1_AC_IN_DEBOUNCETIME 
        
//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------
#define VC_VCOUT0_GPIO_Num                  0x67
#define VC_VCIN0_GPIO_Num                   0x78
#define VC_VCOUT1_GPIO_Num                  0x66
#define VC_VCIN1_GPIO_Num                   0x65
#define VC_AC_IN_GPIO_Num                   0x79
// VC_VCOUT0_ACTPOLAR
#define VC_VCOUT0_ACT_HIGH                  0x01
#define VC_VCOUT0_ACT_LOW                   0x00
// VC_VCOUT0_OUTPUTTYPE        
#define VC_VCOUT0_OUTPUTTYPE_OPEN_DRAIN     0x01
#define VC_VCOUT0_OUTPUTTYPE_PUSH_PULL      0x00    
// VC_VCIN0_CMPSRC
#define VC_VCIN0_CMPSRC_DAC                 0x00
#define VC_VCIN0_CMPSRC_1V_FIX              0x01  
// VC_VCIN0_CMPPOLAR        
#define VC_VCIN0_CMPPOLAR_HIGH              0x01
#define VC_VCIN0_CMPPOLAR_LOW               0x00  
 // VC_VCIN0_DEBOUNCE
#define VCIN0_DEBOUNCE_NO                   0x00
#define VCIN0_DEBOUNCE_16ms                 0x01
#define VCIN0_DEBOUNCE_32ms                 0x02
#define VCIN0_DEBOUNCE_64ms                 0x03  
// VC_VCIN0_DRIVETYPE
#define VC_VCIN0_DRIVETYPE_BY_STATUS        0x01
#define VC_VCIN0_DRIVETYPE_BY_PF            0x00     
// VC_VCOUT1_ACTPOLAR
#define VC_VCOUT1_ACT_HIGH                  0x01
#define VC_VCOUT1_ACT_LOW                   0x00
// VC_VCOUT1_OUTPUTTYPE
#define VC_VCOUT1_OUTPUTTYPE_OPEN_DRAIN     0x01
#define VC_VCOUT1_OUTPUTTYPE_PUSH_PULL      0x00
// VC_VCIN1_CMPSRC
#define VC_VCIN1_CMPSRC_DAC                 0x00
#define VC_VCIN1_CMPSRC_1V_FIX              0x01
// VC_VCIN1_CMPPOLAR
#define VC_VCIN1_CMPPOLAR_HIGH              0x01
#define VC_VCIN1_CMPPOLAR_LOW               0x00
// VC_VCIN1_DEBOUNCE
#define VCIN1_DEBOUNCE_NO                   0x00
#define VCIN1_DEBOUNCE_60us                 0x01
#define VCIN1_DEBOUNCE_120us                0x02
#define VCIN1_DEBOUNCE_240us                0x03
// VC_VCIN1_DRIVETYPE
#define VC_VCIN1_DRIVETYPE_BY_STATUS        0x01
#define VC_VCIN1_DRIVETYPE_BY_PF            0x00
//// VC_AD0_CMPPOLAR 
//#define VC_AD0_CMPPOLAR_HIGH                0x00
//#define VC_AD0_CMPPOLAR_LOW                 0x01
//// VC_AD0_DEBOUNCE
//#define VC_AD0_DEBOUNCE_NO                  0x00
//#define VC_AD0_DEBOUNCE_60us                0x01
//#define VC_AD0_DEBOUNCE_120us               0x02
//#define VC_AD0_DEBOUNCE_240us               0x03
//// VC_AD1_CMPPOLAR
//#define VC_AD1_CMPPOLAR_HIGH                0x00
//#define VC_AD1_CMPPOLAR_LOW                 0x01
//// VC_AD1_DEBOUNCE
//#define VC_AD1_DEBOUNCE_NO                  0x00
//#define VC_AD1_DEBOUNCE_60us                0x01
//#define VC_AD1_DEBOUNCE_120us               0x02
//#define VC_AD1_DEBOUNCE_240us               0x03
//// VC_AC_IN_DEBOUNCE
//#define VC_AC_IN_DEBOUNCE_NO                0x00
//#define VC_AC_IN_DEBOUNCE_60us              0x01
//#define VC_AC_IN_DEBOUNCE_120us             0x02
//#define VC_AC_IN_DEBOUNCE_240us             0x03

//-- Macro Define -----------------------------------------------
// VC VCOUT0  
#define mVC_VCOUT0_ActPolar_Set(VCOUT0_ACTPOLAR)        vc->VC0CFGC = (vc->VC0CFGC&(~0x04))|((VCOUT0_ACTPOLAR&0x01)<<2)
#define mVC_VCOUT0_OutputType_Set(VCOUT0_OUTPUTTYPE)    vc->VC0CFGC = (vc->VC0CFGC&(~0x02))|((VCOUT0_OUTPUTTYPE&0x01)<<1)
// VC VCIN0                                             
#define mVC_VCIN0_CmpSrc_Get                            (vc->VC0CFGC>>7)
#define mVC_VCIN0_CmpSrc_Set(VCIN0_CMPSRC)              vc->VC0CFGC = (vc->VC0CFGC&(~0x80))|((VCIN0_CMPSRC&0x01)<<7)
#define mVC_VCIN0_CmpSrc_DAC_Get                        (vc->VC0CFGDAC)
#define mVC_VCIN0_CmpSrc_DAC_Set(VCIN0_DAC)             mVC_VCIN0_CmpSrc_DAC_Get = VCIN0_DAC
#define mVC_VCIN0_CmpPolar_Get                          ((vc->VC0CFGC&0x40)>>6)
#define mVC_VCIN0_CmpPolar_Set(VCIN0_POLAR)             vc->VC0CFGC =(vc->VC0CFGC&(~0x40))|((VCIN0_POLAR&0x01)<<6)
#define mVC_VCIN0_Debounce_Get                          ((vc->VC0CFGC&0x30)>>4)
#define mVC_VCIN0_Debounce_Set(VCIN0_DEBOUNCE)          vc->VC0CFGC = (vc->VC0CFGC&(~0x30))|((VCIN0_DEBOUNCE&0x03)<<4)
#define mVC_VCIN0_DriveType_Get                         ((vc->VC0CFGC&0x08)>>3)
#define mVC_VCIN0_DriveType_Set(VCIN0_DRIVETYPE)        vc->VC0CFGC = (vc->VC0CFGC&(~0x08))|((VCIN0_DRIVETYPE&0x01)<<3)
#define mVC_VCIN0_PF_Clr                                vc->VC0STS = 0x01
#define mVC_VCIN0_PF_IsSet                              (vc->VC0STS&0x01)
#define mVC_VCIN0_En                                    vc->VC0CFGC |= 0x01
// VC VCOUT1                                            
#define mVC_VCOUT1_ActPolar_Set(VCOUT1_ACTPOLAR)        vc->VC1CFGC = (vc->VC1CFGC&(~0x04))|((VCOUT1_ACTPOLAR&0x01)<<2)    
#define mVC_VCOUT1_OutputType_Set(VCOUT1_OUTPUTTYPE)    vc->VC1CFGC = (vc->VC1CFGC&(~0x02))|((VCOUT1_OUTPUTTYPE&0x01)<<1)  
// VC VCIN1                                                                                                                
#define mVC_VCIN1_CmpSrc_Get                            (vc->VC1CFGC>>7)                                                   
#define mVC_VCIN1_CmpSrc_Set(VCIN1_CMPSRC)              vc->VC1CFGC = (vc->VC1CFGC&(~0x80))|((VCIN1_CMPSRC&0x01)<<7)       
#define mVC_VCIN1_CmpSrc_DAC_Get                        (vc->VC1CFGDAC)                                                    
#define mVC_VCIN1_CmpSrc_DAC_Set(VCIN1_DAC)             mVC_VCIN1_CmpSrc_DAC_Get = VCIN1_DAC                              
#define mVC_VCIN1_CmpPolar_Get                          ((vc->VC1CFGC&0x40)>>6)                                            
#define mVC_VCIN1_CmpPolar_Set(VCIN1_POLAR)             vc->VC1CFGC =(vc->VC1CFGC&(~0x40))|((VCIN1_POLAR&0x01)<<6)         
#define mVC_VCIN1_Debounce_Get                          ((vc->VC1CFGC&0x30)>>4)                                            
#define mVC_VCIN1_Debounce_Set(VCIN1_DEBOUNCE)          vc->VC1CFGC = (vc->VC1CFGC&(~0x30))|((VCIN1_DEBOUNCE&0x03)<<4)     
#define mVC_VCIN1_DriveType_Get                         ((vc->VC1CFGC&0x08)>>3)                                            
#define mVC_VCIN1_DriveType_Set(VCIN1_DRIVETYPE)        vc->VC1CFGC = (vc->VC1CFGC&(~0x08))|((VCIN1_DRIVETYPE&0x01)<<3)    
#define mVC_VCIN1_PF_Clr                                vc->VC1STS = 0x01                                                  
#define mVC_VCIN1_PF_IsSet                              (vc->VC1STS&0x01)                                                  
#define mVC_VCIN1_En                                    vc->VC1CFGC |= 0x01                                                
#define mVC_VCIN1_Dis                                   {vc->VC1CFGC &= ~0x01; mVC_VCIN1_CmpPolar_Set(VC_VCIN1_CMPPOLAR_LOW); mVC_VCIN1_DriveType_Set(VC_VCIN1_DRIVETYPE_BY_STATUS); mVC_VCIN1_PF_Clr;}
//// VC AD0                                               
//#define mVC_AD0_CmpSrc_DAC_Get                          mVC_VCIN0_CmpSrc_DAC_Get
//#define mVC_AD0_CmpSrc_DAC_Set(VC_AD0_DAC)              mVC_VCIN0_CmpSrc_DAC_Set(VC_AD0_DAC)
//#define mVC_AD0_CmpPolar_Get                            ((vc->VC1SRCCFGAD0&0x10)>>4)
//#define mVC_AD0_CmpPolar_Set(VC_AD0_POLAR)              vc->VC1SRCCFGAD0 = (vc->VC1SRCCFGAD0&(~0x10))|((VC_AD0_POLAR&0x01)<<4)
//#define mVC_AD0_Debounce_Get                            ((vc->VC1SRCCFGAD0&0x0C)>>2)
//#define mVC_AD0_Debounce_Set(VC_AD0_DEBOUNCE)           vc->VC1SRCCFGAD0 = (vc->VC1SRCCFGAD0&(~0x0C))|((VC_AD0_DEBOUNCE&0x03)<<2)
//#define mVC_AD0_Output_En                               vc->VC1SRCCFGAD0 |= 0x01
//#define mVC_AD0_Output_Dis                              vc->VC1SRCCFGAD0 &= ~0x01
//#define mVC_AD0_PF_Clr                                  vc->VC1STS = 0x02
//#define mVC_AD0_PF_IsSet                                (vc->VC1STS&0x02)
//#define mVC_AD0_En                                      {mADC_Ch_Enable(0); vc->VC1SRCCFGAD0 |= 0x02;}
//#define mVC_AD0_Dis                                     {mADC_Ch_Disable(0); vc->VC1SRCCFGAD0 &= ~0x02;}
//// VC AD1                                               
//#define mVC_AD1_CmpPolar_Get                            ((vc->VC1SRCCFGAD1&0x10)>>4)                                               
//#define mVC_AD1_CmpPolar_Set(VC_AD1_POLAR)              vc->VC1SRCCFGAD1 = (vc->VC1SRCCFGAD1&(~0x10))|((VC_AD1_POLAR&0x01)<<4)    
//#define mVC_AD1_Debounce_Get                            ((vc->VC1SRCCFGAD1&0x0C)>>2)                                              
//#define mVC_AD1_Debounce_Set(VC_AD1_DEBOUNCE)           vc->VC1SRCCFGAD1 = (vc->VC1SRCCFGAD1&(~0x0C))|((VC_AD1_DEBOUNCE&0x03)<<2) 
//#define mVC_AD1_Output_En                               vc->VC1SRCCFGAD1 |= 0x01                                                  
//#define mVC_AD1_Output_Dis                              vc->VC1SRCCFGAD1 &= ~0x01                                                 
//#define mVC_AD1_PF_Clr                                  vc->VC1STS = 0x04                                                         
//#define mVC_AD1_PF_IsSet                                (vc->VC1STS&0x04)                                                         
//#define mVC_AD1_En                                      {mADC_Ch_Enable(1); vc->VC1SRCCFGAD1 |= 0x02;}                            
//#define mVC_AD1_Dis                                     {mADC_Ch_Disable(1); vc->VC1SRCCFGAD1 &= ~0x02;}                          
//// VC AC_IN                                             
//#define mVC_AC_IN_Debounce_Get                          ((vc->VC1SRCCFGAI&0x0C)>>2)                                              
//#define mVC_AC_IN_Debounce_Set(VC_ACIN_DEBOUNCE)         vc->VC1SRCCFGAI = (vc->VC1SRCCFGAI&(~0x0C))|((VC_ACIN_DEBOUNCE&0x03)<<2) 
//#define mVC_AC_IN_Output_En                             vc->VC1SRCCFGAI |= 0x01                                                  
//#define mVC_AC_IN_Output_Dis                            vc->VC1SRCCFGAI &= ~0x01                                                 
//#define mVC_AC_IN_PF_Cl                                 vc->VC1STS = 0x08                                                         
//#define mVC_AC_IN_PF_IsSet                              (vc->VC1STS&0x08)                                                         

//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************

//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use -----------------------------------------------

//-- For OEM Use -----------------------------------------------
void VC0_VCOUT0_Set(unsigned char Active_Polarity,unsigned char Type_sel);
void VC1_VCOUT1_Set(unsigned char Active_Polarity,unsigned char Type_sel);
void VC0_VCOUT0_Initial(void);
void VC1_VCOUT1_Initial(void);
void VC0_VCIN0_Init(void);   
void VC1_VCIN1_Init(void);   

//void VC1_AD0_Initial(void);
//void VC1_AD1_Initial(void);
//void VC1_AC_IN_Initial(void);

#endif //VC_H
