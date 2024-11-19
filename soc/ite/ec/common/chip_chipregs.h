/*
 * Copyright (c) 2020 ITE Corporation. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CHIP_CHIPREGS_H
#define CHIP_CHIPREGS_H

#include <zephyr/sys/util.h>

#define EC_REG_BASE_ADDR 0x00f00000

#ifdef _ASMLANGUAGE
#define ECREG(x)        x
#else

/*
 * Macros for hardware registers access.
 */
#define ECREG(x)		(*((volatile unsigned char *)(x)))
#define ECREG_u16(x)		(*((volatile unsigned short *)(x)))
#define ECREG_u32(x)		(*((volatile unsigned long  *)(x)))

/*
 * MASK operation macros
 */
#define SET_MASK(reg, bit_mask)			((reg) |= (bit_mask))
#define CLEAR_MASK(reg, bit_mask)		((reg) &= (~(bit_mask)))
#define IS_MASK_SET(reg, bit_mask)		(((reg) & (bit_mask)) != 0)
#endif /* _ASMLANGUAGE */

#ifndef REG_BASE_ADDR
#define REG_BASE_ADDR				EC_REG_BASE_ADDR
#endif

/* Common definition */
/*
 * EC clock frequency (PWM and tachometer driver need it to reply
 * to api or calculate RPM)
 */
#ifdef CONFIG_SOC_IT8XXX2_EC_BUS_24MHZ
#define EC_FREQ			MHZ(24)
#else
#define EC_FREQ			MHZ(8)

#endif

/* --- General Control (GCTRL) --- */
#define IT8XXX2_GCTRL_BASE      0x00F02000
#define IT8XXX2_GCTRL_EIDSR     ECREG(IT8XXX2_GCTRL_BASE + 0x31)
#define IT8XXX2_GCTRL_PMER3     ECREG(IT8XXX2_GCTRL_BASE + 0x46)
/* RISC-V JTAG Debug Interface Enable */
#define IT8XXX2_GCTRL_JTAGEN    BIT(1)
/* RISC-V JTAG Debug Interface Selection */
#define IT8XXX2_GCTRL_JTAGSEL   BIT(0)
#define IT8XXX2_GCTRL_JTAG      (IT8XXX2_GCTRL_JTAGEN | IT8XXX2_GCTRL_JTAGSEL)

#ifdef CONFIG_SOC_IT8XXX2_REG_SET_V2
#define IT8XXX2_JTAG_PINS_BASE  ECREG(0xF01660)
#define IT8XXX2_JTAG_VOLT_SET   ECREG(0xF01648)
#elif CONFIG_SOC_IT8XXX2_REG_SET_V1
#define IT8XXX2_JTAG_PINS_BASE  ECREG(0xF01610)
#define IT8XXX2_JTAG_VOLT_SET   ECREG(0xF016e9)
#endif

#ifdef CONFIG_SOC_IT8XXX2_REG_SET_V2
/* --- External GPIO Control (EGPIO) --- */
#define IT8XXX2_EGPIO_BASE      0x00F02100
#define IT8XXX2_EGPIO_EGCR      ECREG(IT8XXX2_EGPIO_BASE + 0x04)

/* EGPIO register fields */
/*
 * 0x04: External GPIO Control
 * BIT(4): EXGPIO EGAD Pin Output Driving Disable
 */
#define IT8XXX2_EGPIO_EEPODD    BIT(4)
#endif

/**
 *
 * (11xxh) Interrupt controller (INTC)
 *
 */
#ifdef CONFIG_SOC_IT8XXX2_REG_SET_V1
#define ISR0			ECREG(EC_REG_BASE_ADDR + 0x3F00)
#define ISR1			ECREG(EC_REG_BASE_ADDR + 0x3F01)
#define ISR2			ECREG(EC_REG_BASE_ADDR + 0x3F02)
#define ISR3			ECREG(EC_REG_BASE_ADDR + 0x3F03)
#define ISR4			ECREG(EC_REG_BASE_ADDR + 0x3F14)
#define ISR5			ECREG(EC_REG_BASE_ADDR + 0x3F18)
#define ISR6			ECREG(EC_REG_BASE_ADDR + 0x3F1C)
#define ISR7			ECREG(EC_REG_BASE_ADDR + 0x3F20)
#define ISR8			ECREG(EC_REG_BASE_ADDR + 0x3F24)
#define ISR9			ECREG(EC_REG_BASE_ADDR + 0x3F28)
#define ISR10			ECREG(EC_REG_BASE_ADDR + 0x3F2C)
#define ISR11			ECREG(EC_REG_BASE_ADDR + 0x3F30)
#define ISR12			ECREG(EC_REG_BASE_ADDR + 0x3F34)
#define ISR13			ECREG(EC_REG_BASE_ADDR + 0x3F38)
#define ISR14			ECREG(EC_REG_BASE_ADDR + 0x3F3C)
#define ISR15			ECREG(EC_REG_BASE_ADDR + 0x3F40)
#define ISR16			ECREG(EC_REG_BASE_ADDR + 0x3F44)
#define ISR17			ECREG(EC_REG_BASE_ADDR + 0x3F48)
#define ISR18			ECREG(EC_REG_BASE_ADDR + 0x3F4C)
#define ISR19			ECREG(EC_REG_BASE_ADDR + 0x3F50)
#define ISR20			ECREG(EC_REG_BASE_ADDR + 0x3F54)
#define ISR21			ECREG(EC_REG_BASE_ADDR + 0x3F58)
#define ISR22			ECREG(EC_REG_BASE_ADDR + 0x3F5C)
#define ISR23			ECREG(EC_REG_BASE_ADDR + 0x3F90)

#define IER0			ECREG(EC_REG_BASE_ADDR + 0x3F04)
#define IER1			ECREG(EC_REG_BASE_ADDR + 0x3F05)
#define IER2			ECREG(EC_REG_BASE_ADDR + 0x3F06)
#define IER3			ECREG(EC_REG_BASE_ADDR + 0x3F07)
#define IER4			ECREG(EC_REG_BASE_ADDR + 0x3F15)
#define IER5			ECREG(EC_REG_BASE_ADDR + 0x3F19)
#define IER6			ECREG(EC_REG_BASE_ADDR + 0x3F1D)
#define IER7			ECREG(EC_REG_BASE_ADDR + 0x3F21)
#define IER8			ECREG(EC_REG_BASE_ADDR + 0x3F25)
#define IER9			ECREG(EC_REG_BASE_ADDR + 0x3F29)
#define IER10			ECREG(EC_REG_BASE_ADDR + 0x3F2D)
#define IER11			ECREG(EC_REG_BASE_ADDR + 0x3F31)
#define IER12			ECREG(EC_REG_BASE_ADDR + 0x3F35)
#define IER13			ECREG(EC_REG_BASE_ADDR + 0x3F39)
#define IER14			ECREG(EC_REG_BASE_ADDR + 0x3F3D)
#define IER15			ECREG(EC_REG_BASE_ADDR + 0x3F41)
#define IER16			ECREG(EC_REG_BASE_ADDR + 0x3F45)
#define IER17			ECREG(EC_REG_BASE_ADDR + 0x3F49)
#define IER18			ECREG(EC_REG_BASE_ADDR + 0x3F4D)
#define IER19			ECREG(EC_REG_BASE_ADDR + 0x3F51)
#define IER20			ECREG(EC_REG_BASE_ADDR + 0x3F55)
#define IER21			ECREG(EC_REG_BASE_ADDR + 0x3F59)
#define IER22			ECREG(EC_REG_BASE_ADDR + 0x3F5D)
#define IER23			ECREG(EC_REG_BASE_ADDR + 0x3F91)

#define IELMR0			ECREG(EC_REG_BASE_ADDR + 0x3F08)
#define IELMR1			ECREG(EC_REG_BASE_ADDR + 0x3F09)
#define IELMR2			ECREG(EC_REG_BASE_ADDR + 0x3F0A)
#define IELMR3			ECREG(EC_REG_BASE_ADDR + 0x3F0B)
#define IELMR4			ECREG(EC_REG_BASE_ADDR + 0x3F16)
#define IELMR5			ECREG(EC_REG_BASE_ADDR + 0x3F1A)
#define IELMR6			ECREG(EC_REG_BASE_ADDR + 0x3F1E)
#define IELMR7			ECREG(EC_REG_BASE_ADDR + 0x3F22)
#define IELMR8			ECREG(EC_REG_BASE_ADDR + 0x3F26)
#define IELMR9			ECREG(EC_REG_BASE_ADDR + 0x3F2A)
#define IELMR10			ECREG(EC_REG_BASE_ADDR + 0x3F2E)
#define IELMR11			ECREG(EC_REG_BASE_ADDR + 0x3F32)
#define IELMR12			ECREG(EC_REG_BASE_ADDR + 0x3F36)
#define IELMR13			ECREG(EC_REG_BASE_ADDR + 0x3F3A)
#define IELMR14			ECREG(EC_REG_BASE_ADDR + 0x3F3E)
#define IELMR15			ECREG(EC_REG_BASE_ADDR + 0x3F42)
#define IELMR16			ECREG(EC_REG_BASE_ADDR + 0x3F46)
#define IELMR17			ECREG(EC_REG_BASE_ADDR + 0x3F4A)
#define IELMR18			ECREG(EC_REG_BASE_ADDR + 0x3F4E)
#define IELMR19			ECREG(EC_REG_BASE_ADDR + 0x3F52)
#define IELMR20			ECREG(EC_REG_BASE_ADDR + 0x3F56)
#define IELMR21			ECREG(EC_REG_BASE_ADDR + 0x3F5A)
#define IELMR22			ECREG(EC_REG_BASE_ADDR + 0x3F5E)
#define IELMR23			ECREG(EC_REG_BASE_ADDR + 0x3F92)

#define IPOLR0			ECREG(EC_REG_BASE_ADDR + 0x3F0C)
#define IPOLR1			ECREG(EC_REG_BASE_ADDR + 0x3F0D)
#define IPOLR2			ECREG(EC_REG_BASE_ADDR + 0x3F0E)
#define IPOLR3			ECREG(EC_REG_BASE_ADDR + 0x3F0F)
#define IPOLR4			ECREG(EC_REG_BASE_ADDR + 0x3F17)
#define IPOLR5			ECREG(EC_REG_BASE_ADDR + 0x3F1B)
#define IPOLR6			ECREG(EC_REG_BASE_ADDR + 0x3F1F)
#define IPOLR7			ECREG(EC_REG_BASE_ADDR + 0x3F23)
#define IPOLR8			ECREG(EC_REG_BASE_ADDR + 0x3F27)
#define IPOLR9			ECREG(EC_REG_BASE_ADDR + 0x3F2B)
#define IPOLR10			ECREG(EC_REG_BASE_ADDR + 0x3F2F)
#define IPOLR11			ECREG(EC_REG_BASE_ADDR + 0x3F33)
#define IPOLR12			ECREG(EC_REG_BASE_ADDR + 0x3F37)
#define IPOLR13			ECREG(EC_REG_BASE_ADDR + 0x3F3B)
#define IPOLR14			ECREG(EC_REG_BASE_ADDR + 0x3F3F)
#define IPOLR15			ECREG(EC_REG_BASE_ADDR + 0x3F43)
#define IPOLR16			ECREG(EC_REG_BASE_ADDR + 0x3F47)
#define IPOLR17			ECREG(EC_REG_BASE_ADDR + 0x3F4B)
#define IPOLR18			ECREG(EC_REG_BASE_ADDR + 0x3F4F)
#define IPOLR19			ECREG(EC_REG_BASE_ADDR + 0x3F53)
#define IPOLR20			ECREG(EC_REG_BASE_ADDR + 0x3F57)
#define IPOLR21			ECREG(EC_REG_BASE_ADDR + 0x3F5B)
#define IPOLR22			ECREG(EC_REG_BASE_ADDR + 0x3F5F)
#define IPOLR23			ECREG(EC_REG_BASE_ADDR + 0x3F93)
#endif
#define IVECT			ECREG(EC_REG_BASE_ADDR + 0x3F10)


/*
 * TODO: use pinctrl node instead of following register declarations
 *       to fix in tcpm\it83xx_pd.h.
 */
/* GPIO control register */
#ifdef CONFIG_SOC_IT8XXX2_REG_SET_V1
#define GPCRF4			ECREG(EC_REG_BASE_ADDR + 0x163C)
#define GPCRF5			ECREG(EC_REG_BASE_ADDR + 0x163D)
#define GPCRH1			ECREG(EC_REG_BASE_ADDR + 0x1649)
#define GPCRH2			ECREG(EC_REG_BASE_ADDR + 0x164A)
#elif CONFIG_SOC_IT8XXX2_REG_SET_V2
#define GPCRF4			ECREG(EC_REG_BASE_ADDR + 0x168C)
#define GPCRF5			ECREG(EC_REG_BASE_ADDR + 0x168D)
#define GPCRH1			ECREG(EC_REG_BASE_ADDR + 0x1699)
#define GPCRH2			ECREG(EC_REG_BASE_ADDR + 0x169A)
#endif

/*
 * IT8XXX2 register structure size/offset checking macro function to mitigate
 * the risk of unexpected compiling results.
 */
#define IT8XXX2_REG_SIZE_CHECK(reg_def, size) \
	BUILD_ASSERT(sizeof(struct reg_def) == size, \
		"Failed in size check of register structure!")
#define IT8XXX2_REG_OFFSET_CHECK(reg_def, member, offset) \
	BUILD_ASSERT(offsetof(struct reg_def, member) == offset, \
		"Failed in offset check of register structure member!")

/**
 *
 * (18xxh) PWM & SmartAuto Fan Control (PWM)
 *
 */
#ifndef __ASSEMBLER__
struct pwm_it8xxx2_regs {
	/* 0x000: Channel0 Clock Prescaler */
	volatile uint8_t C0CPRS;
	/* 0x001: Cycle Time0 */
	volatile uint8_t CTR;
	/* 0x002~0x00A: Reserved1 */
	volatile uint8_t Reserved1[9];
	/* 0x00B: Prescaler Clock Frequency Select */
	volatile uint8_t PCFSR;
	/* 0x00C~0x00F: Reserved2 */
	volatile uint8_t Reserved2[4];
	/* 0x010: Cycle Time1 MSB */
	volatile uint8_t CTR1M;
	/* 0x011~0x022: Reserved3 */
	volatile uint8_t Reserved3[18];
	/* 0x023: PWM Clock Control */
	volatile uint8_t ZTIER;
	/* 0x024~0x026: Reserved4 */
	volatile uint8_t Reserved4[3];
	/* 0x027: Channel4 Clock Prescaler */
	volatile uint8_t C4CPRS;
	/* 0x028: Channel4 Clock Prescaler MSB */
	volatile uint8_t C4MCPRS;
	/* 0x029~0x02A: Reserved5 */
	volatile uint8_t Reserved5[2];
	/* 0x02B: Channel6 Clock Prescaler */
	volatile uint8_t C6CPRS;
	/* 0x02C: Channel6 Clock Prescaler MSB */
	volatile uint8_t C6MCPRS;
	/* 0x02D: Channel7 Clock Prescaler */
	volatile uint8_t C7CPRS;
	/* 0x02E: Channel7 Clock Prescaler MSB */
	volatile uint8_t C7MCPRS;
	/* 0x02F~0x040: Reserved6 */
	volatile uint8_t reserved6[18];
	/* 0x041: Cycle Time1 */
	volatile uint8_t CTR1;
	/* 0x042: Cycle Time2 */
	volatile uint8_t CTR2;
	/* 0x043: Cycle Time3 */
	volatile uint8_t CTR3;
	/* 0x044~0x048: Reserved7 */
	volatile uint8_t reserved7[5];
	/* 0x049: PWM Output Open-Drain Enable */
	volatile uint8_t PWMODENR;
};
#endif /* !__ASSEMBLER__ */

