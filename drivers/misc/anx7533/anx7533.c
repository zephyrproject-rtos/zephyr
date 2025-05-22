/*
 * Copyright (c) 2025 Applied Materials
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/drivers/i2c.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/kernel.h>
#include "anx7533_reg.h"
#include "anx7533_config.h"
#include "anx7533_flash.h"
#include "anx7533_panel.h"

#define DT_DRV_COMPAT analogix_anx7533

LOG_MODULE_REGISTER(anx7533, LOG_LEVEL_INF);

static unsigned char edid_hdmi[EDID_LENGTH] = {
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x05, 0xD8, 0x30, 0x75, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x1B, 0x01, 0x03, 0x81, 0x06, 0x05, 0x78, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00,
    0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x7B, 0x9A, 0x40, 0x8C, 0xB0, 0xA0, 0x10, 0x50, 0x30, 0x08,
    0x81, 0x00, 0x3C, 0x32, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x41,
    0x4E, 0x58, 0x37, 0x35, 0x33, 0x30, 0x5F, 0x48, 0x44, 0x4D, 0x49, 0x0A, 0x00, 0x00, 0x00, 0x10,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x33};

static unsigned char edid_extension_hdmi[EDID_EXTENSION_LENGTH] = {
    0x02, 0x03, 0x4D, 0xC2, 0x5B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x29, 0x09, 0x07, 0x07, 0x11, 0x17, 0x50, 0x51, 0x07, 0x00, 0x83, 0x01, 0x00, 0x00, 0x76, 0x03,
    0x0C, 0x00, 0x10, 0x00, 0x00, 0x44, 0x20, 0xC0, 0x84, 0x01, 0x02, 0x03, 0x04, 0x01, 0x41, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x67, 0xD8, 0x5D, 0xC4, 0x01, 0x78, 0x80, 0x00, 0x56, 0x98, 0x40,
    0x60, 0xB0, 0xA0, 0x10, 0x50, 0x30, 0x08, 0x81, 0x00, 0x3C, 0x32, 0x00, 0x00, 0x00, 0x10, 0x8E,
    0x65, 0x40, 0x60, 0xB0, 0xA0, 0x10, 0x50, 0x30, 0x08, 0x81, 0x00, 0x3C, 0x32, 0x00, 0x00, 0x00,
    0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2E};

static unsigned char edid_dp[EDID_LENGTH] = {
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x05, 0xD8, 0x39, 0x75, 0x01, 0x00, 0x00, 0x00,
    0x1E, 0x1C, 0x01, 0x04, 0xA5, 0x06, 0x05, 0x78, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00,
    0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0xF7, 0x32, 0x80, 0x34, 0x71, 0xC0, 0x10, 0x30, 0x20, 0x04,
    0x82, 0x40, 0x3C, 0x32, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x41, 0x4E, 0x58,
    0x37, 0x35, 0x33, 0x30, 0x20, 0x55, 0x0A, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x85};

static unsigned char edid_extension_dp[EDID_EXTENSION_LENGTH] = {
    0x02, 0x03, 0x28, 0x40, 0x5B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x23, 0x09, 0x7F, 0x07, 0x83, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02};

struct anx7533_config {
	struct i2c_dt_spec bus;
	uint16_t reg_offset;
	uint16_t reg_offset_rd;
	struct gpio_dt_spec vid_en_pin;
	struct gpio_dt_spec vid_rst_pin;
	struct gpio_dt_spec vid_int_pin;
};

struct anx7533_priv {
	uint8_t dev;
	uint16_t dev_addr;
	uint16_t select_offset_addr;
	uint16_t select_offset_rd_addr;
	struct k_mutex  lock;
	struct k_mutex  pwr_lock;
	uint8_t	chip_power_status;
	struct anx7533_irq_queue irq_q;
	uint8_t *edid_buffer;
	uint8_t *edid_extension_buffer;
	enum anx7533_state current_state;
	uint8_t dp_cable;
	uint8_t cts_testing;
	uint8_t cts_testing_lane;
	uint8_t cts_testing_speed;
	uint8_t audo_flash;
	struct k_work_q workq;
	struct gpio_callback gpio_irq_cb;
	const struct device *anx_dev;
};

struct anx7533_work_item {
	struct k_work_delayable dwork;
	const struct device *dev;
};
static struct anx7533_work_item work_item;


int anx7533_i2c_write_byte(const struct device *dev, uint8_t slave_id,
			   uint16_t offset_addr, uint8_t data)
{
	struct anx7533_priv *priv = (struct anx7533_priv *)(dev->data);
	const struct anx7533_config *cfg = dev->config;
	char buf[1] = {0};
	int err = 0;

	LOG_DBG("I2C write id=%02X offset=%04X data=%02X\n", slave_id,
		offset_addr, data);

	if ((((slave_id & 0x0F) != 0) && ((offset_addr & 0xFF00) !=0)) ||
	    ((offset_addr & 0xF000) != 0)) {
		LOG_ERR("I2C slave_id or offset_addr ERROR!! %02x %04x\n",
			slave_id, offset_addr);
		return -EINVAL;
	}

	buf[0] = (slave_id | (uint8_t)((offset_addr & 0x0F00) >> 8));
	err = i2c_reg_write_byte(cfg->bus.bus, priv->dev_addr, 0x00, buf[0]);
	if (err < 0) {
		LOG_ERR("failed to write i2c addr=%x\n", priv->dev_addr);
		return err;
	}

	buf[0] = (uint8_t)(offset_addr & 0x00FF);
	err = i2c_reg_write_byte(cfg->bus.bus, priv->select_offset_addr, buf[0],
				 data);
	if (err < 0)
		LOG_ERR("failed to write i2c addr=%x\n", offset_addr);

	return err;
}

static int anx7533_i2c_write_byte_keep(const struct device *dev, uint16_t offset_addr, uint8_t data)
{
	struct anx7533_priv *priv = (struct anx7533_priv *)(dev->data);
	const struct anx7533_config *cfg = dev->config;
	char buf[1] = {0};
	int err = 0;

	LOG_DBG("I2C write -- offset=%04X data=%02X\n", offset_addr, data);

	buf[0] = (uint8_t)(offset_addr & 0x00FF);
	err = i2c_reg_write_byte(cfg->bus.bus, priv->select_offset_addr, buf[0],
				 data);
	if (err < 0)
		LOG_ERR("failed to write i2c addr=%x\n", offset_addr);

	return err;
}

static int anx7533_i2c_write_block(const struct device *dev, uint8_t slave_id,
			    uint16_t offset_addr, uint8_t len, uint8_t *p_data)
{
	struct anx7533_priv *priv = (struct anx7533_priv *)(dev->data);
	const struct anx7533_config *cfg = dev->config;
	uint8_t buf[2] = {0};
	int i, err = 0;

	LOG_DBG("I2C block write id=%02X offset=%04X len=%08X\n", slave_id,
		offset_addr, len);

	if ((((slave_id & 0x0F) != 0) && ((offset_addr & 0xFF00) != 0)) ||
	    ((offset_addr & 0xF000) != 0)) {
		LOG_INF("I2C slave_id or offset_addr ERROR!! %02x %04x\n",
			 slave_id, offset_addr);
		return -EINVAL;
	}

	buf[0] = (slave_id | (uint8_t)((offset_addr & 0x0F00) >> 8));
	err = i2c_reg_write_byte(cfg->bus.bus, priv->dev_addr, 0x00, buf[0]);
	if (err < 0) {
		LOG_ERR("failed to write i2c addr=%x\n", priv->dev_addr);
		return err;
	}

	for (i = 0; i < len; i++) {
		buf[0] = (uint8_t)((offset_addr + i) & 0x00FF);
		err = i2c_reg_write_byte(cfg->bus.bus,
					 priv->select_offset_addr,
					 buf[0],
					 p_data[i]);
		if (err < 0) {
			LOG_ERR("failed to write i2c block %u addr=%x\n", i,
				offset_addr);
			break;
		}
	}

	return err;
}

static int anx7533_i2c_write_byte4(const struct device *dev, uint8_t slave_id,
			    uint16_t offset_addr, uint32_t data)
{
	uint8_t  buf[4] = {0};
	int err = 0;
	int i;

	for (i = 0; i < 4;  i++) {
		buf[i] = (uint8_t)(data & 0x000000FF);
		data = data >> 8;
	}

	err = anx7533_i2c_write_block(dev, slave_id, offset_addr, 4, (uint8_t *)buf);

	return err;
}

int anx7533_i2c_read_byte(const struct device *dev, uint8_t slave_id,
			  uint16_t offset_addr, uint8_t *p_data)
{
	struct anx7533_priv *priv = (struct anx7533_priv *)(dev->data);
	const struct anx7533_config *cfg = dev->config;
	char buf[1] = {0};
	int err = 0;

	LOG_DBG("I2C read id=%02X offset=%04X\n", slave_id, offset_addr);

	if ((((slave_id & 0x0F) != 0) && ((offset_addr & 0xFF00) != 0)) ||
	    ((offset_addr & 0xF000) != 0)) {
		LOG_ERR("I2C slave_id or offset_addr ERROR!! %02x %04x\n",
			slave_id, offset_addr);
		return -EINVAL;
	}

	buf[0] = (slave_id | (uint8_t)((offset_addr & 0x0F00) >> 8));
	err = i2c_reg_write_byte(cfg->bus.bus, priv->dev_addr, 0x00, buf[0]);
	if (err < 0) {
		LOG_ERR("failed to write i2c addr=%x\n", priv->dev_addr);
		return err;
	}

	err = i2c_reg_read_byte(cfg->bus.bus, priv->select_offset_addr,
				(uint8_t)(offset_addr & 0x00FF), p_data);
	if (err)
		LOG_ERR("Error reading offset %x to ANX7533: slave id %x\n",
			offset_addr, slave_id);

	return err;
}

static int anx7533_i2c_read_block(const struct device *dev, uint8_t slave_id,
			   uint16_t offset_addr, uint8_t len, uint8_t *p_data)
{
	struct anx7533_priv *priv = (struct anx7533_priv *)(dev->data);
	const struct anx7533_config *cfg = dev->config;
	char buf[2] = {0};
	int err = 0;

	LOG_DBG("read block id=%02X offset=%04X len=%02X\n", slave_id,
		offset_addr, len);

	if ((((slave_id & 0x0F)!= 0) && ((offset_addr & 0xFF00) != 0)) ||
	    ((offset_addr & 0xF000) != 0)) {
		LOG_ERR("I2C slave_id or offset_addr ERROR!! %02x %04x\n",
			slave_id,offset_addr);
		return -EINVAL;
	}

	buf[0] = (slave_id | (uint8_t)((offset_addr & 0x0F00) >> 8));
	err = i2c_reg_write_byte(cfg->bus.bus, priv->dev_addr, 0x00, buf[0]);
	if (err < 0) {
		LOG_ERR("failed to write i2c addr=%x\n", priv->dev_addr);
		return err;
	}

	err = i2c_burst_read(cfg->bus.bus, priv->select_offset_addr,
			     (uint8_t)(offset_addr & 0x00FF), p_data, len);
	if (err < 0)
		LOG_ERR("failed to burst read i2c addr=%x\n", offset_addr);

	return err;
}

static int anx7533_i2c_read_byte4(const struct device *dev, uint8_t slave_id, uint16_t offset_addr,
		   uint32_t *p_data)
{
	int ret = 0;
	uint8_t buf[4] = {0};
	int i;

	ret = anx7533_i2c_read_block(dev, slave_id, offset_addr, 4, buf);

	if(ret>=0){
		*p_data = 0;
		for(i=0;i<3;i++){
			*p_data += (uint32_t)(buf[3-i]);
			*p_data = *p_data << 8;
		}
		*p_data += (uint32_t)(buf[3-i]);
	}

	return ret;
}

static int anx7533_wakeup(const struct device *dev)
{
	struct anx7533_priv *priv = (struct anx7533_priv *)(dev->data);
	const struct anx7533_config *cfg = dev->config;
	uint8_t write_data[1] = {0};
	int err;

	/* check i2c conection to ANX7533 */
	err = i2c_reg_write_byte(cfg->bus.bus, priv->dev_addr, 0x00, write_data[0]);
	if (err)
		LOG_ERR("Can't write data to anx7533 due to: %d ", err);
	else
		LOG_INF("ANX7533 is connected\n");

	return err;
}

