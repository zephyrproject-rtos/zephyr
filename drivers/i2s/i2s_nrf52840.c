/* 
 * Copyright (c) 2018 Webem <embconnect001@gmail.com>
 * 
 * SPDX-License-Identifier: Apache-2.0 
 */ 

#include <errno.h>
#include <string.h>
#include <misc/__assert.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <dma.h>
#include <i2s.h>
#include <soc.h>
#include "nrf52840_i2s.h"

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_I2S_LEVEL
#define SYS_LOG_NO_NEWLINE
#define SYS_LOG_DOMAIN "i2s"
#define SYS_LOG_NO_NEWLINE

#include <logging/sys_log.h>
#define I2S_BUFFER_SIZE 1000
#define I2S_RXTXD_CNT	(I2S_BUFFER_SIZE / 2)

struct nrf_i2s_dev i2s_cfg;

#define NRF_FREQ(FREQ, RATIO, DIV)	\
{					\
	.freq = FREQ,			\
	.ratio = RATIO,			\
	.div = DIV,			\
}

/**
 * Currently handled for 32x ratio and 16 bit_width
 * supported sampling freq (16KHz, 32KHz, 44.1KHz, 48KHz)
 */
const struct nrf_i2s_mclk_freq nrf_mclk_freq_tbl[] = {

	NRF_FREQ(16000, RATIO_32X, DIV63),
	NRF_FREQ(32000, RATIO_32X, DIV31),
	NRF_FREQ(44100, RATIO_32X, DIV23),
	NRF_FREQ(48000, RATIO_32X, DIV21),
};

/**
 * @nrf_i2s_write: writes the value to the given register.
 * addr = register address;
 * val = value to be written to the register.
 */
static inline void nrf_i2s_reg_write(uint32_t *addr, uint32_t val)
{
	__IO uint32_t *ioaddr;

	if (addr) {
		ioaddr  = (__IO uint32_t *)addr;
		*ioaddr = val;
		__ISB();
		__DSB();
	}
}

/**
 * @nrf_i2s_read: Reads the given register.
 * addr = register address;
 * Return Value: value read from the register.
 */
static inline uint32_t nrf_i2s_reg_read(uint32_t *addr)
{
	__I uint32_t *ioaddr;

	if (addr) {
		ioaddr = (__I uint32_t *)addr;
		__ISB();
		__DSB();
		return *(ioaddr);
	}
}

/**
 * @nrf_i2s_rm_write: writes the value to the given register with out
 * modifying the current values.
 * addr = register address;
 * val = value to be written to the register.
 */
static inline void nrf_i2s_rm_reg_write(uint32_t *addr, uint32_t val)
{
        __IO uint32_t *ioaddr;
	uint32_t rval;

        if (addr) {
		ioaddr  = (__IO uint32_t *)addr;
		rval = *(ioaddr);
		rval |= val;
		*ioaddr = rval;
		__ISB();
		__DSB();
	}
}

/*
 * @nrf_i2s_set_rm_set
 * Corresponding BIT POS to set.
 * addr = register address.
 * Pos = bit pos to set.
 */
static inline void nrf_i2s_rm_reg_set(uint32_t *addr, uint32_t pos)
{
	__IO uint32_t *ioaddr;
	uint32_t rval;

	if (addr) {
		ioaddr  = (__IO uint32_t *)addr;
		rval = *(ioaddr);
		rval |= (1 << pos);
		*ioaddr = rval;
		__ISB();
		__DSB();
	}
}

/**
 * @nrf_i2s_rm_clear
 * Corresponding BIT POS to clear.
 * Addr = register Address
 * POS = Bit pos to clear
 */
static inline void nrf_i2s_rm_reg_clear(uint32_t *addr, uint32_t pos)
{
	__IO uint32_t *ioaddr;
	uint32_t rval;

	if (addr) {
		ioaddr  = (__IO uint32_t *)addr;
		rval = *(ioaddr);
		rval &= ~(1 << pos);
		*ioaddr = rval;
		__ISB();
		__DSB();
	}	
}

/**
 * @nrf_i2s_en_rx
 * Reception enabled / Disabled
 * @enable 1 rx enabled / 0 rx disabled
 * Returns: None
 */
static void nrf_i2s_en_rx(uint8_t enable)
{
	if (enable)
		nrf_i2s_reg_write(NRF_I2S_CFG_RXEN, NRF_I2S_CFG_RX_ON);	
	else
		nrf_i2s_reg_write(NRF_I2S_CFG_RXEN, 0x0);	
}