/* PWM register fields */
/* 0x023: PWM Clock Control */
#define IT8XXX2_PWM_PCCE		BIT(1)
/* 0x048: Tachometer Switch Control */
#define IT8XXX2_PWM_T0DVS		BIT(3)
#define IT8XXX2_PWM_T0CHSEL		BIT(2)
#define IT8XXX2_PWM_T1DVS		BIT(1)
#define IT8XXX2_PWM_T1CHSEL		BIT(0)


/* --- Wake-Up Control (WUC) --- */
#ifdef CONFIG_SOC_IT8XXX2_REG_SET_V1
#define IT8XXX2_WUC_BASE   0x00F01B00

/* TODO: should a defined interface for configuring wake-up interrupts */
#define IT8XXX2_WUC_WUEMR1 (IT8XXX2_WUC_BASE + 0x00)
#define IT8XXX2_WUC_WUEMR5 (IT8XXX2_WUC_BASE + 0x0c)
#define IT8XXX2_WUC_WUESR1 (IT8XXX2_WUC_BASE + 0x04)
#define IT8XXX2_WUC_WUESR5 (IT8XXX2_WUC_BASE + 0x0d)
#define IT8XXX2_WUC_WUBEMR1 (IT8XXX2_WUC_BASE + 0x3c)
#define IT8XXX2_WUC_WUBEMR5 (IT8XXX2_WUC_BASE + 0x0f)
#endif

/**
 *
 * (1Dxxh) Keyboard Matrix Scan control (KSCAN)
 *
 */
#ifndef __ASSEMBLER__
struct kscan_it8xxx2_regs {
	/* 0x000: Keyboard Scan Out */
	volatile uint8_t KBS_KSOL;
	/* 0x001: Keyboard Scan Out */
	volatile uint8_t KBS_KSOH1;
	/* 0x002: Keyboard Scan Out Control */
	volatile uint8_t KBS_KSOCTRL;
	/* 0x003: Keyboard Scan Out */
	volatile uint8_t KBS_KSOH2;
	/* 0x004: Keyboard Scan In */
	volatile uint8_t KBS_KSI;
	/* 0x005: Keyboard Scan In Control */
	volatile uint8_t KBS_KSICTRL;
	/* 0x006: Keyboard Scan In [7:0] GPIO Control */
	volatile uint8_t KBS_KSIGCTRL;
	/* 0x007: Keyboard Scan In [7:0] GPIO Output Enable */
	volatile uint8_t KBS_KSIGOEN;
	/* 0x008: Keyboard Scan In [7:0] GPIO Data */
	volatile uint8_t KBS_KSIGDAT;
	/* 0x009: Keyboard Scan In [7:0] GPIO Data Mirror */
	volatile uint8_t KBS_KSIGDMRR;
	/* 0x00A: Keyboard Scan Out [15:8] GPIO Control */
	volatile uint8_t KBS_KSOHGCTRL;
	/* 0x00B: Keyboard Scan Out [15:8] GPIO Output Enable */
	volatile uint8_t KBS_KSOHGOEN;
	/* 0x00C: Keyboard Scan Out [15:8] GPIO Data Mirror */
	volatile uint8_t KBS_KSOHGDMRR;
	/* 0x00D: Keyboard Scan Out [7:0] GPIO Control */
	volatile uint8_t KBS_KSOLGCTRL;
	/* 0x00E: Keyboard Scan Out [7:0] GPIO Output Enable */
	volatile uint8_t KBS_KSOLGOEN;
};
#endif /* !__ASSEMBLER__ */

/* KBS register fields */
/* 0x002: Keyboard Scan Out Control */
#define IT8XXX2_KBS_KSOPU	BIT(2)
#define IT8XXX2_KBS_KSOOD	BIT(0)
/* 0x005: Keyboard Scan In Control */
#define IT8XXX2_KBS_KSIPU	BIT(2)
/* 0x00D: Keyboard Scan Out [7:0] GPIO Control */
#define IT8XXX2_KBS_KSO2GCTRL	BIT(2)
/* 0x00E: Keyboard Scan Out [7:0] GPIO Output Enable */
#define IT8XXX2_KBS_KSO2GOEN	BIT(2)


/**
 *
 * (1Fxxh) External Timer & External Watchdog (ETWD)
 *
 */
#define WDT_IT8XXX2_REGS_BASE \
	((struct wdt_it8xxx2_regs *)DT_REG_ADDR(DT_NODELABEL(twd0)))

#ifndef __ASSEMBLER__
struct wdt_it8xxx2_regs {
	/* 0x000: Reserved1 */
	volatile uint8_t reserved1;
	/* 0x001: External Timer1/WDT Configuration */
	volatile uint8_t ETWCFG;
	/* 0x002: External Timer1 Prescaler */
	volatile uint8_t ET1PSR;
	/* 0x003: External Timer1 Counter High Byte */
	volatile uint8_t ET1CNTLHR;
	/* 0x004: External Timer1 Counter Low Byte */
	volatile uint8_t ET1CNTLLR;
	/* 0x005: External Timer1/WDT Control */
	volatile uint8_t ETWCTRL;
	/* 0x006: External WDT Counter Low Byte */
	volatile uint8_t EWDCNTLR;
	/* 0x007: External WDT Key */
	volatile uint8_t EWDKEYR;
	/* 0x008: Reserved2 */
	volatile uint8_t reserved2;
	/* 0x009: External WDT Counter High Byte */
	volatile uint8_t EWDCNTHR;
	/* 0x00A: External Timer2 Prescaler */
	volatile uint8_t ET2PSR;
	/* 0x00B: External Timer2 Counter High Byte */
	volatile uint8_t ET2CNTLHR;
	/* 0x00C: External Timer2 Counter Low Byte */
	volatile uint8_t ET2CNTLLR;
	/* 0x00D: Reserved3 */
	volatile uint8_t reserved3;
	/* 0x00E: External Timer2 Counter High Byte2 */
	volatile uint8_t ET2CNTLH2R;
};
#endif /* !__ASSEMBLER__ */

/* WDT register fields */
/* 0x001: External Timer1/WDT Configuration */
#define IT8XXX2_WDT_EWDKEYEN		BIT(5)
#define IT8XXX2_WDT_EWDSRC		BIT(4)
#define IT8XXX2_WDT_LEWDCNTL		BIT(3)
#define IT8XXX2_WDT_LET1CNTL		BIT(2)
#define IT8XXX2_WDT_LET1PS		BIT(1)
#define IT8XXX2_WDT_LETWCFG		BIT(0)
/* 0x002: External Timer1 Prescaler */
#define IT8XXX2_WDT_ETPS_32P768_KHZ	0x00
#define IT8XXX2_WDT_ETPS_1P024_KHZ	0x01
#define IT8XXX2_WDT_ETPS_32_HZ		0x02
/* 0x005: External Timer1/WDT Control */
#define IT8XXX2_WDT_EWDSCEN		BIT(5)
#define IT8XXX2_WDT_EWDSCMS		BIT(4)
#define IT8XXX2_WDT_ET2TC		BIT(3)
#define IT8XXX2_WDT_ET2RST		BIT(2)
#define IT8XXX2_WDT_ET1TC		BIT(1)
#define IT8XXX2_WDT_ET1RST		BIT(0)

/* External Timer register fields */
/* External Timer 3~8 control */
#define IT8XXX2_EXT_ETX_COMB_RST_EN	(IT8XXX2_EXT_ETXCOMB | \
					 IT8XXX2_EXT_ETXRST | \
					 IT8XXX2_EXT_ETXEN)
#define IT8XXX2_EXT_ETXCOMB		BIT(3)
#define IT8XXX2_EXT_ETXRST		BIT(1)
#define IT8XXX2_EXT_ETXEN		BIT(0)

/* Control external timer3~8 */
#define IT8XXX2_EXT_TIMER_BASE  DT_REG_ADDR(DT_NODELABEL(timer))  /*0x00F01F10*/
#define IT8XXX2_EXT_CTRLX(n)    ECREG(IT8XXX2_EXT_TIMER_BASE + (n << 3))
#define IT8XXX2_EXT_PSRX(n)     ECREG(IT8XXX2_EXT_TIMER_BASE + 0x01 + (n << 3))
#define IT8XXX2_EXT_CNTX(n)     ECREG_u32(IT8XXX2_EXT_TIMER_BASE + 0x04 + \
					(n << 3))
#define IT8XXX2_EXT_CNTOX(n)    ECREG_u32(IT8XXX2_EXT_TIMER_BASE + 0x38 + \
					(n << 2))

/* Free run timer configurations */
#define FREE_RUN_TIMER          EXT_TIMER_4
#define FREE_RUN_TIMER_IRQ      DT_IRQ_BY_IDX(DT_NODELABEL(timer), 1, irq)
/* Free run timer configurations */
#define FREE_RUN_TIMER_FLAG     DT_IRQ_BY_IDX(DT_NODELABEL(timer), 1, flags)
/* Free run timer max count is 36.4 hr (base on clock source 32768Hz) */
#define FREE_RUN_TIMER_MAX_CNT  0xFFFFFFFFUL

#ifndef __ASSEMBLER__
enum ext_clk_src_sel {
	EXT_PSR_32P768K = 0,
	EXT_PSR_1P024K,
	EXT_PSR_32,
	EXT_PSR_EC_CLK,
};
/*
 * 24-bit timers: external timer 3, 5, and 7
 * 32-bit timers: external timer 4, 6, and 8
 */
enum ext_timer_idx {
	EXT_TIMER_3 = 0,	/* Event timer */
	EXT_TIMER_4,		/* Free run timer */
	EXT_TIMER_5,		/* Busy wait low timer */
	EXT_TIMER_6,		/* Busy wait high timer */
	EXT_TIMER_7,
	EXT_TIMER_8,
};
#endif


/*
 *
 * (2Cxxh) Platform Environment Control Interface (PECI)
 *
 */
#ifndef __ASSEMBLER__
struct peci_it8xxx2_regs {
	/* 0x00: Host Status */
	volatile uint8_t HOSTAR;
	/* 0x01: Host Control */
	volatile uint8_t HOCTLR;
	/* 0x02: Host Command */
	volatile uint8_t HOCMDR;
	/* 0x03: Host Target Address */
	volatile uint8_t HOTRADDR;
	/* 0x04: Host Write Length */
	volatile uint8_t HOWRLR;
	/* 0x05: Host Read Length */
	volatile uint8_t HORDLR;
	/* 0x06: Host Write Data */
	volatile uint8_t HOWRDR;
	/* 0x07: Host Read Data */
	volatile uint8_t HORDDR;
	/* 0x08: Host Control 2 */
	volatile uint8_t HOCTL2R;
	/* 0x09: Received Write FCS value */
	volatile uint8_t RWFCSV;
	/* 0x0A: Received Read FCS value */
	volatile uint8_t RRFCSV;
	/* 0x0B: Write FCS Value */
	volatile uint8_t WFCSV;
	/* 0x0C: Read FCS Value */
	volatile uint8_t RFCSV;
	/* 0x0D: Assured Write FCS Value */
	volatile uint8_t AWFCSV;
	/* 0x0E: Pad Control */
	volatile uint8_t PADCTLR;
};
#endif /* !__ASSEMBLER__ */

/**
 *
 * (2Fxxh) USB Device Controller (USBDC) Registers
 *
 */
#define EP_EXT_REGS_9X        1
#define EP_EXT_REGS_BX        2
#define EP_EXT_REGS_DX        3

#ifndef __ASSEMBLER__

/* EP0 to EP15 Enumeration */
enum usb_dc_endpoints {
	EP0,
	EP1,
	EP2,
	EP3,
	EP4,
	EP5,
	EP6,
	EP7,
	EP8,
	EP9,
	EP10,
	EP11,
	EP12,
	EP13,
	EP14,
	EP15,
	MAX_NUM_ENDPOINTS
};

union ep_ctrl_reg {
	volatile uint8_t value;
	struct {
		volatile uint8_t enable_bit: 1;
		volatile uint8_t ready_bit: 1;
		volatile uint8_t outdata_sequence_bit: 1;
		volatile uint8_t send_stall_bit: 1;
		volatile uint8_t iso_enable_bit: 1;
		volatile uint8_t direction_bit: 1;
		volatile uint8_t reserved: 2;
	} __packed fields;
} __packed;

struct it82xx2_usb_ep_regs {
	union ep_ctrl_reg ep_ctrl;
	volatile uint8_t ep_status;
	volatile uint8_t ep_transtype_sts;
	volatile uint8_t ep_nak_transtype_sts;
};

/* Reserved EP Extended Registers */
struct ep_ext_regs_7x {
	/* 0x75 Reserved */
	volatile uint8_t ep_ext_ctrl_75;
	/* 0x76 Reserved */
	volatile uint8_t ep_ext_ctrl_76;
	/* 0x77 Reserved */
	volatile uint8_t ep_ext_ctrl_77;
	/* 0x78 Reserved */
	volatile uint8_t ep_ext_ctrl_78;
	/* 0x79 Reserved */
	volatile uint8_t ep_ext_ctrl_79;
	/* 0x7A Reserved */
	volatile uint8_t ep_ext_ctrl_7a;
	/* 0x7B Reserved */
	volatile uint8_t ep_ext_ctrl_7b;
	/* 0x7C Reserved */
	volatile uint8_t ep_ext_ctrl_7c;
	/* 0x7D Reserved */
	volatile uint8_t ep_ext_ctrl_7d;
	/* 0x7E Reserved */
	volatile uint8_t ep_ext_ctrl_7e;
	/* 0x7F Reserved */
	volatile uint8_t ep_ext_ctrl_7f;
};

/* From 98h to 9Dh, the EP45/67/89/1011/1213/1415 Extended Control Registers
 * are defined, and their bits definitions are as follows:
 *
 * Bit    Description
 *  7     Reserved
 *  6     EPPOINT5_ISO_ENABLE
 *  5     EPPOINT5_SEND_STALL
 *  4     EPPOINT5_OUT_DATA_SEQUENCE
 *  3     Reserved
 *  2     EPPOINT4_ISO_ENABLE
 *  1     EPPOINT4_SEND_STALL
 *  0     EPPOINT4_OUT_DATA_SEQUENCE
 *
 * Apparently, we can tell that the EP4 and EP5 share the same register, and
 * the EP6 and EP7 share the same one, and the rest EPs are defined in the
 * same way.
 */
