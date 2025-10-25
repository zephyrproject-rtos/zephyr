#ifndef __ELAN_SOC_967__
#define __ELAN_SOC_967__

#include "elan_em32.h"

typedef struct {
	__IO uint32_t ESPI_S2: 2;      // [1:0]
	__IO uint32_t SSP_S2: 2;       // [3:2]
	__IO uint32_t ISO7816_1_S: 1;  // [4]
	__IO uint32_t ISO7816_2_S: 1;  // [5]
	__IO uint32_t UART1_S: 1;      // [6]
	__IO uint32_t UART2_S: 1;      // [7]
	__IO uint32_t I2C1_S: 1;       // [8]
	__IO uint32_t I2C2_S: 1;       // [9]
	__IO uint32_t USART_S: 1;      // [10]
	__IO uint32_t DMA_CS_S2: 2;    // [12:11]
	__IO uint32_t SSP_2_4_S: 1;    // [13]
	__IO uint32_t Flash_JTAG: 1;   // [14]
	__IO uint32_t PWM_D_A1_S: 1;   // [15]
	__IO uint32_t PWM_E_B1_S: 1;   // [16]
	__IO uint32_t PWM_F_C1_S: 1;   // [17]
	__IO uint32_t PWM_S: 1;        // [18]
	__IO uint32_t LPC_PWMAB8_S: 1; // [19]
	__IO uint32_t LPC_PWMBB9_S: 1; // [20]
	__IO uint32_t LPC_Test1_S: 2;  // [22:21]
	__IO uint32_t LPC_Test2_S: 2;  // [24:23]
	__IO uint32_t Reserved: 7;     // [31:25]
} IOShare_Type;
#define IOShareCTRL ((IOShare_Type *)0x4003023c)

typedef struct {
	__IO uint32_t DATA;
	union {
		struct {
			__IO uint32_t TXBUFFULL: 1;
			__IO uint32_t RXBUFFULL: 1;
			__IO uint32_t TXBUFOVERRUN: 1;
			__IO uint32_t RXBUFOVERRUN: 1;
			__IO uint32_t Reserved: 28;
		} STATE_S;
		__IO uint32_t STATE;
	} STATE_U;
	__IO uint32_t CTRL;
	__IO uint32_t INTSTACLR;
	__IO uint32_t BAUDDIV;
	__IO uint32_t Reserved[3];
	__IO uint32_t DMALENGTHL;
	__IO uint32_t DMALENGTHH;
	__IO uint32_t DMAWAITCNT: 8;
	__IO uint32_t ReservedBit0: 24;
	__IO uint32_t DMAENANLE: 1;
	__IO uint32_t TXNRX: 1;
	__IO uint32_t ReservedBit: 30;
} UART_TypeDef;

typedef enum {
	PORTA = 0x00,
	PORTB = 0x01,
	PORTANALOG = 0x02
} GPIOPortDef;

typedef enum {
	GPIO_PINSOURCE0 = 0x00,
	GPIO_PINSOURCE1 = 0x01,
	GPIO_PINSOURCE2 = 0x02,
	GPIO_PINSOURCE3 = 0x03,
	GPIO_PINSOURCE4 = 0x04,
	GPIO_PINSOURCE5 = 0x05,
	GPIO_PINSOURCE6 = 0x06,
	GPIO_PINSOURCE7 = 0x07,
	GPIO_PINSOURCE8 = 0x08,
	GPIO_PINSOURCE9 = 0x09,
	GPIO_PINSOURCE10 = 0x0a,
	GPIO_PINSOURCE11 = 0x0b,
	GPIO_PINSOURCE12 = 0x0c,
	GPIO_PINSOURCE13 = 0x0d,
	GPIO_PINSOURCE14 = 0x0e,
	GPIO_PINSOURCE15 = 0x0f
} GPIOPinNameDef;

typedef enum {
	FALLING = 0x00,
	RISING = 0x01,
	LOWLEVEL = 0x02,
	HIGHLEVEL = 0x03
} GPIOINTDef;

