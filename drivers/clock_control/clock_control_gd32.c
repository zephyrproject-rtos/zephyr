/*
 * Copyright (c) 2021 Tokita, Hiroshi <toita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gd32_rcu

#include <devicetree.h>
#include <device.h>

#include <soc.h>
#include <drivers/clock_control.h>
#include <drivers/clock_control/gd32_clock_control.h>
#include <sys/util.h>
#include <sys/__assert.h>

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(clock_control);

/*
 * register definitions
 */

union gd32_rcu_ctl {
	struct {
		volatile uint32_t IRC8MEN    : 1;       /*!< internal high speed oscillator enable */
		volatile uint32_t IRC8MSTB   : 1;       /*!< IRC8M high speed internal oscillator stabilization flag */
		volatile uint32_t _PAD0      : 1;       /*!< IRC8M high speed internal oscillator stabilization flag */
		volatile uint32_t IRC8MADJ   : 5;       /*!< high speed internal oscillator clock trim adjust value */
		volatile uint32_t IRC8MCALIB : 8;       /*!< high speed internal oscillator calibration value register */
		volatile uint32_t HXTALEN    : 1;       /*!< external high speed oscillator enable */
		volatile uint32_t HXTALSTB   : 1;       /*!< external crystal oscillator clock stabilization flag */
		volatile uint32_t HXTALBPS   : 1;       /*!< external crystal oscillator clock bypass mode enable */
		volatile uint32_t CKMEN      : 1;       /*!< HXTAL clock monitor enable */
		volatile uint32_t _PAD1      : 4;       /*!< PLL enable */
		volatile uint32_t PLLEN      : 1;       /*!< PLL enable */
		volatile uint32_t PLLSTB     : 1;       /*!< PLL clock stabilization flag */
		volatile uint32_t PLL1EN     : 1;       /*!< PLL1 enable */
		volatile uint32_t PLL1STB    : 1;       /*!< PLL1 clock stabilization flag */
		volatile uint32_t PLL2EN     : 1;       /*!< PLL2 enable */
		volatile uint32_t PLL2STB    : 1;       /*!< PLL2 clock stabilization flag */
	};
	volatile uint32_t ALL;
};

union gd32_rcu_cfg0 {
	struct {
		volatile uint32_t SCS        : 2;       /*!< system clock switch */
		volatile uint32_t SCSS       : 2;       /*!< system clock switch status */
		volatile uint32_t AHBPSC     : 4;       /*!< AHB prescaler selection */
		volatile uint32_t APB1PSC    : 3;       /*!< APB1 prescaler selection */
		volatile uint32_t APB2PSC    : 3;       /*!< APB2 prescaler selection */
		volatile uint32_t ADCPSC     : 2;       /*!< ADC prescaler selection */
		volatile uint32_t PLLSEL     : 1;       /*!< PLL clock source selection */
		volatile uint32_t PREDV0_LSB : 1;       /*!< the LSB of PREDV0 division factor */
		volatile uint32_t PLLMF      : 4;       /*!< PLL clock multiplication factor */
		volatile uint32_t USBFSPSC   : 2;       /*!< USBFS clock prescaler selection */
		volatile uint32_t CKOUT0SEL  : 4;       /*!< CKOUT0 clock source selection */
		volatile uint32_t ADCPSC_2   : 1;       /*!< bit 2 of ADCPSC */
		volatile uint32_t PLLMF_4    : 1;       /*!< bit 4 of PLLMF */
	};
	volatile uint32_t ALL;
};