union epn0n1_extend_ctrl_reg {
	volatile uint8_t value;
	struct {
		volatile uint8_t epn0_outdata_sequence_bit: 1;
		volatile uint8_t epn0_send_stall_bit: 1;
		volatile uint8_t epn0_iso_enable_bit: 1;
		volatile uint8_t reserved0: 1;
		volatile uint8_t epn1_outdata_sequence_bit: 1;
		volatile uint8_t epn1_send_stall_bit: 1;
		volatile uint8_t epn1_iso_enable_bit: 1;
		volatile uint8_t reserved1: 1;
	} __packed fields;
} __packed;

struct ep_ext_regs_9x {
	/* 0x95 Reserved */
	volatile uint8_t ep_ext_ctrl_95;
	/* 0x96 Reserved */
	volatile uint8_t ep_ext_ctrl_96;
	/* 0x97 Reserved */
	volatile uint8_t ep_ext_ctrl_97;
	/* 0x98 ~ 0x9D EP45/67/89/1011/1213/1415 Extended Control Registers */
	union epn0n1_extend_ctrl_reg epn0n1_ext_ctrl[6];
	/* 0x9E Reserved */
	volatile uint8_t ep_ext_ctrl_9e;
	/* 0x9F Reserved */
	volatile uint8_t ep_ext_ctrl_9f;
};

/* From BXh to BDh are EP FIFO 1-3 Control 0/1 Registers, and their
 * definitions as follows:
 * B8h: EP_FIFO1_CONTROL0_REG
 * B9h: EP_FIFO1_CONTROL1_REG
 * BAh: EP_FIFO2_CONTROL0_REG
 * BBh: EP_FIFO2_CONTROL1_REG
 * BCh: EP_FIFO3_CONTROL0_REG
 * BDh: EP_FIFO3_CONTROL1_REG
 *
 * For each one, its bits definitions are as follows:
 * (take EP_FIFO1_CONTROL1_REG as example, which controls from EP8 to EP15)
 *
 * Bit  Description
 *
 *  7   EP15 select FIFO1 as data buffer
 *  6   EP14 select FIFO1 as data buffer
 *  5   EP13 select FIFO1 as data buffer
 *  4   EP12 select FIFO1 as data buffer
 *  3   EP11 select FIFO1 as data buffer
 *  2   EP10 select FIFO1 as data buffer
 *  1   EP9 select FIFO1 as data buffer
 *  0   EP8 select FIFO1 as data buffer
 *
 *  1: Select
 *  0: Not select
 */
struct ep_ext_regs_bx {
	/* 0xB5 Reserved */
	volatile uint8_t ep_ext_ctrl_b5;
	/* 0xB6 Reserved */
	volatile uint8_t ep_ext_ctrl_b6;
	/* 0xB7 Reserved */
	volatile uint8_t ep_ext_ctrl_b7;
	/* 0xB8 ~ 0xBD EP FIFO 1-3 Control 0/1 Registers */
	volatile uint8_t ep_fifo_ctrl[6];
	/* 0xBE Reserved */
	volatile uint8_t ep_ext_ctrl_be;
	/* 0xBF Reserved */
	volatile uint8_t ep_ext_ctrl_bf;
};


/* From D6h to DDh are EP Extended Control Registers, and their
 * definitions as follows:
 * D6h: EP0_EXT_CTRL1
 * D7h: EP0_EXT_CTRL2
 * D8h: EP1_EXT_CTRL1
 * D9h: EP1_EXT_CTRL2
 * DAh: EP2_EXT_CTRL1
 * DBh: EP2_EXT_CTRL2
 * DCh: EP3_EXT_CTRL1
 * DDh: EP3_EXT_CTRL2
 *
 * We classify them into 4 groups which each of them contains Control 1 and 2
 * according to the EP number as follows:
 */
union epn_extend_ctrl1_reg {
	volatile uint8_t value;
	struct {
		volatile uint8_t epn0_enable_bit: 1;
		volatile uint8_t epn0_direction_bit: 1;
		volatile uint8_t epn3_enable_bit: 1;
		volatile uint8_t epn3_direction_bit: 1;
		volatile uint8_t epn6_enable_bit: 1;
		volatile uint8_t epn6_direction_bit: 1;
		volatile uint8_t epn9_enable_bit: 1;
		volatile uint8_t epn9_direction_bit: 1;
	} __packed fields;
} __packed;

struct epn_ext_ctrl_regs {
	/* 0xD6/0xD8/0xDA/0xDC EPN Extended Control1 Register */
	union epn_extend_ctrl1_reg epn_ext_ctrl1;
	/* 0xD7/0xD9/0xDB/0xDD EPB Extended Control2 Register */
	volatile uint8_t epn_ext_ctrl2;
};

struct ep_ext_regs_dx {
	/* 0xD5 Reserved */
	volatile uint8_t ep_ext_ctrl_d5;
	/* 0xD6 ~ 0xDD EPN Extended Control 1/2 Registers */
	struct epn_ext_ctrl_regs epn_ext_ctrl[4];
	/* 0xDE Reserved */
	volatile uint8_t ep_ext_ctrl_de;
	/* 0xDF Reserved */
	volatile uint8_t ep_ext_ctrl_df;
};


/* The USB EPx FIFO Registers Definitions
 * EP0: 60h ~ 74h
 * EP1: 80h ~ 94h
 * EP2: A0h ~ B4h
 * EP3: C0h ~ D4h (D6h to DDh will be defined as marcos for usage)
 */
struct it82xx2_usb_ep_fifo_regs {
	/* 0x60 + ep * 0x20 : EP RX FIFO Data Register  */
	volatile uint8_t ep_rx_fifo_data;
	/* 0x61 + ep * 0x20 : EP RX FIFO DMA Count Register */
	volatile uint8_t ep_rx_fifo_dma_count;
	/* 0x62 + ep * 0x20 : EP RX FIFO Data Count MSB */
	volatile uint8_t ep_rx_fifo_dcnt_msb;
	/* 0x63 + ep * 0x20 : EP RX FIFO Data Count LSB */
	volatile uint8_t ep_rx_fifo_dcnt_lsb;
	/* 0x64 + ep * 0x20  : EP RX FIFO Control Register */
	volatile uint8_t ep_rx_fifo_ctrl;
	/* (0x65 ~ 0x6F) + ep * 0x20 */
	volatile uint8_t reserved_65_6f_add_20[11];
	/* 0x70 + ep * 0x20 : EP TX FIFO Data Register  */
	volatile uint8_t ep_tx_fifo_data;
	/* (0x71 ~ 0x73) + ep * 0x20 */
	volatile uint8_t reserved_71_73_add_20[3];
	/* 0x74 + ep * 0x20 : EP TX FIFO Control Register */
	volatile uint8_t ep_tx_fifo_ctrl;
	/* (0x75 ~ 0x7F) + ep * 0x20 */
	union {
		struct ep_ext_regs_7x ep_res;
		struct ep_ext_regs_9x ext_4_15;
		struct ep_ext_regs_bx fifo_ctrl;
		struct ep_ext_regs_dx ext_0_3;
	};

};

/* USB Control registers */
#define USB_IT82XX2_REGS_BASE \
	((struct usb_it82xx2_regs *)DT_REG_ADDR(DT_NODELABEL(usb0)))

/* Bit definitions of the register Port0/Port1 MISC Control: 0XE4/0xE8 */
#define PULL_DOWN_EN	BIT(4)

struct usb_it82xx2_regs {
	/* 0x00:  Host TX Contrl Register */
	volatile uint8_t host_tx_ctrl;
	/* 0x01:  Host TX Transaction Type Register */
	volatile uint8_t host_tx_trans_type;
	/* 0x02:  Host TX Line Control Register */
	volatile uint8_t host_tx_line_ctrl;
	/* 0x03:  Host TX SOF Enable Register */
	volatile uint8_t host_tx_sof_enable;
	/* 0x04:  Host TX Address Register */
	volatile uint8_t host_tx_addr;
	/* 0x05:  Host TX EP Number Register */
	volatile uint8_t host_tx_endp;
	/* 0x06:  Host Frame Number MSP Register */
	volatile uint8_t host_frame_num_msp;
	/* 0x07:  Host Frame Number LSP Register */
	volatile uint8_t host_frame_num_lsp;
	/* 0x08:  Host Interrupt Status Register */
	volatile uint8_t host_interrupt_status;
	/* 0x09:  Host Interrupt Mask Register */
	volatile uint8_t host_interrupt_mask;
	/* 0x0A:  Host RX Status Register */
	volatile uint8_t host_rx_status;
	/* 0x0B:  Host RX PID Register */
	volatile uint8_t host_rx_pid;
	/* 0x0C:  MISC Control Register */
	volatile uint8_t misc_control;
	/* 0x0D:  MISC Status Register */
	volatile uint8_t misc_status;
	/* 0x0E:  Host RX Connect State Register */
	volatile uint8_t host_rx_connect_state;
	/* 0x0F:  Host SOF Timer MSB Register */
	volatile uint8_t host_sof_timer_msb;
	/* 0x10 ~ 0x1F:  Reserved Registers 10h - 1Fh */
	volatile uint8_t reserved_10_1f[16];
	/* 0x20:  Host RX FIFO Data Port Register */
	volatile uint8_t host_rx_fifo_data;
	/* 0x21:  Host RX FIFO DMA Input Data Count Register */
	volatile uint8_t host_rx_fifo_dma_data_count;
	/* 0x22:  Host TX FIFO Data Count MSB Register */
	volatile uint8_t host_rx_fifo_data_count_msb;
	/* 0x23:  Host TX FIFO Data Count LSB Register */
	volatile uint8_t host_rx_fifo_data_count_lsb;
	/* 0x24:  Host RX FIFO Data Port Register */
	volatile uint8_t host_rx_fifo_control;
	/* 0x25 ~ 0x2F:  Reserved Registers 25h - 2Fh */
	volatile uint8_t reserved_25_2f[11];
	/* 0x30:  Host TX FIFO Data Port Register */
	volatile uint8_t host_tx_fifo_data;
	/* 0x31 ~ 0x3F:  Reserved Registers 31h - 3Fh */
	volatile uint8_t reserved_31_3f[15];
	/* 0x40 ~ 0x4F: Endpoint Registers 40h - 4Fh */
	struct it82xx2_usb_ep_regs usb_ep_regs[4];
	/* 0x50:  Device Controller Control Register */
	volatile uint8_t dc_control;
	/* 0x51:  Device Controller LINE Status Register */
	volatile uint8_t dc_line_status;
	/* 0x52:  Device Controller Interrupt Status Register */
	volatile uint8_t dc_interrupt_status;
	/* 0x53:  Device Controller Interrupt Mask Register */
	volatile uint8_t dc_interrupt_mask;
	/* 0x54:  Device Controller Address Register */
	volatile uint8_t dc_address;
	/* 0x55:  Device Controller Frame Number MSP Register */
	volatile uint8_t dc_frame_num_msp;
	/* 0x56:  Device Controller Frame Number LSP Register */
	volatile uint8_t dc_frame_num_lsp;
	/* 0x57 ~ 0x5F:  Reserved Registers 57h - 5Fh */
	volatile uint8_t reserved_57_5f[9];
	/* 0x60 ~ 0xDF: EP FIFO Registers 60h - DFh */
	struct it82xx2_usb_ep_fifo_regs fifo_regs[4];
	/* 0xE0:  Host/Device Control Register */
	volatile uint8_t host_device_control;
	/* 0xE1 ~ 0xE3:  Reserved Registers E1h - E3h */
	volatile uint8_t reserved_e1_e3[3];
	/* 0xE4:  Port0 MISC Control Register */
	volatile uint8_t port0_misc_control;
	/* 0xE5 ~ 0xE7:  Reserved Registers E5h - E7h */
	volatile uint8_t reserved_e5_e7[3];
	/* 0xE8:  Port1 MISC Control Register */
	volatile uint8_t port1_misc_control;
};
#endif /* #ifndef __ASSEMBLER__ */

/**
 *
 * (37xxh, 38xxh) USBPD Controller
 *
 */
#ifndef __ASSEMBLER__
struct usbpd_it8xxx2_regs {
	/* 0x000~0x003: Reserved1 */
	volatile uint8_t Reserved1[4];
	/* 0x004: CC General Configuration */
	volatile uint8_t CCGCR;
	/* 0x005: CC Channel Setting */
	volatile uint8_t CCCSR;
	/* 0x006: CC Pad Setting */
	volatile uint8_t CCPSR;
};
#endif /* !__ASSEMBLER__ */

/* USBPD controller register fields */
/* 0x004: CC General Configuration */
#define IT8XXX2_USBPD_DISABLE_CC			BIT(7)
#define IT8XXX2_USBPD_DISABLE_CC_VOL_DETECTOR		BIT(6)
#define IT8XXX2_USBPD_CC_SELECT_RP_RESERVED		(BIT(3) | BIT(2) | BIT(1))
#define IT8XXX2_USBPD_CC_SELECT_RP_DEF			(BIT(3) | BIT(2))
#define IT8XXX2_USBPD_CC_SELECT_RP_1A5			BIT(3)
#define IT8XXX2_USBPD_CC_SELECT_RP_3A0			BIT(2)
#define IT8XXX2_USBPD_CC1_CC2_SELECTION			BIT(0)
/* 0x005: CC Channel Setting */
#define IT8XXX2_USBPD_CC2_DISCONNECT			BIT(7)
#define IT8XXX2_USBPD_CC2_DISCONNECT_5_1K_TO_GND	BIT(6)
#define IT8XXX2_USBPD_CC1_DISCONNECT			BIT(3)
#define IT8XXX2_USBPD_CC1_DISCONNECT_5_1K_TO_GND	BIT(2)
#define IT8XXX2_USBPD_CC1_CC2_RP_RD_SELECT		(BIT(1) | BIT(5))
/* 0x006: CC Pad Setting */
#define IT8XXX2_USBPD_DISCONNECT_5_1K_CC2_DB		BIT(6)
#define IT8XXX2_USBPD_DISCONNECT_POWER_CC2		BIT(5)
#define IT8XXX2_USBPD_DISCONNECT_5_1K_CC1_DB		BIT(2)
#define IT8XXX2_USBPD_DISCONNECT_POWER_CC1		BIT(1)


/**
 *
 * (10xxh) Shared Memory Flash Interface Bridge (SMFI) registers
 *
 */
