/*
 * Copyright (c) 2025 Texas Instruments Incorporated
 * Copyright (c) 2024 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <inc/hw_scfg.h>

#define CC27_TO_PERM_VAL(en) (IS_ENABLED(en) ? SCFG_PERMISSION_ALLOW : SCFG_PERMISSION_FORBID)

#if CONFIG_CC27XX_BLDR_VTOR_TYPE_UNDEF
#define CC27XX_BLDR_VTOR 0xffffffff
#elif CONFIG_CC27XX_BLDR_VTOR_TYPE_FORBID
#define CC27XX_BLDR_VTOR 0xfffffffc
#elif CONFIG_CC27XX_BLDR_VTOR_TYPE_USE_FCFG
#define CC27XX_BLDR_VTOR 0xfffffff0
#else
#define CC27XX_BLDR_VTOR CONFIG_CC27XX_BLDR_VTOR_FLASH
#endif

#if CONFIG_CC27XX_EM_SENSOR_ENABLED
#define CC27XX_EM_SENSOR_CONFIG SCFG_EMSENSOR_ENABLE
#elif CONFIG_CC27XX_EM_SENSOR_DISABLED
#define CC27XX_EM_SENSOR_CONFIG SCFG_EMSENSOR_DISABLE
#endif

#if CONFIG_CC27XX_POLICY_NO_AUTH
#define CC27XX_POLICY_AUTH_METHOD SCFG_POLICY_NO_AUTH
#elif CONFIG_CC27XX_POLICY_SIGNATURE
#define CC27XX_POLICY_AUTH_METHOD SCFG_POLICY_SIGNATURE
#elif CONFIG_CC27XX_POLICY_HASH_LOCK
#define CC27XX_POLICY_AUTH_METHOD SCFG_POLICY_HASH_LOCK
#endif

#if CONFIG_CC27XX_POLICY_ALG_RSA_3K_SHA256
#define CC27XX_POLICY_AUTH_ALG SCFG_POLICY_ALG_RSA_3K_SHA256
#elif CONFIG_CC27XX_POLICY_ALG_ECDSA_P256_SHA256
#define CC27XX_POLICY_AUTH_ALG SCFG_POLICY_ALG_ECDSA_P256_SHA256
#elif CONFIG_CC27XX_POLICY_ALG_ECDSA_P521_SHA512
#define CC27XX_POLICY_AUTH_ALG SCFG_POLICY_ALG_ECDSA_P521_SHA512
#endif

#if CONFIG_CC27XX_POLICY_MODE_OVRWRT
#define CC27XX_POLICY_MODE SCFG_POLICY_OVRWRT
#elif CONFIG_CC27XX_POLICY_MODE_XIPRE
#define CC27XX_POLICY_MODE SCFG_POLICY_XIP_REVERT_ENABLED
#elif CONFIG_CC27XX_POLICY_MODE_XIPRD
#define CC27XX_POLICY_MODE SCFG_POLICY_XIP_REVERT_DISABLED
#endif

/* Default SCFG */
const scfg_t scfg __attribute__((section(".ti_scfg"))) __attribute__((used)) = {

	/* hsmCfg */
	.hsmCfg.publicKeyHash = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
				 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
				 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},

	/* debugAuthCfg */
	.debugAuthCfg.challengeVector.lifetime = SCFG_DBGAUTH_EPHEMERAL_LIFETIME,
	.debugAuthCfg.challengeVector.deviceConst = SCFG_DBGAUTH_DEVICE_MAC_CONST,

	/* flashCfg */
	.flashCfg.flashLayout.primaryAppSlots[0].addr = SCFG_SLOT_ADDR_UNDEF,
	.flashCfg.flashLayout.primaryAppSlots[0].len = SCFG_SLOT_LEN_UNDEF,
	.flashCfg.flashLayout.primaryAppSlots[1].addr = SCFG_SLOT_ADDR_UNDEF,
	.flashCfg.flashLayout.primaryAppSlots[1].len = SCFG_SLOT_LEN_UNDEF,
	.flashCfg.flashLayout.secondaryAppSlots[0].addr = SCFG_SLOT_ADDR_UNDEF,
	.flashCfg.flashLayout.secondaryAppSlots[0].len = SCFG_SLOT_LEN_UNDEF,
	.flashCfg.flashLayout.secondaryAppSlots[1].addr = SCFG_SLOT_ADDR_UNDEF,
	.flashCfg.flashLayout.secondaryAppSlots[1].len = SCFG_SLOT_LEN_UNDEF,
	.flashCfg.flashLayout.bldrSlot.addr = SCFG_SLOT_ADDR_UNDEF,
	.flashCfg.flashLayout.bldrSlot.len = SCFG_SLOT_LEN_UNDEF,

	.flashCfg.res0 = {0x00000000},

	/* policyCfg */
	.secBootCfg.policyCfg.authMethod = CC27XX_POLICY_AUTH_METHOD,
	.secBootCfg.policyCfg.authAlgorithm = CC27XX_POLICY_AUTH_ALG,
	.secBootCfg.policyCfg.mode = CC27XX_POLICY_MODE,
	.secBootCfg.keyUpdateKeyHash = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},

	/* permissions */
	.permissions.allowMainAppErase = CC27_TO_PERM_VAL(CONFIG_CC27XX_ALLOW_MAIN_APP_ERASE),
	.permissions.allowDebugPort = CC27_TO_PERM_VAL(CONFIG_CC27XX_ALLOW_DEBUG_PORT),
	.permissions.allowEnergyTrace = CC27_TO_PERM_VAL(CONFIG_CC27XX_ALLOW_ENERGY_TRACE),
	.permissions.allowFlashVerify = CC27_TO_PERM_VAL(CONFIG_CC27XX_ALLOW_FLASH_VERIFY),
	.permissions.allowFlashProgram = CC27_TO_PERM_VAL(CONFIG_CC27XX_ALLOW_FLASH_PROGRAM),
	.permissions.allowChipErase = CC27_TO_PERM_VAL(CONFIG_CC27XX_ALLOW_CHIP_ERASE),
	.permissions.allowToolsClientMode = CC27_TO_PERM_VAL(CONFIG_CC27XX_ALLOW_TOOLS_CLIENT_MODE),
	.permissions.allowReturnToFactory = CC27_TO_PERM_VAL(CONFIG_CC27XX_ALLOW_RETURN_TO_FACTORY),
	.permissions.allowFakeStby = CC27_TO_PERM_VAL(CONFIG_CC27XX_ALLOW_FAKE_STANDBY),

	/* emSensorCfg */
	.emSensorCfg = CC27XX_EM_SENSOR_CONFIG,

	.bootSeedOffset = SCFG_BOOT_SEED_DISABLED,

	.keyRingCfg.keyEntries = {
		{SCFG_INVALID_KEY_ENTRY},
		{SCFG_INVALID_KEY_ENTRY},
		{SCFG_INVALID_KEY_ENTRY},
		{SCFG_INVALID_KEY_ENTRY},
		{SCFG_INVALID_KEY_ENTRY},
		{SCFG_INVALID_KEY_ENTRY},
		{SCFG_INVALID_KEY_ENTRY},
		{SCFG_INVALID_KEY_ENTRY},
		{SCFG_INVALID_KEY_ENTRY},
		{SCFG_INVALID_KEY_ENTRY},
		{SCFG_INVALID_KEY_ENTRY},
		{SCFG_INVALID_KEY_ENTRY},
		{SCFG_INVALID_KEY_ENTRY},
		{SCFG_INVALID_KEY_ENTRY},
		{SCFG_INVALID_KEY_ENTRY},
		{SCFG_INVALID_KEY_ENTRY},
		{SCFG_INVALID_KEY_ENTRY},
		{SCFG_INVALID_KEY_ENTRY},
	},
};