union gd32_rcu_int {
	struct {
		volatile uint32_t IRC40KSTBIF : 1;      /*!< IRC40K stabilization interrupt flag */
		volatile uint32_t LXTALSTBIF : 1;       /*!< LXTAL stabilization interrupt flag */
		volatile uint32_t IRC8MSTBIF : 1;       /*!< IRC8M stabilization interrupt flag */
		volatile uint32_t HXTALSTBIF : 1;       /*!< HXTAL stabilization interrupt flag */
		volatile uint32_t PLLSTBIF   : 1;       /*!< PLL stabilization interrupt flag */
		volatile uint32_t PLL1STBIF  : 1;       /*!< PLL1 stabilization interrupt flag */
		volatile uint32_t PLL2STBIF  : 1;       /*!< PLL2 stabilization interrupt flag */
		volatile uint32_t CKMIF      : 1;       /*!< HXTAL clock stuck interrupt flag */
		volatile uint32_t IRC40KSTBIE : 1;      /*!< IRC40K stabilization interrupt enable */
		volatile uint32_t LXTALSTBIE : 1;       /*!< LXTAL stabilization interrupt enable */
		volatile uint32_t IRC8MSTBIE : 1;       /*!< IRC8M stabilization interrupt enable */
		volatile uint32_t HXTALSTBIE : 1;       /*!< HXTAL stabilization interrupt enable */
		volatile uint32_t PLLSTBIE   : 1;       /*!< PLL stabilization interrupt enable */
		volatile uint32_t PLL1STBIE  : 1;       /*!< PLL1 stabilization interrupt enable */
		volatile uint32_t PLL2STBIE  : 1;       /*!< PLL2 stabilization interrupt enable */
		volatile uint32_t IRC40KSTBIC : 1;      /*!< IRC40K stabilization interrupt clear */
		volatile uint32_t LXTALSTBIC : 1;       /*!< LXTAL stabilization interrupt clear */
		volatile uint32_t IRC8MSTBIC : 1;       /*!< IRC8M stabilization interrupt clear */
		volatile uint32_t HXTALSTBIC : 1;       /*!< HXTAL stabilization interrupt clear */
		volatile uint32_t PLLSTBIC   : 1;       /*!< PLL stabilization interrupt clear */
		volatile uint32_t PLL1STBIC  : 1;       /*!< PLL1 stabilization interrupt clear */
		volatile uint32_t PLL2STBIC  : 1;       /*!< PLL2 stabilization interrupt clear */
		volatile uint32_t CKMIC      : 1;       /*!< HXTAL clock stuck interrupt clear */
	};
	volatile uint32_t ALL;
};

union gd32_rcu_apb2rst {
	struct {
		volatile uint32_t AFRST      : 1;       /*!< alternate function I/O reset */
		volatile uint32_t _PAD0      : 1;       /*!< GPIO port A reset */
		volatile uint32_t PARST      : 1;       /*!< GPIO port A reset */
		volatile uint32_t PBRST      : 1;       /*!< GPIO port B reset */
		volatile uint32_t PCRST      : 1;       /*!< GPIO port C reset */
		volatile uint32_t PDRST      : 1;       /*!< GPIO port D reset */
		volatile uint32_t PERST      : 1;       /*!< GPIO port E reset */
		volatile uint32_t _PAD1      : 2;       /*!< ADC0 reset */
		volatile uint32_t ADC0RST    : 1;       /*!< ADC0 reset */
		volatile uint32_t ADC1RST    : 1;       /*!< ADC1 reset */
		volatile uint32_t TIMER0RST  : 1;       /*!< TIMER0 reset */
		volatile uint32_t SPI0RST    : 1;       /*!< SPI0 reset */
		volatile uint32_t _PAD2      : 1;       /*!< USART0 reset */
		volatile uint32_t USART0RST  : 1;       /*!< USART0 reset */
	};
	volatile uint32_t ALL;


};
union gd32_rcu_apb1rst {
	struct {
		volatile uint32_t TIMER1RST  : 1;       /*!< TIMER1 reset */
		volatile uint32_t TIMER2RST  : 1;       /*!< TIMER2 reset */
		volatile uint32_t TIMER3RST  : 1;       /*!< TIMER3 reset */
		volatile uint32_t TIMER4RST  : 1;       /*!< TIMER4 reset */
		volatile uint32_t TIMER5RST  : 1;       /*!< TIMER5 reset */
		volatile uint32_t TIMER6RST  : 1;       /*!< TIMER6 reset */
		volatile uint32_t _PAD0      : 5;       /*!< TIMER6 reset */
		volatile uint32_t WWDGTRST   : 1;       /*!< WWDGT reset */
		volatile uint32_t SPI1RST    : 1;       /*!< SPI1 reset */
		volatile uint32_t SPI2RST    : 1;       /*!< SPI2 reset */
		volatile uint32_t USART1RST  : 1;       /*!< USART1 reset */
		volatile uint32_t USART2RST  : 1;       /*!< USART2 reset */
		volatile uint32_t UART3RST   : 1;       /*!< UART3 reset */
		volatile uint32_t UART4RST   : 1;       /*!< UART4 reset */
		volatile uint32_t I2C0RST    : 1;       /*!< I2C0 reset */
		volatile uint32_t I2C1RST    : 1;       /*!< I2C1 reset */
		volatile uint32_t CAN0RST    : 1;       /*!< CAN0 reset */
		volatile uint32_t CAN1RST    : 1;       /*!< CAN1 reset */
		volatile uint32_t BKPIRST    : 1;       /*!< backup interface reset */
		volatile uint32_t PMURST     : 1;       /*!< PMU reset */
		volatile uint32_t DACRST     : 1;       /*!< DAC reset */
	};
	volatile uint32_t ALL;
};

