#ifndef __NRF52840_H
#define __NRF52840_H

#define I2S_DRV_NAME "nrf_i2s"

/*
 * PCM FORMAT 
 */
#define SND_PCM_FORMAT_16	0x0
#define SND_PCM_FORMAT_24	0x1
#define SND_PCM_FORMAT_32	0x2

#define NRF52840_I2S_BASE	0x40025000UL
#define __I2S_IOMEM	(uint32_t *)

/**
 * Starts continuous I2S transfer.
 * Also starts MCK generator when this is enabled.
 */
#define NRF_I2S_TASKS_START	__I2S_IOMEM (NRF52840_I2S_BASE + 0x0)

/**
 * Stops I2S transfer. Also stops MCK generator.
 * Triggering this task will cause the {event:STOPPED}.
 */
#define NRF_I2S_TASKS_STOP	__I2S_IOMEM (NRF52840_I2S_BASE + 0x4)

/**
 * The RXD.PTR register has been copied to internal double-buffers.
 * When the I2S module is started and RX is enabled,
 * this event will be generated for every RXTXD.
 * MAXCNT words that are received on the SDIN pin.
 */
#define NRF_I2S_EVENTS_RXPTRUPD	__I2S_IOMEM (NRF52840_I2S_BASE + 0x104)

/**
 * I2S transfer has stopped.
 */
#define NRF_I2S_EVENTS_STOPPED	__I2S_IOMEM (NRF52840_I2S_BASE + 0x108)

/**
 * The TDX.PTR register has been copied to internal double-buffers.
 * When the I2S module is started and TX is enabled,
 * this event will be generated for every RXTXD.MAXCNT words that are sent on the
 * SDOUT pin.
 */
#define NRF_I2S_EVENTS_TXPTRUPD	__I2S_IOMEM (NRF52840_I2S_BASE + 0x114)

/**
 * Enable/Disable the interrupt.
 * BIT 1: 1b(RW)  Enable/Disable interrupt for RXPTRUPD event
 * BIT 2: 1b(RW)  Enable/Disable interrupt for STOPPED event
 * BIT 5: 1b(RW)  Enable/Disable interrupt for TXPTRUPD event
 * Reset Val:0x00000000
 */
#define NRF_I2S_INTEN		__I2S_IOMEM (NRF52840_I2S_BASE + 0x300)
#define NRF_I2S_INTEN_RXPTRUPD	(1UL << 1)
#define NRF_I2S_INTEN_STOPPED	(1UL << 2)
#define NRF_I2S_INTEN_TXPTRUPD	(1UL << 5) 

/**
 * Enables the interrupt.
 * BIT1: 1b(RW) Write '1' to Enable interrupt for RXPTRUPD event
 * BIT5: 1b(RW) Write '1' to Enable interrupt for STOPPED event
 * BIT5: 1b(RW) Write '1' to Enable interrupt for TXPTRUPD event
 * Reset Val:0x00000000
 */
#define NRF_I2S_INTENSET		__I2S_IOMEM (NRF52840_I2S_BASE + 0x304)
#define NRF_I2S_INTENSET_NRXPTRUPD	(1UL << 1)
#define NRF_I2S_INTENSET_STOPPED	(1UL << 2)
#define NRF_I2S_INTENSET_TXPTRUPD	(1UL << 5) 

/**
 * Disables the interrupt.
 * BIT1: 1b(RW) Write '1' to Disable interrupt for RXPTRUPD event
 * 	 value 1 Disable
 * 	 value 0 Read Disable
 *	 value 1 Read Enable
 * BIT2: 1b(RW) Write '1' to Disable interrupt for STOPPED event
 *	 value 1 Disable
 *	 value 0 Read Disable
 * 	 value 1 Read Enable
 * BIT5: 1b(RW) Write '1' to Disable interrupt for TXPTRUPD event
 *	 value 1 Disable
 *	 value 0 Read Disable
 *	 value 1 Read Enable
 * Reset Val:0x00000000
 */
#define NRF_I2S_INTENCLR		__I2S_IOMEM (NRF52840_I2S_BASE + 0x308)
#define NRF_I2S_INTENCLR_RXPTRUPD	(1UL << 1)
#define NRF_I2S_INTENCLR_STOPPED	(1UL << 2)
#define NRF_I2S_INTENCLR_TXPTRUPD	(1UL << 5) 

/**
 * Enable I2S Module.
 * BIT0: 1b(RW) 
 * Reset Val:0x00000000
 */
#define NRF_I2S_ENABLE		__I2S_IOMEM (NRF52840_I2S_BASE + 0x500)
#define NRF_I2S_EN		0x1

