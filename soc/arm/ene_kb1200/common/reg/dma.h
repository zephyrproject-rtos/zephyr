/**************************************************************************//**
 * @file     dma.h
 * @brief    Define DMA's register and function
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef DMA_H
#define DMA_H

/**
  \brief  Structure type to access Direct Memory Access Controller (DMA).
 */
typedef volatile struct
{
  uint8_t  DMACFG;                                    /*Configuration Register */
  uint8_t  Reserved0[3];                              /*Reserved */    
  uint8_t  DMACTRL;                                   /*Control Register */
  uint8_t  Reserved1[3];                              /*Reserved */
  uint32_t DMASTA;                                    /*Status Register */
  uint32_t Reserved3;                                 /*Reserved */
  uint16_t DMAC0CFG;                                  /*Channel0 Configuration Register */
  uint16_t Reserved4;                                 /*Reserved */
  uint16_t DMAC0LEN;                                  /*Channel0 Length Register */
  uint16_t Reserved5;                                 /*Reserved */        
  uint32_t DMAC0SA;                                   /*Channel0 Source Address Register */
  uint32_t DMAC0DA;                                   /*Channel0 Destination Address */
  uint16_t DMAC1CFG;                                  /*Channel1 Configuration Register */
  uint16_t Reserved6;                                 /*Reserved */
  uint16_t DMAC1LEN;                                  /*Channel1 Length Register */
  uint16_t Reserved7;                                 /*Reserved */        
  uint32_t DMAC1SA;                                   /*Channel1 Source Address Register */
  uint32_t DMAC1DA;                                   /*Channel1 Destination Address */  
}  DMA_T;

#define dma                ((DMA_T *) DMA_BASE)             /* DMA Struct       */

#define DMA                ((volatile unsigned long  *) DMA_BASE)     /* DMA  array       */

//***************************************************************
// User Define                                                
//***************************************************************
#define SUPPORT_DMA                         USR_SUPPORT_DMA
// DMA Ch0 for Kernel transaction Use 
    #define DMA_CH0_SUPPORT                 ENABLE
    #define DMA_CH1_SUPPORT                 USR_DMA_CH1_SUPPORT
        
//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------
#define DMA_CH0                             0
#define DMA_CH1                             1

#define DMA_CH_CFG_OFFSET                   4
#define DMA_CH_LEN_OFFSET                   5
#define DMA_CH_SA_OFFSET                    6
#define DMA_CH_DA_OFFSET                    7

#define DMA_DESTINATION_NOFIX_U_MODE1       0x00
#define DMA_DESTINATION_NOFIX_MODE2         0x40
#define DMA_DESTINATION_FIX_MODE3           0x80

#define DMA_SOURCE_NOFIX_U_MODE1            0x00
#define DMA_SOURCE_NOFIX_MODE2              0x10
#define DMA_SOURCE_FIX_MODE3                0x20

#define DMA_CHCFG_EN                        0x01
#define DMA_CHINT_EN                        0x02
#define DMA_CH4BYTE_EN                      0x04
#define DMA_CFG_EN                          0x01

#define DMA_FLAG_BUSY                       0x80
#define DMA_FLAG_OVERFLOW                   0x01
#define DMA_FLAG_VIOLATE                    0x10
#define DMA_ISSUE                           0x80

#define DMA_FLAG_CH_BUSY                    0x000100
#define DMA_FLAG_CH_OVERFLOW                0x010000
#define DMA_FLAG_CH_VIOLATE                 0x000100
#define DMA_FLAG_CH_FINISH                  0x000001

//-- Macro Define -----------------------------------------------
//Channel Configure
//#define mDMA_ChConfig(CH, DESTINATION, SOURCE)      DMA[DMA_CH_CFG_OFFSET+((CH&0x01)<<2)] = (DESTINATION|SOURCE|DMA_CHCFG_EN)
#define mDMA_Enable                                 dma->DMACFG = DMA_CFG_EN
#define mDMA_Disable                                dma->DMACFG = 0
#define mDMA_ChConfig(CH, DESTINATION, SOURCE)      DMA[DMA_CH_CFG_OFFSET+(CH<<2)] = (DMA[DMA_CH_CFG_OFFSET+(CH<<2)]&0x06)|(DESTINATION|SOURCE|DMA_CHCFG_EN)
#define mDMA_ChIntEn(CH)                            DMA[DMA_CH_CFG_OFFSET+(CH<<2)]|= DMA_CHINT_EN
#define mDMA_ChIntDis(CH)                           DMA[DMA_CH_CFG_OFFSET+(CH<<2)]&=~DMA_CHINT_EN
#define mDMA_Ch4ByteEn(CH)                          DMA[DMA_CH_CFG_OFFSET+(CH<<2)]|= DMA_CH4BYTE_EN
#define mDMA_Ch4ByteDis(CH)                         DMA[DMA_CH_CFG_OFFSET+(CH<<2)]&=~DMA_CH4BYTE_EN
#define mDMA_ChDis(CH)                              DMA[DMA_CH_CFG_OFFSET+(CH<<2)] = 0