union gd32_rcu_ahben {
	struct {
		volatile uint32_t DMA0EN     : 1;       /*!< DMA0 clock enable */
		volatile uint32_t DMA1EN     : 1;       /*!< DMA1 clock enable */
		volatile uint32_t SRAMSPEN   : 1;       /*!< SRAM clock enable when sleep mode */
		volatile uint32_t FMCSPEN    : 1;       /*!< FMC clock enable when sleep mode */
		volatile uint32_t CRCEN      : 1;       /*!< CRC clock enable */
		volatile uint32_t EXMCEN     : 1;       /*!< EXMC clock enable */
		volatile uint32_t _PAD0      : 3;       /*!< EXMC clock enable */
		volatile uint32_t USBFSEN    : 1;       /*!< USBFS clock enable */
	};
	volatile uint32_t ALL;
};

union gd32_rcu_apb2en {
	struct {
		volatile uint32_t AFEN       : 1;       /*!< alternate function IO clock enable */
		volatile uint32_t PAEN       : 1;       /*!< GPIO port A clock enable */
		volatile uint32_t PBEN       : 1;       /*!< GPIO port B clock enable */
		volatile uint32_t PCEN       : 1;       /*!< GPIO port C clock enable */
		volatile uint32_t PDEN       : 1;       /*!< GPIO port D clock enable */
		volatile uint32_t PEEN       : 1;       /*!< GPIO port E clock enable */
		volatile uint32_t ADC0EN     : 1;       /*!< ADC0 clock enable */
		volatile uint32_t ADC1EN     : 1;       /*!< ADC1 clock enable */
		volatile uint32_t TIMER0EN   : 1;       /*!< TIMER0 clock enable */
		volatile uint32_t SPI0EN     : 1;       /*!< SPI0 clock enable */
		volatile uint32_t _PAD0      : 1;       /*!< SPI0 clock enable */
		volatile uint32_t USART0EN   : 1;       /*!< USART0 clock enable */
	};
	volatile uint32_t ALL;
};

union gd32_rcu_apb1en {
	struct {
		volatile uint32_t TIMER1EN   : 1;       /*!< TIMER1 clock enable */
		volatile uint32_t TIMER2EN   : 1;       /*!< TIMER2 clock enable */
		volatile uint32_t TIMER3EN   : 1;       /*!< TIMER3 clock enable */
		volatile uint32_t TIMER4EN   : 1;       /*!< TIMER4 clock enable */
		volatile uint32_t TIMER5EN   : 1;       /*!< TIMER5 clock enable */
		volatile uint32_t TIMER6EN   : 1;       /*!< TIMER6 clock enable */
		volatile uint32_t _PAD0      : 5;       /*!< EXMC clock enable */
		volatile uint32_t WWDGTEN    : 1;       /*!< WWDGT clock enable */
		volatile uint32_t SPI1EN     : 1;       /*!< SPI1 clock enable */
		volatile uint32_t SPI2EN     : 1;       /*!< SPI2 clock enable */
		volatile uint32_t USART1EN   : 1;       /*!< USART1 clock enable */
		volatile uint32_t USART2EN   : 1;       /*!< USART2 clock enable */
		volatile uint32_t UART3EN    : 1;       /*!< UART3 clock enable */
		volatile uint32_t UART4EN    : 1;       /*!< UART4 clock enable */
		volatile uint32_t I2C0EN     : 1;       /*!< I2C0 clock enable */
		volatile uint32_t I2C1EN     : 1;       /*!< I2C1 clock enable */
		volatile uint32_t CAN0EN     : 1;       /*!< CAN0 clock enable */
		volatile uint32_t CAN1EN     : 1;       /*!< CAN1 clock enable */
		volatile uint32_t BKPIEN     : 1;       /*!< backup interface clock enable */
		volatile uint32_t PMUEN      : 1;       /*!< PMU clock enable */
		volatile uint32_t DACEN      : 1;       /*!< DAC clock enable */
	};
	volatile uint32_t ALL;
};

