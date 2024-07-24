/*
 * Copyright (c) 2024 Texas Instruments Incorporated
 * Copyright (c) 2024 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <inc/hw_ccfg.h>

#define CC23_TO_PERM_VAL(en) (IS_ENABLED(en) ? CCFG_PERMISSION_ALLOW : CCFG_PERMISSION_FORBID)

#if CONFIG_CC23X0_BLDR_VTOR_TYPE_UNDEF
#define CC23X0_BLDR_VTOR 0xffffffff
#elif CONFIG_CC23X0_BLDR_VTOR_TYPE_FORBID
#define CC23X0_BLDR_VTOR 0xfffffffc
#elif CONFIG_CC23X0_BLDR_VTOR_TYPE_USE_FCFG
#define CC23X0_BLDR_VTOR 0xfffffff0
#else
#define CC23X0_BLDR_VTOR CONFIG_CC23X0_BLDR_VTOR_FLASH
#endif

/* Default CCFG */
const ccfg_t ccfg __attribute__((section(".ti_ccfg"))) __attribute__((used)) = {

	.bootCfg.pBldrVtor = (void *)CC23X0_BLDR_VTOR,

	.bootCfg.bldrParam.serialRomBldrParamStruct.bldrEnabled =
		IS_ENABLED(CONFIG_CC23X0_BLDR_ENABLED),
	.bootCfg.bldrParam.serialRomBldrParamStruct.serialIoCfgIndex =
		CONFIG_CC23X0_SERIAL_IO_CFG_INDEX,
	.bootCfg.bldrParam.serialRomBldrParamStruct.pinTriggerEnabled =
		IS_ENABLED(CONFIG_CC23X0_PIN_TRIGGER_ENABLED),
	.bootCfg.bldrParam.serialRomBldrParamStruct.pinTriggerDio = CONFIG_CC23X0_PIN_TRIGGER_DIO,
	.bootCfg.bldrParam.serialRomBldrParamStruct.pinTriggerLevel =
		IS_ENABLED(CONFIG_CC23X0_PIN_TRIGGER_LEVEL_HI),
	.bootCfg.pAppVtor = (void *)CONFIG_CC23X0_P_APP_VTOR,

	.hwOpts = {CONFIG_CC23X0_HW_OPTS_1, CONFIG_CC23X0_HW_OPTS_2},

	.permissions.allowDebugPort = CC23_TO_PERM_VAL(CONFIG_CC23X0_ALLOW_DEBUG_PORT),
	.permissions.allowEnergyTrace = CC23_TO_PERM_VAL(CONFIG_CC23X0_ALLOW_ENERGY_TRACE),
	.permissions.allowFlashVerify = CC23_TO_PERM_VAL(CONFIG_CC23X0_ALLOW_FLASH_VERIFY),
	.permissions.allowFlashProgram = CC23_TO_PERM_VAL(CONFIG_CC23X0_ALLOW_FLASH_PROGRAM),
	.permissions.allowChipErase = CC23_TO_PERM_VAL(CONFIG_CC23X0_ALLOW_CHIP_ERASE),
	.permissions.allowToolsClientMode = CC23_TO_PERM_VAL(CONFIG_CC23X0_ALLOW_TOOLS_CLIENT_MODE),
	.permissions.allowReturnToFactory = CC23_TO_PERM_VAL(CONFIG_CC23X0_ALLOW_RETURN_TO_FACTORY),
	.permissions.allowFakeStby = CCFG_PERMISSION_ALLOW,

	.misc.saciTimeoutOverride = IS_ENABLED(CONFIG_CC23X0_SACI_TIMEOUT_OVERRIDE),
	.misc.saciTimeoutExp = CONFIG_CC23X0_SACI_TIMEOUT_EXP,

	.flashProt.writeEraseProt.mainSectors0_31 = CONFIG_CC23X0_WR_ER_PROT_SECT0_31,
	.flashProt.writeEraseProt.mainSectors32_255 = CONFIG_CC23X0_WR_ER_PROT_SECT32_255,

	.flashProt.writeEraseProt.ccfgSector = CONFIG_CC23X0_WR_ER_PROT_CCFG_SECT,
	.flashProt.writeEraseProt.fcfgSector = CONFIG_CC23X0_WR_ER_PROT_FCFG_SECT,
	.flashProt.writeEraseProt.engrSector = CONFIG_CC23X0_WR_ER_PROT_ENGR_SECT,

	.flashProt.res = 0xffffffff,

	.flashProt.chipEraseRetain.mainSectors0_31 = CONFIG_CC23X0_CHIP_ER_RETAIN_SECT0_31,
	.flashProt.chipEraseRetain.mainSectors32_255 = CONFIG_CC23X0_CHIP_ER_RETAIN_SECT32_255,

	.debugCfg.authorization = CCFG_DBGAUTH_DBGOPEN,
	.debugCfg.allowBldr = CCFG_DBGBLDR_FORBID,
};
