/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief File containing common functions for RPU hardware interaction
 * using QSPI and SPI that can be invoked by shell or the driver.
 */

#include <string.h>
#include <sys/time.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/gpio/nordic-nrf-gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/wifi/nrf_wifi/bus/rpu_hw_if.h>
#include <zephyr/drivers/wifi/nrf_wifi/bus/qspi_if.h>

#include "spi_if.h"

LOG_MODULE_REGISTER(wifi_nrf_bus, CONFIG_WIFI_NRF70_BUSLIB_LOG_LEVEL);

#define NRF7002_NODE DT_NODELABEL(nrf70)

static const struct gpio_dt_spec host_irq_spec =
GPIO_DT_SPEC_GET(NRF7002_NODE, host_irq_gpios);

#if DT_NODE_HAS_PROP(NRF7002_NODE, iovdd_ctrl_gpios)

#define NRF70_IOVDD_GPIO 1
static const struct gpio_dt_spec iovdd_ctrl_spec =
GPIO_DT_SPEC_GET(NRF7002_NODE, iovdd_ctrl_gpios);

#elif DT_NODE_HAS_PROP(NRF7002_NODE, iovdd_regulator)

#include <zephyr/drivers/regulator.h>

static const struct device *iovdd_regulator =
DEVICE_DT_GET(DT_PHANDLE(NRF7002_NODE, iovdd_regulator));

#else
#error "nRF70 device either needs iovdd-ctrl-gpios or iovdd-regulator property"
#endif

static const struct gpio_dt_spec bucken_spec =
GPIO_DT_SPEC_GET(NRF7002_NODE, bucken_gpios);

char blk_name[][15] = { "SysBus",   "ExtSysBus",	   "PBus",	   "PKTRAM",
			       "GRAM",	   "LMAC_ROM",	   "LMAC_RET_RAM", "LMAC_SRC_RAM",
			       "UMAC_ROM", "UMAC_RET_RAM", "UMAC_SRC_RAM" };

uint32_t rpu_7002_memmap[][3] = {
	{ 0x000000, 0x008FFF, 1 },
	{ 0x009000, 0x03FFFF, 2 },
	{ 0x040000, 0x07FFFF, 1 },
	{ 0x0C0000, 0x0F0FFF, 0 },
	{ 0x080000, 0x092000, 2 },
	{ 0x100000, 0x134000, 1 },
	{ 0x140000, 0x14C000, 1 },
	{ 0x180000, 0x190000, 1 },
	{ 0x200000, 0x261800, 1 },
	{ 0x280000, 0x2A4000, 1 },
	{ 0x300000, 0x338000, 1 }
};

static const struct qspi_dev *qdev;
static struct qspi_config *cfg;

static int validate_addr_blk(uint32_t start_addr,
							 uint32_t end_addr,
							 uint32_t block_no,
							 bool *hl_flag,
							 int *selected_blk)
{
	uint32_t *block_map = rpu_7002_memmap[block_no];

	if (((start_addr >= block_map[0]) && (start_addr <= block_map[1])) &&
	    ((end_addr >= block_map[0]) && (end_addr <= block_map[1]))) {
		if (block_no == PKTRAM) {
			*hl_flag = 0;
		}
		*selected_blk = block_no;
		return 0;
	}

	return -1;
}

static int rpu_validate_addr(uint32_t start_addr, uint32_t len, bool *hl_flag)
{
	int ret = 0, i;
	uint32_t end_addr;
	int selected_blk;

	end_addr = start_addr + len - 1;

	*hl_flag = 1;

	for (i = 0; i < NUM_MEM_BLOCKS; i++) {
		ret = validate_addr_blk(start_addr, end_addr, i, hl_flag, &selected_blk);
		if (!ret) {
			break;
		}
	}

	if (ret) {
		LOG_ERR("Address validation failed - pls check memmory map and re-try");
		return -1;
	}

	if ((selected_blk == LMAC_ROM) || (selected_blk == UMAC_ROM)) {
		LOG_ERR("Error: Cannot write to ROM blocks");
		return -1;
	}

	cfg->qspi_slave_latency = (*hl_flag) ? rpu_7002_memmap[selected_blk][2] : 0;

	return 0;
}