union gd32_rcu_bdctl {
	struct {
		volatile uint32_t LXTALEN    : 1;       /*!< LXTAL enable */
		volatile uint32_t LXTALSTB   : 1;       /*!< low speed crystal oscillator stabilization flag */
		volatile uint32_t LXTALBPS   : 1;       /*!< LXTAL bypass mode enable */
		volatile uint32_t _PAD0      : 5;       /*!< EXMC clock enable */
		volatile uint32_t RTCSRC     : 2;       /*!< RTC clock entry selection */
		volatile uint32_t _PAD1      : 5;       /*!< EXMC clock enable */
		volatile uint32_t RTCEN      : 1;       /*!< RTC clock enable */
		volatile uint32_t BKPRST     : 1;       /*!< backup domain reset */

	};
	volatile uint32_t ALL;
};

union gd32_rcu_rstsck {
	struct {
		/* RCU_RSTSCK */
		volatile uint32_t IRC40KEN   : 1;       /*!< IRC40K enable */
		volatile uint32_t IRC40KSTB  : 1;       /*!< IRC40K stabilization flag */
		volatile uint32_t _PAD0      : 20;      /*!< EXMC clock enable */
		volatile uint32_t RSTFC      : 1;       /*!< reset flag clear */
		volatile uint32_t _PAD1      : 1;       /*!< EXMC clock enable */
		volatile uint32_t EPRSTF     : 1;       /*!< external pin reset flag */
		volatile uint32_t PORRSTF    : 1;       /*!< power reset flag */
		volatile uint32_t SWRSTF     : 1;       /*!< software reset flag */
		volatile uint32_t FWDGTRSTF  : 1;       /*!< free watchdog timer reset flag */
		volatile uint32_t WWDGTRSTF  : 1;       /*!< window watchdog timer reset flag */
		volatile uint32_t LPRSTF     : 1;       /*!< low-power reset flag */
	};
	volatile uint32_t ALL;
};

union gd32_rcu_ahbrst {
	struct {
		volatile uint32_t USBFSRST   : 1;   /*!< USBFS reset */
	};
	volatile uint32_t ALL;
};

union gd32_rcu_cfg1 {
	struct {
		volatile uint32_t PREDV0     : 4;       /*!< PREDV0 division factor */
		volatile uint32_t PREDV1     : 4;       /*!< PREDV1 division factor */
		volatile uint32_t PLL1MF     : 4;       /*!< PLL1 clock multiplication factor */
		volatile uint32_t PLL2MF     : 4;       /*!< PLL2 clock multiplication factor */
		volatile uint32_t PREDV0SEL  : 1;       /*!< PREDV0 input clock source selection */
		volatile uint32_t I2S1SEL    : 1;       /*!< I2S1 clock source selection */
		volatile uint32_t I2S2SEL    : 1;       /*!< I2S2 clock source selection  */
		volatile uint32_t PAD        : 13;
	};
	volatile uint32_t ALL;
};

union gd32_rcu_dsv {
	struct {
		volatile uint32_t DSLPVS     : 2;   /*!< deep-sleep mode voltage select */
		volatile uint32_t PAD        : 30;
	};
	volatile uint32_t ALL;
};

struct gd32_rcu {
	volatile union gd32_rcu_ctl CTL;
	volatile union gd32_rcu_cfg0 CFG0;
	volatile union gd32_rcu_int INT;
	volatile union gd32_rcu_apb2rst APB2RST;
	volatile union gd32_rcu_apb1rst APB1RST;
	volatile union gd32_rcu_ahben AHBEN;
	volatile union gd32_rcu_apb1en APB1EN;
	volatile union gd32_rcu_bdctl BDCTL;
	volatile union gd32_rcu_rstsck RSTSCK;
	volatile union gd32_rcu_ahbrst AHBRST;
	volatile uint32_t PAD;
	volatile union gd32_rcu_cfg1 CFG1;
	volatile union gd32_rcu_dsv DSV;
};


#define GD32_RCU_REG(periph)                 (REG32(RCU_BASE + ((uint32_t)(periph))))
#define RCU ((volatile struct gd32_rcu *)RCU_BASE)

/* define clock source */
#define SEL_IRC8M                   0UL
#define SEL_HXTAL                   1UL
#define SEL_PLL                     2UL

#if !defined  HXTAL_VALUE
#define HXTAL_VALUE    8000000UL /*!< value of the external oscillator in Hz */
#define HXTAL_VALUE_25M  HXTAL_VALUE
#endif

#if !defined  (HXTAL_STARTUP_TIMEOUT)
#define HXTAL_STARTUP_TIMEOUT   0xFFFFUL /*!< high speed crystal oscillator startup timeout */
#endif