typedef enum {
	GPIO_MUX00 = 0x00,
	GPIO_MUX01 = 0x01,
	GPIO_MUX02 = 0x02,
	GPIO_MUX03 = 0x03,
	GPIO_MUX04 = 0x04,
	GPIO_MUX05 = 0x05,
	GPIO_MUX06 = 0x06,
	GPIO_MUX07 = 0x07
} GPIOMUXDef;

typedef enum {
	GPIO_Mode_IN = 0x00,  /*!< GPIO Input Mode */
	GPIO_Mode_OUT = 0x01, /*!< GPIO Output Mode */
} GPIOModeDef;

typedef enum {
	GPIO_PuPd_Floating = 0x00,
	GPIO_PuPd_PullUp66K = 0x01,
	GPIO_PuPd_PullUp4_7K = 0x02,
	GPIO_PuPd_Pulldown15K = 0x03
} GPIOPuPdDef;

typedef enum {
	GPIO_OD02 = 0x00,
	GPIO_OD04 = 0x01,
	GPIO_OD06 = 0x02,
	GPIO_OD08 = 0x03,
} GPIOODDef;

typedef enum {
	Bit_RESET = 0,
	Bit_SET = 1
} BitAction;
#define IS_GPIO_BIT_ACTION(ACTION) (((ACTION) == Bit_RESET) || ((ACTION) == Bit_SET))

typedef struct {
	GPIOPinNameDef GPIO_Pin;
	GPIOModeDef GPIO_Mode;
	GPIOPuPdDef GPIO_PuPd;
} GPIO_InitTypeDef;

typedef enum {
	GPIO_PIN_0 = 0x0001,
	GPIO_PIN_1 = 0x0002,
	GPIO_PIN_2 = 0x0004,
	GPIO_PIN_3 = 0x0008,
	GPIO_PIN_4 = 0x0010,
	GPIO_PIN_5 = 0x0020,
	GPIO_PIN_6 = 0x0040,
	GPIO_PIN_7 = 0x0080,
	GPIO_PIN_8 = 0x0100,
	GPIO_PIN_9 = 0x0200,
	GPIO_PIN_10 = 0x0400,
	GPIO_PIN_11 = 0x0800,
	GPIO_PIN_12 = 0x1000,
	GPIO_PIN_13 = 0x2000,
	GPIO_PIN_14 = 0x4000,
	GPIO_PIN_15 = 0x8000,
	GPIO_PIN_ALL = 0xffff
} GPIOPinBitDef;

typedef struct {
	__IO uint16_t DATA; // 0x00
	__IO uint16_t dummy0;
	__IO uint16_t DATAOUT; // 0x04
	__IO uint16_t dummy1;
	__IO uint16_t Reserved0;
	__IO uint16_t dummy2;
	__IO uint16_t Reserved1;
	__IO uint16_t dummy3;
	__IO uint16_t DATAOUTSET; // 0x10
	__IO uint16_t dummy4;
	__IO uint16_t DATAOUTCLR; // 0x14
	__IO uint16_t dummy5;
	__IO uint16_t ALTFUNCSET; // 0x18
	__IO uint16_t dummy6;
	__IO uint16_t ALTFUNCCLR; // 0x1c
	__IO uint16_t dummy7;
	__IO uint16_t INTENSET; // 0x20
	__IO uint16_t dummy8;
	__IO uint16_t INTENCLR; // 0x24
	__IO uint16_t dummy9;
	__IO uint16_t INTTYPEEDGESET; // 0x28
	__IO uint16_t dummy10;
	__IO uint16_t INTTYPEEDGECLR; // 0x2c
	__IO uint16_t dummy11;
	__IO uint16_t INTPOLSET; // 0x30
	__IO uint16_t dummy12;
	__IO uint16_t INTPOLCLR; // 0x34
	__IO uint16_t dummy13;
	__IO uint16_t INTSTATUSANDCLR; // 0x38
	__IO uint16_t dummy14;
} GPIO_IPType;

#define GPIOIPA ((GPIO_IPType *)GPIOA_BASE)
#define GPIOIPB ((GPIO_IPType *)GPIOB_BASE)