#ifndef __ASSEMBLER__
struct smfi_it8xxx2_regs {
	volatile uint8_t reserved1[59];
	/* 0x3B: EC-Indirect memory address 0 */
	volatile uint8_t SMFI_ECINDAR0;
	/* 0x3C: EC-Indirect memory address 1 */
	volatile uint8_t SMFI_ECINDAR1;
	/* 0x3D: EC-Indirect memory address 2 */
	volatile uint8_t SMFI_ECINDAR2;
	/* 0x3E: EC-Indirect memory address 3 */
	volatile uint8_t SMFI_ECINDAR3;
	/* 0x3F: EC-Indirect memory data */
	volatile uint8_t SMFI_ECINDDR;
	/* 0x40: Scratch SRAM 0 address low byte */
	volatile uint8_t SMFI_SCAR0L;
	/* 0x41: Scratch SRAM 0 address middle byte */
	volatile uint8_t SMFI_SCAR0M;
	/* 0x42: Scratch SRAM 0 address high byte */
	volatile uint8_t SMFI_SCAR0H;
	volatile uint8_t reserved1_1[23];
	/* 0x5A: Host RAM Window Control */
	volatile uint8_t SMFI_HRAMWC;
	/* 0x5B: Host RAM Window 0 Base Address [11:4] */
	volatile uint8_t SMFI_HRAMW0BA;
	/* 0x5C: Host RAM Window 1 Base Address [11:4] */
	volatile uint8_t SMFI_HRAMW1BA;
	/* 0x5D: Host RAM Window 0 Access Allow Size */
	volatile uint8_t SMFI_HRAMW0AAS;
	/* 0x5E: Host RAM Window 1 Access Allow Size */
	volatile uint8_t SMFI_HRAMW1AAS;
	volatile uint8_t reserved2[67];
	/* 0xA2: Flash control 6 */
	volatile uint8_t SMFI_FLHCTRL6R;
	volatile uint8_t reserved3[46];
};
#endif /* !__ASSEMBLER__ */

/* SMFI register fields */
/* EC-Indirect read internal flash */
#define EC_INDIRECT_READ_INTERNAL_FLASH BIT(6)
/* Enable EC-indirect page program command */
#define IT8XXX2_SMFI_MASK_ECINDPP BIT(3)
/* Scratch SRAM 0 address(BIT(19)) */
#define IT8XXX2_SMFI_SC0A19 BIT(7)
/* Scratch SRAM enable */
#define IT8XXX2_SMFI_SCAR0H_ENABLE BIT(3)

/* H2RAM Path Select. 1b: H2RAM through LPC IO cycle. */
#define SMFI_H2RAMPS           BIT(4)
/* H2RAM Window 1 Enable */
#define SMFI_H2RAMW1E          BIT(1)
/* H2RAM Window 0 Enable */
#define SMFI_H2RAMW0E          BIT(0)

/* Host RAM Window x Write Protect Enable (All protected) */
#define SMFI_HRAMWXWPE_ALL     (BIT(5) | BIT(4))


/**
 *
 * (16xxh) General Purpose I/O Port (GPIO) registers
 *
 */
#define GPIO_IT8XXX2_REG_BASE \
	((struct gpio_it8xxx2_regs *)DT_REG_ADDR(DT_NODELABEL(gpiogcr)))

#ifndef __ASSEMBLER__
#ifdef CONFIG_SOC_IT8XXX2_REG_SET_V1
struct gpio_it8xxx2_regs {
	/* 0x00: General Control */
	volatile uint8_t GPIO_GCR;
	/* 0x01-D0: Reserved1 */
	volatile uint8_t reserved1[208];
	/* 0xD1: General Control 25 */
	volatile uint8_t GPIO_GCR25;
	/* 0xD2: General Control 26 */
	volatile uint8_t GPIO_GCR26;
	/* 0xD3: General Control 27 */
	volatile uint8_t GPIO_GCR27;
	/* 0xD4: General Control 28 */
	volatile uint8_t GPIO_GCR28;
	/* 0xD5: General Control 31 */
	volatile uint8_t GPIO_GCR31;
	/* 0xD6: General Control 32 */
	volatile uint8_t GPIO_GCR32;
	/* 0xD7: General Control 33 */
	volatile uint8_t GPIO_GCR33;
	/* 0xD8-0xDF: Reserved2 */
	volatile uint8_t reserved2[8];
	/* 0xE0: General Control 16 */
	volatile uint8_t GPIO_GCR16;
	/* 0xE1: General Control 17 */
	volatile uint8_t GPIO_GCR17;
	/* 0xE2: General Control 18 */
	volatile uint8_t GPIO_GCR18;
	/* 0xE3: Reserved3 */
	volatile uint8_t reserved3;
	/* 0xE4: General Control 19 */
	volatile uint8_t GPIO_GCR19;
	/* 0xE5: General Control 20 */
	volatile uint8_t GPIO_GCR20;
	/* 0xE6: General Control 21 */
	volatile uint8_t GPIO_GCR21;
	/* 0xE7: General Control 22 */
	volatile uint8_t GPIO_GCR22;
	/* 0xE8: General Control 23 */
	volatile uint8_t GPIO_GCR23;
	/* 0xE9: General Control 24 */
	volatile uint8_t GPIO_GCR24;
	/* 0xEA-0xEC: Reserved4 */
	volatile uint8_t reserved4[3];
	/* 0xED: General Control 30 */
	volatile uint8_t GPIO_GCR30;
	/* 0xEE: General Control 29 */
	volatile uint8_t GPIO_GCR29;
	/* 0xEF: Reserved5 */
	volatile uint8_t reserved5;
	/* 0xF0: General Control 1 */
	volatile uint8_t GPIO_GCR1;
	/* 0xF1: General Control 2 */
	volatile uint8_t GPIO_GCR2;
	/* 0xF2: General Control 3 */
	volatile uint8_t GPIO_GCR3;
	/* 0xF3: General Control 4 */
	volatile uint8_t GPIO_GCR4;
	/* 0xF4: General Control 5 */
	volatile uint8_t GPIO_GCR5;
	/* 0xF5: General Control 6 */
	volatile uint8_t GPIO_GCR6;
	/* 0xF6: General Control 7 */
	volatile uint8_t GPIO_GCR7;
	/* 0xF7: General Control 8 */
	volatile uint8_t GPIO_GCR8;
	/* 0xF8: General Control 9 */
	volatile uint8_t GPIO_GCR9;
	/* 0xF9: General Control 10 */
	volatile uint8_t GPIO_GCR10;
	/* 0xFA: General Control 11 */
	volatile uint8_t GPIO_GCR11;
	/* 0xFB: General Control 12 */
	volatile uint8_t GPIO_GCR12;
	/* 0xFC: General Control 13 */
	volatile uint8_t GPIO_GCR13;
	/* 0xFD: General Control 14 */
	volatile uint8_t GPIO_GCR14;
	/* 0xFE: General Control 15 */
	volatile uint8_t GPIO_GCR15;
	/* 0xFF: Power Good Watch Control */
	volatile uint8_t GPIO_PGWCR;
};
#elif CONFIG_SOC_IT8XXX2_REG_SET_V2
struct gpio_it8xxx2_regs {
	/* 0x00: General Control */
	volatile uint8_t GPIO_GCR;
	/* 0x01-0x0F: Reserved1 */
	volatile uint8_t reserved1[15];
	/* 0x10: General Control 1 */
	volatile uint8_t GPIO_GCR1;
	/* 0x11: General Control 2 */
	volatile uint8_t GPIO_GCR2;
	/* 0x12: General Control 3 */
	volatile uint8_t GPIO_GCR3;
	/* 0x13: General Control 4 */
	volatile uint8_t GPIO_GCR4;
	/* 0x14: General Control 5 */
	volatile uint8_t GPIO_GCR5;
	/* 0x15: General Control 6 */
	volatile uint8_t GPIO_GCR6;
	/* 0x16: General Control 7 */
	volatile uint8_t GPIO_GCR7;
	/* 0x17: General Control 8 */
	volatile uint8_t GPIO_GCR8;
	/* 0x18: General Control 9 */
	volatile uint8_t GPIO_GCR9;
	/* 0x19: General Control 10 */
	volatile uint8_t GPIO_GCR10;
	/* 0x1A: General Control 11 */
	volatile uint8_t GPIO_GCR11;
	/* 0x1B: General Control 12 */
	volatile uint8_t GPIO_GCR12;
	/* 0x1C: General Control 13 */
	volatile uint8_t GPIO_GCR13;
	/* 0x1D: General Control 14 */
	volatile uint8_t GPIO_GCR14;
	/* 0x1E: General Control 15 */
	volatile uint8_t GPIO_GCR15;
	/* 0x1F: Power Good Watch Control */
	volatile uint8_t GPIO_PGWCR;
	/* 0x20: General Control 16 */
	volatile uint8_t GPIO_GCR16;
	/* 0x21: General Control 17 */
	volatile uint8_t GPIO_GCR17;
	/* 0x22: General Control 18 */
	volatile uint8_t GPIO_GCR18;
	/* 0x23: Reserved2 */
	volatile uint8_t reserved2;
	/* 0x24: General Control 19 */
	volatile uint8_t GPIO_GCR19;
	/* 0x25: Reserved3 */
	volatile uint8_t reserved3;
	/* 0x26: General Control 21 */
	volatile uint8_t GPIO_GCR21;
	/* 0x27-0x28: Reserved4 */
	volatile uint8_t reserved4[2];
	/* 0x29: General Control 24 */
	volatile uint8_t GPIO_GCR24;
	/* 0x2A-0x2C: Reserved5 */
	volatile uint8_t reserved5[3];
	/* 0x2D: General Control 30 */
	volatile uint8_t GPIO_GCR30;
	/* 0x2E: General Control 29 */
	volatile uint8_t GPIO_GCR29;
};

/* GPIO register fields */
/* 0x16: General Control 7 */
#define IT8XXX2_GPIO_SMB2PS                BIT(7)
#define IT8XXX2_GPIO_SMB3PS                BIT(6)
#define IT8XXX2_GPIO_SMB5PS                BIT(5)

#endif
#endif /* !__ASSEMBLER__ */

/* GPIO register fields */
/* 0x00: General Control */
#define IT8XXX2_GPIO_LPCRSTEN              (BIT(2) | BIT(1))
#define IT8XXX2_GPIO_GCR_ESPI_RST_D2       0x2
#define IT8XXX2_GPIO_GCR_ESPI_RST_POS      1
#define IT8XXX2_GPIO_GCR_ESPI_RST_EN_MASK  (0x3 << IT8XXX2_GPIO_GCR_ESPI_RST_POS)
/* 0xF0: General Control 1 */
#define IT8XXX2_GPIO_U2CTRL_SIN1_SOUT1_EN  BIT(2)
#define IT8XXX2_GPIO_U1CTRL_SIN0_SOUT0_EN  BIT(0)
/* 0xE6: General Control 21 */
#define IT8XXX2_GPIO_GPH1VS                BIT(1)
#define IT8XXX2_GPIO_GPH2VS                BIT(0)

#define KSIX_KSOX_KBS_GPIO_MODE     BIT(7)
#define KSIX_KSOX_GPIO_OUTPUT       BIT(6)
#define KSIX_KSOX_GPIO_PULLUP       BIT(2)
#define KSIX_KSOX_GPIO_PULLDOWN     BIT(1)

#define GPCR_PORT_PIN_MODE_INPUT    BIT(7)
#define GPCR_PORT_PIN_MODE_OUTPUT   BIT(6)
#define GPCR_PORT_PIN_MODE_PULLUP   BIT(2)
#define GPCR_PORT_PIN_MODE_PULLDOWN BIT(1)

/*
 * If both PULLUP and PULLDOWN are set to 1b, the corresponding port would be
 * configured as tri-state.
 */
#define GPCR_PORT_PIN_MODE_TRISTATE	(GPCR_PORT_PIN_MODE_INPUT |  \
					 GPCR_PORT_PIN_MODE_PULLUP | \
					 GPCR_PORT_PIN_MODE_PULLDOWN)

/* --- GPIO --- */
#ifdef CONFIG_SOC_IT8XXX2_REG_SET_V1
#define IT8XXX2_GPIO_BASE  0x00F01600

#define IT8XXX2_GPIO_GCRX(offset) ECREG(IT8XXX2_GPIO_BASE + (offset))
#define IT8XXX2_GPIO_GCR25_OFFSET 0xd1
#define IT8XXX2_GPIO_GCR26_OFFSET 0xd2
#define IT8XXX2_GPIO_GCR27_OFFSET 0xd3
#define IT8XXX2_GPIO_GCR28_OFFSET 0xd4
#define IT8XXX2_GPIO_GCR31_OFFSET 0xd5
#define IT8XXX2_GPIO_GCR32_OFFSET 0xd6
#define IT8XXX2_GPIO_GCR33_OFFSET 0xd7
#define IT8XXX2_GPIO_GCR19_OFFSET 0xe4
#define IT8XXX2_GPIO_GCR20_OFFSET 0xe5
#define IT8XXX2_GPIO_GCR21_OFFSET 0xe6
#define IT8XXX2_GPIO_GCR22_OFFSET 0xe7
#define IT8XXX2_GPIO_GCR23_OFFSET 0xe8
#define IT8XXX2_GPIO_GCR24_OFFSET 0xe9
#define IT8XXX2_GPIO_GCR30_OFFSET 0xed
#define IT8XXX2_GPIO_GCR29_OFFSET 0xee
#endif

/*
 * TODO: use pinctrl node instead of following register declarations
 *       to fix in tcpm\it83xx_pd.h.
 */
#define IT8XXX2_GPIO2_BASE 0x00F03E00

#define IT8XXX2_GPIO_GPCRP0     ECREG(IT8XXX2_GPIO2_BASE + 0x18)
#define IT8XXX2_GPIO_GPCRP1     ECREG(IT8XXX2_GPIO2_BASE + 0x19)


/**
 *
 * (19xxh) Analog to Digital Converter (ADC) registers
 *
 */
#ifndef __ASSEMBLER__

/* Data structure to define ADC channel 13-16 control registers. */
struct adc_vchs_ctrl_t {
	/* 0x60: Voltage Channel Control */
	volatile uint8_t VCHCTL;
	/* 0x61: Voltage Channel Data Buffer MSB */
	volatile uint8_t VCHDATM;
	/* 0x62: Voltage Channel Data Buffer LSB */
	volatile uint8_t VCHDATL;
};

