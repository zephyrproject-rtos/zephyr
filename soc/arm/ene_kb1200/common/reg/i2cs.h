/**************************************************************************//**
 * @file     i2cs.h
 * @brief    Define I2CS's register and function
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef I2CS_H
#define I2CS_H

/**
  \brief  Structure type to access I2C Slave (I2CS).
 */
typedef volatile struct
{
  uint32_t I2CSCFG;                                   /*Configuration Register */
  uint8_t  I2CSIE;                                    /*Interrupt Enable Register */
  uint8_t  Reserved0[3];                              /*Reserved */
  uint8_t  I2CSPF;                                    /*Event Pending Flag Register */
  uint8_t  Reserved1[3];                              /*Reserved */
  uint8_t  I2CSSTS;                                   /*Status Register */
  uint8_t  Reserved2[3];                              /*Reserved */        
  uint8_t  I2CSDAT[32];                               /*Data Register */
  uint16_t I2CSCNT;                                   /*Count Register */
  uint16_t Reserved3;                                 /*Reserved */
  uint8_t  I2CSADRM;                                  /*Address from Master Register */
  uint8_t  Reserved4[3];                              /*Reserved */ 
  uint16_t I2CSPEC;                                   /*PEC Register */
  uint16_t Reserved5;                                 /*Reserved */
  uint8_t  I2CSCTRL;                                  /*Control Register */
  uint8_t  Reserved6[3];                              /*Reserved */  
  uint8_t  I2CSNCFG;                                  /*HostNotify Configuration Register */
  uint8_t  Reserved7[3];                              /*Reserved */ 
  uint8_t  I2CSNADR;                                  /*HostNotify slave address Register */
  uint8_t  Reserved8[3];                              /*Reserved */
  uint16_t I2CSNDAT;                                  /*HostNotify slave data Register */
  uint16_t Reserved9;                                 /*Reserved */     
  uint8_t  I2CSNCTRL;                                 /*HostNotify control Register */
  uint8_t  Reserved10[3];                             /*Reserved */
  uint8_t  I2CSNSTS;                                  /*HostNotify Status Register */
  uint8_t  Reserved11[3];                             /*Reserved */                                                                                
}  I2CS_T;

#define i2cs0                ((I2CS_T *) I2CS0_BASE)             /* I2CS Struct       */
#define i2cs1                ((I2CS_T *) I2CS1_BASE)             /* I2CS Struct       */
#define i2cs2                ((I2CS_T *) I2CS2_BASE)             /* I2CS Struct       */
#define i2cs3                ((I2CS_T *) I2CS3_BASE)             /* I2CS Struct       */

#define I2CS                ((volatile unsigned long  *) I2CS_BASE)     /* I2CS  array       */

//***************************************************************
// User Define                                                
//***************************************************************
#define I2CS_V_OFFSET                       USR_I2CS_V_OFFSET
#define SUPPORT_I2CS0                       USR_SUPPORT_I2CS0
    #define I2CS0_BUS_NUM                   USR_I2CS0_BUS_NUM
    #define I2CS0_ADDR_INIT                 USR_I2CS0_ADDR_INIT
    #define I2CS0_ADDRMASK_INIT             USR_I2CS0_ADDRMASK_INIT
    #define I2CS0_CMD_SUPPORT               ENABLE
    #define I2CS0_DOUBLE_BUFFER_SUPPORT     ENABLE
    #define I2CS0_PEC_SUPPORT               DISABLE
    #define I2CS0_SCL_TIMEOUT_SUPPORT       USR_I2CS0_SCL_TIMEOUT_SUPPORT
    #define I2CS0_SDA_TIMEOUT_SUPPORT       USR_I2CS0_SDA_TIMEOUT_SUPPORT
    #define I2CS0_NOTIFY_SUPPORT            USR_I2CS0_NOTIFY_SUPPORT
    #define I2CS0_NOTIFY_TIMEOUT_SUPPORT    USR_I2CS0_NOTIFY_TIMEOUT_SUPPORT
    #define I2CS0_GPIO_INIT                 USR_I2CS0_GPIO_INIT

#define SUPPORT_I2CS1                       USR_SUPPORT_I2CS1
    #define I2CS1_BUS_NUM                   USR_I2CS1_BUS_NUM
    #define I2CS1_ADDR_INIT                 USR_I2CS1_ADDR_INIT
    #define I2CS1_ADDRMASK_INIT             USR_I2CS1_ADDRMASK_INIT
    #define I2CS1_CMD_SUPPORT               ENABLE
    #define I2CS1_DOUBLE_BUFFER_SUPPORT     ENABLE
    #define I2CS1_PEC_SUPPORT               DISABLE
    #define I2CS1_SCL_TIMEOUT_SUPPORT       USR_I2CS1_SCL_TIMEOUT_SUPPORT
    #define I2CS1_SDA_TIMEOUT_SUPPORT       USR_I2CS1_SDA_TIMEOUT_SUPPORT
    #define I2CS1_NOTIFY_SUPPORT            USR_I2CS1_NOTIFY_SUPPORT
    #define I2CS1_NOTIFY_TIMEOUT_SUPPORT    USR_I2CS1_NOTIFY_TIMEOUT_SUPPORT
    #define I2CS1_GPIO_INIT                 USR_I2CS1_GPIO_INIT

