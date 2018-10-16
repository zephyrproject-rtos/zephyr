/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hal_log.h>
LOG_MODULE_DECLARE(LOG_MODULE_NAME);

#include <zephyr.h>
#include <string.h>
#include <uwp_hal.h>

#define CONFIG_CP_SECTOR1_LOAD_BASE 0x40a20000
#define CONFIG_CP_SECTOR2_LOAD_BASE 0x40a80000
#define CONFIG_CP_SECTOR3_LOAD_BASE 0x40e40000
#define CONFIG_CP_SECTOR4_LOAD_BASE 0x40f40000
#define CONFIG_CP_SECTOR1_LEN 0x58000
#define CONFIG_CP_SECTOR2_LEN 0x30000
#define CONFIG_CP_SECTOR3_LEN 0x20000
#define CONFIG_CP_SECTOR4_LEN 0x3D000

#define CP_START_ADDR (0x020C0000 + 16)
#define CP_RUNNING_CHECK_CR 0x40a80000
#define CP_RUNNING_BIT  0
#define CP_WIFI_RUNNING_BIT     1
#define CP_START_ADDR_OFFSET	16

#define CP_START_ADDR_CONTAINER	0x0200D000
#define CP_START_MODEM0_ADDR	(0x02000000 + DT_FLASH_AREA_MODEM_0_OFFSET)
#define CP_START_MODEM1_ADDR	(0x02000000 + DT_FLASH_AREA_MODEM_1_OFFSET)

extern void GNSS_Start(void);

int move_cp(char *src, char *dst, uint32_t size)
{
	int *from, *to;
	int len = 0;
	char *from_8, *to_8;

	if (src == NULL || dst == NULL || size == 0) {
		LOG_ERR("invalid parameter,src=%p,dst=%p,size=%d", src, dst, (int)size);
		return -1;
	}

	from = (int *)src;
	to = (int *)dst;
	len = size / 4;
	while (len > 0) {
		*to = *from;
		to++;
		from++;
		len--;
	}
	len = size % 4;
	from_8 = (char *)from;
	to_8 = (char *)to;
	while (len > 0) {
		*to_8 = *from_8;
		to_8++;
		from_8++;
		len--;
	}
	LOG_DBG("sector load done,from =%p to =%p", from_8, to_8);
	return 0;
}

int load_fw(void)
{
	int ret  = 0;
	int *p_addr = (int *)CP_START_ADDR_CONTAINER;
	char *addr;
	char *src = NULL;
	uint32_t offset = 0;

	LOG_INF("load cp firmware start");

	if ((*p_addr != CP_START_MODEM0_ADDR) &&
		(*p_addr != CP_START_MODEM1_ADDR)) {
		addr = (char *)CP_START_MODEM0_ADDR + CP_START_ADDR_OFFSET;
	} else {
		addr = (char *)(*p_addr) + CP_START_ADDR_OFFSET;
	}

	// load sector1
	src = (char *)(addr);
	ret = move_cp(src, (char *)CONFIG_CP_SECTOR1_LOAD_BASE, (uint32_t)CONFIG_CP_SECTOR1_LEN);
	offset += CONFIG_CP_SECTOR1_LEN;
	// load sector 2
	src = (char *)(addr + offset);
	ret = move_cp(src, (char *)CONFIG_CP_SECTOR2_LOAD_BASE, (uint32_t)CONFIG_CP_SECTOR2_LEN);
	offset += CONFIG_CP_SECTOR2_LEN;
	// load sector 3
	src = (char *)(addr + offset);
	ret = move_cp(src, (char *)CONFIG_CP_SECTOR3_LOAD_BASE, (uint32_t)CONFIG_CP_SECTOR3_LEN);
	offset += CONFIG_CP_SECTOR3_LEN;
	// load sector 4
	src = (char *)(addr + offset);
	// move_cp(src,(char *)CONFIG_CP_SECTOR4_LOAD_BASE,(uint32_t)CONFIG_CP_SECTOR4_LEN);

	if (ret < 0) {
		return ret;
	}

	LOG_DBG("cp firmware copy done");
	return 0;
}

