/*
 * Copyright (c) 2021 Intellinium <giuliano.franchetto@intellinium.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <LoRaMac.h>
#include <settings/settings.h>
#include "lorawan_nvm.h"
#include <logging/log.h>
#if defined(CONFIG_LORAWAN_SE_HAS_OWN_NVM)
#include "../se/lorawan_se.h"
#endif

LOG_MODULE_REGISTER(lorawan_nvm, CONFIG_LORAWAN_LOG_LEVEL);

static uint16_t nvm_notify_flag;

#define LORAWAN_SETTINGS_BASE                "lorawan/nvm"

#define flag_set(f)        ((nvm_notify_flag & (f)) == (f))

#define SAVE_SETTINGS(nvm, flag, name)					\
if (flag_set(flag)) {							\
	LOG_DBG("Saving configuration " LORAWAN_SETTINGS_BASE "/" # name);\
	int err = settings_save_one(LORAWAN_SETTINGS_BASE "/" # name,	\
				&(nvm)->name,				\
				sizeof((nvm)->name));			\
	if (err) {							\
		LOG_ERR("Could not save settings" # name", error %d",	\
			err);						\
	}								\
}

static void nvm_settings_work(struct k_work *work)
{
	ARG_UNUSED(work);

	MibRequestConfirm_t mibReq;

	LOG_DBG("Saving LoRaWAN settings");

	/* Retrieve the actual context */
	mibReq.Type = MIB_NVM_CTXS;
	if (LoRaMacMibGetRequestConfirm(&mibReq) != LORAMAC_STATUS_OK) {
		LOG_ERR("Could not get NVM context");
		return;
	}

	LoRaMacNvmData_t *nvm = mibReq.Param.Contexts;

	LOG_DBG("Crypto version: %"PRIu32", DevNonce: %d, JoinNonce: %"PRIu32,
		mibReq.Param.Contexts->Crypto.LrWanVersion.Value,
		mibReq.Param.Contexts->Crypto.DevNonce,
		mibReq.Param.Contexts->Crypto.JoinNonce);

	/* Wait for LoRa to be idle */
	while (LoRaMacStop() != LORAMAC_STATUS_OK) {
		k_sleep(K_MSEC(100));
	}

	SAVE_SETTINGS(nvm, LORAMAC_NVM_NOTIFY_FLAG_CRYPTO, Crypto)
	SAVE_SETTINGS(nvm, LORAMAC_NVM_NOTIFY_FLAG_MAC_GROUP1, MacGroup1)
	SAVE_SETTINGS(nvm, LORAMAC_NVM_NOTIFY_FLAG_MAC_GROUP2, MacGroup2)
#if !defined(CONFIG_LORAWAN_SE_HAS_OWN_NVM)
	SAVE_SETTINGS(nvm, LORAMAC_NVM_NOTIFY_FLAG_SECURE_ELEMENT,
		      SecureElement)
#else
	lorawan_se_update(&nvm->SecureElement);
#endif
	SAVE_SETTINGS(nvm, LORAMAC_NVM_NOTIFY_FLAG_REGION_GROUP1, RegionGroup1)
	SAVE_SETTINGS(nvm, LORAMAC_NVM_NOTIFY_FLAG_REGION_GROUP2, RegionGroup2)
	SAVE_SETTINGS(nvm, LORAMAC_NVM_NOTIFY_FLAG_CLASS_B, ClassB)

	settings_save();
	/* Clean notification flag */
	nvm_notify_flag = LORAMAC_NVM_NOTIFY_FLAG_NONE;

	/* Restart LoRaMAC */
	LoRaMacStart();
}
K_WORK_DEFINE(nvm_settings_worker, nvm_settings_work);

void lorawan_nvm_data_mgmt_event(uint16_t notifyFlags)
{
	LOG_WRN("New NVM notify flag: 0x%04X", notifyFlags);

	nvm_notify_flag = notifyFlags;

	if (notifyFlags != LORAMAC_NVM_NOTIFY_FLAG_NONE) {
		k_work_submit(&nvm_settings_worker);
	}
}