//Flag
#define mDMA_IsBusy                                 (dma->DMACFG&DMA_FLAG_BUSY)                                          
//#define mDMA_IsOVERFLOW(CH, DMA_FLAG)               (dma->DMASTA&(DMA_FLAG_OVERFLOW<<(CH)))                              
//#define mDMA_IsVIOLATE(CH, DMA_FLAG)                (dma->DMASTA&(DMA_FLAG_VIOLATE<<(CH)))                               

#define mDMA_IsOVERFLOW(CH)                         (dma->DMASTA&(DMA_FLAG_OVERFLOW<<(CH)))                              
#define mDMA_IsVIOLATE(CH)                          (dma->DMASTA&(DMA_FLAG_VIOLATE<<(CH)))
#define mDMA_IsFINISH(CH)                           (dma->DMASTA&(DMA_FLAG_CH_FINISH<<(CH)))
#define mDMA_IsChBusy(CH)                           (DMA[DMA_CH_CFG_OFFSET+(CH<<2)]&DMA_FLAG_CH_BUSY)

#define mDMA_ChFlag_Clr(CH)                         (dma->DMASTA=(0x00010101<<CH))
#define mDMA_ChFinish_Get(CH)                       (dma->DMASTA&(DMA_FLAG_CH_FINISH<<CH))
#define mDMA_ChFinish_Clr(CH)                       (dma->DMASTA=(DMA_FLAG_CH_FINISH<<CH))

                                                                                                                         
//Operator                                                                                                               
//#define mDMA_SrcAddr_Set(CH, SADDR)                 DMA[DMA_CH_SA_OFFSET+((CH&0x01)<<2)] = SADDR
//#define mDMA_DstAddr_Set(CH, DADDR)                 DMA[DMA_CH_DA_OFFSET+((CH&0x01)<<2)] = DADDR
//
//#define mDMA_Length_Set(CH, LEN)                    DMA[DMA_CH_LEN_OFFSET+((CH&0x01)<<2)] = LEN                                          
//#define mDMA_Transaction_Begin(CH)                  dma->DMACTRL = DMA_ISSUE|(CH&0x01)                 
//                            
//#define mDMA_SrcAddr_Get(CH)                        DMA[DMA_CH_SA_OFFSET+((CH&0x01)<<2)]
//#define mDMA_DstAddr_Get(CH)                        DMA[DMA_CH_DA_OFFSET+((CH&0x01)<<2)]

#define mDMA_SrcAddr_Set(CH, SADDR)                 DMA[DMA_CH_SA_OFFSET+((CH)<<2)] = SADDR
#define mDMA_DstAddr_Set(CH, DADDR)                 DMA[DMA_CH_DA_OFFSET+((CH)<<2)] = DADDR
                            
#define mDMA_Length_Set(CH, LEN)                    DMA[DMA_CH_LEN_OFFSET+((CH)<<2)] = LEN
#define mDMA_Transaction_Begin(CH)                  dma->DMACTRL = DMA_ISSUE|(CH)
                            
#define mDMA_SrcAddr_Get(CH)                        DMA[DMA_CH_SA_OFFSET+((CH)<<2)]
#define mDMA_DstAddr_Get(CH)                        DMA[DMA_CH_DA_OFFSET+((CH)<<2)]

//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************

//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use -----------------------------------------------

//-- For OEM Use -----------------------------------------------
void MEM_Copy(volatile unsigned char *TARGET, volatile unsigned char *SOURCE,unsigned short LEN);
void MEM_memset(volatile unsigned char *TARGET, unsigned char bDat, unsigned short Len);
#if SUPPORT_DMA
void DMA_Init(void);
#endif  //SUPPORT_DMA
#if DMA_CH1_SUPPORT
void DMA1_Mode_Setting(unsigned char DESTINATION_Mode, unsigned char SOURCE_Mode);
void DMA1_Transaction(volatile unsigned char *TARGET, volatile unsigned char *SOURCE, unsigned short LEN);
#endif //DMA_CH1_SUPPORT
#endif //DMA_H
