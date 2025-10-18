#include "soc.h"

#include <zephyr/kernel.h>
#include <zephyr/init.h>

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(elan_em32f967_soc);

static void _z_nop_delay(uint32_t cnt)
{
	for (uint32_t i = 0; i < cnt; i++) {
		__asm volatile("nop");
	}
}

#if defined(CONFIG_SOC_PREP_HOOK)
void WatchDogEnable(WDTMode mode, uint32_t msec);
bool GPIO_ReadBit(GPIO_IPType *GPIOx, GPIOPinBitDef GPIOPin);
// test PB8,  if low, enter boot mode
void _test_boot_mode(void)
{
	bool bpb3;
	int cnt, boot;

	// first, set PB3 INPUT, PULL HIGH
	GPIO_SetInput(GPIOIPB, GPIO_PINSOURCE3, GPIO_PuPd_PullUp4_7K);
	// wait to make sure PB3 is high
	for (int i = 0; i < 50; i++) {
		__asm volatile("nop");
	}
	cnt = 0;
	boot = 0;
	do {
		bpb3 = GPIO_ReadBit(GPIOIPB, GPIO_PIN_3);
		if (!bpb3) {
			bpb3 = false;
			boot = 1;
			break;
		}
		for (int i = 0; i < 50; i++) {
			__asm volatile("nop");
		}

		cnt++;
	} while (cnt < 10);
	if (boot) {
		CLKGatingDisable(PCLKG_BKP);
		BACKUPREG0 = 0x56524553;
		BACKUPREG1 = 0x00000000;
		WatchDogEnable(WDTRESET, 10);
		while (1)
			; //(WDT timeout reset ?boot )
	}
	return;
}

void soc_prep_hook(void)
{
	_test_boot_mode();
}
#endif

void GPIO_HighDrive_Enable(GPIO_IPType *GPIOx, GPIOPinBitDef GPIOPin)
{
	if ((uint32_t)GPIOx == GPIOA_BASE) {
		IOHDPACRTL |= GPIOPin;
	} else if ((uint32_t)GPIOx == GPIOB_BASE) {
		IOHDPBCRTL |= GPIOPin;
	}
}

void GPIO_HighDrive_Disable(GPIO_IPType *GPIOx, GPIOPinBitDef GPIOPin)
{
	if ((uint32_t)GPIOx == GPIOA_BASE) {
		IOHDPACRTL &= ~GPIOPin;
	} else if ((uint32_t)GPIOx == GPIOB_BASE) {
		IOHDPBCRTL &= ~GPIOPin;
	}
}

void GPIO_OpenDrain_Enable(GPIO_IPType *GPIOx, GPIOPinBitDef GPIOPin)
{
	if ((uint32_t)GPIOx == GPIOA_BASE) {
		IOODEPACTRL |= GPIOPin;
	} else if ((uint32_t)GPIOx == GPIOB_BASE) {
		IOODEPBCTRL |= GPIOPin;
	}
}

void GPIO_OpenDrain_Disable(GPIO_IPType *GPIOx, GPIOPinBitDef GPIOPin)
{
	if ((uint32_t)GPIOx == GPIOA_BASE) {
		IOODEPACTRL &= ~GPIOPin;
	} else if ((uint32_t)GPIOx == GPIOB_BASE) {
		IOODEPBCTRL &= ~GPIOPin;
	}
}

void GPIO_OpenSource_Enable(GPIO_IPType *GPIOx, GPIOPinBitDef GPIOPin)
{
	if ((uint32_t)GPIOx == GPIOA_BASE) {
		IOOSEPACTRL |= GPIOPin;
	} else if ((uint32_t)GPIOx == GPIOB_BASE) {
		IOOSEPBCTRL |= GPIOPin;
	}
}