void anx7533_chip_poweron(const struct device *dev)
{
	struct anx7533_priv *priv = (struct anx7533_priv *)(dev->data);
	const struct anx7533_config *cfg = dev->config;

	LOG_ERR("anx7533 chip power on\n");

	gpio_pin_set_dt(&cfg->vid_rst_pin, 0);
	k_busy_wait(ANX7533_RESET_DOWN_DELAY*1000);
	gpio_pin_set_dt(&cfg->vid_en_pin, 0);
	k_busy_wait(ANX7533_CHIPPOWER_DOWN_DELAY*1000);

	gpio_pin_set_dt(&cfg->vid_en_pin, 1);
	k_busy_wait(ANX7533_CHIPPOWER_UP_DELAY*1000);
	gpio_pin_set_dt(&cfg->vid_rst_pin, 1);
	k_busy_wait(ANX7533_RESET_UP_DELAY*1000);

	priv->chip_power_status = VALUE_ON;

	LOG_ERR("anx7533 chip power on %x\n", priv->chip_power_status);
}

void anx7533_chip_powerdown(const struct device *dev)
{
	LOG_ERR("anx7533 chip power down\n");
	struct anx7533_priv *priv = (struct anx7533_priv *)(dev->data);
	const struct anx7533_config *cfg = dev->config;

	priv->chip_power_status = VALUE_OFF;

	gpio_pin_set_dt(&cfg->vid_rst_pin, 0);
	k_busy_wait(ANX7533_RESET_DOWN_DELAY*1000);
	gpio_pin_set_dt(&cfg->vid_en_pin, 0);
	k_busy_wait(ANX7533_CHIPPOWER_DOWN_DELAY*1000);

	LOG_ERR("anx7533 chip power off %x\n", priv->chip_power_status);
}

static void anx7533_set_checking_link_speed(const struct device *dev)
{
	uint8_t reg_temp;

	anx7533_i2c_read_byte(dev, SLAVEID_SPI, MISC_NOTIFY_OCM0, &reg_temp);
	reg_temp |= ENABLE_DP_LS_CHECK;
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, MISC_NOTIFY_OCM0, reg_temp);
}

static void anx7533_get_ocm_version(const struct device *dev, uint8_t *major,
				    uint8_t *minor)
{
	uint8_t reg_temp;

	anx7533_i2c_read_byte(dev, SLAVEID_SPI, OCM_VERSION_MAJOR, &reg_temp);

	*major = reg_temp >> 4;
	*minor = reg_temp & 0x0F;
}

#if 0
static void anx7533_set_reset_dp_phy_when_video_mute(const struct device *dev)
{
	struct anx7533_priv *priv = (struct anx7533_priv *)(dev->data);
	const struct anx7533_config *cfg = dev->config;
	uint8_t reg_temp;

	anx7533_i2c_read_byte(dev, SLAVEID_SPI, MISC_NOTIFY_OCM0, &reg_temp);
	reg_temp |= RESET_DP_PHY_WHEN_VIDEO_MUTE;
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, MISC_NOTIFY_OCM0, reg_temp);
}
#endif

static void anx7533_set_video_stable_delay_time(const struct device *dev,
						uint16_t delay_time)
{
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, VIDEO_STABLE_DELAY_L,
			       (uint8_t)(delay_time & 0x00FF));
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, VIDEO_STABLE_DELAY_H,
			       (uint8_t)((delay_time >> 8) & 0x00FF));
}

static uint16_t anx7533_get_pixel_clock(const struct device *dev)
{
	uint32_t pixel_clk_full;
	uint16_t h_active,hfp,hsync,hbp;
	uint16_t v_active,vfp,vsync,vbp;
	uint8_t temp_char;

	temp_char = MIPI_VIDEO_MODE;

	if(VIDEOMODE_STACKED!=temp_char){
		h_active = (PANEL_H_ACTIVE*PANEL_COUNT);
		hfp = (PANEL_HFP*PANEL_COUNT);
		hsync = (PANEL_HSYNC*PANEL_COUNT);
		hbp = (PANEL_HBP*PANEL_COUNT);;
		v_active = PANEL_V_ACTIVE;
		vfp = PANEL_VFP;
		vsync = PANEL_VSYNC;
		vbp = PANEL_VBP;
	}else{
		h_active = PANEL_H_ACTIVE;
		hfp = PANEL_HFP;
		hsync = PANEL_HSYNC;
		hbp = PANEL_HBP;;
		v_active = (PANEL_V_ACTIVE*PANEL_COUNT);
		vfp = (PANEL_VFP*PANEL_COUNT);
		vsync = (PANEL_VSYNC*PANEL_COUNT);
		vbp = (PANEL_VBP*PANEL_COUNT);
	}

	temp_char = MIPI_DSC_STATUS;
	if(DSC_ONE_TO_THREE == temp_char){
		LOG_INF("DSC pixel clock\n");
		h_active = h_active*3;
		hfp = hfp*3;
		hsync = hsync*3;
		hbp = hbp*3;;
	}

	pixel_clk_full = ((uint32_t)(h_active+hfp+hsync+hbp))*((uint32_t)(v_active+vfp+vsync+vbp))*((uint32_t)PANEL_FRAME_RATE);
	return((uint16_t)(pixel_clk_full/(uint32_t)10000));
}

static uint16_t anx7533_read_chip_id(const struct device *dev)
{
	uint8_t reg_temp;
	uint16_t reg_int;

	anx7533_i2c_read_byte(dev, SLAVEID_SPI, CHIP_ID_HIGH, &reg_temp);
	reg_int = ((((uint16_t)reg_temp) << 8) & 0xFF00);
	anx7533_i2c_read_byte(dev, SLAVEID_SPI, CHIP_ID_LOW, &reg_temp);
	reg_int |= (((uint16_t)reg_temp) & 0x00FF);

	LOG_INF("Chip ID = %04X\n", reg_int);
	return reg_int;
}

static char anx7533_ocm_crc_checking(const struct device *dev)
{
	uint8_t reg_temp;
	int rc;

	// OCM FW CRC can check from 01:05 bit6 "BOOT_LOAD_DON"
	rc = anx7533_i2c_read_byte(dev, SLAVEID_SPI, R_RAM_CTRL, &reg_temp);
	//pr_info("%s: %s rc=%d\n", LOG_TAG, __func__, rc);
	if (VALUE_SUCCESS == rc) {
		if(BOOT_LOAD_DONE == (BOOT_LOAD_DONE & reg_temp)){
			return VALUE_SUCCESS;
		}
		return VALUE_FAILURE;
	}
	return VALUE_FAILURE2;
}

void anx7533_state_change(const struct device *dev, enum anx7533_state state)
{
	struct anx7533_priv *priv = (struct anx7533_priv *)(dev->data);

	LOG_INF("state change to %d\n", state);
	priv->current_state = state;
}

enum anx7533_state anx7533_get_current_state(const struct device *dev)
{
	struct anx7533_priv *priv = (struct anx7533_priv *)(dev->data);

	LOG_DBG("current state %d\n", priv->current_state);

	return priv->current_state;
}


uint8_t anx7533_get_cts_state(const struct device *dev)
{
	struct anx7533_priv *priv = (struct anx7533_priv *)(dev->data);

	return priv->cts_testing;
}