struct adc_it8xxx2_regs {
	/* 0x00: ADC Status */
	volatile uint8_t ADCSTS;
	/* 0x01: ADC Configuration */
	volatile uint8_t ADCCFG;
	/* 0x02: ADC Clock Control */
	volatile uint8_t ADCCTL;
	/* 0x03: General Control */
	volatile uint8_t ADCGCR;
	/* 0x04: Voltage Channel 0 Control */
	volatile uint8_t VCH0CTL;
	/* 0x05: Calibration Data Control */
	volatile uint8_t KDCTL;
	/* 0x06-0x17: Reserved1 */
	volatile uint8_t reserved1[18];
	/* 0x18: Voltage Channel 0 Data Buffer LSB */
	volatile uint8_t VCH0DATL;
	/* 0x19: Voltage Channel 0 Data Buffer MSB */
	volatile uint8_t VCH0DATM;
	/* 0x1a-0x43: Reserved2 */
	volatile uint8_t reserved2[42];
	/* 0x44: ADC Data Valid Status */
	volatile uint8_t ADCDVSTS;
	/* 0x45-0x54: Reserved2-1 */
	volatile uint8_t reserved2_1[16];
	/* 0x55: ADC Input Voltage Mapping Full-Scale Code Selection 1 */
	volatile uint8_t ADCIVMFSCS1;
	/* 0x56: ADC Input Voltage Mapping Full-Scale Code Selection 2 */
	volatile uint8_t ADCIVMFSCS2;
	/* 0x57: ADC Input Voltage Mapping Full-Scale Code Selection 3 */
	volatile uint8_t ADCIVMFSCS3;
	/* 0x58-0x5f: Reserved3 */
	volatile uint8_t reserved3[8];
	/* 0x60-0x6b: ADC channel 13~16 controller */
	struct adc_vchs_ctrl_t adc_vchs_ctrl[4];
	/* 0x6c: ADC Data Valid Status 2 */
	volatile uint8_t ADCDVSTS2;
	/* 0x6d-0xef: Reserved4 */
	volatile uint8_t reserved4[131];
	/* 0xf0: ADC Clock Control Register 1 */
	volatile uint8_t ADCCTL1;
};
#endif /* !__ASSEMBLER__ */

/* ADC conversion time select 1 */
#define IT8XXX2_ADC_ADCCTS1			BIT(7)
/* Analog accuracy initialization */
#define IT8XXX2_ADC_AINITB			BIT(3)
/* ADC conversion time select 0 */
#define IT8XXX2_ADC_ADCCTS0			BIT(5)
/* ADC module enable */
#define IT8XXX2_ADC_ADCEN			BIT(0)
/* ADC data buffer keep enable */
#define IT8XXX2_ADC_DBKEN			BIT(7)
/* W/C data valid flag */
#define IT8XXX2_ADC_DATVAL			BIT(7)
/* Data valid interrupt of adc */
#define IT8XXX2_ADC_INTDVEN			BIT(5)
/* Voltage channel enable (Channel 4~7 and 13~16) */
#define IT8XXX2_ADC_VCHEN			BIT(4)
/* Automatic hardware calibration enable */
#define IT8XXX2_ADC_AHCE			BIT(7)
/* 0x046, 0x049, 0x04c, 0x06e, 0x071, 0x074: Voltage comparator x control */
#define IT8XXX2_VCMP_CMPEN			BIT(7)
#define IT8XXX2_VCMP_CMPINTEN			BIT(6)
#define IT8XXX2_VCMP_GREATER_THRESHOLD		BIT(5)
#define IT8XXX2_VCMP_EDGE_TRIGGER		BIT(4)
#define IT8XXX2_VCMP_GPIO_ACTIVE_LOW		BIT(3)
/* 0x077~0x07c: Voltage comparator x channel select MSB */
#define IT8XXX2_VCMP_VCMPXCSELM			BIT(0)

/**
 *
 * (1Exxh) Clock and Power Management (ECPM) registers
 *
 */
#define IT8XXX2_ECPM_BASE  0x00F01E00

#ifndef __ASSEMBLER__
enum chip_pll_mode {
	CHIP_PLL_DOZE = 0,
	CHIP_PLL_SLEEP = 1,
	CHIP_PLL_DEEP_DOZE = 3,
};
#endif
/*
 * TODO: use ecpm_it8xxx2_regs instead of following register declarations
 *       to fix in soc.c.
 */
#define IT8XXX2_ECPM_PLLCTRL    ECREG(IT8XXX2_ECPM_BASE + 0x03)
#define IT8XXX2_ECPM_AUTOCG     ECREG(IT8XXX2_ECPM_BASE + 0x04)
#define IT8XXX2_ECPM_CGCTRL3R   ECREG(IT8XXX2_ECPM_BASE + 0x05)
#define IT8XXX2_ECPM_PLLFREQR   ECREG(IT8XXX2_ECPM_BASE + 0x06)
#define IT8XXX2_ECPM_PLLCSS     ECREG(IT8XXX2_ECPM_BASE + 0x08)
#define IT8XXX2_ECPM_SCDCR0     ECREG(IT8XXX2_ECPM_BASE + 0x0c)
#define IT8XXX2_ECPM_SCDCR1     ECREG(IT8XXX2_ECPM_BASE + 0x0d)
#define IT8XXX2_ECPM_SCDCR2     ECREG(IT8XXX2_ECPM_BASE + 0x0e)
#define IT8XXX2_ECPM_SCDCR3     ECREG(IT8XXX2_ECPM_BASE + 0x0f)
#define IT8XXX2_ECPM_SCDCR4     ECREG(IT8XXX2_ECPM_BASE + 0x10)
#define IT8XXX2_ECPM_PFACC0R    ECREG(IT8XXX2_ECPM_BASE + 0x20)
#define IT8XXX2_ECPM_PFACC1R    ECREG(IT8XXX2_ECPM_BASE + 0x21)
#define IT8XXX2_ECPM_PFACC2R    ECREG(IT8XXX2_ECPM_BASE + 0x40)
#define IT8XXX2_ECPM_LCOTF2     ECREG(IT8XXX2_ECPM_BASE + 0x54)
#define IT8XXX2_ECPM_LCOCR      ECREG(IT8XXX2_ECPM_BASE + 0x55)
#define IT8XXX2_ECPM_LCOCR1     ECREG(IT8XXX2_ECPM_BASE + 0x57)

/*
 * The count number of the counter for 25 ms register.
 * The 25 ms register is calculated by (count number *1.024 kHz).
 */

#define I2C_CLK_LOW_TIMEOUT		255 /* ~=249 ms */

/**
 *
 * (1Cxxh) SMBus Interface (SMB) registers
 *
 */
#define IT8XXX2_SMB_BASE            0x00F01C00
#ifdef CONFIG_SOC_IT8XXX2_REG_SET_V1
#define IT8XXX2_SMB_4P7USL          ECREG(IT8XXX2_SMB_BASE + 0x00)
#define IT8XXX2_SMB_4P0USL          ECREG(IT8XXX2_SMB_BASE + 0x01)
#define IT8XXX2_SMB_300NS           ECREG(IT8XXX2_SMB_BASE + 0x02)
#define IT8XXX2_SMB_250NS           ECREG(IT8XXX2_SMB_BASE + 0x03)
#define IT8XXX2_SMB_25MS            ECREG(IT8XXX2_SMB_BASE + 0x04)
#define IT8XXX2_SMB_45P3USL         ECREG(IT8XXX2_SMB_BASE + 0x05)
#define IT8XXX2_SMB_45P3USH         ECREG(IT8XXX2_SMB_BASE + 0x06)
#define IT8XXX2_SMB_4P7A4P0H        ECREG(IT8XXX2_SMB_BASE + 0x07)
#define IT8XXX2_SMB_SLVISELR        ECREG(IT8XXX2_SMB_BASE + 0x08)
#define IT8XXX2_SMB_SCLKTS(ch)      ECREG(IT8XXX2_SMB_BASE + 0x09 + ch)
#define IT8XXX2_SMB_MSTFCTRL1       ECREG(IT8XXX2_SMB_BASE + 0x0D)
#define IT8XXX2_SMB_MSTFSTS1        ECREG(IT8XXX2_SMB_BASE + 0x0E)
#define IT8XXX2_SMB_MSTFCTRL2       ECREG(IT8XXX2_SMB_BASE + 0x0F)
#define IT8XXX2_SMB_MSTFSTS2        ECREG(IT8XXX2_SMB_BASE + 0x10)
#define IT8XXX2_SMB_SMB45CHS        ECREG(IT8XXX2_SMB_BASE + 0x11)
#define IT8XXX2_SMB_I2CW2RF         ECREG(IT8XXX2_SMB_BASE + 0x12)
#define IT8XXX2_SMB_IWRFISTA        ECREG(IT8XXX2_SMB_BASE + 0x13)
#define IT8XXX2_SMB_SMB01CHS        ECREG(IT8XXX2_SMB_BASE + 0x20)
#define IT8XXX2_SMB_SMB23CHS        ECREG(IT8XXX2_SMB_BASE + 0x21)
#define IT8XXX2_SMB_SFFCTL          ECREG(IT8XXX2_SMB_BASE + 0x55)
#define IT8XXX2_SMB_HOSTA(base)     ECREG(base + 0x00)
#define IT8XXX2_SMB_HOCTL(base)     ECREG(base + 0x01)
#define IT8XXX2_SMB_HOCMD(base)     ECREG(base + 0x02)
#define IT8XXX2_SMB_TRASLA(base)    ECREG(base + 0x03)
#define IT8XXX2_SMB_D0REG(base)     ECREG(base + 0x04)
#define IT8XXX2_SMB_D1REG(base)     ECREG(base + 0x05)
#define IT8XXX2_SMB_HOBDB(base)     ECREG(base + 0x06)
#define IT8XXX2_SMB_PECERC(base)    ECREG(base + 0x07)
#define IT8XXX2_SMB_SMBPCTL(base)   ECREG(base + 0x0A)
#define IT8XXX2_SMB_HOCTL2(base)    ECREG(base + 0x10)
#elif CONFIG_SOC_IT8XXX2_REG_SET_V2
#define IT8XXX2_SMB_SLVISEL         ECREG(IT8XXX2_SMB_BASE + 0x08)
#define IT8XXX2_SMB_SMB01CHS        ECREG(IT8XXX2_SMB_BASE + 0x09)
#define IT8XXX2_SMB_SMB23CHS        ECREG(IT8XXX2_SMB_BASE + 0x0A)
#define IT8XXX2_SMB_SMB45CHS        ECREG(IT8XXX2_SMB_BASE + 0x0B)
#define IT8XXX2_SMB_SCLKTS_BRGS     ECREG(IT8XXX2_SMB_BASE + 0x80)
#define IT8XXX2_SMB_SCLKTS_BRGM     ECREG(IT8XXX2_SMB_BASE + 0x81)
#define IT8XXX2_SMB_CHSBRG          ECREG(IT8XXX2_SMB_BASE + 0x82)
#define IT8XXX2_SMB_CHSMOT          ECREG(IT8XXX2_SMB_BASE + 0x83)

/* SMBus register fields */
/* 0x80: SMCLK Timing Setting Register Bridge Slave */
#define IT8XXX2_SMB_PREDEN            BIT(7)
#endif

/**
 * Enhanced SMBus/I2C Interface
 * Ch_D: 0x00F03680, Ch_E: 0x00F03500, Ch_F: 0x00F03580
 * Ch_D: ch = 0x03, Ch_E: ch = 0x00, Ch_F: ch = 0x01
 */
#define IT8XXX2_I2C_DRR(base)         ECREG(base + 0x00)
#define IT8XXX2_I2C_PSR(base)         ECREG(base + 0x01)
#define IT8XXX2_I2C_HSPR(base)        ECREG(base + 0x02)
#define IT8XXX2_I2C_STR(base)         ECREG(base + 0x03)
#define IT8XXX2_I2C_DHTR(base)        ECREG(base + 0x04)
#define IT8XXX2_I2C_TOR(base)         ECREG(base + 0x05)
#define IT8XXX2_I2C_DTR(base)         ECREG(base + 0x08)
#define IT8XXX2_I2C_CTR(base)         ECREG(base + 0x09)
#define IT8XXX2_I2C_CTR1(base)        ECREG(base + 0x0A)
#define IT8XXX2_I2C_BYTE_CNT_H(base)  ECREG(base + 0x0B)
#define IT8XXX2_I2C_BYTE_CNT_L(base)  ECREG(base + 0x0C)
#define IT8XXX2_I2C_IRQ_ST(base)      ECREG(base + 0x0D)
#define IT8XXX2_I2C_IDR(base)         ECREG(base + 0x06)
#define IT8XXX2_I2C_TOS(base)         ECREG(base + 0x07)
#define IT8XXX2_I2C_SLV_NUM_H(base)   ECREG(base + 0x10)
#define IT8XXX2_I2C_SLV_NUM_L(base)   ECREG(base + 0x11)
#define IT8XXX2_I2C_STR2(base)        ECREG(base + 0x12)
#define IT8XXX2_I2C_NST(base)         ECREG(base + 0x13)
#define IT8XXX2_I2C_TO_ARB_ST(base)   ECREG(base + 0x18)
#define IT8XXX2_I2C_ERR_ST(base)      ECREG(base + 0x19)
#define IT8XXX2_I2C_FST(base)         ECREG(base + 0x1B)
#define IT8XXX2_I2C_EM(base)          ECREG(base + 0x1C)
#define IT8XXX2_I2C_MODE_SEL(base)    ECREG(base + 0x1D)
#define IT8XXX2_I2C_IDR2(base)        ECREG(base + 0x1F)
#define IT8XXX2_I2C_CTR2(base)        ECREG(base + 0x20)
#define IT8XXX2_I2C_RAMHA(base)       ECREG(base + 0x23)
#define IT8XXX2_I2C_RAMLA(base)       ECREG(base + 0x24)
#define IT8XXX2_I2C_RAMHA2(base)      ECREG(base + 0x2C)
#define IT8XXX2_I2C_RAMLA2(base)      ECREG(base + 0x2D)
#define IT8XXX2_I2C_CMD_ADDH(base)    ECREG(base + 0x25)
#define IT8XXX2_I2C_CMD_ADDL(base)    ECREG(base + 0x26)
#define IT8XXX2_I2C_RAMH2A(base)      ECREG(base + 0x50)
#define IT8XXX2_I2C_CMD_ADDH2(base)   ECREG(base + 0x52)

/* SMBus/I2C register fields */
/* 0x09-0xB: SMCLK Timing Setting */
#define IT8XXX2_SMB_SMCLKS_1M         4
#define IT8XXX2_SMB_SMCLKS_400K       3
#define IT8XXX2_SMB_SMCLKS_100K       2
#define IT8XXX2_SMB_SMCLKS_50K        1