static int load_setting(void *tgt, size_t tgt_size,
			const char *key, size_t len,
			settings_read_cb read_cb, void *cb_arg)
{
	if (len != tgt_size) {
		LOG_ERR("Can't load '%s' state, size mismatch.",
			log_strdup(key));
		return -EINVAL;
	}

	if (!tgt) {
		LOG_ERR("Can't load '%s' state, no target.",
			log_strdup(key));
		return -EINVAL;
	}

	if (read_cb(cb_arg, tgt, len) != len) {
		LOG_ERR("Can't load '%s' state, short read.",
			log_strdup(key));
		return -EINVAL;
	}

	return 0;
}

/* @formatter:off */
#define LOAD_SETTINGS(nvm, name)					\
({									\
int $err = -EAGAIN;							\
if (strcmp(#name, key) == 0) {					\
	$err = load_setting(&(nvm)->name,				\
			   sizeof((nvm)->name),			\
			   key, len, read_cb, cb_arg);		\
	if ($err) {							\
		LOG_ERR("Could not read settings" # name);		\
	}								\
	$err = 0;							\
}									\
$err;									\
})
/* @formatter: on */

static int on_setting_loaded(const char *key, size_t len,
			   settings_read_cb read_cb,
			   void *cb_arg, void *param)
{
	int err;
	LoRaMacNvmData_t *nvm = param;

	LOG_DBG("Key: %s", log_strdup(key));

	err = LOAD_SETTINGS(nvm, Crypto);
	if (err != -EAGAIN) {
		return err;
	}

	err = LOAD_SETTINGS(nvm, MacGroup1);
	if (err != -EAGAIN) {
		return err;
	}

	err = LOAD_SETTINGS(nvm, MacGroup2);
	if (err != -EAGAIN) {
		return err;
	}

#if !defined(CONFIG_LORAWAN_SE_HAS_OWN_NVM)
	err = LOAD_SETTINGS(nvm, SecureElement);
	if (err != -EAGAIN) {
		return err;
	}
#endif

	err = LOAD_SETTINGS(nvm, RegionGroup1);
	if (err != -EAGAIN) {
		return err;
	}

	err = LOAD_SETTINGS(nvm, RegionGroup2);
	if (err != -EAGAIN) {
		return err;
	}

	err = LOAD_SETTINGS(nvm, ClassB);
	if (err != -EAGAIN) {
		return err;
	}

	LOG_WRN("Unknown LoRaWAN setting: %s", log_strdup(key));
	return 0;
}

int lorawan_nvm_data_restore(void)
{
	int err;
	LoRaMacStatus_t status;
	MibRequestConfirm_t mibReq;

	/* Retrieve the actual context */
	mibReq.Type = MIB_NVM_CTXS;
	if (LoRaMacMibGetRequestConfirm(&mibReq) != LORAMAC_STATUS_OK) {
		LOG_ERR("Could not get NVM context");
		return -EINVAL;
	}

	err = settings_load_subtree_direct(LORAWAN_SETTINGS_BASE,
					   on_setting_loaded,
					   mibReq.Param.Contexts);
	if (err) {
		LOG_ERR("Could not load LoRaWAN settings, error %d", err);
		return err;
	}

	LOG_DBG("LoRaWAN context restored");
	LOG_DBG("Crypto version: %"PRIu32", DevNonce: %d, JoinNonce: %"PRIu32,
		mibReq.Param.Contexts->Crypto.LrWanVersion.Value,
		mibReq.Param.Contexts->Crypto.DevNonce,
		mibReq.Param.Contexts->Crypto.JoinNonce);

	mibReq.Type = MIB_NVM_CTXS;
	status = LoRaMacMibSetRequestConfirm(&mibReq);
	if (status != LORAMAC_STATUS_OK) {
		LOG_ERR("Could not set the NVM context, error %d", status);
		return -EINVAL;
	}

	return 0;
}
