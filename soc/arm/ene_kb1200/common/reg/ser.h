/**************************************************************************//**
 * @file     ser.h
 * @brief    Define SER's registers
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/
 
#ifndef SER_H
#define SER_H 

/**
  \brief  Structure type to access Serial Port Interface (SER).
 */
typedef volatile struct
{
  uint32_t SERCFG;                                    /*Configuration Register */
  uint32_t SERIE;                                     /*Interrupt Enable Register */
  uint32_t SERPF;                                     /*Pending flag Register */
  uint32_t SERSTS;                                    /*Status Register */
  uint32_t SERRBUF;                                   /*Rx Data Buffer Register */
  uint32_t SERTBUF;                                   /*Tx Data Buffer Register */
  uint32_t SERCTRL;                                   /*Control Register */
}  SER_T;

//#define ser                ((SER_T *) SER_BASE)             /* SER Struct       */
#define ser1               ((SER_T *) SER1_BASE)             /* SER Struct       */
#define ser2               ((SER_T *) SER2_BASE)             /* SER Struct       */
#define ser3               ((SER_T *) SER3_BASE)             /* SER Struct       */

//***************************************************************
// User Define                                                
//***************************************************************
#define Support_SER2                    ENABLE
    #define SER2_PARITY                 0           //0:Disable  1:Odd parity  2:Disable  3:Even parity     
    #define SER2_BAUDRATE               115200                                                              
    #define Support_SER2_TX             ENABLE
        #define Support_SER2_TX_DMA     ENABLE
    #define Support_SER2_RX             ENABLE
        #define Support_SER2_RX_DMA     ENABLE
        #define SER2_RX_FULL_CNT        8           //1~16                                                        
        #define SER2_RX_TIMEOUT         3           //1~16 ms
                                                                                                            
#define Support_SER3                    ENABLE
    #define SER3_PARITY                 0           //0:Disable  1:Odd parity  2:Disable  3:Even parity     
    #define SER3_BAUDRATE               115200                                                              
    #define Support_SER3_TX             ENABLE
        #define Support_SER3_TX_DMA     ENABLE
    #define Support_SER3_RX             ENABLE
        #define Support_SER3_RX_DMA     ENABLE
        #define SER3_RX_FULL_CNT        8           //1~16                                                        
        #define SER3_RX_TIMEOUT         3           //1~16 ms                                               

//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------
#define ser_log                         ser1
#define SER_TX_GPIO_Num                 SER1_TX_GPIO_Num
#define SER_RX_GPIO_Num                 SER1_RX_GPIO_Num
  
#define SER1_TX_GPIO_Num                0x03
#define SER1_RX_GPIO_Num                0x01
#define SER2_TX_GPIO_Num                0x36
#define SER2_RX_GPIO_Num                0x28
#define SER3_TX_GPIO_Num                0x04
#define SER3_RX_GPIO_Num                0x55

#define SER_BAUD_RATE                   115200
#define SER_BAUD_RATE_SET_VALUE         (24000000UL/SER_BAUD_RATE)

#define SER_RX_FUNCTION                 DISABLE
#define SER_TX_FUNCTION                 ENABLE

#define SER_Mode0                       0                   //shift
#define SER_Mode1                       1                   //8-bit
#define SER_Mode2                       2                   //9-bit
#define SER_Mode3                       3                   //9-bit

#define SER_Mode0_8M                    0
#define SER_Mode0_4M                    1
#define SER_Mode0_2M                    2
#define SER_Mode0_1M                    3

#define SER_DisableParity               0
#define SER_OddParity                   1
#define SER_EvenParity                  3


//Pending Flag
#define SER_Rx_CNT_Full                 0x01
#define SER_Tx_Empty                    0x02
#define SER_Rx_Error                    0x04
#define SER_All_PF                      (SER_Tx_Empty|SER_Rx_CNT_Full|SER_Rx_Error)

