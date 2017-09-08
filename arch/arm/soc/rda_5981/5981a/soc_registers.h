/*
 * Copyright (c) 2017 RDA
 * Copyright (c) 2016-2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _RDA5981a_SOC_REGISTERS_H_
#define _RDA5981a_SOC_REGISTERS_H_

/*------------- Watchdog Timer (WDT) ---------------------------------------*/
struct wdt_rda5981a {
	uint32_t wdt_cfg;
};

/* include register mapping headers */
struct uart_rda5981a {
	  union {
		uint32_t rbr;	/* 0x00 : UART Receive buffer register      */
		uint32_t thr;	/* 0x00 : UART Transmit holding register    */
		uint32_t dll;	/* 0x00 : UART Divisor latch(low)           */
	  };
	  union {
		uint32_t dlh;	/* 0x04 : UART Divisor latch(high)          */
		uint32_t ier;	/* 0x04 : UART Interrupt enable register    */
	  };
	  union {
		uint32_t iir;	/* 0x08 : UART Interrupt id register        */
		uint32_t fcr;	/* 0x08 : UART Fifo control register        */
	  };
	uint32_t lcr;		/* 0x0C : UART Line control register        */
	uint32_t mcr;		/* 0x10 : UART Moderm control register      */
	uint32_t lsr;		/* 0x14 : UART Line status register         */
	uint32_t msr;		/* 0x18 : UART Moderm status register       */
	uint32_t scr;		/* 0x1C : UART Scratchpad register          */
	uint32_t fsr;		/* 0x20 : UART FIFO status register         */
	uint32_t frr;		/* 0x24 : UART FIFO tx/rx trigger resiger   */
	uint32_t dl2;		/* 0x28 : UART Baud rate adjust register    */
	uint32_t reserved[4];
	uint32_t baud;		/* 0x3C : UART Auto baud counter            */
	uint32_t dl_slow;	/* 0x40 : UART Divisor Adjust when slow clk */
	uint32_t dl_fast;	/* 0x44 : UART Divisor Adjust when fast clk */
};

/*------------- General Purpose Input/Output (GPIO) ------------------------*/
struct gpio_rda5981a {
	uint32_t ctrl;                   /* 0x00 : GPIO Control */
	uint32_t reserved0;
	uint32_t dout;                   /* 0x08 : GPIO Data Output          */
	uint32_t din;                    /* 0x0C : GPIO Data Input           */
	uint32_t dir;                    /* 0x10 : GPIO Direction            */
	uint32_t slew0;                  /* 0x14 : GPIO Slew Config 0        */
	uint32_t slewiomux;              /* 0x18 : GPIO IOMUX Slew Config    */
	uint32_t intctrl;                /* 0x1C : GPIO Interrupt Control    */
	uint32_t ifctrl;                 /* 0x20 : Interface Control         */
	uint32_t slew1;                  /* 0x24 : GPIO Slew Config 1        */
	uint32_t revid;                  /* 0x28 : ASIC Reversion ID         */
	uint32_t lposel;                 /* 0x2C : LPO Select                */
	uint32_t reserved1;
	uint32_t intsel;                 /* 0x34 : GPIO Interrupt Select     */
	uint32_t reserved2;
	uint32_t sdiocfg;                /* 0x3C : SDIO Config               */
	uint32_t memcfg;                 /* 0x40 : Memory Config             */
	uint32_t iomuxctrl[8];           /* 0x44 - 0x60 : IOMUX Control      */
	uint32_t reserved3;
};

struct pincfg_rda5981a {
	union {
		uint32_t iomuxctrl[8];
		struct {
			uint32_t mux0;
			uint32_t mux1;
			uint32_t mode0;
			uint32_t mode1;
			uint32_t mux2;
			uint32_t mux3;
			uint32_t mode2;
			uint32_t mode3;
		};
	};
};

struct scu_ctrl {
	uint32_t clkgate0;             /* 0x00 : Clock Gating 0               */
	uint32_t pwrctrl;              /* 0x04 : Power Control                */
	uint32_t clkgate1;             /* 0x08 : Clock Gating 1               */
	uint32_t clkgate2;             /* 0x0C : Clock Gating 2               */
	uint32_t resetctrl;            /* 0x10 : Power Control                */
	uint32_t clkgate3;             /* 0x14 : Clock Gating 3               */
	uint32_t corecfg;              /* 0x18 : Core Config                  */
	uint32_t cpucfg;               /* 0x1C : CPU Config                   */
	uint32_t ftmrinival;           /* 0x20 : Free Timer Initial Value     */
	uint32_t ftmrts;               /* 0x24 : Free Timer Timestamp         */
	uint32_t clkgatebp;            /* 0x28 : Clock Gating Bypass          */
	uint32_t reserved[2];
	uint32_t pwmcfg;               /* 0x34 : PWM Config                   */
	uint32_t func0wakeval;         /* 0x38 : SDIO Func0 Wake Val          */
	uint32_t func1wakeval;         /* 0x3C : SDIO Func1 Wake Val          */
	uint32_t bootjumpaddr;         /* 0x40 : Boot Jump Addr               */
	uint32_t sdiointval;           /* 0x44 : SDIO Int Val                 */
	uint32_t i2sclkdiv;            /* 0x48 : I2S Clock Divider            */
	uint32_t bootjumpaddrcfg;      /* 0x4C : Boot Jump Addr Config        */
	uint32_t ftmrpreval;           /* 0x50 : Free Timer Prescale Init Val */
	uint32_t pwropencfg;           /* 0x54 : Power Open Config            */
	uint32_t pwrclosecfg;          /* 0x58 : Power Close Config           */
 } ;