#define SUPPORT_I2CS2                       USR_SUPPORT_I2CS2
    #define I2CS2_BUS_NUM                   USR_I2CS2_BUS_NUM
    #define I2CS2_ADDR_INIT                 USR_I2CS2_ADDR_INIT
    #define I2CS2_ADDRMASK_INIT             USR_I2CS2_ADDRMASK_INIT
    #define I2CS2_CMD_SUPPORT               ENABLE
    #define I2CS2_DOUBLE_BUFFER_SUPPORT     ENABLE
    #define I2CS2_PEC_SUPPORT               DISABLE
    #define I2CS2_SCL_TIMEOUT_SUPPORT       USR_I2CS2_SCL_TIMEOUT_SUPPORT
    #define I2CS2_SDA_TIMEOUT_SUPPORT       USR_I2CS2_SDA_TIMEOUT_SUPPORT
    #define I2CS2_NOTIFY_SUPPORT            USR_I2CS2_NOTIFY_SUPPORT
    #define I2CS2_NOTIFY_TIMEOUT_SUPPORT    USR_I2CS2_NOTIFY_TIMEOUT_SUPPORT
    #define I2CS2_GPIO_INIT                 USR_I2CS2_GPIO_INIT
    
#define SUPPORT_I2CS3                       USR_SUPPORT_I2CS3
    #define I2CS3_BUS_NUM                   USR_I2CS3_BUS_NUM
    #define I2CS3_ADDR_INIT                 USR_I2CS3_ADDR_INIT
    #define I2CS3_ADDRMASK_INIT             USR_I2CS3_ADDRMASK_INIT
    #define I2CS3_CMD_SUPPORT               ENABLE
    #define I2CS3_DOUBLE_BUFFER_SUPPORT     ENABLE
    #define I2CS3_PEC_SUPPORT               DISABLE
    #define I2CS3_SCL_TIMEOUT_SUPPORT       USR_I2CS3_SCL_TIMEOUT_SUPPORT
    #define I2CS3_SDA_TIMEOUT_SUPPORT       USR_I2CS3_SDA_TIMEOUT_SUPPORT
    #define I2CS3_NOTIFY_SUPPORT            USR_I2CS3_NOTIFY_SUPPORT
    #define I2CS3_NOTIFY_TIMEOUT_SUPPORT    USR_I2CS3_NOTIFY_TIMEOUT_SUPPORT
    #define I2CS3_GPIO_INIT                 USR_I2CS3_GPIO_INIT
//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------
#define I2CS_NUM                        4//2
#define I2CS0                           0x00
#define I2CS1                           0x01
#define I2CS2                           0x02
#define I2CS3                           0x03
                                        
#define I2CS_PRTC_ERROR                 0x04
#define I2CS_PEC_ERROR                  0x03
#define I2CS_SDA_TIMEOUT                0x02
#define I2CS_SMBUS_TIMEOUT              0x01
#define I2CS_NO_ERROR                   0x00

#define HOST_NOTIFY_NO_ERROR            0x00     
#define HOST_NOTIFY_TIMEOUT             0x01     
#define HOST_NOTIFY_HOST_NO_ACK         0x02     
#define HOST_NOTIFY_PRTC_ERROR          0x03     
#define HOST_NOTIFY_LOST_ARBIT          0x04     
                                        
#define Slave_RX                        0x00
#define Slave_TX                        0x01
                                        
#define Read_Protocol                   0x01
#define Write_Protocol                  0x00
                                        
#define I2CS_DATA_BUF_SIZE              0x20//0x10
#define I2CS_DATA_BUF_HALF_SIZE         0x10//0x08
                                        
#define I2CS_BUSY                       0x01
#define I2CS_CMD                        0x02
#define I2CS_HALF                       0x04
#define I2CS_HALF_SWAP_FLAG             0x08
#define I2CS_READ_ONLY                  0x10
#define I2CS_WRITE_ONLY                 0x20