/* 0x0E: SMBus FIFO Status 1 */
#define IT8XXX2_SMB_FIFO1_EMPTY       BIT(7)
#define IT8XXX2_SMB_FIFO1_FULL        BIT(6)
/* 0x0D: SMBus FIFO Control 1 */
/* 0x0F: SMBus FIFO Control 2 */
#define IT8XXX2_SMB_BLKDS             BIT(4)
#define IT8XXX2_SMB_FFEN              BIT(3)
#define IT8XXX2_SMB_FFCHSEL2_B        0
#define IT8XXX2_SMB_FFCHSEL2_C        BIT(0)
/* 0x10: SMBus FIFO Status 2 */
#define IT8XXX2_SMB_FIFO2_EMPTY       BIT(7)
#define IT8XXX2_SMB_FIFO2_FULL        BIT(6)
/* 0x12: I2C Wr To Rd FIFO */
#define IT8XXX2_SMB_MAIF              BIT(7)
#define IT8XXX2_SMB_MBCIF             BIT(6)
#define IT8XXX2_SMB_MCIFI             BIT(2)
#define IT8XXX2_SMB_MBIFI             BIT(1)
#define IT8XXX2_SMB_MAIFI             BIT(0)
/* 0x13: I2C Wr To Rd FIFO Interrupt Status */
#define IT8XXX2_SMB_MCIFID            BIT(2)
#define IT8XXX2_SMB_MAIFID            BIT(0)
/* 0x41 0x81 0xC1: Host Control */
#define IT8XXX2_SMB_SRT               BIT(6)
#define IT8XXX2_SMB_LABY              BIT(5)
#define IT8XXX2_SMB_SMCD_EXTND        BIT(4) | BIT(3) | BIT(2)
#define IT8XXX2_SMB_KILL              BIT(1)
#define IT8XXX2_SMB_INTREN            BIT(0)
/* 0x43 0x83 0xC3: Transmit Slave Address */
#define IT8XXX2_SMB_DIR               BIT(0)
/* 0x4A 0x8A 0xCA: SMBus Pin Control */
#define IT8XXX2_SMB_SMBDCS            BIT(1)
#define IT8XXX2_SMB_SMBCS             BIT(0)
/* 0x50 0x90 0xD0: Host Control 2 */
#define IT8XXX2_SMB_SMD_TO_EN         BIT(4)
#define IT8XXX2_SMB_I2C_SW_EN         BIT(3)
#define IT8XXX2_SMB_I2C_SW_WAIT       BIT(2)
#define IT8XXX2_SMB_I2C_EN            BIT(1)
#define IT8XXX2_SMB_SMHEN             BIT(0)
/* 0x55: Slave A FIFO Control */
#define IT8XXX2_SMB_HSAPE             BIT(1)
/* 0x03: Status Register */
#define IT8XXX2_I2C_BYTE_DONE         BIT(7)
#define IT8XXX2_I2C_RW                BIT(2)
#define IT8XXX2_I2C_INT_PEND          BIT(1)
/* 0x04: Data Hold Time */
#define IT8XXX2_I2C_SOFT_RST          BIT(7)
/* 0x07: Time Out Status */
#define IT8XXX2_I2C_CLK_STRETCH       BIT(7)
#define IT8XXX2_I2C_SCL_IN            BIT(2)
#define IT8XXX2_I2C_SDA_IN            BIT(0)
/* 0x09: Control Register */
#define IT8XXX2_I2C_INT_EN            BIT(6)
#define IT8XXX2_I2C_ACK               BIT(3)
#define IT8XXX2_I2C_HALT              BIT(0)
/* 0x0A: Control 1 */
#define IT8XXX2_I2C_COMQ_EN           BIT(7)
#define IT8XXX2_I2C_MDL_EN            BIT(1)
/* 0x0C: Byte count */
#define IT8XXX2_I2C_DMA_ADDR_RELOAD   BIT(5)
#define IT8XXX2_I2C_BYTE_CNT_ENABLE   BIT(3)
/* 0x0D: Interrupt Status */
#define IT8XXX2_I2C_CNT_HOLD          BIT(4)
#define IT8XXX2_I2C_IDW_CLR           BIT(3)
#define IT8XXX2_I2C_IDR_CLR           BIT(2)
#define IT8XXX2_I2C_SLVDATAFLG        BIT(1)
#define IT8XXX2_I2C_P_CLR             BIT(0)
/* 0x13: Nack Status */
#define IT8XXX2_I2C_NST_CNS           BIT(7)
#define IT8XXX2_I2C_NST_ID_NACK       BIT(3)
/* 0x18: Timeout and Arbiter Status */
#define IT8XXX2_I2C_SCL_TIMEOUT_EN    BIT(7)
#define IT8XXX2_I2C_SDA_TIMEOUT_EN    BIT(6)
/* 0x19: Error Status */
#define IT8XXX2_I2C_ERR_ST_DEV1_EIRQ  BIT(0)
/* 0x1B: Finish Status */
#define IT8XXX2_I2C_FST_DEV1_IRQ      BIT(4)
/* 0x1C: Error Mask */
#define IT8XXX2_I2C_EM_DEV1_IRQ       BIT(4)

/*
 * TODO: use gctrl_it8xxx2_regs instead of following register declarations
 *       to fix in cros_flash_it8xxx2.c, cros_shi_it8xxx2.c and tcpm\it8xxx2.c.
 */
/* --- General Control (GCTRL) --- */
#define IT83XX_GCTRL_BASE 0x00F02000

#define IT83XX_GCTRL_CHIPID1         ECREG(IT83XX_GCTRL_BASE + 0x85)
#define IT83XX_GCTRL_CHIPID2         ECREG(IT83XX_GCTRL_BASE + 0x86)
#define IT83XX_GCTRL_CHIPVER         ECREG(IT83XX_GCTRL_BASE + 0x02)
#define IT83XX_GCTRL_MCCR3           ECREG(IT83XX_GCTRL_BASE + 0x20)
#define IT83XX_GCTRL_SPISLVPFE             BIT(6)
#define IT83XX_GCTRL_EWPR0PFH(i)     ECREG(IT83XX_GCTRL_BASE + 0x60 + i)
#define IT83XX_GCTRL_EWPR0PFD(i)     ECREG(IT83XX_GCTRL_BASE + 0xA0 + i)
#define IT83XX_GCTRL_EWPR0PFEC(i)    ECREG(IT83XX_GCTRL_BASE + 0xC0 + i)

/*
 * TODO: use spisc_it8xxx2_regs instead of following register declarations
 *       to fix in cros_shi_it8xxx2.c.
 */
/* Serial Peripheral Interface (SPI) */
#define IT83XX_SPI_BASE  0x00F03A00

#define IT83XX_SPI_SPISGCR           ECREG(IT83XX_SPI_BASE + 0x00)
#define IT83XX_SPI_SPISCEN                 BIT(0)
#define IT83XX_SPI_TXRXFAR           ECREG(IT83XX_SPI_BASE + 0x01)
#define IT83XX_SPI_CPURXF2A                BIT(4)
#define IT83XX_SPI_CPURXF1A                BIT(3)
#define IT83XX_SPI_CPUTFA                  BIT(1)
#define IT83XX_SPI_TXFCR             ECREG(IT83XX_SPI_BASE + 0x02)
#define IT83XX_SPI_TXFCMR                  BIT(2)
#define IT83XX_SPI_TXFR                    BIT(1)
#define IT83XX_SPI_TXFS                    BIT(0)
#define IT83XX_SPI_GCR2              ECREG(IT83XX_SPI_BASE + 0x03)
#define IT83XX_SPI_RXF2OC                  BIT(4)
#define IT83XX_SPI_RXF1OC                  BIT(3)
#define IT83XX_SPI_RXFAR                   BIT(0)
#define IT83XX_SPI_IMR               ECREG(IT83XX_SPI_BASE + 0x04)
#define IT83XX_SPI_RX_FIFO_FULL            BIT(7)
#define IT83XX_SPI_RX_REACH                BIT(5)
#define IT83XX_SPI_EDIM                    BIT(2)
#define IT83XX_SPI_ISR               ECREG(IT83XX_SPI_BASE + 0x05)
#define IT83XX_SPI_TXFSR             ECREG(IT83XX_SPI_BASE + 0x06)
#define IT83XX_SPI_ENDDETECTINT            BIT(2)
#define IT83XX_SPI_RXFSR             ECREG(IT83XX_SPI_BASE + 0x07)
#define IT83XX_SPI_RXFFSM                  (BIT(4) | BIT(3))
#define IT83XX_SPI_RXF2FS                  BIT(2)
#define IT83XX_SPI_RXF1FS                  BIT(1)
#define IT83XX_SPI_SPISRDR           ECREG(IT83XX_SPI_BASE + 0x0b)
#define IT83XX_SPI_CPUWTFDB0         ECREG_u32(IT83XX_SPI_BASE + 0x08)
#define IT83XX_SPI_FCR               ECREG(IT83XX_SPI_BASE + 0x09)
#define IT83XX_SPI_SPISRTXF                BIT(2)
#define IT83XX_SPI_RXFR                    BIT(1)
#define IT83XX_SPI_RXFCMR                  BIT(0)
#define IT83XX_SPI_RXFRDRB0          ECREG_u32(IT83XX_SPI_BASE + 0x0C)
#define IT83XX_SPI_FTCB0R            ECREG(IT83XX_SPI_BASE + 0x18)
#define IT83XX_SPI_FTCB1R            ECREG(IT83XX_SPI_BASE + 0x19)
#define IT83XX_SPI_TCCB0             ECREG(IT83XX_SPI_BASE + 0x1A)
#define IT83XX_SPI_TCCB1             ECREG(IT83XX_SPI_BASE + 0x1B)
#define IT83XX_SPI_HPR2              ECREG(IT83XX_SPI_BASE + 0x1E)
#define IT83XX_SPI_EMMCBMR           ECREG(IT83XX_SPI_BASE + 0x21)
#define IT83XX_SPI_EMMCABM                 BIT(1) /* eMMC Alternative Boot Mode */
#define IT83XX_SPI_RX_VLISMR         ECREG(IT83XX_SPI_BASE + 0x26)
#define IT83XX_SPI_RVLIM                   BIT(0)
#define IT83XX_SPI_RX_VLISR          ECREG(IT83XX_SPI_BASE + 0x27)
#define IT83XX_SPI_RVLI                    BIT(0)

/**
 *
 * (20xxh) General Control (GCTRL) registers
 *
 */
#define GCTRL_IT8XXX2_REGS_BASE \
	((struct gctrl_it8xxx2_regs *)DT_REG_ADDR(DT_NODELABEL(gctrl)))

#ifndef __ASSEMBLER__
struct gctrl_it8xxx2_regs {
	/* 0x00-0x01: Reserved_00_01 */
	volatile uint8_t reserved_00_01[2];
	/* 0x02: Chip Version */
	volatile uint8_t GCTRL_ECHIPVER;
	/* 0x03: DBGR Operate Status */
	volatile uint8_t GCTRL_DBGROS;
	/* 0x04-0x05: Reserved_04_05 */
	volatile uint8_t reserved_04_05[2];
	/* 0x06: Reset Status */
	volatile uint8_t GCTRL_RSTS;
	/* 0x07-0x09: Reserved_07_09 */
	volatile uint8_t reserved_07_09[3];
	/* 0x0A: Base Address Select */
	volatile uint8_t GCTRL_BADRSEL;
	/* 0x0B: Wait Next Clock Rising */
	volatile uint8_t GCTRL_WNCKR;
	/* 0x0C: reserved_0c */
	volatile uint8_t reserved_0c;
	/* 0x0D: Special Control 1 */
	volatile uint8_t GCTRL_SPCTRL1;
	/* 0x0E-0x0F: reserved_0e_0f */
	volatile uint8_t reserved_0e_0f[2];
	/* 0x10: Reset Control DMM */
	volatile uint8_t GCTRL_RSTDMMC;
	/* 0x11: Reset Control 4 */
	volatile uint8_t GCTRL_RSTC4;
	/* 0x12-0x1B: reserved_12_1b */
	volatile uint8_t reserved_12_1b[10];
	/* 0x1C: Special Control 4 */
	volatile uint8_t GCTRL_SPCTRL4;
	/* 0x1D-0x1F: reserved_1d_1f */
	volatile uint8_t reserved_1d_1f[3];
	/* 0x20: Memory Controller Configuration 3 */
	volatile uint8_t GCTRL_MCCR3;
	/* 0x21: Reset Control 5 */
	volatile uint8_t GCTRL_RSTC5;
	/* 0x22-0x2F: reserved_22_2f */
	volatile uint8_t reserved_22_2f[14];
	/* 0x30: Memory Controller Configuration */
	volatile uint8_t GCTRL_MCCR;
	/* 0x31: Externel ILM/DLM Size */
	volatile uint8_t GCTRL_EIDSR;
	/* 0x32: Reserved_32 */
	volatile uint8_t reserved_32;
	/* 0x33: Pin Multi-function Enable 2 */
	volatile uint8_t gctrl_pmer2;
	/* 0x34-0x36: Reserved_34_36 */
	volatile uint8_t reserved_34_36[3];
	/* 0x37: Eflash Protect Lock */
	volatile uint8_t GCTRL_EPLR;
	/* 0x38-0x40: Reserved_38_40 */
	volatile uint8_t reserved_38_40[9];
	/* 0x41: Interrupt Vector Table Base Address */
	volatile uint8_t GCTRL_IVTBAR;
	/* 0x42-0x43: Reserved_42_43 */
	volatile uint8_t reserved_42_43[2];
	/* 0x44: Memory Controller Configuration 2 */
	volatile uint8_t GCTRL_MCCR2;
	/* 0x45: Reserved_45 */
	volatile uint8_t reserved_45;
	/* 0x46: Pin Multi-function Enable 3 */
	volatile uint8_t GCTRL_PMER3;
	/* 0x47-0x4A: reserved_47_4a */
	volatile uint8_t reserved_47_4a[4];
	/* 0x4B: ETWD and UART Control */
	volatile uint8_t GCTRL_ETWDUARTCR;
	/* 0x4C: Wakeup MCU Control */
	volatile uint8_t GCTRL_WMCR;
	/* 0x4D-0x4F: reserved_4d_4f */
	volatile uint8_t reserved_4d_4f[3];
	/* 0x50: Port 80h/81h Status Register */
	volatile uint8_t GCTRL_P80H81HSR;
	/* 0x51: Port 80h Data Register */
	volatile uint8_t GCTRL_P80HDR;
	/* 0x52: Port 81h Data Register */
	volatile uint8_t GCTRL_P81HDR;
	/* 0x53: H2RAM Offset Register */
	volatile uint8_t GCTRL_H2ROFSR;
	/* 0x54-0x5C: reserved_54_5c */
	volatile uint8_t reserved_54_5c[9];
	/* 0x5D: RISCV ILM Configuration 0 */
	volatile uint8_t GCTRL_RVILMCR0;
	/* 0x5E-0x84: reserved_5e_84 */
	volatile uint8_t reserved_5e_84[39];
	/* 0x85: Chip ID Byte 1 */
	volatile uint8_t GCTRL_ECHIPID1;
	/* 0x86: Chip ID Byte 2 */
	volatile uint8_t GCTRL_ECHIPID2;
	/* 0x87: Chip ID Byte 3 */
	volatile uint8_t GCTRL_ECHIPID3;
};
#endif /* !__ASSEMBLER__ */

