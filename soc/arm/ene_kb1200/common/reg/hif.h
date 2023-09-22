/**************************************************************************//**
 * @file     hif.h
 * @brief    Define HIF's registers
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/
 
#ifndef HIF_H
#define HIF_H

/**
  \brief  Structure type to Host Interface (HIF).
 */
typedef volatile struct
{
  uint32_t IDXCFG;                                    /*Index IO Configuration Register */
  uint32_t IDX32CFG;                                  /*Index32 IO Configuration Register */
  uint32_t MEMFCFG;                                   /*Memory to Flash Register */
  uint8_t  USRIRQ;                                    /*User Define IRQ Register */
  uint8_t  Reserved0[3];                              /*Reserved */
  uint32_t IOSCFG;                                    /*IO to SRAM Register */
  uint32_t IOSIE;                                     /*IO to SRAM Interrupt Enable Register */
  uint8_t  IOSPF;                                     /*IO to SRAM Event Pending Flag Register */
  uint8_t  Reserved1[3];                              /*Reserved */     
  uint32_t IOSRWP;                                     /*IO to SRAM Read/Write Protection Register */
  uint32_t MEMSCFG;                                   /*Memory to SRAM Register */
  uint32_t MEMSIE;                                    /*Memory to SRAM Interrupt Enable Register */
  uint8_t  MEMSPF;                                    /*Memory to SRAM Event Pending Flag Register */
  uint8_t  Reserved2[3];                              /*Reserved */        
  uint16_t MEMSAINF;                                  /*Memory to SRAM Address Information Register */
  uint16_t Reserved3;                                 /*Reserved */ 
  uint32_t MEMSBRP;                                   /*Memory to SRAM Block Read Protection Register */
  uint32_t MEMSBWP;                                   /*Memory to SRAM Block Write Protection Register */
  uint8_t  MEMSSS0;                                   /*Memory to SRAM Sector Selection0 Register */
  uint8_t  Reserved4[3];                              /*Reserved */
  uint32_t MEMSSP0;                                   /*Memory to SRAM Sector Protection0 Register */
  uint8_t  MEMSSS1;                                   /*Memory to SRAM Sector Selection1 Register */
  uint8_t  Reserved5[3];                              /*Reserved */
  uint32_t MEMSSP1;                                   /*Memory to SRAM Sector Protection1 Register */
}  HIF_T;

//#define hif                ((HIF_T *) HIF_BASE)             /* HIF Struct       */
//#define HIF                ((volatile unsigned long  *) HIF_BASE)     /* HIF  array       */

//***************************************************************
// User Define
//***************************************************************
// Index IO Setting
#define HIF_IndexIOPortEnable                   USR_HIF_IndexIOPortEnable         // 0: Disable(Default); 1: Enable 
#define HIF_IndexIOECSpaceBaseAddress           (USR_HIF_IndexIOECSpaceBaseAddress&(~0xFFFFUL))  // EC Space Base Address (bit15~0 don't care)
#define HIF_IndexIOPortBaseAddress              (USR_HIF_IndexIOPortBaseAddress&(~0x03UL))  // IO Base Address (bit1~0 don't care)
// Index32 IO Setting
#define HIF_Index32IOPort0Enable                USR_HIF_Index32IOPort0Enable      //0: Disable; 1: Enable(Default)
#define HIF_Index32IOPort0BaseAddress           (USR_HIF_Index32IOPort0BaseAddress&(~0x07UL))  //Base Address (bit2~0 don't care)
#define HIF_Index32IOPort1Enable                USR_HIF_Index32IOPort1Enable      //0 : Disable; 1: Enable(Default)
#define HIF_Index32IOPort1BaseAddress           (USR_HIF_Index32IOPort1BaseAddress&(~0x07UL))  //Base Address (bit2~0 don't care)
// MEM cycle to Embedded Flash
#define HIF_MEMtoEFEnable                       USR_HIF_MEMtoEFEnable
    //MEM setting : Decoding address = 0xYYZZxxxx
    //Byte YYZZ : Bit31~bit18 of decoding address