/**
 * @nrf_i2s_en_tx
 * Transmission Enabled / Disabled.
 * Argument: 1 tx enabled / 0 disabled.
 * Returns: None
 */
static void nrf_i2s_en_tx(uint8_t enable)
{
	if (enable)
		nrf_i2s_reg_write(NRF_I2S_CFG_TXEN, NRF_I2S_CFG_TX_ON);
	else
		nrf_i2s_reg_write(NRF_I2S_CFG_TXEN, 0x0);
}

/**
 * @nrf_i2s_en
 * Enables I2S Module.
 */
static void nrf_i2s_en(uint8_t enable)
{
	if (enable)
		nrf_i2s_reg_write(NRF_I2S_ENABLE, 0x1);
	else
		nrf_i2s_reg_write(NRF_I2S_ENABLE, 0x0);
}

/**
 * @nrf_i2s_task_start
 * Starts continuous I2S transfer.
 * Also starts MCK generator when this is enabled.
 */
static void nrf_i2s_task_start(void)
{
	nrf_i2s_reg_write(NRF_I2S_TASKS_START, 0x1);
}

/**
 * @nrf_i2s_task_stop
 * Stops I2S transfer. Also stops MCK generator.
 * Triggering this task will cause the {event:STOPPED}.
 */
static void nrf_i2s_task_stop(void)
{
	nrf_i2s_reg_write(NRF_I2S_TASKS_STOP, 0x1);
}

/*
 * @nrf_i2s_cfg_mclk_generator()
 * Return: None
 */
static void nrf_i2s_cfg_mclk(enum nrf_mclk_div div)
{
	if (div > DIV_MAX)
		return;

	nrf_i2s_reg_write(NRF_I2S_CFG_MCKFREQ, nrf_mclk_div_val[div]);
}

/**
 * @nrf_i2s_enable_int
 * Enables interrupt.
 * Arguments: Interrupt to enable.
 * Returns: None
 */
static inline void nrf_i2s_en_int(uint32_t event)
{
	nrf_i2s_reg_write(NRF_I2S_INTEN, event);	
}

/**
 * @nrf_i2s_enable_event
 * Enables the interrupt.
 * Argument:Interrupt to enable.
 * Returns: None
 */
static inline void nrf_i2s_en_event(uint32_t event)
{
	nrf_i2s_reg_write(NRF_I2S_INTENSET, event); 
}

static void nrf_i2s_disable_event(uint32_t event)
{
	nrf_i2s_reg_write(NRF_I2S_INTENCLR, event); 
}

/**
 * @nrf_i2s_clear_event_rxptrupd
 * The RXD.PTR register has been copied to internal double-buffers.
 * When the I2S module is started. and RX is enabled,
 * this event will be generated for every RXTXD.MAXCNT words
 * that are received on the SDIN pin.
 */
static inline void nrf_i2s_clear_event_rxptrupd(void)
{
	nrf_i2s_reg_write(NRF_I2S_EVENTS_RXPTRUPD, 0x0);
}

/**
 * @nrf_i2s_clear_event_txptrupd
 * The TDX.PTR register has been copied to internal double-buffers.
 * When the I2S module is started. and TX is enabled, this event will be
 * generated for every RXTXD.MAXCNT words that are sent on the SDOUT pin.
 */
static inline void nrf_i2s_clear_event_txptrupd(void)
{
	nrf_i2s_reg_write(NRF_I2S_EVENTS_TXPTRUPD, 0x0);
	
}

/**
 * @nrf_i2s_get_txptrupd_event
 * returns: txprtupd Event Occurence.
 */
static inline uint8_t nrf_i2s_get_txptrupd_event(void)
{
	return nrf_i2s_reg_read(NRF_I2S_EVENTS_TXPTRUPD);
}

/**
 * @nrf_i2s_get_rxptrupd_event
 * returns: rxptrupd Event occurence
 */
static inline uint8_t nrf_i2s_get_rxptrupd_event(void)
{
	return nrf_i2s_reg_read(NRF_I2S_EVENTS_RXPTRUPD);
}

/**
 * @nrf_i2s_clear_event_stopped.
 * I2S transfer stopped.
 */