static void anx7533_dp_phy_cts(const struct device *dev)
{
	struct anx7533_priv *priv = (struct anx7533_priv *)(dev->data);
	uint8_t reg_temp;

	anx7533_i2c_read_byte(dev, SLAVEID_DPCD, TEST_SINK, &reg_temp);
	if (0 == (reg_temp & PHY_SINK_TEST_LANE_EN)) {
		priv->cts_testing = VALUE_OFF;
		return;
	}

	reg_temp = ((reg_temp & PHY_SINK_TEST_LANE_SEL) >> PHY_SINK_TEST_LANE_SEL_POS);
	if(priv->cts_testing_lane != (reg_temp + 1)) {
		LOG_INF("SINK TEST=%02X\n", reg_temp);
		priv->cts_testing_speed = 0;
		// reset BW
		anx7533_i2c_write_byte(dev, SLAVEID_DPCD, LINK_BW_SET, 0x00);
		priv->cts_testing_lane = reg_temp + 1;
		LOG_INF("CTS testing Lane=%02X\n", reg_temp);
		switch (reg_temp) {
			case 0: // Lane 0
				reg_temp = ALL_SET_LANE0;
				break;
			case 1: // Lane 1
				reg_temp = ALL_SET_LANE1;
				break;
			case 2: // Lane 2
				reg_temp = ALL_SET_LANE2;
				break;
			case 3: // Lane 3
				reg_temp = ALL_SET_LANE3;
				break;
			default: // Lane 0
				reg_temp = ALL_SET_LANE0;
				break;
		}
		anx7533_i2c_write_byte(dev, SLAVEID_SERDES, SERDES_REG_38, reg_temp);

		// Remove link CTS setting in OCM.
		anx7533_i2c_read_byte(dev, SLAVEID_DP_IP, ADDR_SYSTEM_CTRL_0, &reg_temp);
		reg_temp |= SYNC_STATUS_SEL;
		anx7533_i2c_write_byte(dev, SLAVEID_DP_IP, ADDR_SYSTEM_CTRL_0, reg_temp);
		anx7533_i2c_read_byte(dev, SLAVEID_DP_IP, ADDR_RCD_PN_CONVERTE, &reg_temp);
		reg_temp |= BYPASS_RC_PAT_CHK;
		anx7533_i2c_write_byte(dev, SLAVEID_DP_IP, ADDR_RCD_PN_CONVERTE, reg_temp);
	}

	// BW setting
	anx7533_i2c_read_byte(dev, SLAVEID_DPCD, LINK_BW_SET, &reg_temp);
	if (priv->cts_testing_speed != reg_temp){
		priv->cts_testing_speed = reg_temp;
		LOG_INF("CTS testing speed=%02X\n", reg_temp);
		switch (reg_temp){
			case DPCD_BW_1P62G:
			case DPCD_BW_2P7G:
				anx7533_i2c_write_byte(dev, SLAVEID_SERDES, SERDES_SET_8_RX_REG8, 0x0E);
				anx7533_i2c_write_byte(dev, SLAVEID_SERDES, REG7_2_RX_REG7, 0x8D);

				// 2.7G boost, bit7~bit4, customer could modify from 0000 to 1101.
				anx7533_i2c_write_byte(dev, SLAVEID_SERDES, REG7_1_RX_REG7, 0x8D); // 2.7G boost

				anx7533_i2c_write_byte(dev, SLAVEID_SERDES, REG16_2_RX_REG16, 0xD0);
				anx7533_i2c_write_byte(dev, SLAVEID_SERDES, SERDES_SET_1_RX_REG1, 0x85);
				anx7533_i2c_write_byte(dev, SLAVEID_SERDES, SERDES_SET_2_RX_REG2, 0xC5);
				anx7533_i2c_write_byte(dev, SLAVEID_SERDES, REG7_0_RX_REG7, 0xDD);
				anx7533_i2c_write_byte(dev, SLAVEID_SERDES, SERDES_SET_5_RX_REG5, 0x0C);
				anx7533_i2c_write_byte(dev, SLAVEID_SERDES, SERDES_SET_9_RX_REG9, 0x0B);
				anx7533_i2c_write_byte(dev, SLAVEID_SERDES, SERDES_SET_15_RX_REG15, 0xB0);

			break;
			case DPCD_BW_5P4G:
			case DPCD_BW_6P75G:
				anx7533_i2c_write_byte(dev, SLAVEID_SERDES, REG7_0_RX_REG7, 0x0D);
				anx7533_i2c_write_byte(dev, SLAVEID_SERDES, SERDES_SET_9_RX_REG9, 0x07);
				anx7533_i2c_write_byte(dev, SLAVEID_SERDES, SERDES_SET_15_RX_REG15, 0x70);
				anx7533_i2c_write_byte(dev, SLAVEID_SERDES, SERDES_SET_8_RX_REG8, 0x4E);

				// 5.4G boost, bit7~bit4, customer could modify from 0000 to 1101.
				anx7533_i2c_write_byte(dev, SLAVEID_SERDES, REG7_2_RX_REG7, 0xDD); // 5.4G boost

				anx7533_i2c_write_byte(dev, SLAVEID_SERDES, REG16_2_RX_REG16, 0xC8);
				anx7533_i2c_write_byte(dev, SLAVEID_SERDES, SERDES_SET_5_RX_REG5, 0x0E);
				anx7533_i2c_write_byte(dev, SLAVEID_SERDES, SERDES_SET_1_RX_REG1, 0x8F);
				anx7533_i2c_write_byte(dev, SLAVEID_SERDES, SERDES_SET_2_RX_REG2, 0xCF);
				anx7533_i2c_write_byte(dev, SLAVEID_DP_IP, ADDR_SYSTEM_CTRL_1, 0x10);
			break;
			default:
				// Do nothing
				break;
		}
	}
}

static void anx7533_edid_write_buffer(const struct device *dev)
{
	struct anx7533_priv *priv = (struct anx7533_priv *)(dev->data);
	uint16_t checksum;
	uint16_t count;
#ifdef CHICAGO_FEATURE_EDID_AUTO
	uint16_t temp_int;
	uint16_t hactive, hbp, hfp, hsync;
#endif

	checksum = 0;
	count = 0;

#ifdef CHICAGO_FEATURE_EDID_AUTO
	temp_int = MIPI_DSC_STATUS;
	if (DSC_ONE_TO_THREE == temp_int) {
		hactive = PANEL_H_ACTIVE * 3;
		hbp = PANEL_HBP * 3;
		hsync = PANEL_HSYNC * 3;
		hfp = PANEL_HFP * 3;
	} else {
		hactive = PANEL_H_ACTIVE;
		hbp = PANEL_HBP;
		hsync = PANEL_HSYNC;
		hfp = PANEL_HFP;
	}

	anx7533_i2c_write_byte(dev, SLAVEID_EDID, count, priv->edid_buffer[0]);
	checksum = priv->edid_buffer[0];
	for(count = 1; count < EDID_DB1_BASE; count++){
		anx7533_i2c_write_byte_keep(dev, count, priv->edid_buffer[count]);
		checksum = (checksum + priv->edid_buffer[count]);
	}

	temp_int = anx7533_get_pixel_clock(dev);
	LOG_INF("EDID pixel_clk=%d\n", (temp_int / 100));
	// Pixel clock
	anx7533_i2c_write_byte_keep(dev, EDID_DB1_BASE + EDID_PIXEL_CLK_L,
				    (uint8_t)(temp_int & 0x00FF));
	checksum = (checksum + (uint8_t)(temp_int & 0x00FF));
	anx7533_i2c_write_byte_keep(dev, EDID_DB1_BASE+EDID_PIXEL_CLK_H,
				    (uint8_t)((temp_int>>8)&0x00FF));
	checksum = (checksum + (uint8_t)((temp_int>>8 ) &0x00FF));
	// H active low bit
	anx7533_i2c_write_byte_keep(dev, EDID_DB1_BASE + EDID_HACTIVE_L,
			    (uint8_t)((hactive * PANEL_COUNT) & 0x00FF));
	checksum = (checksum + (uint8_t)((hactive * PANEL_COUNT) & 0x00FF));
	// H blank (HBP+Hsync+HFP) low bit
	temp_int = (hbp + hfp + hsync) * PANEL_COUNT;
	anx7533_i2c_write_byte_keep(dev, EDID_DB1_BASE+EDID_HBP_L,
				    (uint8_t)(temp_int&0x00FF));
	checksum = (checksum + (uint8_t)(temp_int&0x00FF));
	// H active and HBP high bit
	anx7533_i2c_write_byte_keep(dev, EDID_DB1_BASE+EDID_HACT_HBP_H,
			    (uint8_t)((((hactive * PANEL_COUNT) >> 4) & 0x00F0) | ((temp_int >> 8) & 0x000F)));
	checksum = (checksum + (uint8_t)((((hactive * PANEL_COUNT) >> 4) & 0x00F0) | ((temp_int>>8) & 0x000F)));
	// V active low bit
	anx7533_i2c_write_byte_keep(dev, EDID_DB1_BASE+EDID_VACTIVE_L,
				    (uint8_t)(PANEL_V_ACTIVE & 0x00FF));
	checksum = (checksum + (uint8_t)(PANEL_V_ACTIVE & 0x00FF));
	// V blank (VBP+Vsync+VFP) low bit
	temp_int = PANEL_VBP+PANEL_VFP+PANEL_VSYNC;
	anx7533_i2c_write_byte_keep(dev, EDID_DB1_BASE+EDID_VBP_L, (uint8_t)(temp_int & 0x00FF));
	checksum = (checksum + (temp_int&0x00FF));
	// V active and VBP high bit
	anx7533_i2c_write_byte_keep(dev, EDID_DB1_BASE+EDID_VACT_VBP_H,
			    (uint8_t)(((PANEL_V_ACTIVE >> 4) & 0x00F0) | ((temp_int >> 8) & 0x000F)));
	checksum = (checksum + (uint8_t)(((PANEL_V_ACTIVE >> 4) & 0x00F0) | ((temp_int >> 8) & 0x000F)));
	// HFP low bit
	anx7533_i2c_write_byte_keep(dev, EDID_DB1_BASE+EDID_HFP_L,
			    (uint8_t)((hfp*PANEL_COUNT) & 0x00FF));
	checksum = (checksum + (uint8_t)((hfp * PANEL_COUNT) & 0x00FF));
	// HSYNC low bit
	anx7533_i2c_write_byte_keep(dev, EDID_DB1_BASE+EDID_HSYNC_L,
			    (uint8_t)((hsync*PANEL_COUNT) & 0x00FF));
	checksum = (checksum + (uint8_t)((hsync * PANEL_COUNT) & 0x00FF));

	// VFP and VSYNC low bit
	temp_int = PANEL_VFP;
	if (temp_int > EDID_VFP_MAX_VALUE){
		// The value of VFP should under 63 in EDID.
		// cut VFP to 63, and VBP = VBP+(VFP-63) 
		anx7533_i2c_write_byte_keep(dev, EDID_DB1_BASE+EDID_VFP_VSYNC_L,
					    (uint8_t)(((EDID_VFP_MAX_VALUE << 4) & 0x00F0) | (PANEL_VSYNC&0x000F)));
		checksum = (checksum + (uint8_t)(((EDID_VFP_MAX_VALUE << 4) & 0x00F0) | (PANEL_VSYNC & 0x000F)));
		// HFP, HSYNC, VFP, VSYNC high bit
		anx7533_i2c_write_byte_keep(dev, EDID_DB1_BASE+EDID_HFP_HSYNC_VFP_VSYNC_H,
			(uint8_t)((((hfp * PANEL_COUNT) >> 2) & 0x00C0) | (((hsync * PANEL_COUNT )>> 4) & 0x0030) | ((EDID_VFP_MAX_VALUE >> 2) & 0x00C0) | ((PANEL_VSYNC >> 4) & 0x0003)));
		checksum = (checksum + (uint8_t)((((hfp*PANEL_COUNT) >> 2) & 0x00C0)|(((hsync*PANEL_COUNT)>>4)&0x0030)|((EDID_VFP_MAX_VALUE>>2)&0x00C0)|((PANEL_VSYNC>>4)&0x0003)));
	} else {
		anx7533_i2c_write_byte_keep(dev, EDID_DB1_BASE + EDID_VFP_VSYNC_L,
					    (uint8_t)(((PANEL_VFP << 4) & 0x00F0) | (PANEL_VSYNC & 0x000F)));
		checksum = (checksum + (uint8_t)(((PANEL_VFP << 4) & 0x00F0) | (PANEL_VSYNC & 0x000F)));
		// HFP, HSYNC, VFP, VSYNC high bit
		anx7533_i2c_write_byte_keep(dev, EDID_DB1_BASE + EDID_HFP_HSYNC_VFP_VSYNC_H,
			(uint8_t)((((hfp * PANEL_COUNT) >> 2) & 0x00C0) | (((hsync * PANEL_COUNT )>> 4) & 0x0030) | ((PANEL_VFP >> 2) & 0x000C) | ((PANEL_VSYNC >> 4) & 0x0003)));
		checksum = (checksum + (uint8_t)((((hfp * PANEL_COUNT) >> 2) & 0x00C0) | (((hsync * PANEL_COUNT) >> 4) & 0x0030) | ((PANEL_VFP >> 2) & 0x000C) | ((PANEL_VSYNC >> 4) & 0x0003))); 
	}

	count = EDID_DB1_BASE+EDID_H_DISPLAY_SIZE;
	for(; count < EDID_DB2_BASE; count++){
		anx7533_i2c_write_byte_keep(dev, count, priv->edid_buffer[count]);
		checksum = (checksum + priv->edid_buffer[count]);
	}

	// copy other EDID
	count = EDID_DB2_BASE;
	for(; count < (EDID_LENGTH - 1); count++){
		anx7533_i2c_write_byte_keep(dev, count, priv->edid_buffer[count]);
		checksum = (checksum + priv->edid_buffer[count]);
	}
#else
	anx7533_i2c_write_byte(dev, SLAVEID_EDID, count, priv->edid_buffer[0]);
	checksum = priv->edid_buffer[0];
	for(count = 1; count < (EDID_LENGTH-1); count++){
		anx7533_i2c_write_byte_keep(dev, count, priv->edid_buffer[count]);
		checksum = (checksum + priv->edid_buffer[count]);
	}
#endif

	checksum = ((0xff - (uint8_t)(checksum & 0x00FF)) + 1);
	anx7533_i2c_write_byte_keep(dev, count, (uint8_t)(checksum & 0x00FF));

	LOG_INF("edid_write_buffer done, checksum=0x%02X\n",
		 (uint8_t)(checksum & 0x00FF));
}

