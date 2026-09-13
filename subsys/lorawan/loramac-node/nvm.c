/*
 * Copyright (c) 2022 Intellinium <giuliano.franchetto@intellinium.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <LoRaMac.h>
#include <zephyr/kernel.h>
#include "nvm.h"
#include "lorawan_nvm.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(lorawan_loramac_nvm, CONFIG_LORAWAN_LOG_LEVEL);

#define LORAWAN_SETTINGS_BASE                "lorawan/nvm"

struct lorawan_nvm_setting_descr {
	const char *name;
	const char *setting_name;
	size_t size;
	size_t offset;
	uint16_t flag;
};

#define NVM_SETTING_DESCR(_flag, _member) \
	{									\
		.flag = _flag,							\
		.name = STRINGIFY(_member),					\
		.setting_name =						\
			LORAWAN_SETTINGS_BASE "/" STRINGIFY(_member),		\
		.offset = offsetof(LoRaMacNvmData_t, _member),		\
		.size = sizeof(((LoRaMacNvmData_t *)0)->_member),		\
	}

static const struct lorawan_nvm_setting_descr nvm_setting_descriptors[] = {
	NVM_SETTING_DESCR(LORAMAC_NVM_NOTIFY_FLAG_CRYPTO, Crypto),
	NVM_SETTING_DESCR(LORAMAC_NVM_NOTIFY_FLAG_MAC_GROUP1, MacGroup1),
	NVM_SETTING_DESCR(LORAMAC_NVM_NOTIFY_FLAG_MAC_GROUP2, MacGroup2),
	NVM_SETTING_DESCR(LORAMAC_NVM_NOTIFY_FLAG_SECURE_ELEMENT, SecureElement),
	NVM_SETTING_DESCR(LORAMAC_NVM_NOTIFY_FLAG_REGION_GROUP1, RegionGroup1),
	NVM_SETTING_DESCR(LORAMAC_NVM_NOTIFY_FLAG_REGION_GROUP2, RegionGroup2),
	NVM_SETTING_DESCR(LORAMAC_NVM_NOTIFY_FLAG_CLASS_B, ClassB),
};

static void lorawan_nvm_save_settings(uint16_t nvm_notify_flag)
{
	MibRequestConfirm_t mib_req;

	LOG_DBG("Saving LoRaWAN settings");

	/* Retrieve the actual context */
	mib_req.Type = MIB_NVM_CTXS;
	if (LoRaMacMibGetRequestConfirm(&mib_req) != LORAMAC_STATUS_OK) {
		LOG_ERR("Could not get NVM context");
		return;
	}

	LoRaMacNvmData_t *nvm = mib_req.Param.Contexts;

	LOG_DBG("Crypto version: %"PRIu32", DevNonce: %d, JoinNonce: %"PRIu32,
		mib_req.Param.Contexts->Crypto.LrWanVersion.Value,
		mib_req.Param.Contexts->Crypto.DevNonce,
		mib_req.Param.Contexts->Crypto.JoinNonce);

	for (uint32_t i = 0; i < ARRAY_SIZE(nvm_setting_descriptors); i++) {
		const struct lorawan_nvm_setting_descr *descr =
			&nvm_setting_descriptors[i];

		if ((nvm_notify_flag & descr->flag) == descr->flag) {
			LOG_DBG("Saving configuration %s", descr->setting_name);
			int err = lorawan_nvm_write(descr->setting_name,
						(char *)nvm + descr->offset,
						descr->size);
			if (err != 0) {
				LOG_ERR("Could not save settings %s, error %d",
					descr->name, err);
			}
		}
	}
}

void loramac_nvm_data_mgmt_event(uint16_t flags)
{
	if (flags != LORAMAC_NVM_NOTIFY_FLAG_NONE) {
		lorawan_nvm_save_settings(flags);
	}
}

int lorawan_nvm_restore(void)
{
	int err;
	LoRaMacStatus_t status;
	MibRequestConfirm_t mib_req;

	LOG_DBG("Restoring LoRaWAN settings");

	/* Retrieve the actual context */
	mib_req.Type = MIB_NVM_CTXS;
	if (LoRaMacMibGetRequestConfirm(&mib_req) != LORAMAC_STATUS_OK) {
		LOG_ERR("Could not get NVM context");
		return -EINVAL;
	}

	for (size_t i = 0; i < ARRAY_SIZE(nvm_setting_descriptors); i++) {
		const struct lorawan_nvm_setting_descr *descr = &nvm_setting_descriptors[i];

		err = lorawan_nvm_read(descr->setting_name,
				       (char *)mib_req.Param.Contexts + descr->offset, descr->size);
		if (err != 0 && err != -ENOENT) {
			LOG_ERR("Could not load setting %s, error %d", descr->name, err);
			return err;
		}
	}

	LOG_DBG("Crypto version: %"PRIu32", DevNonce: %d, JoinNonce: %"PRIu32,
		mib_req.Param.Contexts->Crypto.LrWanVersion.Value,
		mib_req.Param.Contexts->Crypto.DevNonce,
		mib_req.Param.Contexts->Crypto.JoinNonce);

	mib_req.Type = MIB_NVM_CTXS;
	status = LoRaMacMibSetRequestConfirm(&mib_req);
	if (status != LORAMAC_STATUS_OK) {
		LOG_ERR("Could not set the NVM context, error %d", status);
		return -EINVAL;
	}

	LOG_DBG("LoRaWAN context restored");

	return 0;
}