#define HIF_MEMtoEFBaseAddr                     (USR_HIF_MEMtoEFBaseAddr&(~0x3FFFFUL))
//---------- IO cycle to EC SRAM----------------------------------------//
#define HIF_IOtoSRAMEnable                      USR_HIF_IOtoSRAMEnable          
#define HIF_IOtoSRAMIOBaseAddr                  USR_HIF_IOtoSRAMIOBaseAddr    // IO Base Address
#define HIF_IOtoSRAMSRAMBaseAddr                (USR_HIF_IOtoSRAMSRAMBaseAddr&(~0xFFUL)) // SRAM Base Address(bit7~0 don't care)
#define HIF_IOtoSRAMSRAMOffset                  USR_HIF_IOtoSRAMSRAMOffset     // SRAM Offset to generate Write Notify event
#define HIF_IOtoSRAMBReadProtect                USR_HIF_IOtoSRAMBReadProtect   //bit0:0x00~0x0F, bit1:0x10~0x1F, , , , bit15:0xF0~0xFF    
#define HIF_IOtoSRAMBWriteProtect               USR_HIF_IOtoSRAMBWriteProtect  //bit0:0x00~0x0F, bit1:0x10~0x1F, , , , bit15:0xF0~0xFF
#define HIF_IOtoSRAMInterrupt                   USR_HIF_IOtoSRAMInterrupt
// MEM cycle to EC SRAM
#define HIF_MEMtoSRAMEnable                     USR_HIF_MEMtoSRAMEnable
    //Relative REG
    //MEM setting : Decoding address = 0xWWYYZxxx
    //HIF_MEMtoSRAMEnable = Enable bit
    //Byte   WWYYZ : Bit31~bit13 of decoding address
    //
    //Block of write protection enable :
    //{HIF_MEMtoSRAMBlockWriteProtect: Each bit with enable 0x100 block SRAM windows
    //
    //Bytes xxxxxxxx :  [0]: 0x20000000-0x200000FF     
    //                  [1]: 0x20000100-0x200001FF     
    //                  [2]: 0x20000200-0x200002FF     
    //                  [3]: 0x20000300-0x200003FF     
    //                  .   
    //                  .   
    //                  .     
    //                  [31]:0x20001F00-0x20001FFF     
    //
    //Block of read protection enable :
    //{HIF_MEMtoSRAMBlockReadProtect: Each bit with enable 0x100 block SRAM windows
    //
    //Byte xxxxxxxx :   [0]:0x20000000-0x200000FF     
    //                  [1]:0x20000100-0x200001FF     
    //                  [2]:0x20000200-0x200002FF     
    //                  [3]:0x20000300-0x200003FF     
    //                  .   
    //                  .   
    //                  .     
    //                  [31]:0x20001F00-0x20001FFF       
    //
    //Sector protection's block selection :
    //Nibble X  :HIF_MEMtoSRAMNumBlockToSector: Select the number of block to sector
    //
    //Sector of write protection enable :
    //{HIF_MEMtoSRAMSectorWriteProtect: Each bit with enable 0x10 sector SRAM windows
    //
    //Byte xx :    [0]:0x00-0x0F       [ 8]:0x80-0x8F
    //             [1]:0x10-0x1F       [ 9]:0x90-0x9F
    //             [2]:0x20-0x2F       [10]:0xA0-0xAF
    //             [3]:0x30-0x3F       [11]:0xB0-0xBF
    //             [4]:0x40-0x4F       [12]:0xC0-0xCF
    //             [5]:0x50-0x5F       [13]:0xD0-0xDF
    //             [6]:0x60-0x6F       [14]:0xE0-0xEF
    //             [7]:0x70-0x7F       [15]:0xF0-0xFF
    //
    //Sector of read protection enable :
    //{HIF_MEMtoSRAMSectorReadProtect: Each bit with enable 0x10 sector SRAM windows
    //
    //Byte xx :    [0]:0x000-0x00F     [ 8]:0x080-0x08F
    //             [1]:0x010-0x01F     [ 9]:0x090-0x09F
    //             [2]:0x020-0x02F     [10]:0x0A0-0x0AF
    //             [3]:0x030-0x03F     [11]:0x0B0-0x0BF
    //             [4]:0x040-0x04F     [12]:0x0C0-0x0CF
    //             [5]:0x050-0x05F     [13]:0x0D0-0x0DF
    //             [6]:0x060-0x06F     [14]:0x0E0-0x0EF
    //             [7]:0x070-0x07F     [15]:0x0F0-0x0FF
    //