int cp_mcu_pull_reset(void)
{
	int i = 0;

	LOG_DBG("gnss mcu hold start");
	// dap sel
	sci_reg_or(0x4083c064, BIT(0) | BIT(1));
	// dap rst
	sci_reg_and(0x4083c000, ~(BIT(28) | BIT(29)));
	// dap eb
	sci_reg_or(0x4083c024, BIT(30) | BIT(31));
	// check dap is ok ?
	while (sci_read32(0x408600fc) != 0x24770011 && i < 20) {
		i++;
	}
	if (sci_read32(0x408600fc) != 0x24770011) {
		LOG_ERR("check dap is ok fail");
	}
	// hold gnss core
	sci_write32(0x40860000, 0x22000012);
	sci_write32(0x40860004, 0xe000edf0);
	sci_write32(0x4086000c, 0xa05f0003);
	// restore dap sel
	sci_reg_and(0x4083c064, ~(BIT(0) | BIT(1)));
	// remap gnss RAM to 0x0000_0000 address as boot from RAM
	sci_reg_or(0x40bc800c, BIT(0) | BIT(1));

	LOG_DBG("gnss mcu hold done");

	return 0;
}

int cp_mcu_release_reset(void)
{
	unsigned int value = 0;

	LOG_DBG("gnss mcu release start. ");
	// reset the gnss CM4 core,and CM4 will run from IRAM(which is remapped to 0x0000_0000)
	value = sci_read32(0x40bc8004);
	value |= 0x1;
	sci_write32(0x40bc8004, value);

	LOG_DBG("gnss mcu release done. ");

	return 0;
}
void  cp_check_bit_clear(void)
{
	sci_write32(CP_RUNNING_CHECK_CR, 0);

	return;
}

int cp_check_running(void)
{
	int value;
	int cnt = 100;

	do {
		value = sci_read32(CP_RUNNING_CHECK_CR);
		if (value & (1 << CP_RUNNING_BIT)) {
			return 0;
		}
		k_sleep(30);
	} while (cnt-- > 0);

	return -1;
}

int cp_check_wifi_running(void)
{
	int value;
	int cnt = 100;

	LOG_DBG("check if cp wifi is running");
	do {
		value = sci_read32(CP_RUNNING_CHECK_CR);
		if (value & (1 << CP_WIFI_RUNNING_BIT)) {
			LOG_DBG("CP wifi is running !!! %d", cnt);

			while (1) ;
			return 0;
		}
	} while (cnt-- > 0);

	LOG_ERR("CP wifi running fail,Something must be wrong");
	return -1;
}

#ifdef CONFIG_USE_UWP_HAL_SRAM
static void cp_sram_init(void)
{
	LOG_DBG("power on sram start");

	unsigned int val;

	val = sys_read32(0x40130004); /* enable */
	val |= 0x220;
	sys_write32(val, 0x40130004);
	k_sleep(50);

	val = sys_read32(0x4083c088); /* power on WRAP */
	val &= ~(0x2);
	sys_write32(val, 0x4083c088);
	while (!(sys_read32(0x4083c00c) & (0x1 << 14))) {
	}

	val = sys_read32(0x4083c0a8);
	val &= ~(0x4);
	sys_write32(val, 0x4083c0a8);
	while (!(sys_read32(0x4083c00c) & (0x1 << 16))) {
	}

	val = sys_read32(0x4083c134); /* close MEM PD */
	val &= 0xffffff;
	sys_write32(val, 0x4083c134);

	val = sys_read32(0x4083c130);
	val &= 0xfffffff0;
	sys_write32(val, 0x4083c130);

	LOG_INF("CP SRAM init done");
}
#else
#define cp_sram_init(...)
#endif /* CONFIG_USE_UWP_HAL_SRAM */

static bool cp_init_flag = false;
int uwp_mcu_init(void)
{
	int ret = 0;

	if (cp_init_flag) {
		return ret;
	}

	LOG_DBG("Start init mcu and download firmware.");

	cp_sram_init();
	GNSS_Start();

	ret = cp_mcu_pull_reset();
	if (ret < 0) {
		LOG_ERR("reset CP MCU fail");
		return ret;
	}

	ret = load_fw();
	if (ret < 0) {
		LOG_ERR("load fw fail");
		return ret;
	}

	cp_check_bit_clear();
	ret = cp_mcu_release_reset(); // cp start to run
	if (ret < 0) {
		LOG_ERR("release reset fail");
		return ret;
	}

	ret = cp_check_running();
	if (ret < 0) {
		LOG_ERR("cp fw is not running,something must be wrong");
		return ret;
	}

	cp_init_flag = true;

	LOG_DBG("CP Init done,and CP fw is running!!!");
	return 0;
}