//Status Flag
#define SER_FramingError                0x0200
#define SER_ParityError                 0x0100
#define SER_RxTimeOut                   0x0080
#define SER_RxBusy                      0x0040
#define SER_RxOverRun                   0x0020
#define SER_RxEmpty                     0x0010
#define SER_TxBusy                      0x0004
#define SER_TxOverrun                   0x0002
#define SER_TxFull                      0x0001
#define SER_All_StsFlag                 (SER_FramingError|SER_ParityError|SER_RxTimeOut|SER_RxOverRun|SER_TxOverrun)

   
//-- Macro Define -----------------------------------------------
//#define mSER_SET_MODE(mode)             ser->SERCTRL |= (ser->SERCTRL&(~0x03UL))|mode
//#define mGet_SER_MODE                   (ser->SERCTRL&0x03UL)
//#define mSER_SET_BAUD_RATE(baud_rate)   ser->SERCFG |= (ser->SERCFG&0x0000FFFF)|(baud_rate<<16)
//#define mSER_SET_Mode0_Clock(clk)       ser->SERCFG |= (ser->SERCFG&(~0x000000C0))|(clk<<6)    
//
//#define mE51TX_SBUF_FIFO_FULL           (ser->SERSTS&0x01)
//#define mSER_WRITE_TXBUF(Dat)           ser->SERTBUF = Dat
//#define mSER_Read_RXBUF                 ser->SERRBUF
//
//#define mSER_Hold_FIFO                  ser->SERCTRL |= 0x04
//#define mSER_Release_FIFO               ser->SERCTRL &= ~0x04UL
//#define mSER_TX_Busy                    (ser->SERSTS&0x04)

#define mSER_BAUD_RATE_VALUE(baud_rate)     (24000000UL/baud_rate)

#define mSER_SET_BAUD_RATE(ser,baud_rate)   ser->SERCFG = (ser->SERCFG&0x0000FFFF)|(baud_rate<<16)
#define mSER_SET_Mode0_Clock(ser,clk)       ser->SERCFG = (ser->SERCFG&(~0x000000C0))|(clk<<6) 
#define mSER_SET_Parity(ser,EvenOdd)        ser->SERCFG = (ser->SERCFG&(~0x000CUL))|(EvenOdd<<2)

#define mSER_Hold_FIFO(ser)                 ser->SERCTRL |= 0x04
#define mSER_Release_FIFO(ser)              ser->SERCTRL &= ~0x04UL
#define mSER_SET_MODE(ser,mode)             ser->SERCTRL  = (ser->SERCTRL&(~0x03UL))|mode
#define mGet_SER_MODE(ser)                  (ser->SERCTRL&0x03UL)

#define mSER_Tx_Enable(ser)                 ser->SERCFG |= 0x0002UL
#define mSER_Tx_Disable(ser)                ser->SERCFG &= (~0x0002UL)
#define mSER_Tx_Int_Enable(ser)             ser->SERIE  |= 0x0002UL
#define mSER_Tx_Int_Disable(ser)            ser->SERIE  &= (~0x0002UL)

#define mSER_Rx_Enable(ser)                 ser->SERCFG |= 0x0001UL
#define mSER_Rx_Disable(ser)                ser->SERCFG &= (~0x0001UL)
#define mSER_SET_RxFullCNT(ser,cnt)         ser->SERCFG  = (ser->SERCFG&(~0x0F00UL))|((cnt-1)<<8)
#define mSER_SET_RxTimeout(ser,time)        ser->SERCFG  = (ser->SERCFG&(~0xF000UL))|(time<<12)
#define mSER_Rx_Full_Int_Enable(ser)        ser->SERIE  |= 0x0001UL
#define mSER_Rx_Full_Int_Disable(ser)       ser->SERIE  &= (~0x0001UL)
#define mSER_Rx_Error_Int_Enable(ser)       ser->SERIE  |= 0x0004UL
#define mSER_Rx_Error_Int_Disable(ser)      ser->SERIE  &= (~0x0004UL)

#define mSER_PF_Get(ser,Flag)               (ser->SERPF&Flag)
#define mSER_PF_Clr(ser,Flag)               ser->SERPF = Flag

#define mSER_STS_Get(ser,Flag)              (ser->SERSTS&Flag)
#define mSER_STS_Clr(ser,Flag)              ser->SERSTS = Flag
#define mSER_TX_Busy(ser)                   (ser->SERSTS&SER_TxBusy)
#define mSER_TX_FIFO_FULL(ser)              (ser->SERSTS&SER_TxFull)
#define mSER_RX_Busy(ser)                   (ser->SERSTS&SER_RxBusy)
#define mSER_RX_FIFO_Empty(ser)             (ser->SERSTS&SER_RxEmpty)

#define mSER_WRITE_TXBUF(ser,Dat)           ser->SERTBUF = Dat
#define mSER_Read_RXBUF(ser)                ser->SERRBUF

//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************

//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use -----------------------------------------------
void Ser_Init(void);

//-- For OEM Use -----------------------------------------------
//char putchar(char);
//char puts(char *);

void display_char(unsigned char);
void display_word(unsigned short); 
void display_long(unsigned long); 
void DisplayAddr(unsigned char *dst_reg_ptr,unsigned char StartIndex, unsigned char EndIndex);

#endif //SER_H