static inline void nrf_i2s_clear_event_stopped(void)
{
	nrf_i2s_reg_write(NRF_I2S_EVENTS_STOPPED, 0x0);
}

/**
 * @nrf_i2s_get_rxptrupd_event
 * returns: stopped Event occurence
 */
static inline uint8_t nrf_i2s_get_stopped_event(void)
{
	return nrf_i2s_reg_read(NRF_I2S_EVENTS_STOPPED);
}

/**
 * @nrf_i2s_cfg_mclk_en()
 * Master clock generator enable.
 * Value 1 Master clock generator enabled
 * and MCK output available on PSEL.MCK
 * Value 0 Master clock generator disabled.
 * Arguments: ON/OFF.
 * Returns: None
 */
static void nrf_i2s_cfg_mclk_en(uint8_t mclk_en)
{
	if (mclk_en) 
		nrf_i2s_reg_write(NRF_I2S_CFG_MCKEN, 0x1);
	else
		nrf_i2s_reg_write(NRF_I2S_CFG_MCKEN, 0x0);
}

/**
 * @nrf_i2s_cfg_pinmap
 * Configures following GPIO's
 * psel_mclk,psel_sck,psel_lrck,psel_sdin,psel_sdout
 */
static void nrf_i2s_cfg_pinmap(void)
{
	uint32_t pval;
	
	pval = (pcfg[I2S_PSEL_MCLK].pinmap |
		(pcfg[I2S_PSEL_MCLK].portmap << 5) |
		(pcfg[I2S_PSEL_MCLK].connected << 30));
	nrf_i2s_reg_write(NRF_I2S_PSEL_MCLK, pval);

	pval = (pcfg[I2S_PSEL_SCK].pinmap |
		(pcfg[I2S_PSEL_SCK].portmap << 5) |
		(pcfg[I2S_PSEL_SCK].connected << 30));
	nrf_i2s_reg_write(NRF_I2S_PSEL_SCK, pval);

	pval = (pcfg[I2S_PSEL_LRCK].pinmap |
		(pcfg[I2S_PSEL_LRCK].portmap << 5) |
		(pcfg[I2S_PSEL_LRCK].connected << 30));
	nrf_i2s_reg_write(NRF_I2S_PSEL_LRCK, pval);

	pval = (pcfg[I2S_PSEL_SDIN].pinmap |
		(pcfg[I2S_PSEL_SDIN].portmap << 5) |
		(pcfg[I2S_PSEL_SDIN].connected << 30));
	nrf_i2s_reg_write(NRF_I2S_PSEL_SDIN, pval);

	pval = (pcfg[I2S_PSEL_SDOUT].pinmap |
		(pcfg[I2S_PSEL_SDOUT].portmap << 5) |
		(pcfg[I2S_PSEL_SDOUT].connected << 30));
	nrf_i2s_reg_write(NRF_I2S_PSEL_SDOUT, pval);
}

/**
 * @nrf_i2s_cfg_ratio
 * Arguments: enum nrf_i2s_ratio
 * Returns none.
 */
static void nrf_i2s_cfg_ratio(enum nrf_i2s_ratio ratio)
{
	if (ratio > RATIO_MAX)
		return;

	nrf_i2s_reg_write(NRF_I2S_CFG_RATIO, ratio);
}

/**
 * @nrf_i2s_cfg_clk
 * Serial clock  = LRCLK * bit_width * 2(channels)
 * Master clock frequency canot exceed serial clock frequency 
 * which can be formulated as  Ratio >= 2 * bit_width
 * LRCLK = MCLK / Ratio.
 * 
 * Configuration exaples for clock.
 * LRCK(Hz) 	BIT_WIDTH	Ratio 	MCKFreq 	MCK [Hz] 	LRCK(Hz)
 * 16000	16 		32x	32MDIV63	507936.5	15873.0			
 * 16000	16		64x	32MDIV31	1032258.1	16129.0
 * 16000	16		256x	32MDIV8		4000000.0	15625.0
 * 32000	16		32x	32MDIV31	1032258.1	32258.1
 * 32000	16		64x	32MDIV16	2000000.0	31250.0
 * 32000	16		256x	32MDIV4		8000000.0	31250.0
 * 44100	16		32x	32MDIV23	1391304.3	43478.3
 * 44100	16		64x	32MDIV11	2909090.9	45454.5
 * 44100	16		256x	32MDIV3		10666666.7	41666.7
 *
 * To dereive the optimum LRCLK.
 * for various available ratio and compare the results
 * with available MCLK frequencies and choose the right
 * divider based on LRCK error %.
 */