static void anx7533_edid_write_extension_buffer(const struct device *dev)
{
	struct anx7533_priv *priv = (struct anx7533_priv *)(dev->data);
	uint16_t checksum;
	uint16_t count;

	checksum = 0;
	count = 0;

	anx7533_i2c_write_byte(dev, SLAVEID_EDID, EDID_EXTENSION_BUF,
			       priv->edid_extension_buffer[0]);
	checksum = priv->edid_extension_buffer[0];
	for (count=1; count<(EDID_EXTENSION_LENGTH-1); count++){
		anx7533_i2c_write_byte_keep(dev, EDID_EXTENSION_BUF + count,
					    priv->edid_extension_buffer[count]);
		checksum = (checksum + priv->edid_extension_buffer[count]);
	}

	checksum = ((0xff-(uint8_t)(checksum & 0x00FF))+1);

	anx7533_i2c_write_byte_keep(dev, EDID_EXTENSION_BUF + count,
				    (uint8_t)(checksum & 0x00FF));

	LOG_INF("edid_write_extension_buffer done, checksum=0x%02X\n",
		 (uint8_t)(checksum & 0x00FF));

}

static void anx7533_mipi_mcu_write_done(const struct device *dev)
{
	uint8_t reg_temp;

	anx7533_i2c_read_byte(dev, SLAVEID_SPI, MISC_NOTIFY_OCM0, &reg_temp);
	reg_temp |= MCU_LOAD_DONE;
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, MISC_NOTIFY_OCM0, reg_temp);
}

static uint8_t anx7533_check_ocm_status(const struct device *dev)
{
	uint8_t reg_temp;

	// Chicking OCM status
	anx7533_i2c_read_byte(dev, SLAVEID_SERDES, SERDES_POWER_CONTROL, &reg_temp);
	if (OCM_LOAD_DONE == (OCM_LOAD_DONE & reg_temp)) {
		return VALUE_ON;
	} else {
		LOG_ERR("OCM LOAD NOT COMPLETE\n");
		return VALUE_OFF;
	}
}

static uint8_t anx7533_get_ocm_status(const struct device *dev)
{
	uint8_t reg_temp;

	// Chicking OCM status
	anx7533_i2c_read_byte(dev, SLAVEID_SERDES, SERDES_POWER_CONTROL, &reg_temp);

	return reg_temp;
}


void anx7533_intr_queue_pop(const struct device *dev, uint8_t queue0, uint8_t queue1)
{
	struct anx7533_priv *priv = (struct anx7533_priv *)(dev->data);

	priv->irq_q.q0[priv->irq_q.irq_q_output] = queue0;
	priv->irq_q.q1[priv->irq_q.irq_q_output] = queue1;

	if (priv->irq_q.irq_q_output != priv->irq_q.irq_q_input) {
		if ((0 == priv->irq_q.q0[priv->irq_q.irq_q_output]) &&
			(0 == priv->irq_q.q1[priv->irq_q.irq_q_output])) {
			priv->irq_q.irq_q_output++;
		}
	}

	if (ANX7533_IRQ_QUEUE_SIZE == priv->irq_q.irq_q_output) {
		priv->irq_q.irq_q_output = 0;
	}
}

uint8_t anx7533_check_interrupt_state(const struct device *dev)
{
	const struct anx7533_config *cfg = dev->config;
	bool pin_state;

	pin_state = gpio_pin_get_dt(&cfg->vid_int_pin);
	LOG_INF("gpio pin state is %u\n", pin_state);
	if (pin_state) {
		LOG_INF("no DP cable plug-in\n");
		return SIGNAL_HIGH;
	} else {
		LOG_INF("DP cable is detected\n");
		return SIGNAL_LOW;
	}
}

static void anx7533_irq(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	struct anx7533_priv *priv = CONTAINER_OF(cb, struct anx7533_priv, gpio_irq_cb);
	const struct device *anx_dev = priv->anx_dev;
	uint8_t reg_temp, reg_buf[2];

	LOG_ERR("**interrupt from anx7533 %s\n", dev->name);

	if (VALUE_ON == priv->chip_power_status) {
		// this is not cable-in interrupt when CHIP_POWER_UP and RESET are low
		// read 01:90 and 01:91 by i2c read block funciton
		//anx7533_i2c_read_block(anx_dev, SLAVEID_SPI, INT_NOTIFY_MCU0, 2, &reg_buf[0]);
		anx7533_i2c_read_byte(anx_dev, SLAVEID_SPI, INT_NOTIFY_MCU0, &reg_buf[0]);
		anx7533_i2c_read_byte(anx_dev, SLAVEID_SPI, INT_NOTIFY_MCU1, &reg_buf[1]);
		LOG_ERR("irq data0 %u irq data1 %u\n", reg_buf[0], reg_buf[1]);
		LOG_ERR("input q size %u\n", priv->irq_q.irq_q_input);
		priv->irq_q.q0[priv->irq_q.irq_q_input] = reg_buf[0];
		priv->irq_q.q1[priv->irq_q.irq_q_input] = reg_buf[1];

		if ((0 != reg_buf[0]) || (0 != reg_buf[1])) {
			priv->irq_q.irq_q_input++;
			if (ANX7533_IRQ_QUEUE_SIZE == priv->irq_q.irq_q_input) {
				priv->irq_q.irq_q_input = 0;
			}

			if (priv->irq_q.irq_q_output == priv->irq_q.irq_q_input) {
				LOG_ERR("interrupt queue ERROR!!!\n");
			}
		}

		// clear 01:90 and 01:91 by i2c write block funciton
		reg_buf[0] = 0;
		reg_buf[1] = 0;
		anx7533_i2c_write_block(anx_dev, SLAVEID_SPI, INT_NOTIFY_MCU0, 2, reg_buf);
		// Clear interrupt
		LOG_ERR("clear interrupt\n");
		anx7533_i2c_read_byte(anx_dev, SLAVEID_DP_TOP, ADDR_SW_INTR_CTRL, &reg_temp);
		reg_temp &= ~SOFT_INTR;
		anx7533_i2c_write_byte(anx_dev, SLAVEID_DP_TOP, ADDR_SW_INTR_CTRL, reg_temp);
	}
	k_work_schedule(&work_item.dwork, K_NO_WAIT);
}

