/*
 * DM8806 Stand-alone Ethernet PHY with RMII
 *
 * Copyright (c) 2024 Robert Slawinski <robert.slawinski1@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT davicom_dm8806_phy

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eth_dm8806_phy, CONFIG_ETHERNET_LOG_LEVEL);

#include <stdio.h>
#include <sys/types.h>
#include <zephyr/kernel.h>
#include <zephyr/net/phy.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/drivers/gpio.h>

#include "phy_dm8806_priv.h"

struct phy_dm8806_config {
	const struct device *mdio;
	uint8_t phy_addr;
	uint8_t switch_addr;
	struct gpio_dt_spec gpio_rst;
	struct gpio_dt_spec gpio_int;
	bool mii;
};

struct phy_dm8806_data {
	const struct device *dev;
	struct phy_link_state state;
	phy_callback_t link_speed_chenge_cb;
	void *cb_data;
	struct gpio_callback gpio_cb;
#ifdef CONFIG_PHY_DM8806_TRIGGER
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_PHY_DM8806_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#endif
};

#ifdef CONFIG_PHY_DM8806_SMI_BUS_CHECK
static uint16_t phy_crc_check(uint16_t data, uint16_t reg_addr, uint8_t opcode)
{
	uint16_t csum[8];
	uint16_t checksum = 0;

	/* Checksum calculated formula proposed by Davicom on datasheed:
	 * 7.2.1 Host SMI Bus Error Check Function page 84.
	 */
	csum[0] = ((data & 0x1) ^ ((data & 0x100) >> 8) ^ (reg_addr & 0x1) ^
		   ((reg_addr & 0x100) >> 8));
	csum[1] = (((data & 0x2) >> 1) ^ ((data & 0x200) >> 9) ^ ((reg_addr & 0x2) >> 1) ^
		   ((reg_addr & 0x200) >> 9));
	csum[2] = (((data & 0x4) >> 2) ^ ((data & 0x400) >> 10) ^ ((reg_addr & 0x4) >> 2) ^
		   (opcode & 0x1));
	csum[3] = (((data & 0x8) >> 3) ^ ((data & 0x800) >> 11) ^ ((reg_addr & 0x8) >> 3) ^
		   ((opcode & 0x2) >> 1));
	csum[4] = (((data & 0x10) >> 4) ^ ((data & 0x1000) >> 12) ^ ((reg_addr & 0x10) >> 4));
	csum[5] = (((data & 0x20) >> 5) ^ ((data & 0x2000) >> 13) ^ ((reg_addr & 0x20) >> 5));
	csum[6] = (((data & 0x40) >> 6) ^ ((data & 0x4000) >> 14) ^ ((reg_addr & 0x40) >> 6));
	csum[7] = (((data & 0x80) >> 7) ^ ((data & 0x8000) >> 15) ^ ((reg_addr & 0x80) >> 7));
	for (int cnt = 0; cnt < 8; cnt++) {
		checksum |= (csum[cnt] << cnt);
	}
	return checksum;
}
#endif

