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

struct lorawan_nvm_record_descr {
	const char *name;
	enum lorawan_nvm_id id;
	size_t size;
	size_t offset;
	uint16_t flag;
};

#define NVM_RECORD_DESCR(_id, _member) \
	{									\
		.id = LORAWAN_NVM_##_id,				\
		.flag = LORAMAC_NVM_NOTIFY_FLAG_##_id,			\
		.name = STRINGIFY(_member),					\
		.offset = offsetof(LoRaMacNvmData_t, _member),		\
		.size = sizeof(((LoRaMacNvmData_t *)0)->_member),		\
	}

static const struct lorawan_nvm_record_descr nvm_record_descriptors[] = {
	NVM_RECORD_DESCR(CRYPTO, Crypto),
	NVM_RECORD_DESCR(MAC_GROUP1, MacGroup1),
	NVM_RECORD_DESCR(MAC_GROUP2, MacGroup2),
	NVM_RECORD_DESCR(SECURE_ELEMENT, SecureElement),
	NVM_RECORD_DESCR(REGION_GROUP1, RegionGroup1),
	NVM_RECORD_DESCR(REGION_GROUP2, RegionGroup2),
	NVM_RECORD_DESCR(CLASS_B, ClassB),
};

static void lorawan_nvm_save_records(uint16_t nvm_notify_flag)
{
	MibRequestConfirm_t mib_req;

	LOG_DBG("Saving LoRaWAN state");

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

	for (uint32_t i = 0; i < ARRAY_SIZE(nvm_record_descriptors); i++) {
		const struct lorawan_nvm_record_descr *descr =
			&nvm_record_descriptors[i];

		if ((nvm_notify_flag & descr->flag) == descr->flag) {
			LOG_DBG("Saving configuration %s", descr->name);
			int err = lorawan_nvm_write(descr->id,
						(char *)nvm + descr->offset,
						descr->size);
			if (err != 0) {
				LOG_ERR("Could not save record %s, error %d",
					descr->name, err);
			}
		}
	}
}

void loramac_nvm_data_mgmt_event(uint16_t flags)
{
	if (flags != LORAMAC_NVM_NOTIFY_FLAG_NONE) {
		lorawan_nvm_save_records(flags);
	}
}

int lorawan_nvm_restore(void)
{
	int err;
	LoRaMacStatus_t status;
	MibRequestConfirm_t mib_req;

	LOG_DBG("Restoring LoRaWAN state");

	/* Retrieve the actual context */
	mib_req.Type = MIB_NVM_CTXS;
	if (LoRaMacMibGetRequestConfirm(&mib_req) != LORAMAC_STATUS_OK) {
		LOG_ERR("Could not get NVM context");
		return -EINVAL;
	}

	for (size_t i = 0; i < ARRAY_SIZE(nvm_record_descriptors); i++) {
		const struct lorawan_nvm_record_descr *descr = &nvm_record_descriptors[i];

		err = lorawan_nvm_read(descr->id,
				       (char *)mib_req.Param.Contexts + descr->offset, descr->size);
		if (err != 0 && err != -ENOENT) {
			LOG_ERR("Could not load record %s, error %d", descr->name, err);
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