#if !defined  (IRC8M_VALUE)
#define IRC8M_VALUE    8000000UL /*!< internal 8MHz RC oscillator value */
#endif

#define PLLSRC_IRC8M_DIV2           0UL         /*!< IRC8M/2 clock selected as source clock of PLL */
#define PLLSRC_HXTAL                1UL         /*!< HXTAL clock selected as source clock of PLL */

#define PREDV0SRC_HXTAL             0UL         /*!< HXTAL selected as PREDV0 input source clock */
#define PREDV0SRC_CKPLL1            1UL         /*!< CK_PLL1 selected as PREDV0 input source clock */

#define PLL_MUL2                    0UL         /*!< PLL source clock multiply by 2 */
#define PLL_MUL3                    1UL         /*!< PLL source clock multiply by 3 */
#define PLL_MUL4                    2UL         /*!< PLL source clock multiply by 4 */
#define PLL_MUL5                    3UL         /*!< PLL source clock multiply by 5 */
#define PLL_MUL6                    4UL         /*!< PLL source clock multiply by 6 */
#define PLL_MUL7                    5UL         /*!< PLL source clock multiply by 7 */
#define PLL_MUL8                    6UL         /*!< PLL source clock multiply by 8 */
#define PLL_MUL9                    7UL         /*!< PLL source clock multiply by 9 */
#define PLL_MUL10                   8UL         /*!< PLL source clock multiply by 10 */
#define PLL_MUL11                   9UL         /*!< PLL source clock multiply by 11 */
#define PLL_MUL12                   10UL        /*!< PLL source clock multiply by 12 */
#define PLL_MUL13                   11UL        /*!< PLL source clock multiply by 13 */
#define PLL_MUL14                   12UL        /*!< PLL source clock multiply by 14 */
#define PLL_MUL6_5                  13UL        /*!< PLL source clock multiply by 6.5 */
#define PLL_MUL16                   14UL        /*!< PLL source clock multiply by 16 */
#define PLL_MUL17_4                   0UL       /*!< PLL source clock multiply by 17 */
#define PLL_MUL18_4                   1UL       /*!< PLL source clock multiply by 18 */
#define PLL_MUL19_4                   2UL       /*!< PLL source clock multiply by 19 */
#define PLL_MUL20_4                   3UL       /*!< PLL source clock multiply by 20 */
#define PLL_MUL21_4                   4UL       /*!< PLL source clock multiply by 21 */
#define PLL_MUL22_4                   5UL       /*!< PLL source clock multiply by 22 */
#define PLL_MUL23_4                   6UL       /*!< PLL source clock multiply by 23 */
#define PLL_MUL24_4                   7UL       /*!< PLL source clock multiply by 24 */
#define PLL_MUL25_4                   8UL       /*!< PLL source clock multiply by 25 */
#define PLL_MUL26_4                   9UL       /*!< PLL source clock multiply by 26 */
#define PLL_MUL27_4                   10UL      /*!< PLL source clock multiply by 27 */
#define PLL_MUL28_4                   11UL      /*!< PLL source clock multiply by 28 */
#define PLL_MUL29_4                   12UL      /*!< PLL source clock multiply by 29 */
#define PLL_MUL30_4                   13UL      /*!< PLL source clock multiply by 30 */
#define PLL_MUL31_4                   14UL      /*!< PLL source clock multiply by 31 */
#define PLL_MUL32_4                   15UL      /*!< PLL source clock multiply by 32 */
#define PLL1_MUL8                   6UL         /*!< PLL1 source clock multiply by 8 */
#define PLL1_MUL9                   7UL         /*!< PLL1 source clock multiply by 9 */
#define PLL1_MUL10                  8UL         /*!< PLL1 source clock multiply by 10 */
#define PLL1_MUL11                  9UL         /*!< PLL1 source clock multiply by 11 */
#define PLL1_MUL12                  10UL        /*!< PLL1 source clock multiply by 12 */
#define PLL1_MUL13                  11UL        /*!< PLL1 source clock multiply by 13 */
#define PLL1_MUL14                  12UL        /*!< PLL1 source clock multiply by 14 */
#define PLL1_MUL15                  13UL        /*!< PLL1 source clock multiply by 15 */
#define PLL1_MUL16                  14UL        /*!< PLL1 source clock multiply by 16 */
#define PLL1_MUL20                  15UL        /*!< PLL1 source clock multiply by 20 */
#define PLL2_MUL8  PLL1_MUL8
#define PLL2_MUL9  PLL1_MUL9
#define PLL2_MUL10 PLL1_MUL10
#define PLL2_MUL11 PLL1_MUL11
#define PLL2_MUL12 PLL1_MUL12
#define PLL2_MUL13 PLL1_MUL13
#define PLL2_MUL14 PLL1_MUL14
#define PLL2_MUL15 PLL1_MUL15
#define PLL2_MUL16 PLL1_MUL16
#define PLL2_MUL17 PLL1_MUL17
#define PLL2_MUL18 PLL1_MUL18
#define PLL2_MUL19 PLL1_MUL19
#define PLL2_MUL20 PLL1_MUL20