/**
 * I2S Mode (Master/Slave)
 * BIT0: 1b(RW) Val 1 Slave mode
 * 		Val 0 Master mode.
 * Reset Val:0x00000000
 */
#define NRF_I2S_CFG_MODE	__I2S_IOMEM (NRF52840_I2S_BASE + 0x504)
#define NRF_I2S_CFG_MODE_SLAVE	0x1

/**
 * Reception (RX) enable.
 * BIT0: 1b(RW) Value 1 Reception enabled
 * 		Value 0 Reception Disabled.
 * Reset Val:0x00000000
 */
#define NRF_I2S_CFG_RXEN	__I2S_IOMEM (NRF52840_I2S_BASE + 0x508)
#define NRF_I2S_CFG_RX_ON	0x1

/**
 * Transmission (TX) enable.
 * BIT0: 1b(RW) Value 1 Transmission enabled
 * 		Value 0 Transmission Disabled.
 * Reset Val:0x00000000
 */
#define NRF_I2S_CFG_TXEN 	__I2S_IOMEM (NRF52840_I2S_BASE + 0x50c)
#define NRF_I2S_CFG_TX_ON	0x1

/**
 * Master clock generator enable.
 * BIT0: 1b(RW) Value 1 Master clock generator enabled
 * 			and MCK output available on PSEL.MCK
 * 		Value 0 Master clock generator disabled.
 */
#define NRF_I2S_CFG_MCKEN	__I2S_IOMEM (NRF52840_I2S_BASE + 0x510)

/**
 * Master clock generator frequency.
 * Reset Val:0x20000000
 */
#define NRF_I2S_CFG_MCKFREQ	__I2S_IOMEM (NRF52840_I2S_BASE + 0x514)
#define NRF_I2S_MCK_32MDIV2	0x80000000	/* 32 MHz / 2 = 16.0 MHz. */
#define NRF_I2S_MCK_32MDIV3	0x50000000	/* 32 MHz / 3 = 10.6666667 MHz. */
#define NRF_I2S_MCK_32MDIV4	0x40000000	/* 32 MHz / 4 = 8.0 MHz. */
#define NRF_I2S_MCK_32MDIV5	0x30000000	/* 32 MHz / 5 = 6.4 MHz. */
#define NRF_I2S_MCK_32MDIV6	0x28000000	/* 32 MHz / 6 = 5.3333333 MHz. */
#define NRF_I2S_MCK_32MDIV8	0x20000000	/* 32 MHz / 8 = 4.0 MHz. */
#define NRF_I2S_MCK_32MDIV10	0x18000000	/* 32 MHz / 10 = 3.2 MHz. */
#define NRF_I2S_MCK_32MDIV11	0x16000000	/* 32 MHz / 11 = 2.9090909 MHz. */
#define NRF_I2S_MCK_32MDIV15	0x11000000	/* 32 MHz / 15 = 2.1333333 MHz. */
#define NRF_I2S_MCK_32MDIV16	0x10000000	/* 32 MHz / 16 = 2.0 MHz. */
#define NRF_I2S_MCK_32MDIV21	0x0C000000	/* 32 MHz / 21 = 1.5238095 MHz. */
#define NRF_I2S_MCK_32MDIV23	0x0B000000	/* 32 MHz / 23 = 1.3913043 MHz. */
#define NRF_I2S_MCK_32MDIV30	0x08800000	/* 32 MHz / 30 = 1.0666666 MHz. */
#define NRF_I2S_MCK_32MDIV31	0x08400000	/* 32 MHz / 31 = 1.0322581 MHz. */
#define NRF_I2S_MCK_32MDIV32	0x08000000	/* 32 MHz / 32 = 1MHz. */
#define NRF_I2S_MCK_32MDIV42	0x06000000	/* 32 MHz / 42 = 0.7619048 MHz. */
#define NRF_I2S_MCK_32MDIV63	0x04100000	/* 32 MHz / 63 = 0.5079365 MHz. */
#define NRF_I2S_MCK_32MDIV125	0x020C0000	/* 32 MHz / 125 = 0.256 MHz. */

/**
 * MCK / LRCK ratio.
 * Reset Val:0x00000006
 */
#define NRF_I2S_CFG_RATIO	__I2S_IOMEM (NRF52840_I2S_BASE + 0x518)
#define NRF_I2S_RATIO_32X	0x0UL		/* LRCK = MCK / 32.  */
#define NRF_I2S_RATIO_48X	0x1UL		/* LRCK = MCK / 48.  */
#define NRF_I2S_RATIO_64X	0x2UL		/* LRCK = MCK / 64.  */
#define NRF_I2S_RATIO_96X	0x3UL		/* LRCK = MCK / 96.  */
#define NRF_I2S_RATIO_128X	0x4UL		/* LRCK = MCK / 128. */ 
#define NRF_I2S_RATIO_192X	0x5UL		/* LRCK = MCK / 192. */
#define NRF_I2S_RATIO_256X	0x6UL		/* LRCK = MCK / 256. */
#define NRF_I2S_RATIO_384X	0x7UL		/* LRCK = MCK / 384. */
#define NRF_I2S_RATIO_512X	0x8UL		/* LRCK = MCK / 512. */