void GPIO_OpenSource_Disable(GPIO_IPType *GPIOx, GPIOPinBitDef GPIOPin)
{
	if ((uint32_t)GPIOx == GPIOA_BASE) {
		IOOSEPACTRL &= ~GPIOPin;
	} else if ((uint32_t)GPIOx == GPIOB_BASE) {
		IOOSEPBCTRL &= ~GPIOPin;
	}
}

void GPIO_ToggleBits(GPIO_IPType *GPIOx, GPIOPinBitDef GPIOPin)
{

	__disable_irq();
	if ((GPIOx->DATAOUT & GPIOPin) != (uint16_t)Bit_RESET) {
		GPIOx->DATAOUT &= ~GPIOPin;
	} else {
		GPIOx->DATAOUT |= GPIOPin;
	}
	__enable_irq();
}

void GPIO_WriteBit(GPIO_IPType *GPIOx, GPIOPinBitDef GPIOPin, BitAction BitVal)
{
	__disable_irq();
	if (BitVal != Bit_RESET) {
		GPIOx->DATAOUT |= GPIOPin;
	} else {
		GPIOx->DATAOUT &= ~GPIOPin;
	}
	__enable_irq();
}

bool GPIO_ReadBit(GPIO_IPType *GPIOx, GPIOPinBitDef GPIOPin)
{
	bool bitstatus;

	if ((GPIOx->DATA & GPIOPin) != (uint16_t)Bit_RESET) {
		bitstatus = (bool)Bit_SET;
	} else {
		bitstatus = (bool)Bit_RESET;
	}
	return bitstatus;
}

void GPIO_WritePort(GPIO_IPType *GPIOx, uint16_t PortVal)
{
	GPIOx->DATAOUT = PortVal;
}

uint16_t GPIO_ReadPort(GPIO_IPType *GPIOx)
{
	return ((uint16_t)GPIOx->DATA);
}

void GPIO_Init(GPIO_IPType *GPIOx, GPIO_InitTypeDef *GPIO_InitStruct)
{
	uint32_t value_temp, value_mask;

	// pin_t = GPIO_InitStruct->GPIO_Pin;

	// set IO mux = 0x00
	if ((uint32_t)GPIOx == GPIOA_BASE) {
		CLKGatingDisable(HCLKG_GPIOA);
		if ((GPIO_InitStruct->GPIO_Pin == GPIO_PINSOURCE11) ||
		    (GPIO_InitStruct->GPIO_Pin == GPIO_PINSOURCE12)) {
			GPIOMUXSet(PORTA, GPIO_InitStruct->GPIO_Pin, GPIO_MUX01);
		} else {
			GPIOMUXSet(PORTA, GPIO_InitStruct->GPIO_Pin, GPIO_MUX00);
		}
		GPIOMUXSet(PORTANALOG, GPIO_InitStruct->GPIO_Pin, GPIO_MUX00);
	} else if ((uint32_t)GPIOx == GPIOB_BASE) {
		CLKGatingDisable(HCLKG_GPIOB);
		GPIOMUXSet(PORTB, GPIO_InitStruct->GPIO_Pin, GPIO_MUX00);
	}

	// set IO mode

	value_mask = 0x01 << GPIO_InitStruct->GPIO_Pin;
	if (GPIO_InitStruct->GPIO_Mode == GPIO_Mode_IN) {
		GPIOx->DATAOUTCLR = value_mask;
	} else {
		GPIOx->DATAOUTSET = value_mask;
	}

	// set IO PUPD
	value_temp = (GPIO_InitStruct->GPIO_PuPd) << (GPIO_InitStruct->GPIO_Pin * 2);
	value_mask = 0x03 << (GPIO_InitStruct->GPIO_Pin * 2);
	if ((uint32_t)GPIOx == GPIOA_BASE) {
		IOPUPDPACTRL = (IOPUPDPACTRL & ~value_mask) | value_temp;
	} else if ((uint32_t)GPIOx == GPIOB_BASE) {
		IOPUPDPBCTRL = (IOPUPDPBCTRL & ~value_mask) | value_temp;
	}
}