#define DIV1                0UL                                 /*!< PREDV1 input source clock not divided */
#define DIV2                1UL                                 /*!< PREDV1 input source clock divided by 2 */
#define DIV3                2UL                                 /*!< PREDV1 input source clock divided by 3 */
#define DIV4                3UL                                 /*!< PREDV1 input source clock divided by 4 */
#define DIV5                4UL                                 /*!< PREDV1 input source clock divided by 5 */
#define DIV6                5UL                                 /*!< PREDV1 input source clock divided by 6 */
#define DIV7                6UL                                 /*!< PREDV1 input source clock divided by 7 */
#define DIV8                7UL                                 /*!< PREDV1 input source clock divided by 8 */
#define DIV9                8UL                                 /*!< PREDV1 input source clock divided by 9 */
#define DIV10               9UL                                 /*!< PREDV1 input source clock divided by 10 */
#define DIV11               10UL                                /*!< PREDV1 input source clock divided by 11 */
#define DIV12               11UL                                /*!< PREDV1 input source clock divided by 12 */
#define DIV13               12UL                                /*!< PREDV1 input source clock divided by 13 */
#define DIV14               13UL                                /*!< PREDV1 input source clock divided by 14 */
#define DIV15               14UL                                /*!< PREDV1 input source clock divided by 15 */
#define DIV16               15UL                                /*!< PREDV1 input source clock divided by 16 */

/* AHB prescaler selection */
#define CKSYS_DIV1              0UL                             /*!< AHB prescaler select CK_SYS */
#define CKSYS_DIV2              8UL                             /*!< AHB prescaler select CK_SYS/2 */
#define CKSYS_DIV4              9UL                             /*!< AHB prescaler select CK_SYS/4 */
#define CKSYS_DIV8              10UL                            /*!< AHB prescaler select CK_SYS/8 */
#define CKSYS_DIV16             11UL                            /*!< AHB prescaler select CK_SYS/16 */
#define CKSYS_DIV64             12UL                            /*!< AHB prescaler select CK_SYS/64 */
#define CKSYS_DIV128            13UL                            /*!< AHB prescaler select CK_SYS/128 */
#define CKSYS_DIV256            14UL                            /*!< AHB prescaler select CK_SYS/256 */
#define CKSYS_DIV512            15UL                            /*!< AHB prescaler select CK_SYS/512 */

/* APB1/APB2 prescaler selection */
#define CKAHB_DIV1             0UL                              /*!< APB2 prescaler select CK_AHB */
#define CKAHB_DIV2             4UL                              /*!< APB2 prescaler select CK_AHB/2 */
#define CKAHB_DIV4             5UL                              /*!< APB2 prescaler select CK_AHB/4 */
#define CKAHB_DIV8             6UL                              /*!< APB2 prescaler select CK_AHB/8 */
#define CKAHB_DIV16            7UL                              /*!< APB2 prescaler select CK_AHB/16 */

/* RCU_CFG0 register bit define */
/* system clock source select */
#define CKSYSSRC_IRC8M              0UL                         /*!< system clock source select IRC8M */
#define CKSYSSRC_HXTAL              1UL                         /*!< system clock source select HXTAL */
#define CKSYSSRC_PLL                2UL                         /*!< system clock source select PLL */

/* system clock source select status */
#define SCSS_IRC8M                  0UL                         /*!< system clock source select IRC8M */
#define SCSS_HXTAL                  1UL                         /*!< system clock source select HXTAL */
#define SCSS_PLL                    2UL                         /*!< system clock source select PLLP */


static inline void periph_clock_enable(uint32_t bus, uint32_t periph)
{
	GD32_RCU_REG(bus) |= BIT(periph);
}

static inline void periph_clock_disable(uint32_t bus, uint32_t periph)
{
	GD32_RCU_REG(bus) &= ~BIT(periph);
}

