/**************************************************************************//**
 * @file     spis.h
 * @brief    Define SPIS's register and function
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef SPIS_H
#define SPIS_H

/**
  \brief  Structure type to access SPI Slave (SPIS).
 */
typedef struct
{
  uint16_t  SPISCFG;                              /*Configuration Register */
  uint16_t  Reserved0;                            /*Reserved */    
  uint16_t  SPISIE;                               /*Interrupt Register */
  uint16_t  Reserved1;                            /*Reserved */
  uint16_t  SPISNPF;                              /*Event Pending Flag Register */
  uint16_t  Reserved2;                            /*Reserved */
  uint32_t  Reserved;
  uint8_t   SPISNSTA;                             /*Status Register */
  uint8_t   Reserved3;                            /*Reserved */
  uint8_t   SPISNSTA_TPTR;                        /*Tx Buffer Pointer Register */
  uint8_t   SPISNSTA_RPTR;                        /*Rx Buffer Pointer Register */
  uint8_t   SPISNCFG;                             /*Nuvoton Mode Configuration Register */       
  uint8_t   SPISNCFG_TLE;                         /*Tx Level Empty  Configuration Register*/                          
  uint8_t   SPISNCFG_RLF;                         /*Rx Level Full   Configuration Register */                   
  uint8_t   SPISNCFG_RLF2;                        /*Rx Level Full 2 Configuration Register*/                          
  uint32_t  Reserved4[58];                        /*Reserved */ 
  uint8_t   SPISTBUF[128];                        /*Tx Buffer Register */ 
  uint8_t   Reserved5[128];
  uint8_t   SPISRBUF[128];                        /*Rx Buffer Register */
  uint8_t   Reserved6[128];
} SPIS_T;

#define spis                ((SPIS_T *) SPIS_BASE)             /* SPIS Struct       */

#define SPIS                ((volatile unsigned long  *) SPIS_BASE)     /* SPIH  array       */

//***************************************************************
// User Define                                                
//***************************************************************
#define SUPPORT_SPIS                    ENABLE//USR_SUPPORT_SPIS

//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------
#define SPIS_Mode0                      0x00
#define SPIS_Mode1                      0x01

#define SPIS_All_Event                  0x03FF
#define SPIS_CS_Falling                 0x0200
#define SPIS_CS_Rising                  0x0100
#define SPIS_Rx_LevelFull2              0x0080
#define SPIS_StatusRead                 0x0040
#define SPIS_EndofData_Write            0x0020
#define SPIS_EndofData_Read             0x0010
#define SPIS_Rx_LevelFull               0x0008
#define SPIS_Rx_Full                    0x0004
#define SPIS_Tx_LevelEmpty              0x0002
#define SPIS_Tx_Empty                   0x0001

#define SPIS_IndTBE                     0x40 
#define SPIS_IndRBF                     0x80


//-- Macro Define -----------------------------------------------
#define mSPIS_BUSY                      (spis->SPISCFG&0x0100)            
#define mSPIS_ModuleEnable              spis->SPISCFG |= 0x01
#define mSPIS_ModuleDisable             spis->SPISCFG &= ~0x01
#define mSPIS_Mode_Selection(mode)      spis->SPISCFG = ((spis->SPISCFG&0x7F)|((mode&0x01)<<8))
                                          
#define mSPIS_Interrupt_Enable(event)   spis->SPISIE |= event
#define mSPIS_Interrupt_Disable(event)  spis->SPISIE &=(~event)

#define mSPIS_PF_Get(event)             (spis->SPISNPF&event)
#define mSPIS_PF_Clr(event)             spis->SPISNPF = event

#define mSPIS_TBUFPTR                   (spis->SPISNSTA_TPTR)
#define mSPIS_RBUFPTR                   (spis->SPISNSTA_RPTR)
#define mSPIS_IndTBE_Set                spis->SPISNSTA |= 0x40UL
#define mSPIS_IndTBE_Clr                spis->SPISNSTA &=~0x40UL
#define mSPIS_IndRBF_Set                spis->SPISNSTA |= 0x80UL
#define mSPIS_IndRBF_Clr                spis->SPISNSTA &=~0x80UL

#define mSPIS_SimultaneousRW_Enable     spis->SPISNCFG |= 0x01UL
#define mSPIS_SimultaneousRW_Disable    spis->SPISNCFG &=~0x01UL
#define mSPIS_WriteOneShot_Enable       spis->SPISNCFG |= 0x02UL
#define mSPIS_WriteOneShot_Disable      spis->SPISNCFG &=~0x02UL
#define mSPIS_HostWrite_Enable          spis->SPISNCFG |= 0x04UL
#define mSPIS_HostWrite_Disable         spis->SPISNCFG &=~0x04UL
#define mSPIS_IndRBF_Auto_Enable        spis->SPISNCFG |= 0x08UL
#define mSPIS_IndRBF_Auto_Disable       spis->SPISNCFG &=~0x08UL
#define mSPIS_IndTBE_Auto_Enable        spis->SPISNCFG |= 0x10UL
#define mSPIS_IndTBE_Auto_Disable       spis->SPISNCFG &=~0x10UL
#define mSPIS_DAS_Enable                spis->SPISNCFG |= 0x20UL
#define mSPIS_DAS_Disable               spis->SPISNCFG &=~0x20UL
#define mSPIS_RxWrapAround_Enable       spis->SPISNCFG |= 0x40UL
#define mSPIS_RxWrapAround_Disable      spis->SPISNCFG &=~0x40UL

#define mSPIS_TxLevelEmpty_Enable(level) spis->SPISNCFG_TLE = (0x80|(level&0x7F))
#define mSPIS_TxLevelEmpty_Disable       spis->SPISNCFG_TLE = 0x00

#define mSPIS_RxLevelFull_Enable(level)  spis->SPISNCFG_RLF = (0x80|(level&0x7F))
#define mSPIS_RxLevelFull_Disable        spis->SPISNCFG_RLF = 0x00

#define mSPIS_RxLevelFull2_Enable(level) spis->SPISNCFG_RLF2 = (0x80|(level&0x7F))
#define mSPIS_RxLevelFull2_Disable       spis->SPISNCFG_RLF2 = 0x00

#define mSPIS_Write_TBuf(num,dat)       spis->SPISTBUF[num] = dat
#define mGet_SPIS_Read_RBuf(num)        spis->SPISRBUF[num]


//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************

//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use -----------------------------------------------

//-- For OEM Use -----------------------------------------------
#if SUPPORT_SPIS
void SPIS_Init(unsigned char bMode);
void SPIS_Tx_Finish(unsigned char *pTxDat, unsigned char bTxLen);
void SPIS_Rx_Finish(unsigned char *pRxDat, unsigned char bRxLen);
void SPIS_ISR(void);

#endif  //SUPPORT_SPIS
#endif //SPIS_H
