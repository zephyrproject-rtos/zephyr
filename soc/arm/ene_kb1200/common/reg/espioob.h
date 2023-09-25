/**************************************************************************//**
 * @file     espioob.h
 * @brief    Define ESPIOOB's register and function
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef ESPIOOB_H
#define ESPIOOB_H

/**
  \brief  Structure type to access ESPI Out-Of-Band (ESPIOOB).
 */
typedef volatile struct
{
  uint8_t  ESPIOOBIE;                                 /*Interrupt Enable Register */
  uint8_t  Reserved0[3];                              /*Reserved */
  uint8_t  ESPIOOBPF;                                 /*Event Pending Flag Register */
  uint8_t  Reserved1[3];                              /*Reserved */
  uint8_t  ESPIOOBEF;                                 /*Error Flag Register */
  uint8_t  Reserved2[3];                              /*Reserved */        
  uint32_t Reserved3;                                 /*Reserved */ 
  uint16_t ESPIOOBRL;                                 /*Rx Length Register */
  uint16_t Reserved4;                                 /*Reserved */
  uint16_t ESPIOOBTX;                                 /*Tx Control Register */
  uint16_t Reserved5;                                 /*Reserved */
  uint8_t  ESPIOOBPECC;                               /*PEC Register */
  uint8_t  Reserved6[3];                              /*Reserved */
  uint32_t Reserved7;                                 /*Reserved */
  uint8_t  ESPIOOBDAT[73];                            /*Data Register */      
}  ESPIOOB_T;

//#define espioob                ((ESPIOOB_T *) ESPIOOB_BASE)             /* ESPIOOB Struct       */
//#define ESPIOOB                ((volatile unsigned long  *) ESPIOOB_BASE)     /* ESPIOOB  array       */

//***************************************************************
// User Define                                                
//***************************************************************
//#define ESPIOOB_V_OFFSET            USR_ESPIOOB_V_OFFSET
#define SUPPORT_ESPIOOB             0
//#define SUPPORT_ESPIOOB_NOTIFY  USR_SUPPORT_ESPIOOB_NOTIFY

//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------
#define ESPIOOB_ADR_BYTE                0x00
#define ESPIOOB_CMD_BYTE                0x01
#define ESPIOOB_CNT_BYTE                0x02
#define ESPIOOB_DAT_BYTE                0x03
#define ESPIOOB_NADR_BYTE               0x00
#define ESPIOOB_NCMD_BYTE               0x01
#define ESPIOOB_NCNT_BYTE               0x02
#define ESPIOOB_NDAT_BYTE               0x03

#define ESPIOOB_RX                      0x01
#define ESPIOOB_PEC                     0x02
#define ESPIOOB_BUSY                    0x10
#define ESPIOOB_TIMEOUT                 0x20
#define ESPIOOB_TX_OCCURED              0x40

#define ESPIOOB_msTimeOut               1000    // Unit: ms, Range: 300~76500 , Base Clock: 300ms(GPT2), Max. deviation: 150ms
#define ESPIOOB_TIMEOUTCNT              (mGPT_mSecToGPT2Cnt(ESPIOOB_msTimeOut))

#define ESPIOOB_BUFSIZE                 73
#define ESPIOOB_TX_MAX                  69

typedef struct _ESPIOOB     
{
    unsigned char Addr;
    unsigned char Cmd;
    unsigned char TxLen;
    unsigned char RxLen;
    unsigned char*TxDat;    
    unsigned char*RxDat;
    unsigned char TimeOutCnt;
    unsigned char Flag;                                     
    //Bit 7  : Reserved ; Bit 6 : Tx Event Occured ; Bit 5 : TIMEOUT Bit ; Bit 4 : BUSY Bit
    //Bit 2~3: Reserved ; Bit 1 : PEC Byte(0:no PEC,1:has PEC  //Bit 0 : RX  Byte(0:no RX ,1:has Rx )
}_hwESPIOOB;

//-- Macro Define -----------------------------------------------
#define mESPIOOB_TxWithPEC(len)                 espioob->ESPIOOBTX = 0x8100|(unsigned short)len
#define mESPIOOB_TxWithoutPEC(len)              espioob->ESPIOOBTX = 0x0100|(unsigned short)len
#define mESPIOOB_IssueIntSource                 __NVIC_SetPendingIRQ(ESPICH2_IRQn)

//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************
#if SUPPORT_ESPIOOB
extern _hwESPIOOB hwESPIOOB;
#endif  //SUPPORT_ESPIOOB

//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use -----------------------------------------------
#if SUPPORT_ESPIOOB
void ESPIOOB_Timer(void);
void Handle_ESPIOOB_ISR(void);
#endif  //SUPPORT_ESPIOOB

//-- For OEM Use -----------------------------------------------
#if SUPPORT_ESPIOOB
unsigned char ESPIOOB_Transcation(unsigned char bAddr, unsigned char bCmd, unsigned char bTxLen, unsigned char*pTxDat, unsigned char bFlag, unsigned char bRxLen, unsigned char*pRxDat);
unsigned char ESPIOOB_BusyCheck(void);
#endif  //SUPPORT_ESPIOOB

#endif //ESPIOOB_H
