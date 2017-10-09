/**
 *
 * \file
 *
 * \brief This module contains NMC1500 ASIC specific internal APIs.
 *
 * Copyright (c) 2016-2017 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */
#ifndef _NMASIC_H_
#define _NMASIC_H_

#include "common/include/nm_common.h"

#define NMI_PERIPH_REG_BASE     0x1000
#define NMI_CHIPID	            (NMI_PERIPH_REG_BASE)
#define rNMI_GP_REG_0			(0x149c)
#define rNMI_GP_REG_1			(0x14A0)
#define rNMI_GP_REG_2			(0xc0008)
#define rNMI_GLB_RESET			(0x1400)
#define rNMI_BOOT_RESET_MUX		(0x1118)
#define NMI_STATE_REG			(0x108c)
#define BOOTROM_REG				(0xc000c)
#define NMI_REV_REG  			(0x207ac)	/*Also, Used to load ATE firmware from SPI Flash and to ensure that it is running too*/
#define NMI_REV_REG_ATE			(0x1048) 	/*Revision info register in case of ATE FW*/
#define M2M_WAIT_FOR_HOST_REG 	(0x207bc)
#define M2M_FINISH_INIT_STATE 	0x02532636UL
#define M2M_FINISH_BOOT_ROM   	 0x10add09eUL
#define M2M_START_FIRMWARE   	 0xef522f61UL
#define M2M_START_PS_FIRMWARE    0x94992610UL

#define M2M_ATE_FW_START_VALUE	(0x3C1CD57D)	/*Also, Change this value in boot_firmware if it will be changed here*/
#define M2M_ATE_FW_IS_UP_VALUE	(0xD75DC1C3)	/*Also, Change this value in ATE (Burst) firmware if it will be changed here*/

#define REV_2B0        (0x2B0)
#define REV_B0         (0x2B0)
#define REV_3A0        (0x3A0)
#define GET_CHIPID()	nmi_get_chipid()
#define ISNMC1000(id)   ((((id) & 0xfffff000) == 0x100000) ? 1 : 0)
#define ISNMC1500(id)   ((((id) & 0xfffff000) == 0x150000) ? 1 : 0)
#define ISNMC3000(id)   ((((id) & 0xfff00000) == 0x300000) ? 1 : 0)
#define REV(id)         (((id) & 0x00000fff ))
#define EFUSED_MAC(value) (value & 0xffff0000)

#define rHAVE_SDIO_IRQ_GPIO_BIT     (NBIT0)
#define rHAVE_USE_PMU_BIT           (NBIT1)
#define rHAVE_SLEEP_CLK_SRC_RTC_BIT (NBIT2)
#define rHAVE_SLEEP_CLK_SRC_XO_BIT  (NBIT3)
#define rHAVE_EXT_PA_INV_TX_RX      (NBIT4)
#define rHAVE_LEGACY_RF_SETTINGS    (NBIT5)
#define rHAVE_LOGS_DISABLED_BIT		(NBIT6)
#define rHAVE_ETHERNET_MODE_BIT		(NBIT7)
#define rHAVE_RESERVED1_BIT     	(NBIT8)

typedef struct{
	uint32 u32Mac_efuse_mib;
	uint32 u32Firmware_Ota_rev;
}tstrGpRegs;

#ifdef __cplusplus
     extern "C" {
#endif

/*
*	@fn		cpu_halt
*	@brief	
*/
sint8 cpu_halt(void);
/*
*	@fn		chip_sleep
*	@brief	
*/
sint8 chip_sleep(void);
/*
*	@fn		chip_wake
*	@brief	
*/
sint8 chip_wake(void);
/*
*	@fn		chip_idle
*	@brief	
*/
void chip_idle(void);
/*
*	@fn		enable_interrupts
*	@brief	
*/
sint8 enable_interrupts(void);
/*
*	@fn		cpu_start	
*	@brief	
*/
sint8 cpu_start(void);
/*
*	@fn		nmi_get_chipid
*	@brief	
*/
uint32 nmi_get_chipid(void);
/*
*	@fn		nmi_get_rfrevid
*	@brief	
*/
uint32 nmi_get_rfrevid(void);
/*
*	@fn		restore_pmu_settings_after_global_reset
*	@brief	
*/
void restore_pmu_settings_after_global_reset(void);
/*
*	@fn		nmi_update_pll
*	@brief	
*/
void nmi_update_pll(void);
/*
*	@fn		nmi_set_sys_clk_src_to_xo
*	@brief	
*/
void nmi_set_sys_clk_src_to_xo(void);
/*
*	@fn		chip_reset
*	@brief	
*/
sint8 chip_reset(void);
/*
*	@fn		wait_for_bootrom
*	@brief	
*/
sint8 wait_for_bootrom(uint8);
/*
*	@fn		wait_for_firmware_start
*	@brief	
*/
sint8 wait_for_firmware_start(uint8);
/*
*	@fn		chip_deinit
*	@brief	
*/
sint8 chip_deinit(void);
/*
*	@fn		chip_reset_and_cpu_halt
*	@brief	
*/
sint8 chip_reset_and_cpu_halt(void);
/*
*	@fn		set_gpio_dir
*	@brief	
*/
sint8 set_gpio_dir(uint8 gpio, uint8 dir);
/*
*	@fn		set_gpio_val
*	@brief	
*/
sint8 set_gpio_val(uint8 gpio, uint8 val);
/*
*	@fn		get_gpio_val
*	@brief	
*/
sint8 get_gpio_val(uint8 gpio, uint8* val);
/*
*	@fn		pullup_ctrl
*	@brief	
*/
sint8 pullup_ctrl(uint32 pinmask, uint8 enable);
/*
*	@fn		nmi_get_otp_mac_address
*	@brief	
*/
sint8 nmi_get_otp_mac_address(uint8 *pu8MacAddr, uint8 * pu8IsValid);
/*
*	@fn		nmi_get_mac_address
*	@brief	
*/
sint8 nmi_get_mac_address(uint8 *pu8MacAddr);
/*
*	@fn		chip_apply_conf
*	@brief	
*/
sint8 chip_apply_conf(uint32 u32conf);

#ifdef __cplusplus
	 }
#endif

#endif	/*_NMASIC_H_*/