static int phy_dm8806_write_reg(const struct device *dev, uint8_t phyad, uint8_t regad,
				uint16_t data)
{
	int res = 0;
	const struct phy_dm8806_config *cfg = dev->config;

#ifdef CONFIG_PHY_DM8806_SMI_BUS_CHECK
	uint16_t checksum_status;
	uint16_t sw_checksum = 0;
	uint16_t abs_reg;
	int repetition = 0;

	do {
		/* Set register 33AH.[0] = 1 to enable SMI Bus Error Check function. */
		res = mdio_write(cfg->mdio, SMI_BUS_CTRL_PHY_ADDRESS, SMI_BUS_CTRL_REG_ADDRESS,
				 SMI_ECE);
		if (res != 0) {
			LOG_ERR("Failed to write data to PHY register: SMI_BUS_CTRL_REG_ADDRESS, "
				"error code: %d",
				res);
			return res;
		}
#endif
		res = mdio_write(cfg->mdio, phyad, regad, data);
		if (res != 0) {
			LOG_ERR("Failed to read data from PHY, error code: %d", res);
			return res;
		}
#ifdef CONFIG_PHY_DM8806_SMI_BUS_CHECK
		/* Calculate checksum */
		abs_reg = (phyad << 5);
		abs_reg |= (regad & 0x1f);
		sw_checksum = phy_crc_check(data, abs_reg, PHY_WRITE);
		sw_checksum &= 0xffu;
		/* Write calculated checksum to the PHY register 339H.[7:0] */
		res = mdio_write(cfg->mdio, SMI_BUS_ERR_CHK_PHY_ADDRESS,
				 SMI_BUS_ERR_CHK_REG_ADDRESS, sw_checksum);
		if (res != 0) {
			LOG_ERR("Failed to write calculated checksum to the PHY register, "
				"error code: %d",
				res);
			return res;
		}

		/* Read status of the checksum from Serial Bus Error Check Register
		 * 339H.[8].
		 */
		res = mdio_read(cfg->mdio, SMI_BUS_ERR_CHK_PHY_ADDRESS, SMI_BUS_ERR_CHK_REG_ADDRESS,
				&checksum_status);
		if (res != 0) {
			LOG_ERR("Failed to read hardware calculated checksum from PHY, error code: "
				"%d",
				res);
			return res;
		}
		/* Checksum status is present on the 8-th bit of the Serial Bus Error
		 * Check Register (339h) [8].
		 */
		checksum_status &= 0x100;

		/* Repeat the writing procedure for the number of attempts defined in
		 * KConfig after which the transfer will failed.
		 */
		if (CONFIG_PHY_DM8806_SMI_BUS_CHECK_REPETITION > 0) {
			repetition++;
			if (checksum_status) {
				LOG_WRN("%d repeat of PHY read procedure due to CRC error.",
					repetition);
				if (repetition == CONFIG_PHY_DM8806_SMI_BUS_CHECK_REPETITION) {
					LOG_ERR("Maximum number of PHY write repetition exceed.");
					res = (-EIO);
				}
			} else {
				break;
			}
			/* Do not repeat the transfer if repetition number is set to 0. Just check
			 * the CRC in this case and report the error in case of wrong CRC sum.
			 */
		} else {
			if (checksum_status) {
				LOG_ERR("Wrong checksum, during PHY write procedure.");
				res = (-EIO);
				break;
			}
		}
	} while (repetition < CONFIG_PHY_DM8806_SMI_BUS_CHECK_REPETITION);
#endif

	return res;
}

static int phy_dm8806_read_reg(const struct device *dev, uint8_t phyad, uint8_t regad,
			       uint16_t *data)
{
	int res = 0;
	const struct phy_dm8806_config *cfg = dev->config;

#ifdef CONFIG_PHY_DM8806_SMI_BUS_CHECK
	uint16_t hw_checksum;
	uint16_t sw_checksum = 0;
	uint16_t abs_reg;
	int repetition = 0;

	do {
		/* Set register 33AH.[0] = 1 to enable SMI Bus Error Check function. */
		res = mdio_write(cfg->mdio, SMI_BUS_CTRL_PHY_ADDRESS, SMI_BUS_CTRL_REG_ADDRESS,
				 SMI_ECE);
		if (res != 0) {
			LOG_ERR("Failed to write data to PHY register: SMI_BUS_CTRL_REG_ADDRESS, "
				"error code: %d",
				res);
			return res;
		}
#endif
		res = mdio_read(cfg->mdio, phyad, regad, data);
		if (res != 0) {
			LOG_ERR("Failed to read data from PHY, error code: %d", res);
			return res;
		}
#ifdef CONFIG_PHY_DM8806_SMI_BUS_CHECK
		/* Read hardware calculated checksum from Serial Bus Error Check Register. */
		res = mdio_read(cfg->mdio, SMI_BUS_ERR_CHK_PHY_ADDRESS, SMI_BUS_ERR_CHK_REG_ADDRESS,
				&hw_checksum);
		if (res != 0) {
			LOG_ERR("Failed to read hardware calculated checksum from PHY, error code: "
				"%d",
				res);
			return res;
		}
		/* Absolute register address use for checksum calculation is 10-bit value.
		 * Oldest five bit creates PHY address, youngest five bit creates register address.
		 */
		abs_reg = (phyad << 5);
		abs_reg |= (regad & 0x1f);

		sw_checksum = phy_crc_check(*data, abs_reg, PHY_READ);

		if (CONFIG_PHY_DM8806_SMI_BUS_CHECK_REPETITION > 0) {
			repetition++;
			if (hw_checksum != sw_checksum) {
				LOG_WRN("%d repeat of PHY read procedure due to CRC error.",
					repetition);
				if (repetition == CONFIG_PHY_DM8806_SMI_BUS_CHECK_REPETITION) {
					LOG_ERR("Maximum number of PHY read repetition exceed.");
					res = (-EIO);
				}
			} else {
				break;
			}
		} else {
			if (hw_checksum != sw_checksum) {
				LOG_ERR("Wrong checksum, during PHY read procedure.");
				res = (-EIO);
				break;
			}
		}
	} while (repetition < CONFIG_PHY_DM8806_SMI_BUS_CHECK_REPETITION);
#endif

	return res;
}