/* GCTRL register fields */
/* 0x03: DBGR Operate Status */
#define IT8XXX2_GCTRL_SMB_DBGR		BIT(0)
/* 0x06: Reset Status */
#define IT8XXX2_GCTRL_LRS		(BIT(1) | BIT(0))
#define IT8XXX2_GCTRL_IWDTR		BIT(1)
/* 0x0B: Wait Next 65K Rising */
#define IT8XXX2_GCTRL_WN65K		0x00
/* 0x10: Reset Control DMM */
#define IT8XXX2_GCTRL_UART1SD		BIT(3)
#define IT8XXX2_GCTRL_UART2SD		BIT(2)
/* 0x11: Reset Control 4 */
#define IT8XXX2_GCTRL_RPECI		BIT(4)
#define IT8XXX2_GCTRL_RUART2		BIT(2)
#define IT8XXX2_GCTRL_RUART1		BIT(1)
/* 0x1C: Special Control 4 */
#define IT8XXX2_GCTRL_LRSIWR		BIT(2)
#define IT8XXX2_GCTRL_LRSIPWRSWTR	BIT(1)
#define IT8XXX2_GCTRL_LRSIPGWR		BIT(0)
/* 0x20: Memory Controller Configuration 3 */
#define IT8XXX2_GCTRL_SPISLVPFE		BIT(6)
/* 0x30: Memory Controller Configuration */
#define IT8XXX2_GCTRL_USB_DEBUG_EN      BIT(7)
#define IT8XXX2_GCTRL_ICACHE_RESET	BIT(4)
/* 0x37: Eflash Protect Lock */
#define IT8XXX2_GCTRL_EPLR_ENABLE	BIT(0)
/* 0x46: Pin Multi-function Enable 3 */
#define IT8XXX2_GCTRL_SMB3PSEL		BIT(6)
#define IT8XXX2_GCTRL_SRAM_CRYPTO_USED	BIT(5)
/* 0x4B: ETWD and UART Control */
#define IT8XXX2_GCTRL_ETWD_HW_RST_EN	BIT(0)
/* 0x5D: RISCV ILM Configuration 0 */
#define IT8XXX2_GCTRL_ILM0_ENABLE	BIT(0)
/* Accept Port 80h Cycle */
#define IT8XXX2_GCTRL_ACP80		BIT(6)
/* Accept Port 81h Cycle */
#define IT8XXX2_GCTRL_ACP81		BIT(3)
/* USB Debug Enable */
#define IT8XXX2_GCTRL_MCCR_USB_EN	BIT(7)
/* USB Pad Power-On Enable */
#define IT8XXX2_GCTRL_PMER2_USB_PAD_EN	BIT(7)

/*
 * VCC Detector Option.
 * bit[7-6] = 1: The VCC power status is treated as power-on.
 * The VCC supply of eSPI and related functions (EC2I, KBC, PMC and
 * PECI). It means VCC should be logic high before using these
 * functions, or firmware treats VCC logic high.
 */
#define IT8XXX2_GCTRL_VCCDO_MASK	(BIT(6) | BIT(7))
#define IT8XXX2_GCTRL_VCCDO_VCC_ON	BIT(6)
/*
 * bit[3] = 0: The reset source of PNPCFG is RSTPNP bit in RSTCH
 * register and WRST#.
 */
#define IT8XXX2_GCTRL_HGRST		BIT(3)
/* bit[2] = 1: Enable global reset. */
#define IT8XXX2_GCTRL_GRST		BIT(2)

/**
 *
 * (22xxh) Battery-backed SRAM (BRAM) registers
 *
 */
#ifndef __ASSEMBLER__
/* Battery backed RAM indices. */
#define BRAM_MAGIC_FIELD_OFFSET 0xbc
enum bram_indices {

	/* This field is used to indicate BRAM is valid or not. */
	BRAM_IDX_VALID_FLAGS0 = BRAM_MAGIC_FIELD_OFFSET,
	BRAM_IDX_VALID_FLAGS1,
	BRAM_IDX_VALID_FLAGS2,
	BRAM_IDX_VALID_FLAGS3
};
#endif /* !__ASSEMBLER__ */

#ifndef __ASSEMBLER__
/*
 * EC2I bridge registers
 */
struct ec2i_regs {
	/* 0x00: Indirect Host I/O Address Register */
	volatile uint8_t IHIOA;
	/* 0x01: Indirect Host Data Register */
	volatile uint8_t IHD;
	/* 0x02: Lock Super I/O Host Access Register */
	volatile uint8_t LSIOHA;
	/* 0x03: Super I/O Access Lock Violation Register */
	volatile uint8_t SIOLV;
	/* 0x04: EC to I-Bus Modules Access Enable Register */
	volatile uint8_t IBMAE;
	/* 0x05: I-Bus Control Register */
	volatile uint8_t IBCTL;
};

/* Index list of the host interface registers of PNPCFG */
enum host_pnpcfg_index {
	/* Logical Device Number */
	HOST_INDEX_LDN = 0x07,
	/* Chip ID Byte 1 */
	HOST_INDEX_CHIPID1 = 0x20,
	/* Chip ID Byte 2 */
	HOST_INDEX_CHIPID2 = 0x21,
	/* Chip Version */
	HOST_INDEX_CHIPVER = 0x22,
	/* Super I/O Control */
	HOST_INDEX_SIOCTRL = 0x23,
	/* Super I/O IRQ Configuration */
	HOST_INDEX_SIOIRQ = 0x25,
	/* Super I/O General Purpose */
	HOST_INDEX_SIOGP = 0x26,
	/* Super I/O Power Mode */
	HOST_INDEX_SIOPWR = 0x2D,
	/* Depth 2 I/O Address */
	HOST_INDEX_D2ADR = 0x2E,
	/* Depth 2 I/O Data */
	HOST_INDEX_D2DAT = 0x2F,
	/* Logical Device Activate Register */
	HOST_INDEX_LDA = 0x30,
	/* I/O Port Base Address Bits [15:8] for Descriptor 0 */
	HOST_INDEX_IOBAD0_MSB = 0x60,
	/* I/O Port Base Address Bits [7:0] for Descriptor 0 */
	HOST_INDEX_IOBAD0_LSB = 0x61,
	/* I/O Port Base Address Bits [15:8] for Descriptor 1 */
	HOST_INDEX_IOBAD1_MSB = 0x62,
	/* I/O Port Base Address Bits [7:0] for Descriptor 1 */
	HOST_INDEX_IOBAD1_LSB = 0x63,
	/* Interrupt Request Number and Wake-Up on IRQ Enabled */
	HOST_INDEX_IRQNUMX = 0x70,
	/* Interrupt Request Type Select */
	HOST_INDEX_IRQTP = 0x71,
	/* DMA Channel Select 0 */
	HOST_INDEX_DMAS0 = 0x74,
	/* DMA Channel Select 1 */
	HOST_INDEX_DMAS1 = 0x75,
	/* Device Specific Logical Device Configuration 1 to 10 */
	HOST_INDEX_DSLDC1 = 0xF0,
	HOST_INDEX_DSLDC2 = 0xF1,
	HOST_INDEX_DSLDC3 = 0xF2,
	HOST_INDEX_DSLDC4 = 0xF3,
	HOST_INDEX_DSLDC5 = 0xF4,
	HOST_INDEX_DSLDC6 = 0xF5,
	HOST_INDEX_DSLDC7 = 0xF6,
	HOST_INDEX_DSLDC8 = 0xF7,
	HOST_INDEX_DSLDC9 = 0xF8,
	HOST_INDEX_DSLDC10 = 0xF9,
};

/* List of logical device number (LDN) assignments */
enum logical_device_number {
	/* Serial Port 1 */
	LDN_UART1 = 0x01,
	/* Serial Port 2 */
	LDN_UART2 = 0x02,
	/* System Wake-Up Control */
	LDN_SWUC = 0x04,
	/* KBC/Mouse Interface */
	LDN_KBC_MOUSE = 0x05,
	/* KBC/Keyboard Interface */
	LDN_KBC_KEYBOARD = 0x06,
	/* Consumer IR */
	LDN_CIR = 0x0A,
	/* Shared Memory/Flash Interface */
	LDN_SMFI = 0x0F,
	/* RTC-like Timer */
	LDN_RTCT = 0x10,
	/* Power Management I/F Channel 1 */
	LDN_PMC1 = 0x11,
	/* Power Management I/F Channel 2 */
	LDN_PMC2 = 0x12,
	/* Serial Peripheral Interface */
	LDN_SSPI = 0x13,
	/* Platform Environment Control Interface */
	LDN_PECI = 0x14,
	/* Power Management I/F Channel 3 */
	LDN_PMC3 = 0x17,
	/* Power Management I/F Channel 4 */
	LDN_PMC4 = 0x18,
	/* Power Management I/F Channel 5 */
	LDN_PMC5 = 0x19,
};

/* Structure for initializing PNPCFG via ec2i. */
struct ec2i_t {
	/* index port */
	enum host_pnpcfg_index index_port;
	/* data port */
	uint8_t data_port;
};

/* EC2I access index/data port */
enum ec2i_access {
	/* index port */
	EC2I_ACCESS_INDEX = 0,
	/* data port */
	EC2I_ACCESS_DATA = 1,
};

/* EC to I-Bus Access Enabled */
#define EC2I_IBCTL_CSAE  BIT(0)
/* EC Read from I-Bus */
#define EC2I_IBCTL_CRIB  BIT(1)
/* EC Write to I-Bus */
#define EC2I_IBCTL_CWIB  BIT(2)
#define EC2I_IBCTL_CRWIB (EC2I_IBCTL_CRIB | EC2I_IBCTL_CWIB)

/* PNPCFG Register EC Access Enable */
#define EC2I_IBMAE_CFGAE BIT(0)

/*
 * KBC registers
 */
struct kbc_regs {
	/* 0x00: KBC Host Interface Control Register */
	volatile uint8_t KBHICR;
	/* 0x01: Reserved1 */
	volatile uint8_t reserved1;
	/* 0x02: KBC Interrupt Control Register */
	volatile uint8_t KBIRQR;
	/* 0x03: Reserved2 */
	volatile uint8_t reserved2;
	/* 0x04: KBC Host Interface Keyboard/Mouse Status Register */
	volatile uint8_t KBHISR;
	/* 0x05: Reserved3 */
	volatile uint8_t reserved3;
	/* 0x06: KBC Host Interface Keyboard Data Output Register */
	volatile uint8_t KBHIKDOR;
	/* 0x07: Reserved4 */
	volatile uint8_t reserved4;
	/* 0x08: KBC Host Interface Mouse Data Output Register */
	volatile uint8_t KBHIMDOR;
	/* 0x09: Reserved5 */
	volatile uint8_t reserved5;
	/* 0x0a: KBC Host Interface Keyboard/Mouse Data Input Register */
	volatile uint8_t KBHIDIR;
};

/* Output Buffer Full */
#define KBC_KBHISR_OBF      BIT(0)
/* Input Buffer Full */
#define KBC_KBHISR_IBF      BIT(1)
/* A2 Address (A2) */
#define KBC_KBHISR_A2_ADDR  BIT(3)
#define KBC_KBHISR_STS_MASK (KBC_KBHISR_OBF | KBC_KBHISR_IBF \
						| KBC_KBHISR_A2_ADDR)

/* Clear Output Buffer Full */
#define KBC_KBHICR_COBF      BIT(6)
/* IBF/OBF Clear Mode Enable */
#define KBC_KBHICR_IBFOBFCME BIT(5)
/* Input Buffer Full CPU Interrupt Enable */
#define KBC_KBHICR_IBFCIE    BIT(3)
/* Output Buffer Empty CPU Interrupt Enable */
#define KBC_KBHICR_OBECIE    BIT(2)
/* Output Buffer Full Mouse Interrupt Enable */
#define KBC_KBHICR_OBFMIE    BIT(1)
/* Output Buffer Full Keyboard Interrupt Enable */
#define KBC_KBHICR_OBFKIE    BIT(0)

/*
 * PMC registers
 */
struct pmc_regs {
	/* 0x00: Host Interface PM Channel 1 Status */
	volatile uint8_t PM1STS;
	/* 0x01: Host Interface PM Channel 1 Data Out Port */
	volatile uint8_t PM1DO;
	/* 0x02: Host Interface PM Channel 1 Data Out Port with SCI# */
	volatile uint8_t PM1DOSCI;
	/* 0x03: Host Interface PM Channel 1 Data Out Port with SMI# */
	volatile uint8_t PM1DOSMI;
	/* 0x04: Host Interface PM Channel 1 Data In Port */
	volatile uint8_t PM1DI;
	/* 0x05: Host Interface PM Channel 1 Data In Port with SCI# */
	volatile uint8_t PM1DISCI;
	/* 0x06: Host Interface PM Channel 1 Control */
	volatile uint8_t PM1CTL;
	/* 0x07: Host Interface PM Channel 1 Interrupt Control */
	volatile uint8_t PM1IC;
	/* 0x08: Host Interface PM Channel 1 Interrupt Enable */
	volatile uint8_t PM1IE;
	/* 0x09-0x0f: Reserved1 */
	volatile uint8_t reserved1[7];
	/* 0x10: Host Interface PM Channel 2 Status */
	volatile uint8_t PM2STS;
	/* 0x11: Host Interface PM Channel 2 Data Out Port */
	volatile uint8_t PM2DO;
	/* 0x12: Host Interface PM Channel 2 Data Out Port with SCI# */
	volatile uint8_t PM2DOSCI;
	/* 0x13: Host Interface PM Channel 2 Data Out Port with SMI# */
	volatile uint8_t PM2DOSMI;
	/* 0x14: Host Interface PM Channel 2 Data In Port */
	volatile uint8_t PM2DI;
	/* 0x15: Host Interface PM Channel 2 Data In Port with SCI# */
	volatile uint8_t PM2DISCI;
	/* 0x16: Host Interface PM Channel 2 Control */
	volatile uint8_t PM2CTL;
	/* 0x17: Host Interface PM Channel 2 Interrupt Control */
	volatile uint8_t PM2IC;
	/* 0x18: Host Interface PM Channel 2 Interrupt Enable */
	volatile uint8_t PM2IE;
	/* 0x19: Mailbox Control */
	volatile uint8_t MBXCTRL;
	/* 0x1a-0x1f: Reserved2 */
	volatile uint8_t reserved2[6];
	/* 0x20-0xff: Reserved3 */
	volatile uint8_t reserved3[0xe0];
};

/* Input Buffer Full Interrupt Enable */
#define PMC_PM1CTL_IBFIE    BIT(0)
/* Output Buffer Full */
#define PMC_PM1STS_OBF      BIT(0)
/* Input Buffer Full */
#define PMC_PM1STS_IBF      BIT(1)
/* General Purpose Flag */
#define PMC_PM1STS_GPF      BIT(2)
/* A2 Address (A2) */
#define PMC_PM1STS_A2_ADDR  BIT(3)

/* PMC2 Input Buffer Full Interrupt Enable */
#define PMC_PM2CTL_IBFIE    BIT(0)
/* General Purpose Flag */
#define PMC_PM2STS_GPF      BIT(2)