static uint32_t pll_clock_freq( )
{
	uint32_t pllsel, pllmf, ck_src, cksys_freq;

	/* PLL clock source selection, HXTAL or IRC8M/2 */
	pllsel = (RCU->CFG0.PLLSEL);

	if (PLLSRC_HXTAL == pllsel) {
		/* PLL clock source is HXTAL */
		ck_src = HXTAL_VALUE;

		/* source clock use PLL1 */
		if (PREDV0SRC_CKPLL1 == RCU->CFG1.PREDV0SEL) {
			uint32_t pll1mf = RCU->CFG1.PLL1MF + 2U;
			if (17U == pll1mf) {
				pll1mf = 20U;
			}
			ck_src = (ck_src / (RCU->CFG1.PREDV1 + 1U)) * pll1mf;
		}
		ck_src /= ((RCU->CFG1.PREDV0) + 1U);
	} else {
		/* PLL clock source is IRC8M/2 */
		ck_src = IRC8M_VALUE / 2U;
	}

	/* PLL multiplication factor */
	pllmf = RCU->CFG0.PLLMF;
	if ((RCU->CFG0.PLLMF_4)) {
		pllmf |= 0x10U;
	}
	if (pllmf < 15U) {
		pllmf += 2U;
	} else {
		pllmf += 1U;
	}

	cksys_freq = ck_src * pllmf;

	if (15U == pllmf) {
		/* PLL source clock multiply by 6.5 */
		cksys_freq = ck_src * 6U + ck_src / 2U;
	}

	return cksys_freq;
}

static uint32_t clock_freq_get(uint32_t clock)
{
	uint32_t sws, ck_freq = 0U;
	uint32_t cksys_freq, ahb_freq, apb1_freq, apb2_freq, idx, clk_exp;

	/* exponent of AHB, APB1 and APB2 clock divider */
	static const uint8_t ahb_exp[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9 };
	static const uint8_t apb1_exp[8] = { 0, 0, 0, 0, 1, 2, 3, 4 };
	static const uint8_t apb2_exp[8] = { 0, 0, 0, 0, 1, 2, 3, 4 };

	sws = RCU->CFG0.SCSS;
	switch (sws) {
	/* PLL is selected as CK_SYS */
	case SEL_PLL:
		cksys_freq = pll_clock_freq();
		break;
	/* HXTAL is selected as CK_SYS */
	case SEL_HXTAL:
		cksys_freq = HXTAL_VALUE;
		break;
	/* IRC8M is selected as CK_SYS */
	case SEL_IRC8M:
	default:
		cksys_freq = IRC8M_VALUE;
		break;
	}

	/* calculate AHB clock frequency */
	idx = RCU->CFG0.AHBPSC;
	clk_exp = ahb_exp[idx];
	ahb_freq = cksys_freq >> clk_exp;

	/* calculate APB1 clock frequency */
	idx = RCU->CFG0.APB1PSC;
	clk_exp = apb1_exp[idx];
	apb1_freq = ahb_freq >> clk_exp;

	/* calculate APB2 clock frequency */
	idx = RCU->CFG0.APB2PSC;
	clk_exp = apb2_exp[idx];
	apb2_freq = ahb_freq >> clk_exp;

	/* return the clocks frequency */
	switch (clock) {
	case GD32_CLOCK_BUS_SYS:
		ck_freq = cksys_freq;
		break;
	case GD32_CLOCK_BUS_AHB:
		ck_freq = ahb_freq;
		break;
	case GD32_CLOCK_BUS_APB1:
		ck_freq = apb1_freq;
		break;
	case GD32_CLOCK_BUS_APB2:
		ck_freq = apb2_freq;
		break;
	default:
		break;
	}
	return ck_freq;
}

static int gd32_clock_control_on(const struct device *dev,
				 clock_control_subsys_t sub_system)
{
	struct gd32_pclken *pclken = (struct gd32_pclken *)sub_system;

	periph_clock_enable(pclken->bus, pclken->enr);

	return 0;
}

static int gd32_clock_control_off(const struct device *dev,
				  clock_control_subsys_t sub_system)
{
	struct gd32_pclken *pclken = (struct gd32_pclken *)sub_system;

	periph_clock_disable(pclken->bus, pclken->enr);
	return 0;
}

static int gd32_clock_control_get_subsys_rate(const struct device *dev,
					      clock_control_subsys_t sub_system,
					      uint32_t *rate)
{
	struct gd32_pclken *pclken = (struct gd32_pclken *)sub_system;

	*rate = clock_freq_get(pclken->bus);

	return 0;
}

