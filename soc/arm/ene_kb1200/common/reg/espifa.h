/**************************************************************************//**
 * @file     espifa.h
 * @brief    Define ESPIFA's register and function
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef ESPIFA_H
#define ESPIFA_H

/**
  \brief  Structure type to access ESPI Flash Access (ESPIFA).
 */
typedef volatile struct
{
  uint8_t  ESPIFAIE;                                  /*Interrupt Enable Register */
  uint8_t  Reserved0[3];                              /*Reserved */
  uint8_t  ESPIFAPF;                                  /*Event Pending Flag Register */
  uint8_t  Reserved1[3];                              /*Reserved */
  uint8_t  ESPIFAEF;                                  /*Error Flag Register */
  uint8_t  Reserved2[3];                              /*Reserved */
  uint32_t Reserved3;                                 /*Reserved */   
  uint32_t ESPIFABA;                                  /*Base Address Register */
  uint8_t  ESPIFACNT;                                 /*Count Register */
  uint8_t  Reserved4[3];                              /*Reserved */ 
  uint16_t ESPIFALEN;                                 /*Completion length Register */
  uint16_t Reserved5;                                 /*Reserved */ 
  uint8_t  ESPIFAPTCL;                                /*Protocol Issue Register */
  uint8_t  Reserved6[3];                              /*Reserved */
  uint8_t  ESPIFADAT[64];                             /*Data Register */
}  ESPIFA_T;

//#define espifa                ((ESPIFA_T *) ESPIFA_BASE)             /* ESPIFA Struct       */
//#define ESPIFA                ((volatile unsigned long  *) ESPIFA_BASE)     /* ESPIFA  array       */

//***************************************************************
// User Define                                                
//***************************************************************
//#define ESPIFA_V_OFFSET             USR_ESPIFA_V_OFFSET
#define SUPPORT_ESPIFA              0

//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------
#define ESPIFA_WRITE                    0x01
#define ESPIFA_READ                     0x02
#define ESPIFA_ERASE                    0x03

#define ESPIFA_msTimeOut                2000    // Unit: ms, Range: 300~76500 , Base Clock: 300ms(GPT2), Max. deviation: 150ms
#define ESPIFA_TIMEOUTCNT               (mGPT_mSecToGPT2Cnt(ESPIFA_msTimeOut))

#define ESPIFA_UNSUCCESS                0x20

#define ESPIFA_BUFSIZE                  64      //Must be power of 2
#define ESPIFA_ERASE_MAX                7
#define ESPIFA_RW_MAX                   1024

typedef struct _ESPIFA      
{
    unsigned long Addr;
    unsigned short Len;
    unsigned short Index;
    unsigned char *pData;
    unsigned char TimeOutCnt;
    union
    {
        unsigned char Byte;
        struct
        {
            unsigned char BUSY      : 1;
            unsigned char TIMEOUT   : 1;
            unsigned char PROTOCOL  : 2;
        }Flag;
    }Info;
}_hwESPIFA;

//-- Macro Define -----------------------------------------------
#define mESPIFA_COMPLETE_LEN                        espifa->ESPIFALEN
#define mESPIFA_COMPLETE_ADDR                       espifa->ESPIFABA

#define mESPIFA_IssueProtocolwithAcc(bProtocol)     espifa->ESPIFAPTCL = bProtocol | 0x80
#define mESPIFA_IssueIntSource                      __NVIC_SetPendingIRQ(ESPICH3_IRQn)

//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************
#if SUPPORT_ESPIFA
extern _hwESPIFA hwESPIFA;
#endif  //SUPPORT_ESPIFA

//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use -----------------------------------------------
#if SUPPORT_ESPIFA
void ESPIFA_Timer(void);
void Handle_ESPIFA_ISR(void);
#endif  //SUPPORT_ESPIFA

//-- For OEM Use -----------------------------------------------
#if SUPPORT_ESPIFA
unsigned char ESPIFA_Transcation(unsigned long dwAddr, unsigned short bLen, unsigned char* pDat, unsigned char bRWE);
unsigned char ESPIFA_BusyCheck(void);
#endif  //SUPPORT_ESPIFA

#endif //ESPIFA_H
