/*
 * Copyright (c) 2025 ELAN Microelectronics Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_E967_H
#define ZEPHYR_DRIVERS_USB_UDC_E967_H

typedef struct {
	__IO uint32_t XTALHIRCSEL: 1;
	__IO uint32_t XTALLJIRCSEL: 1;
	__IO uint32_t HCLKSEL: 2;
	__IO uint32_t USBCLKSEL: 1;
	__IO uint32_t HCLKDIV: 3;
	__IO uint32_t QSPICLK_SEL: 1;
	__IO uint32_t ACC1CLK_SEL: 1;
	__IO uint32_t ENCRYPT_SEL: 1;
	__IO uint32_t Timer1_SEL: 1;
	__IO uint32_t Timer2_SEL: 1;
	__IO uint32_t Timer3_SEL: 1;
	__IO uint32_t Timer4_SEL: 1;
	__IO uint32_t QSPICLK_DIV: 1;
	__IO uint32_t ACC1CLK_DIV: 1;
	__IO uint32_t EncryptCLK_DIV: 1;
	__IO uint32_t RTC_SEL: 1;
	__IO uint32_t I2C1Reset_SEL: 1;
	__IO uint32_t USBReset_SEL: 1;
	__IO uint32_t HIRC_TESTV: 1;
	__IO uint32_t SWRESTN: 1;
	__IO uint32_t DEEPSLPCLKOFF: 1;
	__IO uint32_t ClearECCKey: 1;
	__IO uint32_t POWEN: 1;
	__IO uint32_t RESETOP: 1;
	__IO uint32_t PMUCTRL: 1;
	__IO uint32_t REAMPMODE: 4;
} E967_SysReg_Type;
#define E967_SYSREGCTRL ((E967_SysReg_Type *)0x40030000)

typedef struct {
	__IO uint32_t XTALFREQSEL: 2;
	__IO uint32_t XTALPD: 1;
	__IO uint32_t XTALHZ: 1;
	__I uint32_t XTALSTABLE: 1;
	__IO uint32_t XTALCounter: 2;
	__IO uint32_t Reserved: 25;
} E967_XTAL_Type;
#define E967_XTALCTRL ((E967_XTAL_Type *)0x40036200)

typedef struct {
	__IO uint32_t USBPLLPD: 1;
	__IO uint32_t USBPLLFASTLOCK: 1;
	__IO uint32_t USBPLLPSET: 3;
	__I uint32_t USBPLLSTABLECNT: 2;
	__I uint32_t USBPLLSTABLE: 1;
	__IO uint32_t Reserved: 24;
} E967_USBPLL_Type;
#define E967_USBPLLCTRL ((E967_USBPLL_Type *)0x40036400)

typedef struct {
	__IO uint32_t LJIRCPD: 1;
	__IO uint32_t LJIRCRCM: 2;
	__IO uint32_t LJIRCFR: 4;
	__IO uint32_t LJIRCCA: 5;
	__IO uint32_t LJIRCFC: 3;
	__IO uint32_t LJIRCTMV10: 2;
	__IO uint32_t LJIRCTESTV10B: 1;
	__IO uint32_t Reserved: 14;
} E967_LJIRC_Type;
#define E967_LJIRCCTRL ((E967_LJIRC_Type *)0x40036004)

typedef struct {
	__IO uint32_t PHYBUFNSEL: 2;
	__IO uint32_t PHYBUFPSEL: 2;
	__IO uint32_t PHYRTRIM: 4;
	__IO uint32_t USBPHYPDB: 1;
	__IO uint32_t USBPHYRESET: 1;
	__IO uint32_t USBPHYRSW: 1;
	__IO uint32_t Reserved: 21;
} E967_PHY_Type;
#define E967_PHYCTRL ((E967_PHY_Type *)0x40036700)

#define E967_USB_BASE (0x40038000)

#define EP1_InLen 64
#define EP2_InLen 64
#define EP3_InLen 64
#define EP4_InLen 64

typedef struct {
	__IO uint32_t UDCSOFTRST: 1;
	__IO uint32_t UDCRSTRDY: 1;
	__IO uint32_t USBSLPRESUME: 1;
	__IO uint32_t EP1EN: 1;
	__IO uint32_t EP2EN: 1;
	__IO uint32_t EP3EN: 1;
	__IO uint32_t EP4EN: 1;
	__IO uint32_t EP0ACKINTEN: 1;
	__IO uint32_t EP1ACKINTEN: 1;
	__IO uint32_t EP2ACKINTEN: 1;
	__IO uint32_t EP3ACKINTEN: 1;
	__IO uint32_t EP4ACKINTEN: 1;
	__IO uint32_t UDCRESPONSESEL: 1;
	__IO uint32_t UDCRESPONSEEN: 1;
	__IO uint32_t EP0OUTBUFNAKCLR: 1;
	__IO uint32_t UDCCTRLRESERVED: 16;
	__IO uint32_t UDCEN: 1;
} UDCCTRLBIT_TypeDef;

typedef struct {
	union {
		__IO uint32_t _UDC_CTRL;
		__IO UDCCTRLBIT_TypeDef UDC_CTRLBIT;
	} UDC_CTRL;
} UDCCTRL_;
#define UDCCTRL ((UDCCTRL_ *)E967_USB_BASE)

typedef struct {
	__IO uint32_t CONFIG_DATA: 8;
	__IO uint32_t EP_CONFIG_DONE: 1;
	__IO uint32_t EP_CONFIG_RDY: 1;
	__IO uint32_t UDCCFDATARESERVED: 22;
} UDCCFDATABIT_TypeDef;

typedef struct {
	union {
		__IO uint32_t UDCCF_DATA;
		__IO UDCCFDATABIT_TypeDef UDC_CF_DATABIT;
	} UDC_CFDATA;
} UDC_CF_DATA;
#define UDCCFDATA ((UDC_CF_DATA *)(E967_USB_BASE + 0x04))

typedef struct {
	__IO uint32_t RSTINTEN: 1;
	__IO uint32_t SUSPENDINTEN: 1;
	__IO uint32_t RESUMEINTEN: 1;
	__IO uint32_t EXTPCKGINTEN: 1;
	__IO uint32_t LPM_RESUNEINTEN: 1;
	__IO uint32_t SOFINTEN: 1;
	__IO uint32_t SE1DETINTEN: 1;
	__IO uint32_t ERRORINTEN: 1;
	__IO uint32_t UDCINTENRESERVED0: 8;
	__IO uint32_t CRCERRINTEN: 1;
	__IO uint32_t ALLCRCERRINTEN: 1;
	__IO uint32_t EP0REFILLINTEN: 1;
	__IO uint32_t UDCINTENRESERVED1: 13;
} UDCINTENBIT_TypeDef;

typedef struct {
	union {
		__IO uint32_t UDC_INTEN;
		__IO UDCINTENBIT_TypeDef UDC_INTENBIT;
	} UDCINT_EN;
} UDC_INT_EN;
#define UDCINTEN ((UDC_INT_EN *)(E967_USB_BASE + 0x08))

typedef struct {
	__IO uint32_t SETUPINTEN: 1;
	__IO uint32_t EP0ININTEN: 1;
	__IO uint32_t EP0OUTINTEN: 1;
	__IO uint32_t EP0DATREADY: 1;
	__IO uint32_t EP0BUFCLR: 1;
	__IO uint32_t UDCEP0INTENRESERVED: 27;
} UDCEP0INTENBIT_TypeDef;

typedef struct {
	union {
		__IO uint32_t UDC_EP0_INTEN;
		__IO UDCEP0INTENBIT_TypeDef UDC_EP0_INT_ENBIT;
	} UDCEP0INT_EN;
} UDC_EP0_INT_EN;
#define UDCEP0INTEN ((UDC_EP0_INT_EN *)(E967_USB_BASE + 0x0C))

typedef struct {
	__IO uint32_t EPxININTEN: 1;
	__IO uint32_t EPxOUTINTEN: 1;
	__IO uint32_t EPxINEMPTYINTEN: 1;
	__IO uint32_t EPxDATREADY: 1;
	__IO uint32_t EPxBUFCLR: 1;
	__IO uint32_t EPxACCESSLATCH: 1;
	__IO uint32_t EPxINTENRESERVED: 26;
} UDCEPxINTENBIT_TypeDef;

typedef struct {
	union {
		__IO uint32_t UDC_EPx_INTEN;
		__IO UDCEPxINTENBIT_TypeDef UDC_EPx_INT_ENBIT;
	} UDCEPx_INT_EN;
} UDC_EPx_INT_EN;

typedef struct {
	__IO uint32_t EP1ININTEN: 1;
	__IO uint32_t EP1OUTINTEN: 1;
	__IO uint32_t EP1INEMPTYINTEN: 1;
	__IO uint32_t EP1DATREADY: 1;
	__IO uint32_t EP1BUFCLR: 1;
	__IO uint32_t EP1ACCESSLATCH: 1;
	__IO uint32_t UdcEp1IntEn_None: 25;
	__IO uint32_t EPXARADREN: 1;
} UDCEP1INTENBIT_TypeDef;

typedef struct {
	union {
		__IO uint32_t UDC_EP1_INTEN;
		__IO UDCEP1INTENBIT_TypeDef UDC_EP1_INT_ENBIT;
	} UDCEP1INT_EN;
} UDC_EP1_INT_EN;
#define UDCEP1INTEN ((UDC_EP1_INT_EN *)(E967_USB_BASE + 0x10))

typedef struct {
	__IO uint32_t EP2ININTEN: 1;
	__IO uint32_t EP2OUTINTEN: 1;
	__IO uint32_t EP2INEMPTYINTEN: 1;
	__IO uint32_t EP2DATREADY: 1;
	__IO uint32_t EP2BUFCLR: 1;
	__IO uint32_t EP2ACCESSLATCH: 1;
	__IO uint32_t UDCEP2INTENRESERVED: 26;
} UDCEP2INTENBIT_TypeDef;

typedef struct {
	union {
		__IO uint32_t UDC_EP2_INTEN;
		__IO UDCEP2INTENBIT_TypeDef UDC_EP2_INT_ENBIT;
	} UDCEP2INT_EN;
} UDC_EP2_INT_EN;
#define UDCEP2INTEN ((UDC_EP2_INT_EN *)(E967_USB_BASE + 0x14))

typedef struct {
	__IO uint32_t EP3ININTEN: 1;
	__IO uint32_t EP3OUTINTEN: 1;
	__IO uint32_t EP3INEMPTYINTEN: 1;
	__IO uint32_t EP3DATREADY: 1;
	__IO uint32_t EP3BUFCLR: 1;
	__IO uint32_t EP3ACCESSLATCH: 1;
	__IO uint32_t UDCEP3INTENRESERVED: 26;
} UDCEP3INTENBIT_TypeDef;

typedef struct {
	union {
		__IO uint32_t UDC_EP3_INTEN;
		__IO UDCEP3INTENBIT_TypeDef UDC_EP3_INT_ENBIT;
	} UDCEP3INT_EN;
} UDC_EP3_INT_EN;
#define UDCEP3INTEN ((UDC_EP3_INT_EN *)(E967_USB_BASE + 0x18))

typedef struct {
	__IO uint32_t EP4ININTEN: 1;
	__IO uint32_t EP4OUTINTEN: 1;
	__IO uint32_t EP4INEMPTYINTEN: 1;
	__IO uint32_t EP4DATREADY: 1;
	__IO uint32_t EP4BUFCLR: 1;
	__IO uint32_t EP4ACCESSLATCH: 1;
	__IO uint32_t UDCEP4INTENRESERVED: 26;
} UDCEP4INTENBIT_TypeDef;

typedef struct {
	union {
		__IO uint32_t UDC_EP4_INTEN;
		__IO UDCEP4INTENBIT_TypeDef UDC_EP4_INT_ENBIT;
	} UDCEP4INT_EN;
} UDC_EP4_INT_EN;
#define UDCEP4INTEN ((UDC_EP4_INT_EN *)(E967_USB_BASE + 0x1C))

typedef struct {
	__IO uint32_t RSTINTSF: 1;
	__IO uint32_t SUSPENDINTSF: 1;
	__IO uint32_t RESUMEINTSF: 1;
	__IO uint32_t EXTPCKGINTSF: 1;
	__IO uint32_t LPMRESUNEINTSF: 1;
	__IO uint32_t SOFINTSF: 1;
	__IO uint32_t SE1DETINTSF: 1;
	__IO uint32_t ERRORINTSF: 1;
	__IO uint32_t RSTINTSFCLR: 1;
	__IO uint32_t SUSPENDINTSFCLR: 1;
	__IO uint32_t RESUMEINTSFCLR: 1;
	__IO uint32_t EXTPCKGINTSFCLR: 1;
	__IO uint32_t LPMRESUNEINTSFCLR: 1;
	__IO uint32_t SOFINTSFCLR: 1;
	__IO uint32_t SE1DETINTSFCLR: 1;
	__IO uint32_t ERRORINTSFCLR: 1;
	__IO uint32_t CRC_ERR_SF: 1;
	__IO uint32_t ALL_CRC_ERR_SF: 1;
	__IO uint32_t EP0_REFILL_SF: 1;
	__IO uint32_t USBWAKEUP_SF: 1;
	__IO uint32_t UDCINTSTARESERVED0: 4;
	__IO uint32_t CRC_ERR_SF_CLR: 1;
	__IO uint32_t ALL_CRC_ERR_SF_CLR: 1;
	__IO uint32_t EP0_REFILL_SF_CLR: 1;
	__IO uint32_t USBWAKEUP_SF_CLR: 1;
	__IO uint32_t UDCINTSTARESERVED1: 4;
} UDCINTSTABIT_TypeDef;

typedef struct {
	union {
		__IO uint32_t UDC_INTSTA;
		__IO UDCINTSTABIT_TypeDef UDC_INT_STABIT;
	} UDCINT_STA;
} UDC_INT_STA;
#define UDCINTSTA ((UDC_INT_STA *)(E967_USB_BASE + 0x20))

typedef struct {
	__IO uint32_t SETUPINTSF: 1;
	__IO uint32_t EP0ININTSF: 1;
	__IO uint32_t EP0OUTINTSF: 1;
	__IO uint32_t UDCEP0INTSTARESERVED0: 5;
	__IO uint32_t SETUPINTSFCLR: 1;
	__IO uint32_t EP0ININTSFCLR: 1;
	__IO uint32_t EP0OUTINTSFCLR: 1;
	__IO uint32_t UDCEP0INTSTARESERVED1: 21;
} UDCEP0INTSTABIT_TypeDef;

typedef struct {
	union {
		__IO uint32_t UDCEP0_INT_STA;
		__IO UDCEP0INTSTABIT_TypeDef UDC_EP0_INT_STABIT;
	} UDC_EP0_INTSTA;
} UDC_EP0_INT_STA;
#define UDCEP0INTSTA ((UDC_EP0_INT_STA *)(E967_USB_BASE + 0x24))

typedef struct {
	__IO uint32_t EPx_IN_INT_SF: 1;
	__IO uint32_t EPx_OUT_INT_SF: 1;
	__IO uint32_t EPx_IN_EMPTY_INT_SF: 1;
	__IO uint32_t UDCEPxINTSTARESERVED0: 5;
	__IO uint32_t EPx_IN_INT_SF_CLR: 1;
	__IO uint32_t EPx_OUT_INT_SF_CLR: 1;
	__IO uint32_t EPx_IN_EMPTY_INT_SF_CLR: 1;
	__IO uint32_t UDCEPxINTSTARESERVED1: 21;
} UDCEPxINTSTABIT_TypeDef;

typedef struct {
	union {
		__IO uint32_t UDCEPx_INT_STA;
		__IO UDCEPxINTSTABIT_TypeDef UDC_EPx_INT_STABIT;
	} UDC_EPx_INTSTA;
} UDC_EPx_INT_STA;

typedef struct {
	__IO uint32_t EP1_IN_INT_SF: 1;
	__IO uint32_t EP1_OUT_INT_SF: 1;
	__IO uint32_t EP1_IN_EMPTY_INT_SF: 1;
	__IO uint32_t UDCEP1INTSTARESERVED0: 5;
	__IO uint32_t EP1_IN_INT_SF_CLR: 1;
	__IO uint32_t EP1_OUT_INT_SF_CLR: 1;
	__IO uint32_t EP1_IN_EMPTY_INT_SF_CLR: 1;
	__IO uint32_t UDCEP1INTSTARESERVED1: 21;
} UDCEP1INTSTABIT_TypeDef;

typedef struct {
	union {
		__IO uint32_t UDCEP1_INT_STA;
		__IO UDCEP1INTSTABIT_TypeDef UDC_EP1_INT_STABIT;
	} UDC_EP1_INTSTA;
} UDC_EP1_INT_STA;
#define UDCEP1INTSTA ((UDC_EP1_INT_STA *)(E967_USB_BASE + 0x28))

typedef struct {
	__IO uint32_t EP2_IN_INT_SF: 1;
	__IO uint32_t EP2_OUT_INT_SF: 1;
	__IO uint32_t EP2_IN_EMPTY_INT_SF: 1;
	__IO uint32_t UDCEP2INTSTARESERVED0: 5;
	__IO uint32_t EP2_IN_INT_SF_CLR: 1;
	__IO uint32_t EP2_OUT_INT_SF_CLR: 1;
	__IO uint32_t EP2_IN_EMPTY_INT_SF_CLR: 1;
	__IO uint32_t UDCEP2INTSTARESERVED1: 21;
} UDCEP2INTSTABIT_TypeDef;

typedef struct {
	union {
		__IO uint32_t UDCEP2_INT_STA;
		__IO UDCEP2INTSTABIT_TypeDef UDC_EP2_INT_STABIT;
	} UDC_EP2_INTSTA;
} UDC_EP2_INT_STA;
#define UDCEP2INTSTA ((UDC_EP2_INT_STA *)(E967_USB_BASE + 0x2C))

typedef struct {
	__IO uint32_t EP3_IN_INT_SF: 1;
	__IO uint32_t EP3_OUT_INT_SF: 1;
	__IO uint32_t EP3_IN_EMPTY_INT_SF: 1;
	__IO uint32_t UDCEP3INTSTARESERVED0: 5;
	__IO uint32_t EP3_IN_INT_SF_CLR: 1;
	__IO uint32_t EP3_OUT_INT_SF_CLR: 1;
	__IO uint32_t EP3_IN_EMPTY_INT_SF_CLR: 1;
	__IO uint32_t UDCEP3INTSTARESERVED1: 21;
} UDCEP3INTSTABIT_TypeDef;

typedef struct {
	union {
		__IO uint32_t UDCEP3_INT_STA;
		__IO UDCEP3INTSTABIT_TypeDef UDC_EP3_INT_STABIT;
	} UDC_EP3_INTSTA;
} UDC_EP3_INT_STA;
#define UDCEP3INTSTA ((UDC_EP3_INT_STA *)(E967_USB_BASE + 0x30))

typedef struct {
	__IO uint32_t EP4_IN_INT_SF: 1;
	__IO uint32_t EP4_OUT_INT_SF: 1;
	__IO uint32_t EP4_IN_EMPTY_INT_SF: 1;
	__IO uint32_t UDCEP4INTSTARESERVED0: 5;
	__IO uint32_t EP4_IN_INT_SF_CLR: 1;
	__IO uint32_t EP4_OUT_INT_SF_CLR: 1;
	__IO uint32_t EP4_IN_EMPTY_INT_SF_CLR: 1;
	__IO uint32_t UDCEP4INTSTARESERVED1: 21;
} UDCEP4INTSTABIT_TypeDef;

typedef struct {
	union {
		__IO uint32_t UDCEP4_INT_STA;
		__IO UDCEP4INTSTABIT_TypeDef UDC_EP4_INT_STABIT;
	} UDC_EP4_INTSTA;
} UDC_EP4_INT_STA;
#define UDCEP4INTSTA ((UDC_EP4_INT_STA *)(E967_USB_BASE + 0x34))

#define EP0BUFDATA (*(__IO uint32_t *)(E967_USB_BASE + 0x38))
#define EP1BUFDATA (*(__IO uint32_t *)(E967_USB_BASE + 0x3C))
#define EP2BUFDATA (*(__IO uint32_t *)(E967_USB_BASE + 0x40))
#define EP3BUFDATA (*(__IO uint32_t *)(E967_USB_BASE + 0x44))
#define EP4BUFDATA (*(__IO uint32_t *)(E967_USB_BASE + 0x48))

typedef struct {
	__IO uint32_t EP0INBUFFULL: 1;
	__IO uint32_t EP0INBUFEMPTY: 1;
	__IO uint32_t EP1INBUFFULL: 1;
	__IO uint32_t EP1INBUFEMPTY: 1;
	__IO uint32_t EP2INBUFFULL: 1;
	__IO uint32_t EP2INBUFEMPTY: 1;
	__IO uint32_t EP3INBUFFULL: 1;
	__IO uint32_t EP3INBUFEMPTY: 1;
	__IO uint32_t EP4INBUFFULL: 1;
	__IO uint32_t EP4INBUFEMPTY: 1;
	__IO uint32_t EP0OUTBUFFULL: 1;
	__IO uint32_t EP0OUTBUFEMPTY: 1;
	__IO uint32_t EP1OUTBUFFULL: 1;
	__IO uint32_t EP1OUTBUFEMPTY: 1;
	__IO uint32_t EP2OUTBUFFULL: 1;
	__IO uint32_t EP2OUTBUFEMPTY: 1;
	__IO uint32_t EP3OUTBUFFULL: 1;
	__IO uint32_t EP3OUTBUFEMPTY: 1;
	__IO uint32_t EP4OUTBUFFULL: 1;
	__IO uint32_t EP4OUTBUFEMPTY: 1;
	__IO uint32_t EPBUFSTARESERVED: 12;
} EPBUFSTABIT_TypeDef;

typedef struct {
	union {
		__IO uint32_t EPBUF_STA;
		__IO EPBUFSTABIT_TypeDef UDC_EPX_INT_STABIT;
	} EP_BUFSTA;
} EP_BUF_STA;
#define EPBUFSTA ((EP_BUF_STA *)(E967_USB_BASE + 0x4C))

#define EP1DATINOUTCNT   (*(__IO uint32_t *)(E967_USB_BASE + 0x50))
#define EP2DATINOUTCNT   (*(__IO uint32_t *)(E967_USB_BASE + 0x54))
#define EP3DATINOUTCNT   (*(__IO uint32_t *)(E967_USB_BASE + 0x58))
#define EP4DATINOUTCNT   (*(__IO uint32_t *)(E967_USB_BASE + 0x5C))
#define E967_EPBUFDEPTH0 (*(__IO uint32_t *)(E967_USB_BASE + 0x60))
#define E967_EPBUFDEPTH1 (*(__IO uint32_t *)(E967_USB_BASE + 0x64))

typedef struct {
	__IO uint32_t SE1PULSEWIDTH: 3;
	__IO uint32_t Se1CtrlNone1: 1;
	__IO uint32_t SE1SIGNALCNT: 2;
	__IO uint32_t Se1CtrlNone2: 2;
	__IO uint32_t SE1FLAG: 1;
	__IO uint32_t DELAYNOISEFLAG: 1;
	__IO uint32_t SE1CTRLRESERVED: 22;
} SE1CTRLBIT_TypeDef;

typedef struct {
	union {
		__IO uint32_t _SE1_CTRL;
		__IO SE1CTRLBIT_TypeDef SE1_CTRLBIT;
	} SE1_CTRL_;
} SE1_CTRL;
#define SE1CTRL ((SE1_CTRL *)(E967_USB_BASE + 0x68))

typedef struct {
	__IO uint32_t PHYTESTSUSPENDEN: 1;
	__IO uint32_t PHYTESTOUTEN: 1;
	__IO uint32_t PHYTESTOUTSEL: 2;
	__IO uint32_t PHYTESTDMIN: 1;
	__IO uint32_t PHYTESTDPIN: 1;
	__IO uint32_t PHYTESTRESERVED: 14;
	__IO uint32_t UDCFIFOTESTMODEEN: 1;
	__IO uint32_t DGDTESTMODEFIBDEBUG: 5;
	__IO uint32_t DEVRESUMETIMESEL: 2;
	__IO uint32_t DEVRESUMESEL: 1;
	__IO uint32_t NEWPIDCLR: 1;
	__IO uint32_t USBWAKEUPEN: 1;
	__IO uint32_t PHYTESTEN: 1;
} PHYTESTBIT_TypeDef;

typedef struct {
	union {
		__IO uint32_t _PHY_TEST;
		__IO PHYTESTBIT_TypeDef PHY_TESTBIT;
	} PHY_TEST_;
} PHY_TEST;
#define PHYTEST ((PHY_TEST *)(E967_USB_BASE + 0x6C))

typedef struct {
	__IO uint32_t UDC_DMA_EN: 1;
	__IO uint32_t UDC_DMA_LEN: 16;
	__IO uint32_t UDC_DMA_ENPT: 2;
	__IO uint32_t UDC_DMA_TXEN: 1;
	__IO uint32_t UDC_DMA_RXEN: 1;
	__IO uint32_t UDC_DMA_DEST_MSIZE: 3;
	__IO uint32_t UDC_DMA_SRC_MSIZE: 3;
	__IO uint32_t UDCDMACTRLRESERVED3: 5;
} UDCDMACTRLBIT_TypeDef;

typedef struct {
	union {
		__IO uint32_t UDC_DMA_CTRL;
		__IO UDCDMACTRLBIT_TypeDef UDCDMACTRLBIT;
	} UDC_DMACTRL;
} UDCDMA_CTRL;
#define UDCDMACTRL ((UDCDMA_CTRL *)(E967_USB_BASE + 0x70))

typedef struct {
	__IO uint32_t UDC_REPLY_DATA: 1;
	__IO uint32_t DATA_STAGE_ACK: 1;
	__IO uint32_t ERR_FUNC_EN: 1;
	__IO uint32_t CURENDPOINT: 3;
	__IO uint32_t CURALTERNATE: 1;
	__IO uint32_t CURINTERFACE: 2;
	__IO uint32_t STALL: 1;
	__IO uint32_t DEVRESUME: 1;
	__IO uint32_t SUSPENDSTA: 1;
	__IO uint32_t EPOUTPREHOLD: 1;
	__IO uint32_t EP1STALL: 1;
	__IO uint32_t EP2STALL: 1;
	__IO uint32_t EP3STALL: 1;
	__IO uint32_t EP4STALL: 1;
	__IO uint32_t EPINPREHOLD: 1;
	__IO uint32_t UDCCTRL1RESERVED: 14;
} UDCCTRL1BIT_TypeDef;

typedef struct {
	union {
		__IO uint32_t _UDC_CTRL1;
		__IO UDCCTRL1BIT_TypeDef UDCCTRL1BIT;
	} UDC_CTRL1_;
} UDC_CTRL1;
#define UDCCTRL1 ((UDC_CTRL1 *)(E967_USB_BASE + 0x74))

typedef struct {
	__IO uint32_t SET_CONFIG: 1;
	__IO uint32_t CLR_FEATURE: 1;
	__IO uint32_t SET_FEATURE: 1;
	__IO uint32_t SET_ADDR: 1;
	__IO uint32_t STDCMDFLGRESERVED: 28;
} STDCMDFLGBIT_TypeDef;

typedef struct {
	union {
		__IO uint32_t STDCMD_FLG;
		__IO STDCMDFLGBIT_TypeDef STDCMDFLGBIT;
	} STD_CMDFLG;
} STD_CMD_FLG;
#define STDCMDFLG ((STD_CMD_FLG *)(E967_USB_BASE + 0x78))

#define TESTMODEEP0BUFDATA (*(__IO uint32_t *)(E967_USB_BASE + 0x7C))
#define TESTMODEEP1BUFDATA (*(__IO uint32_t *)(E967_USB_BASE + 0x80))
#define TESTMODEEP2BUFDATA (*(__IO uint32_t *)(E967_USB_BASE + 0x84))
#define TESTMODEEP3BUFDATA (*(__IO uint32_t *)(E967_USB_BASE + 0x88))
#define TESTMODEEP4BUFDATA (*(__IO uint32_t *)(E967_USB_BASE + 0x8C))

#define SE1_SIGNAL_CNT1  5
#define SE1_SIGNAL_CNT0  4
#define SE1_PULSE_WIDTH2 2
#define SE1_PULSE_WIDTH1 1
#define SE1_PULSE_WIDTH0 0

#define DEVRESUME_TIME1   9
#define DEVRESUME_TIME0   8
#define PHY_TEST_OUT_SEL3 7
#define PHY_TEST_OUT_SEL2 6
#define PHY_TEST_OUT_SEL1 5
#define PHY_TEST_OUT_SEL0 4
#define PHY_TERST_IN_SEL3 3
#define PHY_TERST_IN_SEL2 2
#define PHY_TERST_IN_SEL1 1
#define PHY_TERST_IN_SEL0 0

typedef struct {
	__IO uint32_t IRCSTACTRLRESERVED0: 12;
	__IO uint32_t EXSPECRANGE_EN: 1;
	__IO uint32_t MANUALTRIM: 1;
	__IO uint32_t IRCATTESTS_O: 1;
	__IO uint32_t SYSTEMHOLDDET: 1;
	__IO uint32_t VALIDRDTRIM_INFO: 1;
	__IO uint32_t FREQNONSTB: 1;
	__IO uint32_t EXTRIMSPEC: 1;
	__IO uint32_t FREQSTB: 1;
	__IO uint32_t IRCSTACTRLRESERVED1: 12;
} IRCSTACTRLBIT_TypeDef;

typedef struct {
	union {
		__IO uint32_t IRCSTA_CTRL;
		__IO IRCSTACTRLBIT_TypeDef IRC_STA_CTRLBIT;
	} IRC_STACTRL;
} IRC_STA_CTRL;
#define IRCSTACTRL ((IRC_STA_CTRL *)(E967_USB_BASE + 0x400))

#define RETRIMNTIME (*(__IO uint32_t *)(E967_USB_BASE + 0x404))

typedef struct {
	__IO uint32_t IRCFRVAL: 4;
	__IO uint32_t IRCCAVAL: 5;
	__IO uint32_t IRCFCVAL: 3;
	__IO uint32_t FCCAFRVALRESERVED: 19;
	__IO uint32_t LOADFCCAFR: 1;
} FCCAFRVALBIT_TypeDef;

typedef struct {
	union {
		__IO uint32_t FCCAFR_VAL;
		__IO FCCAFRVALBIT_TypeDef FC_CA_FR_VALBIT;
	} FC_CA_FRVAL;
} FC_CA_FR_VAL;
#define FCCAFRVAL ((FC_CA_FR_VAL *)(E967_USB_BASE + 0x408))

#define SOFCNT (*(__IO uint32_t *)(E967_USB_BASE + 0x40C))

typedef struct {
	__IO uint32_t LJIRCPD: 1;
	__IO uint32_t LJIRCRCM: 2;
	__IO uint32_t MANUALFR: 4;
	__IO uint32_t MANUALCA: 5;
	__IO uint32_t MANUALFC: 3;
	__IO uint32_t LJIRCTMV10: 2;
	__IO uint32_t LJIRCTESTV10B: 1;
	__IO uint32_t MANUALFCCAFRRESERVED1: 14;
} MANUALFCCAFRBIT_TypeDef;

typedef struct {
	union {
		__IO uint32_t MANUAL_FCCAFR;
		__IO MANUALFCCAFRBIT_TypeDef MANUAL_FC_CA_FRBIT;
	} MANUALFC_CA_FR;
} MANUAL_FC_CA_FR;
#define MANUALFCCAFR ((MANUAL_FC_CA_FR *)0x40036004)

#define EP0 0x00
#define EP1 0x01
#define EP2 0x02
#define EP3 0x03
#define EP4 0x04

#define RESET_INT_EN   0x01
#define SUSPEND_INT_EN 0x02
#define RESUME_INT_EN  0x04
#define EXTPCK_INT_EN  0x08
#define LPM_INT_EN     0x10
#define SOF_INT_EN     0x20
#define SE1_INT_EN     0x40
#define ERR_INT_EN     0x80

#define SETUP_INT_EN  0x01
#define EP0IN_INT_EN  0x02
#define EP0OUT_INT_EN 0x04

#define EP1IN_INT_EN      0x01
#define EP1OUT_INT_EN     0x02
#define EP1INEMPTY_INT_EN 0x04

#define EP2IN_INT_EN      0x01
#define EP2OUT_INT_EN     0x02
#define EP2INEMPTY_INT_EN 0x04

#define EP3IN_INT_EN      0x01
#define EP3OUT_INT_EN     0x02
#define EP3INEMPTY_INT_EN 0x04

#define EP4IN_INT_EN      0x01
#define EP4OUT_INT_EN     0x02
#define EP4INEMPTY_INT_EN 0x04

#define EP0_PACKET_SIZE 0x08
#define EP1_PACKET_SIZE 0x40
#define EP2_PACKET_SIZE 0x40
#define EP3_PACKET_SIZE 0x40
#define EP4_PACKET_SIZE 0x40

#define USB_EP_CONTROL 0x00
#define USB_EP_ISOC    0x01
#define USB_EP_BULK    0x02
#define USB_EP_INT     0x03

typedef enum {
	USB_IRC = 0,
	USB_XTAL_12M = 1,
	USB_XTAL_24M = 2,
} USBCLKSEL;

typedef enum {
	USBD_NULL,
	USBD_RESET,
	USBD_SUSPEND,
	USBD_RESUME,
	USBD_LPM,
	USBD_LPM_RSUME,
	USBD_SOF,
	USBD_SE1,
	USBD_EP0_IN,
	USBD_EP1_IN,
	USBD_EP2_IN,
	USBD_EP3_IN,
	USBD_EP4_IN,
	USBD_EP0_OUT,
	USBD_EP1_OUT,
	USBD_EP2_OUT,
	USBD_EP3_OUT,
	USBD_EP4_OUT,
	USBD_DMA_TR,
	USBD_DMA_RE,
} USBD_EPX_Status;

typedef enum {
	USBD_OK = 0,
	USBD_BUSY,
	USBD_FAIL,
} USBD_Status;

typedef enum {
	SE1_INIT = 0,
	SE1_NOT_STB,
	SE1_STB,
} SE1_Status;

typedef enum USB_IRQn {
	E967_USB_Setup_IRQn = 4,
	E967_USB_Suspend_IRQn = 5,
	E967_USB_Resume_IRQn = 6,
	E967_USB_Reset_IRQn = 7,
	E967_USB_EPx_In_EPx_Empty_IRQn = 8,
	E967_USB_EPx_Out_IRQn = 9,
	E967_USB_SOF_IRQn = 10,
	E967_USB_Error_SE1_IRQn = 11,
	E967_USB_LPM_RESUME_EXTPCKG_IRQn = 12,
} USB_IRQn_Type;

#endif
