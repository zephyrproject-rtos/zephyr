/**************************************************************************//**
 * @file     peci.h
 * @brief    Define PECI's registers
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef PECI_H
#define PECI_H

/**
  \brief  Structure type to access Platform Environment Control Interface (PECI).
 */
typedef volatile struct
{
  uint16_t PECICFG;                                   /*Configuration Register */
  uint16_t Reserved0;                                 /*Reserved */    
  uint8_t  PECIIE;                                    /*Interrupt Enable Register */
  uint8_t  Reserved1[3];                              /*Reserved */
  uint8_t  PECIPF;                                    /*Event Pending Flag Register */
  uint8_t  Reserved2[3];                              /*Reserved */
  uint8_t  PECICTL;                                   /*Control Register */
  uint8_t  Reserved3[3];                              /*Reserved */        
  uint8_t  PECIADR;                                   /*Target Address Register */
  uint8_t  Reserved4[3];                              /*Reserved */
  uint8_t  PECILENW;                                  /*Write Length Register */
  uint8_t  PECILENR;                                  /*Read Length Register */  
  uint16_t Reserved5;                                 /*Reserved */ 
  uint8_t  PECIWD;                                    /*Write Data Register */
  uint8_t  Reserved6[3];                              /*Reserved */ 
  uint8_t  PECIRD;                                    /*Read Data Register */
  uint8_t  Reserved7[3];                              /*Reserved */
  uint8_t  PECIST;                                    /*Status Register */
  uint8_t  Reserved8[3];                              /*Reserved */ 
  uint8_t  PECIOFCS;                                  /*FCS generated from originator Register */ 
  uint8_t  PECIAWFCS;                                 /*AW FCS generated from originator Register */ 
  uint8_t  PECICWFCS;                                 /*W-FCS generated from client Register */ 
  uint8_t  PECICRFCS;                                 /*R-FCS generated from client Register */ 
}  PECI_T;

//#define peci                ((PECI_T *) PECI_BASE)             /* PECI Struct       */

//***************************************************************
// User Define                                                
//***************************************************************
#define PECI_V_OFFSET           USR_PECI_V_OFFSET 
#define SUPPORT_PECI            USR_SUPPORT_PECI            //0 : Disable PECI; 1 : Enable PECI

//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------
#define PECI_GPIO_Num           0x21

#define MAILBOX_CMD_DATA_SIZE   0x24

//PECI command
#define	Get_DIB_CMD		        0xF7
#define Get_Temp_CMD	        0x01

//-- Macro Define -----------------------------------------------
//Master status
#define mPECI_Tx_State		            (peci->PECIST&0x20)
#define	mPECI_Rx_State		            (peci->PECIST&0x10)
#define mPECI_Bus_Status		        (peci->PECIST&0x08)
#define mPECI_Bus_Busy		            (peci->PECIST&0x04)
#define mPECI_FIFO_Full		            (peci->PECIST&0x02)
#define mPECI_FIFO_Empty		        (peci->PECIST&0x01)
                                        
//Master interrupt status               
#define mPECI_Client_Abort	            (peci->PECIPF&0x08)
#define mPECI_FCS_Fault		            (peci->PECIPF&0x04)
#define mPECI_FIFO_Half		            (peci->PECIPF&0x02)
#define mPECI_FIFO_Error		        (peci->PECIPF&0x01)
#define mPECI_CLR_Int_Status            peci->PECIPF = 0x0F
                                        
#define mPECI_AWFCS_Enable              (peci->PECICTL&0x10)
#define mPECI_AWFCS_Function_Enable  	peci->PECICTL |=  0x10
#define mPECI_AWFCS_Function_Disable 	peci->PECICTL &= ~0x10

#define mPECI_Issue_CMD                 peci->PECICTL |= 0x01
#define mPECI_Set_CMD(cmd)              peci->PECIWD = cmd
                                        
#define mPECI_Clear_FIFO                peci->PECICTL |= 0x04
                                        
#define mPECI_Set_Address(addr)         peci->PECIADR = addr       

////***************************************************************
////  Extern Varible Declare                                          
////***************************************************************
//// Reserved for Kernel codeCIR Driver Use 
////BIOS ACPI CMD/ARG
//extern unsigned char Mailbox_ACPI_CMD_RetData;
//extern unsigned char Mailbox_ACPI_CMD;
//extern unsigned char Mailbox_ACPI_EC0[1];  //PECI Error counter
//extern unsigned char Mailbox_ACPI_EC1;  //PECI Error counter
//extern unsigned char Mailbox_ACPI_EC2;  //PECI Error counter
//extern unsigned char Mailbox_ACPI_EC3;  //PECI Error counter
//
////Intel Mail Box Table
//extern unsigned char Mailbox_Repeat_Cycle0[1];
//extern unsigned char Mailbox_Repeat_Cycle1;
//extern unsigned char Mailbox_Repeat_Cycle2;
//extern unsigned char Mailbox_Repeat_Cycle3;
//extern unsigned char Mailbox_Repeat_Interval;
//extern unsigned char Mailbox_Stop_on_Error;
//extern unsigned char Mailbox_Client_Address;
//extern unsigned char Mailbox_W_Length;
//extern unsigned char Mailbox_R_Length;
//extern unsigned char Mailbox_CMD_Data[MAILBOX_CMD_DATA_SIZE];
//extern unsigned char Mailbox_AW_FCS;
//extern unsigned char Mailbox_FCS;
//extern unsigned char Mailbox_Flag;
//extern unsigned short GPT0_Temp_PECI;
//extern unsigned char Mailbox_ACPI_Status;
//extern unsigned char Mailbox_ACPI_LEN;
//
////***************************************************************
////  Extern Function Declare                                          
////***************************************************************
////-- Kernel Use -----------------------------------------------
//#if SUPPORT_PECI
//void EnESrvcPECI(void);
//void Check_Mailbox_MainLoop(void);
//
////-- For OEM Use -----------------------------------------------
//unsigned char Ping(unsigned char);
//unsigned char Get_DIB(unsigned char);
//unsigned char Get_Temp(unsigned char);
//unsigned char PECI_protocol(unsigned char ,unsigned char ,unsigned char ,unsigned char ,unsigned char* ,unsigned char);
//unsigned char Check_Complete(void);
//void Read_PECI_Data(unsigned char* );
//void Init_PECI(void);
//void PECI_GPIO_ENABLE(void);
//void PECI_GPIO_DISABLE(void);
//void PECI_Disable(void);
//#endif  //SUPPORT_PECI

#endif //PECI_H