struct spi_rda5981a {
	uint32_t cfg;
	uint32_t d0cmd;
	uint32_t d1cmd;
};

struct i2s_rda5981a {
	uint32_t cfg;
	uint32_t doutwr;
	uint32_t dinrd;
};

#define EXIF_MISC_INT_RFU(x)		((x & 0x1) << 15)

struct exif_rda5981a {
	struct spi_rda5981a spi0;      /* 0x00 - 0x08 : spi0 registers group  */
	struct i2s_rda5981a i2s;       /* 0x0c - 0x14 : i2s registers group   */
	uint32_t misc_st_cfg;          /* 0x18 : misc status config register  */
	uint32_t spi1ctrl;             /* 0x1c : spi1 control register        */
	uint32_t reserved0[4];
	uint32_t misc_int_cfg;         /* 0x30 : misc int config register     */
	uint32_t mbb2w;                /* 0x34 : bt to wifi mailbox register  */
	uint32_t mbw2b;                /* 0x38 : wifi to bt mailbox register  */
	uint32_t misc_cfg;             /* 0x3c : misc configure register      */
	uint32_t pwm0_cfg;             /* 0x40 : pwm0 configure register      */
	uint32_t pwm1_cfg;             /* 0x44 : pwm1 configure register      */
	uint32_t pwm2_cfg;             /* 0x48 : pwm2 configure register      */
	uint32_t pwm3_cfg;             /* 0x4c : pwm3 configure register      */
};

/*------------- AHB Direct Memory Access (DMA) -------------------------------*/
struct dma_cfg_rda5981h {
	uint32_t dma_ctrl;             /* 0x00 : DMA ctrl                     */
	uint32_t dma_src;              /* 0x04 : DMA src                      */
	uint32_t dma_dst;              /* 0x08 : DMA dst                      */
	uint32_t dma_len;              /* 0x0c : DMA len                      */
	uint32_t crc_gen;              /* 0x10 : CRC gen                      */
	uint32_t dma_func_ctrl;        /* 0x14 : DMA func ctrl                */
	uint32_t aes_key0;             /* 0x18 : AES key 0                    */
	uint32_t aes_key1;             /* 0x1c : AES key 1                    */
	uint32_t aes_key2;             /* 0x20 : AES key 2                    */
	uint32_t aes_key3;             /* 0x24 : AES key 2                    */
	uint32_t aes_iv0;              /* 0x28 : AES iv 0                     */
	uint32_t aes_iv1;              /* 0x2c : AES iv 1                     */
	uint32_t aes_iv2;              /* 0x30 : AES iv 2                     */
	uint32_t aes_iv3;              /* 0x34 : AES iv 2                     */
	uint32_t aes_mode;             /* 0x38 : AES mode                     */
	uint32_t cios_ctrl;            /* 0x3c : cios ctrl                    */
	uint32_t cios_reg0;            /* 0x40 : cios reg 0                   */
	uint32_t crc_init_val;         /* 0x44 : CRC init val                 */
	uint32_t crc_out_xorval;       /* 0x48 : CRC out xorval               */
	uint32_t crc_out_val;          /* 0x4c : CRC out val                  */
	uint32_t RESERVED0[12];
	uint32_t dma_int_out;          /* 0x80 : DMA int out                  */
	uint32_t dma_int_mask;         /* 0x84 : DMA int mask                 */
	uint32_t RESERVED1[30];
	uint32_t trng_ctrl;            /* 0x100 : TRNG ctrl                   */
	uint32_t prng_ctrl;            /* 0x104 : PRNG ctrl                   */
	uint32_t prng_seed;            /* 0x108 : PRNG seed                   */
	uint32_t prng_timer_int;       /* 0x10c : PRNG timer init             */
	uint32_t prng_timer;           /* 0x110 : PRNG timer                  */
	uint32_t trng_data0;           /* 0x114 : TRNG data 0                 */
	uint32_t trng_data0_mask;      /* 0x118 : TRNG data 0 mask            */
	uint32_t trng_data1;           /* 0x11c : TRNG data 1                 */
	uint32_t trng_data1_mask;      /* 0x120 : TRNG data 1 mask            */
	uint32_t prng_data;            /* 0x124 : PRNG data                   */
	uint32_t trng_hc;              /* 0x128 : TRNG hc                     */
	uint32_t RESERVED2[437];
	uint32_t cios_data_base;       /* 0x800 : CIOS data base              */
};

#endif