#define HIF_MEMtoSRAMMEMBaseAddr                (USR_HIF_MEMtoSRAMMEMBaseAddr&(~0x1FFFUL))
#define HIF_MEMtoSRAMSRAMBaseAddr               (USR_HIF_MEMtoSRAMSRAMBaseAddr&(~0x1FFFUL))
#define HIF_MEMtoSRAMBlockWriteProtect          USR_HIF_MEMtoSRAMBlockWriteProtect
#define HIF_MEMtoSRAMBlockReadProtect           USR_HIF_MEMtoSRAMBlockReadProtect
#define HIF_MEMtoSRAMNumBlockToSector0          USR_HIF_MEMtoSRAMNumBlockToSector0    //0 ~ 15
#define HIF_MEMtoSRAMSectorWriteProtect0        USR_HIF_MEMtoSRAMSectorWriteProtect0
#define HIF_MEMtoSRAMSectorReadProtect0         USR_HIF_MEMtoSRAMSectorReadProtect0
#define HIF_MEMtoSRAMNumBlockToSector1          USR_HIF_MEMtoSRAMNumBlockToSector1    //0 ~ 15
#define HIF_MEMtoSRAMSectorWriteProtect1        USR_HIF_MEMtoSRAMSectorWriteProtect1
#define HIF_MEMtoSRAMSectorReadProtect1         USR_HIF_MEMtoSRAMSectorReadProtect1
#define HIF_MEMtoSRAMInterrupt                  USR_HIF_MEMtoSRAMInterrupt

//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------
#define HIF_IDXCFG_INIT_TABLE_OFFSET            0
#define HIF_IDX32CFG_INIT_TABLE_OFFSET          1
#define HIF_MEMFCFG_INIT_TABLE_OFFSET           2
#define HIF_IOSCFG_INIT_TABLE_OFFSET            4
#define HIF_IOSIE_INIT_TABLE_OFFSET             5
#define HIF_IOSRWP_INIT_TABLE_OFFSET            7
#define HIF_MEMSCFG_INIT_TABLE_OFFSET           8
#define HIF_MEMSIE_INIT_TABLE_OFFSET            9
#define HIF_MEMSBRP_INIT_TABLE_OFFSET           12
#define HIF_MEMSBWP_INIT_TABLE_OFFSET           13
#define HIF_MEMSSS0_INIT_TABLE_OFFSET           14
#define HIF_MEMSSP0_INIT_TABLE_OFFSET           15
#define HIF_MEMSSS1_INIT_TABLE_OFFSET           16
#define HIF_MEMSSP1_INIT_TABLE_OFFSET           17

#define HIF_MEMFSIZE                            0x10        //256KB

//-- Macro Define -----------------------------------------------
#define mHIF_USRIRQ_ENABLE              hif->USRIRQ |= 0x01;
#define mHIF_USRIRQ_DISABLE             hif->USRIRQ &= ~0x01;
#define mHIF_USRIRQ_HIGH                hif->USRIRQ |= 0x02;
#define mHIF_USRIRQ_LOW                 hif->USRIRQ &= ~0x02;
#define mHIF_USRIRQ_CH_NUM(ch_num)      hif->USRIRQ |= (hif->USRIRQ&0x0F)|(ch_num<<4) 
#define mHIF_CLEAR_MEMS_PF              hif->MEMSPF = 0x01
#define mHIF_GET_MEMS_PF                hif->MEMSPF&0x01
#define mHIF_CLEAR_IOS_PF               hif->IOSPF = 0x01
#define mHIF_GET_IOS_PF                 hif->IOSPF&0x01
//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************
extern const unsigned long HIF_Init_Table[];

//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use -----------------------------------------------
void HIF_Reg_Init(void);
void HIF_Reset(void);
void HIF_MEMS_IOS_ISR(void);
//-- For OEM Use -----------------------------------------------

#endif //HIF_H
