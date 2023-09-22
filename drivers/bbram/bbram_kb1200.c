/*
 * Copyright (c) 2021 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ene_kb1200_bbram

#include <zephyr/drivers/bbram.h>
#include <errno.h>
#include <zephyr/sys/util.h>
#include <soc.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bbram, LOG_LEVEL_ERR);

/** Device config */
struct bbram_kb1200_config {
	/** BBRAM base address */
	uintptr_t base_addr;
};

/* Driver data */
struct bbram_kb1200_data {
	uint32_t status;
};

/* Driver convenience defines */
#define DRV_CONFIG(dev)     ((const struct bbram_kb1200_config *)(dev)->config)
#define DRV_DATA(dev)       ((struct bbram_kb1200_data *)(dev)->data)
#define HAL_INSTANCE(dev)   (VBAT_T *)(DRV_CONFIG(dev)->base_addr)

static int bbram_kb1200_check_invalid(const struct device *dev)
{
    VBAT_T *vbat = HAL_INSTANCE(dev);
    
    DRV_DATA(dev)->status = vbat->BKUPSTS;
	return (vbat->BKUPSTS&0x80);
}

static int bbram_kb1200_check_standby_power(const struct device *dev)
{
    VBAT_T *vbat = HAL_INSTANCE(dev);
    DRV_DATA(dev)->status = vbat->BKUPSTS;
	return (vbat->BKUPSTS&0x02);
}

static int bbram_kb1200_check_power(const struct device *dev)
{
    VBAT_T *vbat = HAL_INSTANCE(dev);
    DRV_DATA(dev)->status = vbat->BKUPSTS;
	return (vbat->BKUPSTS&0x01);
}

static int bbram_kb1200_get_size(const struct device *dev, size_t *size)
{
    VBAT_T *vbat = HAL_INSTANCE(dev);
	*size = 1+ sizeof(vbat->PASCR)/sizeof(uint8_t);
	return 0;
}

static int bbram_kb1200_read(const struct device *dev, size_t offset, size_t size,
			   uint8_t *data)
{
    VBAT_T *vbat = HAL_INSTANCE(dev);
    
    size_t bbram_Max_Size;
	bbram_kb1200_get_size(dev, &bbram_Max_Size);
    if(bbram_kb1200_check_invalid(dev))
    {
        printk("bbram data invaild.\n");
        return 1;
    }
    if((offset+size)>bbram_Max_Size)
    {
        printk("bbram out of range.\n");
        return 1;
    }
    
    //memcpy((void*)data,(void*)(vbat->PASCR +offset), size);
    memcpy((void*)data,(void*)(vbat->BKUPSTS +offset), size);
	printk("bbram_kb1200_read\n");
	for (int i = 0; i < size; i++)
	{
		printk("0x%02X,", data[i]);
		if ((i % 0x10) == 0x0F)
			printk("\n");
	}	
    return 0;	
}

static int bbram_kb1200_write(const struct device *dev, size_t offset, size_t size,
			    const uint8_t *data)
{
    VBAT_T *vbat = HAL_INSTANCE(dev);
    
    size_t bbram_Max_Size;
	bbram_kb1200_get_size(dev, &bbram_Max_Size);
    if(bbram_kb1200_check_invalid(dev))
    {
        printk("bbram data invaild.\n");
        return 1;
    }
    if((offset+size)>bbram_Max_Size)
    {
        printk("bbram out of range.\n");
        return 1;
    }
    //memcpy((void*)(vbat->PASCR +offset),(void*)data, size);
    memcpy((void*)(vbat->BKUPSTS +offset),(void*)data, size);
    
	printk("bbram_kb1200_write\n");
	for (int i = 0; i < size; i++)
	{
		printk("0x%02X,", vbat->PASCR[i+offset]);
		if ((i % 0x10) == 0x0F)
			printk("\n");
	}
	return 0;
}

static const struct bbram_driver_api bbram_kb1200_driver_api = {
	.check_invalid = bbram_kb1200_check_invalid,
	.check_standby_power = bbram_kb1200_check_standby_power,
	.check_power = bbram_kb1200_check_power,
	.get_size = bbram_kb1200_get_size,
	.read = bbram_kb1200_read,
	.write = bbram_kb1200_write,
};

#define BBRAM_KB1200_DEVICE(inst)                                                                       \
	static struct bbram_kb1200_data bbram_data_##inst;                                         \
	static const struct bbram_kb1200_config bbram_cfg_##inst = {                               \
		.base_addr = (uintptr_t)DT_INST_REG_ADDR(inst),                                        \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &bbram_data_##inst, &bbram_cfg_##inst,             \
			      PRE_KERNEL_1, CONFIG_BBRAM_INIT_PRIORITY, &bbram_kb1200_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BBRAM_KB1200_DEVICE);