typedef enum {
	MOTOSPI_HO00 = 0x00,
	TI = 0x01,
	NATIONAL = 0x02,
	MOTOSPI_HO01 = 0x40,
	MOTOSPI_HO10 = 0x80,
	MOTOSPI_HO11 = 0xc0,
} SSPType_Def;

typedef enum {
	SSP_MASTER = 0x00,       /*!< SPI master Mode */
	SSP_SLAVE = 0x01,        /*!< SPI slave Mode */
	SSP_MASTER_CS_IND = 0x02 /*!< CS by GPIO */
} SPIMode_Def;

typedef enum {
	SPI_O0H0 = 0x00, // SPI SPO = 0, SPH = 0
	SPI_O0H1 = 0x01, // SPI SPO = 0, SPH = 1
	SPI_O1H0 = 0x02, // SPI SPO = 1, SPH = 0
	SPI_O1H1 = 0x03  // SPI SPO = 1, SPH = 1
} SPIType_Def;

typedef enum {
	SPI_CS_IP = 0x00,       // SPI_CS by IP
	SPI_CS_IO_FLOAT = 0x01, // SPI_CS by GPIO no pull high
	SPI_CS_IO_PU66K = 0x02, // SPI_CS by GPIO pull high 66K
	SPI_CS_IO_PU4_7K = 0x03 // SPI_CS by GPIO pull high 4.7K
} CSType_Def;

typedef struct {
	SSPType_Def SSPType;
	SPIMode_Def SSPMode;
	uint8_t SSPBits;
	uint8_t SpeedDIV;
} SSP_InitTypeDef;

typedef struct {
	uint32_t DSS: 4;
	uint32_t FRF: 2;
	uint32_t SPO: 1;
	uint32_t SPH: 1;
	uint32_t SCR: 8;
	uint32_t Empty: 16;
} SSPCR0_TypeDef;

typedef struct {
	uint32_t LBM: 1;
	uint32_t SSE: 1;
	uint32_t MS: 1;
	uint32_t SOD: 1;
	uint32_t Reserved: 4;
	uint32_t Empty: 16;
} SSPCR1_TypeDef;

typedef struct {
	uint32_t TFE: 1;
	uint32_t TNF: 1;
	uint32_t RNE: 1;
	uint32_t RFF: 1;
	uint32_t BSY: 1;
	uint32_t Reserved: 11;
	uint32_t Empty: 16;
} SSPSR_TypeDef;

typedef struct {
	uint32_t CPSDVSR: 8;
	uint32_t Reserved: 8;
	uint32_t Empty: 16;
} SSPCPSR_TypeDef;

typedef struct {
	uint32_t RORIM: 1;
	uint32_t RTIM: 1;
	uint32_t RXIM: 1;
	uint32_t TXIM: 1;
	uint32_t Reserved: 12;
	uint32_t Empty: 16;
} SSPIMSC_TypeDef;

typedef struct {
	uint32_t RORMIS: 1;
	uint32_t RTMIS: 1;
	uint32_t RXMIS: 1;
	uint32_t TXMIS: 1;
	uint32_t Reserved: 12;
	uint32_t Empty: 16;
} SSPMIS_TypeDef;

typedef struct {
	uint32_t SSP_MISO_DELAY: 4;
	uint32_t FILLEEVEL_RX: 3;
	uint32_t Reserved: 1;
	uint32_t FILLEEVEL_TX: 3;
	uint32_t Reserved1: 1;
	uint32_t B_ENDIAN: 1;
	uint32_t Reserved2: 2;
	uint32_t DMA_CS_EN: 1;
	uint32_t Reserved3: 16;
} SSPWRAP_TypeDef;

typedef struct {
	union {
		__IO uint32_t SSPCR0;
		__IO SSPCR0_TypeDef SSPCR0_S;
	} SSPCR0_U;
	union {
		__IO uint32_t SSPCR1;
		__IO SSPCR1_TypeDef SSPCR1_S;
	} SSPCR1_U;
	__IO uint32_t SSPDR;
	union {
		__IO uint32_t SSPSR;
		__IO SSPSR_TypeDef SSPSR_S;
	} SSPSR_U;
	union {
		__IO uint32_t SSPCPSR;
		__IO SSPCPSR_TypeDef SSPCPSR_S;
	} SSPCPSR_U;
	union {
		__IO uint32_t SSPIMSC;
		__IO SSPIMSC_TypeDef SSPIMSC_S;
	} SSPIMSC_U;
	__IO uint32_t SSPRIS;
	union {
		__IO uint32_t SSPMIS;
		__IO SSPIMSC_TypeDef SSPMIS_S;
	} SSPMIS_U;
	__IO uint32_t SSPICR;
	__IO uint32_t SSPDMACR;
	__IO SSPWRAP_TypeDef SSPWRAP_S;
} SSP_TypeDef;