static void phy_dm8806_gpio_callback(const struct device *dev, struct gpio_callback *cb,
				     uint32_t pins)
{
	ARG_UNUSED(pins);
	struct phy_dm8806_data *drv_data = CONTAINER_OF(cb, struct phy_dm8806_data, gpio_cb);
	const struct phy_dm8806_config *cfg = drv_data->dev->config;

	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_DISABLE);
	k_sem_give(&drv_data->gpio_sem);
}

static void phy_dm8806_thread_cb(const struct device *dev, struct phy_link_state *state,
				 void *cb_data)
{
	uint16_t data;
	struct phy_dm8806_data *drv_data = dev->data;
	const struct phy_dm8806_config *cfg = dev->config;

	if (drv_data->link_speed_chenge_cb != NULL) {
		drv_data->link_speed_chenge_cb(dev, state, cb_data);
	}
	/* Clear the interrupt flag, by writing "1" to LNKCHG bit of Interrupt Status
	 * Register (318h)
	 */
	mdio_read(cfg->mdio, INT_STAT_PHY_ADDR, INT_STAT_REG_ADDR, &data);
	data |= 0x1;
	mdio_write(cfg->mdio, INT_STAT_PHY_ADDR, INT_STAT_REG_ADDR, data);
	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);
}

static void phy_dm8806_thread(void *p1, void *p2, void *p3)
{
	struct phy_dm8806_data *drv_data = p1;
	void *cb_data = p2;
	struct phy_link_state *state = p3;

	while (1) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		phy_dm8806_thread_cb(drv_data->dev, state, cb_data);
	}
}

int phy_dm8806_port_init(const struct device *dev)
{
	int res;
	const struct phy_dm8806_config *cfg = dev->config;

	res = gpio_pin_configure_dt(&cfg->gpio_rst, (GPIO_OUTPUT_INACTIVE | GPIO_PULL_UP));
	if (res != 0) {
		LOG_ERR("Failed to configure gpio reset pin for PHY DM886 as an output");
		return res;
	}
	/* Hardware reset of the PHY DM8806 */
	gpio_pin_set_dt(&cfg->gpio_rst, true);
	if (res != 0) {
		LOG_ERR("Failed to assert gpio reset pin of the PHY DM886 to physical 0");
		return res;
	}
	/* According to DM8806 datasheet (DM8806-DAVICOM.pdf), low active state on
	 * the reset pin must remain minimum 10ms to perform hardware reset.
	 */
	k_msleep(10);
	res = gpio_pin_set_dt(&cfg->gpio_rst, false);
	if (res != 0) {
		LOG_ERR("Failed to assert gpio reset pin of the PHY DM886 to physical 1");
		return res;
	}

	return res;
}