// PBx IO mux Swtich 3bits : 000~111
// PB0[2:0],PB1[6:4],PB2[10:8],PB3[14:12],PB4[18:16],PB5[22:20],PB6[26:24],PB7[30:28]
// PBx IO mux Swtich 3bits : 000~111
// PB8[2:0],PB9[6:4],PB10[10:8],PB11[14:12],PB12[18:16],PB13[22:20],PB14[26:24],PB15[30:28]

void GPIOMUXSet(GPIOPortDef GPIOx, GPIOPinNameDef GPIOPin, GPIOMUXDef MUXNum)
{
	uint32_t MUXValue, MUXShift, GPIOPinR;

	MUXShift = 0x07;
	MUXValue = MUXNum;
	if (GPIOPin < 8) {
		GPIOPinR = GPIOPin;
	} else {
		GPIOPinR = GPIOPin - 8;
	}

	MUXValue <<= (GPIOPinR * 4);
	MUXShift <<= (GPIOPinR * 4);
	if (GPIOx == PORTA) {
		if (GPIOPin < 8) {
			IOMUXPACTRL = (IOMUXPACTRL & ~MUXShift) | MUXValue;
		} else {
			IOMUXPACTRL2 = (IOMUXPACTRL2 & ~MUXShift) | MUXValue;
		}
	} else if (GPIOx == PORTB) {
		if (GPIOPin < 8) {
			IOMUXPBCTRL = (IOMUXPBCTRL & ~MUXShift) | MUXValue;
		} else {
			IOMUXPBCTRL2 = (IOMUXPBCTRL2 & ~MUXShift) | MUXValue;
		}
	} else if (GPIOx == PORTANALOG) {
		MUXShift = 0x03;
		MUXValue = MUXNum;
		MUXValue <<= (GPIOPin * 2);
		MUXShift <<= (GPIOPin * 2);
		IOANAENCTRL = (IOANAENCTRL & ~MUXShift) | MUXValue;
	}
}

void GPIO_SetOuput2(GPIO_IPType *GPIOx, GPIOPinNameDef GPIOPin)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = GPIOPin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_Floating;

	GPIO_Init(GPIOx, &GPIO_InitStructure);
}

void GPIO_SetOutput(GPIO_IPType *GPIOx, GPIOPinNameDef GPIOPin, GPIOPuPdDef GPIOAttr)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = GPIOPin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_PuPd = GPIOAttr;

	GPIO_Init(GPIOx, &GPIO_InitStructure);
}

void GPIO_SetInput(GPIO_IPType *GPIOx, GPIOPinNameDef GPIOPin, GPIOPuPdDef GPIOAttr)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = GPIOPin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIOAttr;

	GPIO_Init(GPIOx, &GPIO_InitStructure);
}

void GPIO_SetInputFloat(GPIO_IPType *GPIOx, GPIOPinNameDef GPIOPin)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = GPIOPin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_Floating;

	GPIO_Init(GPIOx, &GPIO_InitStructure);
}

void GPIO_SetInputPullDown(GPIO_IPType *GPIOx, GPIOPinNameDef GPIOPin)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = GPIOPin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_Pulldown15K;

	GPIO_Init(GPIOx, &GPIO_InitStructure);
}

void GPIO_SetPB8Toggle(uint32_t us_31_25)
{
	PB8CLOCKRATE = us_31_25;
	PDGPIOCTRL |= 0x04; // PB8 CLK OUT enable
}

void GPIO_SetPB9Toggle(uint32_t us_31_25)
{
	PB9CLOCKRATE = us_31_25;
	PDGPIOCTRL |= 0x08; // PB9 CLK OUT enable
}

void GPIO_StopPB8Toggle(void)
{
	PDGPIOCTRL &= 0xfffffffb; // PB8 CLK OUT disable
}

void GPIO_StopPB9Toggle(void)
{
	PDGPIOCTRL &= 0xfffffff7; // PB9 CLK OUT disable
}