static void anx7533_interrupt_handle(const struct device *dev)
{
	struct anx7533_priv *priv = (struct anx7533_priv *)(dev->data);
	struct anx7533_irq_queue *irq_q = &priv->irq_q;
	uint8_t queue0, queue1;

	LOG_ERR("handle interrupt\n");

	if (irq_q->irq_q_input == irq_q->irq_q_output) {
		LOG_ERR("queue empty\n");
		return;
	}

	queue0 = irq_q->q0[irq_q->irq_q_output];
	queue1 = irq_q->q1[irq_q->irq_q_output];

	LOG_INF("intrQ0=%02X, point=%d\n", queue0, irq_q->irq_q_output);

	// Check AUX status
	if (0 != (queue0 & AUX_CABLE_OUT)) {
		LOG_INF("dp cable out\n");
		anx7533_state_change(dev, ANX7533_STATE_NONE);
		// clear interrupt vector
		queue0 &= ~AUX_CABLE_OUT;
	}

	// Check cable in
	if (0 != (queue0 & AUX_CABLE_IN)) {
		LOG_INF("dp cable in\n");
		// clear interrupt vector
		queue0 &= ~AUX_CABLE_IN;
	}

	// Check VIDEO M/N RE-CALCULATE
	if (0 != (queue0 & VIDEO_RE_CALCULATE)) {
		LOG_INF("M/N re-cal\n");
		// clear interrupt vector
		queue0 &= ~VIDEO_RE_CALCULATE;
		// handle panel off first.
		LOG_INF("Panel Off\n");;
		/* TODO: TURN ON/OFF PANEL */
		//panel_panel_turn_on_off(PANEL_TURN_OFF);
	}

	// Check Video input missing, or chip into standby mode
	if (0 != (queue0 & VIDEO_INPUT_EMPTY)) {
		LOG_INF("video empty\n");
		// clear interrupt vector
		queue0 &= ~(VIDEO_INPUT_EMPTY);
		// Turn off panel
		LOG_INF("Panel Off\n");
		//panel_panel_turn_on_off(PANEL_TURN_OFF);
	}

	// Check Video format stable status
	if (0 != (queue0 & VIDEO_STABLE)) {
		LOG_INF("video stable\n");
		// clear interrupt vector
		queue0 &= ~VIDEO_STABLE;
		// Turn on panel
		LOG_INF("Panel On\n");;
		//panel_panel_turn_on_off(PANEL_TURN_ON);
	}

	// Check Audio sample rate changed
	if (0 != (queue0 & AUDIO_MN_RST)) {
		LOG_INF("Audio MN reset\n");
		// clear interrupt vector
		queue0 &= ~AUDIO_MN_RST;
	}

	// Check Audio PLL RST
	if (0 != (queue0 & AUDIO_PLL_RST)) {
		LOG_INF("Audio PLL RST\n");
		// clear interrupt vector
		queue0 &= ~AUDIO_PLL_RST;
	}

	// Check Chip standby mode
	if (0 != (queue1 & CHIP_STANDBY_MODE)) {
		LOG_INF("Chip Standby mode\n");
		// clear interrupt vector
		queue1 &= ~CHIP_STANDBY_MODE;
		// Turn off panel
		LOG_INF("Panel Off\n");;
		//panel_panel_turn_on_off(PANEL_TURN_OFF);
	}

	// Check Chip normal mode
	if (0 != (queue1 & CHIP_NORMAL_MODE)){
		LOG_INF("Chip Normal mode\n");
		// clear interrupt vector
		queue1 &= ~CHIP_NORMAL_MODE;
	}

	// DP PHY CTS testing, start
	if (0 != (queue1 & DP_PHY_CTS_START)) {
		LOG_INF("DP PHY CTS\n");
		// clear interrupt vector
		queue1 &= ~DP_PHY_CTS_START;
		// clear CTS variable
		priv->cts_testing_lane = 0;
		priv->cts_testing_speed = 0;
		// set CTS flag
		priv->cts_testing = VALUE_ON;
	}

	// DP PHY CTS testing, stop
	if (0 != (queue1 & DP_PHY_CTS_STOP)) {
		LOG_INF("STOP DP PHY CTS\n");
		// clear interrupt vector
		queue1 &= ~DP_PHY_CTS_STOP;
		// clear CTS variable
		priv->cts_testing_lane = 0;
		priv->cts_testing_speed = 0;
		// clear CTS flag
		priv->cts_testing = VALUE_OFF;
		// reset Chicago
		anx7533_state_change(dev, ANX7533_STATE_NONE);
	}

	// Link Speed Fail
	if (0 != (queue1 & DP_LINK_TRAINING_FAIL)) {
		LOG_INF("link sleed Fail\n");
		// clear interrupt vector
		queue1 &= ~DP_LINK_TRAINING_FAIL;
	}

	anx7533_intr_queue_pop(dev, queue0, queue1);
}

static void anx7533_hpd_set(const struct device *dev, uint8_t force,
			    uint8_t high_low)
{
	uint8_t reg_temp;

	LOG_INF("anx7533_hpd_set\n");

	anx7533_i2c_read_byte(dev, SLAVEID_DP_IP, ADDR_SYSTEM_CTRL_0, &reg_temp);

	if(HDP_DATA_HIGH == high_low) { // HPD set to High
		reg_temp |= FORCE_HPD_VALUE;
	} else { // HPD set to Low
		reg_temp &= ~FORCE_HPD_VALUE;
	}

	if (HDP_FORCE == force){ // HPD force
		reg_temp |= FORCE_HPD_EN;
	}else{
		reg_temp &= ~FORCE_HPD_EN;
	}

	anx7533_i2c_write_byte(dev, SLAVEID_DP_IP, ADDR_SYSTEM_CTRL_0, reg_temp);
}

void anx7533_irq_queue_clean(const struct device *dev)
{
	struct anx7533_priv *priv = (struct anx7533_priv *)(dev->data);

	priv->irq_q.irq_q_input = 0;
	priv->irq_q.irq_q_output = 0;
}

void anx7533_panel_dcs_send_short_packet(const struct device *dev,
					 struct packet_short *short_packet)
{
	uint8_t slave_id;
	uint32_t reg_long;

	switch(short_packet->mipi_port) {
	case 0:
		slave_id = SLAVEID_MIPI_PORT0;
		break;
	case 1:
		slave_id = SLAVEID_MIPI_PORT1;
		break;
	case 2:
		slave_id = SLAVEID_MIPI_PORT2;
		break;
	case 3:
		slave_id = SLAVEID_MIPI_PORT3;
		break;
	default:
		LOG_ERR("MIPI port selected error!\n");
		return;
	}

	// Write Data Type
	reg_long = (((uint32_t)short_packet->data_type)&0x000000FF);

	// Send other parameters
	switch(short_packet->data_type) {
	case DATASHORT_EoTp:
		reg_long = (reg_long | (((uint32_t)0x0F)<<8));
		reg_long = (reg_long | (((uint32_t)0x0F)<<16));
		break;
	default:
		reg_long = (reg_long | (((uint32_t)short_packet->param1)<<8));
		reg_long = (reg_long | (((uint32_t)short_packet->param2)<<16));
		break;
	}

	anx7533_i2c_write_byte4(dev, slave_id, GEN_HDR, reg_long);

	// Get return data for Read command
	switch (short_packet->data_type) {
	case DATASHORT_GEN_READ_0:
	case DATASHORT_GEN_READ_1:
	case DATASHORT_GEN_READ_2:
	case DATASHORT_DCS_READ_0:
		// Wait > 500us
		k_busy_wait(550);
		anx7533_i2c_read_block(dev, slave_id, GEN_PLD_DATA, 4,
				       &short_packet->pData[0]);
		break;
	default:
		// Do not thing
		break;
	}
}

void panel_dcs_send_long_packet(const struct device *dev,
				struct packet_long *long_packet)

{
	uint8_t	slave_id;
	uint16_t	div,mod,i;
	uint32_t	reg_long;
	uint32_t read_data;
	uint8_t	wait_count;

	switch(long_packet->mipi_port) {
	case 0:
		slave_id = SLAVEID_MIPI_PORT0;
		break;
	case 1:
		slave_id = SLAVEID_MIPI_PORT1;
		break;
	case 2:
		slave_id = SLAVEID_MIPI_PORT2;
		break;
	case 3:
		slave_id = SLAVEID_MIPI_PORT3;
		break;
	default:
		LOG_INF("MIPI port selected error!\n");
		return;
	}

	div = long_packet->word_count/4;
	mod = long_packet->word_count%4;

	// Select port first
	anx7533_i2c_write_byte(dev, SLAVEID_MIPI_CTRL, R_MIP_TX_SELECT,
			       ((0x10)<<(long_packet->mipi_port)));

	// Write Payload first
	for(i=0; i<div; i++) {
		reg_long = (((uint32_t)(long_packet->pData[i*4])) | (((uint32_t)(long_packet->pData[i*4+1]))<<8) |
			(((uint32_t)(long_packet->pData[i*4+2]))<<16) | (((uint32_t)(long_packet->pData[i*4+3]))<<24) );
		anx7533_i2c_write_byte4(dev, slave_id, GEN_PLD_DATA, reg_long);
		k_busy_wait(1000);
	}

	switch(mod) {
	case 3:
		reg_long = (((uint32_t)(long_packet->pData[i*4])) | (((uint32_t)(long_packet->pData[i*4+1]))<<8) |
			(((uint32_t)(long_packet->pData[i*4+2]))<<16) );
		anx7533_i2c_write_byte4(dev, slave_id, GEN_PLD_DATA, reg_long);
		break;
	case 2:
		reg_long = (((uint32_t)(long_packet->pData[i*4])) | (((uint32_t)(long_packet->pData[i*4+1]))<<8) );
		anx7533_i2c_write_byte4(dev, slave_id, GEN_PLD_DATA, reg_long);
		break;
	case 1:
		reg_long = ((uint32_t)(long_packet->pData[i*4]));
		anx7533_i2c_write_byte4(dev, slave_id, GEN_PLD_DATA, reg_long);
		break;
	case 0:
		// Do not thing
		break;
	default:
		break;
	}

	// Write long packet data type
	// Write long packet word count
	reg_long = (((uint32_t)(long_packet->data_type)) | (((uint32_t)(long_packet->word_count&0x00FF))<<8)
		| (((uint32_t)((long_packet->word_count>>8)&0x00FF))<<16));
	anx7533_i2c_write_byte4(dev, slave_id, GEN_HDR, reg_long);

	// MCU must to check the DCS FIFO status when it send DCS long packet
	wait_count = 0;
	anx7533_i2c_read_byte4(dev, slave_id, CMD_PKT_STATUS, &read_data);
	while (0 == (read_data & gen_pld_w_empty)) {
		k_busy_wait(2000);
		anx7533_i2c_read_byte4(dev, slave_id, CMD_PKT_STATUS, &read_data);
		wait_count++;
		if(wait_count > DCS_LONG_PACKET_TIMEOUT){
			LOG_ERR("DCS ERROR!!\n");
			return;
		}
	}
}