static uint8_t nrf_i2s_cfg_clk(enum nrf_freq lrclk,
			enum nrf_i2s_bit_width swidth)
{
	enum nrf_mclk_div div;
	enum nrf_i2s_ratio ratio;
	div = nrf_mclk_freq_tbl[lrclk].div;
	ratio = nrf_mclk_freq_tbl[lrclk].ratio;

	nrf_i2s_cfg_ratio(ratio);
	nrf_i2s_cfg_mclk(div);
	nrf_i2s_cfg_mclk_en(1);

	return 0;
}

/**
 * @nrf_i2s_cfg_channels;
 * Arguments: Channel to configure.
 * Possible values are STEREO, MONO_LEFT, MONO_RIGHT.
 * Returns: None. 
 * TODO: If Tx and Rx going to share the same cfg space
 * this function should be protected with mutex.
 */
static void nrf_i2s_cfg_channels(enum nrf_i2s_channel channel)
{
	switch (channel) {
	case I2S_STEREO:
		nrf_i2s_reg_write(NRF_I2S_CFG_CHANNELS,
				NRF_I2S_CFG_CHANNEL_STEREO);
		break;
	case I2S_MONO_LEFT:
		nrf_i2s_reg_write(NRF_I2S_CFG_CHANNELS,
				NRF_I2S_CFG_CHANNEL_LEFT);
		break;
	case I2S_MONO_RIGHT:
		nrf_i2s_reg_write(NRF_I2S_CFG_CHANNELS,
				NRF_I2S_CFG_CHANNEL_RIGHT);
		break;
	default:
		SYS_LOG_DBG("Invalid Channel :%d\n", channel);
		break;
	}
}

/**
 * @i2s_nrf_cfg_bit_width - configures bit width for i2s.
 *
 * dev device structure
 * bit_width - word size to configure
 *             (SND_PCM_FORMAT_16,
 *		SND_PCM_FORMAT_24,
 *		SND_PCM_FORMAT_32)
 *
 * Default i2s is configured to 16 bit mode.(on i2s reset)
 *
 */
static void nrf_i2s_cfg_bit_width(enum nrf_i2s_bit_width bwidth)
{
	switch (bwidth) {
	case I2S_PCM_FORMAT_8:
		nrf_i2s_reg_write(NRF_I2S_CFG_SWIDTH, NRF_I2S_SWIDTH_8);
		break;
	case I2S_PCM_FORMAT_16:
		nrf_i2s_reg_write(NRF_I2S_CFG_SWIDTH, NRF_I2S_SWIDTH_16);
		break;
	case I2S_PCM_FORMAT_24:
		nrf_i2s_reg_write(NRF_I2S_CFG_SWIDTH, NRF_I2S_SWIDTH_24);
		break;
	default:
		SYS_LOG_DBG("Invalid bit_width: %d\n", bwidth);
		break;
	}
}

/**
 * @nrf_i2s_cfg_align
 * Alignment of sample within a frame.
 * Arguments: align
 * 	0 - Left Algin
 *	1 - Right Align
 * Returns: None
 */
static void nrf_i2s_cfg_align(uint8_t align)
{
	if (align)
		nrf_i2s_reg_write(NRF_I2S_CFG_ALIGN,
				NRF_I2S_CFG_FORMAT_RALIGN);
	else
		nrf_i2s_reg_write(NRF_I2S_CFG_ALIGN,
				NRF_I2S_CFG_FORMAT_LALIGN);
}

/**
 * @nrf_i2s_cfg_format
 * Config format
 * Arguments: format
 * 	0: i2s format
 *	1: Aligned format(L/R Aligned)
 * Returns: None
 */
static void nrf_i2s_cfg_format(uint8_t format)
{
	if (format)
		nrf_i2s_reg_write(NRF_I2S_CFG_FORMAT,
				NRF_I2S_CFG_FORMAT_ALIGN);
	else
		nrf_i2s_reg_write(NRF_I2S_CFG_FORMAT,
				NRF_I2S_CFG_FORMAT_I2S);
}

/**
 * @nrf_i2s_update_tx_ptr
 * 
 */
