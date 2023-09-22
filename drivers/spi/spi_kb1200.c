/*
 * Copyright (c) 2021 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ene_kb1200_spi

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <soc.h>

LOG_MODULE_REGISTER(spi_kb1200, LOG_LEVEL_ERR);

#include "spi_context.h"

/* Device config */
struct kb1200_spi_config {
	/* flash interface unit base address */
	uintptr_t base_addr;
};

/* Device data */
struct kb1200_spi_data {
	struct spi_context ctx;
};

/* Driver convenience defines */
#define DRV_CONFIG(dev)     ((const struct kb1200_spi_config *)(dev)->config)
#define DRV_DATA(dev)       ((struct kb1200_spi_data *)(dev)->data)
#define HAL_INSTANCE(dev)   (SPIH_T *)(DRV_CONFIG(dev)->base_addr)

#define SPIH_CS_SHR         0x45
#define SPIH_CLK_SHR        0x47
#define SPIH_MOSI_SHR       0x44
#define SPIH_MISO_SHR       0x46

#define SPIH_CS             0x41
#define SPIH_CLK            0x40
#define SPIH_MOSI           0x42
#define SPIH_MISO           0x3E

static int spi_kb1200_configure(const struct device *dev,
			        const struct spi_config *spi_cfg)
{
    SPIH_T *spih = HAL_INSTANCE(dev);
    /*
     * SPI signalling mode: CPOL and CPHA
     * Mode CPOL CPHA
     *  0     0    0
     *  1     0    1
     *  2     1    0
     *  3     1    1
     */
    uint8_t Mode,Clock;
    
    Mode = SPI_MODE_GET(spi_cfg->operation)&0x03;   //Mode0/1/2/3
    
    if(spi_cfg->frequency < 1000000)
        Clock = SPIH_Clock_500K;
    else if(spi_cfg->frequency < 2000000)
        Clock = SPIH_Clock_1M;
    else if(spi_cfg->frequency < 4000000)
        Clock = SPIH_Clock_2M;
    else if(spi_cfg->frequency < 8000000)
        Clock = SPIH_Clock_4M;
    else if(spi_cfg->frequency < 16000000)
        Clock = SPIH_Clock_8M;
    else
        Clock = SPIH_Clock_16M;

    if(!(spi_cfg->operation&SPI_HOLD_ON_CS))
    {
        if(spi_cfg->operation & SPI_CS_ACTIVE_HIGH)
            spih->SPIHCTR |= 0x01;                      //idle set low
        else
            spih->SPIHCTR &= ~0x01;                     //idle set high
    }

    spih->SPIHCFG = (spih->SPIHCFG&0xC1)|((Mode&0x03)<<4)|((Clock&0x07)<<1);
    
    return 0;
}

static int spi_feature_support(const struct spi_config *spi_cfg)
{

    if(SPI_OP_MODE_GET(spi_cfg->operation)==SPI_OP_MODE_SLAVE){
        printk("spih not support slave\n");
        return -ENOTSUP;
    }

    if(spi_cfg->operation&SPI_TRANSFER_LSB){
        printk("spih not support transfer LSB\n");
        return -ENOTSUP;
    }
    
	if (SPI_WORD_SIZE_GET(spi_cfg->operation) != 8) {
		printk("Word sizes other than 8 bits are not supported\n");
		return -ENOTSUP;
	}

	if (spi_cfg->frequency < 500000) {
		printk("Frequencies lower than 500kHz are not supported\n");
		return -ENOTSUP;
	}

    if(spi_cfg->operation&SPI_LOCK_ON){
        printk("spih not support Lock On\n");
        return -ENOTSUP;
    }
    
    if(((spi_cfg->operation)&(SPI_LINES_MASK)) != SPI_LINES_SINGLE){
        printk("spih not support dual/quad mode\n");
        return -ENOTSUP;
    }
    
    return 0;
}