int rpu_irq_config(struct gpio_callback *irq_callback_data, void (*irq_handler)())
{
	int ret;

	if (!device_is_ready(host_irq_spec.port)) {
		LOG_ERR("Host IRQ GPIO %s is not ready", host_irq_spec.port->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&host_irq_spec, GPIO_INPUT);
	if (ret) {
		LOG_ERR("Failed to configure host_irq pin %d", host_irq_spec.pin);
		goto out;
	}

	ret = gpio_pin_interrupt_configure_dt(&host_irq_spec,
			GPIO_INT_EDGE_TO_ACTIVE);
	if (ret) {
		LOG_ERR("Failed to configure interrupt on host_irq pin %d",
				host_irq_spec.pin);
		goto out;
	}

	gpio_init_callback(irq_callback_data,
			irq_handler,
			BIT(host_irq_spec.pin));

	ret = gpio_add_callback(host_irq_spec.port, irq_callback_data);
	if (ret) {
		LOG_ERR("Failed to add callback on host_irq pin %d",
				host_irq_spec.pin);
		goto out;
	}

	LOG_DBG("Finished Interrupt config\n");

out:
	return ret;
}

int rpu_irq_remove(struct gpio_callback *irq_callback_data)
{
	int ret;

	ret = gpio_pin_configure_dt(&host_irq_spec, GPIO_DISCONNECTED);
	if (ret) {
		LOG_ERR("Failed to remove host_irq pin %d", host_irq_spec.pin);
		goto out;
	}

	ret = gpio_remove_callback(host_irq_spec.port, irq_callback_data);
	if (ret) {
		LOG_ERR("Failed to remove callback on host_irq pin %d",
				host_irq_spec.pin);
		goto out;
	}

out:
	return ret;
}

static int rpu_gpio_config(void)
{
	int ret;

#ifdef NRF70_IOVDD_GPIO
	if (!device_is_ready(iovdd_ctrl_spec.port)) {
		LOG_ERR("IOVDD GPIO %s is not ready", iovdd_ctrl_spec.port->name);
		return -ENODEV;
	}
#endif

	if (!device_is_ready(bucken_spec.port)) {
		LOG_ERR("BUCKEN GPIO %s is not ready", bucken_spec.port->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&bucken_spec, (GPIO_OUTPUT | NRF_GPIO_DRIVE_H0H1));
	if (ret) {
		LOG_ERR("BUCKEN GPIO configuration failed...");
		return ret;
	}

#ifdef NRF70_IOVDD_GPIO
	ret = gpio_pin_configure_dt(&iovdd_ctrl_spec, GPIO_OUTPUT);
	if (ret) {
		LOG_ERR("IOVDD GPIO configuration failed...");
		gpio_pin_configure_dt(&bucken_spec, GPIO_DISCONNECTED);
		return ret;
	}
#endif

	LOG_DBG("GPIO configuration done...\n");

	return 0;
}

static int rpu_gpio_remove(void)
{
	int ret;

	ret = gpio_pin_configure_dt(&bucken_spec, GPIO_DISCONNECTED);
	if (ret) {
		LOG_ERR("BUCKEN GPIO remove failed...");
		return ret;
	}

#ifdef NRF70_IOVDD_GPIO
	ret = gpio_pin_configure_dt(&iovdd_ctrl_spec, GPIO_DISCONNECTED);
	if (ret) {
		LOG_ERR("IOVDD GPIO remove failed...");
		return ret;
	}
#endif

	LOG_DBG("GPIO remove done...\n");
	return ret;
}

static int rpu_pwron(void)
{
	int ret;

	ret = gpio_pin_set_dt(&bucken_spec, 1);
	if (ret) {
		LOG_ERR("BUCKEN GPIO set failed...");
		return ret;
	}
	/* Settling time is 50us (H0) or 100us (L0) */
	k_msleep(1);

#ifdef NRF70_IOVDD_GPIO
	ret = gpio_pin_set_dt(&iovdd_ctrl_spec, 1);
#else
	ret = regulator_enable(iovdd_regulator);
#endif
	if (ret) {
		LOG_ERR("IOVDD enable failed...");
		gpio_pin_set_dt(&bucken_spec, 0);
		return ret;
	}
	/* Settling time for IOVDD */
	k_msleep(DT_PROP(NRF7002_NODE, iovdd_power_up_delay_ms));

#ifdef NRF70_IOVDD_GPIO
	if ((bucken_spec.port == iovdd_ctrl_spec.port) &&
	    (bucken_spec.pin == iovdd_ctrl_spec.pin)) {
		/* When a single GPIO is used, we need a total wait time after bucken assertion
		 * to be 6ms (1ms + 1ms + 4ms).
		 */
		k_msleep(4);
	}

	LOG_DBG("Bucken = %d, IOVDD = %d", gpio_pin_get_dt(&bucken_spec),
			gpio_pin_get_dt(&iovdd_ctrl_spec));
#else
	LOG_DBG("Bucken = %d, IOVDD = %d", gpio_pin_get_dt(&bucken_spec),
			regulator_is_enabled(iovdd_regulator) ? 1 : 0);
#endif


	return ret;
}

static int rpu_pwroff(void)
{
	int ret;

#ifdef NRF70_IOVDD_GPIO
	ret = gpio_pin_set_dt(&iovdd_ctrl_spec, 0); /* IOVDD CNTRL = 0 */
#else
	ret = regulator_disable(iovdd_regulator);
#endif
	if (ret) {
		LOG_ERR("IOVDD disable failed...");
		return ret;
	}

	ret = gpio_pin_set_dt(&bucken_spec, 0); /* BUCKEN = 0 */
	if (ret) {
		LOG_ERR("BUCKEN GPIO set failed...");
		return ret;
	}

	return ret;
}

int rpu_read(unsigned int addr, void *data, int len)
{
	bool hl_flag;

	if (rpu_validate_addr(addr, len, &hl_flag)) {
		return -1;
	}

	if (hl_flag) {
		return qdev->hl_read(addr, data, len);
	} else {
		return qdev->read(addr, data, len);
	}
}

int rpu_write(unsigned int addr, const void *data, int len)
{
	bool hl_flag;

	if (rpu_validate_addr(addr, len, &hl_flag)) {
		return -1;
	}

	return qdev->write(addr, data, len);
}

int rpu_sleep(void)
{
#if CONFIG_NRF70_ON_QSPI
	return qspi_cmd_sleep_rpu(&qspi_perip);
#else
	return spim_cmd_sleep_rpu_fn();
#endif
}

int rpu_wakeup(void)
{
	int ret;

	ret = rpu_wrsr2(1);
	if (ret) {
		LOG_ERR("Error: WRSR2 failed");
		return ret;
	}

	ret = rpu_rdsr2();
	if (ret < 0) {
		LOG_ERR("Error: RDSR2 failed");
		return ret;
	}

	ret = rpu_rdsr1();
	if (ret < 0) {
		LOG_ERR("Error: RDSR1 failed");
		return ret;
	}

	return 0;
}

int rpu_sleep_status(void)
{
	return rpu_rdsr1();
}

void rpu_get_sleep_stats(uint32_t addr, uint32_t *buff, uint32_t wrd_len)
{
	int ret;

	ret = rpu_wakeup();
	if (ret) {
		LOG_ERR("Error: RPU wakeup failed");
		return;
	}

	ret = rpu_read(addr, buff, wrd_len * 4);
	if (ret) {
		LOG_ERR("Error: RPU read failed");
		return;
	}

	ret = rpu_sleep();
	if (ret) {
		LOG_ERR("Error: RPU sleep failed");
		return;
	}
}

int rpu_wrsr2(uint8_t data)
{
	int ret;

#if CONFIG_NRF70_ON_QSPI
	ret = qspi_cmd_wakeup_rpu(&qspi_perip, data);
#else
	ret = spim_cmd_rpu_wakeup_fn(data);
#endif

	LOG_DBG("Written 0x%x to WRSR2", data);
	return ret;
}

int rpu_rdsr2(void)
{
#if CONFIG_NRF70_ON_QSPI
	return qspi_validate_rpu_wake_writecmd(&qspi_perip);
#else
	return spi_validate_rpu_wake_writecmd();
#endif
}

int rpu_rdsr1(void)
{
#if CONFIG_NRF70_ON_QSPI
	return qspi_wait_while_rpu_awake(&qspi_perip);
#else
	return spim_wait_while_rpu_awake();
#endif
}

/**
 * @brief Read a register via RPU hardware interface
 *
 * @param reg_addr Register address (opcode)
 * @param reg_value Pointer to store the read value
 * @return int 0 on success, negative error code on failure
 */
int rpu_read_reg(uint8_t reg_addr, uint8_t *reg_value)
{
#if CONFIG_NRF70_ON_QSPI
	return qspi_read_reg(&qspi_perip, reg_addr, reg_value);
#else
	return spim_read_reg_wrapper(NULL, reg_addr, reg_value);
#endif
}

/**
 * @brief Write a register via RPU hardware interface
 *
 * @param reg_addr Register address (opcode)
 * @param reg_value Value to write
 * @return int 0 on success, negative error code on failure
 */
int rpu_write_reg(uint8_t reg_addr, uint8_t reg_value)
{
#if CONFIG_NRF70_ON_QSPI
	return qspi_write_reg(&qspi_perip, reg_addr, reg_value);
#else
	return spim_write_reg_wrapper(NULL, reg_addr, reg_value);
#endif
}

int rpu_clks_on(void)
{
	uint32_t rpu_clks = 0x100;
	/* Enable RPU Clocks */
	qdev->write(0x048C20, &rpu_clks, 4);
	LOG_DBG("RPU Clocks ON...");
	return 0;
}

#define RPU_EXP_SIG 0x42000020
/* Read a known value from RPU memory to validate RPU communication */
int rpu_validate_comms(void)
{
	uint32_t rpu_test;
	int ret;

	/* UCCP_SOC_FAB_MST_READ_IDLE - HW reset value */
	ret = rpu_read(0x0005C, &rpu_test, 4);
	if (ret) {
		LOG_ERR("Error: RPU comms test: read failed\n");
		return ret;
	}

	if (rpu_test != RPU_EXP_SIG) {
		LOG_ERR("Error: RPU comms test: sig failed: expected 0x%x, got 0x%x\n",
			RPU_EXP_SIG, rpu_test);
		return -1;
	}

	LOG_DBG("RPU comms test passed\n");

	return 0;
}

int rpu_init(void)
{
	int ret;

	qdev = qspi_dev();
	cfg = qspi_get_config();

	ret = rpu_gpio_config();
	if (ret) {
		goto out;
	}

#ifdef CONFIG_NRF70_SR_COEX_RF_SWITCH
	ret = sr_gpio_config();
	if (ret) {
		goto remove_rpu_gpio;
	}
#endif
	ret = rpu_pwron();
	if (ret) {
#ifdef CONFIG_NRF70_SR_COEX_RF_SWITCH
		goto remove_sr_gpio;
#else
		goto remove_rpu_gpio;
#endif
	}

	return 0;

#ifdef CONFIG_NRF70_SR_COEX_RF_SWITCH
remove_sr_gpio:
	sr_gpio_remove();
#endif
remove_rpu_gpio:
	rpu_gpio_remove();
out:
	return ret;
}

int rpu_enable(void)
{
	int ret;

	ret = rpu_wakeup();
	if (ret) {
		goto rpu_pwroff;
	}

	ret = rpu_clks_on();
	if (ret) {
		goto rpu_pwroff;
	}

/* TODO: rpu_validate_comms() needs firmware download to be done
 * successfully before it can be called. So, disable this for
 * nrf70_buslib only usage.
 */
#ifdef CONFIG_WIFI_NRF70
	ret = rpu_validate_comms();
	if (ret) {
		goto rpu_pwroff;
	}
#endif
	return 0;
rpu_pwroff:
	rpu_pwroff();
	return ret;
}

int rpu_disable(void)
{
	int ret;

	#ifdef CONFIG_NRF70_SR_COEX_RF_SWITCH
	ret = sr_gpio_remove();
	if (ret) {
		goto out;
	}
#endif

	ret = rpu_pwroff();
	if (ret) {
		goto out;
	}
	ret = rpu_gpio_remove();
	if (ret) {
		goto out;
	}


	qdev = NULL;
	cfg = NULL;

out:
	return ret;
}