int phy_dm8806_init_interrupt(const struct device *dev)
{
	int res = 0;
	uint16_t data;
	struct phy_dm8806_data *drv_data = dev->data;
	void *cb_data = drv_data->cb_data;
	const struct phy_dm8806_config *cfg = dev->config;

	/* Configure Davicom PHY DM8806 interrupts:
	 * Activate global interrupt by writing "1" to LNKCHG of Interrupt Mask
	 * And Control Register (319h)
	 */
	res = mdio_read(cfg->mdio, INT_MASK_CTRL_PHY_ADDR, INT_MASK_CTRL_REG_ADDR, &data);
	if (res) {
		LOG_ERR("Failed to read IRQ_LED_CONTROL, %i", res);
		return res;
	}
	data |= 0x1;
	res = mdio_write(cfg->mdio, INT_MASK_CTRL_PHY_ADDR, INT_MASK_CTRL_REG_ADDR, data);
	if (res) {
		LOG_ERR("Failed to read IRQ_LED_CONTROL, %i", res);
		return res;
	}

	/* Activate interrupt per Ethernet port by writing "1" to LNK_EN0~3
	 * of WoL Control Register (2BBh)
	 */
	res = mdio_read(cfg->mdio, WOLL_CTRL_REG_PHY_ADDR, WOLL_CTRL_REG_REG_ADDR, &data);
	if (res) {
		LOG_ERR("Failed to read IRQ_LED_CONTROL, %i", res);
		return res;
	}
	data |= 0xF;
	res = mdio_write(cfg->mdio, WOLL_CTRL_REG_PHY_ADDR, WOLL_CTRL_REG_REG_ADDR, data);
	if (res) {
		LOG_ERR("Failed to read IRQ_LED_CONTROL, %i", res);
		return res;
	}

	/* Configure external interrupts:
	 * Configure interrupt pin to recognize the rising edge on the Davicom
	 * PHY DM8806 as external interrupt
	 */
	if (device_is_ready(cfg->gpio_int.port) != true) {
		LOG_ERR("gpio_int gpio not ready");
		return -ENODEV;
	}
	drv_data->dev = dev;
	res = gpio_pin_configure_dt(&cfg->gpio_int, GPIO_INPUT);
	if (res != 0) {
		LOG_ERR("Failed to configure gpio interrupt pin for PHY DM886 as an input");
		return res;
	}
	/* Assign callback function to be fired by Davicom PHY DM8806 external
	 * interrupt pin
	 */
	gpio_init_callback(&drv_data->gpio_cb, phy_dm8806_gpio_callback, BIT(cfg->gpio_int.pin));
	res = gpio_add_callback(cfg->gpio_int.port, &drv_data->gpio_cb);
	if (res != 0) {
		LOG_ERR("Failed to set PHY DM886 gpio callback");
		return res;
	}
	k_sem_init(&drv_data->gpio_sem, 0, K_SEM_MAX_LIMIT);
	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_PHY_DM8806_THREAD_STACK_SIZE, phy_dm8806_thread, drv_data, cb_data,
			NULL, K_PRIO_COOP(CONFIG_PHY_DM8806_THREAD_PRIORITY), 0, K_NO_WAIT);
	/* Configure GPIO interrupt to be triggered on pin state change to logical
	 * level 1 asserted by Davicom PHY DM8806 interrupt Pin
	 */
	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);
	if (res != 0) {
		LOG_ERR("Failed to configure PHY DM886 gpio interrupt pin trigger for "
			"active edge");
		return res;
	}

	return 0;
}

static int phy_dm8806_init(const struct device *dev)
{
	int ret;
	uint16_t val;
	const struct phy_dm8806_config *cfg = dev->config;

	/* Configure reset pin for Davicom PHY DM8806 to be able to generate reset
	 * signal
	 */
	ret = phy_dm8806_port_init(dev);
	if (ret != 0) {
		LOG_ERR("Failed to reset PHY DM8806 ");
		return ret;
	}

	ret = mdio_read(cfg->mdio, PHY_ADDRESS_18H, PORT5_MAC_CONTROL, &val);
	if (ret) {
		LOG_ERR("Failed to read PORT5_MAC_CONTROL: %i", ret);
		return ret;
	}

	/* Activate default working mode*/
	val |= (P5_50M_INT_CLK_SOURCE | P5_50M_CLK_OUT_ENABLE | P5_EN_FORCE);
	val &= (P5_SPEED_100M | P5_FULL_DUPLEX | P5_FORCE_LINK_ON);

	ret = mdio_write(cfg->mdio, PHY_ADDRESS_18H, PORT5_MAC_CONTROL, val);
	if (ret) {
		LOG_ERR("Failed to write PORT5_MAC_CONTROL, %i", ret);
		return ret;
	}

	ret = mdio_read(cfg->mdio, PHY_ADDRESS_18H, IRQ_LED_CONTROL, &val);
	if (ret) {
		LOG_ERR("Failed to read IRQ_LED_CONTROL, %i", ret);
		return ret;
	}

	/* Activate LED blinking mode indicator mode 0. */
	val &= LED_MODE_0;
	ret = mdio_write(cfg->mdio, PHY_ADDRESS_18H, IRQ_LED_CONTROL, val);
	if (ret) {
		LOG_ERR("Failed to write IRQ_LED_CONTROL, %i", ret);
		return ret;
	}

#ifdef CONFIG_PHY_DM8806_TRIGGER
	ret = phy_dm8806_init_interrupt(dev);
	if (ret != 0) {
		LOG_ERR("Failed to configure interrupt fot PHY DM8806");
		return ret;
	}
#endif
	return 0;
}