//-- Struction Define --------------------------------------------
typedef struct _HWI2CS          // Size = 17
{
    unsigned short Index;
    unsigned short DataSize;
    unsigned char *pData;       // XRAM/Code data pointer
    volatile unsigned char *pReg;        // I2CS Register pointer
    unsigned char SlaveAddr;
    unsigned char Cmd;    
    unsigned char Flag;         /* Bit 7-6: Reserved
                                   Bit 5  : Write Only Enable
                                   Bit 4  : Read Only Enable
                                   Bit 3  : Half switch flag
                                   Bit 2  : 8 byte (Half) interrupt status
                                   Bit 1  : Cmd(Data0) interrupt status
                                            1: Have Cmd byte interrupt, the hwI2CS.Cmd is the command data
                                            0: No Cmd byte interrupt
                                   Bit 0  : Busy Flag */
}_hwI2CS;

//-- Macro Define -----------------------------------------------
#define mI2CS_Bus0_Enable               mFSMBM_Bus0_Enable  
#define mI2CS_Bus1_Enable               mFSMBM_Bus1_Enable  
#define mI2CS_Bus2_Enable               mFSMBM_Bus2_Enable  
#define mI2CS_Bus3_Enable               mFSMBM_Bus3_Enable  
#define mI2CS_Bus4_Enable               mFSMBM_Bus4_Enable  
#define mI2CS_Bus5_Enable               mFSMBM_Bus5_Enable  
#define mI2CS_Bus6_Enable               mFSMBM_Bus6_Enable 
#define mI2CS_Bus7_Enable               mFSMBM_Bus7_Enable 
#define mI2CS_Bus8_Enable               mFSMBM_Bus8_Enable 
#define mI2CS_Bus9_Enable               mFSMBM_Bus9_Enable 
                                                            
#define mI2CS_Bus0_Disable              mFSMBM_Bus0_Disable 
#define mI2CS_Bus1_Disable              mFSMBM_Bus1_Disable 
#define mI2CS_Bus2_Disable              mFSMBM_Bus2_Disable 
#define mI2CS_Bus3_Disable              mFSMBM_Bus3_Disable 
#define mI2CS_Bus4_Disable              mFSMBM_Bus4_Disable 
#define mI2CS_Bus5_Disable              mFSMBM_Bus5_Disable 
#define mI2CS_Bus6_Disable              mFSMBM_Bus6_Disable
#define mI2CS_Bus7_Disable              mFSMBM_Bus7_Disable
#define mI2CS_Bus8_Disable              mFSMBM_Bus8_Disable
#define mI2CS_Bus9_Disable             mFSMBM_Bus9_Disable

//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************
#if SUPPORT_I2CS0
extern _hwI2CS hwI2CS0;
#endif  // SUPPORT_I2CS0
#if SUPPORT_I2CS1
extern _hwI2CS hwI2CS1;
#endif  // SUPPORT_I2CS1
#if SUPPORT_I2CS2
extern _hwI2CS hwI2CS2;
#endif  // SUPPORT_I2CS2
#if SUPPORT_I2CS3
extern _hwI2CS hwI2CS3;
#endif  // SUPPORT_I2CS3
extern unsigned char I2CS_Host_Notify;

//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use -----------------------------------------------

//-- For OEM Use -----------------------------------------------
extern void I2CS_Init(void);
extern void I2CS_Port_Selection(unsigned char I2CS_Num, unsigned char BusNum);
extern void I2CS_pData_Reset(unsigned char bI2CSNum);
extern void I2CS_Read_Only_Enable(unsigned char bI2CSNum);
extern void I2CS_Write_Only_Enable(unsigned char bI2CSNum);
extern void I2CS_DATA_Half_Handle_ISR(_hwI2CS *hwI2CS);
extern void DataSizeChk_CopyData(_hwI2CS *hwI2CS, unsigned char TempLen, unsigned char DataCopyDir);
extern void I2CS_ChkRestartWriteBuf(I2CS_T* i2cs, _hwI2CS *hwI2CS);
extern void I2CS_PRTO_Finish_ISR(I2CS_T* i2cs, _hwI2CS *hwI2CS);
extern void I2CS_DAT_TRANS_RAM_Assign(unsigned char bI2CSNum, unsigned short LEN, unsigned char *DAT);
extern void I2CS_Issue_Host_Notify(unsigned char bI2CSNum, unsigned char SADDR, unsigned short DAT);
extern void I2CS_Disable(unsigned char bI2CSNum);
extern void I2CS_ISR(I2CS_T* i2cs);
extern void MEM_Copy_I2CS_Reg_to_Dat_ISR(_hwI2CS *hwI2CS, unsigned char len);
extern void MEM_Copy_I2CS_Dat_to_Reg_ISR(_hwI2CS *hwI2CS, unsigned char len);
extern void I2CS_GPIO_Enable(unsigned char Bus_Num);
extern void I2CS_GPIO_Disable(unsigned char Bus_Num);
#endif //I2CS_H