static void nrf_i2s_update_ptr(enum i2s_dir dir, uint32_t *addr)
{
	if (dir == I2S_DIR_RX)
		nrf_i2s_reg_write(NRF_I2S_RXD_PTR, (uint32_t)addr);
	else
		nrf_i2s_reg_write(NRF_I2S_TXD_PTR, (uint32_t)addr);
	
	SYS_LOG_DBG("addr:%d\n", nrf_i2s_reg_read(NRF_I2S_TXD_PTR));

}

/**
 * @nrf_i2s_cfg_rxtxd_size
 * Size of RXD and TXD buffers.
 */
static void inline nrf_i2s_cfg_rxtxd_size(uint32_t buff_size)
{
	nrf_i2s_reg_write(NRF_I2S_RXTXD_MAXCNT, buff_size);
}

static inline enum nrf_i2s_bit_width
	nrf_convert_bitwidth(uint32_t bwidth)
{
	switch (bwidth) {
	case 8:
		return I2S_PCM_FORMAT_8;
	case 16:
		return I2S_PCM_FORMAT_16;
	case 24:
		return I2S_PCM_FORMAT_24;
	default:
		return I2S_PCM_FORMAT_INVALID;
	}
}

static inline enum nrf_freq nrf_convert_freq(uint32_t lrclk)
{
	switch (lrclk) {
	case 16000:
		return FREQ_16000;
	case 32000:
		return FREQ_32000;
	case 44100:
		return FREQ_44100;
	case 48000:
		return FREQ_48000;
	default:
		return FREQ_INVALID;
	}
}

/**
 * @nrf_i2s_handle_event: interrupt handler
 * handles rx ptr update event
 *	   tx ptr update event
 *         stopped event
 */
static void nrf_i2s_handle_event(void *args)
{

	irq_disable(NRF_IRQ_I2S_IRQn);

	if (nrf_i2s_get_rxptrupd_event()) {
		SYS_LOG_DBG("EVENT: NRF_I2S_INTENCLR_RXPTRUPD\n");
		nrf_i2s_clear_event_rxptrupd();
	}

	if (nrf_i2s_get_txptrupd_event()) {
		SYS_LOG_DBG("EVENT: NRF_I2S_INTENCLR_TXPTRUPD\n");
		nrf_i2s_clear_event_txptrupd();

	}

	if (nrf_i2s_get_stopped_event()) {
		nrf_i2s_clear_event_stopped();
		SYS_LOG_DBG("EVENT:NRF_I2S_INTENCLR_STOPPED\n");
	}

	irq_enable(NRF_IRQ_I2S_IRQn);

}

/**
 * @nrf_i2s_register_int
 * Register interrupt
 * Returns: None
 */
static void nrf_i2s_register_int(void)
{
	IRQ_CONNECT(NRF52_IRQ_I2S_IRQn, 0, nrf_i2s_handle_event, 0, 0);
}

/**
 * @nrf_i2s_cfg_init()
 * initializes the sw cfg parameters
 * allocates tx and rx buffer.
 */
static int nrf_i2s_cfg_init(struct device *dev)
{

	if (!dev) {
		return -ENODEV;
	}

	dev->driver_data = &i2s_cfg;

	nrf_i2s_register_int();

	SYS_LOG_DBG("Intialized...\n");
	return 0;	
}

/*
 * @nrf_i2s_configue() configures the I2S based on the I2S
 * parameters that we recieve in i2s_config structure.
 * Arguments: @dev device structure
 *	      @dir RX/TX
 *	      @rcfg runtime config parameters for I2S.
 * Returns: 0 on success/Appropirate Error code. 
 */
static int nrf_i2s_configure(struct device *dev, enum i2s_dir dir,
					struct i2s_config *rcfg)
{
	struct nrf_i2s_dev *cfg;
	struct pcm_stream *substream;

	if (!dev && !dev->driver_data)
		return -ENODEV;

	if (!rcfg)
		return NULL;

	if (dir != I2S_DIR_RX && dir != I2S_DIR_TX) {
		SYS_LOG_DBG("Invalid dir: %d\n", dir);
		return -EINVAL;
	}

	cfg = (struct nrf_i2s_dev *)dev->driver_data;
	if (!cfg)
		return NULL;
	

	substream = &cfg->snd_stream[dir];
	/*Initialize Mutex */
	k_mutex_init(&substream->smutex);

	substream->cfg = rcfg;
	substream->bwidth = nrf_convert_bitwidth(rcfg->word_size);
	substream->lrclk = nrf_convert_freq(rcfg->frame_clk_freq);
	substream->channel = 2;