static int phy_dm8806_get_link_state(const struct device *dev, struct phy_link_state *state)
{
	int ret;
	uint16_t status;
	uint16_t data;
	const struct phy_dm8806_config *cfg = dev->config;

#ifdef CONFIG_PHY_DM8806_TRIGGER
	ret = mdio_read(cfg->mdio, 0x18, 0x18, &data);
	if (ret) {
		LOG_ERR("Failed to read IRQ_LED_CONTROL, %i", ret);
		return ret;
	}
#endif
	/* Read data from Switch Per-Port Register. */
	ret = phy_dm8806_read_reg(dev, cfg->switch_addr, PORTX_SWITCH_STATUS, &data);
	if (ret) {
		LOG_ERR("Failes to read data drom DM8806 Switch Per-Port Registers area");
		return ret;
	}
	/* Extract speed and duplex status from Switch Per-Port Register: Per Port
	 * Status Data Register
	 */
	status = data;
	status >>= SPEED_AND_DUPLEX_OFFSET;
	switch (status & SPEED_AND_DUPLEX_MASK) {
	case SPEED_10MBPS_HALF_DUPLEX:
		state->speed = LINK_HALF_10BASE_T;
		break;
	case SPEED_10MBPS_FULL_DUPLEX:
		state->speed = LINK_FULL_10BASE_T;
		break;
	case SPEED_100MBPS_HALF_DUPLEX:
		state->speed = LINK_HALF_100BASE_T;
		break;
	case SPEED_100MBPS_FULL_DUPLEX:
		state->speed = LINK_FULL_100BASE_T;
		break;
	}
	/* Extract link status from Switch Per-Port Register: Per Port Status Data
	 * Register
	 */
	status = data;
	if (status & LINK_STATUS_MASK) {
		state->is_up = true;
	} else {
		state->is_up = false;
	}
	return ret;
}

static int phy_dm8806_cfg_link(const struct device *dev, enum phy_link_speed adv_speeds)
{
	uint8_t ret;
	uint16_t data;
	uint16_t req_speed;
	const struct phy_dm8806_config *cfg = dev->config;

	req_speed = adv_speeds;
	switch (req_speed) {
	case LINK_HALF_10BASE_T:
		req_speed = MODE_10_BASET_HALF_DUPLEX;
		break;

	case LINK_FULL_10BASE_T:
		req_speed = MODE_10_BASET_FULL_DUPLEX;
		break;

	case LINK_HALF_100BASE_T:
		req_speed = MODE_100_BASET_HALF_DUPLEX;
		break;

	case LINK_FULL_100BASE_T:
		req_speed = MODE_100_BASET_FULL_DUPLEX;
		break;
	}

	/* Power down */
	ret = phy_dm8806_read_reg(dev, cfg->phy_addr, PORTX_PHY_CONTROL_REGISTER, &data);
	if (ret) {
		LOG_ERR("Failes to read data drom DM8806");
		return ret;
	}
	k_busy_wait(500);
	data |= POWER_DOWN;
	ret = phy_dm8806_write_reg(dev, cfg->phy_addr, PORTX_PHY_CONTROL_REGISTER, data);
	if (ret) {
		LOG_ERR("Failed to write data to DM8806");
		return ret;
	}
	k_busy_wait(500);

	/* Turn off the auto-negotiation process. */
	ret = phy_dm8806_read_reg(dev, cfg->phy_addr, PORTX_PHY_CONTROL_REGISTER, &data);
	if (ret) {
		LOG_ERR("Failed to write data to DM8806");
		return ret;
	}
	k_busy_wait(500);
	data &= ~(AUTO_NEGOTIATION);
	ret = phy_dm8806_write_reg(dev, cfg->phy_addr, PORTX_PHY_CONTROL_REGISTER, data);
	if (ret) {
		LOG_ERR("Failed to write data to DM8806");
		return ret;
	}
	k_busy_wait(500);

	/* Change the link speed. */
	ret = phy_dm8806_read_reg(dev, cfg->phy_addr, PORTX_PHY_CONTROL_REGISTER, &data);
	if (ret) {
		LOG_ERR("Failed to read data from DM8806");
		return ret;
	}
	k_busy_wait(500);
	data &= ~(LINK_SPEED | DUPLEX_MODE);
	data |= req_speed;
	ret = phy_dm8806_write_reg(dev, cfg->phy_addr, PORTX_PHY_CONTROL_REGISTER, data);
	if (ret) {
		LOG_ERR("Failed to write data to DM8806");
		return ret;
	}
	k_busy_wait(500);

	/* Power up ethernet port*/
	ret = phy_dm8806_read_reg(dev, cfg->phy_addr, PORTX_PHY_CONTROL_REGISTER, &data);
	if (ret) {
		LOG_ERR("Failes to read data drom DM8806");
		return ret;
	}
	k_busy_wait(500);
	data &= ~(POWER_DOWN);
	ret = phy_dm8806_write_reg(dev, cfg->phy_addr, PORTX_PHY_CONTROL_REGISTER, data);
	if (ret) {
		LOG_ERR("Failed to write data to DM8806");
		return ret;
	}
	k_busy_wait(500);
	return ret;
}

