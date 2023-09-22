/**************************************************************************//**
 * @file     xbi.h
 * @brief    Define XBI's register and function
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef XBI_H
#define XBI_H

/**
  \brief  Structure type to access X-Bus Interface (XBI).
 */
typedef volatile struct
{
//  uint32_t EFCFG;                                     /*Configuration Register */
//  uint8_t  EFSTA;                                     /*Status Register */
//  uint8_t  Reserved0[3];                              /*Reserved */
//  uint32_t EFADDR;                                    /*Address Register */
//  uint16_t EFCMD;                                     /*Command Register */
//  uint16_t Reserved1;                                 /*Reserved */  
//  uint32_t EFDAT;                                     /*Data Register */
//  uint8_t  EFPORF;                                    /*POR Flag Register */
//  uint8_t  Reserved2[3];                              /*Reserved */  
//  uint8_t  EFBCNT;                                    /*Burst Conuter Register */  
  uint32_t SRAMMGT;                              /*Code SRAM Management Register */
}  XBI_T;

#define xbi                ((XBI_T *) XBI_BASE)             /* XBI Struct       */

#define XBI                ((volatile unsigned long  *) XBI_BASE)     /* XBI  array       */

//***************************************************************
// User Define                                                
//***************************************************************

//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------
////Normal code and Fixed code
//#define NORMAL_CODE_BASE_ADDRESS        0x00001000UL
//#define NORMAL_CODE_HEADER_ADDRESS      0x00004000UL
//#define NORMAL_CODE_START_OFFSET        (NORMAL_CODE_HEADER_ADDRESS-NORMAL_CODE_BASE_ADDRESS)
//#define FIXED_CODE_INFO_ADDRESS         (NORMAL_CODE_BASE_ADDRESS-EF_SECTOR_SIZE)
//#define OK_PATTERN_B                    0x00
//#define OK_PATTERN                      0x0000
//#define UPDATE_ERASE_PATTERN_B          0xAA
//#define UPDATE_ERASE_PATTERN            0xAAAA
//#define UPDATE_NOERASE_PATTERN_B        0xFF
//#define UPDATE_NOERASE_PATTERN          0xFFFF
////Sector 
//#define EF_SECTOR_SIZE                  0x200
//#define EF_SECTOR_SIZE_LONG             (EF_SECTOR_SIZE/4)
//#define EF_SECTOR_MASK                  0x1FF
////Burst
//#define EF_BurstProgram_SIZE            0x80
//#define EF_BurstProgram_MASK            0x7F
//#define EF_BurstProgram_DAddress        0x50100010UL
////EFCMD support command                 
//#define CMD_Sector_Erase_Single       0x0010 
//#define CMD_Sector_Erase_Increase       0x0110 
//#define CMD_Program_Single                0x0021
//#define CMD_Program_Increase            0x0121
//#define CMD_BurstProgram_Single       0x0027
//#define CMD_BurstProgram_Increase       0x0127 
//#define CMD_Read32_Single               0x0034
//#define CMD_Read32_Increase             0x0134
//#define CMD_NVR_Read32_Single           0x0074
//#define CMD_NVR_Read32_Increase         0x0174
//#define CMD_Set_Configuration_Single    0x0080
//#define CMD_Set_Configuration_Increase  0x0180   
////Trim 
//#define EF_HHGRACE_Trim_NVR2            DISABLE
//#define EF_ENE_Trim_NVR0                ENABLE                                      
//#define EF_POR_Flag                     0x5A
//#define EF_Done_Flag                    0xA5
////Trim - NVR2
//#define EF_HHGRACE_Trim_NVR2_Address    0x00000400
//#define EF_HHGRACE_Trim_Valid_Flag      0x5A
//#define EF_HHGRACE_Trim_Word_Size       29
//#define EF_HHGRACE_Trim_RDN_Start       13
//#define EF_HHGRACE_Trim_RDN_End         20
//#define EF_HHGRACE_Trim_RDN_Number      4
//#define EF_ENE_Trim_BG_Address          0x00000500
////Trim - NVR0
//#define EF_ENE_Trim_NVR0_Address        0x00000000
//#define EF_ENE_Trim_NVR0_OSC32K_Address 0x0000000C
//#define EF_ENE_Trim_Valid_Flag          0x5A
//#define EF_LAB_Trim_Valid_Flag          0xAB
#define HIFRP                   0x80000000
#define I2CD32RP                0x40000000
#define EDI32RP                 0x20000000
#define DMARP                   0x10000000
#define MCURP                   0x08000000

//-- Macro Define -----------------------------------------------
#define mSRAM_ReadProtect_Enable(ProtectType)   xbi->SRAMMGT |=  (ProtectType)
#define mSRAM_ReadProtect_Disable               xbi->SRAMMGT &= ~0xF8000000UL
#define mSRAM_ReadOnly_Enable               xbi->SRAMMGT |=  0x01000000UL
#define mSRAM_ReadOnly_Disable              xbi->SRAMMGT &= ~0x01000000UL
#define mSRAM_Protect_lock_Enable           xbi->SRAMMGT |=  0x02000000UL
#define mSRAM_Protect_Range_set(Range)      xbi->SRAMMGT |=  (xbi->SRAMMGT&0xFF000000)|Range
//#define mEnable_EFCMD             xbi->EFCFG |= 0x00000001UL
//#define mDisable_EFCMD                xbi->EFCFG &= ~0x00000001UL
////#define mEF_Set_ReadCycle(cyc)      xbi->EFCFG = (xbi->EFCFG&(~0x000000F0))|(cyc<<4) 
//
//#define mEF_Check_Busy              (xbi->EFSTA&0x01)
//#define mEF_Set_Address(addr)       xbi->EFADDR = addr
//#define mGet_EF_Data                (xbi->EFDAT)
//#define mEF_Write_Data(dat)         xbi->EFDAT = dat
//#define mEF_Set_BurstP_CNT(cnt)     xbi->EFBCNT = cnt
//
//#define mEF_POR_Flag_Valid          (xbi->EFPORF==EF_POR_Flag)
//#define mEF_Change_POR_Flag(flag)   xbi->EFPORF = flag

//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************

//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use -----------------------------------------------
//void Trim_Data(void);
//void          Fire_Command(unsigned short cmd);

//-- For OEM Use -----------------------------------------------
//unsigned char EF_Read_Byte(unsigned long EF_Addr);
//unsigned long EF_Read_4Byte(unsigned long EF_Addr);
//void EF_Read_Block(unsigned long *RAM, unsigned long EF_ADDR, unsigned char length);
//void EF_Erase(unsigned long EF_Addr);
//void EF_Write_Byte(unsigned long,unsigned char);
//unsigned char EF_Write_Burst(unsigned long , unsigned char , unsigned char *);
//unsigned char EF_Write_Block(unsigned long, unsigned short, unsigned char*);
//void          Active_Fix_Code_Reload(unsigned char bLED_On);
#endif //XBI_H
