/**************************************************************************//**
 * @file     mbx.h
 * @brief    Define MBX's register and function
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef MBX_H
#define MBX_H

/**
  \brief  Structure type to access MialBox1/2 (MBX).
 */
typedef volatile struct
{
  uint32_t MBXCFG;                                    /*Configuration Register */
  uint8_t  MBXIE;                                     /*Interrupt Enable Register */
  uint8_t  Reserved0[3];                              /*Reserved */
  uint8_t  MBXPF;                                     /*Event Pending Flag Register */
  uint8_t  Reserved1[3];                              /*Reserved */
  uint32_t MBXRBA;                                    /*SRAM Base Address Register */   
  uint32_t MBXAINFO;                                  /*Access Information Register */
  uint32_t MBXPC;                                     /*Protection Control Register */      
}  MBX_T;

//#define mbx1                ((MBX_T *) MBX1_BASE)             /* MBX Struct       */
//#define mbx2                ((MBX_T *) MBX2_BASE)             /* MBX Struct       */
//#define MBX                ((volatile unsigned long  *) MBX_BASE)     /* MBX  array       */

//***************************************************************
// User Define                                                
//***************************************************************
// MailBox1
#define HIF_MailBox1Enable              USR_HIF_MailBox1Enable
    // WP and RP: 
    //      [0]:0xXXXXXX00-0xXXXXXX0F        [ 8]:0xXXXXXX80-0xXXXXXX8F
    //      [1]:0xXXXXXX10-0xXXXXXX1F        [ 9]:0xXXXXXX90-0xXXXXXX9F
    //      [2]:0xXXXXXX20-0xXXXXXX2F        [10]:0xXXXXXXA0-0xXXXXXXAF
    //      [3]:0xXXXXXX30-0xXXXXXX3F        [11]:0xXXXXXXB0-0xXXXXXXBF
    //      [4]:0xXXXXXX40-0xXXXXXX4F        [12]:0xXXXXXXC0-0xXXXXXXCF
    //      [5]:0xXXXXXX50-0xXXXXXX5F        [13]:0xXXXXXXD0-0xXXXXXXDF
    //      [6]:0xXXXXXX60-0xXXXXXX6F        [14]:0xXXXXXXE0-0xXXXXXXEF
    //      [7]:0xXXXXXX70-0xXXXXXX7F        [15]:0xXXXXXXF0-0xXXXXXXFF
    //
    //ex: MailBox1 access to SRAM range: 0x20004000~0x200040FF, Write protect Sector1,Sector2, Read protect Sector2,Sector14
    //    HIF_MailBox1ECHaddress <-- 0x20004000
    //    HIF_MailBox1WP         <-- 0x0006
    //    HIF_MailBox1RP         <-- 0x4004
    //
    //0x20004000~0x200040FF |OROOOOOOOOOOO#WO|
    //
    //Note: O: no R & no W protect, R: R protect, W: W protect, #: R & W protect
#define HIF_MailBox1IOaddress           USR_HIF_MailBox1IOaddress       //(MailBox1 IO base address) = index port, (MailBox1 IO base address+1) = data port
#define HIF_MailBox1ECHaddress          USR_HIF_MailBox1ECHaddress      //MailBox1 can access EC SRAM 
#define HIF_MailBox1Interrupt           USR_HIF_MailBox1Interrupt 
#define HIF_MailBox1WP                  USR_HIF_MailBox1WP              //Sector15~0 write protection
#define HIF_MailBox1RP                  USR_HIF_MailBox1RP              //Sector15~0 read protection
// MailBox2
#define HIF_MailBox2Enable              USR_HIF_MailBox2Enable
#define HIF_MailBox2IOaddress           USR_HIF_MailBox2IOaddress       //(MailBox2 IO base address) = index port, (MailBox2 IO base address+1) = data port
#define HIF_MailBox2ECHaddress          USR_HIF_MailBox2ECHaddress      //MailBox2 can access EC SRAM 
#define HIF_MailBox2Interrupt           USR_HIF_MailBox2Interrupt 

//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------

//-- Macro Define -----------------------------------------------
#define mGet_MBX_Event                  (mbx->MBXPF&0x01)
#define mClr_MBX_Event                  mbx->MBXPF = 0x01
#define mGet_MBX_Access_Inf             mbx->MBXAINFO

//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************

//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use -----------------------------------------------
void MBX_Init(void);
void MBX_Handle_ISR(MBX_T* mbx);

//-- For OEM Use -----------------------------------------------

#endif //MBX_H