void GPIO_PD_SetOuput(GPIOPortDef Portx, GPIOPinBitDef GPIOPin, BitAction BitVal)
{
	if (Portx == PORTA) {
		PDPAOE |= GPIOPin;
		if (BitVal != Bit_RESET) {
			PDPAOUT |= GPIOPin;
		} else {
			PDPAOUT &= ~GPIOPin;
		}
		PDGPIOCTRL |= 0x01; // PA pd enable
	} else {
		PDPBOE |= GPIOPin;
		if (BitVal != Bit_RESET) {
			PDPBOUT |= GPIOPin;
		} else {
			PDPBOUT &= ~GPIOPin;
		}
		PDGPIOCTRL |= 0x02; // PB pd enable
	}
}

void GPIO_PD_SetInput(GPIOPortDef Portx, GPIOPinBitDef GPIOPin)
{
	if (Portx == PORTA) {
		PDPAOE &= ~GPIOPin;
		PDGPIOCTRL |= 0x01; // PA pd enable
	} else {
		PDPBOE &= ~GPIOPin;
		PDGPIOCTRL |= 0x02; // PB pd enable
	}
}

void GPIO_PD_Disable(GPIOPortDef Portx)
{
	if (Portx == PORTA) {
		PDGPIOCTRL &= ~0x01; // PA pd disable
	} else if (Portx == PORTB) {
		PDGPIOCTRL &= ~0x02; // PB pd disable
	}
}

void CLKGatingEnable(CLKGatingSwitch GatingN)
{
	if (GatingN == PCLKG_ALL) {
		CACHECTRL = 0x00; // disable cache
		CLKGATEREG = 0xffffffff;
		CLKGATEREG2 = 0xffffffff;
	} else if (GatingN <= 31) {
		CLKGATEREG |= 0x01 << GatingN;
	} else {
		CLKGATEREG2 |= 0x01 << GatingN;
	}
}

void CLKGatingDisable(CLKGatingSwitch GatingN)
{
	if (GatingN == PCLKG_ALL) {
		CLKGATEREG = 0;
		CLKGATEREG2 = 0;
	} else if (GatingN <= 31) {
		CLKGATEREG &= ~(0x01 << GatingN);
	} else {
		CLKGATEREG2 &= ~(0x01 << (GatingN - 32));
	}
}

bool IsCLKGating(CLKGatingSwitch GatingN)
{
	if (GatingN <= 31) {
		if (CLKGATEREG & (0x01 << GatingN)) {
			return 1;
		} else {
			return 0;
		}
	} else if (CLKGATEREG2 & (0x01 << (GatingN - 32))) {
		return 1;
	} else {
		return 0;
	}
}

void WatchDogEnable(WDTMode mode, uint32_t msec)
{
	CLKGatingDisable(PCLKG_DWG);
	WDOGLOCK = 0x1acce551;
	WDOGLOAD = 32 * msec; // use 32K CLK
	if (mode) {
		WDOGCONTROL = 0x03; // enable reset
	} else {
		WDOGCONTROL = 0x01; // just enable int
	}
	WDOGLOCK = 0;
}

void SetCLKOut(int8_t ClkS, int8_t SourceSel, int16_t DIV)
{
	if (ClkS) {
		TOPTEST->CLKOUT1DIV = DIV;
		TOPTEST->CLKOUT1SEL = SourceSel;
		GPIO_SetOuput2(GPIOIPA, GPIO_PINSOURCE15);
		GPIOMUXSet(PORTA, GPIO_PINSOURCE15, GPIO_MUX07);
	} else {
		TOPTEST->CLKOUT0DIV = DIV;
		TOPTEST->CLKOUT0SEL = SourceSel;
		GPIO_SetOuput2(GPIOIPA, GPIO_PINSOURCE14);
		GPIOMUXSet(PORTA, GPIO_PINSOURCE14, GPIO_MUX07);
	}
	_z_nop_delay(1);
}