#define SSP1 ((SSP_TypeDef *)SSP1_BASE)
#define SSP2 ((SSP_TypeDef *)SSP2_BASE)

#define SPI1 ((SSP_TypeDef *)SSP1_BASE)
#define SPI2 ((SSP_TypeDef *)SSP2_BASE)

typedef struct {
	__I uint32_t PageData: 13;
	__IO uint32_t PageIndex: 7;
	__IO uint32_t Reserved: 12;
} Cache_bound_Type;

#define CACHECTRL       (*(__IO uint32_t *)0x40037000)
#define CACHECTRL_Upper ((Cache_bound_Type *)0x40037004)
#define CACHECTRL_Lower ((Cache_bound_Type *)0x40037008)
#define CACHECOUNTBASE  (*(__IO uint32_t *)0x4003700c)
#define CACHECOUNTHIT   (*(__IO uint32_t *)0x40037010)

#define IOMUXPACTRL  (*(__IO uint32_t *)0x40030200)
#define IOMUXPACTRL2 (*(__IO uint32_t *)0x40030204)
#define IOMUXPBCTRL  (*(__IO uint32_t *)0x40030208)
#define IOMUXPBCTRL2 (*(__IO uint32_t *)0x4003020c)
#define IOANAENCTRL  (*(__IO uint32_t *)0x40030210)
#define IOPUPDPACTRL (*(__IO uint32_t *)0x40030214)
#define IOPUPDPBCTRL (*(__IO uint32_t *)0x40030218)
#define IOHDPACRTL   (*(__IO uint32_t *)0x4003021c)
#define IOHDPBCRTL   (*(__IO uint32_t *)0x40030220)
#define IOSMTPACTRL  (*(__IO uint32_t *)0x40030224)
#define IOSMTPBCTRL  (*(__IO uint32_t *)0x40030228)
#define IOODEPACTRL  (*(__IO uint32_t *)0x4003022c)
#define IOODEPBCTRL  (*(__IO uint32_t *)0x40030230)
#define IOOSEPACTRL  (*(__IO uint32_t *)0x40030234)
#define IOOSEPBCTRL  (*(__IO uint32_t *)0x40030238)

#define PDGPIOCTRL (*(__IO uint32_t *)0x40030240)

#define PDPAOE  (*(__IO uint32_t *)0x40030244)
#define PDPBOE  (*(__IO uint32_t *)0x40030248)
#define PDPAOUT (*(__IO uint32_t *)0x4003024c)
#define PDPBOUT (*(__IO uint32_t *)0x40030250)

#define PB8CLOCKRATE (*(__IO uint32_t *)0x40030254)
#define PB9CLOCKRATE (*(__IO uint32_t *)0x40030258)

#define IPGPIOADATA    (*(__IO uint32_t *)0x40020000)
#define IPGPIOADATAOUT (*(__IO uint32_t *)0x40020004)
#define IPGPIOBDATA    (*(__IO uint32_t *)0x40021000)
#define IPGPIOBDATAOUT (*(__IO uint32_t *)0x40021004)

#define IS_GPIO_MODE(MODE) (((MODE) == GPIO_Mode_IN) || ((MODE) == GPIO_Mode_OUT))

typedef enum {
	WDTINT = 0,
	WDTRESET = 1
} WDTMode;
#define BACKUPREG0 (*(__IO uint32_t *)(BACKUP_BASE + 0))
#define BACKUPREG1 (*(__IO uint32_t *)(BACKUP_BASE + 4))