void anx7533_panel_set_parameters(const struct device *dev)
{
	uint8_t reg_temp;

	// write H active
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, SW_H_ACTIVE_L, (uint8_t)((uint16_t)PANEL_H_ACTIVE&0x00FF));
	anx7533_i2c_read_byte(dev, SLAVEID_SPI, SW_H_ACTIVE_H, &reg_temp);
	reg_temp &= ~SW_H_ACTIVE_H_BITS;
	reg_temp |= (uint8_t)(((uint16_t)PANEL_H_ACTIVE>>8)&SW_H_ACTIVE_H_BITS);
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, SW_H_ACTIVE_H, reg_temp);

	// write HFP
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, SW_HFP_L, (uint8_t)((uint16_t)PANEL_HFP&0x00FF));
	anx7533_i2c_read_byte(dev, SLAVEID_SPI, SW_HFP_H, &reg_temp);
	reg_temp &= ~SW_HFP_H_BITS;
	reg_temp |= (uint8_t)(((uint16_t)PANEL_HFP>>8)&SW_HFP_H_BITS);
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, SW_HFP_H, reg_temp);

	// write HSYNC
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, SW_HSYNC_L, (uint8_t)((uint16_t)PANEL_HSYNC&0x00FF));
	anx7533_i2c_read_byte(dev, SLAVEID_SPI, SW_HSYNC_H, &reg_temp);
	reg_temp &= ~SW_HSYNC_H_BITS;
	reg_temp |= (uint8_t)(((uint16_t)PANEL_HSYNC>>8)&SW_HSYNC_H_BITS);
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, SW_HSYNC_H, reg_temp);

	// write HBP
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, SW_HBP_L, (uint8_t)((uint16_t)PANEL_HBP&0x00FF));
	anx7533_i2c_read_byte(dev, SLAVEID_SPI, SW_HBP_H, &reg_temp);
	reg_temp &= ~SW_HBP_H_BITS;
	reg_temp |= (uint8_t)(((uint16_t)PANEL_HBP>>8)&SW_HBP_H_BITS);
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, SW_HBP_H, reg_temp);

	// write V active
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, SW_V_ACTIVE_L, (uint8_t)((uint16_t)PANEL_V_ACTIVE&0x00FF));
	anx7533_i2c_read_byte(dev, SLAVEID_SPI, SW_V_ACTIVE_H, &reg_temp);
	reg_temp &= ~SW_V_ACTIVE_H_BITS;
	reg_temp |= (uint8_t)(((uint16_t)PANEL_V_ACTIVE>>8)&SW_V_ACTIVE_H_BITS);
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, SW_V_ACTIVE_H, reg_temp);

	// write VFP
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, SW_VFP_L, (uint8_t)((uint16_t)PANEL_VFP&0x00FF));
	anx7533_i2c_read_byte(dev, SLAVEID_SPI, SW_VFP_H, &reg_temp);
	reg_temp &= ~SW_VFP_H_BITS;
	reg_temp |= (uint8_t)(((uint16_t)PANEL_VFP>>8)&SW_VFP_H_BITS);
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, SW_VFP_H, reg_temp);

	// write VSYNC
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, SW_VSYNC_L, (uint8_t)((uint16_t)PANEL_VSYNC&0x00FF));
	anx7533_i2c_read_byte(dev, SLAVEID_SPI, SW_VSYNC_H, &reg_temp);
	reg_temp &= ~SW_VSYNC_H_BITS;
	reg_temp |= (uint8_t)(((uint16_t)PANEL_VSYNC>>8)&SW_VSYNC_H_BITS);
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, SW_VSYNC_H, reg_temp);

	// write VBP
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, SW_VBP_L, (uint8_t)((uint16_t)PANEL_VBP&0x00FF));
	anx7533_i2c_read_byte(dev, SLAVEID_SPI, SW_VBP_H, &reg_temp);
	reg_temp &= ~SW_VBP_H_BITS;
	reg_temp |= (uint8_t)(((uint16_t)PANEL_VBP>>8)&SW_VBP_H_BITS);
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, SW_VBP_H, reg_temp);

	// write Frame Rate
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, SW_PANEL_FRAME_RATE, (uint8_t)PANEL_FRAME_RATE);

	// write SW_PANEL_INFO_0
	reg_temp = 0;
	// set video mode
	switch(MIPI_VIDEO_MODE){
		case VIDEOMODE_SIDE:
			reg_temp |= (((uint8_t)(0x01)<<REG_PANEL_VIDEO_MODE_SHIFT)&REG_PANEL_VIDEO_MODE);
		break;
		case VIDEOMODE_STACKED:
			reg_temp |= (((uint8_t)(0x02)<<REG_PANEL_VIDEO_MODE_SHIFT)&REG_PANEL_VIDEO_MODE);
		break;
		default:
			// do not thing;
		break;
	}
	// set MIPI lane number
	switch(MIPI_LANE_NUMBER){
		case 4:
		case 3:
		case 2:
		case 1:
			reg_temp |= (((uint8_t)(MIPI_LANE_NUMBER-1)<<REG_MIPI_LANE_COUNT_SHIFT)&REG_MIPI_LANE_COUNT);
		break;
		default:
			// do not thing;
		break;
	}
	// set MIPI total port
	switch(MIPI_TOTAL_PORT){
		case 4:
		case 3:
		case 2:
		case 1:
			reg_temp |= (((uint8_t)(MIPI_TOTAL_PORT-1)<<REG_MIPI_TOTAL_PORT_SHIFT)&REG_MIPI_TOTAL_PORT);
		break;
		default:
			// do not thing;
		break;
	}
	// set panel count
	switch(PANEL_COUNT){
		case 4:
		case 3:
		case 2:
		case 1:
			reg_temp |= (((uint8_t)(PANEL_COUNT-1)<<REG_PANEL_COUNT_SHIFT)&REG_PANEL_COUNT);
		break;
		default:
			// do not thing;
		break;
	}
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, SW_PANEL_INFO_0, reg_temp);

	// write SW_PANEL_INFO_1
	anx7533_i2c_read_byte(dev, SLAVEID_SPI, SW_PANEL_INFO_1, &reg_temp);
	reg_temp &= ~REG_PANEL_ORDER;
	switch(MIPI_DISPLAY_EYE){
		case DISPLAY_RIGHT:
			reg_temp |= REG_PANEL_ORDER;
		break;
		default:
			// do not thing;
		break;
	}
	reg_temp &= ~REG_PANEL_DSC_MODE;
	switch(MIPI_DSC_STATUS){
		case DSC_ONE_TO_THREE:
			reg_temp |= REG_PANEL_DSC_MODE;
		break;
		default:
			// do not thing;
		break;
	}
	reg_temp &= ~REG_PANEL_TRANS_MODE;
	switch(MIPI_TRANSMIT_MODE){
		case MOD_NON_BURST_PULSES:
			reg_temp |= (((uint8_t)(0x00)<<REG_PANEL_TRANS_MODE_SHIFT)&REG_PANEL_TRANS_MODE);
		break;
		case MOD_NON_BURST_EVENTS:
			reg_temp |= (((uint8_t)(0x01)<<REG_PANEL_TRANS_MODE_SHIFT)&REG_PANEL_TRANS_MODE);
		break;
		case MOD_BURST:
		default:
			reg_temp |= (((uint8_t)(0x02)<<REG_PANEL_TRANS_MODE_SHIFT)&REG_PANEL_TRANS_MODE);
		break;
	}
	// write DIP switch
	reg_temp &= ~VIDEO_BIST_MODE;
	//reg_temp |= VIDEO_BIST_MODE;

	// Set DPHY timing that be calculated by OCM
	switch(MIPI_DSC_STATUS){
		case DSC_NO_DSC:
			reg_temp |= SET_DPHY_TIMING;
		break;
		case DSC_ONE_TO_THREE:
			reg_temp |= SET_DPHY_TIMING;
		break;
		default:
			// do not thing;
		break;
	}

	// Set all SW_PANEL_INFO_1 done
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, SW_PANEL_INFO_1, reg_temp);

	// for normal "BURST" panel, it should be MULTIPLY 1.2 (M_VALUE_MULTIPLY=120)
	// for normal "NON-BURST" panel, it should be MULTIPLY 1 (M_VALUE_MULTIPLY=100)
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, M_VALUE_MULTIPLY, PANEL_M_MULTIPLY);

	// This bit should be set after patameters setting down !!!
	anx7533_i2c_read_byte(dev, SLAVEID_SPI, MISC_NOTIFY_OCM0, &reg_temp);
	reg_temp |= PANEL_INFO_SET_DONE;
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, MISC_NOTIFY_OCM0, reg_temp);
}

unsigned char *anx7533_panel_get_dp_edid_buf(void)
{
    return &edid_dp[0];
}

unsigned char *anx7533_panel_get_dp_edid_extension_buf(void)
{
    return &edid_extension_dp[0];
}

unsigned char *anx7533_panel_get_hdmi_edid_buf(void)
{
    return &edid_hdmi[0];
}

unsigned char *anx7533_panel_get_hdmi_edid_extension_buf(void)
{
    return &edid_extension_hdmi[0];
}


static void anx7533_state_process(const struct device *dev)
{
	struct anx7533_priv *priv = (struct anx7533_priv *)(dev->data);
	char	ret;


	switch(priv->current_state){
	case ANX7533_STATE_NONE:
		// Panel power supply off, so that also turn off backlight
		// power down Chicago power first
		anx7533_chip_powerdown(dev);
		// Clean dp cable state
		priv->dp_cable = DP_CABLE_OUT;
		// Clean interrupt queue
		anx7533_irq_queue_clean(dev);

		k_busy_wait(5000);

		// power on Chicago power first
		// move to next state
		anx7533_state_change(dev, ANX7533_STATE_WAITCABLE);
		// remove "break", check "dp_cable" immediately.

	case ANX7533_STATE_WAITCABLE:
		// DP cable plug-in
		LOG_INF("STATE wait cable\n");
		if ((DP_CABLE_IN == priv->dp_cable) ||
		    (SIGNAL_LOW == anx7533_check_interrupt_state(dev))) {
			priv->dp_cable = DP_CABLE_IN;

			// RESET and POWER UP Chicago
			anx7533_chip_poweron(dev);

			ret = anx7533_ocm_crc_checking(dev);
			// checking OCM load status, if CRC32 error, re-load OCM again
			if (VALUE_FAILURE == ret) {
				LOG_ERR("OCM CRC Error, please re-burn OCM FW\n");
				// reboot
				anx7533_state_change(dev, ANX7533_STATE_NONE);
			} else if (VALUE_FAILURE2 == ret){
				LOG_ERR("I2C error!!!\n");
				// reboot
				anx7533_state_change(dev, ANX7533_STATE_NONE);
			} else {
				LOG_INF("OCM CRC pass.\n");
				anx7533_set_checking_link_speed(dev);
				// Set "video stable" interrupt delay time
				anx7533_set_video_stable_delay_time(dev, VIDEO_STABLE_DELAY_TIME);

				// EDID for DP
				priv->edid_buffer = anx7533_panel_get_dp_edid_buf();
				priv->edid_extension_buffer = anx7533_panel_get_dp_edid_extension_buf();
				// Write EDID to Chicago
				anx7533_edid_write_buffer(dev);
				anx7533_edid_write_extension_buffer(dev);

				//TODO: Turned on by dip switch, need to check
				//set_standby_mode_feature_enable();

				//TODO: On a debug flag
				//anx7533_set_mailbox_feature_enable();

				// TODO: On a debug flag
				// Audio MCLK always on, sample interface
				//chicago_audio_mclk_always_out();

				// Set panel parameters
				anx7533_panel_set_parameters(dev);
				// Notify to OCM that extrnal device setting done
				anx7533_mipi_mcu_write_done(dev);
				anx7533_state_change(dev, ANX7533_STATE_WAITOCM);
			}
		}
		break;

	case ANX7533_STATE_WAITOCM:
		if(anx7533_check_ocm_status(dev)){
			// OCM load done
			// Read Chip ID
			anx7533_read_chip_id(dev);

			// If customer would like to set customized setting and DPCD that can set here
			// , after "OCM load done" and before HPD to unforce high
			//anx7533_dpcd_setting(dev);
			// Set HPD to unForce High
			anx7533_hpd_set(dev, HDP_UNFORCE, HDP_DATA_HIGH);
			LOG_INF("HPD to high.\n");

			anx7533_state_change(dev, ANX7533_STATE_NORMAL);
			LOG_INF("Waiting for stable video....\n");
		}
		break;

	case ANX7533_STATE_NORMAL:
		LOG_ERR("STATE NORMAL\n");
		// Do nothing
		break;

	// CHIC-433, Add OCM auto-flash function and related Sysfs
	// for debug command usage.
	case ANX7533_STATE_DEBUG:
		// Do nothing
		break;

	default:
		LOG_INF("Error current state, current_state = %d\n",
			 (uint16_t)priv->current_state);
		anx7533_state_change(dev, ANX7533_STATE_NONE);
		break;
	}
}