/**
 * Sample width.
 * Reset Val:0x00000001
 */
#define NRF_I2S_CFG_SWIDTH	__I2S_IOMEM (NRF52840_I2S_BASE + 0x51C)
#define NRF_I2S_SWIDTH_8	0x0UL
#define NRF_I2S_SWIDTH_16	0x1UL
#define NRF_I2S_SWIDTH_24	0x2UL

/**
 * Alignment of sample within a frame.
 * Reset Val:0x00000000
 * BIT0 :1b(RW)	val 0 Left Aligned
 *		Val 1 Right Aligned
 */
#define NRF_I2S_CFG_ALIGN	__I2S_IOMEM (NRF52840_I2S_BASE + 0x520)
#define NRF_I2S_CFG_FORMAT_LALIGN	0x0UL
#define NRF_I2S_CFG_FORMAT_RALIGN	0x1UL

/**
 * FRAME format
 * Reset Val:0x00000000
 * BIT0 :1b(RW)	val 0 Original I2S format
 * 		Val 1 lternate (left- or right-aligned) format.
 */
#define NRF_I2S_CFG_FORMAT	__I2S_IOMEM (NRF52840_I2S_BASE + 0x524)
#define NRF_I2S_CFG_FORMAT_I2S		0x0UL
#define NRF_I2S_CFG_FORMAT_ALIGN	0x1UL

/**
 * Enable channels.
 * Reset Val:0x00000000
 * Value 0x1 Stereo
 *	 0x2 Left Only
 *	 0x3 Right only
 */
#define NRF_I2S_CFG_CHANNELS	__I2S_IOMEM (NRF52840_I2S_BASE + 0x528)
#define NRF_I2S_CFG_CHANNEL_STEREO	0x0UL
#define NRF_I2S_CFG_CHANNEL_LEFT	0x1UL
#define NRF_I2S_CFG_CHANNEL_RIGHT	0x2UL

/**
 * Receive buffer RAM start address.
 * Receive buffer Data RAM start address. When receiving, words
 * containing samples will be written to this address. This address
 * is a word aligned Data RAM address.
 * Reset Val:0x00000000
 */
#define NRF_I2S_RXD_PTR		__I2S_IOMEM (NRF52840_I2S_BASE + 0x538)

/**
 * Transmit buffer RAM start address.
 * Transmit buffer Data RAM start address. When transmitting,
 * words containing samples will be fetched from this address. This
 * address is a word aligned Data RAM address.
 * Reset Val:0x00000000
 */
#define NRF_I2S_TXD_PTR		__I2S_IOMEM (NRF52840_I2S_BASE + 0x540)

/** 
 * Size of RXD and TXD buffers.
 * Size of RXD and TXD buffers in number of 32 bit words
 */
#define NRF_I2S_RXTXD_MAXCNT	__I2S_IOMEM (NRF52840_I2S_BASE + 0x550)

/**
 * Pin select for MCK signal.
 */
#define NRF_I2S_PSEL_MCLK	__I2S_IOMEM (NRF52840_I2S_BASE + 0x560)

/**
 * Pin select for SCK signal
 */
#define NRF_I2S_PSEL_SCK	__I2S_IOMEM (NRF52840_I2S_BASE + 0x564)

/**
 * Pin select for LRCK signal
 */
#define NRF_I2S_PSEL_LRCK	__I2S_IOMEM (NRF52840_I2S_BASE + 0x568)

/**
 * Pin select for SDIN signal.
 */
#define NRF_I2S_PSEL_SDIN	__I2S_IOMEM (NRF52840_I2S_BASE + 0x56C)

/**
 * Ping select for SDOUT signal.
 */
#define NRF_I2S_PSEL_SDOUT	__I2S_IOMEM (NRF52840_I2S_BASE + 0x570)

/**
 * Interrupt IRQn
 */
#define NRF_IRQ_I2S_IRQn	37

enum nrf_i2s_channel {
	I2S_STEREO = 0,
	I2S_MONO_LEFT = 1,
	I2S_MONO_RIGHT = 2,
};

/**
 * Source clock frequency 32Mhz.
 * For DIV2 the value would be (32MHz / 2 ) = 16.0MHz
 */