typedef enum {
	IRC12 = 0x00,
	IRC16 = 0x01,
	IRC20 = 0x02,
	IRC24 = 0x03,
	IRC28 = 0x04,
	IRC32 = 0x05,
	XTAL24 = 0x11,
	XTAL12 = 0x13,
	External = 0x20
} FreqSource;

typedef struct {
	__IO uint32_t MIRC_Tall: 10;
	__IO uint32_t MIRC_TV12: 3;
	__IO uint32_t Reserved: 17;
} MIRCTRIM_Type2;
#define MIRC12M_R_2 ((MIRCTRIM_Type2 *)0x100a7f60)
#define MIRC16M_2   ((MIRCTRIM_Type2 *)0x100a6070)
#define MIRC20M_2   ((MIRCTRIM_Type2 *)0x100a6074)
#define MIRC24M_2   ((MIRCTRIM_Type2 *)0x100a6078)
#define MIRC28M_2   ((MIRCTRIM_Type2 *)0x100a607c)
#define MIRC32M_2   ((MIRCTRIM_Type2 *)0x100a6080)

typedef struct {
	__IO uint32_t CLKOUT0SEL: 4;
	__IO uint32_t CLKOUT1SEL: 4;
	__IO uint32_t CLKOUT0DIV: 7;
	__IO uint32_t CLKOUT1DIV: 7;
	__IO uint32_t Reserved: 10;
} TopTest_Type;
#define TOPTEST ((TopTest_Type *)0x40030300)

bool GPIO_ReadBit(GPIO_IPType *GPIOx, GPIOPinBitDef GPIOPin);
uint16_t GPIO_ReadPort(GPIO_IPType *GPIOx);
void GPIO_WritePort(GPIO_IPType *GPIOx, uint16_t PortVal);
void GPIOMUXSet(GPIOPortDef GPIOx, GPIOPinNameDef GPIOPin, GPIOMUXDef MUXNum);
void GPIO_WriteBit(GPIO_IPType *GPIOx, GPIOPinBitDef GPIOPin, BitAction BitVal);
void GPIO_ToggleBits(GPIO_IPType *GPIOx, GPIOPinBitDef GPIOPin);
void GPIO_Init(GPIO_IPType *GPIOx, GPIO_InitTypeDef *GPIO_InitStruct);
void EnableGPIOINT(GPIO_IPType *GPIOx, GPIOPinBitDef GPIOPin, GPIOINTDef TriggerType);
void DisableGPIOINT(GPIO_IPType *GPIOx, GPIOPinBitDef GPIOPin);

void GPIO_HighDrive_Enable(GPIO_IPType *GPIOx, GPIOPinBitDef GPIOPin);
void GPIO_HighDrive_Disable(GPIO_IPType *GPIOx, GPIOPinBitDef GPIOPin);
void GPIO_OpenDrain_Enable(GPIO_IPType *GPIOx, GPIOPinBitDef GPIOPin);
void GPIO_OpenDrain_Disable(GPIO_IPType *GPIOx, GPIOPinBitDef GPIOPin);
void GPIO_OpenSource_Enable(GPIO_IPType *GPIOx, GPIOPinBitDef GPIOPin);
void GPIO_OpenSource_Disable(GPIO_IPType *GPIOx, GPIOPinBitDef GPIOPin);

void GPIO_SetOuput2(GPIO_IPType *GPIOx, GPIOPinNameDef GPIOPin);
void GPIO_SetOutput(GPIO_IPType *GPIOx, GPIOPinNameDef GPIOPin, GPIOPuPdDef GPIOAttr);
void GPIO_SetInputFloat(GPIO_IPType *GPIOx, GPIOPinNameDef GPIOPin);
void GPIO_SetInput(GPIO_IPType *GPIOx, GPIOPinNameDef GPIOPin, GPIOPuPdDef GPIOAttr);

void CLKGatingEnable(CLKGatingSwitch GatingN);
void CLKGatingDisable(CLKGatingSwitch GatingN);
bool IsCLKGating(CLKGatingSwitch GatingN);
void SetCLKOut(int8_t ClkS, int8_t SourceSel, int16_t DIV);

void WatchDogEnable(WDTMode mode, uint32_t msec);

#endif
