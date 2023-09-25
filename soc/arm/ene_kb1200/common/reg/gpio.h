/**************************************************************************//**
 * @file     gpio.h
 * @brief    Define GPIO's registers
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef GPIO_H
#define GPIO_H


/**
  \brief  Structure type to access General Purpose Input/Output (GPIO).
 */
typedef volatile struct
{
  uint32_t GPIOFSxx[4];                               /*Function Selection Register */
  uint32_t GPIOOExx[4];                               /*Output Enable Register */
  uint32_t GPIODxx[4];                                /*Output Data Register */
  uint32_t GPIOINxx[4];                               /*Input Data Register */
  uint32_t GPIOPUxx[4];                               /*Pull Up Register */
  uint32_t GPIOODxx[4];                               /*Open Drain Register */
  uint32_t GPIOIExx[4];                               /*Input Enable Register */
  uint32_t GPIODCxx[4];                               /*Driving Control Register */
  uint32_t GPIOLVxx[4];                               /*Low Voltage Mode Enable Register */
  uint32_t GPIOPDxx[4];                               /*Pull Down Register */
  uint32_t GPIOFLxx[4];                               /*Function Lock Register */
}  GPIO_T;


//***************************************************************
// User Define                                                
//***************************************************************
#define GPIO_V_OFFSET                   USR_GPIO_V_OFFSET
#define SUPPORT_GPIO_00_0F_ISR          USR_SUPPORT_GPIO_00_0F_ISR
#define SUPPORT_GPIO_10_1F_ISR          USR_SUPPORT_GPIO_10_1F_ISR
#define SUPPORT_GPIO_20_2F_ISR          USR_SUPPORT_GPIO_20_2F_ISR
#define SUPPORT_GPIO_30_3F_ISR          USR_SUPPORT_GPIO_30_3F_ISR
#define SUPPORT_GPIO_40_4F_ISR          USR_SUPPORT_GPIO_40_4F_ISR
#define SUPPORT_GPIO_50_5F_ISR          USR_SUPPORT_GPIO_50_5F_ISR
#define SUPPORT_GPIO_60_6F_ISR          USR_SUPPORT_GPIO_60_6F_ISR
#define SUPPORT_GPIO_70_7F_ISR          USR_SUPPORT_GPIO_70_7F_ISR

//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define --------------------------------------------
#define GPIO00_PWMLED0_PWM8             0x00
#define GPIO22_ESBDAT_PWM9              0x22
#define GPIO28_32KOUT_SERCLK2           0x28
#define GPIO36_UARTSOUT_SERTXD2         0x36
#define GPIO5C_KSO6_P80DAT              0x5C
#define GPIO5D_KSO7_P80CLK              0x5D
#define GPIO5E_KSO8_SERRXD1             0x5E
#define GPIO5F_KSO9_SERTXD1             0x5F
#define GPIO71_SDA8_UARTRTS             0x71
#define GPIO38_SCL4_PWM1                0x38


#define GPIO_FS_OFFSET                  0x00
#define GPIO_OE_OFFSET                  0x04
#define GPIO_DO_OFFSET                  0x08
#define GPIO_IN_OFFSET                  0x0C
#define GPIO_PU_OFFSET                  0x10
#define GPIO_OD_OFFSET                  0x14
#define GPIO_IE_OFFSET                  0x18
#define GPIO_DC_OFFSET                  0x1C
#define GPIO_LV_OFFSET                  0x20
#define GPIO_PD_OFFSET                  0x24
#define GPIO_FL_OFFSET                  0x28

//GPIO Configuration Define
#define GPIO_bTYPE_BITNum               10//8
#define GPIO_bTYPE_FS                   0x01
#define GPIO_bTYPE_OE                   0x02
#define GPIO_bTYPE_DO                   0x04
#define GPIO_bTYPE_PU                   0x08
#define GPIO_bTYPE_OD                   0x10
#define GPIO_bTYPE_IE                   0x20
#define GPIO_bTYPE_LV                   0x40                                        
#define GPIO_bTYPE_DC                   0x80
#define GPIO_bTYPE_PD                   0x100                                        
#define GPIO_bTYPE_FL                   0x200


