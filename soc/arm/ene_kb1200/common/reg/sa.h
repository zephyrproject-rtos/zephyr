/**************************************************************************//**
 * @file     sa.h
 * @brief    Define Security Accelerator's registers
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/
 
#ifndef SA_H
#define SA_H 

/**
  \brief  Structure type to access Security Accelerator (SA).
 */
typedef struct
{
  uint32_t SAFADDR;                                   /*SA Fetcher Address Register */
  uint32_t Reserved0;                                 /*Reserved */
  uint32_t SAFCTR;                                    /*SA Fetcher Control Register */
  uint32_t SAFTAG;                                    /*SA Fetcher Tag Register */
  uint32_t SAPADDR;                                   /*SA Pusher Address */
  uint32_t Reserved1;                                 /*Reserved */
  uint32_t SAPCTR;                                    /*SA Pusher Control Register */
  uint32_t SAIE;                                      /*SA Interrupt Enable Register */
  uint32_t SAIES;                                     /*SA Interrupt Enable Single Set Register */
  uint32_t SAIEC;                                     /*SA Interrupt Enable Single Clear */
  uint32_t SAE;                                       /*SA event Register */
  uint32_t SAINTE;                                    /*SA Interrupt Event Register */
  uint32_t SAEC;                                      /*SA event clear Register */
  uint32_t SACFG;                                     /*SA Configure Register */
  uint32_t SAST;                                      /*SA Start Register*/
  uint32_t SASTA;                                     /*SA Status Register */
} SA_T;

#define sa                ((SA_T *) SA_BASE)             /* SER Struct       */

//***************************************************************
// User Define                                                
//***************************************************************

//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------


//-- Macro Define -----------------------------------------------

//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************
extern unsigned char *sa_dmamem;
extern void hexdump1(const char *name, const char *v, size_t len);

//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use -----------------------------------------------
void Sa_Init(void);

//-- For OEM Use -----------------------------------------------
int SA_SHA_Calculate(const unsigned char SHA_Arg,unsigned long len,unsigned char *msg);

#endif //SA_H