void anx7533_main_process(const struct device *dev)
{
	struct anx7533_priv *priv = (struct anx7533_priv *)(dev->data);

	LOG_INF("main process\n");

	anx7533_interrupt_handle(dev);
	anx7533_state_process(dev);

	if (priv->cts_testing) {
		anx7533_dp_phy_cts(dev);
	}
}

static void anx7533_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct anx7533_work_item *item =
		CONTAINER_OF(dwork, struct anx7533_work_item, dwork);
	struct anx7533_priv *priv = (struct anx7533_priv *)item->dev->data;
	int workq_timer;

	LOG_INF("anx7533 work function\n");

	k_mutex_lock(&priv->lock, K_FOREVER);
	anx7533_main_process(item->dev);

	if (priv->irq_q.irq_q_output != priv->irq_q.irq_q_input) {
		workq_timer = 15;
	}else if (ANX7533_STATE_WAITOCM == anx7533_get_current_state(item->dev)) {
		// need to waiting OCM load done, OCM load done time = 80~85ms
		workq_timer = 100;
	}else if (VALUE_ON == anx7533_get_cts_state(item->dev)) {
		// Driver keep running when it in DP PHY CTS mode
		workq_timer = 15;
	}else{
		workq_timer = 0;
	}

	k_mutex_unlock(&priv->lock);

	LOG_INF("workq timer %d\n", workq_timer);
	if (workq_timer > 0) {
		LOG_INF("schedule more work\n");
		k_work_schedule(&item->dwork, K_MSEC((workq_timer)));
	}
}

#if defined(CONFIG_SHELL)

static inline uint16_t anx7533_read_system_status(const struct device *dev, uint8_t *status0,
						  uint8_t *status1)
{
	int err;

	err = anx7533_i2c_read_byte(dev, SLAVEID_DP_IP,  ADDR_SYSTEM_STATUS_0, status0);
	if (err)
		goto out;

	err = anx7533_i2c_read_byte(dev, SLAVEID_DP_IP,  ADDR_SYSTEM_STATUS_1, status1);

out:
	return err;
}

static inline uint16_t anx7533_read_prbs_ctrl(const struct device *dev, uint8_t *reg)
{
	return anx7533_i2c_read_byte(dev, SLAVEID_DP_IP,  ADDR_PRBS_CTRL, reg);
}

static inline uint16_t anx7533_read_rc_training(const struct device *dev, uint8_t *reg)
{
	return anx7533_i2c_read_byte(dev, SLAVEID_DP_IP, ADDR_RC_TRAINING_RESULT, reg);
}

static inline uint16_t anx7533_read_prbs31_err_ind(const struct device *dev, uint8_t *reg)
{
	return anx7533_i2c_read_byte(dev, SLAVEID_DP_IP, ADDR_PRBS31_ERR_IND, reg);
}

static inline uint16_t anx7533_read_power_status(const struct device *dev, uint8_t *reg)
{
	return anx7533_i2c_read_byte(dev, SLAVEID_DP_TOP, ADDR_POWER_STATUS, reg);
}

static inline uint16_t anx7533_read_stable_video_detect(const struct device *dev, uint8_t *reg)
{
	return anx7533_i2c_read_byte(dev, SLAVEID_VIDEO, ADDR_VID_STABLE_DET, reg);
}

static inline uint16_t anx7533_read_link_bw_set(const struct device *dev, uint8_t *reg)
{
	return anx7533_i2c_read_byte(dev, SLAVEID_DP_RX1, ADDR_LINK_BW_SET, reg);
}

static inline uint16_t anx7533_read_lane_cnt_set(const struct device *dev, uint8_t *reg)
{
	return anx7533_i2c_read_byte(dev, SLAVEID_DP_RX1, ADDR_LANE_CNT_SET, reg);
}

static inline uint16_t anx7533_read_lane_cnt_set2(const struct device *dev, uint8_t *reg)
{
	return anx7533_i2c_read_byte(dev, SLAVEID_DP_RX2, ADDR_LANE_CNT_SET, reg);
}

static inline uint16_t anx7533_read_lane_status(const struct device *dev, uint8_t *reg, uint8_t *reg2)
{
	int err;

	err = anx7533_i2c_read_byte(dev, SLAVEID_DP_RX2, ADDR_LANE0_STATUS, reg);
	if (err)
		goto out;

	err = anx7533_i2c_read_byte(dev, SLAVEID_DP_RX2, ADDR_LANE1_STATUS, reg2);
out:
	return err;
}

static inline uint16_t anx7533_read_lane_align_status(const struct device *dev, uint8_t *reg)
{
	return anx7533_i2c_read_byte(dev, SLAVEID_DP_RX2, ADDR_LANE_ALIGN_STATUS, reg);
}

static inline uint16_t anx7533_read_lane0_err_cnt(const struct device *dev, uint16_t *reg)
{
	uint8_t h_reg, l_reg;
	int err;

	err = anx7533_i2c_read_byte(dev, SLAVEID_DP_RX2, ADDR_LANE0_ERR_CNT0, &h_reg);
	if (err)
		goto out;

	err = anx7533_i2c_read_byte(dev, SLAVEID_DP_RX2, ADDR_LANE0_ERR_CNT1, &l_reg);
	if (err)
		goto out;

	*reg = (h_reg << 8) | l_reg;
out:
	return err;
}

static inline uint16_t anx7533_read_lane1_err_cnt(const struct device *dev, uint16_t *reg)
{
	uint8_t h_reg, l_reg;
	int err;

	err = anx7533_i2c_read_byte(dev, SLAVEID_DP_RX2, ADDR_LANE1_ERR_CNT0, &h_reg);
	if (err)
		goto out;

	err = anx7533_i2c_read_byte(dev, SLAVEID_DP_RX2, ADDR_LANE1_ERR_CNT1, &l_reg);
	if (err)
		goto out;

	*reg = (h_reg << 8) | l_reg;
out:
	return err;
}

static inline uint16_t anx7533_read_main_link_debug(const struct device *dev, uint16_t *reg)
{
	uint8_t h_reg, l_reg;
	int err;

	err = anx7533_i2c_read_byte(dev, SLAVEID_MAIN_LINK, ADDR_HWIDTH15_8_DBG, &h_reg);
	if (err)
		goto out;

	err = anx7533_i2c_read_byte(dev, SLAVEID_MAIN_LINK, ADDR_HWIDTH7_0_DBG, &l_reg);
	if (err)
		goto out;

	*reg = (h_reg << 8) | l_reg;
out:
	return err;
}

static inline uint16_t anx7533_read_active_pixel(const struct device *dev, uint16_t *reg)
{
	uint8_t h_reg, l_reg;
	int err;

	err = anx7533_i2c_read_byte(dev, SLAVEID_VIDEO, ADDR_ACT_PIX_HIGH, &h_reg);
	if (err)
		goto out;

	err = anx7533_i2c_read_byte(dev, SLAVEID_VIDEO, ADDR_ACT_PIX_LOW, &l_reg);
	if (err)
		goto out;

	*reg = (h_reg << 8) | l_reg;
out:
	return err;
}

static inline uint16_t anx7533_read_active_line(const struct device *dev, uint16_t *reg)
{
	uint8_t h_reg, l_reg;
	int err;

	err = anx7533_i2c_read_byte(dev, SLAVEID_VIDEO, ADDR_ACT_LINE_HIGH, &h_reg);
	if (err)
		goto out;

	err = anx7533_i2c_read_byte(dev, SLAVEID_VIDEO, ADDR_ACT_LINE_LOW, &l_reg);
	if (err)
		goto out;

	*reg = (h_reg << 8) | l_reg;
out:
	return err;
}

static inline uint16_t anx7533_read_debug_reg1(const struct device *dev, uint8_t *reg)
{
	return anx7533_i2c_read_byte(dev, SLAVEID_DP_DEBUG, ADDR_DEBUG_REG1, reg);
}

static inline uint16_t anx7533_read_debug_reg2(const struct device *dev, uint8_t *reg)
{
	return anx7533_i2c_read_byte(dev, SLAVEID_DP_DEBUG, ADDR_DEBUG_REG2, reg);
}

static inline uint16_t anx7533_read_debug_reg3(const struct device *dev, uint8_t *reg)
{
	return anx7533_i2c_read_byte(dev, SLAVEID_DP_DEBUG, ADDR_DEBUG_REG3, reg);
}