//-- Macro Define -----------------------------------------------
#define mGPIOFS_Dis(GPIONum)            gpio->GPIOFSxx[GPIONum>>5] &= ~(1UL<<((GPIONum)&0x1F))
#define mGPIOFS_En(GPIONum)             gpio->GPIOFSxx[GPIONum>>5] |=  (1UL<<((GPIONum)&0x1F))
#define mGPIOOE_Dis(GPIONum)            gpio->GPIOOExx[GPIONum>>5] &= ~(1UL<<((GPIONum)&0x1F))
#define mGPIOOE_En(GPIONum)             gpio->GPIOOExx[GPIONum>>5] |=  (1UL<<((GPIONum)&0x1F))
#define mGPIODO_Low(GPIONum)            gpio->GPIODxx[GPIONum>>5]  &= ~(1UL<<((GPIONum)&0x1F))
#define mGPIODO_High(GPIONum)           gpio->GPIODxx[GPIONum>>5]  |=  (1UL<<((GPIONum)&0x1F))
#define mGPIODO_Get(GPIONum)            ((gpio->GPIODxx[GPIONum>>5] & (1UL<<((GPIONum)&0x1F)))?1:0)
#define mGPIODO_Chg(GPIONum)            gpio->GPIODxx[GPIONum>>5]  ^=  (1UL<<((GPIONum)&0x1F))    
#define mGPIOIN_Get(GPIONum)            ((gpio->GPIOINxx[GPIONum>>5] & (1UL<<((GPIONum)&0x1F)))?1:0)
#define mGPIOPU_Dis(GPIONum)            gpio->GPIOPUxx[GPIONum>>5] &= ~(1UL<<((GPIONum)&0x1F))
#define mGPIOPU_En(GPIONum)             gpio->GPIOPUxx[GPIONum>>5] |=  (1UL<<((GPIONum)&0x1F))
#define mGPIOOD_Dis(GPIONum)            gpio->GPIOODxx[GPIONum>>5] &= ~(1UL<<((GPIONum)&0x1F))
#define mGPIOOD_En(GPIONum)             gpio->GPIOODxx[GPIONum>>5] |=  (1UL<<((GPIONum)&0x1F))
#define mGPIOIE_Dis(GPIONum)            gpio->GPIOIExx[GPIONum>>5] &= ~(1UL<<((GPIONum)&0x1F))
#define mGPIOIE_En(GPIONum)             gpio->GPIOIExx[GPIONum>>5] |=  (1UL<<((GPIONum)&0x1F))
#define mGPIODC_Dis(GPIONum)            gpio->GPIODCxx[GPIONum>>5] &= ~(1UL<<((GPIONum)&0x1F))
#define mGPIODC_En(GPIONum)             gpio->GPIODCxx[GPIONum>>5] |=  (1UL<<((GPIONum)&0x1F))
#define mGPIOLV_Dis(GPIONum)            gpio->GPIOLVxx[GPIONum>>5] &= ~(1UL<<((GPIONum)&0x1F))
#define mGPIOLV_En(GPIONum)             gpio->GPIOLVxx[GPIONum>>5] |=  (1UL<<((GPIONum)&0x1F))

#define mGPIOPD_Dis(GPIONum)            gpio->GPIOPDxx[GPIONum>>5] &= ~(1UL<<((GPIONum)&0x1F))
#define mGPIOPD_En(GPIONum)             gpio->GPIOPDxx[GPIONum>>5] |=  (1UL<<((GPIONum)&0x1F))
#define mGPIOFL_En(GPIONum)             gpio->GPIOFLxx[GPIONum>>5] |=  (1UL<<((GPIONum)&0x1F))  //Function Lock Write 1 only

//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************
#if SUPPORT_GPIO_00_0F_ISR
extern unsigned short GPIO_00_0F_ServiceFlag;
#endif  //SUPPORT_GPIO_00_0F_ISR
#if SUPPORT_GPIO_10_1F_ISR
extern unsigned short GPIO_10_1F_ServiceFlag;
#endif  //SUPPORT_GPIO_10_1F_ISR
#if SUPPORT_GPIO_20_2F_ISR
extern unsigned short GPIO_20_2F_ServiceFlag;
#endif  //SUPPORT_GPIO_20_2F_ISR
#if SUPPORT_GPIO_30_3F_ISR
extern unsigned short GPIO_30_3F_ServiceFlag;
#endif  //SUPPORT_GPIO_30_3F_ISR
#if SUPPORT_GPIO_40_4F_ISR
extern unsigned short GPIO_40_4F_ServiceFlag;
#endif  //SUPPORT_GPIO_40_4F_ISR
#if SUPPORT_GPIO_50_5F_ISR
extern unsigned short GPIO_50_5F_ServiceFlag;
#endif  //SUPPORT_GPIO_50_5F_ISR
#if SUPPORT_GPIO_60_6F_ISR
extern unsigned short GPIO_60_6F_ServiceFlag;
#endif  //SUPPORT_GPIO_60_6F_ISR
#if SUPPORT_GPIO_70_7F_ISR
extern unsigned short GPIO_70_7F_ServiceFlag;
#endif  //SUPPORT_GPIO_70_7F_ISR

//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use ------------------------------------------------
void EnESrvcGPIO_00_0F(void);
void EnESrvcGPIO_10_1F(void);
void EnESrvcGPIO_20_2F(void);
void EnESrvcGPIO_30_3F(void);
void EnESrvcGPIO_40_4F(void);
void EnESrvcGPIO_50_5F(void);
void EnESrvcGPIO_60_6F(void);
void EnESrvcGPIO_70_7F(void);

void GPIO_00_0F_ISR(void);
void GPIO_10_1F_ISR(void);
void GPIO_20_2F_ISR(void);
void GPIO_30_3F_ISR(void);
void GPIO_40_4F_ISR(void);
void GPIO_50_5F_ISR(void);
void GPIO_60_6F_ISR(void);
void GPIO_70_7F_ISR(void);

//-- For OEM Use -----------------------------------------------
void GPIO_TypeConfig(unsigned char bGPIONum, unsigned short bType);

#endif //GPIO_H