static struct clock_control_driver_api gd32_clock_control_api = {
	.on = gd32_clock_control_on,
	.off = gd32_clock_control_off,
	.get_rate = gd32_clock_control_get_subsys_rate,
};

/**
 * Initialize clocks
 */
static int gd32_clock_control_init(const struct device *dev)
{
	uint32_t timeout = 0U;
	uint32_t stab_flag = 0U;

	/* enable IRC8M */
	RCU->CTL.IRC8MEN = 1;

	/* resetting */
	RCU->CFG0.SCS = 0;
	RCU->CFG0.AHBPSC = 0;
	RCU->CFG0.APB1PSC = 0;
	RCU->CFG0.APB2PSC = 0;
	RCU->CFG0.ADCPSC = 0;
	RCU->CFG0.ADCPSC_2 = 0;
	RCU->CFG0.CKOUT0SEL = 0;
	RCU->CFG0.PLLSEL = 0;
	RCU->CFG0.PREDV0_LSB = 0;
	RCU->CFG0.PLLMF = 0;
	RCU->CFG0.USBFSPSC = 0;
	RCU->CFG0.PLLMF_4 = 0;
	RCU->CFG1.ALL = 0x00000000U;
	RCU->CTL.PLLEN = 0;
	RCU->CTL.CKMEN = 0;
	RCU->CTL.HXTALEN = 0;
	RCU->CTL.HXTALBPS = 0;
	RCU->CTL.PLLEN = 0;
	RCU->CTL.PLL1EN = 0;
	RCU->CTL.PLL2EN = 0;
	RCU->CTL.CKMEN = 0;
	RCU->CTL.HXTALEN = 0;

	/* disable all interrupts */
	RCU->INT.ALL = 0x00FF0000U;

	/* enable HXTAL */
	RCU->CTL.HXTALEN = 1;

	/* wait until HXTAL is stable or the startup time is */
	/* longer than HXTAL_STARTUP_TIMEOUT                 */
	do {
		timeout++;
		stab_flag = (RCU->CTL.HXTALSTB);
	} while ((!stab_flag) && (timeout < HXTAL_STARTUP_TIMEOUT));

	/* if fail */
	if (!RCU->CTL.HXTALSTB) {
		while (1) {
		}
	}

	RCU->CFG0.AHBPSC = CKSYS_DIV1;
	RCU->CFG0.APB2PSC = CKAHB_DIV1;
	RCU->CFG0.APB1PSC = CKAHB_DIV2;

	/* CK_PLL = (CK_PREDIV0) * 27 = 108 MHz */
	RCU->CFG0.PLLSEL = PLLSRC_HXTAL;
	RCU->CFG0.PLLMF_4 = 1;
	RCU->CFG0.PLLMF = PLL_MUL27_4;

	/* HXTAL = 8000000 */

	RCU->CFG1.PREDV0SEL = 0;
	RCU->CFG1.PREDV0 = DIV2;
	RCU->CFG1.PREDV1 = DIV2;
	RCU->CFG1.PLL1MF = PLL1_MUL20;
	RCU->CFG1.PLL2MF = PLL1_MUL20;

	/* enable PLL1 */
	RCU->CTL.PLL1EN = 1;
	/* wait till PLL1 is ready */
	while (!RCU->CTL.PLL1STB) {
	}

	/* enable PLL2 */
	RCU->CTL.PLL2EN = 1;
	/* wait till PLL1 is ready */
	while (!RCU->CTL.PLL2STB) {
	}

	/* enable PLL */
	RCU->CTL.PLLEN = 1;

	/* wait until PLL is stable */
	while (!RCU->CTL.PLLSTB) {
	}

	/* select PLL as system clock */
	RCU->CFG0.SCS = CKSYSSRC_PLL;
	/* wait until PLL is selected as system clock */
	while (!(RCU->CFG0.SCSS & SCSS_PLL)) {
	}

	return 0;
}

/**
 * @brief RCU device, note that priority is intentionally set to 1 so
 * that the device init runs just after SOC init
 */
DEVICE_DT_DEFINE(DT_NODELABEL(rcu),
		 &gd32_clock_control_init,
		 NULL,
		 NULL, NULL,
		 PRE_KERNEL_1,
		 CONFIG_KERNEL_INIT_PRIORITY_OBJECTS,
		 &gd32_clock_control_api);