/*
 * Dedicated Interrupt
 * 0b:
 * INT3: PMC Output Buffer Empty Int
 * INT25: PMC Input Buffer Full Int
 * 1b:
 * INT3: PMC1 Output Buffer Empty Int
 * INT25: PMC1 Input Buffer Full Int
 * INT26: PMC2 Output Buffer Empty Int
 * INT27: PMC2 Input Buffer Full Int
 */
#define PMC_MBXCTRL_DINT    BIT(5)

/*
 * eSPI slave registers
 */
struct espi_slave_regs {
	/* 0x00-0x03: Reserved1 */
	volatile uint8_t reserved1[4];

	/* 0x04: General Capabilities and Configuration 0 */
	volatile uint8_t GCAPCFG0;
	/* 0x05: General Capabilities and Configuration 1 */
	volatile uint8_t GCAPCFG1;
	/* 0x06: General Capabilities and Configuration 2 */
	volatile uint8_t GCAPCFG2;
	/* 0x07: General Capabilities and Configuration 3 */
	volatile uint8_t GCAPCFG3;

	/* Channel 0 (Peripheral Channel) Capabilities and Configurations */
	/* 0x08: Channel 0 Capabilities and Configuration 0 */
	volatile uint8_t CH_PC_CAPCFG0;
	/* 0x09: Channel 0 Capabilities and Configuration 1 */
	volatile uint8_t CH_PC_CAPCFG1;
	/* 0x0A: Channel 0 Capabilities and Configuration 2 */
	volatile uint8_t CH_PC_CAPCFG2;
	/* 0x0B: Channel 0 Capabilities and Configuration 3 */
	volatile uint8_t CH_PC_CAPCFG3;

	/* Channel 1 (Virtual Wire Channel) Capabilities and Configurations */
	/* 0x0C: Channel 1 Capabilities and Configuration 0 */
	volatile uint8_t CH_VW_CAPCFG0;
	/* 0x0D: Channel 1 Capabilities and Configuration 1 */
	volatile uint8_t CH_VW_CAPCFG1;
	/* 0x0E: Channel 1 Capabilities and Configuration 2 */
	volatile uint8_t CH_VW_CAPCFG2;
	/* 0x0F: Channel 1 Capabilities and Configuration 3 */
	volatile uint8_t CH_VW_CAPCFG3;

	/* Channel 2 (OOB Message Channel) Capabilities and Configurations */
	/* 0x10: Channel 2 Capabilities and Configuration 0 */
	volatile uint8_t CH_OOB_CAPCFG0;
	/* 0x11: Channel 2 Capabilities and Configuration 1 */
	volatile uint8_t CH_OOB_CAPCFG1;
	/* 0x12: Channel 2 Capabilities and Configuration 2 */
	volatile uint8_t CH_OOB_CAPCFG2;
	/* 0x13: Channel 2 Capabilities and Configuration 3 */
	volatile uint8_t CH_OOB_CAPCFG3;

	/* Channel 3 (Flash Access Channel) Capabilities and Configurations */
	/* 0x14: Channel 3 Capabilities and Configuration 0 */
	volatile uint8_t CH_FLASH_CAPCFG0;
	/* 0x15: Channel 3 Capabilities and Configuration 1 */
	volatile uint8_t CH_FLASH_CAPCFG1;
	/* 0x16: Channel 3 Capabilities and Configuration 2 */
	volatile uint8_t CH_FLASH_CAPCFG2;
	/* 0x17: Channel 3 Capabilities and Configuration 3 */
	volatile uint8_t CH_FLASH_CAPCFG3;
	/* Channel 3 Capabilities and Configurations 2 */
	/* 0x18: Channel 3 Capabilities and Configuration 2-0 */
	volatile uint8_t CH_FLASH_CAPCFG2_0;
	/* 0x19: Channel 3 Capabilities and Configuration 2-1 */
	volatile uint8_t CH_FLASH_CAPCFG2_1;
	/* 0x1A: Channel 3 Capabilities and Configuration 2-2 */
	volatile uint8_t CH_FLASH_CAPCFG2_2;
	/* 0x1B: Channel 3 Capabilities and Configuration 2-3 */
	volatile uint8_t CH_FLASH_CAPCFG2_3;

	/* 0x1c-0x1f: Reserved2 */
	volatile uint8_t reserved2[4];
	/* 0x20-0x8f: Reserved3 */
	volatile uint8_t reserved3[0x70];

	/* 0x90: eSPI PC Control 0 */
	volatile uint8_t ESPCTRL0;
	/* 0x91: eSPI PC Control 1 */
	volatile uint8_t ESPCTRL1;
	/* 0x92: eSPI PC Control 2 */
	volatile uint8_t ESPCTRL2;
	/* 0x93: eSPI PC Control 3 */
	volatile uint8_t ESPCTRL3;
	/* 0x94: eSPI PC Control 4 */
	volatile uint8_t ESPCTRL4;
	/* 0x95: eSPI PC Control 5 */
	volatile uint8_t ESPCTRL5;
	/* 0x96: eSPI PC Control 6 */
	volatile uint8_t ESPCTRL6;
	/* 0x97: eSPI PC Control 7 */
	volatile uint8_t ESPCTRL7;
	/* 0x98-0x9f: Reserved4 */
	volatile uint8_t reserved4[8];

	/* 0xa0: eSPI General Control 0 */
	volatile uint8_t ESGCTRL0;
	/* 0xa1: eSPI General Control 1 */
	volatile uint8_t ESGCTRL1;
	/* 0xa2: eSPI General Control 2 */
	volatile uint8_t ESGCTRL2;
	/* 0xa3: eSPI General Control 3 */
	volatile uint8_t ESGCTRL3;
	/* 0xa4-0xaf: Reserved5 */
	volatile uint8_t reserved5[12];

	/* 0xb0: eSPI Upstream Control 0 */
	volatile uint8_t ESUCTRL0;
	/* 0xb1: eSPI Upstream Control 1 */
	volatile uint8_t ESUCTRL1;
	/* 0xb2: eSPI Upstream Control 2 */
	volatile uint8_t ESUCTRL2;
	/* 0xb3: eSPI Upstream Control 3 */
	volatile uint8_t ESUCTRL3;
	/* 0xb4-0xb5: Reserved6 */
	volatile uint8_t reserved6[2];
	/* 0xb6: eSPI Upstream Control 6 */
	volatile uint8_t ESUCTRL6;
	/* 0xb7: eSPI Upstream Control 7 */
	volatile uint8_t ESUCTRL7;
	/* 0xb8: eSPI Upstream Control 8 */
	volatile uint8_t ESUCTRL8;
	/* 0xb9-0xbf: Reserved7 */
	volatile uint8_t reserved7[7];

	/* 0xc0: eSPI OOB Control 0 */
	volatile uint8_t ESOCTRL0;
	/* 0xc1: eSPI OOB Control 1 */
	volatile uint8_t ESOCTRL1;
	/* 0xc2-0xc3: Reserved8 */
	volatile uint8_t reserved8[2];
	/* 0xc4: eSPI OOB Control 4 */
	volatile uint8_t ESOCTRL4;
	/* 0xc5-0xcf: Reserved9 */
	volatile uint8_t reserved9[11];

	/* 0xd0: eSPI SAFS Control 0 */
	volatile uint8_t ESPISAFSC0;
	/* 0xd1: eSPI SAFS Control 1 */
	volatile uint8_t ESPISAFSC1;
	/* 0xd2: eSPI SAFS Control 2 */
	volatile uint8_t ESPISAFSC2;
	/* 0xd3: eSPI SAFS Control 3 */
	volatile uint8_t ESPISAFSC3;
	/* 0xd4: eSPI SAFS Control 4 */
	volatile uint8_t ESPISAFSC4;
	/* 0xd5: eSPI SAFS Control 5 */
	volatile uint8_t ESPISAFSC5;
	/* 0xd6: eSPI SAFS Control 6 */
	volatile uint8_t ESPISAFSC6;
	/* 0xd7: eSPI SAFS Control 7 */
	volatile uint8_t ESPISAFSC7;
};

/*
 * eSPI VW registers
 */
struct espi_vw_regs {
	/* 0x00-0x7f: VW index */
	volatile uint8_t VW_INDEX[0x80];
	/* 0x80-0x8f: Reserved1 */
	volatile uint8_t reserved1[0x10];
	/* 0x90: VW Contrl 0 */
	volatile uint8_t VWCTRL0;
	/* 0x91: VW Contrl 1 */
	volatile uint8_t VWCTRL1;
	/* 0x92: VW Contrl 2 */
	volatile uint8_t VWCTRL2;
	/* 0x93: VW Contrl 3 */
	volatile uint8_t VWCTRL3;
	/* 0x94: Reserved2 */
	volatile uint8_t reserved2;
	/* 0x95: VW Contrl 5 */
	volatile uint8_t VWCTRL5;
	/* 0x96: VW Contrl 6 */
	volatile uint8_t VWCTRL6;
	/* 0x97: VW Contrl 7 */
	volatile uint8_t VWCTRL7;
	/* 0x98-0x99: Reserved3 */
	volatile uint8_t reserved3[2];
};

#define ESPI_IT8XXX2_OOB_MAX_PAYLOAD_SIZE 80
/*
 * eSPI Queue 0 registers
 */
struct espi_queue0_regs {
	/* 0x00-0x3f: PUT_PC Data Byte 0-63 */
	volatile uint8_t PUT_PC_DATA[0x40];
	/* 0x40-0x7f: Reserved1 */
	volatile uint8_t reserved1[0x40];
	/* 0x80-0xcf: PUT_OOB Data Byte 0-79 */
	volatile uint8_t PUT_OOB_DATA[ESPI_IT8XXX2_OOB_MAX_PAYLOAD_SIZE];
};

/*
 * eSPI Queue 1 registers
 */
struct espi_queue1_regs {
	/* 0x00-0x4f: Upstream Data Byte 0-79 */
	volatile uint8_t UPSTREAM_DATA[ESPI_IT8XXX2_OOB_MAX_PAYLOAD_SIZE];
	/* 0x50-0x7f: Reserved1 */
	volatile uint8_t reserved1[0x30];
	/* 0x80-0xbf: PUT_FLASH_NP Data Byte 0-63 */
	volatile uint8_t PUT_FLASH_NP_DATA[0x40];
};

#endif /* !__ASSEMBLER__ */


/**
 *
 * (3Axxh) SPI Slave Controller (SPISC) registers
 *
 */
#ifndef __ASSEMBLER__
struct spisc_it8xxx2_regs {
	/* 0x00: SPI Slave General Control */
	volatile uint8_t SPISC_SPISGCR;
	/* 0x01: Tx/Rx FIFO Access */
	volatile uint8_t SPISC_TXRXFAR;
	/* 0x02: Tx FIFO Control */
	volatile uint8_t SPISC_TXFCR;
	/* 0x03: SPI Slave General Control 2 */
	volatile uint8_t SPISC_SPISGCR2;
	/* 0x04: Interrupt Mask */
	volatile uint8_t SPISC_IMR;
	/* 0x05: Interrupt Status */
	volatile uint8_t SPISC_ISR;
	/* 0x06: Tx FIFO Status */
	volatile uint8_t SPISC_TXFSR;
	/* 0x07: Rx FIFO Status */
	volatile uint8_t SPISC_RXFSR;
	/* 0x08: CPU Write Tx FIFO Data Byte0 */
	volatile uint8_t SPISC_CPUWTXFDB0R;
	/* 0x09: FIFO Control / CPU Write Tx FIFO Data Byte1 */
	volatile uint8_t SPISC_FCR;
	/* 0x0A: CPU Write Tx FIFO Data Byte2 */
	volatile uint8_t SPISC_CPUWTXFDB2R;
	/* 0x0B: SPI Slave Response Data / CPU Write Tx FIFO Data Byte3 */
	volatile uint8_t SPISC_SPISRDR;
	/* 0x0C: Rx FIFO Readout Data Byte0 */
	volatile uint8_t SPISC_RXFRDRB0;
	/* 0x0D: Rx FIFO Readout Data Byte1 */
	volatile uint8_t SPISC_RXFRDRB1;
	/* 0x0E: Rx FIFO Readout Data Byte2 */
	volatile uint8_t SPISC_RXFRDRB2;
	/* 0x0F: Rx FIFO Readout Data Byte3 */
	volatile uint8_t SPISC_RXFRDRB3;
	/* 0x10-0x17: Reserved1 */
	volatile uint8_t reserved1[8];
	/* 0x18: FIFO Target Count Byte0 */
	volatile uint8_t SPISC_FTCB0R;
	/* 0x19: FIFO Target Count Byte1 */
	volatile uint8_t SPISC_FTCB1R;
	/* 0x1A: Target Count Capture Byte0 */
	volatile uint8_t SPISC_TCCB0;
	/* 0x1B: Target Count Capture Byte1 */
	volatile uint8_t SPISC_TCCB1;
	/* 0x1C-0x1D: Reserved2 */
	volatile uint8_t reserved2[2];
	/* 0x1E: Hardware Parsing 2 */
	volatile uint8_t SPISC_HPR2;
	/* 0x1F-0x25: Reserved3 */
	volatile uint8_t reserved3[7];
	/* 0x26: Rx Valid Length Interrupt Status Mask */
	volatile uint8_t SPISC_RXVLISMR;
	/* 0x27: Rx Valid Length Interrupt Status */
	volatile uint8_t SPISC_RXVLISR;
};
#endif /* !__ASSEMBLER__ */

/* SPISC register fields */
/* 0x00: SPI Slave General Control */
#define IT8XXX2_SPISC_SPISCEN		BIT(0)
/* 0x01: Tx/Rx FIFO Access */
#define IT8XXX2_SPISC_CPURXF1A		BIT(3)
#define IT8XXX2_SPISC_CPUTFA		BIT(1)
/* 0x02: Tx FIFO Control */
#define IT8XXX2_SPISC_TXFCMR		BIT(2)
#define IT8XXX2_SPISC_TXFR		BIT(1)
#define IT8XXX2_SPISC_TXFS		BIT(0)
/* 0x03: SPI Slave General Control 2 */
#define IT8XXX2_SPISC_RXF2OC		BIT(4)
#define IT8XXX2_SPISC_RXF1OC		BIT(3)
#define IT8XXX2_SPISC_RXFAR		BIT(0)
/* 0x04: Interrupt Mask */
#define IT8XXX2_SPISC_EDIM		BIT(2)
/* 0x06: Tx FIFO Status */
#define IT8XXX2_SPISC_ENDDETECTINT	BIT(2)
/* 0x09: FIFO Control */
#define IT8XXX2_SPISC_SPISRTXF		BIT(2)
#define IT8XXX2_SPISC_RXFR		BIT(1)
#define IT8XXX2_SPISC_RXFCMR		BIT(0)
/* 0x26: Rx Valid Length Interrupt Status Mask */
#define IT8XXX2_SPISC_RVLIM		BIT(0)
/* 0x27: Rx Valid Length Interrupt Status */
#define IT8XXX2_SPISC_RVLI		BIT(0)

#endif /* CHIP_CHIPREGS_H */