static int spi_kb1200_transceive(const struct device *dev,
				   const struct spi_config *spi_cfg,
				   const struct spi_buf_set *tx_bufs,
				   const struct spi_buf_set *rx_bufs)
{
    SPIH_T *spih = HAL_INSTANCE(dev);
	const struct spi_buf *pTx = tx_bufs->buffers;
	const struct spi_buf *pRx = rx_bufs->buffers;
	uint8_t *pDat;
	int ret = 0;
    /*
    	const struct spi_buf{
		.buf : pointer
		.len : data len
	};
	    const struct spi_buf_set{
		.buffers : spi_buf pointer
		.count : data len?
	};
    */

	if (!spi_cfg) {
		return -EINVAL;
	}
	
	/* check new configuration */
	ret = spi_feature_support(spi_cfg);
	if (ret) {
		return ret;
	}
	
	/* Setting new configuration */
    spi_kb1200_configure(dev,spi_cfg);

    /* CS Active */
    if(spi_cfg->operation & SPI_CS_ACTIVE_HIGH)
        spih->SPIHCTR &= ~0x01;                     //idle set high
    else
        spih->SPIHCTR |= 0x01;                      //idle set low
    
    /* Issue Tx */
    printk("\nTx(%x):",pTx->len);
    pDat = (uint8_t *)pTx->buf;
    for(int i=0;i<(pTx->len); i++)
    {
        spih->SPIHTBUF = *pDat;
        printk("%x ",*pDat);
        while(spih->SPIHCTR&0x80);
        pDat++;
    }
    
    /* Issue Rx */
    printk(" Rx(%x):",pRx->len);
    pDat = (uint8_t *)pRx->buf;
    for(int i=0;i<(pRx->len); i++)
    {
        spih->SPIHTBUF = SPIH_DUMMY_BYTE;
        while(spih->SPIHCTR&0x80);
        *pDat = spih->SPIHRBUF;
        printk("%x ",*pDat);
        pDat++;
        
    }
    
    /* CS In-Active */
    if(!(spi_cfg->operation&SPI_HOLD_ON_CS))
    {
        if(spi_cfg->operation & SPI_CS_ACTIVE_HIGH)
            spih->SPIHCTR |= 0x01;                      //idle set low
        else
            spih->SPIHCTR &= ~0x01;                     //idle set high
    }
    
	return ret;
}

int spi_kb1200_release(const struct device *dev,
			 const struct spi_config *config)
{
    SPIH_T *spih = HAL_INSTANCE(dev);
    printk("spi_kb1200_release\n");
    spih->SPIHCFG &= ~0x01;
    spih->SPIHCTR &= ~0x01;
    
	return 0;
}
#define GCFG_REG_BASE       ((GCFG_T*)GCFG_BASE)
static int spi_kb1200_init(const struct device *dev)
{
    SPIH_T *spih = HAL_INSTANCE(dev);
    GCFG_T *gcfg_regs = GCFG_REG_BASE;
    
    printk("spi_kb1200_init\n");
    gpio_pinmux_set(SPIH_CS>>5  , SPIH_CS&0x1F  , PINMUX_FUNC_B);
    gpio_pinmux_set(SPIH_CLK>>5 , SPIH_CLK&0x1F , PINMUX_FUNC_B);
    gpio_pinmux_set(SPIH_MOSI>>5, SPIH_MOSI&0x1F, PINMUX_FUNC_B);
    gpio_pinmux_set(SPIH_MISO>>5, SPIH_MISO&0x1F, PINMUX_FUNC_A);       //FS/OE disable and IE enable
    gcfg_regs->GPIOMUX |= 0x03;
    
    spih->SPIHCFG |= 0x41;                   //push pull and enable
    
	return 0;
}

static struct spi_driver_api spi_kb1200_api = {
	.transceive = spi_kb1200_transceive,
	.release = spi_kb1200_release,
};

static const struct kb1200_spi_config kb1200_spi_config = {
	.base_addr = DT_INST_REG_ADDR(0),
};

static struct kb1200_spi_data kb1200_spi_data = {
	SPI_CONTEXT_INIT_LOCK(kb1200_spi_data, ctx),
};

DEVICE_DT_INST_DEFINE(0, &spi_kb1200_init, NULL, &kb1200_spi_data,
		      &kb1200_spi_config, POST_KERNEL,
		      CONFIG_SPI_INIT_PRIORITY, &spi_kb1200_api);