static int phy_dm8806_reg_read(const struct device *dev, uint16_t reg_addr, uint32_t *data)
{
	int res;
	const struct phy_dm8806_config *cfg = dev->config;

	res = mdio_read(cfg->mdio, cfg->switch_addr, reg_addr, (uint16_t *)data);
	if (res) {
		LOG_ERR("Failed to read data from DM8806");
		return res;
	}
	return res;
}

static int phy_dm8806_reg_write(const struct device *dev, uint16_t reg_addr, uint32_t data)
{
	int res;
	const struct phy_dm8806_config *cfg = dev->config;

	res = mdio_write(cfg->mdio, cfg->switch_addr, reg_addr, data);
	if (res) {
		LOG_ERR("Failed to write data to DM8806");
		return res;
	}
	return res;
}

static int phy_dm8806_link_cb_set(const struct device *dev, phy_callback_t cb, void *user_data)
{
	int res = 0;
	struct phy_dm8806_data *data = dev->data;
	const struct phy_dm8806_config *cfg = dev->config;

	res = gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_DISABLE);
	if (res != 0) {
		LOG_WRN("Failed to disable DM8806 interrupt: %i", res);
		return res;
	}
	data->link_speed_chenge_cb = cb;
	data->cb_data = user_data;
	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);
	if (res != 0) {
		LOG_WRN("Failed to enable DM8806 interrupt: %i", res);
		return res;
	}

	return res;
}

static DEVICE_API(ethphy, phy_dm8806_api) = {
	.get_link = phy_dm8806_get_link_state,
	.cfg_link = phy_dm8806_cfg_link,
#ifdef CONFIG_PHY_DM8806_TRIGGER
	.link_cb_set = phy_dm8806_link_cb_set,
#endif
	.read = phy_dm8806_reg_read,
	.write = phy_dm8806_reg_write,
};

#define DM8806_PHY_DEFINE_CONFIG(n)                                                                \
	static const struct phy_dm8806_config phy_dm8806_config_##n = {                            \
		.mdio = DEVICE_DT_GET(DT_INST_BUS(n)),                                             \
		.phy_addr = DT_INST_REG_ADDR(n),                                                   \
		.switch_addr = DT_PROP(DT_NODELABEL(dm8806_phy##n), reg_switch),                   \
		.gpio_int = GPIO_DT_SPEC_INST_GET(n, interrupt_gpio),                              \
		.gpio_rst = GPIO_DT_SPEC_INST_GET(n, reset_gpio),                                  \
	}

#define DM8806_PHY_INITIALIZE(n)                                                                   \
	DM8806_PHY_DEFINE_CONFIG(n);                                                               \
	static struct phy_dm8806_data phy_dm8806_data_##n = {                                      \
		.gpio_sem = Z_SEM_INITIALIZER(phy_dm8806_data_##n.gpio_sem, 1, 1),                 \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, phy_dm8806_init, NULL, &phy_dm8806_data_##n,                      \
			      &phy_dm8806_config_##n, POST_KERNEL, CONFIG_PHY_INIT_PRIORITY,       \
			      &phy_dm8806_api);

DT_INST_FOREACH_STATUS_OKAY(DM8806_PHY_INITIALIZE)