	SYS_LOG_DBG("channel:%d swidth:%d \n",
			substream->channel, substream->bwidth);

	nrf_i2s_en(1);
	nrf_i2s_cfg_clk(substream->lrclk, substream->bwidth);
	nrf_i2s_cfg_bit_width(substream->bwidth);
	nrf_i2s_cfg_align(NRF_I2S_CFG_FORMAT_LALIGN);
	nrf_i2s_cfg_format(NRF_I2S_CFG_FORMAT_I2S);
	nrf_i2s_cfg_channels(substream->channel);

	/*
	 * Enable interrupts && Events.
	 */
	nrf_i2s_clear_event_rxptrupd();
	nrf_i2s_clear_event_txptrupd();

	nrf_i2s_en_int(NRF_I2S_INTEN_RXPTRUPD |
			NRF_I2S_INTEN_STOPPED |
			NRF_I2S_INTEN_TXPTRUPD);

	nrf_i2s_en_event(NRF_I2S_INTENSET_NRXPTRUPD |
			 NRF_I2S_INTENSET_STOPPED   |
			 NRF_I2S_INTENSET_TXPTRUPD);	
	
	nrf_i2s_cfg_pinmap();
	nrf_i2s_cfg_rxtxd_size(I2S_RXTXD_CNT);
	irq_enable(NRF_IRQ_I2S_IRQn);

	substream->state = I2S_STATE_READY;

	return 0;
}

static int nrf_i2s_read(struct device *dev, void **mem_block, size_t *size)
{
	return 0;
}

static int nrf_i2s_write(struct device *dev, void *mem_block, size_t size)
{
	return 0;
}

static int nrf_i2s_trigger(struct device *dev, enum i2s_dir dir,
				enum i2s_trigger_cmd cmd)
{
	struct nrf_i2s_dev *cfg;
	struct pcm_stream *substream;

	if (!dev && !dev->driver_data) {
		SYS_LOG_DBG("Device not found\n");
		return -ENODEV;
	}

	cfg = (struct nrf_i2s_dev *)dev->driver_data;
	if (!cfg) {
		SYS_LOG_DBG("driver_data (Null)\n");
		return NULL;
	}

	if (dir != I2S_DIR_RX && dir != I2S_DIR_TX) {
		SYS_LOG_DBG("Invalid dir: %d\n", dir);
		return -EINVAL;
	}

	substream = &cfg->snd_stream[dir];
	if (substream->state != I2S_STATE_READY) {
		SYS_LOG_DBG("Device not configured..\n");
		return -EINVAL;
	}

	/* Enable based on direction */
	SYS_LOG_DBG("I2S... %d \n", dir);
	switch (cmd) {
	case I2S_TRIGGER_START:
		if (dir == I2S_DIR_RX) {
			nrf_i2s_en_rx(1);
		} else if (dir == I2S_DIR_TX) {
			nrf_i2s_en_tx(1);
		}
		substream->state = I2S_STATE_RUNNING;	
		nrf_i2s_task_start();
		break;
	case I2S_TRIGGER_STOP:
		if (dir == I2S_DIR_RX) {
			nrf_i2s_en_rx(0);
		} else if (dir == I2S_DIR_TX) {
			nrf_i2s_en_tx(0);
		}

		nrf_i2s_disable_event(0x0);
		nrf_i2s_en(0);
		nrf_i2s_task_stop();
		irq_disable(NRF_IRQ_I2S_IRQn);
		substream->state = I2S_STATE_STOPPING;	
		break;
	case I2S_TRIGGER_PREPARE:
		break;
	/* ToDo */
	case I2S_TRIGGER_DRAIN:
		break;
	default:
		SYS_LOG_DBG("Invalid CMD\n");
		return -EINVAL;
	}
	return 0;
}

static const struct i2s_driver_api i2s_nrf_driver_api = {
	.configure = nrf_i2s_configure,
	.read = nrf_i2s_read,
	.write = nrf_i2s_write,
	.trigger = nrf_i2s_trigger,
};

DEVICE_AND_API_INIT(I2S0_NRF, I2S_DRV_NAME, &nrf_i2s_cfg_init,
		    &i2s_cfg, NULL, POST_KERNEL, CONFIG_I2S_INIT_PRIORITY,
		    &i2s_nrf_driver_api);