enum nrf_mclk_div {
	DIV2,
	DIV3,	
	DIV4,	
	DIV5,	
	DIV6,	
	DIV8,	
	DIV10,	
	DIV11,	
	DIV15,	
	DIV16,	
	DIV21,	
	DIV23,	
	DIV30,	
	DIV31,	
	DIV32,	
	DIV42,	
	DIV63,	
	DIV125,	
	DIV_MAX,
};

uint32_t nrf_mclk_div_val[DIV_MAX] = {
	NRF_I2S_MCK_32MDIV2,
	NRF_I2S_MCK_32MDIV3,
	NRF_I2S_MCK_32MDIV4,
	NRF_I2S_MCK_32MDIV5,
	NRF_I2S_MCK_32MDIV6,
	NRF_I2S_MCK_32MDIV8,
	NRF_I2S_MCK_32MDIV10,
	NRF_I2S_MCK_32MDIV11,
	NRF_I2S_MCK_32MDIV15,
	NRF_I2S_MCK_32MDIV16,
	NRF_I2S_MCK_32MDIV21,
	NRF_I2S_MCK_32MDIV23,
	NRF_I2S_MCK_32MDIV30,
	NRF_I2S_MCK_32MDIV31,
	NRF_I2S_MCK_32MDIV32,
	NRF_I2S_MCK_32MDIV42,
	NRF_I2S_MCK_32MDIV63,
	NRF_I2S_MCK_32MDIV125,
};

/**
 * MCLK/LRCLK Ratio
 * LRCLK = MCLK / Ratio.
 */
enum nrf_i2s_ratio {
	RATIO_32X,
	RATIO_48X,
	RATIO_64X,
	RATIO_96X,
	RATIO_128X,
	RATIO_192X,
	RATIO_256X,
	RATIO_384X,
	RATIO_512X,
	RATIO_MAX,
};

#define NRF_I2S_CHANNEL	2
#define NRF_I2S_BWIDTH	2
#define FRAME (NRF_I2S_CHANNEL * NRF_I2S_BWIDTH)
#define BUFF_MAX	2
//#define I2S_BUFF_SIZE (FRAME * 100)
#define I2S_BUFF_SIZE 64

struct i2s_buff {
	uint32_t buff[I2S_BUFF_SIZE];
	uint32_t rindex;
	uint32_t windex;
	uint8_t buffer_valid;
};

enum nrf_freq {
	FREQ_16000,
	FREQ_32000,
	FREQ_44100,
	FREQ_48000,
	FREQ_INVALID,
};

enum nrf_i2s_bit_width {
	I2S_PCM_FORMAT_8,
	I2S_PCM_FORMAT_16,
	I2S_PCM_FORMAT_24,
	I2S_PCM_FORMAT_INVALID,
	
};

enum nrf_i2s_port {
	I2S_PSEL_MCLK,
	I2S_PSEL_SCK,
	I2S_PSEL_LRCK,
	I2S_PSEL_SDIN,
	I2S_PSEL_SDOUT,
};

struct nrf_i2s_port_cfg {
	uint32_t pinmap:5;
	uint32_t portmap:1;
	uint32_t rsvd:25;
	uint32_t connected:1;
};

struct nrf_i2s_port_cfg pcfg[] = {
	{
		.pinmap = 255,
		.portmap = 0,
		.connected = 0,
	},
	{
		.pinmap = 31,
		.portmap = 0,
		.connected = 0,
	},
	{
		.pinmap = 30,
		.portmap = 0,
		.connected = 0,
	},
	{
		.pinmap = 26,
		.portmap = 0,
		.connected = 0,
	},
	{
		.pinmap = 27,
		.portmap = 0,
		.connected = 0,
	},
};

struct nrf_device_cfg {
	uint8_t irq_no;
	struct nrf_i2s_port_cfg *i2s_port_cfg;
};

struct pcm_stream {
	
	int state;
	struct k_mutex smutex;
	struct i2s_config *cfg;
	struct i2s_buff rx_buf[BUFF_MAX];
	struct i2s_buff tx_buf[BUFF_MAX];
	uint32_t rxeventcnt;
	uint32_t txeventcnt;
	enum nrf_i2s_bit_width bwidth;
	enum nrf_freq lrclk;
	enum nrf_i2s_channel channel;
};

#define PCM_MAX_STREAM	2
struct nrf_i2s_dev {
	struct pcm_stream snd_stream[PCM_MAX_STREAM];
	struct nrf_device_cfg dcfg;
};

struct nrf_i2s_mclk_freq {
	uint32_t freq;
	enum nrf_mclk_div div;
	enum nrf_i2s_ratio ratio;
};

#endif /* __NRF52840_H */