static int anx7533_dump_reg(const struct shell *sh, size_t argc, char **argv)
{
	uint8_t reg, ocm_major, ocm_minor, dev_state = 0;
	const struct device *dev;
	struct anx7533_priv *priv;

	shell_print(sh, "dump regs : %s\n", argv[1]);
	/* device is the only arg, hard coded to 1 */
	dev = shell_device_get_binding(argv[1]);
	if (!dev) {
		shell_error(sh, "anx7533 device not found");
		return -EINVAL;
	}
	priv = (struct anx7533_priv *)(dev->data);

	k_mutex_lock(&priv->lock, K_FOREVER);

	if (priv->chip_power_status == VALUE_OFF) {
		anx7533_chip_poweron(dev);
		dev_state = 1;
	}

	anx7533_wakeup(dev);
	shell_print(sh, "CHIP ID: %u\n", anx7533_read_chip_id(dev));

	anx7533_get_ocm_version(dev, &ocm_major, &ocm_minor);
	shell_print(sh, "OCM MAJ %u MIN %u\n", ocm_major, ocm_minor);

	reg = anx7533_check_ocm_status(dev);
	if (reg)
		shell_print(sh, "OCM is on\n");
	else
		shell_print(sh, "OCM is off\n");

	reg = anx7533_get_ocm_status(dev);
        if (reg & OCM_LOAD_DONE)
		shell_print(sh, "OCM initialization load done\n");
        else
		shell_print(sh, "OCM initialization load not done\n");

        shell_print(sh, "SERDES_PWR_CNTRL: 0x%02X\n", reg);

	reg = anx7533_check_interrupt_state(dev);
	shell_print(sh, "interrupt pin: %u\n", reg);

	anx7533_i2c_read_byte(priv->anx_dev, SLAVEID_SPI, INT_NOTIFY_MCU0, &reg);
	shell_print(sh, "int notify MCU0: %u\n", reg);

	anx7533_i2c_read_byte(priv->anx_dev, SLAVEID_SPI, INT_NOTIFY_MCU1, &reg);
	shell_print(sh, "int notify MCU1: %u\n", reg);

	if (dev_state)
		anx7533_chip_powerdown(dev);

	k_mutex_unlock(&priv->lock);

	return 0;
}

static int anx7533_print_dprx_info(const struct shell *sh, size_t argc, char **argv)
{
	uint8_t h_reg, l_reg, dev_state = 0;
	const struct device *dev;
	struct anx7533_priv *priv;
	uint16_t dbl_reg = 0;
	int loop;

	shell_print(sh, "dump regs : %s\n", argv[1]);
	/* device is the only arg, hard coded to 1 */
	dev = shell_device_get_binding(argv[1]);
	if (!dev) {
		shell_error(sh, "anx7533 device not found");
		return -EINVAL;
	}
	priv = (struct anx7533_priv *)(dev->data);

	k_mutex_lock(&priv->lock, K_FOREVER);

	if (priv->chip_power_status == VALUE_OFF)
		anx7533_chip_poweron(dev);

	anx7533_read_system_status(dev, &h_reg, &l_reg);
	shell_print(sh, "system status 0 %u, status 1 %u\n", h_reg, l_reg);

	anx7533_read_prbs_ctrl(dev, &h_reg);
	shell_print(sh, "prbs ctrl %u\n", h_reg);

	anx7533_read_rc_training(dev, &h_reg);
	shell_print(sh, "rc training res %u\n", h_reg);

	anx7533_read_prbs31_err_ind(dev, &h_reg);
	shell_print(sh, "prbs31 err ind %u\n", h_reg);

	anx7533_read_power_status(dev, &h_reg);
	shell_print(sh, "power status %u\n", h_reg);

	anx7533_read_link_bw_set(dev, &h_reg);
	shell_print(sh, "link bw set %u\n", h_reg);

	anx7533_read_lane_cnt_set(dev, &h_reg);
	shell_print(sh, "lane cnt set %u\n", h_reg);

	anx7533_read_lane_cnt_set2(dev, &h_reg);
	shell_print(sh, "lane cnt set 2 %u\n", h_reg);

	anx7533_read_lane_status(dev, &h_reg, &l_reg);
	shell_print(sh, "system status 0 %u, status 1 %u\n", h_reg, l_reg);

	anx7533_read_lane_align_status(dev, &h_reg);
	shell_print(sh, "lane align status %u\n", h_reg);

	loop = 2;
	do {
		anx7533_read_lane0_err_cnt(dev, &dbl_reg);
		shell_print(sh, "lane 0 err count %u\n", dbl_reg);
		anx7533_read_lane1_err_cnt(dev, &dbl_reg);
		shell_print(sh, "lane 1 err count %u\n", dbl_reg);
		loop--;
	} while (loop != 0);

	loop = 2;
	do {
		anx7533_read_main_link_debug(dev, &dbl_reg);
		shell_print(sh, "main link debug %u\n", dbl_reg);
		loop--;
	} while (loop != 0);

	anx7533_read_active_pixel(dev, &dbl_reg);
	shell_print(sh, "active pixel %u\n", dbl_reg);

	anx7533_read_active_line(dev, &dbl_reg);
	shell_print(sh, "active line %u\n", dbl_reg);

	anx7533_read_debug_reg1(dev, &h_reg);
	shell_print(sh, " %u : debug reg 1 %u\n", SLAVEID_DP_DEBUG, h_reg);

	anx7533_read_debug_reg2(dev, &h_reg);
	shell_print(sh, "%u : debug reg 2 %u\n", SLAVEID_DP_DEBUG, h_reg);

	anx7533_read_debug_reg3(dev, &h_reg);
	shell_print(sh, "%u : debug reg 3 %u\n", SLAVEID_DP_DEBUG, h_reg);

	if (dev_state)
		anx7533_chip_powerdown(dev);

	k_mutex_unlock(&priv->lock);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(anx7533_cmds,
	SHELL_CMD_ARG(dump_reg, NULL, "<device>", anx7533_dump_reg, 2, 0),
	SHELL_CMD_ARG(print_dprx_info, NULL, "<device>", anx7533_print_dprx_info, 2, 0),
	SHELL_SUBCMD_SET_END
);

static int anx7533_cmd(const struct shell *sh, size_t argc, char **argv)
{
	shell_error(sh, "%s: unknown parameter: %s", argv[0], argv[1]);
	return -EINVAL;
}

SHELL_COND_CMD_ARG_REGISTER(CONFIG_ANX7533_SHELL, anx7533, &anx7533_cmds,
		       "anx7533 shell commands", anx7533_cmd, 2, 0);

#endif /* defined(CONFIG_SHELL) */


static int anx7533_init_gpio(const struct device *dev)
{
	struct anx7533_priv *priv = (struct anx7533_priv *)(dev->data);
	const struct anx7533_config *cfg = dev->config;
	int err;

	LOG_DBG("ANX7533 init gpios\n");

	if (!gpio_is_ready_dt(&cfg->vid_en_pin)) {
		LOG_ERR("Error: vid enable pin is not ready\n");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&cfg->vid_en_pin, GPIO_OUTPUT);
	if (err != 0) {
		LOG_ERR("Error %d: failed to configure vid en pin\n", err);
		return -ENODEV;
	}
	gpio_pin_set_dt(&cfg->vid_en_pin, 0);

	if (!gpio_is_ready_dt(&cfg->vid_rst_pin)) {
		LOG_ERR("Error: vid reset pin is not ready\n");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&cfg->vid_rst_pin, GPIO_OUTPUT);
	if (err != 0) {
		LOG_ERR("Error %d: failed to configure vid reset pin\n", err);
		return -ENODEV;
	}
	gpio_pin_set_dt(&cfg->vid_rst_pin, 0);

	if (!gpio_is_ready_dt(&cfg->vid_int_pin)) {
		LOG_ERR("Error: vid int pin is not ready\n");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&cfg->vid_int_pin, GPIO_INPUT);
	if (err != 0) {
		LOG_ERR("Error %d: failed to configure vid int pin\n", err);
		return -ENODEV;
	}

	err = gpio_pin_interrupt_configure_dt(&cfg->vid_int_pin,
					      GPIO_INT_EDGE_FALLING);
	if (err != 0) {
		LOG_ERR("Error %d: failed to configure interrupt on vid pin\n", err);
		return -ENODEV;
	}

	gpio_init_callback(&priv->gpio_irq_cb, anx7533_irq, BIT(cfg->vid_int_pin.pin));
	gpio_add_callback(cfg->vid_int_pin.port, &priv->gpio_irq_cb);

	LOG_INF("Set up vid int at %s pin %d\n", cfg->vid_int_pin.port->name,
						 cfg->vid_int_pin.pin);

	return 0;
}

#define WQ_STACK_SIZE 512
#define WQ_PRIORITY 5
K_THREAD_STACK_DEFINE(wq_stack_area, WQ_STACK_SIZE);

static int anx7533_init(const struct device *dev)
{
	struct anx7533_priv *priv = (struct anx7533_priv *)(dev->data);
	const struct anx7533_config *cfg = dev->config;
	int err = 0;

	LOG_INF("ANX7533 initialize called");

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("ANX7533 i2c device not ready.");
		err = -ENODEV;
		goto out;
	}

	if (!device_is_ready(cfg->bus.bus)) {
		LOG_ERR("ANX7533 device not ready.");
		err = -ENODEV;
		goto out;
	}

	priv->dev_addr = (cfg->bus.addr >> 1);
	priv->select_offset_addr = (cfg->reg_offset >> 1);
	priv->select_offset_rd_addr = (cfg->reg_offset_rd >> 1);
	priv->chip_power_status = VALUE_OFF;
	priv->anx_dev = dev;
	err = k_mutex_init(&priv->lock);
	if (err) {
		LOG_ERR("Error initializing mutex\n");
		goto out;
	}

	k_work_queue_init(&priv->workq);
	k_work_queue_start(&priv->workq, wq_stack_area,
			   K_THREAD_STACK_SIZEOF(wq_stack_area), WQ_PRIORITY,
			   NULL);
	k_work_init_delayable(&work_item.dwork, anx7533_work);

	err = anx7533_init_gpio(dev);
	if (err) {
		LOG_ERR("Error initialzing gpio pins\n");
		goto out;
	}

	anx7533_state_change(dev, ANX7533_STATE_NONE);
	work_item.dev = dev;

	k_work_schedule(&work_item.dwork, K_NO_WAIT);

out:
	return err;
}

#define ANX7533_INIT(n)                                                        \
	static struct anx7533_priv anx7533_priv_##n = {                        \
		.dev = n,						       \
		.chip_power_status = VALUE_OFF,				       \
									       \
	};                                                                     \
                                                                               \
	static const struct anx7533_config anx7533_cfg_##n = {                 \
		.bus = I2C_DT_SPEC_INST_GET(n),				       \
		.vid_en_pin    = GPIO_DT_SPEC_INST_GET(n, vid_en_pin_gpios),   \
		.vid_rst_pin = GPIO_DT_SPEC_INST_GET(n, vid_rst_pin_gpios),  \
		.vid_int_pin =   GPIO_DT_SPEC_INST_GET(n, vid_int_pin_gpios),  \
		.reg_offset =    DT_INST_PROP(n, reg_offset),		       \
		.reg_offset_rd = DT_INST_PROP(n, reg_offset_rd),	       \
	};								       \
                                                                               \
	DEVICE_DT_INST_DEFINE(n, anx7533_init, NULL, &anx7533_priv_##n,        \
			      &anx7533_cfg_##n, POST_KERNEL,		       \
			      CONFIG_ANX7533_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(ANX7533_INIT)

