/*
 * Copyright (c) 2024 Texas Instruments Incorporated
 * Copyright (c) 2024 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <inc/hw_ccfg.h>

#define CC23_CCFG_FLASH            DT_INST(0, ti_cc23x0_ccfg_flash)
#define CC23_CCFG_FLASH_PROP(prop) DT_PROP(CC23_CCFG_FLASH, prop)

#define CC23_TO_PERM_VAL(en) ((en) ? CCFG_PERMISSION_ALLOW : CCFG_PERMISSION_FORBID)

#if CONFIG_CC23X0_BLDR_VTOR_TYPE_UNDEF
#define CC23X0_BLDR_VTOR 0xffffffff
#elif CONFIG_CC23X0_BLDR_VTOR_TYPE_FORBID
#define CC23X0_BLDR_VTOR 0xfffffffc
#elif CONFIG_CC23X0_BLDR_VTOR_TYPE_USE_FCFG
#define CC23X0_BLDR_VTOR 0xfffffff0
#else
#define CC23X0_BLDR_VTOR CC23_CCFG_FLASH_PROP(ti_bldr_vtor_flash)
#endif

#define CC23X0_P_APP_VTOR DT_REG_ADDR(DT_CHOSEN(zephyr_code_partition))

#if CC23_CCFG_FLASH_PROP(ti_chip_erase) == 0
#warning ti,chip-erase property is NOT PRESENT in your device tree, \
	 flashing this firmware will LOCK YOUR DEVICE.
#endif

/* Default CCFG */
const ccfg_t ccfg __attribute__((section(".ti_ccfg"))) __attribute__((used)) = {
	.bootCfg.pBldrVtor = (void *)CC23X0_BLDR_VTOR,
	.bootCfg.bldrParam.serialRomBldrParamStruct.bldrEnabled =
		IS_ENABLED(CONFIG_CC23X0_BLDR_ENABLED),
	.bootCfg.bldrParam.serialRomBldrParamStruct.serialIoCfgIndex =
		CC23_CCFG_FLASH_PROP(ti_serial_io_cfg_index),
	.bootCfg.bldrParam.serialRomBldrParamStruct.pinTriggerEnabled =
		CC23_CCFG_FLASH_PROP(ti_pin_trigger),
	.bootCfg.bldrParam.serialRomBldrParamStruct.pinTriggerDio =
		CC23_CCFG_FLASH_PROP(ti_pin_trigger_dio),
	.bootCfg.bldrParam.serialRomBldrParamStruct.pinTriggerLevel =
		CC23_CCFG_FLASH_PROP(ti_pin_trigger_level_hi),
	.bootCfg.pAppVtor = (void *)CC23X0_P_APP_VTOR,

	.hwOpts = {0xffffffff, 0xffffffff},

	.permissions.allowDebugPort = CC23_TO_PERM_VAL(CC23_CCFG_FLASH_PROP(ti_debug_port)),
	.permissions.allowEnergyTrace = CC23_TO_PERM_VAL(CC23_CCFG_FLASH_PROP(ti_energy_trace)),
	.permissions.allowFlashVerify = CC23_TO_PERM_VAL(CC23_CCFG_FLASH_PROP(ti_flash_verify)),
	.permissions.allowFlashProgram = CC23_TO_PERM_VAL(CC23_CCFG_FLASH_PROP(ti_flash_program)),
	.permissions.allowChipErase = CC23_TO_PERM_VAL(CC23_CCFG_FLASH_PROP(ti_chip_erase)),
	.permissions.allowToolsClientMode = CCFG_PERMISSION_ALLOW,
	.permissions.allowReturnToFactory =
		CC23_TO_PERM_VAL(CC23_CCFG_FLASH_PROP(ti_ret_to_factory)),
	.permissions.allowFakeStby = CCFG_PERMISSION_ALLOW,

	.misc.saciTimeoutOverride = 0,
	.misc.saciTimeoutExp = XCFG_MISC_SACITOEXP_8SEC,

	.flashProt.writeEraseProt.mainSectors0_31 = CC23_CCFG_FLASH_PROP(ti_wr_er_prot_sect0_31),
	.flashProt.writeEraseProt.mainSectors32_255 =
		CC23_CCFG_FLASH_PROP(ti_wr_er_prot_sect32_255),
	.flashProt.writeEraseProt.ccfgSector = CC23_CCFG_FLASH_PROP(ti_wr_er_prot_ccfg_sect),
	.flashProt.writeEraseProt.fcfgSector = CC23_CCFG_FLASH_PROP(ti_wr_er_prot_fcfg_sect),
	.flashProt.writeEraseProt.engrSector = CC23_CCFG_FLASH_PROP(ti_wr_er_prot_engr_sect),

	.flashProt.res = 0xffffffff,

	.flashProt.chipEraseRetain.mainSectors0_31 =
		CC23_CCFG_FLASH_PROP(ti_chip_er_retain_sect0_31),
	.flashProt.chipEraseRetain.mainSectors32_255 =
		CC23_CCFG_FLASH_PROP(ti_chip_er_retain_sect32_255),

	.debugCfg.authorization = CCFG_DBGAUTH_DBGOPEN,
	.debugCfg.allowBldr = CCFG_DBGBLDR_ALLOW,
};
